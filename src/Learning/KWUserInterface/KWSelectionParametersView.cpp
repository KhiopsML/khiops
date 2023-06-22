// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2018-07-19 14:35:54
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "KWSelectionParametersView.h"

KWSelectionParametersView::KWSelectionParametersView()
{
	SetIdentifier("KWSelectionParameters");
	SetLabel("Selection parameters");
	AddIntField("MaxSelectedAttributeNumber", "Max number of selected variables", 0);
	AddStringField("SelectionCriterion", "Selection criterion", "");
	AddStringField("OptimizationAlgorithm", "Optimization algorithm", "");
	AddIntField("OptimizationLevel", "Optimization level", 0);
	AddDoubleField("PriorWeight", "Prior weight (expert)", 0);
	AddDoubleField("PriorExponent", "Prior exponent (expert)", 0);
	AddBooleanField("ConstructionCost", "Construction cost (expert)", false);
	AddBooleanField("PreparationCost", "Preparation cost (expert)", false);
	AddIntField("TraceLevel", "Trace level", 0);
	AddBooleanField("TraceSelectedAttributes", "Trace selected variables", false);

	// Parametrage des styles;
	GetFieldAt("MaxSelectedAttributeNumber")->SetStyle("Spinner");
	GetFieldAt("SelectionCriterion")->SetStyle("ComboBox");
	GetFieldAt("OptimizationAlgorithm")->SetStyle("ComboBox");
	GetFieldAt("OptimizationLevel")->SetStyle("Spinner");
	GetFieldAt("ConstructionCost")->SetStyle("CheckBox");
	GetFieldAt("PreparationCost")->SetStyle("CheckBox");
	GetFieldAt("TraceLevel")->SetStyle("Spinner");
	GetFieldAt("TraceSelectedAttributes")->SetStyle("CheckBox");

	// ## Custom constructor

	// Parametrage specifique
	cast(UIIntElement*, GetFieldAt("MaxSelectedAttributeNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxSelectedAttributeNumber"))->SetMaxValue(1000000);
	GetFieldAt("SelectionCriterion")->SetParameters("CMA\nMA\nMAP");
	GetFieldAt("OptimizationAlgorithm")->SetParameters("MS_FFWBW\nOPT\nFW\nFWBW\nFFW\nFFWBW");
	cast(UIIntElement*, GetFieldAt("OptimizationLevel"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("OptimizationLevel"))->SetMaxValue(1000);
	cast(UIIntElement*, GetFieldAt("TraceLevel"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("TraceLevel"))->SetMaxValue(3);
	cast(UIDoubleElement*, GetFieldAt("PriorWeight"))->SetMinValue(0);
	cast(UIDoubleElement*, GetFieldAt("PriorWeight"))->SetMaxValue(1);

	// Le parametrage expert n'est visible qu'en mode expert
	GetFieldAt("SelectionCriterion")->SetVisible(GetLearningExpertMode());
	GetFieldAt("OptimizationAlgorithm")->SetVisible(GetLearningExpertMode());
	GetFieldAt("OptimizationLevel")->SetVisible(GetLearningExpertMode());
	GetFieldAt("PriorWeight")->SetVisible(GetLearningPriorStudyMode());
	GetFieldAt("PriorExponent")->SetVisible(GetLearningPriorStudyMode());
	GetFieldAt("ConstructionCost")->SetVisible(GetLearningPriorStudyMode());
	GetFieldAt("PreparationCost")->SetVisible(GetLearningPriorStudyMode());
	GetFieldAt("TraceLevel")->SetVisible(GetLearningExpertMode());
	GetFieldAt("TraceSelectedAttributes")->SetVisible(GetLearningExpertMode());

	// Info-bulles
	GetFieldAt("MaxSelectedAttributeNumber")
	    ->SetHelpText(
		"Max number of variables originating from the multivariate variable selection,"
		"\n to use in the final Selective Naive Bayes predictor."
		"\n The selected variables are those with the largest importance in the multivariate selection."
		"\n This parameter allows to simplify and speed up the deployment phase"
		"\n (default: 0, means that all variables with non-zero weight are selected).");

	// TODO : Eliminer pour de vrai les options suivantes de l'ancien SNB
	GetFieldAt("SelectionCriterion")->SetVisible(false);
	GetFieldAt("OptimizationAlgorithm")->SetVisible(false);
	GetFieldAt("OptimizationLevel")->SetVisible(false);
	GetFieldAt("TraceLevel")->SetVisible(false);
	GetFieldAt("TraceSelectedAttributes")->SetVisible(false);

	// ##
}

KWSelectionParametersView::~KWSelectionParametersView()
{
	// ## Custom destructor

	// ##
}

KWSelectionParameters* KWSelectionParametersView::GetKWSelectionParameters()
{
	require(objValue != NULL);
	return cast(KWSelectionParameters*, objValue);
}

void KWSelectionParametersView::EventUpdate(Object* object)
{
	KWSelectionParameters* editedObject;

	require(object != NULL);

	editedObject = cast(KWSelectionParameters*, object);
	editedObject->SetMaxSelectedAttributeNumber(GetIntValueAt("MaxSelectedAttributeNumber"));
	editedObject->SetSelectionCriterion(GetStringValueAt("SelectionCriterion"));
	editedObject->SetOptimizationAlgorithm(GetStringValueAt("OptimizationAlgorithm"));
	editedObject->SetOptimizationLevel(GetIntValueAt("OptimizationLevel"));
	editedObject->SetPriorWeight(GetDoubleValueAt("PriorWeight"));
	editedObject->SetPriorExponent(GetDoubleValueAt("PriorExponent"));
	editedObject->SetConstructionCost(GetBooleanValueAt("ConstructionCost"));
	editedObject->SetPreparationCost(GetBooleanValueAt("PreparationCost"));
	editedObject->SetTraceLevel(GetIntValueAt("TraceLevel"));
	editedObject->SetTraceSelectedAttributes(GetBooleanValueAt("TraceSelectedAttributes"));

	// ## Custom update

	// ##
}

void KWSelectionParametersView::EventRefresh(Object* object)
{
	KWSelectionParameters* editedObject;

	require(object != NULL);

	editedObject = cast(KWSelectionParameters*, object);
	SetIntValueAt("MaxSelectedAttributeNumber", editedObject->GetMaxSelectedAttributeNumber());
	SetStringValueAt("SelectionCriterion", editedObject->GetSelectionCriterion());
	SetStringValueAt("OptimizationAlgorithm", editedObject->GetOptimizationAlgorithm());
	SetIntValueAt("OptimizationLevel", editedObject->GetOptimizationLevel());
	SetDoubleValueAt("PriorWeight", editedObject->GetPriorWeight());
	SetDoubleValueAt("PriorExponent", editedObject->GetPriorExponent());
	SetBooleanValueAt("ConstructionCost", editedObject->GetConstructionCost());
	SetBooleanValueAt("PreparationCost", editedObject->GetPreparationCost());
	SetIntValueAt("TraceLevel", editedObject->GetTraceLevel());
	SetBooleanValueAt("TraceSelectedAttributes", editedObject->GetTraceSelectedAttributes());

	// ## Custom refresh

	// ##
}

const ALString KWSelectionParametersView::GetClassLabel() const
{
	return "Selection parameters";
}

// ## Method implementation

// ##
