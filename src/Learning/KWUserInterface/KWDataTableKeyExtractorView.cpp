// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataTableKeyExtractorView.h"

KWDataTableKeyExtractorView::KWDataTableKeyExtractorView()
{
	KWSTDatabaseTextFileView* sourceDataTableView;
	KWSTDatabaseTextFileView* targetDataTableView;
	int i;

	// Parametrage general
	SetIdentifier("KWDataTableKeyExtractor");
	SetLabel("Data table key extractor");

	// Ajout de l'action d'extraction des cles
	AddAction("ExtractKeysFromDataTable", "Extract keys from data table",
		  (ActionMethod)(&KWDataTableKeyExtractorView::ExtractKeysFromDataTable));
	GetActionAt("ExtractKeysFromDataTable")->SetStyle("Button");

	// Ajout de l'action de creation du dictionnaire multi-tables
	AddAction("BuildMultiTableClass", "Build multi-table dictionary...",
		  (ActionMethod)(&KWDataTableKeyExtractorView::BuildMultiTableClass));
	GetActionAt("BuildMultiTableClass")->SetStyle("Button");

	// Ajout d'un champ de saisie du nom du dictionnaire
	AddStringField("ClassName", "Input dictionary", "");

	// Ajout du parametrage de la base d'origine
	sourceDataTableView = new KWSTDatabaseTextFileView;
	sourceDataTableView->SetObject(&sourceDataTable);
	AddCardField("SourceDataTable", "Input data table", sourceDataTableView);

	// Parametrage de la visibilite des specifications de la base de destination
	for (i = 0; i < sourceDataTableView->GetFieldNumber(); i++)
		sourceDataTableView->GetFieldAtIndex(i)->SetVisible(false);
	for (i = 0; i < sourceDataTableView->GetActionNumber(); i++)
		sourceDataTableView->GetActionAtIndex(i)->SetVisible(false);
	sourceDataTableView->GetFieldAt("DatabaseName")->SetVisible(true);
	sourceDataTableView->GetFieldAt("HeaderLineUsed")->SetVisible(true);
	sourceDataTableView->GetFieldAt("FieldSeparator")->SetVisible(true);
	sourceDataTableView->GetFieldAt("DatabaseFormatDetector")->SetVisible(true);

	// Ajout du parametrage de la base destination
	targetDataTableView = new KWSTDatabaseTextFileView;
	targetDataTableView->SetObject(&targetDataTable);
	AddCardField("TargetDataTable", "Extracted key data table", targetDataTableView);

	// Parametrage de la visibilite des specifications de la base de destination
	for (i = 0; i < targetDataTableView->GetFieldNumber(); i++)
		targetDataTableView->GetFieldAtIndex(i)->SetVisible(false);
	for (i = 0; i < targetDataTableView->GetActionNumber(); i++)
		targetDataTableView->GetActionAtIndex(i)->SetVisible(false);
	targetDataTableView->GetFieldAt("DatabaseName")->SetVisible(true);
	targetDataTableView->GetFieldAt("HeaderLineUsed")->SetVisible(true);
	targetDataTableView->GetFieldAt("FieldSeparator")->SetVisible(true);

	// Info-bulles
	GetFieldAt("ClassName")->SetHelpText("Dictionary that describes the content of the input data table.");
	GetActionAt("ExtractKeysFromDataTable")
	    ->SetHelpText("Extract keys from a sorted input data table."
			  "\n It is dedicated to the preparation of multi-table databases, where a"
			  "\n root entity has to be extracted from a detailed 0-n entity."
			  "\n For example, in case of a web log file with cookies, page, timestamp in each log,"
			  "\n extracting keys allow to build a table with unique cookies from the table of logs.");
	GetActionAt("BuildMultiTableClass")
	    ->SetHelpText("Build a root dictionary with a Table variable based on"
			  "\n the input dictionary, then save the dictionary file.");

	// Short cuts
	GetActionAt("ExtractKeysFromDataTable")->SetShortCut('E');
	GetActionAt("BuildMultiTableClass")->SetShortCut('B');
}

KWDataTableKeyExtractorView::~KWDataTableKeyExtractorView() {}

void KWDataTableKeyExtractorView::InitializeSourceDataTable(KWDatabase* database)
{
	const KWSTDatabaseTextFile defaultSTDataTable;
	const KWMTDatabaseTextFile defaultMTDataTable;
	KWSTDatabaseTextFile initializationDataTable;
	ALString sTargetDatabaseName;
	const ALString sPrefix = "K_";
	ALString sPathName;
	ALString sFileName;

	// Parametrage du nom du dictionnaire
	sInputClassName = database->GetClassName();
	if (KWClassDomain::GetCurrentDomain()->LookupClass(sInputClassName) == NULL)
		sInputClassName = "";

	// Parametrage si possible de la base initiale
	if (database != NULL)
	{
		initializationDataTable.SetClassName(sInputClassName);
		initializationDataTable.SetDatabaseName(database->GetDatabaseName());

		// Specialisation du format si possible
		if (database->GetTechnologyName() == defaultSTDataTable.GetTechnologyName())
		{
			initializationDataTable.SetHeaderLineUsed(
			    cast(KWSTDatabaseTextFile*, database)->GetHeaderLineUsed());
			initializationDataTable.SetFieldSeparator(
			    cast(KWSTDatabaseTextFile*, database)->GetFieldSeparator());
		}
		else if (database->GetTechnologyName() == defaultMTDataTable.GetTechnologyName())
		{
			initializationDataTable.SetHeaderLineUsed(
			    cast(KWMTDatabaseTextFile*, database)->GetHeaderLineUsed());
			initializationDataTable.SetFieldSeparator(
			    cast(KWMTDatabaseTextFile*, database)->GetFieldSeparator());
		}
	}

	// Parametrage de la base source a partir de la base d'initialisation
	sourceDataTable.CopyFrom(&initializationDataTable);

	// Parametrage de la base cible a partir de la base source
	targetDataTable.CopyFrom(&initializationDataTable);

	// On reinitialise le parametrage lies a la selection
	targetDataTable.SetSelectionAttribute("");
	targetDataTable.SetSelectionValue("");

	// Nom par defaut de la base de donnee cible
	targetDataTable.AddPrefixToUsedFiles(sPrefix);
}

void KWDataTableKeyExtractorView::Open()
{
	ALString sClassNames;
	int i;

	// Warning s'il n'y a pas de dictionnaire
	if (KWClassDomain::GetCurrentDomain()->GetClassNumber() == 0)
		AddWarning("No available dictionary");

	// Parametrage du champ de saisie des dictionnaires en style ComboBox,
	// avec la liste des dictionnaires en cours
	SetStringValueAt("ClassName", sInputClassName);
	for (i = 0; i < KWClassDomain::GetCurrentDomain()->GetClassNumber(); i++)
	{
		if (i > 0)
			sClassNames += "\n";
		sClassNames += KWClassDomain::GetCurrentDomain()->GetClassAt(i)->GetName();
	}
	GetFieldAt("ClassName")->SetStyle("EditableComboBox");
	GetFieldAt("ClassName")->SetParameters(sClassNames);

	// Appel de la methode ancetre pour l'ouverture
	UICard::Open();
	sInputClassName = GetStringValueAt("ClassName");
}

void KWDataTableKeyExtractorView::ExtractKeysFromDataTable()
{
	KWSTDatabaseTextFile workingTargetDataTable;
	ALString sOutputPathName;
	boolean bOk;
	KWClass* initialClass;
	KWFileKeyExtractorTask keyExtractorTask;
	ALString sTmp;

	require(not sourceDataTable.IsOpenedForRead());
	require(not sourceDataTable.IsOpenedForWrite());
	require(not workingTargetDataTable.IsOpenedForRead());
	require(not workingTargetDataTable.IsOpenedForWrite());
	require(sourceDataTable.GetObjects()->GetSize() == 0);
	require(workingTargetDataTable.GetObjects()->GetSize() == 0);

	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

	// Verification du directory des fichiers temporaires
	if (not FileService::CreateApplicationTmpDir())
		return;

	// On passe par une autre table en sortie, pour pouvoir specifier son chemin si elle n'en a pas
	workingTargetDataTable.CopyFrom(&targetDataTable);
	workingTargetDataTable.AddPathToUsedFiles(FileService::GetPathName(sourceDataTable.GetDatabaseName()));

	// On sort si on ne peut effectuer le traitement demande
	if (not CheckSpec(&workingTargetDataTable))
		return;

	// On tente de cree le repertoire cible, et on sort en cas d'echec
	sOutputPathName = FileService::GetPathName(workingTargetDataTable.GetDatabaseName());
	bOk = true;
	if (sOutputPathName != "" and not PLRemoteFileService::DirExists(sOutputPathName))
	{
		bOk = PLRemoteFileService::MakeDirectories(sOutputPathName);
		if (not bOk)
			AddError("Unable to create output directory (" + sOutputPathName + ")");
	}
	if (not bOk)
		return;

	// Recherche de la classe initiale
	initialClass = KWClassDomain::GetCurrentDomain()->LookupClass(sInputClassName);
	check(initialClass);

	// Demarrage du suivi de la tache
	TaskProgression::SetTitle("Extract key data table " + workingTargetDataTable.GetDatabaseName());
	TaskProgression::Start();
	AddSimpleMessage("Create key data table file " + workingTargetDataTable.GetDatabaseName());

	// Parametres
	initialClass->ExportKeyAttributeNames(keyExtractorTask.GetKeyAttributeNames());
	initialClass->ExportNativeFieldNames(keyExtractorTask.GetNativeFieldNames());
	keyExtractorTask.SetInputFileName(sourceDataTable.GetDatabaseName());
	keyExtractorTask.SetInputHeaderLineUsed(sourceDataTable.GetHeaderLineUsed());
	keyExtractorTask.SetInputFieldSeparator(sourceDataTable.GetFieldSeparator());
	keyExtractorTask.SetOutputFileName(workingTargetDataTable.GetDatabaseName());
	keyExtractorTask.SetOutputHeaderLineUsed(workingTargetDataTable.GetHeaderLineUsed());
	keyExtractorTask.SetOutputFieldSeparator(workingTargetDataTable.GetFieldSeparator());

	// Appel de la tache d'extraction des cles
	bOk = keyExtractorTask.ExtractKeys(true);
	AddSimpleMessage("");

	// Fin suivi de la tache
	TaskProgression::Stop();
}

void KWDataTableKeyExtractorView::BuildMultiTableClass()
{
	KWMTClassBuilderView mtClassBuilderView;
	boolean bTry;
	boolean bOk;

	// Parametrage de la boite de dialogue de construction d'un dictionnaire
	mtClassBuilderView.SetSecondaryDictionaryName(GetStringValueAt("ClassName"));
	mtClassBuilderView.InitDefaultParameters();

	// On rappele la boite de dialogue jusqu'a annulation, ou construction sans erreur
	bTry = true;
	while (bTry)
	{
		bTry = mtClassBuilderView.OpenAndConfirm();

		// Constructionn demandee
		if (bTry)
		{
			bOk = mtClassBuilderView.BuildMultiTableClass();

			// Arret si construction reussie
			if (bOk)
				break;
		}
	}
}

void KWDataTableKeyExtractorView::EventUpdate()
{
	// Appel de la methode ancetre
	UICard::EventUpdate();

	// Synchronisation du dictionnaire des bases
	sInputClassName = GetStringValueAt("ClassName");
	sourceDataTable.SetClassName(sInputClassName);
	targetDataTable.SetClassName(sInputClassName);
	cast(KWSTDatabaseTextFileView*, GetFieldAt("SourceDataTable"))->SetStringValueAt("ClassName", sInputClassName);
	cast(KWSTDatabaseTextFileView*, GetFieldAt("TargetDataTable"))->SetStringValueAt("ClassName", sInputClassName);
}

void KWDataTableKeyExtractorView::EventRefresh()
{
	// Appel de la methode ancetre
	UICard::EventRefresh();
}

const ALString KWDataTableKeyExtractorView::GetClassLabel() const
{
	return "Data table key extractor";
}

const ALString KWDataTableKeyExtractorView::GetObjectLabel() const
{
	return sourceDataTable.GetDatabaseName();
}

boolean KWDataTableKeyExtractorView::CheckSpec(KWSTDatabaseTextFile* workingTargetDataTable) const
{
	boolean bOk = true;
	KWClass* kwcClass;
	FileSpec specSourceTable;
	FileSpec specTargetTable;

	assert(sourceDataTable.GetClassName() == targetDataTable.GetClassName());
	require(workingTargetDataTable != NULL);
	require(workingTargetDataTable->GetClassName() == sourceDataTable.GetClassName());

	// Verification des tables d'entree et de sortie
	if (bOk)
		bOk = sourceDataTable.Check();
	if (bOk)
		bOk = workingTargetDataTable->Check();

	// Verification de la presence d'une cle
	if (bOk)
	{
		// Acces a la classe
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(sInputClassName);
		check(kwcClass);

		// Erreur si pas de cle
		if (kwcClass->GetKeyAttributeNumber() == 0)
		{
			bOk = false;
			AddError("Missing key in dictionary " + kwcClass->GetName());
		}
	}

	// Verification de la specification des tables d'entree et de sortie
	if (bOk and sourceDataTable.GetDatabaseName() == "")
	{
		bOk = false;
		AddError("Missing source data table name");
	}
	if (bOk and workingTargetDataTable->GetDatabaseName() == "")
	{
		bOk = false;
		AddError("Missing target data table name");
	}

	// Verification de la difference entre les tables d'entree et de sortie
	specSourceTable.SetLabel("input data table");
	specSourceTable.SetFilePathName(sourceDataTable.GetDatabaseName());
	specTargetTable.SetLabel("key data table");
	specTargetTable.SetFilePathName(workingTargetDataTable->GetDatabaseName());
	if (bOk)
		bOk = specTargetTable.CheckReferenceFileSpec(&specSourceTable);
	return bOk;
}
