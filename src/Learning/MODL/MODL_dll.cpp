// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#ifdef __ANDROID__
#define KHIOPS_EXPORTS
#include "MODL_dll.h"

// Tous les include se font directement dans le source, pour garder un header C
// independant des librairies Norm et Learning
#include "Standard.h"
#include "MDKhiopsLearningProject.h"
#include "KWKhiopsVersion.h"
#include "FileService.h"
#include "DTDecisionTreeCreationTask.h"
#include "KWTimestamp.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation des methodes de l'API

KHIOPS_API int GetVersion()
{
	return KHIOPS_API_VERSION_10_0;
}

char* _strdup(const char* sChar)
{
	int len;
	char* sRet;

	len = strlen(sChar);
	sRet = new char[len + 1];
	strcpy(sRet, sChar);
	return sRet;
}

KHIOPS_API int StartKhiops(const char* sInputScenario, const char* sLogFileName, const char* sTaskFileName)
{
	int argc;
	char** argv;
	int i;
	Date currentDate;
	Date expirationDate;
	int nDaysBeforeExpiration;

	// Mise en place d'une date de peremption
	expirationDate.Init(2023, 6, 30);
	currentDate.SetCurrentDate();
	nDaysBeforeExpiration = expirationDate.Diff(currentDate);
	if (nDaysBeforeExpiration < 0)
	{
		printf("License not valid");
		return EXIT_FAILURE;
	}

	if (sInputScenario == NULL or sLogFileName == NULL or sTaskFileName == NULL)
		return EXIT_FAILURE;
	if (not FileService::IsFile(sInputScenario) or sLogFileName[0] == '\0')
		return EXIT_FAILURE;

	argc = 6;
	if (sTaskFileName[0] != '\0')
		argc = 8;

	// Parametrage de la ligne de commande
	// khiops -b -i sInputScenario -e sLogFileName -t sTaskFileName
	argv = new char*[argc];
	argv[0] = _strdup("khiops");
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

	// Creation d'attribut de type arbre, disponible en V9
	KDDataPreparationAttributeCreationTask::SetGlobalCreationTask(new DTDecisionTreeCreationTask);

	// Lancement du projet
	MDKhiopsLearningProject learningProject;
	learningProject.Start(argc, argv);

	// Nettoyage
	for (i = 0; i < argc; i++)
		delete[] argv[i];
	delete[] argv;
	return EXIT_SUCCESS;
}

/********************************************************************
 * Le source suivant permet de compiler des sources developpes avec *
 * l'environnement Norm, d'utiliser le mode UIObject::Textual et    *
 * de ne pas linker avec jvm.lib (a eviter absoluement).            *
 * Moyennant ces conditions, on peut livrer un executable en mode   *
 * textuel ne necessitant pas l'intallation prealable du JRE Java   *
 ********************************************************************/

extern "C"
{
#ifdef _MSC_VER
	// Version 32 bits
	int __stdcall _imp__JNI_CreateJavaVM(void** pvm, void** penv, void* args)
	{
		exit(11);
	}

	// Version 64 bits
	int __stdcall __imp_JNI_CreateJavaVM(void** pvm, void** penv, void* args)
	{
		exit(11);
	}
#endif // _MSC_VER

#ifdef __UNIX__
#ifndef __ANDROID__
	int JNI_CreateJavaVM(void** pvm, void** penv, void* args)
	{
		exit(11);
	}
#endif // __ANDROID__
#endif // __UNIX__
}
#endif // __ANDROID__