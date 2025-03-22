// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCLearningProblemActionView.h"

CCLearningProblemActionView::CCLearningProblemActionView()
{
	// Ajout d'actions
	AddAction("CheckData", "Check database", (ActionMethod)(&CCLearningProblemActionView::CheckData));
	AddAction("SortDataTableByKey", "Sort data table by key...",
		  (ActionMethod)(&CCLearningProblemActionView::SortDataTableByKey));
	AddAction("BuildCoclustering", "Train coclustering",
		  (ActionMethod)(&CCLearningProblemActionView::BuildCoclustering));
	AddAction("PostProcessCoclustering", "Simplify coclustering...",
		  (ActionMethod)(&CCLearningProblemActionView::PostProcessCoclustering));
	AddAction("ExtractClusters", "Extract clusters...",
		  (ActionMethod)(&CCLearningProblemActionView::ExtractClusters));
	AddAction("PrepareDeployment", "Prepare deployment...",
		  (ActionMethod)(&CCLearningProblemActionView::PrepareDeployment));
	AddAction("TransferDatabase", "Deploy model...",
		  (ActionMethod)(&CCLearningProblemActionView::TransferDatabase));

	// Ajout d'accelateurs sur les actions principales
	GetActionAt("BuildCoclustering")->SetAccelKey("control T");
	GetActionAt("PostProcessCoclustering")->SetAccelKey("control I");
	GetActionAt("ExtractClusters")->SetAccelKey("control E");
	GetActionAt("PrepareDeployment")->SetAccelKey("control P");
	GetActionAt("TransferDatabase")->SetAccelKey("control D");

	// Info-bulles
	GetActionAt("CheckData")
	    ->SetHelpText("Check the integrity of the train database."
			  "\n This action reads each line of the train database to perform the integrity checks."
			  "\n During formatting checks, the number of fields in the train database is compared"
			  "\n to the number of native variables in the dictionary."
			  "\n Data conversion checks are performed for the fields corresponding to "
			  "\n numerical, date, time, timestamp and timestampTZ variables."
			  "\n In case of multi-tables database, data tables must be sorted by key."
			  "\n Error messages are displayed in the message log window.");
	GetActionAt("SortDataTableByKey")
	    ->SetHelpText("Sort a data table according to sort variables."
			  "\n It is dedicated to the preparation of multi-table databases,"
			  "\n which requires all data table files to be sorted by key, for efficiency reasons.");
	GetActionAt("BuildCoclustering")
	    ->SetHelpText("Train a coclustering model given the coclustering parameters."
			  "\n This action is anytime: coclustering models are computed and continuously improved,"
			  "\n with new solutions saved as soon as improvements are reached."
			  "\n The intermediate solutions can be used without waiting for the final solution,"
			  "\n and the process can be stopped at any time to keep the last best solution.");
	GetActionAt("PostProcessCoclustering")
	    ->SetHelpText("Build a simplified coclustering model."
			  "\n Opens a new window named 'Coclustering simplification'.");
	GetActionAt("ExtractClusters")
	    ->SetHelpText("Extract clusters in a text file for a given coclustering variable."
			  "\n Opens a new window named 'Cluster extraction'.");
	GetActionAt("PrepareDeployment")
	    ->SetHelpText(
		"Enables the deployment of a coclustering model by the means of a Khiops deployment dictionary."
		"\n Opens a new window named 'Coclustering deployment preparation'.");
	GetActionAt("TransferDatabase")
	    ->SetHelpText("Open a dialog box allowing to specify an input and output database,"
			  "\n and a dictionary describing the variables to keep, discard or derive."
			  "\n This allows to recode a database or to deploy a predictor on new data.");

	// Short cuts
	SetShortCut('T');
	GetActionAt("CheckData")->SetShortCut('C');
	GetActionAt("SortDataTableByKey")->SetShortCut('S');
	GetActionAt("BuildCoclustering")->SetShortCut('T');
	GetActionAt("PostProcessCoclustering")->SetShortCut('i');
	GetActionAt("ExtractClusters")->SetShortCut('E');
	GetActionAt("PrepareDeployment")->SetShortCut('P');
	GetActionAt("TransferDatabase")->SetShortCut('D');
}

CCLearningProblemActionView::~CCLearningProblemActionView() {}

void CCLearningProblemActionView::EventUpdate(Object* object)
{
	require(object != NULL);
}

void CCLearningProblemActionView::EventRefresh(Object* object)
{
	require(object != NULL);
}

void CCLearningProblemActionView::CheckData()
{
	GetLearningProblemView()->CheckData();
}

void CCLearningProblemActionView::SortDataTableByKey()
{
	GetLearningProblemView()->SortDataTableByKey();
}

void CCLearningProblemActionView::BuildCoclustering()
{
	GetLearningProblemView()->BuildCoclustering();
}

void CCLearningProblemActionView::PostProcessCoclustering()
{
	CCLearningProblemPostProcessingView postProcessingView;

	// Initialisation des caracteristiques du coclustering a traiter
	InitializeInputCoclusteringSpecs();

	// Lancement de la fenetre de post-processing
	postProcessingView.SetObject(GetLearningProblem());
	postProcessingView.Open();
}

void CCLearningProblemActionView::ExtractClusters()
{
	CCLearningProblemClusterExtractionView clusterExtractioniew;

	// Initialisation des caracteristiques du coclustering a traiter
	InitializeInputCoclusteringSpecs();

	// Lancement de la fenetre de post-processing
	clusterExtractioniew.SetObject(GetLearningProblem());
	clusterExtractioniew.Open();
}

void CCLearningProblemActionView::PrepareDeployment()
{
	CCLearningProblemDeploymentPreparationView deploymentPreparationView;

	// Initialisation des caracteristiques du coclustering a traiter
	InitializeInputCoclusteringSpecs();

	// Lancement de la fenetre de post-processing
	deploymentPreparationView.SetObject(GetLearningProblem());
	deploymentPreparationView.Open();
}

void CCLearningProblemActionView::TransferDatabase()
{
	GetLearningProblemView()->TransferDatabase();
}

CCLearningProblem* CCLearningProblemActionView::GetLearningProblem()
{
	require(objValue != NULL);

	return cast(CCLearningProblem*, objValue);
}

CCLearningProblemView* CCLearningProblemActionView::GetLearningProblemView()
{
	require(GetParent() != NULL);

	return cast(CCLearningProblemView*, GetParent());
}

void CCLearningProblemActionView::InitializeInputCoclusteringSpecs()
{
	ALString sCoclusteringReportFileName;

	// Parametrage des specifications de coclustering a partir du rapport de coclustering
	sCoclusteringReportFileName = GetLearningProblem()->GetAnalysisResults()->GetInputCoclusteringFileName();
	if (sCoclusteringReportFileName != "")
	{
		GetLearningProblem()->GetPostProcessingSpec()->UpdateCoclusteringSpec(sCoclusteringReportFileName);
		GetLearningProblem()->GetDeploymentSpec()->FillInputClassAndAttributeNames(
		    GetLearningProblem()->GetPostProcessingSpec());
	}
}
