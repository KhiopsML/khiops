// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#ifdef __UNIX__
#include "PLParallelTask.h"
#include "PLMPITaskDriver.h"

// Sous windows, la methode UseMPI est definie dans le fichier PLUseMPI de PLParallelTask
// Car l'appel de PLMPITaskDriver cree une dependance cyclique qui est bien gere sous windows
// mais n'est pas tolere sous linux.
// Lorsqu'on modifie cette methode, il faut egalement la modifier dans l'autre fichier

void PLParallelTask::UseMPI(const ALString& sVersion)
{
	require(sVersion != "");

	// Version du logiciel
	PLParallelTask::SetVersion(sVersion);

	// Le driver doit etre positionner avant la declaration des taches
	// On s'assure ainsi qu'il est declare tres tot dans le programme
	require(nInstanciatedTaskNumber == 0);
	PLParallelTask::SetDriver(PLMPITaskDriver::GetDriver());

	// Initialisation des ressources systeme
	PLParallelTask::GetDriver()->InitializeResourceSystem();

	// Verification des versions de chaque processus
	PLMPITaskDriver::CheckVersion();
}
#endif // __UNIX__