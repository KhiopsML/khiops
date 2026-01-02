// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTDecisionTreeParameterView.h"

DTDecisionTreeParameterView::DTDecisionTreeParameterView()
{
	// Parametrage principal de l'interface
	SetIdentifier("DTDecisionTreeParameter");
	SetLabel("DT Classifiers Parameters");

	AddStringField("NodeVariablesSelection", "Node variables selection", DTGlobalTag::IDENTICAL_LABEL);
	GetFieldAt("NodeVariablesSelection")
	    ->SetParameters(DTGlobalTag::IDENTICAL_LABEL + "\n" + DTGlobalTag::RANDOM_UNIFORM_LABEL + "\n" +
			    DTGlobalTag::RANDOM_LEVEL_LABEL);
	AddStringField("AttributesSplitSelection", "Attributes split selection", DTGlobalTag::BEST_TREE_COST_LABEL);
	GetFieldAt("AttributesSplitSelection")
	    ->SetParameters(DTGlobalTag::BEST_TREE_COST_LABEL + "\n" + DTGlobalTag::RANDOM_UNIFORM_LABEL + "\n" +
			    DTGlobalTag::RANDOM_LEVEL_LABEL);
	AddStringField("PruningMode", "Pruning mode", DTGlobalTag::POST_PRUNING_LABEL);
	GetFieldAt("PruningMode")
	    ->SetParameters(DTGlobalTag::PRE_PRUNING_LABEL + "\n" + DTGlobalTag::POST_PRUNING_LABEL + "\n" +
			    DTGlobalTag::NO_PRUNING_LABEL);
	AddStringField("TreeCost", "Selection criterion", "DTGlobalTag::REC_NEW_BIN_LABEL");
	if (GetLearningExpertMode())
		GetFieldAt("TreeCost")
		    ->SetParameters(DTGlobalTag::REC_LABEL + "\n" + DTGlobalTag::REC_RISSANEN_LABEL + "\n" +
				    DTGlobalTag::REC_RISSANEN_V1_LABEL + "\n" + DTGlobalTag::REC_RISSANEN_V2_LABEL +
				    "\n" + DTGlobalTag::REC_N2_LABEL + "\n" + DTGlobalTag::REC_NEW_BIN_LABEL + "\n" +
				    DTGlobalTag::REC_BIN_LABEL + "\n" + DTGlobalTag::REC_TRI_LABEL + "\n" +
				    DTGlobalTag::SCHRODER_LABEL + "\n" + DTGlobalTag::CATALAN_LABEL);
	else
		GetFieldAt("TreeCost")
		    ->SetParameters(DTGlobalTag::REC_NEW_BIN_LABEL + "\n" + DTGlobalTag::REC_BIN_LABEL);

	AddIntField("NbMaxChildren", "Maximum Children Number", 0);
	AddStringField("DiscretizationMethod", "Discretization method", DTGlobalTag::MODL_LABEL);
	GetFieldAt("DiscretizationMethod")
	    ->SetParameters(DTGlobalTag::MODL_LABEL + "\n" + DTGlobalTag::MODL_EQUAL_FREQUENCY_LABEL);
	AddStringField("GroupingMethod", "Grouping method", DTGlobalTag::MODL_LABEL);
	GetFieldAt("GroupingMethod")->SetParameters(DTGlobalTag::MODL_LABEL + "\n" + DTGlobalTag::BASIC_GROUPING_LABEL);
	AddIntField("NbMaxLeaves", "Maximum Leaves Number", 0);
	AddIntField("NbMaxInternalNodes", "Maximum Internal Nodes Number", 0);
	AddIntField("MinInstancesPerLeaveNumber", "Min instances number per leave", 8);
	AddIntField("MaxDepth", "Maximum Depth", 0);
	AddBooleanField("UnloadNonInformativeAttributes", "Unload non informative attributes", true);
	AddBooleanField("WithoutLeavesCosts", "Without leaves costs", true);
	AddBooleanField("VerboseMode", "Verbose mode", false);

	// parametrage des styles
	GetFieldAt("NodeVariablesSelection")->SetStyle("ComboBox");
	GetFieldAt("AttributesSplitSelection")->SetStyle("ComboBox");
	GetFieldAt("PruningMode")->SetStyle("ComboBox");
	GetFieldAt("TreeCost")->SetStyle("ComboBox");
	GetFieldAt("DiscretizationMethod")->SetStyle("ComboBox");
	GetFieldAt("NbMaxChildren")->SetStyle("Spinner");
	GetFieldAt("MaxDepth")->SetStyle("Spinner");
	GetFieldAt("MinInstancesPerLeaveNumber")->SetStyle("Spinner");
	GetFieldAt("NbMaxLeaves")->SetStyle("Spinner");
	GetFieldAt("GroupingMethod")->SetStyle("ComboBox");
	GetFieldAt("NbMaxInternalNodes")->SetStyle("Spinner");

	// Contraintes sur les valeurs
	cast(UIIntElement*, GetFieldAt("MinInstancesPerLeaveNumber"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("NbMaxLeaves"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("NbMaxChildren"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("NbMaxInternalNodes"))->SetMinValue(0);
	cast(UIIntElement*, GetFieldAt("MaxDepth"))->SetMinValue(0);

	// rendre visibles ou invisibles les champs du mode expert :
	GetFieldAt("NbMaxLeaves")->SetVisible(GetLearningExpertMode());
	GetFieldAt("MinInstancesPerLeaveNumber")->SetVisible(GetLearningExpertMode());
	GetFieldAt("NbMaxChildren")->SetVisible(GetLearningExpertMode());
	GetFieldAt("WithoutLeavesCosts")->SetVisible(GetLearningExpertMode());
	SetStyle("TabbedPanes");
}

DTDecisionTreeParameterView::~DTDecisionTreeParameterView() {}

DTDecisionTreeParameterView* DTDecisionTreeParameterView::Create() const
{
	return new DTDecisionTreeParameterView;
}

void DTDecisionTreeParameterView::EventUpdate(Object* object)
{
	DTDecisionTreeParameter* editedObject;

	require(object != NULL);

	editedObject = cast(DTDecisionTreeParameter*, object);

	editedObject->SetTreeCost(GetStringValueAt("TreeCost"));
	editedObject->SetMaxChildrenNumber(GetIntValueAt("NbMaxChildren"));
	editedObject->SetMaxLeavesNumber(GetIntValueAt("NbMaxLeaves"));
	editedObject->SetMinInstancesPerLeaveNumber(GetIntValueAt("MinInstancesPerLeaveNumber"));
	editedObject->SetMaxInternalNodesNumber(GetIntValueAt("NbMaxInternalNodes"));
	editedObject->SetMaxDepth(GetIntValueAt("MaxDepth"));
	editedObject->SetUnloadNonInformativeAttributes(GetBooleanValueAt("UnloadNonInformativeAttributes"));
	editedObject->SetVerboseMode(GetBooleanValueAt("VerboseMode"));
	editedObject->SetNodeVariablesSelection(GetStringValueAt("NodeVariablesSelection"));
	editedObject->SetAttributesSplitSelection(GetStringValueAt("AttributesSplitSelection"));
	editedObject->SetPruningMode(GetStringValueAt("PruningMode"));
	editedObject->SetDiscretizationMethod(GetStringValueAt("DiscretizationMethod"));
	editedObject->SetGroupingMethod(GetStringValueAt("GroupingMethod"));
}

void DTDecisionTreeParameterView::EventRefresh(Object* object)
{
	DTDecisionTreeParameter* editedObject;
	require(object != NULL);

	editedObject = cast(DTDecisionTreeParameter*, object);

	SetStringValueAt("TreeCost", editedObject->GetTreeCost());
	SetIntValueAt("NbMaxLeaves", editedObject->GetMaxLeavesNumber());
	SetIntValueAt("MinInstancesPerLeaveNumber", editedObject->GetMinInstancesPerLeaveNumber());
	SetIntValueAt("NbMaxInternalNodes", editedObject->GetMaxInternalNodesNumber());
	SetIntValueAt("MaxDepth", editedObject->GetMaxDepth());
	SetIntValueAt("NbMaxChildren", editedObject->GetMaxChildrenNumber());
	SetBooleanValueAt("UnloadNonInformativeAttributes", editedObject->GetUnloadNonInformativeAttributes());
	SetBooleanValueAt("VerboseMode", editedObject->GetVerboseMode());
	SetStringValueAt("NodeVariablesSelection", editedObject->GetNodeVariablesSelection());
	SetStringValueAt("AttributesSplitSelection", editedObject->GetAttributesSplitSelection());
	SetStringValueAt("PruningMode", editedObject->GetPruningMode());
	SetStringValueAt("DiscretizationMethod", editedObject->GetDiscretizationMethod());
	SetStringValueAt("GroupingMethod", editedObject->GetGroupingMethod());
}
