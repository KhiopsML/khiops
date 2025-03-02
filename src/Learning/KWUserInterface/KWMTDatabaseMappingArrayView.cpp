// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWMTDatabaseMappingArrayView.h"

KWMTDatabaseMappingArrayView::KWMTDatabaseMappingArrayView()
{
	SetIdentifier("Array.KWMTDatabaseMapping");
	SetLabel("Multi-table mappings");
	AddStringField("DataPath", "Data path", "");
	AddStringField("ClassName", "Dictionary", "");
	AddStringField("DataTableName", "Data table file", "");

	// Card and help prameters
	SetItemView(new KWMTDatabaseMappingView);
	CopyCardHelpTexts();

	// Parametrage des styles;
	GetFieldAt("DataTableName")->SetStyle("FileChooser");

	// ## Custom constructor

	// Specialisation des extensions
	GetFieldAt("DataTableName")->SetParameters("Data\ntxt\ncsv");

	// Champ a utiliser comme identifiant pour les selections
	SetKeyFieldId("DataPath");

	// ##
}

KWMTDatabaseMappingArrayView::~KWMTDatabaseMappingArrayView()
{
	// ## Custom destructor

	// ##
}

Object* KWMTDatabaseMappingArrayView::EventNew()
{
	return new KWMTDatabaseMapping;
}

void KWMTDatabaseMappingArrayView::EventUpdate(Object* object)
{
	KWMTDatabaseMapping* editedObject;

	require(object != NULL);

	editedObject = cast(KWMTDatabaseMapping*, object);
	editedObject->SetClassName(GetStringValueAt("ClassName"));
	editedObject->SetDataTableName(GetStringValueAt("DataTableName"));

	// ## Custom update

	// ##
}

void KWMTDatabaseMappingArrayView::EventRefresh(Object* object)
{
	KWMTDatabaseMapping* editedObject;

	require(object != NULL);

	editedObject = cast(KWMTDatabaseMapping*, object);
	SetStringValueAt("DataPath", editedObject->GetDataPath());
	SetStringValueAt("ClassName", editedObject->GetClassName());
	SetStringValueAt("DataTableName", editedObject->GetDataTableName());

	// ## Custom refresh

	// ##
}

const ALString KWMTDatabaseMappingArrayView::GetClassLabel() const
{
	return "Multi-table mappings";
}

// ## Method implementation

// ##
