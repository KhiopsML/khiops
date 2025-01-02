// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCLearningProblemClusterExtractionView.h"

CCLearningProblemClusterExtractionView::CCLearningProblemClusterExtractionView()
{
	CCPostProcessingSpecView* postProcessingSpecView;
	CCAnalysisResultsView* analysisResultsView;
	UICard* coclusteringAttributeSpecView;

	// Libelles
	SetIdentifier("CCLearningProblemPostProcessing");
	SetLabel("Cluster extraction");

	// Creation des sous fiches (creation generique pour les vues sur bases de donnees)
	postProcessingSpecView = new CCPostProcessingSpecView;
	analysisResultsView = new CCAnalysisResultsView;

	// Creation d'une sous-fiche "en dur" pour le nom de la variable de coclustering dont il faut extraire les
	// cluster Ce n'est pas la peine de creer une structure pour memoriser ce seul parametre
	coclusteringAttributeSpecView = new UICard;
	coclusteringAttributeSpecView->AddStringField("CoclusteringAttribute", "Coclustering variable", "");

	// Ajout des sous-fiches
	AddCardField("PostProcessingSpec", "Simplification parameters", postProcessingSpecView);
	AddCardField("CoclusteringAttributeSpec", "Cluster parameter", coclusteringAttributeSpecView);
	AddCardField("AnalysisResults", "Results", analysisResultsView);
	analysisResultsView->SetResultFieldsVisible(false);
	analysisResultsView->GetFieldAt("ClusterFileName")->SetVisible(true);

	// Parametrage de liste d'aide pour le nom de l'attribut de coclustering a deployer
	coclusteringAttributeSpecView->GetFieldAt("CoclusteringAttribute")->SetStyle("HelpedComboBox");
	coclusteringAttributeSpecView->GetFieldAt("CoclusteringAttribute")
	    ->SetParameters("PostProcessingSpec.PostProcessedAttributes:Name");

	// Passage en ergonomie onglets
	SetStyle("TabbedPanes");

	// Ajout d'actions sous formes de boutons
	AddAction("ExtractClusters", "Extract clusters",
		  (ActionMethod)(&CCLearningProblemClusterExtractionView::ExtractClusters));
	GetActionAt("ExtractClusters")->SetStyle("Button");

	// Info-bulles
	coclusteringAttributeSpecView->GetFieldAt("CoclusteringAttribute")
	    ->SetHelpText("Name of the variable from which to extract the clusters.");
	GetActionAt("ExtractClusters")
	    ->SetHelpText("Extract clusters from the input coclustering."
			  "\n The clusters are extracted for a given variable from the simplified coclustering"
			  "\n (provided that simplification parameters are specified).");

	// Short cuts
	GetFieldAt("PostProcessingSpec")->SetShortCut('M');
	GetFieldAt("CoclusteringAttributeSpec")->SetShortCut('C');
	GetFieldAt("AnalysisResults")->SetShortCut('R');
	GetActionAt("ExtractClusters")->SetShortCut('E');
}

CCLearningProblemClusterExtractionView::~CCLearningProblemClusterExtractionView() {}

//////////////////////////////////////////////////////////////////////////

void CCLearningProblemClusterExtractionView::ExtractClusters()
{
	ALString sCoclusteringAttributeName;

	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

	// OK si fichiers corrects
	if (GetLearningProblem()->CheckResultFileNames(CCLearningProblem::TaskPostProcessCoclustering))
	{
		// Acces au nom de l'attribut extrait
		sCoclusteringAttributeName =
		    cast(UICard*, GetFieldAt("CoclusteringAttributeSpec"))->GetStringValueAt("CoclusteringAttribute");

		// Test si attribut specifie
		if (sCoclusteringAttributeName == "")
			Global::AddError("Cluster extraction", "", "Missing coclustering variable name");
		// Sinon, extraction des clusters
		else
		{
			GetLearningProblem()->ExtractClusters(sCoclusteringAttributeName);
			AddSimpleMessage("");
		}
	}
}

void CCLearningProblemClusterExtractionView::SetObject(Object* object)
{
	CCLearningProblem* learningProblem;

	require(object != NULL);

	// Acces a l'objet edite
	learningProblem = cast(CCLearningProblem*, object);

	// Parametrage des sous-fiches
	cast(CCPostProcessingSpecView*, GetFieldAt("PostProcessingSpec"))
	    ->SetObject(learningProblem->GetPostProcessingSpec());
	cast(CCAnalysisResultsView*, GetFieldAt("AnalysisResults"))->SetObject(learningProblem->GetAnalysisResults());

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}
