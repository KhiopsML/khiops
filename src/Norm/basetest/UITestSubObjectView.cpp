// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2021-01-31 18:14:10
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "UITestSubObjectView.h"

UITestSubObjectView::UITestSubObjectView()
{
	SetIdentifier("UITestSubObject");
	SetLabel("Test sub-object");
	AddStringField("Info", "Info", "");

	// ## Custom constructor

	// ##
}

UITestSubObjectView::~UITestSubObjectView()
{
	// ## Custom destructor

	// ##
}

UITestSubObject* UITestSubObjectView::GetUITestSubObject()
{
	require(objValue != NULL);
	return cast(UITestSubObject*, objValue);
}

void UITestSubObjectView::EventUpdate(Object* object)
{
	UITestSubObject* editedObject;

	require(object != NULL);

	editedObject = cast(UITestSubObject*, object);
	editedObject->SetInfo(GetStringValueAt("Info"));

	// ## Custom update

	// ##
}

void UITestSubObjectView::EventRefresh(Object* object)
{
	UITestSubObject* editedObject;

	require(object != NULL);

	editedObject = cast(UITestSubObject*, object);
	SetStringValueAt("Info", editedObject->GetInfo());

	// ## Custom refresh

	// ##
}

const ALString UITestSubObjectView::GetClassLabel() const
{
	return "Test sub-object";
}

// ## Method implementation

// ##
