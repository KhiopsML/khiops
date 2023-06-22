// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2021-04-25 11:10:57
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "KWAttributeSpecView.h"

KWAttributeSpecView::KWAttributeSpecView()
{
	SetIdentifier("KWAttributeSpec");
	SetLabel("Variable");
	AddBooleanField("Used", "Used", false);
	AddStringField("Type", "Type", "");
	AddStringField("Name", "Name", "");
	AddBooleanField("Derived", "Derived", false);
	AddStringField("MetaData", "Meta-data", "");
	AddStringField("Label", "Label", "");

	// Parametrage des styles;
	GetFieldAt("Used")->SetStyle("CheckBox");
	GetFieldAt("Type")->SetStyle("EditableComboBox");
	GetFieldAt("Derived")->SetStyle("CheckBox");

	// ## Custom constructor

	// Liste des types d'attributs stockes
	ALString sStoredTypesParameter;
	ALString sStoredTypesLabel;
	int i;
	ALString sTmp;

	// Liste des types stockes pour le parametrage et les info-bulles
	for (i = 0; i < KWType::None; i++)
	{
		if (KWType::IsStored(i))
		{
			if (sStoredTypesParameter != "")
			{
				sStoredTypesParameter += "\n";
				sStoredTypesLabel += ", ";
			}
			sStoredTypesParameter += KWType::ToString(i);
			sStoredTypesLabel += KWType::ToString(i);
		}
	}

	// Liste des valeurs de la combo
	GetFieldAt("Type")->SetParameters(sStoredTypesParameter);

	// Info-bulles
	GetFieldAt("Used")->SetHelpText("Indicate whether the variable is used in the analysis.");
	GetFieldAt("Type")->SetHelpText(
	    sTmp + "Type of the variable." + "\n   Standard types: " + sStoredTypesLabel +
	    "\n   Multi-table types: Entity (0-1) relationship, Table (0-n) relationship," +
	    "\n   Algorithmic types: Structure." + "\n" +
	    "\n Type can be changed only for standard types, if the related variable is not" +
	    "\n   an input or output of a derivation rule, and has no meta-data format." +
	    "\n The dictionary file must be edited directly for complex modifications.");
	GetFieldAt("Name")->SetHelpText("Name of variable.");
	GetFieldAt("Derived")->SetHelpText("Indicate whether the variable if native (in the file)"
					   "\n or derived (computed using a formula).");
	GetFieldAt("MetaData")->SetHelpText("Meta-data for additional information related to the variable.");
	GetFieldAt("Label")->SetHelpText("Label of the variable.");

	// ##
}

KWAttributeSpecView::~KWAttributeSpecView()
{
	// ## Custom destructor

	// ##
}

KWAttributeSpec* KWAttributeSpecView::GetKWAttributeSpec()
{
	require(objValue != NULL);
	return cast(KWAttributeSpec*, objValue);
}

void KWAttributeSpecView::EventUpdate(Object* object)
{
	KWAttributeSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributeSpec*, object);
	editedObject->SetUsed(GetBooleanValueAt("Used"));
	editedObject->SetType(GetStringValueAt("Type"));
	editedObject->SetName(GetStringValueAt("Name"));
	editedObject->SetDerived(GetBooleanValueAt("Derived"));
	editedObject->SetMetaData(GetStringValueAt("MetaData"));
	editedObject->SetLabel(GetStringValueAt("Label"));

	// ## Custom update

	// ##
}

void KWAttributeSpecView::EventRefresh(Object* object)
{
	KWAttributeSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWAttributeSpec*, object);
	SetBooleanValueAt("Used", editedObject->GetUsed());
	SetStringValueAt("Type", editedObject->GetType());
	SetStringValueAt("Name", editedObject->GetName());
	SetBooleanValueAt("Derived", editedObject->GetDerived());
	SetStringValueAt("MetaData", editedObject->GetMetaData());
	SetStringValueAt("Label", editedObject->GetLabel());

	// ## Custom refresh

	// ##
}

const ALString KWAttributeSpecView::GetClassLabel() const
{
	return "Variable";
}

// ## Method implementation

// ##
