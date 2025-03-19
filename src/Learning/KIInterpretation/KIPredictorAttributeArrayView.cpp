// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KIPredictorAttributeArrayView.h"

KIPredictorAttributeArrayView::KIPredictorAttributeArrayView()
{
	SetIdentifier("Array.KIPredictorAttribute");
	SetLabel("Predictor variables");
	AddBooleanField("Used", "Used", false);
	AddStringField("Type", "Type", "");
	AddStringField("Name", "Name", "");
	AddDoubleField("Importance", "Importance", 0);

	// Card and help prameters
	SetItemView(new KIPredictorAttributeView);
	CopyCardHelpTexts();

	// Parametrage des styles;
	GetFieldAt("Used")->SetStyle("CheckBox");

	// ## Custom constructor

	// Ajout des actions
	AddAction("SelectAll", "Select all", (ActionMethod)(&KIPredictorAttributeArrayView::SelectAll));
	AddAction("UnselectAll", "Unselect all", (ActionMethod)(&KIPredictorAttributeArrayView::UnselectAll));
	GetActionAt("SelectAll")->SetStyle("Button");
	GetActionAt("UnselectAll")->SetStyle("Button");

	// Info-bulles
	GetFieldAt("Used")->SetHelpText("Indicate whether the variable is used as a lever variabe.");
	GetFieldAt("Type")->SetHelpText("Type of the variable.");
	GetFieldAt("Name")->SetHelpText("Name of the variable.");
	GetFieldAt("Importance")->SetHelpText("Importance of the variable.");
	GetActionAt("SelectAll")->SetHelpText("Select all variables.");
	GetActionAt("UnselectAll")->SetHelpText("Unselect all variables.");

	// Short cuts
	GetActionAt("SelectAll")->SetShortCut('S');
	GetActionAt("UnselectAll")->SetShortCut('U');

	// ##
}

KIPredictorAttributeArrayView::~KIPredictorAttributeArrayView()
{
	// ## Custom destructor

	// ##
}

Object* KIPredictorAttributeArrayView::EventNew()
{
	return new KIPredictorAttribute;
}

void KIPredictorAttributeArrayView::EventUpdate(Object* object)
{
	KIPredictorAttribute* editedObject;

	require(object != NULL);

	editedObject = cast(KIPredictorAttribute*, object);
	editedObject->SetUsed(GetBooleanValueAt("Used"));
	editedObject->SetType(GetStringValueAt("Type"));
	editedObject->SetName(GetStringValueAt("Name"));
	editedObject->SetImportance(GetDoubleValueAt("Importance"));

	// ## Custom update

	// ##
}

void KIPredictorAttributeArrayView::EventRefresh(Object* object)
{
	KIPredictorAttribute* editedObject;

	require(object != NULL);

	editedObject = cast(KIPredictorAttribute*, object);
	SetBooleanValueAt("Used", editedObject->GetUsed());
	SetStringValueAt("Type", editedObject->GetType());
	SetStringValueAt("Name", editedObject->GetName());
	SetDoubleValueAt("Importance", editedObject->GetImportance());

	// ## Custom refresh

	// ##
}

const ALString KIPredictorAttributeArrayView::GetClassLabel() const
{
	return "Predictor variables";
}

// ## Method implementation

void KIPredictorAttributeArrayView::SelectAll()
{
	ObjectArray* oaLeverAttribute;
	KIPredictorAttribute* leverAttribute;
	int i;

	// On met toutes les variables leviers en used
	oaLeverAttribute = GetObjectArray();
	for (i = 0; i < oaLeverAttribute->GetSize(); i++)
	{
		leverAttribute = cast(KIPredictorAttribute*, oaLeverAttribute->GetAt(i));
		leverAttribute->SetUsed(true);
	}
}

void KIPredictorAttributeArrayView::UnselectAll()
{
	ObjectArray* oaLeverAttribute;
	KIPredictorAttribute* leverAttribute;
	int i;

	// On met toutes les variables leviers en unused
	oaLeverAttribute = GetObjectArray();
	for (i = 0; i < oaLeverAttribute->GetSize(); i++)
	{
		leverAttribute = cast(KIPredictorAttribute*, oaLeverAttribute->GetAt(i));
		leverAttribute->SetUsed(false);
	}
}

// ##
