// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MODL_Coclustering.h"

#ifndef __ANDROID__

int main(int argc, char** argv)
{
	CCLearningProject learningProject;

	// MemSetAllocIndexExit(1290133);

	// Possibilite de parametrage des log memoire depuis les variable d'environnement
	if (GetLearningExpertMode())
	{
		//   KhiopsMemStatsLogFileName, KhiopsMemStatsLogFrequency, KhiopsMemStatsLogToCollect
		// On ne tente d'ouvrir le fichier que si ces trois variables sont presentes et valides
		// Sinon, on ne fait rien, sans message d'erreur
		// Pour avoir toutes les stats: KhiopsMemStatsLogToCollect=16383
		MemoryStatsManager::OpenLogFileFromEnvVars(true);
		// MemoryStatsManager::OpenLogFile("D:\\temp\\KhiopsMemoryStats\\Test\\KhiopsMemoryStats.log", 10000,
		// MemoryStatsManager::AllStats);
	}

	// Lancement du projet
	learningProject.Start(argc, argv);
	return EXIT_SUCCESS;
}

#endif // __ANDROID__