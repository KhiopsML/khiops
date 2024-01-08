// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWClassSpecArrayView.h"

KWClassSpecArrayView::KWClassSpecArrayView()
{
	SetIdentifier("Array.KWClassSpec");
	SetLabel("Dictionarys");
	AddStringField("ClassName", "Name", "");
	AddBooleanField("Root", "Root", false);
	AddStringField("Key", "Key", "");
	AddIntField("AttributeNumber", "Variables", 0);
	AddIntField("SymbolAttributeNumber", "Categorical", 0);
	AddIntField("ContinuousAttributeNumber", "Numerical", 0);
	AddIntField("DerivedAttributeNumber", "Derived", 0);

	// Card and help prameters
	SetItemView(new KWClassSpecView);
	CopyCardHelpTexts();

	// ## Custom constructor

	// La visualisation du parametre Root n'est disponible qu'en mode multitables
	GetFieldAt("Root")->SetVisible(GetLearningMultiTableMode());

	// La cle n'est pas visible en mode tableau
	GetFieldAt("Key")->SetVisible(false);

	// Champ a utiliser pour les selections
	SetKeyFieldId("ClassName");

	// ##
}

KWClassSpecArrayView::~KWClassSpecArrayView()
{
	// ## Custom destructor

	// ##
}

Object* KWClassSpecArrayView::EventNew()
{
	return new KWClassSpec;
}

void KWClassSpecArrayView::EventUpdate(Object* object)
{
	KWClassSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWClassSpec*, object);
	editedObject->SetClassName(GetStringValueAt("ClassName"));
	editedObject->SetRoot(GetBooleanValueAt("Root"));
	editedObject->SetKey(GetStringValueAt("Key"));
	editedObject->SetAttributeNumber(GetIntValueAt("AttributeNumber"));
	editedObject->SetSymbolAttributeNumber(GetIntValueAt("SymbolAttributeNumber"));
	editedObject->SetContinuousAttributeNumber(GetIntValueAt("ContinuousAttributeNumber"));
	editedObject->SetDerivedAttributeNumber(GetIntValueAt("DerivedAttributeNumber"));

	// ## Custom update

	// ##
}

void KWClassSpecArrayView::EventRefresh(Object* object)
{
	KWClassSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWClassSpec*, object);
	SetStringValueAt("ClassName", editedObject->GetClassName());
	SetBooleanValueAt("Root", editedObject->GetRoot());
	SetStringValueAt("Key", editedObject->GetKey());
	SetIntValueAt("AttributeNumber", editedObject->GetAttributeNumber());
	SetIntValueAt("SymbolAttributeNumber", editedObject->GetSymbolAttributeNumber());
	SetIntValueAt("ContinuousAttributeNumber", editedObject->GetContinuousAttributeNumber());
	SetIntValueAt("DerivedAttributeNumber", editedObject->GetDerivedAttributeNumber());

	// ## Custom refresh

	// ##
}

const ALString KWClassSpecArrayView::GetClassLabel() const
{
	return "Dictionarys";
}

// ## Method implementation

// ##
