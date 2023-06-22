// Copyright (c) 2023 Orange. All rights reserved.
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
#ifdef _DEBUG
	std::cout << "*******COMPILATION********" << std::endl;
	std::cout << "        Sample 1           " << std::endl;
	std::cout << __DATE__ << " " << __TIME__ << std::endl;
	std::cout << "**************************" << std::endl;
#endif

	// MemSetAllocIndexExit(12431);

	// Lancement de la vue principale
	StartSampleOne(argc, argv);
	// StartLearningProblem(argc, argv);

#ifdef _DEBUG
	std::cout << "'Debug' executable, press enter..." << std::endl;
	std::cin.get();
#endif

	return 0;
}