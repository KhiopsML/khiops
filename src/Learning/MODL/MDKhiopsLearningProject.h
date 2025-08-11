// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWLearningProject.h"
#include "MDKhiopsLearningProblem.h"
#include "MDKhiopsLearningProblemView.h"
#include "KWDRDataGridDeployment.h"

// DTFOREST
#include "DTDiscretizerMODL.h"
#include "DTGrouperMODL.h"

// Service de lancement du projet Khiops
class MDKhiopsLearningProject : public KWLearningProject
{
public:
	// Constructeur
	MDKhiopsLearningProject();
	~MDKhiopsLearningProject();

	///////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void OpenLearningEnvironnement() override;
	KWLearningProblem* CreateLearningProblem() override;
	KWLearningProblemView* CreateLearningProblemView() override;
};
