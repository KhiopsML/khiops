// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SampleOneLearningProject.h"

SampleOneLearningProject::SampleOneLearningProject() {}

SampleOneLearningProject::~SampleOneLearningProject() {}

void SampleOneLearningProject::OpenLearningEnvironnement()
{
	// Appel de la methode ancetre
	KWLearningProject::OpenLearningEnvironnement();

	// Declaration des licences
	LMLicenseManager::DeclarePredefinedLicense(LMLicenseManager::Khiops);
}

KWLearningProblem* SampleOneLearningProject::CreateLearningProblem()
{
	return new SampleOneLearningProblem;
}

KWLearningProblemView* SampleOneLearningProject::CreateLearningProblemView()
{
	return new SampleOneLearningProblemView;
}