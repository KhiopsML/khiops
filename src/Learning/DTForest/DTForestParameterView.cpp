// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTForestParameterView.h"
#include "DTDecisionTreeParameterView.h"
// #include "DTForest.h"

DTForestParameterView::DTForestParameterView()
{
	// Parametrage principal de l'interface
	SetIdentifier("RFRandomForestClassifier");
	SetLabel("RF RandomForest Classifier");

	// parametres specifiques aux random forests :

	// Parametres de structure de l'arbre
	AddStringField("TreesVariablesSelection", "Trees variables selection",
		       DTGlobalTag::RANK_WITH_REPLACEMENT_LABEL);
	GetFieldAt("TreesVariablesSelection")
	    ->SetParameters(DTGlobalTag::RANK_WITH_REPLACEMENT_LABEL + '\n' +
			    DTGlobalTag::UNIFORM_SAMPLING_WITH_REPLACEMENT_LABEL + '\n' +
			    DTGlobalTag::LEVEL_SAMPLING_WITH_REPLACEMENT_LABEL + '\n' +
			    DTGlobalTag::NODE_UNIFORM_SAMPLING_WITH_REPLACEMENT_LABEL + '\n' +
			    DTGlobalTag::NODE_LEVEL_SAMPLING_WITH_REPLACEMENT_LABEL);
	AddDoubleField("InstancePercentage", "Percentage of instances used", 1.0);
	AddDoubleField("VariablePercentage", "Percentage of variables used", 0.0);
	AddIntField("VariableNumberMin", "Variable Number Min", MIN_VARIABLE_2_BUILTREE);

	AddStringField("DrawingType", "Drawing type", DTForestParameter::DRAWING_TYPE_NO_REPLACEMENT_LABEL);
	GetFieldAt("DrawingType")
	    ->SetParameters(DTForestParameter::DRAWING_TYPE_NO_REPLACEMENT_LABEL + '\n' +
			    DTForestParameter::DRAWING_TYPE_USE_OUT_OF_BAG_LABEL);

	AddStringField("DiscretizationTargetMethod", "Discretization target method",
		       DTForestParameter::DISCRETIZATION_MODL);
	GetFieldAt("DiscretizationTargetMethod")
	    ->SetParameters(DTForestParameter::DISCRETIZATION_MODL + "\n" +
			    DTForestParameter::DISCRETIZATION_BINARY_EQUAL_FREQUENCY + "\n" +
			    DTForestParameter::DISCRETIZATION_EQUAL_FREQUENCY);
	AddIntField("MaxIntervalsNumberForTarget", "Max intervals number for target ", 2);

	AddIntField("OptimizationLoopNumber", "Optimization loop number", 10);
	AddStringField("Weigth", "Weighting", "UNIFORM");
	GetFieldAt("Weigth")->SetParameters("UNIFORM\nCOMPRESSION RATE");
	AddBooleanField("RecodeRFDictionary", "Recode RF Dictionnary", true);
	AddBooleanField("WriteDetailedStatistics", "Write detailed statistics", false);

	// parametrage des styles
	GetFieldAt("Weigth")->SetStyle("ComboBox");
	GetFieldAt("TreesVariablesSelection")->SetStyle("ComboBox");
	GetFieldAt("OptimizationLoopNumber")->SetStyle("Spinner");
	GetFieldAt("VariableNumberMin")->SetStyle("Spinner");
	GetFieldAt("DrawingType")->SetStyle("ComboBox");
	GetFieldAt("DiscretizationTargetMethod")->SetStyle("ComboBox");
	GetFieldAt("MaxIntervalsNumberForTarget")->SetStyle("Spinner");

	// Contraintes sur les valeurs
	cast(UIDoubleElement*, GetFieldAt("VariablePercentage"))->SetMinValue(0.0);
	cast(UIDoubleElement*, GetFieldAt("VariablePercentage"))->SetMaxValue(1.0);
	cast(UIIntElement*, GetFieldAt("OptimizationLoopNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("OptimizationLoopNumber"))->SetMaxValue(500);
	cast(UIDoubleElement*, GetFieldAt("InstancePercentage"))->SetMinValue(0.0);
	cast(UIDoubleElement*, GetFieldAt("InstancePercentage"))->SetMaxValue(1.0);
	cast(UIIntElement*, GetFieldAt("VariableNumberMin"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("VariableNumberMin"))->SetMaxValue(20);
	cast(UIIntElement*, GetFieldAt("MaxIntervalsNumberForTarget"))->SetMinValue(2);
	cast(UIIntElement*, GetFieldAt("MaxIntervalsNumberForTarget"))->SetMaxValue(64);

	// rendre visibles ou invisibles les champs du mode expert :
	GetFieldAt("InstancePercentage")->SetVisible(GetLearningExpertMode());
	GetFieldAt("OptimizationLoopNumber")->SetVisible(GetLearningExpertMode());
	GetFieldAt("RecodeRFDictionary")->SetVisible(GetLearningExpertMode());
	GetFieldAt("WriteDetailedStatistics")->SetVisible(GetLearningExpertMode());

	// SetStyle("TabbedPanes");
}

DTForestParameterView::~DTForestParameterView() {}

DTForestParameterView* DTForestParameterView::Create() const
{
	return new DTForestParameterView;
}

void DTForestParameterView::EventUpdate(Object* object)
{
	DTForestParameter* editedObject;

	require(object != NULL);

	editedObject = cast(DTForestParameter*, object);

	// DTDecisionTreeParameterView::EventUpdate(object);

	editedObject->SetTreesVariablesSelection(GetStringValueAt("TreesVariablesSelection"));
	editedObject->SetAttributePercentage(GetDoubleValueAt("VariablePercentage"));
	editedObject->SetWeightedClassifier(GetStringValueAt("Weigth"));
	editedObject->SetWriteDetailedStatistics(GetBooleanValueAt("WriteDetailedStatistics"));
	editedObject->SetInstancePercentage(GetDoubleValueAt("InstancePercentage"));
	editedObject->SetRecodeRFDictionary(GetBooleanValueAt("RecodeRFDictionary"));
	editedObject->SetDrawingType(GetStringValueAt("DrawingType"));
	editedObject->SetOptimizationLoopNumber(GetIntValueAt("OptimizationLoopNumber"));
	editedObject->SetVariableNumberMin(GetIntValueAt("VariableNumberMin"));
	editedObject->SetDiscretizationTargetMethod(GetStringValueAt("DiscretizationTargetMethod"));
	editedObject->SetMaxIntervalsNumberForTarget(GetIntValueAt("MaxIntervalsNumberForTarget"));

	// mise a jour automatique du NodeVariablesSelection en fonction du TreesVariablesSelection

	const ALString s = GetStringValueAt("TreesVariablesSelection");

	if (s == DTGlobalTag::RANK_WITH_REPLACEMENT_LABEL or
	    s == DTGlobalTag::UNIFORM_SAMPLING_WITH_REPLACEMENT_LABEL or
	    s == DTGlobalTag::LEVEL_SAMPLING_WITH_REPLACEMENT_LABEL)

		editedObject->GetDecisionTreeParameter()->SetNodeVariablesSelection(DTGlobalTag::IDENTICAL_LABEL);

	else

	    if (s == DTGlobalTag::NODE_UNIFORM_SAMPLING_WITH_REPLACEMENT_LABEL)
		editedObject->GetDecisionTreeParameter()->SetNodeVariablesSelection(DTGlobalTag::RANDOM_UNIFORM_LABEL);
	else if (s == DTGlobalTag::NODE_LEVEL_SAMPLING_WITH_REPLACEMENT_LABEL)
		editedObject->GetDecisionTreeParameter()->SetNodeVariablesSelection(DTGlobalTag::RANDOM_LEVEL_LABEL);
}

void DTForestParameterView::EventRefresh(Object* object)
{
	DTForestParameter* editedObject;
	require(object != NULL);

	editedObject = cast(DTForestParameter*, object);

	SetStringValueAt("TreesVariablesSelection", editedObject->GetTreesVariablesSelection());
	SetDoubleValueAt("VariablePercentage", editedObject->GetAttributePercentage());
	SetStringValueAt("Weigth", editedObject->GetWeightedClassifier());
	SetBooleanValueAt("WriteDetailedStatistics", editedObject->IsWriteDetailedStatistics());
	SetDoubleValueAt("InstancePercentage", editedObject->GetInstancePercentage());
	SetBooleanValueAt("RecodeRFDictionary", editedObject->GetRecodeRFDictionary());
	SetStringValueAt("DrawingType", editedObject->GetDrawingTypeLabel());
	SetIntValueAt("OptimizationLoopNumber", editedObject->GetOptimizationLoopNumber());
	SetIntValueAt("VariableNumberMin", editedObject->GetVariableNumberMin());
	SetStringValueAt("DiscretizationTargetMethod", editedObject->GetDiscretizationTargetMethod());
	SetIntValueAt("MaxIntervalsNumberForTarget", editedObject->GetMaxIntervalsNumberForTarget());
}