// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWPredictorEvaluator.h"
#include "KWDatabaseView.h"
#include "KWEvaluatedPredictorSpecArrayView.h"
#include "KWClassAttributeHelpList.h"
#include "KWDatabaseAttributeValuesHelpList.h"
#include "LMLicenseManager.h"

////////////////////////////////////////////////////////////
// Classe KWPredictorEvaluatorView
//    Evaluate predictors
// Editeur de KWPredictorEvaluator
class KWPredictorEvaluatorView : public UIObjectView
{
public:
	// Constructeur
	KWPredictorEvaluatorView();
	~KWPredictorEvaluatorView();

	////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Reimplementation de la methode Open avec l'objet a editer en parametre
	void Open(KWPredictorEvaluator* predictorEvaluator);

	// Action d'evaluation des predicteurs (avec suivi de progression de la tache)
	void EvaluatePredictors();

	// Parametrage de l'objet edite
	void SetObject(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
};
