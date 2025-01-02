// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWLearningProblemView.h"

KWLearningProblemView::KWLearningProblemView()
{
	KWClassManagementView* classManagementView;
	KWAnalysisSpecView* analysisSpecView;
	KWAnalysisResultsView* analysisResultsView;
	KWLearningProblemActionView* learningProblemActionView;
	KWLearningProblemHelpCard* learningProblemHelpCard;
	UIList* attributeNameHelpList;
	UIList* mainTargetModalityHelpList;

	// Libelles
	SetIdentifier("KWLearningProblem");
	SetLabel(GetLearningMainWindowTitle());

	// On rend le bouton de sortie invisible sur la fenetre principale
	GetActionAt("Exit")->SetVisible(false);

	// Creation des sous fiches (creation generique pour les vue sur bases de donnees)
	classManagementView = new KWClassManagementView;
	trainDatabaseView = KWDatabaseView::CreateDefaultDatabaseTechnologyView();
	testDatabaseView = KWDatabaseView::CreateDefaultDatabaseTechnologyView();
	analysisSpecView = new KWAnalysisSpecView;
	analysisResultsView = new KWAnalysisResultsView;
	learningProblemActionView = new KWLearningProblemActionView;
	learningProblemHelpCard = new KWLearningProblemHelpCard;

	// Ajout des sous-fiches
	AddCardField("ClassManagement", "Data dictionary", classManagementView);
	AddCardField("TrainDatabase", "Train database", trainDatabaseView);
	AddCardField("AnalysisSpec", "Parameters", analysisSpecView);
	AddCardField("AnalysisResults", "Results", analysisResultsView);
	AddCardField("LearningTools", "Tools", learningProblemActionView);
	AddCardField("Help", "Help", learningProblemHelpCard);

	// Parametrage des actions d'import/export entre les bases d'apprentissage et de test
	testDatabaseView->AddImportTrainDatabaseSettingsAction();

	// Titre specifique pour la base de test
	testDatabaseView->SetLabel("Test database");

	// Parametrage de l'action d'inspection de la base de train depuis la base d'apprentissage
	trainDatabaseView->AddInspectTestDatabaseSettingsAction();
	trainDatabaseView->AddTestDatabaseSpecificationMode();

	// On parametre egalement la base de test pour avoir le champ du type de specification
	// de la base de test en non editable
	testDatabaseView->AddTestDatabaseSpecificationMode();

#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10: fonctionnalite obsolete, conservee de facon cachee en V10 pour compatibilite
		// ascendante des scenarios L'action FillTestDatabaseSettingsAction est maintenue pour compatibilite
		// ascendante
		trainDatabaseView->AddFillTestDatabaseSettingsAction();
		trainDatabaseView->GetActionAt("FillTestDatabaseSettings")->SetVisible(false);
	}
#endif // DEPRECATED_V10

	// Parametrage de la vue sur la base de train par la vue sur la base de test,
	// pour permettre d'editer la base de test
	trainDatabaseView->SetTestDatabaseView(testDatabaseView);

	// Le champ des dictionnaires n'est pas visibles dans les parametrages de
	// base de donnees: il est defini dans la fiche de gestion des dictionnaires
	trainDatabaseView->GetFieldAt("ClassName")->SetVisible(false);
	testDatabaseView->GetFieldAt("ClassName")->SetVisible(false);

	// Parametrage de liste d'aide pour le nom du dictionnaire
	classManagementView->GetFieldAt("ClassName")->SetStyle("HelpedComboBox");
	classManagementView->GetFieldAt("ClassName")->SetParameters("ClassManagement.Classes:ClassName");

	// Specialisation du parametrage des listes d'aide de la base
	trainDatabaseView->SetHelpListViewPath("TrainDatabase");

	// Creation d'une liste cachee des attributs de la classe en cours
	attributeNameHelpList = new UIList;
	attributeNameHelpList->AddStringField("Name", "Name", "");
	AddListField("Attributes", "Variables", attributeNameHelpList);
	attributeNameHelpList->SetVisible(false);

	// Parametrage de liste d'aide pour le nom de l'attribut cible
	analysisSpecView->GetFieldAt("TargetAttributeName")->SetStyle("HelpedComboBox");
	analysisSpecView->GetFieldAt("TargetAttributeName")->SetParameters("Attributes:Name");

	// Creation d'une liste cachee des modalites de l'attribut cible
	mainTargetModalityHelpList = new UIList;
	mainTargetModalityHelpList->AddStringField("Value", "Value", "");
	AddListField("TargetValues", "Target values", mainTargetModalityHelpList);
	mainTargetModalityHelpList->SetVisible(false);

	// Parametrage de liste d'aide pour les modalites de l'attribut cible
	analysisSpecView->GetFieldAt("MainTargetModality")->SetStyle("HelpedComboBox");
	analysisSpecView->GetFieldAt("MainTargetModality")->SetParameters("TargetValues:Value");

#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10: Creation de la vue et des donnees de la base de test obsolete

		// Creation des vues et donnnees
		deprecatedEmptyDatabase = KWDatabase::CreateDefaultDatabaseTechnology();
		deprecatedTestDatabase = KWDatabase::CreateDefaultDatabaseTechnology();
		deprecatedTestDatabaseView = KWDatabaseView::CreateDefaultDatabaseTechnologyView();

		// Insertion dans les onglets
		AddCardField("TestDatabase", "Deprecated test database", deprecatedTestDatabaseView);
		MoveFieldBefore("TestDatabase", "AnalysisSpec");

		// Parametrage avance, comme pour l'ancienne vue (mais sans les liste d'aide)
		deprecatedTestDatabaseView->AddImportTrainDatabaseSettingsAction();
		deprecatedTestDatabaseView->GetFieldAt("ClassName")->SetVisible(false);

		// La vue entiere est non visible
		deprecatedTestDatabaseView->SetVisible(false);

		// Parametrage de l'objet edite par la vue
		deprecatedTestDatabaseView->SetObject(deprecatedTestDatabase);
		bDeprecatedTestDataViewUsed = false;
		bDeprecatedTestDataViewWarningEmited = false;
	}
#endif // DEPRECATED_V10

	// Passage en ergonomie onglets
	SetStyle("TabbedPanes");

	// Ajout d'actions sous formes de boutons
	AddAction("ComputeStats", "Train model", (ActionMethod)(&KWLearningProblemView::ComputeStats));
	AddAction("TransferDatabase", "Deploy model...", (ActionMethod)(&KWLearningProblemView::TransferDatabase));
	GetActionAt("ComputeStats")->SetStyle("Button");
	GetActionAt("TransferDatabase")->SetStyle("Button");

	// Info-bulles
	GetActionAt("ComputeStats")
	    ->SetHelpText("Analyze the database and to train prediction models."
			  "\n Khiops then performs discretizations for numerical variables, value groupings for "
			  "categorical variables."
			  "\n Variables are also constructed according to whether the variable construction options "
			  "are activated."
			  "\n Finally, the requested predictors are trained and evaluated."
			  "\n All the preparation, modeling and evaluation reports are produced.");
	GetActionAt("TransferDatabase")
	    ->SetHelpText("Open a dialog box allowing to specify an input and output database,"
			  "\n and a deployment dictionary describing the variables to keep, discard or derive."
			  "\n This allows to recode a database or to deploy a predictor on new data.");
	GetActionAt("Exit")->SetHelpText("Quit the tool.");

	// Short cuts
	GetFieldAt("TrainDatabase")->SetShortCut('A');
	GetFieldAt("AnalysisSpec")->SetShortCut('P');
	GetFieldAt("AnalysisResults")->SetShortCut('R');
}

KWLearningProblemView::~KWLearningProblemView()
{
	assert(trainDatabaseView != NULL);
	assert(testDatabaseView != NULL);

	// Seule la vue sur la base de test, non geree par l'interface, est a detruire
	delete testDatabaseView;

#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10: destruction de la gestion de la vue obsolete sur la base de test
		delete deprecatedEmptyDatabase;
		delete deprecatedTestDatabase;
	}
#endif // DEPRECATED_V10
}

void KWLearningProblemView::EventUpdate(Object* object)
{
	KWLearningProblem* editedObject;
	ALString sClassName;

	require(object != NULL);

	editedObject = cast(KWLearningProblem*, object);

	// Synchronisation des specifications des bases d'apprentissage et de test avec le dictionnaire en cours
	// L'appel aux methodes d'acces permet de faire cette synchronisation
	editedObject->GetTrainDatabase();
	editedObject->GetTestDatabase();

	// Parametrage de la classe de gestion des paires
	editedObject->GetAnalysisSpec()
	    ->GetModelingSpec()
	    ->GetAttributeConstructionSpec()
	    ->GetAttributePairsSpec()
	    ->SetClassName(editedObject->GetClassName());

	// Synchronisation egalement des interfaces pour forcer la coherence entre interface et objet edite
	trainDatabaseView->SetStringValueAt("ClassName", editedObject->GetTrainDatabase()->GetClassName());
	testDatabaseView->SetStringValueAt("ClassName", editedObject->GetTestDatabase()->GetClassName());

#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10: Recopie de la base de test obsolete vers la vraie base de test si necessaire
		// avec warning utilisateur si necessaire

		// Synchronisation egalement de l'interface obsolete
		deprecatedTestDatabaseView->SetStringValueAt("ClassName",
							     editedObject->GetTestDatabase()->GetClassName());

		// On met la base vide de reference a niveau pour la mapping multi-tables, ce qui est necessaire
		// pour detecter si la base de test obsolete a ete modifiee par l'utilisateur, et non seulement
		// du fait de sa structure multi-table
		deprecatedEmptyDatabase->SetClassName(deprecatedTestDatabase->GetClassName());
		cast(KWMTDatabase*, deprecatedEmptyDatabase)->UpdateMultiTableMappings();

		// On detecte si la base de test a fait l'objet de saisies utilisateur via un scenario
		if (deprecatedTestDatabase->Compare(deprecatedEmptyDatabase) != 0 or bDeprecatedTestDataViewUsed)
		{
			// On recopie les specification de la base de test obsolete vers la vraie base de test
			editedObject->GetTestDatabase()->CopyFrom(deprecatedTestDatabase);

			// On positionne les interfaces en mode "Specific"
			trainDatabaseView->SetStringValueAt("TestDatabaseSpecificationMode", "Specific");
			testDatabaseView->SetStringValueAt("TestDatabaseSpecificationMode", "Specific");
			trainDatabaseView->bDeprecatedTestDataViewUsed = true;
			testDatabaseView->bDeprecatedTestDataViewUsed = true;

			// On emet un warning
			if (not bDeprecatedTestDataViewWarningEmited)
			{
				editedObject->AddWarning("Pane 'Test database' is deprecated since Khiops V10 : see "
							 "release notes to avoid this message");
				bDeprecatedTestDataViewWarningEmited = true;
			}
		}
	}
#endif // DEPRECATED_V10

	// Mise a jour si necessaire la base de test a partir de la derniere version de la base d'apprentissage
	trainDatabaseView->UpdateTestDatabase();
}

void KWLearningProblemView::EventRefresh(Object* object)
{
	KWLearningProblem* editedObject;

	require(object != NULL);

	editedObject = cast(KWLearningProblem*, object);

	// Synchronisation des specifications des bases d'apprentissage et de test avec le dictionnaire en cours
	// L'appel aux methodes d'acces permet de faire cette synchronisation
	editedObject->GetTrainDatabase();
	editedObject->GetTestDatabase();

#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10: idem pour les base de gestion de la base de test obsolete
		deprecatedTestDatabase->SetClassName(editedObject->GetClassManagement()->GetClassName());
	}
#endif // DEPRECATED_V10

	// Rafraichissement des listes d'aide
	RefreshHelpLists();
}

//////////////////////////////////////////////////////////////////////////

void KWLearningProblemView::CheckData()
{
	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

	// OK si nom du fichier renseigne et classe correcte
	if (FileService::CreateApplicationTmpDir() and GetLearningProblem()->CheckTrainDatabaseName() and
	    GetLearningProblem()->GetTrainDatabase()->CheckSelectionValue(
		GetLearningProblem()->GetTrainDatabase()->GetSelectionValue()) and
	    GetLearningProblem()->CheckClass())
	{
		GetLearningProblem()->CheckData();
		AddSimpleMessage("");
	}
}

void KWLearningProblemView::SortDataTableByKey()
{
	KWDataTableSorterView dataTableSorterView;
	// Initialisation a partir de la base d'apprentissage
	dataTableSorterView.InitializeSourceDataTable(GetLearningProblem()->GetTrainDatabase());

	// Ouverture
	dataTableSorterView.Open();
}

void KWLearningProblemView::ExtractKeysFromDataTable()
{
	KWDataTableKeyExtractorView dataTableKeyExtractorView;
	int nClassNumber;
	int nNewClassNumber;

	// Memorisation du nombre de dictionnaires avant l'extraction des cles
	// (qui peut fabriquer des dictionnaires multi-tables)
	nClassNumber = KWClassDomain::GetCurrentDomain()->GetClassNumber();

	// Initialisation a partir de la base d'apprentissage
	dataTableKeyExtractorView.InitializeSourceDataTable(GetLearningProblem()->GetTrainDatabase());

	// Ouverture
	dataTableKeyExtractorView.Open();

	// Nombre de dictionnaires apres l'extraction des cles
	nNewClassNumber = KWClassDomain::GetCurrentDomain()->GetClassNumber();
	assert(nNewClassNumber == nClassNumber or nClassNumber > 0);

	// Memorisation du fichier dictionnaire si nouveaux dictionnaires construits
	if (nNewClassNumber > nClassNumber)
		GetLearningProblem()->GetClassManagement()->WriteClasses();
}

void KWLearningProblemView::BuildConstructedDictionary()
{
	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

	// OK si prerequis corrects
	if (FileService::CreateApplicationTmpDir() and GetLearningProblem()->CheckTrainDatabaseName() and
	    GetLearningProblem()->GetTrainDatabase()->Check() and
	    GetLearningProblem()->GetTrainDatabase()->CheckSelectionValue(
		GetLearningProblem()->GetTrainDatabase()->GetSelectionValue()) and
	    GetLearningProblem()->CheckClass() and GetLearningProblem()->CheckTargetAttribute() and
	    GetLearningProblem()->CheckResultFileNames())
	{
		// Verification supplementaire
		if (GetLearningProblem()
			->GetAnalysisSpec()
			->GetModelingSpec()
			->GetAttributeConstructionSpec()
			->GetConstructionDomain()
			->GetImportAttributeConstructionCosts())
			Global::AddError(
			    "", "", "To construct a dictionary, variable costs must not be imported from dictionary");
		else
			// Construction du dictionnaire de variables
			GetLearningProblem()->BuildConstructedDictionary();
	}
}

void KWLearningProblemView::ComputeStats()
{
	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

	// OK si prerequis corrects
	if (FileService::CreateApplicationTmpDir() and GetLearningProblem()->CheckTrainDatabaseName() and
	    GetLearningProblem()->GetTrainDatabase()->Check() and
	    GetLearningProblem()->GetTrainDatabase()->CheckSelectionValue(
		GetLearningProblem()->GetTrainDatabase()->GetSelectionValue()) and
	    GetLearningProblem()->GetTestDatabase()->CheckSelectionValue(
		GetLearningProblem()->GetTestDatabase()->GetSelectionValue()) and
	    GetLearningProblem()->CheckClass() and GetLearningProblem()->CheckTargetAttribute() and
#ifdef DEPRECATED_V10
	    GetLearningProblem()->CheckMandatoryAttributeInPairs() and
#endif // DEPRECATED_V10
	    GetLearningProblem()->CheckResultFileNames() and
	    GetLearningProblem()->GetAnalysisSpec()->GetRecoderSpec()->GetRecodingSpec()->Check() and
	    GetLearningProblem()->CheckRecodingSpecs() and GetLearningProblem()->CheckPreprocessingSpecs())
	{
		// Verifications supplementaires
		if (GetLearningProblem()
			    ->GetAnalysisSpec()
			    ->GetModelingSpec()
			    ->GetAttributeConstructionSpec()
			    ->GetMaxConstructedAttributeNumber() > 0 and
		    GetLearningProblem()
			->GetAnalysisSpec()
			->GetModelingSpec()
			->GetAttributeConstructionSpec()
			->GetConstructionDomain()
			->GetImportAttributeConstructionCosts())
			Global::AddError("", "",
					 "Max number of constructed variables must be 0 when variable costs are "
					 "imported from dictionary");
		else if (GetLearningProblem()
				 ->GetAnalysisSpec()
				 ->GetModelingSpec()
				 ->GetAttributeConstructionSpec()
				 ->GetMaxTextFeatureNumber() > 0 and
			 GetLearningProblem()
			     ->GetAnalysisSpec()
			     ->GetModelingSpec()
			     ->GetAttributeConstructionSpec()
			     ->GetConstructionDomain()
			     ->GetImportAttributeConstructionCosts())
			Global::AddError(
			    "", "",
			    "Max number of text features must be 0 when variable costs are imported from dictionary");
		else
		{
			// Calcul des stats
			GetLearningProblem()->ComputeStats();
			AddSimpleMessage("");
		}
	}
}

void KWLearningProblemView::TransferDatabase()
{
	KWDatabaseTransferView databaseTransferView;

	// Initialisation a partir de la base d'apprentissage
	databaseTransferView.InitializeSourceDatabase(GetLearningProblem()->GetTrainDatabase());

	// Ouverture
	databaseTransferView.Open();
}

void KWLearningProblemView::EvaluatePredictors()
{
	ALString sPredictorLabel;
	ALString sTargetAttributeName;
	ALString sMainTargetModality;
	ObjectArray oaClasses;
	ObjectArray oaTrainedPredictors;
	ObjectArray oaEvaluatedPredictors;
	ALString sReferenceTargetAttribute;
	KWPredictorEvaluator* predictorEvaluator;
	KWPredictorEvaluatorView predictorEvaluatorView;

	// Initialisation de l'evaluateur de predicteur avec les caracteristique disponibles
	predictorEvaluator = GetLearningProblem()->GetPredictorEvaluator();
	predictorEvaluator->GetEvaluationDatabase()->CopyFrom(GetLearningProblem()->GetTrainDatabase());
	predictorEvaluator->SetMainTargetModality(GetLearningProblem()->GetAnalysisSpec()->GetMainTargetModality());
	predictorEvaluator->FillEvaluatedPredictorSpecs();

	// Reinitialisation des parametre d'echantilonnage et de selection de la base d'evaluation
	predictorEvaluator->GetEvaluationDatabase()->InitializeSamplingAndSelection();

	// Ouverture
	predictorEvaluatorView.Open(predictorEvaluator);
}

void KWLearningProblemView::InterpretPredictor()
{
	KIPredictorInterpretationView view;

	// Initialisation a partir de la base d'apprentissage
	view.InitializeSourceDatabase(GetLearningProblem()->GetTrainDatabase());

	view.Open();
}

void KWLearningProblemView::SetObject(Object* object)
{
	KWLearningProblem* learningProblem;

	require(object != NULL);

	// Acces a l'objet edite
	learningProblem = cast(KWLearningProblem*, object);

	// Parametrage des sous-fiches
	cast(KWClassManagementView*, GetFieldAt("ClassManagement"))->SetObject(learningProblem->GetClassManagement());
	trainDatabaseView->SetObject(learningProblem->GetTrainDatabase());
	testDatabaseView->SetObject(learningProblem->GetTestDatabase());
	cast(KWAnalysisSpecView*, GetFieldAt("AnalysisSpec"))->SetObject(learningProblem->GetAnalysisSpec());
	cast(KWAnalysisResultsView*, GetFieldAt("AnalysisResults"))->SetObject(learningProblem->GetAnalysisResults());
	cast(KWLearningProblemActionView*, GetFieldAt("LearningTools"))->SetObject(learningProblem);

	// Parametrage des bases d'apprentissage et de test pour les actions
	// d'import/export entre ces bases
	trainDatabaseView->SetTestDatabase(learningProblem->GetTestDatabase());
	testDatabaseView->SetTrainDatabase(learningProblem->GetTrainDatabase());

#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10: parametrage de la vue sur la base de test obsolete
		deprecatedTestDatabaseView->SetTrainDatabase(learningProblem->GetTrainDatabase());
	}
#endif // DEPRECATED_V10

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

KWLearningProblem* KWLearningProblemView::GetLearningProblem()
{
	return cast(KWLearningProblem*, objValue);
}

///////////////////////////////////////////////////////////////////

void KWLearningProblemView::RefreshHelpLists()
{
	// Rafraichissement de la liste d'aide pour les attributs cibles (et les attribut obligatoires dans une  paire)
	classAttributeHelpList.FillAttributeNames(GetLearningProblem()->GetClassName(), true, true, false, true,
						  cast(UIList*, GetFieldAt("Attributes")), "Name");

	// Rafraichissement des valeurs de l'attribut cible
	dbTargetValuesHelpList.FillAttributeValues(GetLearningProblem()->GetTrainDatabase(),
						   GetLearningProblem()->GetTargetAttributeName(), true, false, true,
						   cast(UIList*, GetFieldAt("TargetValues")), "Value");
}
