// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "test.h"

void StartSampleOne(int argc, char** argv)
{
	SampleOneLearningProject learningProject;
	learningProject.Start(argc, argv);
}

void StartLearningProblem(int argc, char** argv)
{
	KWLearningProject learningProject;
	learningProject.Start(argc, argv);
}

int main(int argc, char** argv)
{
	// MemSetAllocIndexExit(12431);

	// Pour permettre une interaction en mode textuelle dans les environnements sans interface utilisateur java
	// (par defaut, cela n'est pas autorise pour l'outil Khiops)
	UIObject::SetTextualInteractiveModeAllowed(true);

	// Lancement de la vue principale
	StartSampleOne(argc, argv);
	// StartLearningProblem(argc, argv);

	return 0;
}
