// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWTrainParametersView.h"

KWTrainParametersView::KWTrainParametersView()
{
	SetIdentifier("KWTrainParameters");
	SetLabel("Train parameters");
	AddIntField("MaxEvaluatedAttributeNumber", "Max number of evaluated variables", 0);
	AddStringField("ClassifierCriterion", "Classification criterion", "");

	// Parametrage des styles;
	GetFieldAt("MaxEvaluatedAttributeNumber")->SetStyle("Spinner");
	GetFieldAt("ClassifierCriterion")->SetStyle("ComboBox");

	// ## Custom constructor

	// Parametrage specifique
	cast(UIIntElement*, GetFieldAt("MaxEvaluatedAttributeNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxEvaluatedAttributeNumber"))->SetMaxValue(1000000);
	GetFieldAt("ClassifierCriterion")->SetParameters("none\nAccuracy\nBalancedAccuracy");

	// Le parametrage expert n'est visible qu'en mode expert
	GetFieldAt("ClassifierCriterion")->SetVisible(GetLearningExpertMode());

	// Info-bulles
	GetFieldAt("MaxEvaluatedAttributeNumber")
	    ->SetHelpText("Max number of variables originating from the data preparation, to use as"
			  "\n input variables in the multivariate selection of the Selective Naive Bayes predictor."
			  "\n The evaluated variables are those having the highest predictive importance (level)."
			  "\n This parameter allows to simplify and speed up the training phase"
			  "\n (default: 0, means that all variables with non-zero level are evaluated).");
	GetFieldAt("ClassifierCriterion")
	    ->SetHelpText("Classification criterion optimized after training in a post-processing step");

	// ##
}

KWTrainParametersView::~KWTrainParametersView()
{
	// ## Custom destructor

	// ##
}

KWTrainParameters* KWTrainParametersView::GetKWTrainParameters()
{
	require(objValue != NULL);
	return cast(KWTrainParameters*, objValue);
}

void KWTrainParametersView::EventUpdate(Object* object)
{
	KWTrainParameters* editedObject;

	require(object != NULL);

	editedObject = cast(KWTrainParameters*, object);
	editedObject->SetMaxEvaluatedAttributeNumber(GetIntValueAt("MaxEvaluatedAttributeNumber"));
	editedObject->SetClassifierCriterion(GetStringValueAt("ClassifierCriterion"));

	// ## Custom update

	// ##
}

void KWTrainParametersView::EventRefresh(Object* object)
{
	KWTrainParameters* editedObject;

	require(object != NULL);

	editedObject = cast(KWTrainParameters*, object);
	SetIntValueAt("MaxEvaluatedAttributeNumber", editedObject->GetMaxEvaluatedAttributeNumber());
	SetStringValueAt("ClassifierCriterion", editedObject->GetClassifierCriterion());

	// ## Custom refresh

	// ##
}

const ALString KWTrainParametersView::GetClassLabel() const
{
	return "Train parameters";
}

// ## Method implementation

// ##
