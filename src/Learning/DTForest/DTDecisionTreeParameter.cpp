// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTDecisionTreeParameter.h"
#include "DTGlobalTag.h"

DTDecisionTreeParameter::DTDecisionTreeParameter()
{
	// Valeurs par defaut
	nMaxDepth = 0;
	nMaxInternalNodesNumber = 0;
	nMaxLeavesNumber = 0;
	nMinInstancesPerLeaveNumber = 8;
	nMaxChildren = 0;
	sTreeCost = DTGlobalTag::REC_NEW_BIN_LABEL;
	bUnloadNonInformativeAttributes = false;
	sPruningMode = DTGlobalTag::PRE_PRUNING_LABEL; // POST_PRUNING_LABEL;
	sNodeVariablesSelection = DTGlobalTag::IDENTICAL_LABEL;
	// sAttributesSplitSelection = DTGlobalTag::BEST_TREE_COST_LABEL;
	sAttributesSplitSelection = DTGlobalTag::RANDOM_LEVEL_LABEL;
	sDiscretizationMethod = DTGlobalTag::MODL_LABEL;
	sGroupingMethod = DTGlobalTag::MODL_LABEL;
	bVerboseMode = false;
}

DTDecisionTreeParameter::~DTDecisionTreeParameter() {}

const ALString& DTDecisionTreeParameter::GetTreeCost() const
{
	return sTreeCost;
}

void DTDecisionTreeParameter::SetTreeCost(const ALString& sValue)
{
	sTreeCost = sValue;
}

int DTDecisionTreeParameter::GetMaxChildrenNumber() const
{
	return nMaxChildren;
}

void DTDecisionTreeParameter::SetMaxChildrenNumber(int nNumber)
{
	nMaxChildren = nNumber;
}

int DTDecisionTreeParameter::GetMaxInternalNodesNumber() const
{
	return nMaxInternalNodesNumber;
}

void DTDecisionTreeParameter::SetMaxInternalNodesNumber(int nNumber)
{
	nMaxInternalNodesNumber = nNumber;
}

int DTDecisionTreeParameter::GetMaxLeavesNumber() const
{
	return nMaxLeavesNumber;
}

void DTDecisionTreeParameter::SetMaxLeavesNumber(int nNumber)
{
	nMaxLeavesNumber = nNumber;
}
int DTDecisionTreeParameter::GetMinInstancesPerLeaveNumber() const
{
	return nMinInstancesPerLeaveNumber;
}

void DTDecisionTreeParameter::SetMinInstancesPerLeaveNumber(int nNumber)
{
	nMinInstancesPerLeaveNumber = nNumber;
}

int DTDecisionTreeParameter::GetMaxDepth() const
{
	return nMaxDepth;
}

void DTDecisionTreeParameter::SetMaxDepth(int nNumber)
{
	nMaxDepth = nNumber;
}

boolean DTDecisionTreeParameter::GetVerboseMode() const
{
	return bVerboseMode;
}

void DTDecisionTreeParameter::SetVerboseMode(boolean b)
{
	bVerboseMode = b;
}

DTDecisionTreeParameter* DTDecisionTreeParameter::Clone() const
{
	DTDecisionTreeParameter* copyParam;

	copyParam = new DTDecisionTreeParameter;
	copyParam->CopyFrom(this);

	return copyParam;
}

void DTDecisionTreeParameter::CopyFrom(const DTDecisionTreeParameter* dtParam)
{
	require(dtParam != NULL);

	// Recopie des parametres
	bUnloadNonInformativeAttributes = dtParam->bUnloadNonInformativeAttributes;
	nMaxDepth = dtParam->nMaxDepth;
	bVerboseMode = dtParam->bVerboseMode;
	nMaxInternalNodesNumber = dtParam->nMaxInternalNodesNumber;
	nMaxLeavesNumber = dtParam->nMaxLeavesNumber;
	nMinInstancesPerLeaveNumber = dtParam->nMinInstancesPerLeaveNumber;
	nMaxChildren = dtParam->nMaxChildren;
	sTreeCost = dtParam->sTreeCost;
	sNodeVariablesSelection = dtParam->sNodeVariablesSelection;
	sAttributesSplitSelection = dtParam->sAttributesSplitSelection;
	sPruningMode = dtParam->sPruningMode;
	sDiscretizationMethod = dtParam->sDiscretizationMethod;
	sGroupingMethod = dtParam->sGroupingMethod;
}

boolean DTDecisionTreeParameter::GetUnloadNonInformativeAttributes() const
{
	return bUnloadNonInformativeAttributes;
}
void DTDecisionTreeParameter::SetUnloadNonInformativeAttributes(boolean bUnLoad)
{
	bUnloadNonInformativeAttributes = bUnLoad;
}

ALString DTDecisionTreeParameter::GetNodeVariablesSelection() const
{
	return sNodeVariablesSelection;
}
void DTDecisionTreeParameter::SetNodeVariablesSelection(ALString s)
{
	sNodeVariablesSelection = s;
}
ALString DTDecisionTreeParameter::GetAttributesSplitSelection() const
{
	return sAttributesSplitSelection;
}
void DTDecisionTreeParameter::SetAttributesSplitSelection(ALString s)
{
	sAttributesSplitSelection = s;
}

ALString DTDecisionTreeParameter::GetDiscretizationMethod() const
{
	return sDiscretizationMethod;
}
void DTDecisionTreeParameter::SetDiscretizationMethod(ALString s)
{
	sDiscretizationMethod = s;
}

ALString DTDecisionTreeParameter::GetGroupingMethod() const
{
	return sGroupingMethod;
}
void DTDecisionTreeParameter::SetGroupingMethod(ALString s)
{
	sGroupingMethod = s;
}

ALString DTDecisionTreeParameter::GetPruningMode() const
{
	return sPruningMode;
}
void DTDecisionTreeParameter::SetPruningMode(ALString s)
{
	sPruningMode = s;
}

void DTDecisionTreeParameter::Write(ostream& ost)
{
	// Contraintes utilisees lors de la recherche de l'arbre optimal
	ost << "\n"
	    << "Model constraints"
	    << "\n";
	ost << "\nNodes variables selection"
	    << "\t" << GetNodeVariablesSelection();
	ost << "\nAttributes split selection"
	    << "\t" << GetAttributesSplitSelection();
	ost << "\nPruning mode"
	    << "\t" << GetPruningMode();
	ost << "\nTree criterion"
	    << "\t" << GetTreeCost();
	ost << "\nUnload non informative variables"
	    << "\t" << (GetUnloadNonInformativeAttributes() ? "yes" : "no");
	ost << "\nMax of Children"
	    << "\t" << GetMaxChildrenNumber();
	ost << "\nMax of depth"
	    << "\t" << GetMaxDepth();
	ost << "\nMaximum number of internal nodes "
	    << "\t" << GetMaxInternalNodesNumber();
	ost << "\nMaximum number of leaves (before the last split)"
	    << "\t" << GetMaxLeavesNumber();
	ost << "\nMinimum number of instances per leave"
	    << "\t" << GetMinInstancesPerLeaveNumber();
	ost << "\n\n";
}

/////////////  classe PLShared_DecisionTreeParameter

PLShared_DecisionTreeParameter::PLShared_DecisionTreeParameter() {}

PLShared_DecisionTreeParameter::~PLShared_DecisionTreeParameter() {}

void PLShared_DecisionTreeParameter::DeserializeObject(PLSerializer* serializer, Object* object) const
{
	DTDecisionTreeParameter* param;
	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	require(object != NULL);

	param = cast(DTDecisionTreeParameter*, object);

	param->sTreeCost = serializer->GetString();
	param->nMaxChildren = serializer->GetInt();
	param->nMaxDepth = serializer->GetInt();
	param->nMaxLeavesNumber = serializer->GetInt();
	param->nMaxInternalNodesNumber = serializer->GetInt();
	param->nMinInstancesPerLeaveNumber = serializer->GetInt();
	param->bUnloadNonInformativeAttributes = serializer->GetBoolean();
	param->sNodeVariablesSelection = serializer->GetString();
	param->sAttributesSplitSelection = serializer->GetString();
	param->sPruningMode = serializer->GetString();
	param->sDiscretizationMethod = serializer->GetString();
	param->sGroupingMethod = serializer->GetString();
	param->bVerboseMode = serializer->GetBoolean();
}

void PLShared_DecisionTreeParameter::SerializeObject(PLSerializer* serializer, const Object* object) const
{
	DTDecisionTreeParameter* param;
	require(serializer != NULL);
	require(serializer->IsOpenForWrite());
	require(object != NULL);

	param = cast(DTDecisionTreeParameter*, object);

	serializer->PutString(param->sTreeCost);
	serializer->PutInt(param->nMaxChildren);
	serializer->PutInt(param->nMaxDepth);
	serializer->PutInt(param->nMaxLeavesNumber);
	serializer->PutInt(param->nMaxInternalNodesNumber);
	serializer->PutInt(param->nMinInstancesPerLeaveNumber);
	serializer->PutBoolean(param->bUnloadNonInformativeAttributes);
	serializer->PutString(param->sNodeVariablesSelection);
	serializer->PutString(param->sAttributesSplitSelection);
	serializer->PutString(param->sPruningMode);
	serializer->PutString(param->sDiscretizationMethod);
	serializer->PutString(param->sGroupingMethod);
	serializer->PutBoolean(param->bVerboseMode);
}

Object* PLShared_DecisionTreeParameter::Create() const
{
	return new DTDecisionTreeParameter;
}

void PLShared_DecisionTreeParameter::SetDecisionTreeParameter(DTDecisionTreeParameter* r)
{
	require(r != NULL);
	SetObject(r);
}

DTDecisionTreeParameter* PLShared_DecisionTreeParameter::GetDecisionTreeParameter() const
{
	return cast(DTDecisionTreeParameter*, GetObject());
}
