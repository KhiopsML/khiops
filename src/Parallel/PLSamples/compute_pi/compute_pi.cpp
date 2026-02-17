// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Standard.h"
#if defined USE_MPI
#include "PLMPITaskDriver.h"
#include "PLMPISystemFileDriverRemote.h"
#endif // USE_MPI
#include "PLComputePiTask.h"

void UseMPI()
{
    #if defined USE_MPI
        // Mise en place du driver parallel
        PLParallelTask::SetDriver(PLMPITaskDriver::GetDriver());

        // Initialisation des ressources systeme
        PLParallelTask::GetDriver()->InitializeResourceSystem();

        // Chargement du driver pour l'acces aux fichiers distants (file://)
        if (RMResourceManager::GetResourceSystem()->GetHostNumber() > 1 or PLTaskDriver::GetFileServerOnSingleHost())
            SystemFileDriverCreator::RegisterDriver(new PLMPISystemFileDriverRemote);

        // Verification des versions de chaque processus
        PLParallelTask::SetVersion("1.0");
        PLMPITaskDriver::CheckVersion();
    #endif // USE_MPI
}

int main(int argc, char** argv)
{
    UseMPI();
    boolean bIsSimulatedParallel = StringToBoolean(argv[1]);
    int nTotalIter = 100;
    int nWorkerNumber = 4;
    PLComputePiTask* plComputePiTask = new PLComputePiTask();
    PLParallelTask::RegisterTask(plComputePiTask);
    PLParallelTask::SetParallelSimulated(bIsSimulatedParallel);
    plComputePiTask->SetTotalIteration(nTotalIter);
    PLParallelTask::SetSimulatedSlaveNumber(nWorkerNumber);
	SystemFileDriverCreator::RegisterExternalDrivers();
    if (PLParallelTask::IsMasterProcess())
    {
        cout << "Parallel mode availability is: " << PLParallelTask::GetDriver()->IsParallelModeAvailable() << endl;
		FileService::SetApplicationName("compute_pi");
        plComputePiTask->ComputePi();
        cout << "Computed Pi (parallel): " << plComputePiTask->GetComputedPi() << endl;
        PLParallelTask::GetDriver()->StopSlaves();
    }
    else
    {
        // Lancement de l'esclave
        PLParallelTask::GetDriver()->StartSlave();
    }
    SystemFileDriverCreator::UnregisterDrivers();
    PLParallelTask::DeleteAllTasks();
}
