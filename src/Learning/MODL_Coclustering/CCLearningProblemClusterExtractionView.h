// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"

#include "CCLearningProblem.h"

#include "CCLearningProblemToolView.h"
#include "CCPostProcessingSpecView.h"

////////////////////////////////////////////////////////////
// Classe CCLearningProblemClusterExtractionView
class CCLearningProblemClusterExtractionView : public CCLearningProblemToolView
{
public:
	// Constructeur
	CCLearningProblemClusterExtractionView();
	~CCLearningProblemClusterExtractionView();

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Actions disponibles
	void ExtractClusters();

	// Parametrage de l'objet edite
	void SetObject(Object* object) override;
};
