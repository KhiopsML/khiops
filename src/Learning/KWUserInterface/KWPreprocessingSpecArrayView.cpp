// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWPreprocessingSpecArrayView.h"

KWPreprocessingSpecArrayView::KWPreprocessingSpecArrayView()
{
	SetIdentifier("Array.KWPreprocessingSpec");
	SetLabel("Preprocessing parameterss");
	AddStringField("ObjectLabel", "Preprocessing", "");
	AddBooleanField("TargetGrouped", "Group target values", false);

	// Card and help prameters
	SetItemView(new KWPreprocessingSpecView);
	CopyCardHelpTexts();

	// Parametrage des styles;
	GetFieldAt("TargetGrouped")->SetStyle("CheckBox");

	// ## Custom constructor

	// Champ a utiliser pour les selections
	SetKeyFieldId("ObjectLabel");

	// ##
}

KWPreprocessingSpecArrayView::~KWPreprocessingSpecArrayView()
{
	// ## Custom destructor

	// ##
}

Object* KWPreprocessingSpecArrayView::EventNew()
{
	return new KWPreprocessingSpec;
}

void KWPreprocessingSpecArrayView::EventUpdate(Object* object)
{
	KWPreprocessingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWPreprocessingSpec*, object);
	editedObject->SetTargetGrouped(GetBooleanValueAt("TargetGrouped"));

	// ## Custom update

	// ##
}

void KWPreprocessingSpecArrayView::EventRefresh(Object* object)
{
	KWPreprocessingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWPreprocessingSpec*, object);
	SetStringValueAt("ObjectLabel", editedObject->GetObjectLabel());
	SetBooleanValueAt("TargetGrouped", editedObject->GetTargetGrouped());

	// ## Custom refresh

	// ##
}

const ALString KWPreprocessingSpecArrayView::GetClassLabel() const
{
	return "Preprocessing parameterss";
}

// ## Method implementation

// ##
