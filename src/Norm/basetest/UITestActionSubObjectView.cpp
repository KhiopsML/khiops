// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UITestActionSubObjectView.h"

UITestActionSubObjectView::UITestActionSubObjectView()
{
	SetIdentifier("UITestActionSubObject");
	SetLabel("Test sub-object with action");
	AddStringField("FilePath", "File", "");
	AddBooleanField("DirectFileChooser", "Direct file chooser", false);

	// ## Custom constructor

	// Parametrage du champ
	GetFieldAt("FilePath")->SetEditable(false);

	// Ajout des actions
	AddAction("ChooseFile", "Choose file...", (ActionMethod)(&UITestActionSubObjectView::ChooseFile));
	AddAction("AskQuestion", "Question...", (ActionMethod)(&UITestActionSubObjectView::AskQuestion));
	AddAction("Quit", "Quit", (ActionMethod)(&UITestActionSubObjectView::OK));

	// Parametrage de l'action Quit pour quiter la fenetre
	GetActionAt("Quit")->SetParameters("Exit");

	// ##
}

UITestActionSubObjectView::~UITestActionSubObjectView()
{
	// ## Custom destructor

	// ##
}

UITestActionSubObject* UITestActionSubObjectView::GetUITestActionSubObject()
{
	require(objValue != NULL);
	return cast(UITestActionSubObject*, objValue);
}

void UITestActionSubObjectView::EventUpdate(Object* object)
{
	UITestActionSubObject* editedObject;

	require(object != NULL);

	editedObject = cast(UITestActionSubObject*, object);
	editedObject->SetFilePath(GetStringValueAt("FilePath"));
	editedObject->SetDirectFileChooser(GetBooleanValueAt("DirectFileChooser"));

	// ## Custom update

	// ##
}

void UITestActionSubObjectView::EventRefresh(Object* object)
{
	UITestActionSubObject* editedObject;

	require(object != NULL);

	editedObject = cast(UITestActionSubObject*, object);
	SetStringValueAt("FilePath", editedObject->GetFilePath());
	SetBooleanValueAt("DirectFileChooser", editedObject->GetDirectFileChooser());

	// ## Custom refresh

	// ##
}

const ALString UITestActionSubObjectView::GetClassLabel() const
{
	return "Test sub-object with action";
}

// ## Method implementation

void UITestActionSubObjectView::ChooseFile()
{
	UITestActionSubObject* actionSubObject;
	ALString sFilePath;
	UIFileChooserCard fileChooserCard;

	// Acces a l'objet edite
	actionSubObject = cast(UITestActionSubObject*, GetRawObject());

	// Demande d'un nouveau nom de fichier
	sFilePath = fileChooserCard.ChooseFile("Choose file", "Choose", "FileChooser", "", "ChooseFile", "File",
					       actionSubObject->GetFilePath());

	// Memorisation si neessaire
	if (sFilePath != "")
		actionSubObject->SetFilePath(sFilePath);
}

void UITestActionSubObjectView::AskQuestion()
{
	UIQuestionCard questionCard;
	boolean bAnswer;
	ALString sTmp;

	bAnswer = questionCard.GetAnswer("C'est la nuit", "Question", "Il est tard");
	AddSimpleMessage(sTmp + "The asnwer is: " + BooleanToString(bAnswer));
}

void UITestActionSubObjectView::OK()
{
	AddSimpleMessage("Quit application");
}

// ##
