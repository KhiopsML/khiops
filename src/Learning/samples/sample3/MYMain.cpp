// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MYMain.h"

void StartMyProject(int argc, char** argv)
{
	MYLearningProject learningProject;
	learningProject.Start(argc, argv);
}

void StartLearningProblem(int argc, char** argv)
{
	KWLearningProject learningProject;
	learningProject.Start(argc, argv);
}

int main(int argc, char** argv)
{
	// Parametrage du nom du module applicatif
	SetLearningModuleName("My project");

	// Parametrage du copyright dans les fenetres
	UIUnit::SetCopyrightLabel(GetLearningCopyrightLabel());

	// Baniere de l'application
	cout << GetLearningShellBanner() << endl;

	// Mode multi-tables
	if (GetLearningMultiTableMode())
		cout << "    Multiple-table version" << endl;

		// Parametrage de l'arret de l'allocateur
		// MemSetAllocIndexExit(10877);
		// MemSetAllocSizeExit(73724);
		// MemSetAllocBlockExit((void*)0x00F42044);

#ifdef KWLearningBatchMode
	// Mode textuel, sans JNI
	UIObject::SetUIMode(UIObject::Textual);
#endif // KWLearningBatchMode

	// Lancement de l'outil
	StartMyProject(argc, argv);
	// StartLearningProblem(argc, argv);
	return 0;
}

#ifdef KWLearningBatchMode
/********************************************************************
 * Le source suivant permet de compiler des sources developpes avec *
 * l'environnement Norm, d'utiliser le mode UIObject::Textual et    *
 * de ne pas linker avec jvm.lib (a eviter absoluement).            *
 * Moyennant ces conditions, on peut livrer un executable en mode   *
 * textuel ne necessitant pas l'intallation prealable du JRE Java   *
 ********************************************************************/

extern "C"
{
#ifdef _WIN32
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
#else

#ifndef __ANDROID__
	int JNI_CreateJavaVM(void** pvm, void** penv, void* args)
	{
		exit(11);
	}
#endif // __ANDROID__

#endif // _WIN32
}
#endif // __ANDROID__
#endif // KWLearningBatchMode
