// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KIModelInterpreterView.h"

KIModelInterpreterView::KIModelInterpreterView()
{
	SetIdentifier("KIModelInterpreter");
	SetLabel("Interpret model");
	AddStringField("ShapleyValues", "Shapley values", "");
	AddIntField("ContributionAttributeNumber", "Number of contribution variables", 0);

	// Parametrage des styles;
	GetFieldAt("ContributionAttributeNumber")->SetStyle("Spinner");

	// ## Custom constructor

	// ##
}

KIModelInterpreterView::~KIModelInterpreterView()
{
	// ## Custom destructor

	// ##
}

KIModelInterpreter* KIModelInterpreterView::GetKIModelInterpreter()
{
	require(objValue != NULL);
	return cast(KIModelInterpreter*, objValue);
}

void KIModelInterpreterView::EventUpdate(Object* object)
{
	KIModelInterpreter* editedObject;

	require(object != NULL);

	KIModelServiceView::EventUpdate(object);
	editedObject = cast(KIModelInterpreter*, object);
	editedObject->SetShapleyValues(GetStringValueAt("ShapleyValues"));
	editedObject->SetContributionAttributeNumber(GetIntValueAt("ContributionAttributeNumber"));

	// ## Custom update

	// ##
}

void KIModelInterpreterView::EventRefresh(Object* object)
{
	KIModelInterpreter* editedObject;

	require(object != NULL);

	KIModelServiceView::EventRefresh(object);
	editedObject = cast(KIModelInterpreter*, object);
	SetStringValueAt("ShapleyValues", editedObject->GetShapleyValues());
	SetIntValueAt("ContributionAttributeNumber", editedObject->GetContributionAttributeNumber());

	// ## Custom refresh

	// ##
}

const ALString KIModelInterpreterView::GetClassLabel() const
{
	return "Interpret model";
}

// ## Method implementation

// ##
