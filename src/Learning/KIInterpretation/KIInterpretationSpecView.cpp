// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

/*
 * #%L
 * Software Name: Khiops Interpretation
 * Version : 9.0
 * %%
 * Copyright (C) 2019 Orange
 * This software is the confidential and proprietary information of Orange.
 * You shall not disclose such confidential information and shall use it only
 * in accordance with the terms of the license agreement you entered into
 * with Orange.
 * #L%
 */

#include "KIInterpretationSpecView.h"

KIInterpretationSpecView::KIInterpretationSpecView()
{
	UIList* keyAttributeNameHelpList;

	SetIdentifier("KIInterpretationSpec");
	SetLabel("Interpretation parameters");

	whyParameterView = new KIWhyParameterView;
	AddCardField("WhyParameter", "Contribution", whyParameterView);

	howParameterView = new KIHowParameterView;
	AddCardField("HowParameter", "Reinforcement", howParameterView);

	// Passage en ergonomie onglets
	SetStyle("TabbedPanes");

	// Creation d'une liste cachee des attributs de la classe en cours
	keyAttributeNameHelpList = new UIList;
	keyAttributeNameHelpList->AddStringField("Name", "Name", "");
	AddListField("Attributes", "Variables", keyAttributeNameHelpList);
	keyAttributeNameHelpList->SetVisible(false);

	GetActionAt("Exit")->SetLabel("Close");
}

KIInterpretationSpecView::~KIInterpretationSpecView() {}

void KIInterpretationSpecView::SetObject(Object* object)
{

	UIObjectView::SetObject(object);
	whyParameterView->SetObject(object);
	howParameterView->SetObject(object);
}

KIInterpretationSpec* KIInterpretationSpecView::GetKIInterpretationSpec()
{
	require(objValue != NULL);
	return cast(KIInterpretationSpec*, objValue);
}

void KIInterpretationSpecView::EventUpdate(Object* object)
{
	KIInterpretationSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KIInterpretationSpec*, object);
}

void KIInterpretationSpecView::EventRefresh(Object* object)
{
	KIInterpretationSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KIInterpretationSpec*, object);
}

const ALString KIInterpretationSpecView::GetClassLabel() const
{
	return "Interpretation parameters";
}
