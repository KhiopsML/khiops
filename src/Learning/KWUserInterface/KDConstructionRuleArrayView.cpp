// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KDConstructionRuleArrayView.h"

KDConstructionRuleArrayView::KDConstructionRuleArrayView()
{
	SetIdentifier("Array.KDConstructionRule");
	SetLabel("Construction rules");
	AddBooleanField("Used", "Used", false);
	AddStringField("FamilyName", "Family", "");
	AddStringField("Name", "Name", "");
	AddStringField("Label", "Label", "");

	// Card and help prameters
	SetItemView(new KDConstructionRuleView);
	CopyCardHelpTexts();

	// Parametrage des styles;
	GetFieldAt("Used")->SetStyle("CheckBox");

	// ## Custom constructor

	// Champ a utiliser pour les selections
	SetKeyFieldId("Name");

	// ##
}

KDConstructionRuleArrayView::~KDConstructionRuleArrayView()
{
	// ## Custom destructor

	// ##
}

Object* KDConstructionRuleArrayView::EventNew()
{
	return new KDConstructionRule;
}

void KDConstructionRuleArrayView::EventUpdate(Object* object)
{
	KDConstructionRule* editedObject;

	require(object != NULL);

	editedObject = cast(KDConstructionRule*, object);
	editedObject->SetUsed(GetBooleanValueAt("Used"));
	editedObject->SetFamilyName(GetStringValueAt("FamilyName"));
	editedObject->SetName(GetStringValueAt("Name"));
	editedObject->SetLabel(GetStringValueAt("Label"));

	// ## Custom update

	// ##
}

void KDConstructionRuleArrayView::EventRefresh(Object* object)
{
	KDConstructionRule* editedObject;

	require(object != NULL);

	editedObject = cast(KDConstructionRule*, object);
	SetBooleanValueAt("Used", editedObject->GetUsed());
	SetStringValueAt("FamilyName", editedObject->GetFamilyName());
	SetStringValueAt("Name", editedObject->GetName());
	SetStringValueAt("Label", editedObject->GetLabel());

	// ## Custom refresh

	// ##
}

const ALString KDConstructionRuleArrayView::GetClassLabel() const
{
	return "Construction rules";
}

// ## Method implementation

// ##
