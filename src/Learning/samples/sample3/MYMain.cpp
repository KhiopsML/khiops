// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MYMain.h"

void StartMyProject(int argc, char** argv)
{
	MYLearningProject learningProject;
	learningProject.Start(argc, argv);
}

void StartLearningProblem(int argc, char** argv)
{
	KWLearningProject learningProject;
	learningProject.Start(argc, argv);
}

int main(int argc, char** argv)
{
	// Parametrage du nom du module applicatif
	SetLearningModuleName("My project");

	// Parametrage de l'arret de l'allocateur
	// MemSetAllocIndexExit(10877);

	// Lancement de l'outil
	StartMyProject(argc, argv);
	// StartLearningProblem(argc, argv);
	return 0;
}
