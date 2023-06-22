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
	AddStringField("ResultFilesDirectory", "Result files directory", "");
	AddStringField("ResultFilesPrefix", "Result files prefix", "");
	AddStringField("ShortDescription", "Short description", "");
	AddStringField("PreparationFileName", "Preparation report", "");
	AddStringField("TextPreparationFileName", "Text preparation report", "");
	AddStringField("TreePreparationFileName", "Tree preparation report", "");
	AddStringField("Preparation2DFileName", "2D preparation report", "");
	AddStringField("ModelingDictionaryFileName", "Modeling dictionary file", "");
	AddStringField("ModelingFileName", "Modeling report", "");
	AddStringField("TrainEvaluationFileName", "Train evaluation report", "");
	AddStringField("TestEvaluationFileName", "Test evaluation report", "");
	AddStringField("VisualizationFileName", "Visualization report", "");
	AddStringField("JSONFileName", "JSON report", "");

	// Parametrage des styles;
	GetFieldAt("ResultFilesDirectory")->SetStyle("DirectoryChooser");

	// ## Custom constructor

#ifdef DEPRECATED_V10
	{
		// A terme, suprrimer VisualizationFileName du fichier KWAnalysisResults.dd et regenerer
		// DEPRECATED V10: champ obsolete, conserve de facon cachee en V10 pour compatibilite ascendante des
		// scenarios Pas de message, car pas de vrai impact utilisateur
		GetFieldAt("VisualizationFileName")->SetVisible(false);
	}
#endif // DEPRECATED_V10

	// Parametrage du rapport des variables de type texte
	GetFieldAt("TextPreparationFileName")->SetVisible(GetLearningTextVariableMode());

	// Parametrage du rapport des variables de type arbres
	GetFieldAt("TreePreparationFileName")->SetVisible(GetForestExpertMode());

	// Info-bulles
	GetFieldAt("ResultFilesDirectory")
	    ->SetHelpText("Name of the directory where the results files are stored."
			  "\n By default, the results files are stored in the directory of the train database.");
	GetFieldAt("ResultFilesPrefix")->SetHelpText("Prefix added before the name of each result file.");
	GetFieldAt("ShortDescription")
	    ->SetHelpText(
		"Brief description to summarize the current analysis, which will be included in the reports.");
	GetFieldAt("PreparationFileName")
	    ->SetHelpText(
		"Name of the data report file produced after the univariate data analysis on the train database.");
	GetFieldAt("TextPreparationFileName")
	    ->SetHelpText("Name of the data report file produced after the univariate data analysis of the text "
			  "features on the train database.");
	GetFieldAt("TextPreparationFileName")
	    ->SetHelpText("Name of the data report file produced after the univariate data analysis of the tree "
			  "variables on the train database.");
	GetFieldAt("Preparation2DFileName")
	    ->SetHelpText("Name of the report file produced after the bivariate data analysis on the train database.");
	GetFieldAt("ModelingDictionaryFileName")
	    ->SetHelpText("Name of the dictionary file that contains the trained predictors."
			  "\n This dictionary file can then be used to deploy the predictors on new data.");
	GetFieldAt("ModelingFileName")
	    ->SetHelpText("Name of the report file produced once the predictors have been trained.");
	GetFieldAt("TrainEvaluationFileName")
	    ->SetHelpText(
		"Name of the report file produced after the evaluation of the predictors on the train database.");
	GetFieldAt("TestEvaluationFileName")
	    ->SetHelpText(
		"Name of the report file produced after the evaluation of the predictors on the test database.");
	GetFieldAt("JSONFileName")
	    ->SetHelpText("Name of the JSON file that contains the results of all the reports."
			  "\n The JSON file is useful to inspect the modeling results from any external tool.");

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
	editedObject->SetResultFilesDirectory(GetStringValueAt("ResultFilesDirectory"));
	editedObject->SetResultFilesPrefix(GetStringValueAt("ResultFilesPrefix"));
	editedObject->SetShortDescription(GetStringValueAt("ShortDescription"));
	editedObject->SetPreparationFileName(GetStringValueAt("PreparationFileName"));
	editedObject->SetTextPreparationFileName(GetStringValueAt("TextPreparationFileName"));
	editedObject->SetTreePreparationFileName(GetStringValueAt("TreePreparationFileName"));
	editedObject->SetPreparation2DFileName(GetStringValueAt("Preparation2DFileName"));
	editedObject->SetModelingDictionaryFileName(GetStringValueAt("ModelingDictionaryFileName"));
	editedObject->SetModelingFileName(GetStringValueAt("ModelingFileName"));
	editedObject->SetTrainEvaluationFileName(GetStringValueAt("TrainEvaluationFileName"));
	editedObject->SetTestEvaluationFileName(GetStringValueAt("TestEvaluationFileName"));
	editedObject->SetVisualizationFileName(GetStringValueAt("VisualizationFileName"));
	editedObject->SetJSONFileName(GetStringValueAt("JSONFileName"));

	// ## Custom update

	// ##
}

void KWAnalysisResultsView::EventRefresh(Object* object)
{
	KWAnalysisResults* editedObject;

	require(object != NULL);

	editedObject = cast(KWAnalysisResults*, object);
	SetStringValueAt("ResultFilesDirectory", editedObject->GetResultFilesDirectory());
	SetStringValueAt("ResultFilesPrefix", editedObject->GetResultFilesPrefix());
	SetStringValueAt("ShortDescription", editedObject->GetShortDescription());
	SetStringValueAt("PreparationFileName", editedObject->GetPreparationFileName());
	SetStringValueAt("TextPreparationFileName", editedObject->GetTextPreparationFileName());
	SetStringValueAt("TreePreparationFileName", editedObject->GetTreePreparationFileName());
	SetStringValueAt("Preparation2DFileName", editedObject->GetPreparation2DFileName());
	SetStringValueAt("ModelingDictionaryFileName", editedObject->GetModelingDictionaryFileName());
	SetStringValueAt("ModelingFileName", editedObject->GetModelingFileName());
	SetStringValueAt("TrainEvaluationFileName", editedObject->GetTrainEvaluationFileName());
	SetStringValueAt("TestEvaluationFileName", editedObject->GetTestEvaluationFileName());
	SetStringValueAt("VisualizationFileName", editedObject->GetVisualizationFileName());
	SetStringValueAt("JSONFileName", editedObject->GetJSONFileName());

	// ## Custom refresh

	// ##
}

const ALString KWAnalysisResultsView::GetClassLabel() const
{
	return "Results";
}

// ## Method implementation

// ##