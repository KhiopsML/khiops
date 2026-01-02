// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KDConstructionRuleView.h"

KDConstructionRuleView::KDConstructionRuleView()
{
	SetIdentifier("KDConstructionRule");
	SetLabel("Construction rule");
	AddBooleanField("Used", "Used", false);
	AddStringField("FamilyName", "Family", "");
	AddStringField("Name", "Name", "");
	AddStringField("Label", "Label", "");

	// Parametrage des styles;
	GetFieldAt("Used")->SetStyle("CheckBox");

	// ## Custom constructor

	// Info-bulles
	GetFieldAt("Used")->SetHelpText("Indicate whether the rule is used for automatic variable construction.");
	GetFieldAt("FamilyName")
	    ->SetHelpText("Family of the construction rule "
			  "\n based on the type of the first operand.");
	GetFieldAt("Name")->SetHelpText("Name of the construction rule.");
	GetFieldAt("Label")->SetHelpText("Label of the construction rule.");

	// ##
}

KDConstructionRuleView::~KDConstructionRuleView()
{
	// ## Custom destructor

	// ##
}

KDConstructionRule* KDConstructionRuleView::GetKDConstructionRule()
{
	require(objValue != NULL);
	return cast(KDConstructionRule*, objValue);
}

void KDConstructionRuleView::EventUpdate(Object* object)
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

void KDConstructionRuleView::EventRefresh(Object* object)
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

const ALString KDConstructionRuleView::GetClassLabel() const
{
	return "Construction rule";
}

// ## Method implementation

// ##
