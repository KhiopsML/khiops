// Copyright (c) 2023-2025 Orange. All rights reserved.
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

	GetFieldAt("TextFeatures")->SetParameters("words\nngrams\ntokens");
	GetFieldAt("TextFeatures")
	    ->SetHelpText(
		"Type of constructed text features :"
		"\n - words : text words obtained with an automatic tokenization process"
		"\n - ngrams: ngrams of bytes; generic, fast, robust, but less interpretable"
		"\n - tokens : text tokens whose interpretability and interest depend on the quality of the input text "
		"preprocessing."
		"\n"
		"\n The \"words\" automatic tokenization process uses space or control characters as delimiters."
		"\n The obtained words are either sequences of punctuation characters or sequences of any other "
		"character."
		"\n"
		"\n The \"tokens\" tokenization process simply uses the blank character as delimiter."
		"\n This method assumes that the text has been already preprocessed (eg. lemmatization).");

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
