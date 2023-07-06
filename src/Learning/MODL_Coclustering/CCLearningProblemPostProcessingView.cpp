// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCLearningProblemPostProcessingView.h"

CCLearningProblemPostProcessingView::CCLearningProblemPostProcessingView()
{
	CCPostProcessingSpecView* postProcessingSpecView;

	// Libelles
	SetIdentifier("CCLearningProblemPostProcessing");
	SetLabel("Coclustering simplification");

	// Champ du coclustering simplifie en resultat
	AddStringField("PostProcessedCoclusteringFileName", "Simplified coclustering report", "");
	GetFieldAt("PostProcessedCoclusteringFileName")->SetStyle("FileChooser");

	// Creation des sous fiches (creation generique pour les vues sur bases de donnees)
	postProcessingSpecView = new CCPostProcessingSpecView;

	// Ajout des sous-fiches
	AddCardField("PostProcessingSpec", "Simplification parameters", postProcessingSpecView);

	// Ajout d'actions sous formes de boutons
	AddAction("PostProcessCoclustering", "Simplify coclustering",
		  (ActionMethod)(&CCLearningProblemPostProcessingView::PostProcessCoclustering));
	GetActionAt("PostProcessCoclustering")->SetStyle("Button");

	// Info-bulles
	GetFieldAt("PostProcessedCoclusteringFileName")
	    ->SetHelpText("Name of the simplified coclustering report,"
			  "\n that is the most detailed version of the input coclustering report"
			  "\n that meets all the simplification constraints.");
	GetActionAt("PostProcessCoclustering")
	    ->SetHelpText(
		"Build the simplified coclustering report."
		"\n The input coclustering is simplified using a bottom-up hierarchical agglomeration of the parts,"
		"\n until al the active simplification constraints are fulfilled"
		"\n (max cell number, max preserved information and max part number per variable).");

	// Short cuts
	GetFieldAt("PostProcessingSpec")->SetShortCut('M');
	GetActionAt("PostProcessCoclustering")->SetShortCut('S');
}

CCLearningProblemPostProcessingView::~CCLearningProblemPostProcessingView() {}

void CCLearningProblemPostProcessingView::EventUpdate(Object* object)
{
	CCLearningProblem* editedObject;

	require(object != NULL);

	// Appel de la methode ancetre
	CCLearningProblemToolView::EventUpdate(object);

	// Specialisation
	editedObject = cast(CCLearningProblem*, object);
	editedObject->GetAnalysisResults()->SetPostProcessedCoclusteringFileName(
	    GetStringValueAt("PostProcessedCoclusteringFileName"));
}

void CCLearningProblemPostProcessingView::EventRefresh(Object* object)
{
	CCLearningProblem* editedObject;

	require(object != NULL);

	// Appel de la methode ancetre
	CCLearningProblemToolView::EventRefresh(object);

	// Specialisation
	editedObject = cast(CCLearningProblem*, object);
	SetStringValueAt("PostProcessedCoclusteringFileName",
			 editedObject->GetAnalysisResults()->GetPostProcessedCoclusteringFileName());
}

//////////////////////////////////////////////////////////////////////////

void CCLearningProblemPostProcessingView::PostProcessCoclustering()
{
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

	// Memorisation de l'objet pour la fiche courante
	UIObjectView::SetObject(object);
}
