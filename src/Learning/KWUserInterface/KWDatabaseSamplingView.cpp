// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWDatabaseSamplingView.h"

KWDatabaseSamplingView::KWDatabaseSamplingView()
{
	SetIdentifier("KWDatabaseSampling");
	SetLabel("Sampling");
	AddDoubleField("SampleNumberPercentage", "Sample percentage", 0);
	AddStringField("SamplingMode", "Sampling mode", "");

	// Parametrage des styles;
	GetFieldAt("SampleNumberPercentage")->SetStyle("Spinner");
	GetFieldAt("SamplingMode")->SetStyle("ComboBox");

	// ## Custom constructor

	// Precision a 1 chiffre apres la virgule pour le pourcentage
	GetFieldAt("SampleNumberPercentage")->SetParameters("1");

	// Plage de valeur pour le champ pourcentage des exemples
	cast(UIDoubleElement*, GetFieldAt("SampleNumberPercentage"))->SetMinValue(0);
	cast(UIDoubleElement*, GetFieldAt("SampleNumberPercentage"))->SetMaxValue(100);

	// Valeurs pour le mode d'echantillonnage
	GetFieldAt("SamplingMode")->SetParameters("Include sample\nExclude sample");

	// Info-bulles
	GetFieldAt("SampleNumberPercentage")->SetHelpText("Percentage of samples.");
	GetFieldAt("SamplingMode")
	    ->SetHelpText("To include or exclude the records of the sample."
			  "\n This allows to extract a train sample and its exact complementary as a test sample"
			  "\n (if the same sample percentage is used both in train and test, "
			  "\n in include sample mode in train and exclude sample mode in test).");

	// ##
}

KWDatabaseSamplingView::~KWDatabaseSamplingView()
{
	// ## Custom destructor

	// ##
}

KWDatabase* KWDatabaseSamplingView::GetKWDatabase()
{
	require(objValue != NULL);
	return cast(KWDatabase*, objValue);
}

void KWDatabaseSamplingView::EventUpdate(Object* object)
{
	KWDatabase* editedObject;

	require(object != NULL);

	editedObject = cast(KWDatabase*, object);
	editedObject->SetSampleNumberPercentage(GetDoubleValueAt("SampleNumberPercentage"));
	editedObject->SetSamplingMode(GetStringValueAt("SamplingMode"));

	// ## Custom update

	// ##
}

void KWDatabaseSamplingView::EventRefresh(Object* object)
{
	KWDatabase* editedObject;

	require(object != NULL);

	editedObject = cast(KWDatabase*, object);
	SetDoubleValueAt("SampleNumberPercentage", editedObject->GetSampleNumberPercentage());
	SetStringValueAt("SamplingMode", editedObject->GetSamplingMode());

	// ## Custom refresh

	// ##
}

const ALString KWDatabaseSamplingView::GetClassLabel() const
{
	return "Sampling";
}

// ## Method implementation

// ##
