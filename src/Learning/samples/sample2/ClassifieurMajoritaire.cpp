// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#define VERSION_FULL "2.0.0"

#pragma warning(disable : 4996)

#include "CMLearningProject.h"

// Vue principale de lancement de l'application

#define CGEDBG(x) cout << "[" << x << "]" << endl

void StartLogisticRegression(int argc, char** argv)
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
#ifdef _DEBUG
	std::cout << "*******COMPILATION********" << std::endl;
	std::cout << "    Sample 2 : Classifieur Majoritaire    " << std::endl;
	std::cout << " VERSION ";
	std::cout << " " << VERSION_FULL << std::endl;
	std::cout << __DATE__ << " " << __TIME__ << std::endl;
	std::cout << "**************************" << std::endl;
#endif

	// A decommenter lorsque l'on trace un probleme de desallocation memoire
	// MemSetAllocIndexExit(12431);

	// Lancement de la vue principale
	StartLogisticRegression(argc, argv);
	// StartLearningProblem(argc, argv);

#ifdef _DEBUG
	std::cout << "'Debug' executable, press enter..." << std::endl;
	std::cin.get();
#endif

	return 0;
}