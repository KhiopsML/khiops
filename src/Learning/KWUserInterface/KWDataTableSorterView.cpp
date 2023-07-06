// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataTableSorterView.h"

KWDataTableSorterView::KWDataTableSorterView()
{
	KWSortAttributeNameArrayView* attributeNameArrayView;
	UIList* attributeNameHelpList;
	KWSTDatabaseTextFileView* sourceDataTableView;
	KWSTDatabaseTextFileView* targetDataTableView;

	// Parametrage general
	SetIdentifier("KWDataTableSorter");
	SetLabel("Data table sorter");

	// Ajout de l'action de transfert de base
	AddAction("SortDataTableByKey", "Sort data table by key",
		  (ActionMethod)(&KWDataTableSorterView::SortDataTableByKey));
	GetActionAt("SortDataTableByKey")->SetStyle("Button");

	// Ajout d'un champ de saisie du nom du dictionnaire
	AddStringField("ClassName", "Sort dictionary", "");

	// On indique que le champ de parametrage du dictionnaire declenche une action de rafraichissement
	// de l'interface immediatement apres une mise a jour, pour pouvoir rafraichir les aides a la saisie des
	// attributs de tri
	cast(UIElement*, GetFieldAt("ClassName"))->SetTriggerRefresh(true);

	// Ajout d'un tableau des variables de trie
	attributeNameArrayView = new KWSortAttributeNameArrayView;
	attributeNameArrayView->SetMaxAttributeNumber(10);
	attributeNameArrayView->SetAttributeLabel("sort");
	attributeNameArrayView->SetObjectArray(&oaSortAttributeNames);
	AddListField("SortAttributes", "Sort variables", attributeNameArrayView);

	// Personnalisation des action d'insertion et de supression
	attributeNameArrayView->GetActionAt("InsertItemBefore")->SetVisible(false);
	attributeNameArrayView->GetActionAt("InsertItemAfter")->SetStyle("Button");
	attributeNameArrayView->GetActionAt("RemoveItem")->SetStyle("Button");
	attributeNameArrayView->GetActionAt("InsertItemAfter")->SetLabel("Insert variable");
	attributeNameArrayView->GetActionAt("RemoveItem")->SetLabel("Remove variable");

	// Creation d'une liste cachee des attributs de type simple de la classe en cours
	attributeNameHelpList = new UIList;
	attributeNameHelpList->AddStringField("Name", "Name", "");
	AddListField("Attributes", "Variables", attributeNameHelpList);
	attributeNameHelpList->SetVisible(false);

	// Parametrage de liste d'aide pour le nom des attributs de tri
	attributeNameArrayView->GetFieldAt("Name")->SetStyle("HelpedComboBox");
	attributeNameArrayView->GetFieldAt("Name")->SetParameters("Attributes:Name");

	// Ajout du parametrage de la base d'origine
	sourceDataTableView = new KWSTDatabaseTextFileView;
	sourceDataTableView->ToBasicReadMode();
	sourceDataTableView->SetObject(&sourceDataTable);
	AddCardField("SourceDataTable", "Input data table", sourceDataTableView);

	// Ajout du parametrage de la base destination
	targetDataTableView = new KWSTDatabaseTextFileView;
	targetDataTableView->ToWriteOnlyMode();
	targetDataTableView->SetObject(&targetDataTable);
	AddCardField("TargetDataTable", "Output data table", targetDataTableView);

	// Info-bulles
	GetFieldAt("ClassName")->SetHelpText("Dictionary that describes all native variables of the database file.");
	attributeNameArrayView->GetActionAt("InsertItemAfter")
	    ->SetHelpText("Insert a variable in the list of sort variables.");
	attributeNameArrayView->GetActionAt("RemoveItem")
	    ->SetHelpText("Remove a variable from the list of sort variables.");
	GetActionAt("SortDataTableByKey")
	    ->SetHelpText(
		"Sort the input data table by key."
		"\n The action reads the input data, sorts the lines by key, and writes the sorted output data."
		"\n All native variables (either used or not in the sort dictionary) are written in the output "
		"database,"
		"\n whereas derived variables are ignored: the output database has the same content than"
		"\n the input database, except than the lines are now sorted by key.");

	// Short cuts
	GetActionAt("SortDataTableByKey")->SetShortCut('S');
	attributeNameArrayView->GetActionAt("SelectDefaultKeyAttributes")->SetShortCut('D');
	attributeNameArrayView->GetActionAt("InsertItemAfter")->SetShortCut('I');
	attributeNameArrayView->GetActionAt("RemoveItem")->SetShortCut('R');
}

KWDataTableSorterView::~KWDataTableSorterView()
{
	oaSortAttributeNames.DeleteAll();
}

void KWDataTableSorterView::InitializeSourceDataTable(KWDatabase* database)
{
	const KWSTDatabaseTextFile defaultSTDataTable;
	const KWMTDatabaseTextFile defaultMTDataTable;
	KWSTDatabaseTextFile initializationDataTable;
	ALString sTargetDatabaseName;
	const ALString sPrefix = "S_";
	ALString sPathName;
	ALString sFileName;

	// Parametrage du nom du dictionnaire
	sSortClassName = database->GetClassName();
	if (KWClassDomain::GetCurrentDomain()->LookupClass(sSortClassName) == NULL)
		sSortClassName = "";

	// Parametrage si possible de la base initiale
	if (database != NULL)
	{
		initializationDataTable.SetClassName(sSortClassName);
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

void KWDataTableSorterView::Open()
{
	ALString sClassNames;
	int i;

	// Warning s'il n'y a pas de dictionnaire
	if (KWClassDomain::GetCurrentDomain()->GetClassNumber() == 0)
		AddWarning("No available dictionary");

	// Parametrage du champ de saisie des dictionnaires en style ComboBox,
	// avec la liste des dictionnaires en cours
	SetStringValueAt("ClassName", sSortClassName);
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
	sSortClassName = GetStringValueAt("ClassName");
}

boolean KWDataTableSorterView::Check(const ObjectArray* oaCheckedSortAttributeNames) const
{
	boolean bOk = true;
	KWClass* kwcClass;
	NumericKeyDictionary nkdSortAttributes;
	KWAttributeName* soVariableName;
	ALString sSortVariableName;
	KWAttribute* attribute;
	int i;
	FileSpec specSourceTable;
	FileSpec specTargetTable;
	ALString sTmp;

	require(oaCheckedSortAttributeNames != NULL);
	assert(sourceDataTable.GetClassName() == targetDataTable.GetClassName());

	// Verification des tables d'entree et de sortie
	if (bOk)
		bOk = sourceDataTable.Check();
	if (bOk)
		bOk = targetDataTable.Check();

	// Verification de la presence d'une cle
	if (bOk)
	{
		// Acces a la classe
		kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(sSortClassName);
		check(kwcClass);

		// Erreur si pas de cle
		if (oaCheckedSortAttributeNames->GetSize() == 0)
		{
			bOk = false;
			AddError("Missing sort key");
		}
		// Verification des champ de la cle sinon
		else
		{
			for (i = 0; i < oaCheckedSortAttributeNames->GetSize(); i++)
			{
				soVariableName = cast(KWAttributeName*, oaCheckedSortAttributeNames->GetAt(i));
				sSortVariableName = soVariableName->GetName();
				attribute = kwcClass->LookupAttribute(sSortVariableName);

				// Presence du d'une cle
				if (sSortVariableName == "")
				{
					bOk = false;
					AddError(sTmp + "Sort variable " + IntToString(i + 1) + " is empty");
				}
				// Existence de la cle
				else if (attribute == NULL)
				{
					bOk = false;
					AddError("Sort variable " + sSortVariableName + " not found in dictionary " +
						 kwcClass->GetName());
				}
				// La cle doit etre de type Symbol
				else if (attribute->GetType() != KWType::Symbol)
				{
					bOk = false;
					AddError("Sort variable " + sSortVariableName + " in dictionary " +
						 kwcClass->GetName() + " must be of type " +
						 KWType::ToString(KWType::Symbol));
				}
				// La cle ne doit pas etre calculee
				else if (attribute->GetAnyDerivationRule() != NULL)
				{
					bOk = false;
					AddError("Sort variable " + sSortVariableName + " in dictionary " +
						 kwcClass->GetName() +
						 " must be a native variable, without derivation rule");
				}
				// Un meme attribut ne doit pas etre utilise plusieurs fois pour la cle
				else
				{
					// Test d'utilisation de l'attribut pour la cle
					if (nkdSortAttributes.Lookup(attribute) != NULL)
					{
						bOk = false;
						AddError("Sort variable " + sSortVariableName + " in dictionary " +
							 kwcClass->GetName() + " used several times");
					}
					// Memorisation de l'utilisation de l'attribut pour la cle
					else
						nkdSortAttributes.SetAt(attribute, attribute);
				}
			}
		}
	}

	// Verification de la specification des tables d'entree et de sortie
	if (bOk and sourceDataTable.GetDatabaseName() == "")
	{
		bOk = false;
		AddError("Missing source data table name");
	}
	if (bOk and targetDataTable.GetDatabaseName() == "")
	{
		bOk = false;
		AddError("Missing target data table name");
	}

	// Verification de la difference entre les tables d'entree et de sortie
	specSourceTable.SetLabel("data table to sort");
	specSourceTable.SetFilePathName(sourceDataTable.GetDatabaseName());
	specTargetTable.SetLabel("target data table");
	specTargetTable.SetFilePathName(targetDataTable.GetDatabaseName());
	if (bOk)
		bOk = specTargetTable.CheckReferenceFileSpec(&specSourceTable);
	return bOk;
}

void KWDataTableSorterView::SortDataTableByKey()
{
	boolean bOk = true;
	KWFileSorter fileSorter;
	KWSTDatabaseTextFile workingTargetDataTable;
	ALString sOutputPathName;
	KWClass* kwClass;
	KWAttributeName* attribute;
	int nAttributes;

	// Verification du directory des fichiers temporaires
	if (not FileService::CreateApplicationTmpDir())
		return;

	// Memorisation des donnees modifies (non geres par les View)
	sSortClassName = GetStringValueAt("ClassName");
	sourceDataTable.SetClassName(sSortClassName);
	targetDataTable.SetClassName(sSortClassName);

	// On passe par une autre table en sortie, pour pouvoir specifier son chemin si elle n'en a pas
	workingTargetDataTable.CopyFrom(&targetDataTable);
	workingTargetDataTable.AddPathToUsedFiles(FileService::GetPathName(sourceDataTable.GetDatabaseName()));

	// Test si specification valides
	if (sSortClassName == "")
	{
		bOk = false;
		AddError("Sort dictionary not specified");
	}
	bOk = bOk and sourceDataTable.Check();
	bOk = bOk and workingTargetDataTable.Check();
	bOk = bOk and Check(&oaSortAttributeNames);

	// Specification du tri
	if (bOk)
	{
		// Parametrage du sorter
		fileSorter.SetInputFieldSeparator(sourceDataTable.GetFieldSeparator());
		fileSorter.SetInputFileName(sourceDataTable.GetDatabaseName());
		fileSorter.SetInputHeaderLineUsed(sourceDataTable.GetHeaderLineUsed());
		fileSorter.SetOutputFieldSeparator(workingTargetDataTable.GetFieldSeparator());
		fileSorter.SetOutputFileName(workingTargetDataTable.GetDatabaseName());
		fileSorter.SetOutputHeaderLineUsed(workingTargetDataTable.GetHeaderLineUsed());

		// Specification du dictionnaire et des attributs a trier
		kwClass = KWClassDomain::GetCurrentDomain()->LookupClass(sSortClassName);
		kwClass->ExportNativeFieldNames(fileSorter.GetNativeFieldNames());
		for (nAttributes = 0; nAttributes < oaSortAttributeNames.GetSize(); nAttributes++)
		{
			attribute = cast(KWAttributeName*, oaSortAttributeNames.GetAt(nAttributes));
			fileSorter.GetKeyAttributeNames()->Add(attribute->GetName());
		}
	}

	// Creation si necessaire du repertoire du fichier resultat du tri
	if (bOk)
	{
		// Acces au repertoire du fichier resultat du tri
		sOutputPathName = FileService::GetPathName(workingTargetDataTable.GetDatabaseName());
		bOk = KWResultFilePathBuilder::CheckResultDirectory(sOutputPathName, GetClassLabel());
	}

	// Tri des donnees si specifications valides
	if (bOk)
	{
		// Demarrage du suivi de la tache
		TaskProgression::SetDisplayedLevelNumber(1);
		TaskProgression::SetTitle("Sort data table " + sourceDataTable.GetDatabaseName());
		TaskProgression::Start();
		AddSimpleMessage("Create sorted data table file " + workingTargetDataTable.GetDatabaseName());

		// Tri de la table
		fileSorter.Sort(true);
		AddSimpleMessage("");

		// Fin suivi de la tache
		TaskProgression::Stop();
	}
}

void KWDataTableSorterView::EventUpdate()
{
	// Appel de la methode ancetre
	UICard::EventUpdate();

	// Synchronisation du dictionnaire des bases
	sSortClassName = GetStringValueAt("ClassName");
	cast(KWSortAttributeNameArrayView*, GetFieldAt("SortAttributes"))->SetClassName(sSortClassName);
	sourceDataTable.SetClassName(sSortClassName);
	targetDataTable.SetClassName(sSortClassName);
	cast(KWSTDatabaseTextFileView*, GetFieldAt("SourceDataTable"))->SetStringValueAt("ClassName", sSortClassName);
	cast(KWSTDatabaseTextFileView*, GetFieldAt("TargetDataTable"))->SetStringValueAt("ClassName", sSortClassName);
}

void KWDataTableSorterView::EventRefresh()
{
	// Appel de la methode ancetre
	UICard::EventRefresh();

	// Rafraichissement de la liste d'aide sur les nom de variables
	RefreshHelpLists();
}

const ALString KWDataTableSorterView::GetClassLabel() const
{
	return "Data table sorter";
}

void KWDataTableSorterView::RefreshHelpLists()
{
	// Rafraichissement de la liste d'aide pour les attributs de tri
	sortAttributeHelpList.FillAttributeNames(sSortClassName, false, true, true, true,
						 cast(UIList*, GetFieldAt("Attributes")), "Name");
}

const ALString KWDataTableSorterView::GetObjectLabel() const
{
	return sourceDataTable.GetDatabaseName();
}
