// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWLearningProject.h"
#include "CCLearningProblem.h"
#include "CCLearningProblemView.h"
#include "KWDRDataGridDeployment.h"

// Service de lancement du projet Khiops
class CCLearningProject : public KWLearningProject
{
public:
	// Constructeur
	CCLearningProject();
	~CCLearningProject();

	///////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void OpenLearningEnvironnement() override;
	Object* CreateGenericLearningProblem() override;
	UIObjectView* CreateGenericLearningProblemView() override;
};
