// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CMLearningProject.h"

// Vue principale de lancement de l'application
void StartMajorityClassifierProblem(int argc, char** argv)
{
	CMLearningProject learningProject;
	learningProject.Start(argc, argv);
}

void StartLearningProblem(int argc, char** argv)
{
	KWLearningProject learningProject;
	learningProject.Start(argc, argv);
}

int main(int argc, char** argv)
{
	// A decommenter lorsque l'on trace un probleme de desallocation memoire
	// MemSetAllocIndexExit(12431);

	// Pour permettre une interaction en mode textuelle dans les environnements sans interface utilisateur java
	// (par defaut, cela n'est pas autorise pour l'outil Khiops)
	UIObject::SetTextualInteractiveModeAllowed(true);

	// Lancement de la vue principale
	StartMajorityClassifierProblem(argc, argv);
	// StartLearningProblem(argc, argv);

	return 0;
}