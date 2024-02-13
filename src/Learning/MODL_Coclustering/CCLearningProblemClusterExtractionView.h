// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"

#include "CCLearningProblem.h"

#include "CCLearningProblemToolView.h"
#include "CCPostProcessingSpecView.h"
#include "CCAnalysisResultsView.h"
#include "LMLicenseManager.h"

////////////////////////////////////////////////////////////
// Classe CCLearningProblemClusterExtractionView
class CCLearningProblemClusterExtractionView : public CCLearningProblemToolView
{
public:
	// Constructeur
	CCLearningProblemClusterExtractionView();
	~CCLearningProblemClusterExtractionView();

	// Actions disponibles
	void ExtractClusters();

	// Parametrage de l'objet edite
	void SetObject(Object* object) override;
};
