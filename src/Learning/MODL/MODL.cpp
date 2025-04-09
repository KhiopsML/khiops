// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MODL.h"

// Parametrage des variables d'environnement pour les jeux de test de LearningTest (via .vs/launch.vs.json sous Windows Visual C++)
// KHIOPS_API_MODE="true"
// - pour que les chemins relatifs soient traites par rapport au cwd (current working directory) pour le stockage des rapports
// - sinon, en mode GUI, les chemins relatifs sont geres par rapport au repertoire contenant le fichier de donnees
// KhiopsFastExitMode="false"
// - pour que l'on puisse exploiter un scenario de test integralement, meme en cas d'erreurs multiples
// - sinon, on sort du scenario apres la premiere erreur

int main(int argc, char** argv)
{
	MDKhiopsLearningProject learningProject;

	// Activation de la gestion des signaux via des erreurs, pour afficher des messages d'erreur explicites
	// A potentiellement commenter sur certains IDE lors des phases de debuggage
	//Global::ActivateSignalErrorManagement();

	// Parametrage des logs memoires depuis les variables d'environnement, pris en compte dans KWLearningProject
	//   KhiopsMemStatsLogFileName, KhiopsMemStatsLogFrequency, KhiopsMemStatsLogToCollect
	// On ne tente d'ouvrir le fichier que si ces trois variables sont presentes et valides
	// Sinon, on ne fait rien, sans message d'erreur
	// Pour avoir toutes les stats: KhiopsMemStatsLogToCollect=16383
	// Pour la trace des IO: KhiopsIOTraceMode
	// FileService::SetIOStatsActive(true);
	// MemoryStatsManager::OpenLogFile("D:\\temp\\KhiopsMemoryStats\\Test\\KhiopsMemoryStats.log", 10000,
	// MemoryStatsManager::AllStats);

	// Point d'arret sur l'allocation d'un bloc memoire
	// MemSetAllocIndexExit(68702);

	// Parametrage de l'arret pour les interruptions utilisateurs
	// TaskProgression::SetExternalInterruptionRequestIndex();
	// TaskProgression::SetInterruptionRequestIndex(75);

	// Parametrage de l'utilisation de MPI
	UseMPI();

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

	// On renvoie 0 si tout s'est bien passe, 1 en cas de FatalError (dans Standard.cpp)
	return EXIT_SUCCESS;
}
