// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "main.h"

int main(int argc, char** argv)
{
	const ALString sBatchOption = "-batch";

	// MemSetAllocIndexExit(435);
	// MemoryStatsManager::OpenLogFile("d:\\temp\\MemoryStats.log", 10000, MemoryStatsManager::AllStats);
	// MemoryStatsManager::OpenLogFileFromEnvVars();

	// Execution des tests en mode batch si option -batch sur la ligne de commande
	if (argc >= 2 and sBatchOption == (const char*)argv[1])
	{
		// Cas standard (avec cout)
		if (argc == 2)
			TestBaseComponents::BatchTest();
		// Cas avec nom de fichier de sortie sur la ligne de commande
		else
		{
			// Utilisation d'un stream dedie au batch
			ofstream batchCout(argv[2]);

			// Sauvegarde du buffer associe a cout
			streambuf* coutBuf = std::cout.rdbuf();

			// Redirection de cout vers le stream dedie au batch
			cout.rdbuf(batchCout.rdbuf());

			// Lancement des tests
			try
			{
				TestBaseComponents::BatchTest();
			}
			catch (exception& e)
			{
				ALString sTmp;
				Global::AddFatalError("", "", sTmp + "unknown error:" + e.what());
			}

			// On restitue cout dans son etat initial
			cout.rdbuf(coutBuf);
		}
	}
	// Execution des test en mode interactif dans une fenetre de shell
	else
	{
		UIObject::ParseMainParameters(argc, argv);

		// Test des composants en mode interactif
		TestBaseComponents::InteractiveTest();
	}

	// Affichage des stats sur la heap
	// MemPrintHeapStats(stdout);
	MemoryStatsManager::CloseLogFile();
	return 0;
}