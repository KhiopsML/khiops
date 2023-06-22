// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KNITransfer.h"

int main(int argc, char** argv)
{
	KNITransferProject learningProject;

	// #define USE_MPI
#if defined(USE_MPI)
	PLParallelTask::UseMPI(GetLearningVersion());
#endif // defined(USE_MPI)

	// MemSetAllocIndexExit(1290133);

	// Lancement du projet
	learningProject.Start(argc, argv);
	return 0;
}