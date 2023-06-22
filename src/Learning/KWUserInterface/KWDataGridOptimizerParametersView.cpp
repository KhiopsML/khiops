// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2021-04-25 11:10:57
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "KWDataGridOptimizerParametersView.h"

KWDataGridOptimizerParametersView::KWDataGridOptimizerParametersView()
{
	SetIdentifier("KWDataGridOptimizerParameters");
	SetLabel("Data Grid optimization");
	AddStringField("OptimizationAlgorithm", "Algorithm", "");
	AddIntField("OptimizationTime", "Optimization time", 0);
	AddIntField("OptimizationLevel", "Level", 0);
	AddBooleanField("UnivariateInitialization", "Univariate initialization", false);
	AddBooleanField("PreOptimize", "Pre-optimize each solution", false);
	AddBooleanField("Optimize", "Optimize each solution", false);
	AddBooleanField("PostOptimize", "Post-optimize each solution", false);
	AddStringField("InternalParameter", "Internal parameter", "");
	AddBooleanField("DisplayDetails", "Display details", false);

	// Parametrage des styles;
	GetFieldAt("OptimizationAlgorithm")->SetStyle("ComboBox");
	GetFieldAt("OptimizationTime")->SetStyle("Spinner");
	GetFieldAt("OptimizationLevel")->SetStyle("Spinner");
	GetFieldAt("UnivariateInitialization")->SetStyle("CheckBox");
	GetFieldAt("PreOptimize")->SetStyle("CheckBox");
	GetFieldAt("Optimize")->SetStyle("CheckBox");
	GetFieldAt("PostOptimize")->SetStyle("CheckBox");
	GetFieldAt("DisplayDetails")->SetStyle("CheckBox");

	// ## Custom constructor

	// Contrainte sur les valeurs
	GetFieldAt("OptimizationAlgorithm")->SetParameters("Greedy\nMultiStart\nVNS\nNone");
	cast(UIIntElement*, GetFieldAt("OptimizationTime"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("OptimizationTime"))->SetMaxValue(100000000);
	cast(UIIntElement*, GetFieldAt("OptimizationLevel"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("OptimizationLevel"))->SetMaxValue(20);

	// Parametre interne non accessible depuis l'interface, meme en mode expert
	GetFieldAt("InternalParameter")->SetVisible(false);

	// ##
}

KWDataGridOptimizerParametersView::~KWDataGridOptimizerParametersView()
{
	// ## Custom destructor

	// ##
}

KWDataGridOptimizerParameters* KWDataGridOptimizerParametersView::GetKWDataGridOptimizerParameters()
{
	require(objValue != NULL);
	return cast(KWDataGridOptimizerParameters*, objValue);
}

void KWDataGridOptimizerParametersView::EventUpdate(Object* object)
{
	KWDataGridOptimizerParameters* editedObject;

	require(object != NULL);

	editedObject = cast(KWDataGridOptimizerParameters*, object);
	editedObject->SetOptimizationAlgorithm(GetStringValueAt("OptimizationAlgorithm"));
	editedObject->SetOptimizationTime(GetIntValueAt("OptimizationTime"));
	editedObject->SetOptimizationLevel(GetIntValueAt("OptimizationLevel"));
	editedObject->SetUnivariateInitialization(GetBooleanValueAt("UnivariateInitialization"));
	editedObject->SetPreOptimize(GetBooleanValueAt("PreOptimize"));
	editedObject->SetOptimize(GetBooleanValueAt("Optimize"));
	editedObject->SetPostOptimize(GetBooleanValueAt("PostOptimize"));
	editedObject->SetInternalParameter(GetStringValueAt("InternalParameter"));
	editedObject->SetDisplayDetails(GetBooleanValueAt("DisplayDetails"));

	// ## Custom update

	// ##
}

void KWDataGridOptimizerParametersView::EventRefresh(Object* object)
{
	KWDataGridOptimizerParameters* editedObject;

	require(object != NULL);

	editedObject = cast(KWDataGridOptimizerParameters*, object);
	SetStringValueAt("OptimizationAlgorithm", editedObject->GetOptimizationAlgorithm());
	SetIntValueAt("OptimizationTime", editedObject->GetOptimizationTime());
	SetIntValueAt("OptimizationLevel", editedObject->GetOptimizationLevel());
	SetBooleanValueAt("UnivariateInitialization", editedObject->GetUnivariateInitialization());
	SetBooleanValueAt("PreOptimize", editedObject->GetPreOptimize());
	SetBooleanValueAt("Optimize", editedObject->GetOptimize());
	SetBooleanValueAt("PostOptimize", editedObject->GetPostOptimize());
	SetStringValueAt("InternalParameter", editedObject->GetInternalParameter());
	SetBooleanValueAt("DisplayDetails", editedObject->GetDisplayDetails());

	// ## Custom refresh

	// ##
}

const ALString KWDataGridOptimizerParametersView::GetClassLabel() const
{
	return "Data Grid optimization";
}

// ## Method implementation

// ##
