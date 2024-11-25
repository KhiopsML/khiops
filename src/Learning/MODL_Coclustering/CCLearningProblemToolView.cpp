// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCLearningProblemToolView.h"

CCLearningProblemToolView::CCLearningProblemToolView()
{
	// Libelles
	SetIdentifier("CCLearningProblemTool");
	SetLabel("Coclustering tool");

	// Ajout d'un champ rappelant le fichier de coclustering en entree
	AddStringField("InputCoclusteringFileName", "Input coclustering report", "");
	GetFieldAt("InputCoclusteringFileName")->SetEditable(false);

	// Ajout d'actions sous formes de boutons
	AddAction("SelectInputCoclustering", "Select input coclustering...",
		  (ActionMethod)(&CCLearningProblemToolView::SelectInputCoclustering));
	GetActionAt("SelectInputCoclustering")->SetStyle("Button");

	// Info-bulles
	GetActionAt("SelectInputCoclustering")->SetHelpText("Select the input coclustering report.");
	GetFieldAt("InputCoclusteringFileName")
	    ->SetHelpText("Name of the input coclustering report."
			  "\n Use button '" +
			  GetActionAt("SelectInputCoclustering")->GetLabel() + "' to change file.");

	// Short cuts
	GetActionAt("SelectInputCoclustering")->SetShortCut('I');
}

CCLearningProblemToolView::~CCLearningProblemToolView() {}

void CCLearningProblemToolView::EventUpdate(Object* object) {}

void CCLearningProblemToolView::EventRefresh(Object* object)
{
	CCLearningProblem* editedObject;

	require(object != NULL);

	editedObject = cast(CCLearningProblem*, object);
	SetStringValueAt("InputCoclusteringFileName",
			 editedObject->GetAnalysisResults()->GetInputCoclusteringFileName());
}

//////////////////////////////////////////////////////////////////////////

void CCLearningProblemToolView::SelectInputCoclustering()
{
	UIFileChooserCard openCard;
	ALString sCoclusteringReportFileName;

	// Ouverture du FileChooser
	sCoclusteringReportFileName = openCard.ChooseFile(
	    "Select input coclustering", "Open", "FileChooser", "Coclustering\nkhcj", "InputCoclusteringFileName",
	    "Input coclustering file", GetLearningProblem()->GetAnalysisResults()->GetInputCoclusteringFileName());

	// Parametrage des specifications de coclustering a partir du rapport de coclustering
	if (sCoclusteringReportFileName != "")
	{
		sCoclusteringReportFileName = openCard.GetStringValueAt("InputCoclusteringFileName");
		GetLearningProblem()->GetAnalysisResults()->SetInputCoclusteringFileName(sCoclusteringReportFileName);
		GetLearningProblem()->GetPostProcessingSpec()->UpdateCoclusteringSpec(sCoclusteringReportFileName);
		GetLearningProblem()->GetDeploymentSpec()->FillInputClassAndAttributeNames(
		    GetLearningProblem()->GetPostProcessingSpec());
	}
}

CCLearningProblem* CCLearningProblemToolView::GetLearningProblem()
{
	return cast(CCLearningProblem*, objValue);
}
