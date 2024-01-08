// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWGrouperSpecView.h"

KWGrouperSpecView::KWGrouperSpecView()
{
	SetIdentifier("KWGrouperSpec");
	SetLabel("Value grouping");
	AddStringField("SupervisedMethodName", "Supervised method", "");
	AddStringField("UnsupervisedMethodName", "Unsupervised method", "");
	AddIntField("MinGroupFrequency", "Min group frequency", 0);
	AddIntField("MaxGroupNumber", "Max group number", 0);

	// Parametrage des styles;
	GetFieldAt("SupervisedMethodName")->SetStyle("ComboBox");
	GetFieldAt("UnsupervisedMethodName")->SetStyle("ComboBox");
	GetFieldAt("MinGroupFrequency")->SetStyle("Spinner");
	GetFieldAt("MaxGroupNumber")->SetStyle("Spinner");

	// ## Custom constructor

	// Contrainte sur les valeurs
	cast(UIIntElement*, GetFieldAt("MinGroupFrequency"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MinGroupFrequency"))->SetMaxValue(1000000);
	cast(UIIntElement*, GetFieldAt("MaxGroupNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxGroupNumber"))->SetMaxValue(1000000);

	// Valeurs possibles initiales pour le groupage
	GetFieldAt("SupervisedMethodName")->SetParameters("MODL\nBasicGrouping");
	GetFieldAt("UnsupervisedMethodName")->SetParameters("BasicGrouping\nNone");

	// Info-bulles
	GetFieldAt("SupervisedMethodName")
	    ->SetHelpText("Name of the value grouping method in case of classification or regression.");
	GetFieldAt("UnsupervisedMethodName")
	    ->SetHelpText("Name of the value grouping method in case of unsupervised analysis.");
	GetFieldAt("MinGroupFrequency")
	    ->SetHelpText("Min number of instances in each group."
			  "\n When this user constraint is active, all the explanatory values with frequency below the "
			  "threshold are unconditionally grouped in a 'garbage' group."
			  "\n Default value 0: automatically set by the method.");
	GetFieldAt("MaxGroupNumber")
	    ->SetHelpText("Max number of groups produced by the value grouping."
			  "\n When this user constraint is active, the value grouping method does not stop the "
			  "grouping until the required number of group is reached."
			  "\n Default value 0: automatically set by the method (10 groups in case of BasicGrouping).");

	// ##
}

KWGrouperSpecView::~KWGrouperSpecView()
{
	// ## Custom destructor

	// ##
}

KWGrouperSpec* KWGrouperSpecView::GetKWGrouperSpec()
{
	require(objValue != NULL);
	return cast(KWGrouperSpec*, objValue);
}

void KWGrouperSpecView::EventUpdate(Object* object)
{
	KWGrouperSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWGrouperSpec*, object);
	editedObject->SetSupervisedMethodName(GetStringValueAt("SupervisedMethodName"));
	editedObject->SetUnsupervisedMethodName(GetStringValueAt("UnsupervisedMethodName"));
	editedObject->SetMinGroupFrequency(GetIntValueAt("MinGroupFrequency"));
	editedObject->SetMaxGroupNumber(GetIntValueAt("MaxGroupNumber"));

	// ## Custom update

	// ##
}

void KWGrouperSpecView::EventRefresh(Object* object)
{
	KWGrouperSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWGrouperSpec*, object);
	SetStringValueAt("SupervisedMethodName", editedObject->GetSupervisedMethodName());
	SetStringValueAt("UnsupervisedMethodName", editedObject->GetUnsupervisedMethodName());
	SetIntValueAt("MinGroupFrequency", editedObject->GetMinGroupFrequency());
	SetIntValueAt("MaxGroupNumber", editedObject->GetMaxGroupNumber());

	// ## Custom refresh

	// ##
}

const ALString KWGrouperSpecView::GetClassLabel() const
{
	return "Value grouping";
}

// ## Method implementation

// ##
