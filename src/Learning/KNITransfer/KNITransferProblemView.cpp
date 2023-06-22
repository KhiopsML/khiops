// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KNITransferProblemView.h"

KNITransferProblemView::KNITransferProblemView()
{
	const ALString sTrainDatabaseIdentifier = "TrainDatabase";
	const ALString sTestDatabaseIdentifier = "TestDatabase";
	const ALString sAnalysisSpecIdentifier = "AnalysisSpec";
	const ALString sAnalysisResultsIdentifier = "AnalysisResults";
	const ALString sLearningToolsIdentifier = "LearningTools";
	const ALString sComputeStatsIdentifier = "ComputeStats";
	const ALString sTransferDatabaseIdentifier = "TransferDatabase";
	UIAction* action;

	// On inhibe toules onglets inutiles pour le transfer
	GetFieldAt(sTrainDatabaseIdentifier)->SetVisible(false);
	GetFieldAt(sTestDatabaseIdentifier)->SetVisible(false);
	GetFieldAt(sAnalysisSpecIdentifier)->SetVisible(false);
	GetFieldAt(sAnalysisResultsIdentifier)->SetVisible(false);
	GetFieldAt(sLearningToolsIdentifier)->SetVisible(false);
	GetActionAt(sComputeStatsIdentifier)->SetVisible(false);

	// Redefinition de l'action de transfer
	action = GetActionAt(sTransferDatabaseIdentifier);
	action->SetActionMethod((ActionMethod)(&KNITransferProblemView::KNITransferDatabase));
}

KNITransferProblemView::~KNITransferProblemView() {}

void KNITransferProblemView::KNITransferDatabase()
{
	KNIDatabaseTransferView databaseTransferView;

	// Parametrage du fichier de dictionnaire
	databaseTransferView.SetClassFileName(GetLearningProblem()->GetClassFileName());

	// Initialisation a partir de la base d'apprentissage (pour recuperer le nom du dictionnaire par defaut)
	databaseTransferView.InitializeSourceDatabase(GetLearningProblem()->GetTrainDatabase());

	// Ouverture
	databaseTransferView.Open();
}

void KNITransferProblemView::SetObject(Object* object)
{
	KNITransferProblem* learningProblem;

	require(object != NULL);

	// Appel de la methode ancetre
	KWLearningProblemView::SetObject(object);

	// Acces a l'objet edite
	learningProblem = cast(KNITransferProblem*, object);
}

KNITransferProblem* KNITransferProblemView::GetTransferProblem()
{
	return cast(KNITransferProblem*, objValue);
}