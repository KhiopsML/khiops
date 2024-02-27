// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWEvaluatedPredictorSpecView.h"

KWEvaluatedPredictorSpecView::KWEvaluatedPredictorSpecView()
{
	SetIdentifier("KWEvaluatedPredictorSpec");
	SetLabel("Evaluated predictor");
	AddBooleanField("Evaluated", "Evaluated", false);
	AddStringField("PredictorType", "Predictor", "");
	AddStringField("PredictorName", "Name", "");
	AddStringField("ClassName", "Dictionary", "");
	AddStringField("TargetAttributeName", "Target variable", "");

	// Parametrage des styles;
	GetFieldAt("Evaluated")->SetStyle("CheckBox");

	// ## Custom constructor

	// Info-bulles
	GetFieldAt("Evaluated")->SetHelpText("Indicate whether to evaluate the predictor.");
	GetFieldAt("PredictorType")->SetHelpText("Type of predictor: Classifier or Regressor.");
	GetFieldAt("PredictorName")->SetHelpText("Label of the predictor.");
	GetFieldAt("ClassName")->SetHelpText("Name of the predictor dictionary.");
	GetFieldAt("TargetAttributeName")
	    ->SetHelpText("Name of the target variable, for classification or regression.");

	// ##
}

KWEvaluatedPredictorSpecView::~KWEvaluatedPredictorSpecView()
{
	// ## Custom destructor

	// ##
}

KWEvaluatedPredictorSpec* KWEvaluatedPredictorSpecView::GetKWEvaluatedPredictorSpec()
{
	require(objValue != NULL);
	return cast(KWEvaluatedPredictorSpec*, objValue);
}

void KWEvaluatedPredictorSpecView::EventUpdate(Object* object)
{
	KWEvaluatedPredictorSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWEvaluatedPredictorSpec*, object);
	editedObject->SetEvaluated(GetBooleanValueAt("Evaluated"));
	editedObject->SetPredictorType(GetStringValueAt("PredictorType"));
	editedObject->SetPredictorName(GetStringValueAt("PredictorName"));
	editedObject->SetClassName(GetStringValueAt("ClassName"));
	editedObject->SetTargetAttributeName(GetStringValueAt("TargetAttributeName"));

	// ## Custom update

	// ##
}

void KWEvaluatedPredictorSpecView::EventRefresh(Object* object)
{
	KWEvaluatedPredictorSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWEvaluatedPredictorSpec*, object);
	SetBooleanValueAt("Evaluated", editedObject->GetEvaluated());
	SetStringValueAt("PredictorType", editedObject->GetPredictorType());
	SetStringValueAt("PredictorName", editedObject->GetPredictorName());
	SetStringValueAt("ClassName", editedObject->GetClassName());
	SetStringValueAt("TargetAttributeName", editedObject->GetTargetAttributeName());

	// ## Custom refresh

	// ##
}

const ALString KWEvaluatedPredictorSpecView::GetClassLabel() const
{
	return "Evaluated predictor";
}

// ## Method implementation

// ##
