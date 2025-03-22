// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#ifdef USE_MPI
#include "PLMPITaskDriver.h"
#include "PLMPISystemFileDriverRemote.h"
#include "KWVersion.h"
#endif // USE_MPI

// Methode technique a appeler pour parametrer l'usage de la librairie Parallel depuis Learning
// Cette methode ne fait rien si USE_MPI n'est pas defini
inline void UseMPI()
{
#ifdef USE_MPI
	// Mise en place du driver parallel
	PLParallelTask::SetDriver(PLMPITaskDriver::GetDriver());

	// Initialisation des ressources systeme
	PLParallelTask::GetDriver()->InitializeResourceSystem();

	// Chargement du driver pour l'acces aux fichiers distants (file://)
	if (RMResourceManager::GetResourceSystem()->GetHostNumber() > 1 or GetFileServerActivated())
		SystemFileDriverCreator::RegisterDriver(new PLMPISystemFileDriverRemote);

	// Activation du serveure de fichier en mono-machine si necessaire
	if (GetFileServerActivated())
		PLTaskDriver::SetFileServerOnSingleHost(true);

	// Verification des versions de chaque processus
	PLParallelTask::SetVersion(GetLearningVersion());
	PLMPITaskDriver::CheckVersion();

#endif // USE_MPI
}
