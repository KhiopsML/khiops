// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KIModelServiceView.h"

KIModelServiceView::KIModelServiceView()
{
	SetIdentifier("KIModelService");
	SetLabel("Interpretation service");
	AddStringField("PredictorClassName", "Predictor dictionary", "");
	AddIntField("PredictorAttributeNumber", "Number of predictor variables", 0);

	// ## Custom constructor

	// ##
}

KIModelServiceView::~KIModelServiceView()
{
	// ## Custom destructor

	// ##
}

KIModelService* KIModelServiceView::GetKIModelService()
{
	require(objValue != NULL);
	return cast(KIModelService*, objValue);
}

void KIModelServiceView::EventUpdate(Object* object)
{
	KIModelService* editedObject;

	require(object != NULL);

	editedObject = cast(KIModelService*, object);
	editedObject->SetPredictorClassName(GetStringValueAt("PredictorClassName"));
	editedObject->SetPredictorAttributeNumber(GetIntValueAt("PredictorAttributeNumber"));

	// ## Custom update

	// ##
}

void KIModelServiceView::EventRefresh(Object* object)
{
	KIModelService* editedObject;

	require(object != NULL);

	editedObject = cast(KIModelService*, object);
	SetStringValueAt("PredictorClassName", editedObject->GetPredictorClassName());
	SetIntValueAt("PredictorAttributeNumber", editedObject->GetPredictorAttributeNumber());

	// ## Custom refresh

	// ##
}

const ALString KIModelServiceView::GetClassLabel() const
{
	return "Interpretation service";
}

// ## Method implementation

// ##
