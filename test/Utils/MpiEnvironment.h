// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "gtest/gtest.h"
#include "PLParallelTask.h"
#include "PLMPITaskDriver.h"
#include "PLMPISystemFileDriverRemote.h"
#include "PEProtocolTestTask.h"

////////////////////////////////////////////////////////////////////////////
// Classe MpiEnvironment
// Cette classe permet de lancer les tests unitaires en parallele avec MPI
// Tout l'environement de la bibliotheque parallele est mis en place :
// - declaration des taches paralleles
// - Mise en place du driver
// - initialisation des ressources
// - lancement des processus esclaves (PLMPISlaveLauncher)
// Cette classe doit etre utilisee dans la methode main du test, apres la methode d'initialisation :
//
//         ::testing::InitGoogleTest(&argc, argv);
//         ::testing::AddGlobalTestEnvironment(new MpiEnvironment);
//
class MpiEnvironment : public ::testing::Environment
{
public:
	~MpiEnvironment() override {}

	// Override this to define how to set up the environment.
	void SetUp() override
	{

#if defined(USE_MPI)

		// Mise en place du fdriver parallel
		PLParallelTask::SetDriver(PLMPITaskDriver::GetDriver());

		// Initialisation des ressources systeme
		PLParallelTask::GetDriver()->InitializeResourceSystem();

		// Chargement du driver pour l'acces aux fichiers distants (file://)
		if (RMResourceManager::GetResourceSystem()->GetHostNumber() > 1 or
		    PLTaskDriver::GetFileServerOnSingleHost())
			SystemFileDriverCreator::RegisterDriver(new PLMPISystemFileDriverRemote);

		// Verification des versions de chaque processus
		PLParallelTask::SetVersion("1.0");
		PLMPITaskDriver::CheckVersion();

#endif // defined(USE_MPI)

		PLParallelTask::RegisterTask(new PEProtocolTestTask);

		if (PLParallelTask::IsMasterProcess())
		{
			PLParallelTask::GetDriver()->MasterInitializeResourceSystem();
		}
		else
		{
			// Lancement de l'esclave
			PLParallelTask::GetDriver()->StartSlave();
		}
	}

	// Override this to define how to tear down the environment.
	void TearDown() override
	{

		if (PLParallelTask::IsMasterProcess())
		{
			PLParallelTask::GetDriver()->StopSlaves();
		}

		SystemFileDriverCreator::UnregisterDrivers();

		PLParallelTask::DeleteAllTasks();
	}
};