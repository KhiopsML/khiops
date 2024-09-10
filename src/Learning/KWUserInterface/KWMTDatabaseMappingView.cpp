// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWMTDatabaseMappingView.h"

KWMTDatabaseMappingView::KWMTDatabaseMappingView()
{
	SetIdentifier("KWMTDatabaseMapping");
	SetLabel("Multi-table mapping");
	AddStringField("DataPath", "Data path", "");
	AddStringField("ClassName", "Dictionary", "");
	AddStringField("DataTableName", "Data table file", "");

	// Parametrage des styles;
	GetFieldAt("DataTableName")->SetStyle("FileChooser");

	// ## Custom constructor

	// Specialisation des extensions
	GetFieldAt("DataTableName")->SetParameters("Data\ntxt\ncsv");

	// Info-bulles
	GetFieldAt("DataPath")
	    ->SetHelpText(
		"Data path of the data table (multi-table database only)."
		"\n"
		"\n In a multi-table schema, each data path refers to a Table or Entity variable and identifies a data "
		"table file."
		"\n The main table has an empty data path."
		"\n In a star schema, the data paths are the names of Table or Entity variables for each secondary "
		"table."
		"\n In a snowflake schema, data paths consist of a list of variable names with a '/' separator."
		"\n External tables begin with a data root prefixed with '/', which refers to the name of the "
		"referenced root dictionary.");
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
