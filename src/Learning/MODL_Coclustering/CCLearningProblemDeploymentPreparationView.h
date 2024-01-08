// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"

#include "CCLearningProblem.h"

#include "CCLearningProblemToolView.h"
#include "CCPostProcessingSpecView.h"
#include "CCDeploymentSpecView.h"
#include "CCAnalysisResultsView.h"
#include "LMLicenseManager.h"

////////////////////////////////////////////////////////////
// Classe CCLearningProblemDeploymentPreparationView
class CCLearningProblemDeploymentPreparationView : public CCLearningProblemToolView
{
public:
	// Constructeur
	CCLearningProblemDeploymentPreparationView();
	~CCLearningProblemDeploymentPreparationView();

	////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Actions disponibles
	void PrepareDeployment();

	// Acces au probleme d'apprentissage
	void SetObject(Object* object) override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Calcul d'un parametre constitue de la liste des classes disponible
	const ALString BuildInputListClassNamesParameter();

	// Calcul de la signature du coclustering
	const ALString BuildCoclusteringSignature();

	// Rafraichissement des listes d'aide
	void RefreshHelpLists();

	// Memorisation des caracteristiques des listes d'aides pour
	// optimiser leur rafraichissement
	ALString sAttributeHelpListLastClassName;
	ALString sAttributeHelpListLastCoclusteringSignature;
};
