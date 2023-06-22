// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "SampleView.h"

SampleView::SampleView()
{
	SetIdentifier("Sample");
	SetLabel("Sample");
	AddStringField("First", "Key first field", "");
	AddIntField("Second", "Key second field", 0);
	AddStringField("String", "String", "");
	AddIntField("Integer", "Integer", 0);
	AddIntField("Transient1", "Transient 1", 0);
	AddDoubleField("Double", "Double", 0);
	AddBooleanField("Boolean", "Boolean", false);
	AddIntField("Derived1", "Derived 1", 0);
	AddStringField("Derived2", "Derived 2", "");

	// Parametrage des styles;
	GetFieldAt("Boolean")->SetStyle("CheckBox");

	// ## Custom constructor

	// Aide en ligne
	SetHelpText("Help text sample");
	GetFieldAt("First")->SetHelpText("First help text sample");
	GetFieldAt("Second")->SetHelpText("Second help text sample");

	// ##
}

SampleView::~SampleView()
{
	// ## Custom destructor

	// ##
}

Sample* SampleView::GetSample()
{
	require(objValue != NULL);
	return cast(Sample*, objValue);
}

void SampleView::EventUpdate(Object* object)
{
	Sample* editedObject;

	require(object != NULL);

	editedObject = cast(Sample*, object);
	editedObject->SetFirst(GetStringValueAt("First"));
	editedObject->SetSecond(GetIntValueAt("Second"));
	editedObject->SetString(GetStringValueAt("String"));
	editedObject->SetInteger(GetIntValueAt("Integer"));
	editedObject->SetTransient1(GetIntValueAt("Transient1"));
	editedObject->SetDouble(GetDoubleValueAt("Double"));
	editedObject->SetBoolean(GetBooleanValueAt("Boolean"));
	editedObject->SetDerived1(GetIntValueAt("Derived1"));
	editedObject->SetDerived2(GetStringValueAt("Derived2"));

	// ## Custom update

	// ##
}

void SampleView::EventRefresh(Object* object)
{
	Sample* editedObject;

	require(object != NULL);

	editedObject = cast(Sample*, object);
	SetStringValueAt("First", editedObject->GetFirst());
	SetIntValueAt("Second", editedObject->GetSecond());
	SetStringValueAt("String", editedObject->GetString());
	SetIntValueAt("Integer", editedObject->GetInteger());
	SetIntValueAt("Transient1", editedObject->GetTransient1());
	SetDoubleValueAt("Double", editedObject->GetDouble());
	SetBooleanValueAt("Boolean", editedObject->GetBoolean());
	SetIntValueAt("Derived1", editedObject->GetDerived1());
	SetStringValueAt("Derived2", editedObject->GetDerived2());

	// ## Custom refresh

	// ##
}

const ALString SampleView::GetClassLabel() const
{
	return "Sample";
}

// ## Method implementation

// ##