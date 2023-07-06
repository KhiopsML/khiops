// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class CCLearningProblemView;

#include "UserInterface.h"
#include "CCAnalysisResultsView.h"
#include "CCLearningProblem.h"
#include "CCLearningProblemView.h"
#include "CCLearningProblemClusterExtractionView.h"
#include "CCLearningProblemPostProcessingView.h"
#include "CCLearningProblemDeploymentPreparationView.h"

////////////////////////////////////////////////////////////
// Classe CCLearningProblemActionView
//    Actions d'analyse deportees de CCLearningProblemView
class CCLearningProblemActionView : public UIObjectView
{
public:
	// Constructeur
	CCLearningProblemActionView();
	~CCLearningProblemActionView();

	////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Actions disponibles
	void BuildCoclustering();
	void PostProcessCoclustering();
	void ExtractClusters();
	void PrepareDeployment();

	// Acces au probleme d'apprentissage
	CCLearningProblem* GetLearningProblem();

	// Acces a la vue principale sur le probleme d'apprentissage
	CCLearningProblemView* GetLearningProblemView();

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Initialisation des caracteristiques du coclustering a traiter
	void InitializeInputCoclusteringSpecs();
};
