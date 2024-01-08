// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "PRChildView.h"

PRChildView::PRChildView()
{
	SetIdentifier("PRChild");
	SetLabel("Enfant");
	AddStringField("FirstName", "Prenom", "");
	AddIntField("Age", "Age", 0);

	// ## Custom constructor

	// ##
}

PRChildView::~PRChildView()
{
	// ## Custom destructor

	// ##
}

PRChild* PRChildView::GetPRChild()
{
	require(objValue != NULL);
	return cast(PRChild*, objValue);
}

void PRChildView::EventUpdate(Object* object)
{
	PRChild* editedObject;

	require(object != NULL);

	editedObject = cast(PRChild*, object);
	editedObject->SetFirstName(GetStringValueAt("FirstName"));
	editedObject->SetAge(GetIntValueAt("Age"));

	// ## Custom update

	// ##
}

void PRChildView::EventRefresh(Object* object)
{
	PRChild* editedObject;

	require(object != NULL);

	editedObject = cast(PRChild*, object);
	SetStringValueAt("FirstName", editedObject->GetFirstName());
	SetIntValueAt("Age", editedObject->GetAge());

	// ## Custom refresh

	// ##
}

const ALString PRChildView::GetClassLabel() const
{
	return "Enfant";
}

// ## Method implementation

// ##
