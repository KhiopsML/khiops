// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWDatabaseView.h"

KWDatabaseView::KWDatabaseView()
{
	SetIdentifier("KWDatabase");
	SetLabel("Database");
	AddStringField("ClassName", "Dictionary", "");
	AddDoubleField("SampleNumberPercentage", "Sample percentage", 0);
	AddStringField("SamplingMode", "Sampling mode", "");
	AddStringField("SelectionAttribute", "Selection variable", "");
	AddStringField("SelectionValue", "Selection value", "");

	// Parametrage des styles;
	GetFieldAt("SampleNumberPercentage")->SetStyle("Spinner");
	GetFieldAt("SamplingMode")->SetStyle("ComboBox");

	// ## Custom constructor

	// Declaration des liste d'aides
	UIList* selectionAttributeNameHelpList;
	UIList* selectionValuesHelpList;

	// Initialisation des specifications de classe
	bModeWriteOnly = false;

	// Precision a 1 chiffre apres la virgule pour le pourcentage
	GetFieldAt("SampleNumberPercentage")->SetParameters("1");

	// Plage de valeur pour le champ pourcentage des exemples
	cast(UIDoubleElement*, GetFieldAt("SampleNumberPercentage"))->SetMinValue(0);
	cast(UIDoubleElement*, GetFieldAt("SampleNumberPercentage"))->SetMaxValue(100);

	// Valeurs pour le mode d'echantillonnage
	GetFieldAt("SamplingMode")->SetParameters("Include sample\nExclude sample");

	// Initialisation des bases d'apprentissage et test
	trainDatabase = NULL;
	testDatabase = NULL;

	// Initialisation de la vue sur la base de test
	testDatabaseView = NULL;

	// Creation d'une liste cachee des attributs de selection pour la classe en cours
	selectionAttributeNameHelpList = new UIList;
	selectionAttributeNameHelpList->AddStringField("Name", "Name", "");
	AddListField("SelectionAttributes", "Selection variables", selectionAttributeNameHelpList);
	selectionAttributeNameHelpList->SetVisible(false);

	// Creation d'une liste cachee des valeurs de selection pour l'attribut de selection en cours, en train
	selectionValuesHelpList = new UIList;
	selectionValuesHelpList->AddStringField("Value", "Value", "");
	AddListField("SelectionValues", "Selection values", selectionValuesHelpList);
	selectionValuesHelpList->SetVisible(false);

	// Parametrage de liste d'aide pour le nom de l'attribut de selection et sa valeur
	GetFieldAt("SelectionAttribute")->SetStyle("HelpedComboBox");
	GetFieldAt("SelectionAttribute")->SetParameters(sDefaultSelectionAttributeParameters);
	GetFieldAt("SelectionValue")->SetStyle("HelpedComboBox");
	GetFieldAt("SelectionValue")->SetParameters(sDefaultSelectionValueParameters);

	// Info-bulles
	GetFieldAt("ClassName")->SetHelpText("Name of the dictionary related to the database.");
	GetFieldAt("SampleNumberPercentage")->SetHelpText("Percentage of samples.");
	GetFieldAt("SamplingMode")
	    ->SetHelpText("To include or exclude the records of the sample."
			  "\n This allows to extract a train sample and its exact complementary as a test sample"
			  "\n (if the same sample percentage is used both in train and test, "
			  "\n in include sample mode in train and exclude sample mode in test).");
	GetFieldAt("SelectionAttribute")
	    ->SetHelpText("When a selection variable is specified, the records are selected "
			  "\n when the value of their selection variable is equal to the selection value.");
	GetFieldAt("SelectionValue")->SetHelpText("Selection value used when a selection variable is specified.");

#ifdef DEPRECATED_V10
	bDeprecatedTestDataViewUsed = false;
#endif // DEPRECATED_V10

	// ##
}

KWDatabaseView::~KWDatabaseView()
{
	// ## Custom destructor

	// ##
}

KWDatabase* KWDatabaseView::GetKWDatabase()
{
	require(objValue != NULL);
	return cast(KWDatabase*, objValue);
}

void KWDatabaseView::EventUpdate(Object* object)
{
	KWDatabase* editedObject;

	require(object != NULL);

	editedObject = cast(KWDatabase*, object);
	editedObject->SetClassName(GetStringValueAt("ClassName"));
	editedObject->SetSampleNumberPercentage(GetDoubleValueAt("SampleNumberPercentage"));
	editedObject->SetSamplingMode(GetStringValueAt("SamplingMode"));
	editedObject->SetSelectionAttribute(GetStringValueAt("SelectionAttribute"));
	editedObject->SetSelectionValue(GetStringValueAt("SelectionValue"));

	// ## Custom update

	// ##
}

void KWDatabaseView::EventRefresh(Object* object)
{
	KWDatabase* editedObject;

	require(object != NULL);

	editedObject = cast(KWDatabase*, object);
	SetStringValueAt("ClassName", editedObject->GetClassName());
	SetDoubleValueAt("SampleNumberPercentage", editedObject->GetSampleNumberPercentage());
	SetStringValueAt("SamplingMode", editedObject->GetSamplingMode());
	SetStringValueAt("SelectionAttribute", editedObject->GetSelectionAttribute());
	SetStringValueAt("SelectionValue", editedObject->GetSelectionValue());

	// ## Custom refresh

	// Rafraichissement des listes d'aide
	RefreshHelpLists();

	// ##
}

const ALString KWDatabaseView::GetClassLabel() const
{
	return "Database";
}

// ## Method implementation

void KWDatabaseView::SetModeWriteOnly(boolean bValue)
{
	bModeWriteOnly = bValue;
}

boolean KWDatabaseView::GetModeWriteOnly() const
{
	return bModeWriteOnly;
}

void KWDatabaseView::SetHelpListViewPath(const ALString& sViewPath)
{
	ALString sSelectionAttributeParameters;
	ALString sSelectionValueParameters;

	// Acces aux parametres en cours
	sSelectionAttributeParameters = GetFieldAt("SelectionAttribute")->GetParameters();
	sSelectionValueParameters = GetFieldAt("SelectionValue")->GetParameters();
	assert(sSelectionAttributeParameters.Find(sDefaultSelectionAttributeParameters) >= 0);
	assert(sSelectionAttributeParameters.Find(sDefaultSelectionValueParameters) ==
	       sSelectionValueParameters.Find(sDefaultSelectionAttributeParameters));

	// Modification des parametres
	sSelectionAttributeParameters = sViewPath + "." + sDefaultSelectionAttributeParameters;
	sSelectionValueParameters = sViewPath + "." + sDefaultSelectionValueParameters;

	// Reparametrage des champs
	GetFieldAt("SelectionAttribute")->SetParameters(sSelectionAttributeParameters);
	GetFieldAt("SelectionValue")->SetParameters(sSelectionValueParameters);
}

const ALString KWDatabaseView::GetHelpListViewPath()
{
	ALString sHelpListViewPath;
	ALString sSelectionAttributeParameters;
	ALString sSelectionValueParameters;
	int nDefaultPosition;

	// Acces aux parametres en cours
	sSelectionAttributeParameters = GetFieldAt("SelectionAttribute")->GetParameters();
	sSelectionValueParameters = GetFieldAt("SelectionValue")->GetParameters();
	assert(sSelectionAttributeParameters.Find(sDefaultSelectionAttributeParameters) >= 0);
	assert(sSelectionAttributeParameters.Find(sDefaultSelectionValueParameters) ==
	       sSelectionValueParameters.Find(sDefaultSelectionAttributeParameters));

	// Recherche du path
	nDefaultPosition = sSelectionAttributeParameters.Find(sDefaultSelectionAttributeParameters);
	if (nDefaultPosition > 0)
		sHelpListViewPath = sSelectionAttributeParameters.Left(nDefaultPosition - 1);
	return sHelpListViewPath;
}

void KWDatabaseView::SetTrainDatabase(KWDatabase* trainDatabaseSettings)
{
	trainDatabase = trainDatabaseSettings;
}

KWDatabase* KWDatabaseView::GetTrainDatabase()
{
	return trainDatabase;
}

void KWDatabaseView::SetTestDatabase(KWDatabase* testDatabaseSettings)
{
	testDatabase = testDatabaseSettings;
	sLastTestDatabaseSpecificationMode = "";
	sLastTestDatabaseClassName = "";
}

KWDatabase* KWDatabaseView::GetTestDatabase()
{
	return testDatabase;
}

void KWDatabaseView::SetTestDatabaseView(KWDatabaseView* testDatabaseViewSettings)
{
	testDatabaseView = testDatabaseViewSettings;
}

KWDatabaseView* KWDatabaseView::GetTestDatabaseView()
{
	return testDatabaseView;
}

void KWDatabaseView::AddFillTestDatabaseSettingsAction()
{
	require(not IsOpened());
	require(GetActionIndex("FillTestDatabaseSettings") == -1);

	AddAction("FillTestDatabaseSettings", "Fill test database settings",
		  (ActionMethod)(&KWDatabaseView::FillTestDatabaseSettings));
	GetActionAt("FillTestDatabaseSettings")->SetStyle("Button");

	// Info-bulles
	GetActionAt("FillTestDatabaseSettings")
	    ->SetHelpText("Allows to fill all the fields of the 'Test database' pane, by copying them from the 'Train "
			  "database' pane."
			  "\n The only change is the 'Sampling mode' which value is inverted in the test pane,"
			  "\n in order to get a test sample that is the exact complementary of the train sample.");
	// Short cuts
	GetActionAt("InspectTestDatabaseSettings")->SetShortCut('I');
}

void KWDatabaseView::AddImportTrainDatabaseSettingsAction()
{
	require(not IsOpened());
	require(GetActionIndex("ImportTrainDatabaseSettings") == -1);

	AddAction("ImportTrainDatabaseSettings", "Import train database settings",
		  (ActionMethod)(&KWDatabaseView::ImportTrainDatabaseSettings));
	GetActionAt("ImportTrainDatabaseSettings")->SetStyle("Button");

	// Info-bulles
	GetActionAt("ImportTrainDatabaseSettings")
	    ->SetHelpText("Allows to fill all the test database fields, by copying them from the 'Train database' pane."
			  "\n The only change is the 'Sampling mode' which value is inverted in the test parameters,"
			  "\n in order to get a test sample that is the exact complementary of the train sample.");

	// Short cuts
	GetActionAt("ImportTrainDatabaseSettings")->SetShortCut('I');
}

void KWDatabaseView::AddInspectTestDatabaseSettingsAction()
{
	require(not IsOpened());
	require(GetActionIndex("InspectTestDatabaseSettings") == -1);

	// Ajout d'une action de visualisation de la base de test
	AddAction("InspectTestDatabaseSettings", "Inspect test database settings...",
		  (ActionMethod)(&KWDatabaseView::InspectTestDatabaseSettings));
	GetActionAt("InspectTestDatabaseSettings")->SetStyle("Button");

	// Info-bulles
	GetActionAt("InspectTestDatabaseSettings")
	    ->SetHelpText("Inspect the test database parameters."
			  "\n The test parameters are editable only in the case of a specific test database.");

	// Short cuts
	GetActionAt("InspectTestDatabaseSettings")->SetShortCut('I');
}

void KWDatabaseView::AddTestDatabaseSpecificationMode()
{
	require(not IsOpened());
	require(GetFieldIndex("TestDatabaseSpecificationMode") == -1);

	// Ajout d'un champ de saisie pour specifier le type de base en test
	AddStringField("TestDatabaseSpecificationMode", "Test database", "Complementary");
	GetFieldAt("TestDatabaseSpecificationMode")->SetStyle("ComboBox");
	GetFieldAt("TestDatabaseSpecificationMode")->SetParameters("Complementary\nSpecific\nNone");

	// Info-bulles
	GetFieldAt("TestDatabaseSpecificationMode")
	    ->SetHelpText(
		"Specification of test database:"
		"\nComplementary: same as the train database, with 'Sampling mode' inverted in the test database,"
		"\nin order to get test samples that are the exact complementary of the train samples,"
		"\nSpecific: specific parameters for the test database,"
		"\nNone: no test database is used.");
}

void KWDatabaseView::UpdateTestDatabase()
{
	KWDatabase* editedTrainDatabase;
	KWDatabase* emptyDatabase;

	// Acces a la base d'apprentissage pour mettre a jour son mapping
	editedTrainDatabase = cast(KWDatabase*, GetObject());
	cast(KWMTDatabase*, editedTrainDatabase)->UpdateMultiTableMappings();

	// On force un Refresh de la base d'apprentissage, ce va synchroniser ses mappings vi la methode
	// ResfreshMultiTableMapping
	EventRefresh(editedTrainDatabase);

	// Copie des parametres vers la base de test
	if (testDatabase != NULL)
	{
		assert(GetFieldIndex("TestDatabaseSpecificationMode") != -1);

		// Cas d'une base complementaire: on initialise avec la base complementaire
		if (GetStringValueAt("TestDatabaseSpecificationMode") == "Complementary")
		{
			// Initialisation de la base avec le complementaire
			testDatabase->CopyFrom(editedTrainDatabase);
			testDatabase->SetModeExcludeSample(not editedTrainDatabase->GetModeExcludeSample());
		}
		// Cas d'une base avec specification particuliere
		else if (GetStringValueAt("TestDatabaseSpecificationMode") == "Specific")
		{
#ifdef DEPRECATED_V10
			if (bDeprecatedTestDataViewUsed)
			{
				// On garde la base en cours, sauf s'il n'y a pas de specification pour la base courante
				// Dans ce cas, on force le taux d'echantillonnage a 100%
				if (testDatabase->GetDatabaseName() == "")
				{
					testDatabase->SetSampleNumberPercentage(100);
					testDatabase->SetModeExcludeSample(false);
				}
			}
			else
#endif // DEPRECATED_V10

				// On garde la base en cours, sauf si on vient juste de passer au mode specific ou que
				// l'on a change de classe
				if (sLastTestDatabaseSpecificationMode != "Specific" or
				    sLastTestDatabaseClassName != editedTrainDatabase->GetClassName())
				{
					// Creation d'une base vide de meme technologie que la base d'apprentissage
					emptyDatabase = KWDatabase::CloneDatabaseTechnology(
					    editedTrainDatabase->GetTechnologyName());

					// On l'initialise avec le bon dictionnaire, en supposant qu'elle est
					// multi-table
					emptyDatabase->SetClassName(editedTrainDatabase->GetClassName());
					cast(KWMTDatabase*, emptyDatabase)->UpdateMultiTableMappings();

					// On recopie la base vide avec son taux d'echantillonnage a 100 par defaut
					testDatabase->CopyFrom(emptyDatabase);

					// Nettoyage
					delete emptyDatabase;
				}
		}
		// Cas sans base de test
		else if (GetStringValueAt("TestDatabaseSpecificationMode") == "None")
		{
			// Creation d'une base vide de meme technologie que la base d'apprentissage
			emptyDatabase = KWDatabase::CloneDatabaseTechnology(editedTrainDatabase->GetTechnologyName());

			// On l'initialise avec le bon dictionnaire, en supposant qu'elle est multi-table
			emptyDatabase->SetClassName(editedTrainDatabase->GetClassName());
			cast(KWMTDatabase*, emptyDatabase)->UpdateMultiTableMappings();

			// On recopie la base vide et on met le taux d'echantillonnage a 0
			testDatabase->CopyFrom(emptyDatabase);
			testDatabase->SetSampleNumberPercentage(0);

			// Nettoyage
			delete emptyDatabase;
		}

		// Memorisation du dernier mode de specification utilise
		sLastTestDatabaseSpecificationMode = GetStringValueAt("TestDatabaseSpecificationMode");

		// Derniere classe utilisee pour la base de test
		sLastTestDatabaseClassName = editedTrainDatabase->GetClassName();

		// On force un Refresh de la base de test, ce va synchroniser ses mappings vi la methode
		// ResfreshMultiTableMapping
		testDatabaseView->EventRefresh(testDatabase);
	}
}

void KWDatabaseView::InspectTestDatabaseSettings()
{
	require(GetActionIndex("InspectTestDatabaseSettings") != -1);
	require(GetFieldIndex("TestDatabaseSpecificationMode") != -1);

	// Copie des parametre vers la base de test, en inversant le mode d'echantillonnage "ExcludeSample"
	if (testDatabase != NULL and testDatabaseView != NULL)
	{
		// Initialisation de la base de test
		UpdateTestDatabase();

		// Cas d'une base specifique
		if (GetStringValueAt("TestDatabaseSpecificationMode") == "Specific")
		{
			// Parametrage de la boite de dialogue pour la base de test
			testDatabaseView->SetEditable(true);
			testDatabaseView->GetFieldAt("DatabaseFiles")->SetEditable(false);
			cast(UIObjectArrayView*, testDatabaseView->GetFieldAt("DatabaseFiles"))
			    ->GetFieldAt("DataTableName")
			    ->SetEditable(true);
			testDatabaseView->GetFieldAt("DatabaseFormatDetector")->SetVisible(true);
			testDatabaseView->GetActionAt("ImportTrainDatabaseSettings")->SetVisible(true);
		}
		// Sinon: base complementaire ou pas de base
		else
		{
			assert(GetStringValueAt("TestDatabaseSpecificationMode") == "Complementary" or
			       GetStringValueAt("TestDatabaseSpecificationMode") == "None");

			// Passage de tous les champs en read-only
			testDatabaseView->SetEditable(false);
			testDatabaseView->GetFieldAt("DatabaseFormatDetector")->SetVisible(false);
			testDatabaseView->GetActionAt("ImportTrainDatabaseSettings")->SetVisible(false);
		}

		// Ouverture de la boite de dialogue
		testDatabaseView->SetStringValueAt("TestDatabaseSpecificationMode",
						   GetStringValueAt("TestDatabaseSpecificationMode"));
		testDatabaseView->GetFieldAt("TestDatabaseSpecificationMode")->SetEditable(false);
		testDatabaseView->SetVisible(true);
		testDatabaseView->Open();
	}
}

void KWDatabaseView::FillTestDatabaseSettings()
{
	KWDatabase* editedObject;

	// Copie des parametre vers la base de test, en inversant le mode d'echantillonnage "ExcludeSample"
	if (testDatabase != NULL)
	{
		editedObject = cast(KWDatabase*, GetObject());
		testDatabase->CopyFrom(editedObject);
		testDatabase->SetModeExcludeSample(not testDatabase->GetModeExcludeSample());
	}
}

void KWDatabaseView::ImportTrainDatabaseSettings()
{
	KWDatabase* editedObject;

	// Copie des parametres depuis la base d'apprentissage, en inversant le mode d'echantillonnage "ExcludeSample"
	if (trainDatabase != NULL)
	{
		editedObject = cast(KWDatabase*, GetObject());
		editedObject->CopyFrom(trainDatabase);
		editedObject->SetModeExcludeSample(not editedObject->GetModeExcludeSample());
	}
}

//////////////////////////////////////////////////////////////////
// Administration des vues sur les technologies de KWDatabase

ALString KWDatabaseView::GetTechnologyName() const
{
	return "";
}

KWDatabaseView* KWDatabaseView::Create() const
{
	return new KWDatabaseView;
}

KWDatabaseView* KWDatabaseView::CreateDefaultDatabaseTechnologyView()
{
	require(KWDatabase::GetDefaultTechnologyName() != "");
	require(KWDatabase::LookupDatabaseTechnology(KWDatabase::GetDefaultTechnologyName()) != NULL);
	require(LookupDatabaseTechnologyView(KWDatabase::GetDefaultTechnologyName()) != NULL);

	return CloneDatabaseTechnologyView(KWDatabase::GetDefaultTechnologyName());
}

void KWDatabaseView::RegisterDatabaseTechnologyView(KWDatabaseView* databaseView)
{
	require(databaseView != NULL);
	require(databaseView->GetTechnologyName() != "");
	require(KWClass::CheckName(databaseView->GetTechnologyName(), databaseView));
	require(odDatabaseTechnologyViews == NULL or
		odDatabaseTechnologyViews->Lookup(databaseView->GetTechnologyName()) == NULL);

	// Creation si necessaire du dictionnaire de databaseViews
	if (odDatabaseTechnologyViews == NULL)
		odDatabaseTechnologyViews = new ObjectDictionary;

	// Memorisation de la databaseView
	odDatabaseTechnologyViews->SetAt(databaseView->GetTechnologyName(), databaseView);
}

KWDatabaseView* KWDatabaseView::LookupDatabaseTechnologyView(const ALString& sTechnologyName)
{
	// Creation si necessaire du dictionnaire de databaseViews
	if (odDatabaseTechnologyViews == NULL)
		odDatabaseTechnologyViews = new ObjectDictionary;

	return cast(KWDatabaseView*, odDatabaseTechnologyViews->Lookup(sTechnologyName));
}

KWDatabaseView* KWDatabaseView::CloneDatabaseTechnologyView(const ALString& sTechnologyName)
{
	KWDatabaseView* referenceDatabaseView;

	// Creation si necessaire du dictionnaire de databaseViews
	if (odDatabaseTechnologyViews == NULL)
		odDatabaseTechnologyViews = new ObjectDictionary;

	// Recherche d'un databaseView de meme nom
	referenceDatabaseView = cast(KWDatabaseView*, odDatabaseTechnologyViews->Lookup(sTechnologyName));

	// Retour de son Clone si possible
	if (referenceDatabaseView == NULL)
		return NULL;
	else
		return referenceDatabaseView->Create();
}

void KWDatabaseView::ExportAllDatabaseTechnologyViews(ObjectArray* oaDatabaseViews)
{
	require(oaDatabaseViews != NULL);

	// Creation si necessaire du dictionnaire de databaseViews
	if (odDatabaseTechnologyViews == NULL)
		odDatabaseTechnologyViews = new ObjectDictionary;

	// Tri des vues avant de retourner le tableau
	odDatabaseTechnologyViews->ExportObjectArray(oaDatabaseViews);
}

void KWDatabaseView::DeleteAllDatabaseTechnologyViews()
{
	if (odDatabaseTechnologyViews != NULL)
	{
		odDatabaseTechnologyViews->DeleteAll();
		delete odDatabaseTechnologyViews;
		odDatabaseTechnologyViews = NULL;
	}
	ensure(odDatabaseTechnologyViews == NULL);
}

void KWDatabaseView::RefreshHelpLists()
{
	KWDatabase* database;

	assert(objValue != NULL);

	// Acces a l'objet edite
	database = cast(KWDatabase*, objValue);

	// Rafraichissement de la liste d'aide des attributs de selection
	selectionAttributeHelpList.FillAttributeNames(database->GetClassName(), true, true, false, false,
						      cast(UIList*, GetFieldAt("SelectionAttributes")), "Name");

	// Rafraichissement des valeurs de selection
	dbSelectionValuesHelpList.FillAttributeValues(database, database->GetSelectionAttribute(), false, true, true,
						      cast(UIList*, GetFieldAt("SelectionValues")), "Value");
}

const ALString KWDatabaseView::sDefaultSelectionAttributeParameters = "SelectionAttributes:Name";
const ALString KWDatabaseView::sDefaultSelectionValueParameters = "SelectionValues:Value";

ObjectDictionary* KWDatabaseView::odDatabaseTechnologyViews = NULL;

// ##
