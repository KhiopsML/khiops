// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCLearningProblemClusterExtractionView.h"

CCLearningProblemClusterExtractionView::CCLearningProblemClusterExtractionView()
{
	CCPostProcessingSpecView* postProcessingSpecView;

	// Libelles
	SetIdentifier("CCLearningProblemPostProcessing");
	SetLabel("Cluster extraction");

	// Champ du coclustering simplifie en resultat
	AddStringField("CoclusteringAttribute", "Coclustering variable", "");
	GetFieldAt("CoclusteringAttribute")->SetStyle("HelpedComboBox");
	GetFieldAt("CoclusteringAttribute")->SetParameters("PostProcessingSpec.PostProcessedAttributes:Name");

	// Champ du fichier des clusters en resultat
	AddStringField("ClusterFileName", "Cluster table file", "");
	GetFieldAt("ClusterFileName")->SetStyle("FileChooser");

	// Creation des sous fiches (creation generique pour les vues sur bases de donnees)
	postProcessingSpecView = new CCPostProcessingSpecView;

	// Ajout des sous-fiches
	AddCardField("PostProcessingSpec", "Simplification parameters", postProcessingSpecView);

	// Ajout d'actions sous formes de boutons
	AddAction("ExtractClusters", "Extract clusters",
		  (ActionMethod)(&CCLearningProblemClusterExtractionView::ExtractClusters));
	GetActionAt("ExtractClusters")->SetStyle("Button");

	// Info-bulles
	GetFieldAt("CoclusteringAttribute")->SetHelpText("Name of the variable from which to extract the clusters.");
	GetFieldAt("ClusterFileName")->SetHelpText("Name of the file that contains the extracted clusters.");
	GetActionAt("ExtractClusters")
	    ->SetHelpText("Extract clusters from the input coclustering."
			  "\n The clusters are extracted for a given variable from the simplified coclustering"
			  "\n (provided that simplification parameters are specified).");

	// Short cuts
	GetFieldAt("PostProcessingSpec")->SetShortCut('M');
	GetActionAt("ExtractClusters")->SetShortCut('E');
}

CCLearningProblemClusterExtractionView::~CCLearningProblemClusterExtractionView() {}

void CCLearningProblemClusterExtractionView::EventUpdate(Object* object)
{
	CCLearningProblem* editedObject;

	require(object != NULL);

	// Appel de la methode ancetre
	CCLearningProblemToolView::EventUpdate(object);

	// Specialisation
	editedObject = cast(CCLearningProblem*, object);
	editedObject->GetAnalysisResults()->SetClusterFileName(GetStringValueAt("ClusterFileName"));
}

void CCLearningProblemClusterExtractionView::EventRefresh(Object* object)
{
	CCLearningProblem* editedObject;

	require(object != NULL);

	// Appel de la methode ancetre
	CCLearningProblemToolView::EventRefresh(object);

	// Specialisation
	editedObject = cast(CCLearningProblem*, object);
	SetStringValueAt("ClusterFileName", editedObject->GetAnalysisResults()->GetClusterFileName());
}

//////////////////////////////////////////////////////////////////////////

void CCLearningProblemClusterExtractionView::ExtractClusters()
{
	ALString sCoclusteringAttributeName;

	// OK si fichiers corrects
	if (GetLearningProblem()->CheckResultFileNames(CCLearningProblem::TaskExtractClusters))
	{
		// Acces au nom de l'attribut extrait
		sCoclusteringAttributeName = GetStringValueAt("CoclusteringAttribute");

		// Test si attribut specifie
		if (sCoclusteringAttributeName == "")
			Global::AddError("Cluster extraction", "", "Missing coclustering variable name");
		// Sinon, extraction des clusters
		else
			GetLearningProblem()->ExtractClusters(sCoclusteringAttributeName);
	}

	// Ligne de separation dans le log
	AddSimpleMessage("");
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

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}
