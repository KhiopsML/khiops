// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Genere.h"

void CommandLineError()
{
	cout << "Genere [options] <ClassName> <ClassLabel> <AttributeFileName>\n";
	cout << "\t  <ClassName>: base name for generated classes\n";
	cout << "\t  <ClassLabel>: label for generated classes\n";
	cout << "\t  <AttributeFileName>: name of the file (.dd) containing the attribute specifications\n";
	cout << "\tOptions:\n";
	cout << "\t  -nomodel\n";
	cout << "\t      no generation of class <ClassName>\n";
	cout << "\t  -noarrayview\n";
	cout << "\t      no generation of class <ClassName>ArrayView\n";
	cout << "\t  -noview\n";
	cout << "\t      no generation of classes <ClassName>View and <ClassName>ArrayView\n";
	cout << "\t  -nousersection\n";
	cout << "\t      no generation of user sections\n";
	cout << "\t  -specificmodel <SpecificModelClassName>\n";
	cout << "\t      (optionnal) name of a specific model class, to use instead of ClassName\n";
	cout << "\t  -super <SuperClassName>\n";
	cout << "\t      (optionnal) name of the parent class\n";
	cout << "\t  -outputdir <DirName>\n";
	cout << "\t      ouput directory for generation (default: current directory)";
	cout << endl;
	exit(0);
}

void Genere(int argc, char** argv)
{
	TableGenerator tgTest;
	ALString sOption;
	ALString sClassName;
	ALString sClassUserLabel;
	ALString sSpecificModelClassName;
	ALString sSuperClassName;
	ALString sAttributeFileName;

	if (argc >= 4)
	{
		for (int i = 1; i < argc - 3; i++)
		{
			sOption = argv[i];
			if (sOption == "-nomodel")
				tgTest.SetGenereModel(false);
			else if (sOption == "-noarrayview")
				tgTest.SetGenereArrayView(false);
			else if (sOption == "-noview")
				tgTest.SetGenereView(false);
			else if (sOption == "-nousersection")
				tgTest.SetGenereUserSection(false);
			else if (sOption == "-specificmodel")
			{
				// Acces si possible au nom de la classe mere
				if (i + 1 < argc - 3)
				{
					i++;
					sSpecificModelClassName = argv[i];
				}
			}
			else if (sOption == "-super")
			{
				// Acces si possible au nom de la classe mere
				if (i + 1 < argc - 3)
				{
					i++;
					sSuperClassName = argv[i];
				}
			}
			else if (sOption == "-outputdir")
			{
				// Acces si possible au nom de la classe mere
				if (i + 1 < argc - 3)
				{
					i++;
					tgTest.SetOutputDir(argv[i]);
					if (not FileService::DirExists(tgTest.GetOutputDir()))
					{
						cout << "ouputdir <" << tgTest.GetOutputDir()
						     << "> is not a valid directory\n";
						CommandLineError();
					}
				}
			}
			else
			{
				cout << sOption << " is not a valid option\n";
				CommandLineError();
			}
		}
		sClassName = argv[argc - 3];
		sClassUserLabel = argv[argc - 2];
		sAttributeFileName = argv[argc - 1];
		cout << "Genere " << sClassName << " " << sClassUserLabel << " " << sAttributeFileName << endl;
		tgTest.GenereWith(sClassName, sSpecificModelClassName, sSuperClassName, sClassUserLabel,
				  sAttributeFileName);
		cout << endl;
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
 * textuel ne necessitant pas l'installation prealable du JRE Java   *
 ********************************************************************/

extern "C"
{
#ifdef _WIN32
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
#else
	int JNI_CreateJavaVM(void** pvm, void** penv, void* args)
	{
		exit(11);
	}
	int JNI_GetCreatedJavaVMs(void** pvm, void** penv, void* args)
	{
		exit(11);
	}
#endif // _WIN32
}
