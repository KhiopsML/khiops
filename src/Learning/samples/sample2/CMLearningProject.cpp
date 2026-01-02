// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CMLearningProject.h"

CMLearningProject::CMLearningProject() {}

CMLearningProject::~CMLearningProject() {}

KWLearningProblem* CMLearningProject::CreateLearningProblem()
{
	return new CMLearningProblem;
}

KWLearningProblemView* CMLearningProject::CreateLearningProblemView()
{
	return new CMLearningProblemView;
}

void CMLearningProject::OpenLearningEnvironnement()
{
	// Appel de la methode ancetre
	KWLearningProject::OpenLearningEnvironnement();

	// Enregistrement de la regle de derivation Majority
	KWDerivationRule::RegisterDerivationRule(new CMDRMajorityClassifier);

	// Enregistrement du predicteur majoritaire
	KWPredictor::RegisterPredictor(new CMMajorityClassifier);

	// Enregistrement de vues sur les predicteurs
	// KWPredictorView::RegisterPredictorView(new CMMajorityClassifierView);
}
