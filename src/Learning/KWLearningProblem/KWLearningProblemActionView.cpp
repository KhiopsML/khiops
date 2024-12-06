// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWLearningProblemActionView.h"

KWLearningProblemActionView::KWLearningProblemActionView()
{
	ALString sUserName;

	// Libelles
	SetIdentifier("KWLearningProblemAction");
	SetLabel("Tools");

	// Declaration des actions
	AddAction("CheckData", "Check database", (ActionMethod)(&KWLearningProblemActionView::CheckData));
	AddAction("SortDataTableByKey", "Sort data table by key...",
		  (ActionMethod)(&KWLearningProblemActionView::SortDataTableByKey));
	AddAction("ExtractKeysFromDataTable", "Extract keys from data table...",
		  (ActionMethod)(&KWLearningProblemActionView::ExtractKeysFromDataTable));
	AddAction("BuildConstructedDictionary", "Construct variables and export construction dictionary (expert)",
		  (ActionMethod)(&KWLearningProblemActionView::BuildConstructedDictionary));
	AddAction("ComputeStats", "Train model", (ActionMethod)(&KWLearningProblemActionView::ComputeStats));
	AddAction("TransferDatabase", "Deploy model...",
		  (ActionMethod)(&KWLearningProblemActionView::TransferDatabase));
	AddAction("EvaluatePredictors", "Evaluate model...",
		  (ActionMethod)(&KWLearningProblemActionView::EvaluatePredictors));
	AddAction("InterpretPredictor", "Interpret model...",
		  (ActionMethod)(&KWLearningProblemActionView::InterpretPredictor));

	// Ajout d'accelateurs sur les actions principales
	GetActionAt("ComputeStats")->SetAccelKey("control T");
	GetActionAt("TransferDatabase")->SetAccelKey("control D");
	GetActionAt("EvaluatePredictors")->SetAccelKey("control E");
	GetActionAt("InterpretPredictor")->SetAccelKey("control I");

	// Construction du dictionnaire uniquement en mode expert
	GetActionAt("BuildConstructedDictionary")->SetVisible(GetLearningExpertMode());

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
	GetActionAt("ExtractKeysFromDataTable")
	    ->SetHelpText("Extract keys from a sorted input data table."
			  "\n It is dedicated to the preparation of multi-table databases,"
			  "\n where a main entity has to be extracted from a detailed 0-n entity."
			  "\n For example, in case of a web log file with cookies, page, timestamp in each log,"
			  "\n extracting keys allow to build a table with unique cookies from the table of logs.");
	GetActionAt("EvaluatePredictors")
	    ->SetHelpText("Open a dialog box allowing to specify an evaluation report, an evaluation database"
			  "\n and to choose the predictor(s) to evaluate.");
	GetActionAt("EvaluatePredictors")
	    ->SetHelpText("Open a dialog box allowing to specify an evaluation report, an evaluation database"
			  "\n and to choose the predictor(s) to evaluate.");
	GetActionAt("InterpretPredictor")
	    ->SetHelpText("Open a dialog box allowing to specify and build an interpretation dictionary for a "
			  "predictor to interpret.");

	// Recopie des info-bulles de la vue principale (KWLearningProblemView)
	// (pas de reutilisation possible)
	GetActionAt("ComputeStats")
	    ->SetHelpText("Analyze the database and build prediction models."
			  "\n Khiops then performs discretizations for numerical variables, value groupings for "
			  "categorical variables."
			  "\n Variables are also constructed according to whether the variable construction options "
			  "are activated."
			  "\n Finally, the requested predictors are trained and evaluated."
			  "\n All the preparation, modeling and evaluation reports are produced.");
	GetActionAt("TransferDatabase")
	    ->SetHelpText("Open a dialog box allowing to specify an input and output database,"
			  "\n and a dictionary describing the variables to keep, discard or derive."
			  "\n This allows to recode a database or to deploy a predictor on new data.");

	// Short cuts
	SetShortCut('T');
	GetActionAt("CheckData")->SetShortCut('C');
	GetActionAt("SortDataTableByKey")->SetShortCut('S');
	GetActionAt("ExtractKeysFromDataTable")->SetShortCut('k');
	GetActionAt("BuildConstructedDictionary")->SetShortCut('B');
	GetActionAt("ComputeStats")->SetShortCut('T');
	GetActionAt("TransferDatabase")->SetShortCut('D');
	GetActionAt("EvaluatePredictors")->SetShortCut('E');
	GetActionAt("InterpretPredictor")->SetShortCut('I');
}

KWLearningProblemActionView::~KWLearningProblemActionView() {}

void KWLearningProblemActionView::EventUpdate(Object* object)
{
	require(object != NULL);
}

void KWLearningProblemActionView::EventRefresh(Object* object)
{
	require(object != NULL);
}

void KWLearningProblemActionView::CheckData()
{
	GetLearningProblemView()->CheckData();
}

void KWLearningProblemActionView::SortDataTableByKey()
{
	GetLearningProblemView()->SortDataTableByKey();
}

void KWLearningProblemActionView::ExtractKeysFromDataTable()
{
	GetLearningProblemView()->ExtractKeysFromDataTable();
}

void KWLearningProblemActionView::BuildConstructedDictionary()
{
	GetLearningProblemView()->BuildConstructedDictionary();
}

void KWLearningProblemActionView::ComputeStats()
{
	GetLearningProblemView()->ComputeStats();
}

void KWLearningProblemActionView::TransferDatabase()
{
	GetLearningProblemView()->TransferDatabase();
}

void KWLearningProblemActionView::EvaluatePredictors()
{
	GetLearningProblemView()->EvaluatePredictors();
}

void KWLearningProblemActionView::InterpretPredictor()
{
	GetLearningProblemView()->InterpretPredictor();
}

KWLearningProblem* KWLearningProblemActionView::GetLearningProblem()
{
	require(objValue != NULL);

	return cast(KWLearningProblem*, objValue);
}

KWLearningProblemView* KWLearningProblemActionView::GetLearningProblemView()
{
	require(GetParent() != NULL);

	return cast(KWLearningProblemView*, GetParent());
}
