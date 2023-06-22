// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KDTextFeatureSpecView.h"

KDTextFeatureSpecView::KDTextFeatureSpecView()
{
	SetIdentifier("KDTextFeatureSpec");
	SetLabel("Text feature parameters");
	AddStringField("TextFeatures", "Text features", "");

	// Parametrage des styles;
	GetFieldAt("TextFeatures")->SetStyle("ComboBox");

	// ## Custom constructor

	GetFieldAt("TextFeatures")->SetParameters("n-grams\nwords");

	GetFieldAt("TextFeatures")
	    ->SetHelpText("Type of contructed texte features:"
			  "\n . n-grams: n-grams of bytes; fast, robust, but not interpretable"
			  "\n . words: interpretable tokens extracted from text variables,"
			  "\n    whose interest depends on the quality of the text preprocessing.");

	// ##
}

KDTextFeatureSpecView::~KDTextFeatureSpecView()
{
	// ## Custom destructor

	// ##
}

KDTextFeatureSpec* KDTextFeatureSpecView::GetKDTextFeatureSpec()
{
	require(objValue != NULL);
	return cast(KDTextFeatureSpec*, objValue);
}

void KDTextFeatureSpecView::EventUpdate(Object* object)
{
	KDTextFeatureSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KDTextFeatureSpec*, object);
	editedObject->SetTextFeatures(GetStringValueAt("TextFeatures"));

	// ## Custom update

	// ##
}

void KDTextFeatureSpecView::EventRefresh(Object* object)
{
	KDTextFeatureSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KDTextFeatureSpec*, object);
	SetStringValueAt("TextFeatures", editedObject->GetTextFeatures());

	// ## Custom refresh

	// ##
}

const ALString KDTextFeatureSpecView::GetClassLabel() const
{
	return "Text feature parameters";
}

// ## Method implementation

// ##