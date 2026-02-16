// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLComputePiTask.h"

int main()
{
    int totalIter = 100;
    int nWorkerNumber = 4;
    boolean isSimulatedParallel = true;
    PLComputePiTask* plComputePiTask = new PLComputePiTask();
    plComputePiTask->SetTotalIteration(totalIter);
    plComputePiTask->SetSimulatedSlaveNumber(nWorkerNumber);
    PLParallelTask::RegisterTask(plComputePiTask);
    PLParallelTask::SetParallelSimulated(isSimulatedParallel);
    plComputePiTask->ComputePi();
    cout << "Computed Pi (parallel): " << plComputePiTask->GetComputedPi() << endl;
    PLParallelTask::DeleteAllTasks();
}
