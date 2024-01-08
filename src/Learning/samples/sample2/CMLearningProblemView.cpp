// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CMLearningProblemView.h"

CMLearningProblemView::CMLearningProblemView()
{
	CMAnalysisSpecView* analysisSpecView;
	const ALString sAnalysisSpecIdentifier = "AnalysisSpec";

	// Specialisation de la fiche des parametres d'analyse,
	// en remplacant l'ancienne version par une sous-classe
	analysisSpecView = new CMAnalysisSpecView;
	ReplaceCardField(sAnalysisSpecIdentifier, analysisSpecView);

	// Parametrage de liste d'aide pour le nom de l'attribut cible
	analysisSpecView->GetFieldAt("TargetAttributeName")->SetStyle("HelpedComboBox");
	analysisSpecView->GetFieldAt("TargetAttributeName")->SetParameters("Attributes:Name");

	// Parametrage de liste d'aide pour le nom de la modalite principale
	analysisSpecView->GetFieldAt("MainTargetModality")->SetStyle("HelpedComboBox");
	analysisSpecView->GetFieldAt("MainTargetModality")->SetParameters("Modalities:Value");

	// Libelles
	SetIdentifier("CMLearningProblem");
	SetLabel("Majoritary Classifier");
}

CMLearningProblemView::~CMLearningProblemView() {}

CMLearningProblem* CMLearningProblemView::GetCMLearningProblem()
{
	return cast(CMLearningProblem*, objValue);
}

////////////////////////////////////////////////////////////
// Classe CMAnalysisSpecView

CMAnalysisSpecView::CMAnalysisSpecView()
{
	CMModelingSpecView* modelingSpecView;
	const ALString sModelingSpecIdentifier = "PredictorsSpec";

	// Specialisation de la fiche des parametres de modelisation,
	// en remplacant l'ancienne version par une sous-classe
	modelingSpecView = new CMModelingSpecView;
	ReplaceCardField(sModelingSpecIdentifier, modelingSpecView);
}

CMAnalysisSpecView::~CMAnalysisSpecView() {}
