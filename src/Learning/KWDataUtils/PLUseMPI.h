#pragma once

#ifdef USE_MPI
#include "PLMPITaskDriver.h"
#include "PLMPISystemFileDriverRemote.h"
#endif // USE_MPI

// Methode technique a appeler pour parametrer l'usage de la librairie Parallel depuis Learning
// Cette methode ne fait rien si USE_MPI n'est pas defini
inline void UseMPI()
{
#ifdef USE_MPI

// Pour indiquer les libraries a utiliser par le linker
// Potentiellement inutile apres utilisation de cmake
#pragma comment(lib, "msmpi")
#pragma comment(lib, "PLMPI")

	// Mise en place du driver parallel
	PLParallelTask::SetDriver(PLMPITaskDriver::GetDriver());

	// Initialisation des ressources systeme
	PLParallelTask::GetDriver()->InitializeResourceSystem();

	// Chargement du driver pour l'acces aux fichiers distants (file://)
	if (RMResourceManager::GetResourceSystem()->GetHostNumber() > 1 or PLTaskDriver::GetFileServerOnSingleHost())
		SystemFileDriverCreator::RegisterDriver(new PLMPISystemFileDriverRemote);

	// Verification des versions de chaque processus
	PLParallelTask::SetVersion(GetLearningVersion());
	PLMPITaskDriver::CheckVersion();

#endif // USE_MPI
}