// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// Thu Jul 15 10:55:39 2004
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "UITestClassSpecView.h"

UITestClassSpecView::UITestClassSpecView()
{
	SetIdentifier("UITestClassSpec");
	SetLabel("Test UI (class spec)");
	AddStringField("ClassName", "Nom", "");
	AddIntField("AttributeNumber", "Attributs", 0);
	AddIntField("SymbolAttributeNumber", "Modaux", 0);
	AddIntField("ContinuousAttributeNumber", "Continus", 0);
	AddIntField("DerivedAttributeNumber", "Calcules", 0);

	// ## Custom constructor

	// Parametrage des styles
	GetFieldAt("AttributeNumber")->SetStyle("Spinner");

	// ##
}

UITestClassSpecView::~UITestClassSpecView()
{
	// ## Custom destructor

	// ##
}

void UITestClassSpecView::EventUpdate(Object* object)
{
	UITestClassSpec* editedObject;

	require(object != NULL);

	editedObject = cast(UITestClassSpec*, object);
	editedObject->SetClassName(GetStringValueAt("ClassName"));
	editedObject->SetAttributeNumber(GetIntValueAt("AttributeNumber"));
	editedObject->SetSymbolAttributeNumber(GetIntValueAt("SymbolAttributeNumber"));
	editedObject->SetContinuousAttributeNumber(GetIntValueAt("ContinuousAttributeNumber"));
	editedObject->SetDerivedAttributeNumber(GetIntValueAt("DerivedAttributeNumber"));

	// ## Custom update

	// ##
}

void UITestClassSpecView::EventRefresh(Object* object)
{
	UITestClassSpec* editedObject;

	require(object != NULL);

	editedObject = cast(UITestClassSpec*, object);
	SetStringValueAt("ClassName", editedObject->GetClassName());
	SetIntValueAt("AttributeNumber", editedObject->GetAttributeNumber());
	SetIntValueAt("SymbolAttributeNumber", editedObject->GetSymbolAttributeNumber());
	SetIntValueAt("ContinuousAttributeNumber", editedObject->GetContinuousAttributeNumber());
	SetIntValueAt("DerivedAttributeNumber", editedObject->GetDerivedAttributeNumber());

	// ## Custom refresh

	// ##
}

const ALString UITestClassSpecView::GetClassLabel() const
{
	return "Test UI (class spec)";
}

// ## Method implementation

// ##
