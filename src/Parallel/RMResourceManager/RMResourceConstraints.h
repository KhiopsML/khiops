// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "Object.h"

enum Resource
{
	MEMORY,
	DISK,
	UNKNOWN
};

// Affichage human readable du type de resource
ALString ResourceToString(int nRessource);

/////////////////////////////////////////////////////////////////////////////
// Classe RMResourceConstraints
// Classe qui contient les exigences globales de l'utilisateur sur le systeme
// Ces contraintes sont prises en compte par RMResourceManager et RMParallelResourceManager pour le calcul
// des ressources disponibles.
class RMResourceConstraints : public Object
{
public:
	// Constructeur
	RMResourceConstraints();
	~RMResourceConstraints();

	// Nombre maximum de coeurs utilisable sur l'ensemble du systeme (par defaut a l'infini)
	static void SetMaxCoreNumber(int nCoreNumber);
	static int GetMaxCoreNumber();

	// Nombre maximum de coeurs utilisable sur chaque machine (par defaut a l'infini)
	static void SetMaxCoreNumberPerHost(int nCoreNumber);
	static int GetMaxCoreNumberPerHost();

	// Nombre maximum de processus sur le systeme (par defaut a l'infini)
	static void SetMaxProcessNumber(int nProcessNumber);
	static int GetMaxProcessNumber();

	// Parametrage du temps utilisable en secondes (0 si pas de contrainte)
	static void SetOptimizationTime(int nValue);
	static int GetOptimizationTime();

	// Memoire physique maximale utilisee par chaque machine en MB (par defaut a l'infini)
	static void SetMemoryLimit(int nMemory);
	static int GetMemoryLimit();

	// Memoire physique maximale utilisee par chaque processus en MB (par defaut a l'infini)
	static void SetMemoryLimitPerProc(int nMemory);
	static int GetMemoryLimitPerProc();

	// Parametrage de l'espace disque utilisable par chaque machine en MB (par defaut a l'infini)
	static void SetDiskLimit(int nValue);
	static int GetDiskLimit();

	// Parametrage de l'espace disque utilisable par chaque processus en MB (par defaut a l'infini)
	static void SetDiskLimitPerProc(int nValue);
	static int GetDiskLimitPerProc();

	// Indicateur pour ignorer la gestion preventive des probleme memoire (aux risque de planter le programme!)
	// (par defaut: false)
	static void SetIgnoreMemoryLimit(boolean bValue);
	static boolean GetIgnoreMemoryLimit();

	// Repertoire "utilisateur" des fichiers temporaires
	static void SetUserTmpDir(const ALString& sValue);
	static const ALString& GetUserTmpDir();

	// Parametrage generique des ressources physiques
	static void SetResourceLimit(int nResourceType, int nValue);
	static int GetResourceLimit(int nResourceType);
	static void SetResourceLimitPerProc(int nResourceType, int nValue);
	static int GetResourceLimitPerProc(int nResourceType);

	// Affichage des contraintes
	static ALString ToString();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs
	static int nOptimizationTime;
	static int nMaxCoreNumber;
	static int nMaxCoreNumberOnHost;
	static int nMaxProcessNumber;

	static int nResourceLimit[2];
	static int nResourceLimitPerProc[2];

	static boolean bIgnoreMemoryLimit;
	static ALString sUserTmpDir;
};
