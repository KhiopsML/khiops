// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWLearningProblemView.h"

KWLearningProblemView::KWLearningProblemView()
{
	KWClassManagementActionView* classManagementActionView;
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
	classManagementActionView = new KWClassManagementActionView;
	trainDatabaseView = KWDatabaseView::CreateDefaultDatabaseTechnologyView();
	analysisSpecView = new KWAnalysisSpecView;
	analysisResultsView = new KWAnalysisResultsView;
	learningProblemActionView = new KWLearningProblemActionView;
	learningProblemHelpCard = new KWLearningProblemHelpCard;

	// Ajout des sous-fiches
	AddCardField("ClassManagement", "Data dictionary", classManagementActionView);
	AddCardField("TrainDatabase", "Train database", trainDatabaseView);
	AddCardField("AnalysisSpec", "Parameters", analysisSpecView);
	AddCardField("AnalysisResults", "Results", analysisResultsView);
	AddCardField("LearningTools", "Tools", learningProblemActionView);
	AddCardField("Help", "Help", learningProblemHelpCard);

	// Ajout pour la base de train d'un champ en read-only pour rappeler le nom du fichier de dictionnaire
	// Attention, il s'agit d'un champ d'interface uniquement, a mettre ajour dans EventRefresh
	trainDatabaseView->AddStringField("ClassFileName", "Dictionary file", "");
	trainDatabaseView->GetFieldAt("ClassFileName")->SetEditable(false);
	trainDatabaseView->GetFieldAt("ClassFileName")->SetHelpText("Name of the current dictionary file.");
	trainDatabaseView->MoveFieldBefore("ClassFileName", "DatabaseSpec");

	// Creation de la fiche de base de test, qui sera lancee depuis la fiche de base de train
	testDatabaseView = KWDatabaseView::CreateDefaultDatabaseTechnologyView();

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

	// Parametrage de la vue sur la base de train par la vue sur la base de test,
	// pour permettre d'editer la base de test
	trainDatabaseView->SetTestDatabaseView(testDatabaseView);

	// On montre le nom du dictionnaire sur les bases
	// On utilise le meme nom que dans l'onglet ClassManagement
	trainDatabaseView->GetFieldAt("ClassName")->SetLabel("Analysis dictionary");

	// On parametre la liste des dictionnaires en fonction des dictionnaire charges dans ClassManagement
	trainDatabaseView->GetFieldAt("ClassName")->SetStyle("HelpedComboBox");
	trainDatabaseView->GetFieldAt("ClassName")->SetParameters("ClassManagement.Classes:ClassName");

	// On indique que le champ de parametrage du dictionnaire declenche une action de rafraichissement
	// de l'interface immediatement apres une mise a jour, pour pouvoir rafraichir les mapping des databases
	cast(UIElement*, trainDatabaseView->GetFieldAt("ClassName"))->SetTriggerRefresh(true);

	// Le champ est visible dans l'onglet train
	trainDatabaseView->GetFieldAt("ClassName")->SetVisible(true);

	// Aide specifique a la base de train
	trainDatabaseView->GetFieldAt("ClassName")
	    ->SetHelpText("Name of the dictionary related to the database."
			  "\n Automatically generated from data table file if not specified");

	// Le champ est visible dans l'onglet test, mais pas editable
	testDatabaseView->GetFieldAt("ClassName")->SetVisible(true);
	testDatabaseView->GetFieldAt("ClassName")->SetEditable(false);

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
}

void KWLearningProblemView::EventUpdate(Object* object)
{
	KWLearningProblem* editedObject;
	ALString sClassName;

	require(object != NULL);

	editedObject = cast(KWLearningProblem*, object);

	// Synchronisation des specifications des bases d'apprentissage et de test avec le dictionnaire en cours
	// L'appel aux methodes d'acces permet de faire cette synchronisation
	editedObject->UpdateClassNameFromTrainDatabase();

	// Parametrage de la classe de gestion des paires
	editedObject->GetAnalysisSpec()
	    ->GetModelingSpec()
	    ->GetAttributeConstructionSpec()
	    ->GetAttributePairsSpec()
	    ->SetClassName(editedObject->GetClassName());

	// Synchronisation egalement des interfaces pour forcer la coherence entre interface et objet edite
	trainDatabaseView->SetStringValueAt("ClassName", editedObject->GetTrainDatabase()->GetClassName());
	testDatabaseView->SetStringValueAt("ClassName", editedObject->GetTestDatabase()->GetClassName());

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
	editedObject->UpdateClassNameFromTrainDatabase();

	// On rappatrie la valeur du ClassFileName dans le champ read-only correspondant de la base de train
	trainDatabaseView->SetStringValueAt("ClassFileName", editedObject->GetClassManagement()->GetClassFileName());

	// Rafraichissement des listes d'aide
	RefreshHelpLists();
}

//////////////////////////////////////////////////////////////////////////

void KWLearningProblemView::CheckData()
{
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
		{
			// Construction du dictionnaire de variables
			GetLearningProblem()->BuildConstructedDictionary();
			AddSimpleMessage("");
		}
	}
}

void KWLearningProblemView::ComputeStats()
{
	boolean bOk = true;

	// Test si on a pas specifie de dictionnaire d'analyse, pour le construire automatiquement a la volee
	if (GetLearningProblem()->CheckTrainDatabaseName() and
	    GetLearningProblem()->GetTrainDatabase()->GetClassName() == "")
		bOk = BuildClassFromDataTable();

	// OK si prerequis corrects
	if (bOk and FileService::CreateApplicationTmpDir() and GetLearningProblem()->CheckTrainDatabaseName() and
	    GetLearningProblem()->GetTrainDatabase()->Check() and
	    GetLearningProblem()->GetTrainDatabase()->CheckSelectionValue(
		GetLearningProblem()->GetTrainDatabase()->GetSelectionValue()) and
	    GetLearningProblem()->GetTestDatabase()->CheckSelectionValue(
		GetLearningProblem()->GetTestDatabase()->GetSelectionValue()) and
	    GetLearningProblem()->CheckClass() and GetLearningProblem()->CheckTargetAttribute() and
	    GetLearningProblem()->CheckResultFileNames() and
	    GetLearningProblem()
		->GetAnalysisSpec()
		->GetModelingSpec()
		->GetAttributeConstructionSpec()
		->GetTextFeatureSpec()
		->Check() and
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
	databaseTransferView.InitializeSourceDatabase(GetLearningProblem()->GetTrainDatabase(),
						      GetLearningProblem()->GetClassFileName());

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
	view.InitializeSourceDatabase(GetLearningProblem()->GetTrainDatabase(),
				      GetLearningProblem()->GetClassFileName());

	view.Open();
}

void KWLearningProblemView::SetObject(Object* object)
{
	KWLearningProblem* learningProblem;
	KWClassManagementActionView* classManagementActionView;

	require(object != NULL);

	// Acces a l'objet edite
	learningProblem = cast(KWLearningProblem*, object);

	// Parametrage des sous-fiches
	classManagementActionView = cast(KWClassManagementActionView*, GetFieldAt("ClassManagement"));
	classManagementActionView->SetObject(learningProblem->GetClassManagement());
	trainDatabaseView->SetObject(learningProblem->GetTrainDatabase());
	testDatabaseView->SetObject(learningProblem->GetTestDatabase());
	cast(KWAnalysisSpecView*, GetFieldAt("AnalysisSpec"))->SetObject(learningProblem->GetAnalysisSpec());
	cast(KWAnalysisResultsView*, GetFieldAt("AnalysisResults"))->SetObject(learningProblem->GetAnalysisResults());
	cast(KWLearningProblemActionView*, GetFieldAt("LearningTools"))->SetObject(learningProblem);

	// Parametrage des bases d'apprentissage et de test pour les actions
	// d'import/export entre ces bases
	trainDatabaseView->SetTestDatabase(learningProblem->GetTestDatabase());
	testDatabaseView->SetTrainDatabase(learningProblem->GetTrainDatabase());

	// Parametrage de la vue ClassManagement pour qu'elle connaisse les bases de train et test
	classManagementActionView->SetTrainDatabase(learningProblem->GetTrainDatabase());
	classManagementActionView->SetTestDatabase(learningProblem->GetTestDatabase());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

KWLearningProblem* KWLearningProblemView::GetLearningProblem()
{
	return cast(KWLearningProblem*, objValue);
}

///////////////////////////////////////////////////////////////////

boolean KWLearningProblemView::BuildClassFromDataTable()
{
	boolean bOk = true;
	KWDatabaseFormatDetector databaseFormatDetector;
	KWClassManagementActionView* classManagementActionView;
	ALString sClassManagementMenuItemName;
	KWDatabase* database;
	ALString sClassName;
	KWClass* kwcClass;

	require(GetLearningProblem()->CheckTrainDatabaseName() and
		GetLearningProblem()->GetTrainDatabase()->GetClassName() == "");

	// Recherche de la base de travail dans une variable locale, pour travailler les contraintes de coherence
	// globale
	database = GetLearningProblem()->GetTrainDatabase();

	// Detection du format de fichier
	databaseFormatDetector.SetDatabase(database);
	bOk = databaseFormatDetector.DetectFileFormat();

	// Construction automatique de dictionnaire
	if (bOk)
	{
		// On initialise le nom a partir du prefix du fichier d'apprentissage
		sClassName = FileService::GetFilePrefix(database->GetDatabaseName());
		if (sClassName == "")
			sClassName = FileService::GetFileSuffix(database->GetDatabaseName());

		// On recherche un nom de classe nouveau
		sClassName = KWClassDomain::GetCurrentDomain()->BuildClassName(sClassName);

		// Demarage du suivi de la tache
		TaskProgression::SetTitle("Build dictionary fom data table");
		TaskProgression::Start();

		// Parametrage du driver la base source pour qu'il n'emette pas de warning pour des champs categoriels
		// trop long Cela permet d'identifier des champs Text via des champs categoriel
		KWDataTableDriverTextFile::SetOverlengthyFieldsVerboseMode(false);

		// Construction effective de la classe
		database->SetClassName(sClassName);
		kwcClass = database->ComputeClass();
		bOk = (kwcClass != NULL);

		// Synchronisation des nom de dictionnaire partout si ok
		if (bOk)
		{
			// Synchronisation entre ClassManagement et les databases
			GetLearningProblem()->UpdateClassNameFromTrainDatabase();

			// Parametrage de la classe de gestion des paires egalement
			GetLearningProblem()
			    ->GetAnalysisSpec()
			    ->GetModelingSpec()
			    ->GetAttributeConstructionSpec()
			    ->GetAttributePairsSpec()
			    ->SetClassName(sClassName);
		}
		// Reinitialisation sinon
		else
			database->SetClassName("");
		assert(database == GetLearningProblem()->GetTrainDatabase());

		// Restitutuion du parametrage initial du driver
		KWDataTableDriverTextFile::SetOverlengthyFieldsVerboseMode(true);

		// Fin du suivi de la tache
		TaskProgression::Stop();
	}

	// Nom du menu pour la gestion des dictionnaires
	classManagementActionView = cast(KWClassManagementActionView*, GetFieldAt("ClassManagement"));
	sClassManagementMenuItemName = classManagementActionView->GetLabel();
	sClassManagementMenuItemName += "/";
	sClassManagementMenuItemName += classManagementActionView->GetActionAt("ManageClasses")->GetLabel();

	// Warning utilisateur pour prevenir de cet usage un peu atypique et de ses limites
	if (bOk)
		Global::AddWarning("", "",
				   "A dictionary has been generated automatically. "
				   "The field types should be checked and the dictionary saved if necessary. "
				   "For standard use, refer to the '" +
				       sClassManagementMenuItemName + "' menu.\n");

	return bOk;
}

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
