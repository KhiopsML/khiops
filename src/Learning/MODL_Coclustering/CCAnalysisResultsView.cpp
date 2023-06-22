// Copyright (c) 2023 Orange. All rights reserved.
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
	AddStringField("ResultFilesDirectory", "Result files directory", "");
	AddStringField("ResultFilesPrefix", "Result files prefix", "");
	AddStringField("ShortDescription", "Short description", "");
	AddStringField("CoclusteringFileName", "Coclustering report", "");
	AddStringField("InputCoclusteringFileName", "Input coclustering report", "");
	AddStringField("ClusterFileName", "Cluster table file", "");
	AddStringField("PostProcessedCoclusteringFileName", "Simplified coclustering report", "");
	AddStringField("CoclusteringDictionaryFileName", "Coclustering dictionary file", "");
	AddBooleanField("ExportJSON", "Export JSON", false);

	// Parametrage des styles;
	GetFieldAt("ResultFilesDirectory")->SetStyle("DirectoryChooser");

	// ## Custom constructor

	// Info-bulles
	GetFieldAt("ShortDescription")
	    ->SetHelpText(
		"Brief description to summarize the current analysis, which will be included in the reports.");
	GetFieldAt("ResultFilesDirectory")
	    ->SetHelpText("Name of the directory where the results files are stored."
			  "\n By default, the results files are stored in the directory of the train database.");
	GetFieldAt("ResultFilesPrefix")->SetHelpText("Prefix added before the name of each result file.");
	GetFieldAt("CoclusteringFileName")
	    ->SetHelpText("Name of the coclustering report that contains the full definition of the coclustering model."
			  "\n The coclustering report is the input of the Khiops Covisualization tool.");
	GetFieldAt("InputCoclusteringFileName")->SetHelpText("Name of the coclustering report to post-process.");
	GetFieldAt("ClusterFileName")->SetHelpText("Name of the file that contains the extracted clusters.");
	GetFieldAt("PostProcessedCoclusteringFileName")
	    ->SetHelpText("Name of the simplified coclustering report,"
			  "\n that is the most detailed version of the input coclustering report"
			  "\n that meets all the simplification constraints.");
	GetFieldAt("CoclusteringDictionaryFileName")
	    ->SetHelpText("Name of the deployment dictionary that contains the coclustering deployment model.");
	GetFieldAt("ExportJSON")
	    ->SetHelpText("Export the coclustering report under a JSON format."
			  "\n The exported JSON file has the same name as the coclustering report file, with a ." +
			  CCCoclusteringReport::GetJSONReportSuffix() +
			  "extension."
			  "\n The JSON file is useful to inspect the coclustering results from any external tool.");

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
	editedObject->SetResultFilesDirectory(GetStringValueAt("ResultFilesDirectory"));
	editedObject->SetResultFilesPrefix(GetStringValueAt("ResultFilesPrefix"));
	editedObject->SetShortDescription(GetStringValueAt("ShortDescription"));
	editedObject->SetCoclusteringFileName(GetStringValueAt("CoclusteringFileName"));
	editedObject->SetInputCoclusteringFileName(GetStringValueAt("InputCoclusteringFileName"));
	editedObject->SetClusterFileName(GetStringValueAt("ClusterFileName"));
	editedObject->SetPostProcessedCoclusteringFileName(GetStringValueAt("PostProcessedCoclusteringFileName"));
	editedObject->SetCoclusteringDictionaryFileName(GetStringValueAt("CoclusteringDictionaryFileName"));
	editedObject->SetExportJSON(GetBooleanValueAt("ExportJSON"));

	// ## Custom update

	// ##
}

void CCAnalysisResultsView::EventRefresh(Object* object)
{
	CCAnalysisResults* editedObject;

	require(object != NULL);

	editedObject = cast(CCAnalysisResults*, object);
	SetStringValueAt("ResultFilesDirectory", editedObject->GetResultFilesDirectory());
	SetStringValueAt("ResultFilesPrefix", editedObject->GetResultFilesPrefix());
	SetStringValueAt("ShortDescription", editedObject->GetShortDescription());
	SetStringValueAt("CoclusteringFileName", editedObject->GetCoclusteringFileName());
	SetStringValueAt("InputCoclusteringFileName", editedObject->GetInputCoclusteringFileName());
	SetStringValueAt("ClusterFileName", editedObject->GetClusterFileName());
	SetStringValueAt("PostProcessedCoclusteringFileName", editedObject->GetPostProcessedCoclusteringFileName());
	SetStringValueAt("CoclusteringDictionaryFileName", editedObject->GetCoclusteringDictionaryFileName());
	SetBooleanValueAt("ExportJSON", editedObject->GetExportJSON());

	// ## Custom refresh

	// ##
}

const ALString CCAnalysisResultsView::GetClassLabel() const
{
	return "Results";
}

// ## Method implementation

void CCAnalysisResultsView::SetResultFieldsVisible(boolean bValue)
{
	GetFieldAt("ShortDescription")->SetVisible(bValue);
	GetFieldAt("CoclusteringFileName")->SetVisible(bValue);
	GetFieldAt("InputCoclusteringFileName")->SetVisible(bValue);
	GetFieldAt("ClusterFileName")->SetVisible(bValue);
	GetFieldAt("PostProcessedCoclusteringFileName")->SetVisible(bValue);
	GetFieldAt("CoclusteringDictionaryFileName")->SetVisible(bValue);
	GetFieldAt("ExportJSON")->SetVisible(bValue);
}

// ##