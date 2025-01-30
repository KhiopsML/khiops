// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCLearningProblemView.h"

CCLearningProblemView::CCLearningProblemView()
{
	KWClassManagementView* classManagementView;
	KWDatabaseView* databaseView;
	CCAnalysisSpecView* analysisSpecView;
	CCAnalysisResultsView* analysisResultsView;
	CCLearningProblemActionView* learningProblemActionView;
	KWLearningProblemHelpCard* learningProblemHelpCard;
	UIList* simpleAttributeNameHelpList;
	UIList* continuousAttributeNameHelpList;
	CCCoclusteringSpecView* coclusteringSpecView;

	// Libelles
	SetIdentifier("CCLearningProblem");
	SetLabel(GetLearningMainWindowTitle());

	// On rend le bouton de sortie invisible sur la fenetre principale
	GetActionAt("Exit")->SetVisible(false);

	// Creation des sous fiches (creation generique pour les vues sur bases de donnees)
	classManagementView = new KWClassManagementView;
	databaseView = KWDatabaseView::CreateDefaultDatabaseTechnologyView();
	analysisSpecView = new CCAnalysisSpecView;
	analysisResultsView = new CCAnalysisResultsView;
	learningProblemActionView = new CCLearningProblemActionView;
	learningProblemHelpCard = new KWLearningProblemHelpCard;

	// Ajout des sous-fiches
	AddCardField("ClassManagement", "Data dictionary", classManagementView);
	AddCardField("Database", "Database", databaseView);
	AddCardField("AnalysisSpec", "Parameters", analysisSpecView);
	AddCardField("AnalysisResults", "Results", analysisResultsView);
	AddCardField("LearningTools", "Tools", learningProblemActionView);
	AddCardField("Help", "Help", learningProblemHelpCard);

	// Le champ des dictionnaires n'est pas visibles dans les parametrages de
	// base de donnees: il est defini dans la fiche de gestion des dictionnaires
	databaseView->GetFieldAt("ClassName")->SetVisible(false);

	// Seul le champ sur le rapport de coclustering est visible dans cette vue
	analysisResultsView->SetResultFieldsVisible(false);
	analysisResultsView->GetFieldAt("ShortDescription")->SetVisible(true);
	analysisResultsView->GetFieldAt("CoclusteringFileName")->SetVisible(true);
	analysisResultsView->GetFieldAt("ExportJSON")->SetVisible(true);

	// Parametrage de liste d'aide pour le nom du dictionnaire
	classManagementView->GetFieldAt("ClassName")->SetStyle("HelpedComboBox");
	classManagementView->GetFieldAt("ClassName")->SetParameters("ClassManagement.Classes:ClassName");

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
	editedObject->GetDatabase();

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
	editedObject->GetDatabase();

	// Rafraichissement des listes d'aide
	RefreshHelpLists();
}

//////////////////////////////////////////////////////////////////////////

void CCLearningProblemView::BuildCoclustering()
{
	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

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

	require(object != NULL);

	// Acces a l'objet edite
	learningProblem = cast(CCLearningProblem*, object);

	// Parametrage des sous-fiches
	cast(KWClassManagementView*, GetFieldAt("ClassManagement"))->SetObject(learningProblem->GetClassManagement());
	cast(KWDatabaseView*, GetFieldAt("Database"))->SetObject(learningProblem->GetDatabase());
	cast(CCAnalysisSpecView*, GetFieldAt("AnalysisSpec"))->SetObject(learningProblem->GetAnalysisSpec());
	cast(CCAnalysisResultsView*, GetFieldAt("AnalysisResults"))->SetObject(learningProblem->GetAnalysisResults());
	cast(CCLearningProblemActionView*, GetFieldAt("LearningTools"))->SetObject(learningProblem);

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}

CCLearningProblem* CCLearningProblemView::GetLearningProblem()
{
	return cast(CCLearningProblem*, objValue);
}

void CCLearningProblemView::RefreshHelpLists()
{
	// Rafraichissement de la liste d'aide pour les attributs de coclustering
	simpleAttributeHelpList.FillAttributeNames(GetLearningProblem()->GetClassName(), true, true, false, true,
						   cast(UIList*, GetFieldAt("SimpleAttributes")), "Name");

	// Rafraichissement de la liste d'aide pour l'attribut de frequency
	continuousAttributeHelpList.FillAttributeNames(GetLearningProblem()->GetClassName(), true, false, false, true,
						       cast(UIList*, GetFieldAt("ContinuousAttributes")), "Name");
}
