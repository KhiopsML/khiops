// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MODL.h"

#ifndef __ANDROID__

int main(int argc, char** argv)
{
	MDKhiopsLearningProject learningProject;

	// Pour desactiver l'interception du signal "segmentation fault", pour permettre au debugger d'identifier le
	// probleme debug(signal(SIGSEGV, NULL));

	// Parametrage de l'arret pour la memoire ou les interruptions utilisateurs
	// MemSetAllocSizeExit(62181);
	// TaskProgression::SetExternalInterruptionRequestIndex();
	// TaskProgression::SetInterruptionRequestIndex(75);

	// #define USE_MPI (on passe par les directive de compilation)
#if defined(USE_MPI)
	PLParallelTask::UseMPI(GetLearningVersion());
#endif // defined(USE_MPI)

	// Possibilite de parametrage des log memoire depuis les variable d'environnement
	// p_setenv("KhiopsExpertMode", "true");
	if (GetLearningExpertMode())
	{
		//   KhiopsMemStatsLogFileName, KhiopsMemStatsLogFrequency, KhiopsMemStatsLogToCollect
		// On ne tente d'ouvrir le fichier que si ces trois variables sont presentes et valides
		// Sinon, on ne fait rien, sans message d'erreur
		// Pour avoir toutes les stats: KhiopsMemStatsLogToCollect=16383
		if (GetIOTraceMode())
			FileService::SetIOStatsActive(true);
		MemoryStatsManager::OpenLogFileFromEnvVars(true);
		// MemoryStatsManager::OpenLogFile("D:\\temp\\KhiopsMemoryStats\\Test\\KhiopsMemoryStats.log", 10000,
		// MemoryStatsManager::AllStats);
	}

	// Simulation du mode parallele pour le debuggage
	// PLParallelTask::SetParallelSimulated(true);
	// PLParallelTask::SetSimulatedSlaveNumber(3);
	// PLParallelTask::SetTracerResources(1);
	// PLParallelTask::SetTracerProtocolActive(true);
	// PLParallelTask::SetTracerMPIActive(true);
	// PLParallelTask::SetVerbose(true);
	// Activation d'un serveur de fichier sur une seule machine, pour debuggage
	// PLTaskDriver::SetFileServerOnSingleHost(true);

	// Creation d'attributs de type arbre, disponible en V9
	// La vue sur les parametres n'est disponible qu'en mode expert des arbres
	// KDDataPreparationAttributeCreationTask::SetGlobalCreationTask(new KDDPBivariateCrossProductsCreationTask);
	KDDataPreparationAttributeCreationTask::SetGlobalCreationTask(new DTDecisionTreeCreationTask);
	if (GetForestExpertMode())
		KDDataPreparationAttributeCreationTaskView::SetGlobalCreationTaskView(
		    new DTDecisionTreeCreationTaskView);

	// Lancement du projet
	learningProject.Start(argc, argv);

	// Nombre total d'interruptions utilisateurs
	// cout << "Interruption request number: " << TaskProgression::GetInterruptionRequestNumber() << endl;
	MemoryStatsManager::CloseLogFile();
	return EXIT_SUCCESS;
}

#endif // __ANDROID__