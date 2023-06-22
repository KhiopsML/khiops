// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#define UIDEV
#include "UserInterface.h"

UIObjectView::UIObjectView()
{
	objValue = NULL;
}

UIObjectView::~UIObjectView() {}

const ALString UIObjectView::GetClassLabel() const
{
	return "UI object view";
}

void UIObjectView::SetObject(Object* object)
{
	objValue = object;
	if (objValue != NULL)
		EventRefresh(objValue);
}

Object* UIObjectView::GetObject()
{
	if (objValue != NULL)
		EventUpdate(objValue);
	return objValue;
}

Object* UIObjectView::GetRawObject()
{
	return objValue;
}

void UIObjectView::EventRefresh()
{
	if (objValue != NULL)
		EventRefresh(objValue);
}

void UIObjectView::EventUpdate()
{
	if (objValue != NULL)
		EventUpdate(objValue);
}

////////////////////////////////////////////////////////////////////////
// Classe SampleObjectView

SampleObjectView::SampleObjectView()
{
	SetIdentifier("SampleObject");
	SetLabel("Objet Sample");
	SetHelpText("Texte d'aide des objets Sample");
	AddStringField("String", "Chaine", "");
	AddIntField("Int", "Entier", 0);
}

SampleObjectView::~SampleObjectView() {}

void SampleObjectView::EventUpdate(Object* object)
{
	SampleObject* editedObject;

	require(object != NULL);

	editedObject = cast(SampleObject*, object);
	editedObject->SetString(GetStringValueAt("String"));
	editedObject->SetInt(GetIntValueAt("Int"));
}

void SampleObjectView::EventRefresh(Object* object)
{
	SampleObject* editedObject;

	require(object != NULL);

	editedObject = cast(SampleObject*, object);
	SetStringValueAt("String", editedObject->GetString());
	SetIntValueAt("Int", editedObject->GetInt());
}