// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWAttributeNameView.h"

KWAttributeNameView::KWAttributeNameView()
{
	SetIdentifier("KWAttributeName");
	SetLabel("Variable");
	AddStringField("Name", "Name", "");

	// ## Custom constructor

	// ##
}

KWAttributeNameView::~KWAttributeNameView()
{
	// ## Custom destructor

	// ##
}

KWAttributeName* KWAttributeNameView::GetKWAttributeName()
{
	require(objValue != NULL);
	return cast(KWAttributeName*, objValue);
}

void KWAttributeNameView::EventUpdate(Object* object)
{
	KWAttributeName* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributeName*, object);
	editedObject->SetName(GetStringValueAt("Name"));

	// ## Custom update

	// ##
}

void KWAttributeNameView::EventRefresh(Object* object)
{
	KWAttributeName* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributeName*, object);
	SetStringValueAt("Name", editedObject->GetName());

	// ## Custom refresh

	// ##
}

const ALString KWAttributeNameView::GetClassLabel() const
{
	return "Variable";
}

// ## Method implementation

// ##
