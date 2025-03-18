// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KIPredictorAttributeArrayView.h"

KIPredictorAttributeArrayView::KIPredictorAttributeArrayView()
{
	SetIdentifier("Array.KIPredictorAttribute");
	SetLabel("Predictor variables");
	AddBooleanField("Used", "Used", false);
	AddStringField("Type", "Type", "");
	AddStringField("Name", "Name", "");
	AddDoubleField("Importance", "Importance", 0);

	// Card and help prameters
	SetItemView(new KIPredictorAttributeView);
	CopyCardHelpTexts();

	// Parametrage des styles;
	GetFieldAt("Used")->SetStyle("CheckBox");

	// ## Custom constructor

	// ##
}

KIPredictorAttributeArrayView::~KIPredictorAttributeArrayView()
{
	// ## Custom destructor

	// ##
}

Object* KIPredictorAttributeArrayView::EventNew()
{
	return new KIPredictorAttribute;
}

void KIPredictorAttributeArrayView::EventUpdate(Object* object)
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

void KIPredictorAttributeArrayView::EventRefresh(Object* object)
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

const ALString KIPredictorAttributeArrayView::GetClassLabel() const
{
	return "Predictor variables";
}

// ## Method implementation

// ##
