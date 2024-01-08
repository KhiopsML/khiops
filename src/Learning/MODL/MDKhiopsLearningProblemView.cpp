// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MDKhiopsLearningProblemView.h"

MDKhiopsLearningProblemView::MDKhiopsLearningProblemView()
{
	KWAnalysisSpecView* currentAnalysisSpecView;
	MDKhiopsAnalysisSpecView* analysisSpecView;
	const ALString sAnalysisSpecIdentifier = "AnalysisSpec";
	const ALString sAnalysisResultsIdentifier = "AnalysisResults";

	// Acces a la fiche des parametre d'analyse
	currentAnalysisSpecView = cast(KWAnalysisSpecView*, GetFieldAt(sAnalysisSpecIdentifier));

	// Creation d'une specialisation de la fiche des parametres d'analyse,
	analysisSpecView = new MDKhiopsAnalysisSpecView;

	// Parametrage de liste d'aide pour le nom de l'attribut cible
	analysisSpecView->GetFieldAt("TargetAttributeName")
	    ->SetStyle(currentAnalysisSpecView->GetFieldAt("TargetAttributeName")->GetStyle());
	analysisSpecView->GetFieldAt("TargetAttributeName")
	    ->SetParameters(currentAnalysisSpecView->GetFieldAt("TargetAttributeName")->GetParameters());

	// Parametrage de liste d'aide pour les modalites de l'attribut cible
	analysisSpecView->GetFieldAt("MainTargetModality")
	    ->SetStyle(currentAnalysisSpecView->GetFieldAt("MainTargetModality")->GetStyle());
	analysisSpecView->GetFieldAt("MainTargetModality")
	    ->SetParameters(currentAnalysisSpecView->GetFieldAt("MainTargetModality")->GetParameters());

	// Remplacement de la fiche courante par sa version specialisee
	ReplaceCardField(sAnalysisSpecIdentifier, analysisSpecView);

	// Fonctionnalites avancees, disponible uniquement en mode expert
	if (GetLearningExpertMode())
	{
		AddCardField("LearningProblemStudy", "Benchmark", new MDKhiopsLearningProblemExtendedActionView);
		MoveFieldBefore("LearningProblemStudy", "Help");
	}
}

MDKhiopsLearningProblemView::~MDKhiopsLearningProblemView() {}

void MDKhiopsLearningProblemView::ClassifierBenchmark()
{
	KWLearningBenchmark* classifierBenchmark;
	KWLearningBenchmarkView view;

	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

	// Acces au parametrage du benchmark
	classifierBenchmark = GetKhiopsLearningProblem()->GetClassifierBenchmark();
	assert(classifierBenchmark->GetTargetAttributeType() == KWType::Symbol);

	// Ouverture de la fenetre
	view.SetObject(classifierBenchmark);
	view.Open();
}

void MDKhiopsLearningProblemView::RegressorBenchmark()
{
	KWLearningBenchmark* regressorBenchmark;
	KWLearningBenchmarkView view;

	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

	// Acces au parametrage du benchmark
	regressorBenchmark = GetKhiopsLearningProblem()->GetRegressorBenchmark();
	assert(regressorBenchmark->GetTargetAttributeType() == KWType::Continuous);

	// Ouverture de la fenetre
	view.SetObject(regressorBenchmark);
	view.Open();
}

void MDKhiopsLearningProblemView::ClassifierBenchmarkUnivariate()
{
	KWLearningBenchmarkUnivariate* classifierBenchmarkUnivariate;
	KWLearningBenchmarkView view;

	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

	// Acces au parametrage du benchmark
	classifierBenchmarkUnivariate = GetKhiopsLearningProblem()->GetClassifierBenchmarkUnivariate();

	// Ouverture de la fenetre
	view.SetObject(classifierBenchmarkUnivariate);
	view.Open();
}

void MDKhiopsLearningProblemView::ClassifierBenchmarkBivariate()
{
	KWLearningBenchmarkBivariate* classifierBenchmarkBivariate;
	KWLearningBenchmarkView view;

	// Execution controlee par licence
	if (LMLicenseManager::IsEnabled())
		if (not LMLicenseManager::RequestLicenseKey())
			return;

	// Acces au parametrage du benchmark
	classifierBenchmarkBivariate = GetKhiopsLearningProblem()->GetClassifierBenchmarkBivariate();

	// Ouverture de la fenetre
	view.SetObject(classifierBenchmarkBivariate);
	view.Open();
}

void MDKhiopsLearningProblemView::SetObject(Object* object)
{
	MDKhiopsLearningProblem* learningProblem;

	require(object != NULL);

	// Appel de la methode ancetre
	KWLearningProblemView::SetObject(object);

	// Acces a l'objet edite
	learningProblem = cast(MDKhiopsLearningProblem*, object);

	// Fonctionnalites avancees, disponible uniquement en mode expert
	if (GetLearningExpertMode())
	{
		cast(MDKhiopsLearningProblemExtendedActionView*, GetFieldAt("LearningProblemStudy"))
		    ->SetObject(learningProblem);
	}
}

MDKhiopsLearningProblem* MDKhiopsLearningProblemView::GetKhiopsLearningProblem()
{
	return cast(MDKhiopsLearningProblem*, objValue);
}

/////////////////////////////////////////////////////////////////////////

MDKhiopsLearningProblemExtendedActionView::MDKhiopsLearningProblemExtendedActionView()
{
	// Libelles
	SetIdentifier("MDKhiopsLearningExtendedProblemAction");
	SetLabel("Study");

	// Benchmarks
	AddAction("ClassifierBenchmark", "Evaluate classifiers...",
		  (ActionMethod)(&MDKhiopsLearningProblemExtendedActionView::ClassifierBenchmark));
	AddAction("RegressorBenchmark", "Evaluate regressors...",
		  (ActionMethod)(&MDKhiopsLearningProblemExtendedActionView::RegressorBenchmark));
	AddAction("ClassifierBenchmarkUnivariate", "Evaluate univariate classifiers...",
		  (ActionMethod)(&MDKhiopsLearningProblemExtendedActionView::ClassifierBenchmarkUnivariate));
	AddAction("ClassifierBenchmarkBivariate", "Evaluate bivariate classifiers...",
		  (ActionMethod)(&MDKhiopsLearningProblemExtendedActionView::ClassifierBenchmarkBivariate));
}

MDKhiopsLearningProblemExtendedActionView::~MDKhiopsLearningProblemExtendedActionView() {}

void MDKhiopsLearningProblemExtendedActionView::EventUpdate(Object* object)
{
	require(object != NULL);
}

void MDKhiopsLearningProblemExtendedActionView::EventRefresh(Object* object)
{
	require(object != NULL);
}

void MDKhiopsLearningProblemExtendedActionView::ClassifierBenchmark()
{
	GetKhiopsLearningProblemView()->ClassifierBenchmark();
}

void MDKhiopsLearningProblemExtendedActionView::RegressorBenchmark()
{
	GetKhiopsLearningProblemView()->RegressorBenchmark();
}

void MDKhiopsLearningProblemExtendedActionView::ClassifierBenchmarkUnivariate()
{
	GetKhiopsLearningProblemView()->ClassifierBenchmarkUnivariate();
}

void MDKhiopsLearningProblemExtendedActionView::ClassifierBenchmarkBivariate()
{
	GetKhiopsLearningProblemView()->ClassifierBenchmarkBivariate();
}

MDKhiopsLearningProblem* MDKhiopsLearningProblemExtendedActionView::GetKhiopsLearningProblem()
{
	require(objValue != NULL);

	return cast(MDKhiopsLearningProblem*, objValue);
}

MDKhiopsLearningProblemView* MDKhiopsLearningProblemExtendedActionView::GetKhiopsLearningProblemView()
{
	require(GetParent() != NULL);

	return cast(MDKhiopsLearningProblemView*, GetParent());
}

////////////////////////////////////////////////////////////
// Classe MDKhiopsAnalysisSpecView

MDKhiopsAnalysisSpecView::MDKhiopsAnalysisSpecView()
{
	MDKhiopsModelingSpecView* modelingSpecView;
	const ALString sModelingSpecIdentifier = "PredictorsSpec";

	// Specialisation de la fiche des parametres de modelisation,
	// en remplacant l'ancienne version par une sous-classe
	modelingSpecView = new MDKhiopsModelingSpecView;
	ReplaceCardField(sModelingSpecIdentifier, modelingSpecView);

#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10: idem pour l'ancien onglet obsolete
		ReplaceCardField("ModelingSpec", new MDKhiopsModelingSpecView);
		GetFieldAt("ModelingSpec")->SetVisible(false);
		delete DEPRECATEDModelingSpec;
		delete DEPRECATEDModelingSpecReference;
		DEPRECATEDModelingSpec = new MDKhiopsModelingSpec;
		DEPRECATEDModelingSpecReference = new MDKhiopsModelingSpec;
	}
#endif // DEPRECATED_V10
}

MDKhiopsAnalysisSpecView::~MDKhiopsAnalysisSpecView() {}
