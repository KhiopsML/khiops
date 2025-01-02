// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWMTDatabaseTextFileView.h"

KWMTDatabaseTextFileView::KWMTDatabaseTextFileView()
{
	SetIdentifier("KWMTDatabaseTextFile");
	SetLabel("Database");
	AddBooleanField("HeaderLineUsed", "Header line used", false);
	AddCharField("FieldSeparator", "Field separator", ' ');

	// Parametrage des styles;
	GetFieldAt("HeaderLineUsed")->SetStyle("CheckBox");

	// ## Custom constructor

	// Variables specifiques
	int nClassNameFieldIndex;
	ALString sFieldId;
	KWMTDatabaseMappingArrayView* multiTableMappingArrayView;

	// Acces a l'identifiant du champ "ClassName"
	nClassNameFieldIndex = GetFieldIndex("ClassName");
	assert(nClassNameFieldIndex >= 0);

	// Deplacement des nouveaux champs vers le haut, pour arriver apres la specification du nom du dictionnaire
	sFieldId = GetFieldAtIndex(nClassNameFieldIndex + 1)->GetIdentifier();
	MoveFieldBefore("HeaderLineUsed", sFieldId);
	MoveFieldBefore("FieldSeparator", sFieldId);

	// Ajout d'une fiche de detection de format
	AddCardField("DatabaseFormatDetector", "Detect database format", new KWDatabaseFormatDetectorView);
	MoveFieldBefore("DatabaseFormatDetector", "HeaderLineUsed");

	// Parametrage de l'editioon multi-tables des fichiers de la base
	ObjectArray oaMultiTableMappingForWrite;

	// Parametrage de la vue sur la mapping multi-table
	multiTableMappingArrayView = new KWMTDatabaseMappingArrayView;
	multiTableMappingArrayView->SetLastColumnExtraWidth(5);
	multiTableMappingArrayView->SetEditable(false);
	multiTableMappingArrayView->GetFieldAt("DataTableName")->SetEditable(true);
	AddListField("DatabaseFiles", "Database files", multiTableMappingArrayView);

	// On passe le parametrage multitable en tete
	sFieldId = GetFieldAtIndex(nClassNameFieldIndex + 1)->GetIdentifier();
	MoveFieldBefore("DatabaseFiles", sFieldId);

	// Info-bulles
	GetFieldAt("HeaderLineUsed")->SetHelpText("Use of a header line in the file that contains the variables names");
	GetFieldAt("FieldSeparator")
	    ->SetHelpText("Character used as field separator in the file."
			  "\n It can be space (S), semi-colon (;), comma (,) or any character."
			  "\n By default, if nothing is specified, the tabulation is used as the field separator."
			  "\n Fields can contain separator chars provided that they are surrounded by double-quotes "
			  "(see documentation).");

	// ##
}

KWMTDatabaseTextFileView::~KWMTDatabaseTextFileView()
{
	// ## Custom destructor

	// ##
}

KWMTDatabaseTextFile* KWMTDatabaseTextFileView::GetKWMTDatabaseTextFile()
{
	require(objValue != NULL);
	return cast(KWMTDatabaseTextFile*, objValue);
}

void KWMTDatabaseTextFileView::EventUpdate(Object* object)
{
	KWMTDatabaseTextFile* editedObject;

	require(object != NULL);

	KWDatabaseView::EventUpdate(object);
	editedObject = cast(KWMTDatabaseTextFile*, object);
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

void KWMTDatabaseTextFileView::EventRefresh(Object* object)
{
	KWMTDatabaseTextFile* editedObject;

	require(object != NULL);

	KWDatabaseView::EventRefresh(object);
	editedObject = cast(KWMTDatabaseTextFile*, object);
	SetBooleanValueAt("HeaderLineUsed", editedObject->GetHeaderLineUsed());
	SetCharValueAt("FieldSeparator", editedObject->GetFieldSeparator());

	// ## Custom refresh

	// Rafraichissement des mappings, qui peuvent avoir change si le dictionnaire a ete modifie
	ResfreshMultiTableMapping();

	// Transformation du separateur de champ TAB en blanc et space en 'S' pour l'interface
	if (editedObject->GetFieldSeparator() == '\t')
		SetCharValueAt("FieldSeparator", ' ');
	else if (editedObject->GetFieldSeparator() == ' ')
		SetCharValueAt("FieldSeparator", 'S');

	// ##
}

const ALString KWMTDatabaseTextFileView::GetClassLabel() const
{
	return "Database";
}

// ## Method implementation

KWDatabaseView* KWMTDatabaseTextFileView::Create() const
{
	return new KWMTDatabaseTextFileView;
}

ALString KWMTDatabaseTextFileView::GetTechnologyName() const
{
	return "Multiple table text file";
}

void KWMTDatabaseTextFileView::SetObject(Object* object)
{
	KWMTDatabaseTextFile* mtDatabaseTextFile;

	require(object != NULL);

	// Acces a l'objet edite
	mtDatabaseTextFile = cast(KWMTDatabaseTextFile*, object);

	// Parametrage de la sous-liste des mappings multi-tables
	cast(KWMTDatabaseMappingArrayView*, GetFieldAt("DatabaseFiles"))->SetObjectArray(&oaMultiTableMapping);

	// Parametrage du detecteur de format
	cast(KWDatabaseFormatDetectorView*, GetFieldAt("DatabaseFormatDetector"))->SetDatabase(mtDatabaseTextFile);

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

KWMTDatabaseTextFile* KWMTDatabaseTextFileView::GetMTDatabaseTextFile()
{
	require(objValue != NULL);
	return cast(KWMTDatabaseTextFile*, objValue);
}

void KWMTDatabaseTextFileView::ResfreshMultiTableMapping()
{
	KWMTDatabaseTextFile* database;
	KWMTDatabaseMappingArrayView multiTableMappingArrayView;
	int nMapping;
	KWMTDatabaseMapping* mapping;

	// Acces a la base multi-table
	database = GetMTDatabaseTextFile();

	// Mise a jour des mapping prealablement a leur edition
	database->UpdateMultiTableMappings();

	// Filtrage des mappings pour ne garder que ceux qui peuvent etre ecrits
	// En mode ecriture, les tables references ne sont pas prises en compte
	oaMultiTableMapping.RemoveAll();
	for (nMapping = 0; nMapping < database->GetMultiTableMappings()->GetSize(); nMapping++)
	{
		mapping = cast(KWMTDatabaseMapping*, database->GetMultiTableMappings()->GetAt(nMapping));
		if (not GetModeWriteOnly() or not database->IsReferencedClassMapping(mapping))
			oaMultiTableMapping.Add(mapping);
	}
}

// ##
