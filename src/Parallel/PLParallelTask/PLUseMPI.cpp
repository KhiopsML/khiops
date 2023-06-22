// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLParallelTask.h"
#ifdef USE_MPI
#include "PLMPITaskDriver.h"
#include "PLMPISystemFileDriverRemote.h"
#endif // USE_MPI

#ifndef __UNIX__

#ifdef USE_MPI
// Bibliotheques necessaires a l'execution parallele
#pragma comment(lib, "msmpi")
#pragma comment(lib, "PLMPI")
#endif // USE_MPI
#endif //__UNIX__

void PLParallelTask::UseMPI(const ALString& sCurrentVersion)
{
#ifdef USE_MPI
	require(sCurrentVersion != "");

	// Version du logiciel
	PLParallelTask::SetVersion(sCurrentVersion);

	// Le driver doit etre positionner avant la declaration des taches
	// On s'assure ainsi qu'il est declare tres tot dans le programme
	require(nInstanciatedTaskNumber == 0);
	PLParallelTask::SetDriver(PLMPITaskDriver::GetDriver());

	// Initialisation locale des ressources systeme
	PLParallelTask::GetDriver()->InitializeResourceSystem();

	// Chargement du driver pour l'acces aux fichiers distants (file://)
	if (RMResourceManager::GetResourceSystem()->GetHostNumber() > 1 or PLTaskDriver::GetFileServerOnSingleHost())
		SystemFileDriverCreator::RegisterDriver(new PLMPISystemFileDriverRemote);

	// Verification des versions de chaque processus
	PLMPITaskDriver::CheckVersion();
#endif // USE_MPI
}
