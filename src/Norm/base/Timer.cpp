// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Timer.h"

Timer::Timer()
{
	Reset();
}

Timer::~Timer() {}

// Implementation avec chrono
#ifdef __C11__
void Timer::Reset()
{
	bIsStarted = false;
	lStartNumber = 0;
	dElapsedNanoTime = 0;
}

double Timer::GetAbsoluteTime()
{
	double dAbsoluteTime;
	std::chrono::system_clock::time_point tNow;

	// Acces au wall-clock time
	tNow = std::chrono::system_clock::now();

	// Conversion en secondes, a la plus grande precision possible
	auto now_ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(tNow);
	auto value = now_ns.time_since_epoch();
	dAbsoluteTime = value.count() / 1e9;
	return dAbsoluteTime;
}

// Sinon, implementation avec time_t
#else
void Timer::Reset()
{
	bIsStarted = false;
	lStartNumber = 0;
	tLastStartTime = 0;
	tElapsedTime = 0;
}

double Timer::GetAbsoluteTime()
{
	double dAbsoluteTime;
	time_t tTime;
	struct timeval tv;
	int nRet;

	// Acces au wall-clock time
	nRet = gettimeofday(&tv, NULL);
	assert(nRet == 0);

	// Conversion en secondes, a la plus grande precision possible
	dAbsoluteTime = tv.tv_sec + tv.tv_usec / 1e-6;
	return dAbsoluteTime;
}
#endif //__C11__

longint Timer::GetUsedMemory() const
{
	return sizeof(Timer);
}

void Timer::Test()
{
	Timer testTimer;
	double dAbsoluteTime;
	int i;
	int j;
	double dTotal = 0;
	double dTime;
	int n;
	int nLoopSize = 100000000;
	int iMax = 5;
	debug(iMax = 3);

	// Acces a un date de refrence absolue
	dAbsoluteTime = testTimer.GetAbsoluteTime();
	cout << "SYS TIME\tAbsolute time\t" << dAbsoluteTime << endl;

	// Test de base
	cout << "SYS TIME\tTimer (short)\t" << flush;
	testTimer.Reset();
	for (i = 0; i < 10; i++)
	{
		testTimer.Start();
		for (j = 0; j < nLoopSize; j++)
			dTotal++;
		testTimer.Stop();
		dTime = testTimer.GetElapsedTime();
		cout << " " << dTime << flush;
	}
	cout << endl;

	// Test de longue periode
	cout << "SYS TIME\tTimer (long)\t" << flush;
	testTimer.Reset();
	for (i = 0; i < iMax; i++)
	{
		dTotal = 0;
		testTimer.Start();
		for (n = 0; n < pow(2.0, i * 1.0); n++)
		{
			for (j = 0; j < nLoopSize; j++)
				dTotal++;
		}
		testTimer.Stop();
		dTime = testTimer.GetElapsedTime();
		cout << " " << dTime << flush;
	}
	cout << endl;

	// Instruction utilisee pour empecher le compilateur optimiseur de supprimer les boucles de calcul
	if (dTotal < 0)
		cout << flush;

	// Test de precision de SystemSleep et des timer
	testTimer.Reset();
	testTimer.Start();
	SystemSleep(1.0);
	testTimer.Stop();
	cout << "SYS TIME\tSystemSleep(1) -> " << DoubleToString(testTimer.GetElapsedTime()) << " s" << endl;

	testTimer.Reset();
	testTimer.Start();
	SystemSleep(0.25);
	testTimer.Stop();
	cout << "SYS TIME\tSystemSleep(0.25) -> " << DoubleToString(testTimer.GetElapsedTime()) << " s" << endl;

	testTimer.Reset();
	testTimer.Start();
	SystemSleep(0.01);
	testTimer.Stop();
	cout << "SYS TIME\tSystemSleep(0.01) -> " << DoubleToString(testTimer.GetElapsedTime()) << " s" << endl;

	testTimer.Reset();
	testTimer.Start();
	SystemSleep(0.001);
	testTimer.Stop();
	cout << "SYS TIME\tSystemSleep(0.001) -> " << DoubleToString(testTimer.GetElapsedTime()) << " s" << endl;

	testTimer.Reset();
	testTimer.Start();
	SystemSleep(0.0001);
	testTimer.Stop();
	cout << "SYS TIME\tSystemSleep(0.0001) -> " << DoubleToString(testTimer.GetElapsedTime()) << " s" << endl;

	testTimer.Reset();
	testTimer.Start();
	SystemSleep(0.00001);
	testTimer.Stop();
	cout << "SYS TIME\tSystemSleep(0.00001) -> " << DoubleToString(testTimer.GetElapsedTime()) << " s" << endl;
}
