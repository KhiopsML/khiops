// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDConstructionDomain.h"

KDConstructionDomain::KDConstructionDomain()
{
	nUpdateNumber = 0;
	nAllConstructionRulesFreshness = 0;
	InitializeStandardConstructionRules();
	SelectDefaultConstructionRules();
	nSparseBlockMinSize = 0;
	bInterpretableNames = true;
	bRuleOptimization = true;
	bSparseOptimization = true;
	bImportAttributeConstructionCosts = false;
	bConstructionRegularization = true;

	// La taille minimum des blocs est fixee a 4, ce qui correspond
	// a un compromis optimal d'apres les experimentation
	nSparseBlockMinSize = 4;
}

KDConstructionDomain::~KDConstructionDomain()
{
	odConstructionRules.DeleteAll();
}

boolean KDConstructionDomain::GetInterpretableNames() const
{
	return bInterpretableNames;
}

void KDConstructionDomain::SetInterpretableNames(boolean bValue)
{
	bInterpretableNames = bValue;
}

boolean KDConstructionDomain::GetRuleOptimization() const
{
	return bRuleOptimization;
}

void KDConstructionDomain::SetRuleOptimization(boolean bValue)
{
	bRuleOptimization = bValue;
}

boolean KDConstructionDomain::GetSparseOptimization() const
{
	return bSparseOptimization;
}

void KDConstructionDomain::SetSparseOptimization(boolean bValue)
{
	bSparseOptimization = bValue;
}

int KDConstructionDomain::GetSparseBlockMinSize() const
{
	return nSparseBlockMinSize;
}

void KDConstructionDomain::SetSparseBlockMinSize(int nValue)
{
	nSparseBlockMinSize = nValue;
}

boolean KDConstructionDomain::GetImportAttributeConstructionCosts() const
{
	return bImportAttributeConstructionCosts;
}

void KDConstructionDomain::SetImportAttributeConstructionCosts(boolean bValue)
{
	bImportAttributeConstructionCosts = bValue;
}

boolean KDConstructionDomain::GetConstructionRegularization() const
{
	return bConstructionRegularization;
}

void KDConstructionDomain::SetConstructionRegularization(boolean bValue)
{
	bConstructionRegularization = bValue;
}

void KDConstructionDomain::InitializeStandardConstructionRules()
{
	// Regle sur la selection d'une partie d'un tableau d'objet
	// Regle predefinie, traitee de facon specifique
	InitializeConstructionRule("Table", new KWDRTableSelection, NULL, NULL);

	// Regles portant sur les tableaux d'objet
	// La regle portant sur une partion (pour la gestion des blocs sparse) ainsi
	// que la regle de base sont initalisees
	InitializeConstructionRule("Table", new KWDRTableCount, new KWDRTablePartitionCount, NULL);
	InitializeConstructionRule("Table", new KWDRTableMedian, new KWDRTablePartitionMedian,
				   new KWDRTableBlockMedian);
	InitializeConstructionRule("Table", new KWDRTableMean, new KWDRTablePartitionMean, new KWDRTableBlockMean);
	InitializeConstructionRule("Table", new KWDRTableMin, new KWDRTablePartitionMin, new KWDRTableBlockMin);
	InitializeConstructionRule("Table", new KWDRTableMax, new KWDRTablePartitionMax, new KWDRTableBlockMax);
	InitializeConstructionRule("Table", new KWDRTableStandardDeviation, new KWDRTablePartitionStandardDeviation,
				   new KWDRTableBlockStandardDeviation);
	InitializeConstructionRule("Table", new KWDRTableSum, new KWDRTablePartitionSum, new KWDRTableBlockSum);
	InitializeConstructionRule("Table", new KWDRTableMode, new KWDRTablePartitionMode, new KWDRTableBlockMode);
	InitializeConstructionRule("Table", new KWDRTableCountDistinct, new KWDRTablePartitionCountDistinct,
				   new KWDRTableBlockCountDistinct);

	// Regles portant sur les objets
	InitializeConstructionRule("Entity", new KWDRGetSymbolValue, NULL, new KWDRGetSymbolValueBlock);
	InitializeConstructionRule("Entity", new KWDRGetContinuousValue, NULL, new KWDRGetContinuousValueBlock);

	// Regles portant sur les date
	InitializeConstructionRule("Date", new KWDRDay, NULL, NULL);
	InitializeConstructionRule("Date", new KWDRWeekDay, NULL, NULL);
	InitializeConstructionRule("Date", new KWDRYearDay, NULL, NULL);
	InitializeConstructionRule("Date", new KWDRDecimalYear, NULL, NULL);

	// Regles portant sur les time
	InitializeConstructionRule("Time", new KWDRDecimalTime, NULL, NULL);

	// Regles portant sur les timestamp
	InitializeConstructionRule("Timestamp", new KWDRGetDate, NULL, NULL);
	InitializeConstructionRule("Timestamp", new KWDRGetTime, NULL, NULL);
	InitializeConstructionRule("Timestamp", new KWDRDecimalWeekDay, NULL, NULL);
	InitializeConstructionRule("Timestamp", new KWDRDecimalYearTS, NULL, NULL);

	// Regles portant sur les timestampTZ
	InitializeConstructionRule("TimestampTZ", new KWDRLocalTimestamp, NULL, NULL);
}

void KDConstructionDomain::SelectDefaultConstructionRules()
{
	int i;
	KDConstructionRule* constructionRule;

	// Parcours des regles de construction pour determiner si la regle de selection est presente et utilisee
	for (i = 0; i < GetConstructionRuleNumber(); i++)
	{
		constructionRule = GetConstructionRuleAt(i);

		// Par defaut, on ne selection pas les regles de construction temporelle
		constructionRule->SetPriority(0);
		constructionRule->SetUsed(true);
		if (constructionRule->GetFamilyName() == "Time" or constructionRule->GetFamilyName() == "Date" or
		    constructionRule->GetFamilyName() == "Timestamp" or
		    constructionRule->GetFamilyName() == "TimestampTZ")
		{
			constructionRule->SetPriority(1);
			constructionRule->SetUsed(false);
		}
	}

	// On indique une mise a jour
	nUpdateNumber++;
}

KDConstructionRule* KDConstructionDomain::LookupConstructionRule(const ALString& sName) const
{
	return cast(KDConstructionRule*, odConstructionRules.Lookup(sName));
}

boolean KDConstructionDomain::InsertConstructionRule(KDConstructionRule* constructionRule)
{
	require(constructionRule != NULL);
	if (LookupConstructionRule(constructionRule->GetName()) == NULL)
	{
		odConstructionRules.SetAt(constructionRule->GetName(), constructionRule);
		nUpdateNumber++;
		return true;
	}
	else
		return false;
}

boolean KDConstructionDomain::DeleteConstructionRule(const ALString& sName)
{
	KDConstructionRule* constructionRuleToDelete;

	constructionRuleToDelete = LookupConstructionRule(sName);
	if (odConstructionRules.RemoveKey(sName))
	{
		check(constructionRuleToDelete);
		delete constructionRuleToDelete;
		nUpdateNumber++;
		return true;
	}
	else
		return false;
}

int KDConstructionDomain::GetConstructionRuleNumber() const
{
	return AllConstructionRules()->GetSize();
}

KDConstructionRule* KDConstructionDomain::GetConstructionRuleAt(int i) const
{
	require(0 <= i and i < GetConstructionRuleNumber());
	return cast(KDConstructionRule*, AllConstructionRules()->GetAt(i));
}

boolean KDConstructionDomain::IsSelectionRuleUsed() const
{
	boolean bIsSelectionRuleUsed;
	int i;
	KDConstructionRule* constructionRule;

	// Parcours des regles de construction pour determiner si la regle de selection est presente et utilisee
	bIsSelectionRuleUsed = false;
	for (i = 0; i < GetConstructionRuleNumber(); i++)
	{
		constructionRule = GetConstructionRuleAt(i);
		if (constructionRule->IsSelectionRule() and constructionRule->GetUsed())
			bIsSelectionRuleUsed = true;
	}
	return bIsSelectionRuleUsed;
}

void KDConstructionDomain::DeleteAllConstructionRules()
{
	oaConstructionRules.RemoveAll();
	odConstructionRules.DeleteAll();
	nUpdateNumber++;
}

ObjectArray* KDConstructionDomain::GetConstructionRules()
{
	return AllConstructionRules();
}

const ALString KDConstructionDomain::GetClassLabel() const
{
	return "Variable construction parameters";
}

const ALString KDConstructionDomain::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

KDConstructionRule* KDConstructionDomain::InitializeConstructionRule(const ALString& sFamilyName,
								     const KWDerivationRule* derivationRule,
								     const KWDRTablePartitionStats* partitionStatsRule,
								     const KWDRValueBlockRule* valueBlockRule)
{
	KDConstructionRule* constructionRule;

	require(derivationRule != NULL);
	require(LookupConstructionRule(derivationRule->GetName()) == NULL);

	// Initialisation de la regle de construction
	constructionRule = new KDConstructionRule;
	constructionRule->InitializeFromDerivationRule(sFamilyName, derivationRule);

	// Parametrage des regles supplementaires
	constructionRule->SetPartitionStatsRule(partitionStatsRule);
	constructionRule->SetValueBlockRule(valueBlockRule);

	// Memorisation dans le dictionnaire de regles
	assert(LookupConstructionRule(constructionRule->GetName()) == NULL);
	InsertConstructionRule(constructionRule);
	assert(LookupConstructionRule(derivationRule->GetName()) == constructionRule);
	return constructionRule;
}

ObjectArray* KDConstructionDomain::AllConstructionRules() const
{
	if (nAllConstructionRulesFreshness < nUpdateNumber or oaConstructionRules.GetSize() == 0)
	{
		odConstructionRules.ExportObjectArray(&oaConstructionRules);
		oaConstructionRules.SetCompareFunction(KWConstructionRuleCompare);
		oaConstructionRules.Sort();
		nAllConstructionRulesFreshness = nUpdateNumber;
	}
	return &oaConstructionRules;
}
