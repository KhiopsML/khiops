// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCLearningProblemPostProcessingView.h"

CCLearningProblemPostProcessingView::CCLearningProblemPostProcessingView()
{
	CCPostProcessingSpecView* postProcessingSpecView;
	CCAnalysisResultsView* analysisResultsView;

	// Libelles
	SetIdentifier("CCLearningProblemPostProcessing");
	SetLabel("Coclustering simplification");

	// Creation des sous fiches (creation generique pour les vues sur bases de donnees)
	postProcessingSpecView = new CCPostProcessingSpecView;
	analysisResultsView = new CCAnalysisResultsView;

	// Ajout des sous-fiches
	AddCardField("PostProcessingSpec", "Simplification parameters", postProcessingSpecView);
	AddCardField("AnalysisResults", "Results", analysisResultsView);
	analysisResultsView->SetResultFieldsVisible(false);
	analysisResultsView->GetFieldAt("PostProcessedCoclusteringFileName")->SetVisible(true);
	analysisResultsView->GetFieldAt("ExportJSON")->SetVisible(true);

	// Passage en ergonomie onglets
	SetStyle("TabbedPanes");

	// Ajout d'actions sous formes de boutons
	AddAction("PostProcessCoclustering", "Simplify coclustering",
		  (ActionMethod)(&CCLearningProblemPostProcessingView::PostProcessCoclustering));
	GetActionAt("PostProcessCoclustering")->SetStyle("Button");

	// Info-bulles
	GetActionAt("PostProcessCoclustering")
	    ->SetHelpText(
		"Build the simplified coclustering report."
		"\n The input coclustering is simplified using a bottom-up hierarchical agglomeration of the parts,"
		"\n until al the active simplification constraints are fulfilled"
		"\n (max cell number, max preserved information and max part number per variable).");

	// Short cuts
	GetFieldAt("PostProcessingSpec")->SetShortCut('M');
	GetFieldAt("AnalysisResults")->SetShortCut('R');
	GetActionAt("PostProcessCoclustering")->SetShortCut('S');
}

CCLearningProblemPostProcessingView::~CCLearningProblemPostProcessingView() {}

//////////////////////////////////////////////////////////////////////////

void CCLearningProblemPostProcessingView::PostProcessCoclustering()
{
	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

	// OK si fichiers corrects
	if (GetLearningProblem()->CheckResultFileNames(CCLearningProblem::TaskPostProcessCoclustering))
	{
		// Simplification du coclustering
		GetLearningProblem()->PostProcessCoclustering();
		AddSimpleMessage("");
	}
}

void CCLearningProblemPostProcessingView::SetObject(Object* object)
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