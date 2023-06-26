// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "main.h"

int main(int argv, char** argc)
{
#if defined(USE_MPI)

	// Mise en place du fdriver parallel
	PLParallelTask::SetDriver(PLMPITaskDriver::GetDriver());

	// Initialisation des ressources systeme
	PLParallelTask::GetDriver()->InitializeResourceSystem();

	// Chargement du driver pour l'acces aux fichiers distants (file://)
	if (RMResourceManager::GetResourceSystem()->GetHostNumber() > 1 or PLTaskDriver::GetFileServerOnSingleHost())
		SystemFileDriverCreator::RegisterDriver(new PLMPISystemFileDriverRemote);

	// Verification des versions de chaque processus
	PLParallelTask::SetVersion("1.0");
	PLMPITaskDriver::CheckVersion();

#endif // defined(USE_MPI)

	// Activation des traces
	PLParallelTask::SetVerbose(false);
	PLParallelTask::SetTracerResources(0);

	// Activation du mode Parallele Simule
	PLParallelTask::SetParallelSimulated(false);
	PLParallelTask::SetSimulatedSlaveNumber(20);

	// Declaration des taches paralleles
	// DEPRECTATED PLParallelTask::RegisterTask(new PEFileMasterTestTask);
	PLParallelTask::RegisterTask(new PEFileSearchTask);
	PLParallelTask::RegisterTask(new PEHelloWorldTask);
	PLParallelTask::RegisterTask(new PEIOParallelTestTask);
	PLParallelTask::RegisterTask(new PEPiTask);
	PLParallelTask::RegisterTask(new PEProtocolTestTask);

	// Mode textuel par defaut pour le maitre et les esclaves
	UIObject::SetUIMode(UIObject::Textual);

	if (PLParallelTask::IsMasterProcess())
	{
		// Mise en place des repertoires temporaires
		FileService::SetApplicationName("ParallelSample");

		// Mode maitre

		// Parametrage de l'arret de l'allocateur
		// MemSetAllocIndexExit(648);
		// MemSetAllocSizeExit(488);
		// MemSetAllocBlockExit((void*)0x00F42044);

		// Analyse de la ligne de commande
		UIObject::ParseMainParameters(argv, argc);

		// Lancement de l'outil
		PEPiView::Test(argv, argc);
		// PEIOParallelTestTask::Test();
		// PEFileMasterTestTask::Test();
		// PEProtocolTestTask::Test();
		// PEProtocolTestTask::TestFatalError();
		// PEFileSearchTask::Test();
		// PEHelloWorldTask::Test();

		PLParallelTask::GetDriver()->StopSlaves();
	}
	// Mode esclave
	else
	{
		// Lancement de l'esclave
		PLParallelTask::GetDriver()->StartSlave();
	}

	PLParallelTask::DeleteAllTasks();
	return EXIT_SUCCESS;
}