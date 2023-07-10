// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Timer.h"
#include "FileService.h"

class ProfileStats;

//////////////////////////////////////////////////////////
// Classe ProfileStats
// Collecte de statistiques pour auditer les performances
// Pour une portion de code que l'on identifie par un libelle,
// on appele un Start et un Stop en debut et fin d'execution.
// On a ensuite acces au nombre total d'appel et au
// temps d'execution total
class ProfileStats : public Object
{
public:
	// Constructeur
	ProfileStats();
	~ProfileStats();

	// Nom de la portion de code auditee (nom de methode par exemple)
	void SetLabel(const ALString& sValue);
	const ALString& GetLabel() const;

	// Remise a zero et en etat initial
	void Reset();

	// Depart/arret de la collecte des stats
	void Start();
	void Stop();
	boolean IsStarted() const;

	// Renvoie le nombre d'appels effectues
	int GetCallNumber() const;

	// Renvoie le temp ecoule total en secondes depuis le dernier Reset
	// en cumulant l'ensemble des plages entre Start et Stop, et
	// l'eventuel temps courant depuis le dernier Start
	double GetElapsedTime() const;

	// Rapport synthetique destine a rentrer dans un tableau
	// Ces methodes ne creent pas retour charriot ('\n') en fin de ligne
	void WriteHeaderLineReport(ostream& ost);
	void WriteLineReport(ostream& ost);

	// Rapport synthetique pour un tableau de stats, avec une colonne de contexte additionnelle
	void WriteArrayLineReportFile(const ALString& sFileName, const ALString& sContextLabel,
				      ObjectArray* oaProfileStats);
	void WriteArrayLineReport(ostream& ost, const ALString& sContextLabel, ObjectArray* oaProfileStats);

	// Remise a zero de toutes les stats d'un tableau
	void ResetAll(ObjectArray* oaProfileStats);

	///// Implementation
protected:
	ALString sLabel;
	int nCallNumber;
	Timer timer;

	// Niveau d'empilement de la gestion des starts
	int nStartLevel;
};

// Methodes en inline

inline void ProfileStats::Start()
{
	require(nStartLevel >= 0);
	if (nStartLevel == 0)
		timer.Start();
	nCallNumber++;
	nStartLevel++;
}

inline void ProfileStats::Stop()
{
	require(nStartLevel > 0);
	nStartLevel--;
	if (nStartLevel == 0)
		timer.Stop();
}

inline boolean ProfileStats::IsStarted() const
{
	return timer.IsStarted();
}

inline int ProfileStats::GetCallNumber() const
{
	return nCallNumber;
}

inline double ProfileStats::GetElapsedTime() const
{
	return timer.GetElapsedTime();
}
