// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWAnalysisResultsView.h"

KWAnalysisResultsView::KWAnalysisResultsView()
{
	SetIdentifier("KWAnalysisResults");
	SetLabel("Results");
	AddStringField("ReportFileName", "Analysis report", "");
	AddStringField("ShortDescription", "Short description", "");
	AddBooleanField("ExportAsXls", "Export as xls", false);

	// Parametrage des styles;
	GetFieldAt("ReportFileName")->SetStyle("FileChooser");

	// ## Custom constructor

	// Action de visualisation des resultats
	AddAction("VisualizeResults", "Visualize results", (ActionMethod)(&KWAnalysisResultsView::VisualizeResults));
	GetActionAt("VisualizeResults")->SetStyle("Button");

	// Info-bulles
	GetFieldAt("ReportFileName")
	    ->SetHelpText("Name of the analysis report file in JSON format."
			  "\n An additionnal dictionary file with extension.model.kdic is built, which contains the "
			  "trained models."
			  "\n By default, the result files are stored in the train database directory, unless an "
			  "absolute path is specified."
			  "\n The JSON file is useful to inspect the modeling results from any external tool.");
	GetFieldAt("ShortDescription")
	    ->SetHelpText(
		"Brief description to summarize the current analysis, which will be included in the reports.");
	GetFieldAt("ExportAsXls")
	    ->SetHelpText("Option to export each report to a tabular file that can be opened using Excel, with the "
			  "following extensions:"
			  "\n - PreparationReport.xls: data preparation report produced after the univariate data "
			  "analysis on the train database"
			  "\n - TextPreparationReport.xls: data preparation report for text variables"
			  "\n - TreePreparationReport.xls: data preparation report for tree variables"
			  "\n - Preparation2DReport.xls: data preparation report for bivariate analysis"
			  "\n - ModelingReport.xls: modeling report produced once the predictors have been trained"
			  "\n - TrainEvaluationReport.xls: evaluation report produced after the evaluation of the "
			  "predictors on the train database."
			  "\n - TestEvaluationReport.xls: evaluation report produced after the evaluation of the "
			  "predictors on the train database.");
	GetActionAt("VisualizeResults")
	    ->SetHelpText("Visualize result report if available, using Khiops visualization tool.");

	// Short cuts
	GetActionAt("VisualizeResults")->SetShortCut('V');

	// ##
}

KWAnalysisResultsView::~KWAnalysisResultsView()
{
	// ## Custom destructor

	// ##
}

KWAnalysisResults* KWAnalysisResultsView::GetKWAnalysisResults()
{
	require(objValue != NULL);
	return cast(KWAnalysisResults*, objValue);
}

void KWAnalysisResultsView::EventUpdate(Object* object)
{
	KWAnalysisResults* editedObject;

	require(object != NULL);

	editedObject = cast(KWAnalysisResults*, object);
	editedObject->SetReportFileName(GetStringValueAt("ReportFileName"));
	editedObject->SetShortDescription(GetStringValueAt("ShortDescription"));
	editedObject->SetExportAsXls(GetBooleanValueAt("ExportAsXls"));

	// ## Custom update

	// ##
}

void KWAnalysisResultsView::EventRefresh(Object* object)
{
	KWAnalysisResults* editedObject;

	require(object != NULL);

	editedObject = cast(KWAnalysisResults*, object);
	SetStringValueAt("ReportFileName", editedObject->GetReportFileName());
	SetStringValueAt("ShortDescription", editedObject->GetShortDescription());
	SetBooleanValueAt("ExportAsXls", editedObject->GetExportAsXls());

	// ## Custom refresh

	// ##
}

const ALString KWAnalysisResultsView::GetClassLabel() const
{
	return "Results";
}

// ## Method implementation

void KWAnalysisResultsView::VisualizeResults()
{
	KWAnalysisResults* analysisResults;
	ALString sAnalysisReportFileName;
	boolean bOk;
	char sResult[SYSTEM_MESSAGE_LENGTH + 1];

	// Acces aux resultats d'analyse
	analysisResults = GetKWAnalysisResults();
	assert(analysisResults->GetTrainDatabase() != NULL);

	// Recherche du nom du rapport d'analyse
	sAnalysisReportFileName = analysisResults->BuildOutputFilePathName(analysisResults->GetReportFileName());

	// Warning si pas de fichier de rapprot
	if (not FileService::FileExists(sAnalysisReportFileName))
		AddWarning("No report found for file name " + sAnalysisReportFileName);
	// Ouverture sinon
	else
	{
		assert(FileService::GetFileSuffix(sAnalysisReportFileName) == "khj");
		bOk = OpenApplication("khiops-visualization", "Khiops visualization", sAnalysisReportFileName, sResult);
		if (not bOk)
			AddWarning(sResult);
	}
}

const ALString KWAnalysisResultsView::GetObjectLabel() const
{
	// Redefini a vide, car le ClassLabel est suffisant dans les messages
	return "";
}

// ##