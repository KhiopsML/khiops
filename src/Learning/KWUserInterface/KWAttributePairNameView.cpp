// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWAttributePairNameView.h"

KWAttributePairNameView::KWAttributePairNameView()
{
	SetIdentifier("KWAttributePairName");
	SetLabel("Variable pair");
	AddStringField("FirstName", "First name", "");
	AddStringField("SecondName", "Second name", "");

	// ## Custom constructor

	// Info-bulles
	GetFieldAt("FirstName")->SetHelpText("Name of first variable in the pair");
	GetFieldAt("SecondName")->SetHelpText("Name of second variable in the pair");

	// ##
}

KWAttributePairNameView::~KWAttributePairNameView()
{
	// ## Custom destructor

	// ##
}

KWAttributePairName* KWAttributePairNameView::GetKWAttributePairName()
{
	require(objValue != NULL);
	return cast(KWAttributePairName*, objValue);
}

void KWAttributePairNameView::EventUpdate(Object* object)
{
	KWAttributePairName* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributePairName*, object);
	editedObject->SetFirstName(GetStringValueAt("FirstName"));
	editedObject->SetSecondName(GetStringValueAt("SecondName"));

	// ## Custom update

	// ##
}

void KWAttributePairNameView::EventRefresh(Object* object)
{
	KWAttributePairName* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributePairName*, object);
	SetStringValueAt("FirstName", editedObject->GetFirstName());
	SetStringValueAt("SecondName", editedObject->GetSecondName());

	// ## Custom refresh

	// ##
}

const ALString KWAttributePairNameView::GetClassLabel() const
{
	return "Variable pair";
}

// ## Method implementation

// ##