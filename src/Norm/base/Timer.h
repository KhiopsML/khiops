// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"

// Utilisation de la bibliotheque chrono uniquement a partir de C++ 11
#if defined __UNIX__ and not defined __C11__
#undef __CHRONO__
#else
// Chrono est toujous disponible sous Windows
#include <chrono>
#define __CHRONO__
#endif

class Timer;
class PeriodicTest;

//////////////////////////////////////////////////////////
// Classe Timer
// Mesure du temps ecoule pour auditer les performances
class Timer : public Object
{
public:
	// Constructeur
	Timer();
	~Timer();

	// Remise a zero et en etat initial
	void Reset();

	// Depart/arret de la comptabilisation du temps ecoule
	void Start();
	void Stop();
	boolean IsStarted() const;

	// Nombre de start effectues depuis le dernier reset
	longint GetStartNumber() const;

	// Renvoie le temp ecoule total en secondes depuis le dernier Reset
	// en cumulant l'ensemble des plages entre Start et Stop, et
	// l'eventuel temps courant depuis le dernier Start
	// La precision est de l'ordre du millioneme de seconde
	double GetElapsedTime() const;

	// Temps moyen passe entre un start et un stop, depuis le dernier reset
	double GetMeanElapsedTime() const;

	//////////////////////////////////////////////
	// Methodes avancees

	// Renvoie un temps absolu en secondes, a la plus grande precsion possible
	// En general, il s'agit de la duree ecoulee depuis une date de reference (e.g. 01/01/1970)
	static double GetAbsoluteTime();

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Test
	static void Test();

	/////////////////////////////////////////////////////
	///// Implementation
protected:
	// Variables d'instance
	boolean bIsStarted;
	longint lStartNumber;

	// Implementation preferentielle avec chrono, qui est tres precis, mais disponible uniquement en C++ 11
#ifdef __CHRONO__
	// Doc sur https://www.modernescpp.com/index.php/the-three-clocks
	std::chrono::steady_clock::time_point tLastStartNanoTime;
	double dElapsedNanoTime;
	// Sinon, implementation avec time_t, sous Unix, si chrono non disponible
#else
	time_t tLastStartTime;
	time_t tElapsedTime;
#endif // __CHRONO__
};

//////////////////////////////////////////////////////////
// Classe PeriodicTest
// Utilitaire pour effectuer des test selon une frequence d'evenement et de temps controlee
class PeriodicTest : public Object
{
public:
	// Constructeur
	PeriodicTest();
	~PeriodicTest();

	// Indique si on peut effectuer un test, pour un nombre d'evenement donne
	// Les tests sont effectues au plus selon une periodicite en nombre d'evenements et en secondes
	boolean IsTestAllowed(longint lEventNumber) const;

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Timer pour gerer la periodicite
	mutable Timer timer;
};

//////////////////////////////////
// Implementation inline de Timer

inline boolean Timer::IsStarted() const
{
	return bIsStarted;
}

inline longint Timer::GetStartNumber() const
{
	return lStartNumber;
}

inline double Timer::GetMeanElapsedTime() const
{
	if (lStartNumber == 0)
		return 0;
	else
		return GetElapsedTime() / lStartNumber;
}

// Implementation avec chrono
#ifdef __CHRONO__

inline void Timer::Start()
{
	require(not bIsStarted);
	bIsStarted = true;
	lStartNumber++;
	tLastStartNanoTime = std::chrono::steady_clock::now();
}

inline void Timer::Stop()
{
	require(bIsStarted);

	dElapsedNanoTime +=
	    std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - tLastStartNanoTime)
		.count();
	assert(dElapsedNanoTime >= 0);
	bIsStarted = false;
}

inline double Timer::GetElapsedTime() const
{
	double dTotalElapsedNanoTime;

	// Prise en compte si necessaire du temps ecoule additionnel depuis le dernier Start
	if (bIsStarted)
	{
		dTotalElapsedNanoTime = dElapsedNanoTime;
		dTotalElapsedNanoTime += std::chrono::duration_cast<std::chrono::nanoseconds>(
					     std::chrono::steady_clock::now() - tLastStartNanoTime)
					     .count();
		return dTotalElapsedNanoTime * 1.e-9;
	}
	// Sinon, on renvoie le temps ecoule depuis le dernier stop
	else
		return dElapsedNanoTime * 1.e-9;
}
// Sinon, implementation avec time_t
#else

inline void Timer::Start()
{
	int bRet;
	struct timeval tv;
	require(not bIsStarted);
	bRet = gettimeofday(&tv, NULL);
	assert(bRet == 0);
	tLastStartTime = tv.tv_sec * 1e6 + tv.tv_usec;
	bIsStarted = true;
	lStartNumber++;
}

inline void Timer::Stop()
{
	time_t tTime;
	struct timeval tv;
	int nRet;

	require(bIsStarted);

	nRet = gettimeofday(&tv, NULL);
	assert(nRet == 0);
	tTime = tv.tv_sec * 1e6 + tv.tv_usec;
	tElapsedTime += tTime - tLastStartTime;
	assert(tElapsedTime >= 0);
	bIsStarted = false;
}

inline double Timer::GetElapsedTime() const
{
	time_t tTotalElapsedTime;
	time_t tTime;
	struct timeval tv;
	int nRetCode;

	// Temps total ecoule dans les plage Start/Stop du timer
	tTotalElapsedTime = tElapsedTime;

	// Prise en compte si necessaire du temps ecoule additionnel depuis le dernier Start
	if (bIsStarted)
	{
		tTotalElapsedTime = tElapsedTime;
		nRetCode = gettimeofday(&tv, NULL);
		assert(nRetCode == 0);
		tTime = tv.tv_sec * 1e6 + tv.tv_usec;
		tTotalElapsedTime += (tTime - tLastStartTime);
		return ((double)tTotalElapsedTime) / 1e6;
	}
	// Sinon, on renvoie le temps ecoule depuis le dernier stop
	else
		return ((double)tElapsedTime) / 1e6;
}
#endif // __CHRONO__

/////////////////////////////////////////
// Implementation inline de PeriodicTest

inline PeriodicTest::PeriodicTest()
{
	timer.Start();
}

inline PeriodicTest::~PeriodicTest()
{
	timer.Stop();
}

inline boolean PeriodicTest::IsTestAllowed(longint lEventNumber) const
{
	if (lEventNumber % 128 == 0 and timer.GetElapsedTime() > 0.25)
	{
		// On reinitialise le timer
		timer.Reset();
		timer.Start();
		return true;
	}
	else
		return false;
}
