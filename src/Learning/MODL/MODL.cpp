// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MODL.h"

// Debogage sous Windows Visual C++ 2022 (bug https://github.com/microsoft/vscode-cpptools/issues/8084)
// Choix en dur du repertoire de lancement
void SetWindowsDebugDir(const ALString& sDatasetFamily, const ALString& sDataset)
{
#ifdef _WIN32
	ALString sUserRootPath;
	int nRet;

	// A parametrer pour chaque utilisateur
	// Devra etre fait plus proprement quand tout l'equipe sera sur git, par exemple via une variable
	// d'environnement et quelques commentaires clairs
	sUserRootPath = "D:/Users/miib6422/Documents/boullema/LearningTest.V10.5.0-a1/LearningTest/TestKhiops/";

	// Pour permettre de continuer a utiliser LearningTest, on ne fait rien s'il y a deja un fichier test.prm
	// dans le repertoire courante
	if (FileService::FileExists("test.prm"))
		return;

	// Changement de repertoire, uniquement pour Windows
	nRet = _chdir(sUserRootPath + sDatasetFamily + "/" + sDataset);
#endif
}

int main(int argc, char** argv)
{
	MDKhiopsLearningProject learningProject;

	// Activation de la gestion des signaux via des erreurs, pour afficher des messages d'erreur explicites
	// A potentiellement commenter sur certains IDE lors des phases de debuggage
	Global::ActivateSignalErrorManagement();

	// Choix du repertoire de lancement pour le debugage sous Windows (a commenter apres fin du debug)
	// SetWindowsDebugDir("Standard", "IrisLight");
	// SetWindowsDebugDir("Standard", "IrisU2D");

	// Parametrage des logs memoires depuis les variables d'environnement, pris en compte dans KWLearningProject
	//   KhiopsMemStatsLogFileName, KhiopsMemStatsLogFrequency, KhiopsMemStatsLogToCollect
	// On ne tente d'ouvrir le fichier que si ces trois variables sont presentes et valides
	// Sinon, on ne fait rien, sans message d'erreur
	// Pour avoir toutes les stats: KhiopsMemStatsLogToCollect=16383
	// Pour la trace des IO: KhiopsIOTraceMode
	// FileService::SetIOStatsActive(true);
	// MemoryStatsManager::OpenLogFile("D:\\temp\\KhiopsMemoryStats\\Test\\KhiopsMemoryStats.log", 10000,
	// MemoryStatsManager::AllStats);

	// Parametrage de l'arret pour la memoire ou les interruptions utilisateurs
	// MemSetAllocIndexExit(37140);
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
