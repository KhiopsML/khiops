// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWAttributeAxisNameView.h"

KWAttributeAxisNameView::KWAttributeAxisNameView()
{
	SetIdentifier("KWAttributeAxisName");
	SetLabel("VariableAxis");
	AddStringField("Attribute", "AttributeName", "");
	AddStringField("AxisName", "AxisName", "");

	// ## Custom constructor

	// ##
}

KWAttributeAxisNameView::~KWAttributeAxisNameView()
{
	// ## Custom destructor

	// ##
}

KWAttributeAxisName* KWAttributeAxisNameView::GetKWAttributeAxisName()
{
	require(objValue != NULL);
	return cast(KWAttributeAxisName*, objValue);
}

void KWAttributeAxisNameView::EventUpdate(Object* object)
{
	KWAttributeAxisName* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributeAxisName*, object);
	editedObject->SetAttributeName(GetStringValueAt("Name"));
	editedObject->SetAxisName(GetStringValueAt("AxisName"));

	// ## Custom update

	// ##
}

void KWAttributeAxisNameView::EventRefresh(Object* object)
{
	KWAttributeAxisName* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributeAxisName*, object);
	SetStringValueAt("Name", editedObject->GetAttributeName());
	SetStringValueAt("AxisName", editedObject->GetAxisName());

	// ## Custom refresh

	// ##
}

const ALString KWAttributeAxisNameView::GetClassLabel() const
{
	return "VariableAxis";
}

// ## Method implementation

// ##
