// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWPreprocessingAdvancedSpecView.h"

KWPreprocessingAdvancedSpecView::KWPreprocessingAdvancedSpecView()
{
	SetIdentifier("KWPreprocessingAdvancedSpec");
	SetLabel("Unsupervised parameters");
	AddStringField("DiscretizerUnsupervisedMethodName", "Discretization method", "");
	AddStringField("GrouperUnsupervisedMethodName", "Grouping method", "");

	// Parametrage des styles;
	GetFieldAt("DiscretizerUnsupervisedMethodName")->SetStyle("ComboBox");
	GetFieldAt("GrouperUnsupervisedMethodName")->SetStyle("ComboBox");

	// ## Custom constructor

	// Valeurs possibles pour les pretraitements non supervises
	GetFieldAt("DiscretizerUnsupervisedMethodName")->SetParameters("MODL\nEqualWidth\nEqualFrequency\nNone");
	GetFieldAt("GrouperUnsupervisedMethodName")->SetParameters("MODL\nBasicGrouping\nNone");

	// Info-bulles
	GetFieldAt("DiscretizerUnsupervisedMethodName")
	    ->SetHelpText("Name of the discretization method in case of unsupervised analysis.");
	GetFieldAt("GrouperUnsupervisedMethodName")
	    ->SetHelpText("Name of the value grouping method in case of unsupervised analysis.");

	// ##
}

KWPreprocessingAdvancedSpecView::~KWPreprocessingAdvancedSpecView()
{
	// ## Custom destructor

	// ##
}

KWPreprocessingSpec* KWPreprocessingAdvancedSpecView::GetKWPreprocessingSpec()
{
	require(objValue != NULL);
	return cast(KWPreprocessingSpec*, objValue);
}

void KWPreprocessingAdvancedSpecView::EventUpdate(Object* object)
{
	KWPreprocessingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWPreprocessingSpec*, object);
	editedObject->SetDiscretizerUnsupervisedMethodName(GetStringValueAt("DiscretizerUnsupervisedMethodName"));
	editedObject->SetGrouperUnsupervisedMethodName(GetStringValueAt("GrouperUnsupervisedMethodName"));

	// ## Custom update

	// ##
}

void KWPreprocessingAdvancedSpecView::EventRefresh(Object* object)
{
	KWPreprocessingSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWPreprocessingSpec*, object);
	SetStringValueAt("DiscretizerUnsupervisedMethodName", editedObject->GetDiscretizerUnsupervisedMethodName());
	SetStringValueAt("GrouperUnsupervisedMethodName", editedObject->GetGrouperUnsupervisedMethodName());

	// ## Custom refresh

	// ##
}

const ALString KWPreprocessingAdvancedSpecView::GetClassLabel() const
{
	return "Unsupervised parameters";
}

// ## Method implementation

// ##
