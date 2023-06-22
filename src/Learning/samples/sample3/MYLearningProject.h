// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWLearningProject.h"
#include "MYLearningProblem.h"
#include "MYLearningProblemView.h"

// Service de lancement du projet Khiops
class MYLearningProject : public KWLearningProject
{
public:
	// Constructeur
	MYLearningProject();
	~MYLearningProject();

	///////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void OpenLearningEnvironnement();
	KWLearningProblem* CreateLearningProblem();
	KWLearningProblemView* CreateLearningProblemView();
};
