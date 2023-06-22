// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWLearningProject.h"
#include "KNITransferProblem.h"
#include "KNITransferProblemView.h"

// Service de lancement du projet Khiops
class KNITransferProject : public KWLearningProject
{
public:
	// Constructeur
	KNITransferProject();
	~KNITransferProject();

	///////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles
	void OpenLearningEnvironnement() override;
	KWLearningProblem* CreateLearningProblem() override;
	KWLearningProblemView* CreateLearningProblemView() override;
};
