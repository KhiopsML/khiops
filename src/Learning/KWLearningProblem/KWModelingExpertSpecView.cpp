// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWModelingExpertSpecView.h"

KWModelingExpertSpecView::KWModelingExpertSpecView()
{
	SetIdentifier("KWModelingExpertSpec");
	SetLabel("Expert predictor parameters");
	AddBooleanField("BaselinePredictor", "Baseline predictor", false);
	AddIntField("UnivariatePredictorNumber", "Number of univariate predictors", 0);
	AddBooleanField("SelectiveNaiveBayesPredictor", "Selective Naive Bayes predictor", false);
	AddBooleanField("NaiveBayesPredictor", "Naive Bayes predictor", false);
	AddBooleanField("DataGridPredictor", "Data Grid predictor", false);

	// Parametrage des styles;
	GetFieldAt("BaselinePredictor")->SetStyle("CheckBox");
	GetFieldAt("UnivariatePredictorNumber")->SetStyle("Spinner");
	GetFieldAt("SelectiveNaiveBayesPredictor")->SetStyle("CheckBox");
	GetFieldAt("NaiveBayesPredictor")->SetStyle("CheckBox");
	GetFieldAt("DataGridPredictor")->SetStyle("CheckBox");

	// ## Custom constructor

	// Plage de valeurs pour le nombre de predicteurs univaries
	cast(UIIntElement*, GetFieldAt("UnivariatePredictorNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("UnivariatePredictorNumber"))->SetMaxValue(100);

	// Declaration des actions avancees
	AddAction("InspectDataGridParameters", "Data Grid parameters",
		  (ActionMethod)(&KWModelingExpertSpecView::InspectDataGridParameters));
	GetActionAt("InspectDataGridParameters")->SetStyle("Button");

	// Info-bulles
	GetFieldAt("BaselinePredictor")
	    ->SetHelpText("Build a base line predictor, only in case of regression"
			  "\n The baseline regressor predicts the train mean of the target variable.");
	GetFieldAt("UnivariatePredictorNumber")
	    ->SetHelpText("Number of univariate predictors to build."
			  "\n The univariate predictors are chosen according to their predictive importance,"
			  "\n which is assessed during the analysis of the train database.");
	GetFieldAt("SelectiveNaiveBayesPredictor")
	    ->SetHelpText(
		"Build a Selective Naive Bayes predictor."
		"\n The Selective Naive Bayes predictor performs a variable selection using a greedy heuristic."
		"\n The selection consists in successive Forward and Backward passes, and is repeated several times"
		"\n in order to optimize variable weights.");
	GetFieldAt("NaiveBayesPredictor")->SetHelpText("Build a Naive Bayes predictor.");
	GetFieldAt("DataGridPredictor")
	    ->SetHelpText("Build a Data Grid predictor."
			  "\n (expert mode only)");
	GetActionAt("InspectDataGridParameters")
	    ->SetHelpText("Inspect advanced parameters for the Data Grid predictor.");

	// Short cuts
	GetActionAt("InspectDataGridParameters")->SetShortCut('I');

	// ##
}

KWModelingExpertSpecView::~KWModelingExpertSpecView()
{
	// ## Custom destructor

	// ##
}

KWModelingSpec* KWModelingExpertSpecView::GetKWModelingSpec()
{
	require(objValue != NULL);
	return cast(KWModelingSpec*, objValue);
}

void KWModelingExpertSpecView::EventUpdate(Object* object)
{
	KWModelingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWModelingSpec*, object);
	editedObject->SetBaselinePredictor(GetBooleanValueAt("BaselinePredictor"));
	editedObject->SetUnivariatePredictorNumber(GetIntValueAt("UnivariatePredictorNumber"));
	editedObject->SetSelectiveNaiveBayesPredictor(GetBooleanValueAt("SelectiveNaiveBayesPredictor"));
	editedObject->SetNaiveBayesPredictor(GetBooleanValueAt("NaiveBayesPredictor"));
	editedObject->SetDataGridPredictor(GetBooleanValueAt("DataGridPredictor"));

	// ## Custom update

	// ##
}

void KWModelingExpertSpecView::EventRefresh(Object* object)
{
	KWModelingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWModelingSpec*, object);
	SetBooleanValueAt("BaselinePredictor", editedObject->GetBaselinePredictor());
	SetIntValueAt("UnivariatePredictorNumber", editedObject->GetUnivariatePredictorNumber());
	SetBooleanValueAt("SelectiveNaiveBayesPredictor", editedObject->GetSelectiveNaiveBayesPredictor());
	SetBooleanValueAt("NaiveBayesPredictor", editedObject->GetNaiveBayesPredictor());
	SetBooleanValueAt("DataGridPredictor", editedObject->GetDataGridPredictor());

	// ## Custom refresh

	// ##
}

const ALString KWModelingExpertSpecView::GetClassLabel() const
{
	return "Expert predictor parameters";
}

// ## Method implementation

void KWModelingExpertSpecView::InspectDataGridParameters()
{
	KWPredictorDataGridView predictorDataGridView;
	KWModelingSpec* modelingSpec;

	// Acces a l'objet edite
	modelingSpec = cast(KWModelingSpec*, GetObject());
	check(modelingSpec);

	// Ouverture de la sous-fiche
	predictorDataGridView.SetObject(modelingSpec->GetPredictorDataGrid());
	predictorDataGridView.Open();
}

// ##
