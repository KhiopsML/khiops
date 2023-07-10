// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MODL_Coclustering.h"

int main(int argc, char** argv)
{
	CCLearningProject learningProject;

	// MemSetAllocIndexExit(1290133);

	// Lancement du projet
	learningProject.Start(argc, argv);

	// On renvoie 0 si tout s'est bien passe, 1 en cas de FatalError (dans Standard.cpp) et 2 si il y eu au moins
	// une erreur
	if (GetProcessId() == 0 and Global::IsAtLeastOneError())
		return EXIT_FAILURE + 1;
	else
		return EXIT_SUCCESS;
}
