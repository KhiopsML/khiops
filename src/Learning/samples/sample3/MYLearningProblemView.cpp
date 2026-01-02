// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MYLearningProblemView.h"

MYLearningProblemView::MYLearningProblemView()
{
	MYAnalysisSpecView* analysisSpecView;
	MYAnalysisResultsView* analysisResultsView;
	const ALString sAnalysisSpecIdentifier = "AnalysisSpec";
	const ALString sAnalysisResultsIdentifier = "AnalysisResults";

	// Specialisation de la fiche des parametres d'analyse,
	// en remplacant l'ancienne version par une sous-classe
	analysisSpecView = new MYAnalysisSpecView;
	ReplaceCardField(sAnalysisSpecIdentifier, analysisSpecView);

	// Parametrage de liste d'aide pour le nom de l'attribut cible
	analysisSpecView->GetFieldAt("TargetAttributeName")->SetStyle("HelpedComboBox");
	analysisSpecView->GetFieldAt("TargetAttributeName")->SetParameters("Attributes:Name");

	// Parametrage de liste d'aide pour les modalites de l'attribut cible
	analysisSpecView->GetFieldAt("MainTargetModality")->SetStyle("HelpedComboBox");
	analysisSpecView->GetFieldAt("MainTargetModality")->SetParameters("Modalities:Value");

	// Specialisation de la fiche des resultats d'analyse,
	// en remplacant l'ancienne version par une sous-classe
	analysisResultsView = new MYAnalysisResultsView;
	ReplaceCardField(sAnalysisResultsIdentifier, analysisResultsView);

	// Fonctionnalites avancees, disponible uniquement pour l'auteur
	if (GetLearningExpertMode())
	{
		AddCardField("LearningProblemStudy", "Benchmark", new MYLearningProblemExtendedActionView);
	}
}

MYLearningProblemView::~MYLearningProblemView() {}

void MYLearningProblemView::ClassifierBenchmark()
{
	KWLearningBenchmark* classifierBenchmark;
	KWLearningBenchmarkView view;

	// Acces au parametrage du benchmark
	classifierBenchmark = GetMyLearningProblem()->GetClassifierBenchmark();
	assert(classifierBenchmark->GetTargetAttributeType() == KWType::Symbol);

	// Ouverture de la fenetre
	view.SetObject(classifierBenchmark);
	view.Open();
}

void MYLearningProblemView::RegressorBenchmark()
{
	KWLearningBenchmark* regressorBenchmark;
	KWLearningBenchmarkView view;

	// Acces au parametrage du benchmark
	regressorBenchmark = GetMyLearningProblem()->GetRegressorBenchmark();
	assert(regressorBenchmark->GetTargetAttributeType() == KWType::Continuous);

	// Ouverture de la fenetre
	view.SetObject(regressorBenchmark);
	view.Open();
}

void MYLearningProblemView::SetObject(Object* object)
{
	MYLearningProblem* learningProblem;

	require(object != NULL);

	// Appel de la methode ancetre
	KWLearningProblemView::SetObject(object);

	// Acces a l'objet edite
	learningProblem = cast(MYLearningProblem*, object);

	// Fonctionnalites avancees, disponible uniquement pour l'auteur
	if (GetLearningExpertMode())
	{
		cast(MYLearningProblemExtendedActionView*, GetFieldAt("LearningProblemStudy"))
		    ->SetObject(learningProblem);
	}
}

MYLearningProblem* MYLearningProblemView::GetMyLearningProblem()
{
	return cast(MYLearningProblem*, objValue);
}

/////////////////////////////////////////////////////////////////////////

MYLearningProblemExtendedActionView::MYLearningProblemExtendedActionView()
{
	// Libelles
	SetIdentifier("MYLearningExtendedProblemAction");
	SetLabel("Study");

	// Benchmarks
	AddAction("ClassifierBenchmark", "Evaluate classifiers...",
		  (ActionMethod)(&MYLearningProblemExtendedActionView::ClassifierBenchmark));
	AddAction("RegressorBenchmark", "Evaluate regressors...",
		  (ActionMethod)(&MYLearningProblemExtendedActionView::RegressorBenchmark));
}

MYLearningProblemExtendedActionView::~MYLearningProblemExtendedActionView() {}

void MYLearningProblemExtendedActionView::EventUpdate(Object* object)
{
	MYLearningProblem* editedObject;

	require(object != NULL);

	editedObject = cast(MYLearningProblem*, object);
}

void MYLearningProblemExtendedActionView::EventRefresh(Object* object)
{
	MYLearningProblem* editedObject;

	require(object != NULL);

	editedObject = cast(MYLearningProblem*, object);
}

void MYLearningProblemExtendedActionView::ClassifierBenchmark()
{
	GetMyLearningProblemView()->ClassifierBenchmark();
}

void MYLearningProblemExtendedActionView::RegressorBenchmark()
{
	GetMyLearningProblemView()->RegressorBenchmark();
}

MYLearningProblem* MYLearningProblemExtendedActionView::GetMyLearningProblem()
{
	require(objValue != NULL);

	return cast(MYLearningProblem*, objValue);
}

MYLearningProblemView* MYLearningProblemExtendedActionView::GetMyLearningProblemView()
{
	require(GetParent() != NULL);

	return cast(MYLearningProblemView*, GetParent());
}

////////////////////////////////////////////////////////////
// Classe MYAnalysisSpecView

MYAnalysisSpecView::MYAnalysisSpecView()
{
	MYModelingSpecView* modelingSpecView;
	const ALString sModelingSpecIdentifier = "PredictorsSpec";

	// Specialisation de la fiche des parametres de modelisation,
	// en remplacant l'ancienne version par une sous-classe
	modelingSpecView = new MYModelingSpecView;
	ReplaceCardField(sModelingSpecIdentifier, modelingSpecView);
}

MYAnalysisSpecView::~MYAnalysisSpecView() {}
