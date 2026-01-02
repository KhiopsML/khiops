// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KIPredictorAttributeView.h"

KIPredictorAttributeView::KIPredictorAttributeView()
{
	SetIdentifier("KIPredictorAttribute");
	SetLabel("Predictor variable");
	AddBooleanField("Used", "Used", false);
	AddStringField("Type", "Type", "");
	AddStringField("Name", "Name", "");
	AddDoubleField("Importance", "Importance", 0);

	// Parametrage des styles;
	GetFieldAt("Used")->SetStyle("CheckBox");

	// ## Custom constructor

	// ##
}

KIPredictorAttributeView::~KIPredictorAttributeView()
{
	// ## Custom destructor

	// ##
}

KIPredictorAttribute* KIPredictorAttributeView::GetKIPredictorAttribute()
{
	require(objValue != NULL);
	return cast(KIPredictorAttribute*, objValue);
}

void KIPredictorAttributeView::EventUpdate(Object* object)
{
	KIPredictorAttribute* editedObject;

	require(object != NULL);

	editedObject = cast(KIPredictorAttribute*, object);
	editedObject->SetUsed(GetBooleanValueAt("Used"));
	editedObject->SetType(GetStringValueAt("Type"));
	editedObject->SetName(GetStringValueAt("Name"));
	editedObject->SetImportance(GetDoubleValueAt("Importance"));

	// ## Custom update

	// ##
}

void KIPredictorAttributeView::EventRefresh(Object* object)
{
	KIPredictorAttribute* editedObject;

	require(object != NULL);

	editedObject = cast(KIPredictorAttribute*, object);
	SetBooleanValueAt("Used", editedObject->GetUsed());
	SetStringValueAt("Type", editedObject->GetType());
	SetStringValueAt("Name", editedObject->GetName());
	SetDoubleValueAt("Importance", editedObject->GetImportance());

	// ## Custom refresh

	// ##
}

const ALString KIPredictorAttributeView::GetClassLabel() const
{
	return "Predictor variable";
}

// ## Method implementation

// ##
