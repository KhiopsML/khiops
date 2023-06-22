// Copyright (c) 2023 Orange. All rights reserved.
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
	//	sHeuristicCreation = DTForestParameter::Heuristic_NODRAW_LABEL;
	nVariableNumberMin = MIN_VARIABLE_2_BUILTREE;
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
const ALString DTForestParameter::Heuristic_NODRAW_LABEL = "No draw";