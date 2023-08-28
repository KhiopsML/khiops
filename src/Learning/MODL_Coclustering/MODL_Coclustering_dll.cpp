// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#define KHIOPS_COCLUSTERING_EXPORTS
#include "MODL_Coclustering_dll.h"

// Tous les include se font directement dans le source, pour garder un header C
// independant des librairies Norm et Learning
#include "Standard.h"
#include "CCLearningProject.h"
#include "KWKhiopsVersion.h"
#include "FileService.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation des methodes de l'API

KHIOPS_COCLUSTERING_API int GetVersion()
{
	return 10 * GetMajorVersion(KHIOPS_VERSION) + GetMinorVersion(KHIOPS_VERSION);
}

KHIOPS_COCLUSTERING_API const char* GetFullVersion()
{
	static ALString sFullVersion;

	// Initialisation la premiere fois
	if (sFullVersion == "")
		sFullVersion = KHIOPS_VERSION;
	return sFullVersion;
}

inline char* _strdup(const char* sChar)
{
	longint len;
	char* sRet;

	len = strlen(sChar);
	sRet = new char[len + 1];
	strcpy(sRet, sChar);
	return sRet;
}

KHIOPS_COCLUSTERING_API int StartKhiopsCoclustering(const char* sInputScenario, const char* sLogFileName,
						    const char* sTaskFileName)
{
	int argc;
	char** argv;
	int i;
	Timestamp tsNow;
	Timestamp tsLimit;

	// Mise en place d'une date de peremption
	tsNow.SetCurrentTimestamp();
	tsLimit.Init(2020, 1, 1, 0, 0, 0);
	if (tsNow.Compare(tsLimit) == 1)
	{
		printf("License not valid");
		return EXIT_FAILURE;
	}

	if (sInputScenario == NULL or sLogFileName == NULL or sTaskFileName == NULL)
		return EXIT_FAILURE;
	if (not FileService::FileExists(sInputScenario) or sLogFileName[0] == '\0')
		return EXIT_FAILURE;

	argc = 6;
	if (sTaskFileName[0] != '\0')
		argc = 8;

	// Parametrage de la ligne de commande
	// khiops_colcustering -b -i sInputScenario -e sLogFileName -t sTaskFileName
	argv = new char*[argc];
	argv[0] = _strdup("khiops_colcustering");
	argv[1] = _strdup("-b");
	argv[2] = _strdup("-i");
	argv[3] = _strdup(sInputScenario);
	argv[4] = _strdup("-e");
	argv[5] = _strdup(sLogFileName);
	if (sTaskFileName[0] != '\0')
	{
		argv[6] = _strdup("-t");
		argv[7] = _strdup(sTaskFileName);
	}

	// Lancement du projet
	CCLearningProject learningProject;
	learningProject.Start(argc, argv);

	// Nettoyage
	for (i = 0; i < argc; i++)
		delete[] argv[i];
	delete[] argv;
	return EXIT_SUCCESS;
}