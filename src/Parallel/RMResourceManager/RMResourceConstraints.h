// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "Object.h"

enum Resource
{
	MEMORY,
	DISK,
	RESOURCES_NUMBER
};

// Affichage human readable du type de ressource
ALString ResourceToString(int nResource);

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
	static void SetMaxCoreNumberOnCluster(int nCoreNumber);
	static int GetMaxCoreNumberOnCluster();

	// Nombre maximum de coeurs utilisable sur chaque machine (par defaut a l'infini)
	static void SetMaxCoreNumberPerHost(int nCoreNumber);
	static int GetMaxCoreNumberPerHost();

	// Parametrage du temps utilisable en secondes (0 si pas de contrainte)
	static void SetOptimizationTime(int nValue);
	static int GetOptimizationTime();

	// Memoire physique maximale utilisee par chaque machine en MB (par defaut a l'infini)
	static void SetMemoryLimit(int nMemory);
	static int GetMemoryLimit();

	// Parametrage de l'espace disque utilisable par chaque machine en MB (par defaut a l'infini)
	static void SetDiskLimit(int nValue);
	static int GetDiskLimit();

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
	static boolean bIgnoreMemoryLimit;
	static ALString sUserTmpDir;
};