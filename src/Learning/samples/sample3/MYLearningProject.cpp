// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MYLearningProject.h"

MYLearningProject::MYLearningProject() {}

MYLearningProject::~MYLearningProject() {}

void MYLearningProject::OpenLearningEnvironnement()
{
	// Appel de la methode ancetre
	KWLearningProject::OpenLearningEnvironnement();

	// Declaration des licences
	LMLicenseManager::DeclarePredefinedLicense(LMLicenseManager::Khiops);
}

KWLearningProblem* MYLearningProject::CreateLearningProblem()
{
	return new MYLearningProblem;
}

KWLearningProblemView* MYLearningProject::CreateLearningProblemView()
{
	return new MYLearningProblemView;
}