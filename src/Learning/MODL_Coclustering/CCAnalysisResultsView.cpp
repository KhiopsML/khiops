// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "CCAnalysisResultsView.h"

CCAnalysisResultsView::CCAnalysisResultsView()
{
	SetIdentifier("CCAnalysisResults");
	SetLabel("Results");
	AddStringField("CoclusteringFileName", "Coclustering report", "");
	AddStringField("ShortDescription", "Short description", "");

	// Parametrage des styles;
	GetFieldAt("CoclusteringFileName")->SetStyle("FileChooser");

	// ## Custom constructor

	// Action de visualisation des resultats
	AddAction("VisualizeReport", "Visualize report", (ActionMethod)(&CCAnalysisResultsView::VisualizeReport));
	GetActionAt("VisualizeReport")->SetStyle("Button");

	// Info-bulles
	GetFieldAt("CoclusteringFileName")
	    ->SetHelpText(
		"Name of the coclustering report that contains the full definition of the coclustering model."
		"\n By default, the report is stored in the database directory, unless an absolute path is specified."
		"\n The coclustering report is the input of the Khiops Covisualization tool.");
	GetFieldAt("ShortDescription")
	    ->SetHelpText(
		"Brief description to summarize the current analysis, which will be included in the reports.");
	GetActionAt("VisualizeReport")
	    ->SetHelpText("Visualize coclustering report if available, using Khiops covisualization tool.");

	// ##
}

CCAnalysisResultsView::~CCAnalysisResultsView()
{
	// ## Custom destructor

	// ##
}

CCAnalysisResults* CCAnalysisResultsView::GetCCAnalysisResults()
{
	require(objValue != NULL);
	return cast(CCAnalysisResults*, objValue);
}

void CCAnalysisResultsView::EventUpdate(Object* object)
{
	CCAnalysisResults* editedObject;

	require(object != NULL);

	editedObject = cast(CCAnalysisResults*, object);
	editedObject->SetCoclusteringFileName(GetStringValueAt("CoclusteringFileName"));
	editedObject->SetShortDescription(GetStringValueAt("ShortDescription"));

	// ## Custom update

	// ##
}

void CCAnalysisResultsView::EventRefresh(Object* object)
{
	CCAnalysisResults* editedObject;

	require(object != NULL);

	editedObject = cast(CCAnalysisResults*, object);
	SetStringValueAt("CoclusteringFileName", editedObject->GetCoclusteringFileName());
	SetStringValueAt("ShortDescription", editedObject->GetShortDescription());

	// ## Custom refresh

	// ##
}

const ALString CCAnalysisResultsView::GetClassLabel() const
{
	return "Results";
}

// ## Method implementation

void CCAnalysisResultsView::VisualizeReport()
{
	CCAnalysisResults* analysisResults;
	ALString sAnalysisReportFileName;
	boolean bOk;
	KWResultFilePathBuilder resultFilePathBuilder;
	char sResult[SYSTEM_MESSAGE_LENGTH + 1];

	// Acces aux resultats d'analyse
	analysisResults = GetCCAnalysisResults();
	assert(analysisResults->GetTrainDatabase() != NULL);

	// Initialisation du createur de chemin de fichier
	if (analysisResults->GetTrainDatabase() != NULL)
		resultFilePathBuilder.SetInputFilePathName(analysisResults->GetTrainDatabase()->GetDatabaseName());
	resultFilePathBuilder.SetOutputFilePathName(analysisResults->GetCoclusteringFileName());
	resultFilePathBuilder.SetFileSuffix("khcj");

	// Recherche du nom du rapport d'analyse
	sAnalysisReportFileName = resultFilePathBuilder.BuildResultFilePathName();

	// Warning si pas de fichier de rapprot
	if (not FileService::FileExists(sAnalysisReportFileName))
		AddWarning("No report found for file name " + sAnalysisReportFileName);
	// Ouverture sinon
	else
	{
		assert(FileService::GetFileSuffix(sAnalysisReportFileName) == "khcj");
		bOk = OpenApplication("khiops-covisualization", "Khiops covisualization", sAnalysisReportFileName,
				      sResult);
		if (not bOk)
			AddWarning(sResult);
	}
}

const ALString CCAnalysisResultsView::GetObjectLabel() const
{
	// Redefini a vide, car le ClassLabel est suffisant dans les messages
	return "";
}

// ##
