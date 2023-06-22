// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2021-04-25 11:10:56
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "KWMTDatabaseMappingView.h"

KWMTDatabaseMappingView::KWMTDatabaseMappingView()
{
	SetIdentifier("KWMTDatabaseMapping");
	SetLabel("Multi-table mapping");
	AddStringField("DataPath", "Data path", "");
	AddStringField("DataPathClassName", "Data root", "");
	AddStringField("DataPathAttributeNames", "Path", "");
	AddStringField("ClassName", "Dictionary", "");
	AddStringField("DataTableName", "Data table file", "");

	// Parametrage des styles;
	GetFieldAt("DataTableName")->SetStyle("FileChooser");

	// ## Custom constructor

	// Specialisation des extensions
	GetFieldAt("DataTableName")->SetParameters("Data\ntxt\ncsv");

	// Info-bulles
	GetFieldAt("DataPath")->SetHelpText("Full data path of the data table.");
	GetFieldAt("DataPathClassName")->SetHelpText("Name of the dictionary that describes the database.");
	GetFieldAt("DataPathAttributeNames")
	    ->SetHelpText("Variable path from the data root."
			  "\n (multi-tables database only)");
	GetFieldAt("ClassName")
	    ->SetHelpText("Name of the dictionary that describes the data table."
			  "\n (multi-tables database only)");
	GetFieldAt("DataTableName")->SetHelpText("Name of the data table file.");

	// ##
}

KWMTDatabaseMappingView::~KWMTDatabaseMappingView()
{
	// ## Custom destructor

	// ##
}

KWMTDatabaseMapping* KWMTDatabaseMappingView::GetKWMTDatabaseMapping()
{
	require(objValue != NULL);
	return cast(KWMTDatabaseMapping*, objValue);
}

void KWMTDatabaseMappingView::EventUpdate(Object* object)
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

void KWMTDatabaseMappingView::EventRefresh(Object* object)
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

const ALString KWMTDatabaseMappingView::GetClassLabel() const
{
	return "Multi-table mapping";
}

// ## Method implementation

// ##
