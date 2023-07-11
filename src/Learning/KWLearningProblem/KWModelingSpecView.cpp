// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWModelingSpecView.h"

KWModelingSpecView::KWModelingSpecView()
{
	SetIdentifier("KWModelingSpec");
	SetLabel("Predictors");

	// Ajout de sous-fiches
	AddCardField("ConstructionSpec", "Feature engineering", new KWAttributeConstructionSpecView);
	AddCardField("AdvancedSpec", "Advanced predictor parameters", new KWModelingAdvancedSpecView);

	// Passage en ergonomie onglets
	SetStyle("TabbedPanes");

	// Short cuts
	GetFieldAt("ConstructionSpec")->SetShortCut('F');
	GetFieldAt("AdvancedSpec")->SetShortCut('V');

#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10: champs obsolete dans cette fiche, conserves de facon cachee en V10 pour compatibilite
		// ascendante des scenarios Champs a supprimer a terme de cette vue, et les deux  champs sont a
		// supprimer de KWModelingSpec.dd pour la generation de code
		AddBooleanField("BaselinePredictor", "Baseline predictor", false);
		AddIntField("UnivariatePredictorNumber", "Number of univariate predictors", 0);
		//
		GetFieldAt("BaselinePredictor")->SetStyle("CheckBox");
		GetFieldAt("UnivariatePredictorNumber")->SetStyle("Spinner");
		//
		cast(UIIntElement*, GetFieldAt("UnivariatePredictorNumber"))->SetMinValue(0);
		cast(UIIntElement*, GetFieldAt("UnivariatePredictorNumber"))->SetMaxValue(100000);
		//
		GetFieldAt("BaselinePredictor")
		    ->SetHelpText("Build a base line predictor"
				  "\n The baseline classifier predicts the train majority class."
				  "\n The baseline regressor predicts the train mean of the target variable.");
		GetFieldAt("UnivariatePredictorNumber")
		    ->SetHelpText("Number of univariate predictors to build."
				  "\n The univariate predictors are chosen according to their predictive importance,"
				  "\n which is assessed during the analysis of the train database.");
		//
		GetFieldAt("BaselinePredictor")->SetVisible(false);
		GetFieldAt("UnivariatePredictorNumber")->SetVisible(false);
	}
#endif // DEPRECATED_V10
}

KWModelingSpecView::~KWModelingSpecView() {}

KWModelingSpec* KWModelingSpecView::GetKWModelingSpec()
{
	require(objValue != NULL);
	return cast(KWModelingSpec*, objValue);
}

void KWModelingSpecView::EventUpdate(Object* object)
{
	KWModelingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWModelingSpec*, object);

#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10: champs obsolete dans cette fiche, conserves de facon cachee en V10 pour compatibilite
		// ascendante des scenarios Mise a jour que pour la version obsolete de l'ongelt
		if (GetObjectLabel().Find("deprecated") >= 0)
		{
			editedObject->SetBaselinePredictor(GetBooleanValueAt("BaselinePredictor"));
			editedObject->SetUnivariatePredictorNumber(GetIntValueAt("UnivariatePredictorNumber"));
		}
	}
#endif // DEPRECATED_V10
}

void KWModelingSpecView::EventRefresh(Object* object)
{
	KWModelingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWModelingSpec*, object);

#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10: champs obsolete dans cette fiche, conserves de facon cachee en V10 pour compatibilite
		// ascendante des scenarios Mise a jour que pour la version obsolete de l'ongelt
		if (GetObjectLabel().Find("deprecated") >= 0)
		{
			SetBooleanValueAt("BaselinePredictor", editedObject->GetBaselinePredictor());
			SetIntValueAt("UnivariatePredictorNumber", editedObject->GetUnivariatePredictorNumber());
		}
	}
#endif // DEPRECATED_V10
}

const ALString KWModelingSpecView::GetClassLabel() const
{
	return "Predictors";
}

void KWModelingSpecView::SetObject(Object* object)
{
	KWModelingSpec* modelingSpec;

	require(object != NULL);

	// Acces a l'objet edite
	modelingSpec = cast(KWModelingSpec*, object);

	// Parametrages des sous-fiches par les sous-objets
	cast(KWAttributeConstructionSpecView*, GetFieldAt("ConstructionSpec"))
	    ->SetObject(modelingSpec->GetAttributeConstructionSpec());
	cast(KWModelingAdvancedSpecView*, GetFieldAt("AdvancedSpec"))->SetObject(modelingSpec);

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}
