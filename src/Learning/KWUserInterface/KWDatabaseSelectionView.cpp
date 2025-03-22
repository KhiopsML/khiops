// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWDatabaseSelectionView.h"

KWDatabaseSelectionView::KWDatabaseSelectionView()
{
	SetIdentifier("KWDatabaseSelection");
	SetLabel("Selection");
	AddStringField("SelectionAttribute", "Selection variable", "");
	AddStringField("SelectionValue", "Selection value", "");

	// ## Custom constructor

	// Info-bulles
	GetFieldAt("SelectionAttribute")
	    ->SetHelpText("When a selection variable is specified, the records are selected "
			  "\n when the value of their selection variable is equal to the selection value.");
	GetFieldAt("SelectionValue")->SetHelpText("Selection value used when a selection variable is specified.");

	// ##
}

KWDatabaseSelectionView::~KWDatabaseSelectionView()
{
	// ## Custom destructor

	// ##
}

KWDatabase* KWDatabaseSelectionView::GetKWDatabase()
{
	require(objValue != NULL);
	return cast(KWDatabase*, objValue);
}

void KWDatabaseSelectionView::EventUpdate(Object* object)
{
	KWDatabase* editedObject;

	require(object != NULL);

	editedObject = cast(KWDatabase*, object);
	editedObject->SetSelectionAttribute(GetStringValueAt("SelectionAttribute"));
	editedObject->SetSelectionValue(GetStringValueAt("SelectionValue"));

	// ## Custom update

	// ##
}

void KWDatabaseSelectionView::EventRefresh(Object* object)
{
	KWDatabase* editedObject;

	require(object != NULL);

	editedObject = cast(KWDatabase*, object);
	SetStringValueAt("SelectionAttribute", editedObject->GetSelectionAttribute());
	SetStringValueAt("SelectionValue", editedObject->GetSelectionValue());

	// ## Custom refresh

	// ##
}

const ALString KWDatabaseSelectionView::GetClassLabel() const
{
	return "Selection";
}

// ## Method implementation

// ##
