// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWDiscretizerSpecView.h"

KWDiscretizerSpecView::KWDiscretizerSpecView()
{
	SetIdentifier("KWDiscretizerSpec");
	SetLabel("Discretization");
	AddStringField("SupervisedMethodName", "Supervised method", "");
	AddStringField("UnsupervisedMethodName", "Unsupervised method", "");
	AddIntField("MinIntervalFrequency", "Min interval frequency", 0);
	AddIntField("MaxIntervalNumber", "Max interval number", 0);

	// Parametrage des styles;
	GetFieldAt("SupervisedMethodName")->SetStyle("ComboBox");
	GetFieldAt("UnsupervisedMethodName")->SetStyle("ComboBox");
	GetFieldAt("MinIntervalFrequency")->SetStyle("Spinner");
	GetFieldAt("MaxIntervalNumber")->SetStyle("Spinner");

	// ## Custom constructor

	// Contrainte sur les valeurs
	cast(UIIntElement*, GetFieldAt("MinIntervalFrequency"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MinIntervalFrequency"))->SetMaxValue(1000000);
	cast(UIIntElement*, GetFieldAt("MaxIntervalNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxIntervalNumber"))->SetMaxValue(1000000);

	// Valeurs possibles initiales pour la discretization
	GetFieldAt("SupervisedMethodName")->SetParameters("MODL\nEqualFrequency\nEqualWidth");
	GetFieldAt("UnsupervisedMethodName")->SetParameters("EqualWidth\nEqualFrequency\nNone");

	// Info-bulles
	GetFieldAt("SupervisedMethodName")
	    ->SetHelpText("Name of the discretization method in case of classification or regression.");
	GetFieldAt("UnsupervisedMethodName")
	    ->SetHelpText("Name of the discretization method in case of unsupervised analysis.");
	GetFieldAt("MinIntervalFrequency")
	    ->SetHelpText("Min number of instances in each interval."
			  "\n When this user constraint is active, it has priority over the criterion of the "
			  "discretization method."
			  "\n Default value 0: automatically set by the method.");
	GetFieldAt("MaxIntervalNumber")
	    ->SetHelpText("Max number of intervals produced by the discretization."
			  "\n When this user constraint is active, it has priority over the criterion of the "
			  "discretization method."
			  "\n Default value 0: automatically set by the method (10 intervals in case of EqualWidth or "
			  "EqualFrequency).");

	// ##
}

KWDiscretizerSpecView::~KWDiscretizerSpecView()
{
	// ## Custom destructor

	// ##
}

KWDiscretizerSpec* KWDiscretizerSpecView::GetKWDiscretizerSpec()
{
	require(objValue != NULL);
	return cast(KWDiscretizerSpec*, objValue);
}

void KWDiscretizerSpecView::EventUpdate(Object* object)
{
	KWDiscretizerSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWDiscretizerSpec*, object);
	editedObject->SetSupervisedMethodName(GetStringValueAt("SupervisedMethodName"));
	editedObject->SetUnsupervisedMethodName(GetStringValueAt("UnsupervisedMethodName"));
	editedObject->SetMinIntervalFrequency(GetIntValueAt("MinIntervalFrequency"));
	editedObject->SetMaxIntervalNumber(GetIntValueAt("MaxIntervalNumber"));

	// ## Custom update

	// ##
}

void KWDiscretizerSpecView::EventRefresh(Object* object)
{
	KWDiscretizerSpec* editedObject;

	require(object != NULL);

	editedObject = cast(KWDiscretizerSpec*, object);
	SetStringValueAt("SupervisedMethodName", editedObject->GetSupervisedMethodName());
	SetStringValueAt("UnsupervisedMethodName", editedObject->GetUnsupervisedMethodName());
	SetIntValueAt("MinIntervalFrequency", editedObject->GetMinIntervalFrequency());
	SetIntValueAt("MaxIntervalNumber", editedObject->GetMaxIntervalNumber());

	// ## Custom refresh

	// ##
}

const ALString KWDiscretizerSpecView::GetClassLabel() const
{
	return "Discretization";
}

// ## Method implementation

// ##