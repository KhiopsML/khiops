// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWEvaluatedPredictorSpecArrayView.h"

KWEvaluatedPredictorSpecArrayView::KWEvaluatedPredictorSpecArrayView()
{
	SetIdentifier("Array.KWEvaluatedPredictorSpec");
	SetLabel("Evaluated predictors");
	AddBooleanField("Evaluated", "Evaluated", false);
	AddStringField("PredictorType", "Predictor", "");
	AddStringField("PredictorName", "Name", "");
	AddStringField("ClassName", "Dictionary", "");
	AddStringField("TargetAttributeName", "Target variable", "");

	// Card and help prameters
	SetItemView(new KWEvaluatedPredictorSpecView);
	CopyCardHelpTexts();

	// Parametrage des styles;
	GetFieldAt("Evaluated")->SetStyle("CheckBox");

	// ## Custom constructor

	// Champ a utiliser pour les selections
	SetKeyFieldId("ClassName");

	// ##
}

KWEvaluatedPredictorSpecArrayView::~KWEvaluatedPredictorSpecArrayView()
{
	// ## Custom destructor

	// ##
}

Object* KWEvaluatedPredictorSpecArrayView::EventNew()
{
	return new KWEvaluatedPredictorSpec;
}

void KWEvaluatedPredictorSpecArrayView::EventUpdate(Object* object)
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

void KWEvaluatedPredictorSpecArrayView::EventRefresh(Object* object)
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

const ALString KWEvaluatedPredictorSpecArrayView::GetClassLabel() const
{
	return "Evaluated predictors";
}

// ## Method implementation

// ##
