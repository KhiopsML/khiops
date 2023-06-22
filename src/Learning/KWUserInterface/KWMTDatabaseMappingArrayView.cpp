// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2021-04-25 11:10:56
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "KWMTDatabaseMappingArrayView.h"

KWMTDatabaseMappingArrayView::KWMTDatabaseMappingArrayView()
{
	SetIdentifier("Array.KWMTDatabaseMapping");
	SetLabel("Multi-table mappings");
	AddStringField("DataPath", "Data path", "");
	AddStringField("DataPathClassName", "Data root", "");
	AddStringField("DataPathAttributeNames", "Path", "");
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

	// Ce champ est invisible: il est decompose en DataPathClass et DataPathAttributes pour l'interface
	GetFieldAt("DataPath")->SetVisible(false);

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
	editedObject->SetDataPathClassName(GetStringValueAt("DataPathClassName"));
	editedObject->SetDataPathAttributeNames(GetStringValueAt("DataPathAttributeNames"));
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
	SetStringValueAt("DataPathClassName", editedObject->GetDataPathClassName());
	SetStringValueAt("DataPathAttributeNames", editedObject->GetDataPathAttributeNames());
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
