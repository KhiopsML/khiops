// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDConstructionRule.h"

KDConstructionRule::KDConstructionRule()
{
	rule = NULL;
	partitionRule = NULL;
	blockRule = NULL;
	bUsed = true;
	nRecursionLevel = 1;
	nPriority = 0;
	bIsSelectionRule = false;
}

KDConstructionRule::~KDConstructionRule()
{
	if (rule != NULL)
		delete rule;
	if (partitionRule != NULL)
		delete partitionRule;
	if (blockRule != NULL)
		delete blockRule;
}

void KDConstructionRule::SetName(const ALString& sValue)
{
	sName = sValue;
}

const ALString& KDConstructionRule::GetName() const
{
	return sName;
}

void KDConstructionRule::SetPriority(int nValue)
{
	require(nValue >= 0);
	nPriority = nValue;
}

int KDConstructionRule::GetPriority() const
{
	return nPriority;
}

void KDConstructionRule::SetFamilyName(const ALString& sValue)
{
	sFamilyName = sValue;
}

const ALString& KDConstructionRule::GetFamilyName() const
{
	return sFamilyName;
}

void KDConstructionRule::SetLabel(const ALString& sValue)
{
	sLabel = sValue;
}

const ALString KDConstructionRule::GetLabel() const
{
	return sLabel;
}

boolean KDConstructionRule::IsSelectionRule() const
{
	debug(KWDRTableSelection selectionRule);
	debug(ensure(bIsSelectionRule == (rule != NULL and rule->GetName() == selectionRule.GetName())));
	return bIsSelectionRule;
}

void KDConstructionRule::SetDerivationRule(const KWDerivationRule* derivationRule)
{
	KWDRTableSelection selectionRule;

	if (rule != NULL)
		delete rule;
	rule = derivationRule;

	// Gestion de l'indicateur de regle de selection
	bIsSelectionRule = false;
	if (rule != NULL and rule->GetName() == selectionRule.GetName())
		bIsSelectionRule = true;
}

const KWDerivationRule* KDConstructionRule::GetDerivationRule() const
{
	return rule;
}

void KDConstructionRule::SetPartitionStatsRule(const KWDRTablePartitionStats* partitionStatsRule)
{
	if (partitionRule != NULL)
		delete partitionRule;
	partitionRule = partitionStatsRule;
}

const KWDRTablePartitionStats* KDConstructionRule::GetPartitionStatsRule() const
{
	ensure(partitionRule == NULL or
	       partitionRule->GetTableStatsRule()->GetName() == GetDerivationRule()->GetName());
	return partitionRule;
}

void KDConstructionRule::SetValueBlockRule(const KWDRValueBlockRule* valueBlockRule)
{
	if (blockRule != NULL)
		delete blockRule;
	blockRule = valueBlockRule;
}

const KWDRValueBlockRule* KDConstructionRule::GetValueBlockRule() const
{
	ensure(blockRule == NULL or blockRule->GetOperandNumber() == GetDerivationRule()->GetOperandNumber());
	ensure(blockRule == NULL or
	       blockRule->GetFirstOperand()->GetType() == GetDerivationRule()->GetFirstOperand()->GetType());
	ensure(blockRule == NULL or KWType::GetBlockBaseType(blockRule->GetSecondOperand()->GetType()) ==
					GetDerivationRule()->GetSecondOperand()->GetType());
	ensure(blockRule == NULL or GetDerivationRule()->GetFirstOperand()->GetType() != KWType::ObjectArray or
	       cast(KWDRTableBlockStats*, blockRule)->GetTableStatsRule()->GetName() == GetDerivationRule()->GetName());
	return blockRule;
}

int KDConstructionRule::GetType() const
{
	if (rule == NULL)
		return KWType::Unknown;
	else
		return rule->GetType();
}

int KDConstructionRule::GetOperandNumber() const
{
	if (rule == NULL)
		return 0;
	else
		return rule->GetOperandNumber();
}

const KWDerivationRuleOperand* KDConstructionRule::GetOperandAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetOperandNumber());
	assert(rule != NULL);
	return rule->GetOperandAt(nIndex);
}

void KDConstructionRule::InitializeFromDerivationRule(const ALString& sFamily, const KWDerivationRule* derivationRule)
{
	require(derivationRule == NULL or derivationRule != rule);

	// Nettoyage initial
	sFamilyName = "";
	sName = "";
	sLabel = "";
	if (rule != NULL)
		delete rule;
	rule = NULL;
	if (partitionRule != NULL)
		delete partitionRule;
	partitionRule = NULL;
	if (blockRule != NULL)
		delete blockRule;
	blockRule = NULL;
	sClassName = "";
	nRecursionLevel = 1;

	// Initialisation de la regle de construction
	if (derivationRule != NULL)
	{
		SetName(derivationRule->GetName());
		SetFamilyName(sFamily);
		SetLabel(derivationRule->GetLabel());
		SetDerivationRule(derivationRule);
	}
}

void KDConstructionRule::SetUsed(boolean bValue)
{
	bUsed = bValue;
}

boolean KDConstructionRule::GetUsed() const
{
	return bUsed;
}

void KDConstructionRule::SetClassName(const ALString& sValue)
{
	sClassName = sValue;
}

const ALString& KDConstructionRule::GetClassName() const
{
	return sClassName;
}

void KDConstructionRule::SetRecursionLevel(int nValue)
{
	require(nRecursionLevel >= 1);
	nRecursionLevel = nValue;
}

int KDConstructionRule::GetRecursionLevel() const
{
	return nRecursionLevel;
}

KDConstructionRule* KDConstructionRule::Clone() const
{
	KDConstructionRule* constructionRuleClone;

	// Creation et copie de la regle courante
	constructionRuleClone = new KDConstructionRule;
	constructionRuleClone->SetPriority(GetPriority());
	constructionRuleClone->SetFamilyName(GetFamilyName());
	constructionRuleClone->SetName(GetName());
	constructionRuleClone->SetLabel(GetLabel());
	if (GetDerivationRule() != NULL)
		constructionRuleClone->SetDerivationRule(GetDerivationRule()->Clone());
	if (GetPartitionStatsRule() != NULL)
		constructionRuleClone->SetPartitionStatsRule(
		    cast(const KWDRTablePartitionStats*, GetPartitionStatsRule()->Clone()));
	if (GetValueBlockRule() != NULL)
		constructionRuleClone->SetValueBlockRule(cast(const KWDRValueBlockRule*, GetValueBlockRule()->Clone()));
	constructionRuleClone->SetUsed(GetUsed());
	constructionRuleClone->SetClassName(GetClassName());
	constructionRuleClone->SetRecursionLevel(GetRecursionLevel());
	return constructionRuleClone;
}

boolean KDConstructionRule::Check() const
{
	boolean bOk = true;
	int i;

	bOk = bOk and rule != NULL;
	bOk = bOk and rule->Check();

	// Seules les regles simples sont utilisables pour la construction de variable
	if (bOk)
	{
		bOk = bOk and not rule->GetVariableOperandNumber();
		bOk = bOk and KWType::IsData(rule->GetType());
		bOk = bOk and rule->GetOperandNumber() > 0;

		// Les operandes doivent etre de type data
		for (i = 0; i < GetOperandNumber(); i++)
			bOk = bOk and KWType::IsData(rule->GetOperandAt(i)->GetType());
	}
	return bOk;
}

void KDConstructionRule::Write(ostream& ost) const
{
	ost << GetClassLabel() << " " << GetFamilyName() << " " << GetName();
}

const ALString KDConstructionRule::GetObjectLabel() const
{
	return GetName();
}

const ALString KDConstructionRule::GetClassLabel() const
{
	return "Construction rule";
}

int KWConstructionRuleCompare(const void* first, const void* second)
{
	KDConstructionRule* aFirst;
	KDConstructionRule* aSecond;
	int nResult;

	aFirst = cast(KDConstructionRule*, *(Object**)first);
	aSecond = cast(KDConstructionRule*, *(Object**)second);
	nResult = aFirst->GetPriority() - aSecond->GetPriority();
	if (nResult == 0)
		nResult = aFirst->GetFamilyName().Compare(aSecond->GetFamilyName());
	if (nResult == 0)
		nResult = aFirst->GetName().Compare(aSecond->GetName());
	return nResult;
}

int KWConstructionRuleCompareRecursionLevel(const void* first, const void* second)
{
	KDConstructionRule* aFirst;
	KDConstructionRule* aSecond;
	int nResult;

	aFirst = cast(KDConstructionRule*, *(Object**)first);
	aSecond = cast(KDConstructionRule*, *(Object**)second);
	nResult = aFirst->GetRecursionLevel() - aSecond->GetRecursionLevel();
	if (nResult == 0)
		nResult = aFirst->GetOperandNumber() - aSecond->GetOperandNumber();
	if (nResult == 0)
	{
		nResult = aFirst->GetPriority() - aSecond->GetPriority();
		if (nResult == 0)
			nResult = aFirst->GetFamilyName().Compare(aSecond->GetFamilyName());
		if (nResult == 0)
			nResult = aFirst->GetName().Compare(aSecond->GetName());
	}
	return nResult;
}
