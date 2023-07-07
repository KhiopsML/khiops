// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCLearningProblemView.h"

CCLearningProblemView::CCLearningProblemView()
{
	KWClassManagementActionView* classManagementActionView;
	KWDatabaseView* databaseView;
	CCAnalysisSpecView* analysisSpecView;
	CCAnalysisResultsView* analysisResultsView;
	CCLearningProblemActionView* learningProblemActionView;
	KWLearningProblemHelpCard* learningProblemHelpCard;
	UIList* simpleAttributeNameHelpList;
	UIList* continuousAttributeNameHelpList;
	UIList* categoricalAttributeNameHelpList;
	CCCoclusteringSpecView* coclusteringSpecView;
	// CH IV Refactoring: renommer en varPartCoclusteringSpecView
	CCVarPartCoclusteringSpecView* instancesVariablesCoclusteringSpecView;

	// Libelles
	SetIdentifier("CCLearningProblem");
	SetLabel(GetLearningMainWindowTitle());

	// On rend le bouton de sortie invisible sur la fenetre principale
	GetActionAt("Exit")->SetVisible(false);

	// Creation des sous fiches (creation generique pour les vues sur bases de donnees)
	classManagementActionView = new KWClassManagementActionView;
	databaseView = KWDatabaseView::CreateDefaultDatabaseTechnologyView();
	analysisSpecView = new CCAnalysisSpecView;
	analysisResultsView = new CCAnalysisResultsView;
	learningProblemActionView = new CCLearningProblemActionView;
	learningProblemHelpCard = new KWLearningProblemHelpCard;

	// Ajout des sous-fiches
	AddCardField("ClassManagement", "Data dictionary", classManagementActionView);
	AddCardField("Database", "Database", databaseView);
	AddCardField("AnalysisSpec", "Parameters", analysisSpecView);
	AddCardField("AnalysisResults", "Results", analysisResultsView);
	AddCardField("LearningTools", "Tools", learningProblemActionView);
	AddCardField("Help", "Help", learningProblemHelpCard);

	// Ajout pour la basxe de train d'un champ en read-only pour rappeler le nom du fichier de dictionnaire
	// Attention, il s'agit d'un champ d'interface uniquement, a mettre ajour dans EventRefresh
	databaseView->AddStringField("ClassFileName", "Dictionary file", "");
	databaseView->GetFieldAt("ClassFileName")->SetEditable(false);
	databaseView->GetFieldAt("ClassFileName")->SetHelpText("Name of the current dictionary file.");
	databaseView->MoveFieldBefore("ClassFileName", "DatabaseSpec");

	// On montre le nom du dictionnaire sur les bases
	// On utilise le meme nom que dans l'onglet ClassManagement
	databaseView->GetFieldAt("ClassName")->SetLabel("Analysis dictionary");

	// On parametre la liste des dictionnaires en fonction des dictionnaire charges dans ClassManagement
	databaseView->GetFieldAt("ClassName")->SetStyle("HelpedComboBox");
	databaseView->GetFieldAt("ClassName")->SetParameters("ClassManagement.Classes:ClassName");

	// On indique que le champ de parametrage du dictionnaire declenche une action de rafraichissement
	// de l'interface immediatement apres une mise a jour, pour pouvoir rafraichir les mapping des databases
	cast(UIElement*, databaseView->GetFieldAt("ClassName"))->SetTriggerRefresh(true);

	// Le champ est visible dans l'onglet train
	databaseView->GetFieldAt("ClassName")->SetVisible(true);

	// Aide specifique a la base de train
	databaseView->GetFieldAt("ClassName")
	    ->SetHelpText("Name of the dictionary related to the database."
			  "\n Automatically generated from data table file if not specified");

	// Specialisation du parametrage des listes d'aide de la base
	databaseView->SetHelpListViewPath("Database");

	// Creation d'une liste cachee des attributs de type Continuous de la classe en cours
	continuousAttributeNameHelpList = new UIList;
	continuousAttributeNameHelpList->AddStringField("Name", "Name", "");
	AddListField("ContinuousAttributes", "Simple variables", continuousAttributeNameHelpList);
	continuousAttributeNameHelpList->SetVisible(false);

	// Parametrage de liste d'aide pour le nom de l'attribut d'effectif pour le coclustering
	coclusteringSpecView = cast(CCCoclusteringSpecView*, analysisSpecView->GetFieldAt("CoclusteringParameters"));
	coclusteringSpecView->GetFieldAt("FrequencyAttribute")->SetStyle("HelpedComboBox");
	coclusteringSpecView->GetFieldAt("FrequencyAttribute")->SetParameters("ContinuousAttributes:Name");

	// CH IV Begin
	// CH IV Refactoring: plutot tout declarer et utiliser "normalement" (partout dans ce fichier source),
	//   et parametrer la visibilite selon GetLearningCoclusteringIVExpertMode()

	// Parametrage de liste d'aide pour le nom de l'attribut identifiant pour le coclustering individus * variables
	if (GetLearningCoclusteringIVExpertMode())
	{
		instancesVariablesCoclusteringSpecView =
		    cast(CCVarPartCoclusteringSpecView*, analysisSpecView->GetFieldAt("VarPartCoclusteringParameters"));

		// Creation d'une liste cachee des attributs de type Categorical de la classe en cours
		categoricalAttributeNameHelpList = new UIList;
		categoricalAttributeNameHelpList->AddStringField("Name", "Name", "");
		AddListField("CategoricalAttributes", "Simple variables", categoricalAttributeNameHelpList);
		categoricalAttributeNameHelpList->SetVisible(false);

		instancesVariablesCoclusteringSpecView->GetFieldAt("IdentifierAttribute")->SetStyle("HelpedComboBox");
		instancesVariablesCoclusteringSpecView->GetFieldAt("IdentifierAttribute")
		    ->SetParameters("CategoricalAttributes:Name");
	}
	// CH IV End

	// Creation d'une liste cachee des attributs de type simple de la classe en cours
	simpleAttributeNameHelpList = new UIList;
	simpleAttributeNameHelpList->AddStringField("Name", "Name", "");
	AddListField("SimpleAttributes", "Simple variables", simpleAttributeNameHelpList);
	simpleAttributeNameHelpList->SetVisible(false);

	// Parametrage de liste d'aide pour le nom des attributs de coclustering
	coclusteringSpecView = cast(CCCoclusteringSpecView*, analysisSpecView->GetFieldAt("CoclusteringParameters"));
	cast(KWAttributeNameArrayView*, coclusteringSpecView->GetFieldAt("Attributes"))
	    ->GetFieldAt("Name")
	    ->SetStyle("HelpedComboBox");
	cast(KWAttributeNameArrayView*, coclusteringSpecView->GetFieldAt("Attributes"))
	    ->GetFieldAt("Name")
	    ->SetParameters("SimpleAttributes:Name");

	// CH IV Begin
	if (GetLearningCoclusteringIVExpertMode())
	{
		// Parametrage de liste d'aide pour le nom des attributs de coclustering VarPart
		instancesVariablesCoclusteringSpecView =
		    cast(CCVarPartCoclusteringSpecView*, analysisSpecView->GetFieldAt("VarPartCoclusteringParameters"));
		cast(KWAttributeAxisNameArrayView*, instancesVariablesCoclusteringSpecView->GetFieldAt("Attributes"))
		    ->GetFieldAt("AttributeName")
		    ->SetStyle("HelpedComboBox");
		cast(KWAttributeAxisNameArrayView*, instancesVariablesCoclusteringSpecView->GetFieldAt("Attributes"))
		    ->GetFieldAt("AttributeName")
		    ->SetParameters("SimpleAttributes:Name");
	}
	// CH IV End

	// Passage en ergonomie onglets
	SetStyle("TabbedPanes");

	// Ajout d'actions sous formes de boutons
	AddAction("BuildCoclustering", "Train coclustering", (ActionMethod)(&CCLearningProblemView::BuildCoclustering));
	GetActionAt("BuildCoclustering")->SetStyle("Button");

	// Info-bulles
	GetActionAt("BuildCoclustering")
	    ->SetHelpText("Train a coclustering model given the coclustering parameters."
			  "\n This action is anytime: coclustering models are computed and continuously improved,"
			  "\n with new solutions saved as soon as improvements are reached."
			  "\n The intermediate solutions can be used without waiting for the final solution,"
			  "\n and the process can be stopped at any time to keep the last best solution.");
	// CH IV Begin
	if (GetLearningCoclusteringIVExpertMode())
		GetActionAt("BuildCoclustering")
		    ->SetHelpText(
			"Build a coclustering model given the coclustering parameters."
			"\n A variable * variable or an instances * variables coclustering is computed given the "
			"parameters."
			"\n This action is anytime: coclustering models are computed and continuously improved,"
			"\n with new solutions saved as soon as improvements are reached."
			"\n The intermediate solutions can be used without waiting for the final solution,"
			"\n and the process can be stopped at any time to keep the last best solution.");

	// CH IV End
	GetActionAt("Exit")->SetHelpText("Quit the tool.");

	// Short cuts
	GetFieldAt("Database")->SetShortCut('A');
	GetFieldAt("AnalysisSpec")->SetShortCut('P');
	GetFieldAt("AnalysisResults")->SetShortCut('R');
}

CCLearningProblemView::~CCLearningProblemView() {}

void CCLearningProblemView::EventUpdate(Object* object)
{
	CCLearningProblem* editedObject;
	ALString sClassName;

	require(object != NULL);

	editedObject = cast(CCLearningProblem*, object);

	// Synchronisation des specifications des bases d'apprentissage et de test avec le dictionnaire en cours
	// L'appel aux methodes d'acces permet de faire cette synchronisation
	editedObject->UpdateClassNameFromTrainDatabase();

	// Synchronisation egalement des interfaces pour forcer la coherence entre interface et objet edite
	cast(KWDatabaseView*, GetFieldAt("Database"))
	    ->SetStringValueAt("ClassName", editedObject->GetDatabase()->GetClassName());
}

void CCLearningProblemView::EventRefresh(Object* object)
{
	CCLearningProblem* editedObject;

	require(object != NULL);

	editedObject = cast(CCLearningProblem*, object);

	// Synchronisation des specifications des bases d'apprentissage et de test avec le dictionnaire en cours
	// L'appel aux methodes d'acces permet de faire cette synchronisation
	editedObject->UpdateClassNameFromTrainDatabase();

	// On rappatrie la valeur du ClassFileName dans le champ read-only correspondant de la base
	cast(KWDatabaseView*, GetFieldAt("Database"))
	    ->SetStringValueAt("ClassFileName", editedObject->GetClassManagement()->GetClassFileName());

	// Rafraichissement des listes d'aide
	RefreshHelpLists();
}

//////////////////////////////////////////////////////////////////////////

void CCLearningProblemView::BuildCoclustering()
{
	boolean bOk = true;

	// Test si on a pas specifie de dictionnaire d'analyse, pour le construire automatiquement a la volee
	if (GetLearningProblem()->CheckDatabaseName() and GetLearningProblem()->GetDatabase()->GetClassName() == "")
		bOk = BuildClassFromDataTable();

	// OK si nom du fichier renseigne et classe correcte
	if (GetLearningProblem()->CheckClass() and GetLearningProblem()->CheckDatabaseName() and
	    GetLearningProblem()->GetDatabase()->Check() and
	    GetLearningProblem()->GetDatabase()->CheckSelectionValue(
		GetLearningProblem()->GetDatabase()->GetSelectionValue()) and
	    GetLearningProblem()->CheckCoclusteringAttributeNames() and
	    GetLearningProblem()->CheckResultFileNames(CCLearningProblem::TaskBuildCoclustering))
	{
		// Calcul des stats
		GetLearningProblem()->BuildCoclustering();
		AddSimpleMessage("");
	}
}

void CCLearningProblemView::SetObject(Object* object)
{
	CCLearningProblem* learningProblem;
	KWClassManagementActionView* classManagementActionView;

	require(object != NULL);

	// Acces a l'objet edite
	learningProblem = cast(CCLearningProblem*, object);

	// Parametrage des sous-fiches
	classManagementActionView = cast(KWClassManagementActionView*, GetFieldAt("ClassManagement"));
	classManagementActionView->SetObject(learningProblem->GetClassManagement());
	cast(KWDatabaseView*, GetFieldAt("Database"))->SetObject(learningProblem->GetDatabase());
	cast(CCAnalysisSpecView*, GetFieldAt("AnalysisSpec"))->SetObject(learningProblem->GetAnalysisSpec());
	cast(CCAnalysisResultsView*, GetFieldAt("AnalysisResults"))->SetObject(learningProblem->GetAnalysisResults());
	cast(CCLearningProblemActionView*, GetFieldAt("LearningTools"))->SetObject(learningProblem);

	// Parametrage de la vue ClassManagement pour qu'elle connaisse la base
	classManagementActionView->SetTrainDatabase(learningProblem->GetDatabase());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

CCLearningProblem* CCLearningProblemView::GetLearningProblem()
{
	return cast(CCLearningProblem*, objValue);
}

boolean CCLearningProblemView::BuildClassFromDataTable()
{
	boolean bOk = true;
	KWDatabaseFormatDetector databaseFormatDetector;
	KWClassManagementActionView* classManagementActionView;
	ALString sClassManagementMenuItemName;
	KWDatabase* database;
	ALString sClassName;
	KWClass* kwcClass;

	require(GetLearningProblem()->CheckDatabaseName() and
		GetLearningProblem()->GetDatabase()->GetClassName() == "");

	// Recherche de la base de travail dans une variable locale, pour travailler les contraintes de coherence
	// globale
	database = GetLearningProblem()->GetDatabase();

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
			// Synchronisation entre ClassManagement et la database
			GetLearningProblem()->UpdateClassNameFromTrainDatabase();
		}
		// Reinitialisation sinon
		else
			database->SetClassName("");
		assert(database == GetLearningProblem()->GetDatabase());

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

void CCLearningProblemView::RefreshHelpLists()
{
	// Rafraichissement de la liste d'aide pour les attributs de coclustering
	simpleAttributeHelpList.FillAttributeNames(GetLearningProblem()->GetClassName(), true, true, false, true,
						   cast(UIList*, GetFieldAt("SimpleAttributes")), "Name");

	// Rafraichissement de la liste d'aide pour l'attribut de frequency
	continuousAttributeHelpList.FillAttributeNames(GetLearningProblem()->GetClassName(), true, false, false, true,
						       cast(UIList*, GetFieldAt("ContinuousAttributes")), "Name");

	// CH IV Begin
	if (GetLearningCoclusteringIVExpertMode())
	{
		// Rafraichissement de la liste d'aide pour l'attribut identifiant
		categoricalAttributeHelpList.FillAttributeNames(
		    GetLearningProblem()->GetClassName(), false, true, false, true,
		    cast(UIList*, GetFieldAt("CategoricalAttributes")), "Name");
	}
	// CH IV End
}
