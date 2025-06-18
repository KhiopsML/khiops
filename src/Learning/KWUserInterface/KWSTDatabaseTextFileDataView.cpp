// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWSTDatabaseTextFileDataView.h"

KWSTDatabaseTextFileDataView::KWSTDatabaseTextFileDataView()
{
	SetIdentifier("KWSTDatabaseTextFileData");
	SetLabel("Data");
	AddStringField("DatabaseName", "Data table file", "");
	AddBooleanField("HeaderLineUsed", "Header line used", false);
	AddCharField("FieldSeparator", "Field separator", ' ');

	// Parametrage des styles;
	GetFieldAt("DatabaseName")->SetStyle("FileChooser");
	GetFieldAt("HeaderLineUsed")->SetStyle("CheckBox");

	// ## Custom constructor

	// Specialisation des extensions
	GetFieldAt("DatabaseName")->SetParameters("Data\ntxt\ncsv");

	// Ajout d'une fiche de detection de format
	AddCardField("DatabaseFormatDetector", "Detect database format", new KWDatabaseFormatDetectorView);
	MoveFieldBefore("DatabaseFormatDetector", "HeaderLineUsed");

	// Info-bulles
	GetFieldAt("DatabaseName")->SetHelpText("Name of the database file.");
	GetFieldAt("HeaderLineUsed")->SetHelpText("Use of a header line in the file that contains the variables names");
	GetFieldAt("FieldSeparator")
	    ->SetHelpText("Character used as field separator in the file."
			  "\n It can be space (S), semi-colon (;), comma (,) or any character."
			  "\n By default, if nothing is specified, the tabulation is used as the field separator."
			  "\n Fields can contain separator chars provided that they are surrounded by double quotes "
			  "(see documentation).");

	// ##
}

KWSTDatabaseTextFileDataView::~KWSTDatabaseTextFileDataView()
{
	// ## Custom destructor

	// ##
}

KWSTDatabaseTextFile* KWSTDatabaseTextFileDataView::GetKWSTDatabaseTextFile()
{
	require(objValue != NULL);
	return cast(KWSTDatabaseTextFile*, objValue);
}

void KWSTDatabaseTextFileDataView::EventUpdate(Object* object)
{
	KWSTDatabaseTextFile* editedObject;

	require(object != NULL);

	editedObject = cast(KWSTDatabaseTextFile*, object);
	editedObject->SetDatabaseName(GetStringValueAt("DatabaseName"));
	editedObject->SetHeaderLineUsed(GetBooleanValueAt("HeaderLineUsed"));
	editedObject->SetFieldSeparator(GetCharValueAt("FieldSeparator"));

	// ## Custom update

	// Transformation du separateur de champ blanc en TAB et 'S' en space pour la mise a jour
	if (editedObject->GetFieldSeparator() == ' ')
		editedObject->SetFieldSeparator('\t');
	else if (editedObject->GetFieldSeparator() == 'S')
		editedObject->SetFieldSeparator(' ');

	// ##
}

void KWSTDatabaseTextFileDataView::EventRefresh(Object* object)
{
	KWSTDatabaseTextFile* editedObject;

	require(object != NULL);

	editedObject = cast(KWSTDatabaseTextFile*, object);
	SetStringValueAt("DatabaseName", editedObject->GetDatabaseName());
	SetBooleanValueAt("HeaderLineUsed", editedObject->GetHeaderLineUsed());
	SetCharValueAt("FieldSeparator", editedObject->GetFieldSeparator());

	// ## Custom refresh

	// Transformation du separateur de champ TAB en blanc et space en 'S' pour l'interface
	if (editedObject->GetFieldSeparator() == '\t')
		SetCharValueAt("FieldSeparator", ' ');
	else if (editedObject->GetFieldSeparator() == ' ')
		SetCharValueAt("FieldSeparator", 'S');

	// ##
}

const ALString KWSTDatabaseTextFileDataView::GetClassLabel() const
{
	return "Data";
}

// ## Method implementation

void KWSTDatabaseTextFileDataView::SetObject(Object* object)
{
	KWSTDatabaseTextFile* stDatabaseTextFile;

	require(object != NULL);

	// Acces a l'objet edite
	stDatabaseTextFile = cast(KWSTDatabaseTextFile*, object);

	// Parametrage du detecteur de format
	cast(KWDatabaseFormatDetectorView*, GetFieldAt("DatabaseFormatDetector"))->SetDatabase(stDatabaseTextFile);

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

KWSTDatabaseTextFile* KWSTDatabaseTextFileDataView::GetSTDatabaseTextFile()
{
	require(objValue != NULL);
	return cast(KWSTDatabaseTextFile*, objValue);
}

void KWSTDatabaseTextFileDataView::AddShowFirstLinesAction()
{
	require(not IsOpened());
	require(GetActionIndex("ShowFirstLines") == -1);

	AddAction("ShowFirstLines", "Show first lines", (ActionMethod)(&KWSTDatabaseTextFileDataView::ShowFirstLines));
	GetActionAt("ShowFirstLines")->SetStyle("Button");

	// Info-bulles
	GetActionAt("ShowFirstLines")->SetHelpText("Show first lines of data table file in log window.");

	// Short cuts
	GetActionAt("ShowFirstLines")->SetShortCut('S');
}

void KWSTDatabaseTextFileDataView::ShowFirstLines()
{
	KWDatabaseFormatDetector databaseFormatDetector;

	// Utilisation du service d'affichage des premiere lignes du detecteur de format
	databaseFormatDetector.SetDatabase(GetSTDatabaseTextFile());
	databaseFormatDetector.ShowFirstLines(10);
}

// ##
