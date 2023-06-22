// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "PRChildArrayView.h"

PRChildArrayView::PRChildArrayView()
{
	SetIdentifier("Array.PRChild");
	SetLabel("Enfants");
	AddStringField("FirstName", "Prenom", "");
	AddIntField("Age", "Age", 0);

	// Card and help prameters
	SetItemView(new PRChildView);
	CopyCardHelpTexts();

	// ## Custom constructor

	// ##
}

PRChildArrayView::~PRChildArrayView()
{
	// ## Custom destructor

	// ##
}

Object* PRChildArrayView::EventNew()
{
	return new PRChild;
}

void PRChildArrayView::EventUpdate(Object* object)
{
	PRChild* editedObject;

	require(object != NULL);

	editedObject = cast(PRChild*, object);
	editedObject->SetFirstName(GetStringValueAt("FirstName"));
	editedObject->SetAge(GetIntValueAt("Age"));

	// ## Custom update

	// ##
}

void PRChildArrayView::EventRefresh(Object* object)
{
	PRChild* editedObject;

	require(object != NULL);

	editedObject = cast(PRChild*, object);
	SetStringValueAt("FirstName", editedObject->GetFirstName());
	SetIntValueAt("Age", editedObject->GetAge());

	// ## Custom refresh

	// ##
}

const ALString PRChildArrayView::GetClassLabel() const
{
	return "Enfants";
}

// ## Method implementation

// ##