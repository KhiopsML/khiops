// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2021-01-31 18:14:10
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "UITestObjectView.h"

UITestObjectView::UITestObjectView()
{
	SetIdentifier("UITestObject");
	SetLabel("Test object");
	AddIntField("Duration", "Duration", 0);
	AddStringField("Result", "Result", "");

	// Parametrage des styles;
	GetFieldAt("Duration")->SetStyle("Spinner");

	// ## Custom constructor

	// Le resulta n'est pas editable
	GetFieldAt("Result")->SetEditable(false);

	// Limitation des valeurs du champ entier
	cast(UIIntElement*, GetFieldAt("Duration"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("Duration"))->SetMaxValue(100);

	// Ajout de la sous-fiche editee localement
	AddCardField("TestActionSubObject", "Sub card with action", new UITestActionSubObjectView);

	// Ajout des actions
	AddAction("InspectSubObject", "Inspect sub-object...", (ActionMethod)(&UITestObjectView::InspectSubObject));
	AddAction("StartBatchProcessing", "Start batch processing",
		  (ActionMethod)(&UITestObjectView::StartBatchProcessing));

	// On les met sous forme de bouton
	GetActionAt("InspectSubObject")->SetStyle("Button");
	GetActionAt("StartBatchProcessing")->SetStyle("SmallButton");

	// On enleve le bouton de sortie
	GetActionAt("Exit")->SetVisible(false);

	// ##
}

UITestObjectView::~UITestObjectView()
{
	// ## Custom destructor

	// ##
}

UITestObject* UITestObjectView::GetUITestObject()
{
	require(objValue != NULL);
	return cast(UITestObject*, objValue);
}

void UITestObjectView::EventUpdate(Object* object)
{
	UITestObject* editedObject;

	require(object != NULL);

	editedObject = cast(UITestObject*, object);
	editedObject->SetDuration(GetIntValueAt("Duration"));
	editedObject->SetResult(GetStringValueAt("Result"));

	// ## Custom update

	// ##
}

void UITestObjectView::EventRefresh(Object* object)
{
	UITestObject* editedObject;

	require(object != NULL);

	editedObject = cast(UITestObject*, object);
	SetIntValueAt("Duration", editedObject->GetDuration());
	SetStringValueAt("Result", editedObject->GetResult());

	// ## Custom refresh

	// ##
}

const ALString UITestObjectView::GetClassLabel() const
{
	return "Test object";
}

// ## Method implementation

void UITestObjectView::InspectSubObject()
{
	UITestSubObjectView testSubObjectView;

	// Parametrage du sous-objet edite par la sous-fiche
	cout << "UITestObjectView::InspectSubObject Start" << endl;
	testSubObjectView.SetObject(GetTestObject()->GetSubObject());
	testSubObjectView.Open();
	cout << "UITestObjectView::InspectSubObject Stop" << endl;
}

void UITestObjectView::StartBatchProcessing()
{
	int nMax;
	int i;
	ALString sTmp;

	// Acces a l'objet edite
	nMax = GetTestObject()->GetDuration();
	AddSimpleMessage("Batch");
	for (i = 0; i < nMax; i++)
	{
		SystemSleep(1);
		AddSimpleMessage(sTmp + "\tIter" + IntToString(i + 1));
	}
	GetTestObject()->SetResult(sTmp + "Batch " + IntToString(nMax) + " done");
}

void UITestObjectView::SetObject(Object* object)
{
	UITestObject* testObject;

	require(object != NULL);

	// Appel de la methode ancetre
	UIObjectView::SetObject(object);

	// Acces a l'objet edite
	testObject = cast(UITestObject*, object);

	// Parametrage de la sous-vue editee localement a la fiche
	cast(UITestActionSubObjectView*, GetFieldAt("TestActionSubObject"))
	    ->SetObject(testObject->GetActionSubObject());
}

UITestObject* UITestObjectView::GetTestObject()
{
	return cast(UITestObject*, objValue);
}

// ##