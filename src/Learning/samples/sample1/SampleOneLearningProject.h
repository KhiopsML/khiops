// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWLearningProject.h"
#include "SampleOneLearningProblem.h"
#include "SampleOneLearningProblemView.h"

// Service de lancement du projet
class SampleOneLearningProject : public KWLearningProject
{
public:
	// Constructeur
	SampleOneLearningProject();
	~SampleOneLearningProject();

	///////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void OpenLearningEnvironnement() override;
	KWLearningProblem* CreateLearningProblem() override;
	KWLearningProblemView* CreateLearningProblemView() override;
};
