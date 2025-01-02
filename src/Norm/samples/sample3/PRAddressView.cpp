// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "PRAddressView.h"

PRAddressView::PRAddressView()
{
	SetIdentifier("PRAddress");
	SetLabel("Adresse");
	AddStringField("ZipCode", "Code postal", "");
	AddStringField("City", "Ville", "");

	// ## Custom constructor

	// ##
}

PRAddressView::~PRAddressView()
{
	// ## Custom destructor

	// ##
}

PRAddress* PRAddressView::GetPRAddress()
{
	require(objValue != NULL);
	return cast(PRAddress*, objValue);
}

void PRAddressView::EventUpdate(Object* object)
{
	PRAddress* editedObject;

	require(object != NULL);

	editedObject = cast(PRAddress*, object);
	editedObject->SetZipCode(GetStringValueAt("ZipCode"));
	editedObject->SetCity(GetStringValueAt("City"));

	// ## Custom update

	// ##
}

void PRAddressView::EventRefresh(Object* object)
{
	PRAddress* editedObject;

	require(object != NULL);

	editedObject = cast(PRAddress*, object);
	SetStringValueAt("ZipCode", editedObject->GetZipCode());
	SetStringValueAt("City", editedObject->GetCity());

	// ## Custom refresh

	// ##
}

const ALString PRAddressView::GetClassLabel() const
{
	return "Adresse";
}

// ## Method implementation

// ##
