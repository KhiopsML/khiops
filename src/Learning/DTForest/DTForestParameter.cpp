// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTForestParameter.h"
#include "DTGlobalTag.h"
#include "DTConfig.h"

DTForestParameter::DTForestParameter()
{
	// Valeurs par defaut
	//	nTreeNumber = 10;
	nOptimizationLoopNumber = 10;
	cInstancePercentage = 0.0;
	cKeptAttributePercentage = 0.0;
	bRecodeRFDictionary = true;
	drawingType = DTDecisionTree::DrawingType::NoReplacement;
	// sInitRFOptimisation = DTGlobalTag::RF_OPTIMIZATION_ALL_TREES;
	sWeightedClassifier = "UNIFORM";
	sTreesVariablesSelection = DTGlobalTag::RANK_WITH_REPLACEMENT_LABEL;
	// sTreesVariablesSelection = DTGlobalTag::LEVEL_SAMPLING_WITH_REPLACEMENT_LABEL;
	bWriteDetailedStatistics = false;
	//	sHeuristicCreation = DTForestParameter::HEURISTIC_NODRAW_LABEL;
	nVariableNumberMin = MIN_VARIABLE_2_BUILTREE;
	sDiscretizationTargetMethod = DISCRETIZATION_MODL; // DISCRETIZATION_EQUAL_FREQUENCY
	nMaxIntervalsNumberForTarget = 2;
}

DTForestParameter::~DTForestParameter() {}

int DTForestParameter::GetOptimizationLoopNumber() const
{
	return nOptimizationLoopNumber;
}

void DTForestParameter::SetOptimizationLoopNumber(int nValue)
{
	nOptimizationLoopNumber = nValue;
}

int DTForestParameter::GetVariableNumberMin() const
{
	return nVariableNumberMin;
}

void DTForestParameter::SetVariableNumberMin(int nNumber)
{
	nVariableNumberMin = nNumber;
}

boolean DTForestParameter::IsWriteDetailedStatistics() const
{
	return bWriteDetailedStatistics;
}
void DTForestParameter::SetWriteDetailedStatistics(boolean bValue)
{
	bWriteDetailedStatistics = bValue;
}
Continuous DTForestParameter::GetInstancePercentage() const
{
	return cInstancePercentage;
}

void DTForestParameter::SetInstancePercentage(Continuous cValue)
{
	cInstancePercentage = cValue;
}

Continuous DTForestParameter::GetAttributePercentage() const
{
	return cKeptAttributePercentage;
}

void DTForestParameter::SetAttributePercentage(Continuous cValue)
{
	cKeptAttributePercentage = cValue;
}

void DTForestParameter::SetDrawingType(DTDecisionTree::DrawingType d)
{
	drawingType = d;
}

void DTForestParameter::SetRecodeRFDictionary(boolean bValue)
{
	bRecodeRFDictionary = bValue;
}

boolean DTForestParameter::GetRecodeRFDictionary() const
{
	return bRecodeRFDictionary;
}

void DTForestParameter::SetWeightedClassifier(ALString sValue)
{
	sWeightedClassifier = sValue;
}

ALString DTForestParameter::GetWeightedClassifier() const
{
	return sWeightedClassifier;
}
void DTForestParameter::SetDiscretizationTargetMethod(ALString sValue)
{
	sDiscretizationTargetMethod = sValue;
}

ALString DTForestParameter::GetDiscretizationTargetMethod() const
{
	return sDiscretizationTargetMethod;
}
void DTForestParameter::SetMaxIntervalsNumberForTarget(int i)
{
	nMaxIntervalsNumberForTarget = i;
}

int DTForestParameter::GetMaxIntervalsNumberForTarget() const
{
	return nMaxIntervalsNumberForTarget;
}

void DTForestParameter::SetInitRFOptimisation(ALString sValue)
{
	sInitRFOptimisation = sValue;
}
ALString DTForestParameter::GetInitRFOptimisation() const
{
	return sInitRFOptimisation;
}

DTForestParameter* DTForestParameter::Clone() const
{
	DTForestParameter* copyParam;

	copyParam = new DTForestParameter;
	copyParam->CopyFrom(this);

	return copyParam;
}

void DTForestParameter::CopyFrom(const DTForestParameter* dtParam)
{
	assert(dtParam != NULL);

	//	nTreeNumber = dtParam->nTreeNumber ;
	sTreesVariablesSelection = dtParam->sTreesVariablesSelection;
	cInstancePercentage = dtParam->cInstancePercentage;
	cKeptAttributePercentage = dtParam->cKeptAttributePercentage;
	bRecodeRFDictionary = dtParam->bRecodeRFDictionary;
	drawingType = dtParam->drawingType;
	nOptimizationLoopNumber = dtParam->nOptimizationLoopNumber;
	sInitRFOptimisation = dtParam->sInitRFOptimisation;
	sWeightedClassifier = dtParam->sWeightedClassifier;
	bWriteDetailedStatistics = dtParam->bWriteDetailedStatistics;
	nVariableNumberMin = dtParam->nVariableNumberMin;
	// sHeuristicCreation = dtParam->sHeuristicCreation;
	pDecisionTreeParameter.CopyFrom(&dtParam->pDecisionTreeParameter);
	sDiscretizationTargetMethod = dtParam->sDiscretizationTargetMethod;
	nMaxIntervalsNumberForTarget = dtParam->nMaxIntervalsNumberForTarget;
}

void DTForestParameter::SetTreesVariablesSelection(ALString sValue)
{
	sTreesVariablesSelection = sValue;
}
ALString DTForestParameter::GetTreesVariablesSelection() const
{
	return sTreesVariablesSelection;
}

const ALString DTForestParameter::GetDrawingTypeLabel() const
{
	switch (drawingType)
	{
	case DTDecisionTree::DrawingType::NoReplacement:
		return DRAWING_TYPE_NO_REPLACEMENT_LABEL;

	case DTDecisionTree::DrawingType::UseOutOfBag:
		return DRAWING_TYPE_USE_OUT_OF_BAG_LABEL;

	case DTDecisionTree::DrawingType::WithReplacementAdaBoost:
		return DRAWING_TYPE_ADABOOST_REPLACEMENT_LABEL;

	default:
		return "undefined";
	}
}

void DTForestParameter::SetDrawingType(const ALString drawingTypeLabel)
{
	if (drawingTypeLabel == DRAWING_TYPE_USE_OUT_OF_BAG_LABEL)
		drawingType = DTDecisionTree::DrawingType::UseOutOfBag;
	else if (drawingTypeLabel == DRAWING_TYPE_ADABOOST_REPLACEMENT_LABEL)
		drawingType = DTDecisionTree::DrawingType::WithReplacementAdaBoost;
	else
		drawingType = DTDecisionTree::DrawingType::NoReplacement;
}

void DTForestParameter::WriteReport(ostream& ost)
{
	// Contraintes utilisees lors de la recherche de l'arbre optimal
	ost << "\n"
	    << "Model constraints"
	    << "\n";
	ost << "\nNodes variables selection"
	    << "\t" << pDecisionTreeParameter.GetNodeVariablesSelection();
	ost << "\nAttributes split selection"
	    << "\t" << pDecisionTreeParameter.GetAttributesSplitSelection();
	ost << "\nPruning mode"
	    << "\t" << pDecisionTreeParameter.GetPruningMode();

	ost << "\nTrees criterion"
	    << "\t" << pDecisionTreeParameter.GetTreeCost();
	ost << "\nUnload non informative variables"
	    << "\t" << (pDecisionTreeParameter.GetUnloadNonInformativeAttributes() ? "yes" : "no");

	ost << "\nMax of children"
	    << "\t" << pDecisionTreeParameter.GetMaxChildrenNumber();
	ost << "\nMax of depth"
	    << "\t" << pDecisionTreeParameter.GetMaxDepth();
	ost << "\nMaximum number of internal nodes "
	    << "\t" << pDecisionTreeParameter.GetMaxInternalNodesNumber();
	ost << "\nMaximum number of leaves (before the last split)"
	    << "\t" << pDecisionTreeParameter.GetMaxLeavesNumber();
	ost << "\nMinimum number of instances per leave"
	    << "\t" << pDecisionTreeParameter.GetMinInstancesPerLeaveNumber();
	ost << "\n\n";

	// Contraintes utilisees lors de la recherche de l'arbre optimal
	ost << "Random Forest Parameters"
	    << "\n";

	//	ost << "\nNumber of decision trees" << "\t" << GetTreesNumber();
	ost << "\nInstances Percentage"
	    << "\t" << GetInstancePercentage();
	ost << "\nAttributes Percentage"
	    << "\t" << GetAttributePercentage();
	ost << "\nOptimisation Init type"
	    << "\t" << GetInitRFOptimisation();
	ost << "\nOptimisation loop number"
	    << "\t" << GetOptimizationLoopNumber();
	ost << "\nTree variables selection"
	    << "\t" << GetTreesVariablesSelection();
}

const ALString DTForestParameter::DRAWING_TYPE_NO_REPLACEMENT_LABEL = "No draw, use all data";
const ALString DTForestParameter::DRAWING_TYPE_USE_OUT_OF_BAG_LABEL = "Random draw, use out-of-bag data";
const ALString DTForestParameter::DRAWING_TYPE_ADABOOST_REPLACEMENT_LABEL = "AdaBoost draw";
const ALString DTForestParameter::HEURISTIC_NODRAW_LABEL = "No draw";
const ALString DTForestParameter::DISCRETIZATION_EQUAL_FREQUENCY = "EFDiscretization";
const ALString DTForestParameter::DISCRETIZATION_BINARY_EQUAL_FREQUENCY = "EFBinDiscretization";
const ALString DTForestParameter::DISCRETIZATION_MODL = "MODLDiscretization";

////////////////  classe PLShared_ForestParameter

PLShared_ForestParameter::PLShared_ForestParameter()
{
	shared_DecisionTreeParameter = new PLShared_DecisionTreeParameter;
}

PLShared_ForestParameter::~PLShared_ForestParameter()
{
	delete shared_DecisionTreeParameter;
}

void PLShared_ForestParameter::DeserializeObject(PLSerializer* serializer, Object* object) const
{
	DTForestParameter* param;
	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	require(object != NULL);

	param = cast(DTForestParameter*, object);

	param->sTreesVariablesSelection = serializer->GetString();
	param->cInstancePercentage = serializer->GetDouble();
	param->cKeptAttributePercentage = serializer->GetDouble();
	param->bRecodeRFDictionary = serializer->GetBoolean();
	param->nVariableNumberMin = serializer->GetInt();
	param->sInitRFOptimisation = serializer->GetString();
	param->nOptimizationLoopNumber = serializer->GetInt();
	param->sWeightedClassifier = serializer->GetString();
	param->bWriteDetailedStatistics = serializer->GetBoolean();
	shared_DecisionTreeParameter->DeserializeObject(serializer, &param->pDecisionTreeParameter);
	param->sDiscretizationTargetMethod = serializer->GetString();
	param->nMaxIntervalsNumberForTarget = serializer->GetInt();
}

void PLShared_ForestParameter::SerializeObject(PLSerializer* serializer, const Object* object) const
{
	DTForestParameter* param;
	require(serializer != NULL);
	require(serializer->IsOpenForWrite());
	require(object != NULL);

	param = cast(DTForestParameter*, object);

	serializer->PutString(param->sTreesVariablesSelection);
	serializer->PutDouble(param->cInstancePercentage);
	serializer->PutDouble(param->cKeptAttributePercentage);
	serializer->PutBoolean(param->bRecodeRFDictionary);
	serializer->PutInt(param->nVariableNumberMin);
	serializer->PutString(param->sInitRFOptimisation);
	serializer->PutInt(param->nOptimizationLoopNumber);
	serializer->PutString(param->sWeightedClassifier);
	serializer->PutBoolean(param->bWriteDetailedStatistics);
	shared_DecisionTreeParameter->SerializeObject(serializer, &param->pDecisionTreeParameter);
	serializer->PutString(param->sDiscretizationTargetMethod);
	serializer->PutInt(param->nMaxIntervalsNumberForTarget);
}

Object* PLShared_ForestParameter::Create() const
{
	return new DTForestParameter;
}

void PLShared_ForestParameter::SetForestParameter(DTForestParameter* r)
{
	require(r != NULL);
	SetObject(r);
}

DTForestParameter* PLShared_ForestParameter::GetForestParameter() const
{
	return cast(DTForestParameter*, GetObject());
}
