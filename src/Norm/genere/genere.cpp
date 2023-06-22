// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Genere.h"

void CommandLineError()
{
	cout << "  Genere [options] <ClassName> <ClassLabel> <Attribute FileName>\n";
	cout << "\tOptions:\n";
	cout << "\t  -noarrayview\n";
	cout << "\t      pas de generation la classe <ClassName>ArrayView\n";
	cout << "\t  -noview\n";
	cout << "\t      pas de generation des classes <ClassName>View et <ClassName>ArrayView\n";
	cout << "\t  -nousersection\n";
	cout << "\t      pas de generation de sections utilisateurs\n";
	cout << "\t  -super <SuperClassName>\n";
	cout << "\t      parametrage (optionnel) du nom de la classe mere";
	cout << endl;
	exit(0);
}

void Genere(int argc, char** argv)
{
	TableGenerator tgTest;
	ALString sOption;
	ALString sClassName;
	ALString sClassUserLabel;
	ALString sSuperClassName;
	ALString sAttributeFileName;

	if (argc >= 4)
	{
		for (int i = 1; i < argc - 3; i++)
		{
			sOption = argv[i];
			if (sOption == "-nomanagement")
			{
				cout << "option -nomanagement is deprecated" << endl;
			}
			else if (sOption == "-noarrayview")
				tgTest.SetGenereArrayView(false);
			else if (sOption == "-noview")
				tgTest.SetGenereView(false);
			else if (sOption == "-nousersection")
				tgTest.SetGenereUserSection(false);
			else if (sOption == "-super")
			{
				// Acces si possible au nom de la classe mere
				if (i + 1 < argc - 3)
				{
					i++;
					sSuperClassName = argv[i];
				}
			}
			else
			{
				cout << sOption << " n'est pas une option valide\n";
				CommandLineError();
			}
		}
		sClassName = argv[argc - 3];
		sClassUserLabel = argv[argc - 2];
		sAttributeFileName = argv[argc - 1];
		cout << "Genere " << sClassName << " " << sClassUserLabel << " " << sAttributeFileName << endl;
		tgTest.GenereWith(sClassName, sSuperClassName, sClassUserLabel, sAttributeFileName);
	}
	else
		CommandLineError();
}

// Portage unix void->int
int main(int argc, char** argv)
{
	Genere(argc, argv);
	return 0;
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
	int __stdcall _imp__JNI_GetCreatedJavaVMs(void**, long, long*)
	{
		exit(11);
	}

	// Version 64 bits
	int __stdcall __imp_JNI_CreateJavaVM(void** pvm, void** penv, void* args)
	{
		exit(11);
	}
	int __stdcall __imp_JNI_GetCreatedJavaVMs(void**, long, long*)
	{
		exit(11);
	}
#endif // _MSC_VER

#ifdef __UNIX__
	int JNI_CreateJavaVM(void** pvm, void** penv, void* args)
	{
		exit(11);
	}
	int JNI_GetCreatedJavaVMs(void** pvm, void** penv, void* args)
	{
		exit(11);
	}
#endif // __UNIX__
}