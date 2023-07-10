// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDConstructedRule.h"

//////////////////////////////////////////////////////////////////////////
// Classe KDConstructedRule

KDConstructedRule::KDConstructedRule()
{
	int i;
	constructionRule = NULL;
	dCost = 0;
	nRandomIndex = 0;
	for (i = 0; i < sizeof(cOperandOrigins); i++)
		cOperandOrigins[i] = None;
}

KDConstructedRule::~KDConstructedRule()
{
	int i;

	// Nettoyage des operandes en cours
	if (constructionRule != NULL)
	{
		for (i = 0; i < GetOperandNumber(); i++)
		{
			if (GetOperandOriginAt(i) == Rule)
				delete oaConstructedOperands.GetAt(i);
		}
	}
}

void KDConstructedRule::SetConstructionRule(const KDConstructionRule* rule)
{
	int i;

	// Nettoyage des operandes en cours
	if (constructionRule != NULL)
	{
		for (i = 0; i < GetOperandNumber(); i++)
		{
			if (GetOperandOriginAt(i) == Rule)
				delete oaConstructedOperands.GetAt(i);
			cOperandOrigins[i] = None;
		}
		oaConstructedOperands.SetSize(0);
	}

	// Initialisation du nouveau tableau d'operandes
	constructionRule = rule;
	if (constructionRule != NULL)
	{
		assert(constructionRule->GetOperandNumber() <= sizeof(cOperandOrigins));
		oaConstructedOperands.SetSize(constructionRule->GetOperandNumber());
	}
}

const KDConstructionRule* KDConstructedRule::GetConstructionRule() const
{
	return constructionRule;
}

const ALString& KDConstructedRule::GetClassName() const
{
	require(GetConstructionRule() != NULL);
	return constructionRule->GetClassName();
}

const ALString& KDConstructedRule::GetName() const
{
	require(GetConstructionRule() != NULL);
	return constructionRule->GetName();
}

int KDConstructedRule::GetType() const
{
	require(GetConstructionRule() != NULL);
	return constructionRule->GetType();
}

const ALString& KDConstructedRule::GetObjectClassName() const
{
	KWAttribute* attribute;
	KDConstructedRule* rule;

	require(GetConstructionRule() != NULL);
	require(KWType::IsRelation(GetType()));
	require(GetOperandNumber() > 0);
	require(GetOperandOriginAt(0) == Attribute or GetOperandOriginAt(0) == Rule);

	if (GetOperandOriginAt(0) == Attribute)
	{
		attribute = GetAttributeOperandAt(0);
		assert(attribute->GetType() == GetType());
		return attribute->GetClass()->GetName();
	}
	else
	{
		rule = GetRuleOperandAt(0);
		assert(rule->GetType() == GetType());
		return rule->GetObjectClassName();
	}
}

const KWDerivationRule* KDConstructedRule::GetDerivationRule() const
{
	require(GetConstructionRule() != NULL);
	return constructionRule->GetDerivationRule();
}

int KDConstructedRule::GetOperandNumber() const
{
	require(GetConstructionRule() != NULL);
	return constructionRule->GetOperandNumber();
}

void KDConstructedRule::SetAttributeOperandAt(int nIndex, KWAttribute* attribute)
{
	require(GetConstructionRule() != NULL);
	require(0 <= nIndex and nIndex < GetOperandNumber());
	require(attribute != NULL);

	// Destruction si necessaire de l'operande en cours
	if (GetOperandOriginAt(nIndex) == Rule)
		delete oaConstructedOperands.GetAt(nIndex);

	// Parametrage du nouvel operande
	oaConstructedOperands.SetAt(nIndex, attribute);
	cOperandOrigins[nIndex] = Attribute;
}

KWAttribute* KDConstructedRule::GetAttributeOperandAt(int nIndex) const
{
	require(GetConstructionRule() != NULL);
	require(0 <= nIndex and nIndex < GetOperandNumber());
	require(GetOperandOriginAt(nIndex) == Attribute);
	return cast(KWAttribute*, oaConstructedOperands.GetAt(nIndex));
}

void KDConstructedRule::SetRuleOperandAt(int nIndex, KDConstructedRule* rule)
{
	require(GetConstructionRule() != NULL);
	require(0 <= nIndex and nIndex < GetOperandNumber());
	require(rule != NULL);

	// Destruction si necessaire de l'operande en cours
	if (GetOperandOriginAt(nIndex) == Rule)
		delete oaConstructedOperands.GetAt(nIndex);

	// Parametrage du nouvel operande
	oaConstructedOperands.SetAt(nIndex, rule);
	cOperandOrigins[nIndex] = Rule;
}

KDConstructedRule* KDConstructedRule::GetRuleOperandAt(int nIndex) const
{
	require(GetConstructionRule() != NULL);
	require(0 <= nIndex and nIndex < GetOperandNumber());
	require(GetOperandOriginAt(nIndex) == Rule);
	return cast(KDConstructedRule*, oaConstructedOperands.GetAt(nIndex));
}

void KDConstructedRule::SetPartOperandAt(int nIndex, KDConstructedPart* part)
{
	require(GetConstructionRule() != NULL);
	require(0 <= nIndex and nIndex < GetOperandNumber());
	require(part != NULL);

	// Destruction si necessaire de l'operande en cours
	if (GetOperandOriginAt(nIndex) == Rule)
		delete oaConstructedOperands.GetAt(nIndex);

	// Parametrage du nouvel operande
	oaConstructedOperands.SetAt(nIndex, part);
	cOperandOrigins[nIndex] = Part;
}

KDConstructedPart* KDConstructedRule::GetPartOperandAt(int nIndex) const
{
	require(GetConstructionRule() != NULL);
	require(0 <= nIndex and nIndex < GetOperandNumber());
	require(GetOperandOriginAt(nIndex) == Part);
	return cast(KDConstructedPart*, oaConstructedOperands.GetAt(nIndex));
}

int KDConstructedRule::GetOperandOriginAt(int nIndex) const
{
	require(GetConstructionRule() != NULL);
	require(0 <= nIndex and nIndex < GetOperandNumber());
	return cOperandOrigins[nIndex];
}

void KDConstructedRule::DeleteOperandAt(int nIndex)
{
	require(GetConstructionRule() != NULL);
	require(0 <= nIndex and nIndex < GetOperandNumber());

	// Destruction si necessaire de l'operande en cours
	if (GetOperandOriginAt(nIndex) == Rule)
		delete oaConstructedOperands.GetAt(nIndex);

	// Reinitialisation de l'operande
	oaConstructedOperands.SetAt(nIndex, NULL);
	cOperandOrigins[nIndex] = None;
}

void KDConstructedRule::RemoveOperandAt(int nIndex)
{
	require(GetConstructionRule() != NULL);
	require(0 <= nIndex and nIndex < GetOperandNumber());

	// Reinitialisation de l'operande
	oaConstructedOperands.SetAt(nIndex, NULL);
	cOperandOrigins[nIndex] = None;
}

void KDConstructedRule::DeleteAllOperands()
{
	int i;

	require(GetConstructionRule() != NULL);

	for (i = 0; i < GetOperandNumber(); i++)
		DeleteOperandAt(i);
}

void KDConstructedRule::RemoveAllOperands()
{
	int i;

	require(GetConstructionRule() != NULL);

	for (i = 0; i < GetOperandNumber(); i++)
		RemoveOperandAt(i);
}

void KDConstructedRule::SetRandomIndex(int nValue)
{
	require(nValue >= 0);
	nRandomIndex = nValue;
}

int KDConstructedRule::GetRandomIndex() const
{
	return nRandomIndex;
}

void KDConstructedRule::IncrementUseCounts()
{
	int i;
	KDConstructedPart* part;

	// Pproparagtion aux operandes
	for (i = 0; i < GetOperandNumber(); i++)
	{
		// Operandes de type Rule: propagation
		if (GetOperandOriginAt(i) == Rule)
			GetRuleOperandAt(i)->IncrementUseCounts();
		// Incrementation des comptes pour la partie, sa partition, et les dimensions de la partition
		else if (GetOperandOriginAt(i) == Part)
		{
			part = GetPartOperandAt(i);
			part->IncrementUseCount();
			part->GetPartition()->IncrementUseCounts();
		}
	}
}

KWDerivationRule* KDConstructedRule::BuildDerivationRule() const
{
	KWDerivationRule* builtDerivationRule;
	KWDerivationRuleOperand* operand;
	KWDerivationRule* operandRule;
	int i;
	KWAttribute* attribute;
	KDConstructedRule* rule;
	KDConstructedPart* part;

	require(Check());

	// Creation de la regle
	builtDerivationRule = constructionRule->GetDerivationRule()->Clone();

	// Parametrage de sa classe
	builtDerivationRule->SetClassName(GetClassName());

	// Parametrage de ses operandes
	builtDerivationRule->DeleteAllOperands();
	for (i = 0; i < GetOperandNumber(); i++)
	{
		// Creation de l'operande
		operand = new KWDerivationRuleOperand;
		builtDerivationRule->AddOperand(operand);

		// Operande de type Attribute
		if (GetOperandOriginAt(i) == Attribute)
		{
			attribute = GetAttributeOperandAt(i);

			// Parametrage par les caracteristiques de l'attribut
			operand->SetType(attribute->GetType());
			if (KWType::IsRelation(attribute->GetType()))
				operand->SetObjectClassName(attribute->GetClass()->GetName());
			operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
			operand->SetAttributeName(attribute->GetName());
		}
		// Operandes de type Rule
		else if (GetOperandOriginAt(i) == Rule)
		{
			rule = GetRuleOperandAt(i);

			// Parametrage par la regle de derivation construite a partir de la regle construite en operande
			operandRule = rule->BuildDerivationRule();
			operand->SetType(operandRule->GetType());
			if (KWType::IsRelation(operandRule->GetType()))
				operand->SetObjectClassName(operandRule->GetObjectClassName());
			operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
			operand->SetDerivationRule(operandRule);
		}
		// Test des operandes de type Part
		else if (GetOperandOriginAt(i) == Part)
		{
			part = GetPartOperandAt(i);

			// Parametrage par la regle de derivation construite a partir de la partie de partition en
			// operande
			operandRule = part->BuildDerivationRule();
			operand->SetType(operandRule->GetType());
			if (KWType::IsRelation(operandRule->GetType()))
				operand->SetObjectClassName(operandRule->GetObjectClassName());
			operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
			operand->SetDerivationRule(operandRule);
		}
	}
	return builtDerivationRule;
}

ALString KDConstructedRule::BuildAttributeName(boolean bIsBlockName) const
{
	return BuildInterpretableName(bIsBlockName);
}

boolean KDConstructedRule::IsSelectionRule() const
{
	return constructionRule->IsSelectionRule();
}

boolean KDConstructedRule::IsPartitionBlockRule() const
{
	return GetConstructionRule()->GetPartitionStatsRule() != NULL and GetOperandOriginAt(0) == Rule and
	       GetRuleOperandAt(0)->IsSelectionRule();
}

boolean KDConstructedRule::IsValueBlockRule() const
{
	return GetConstructionRule()->GetValueBlockRule() != NULL and GetOperandOriginAt(1) == Rule and
	       GetRuleOperandAt(1)->IsBlockRule();
}

boolean KDConstructedRule::IsBlockRule() const
{
	return IsPartitionBlockRule() or IsValueBlockRule();
}

boolean KDConstructedRule::IsStandardRule() const
{
	return not IsSelectionRule() and not IsBlockRule();
}

boolean KDConstructedRule::UsesSelectionRule() const
{
	boolean bUsesSelectionRule;
	int i;

	// Test si la regle est une regle de selection
	bUsesSelectionRule = IsSelectionRule();

	// Test sur les oiperande si necessaire
	if (not bUsesSelectionRule)
	{
		for (i = 0; i < GetOperandNumber(); i++)
		{
			if (GetOperandOriginAt(i) == KDConstructedRule::Rule)
			{
				bUsesSelectionRule = GetRuleOperandAt(i)->UsesSelectionRule();
				if (bUsesSelectionRule)
					break;
			}
		}
	}
	return bUsesSelectionRule;
}

const KDConstructedPart* KDConstructedRule::GetUsedPart() const
{
	// Cas d'une regle de selection
	if (IsSelectionRule())
		return GetPartOperandAt(1);
	// Cas d'une regle de type bloc de parties
	else if (IsPartitionBlockRule())
		return GetRuleOperandAt(0)->GetPartOperandAt(1);
	// Cas d'une regle de type bloc de valeurs
	else if (IsValueBlockRule())
		return GetRuleOperandAt(1)->GetUsedPart();
	// Cas standard
	else
		return NULL;
}

KDConstructedRule* KDConstructedRule::Clone() const
{
	KDConstructedRule* constructedRuleClone;
	int i;

	// Creation
	constructedRuleClone = new KDConstructedRule;

	// Initialisation
	constructedRuleClone->SetConstructionRule(constructionRule);
	constructedRuleClone->dCost = dCost;

	// Initialisation des operandes
	if (constructionRule != NULL)
	{
		for (i = 0; i < GetOperandNumber(); i++)
		{
			// Operande de type Attribute
			if (GetOperandOriginAt(i) == Attribute)
				constructedRuleClone->SetAttributeOperandAt(i, GetAttributeOperandAt(i));
			// Operandes de type Rule
			else if (GetOperandOriginAt(i) == Rule)
				constructedRuleClone->SetRuleOperandAt(i, GetRuleOperandAt(i)->Clone());
			// Test des operandes de type Part
			else if (GetOperandOriginAt(i) == Part)
				constructedRuleClone->SetPartOperandAt(i, GetPartOperandAt(i));
		}
	}
	return constructedRuleClone;
}

boolean KDConstructedRule::Check() const
{
	boolean bOk = true;
	ALString sTmp;
	KWAttribute* attribute;
	KDConstructedRule* rule;
	KDConstructedPart* part;
	int i;

	// Test de presence de la regle de construction
	if (bOk and constructionRule == NULL)
	{
		AddError("Missing construction rule");
		bOk = false;
	}

	// Test de presence des operandes
	if (bOk)
	{
		assert(oaConstructedOperands.GetSize() == constructionRule->GetOperandNumber());
		assert(oaConstructedOperands.GetSize() <= sizeof(cOperandOrigins));
		for (i = 0; i < oaConstructedOperands.GetSize(); i++)
		{
			assert((cOperandOrigins[i] == None and oaConstructedOperands.GetAt(i) == NULL) or
			       (cOperandOrigins[i] != None and oaConstructedOperands.GetAt(i) != NULL));

			// Test de presence de l'operande
			if (GetOperandOriginAt(i) == None)
			{
				AddError(sTmp + "Missing operand at index " + IntToString(i + 1));
				bOk = false;
			}
			// Test des operandes de type Attribute
			else if (GetOperandOriginAt(i) == Attribute)
			{
				attribute = GetAttributeOperandAt(i);
				if (attribute->GetType() != constructionRule->GetOperandAt(i)->GetType())
				{
					AddError(sTmp + "Type of variable " + attribute->GetName() + " (" +
						 KWType::ToString(attribute->GetType()) +
						 ") inconsistent with that expected for operand " + IntToString(i + 1) +
						 " (" + KWType::ToString(constructionRule->GetOperandAt(i)->GetType()) +
						 ")");
					bOk = false;
				}
			}
			// Test des operandes de type Rule
			else if (GetOperandOriginAt(i) == Rule)
			{
				rule = GetRuleOperandAt(i);
				if (rule->GetConstructionRule()->GetType() !=
				    constructionRule->GetOperandAt(i)->GetType())
				{
					AddError(sTmp + "Type of rule " + rule->GetConstructionRule()->GetName() +
						 " (" + KWType::ToString(rule->GetConstructionRule()->GetType()) +
						 ") inconsistent with that expected for operand " + IntToString(i + 1) +
						 " (" + KWType::ToString(constructionRule->GetOperandAt(i)->GetType()) +
						 ")");
					bOk = false;
				}
			}
			// Test des operandes de type Part
			else if (GetOperandOriginAt(i) == Part)
			{
				part = GetPartOperandAt(i);

				// La regle ne peut etre qu'un regle de selection
				if (not GetConstructionRule()->IsSelectionRule())
				{
					AddError(sTmp + "Part operand " + part->GetObjectLabel() +
						 " should be used with a " + GetConstructionRule()->GetName() +
						 " construction rule rule");
					bOk = false;
				}
				// Et l'operande doit etre le dernier
				else if (i != GetConstructionRule()->GetOperandNumber() - 1)
				{
					AddError(sTmp + "Part operand " + part->GetObjectLabel() + " at operand " +
						 IntToString(i + 1) + " is expected only a operand " +
						 IntToString(GetConstructionRule()->GetOperandNumber()));
					bOk = false;
				}
			}
			if (not bOk)
				break;
		}
	}
	return bOk;
}

int KDConstructedRule::Compare(const KDConstructedRule* rule) const
{
	int nDiff;
	KDConstructedPart* part;
	KDConstructedPart* rulePart;
	int i;

	require(rule != NULL);

	// Les regles de constructions doivent etre specifiees
	assert(GetConstructionRule() != NULL);
	assert(rule->GetConstructionRule() != NULL);

	// Comparaison des classes sur lesquelle portent les regles
	nDiff = GetClassName().Compare(rule->GetClassName());

	// Comparaison du nom de la regle de derivation
	if (nDiff == 0)
		nDiff = GetName().Compare(rule->GetName());

	// Comparaison des nombre d'operandes
	if (nDiff == 0)
		nDiff = GetOperandNumber() - rule->GetOperandNumber();

	// Comparaison des operandes
	if (nDiff == 0)
	{
		for (i = 0; i < GetOperandNumber(); i++)
		{
			// Comparaison des types d'operande
			nDiff = GetOperandOriginAt(i) - rule->GetOperandOriginAt(i);
			if (nDiff != 0)
				break;

			// Comparaison des noms d'attribut
			if (GetOperandOriginAt(i) == KDConstructedRule::Attribute)
				nDiff = GetAttributeOperandAt(i)->GetName().Compare(
				    rule->GetAttributeOperandAt(i)->GetName());
			// Comparaison des regles
			else if (GetOperandOriginAt(i) == KDConstructedRule::Rule)
				nDiff = GetRuleOperandAt(i)->Compare(rule->GetRuleOperandAt(i));
			// Comparaison des parties
			else if (GetOperandOriginAt(i) == KDConstructedRule::Part)
			{
				part = cast(KDConstructedPart*, GetPartOperandAt(i));
				rulePart = cast(KDConstructedPart*, rule->GetPartOperandAt(i));

				// Comparaison des partitions
				nDiff = part->GetPartition()->Compare(rulePart->GetPartition());

				// Comparaison des parties
				if (nDiff == 0)
					nDiff = part->Compare(rulePart);
			}
			if (nDiff != 0)
				break;
		}
	}
	return nDiff;
}

int KDConstructedRule::CompareBlock(const KDConstructedRule* rule) const
{
	int nPartitionLevel = 0;
	return InternalCompareBlock(rule, nPartitionLevel);
}

int KDConstructedRule::CompareBlockCostName(const KDConstructedRule* rule) const
{
	int nPartitionLevel = 0;

	int nResult;
	longint lSortValue1;
	longint lSortValue2;

	require(rule != NULL);

	// Les regles de constructions doivent etre specifiees
	assert(GetConstructionRule() != NULL);
	assert(rule->GetConstructionRule() != NULL);

	// On se base sur un comparaison a sept decimales pres pour le critere de cout
	// Sept decimales semblent le bon niveau pour assurer la stabilite de la comparaison
	lSortValue1 = longint(floor(GetCost() * 1e7 + 0.5));
	lSortValue2 = longint(floor(rule->GetCost() * 1e7 + 0.5));

	// Comparaison sur le cout
	nResult = CompareLongint(lSortValue1, lSortValue2);

	// Si egalite, comparison du bloc
	if (nResult == 0)
		nResult = InternalCompareBlock(rule, nPartitionLevel);
	return nResult;
}

int KDConstructedRule::InternalCompareBlock(const KDConstructedRule* rule, int& nPartitionLevel) const
{
	int nDiff;
	KDConstructedPart* part;
	KDConstructedPart* rulePart;
	int i;

	require(rule != NULL);
	require(nPartitionLevel >= 0);

	// Les regles de constructions doivent etre specifiees
	assert(GetConstructionRule() != NULL);
	assert(rule->GetConstructionRule() != NULL);

	// Comparaison des classes sur lesquelle portent les regles
	nDiff = GetClassName().Compare(rule->GetClassName());

	// Comparaison du nom de la regle de derivation
	if (nDiff == 0)
		nDiff = GetName().Compare(rule->GetName());

	// Comparaison des nombre d'operandes
	if (nDiff == 0)
		nDiff = GetOperandNumber() - rule->GetOperandNumber();

	// Comparaison des operandes
	if (nDiff == 0)
	{
		for (i = 0; i < GetOperandNumber(); i++)
		{
			// Comparaison des types d'operande
			nDiff = GetOperandOriginAt(i) - rule->GetOperandOriginAt(i);
			if (nDiff != 0)
				break;

			// Comparaison des noms d'attribut
			if (GetOperandOriginAt(i) == KDConstructedRule::Attribute)
				nDiff = GetAttributeOperandAt(i)->GetName().Compare(
				    rule->GetAttributeOperandAt(i)->GetName());
			// Comparaison des regles
			else if (GetOperandOriginAt(i) == KDConstructedRule::Rule)
				nDiff = GetRuleOperandAt(i)->InternalCompareBlock(rule->GetRuleOperandAt(i),
										  nPartitionLevel);
			// Comparaison des parties
			else if (GetOperandOriginAt(i) == KDConstructedRule::Part)
			{
				part = cast(KDConstructedPart*, GetPartOperandAt(i));
				rulePart = cast(KDConstructedPart*, rule->GetPartOperandAt(i));

				// Comparaison des partitions
				nDiff = part->GetPartition()->Compare(rulePart->GetPartition());

				// Comparaison des parties uniquement au niveau superieur
				if (nPartitionLevel > 0 and nDiff == 0)
					nDiff = part->Compare(rulePart);

				// Incrementation du niveau de partition rencontree
				nPartitionLevel++;
			}
			if (nDiff != 0)
				break;
		}
	}
	return nDiff;
}

int KDConstructedRule::CompareWithDerivationRule(const KWDerivationRule* derivationRule) const
{
	int nDiff;
	KWDerivationRule* comparedDerivationRule;

	require(Check());
	require(derivationRule != NULL);
	require(derivationRule->Check());

	// Comparaison des classes sur lesquelle portent les regles
	nDiff = GetClassName().Compare(derivationRule->GetClassName());

	// Comparaison du nom de la regle de derivation
	if (nDiff == 0)
		nDiff = constructionRule->GetDerivationRule()->GetName().Compare(derivationRule->GetName());

	// Comparaison du nombre d'operandes
	if (nDiff == 0)
		nDiff = GetOperandNumber() - derivationRule->GetOperandNumber();

	// Comparaison des operandes
	if (nDiff == 0)
	{
		// Construction d'une regle de derivation temporaire
		comparedDerivationRule = BuildDerivationRule();
		comparedDerivationRule->CompleteTypeInfo(derivationRule->GetOwnerClass());

		// Comparaison
		nDiff = comparedDerivationRule->FullCompare(derivationRule);

		// Nettoyage
		delete comparedDerivationRule;
	}
	return nDiff;
}

int KDConstructedRule::CompareCost(const KDConstructedRule* rule) const
{
	int nResult;
	longint lSortValue1;
	longint lSortValue2;
	int i;

	require(rule != NULL);

	// Les regles de constructions doivent etre specifiees
	assert(GetConstructionRule() != NULL);
	assert(rule->GetConstructionRule() != NULL);

	// On se base sur un comparaison a sept decimales pres pour le critere de cout
	// Sept decimales semblent le bon niveau pour assurer la stabilite de la comparaison
	lSortValue1 = longint(floor(GetCost() * 1e7 + 0.5));
	lSortValue2 = longint(floor(rule->GetCost() * 1e7 + 0.5));

	// Comparaison sur le cout
	nResult = CompareLongint(lSortValue1, lSortValue2);

	///////////////////////////////////////////////////////////////////////////////
	// Comparaison sur la simplicite de la formule (nombre et type des operandes)
	// mais pas sur leur nom, pour preserver un ordre aleatoire en cas d'egalite

	// Comparaison des nombre d'operandes
	if (nResult == 0)
		nResult = GetOperandNumber() - rule->GetOperandNumber();

	// On verifie que les types d'operandes sont correctement ordonnes
	assert(Attribute < Rule and Rule < Part);

	// Comparaison des operandes
	if (nResult == 0)
	{
		for (i = 0; i < GetOperandNumber(); i++)
		{
			// Comparaison sur l'operande en cours
			nResult = GetOperandOriginAt(i) - rule->GetOperandOriginAt(i);
			if (nResult != 0)
				break;

			// Comparaison des regles
			if (GetOperandOriginAt(i) == KDConstructedRule::Rule)
				nResult = GetRuleOperandAt(i)->CompareCost(rule->GetRuleOperandAt(i));
			// Comparaison des parties
			else if (GetOperandOriginAt(i) == KDConstructedRule::Part)
				nResult = GetPartOperandAt(i)->CompareCost(rule->GetPartOperandAt(i));

			// Arret si operandes differents
			if (nResult != 0)
				break;
		}
	}
	return nResult;
}

int KDConstructedRule::CompareCostRandomIndex(const KDConstructedRule* rule) const
{
	int nResult;

	require(rule != NULL);

	// Comparaison sur le cout
	nResult = CompareCost(rule);

	// Comparaison sur l'index aleatoire si egalite
	if (nResult == 0)
		nResult = GetRandomIndex() - rule->GetRandomIndex();
	return nResult;
}

int KDConstructedRule::CompareCostName(const KDConstructedRule* rule) const
{
	int nResult;

	require(rule != NULL);

	// Comparaison sur le cout
	nResult = CompareCost(rule);

	// Comparaison sur le nom de la variable si egalite
	if (nResult == 0)
		nResult = Compare(rule);
	return nResult;
}

void KDConstructedRule::Write(ostream& ost) const
{
	int i;
	KWAttribute* attribute;
	KDConstructedRule* rule;
	KDConstructedPart* part;

	// Nom de la regle utilisee
	if (GetConstructionRule() != NULL)
	{
		ost << GetConstructionRule()->GetName();

		// Operandes
		ost << "(";
		for (i = 0; i < GetOperandNumber(); i++)
		{
			if (i > 0)
				ost << ", ";

			// Operande de type Attribute
			if (GetOperandOriginAt(i) == Attribute)
			{
				attribute = GetAttributeOperandAt(i);
				ost << attribute->GetName();
			}
			// Operande de type Rule
			else if (GetOperandOriginAt(i) == Rule)
			{
				rule = GetRuleOperandAt(i);
				ost << *rule;
			}
			// Operande de type Part
			else if (GetOperandOriginAt(i) == Part)
			{
				part = GetPartOperandAt(i);
				ost << *part;
			}
		}
		ost << ")";
	}
}

void KDConstructedRule::WriteOperandAt(int nIndex, ostream& ost) const
{
	KWAttribute* attribute;
	KDConstructedRule* rule;
	KDConstructedPart* part;

	// Si la regle est specifiee
	if (GetConstructionRule() != NULL)
	{
		assert(0 <= nIndex and nIndex < GetOperandNumber());

		// Operande de type Attribute
		if (GetOperandOriginAt(nIndex) == Attribute)
		{
			attribute = GetAttributeOperandAt(nIndex);
			ost << attribute->GetName();
		}
		// Operande de type Rule
		else if (GetOperandOriginAt(nIndex) == Rule)
		{
			rule = GetRuleOperandAt(nIndex);
			ost << *rule;
		}
		// Operande de type Part
		else if (GetOperandOriginAt(nIndex) == Part)
		{
			part = GetPartOperandAt(nIndex);
			ost << *part;
		}
	}
}

void KDConstructedRule::WriteCostDetails(ostream& ost, const ALString& sTreePrefix) const
{
	int i;
	KWAttribute* attribute;
	KDConstructedRule* rule;
	KDConstructedPart* part;

	// Information de cout et de nom
	ost << sTreePrefix << "\t" << GetCost() << "\t" << *this << "\n";

	// Details par operande
	if (GetConstructionRule() != NULL)
	{
		for (i = 0; i < GetOperandNumber(); i++)
		{
			// Operande de type Attribute
			if (GetOperandOriginAt(i) == Attribute)
			{
				attribute = GetAttributeOperandAt(i);
				ost << sTreePrefix << "." << i + 1 << "\t\t" << attribute->GetName() << "\n";
			}
			// Operande de type Rule
			else if (GetOperandOriginAt(i) == Rule)
			{
				rule = GetRuleOperandAt(i);
				rule->WriteCostDetails(ost, sTreePrefix + "." + IntToString(i + 1));
			}
			// Operande de type Part
			else if (GetOperandOriginAt(i) == Part)
			{
				part = GetPartOperandAt(i);
				part->WriteCostDetails(ost, sTreePrefix + "." + IntToString(i + 1));
			}
		}
		ost << ")";
	}
}

longint KDConstructedRule::GetUsedMemory() const
{
	longint lUsedMemory;
	int i;

	// Memoire de base
	lUsedMemory = sizeof(KDConstructedRule);
	lUsedMemory += oaConstructedOperands.GetUsedMemory() - sizeof(ObjectArray);

	// Ajout de la memoire des sous-regles construites
	if (GetConstructionRule() != NULL)
	{
		for (i = 0; i < GetOperandNumber(); i++)
		{
			if (GetOperandOriginAt(i) == Rule)
				lUsedMemory += oaConstructedOperands.GetAt(i)->GetUsedMemory();
		}
	}
	return lUsedMemory;
}

const ALString KDConstructedRule::GetObjectLabel() const
{
	if (constructionRule == NULL)
		return "";
	else
		return constructionRule->GetName();
}

const ALString KDConstructedRule::GetClassLabel() const
{
	return "Constructed rule";
}

ALString KDConstructedRule::BuildInterpretableName(boolean bIsBlockName) const
{
	ALString sVariableName;
	ALString sOperandName;

	// Cas d'une regle de type Date
	check(constructionRule);
	if (constructionRule->GetFamilyName() == "Date")
	{
		sOperandName = BuildInterpretableNameFromOperandAt(0, bIsBlockName);
		sVariableName = sOperandName + "." + GetName();
	}
	// Cas d'une regle de type Time
	else if (constructionRule->GetFamilyName() == "Time")
	{
		sOperandName = BuildInterpretableNameFromOperandAt(0, bIsBlockName);
		sVariableName = sOperandName + "." + GetName();
	}
	// Cas d'une regle de type Timestamp
	else if (constructionRule->GetFamilyName() == "Timestamp")
	{
		sOperandName = BuildInterpretableNameFromOperandAt(0, bIsBlockName);

		// Cas particulier si on extrait la date ou l'heure
		if (GetName() == "GetDate")
			sVariableName = sOperandName + ".Date";
		else if (GetName() == "GetTime")
			sVariableName = sOperandName + ".Time";
		// Cas standard
		else
			sVariableName = sOperandName + "." + GetName();
	}
	// Cas d'une regle de type TimestampTZ
	else if (constructionRule->GetFamilyName() == "TimestampTZ")
	{
		sOperandName = BuildInterpretableNameFromOperandAt(0, bIsBlockName);

		// Cas particulier si on extrait la date ou l'heure
		if (GetName() == "LocalTimestamp")
			sVariableName = sOperandName + ".LocalTZ";
		// Cas standard
		else
			sVariableName = sOperandName + "." + GetName();
	}
	// Cas d'une regle de type Entity
	else if (constructionRule->GetFamilyName() == "Entity")
	{
		// Le cas de la regle "Exist" a ete teste, mais n'est pas retenu actuellement comme regle de
		// construction
		assert(GetName() == "Exist" or GetName() == "GetValueC" or GetName() == "GetValue");

		// Cas d'une regle avec un seul operande
		if (GetOperandNumber() == 1)
		{
			// Nommage de la variable sous la forme d'une fonction
			sVariableName = GetName() + "(" + BuildInterpretableNameFromOperandAt(0, bIsBlockName) + ")";
		}
		// Cas avec plusieurs operandes
		else
		{
			assert(GetOperandNumber() == 2);

			// Nommage de l'operande (ici en deuxieme position)
			sOperandName = BuildInterpretableNameFromOperandAt(1, bIsBlockName);

			// Nommage de la variable sous la forme d'un chemin d'acces
			sVariableName = BuildInterpretableNameFromOperandAt(0, bIsBlockName) + "." + sOperandName;
		}
	}
	// Cas d'une regle de type Table
	else
	{
		assert(constructionRule->GetFamilyName() == "Table");
		sVariableName = BuildTableInterpretableName(bIsBlockName);
	}

	return sVariableName;
}

ALString KDConstructedRule::BuildTableInterpretableName(boolean bIsBlockName) const
{
	ALString sVariableName;
	const KDConstructedRule* selectionRule;
	ALString sTableName;
	ALString sOperandName;
	ALString sSelectionName;
	ALString sOperatorName;

	require(GetConstructionRule()->GetFamilyName() == "Table");

	// Cas particulier de la regle de selection
	selectionRule = NULL;
	if (GetConstructionRule()->IsSelectionRule())
	{
		// Acces au nom de la table
		assert(GetOperandOriginAt(0) == Attribute);
		sTableName = GetAttributeOperandAt(0)->GetName();

		// Nom de la variable
		selectionRule = this;
		sVariableName = sTableName;
	}
	// Cas standard: operateur sur une variable de la table
	else
	{
		assert(GetOperandNumber() <= 2);

		// Extraction du nom de l'operateur en supprimant le prefixe "Table" du nom de la regle
		sOperatorName = GetName();
		assert(sOperatorName.Find("Table") == 0);
		sOperatorName = sOperatorName.Right(sOperatorName.GetLength() - 5);

		// Extraction du nom de la table dans le cas d'un attribut
		if (GetOperandOriginAt(0) == Attribute)
			sTableName = GetAttributeOperandAt(0)->GetName();
		// Extraction du nom de la table dans le cas d'une regle, qui ne peut etre que la regle de selection
		else
		{
			assert(GetOperandOriginAt(0) == Rule);

			// Extraction du nom de la table de la regle de selection
			selectionRule = GetRuleOperandAt(0);
			assert(selectionRule->GetName() == "TableSelection");
			assert(selectionRule->GetOperandOriginAt(0) == Attribute);
			sTableName = selectionRule->GetAttributeOperandAt(0)->GetName();
		}

		// Extrait du nom de l'operande
		if (GetOperandNumber() == 2)
		{
			// Dans le cas d'une regle de selection, comme on n'est pas dans le critere de selection
			// principal, on construit ici un nom de variable plutot que de bloc pour cet operande
			if (selectionRule != NULL)
				sOperandName = BuildInterpretableNameFromOperandAt(1, false);
			// Cas general sinon
			else
				sOperandName = BuildInterpretableNameFromOperandAt(1, bIsBlockName);
		}

		// Construction du nom de la variable sous une forme fonctionnelle, avec chemin d'acces aux donnees
		sVariableName = sOperatorName + "(" + sTableName;
		if (sOperandName != "")
			sVariableName += "." + sOperandName;
		sVariableName += ")";
	}

	// Ajout du critere de selection dans le cas de la regle de selection
	if (selectionRule != NULL)
	{
		assert(selectionRule->GetOperandOriginAt(1) == Part);
		if (bIsBlockName)
		{
			sSelectionName = selectionRule->GetPartOperandAt(1)->GetPartition()->BuildSelectionName();
			sVariableName += " per " + sSelectionName;
		}
		else
		{
			sSelectionName = selectionRule->GetPartOperandAt(1)->BuildInterpretableSelectionName();
			sVariableName += " where " + sSelectionName;
		}
	}
	return sVariableName;
}

ALString KDConstructedRule::BuildInterpretableNameFromOperandAt(int nIndex, boolean bIsBlockName) const
{
	ALString sOperandName;

	require(0 <= nIndex and nIndex < GetOperandNumber());
	require(GetOperandOriginAt(nIndex) != Part);

	// Si origine attribut
	if (GetOperandOriginAt(nIndex) == Attribute)
		sOperandName = GetAttributeOperandAt(nIndex)->GetName();
	// Si origine regle
	else
		sOperandName = GetRuleOperandAt(nIndex)->BuildInterpretableName(bIsBlockName);
	return sOperandName;
}

int KDConstructedRuleCompare(const void* elem1, const void* elem2)
{
	int nResult;
	KDConstructedRule* rule1;
	KDConstructedRule* rule2;

	// Acces aux partitions
	rule1 = cast(KDConstructedRule*, *(Object**)elem1);
	rule2 = cast(KDConstructedRule*, *(Object**)elem2);

	// Comparaison
	nResult = rule1->Compare(rule2);
	return nResult;
}

int KDConstructedRuleCompareCost(const void* elem1, const void* elem2)
{
	int nResult;
	KDConstructedRule* rule1;
	KDConstructedRule* rule2;

	// Acces aux partitions
	rule1 = cast(KDConstructedRule*, *(Object**)elem1);
	rule2 = cast(KDConstructedRule*, *(Object**)elem2);

	// Comparaison
	nResult = rule1->CompareCost(rule2);
	return nResult;
}

int KDConstructedRuleCompareCostRandomIndex(const void* elem1, const void* elem2)
{
	int nResult;
	KDConstructedRule* rule1;
	KDConstructedRule* rule2;

	// Acces aux partitions
	rule1 = cast(KDConstructedRule*, *(Object**)elem1);
	rule2 = cast(KDConstructedRule*, *(Object**)elem2);

	// Comparaison
	nResult = rule1->CompareCostRandomIndex(rule2);
	return nResult;
}

int KDConstructedRuleCompareCostName(const void* elem1, const void* elem2)
{
	int nResult;
	KDConstructedRule* rule1;
	KDConstructedRule* rule2;

	// Acces aux partitions
	rule1 = cast(KDConstructedRule*, *(Object**)elem1);
	rule2 = cast(KDConstructedRule*, *(Object**)elem2);

	// Comparaison
	nResult = rule1->CompareCostName(rule2);
	return nResult;
}

//////////////////////////////////////////////////////////////////////////
// Classe KDConstructedPartition

KDConstructedPartition::KDConstructedPartition()
{
	kwcPartitionClass = NULL;
	tableAttribute = NULL;
	slSortedParts.SetCompareFunction(KDConstructedPartCompare);
	lUseCount = 0;
	kwaPartitionAttribute = NULL;
	kwabTablePartitionAttributeBlock = NULL;
}

KDConstructedPartition::~KDConstructedPartition()
{
	// Destruction des parties
	slSortedParts.DeleteAll();

	// Nettoyage des vecteurs de correspondance entre index theorique et index pratique
	oaPartitionBuiltPartIndexes.DeleteAll();
}

void KDConstructedPartition::SetPartitionClass(const KWClass* kwcClass)
{
	kwcPartitionClass = kwcClass;
	SetDimensionNumber(0);
}

const KWClass* KDConstructedPartition::GetPartitionClass() const
{
	return kwcPartitionClass;
}

void KDConstructedPartition::SetTableAttribute(const KWAttribute* kwaAttribute)
{
	require(kwaAttribute == NULL or kwaAttribute->GetClass()->GetName() == GetPartitionClass()->GetName());
	tableAttribute = kwaAttribute;
}

const KWAttribute* KDConstructedPartition::GetTableAttribute() const
{
	return tableAttribute;
}

const KWClass* KDConstructedPartition::GetParentClass() const
{
	require(GetTableAttribute() != NULL);
	return GetTableAttribute()->GetParentClass();
}

void KDConstructedPartition::SetDimensionNumber(int nValue)
{
	int i;

	require(nValue >= 0);

	// Reinitialisation de la partition
	if (nValue > 0)
	{
		// Destruction des parties
		slSortedParts.DeleteAll();
	}

	// Initialisation des caracteristiques de la partition
	oaPartitionDimensions.SetSize(nValue);
	ivPartitionGranularities.SetSize(nValue);
	for (i = 0; i < nValue; i++)
	{
		oaPartitionDimensions.SetAt(i, NULL);
		ivPartitionGranularities.SetAt(i, 0);
	}

	// Initialisation des vecteurs de correspondance entre index de partie theorique et construites
	ivPartitionBuiltPartNumbers.SetSize(0);
	oaPartitionBuiltPartIndexes.DeleteAll();
	ivPartitionBuiltPartNumbers.SetSize(nValue);
	oaPartitionBuiltPartIndexes.SetSize(nValue);
	for (i = 0; i < nValue; i++)
		oaPartitionBuiltPartIndexes.SetAt(i, new IntVector);
	ensure(slSortedParts.GetCount() == 0);
}

int KDConstructedPartition::GetDimensionNumber() const
{
	return oaPartitionDimensions.GetSize();
}

void KDConstructedPartition::SetGranularityAt(int nIndex, int nValue)
{
	IntVector* ivBuiltPartIndexes;

	require(0 <= nIndex and nIndex < GetDimensionNumber());
	require(nValue >= 0);

	// Parametrage de la granularite
	ivPartitionGranularities.SetAt(nIndex, nValue);

	// Parametrage de la taille du vecteur de correspondance
	ivBuiltPartIndexes = cast(IntVector*, oaPartitionBuiltPartIndexes.GetAt(nIndex));
	ivBuiltPartIndexes->SetSize(nValue);
}

int KDConstructedPartition::GetGranularityAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetDimensionNumber());
	return ivPartitionGranularities.GetAt(nIndex);
}

void KDConstructedPartition::SetDimensionAt(int nIndex, const KDConstructedPartitionDimension* dimension)
{
	require(GetPartitionClass() != NULL);
	require(0 <= nIndex and nIndex < GetDimensionNumber());
	oaPartitionDimensions.SetAt(nIndex, cast(Object*, dimension));
}

const KDConstructedPartitionDimension* KDConstructedPartition::GetDimensionAt(int nIndex) const
{
	require(GetPartitionClass() != NULL);
	require(0 <= nIndex and nIndex < GetDimensionNumber());
	return cast(const KDConstructedPartitionDimension*, oaPartitionDimensions.GetAt(nIndex));
}

int KDConstructedPartition::GetPartitionSize() const
{
	int nSize;
	int nIndex;

	// Calcul du produit des tailles par dimension
	nSize = 1;
	for (nIndex = 0; nIndex < GetDimensionNumber(); nIndex++)
		nSize *= GetGranularityAt(nIndex);
	return nSize;
}

int KDConstructedPartition::GetActualPartitionSize() const
{
	int nSize;
	int nIndex;
	const KDConstructedPartitionDimension* partitionDimension;
	const KDClassSelectionOperandStats* classSelectionOperandStats;
	int nGranularity;
	int nGranularityIndex;

	// Calcul du produit des tailles par dimension
	nSize = 1;
	for (nIndex = 0; nIndex < GetDimensionNumber(); nIndex++)
	{
		partitionDimension = GetDimensionAt(nIndex);
		classSelectionOperandStats = partitionDimension->GetClassSelectionOperandStats();
		nGranularity = GetGranularityAt(nIndex);
		nGranularityIndex = classSelectionOperandStats->SearchGranularityIndex(nGranularity);

		// Prise en compte des particles utiles par dimension
		nSize *= classSelectionOperandStats->GetPartsAt(nGranularityIndex)->GetSize();
	}
	return nSize;
}

longint KDConstructedPartition::GetUseCount() const
{
	return lUseCount;
}

void KDConstructedPartition::IncrementUseCount()
{
	lUseCount++;
}

void KDConstructedPartition::IncrementUseCounts()
{
	int i;
	lUseCount++;
	for (i = 0; i < GetDimensionNumber(); i++)
		cast(KDConstructedPartitionDimension*, oaPartitionDimensions.GetAt(i))->IncrementUseCount();
}

void KDConstructedPartition::ResetUseCount()
{
	lUseCount = 0;
}

void KDConstructedPartition::ResetAllPartsUseCounts()
{
	POSITION position;
	KDConstructedPart* part;

	position = slSortedParts.GetHeadPosition();
	while (position != NULL)
	{
		part = cast(KDConstructedPart*, slSortedParts.GetNext(position));
		part->ResetUseCount();
	}
}

KDConstructedPart* KDConstructedPartition::AddPart(const IntVector* ivPartileIndexes)
{
	KDConstructedPart* newPart;

	require(ivPartileIndexes != NULL);
	require(ivPartileIndexes->GetSize() == GetDimensionNumber());
	require(LookupPart(ivPartileIndexes) == NULL);

	// Creation de la nouvelle partie
	newPart = new KDConstructedPart;
	newPart->SetPartition(this);
	newPart->ivPartilesIndexes.CopyFrom(ivPartileIndexes);

	// Insertion dans la partition
	slSortedParts.Add(newPart);
	require(LookupPart(ivPartileIndexes) == newPart);
	return newPart;
}

KDConstructedPart* KDConstructedPartition::LookupPart(const IntVector* ivPartileIndexes) const
{
	KDConstructedPart searchedPart;
	POSITION searchedPosition;

	// Recherche de la position dans la liste triee des parties
	searchedPart.SetPartition(cast(KDConstructedPartition*, this));
	searchedPart.ivPartilesIndexes.CopyFrom(ivPartileIndexes);
	searchedPosition = slSortedParts.Find(&searchedPart);

	// On retourne la partie trouvee,
	if (searchedPosition != NULL)
		return cast(KDConstructedPart*, slSortedParts.GetAt(searchedPosition));
	// Ou NULL si non trouvee
	else
		return NULL;
}

int KDConstructedPartition::GetPartNumber() const
{
	return slSortedParts.GetCount();
}

void KDConstructedPartition::ExportParts(ObjectArray* oaExportedParts) const
{
	require(oaExportedParts != NULL);
	slSortedParts.ExportObjectArray(oaExportedParts);
}

KWDerivationRule* KDConstructedPartition::BuildPartitionDerivationRule() const
{
	KWDRPartition* partitionRule;
	KWDRIntervalBounds* intervalBoundsRule;
	KWDRValueGroups* valueGroupsRule;
	KWDRValueGroup* valueGroupRule;
	int nOperand;
	const KDConstructedPartitionDimension* partitionDimension;
	const KDClassSelectionOperandStats* classSelectionOperandStats;
	IntVector* ivBuiltPartIndexes;
	int nGranularity;
	int nGranularityIndex;
	int nPreviousGranularityIndex;
	int nPart;
	KDSelectionInterval* selectionInterval;
	ContinuousVector cvIntervalBounds;
	Continuous cLastBound;
	int nBound;
	KDSelectionValue* selectionValue;
	SymbolVector svValues;
	int i;

	require(Check());

	// Creation de la regle de partition
	partitionRule = new KWDRPartition;
	partitionRule->SetOperandNumber(0);

	// Parcours des dimensions de la partition
	for (nOperand = 0; nOperand < GetDimensionNumber(); nOperand++)
	{
		partitionDimension = GetDimensionAt(nOperand);
		classSelectionOperandStats = partitionDimension->GetClassSelectionOperandStats();
		nGranularity = GetGranularityAt(nOperand);
		nGranularityIndex = classSelectionOperandStats->SearchGranularityIndex(nGranularity);

		// Acces au vecteur de correspondance entre parties theoriques et construites
		ivBuiltPartIndexes = cast(IntVector*, oaPartitionBuiltPartIndexes.GetAt(nOperand));

		// Reinitialisation du vecteur
		ivPartitionBuiltPartNumbers.SetAt(nOperand, 0);
		ivBuiltPartIndexes->Initialize();

		// Creation d'une regle pour les dimension Continuous
		if (partitionDimension->GetType() == KWType::Continuous)
		{
			intervalBoundsRule = new KWDRIntervalBounds;
			partitionRule->AddUnivariatePartitionOperand(intervalBoundsRule);

			// Recherche des bornes des intervalles
			// Attention, les intervalles de l'operandes de selection ne sont pas necessairement adjacents,
			// et il faut donc collecter toutes les bornes possibles
			cLastBound = KWContinuous::GetMissingValue();
			for (nPart = 0; nPart < classSelectionOperandStats->GetPartsAt(nGranularityIndex)->GetSize();
			     nPart++)
			{
				selectionInterval =
				    cast(KDSelectionInterval*,
					 classSelectionOperandStats->GetPartsAt(nGranularityIndex)->GetAt(nPart));

				// Cas particulier ou l'intervalle se limite a la valeur missing
				if (selectionInterval->GetUpperBound() == KWContinuous::GetMissingValue())
				{
					assert(cvIntervalBounds.GetSize() == 0);
					assert(cLastBound == KWContinuous::GetMissingValue());
					cvIntervalBounds.Add(KWContinuous::GetMissingValue());
				}

				// Ou cas ou la borne inf est missing, alors que ce n'est pas le premier intervalle
				// ce qui signifie que missing est exclu
				if (selectionInterval->GetIndex() > 0 and
				    selectionInterval->GetLowerBound() == KWContinuous::GetMissingValue() and
				    cvIntervalBounds.GetSize() == 0)
				{
					assert(cLastBound == KWContinuous::GetMissingValue());
					cvIntervalBounds.Add(KWContinuous::GetMissingValue());
				}

				// Ajout des bornes non deja presentes
				if (selectionInterval->GetLowerBound() != cLastBound)
				{
					cLastBound = selectionInterval->GetLowerBound();
					cvIntervalBounds.Add(cLastBound);
				}
				if (selectionInterval->GetUpperBound() != cLastBound and
				    selectionInterval->GetUpperBound() != KWContinuous::GetMaxValue())
				{
					cLastBound = selectionInterval->GetUpperBound();
					cvIntervalBounds.Add(cLastBound);
				}

				// Memorisation de la correspondance entre index de partie theorique et construite
				if (selectionInterval->GetUpperBound() > cLastBound)
					ivBuiltPartIndexes->SetAt(selectionInterval->GetIndex(),
								  cvIntervalBounds.GetSize());
				else
					ivBuiltPartIndexes->SetAt(selectionInterval->GetIndex(),
								  cvIntervalBounds.GetSize() - 1);
			}

			// Parametrage des bornes des intervalles dans la regle
			intervalBoundsRule->SetIntervalBoundNumber(cvIntervalBounds.GetSize());
			for (nBound = 0; nBound < intervalBoundsRule->GetIntervalBoundNumber(); nBound++)
				intervalBoundsRule->SetIntervalBoundAt(nBound, cvIntervalBounds.GetAt(nBound));
			cvIntervalBounds.SetSize(0);

			// Memorisation du nombre de parties construites
			ivPartitionBuiltPartNumbers.SetAt(nOperand, intervalBoundsRule->GetPartNumber());
		}
		// Creation d'une regle pour les dimension Symbol
		else
		{
			assert(partitionDimension->GetType() == KWType::Symbol);
			valueGroupsRule = new KWDRValueGroups;
			partitionRule->AddUnivariatePartitionOperand(valueGroupsRule);

			// Initialisation du nombre de groupes
			valueGroupsRule->SetValueGroupNumber(
			    classSelectionOperandStats->GetPartsAt(nGranularityIndex)->GetSize());

			// Ajout d'un groupe par valeur, plus le groupe poubelle
			for (nPart = 0; nPart < classSelectionOperandStats->GetPartsAt(nGranularityIndex)->GetSize();
			     nPart++)
			{
				selectionValue =
				    cast(KDSelectionValue*,
					 classSelectionOperandStats->GetPartsAt(nGranularityIndex)->GetAt(nPart));
				valueGroupRule = valueGroupsRule->GetValueGroupAt(nPart);

				// Cas du dernier groupe
				if (nPart == classSelectionOperandStats->GetPartsAt(nGranularityIndex)->GetSize() - 1)
				{
					valueGroupRule->SetValueNumber(2);
					valueGroupRule->SetValueAt(0, selectionValue->GetValue());
					valueGroupRule->SetValueAt(1, Symbol::GetStarValue());
				}
				// Cas des autres groupes, necessairement singletons
				else
				{
					assert(not selectionValue->IsGarbagePart());
					valueGroupRule->SetValueNumber(1);
					valueGroupRule->SetValueAt(0, selectionValue->GetValue());
				}

				// Memorisation de la correspondance entre index de partie theorique et construite
				ivBuiltPartIndexes->SetAt(selectionValue->GetIndex(), nPart);
			}

			// On doit egalement eventuellement ajouter dans un dernier groupe les valeurs singletons des
			// granularites precedentes Ce dernier groupes doit "capturer" des valeurs singletons
			// precenednte, de facon a ce que le groupe poubelle ne capture que les modalites peu frequentes
			if (nGranularityIndex > 0)
			{
				// Ajout d'un dernier groupe
				valueGroupsRule->SetValueGroupNumber(valueGroupsRule->GetValueGroupNumber() + 1);
				valueGroupRule =
				    valueGroupsRule->GetValueGroupAt(valueGroupsRule->GetValueGroupNumber() - 1);

				// Ajout d'une valeur par valeur non poubelle des granularites precedentes
				for (nPreviousGranularityIndex = 0; nPreviousGranularityIndex < nGranularityIndex;
				     nPreviousGranularityIndex++)
				{
					for (nPart = 0;
					     nPart < classSelectionOperandStats->GetPartsAt(nPreviousGranularityIndex)
							 ->GetSize();
					     nPart++)
					{
						selectionValue =
						    cast(KDSelectionValue*, classSelectionOperandStats
										->GetPartsAt(nPreviousGranularityIndex)
										->GetAt(nPart));

						// Ajout de la valeur, sauf s'il s'agit de la valeur pouvelle
						if (not selectionValue->IsGarbagePart())
						{
							assert(svValues.GetSize() == 0 or
							       svValues.GetAt(svValues.GetSize() - 1) !=
								   selectionValue->GetValue());
							svValues.Add(selectionValue->GetValue());
						}
					}
				}

				// Parametrages des valeurs du dernier groupe
				valueGroupRule->SetValueNumber(svValues.GetSize());
				for (i = 0; i < svValues.GetSize(); i++)
					valueGroupRule->SetValueAt(i, svValues.GetAt(i));
				svValues.SetSize(0);
			}

			// Memorisation du nombre de parties construites
			ivPartitionBuiltPartNumbers.SetAt(nOperand, valueGroupsRule->GetPartNumber());
		}
	}
	ensure(partitionRule->GetOperandNumber() == GetDimensionNumber());
	return partitionRule;
}

KWDerivationRule* KDConstructedPartition::BuildTablePartitionBlockDerivationRule() const
{
	KWDRTablePartition* tablePartitionRule;
	int nOperand;
	const KDConstructedPartitionDimension* partitionDimension;
	KWDerivationRuleOperand* dimensionOperand;

	require(Check());
	require(GetPartitionAttribute() != NULL);

	// Creation de la regle de partition
	tablePartitionRule = new KWDRTablePartition;

	// Parametrage du premier operande: la table secondaire
	tablePartitionRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	tablePartitionRule->GetFirstOperand()->SetAttributeName(GetTableAttribute()->GetName());
	tablePartitionRule->GetFirstOperand()->SetObjectClassName(GetPartitionClass()->GetName());

	// Parametrage du deuxieme operande: la partition
	tablePartitionRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	tablePartitionRule->GetSecondOperand()->SetAttributeName(GetPartitionAttribute()->GetName());

	// Parcours des dimensions de la partition
	for (nOperand = 0; nOperand < GetDimensionNumber(); nOperand++)
	{
		partitionDimension = GetDimensionAt(nOperand);

		// Recherche de l'operande de la regle a parametrer, en la creant si necessaire
		if (nOperand == 0)
			dimensionOperand = tablePartitionRule->GetOperandAt(2);
		else
		{
			dimensionOperand = new KWDerivationRuleOperand;
			tablePartitionRule->AddOperand(dimensionOperand);
		}

		// Parametrage de l'operande
		assert(partitionDimension->GetSelectionAttribute() != NULL);
		dimensionOperand->SetType(partitionDimension->GetType());
		dimensionOperand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		dimensionOperand->SetAttributeName(partitionDimension->GetSelectionAttribute()->GetName());
	}

	ensure(tablePartitionRule->GetOperandNumber() == 2 + GetDimensionNumber());
	return tablePartitionRule;
}

const ALString KDConstructedPartition::BuildPartitionAttributeName() const
{
	boolean bUseAttributeName = false;
	ALString sName;

	require(Check());

	// Initialisation du nom de partition d'apres le nom de la classe du dernier attribut du data path
	// car la partition peut etre reutilisee pour plusieurs partitions de table
	sName = "Partition";
	sName += '(';
	if (bUseAttributeName)
		sName += GetTableAttribute()->GetName();
	else
		sName += GetTableAttribute()->GetClass()->GetName();
	sName += ')';
	sName += " per ";

	// Ajout du nom de selection
	sName += BuildSelectionName();
	return sName;
}

const ALString KDConstructedPartition::BuildTablePartitionAttributeBlockName() const
{
	ALString sName;

	require(Check());

	// Initialisation du nom de partition d'apres le nom du dernier attribut du data path
	sName = "TablePartition";
	sName += '(';
	sName += GetTableAttribute()->GetName();
	sName += ')';
	sName += " per ";

	// Ajout du nom de selection
	sName += BuildSelectionName();
	return sName;
}

const ALString KDConstructedPartition::BuildSelectionName() const
{
	ALString sName;
	int nOperand;
	const KDConstructedPartitionDimension* partitionDimension;
	int nGranularity;

	require(Check());

	// Parcours des dimensions de la partition
	for (nOperand = 0; nOperand < GetDimensionNumber(); nOperand++)
	{
		partitionDimension = GetDimensionAt(nOperand);
		nGranularity = GetGranularityAt(nOperand);

		// Ajout du nom de la dimension et de sa granularite dans le nom
		if (nOperand > 0)
			sName += " and ";
		sName += partitionDimension->BuildInterpretableName();
		sName += '(';
		sName += IntToString(nGranularity);
		sName += ')';
	}
	return sName;
}

const KWAttribute* KDConstructedPartition::GetPartitionAttribute() const
{
	return kwaPartitionAttribute;
}

void KDConstructedPartition::SetPartitionAttribute(const KWAttribute* attribute)
{
	require(attribute == NULL or attribute->GetParentClass()->GetName() == GetParentClass()->GetName());
	kwaPartitionAttribute = attribute;
}

const KWAttributeBlock* KDConstructedPartition::GetTablePartitionAttributeBlock() const
{
	return kwabTablePartitionAttributeBlock;
}

void KDConstructedPartition::SetTablePartitionAttributeBlock(const KWAttributeBlock* attributeBlock)
{
	require(attributeBlock == NULL or attributeBlock->GetParentClass()->GetName() == GetParentClass()->GetName());
	kwabTablePartitionAttributeBlock = attributeBlock;
}

int KDConstructedPartition::ComputeVariableKeyIndex(const KDConstructedPart* constructedPart) const
{
	int nKeyIndex;
	int nFactor;
	int nDimension;
	IntVector* ivBuiltPartIndexes;
	int nPartIndex;
	int nBuildPartIndex;

	require(constructedPart != NULL);
	require(LookupPart(constructedPart->GetPartilesIndexes()) == constructedPart);

	// Calcul de l'index de la partie sur la base des dimensions
	nKeyIndex = 0;
	nFactor = 1;
	for (nDimension = 0; nDimension < GetDimensionNumber(); nDimension++)
	{
		assert(ivPartitionBuiltPartNumbers.GetAt(nDimension) <= GetGranularityAt(nDimension));

		// Calcul de la correspondance entre index de partie theorique et construite
		nPartIndex = constructedPart->GetPartilesIndexes()->GetAt(nDimension);
		ivBuiltPartIndexes = cast(IntVector*, oaPartitionBuiltPartIndexes.GetAt(nDimension));
		nBuildPartIndex = ivBuiltPartIndexes->GetAt(nPartIndex);

		// Calcul de l'index de cellule
		nKeyIndex += nFactor * nBuildPartIndex;
		nFactor *= ivPartitionBuiltPartNumbers.GetAt(nDimension);
	}
	nKeyIndex++;
	return nKeyIndex;
}

boolean KDConstructedPartition::Check() const
{
	boolean bOk = true;
	ALString sTmp;
	int i;
	ObjectArray oaParts;
	KDConstructedPart* part;
	KDConstructedPartitionDimension* previousDimension;
	KDConstructedPartitionDimension* currentDimension;
	int nCompare;

	// Test si la classe est renseignee
	if (bOk and GetPartitionClass() == NULL)
	{
		AddError("No dictionary specified");
		bOk = false;
	}

	// Test de validite de l'attribut de table secondaire
	if (bOk and GetTableAttribute() == NULL)
	{
		AddError("No table variable specified");
		bOk = false;
	}
	if (bOk and GetTableAttribute()->GetClass() != GetPartitionClass())
	{
		AddError("Dictionary " + GetPartitionClass()->GetName() + " not consistent that of table variable " +
			 GetTableAttribute()->GetName() + " (" + GetTableAttribute()->GetClass()->GetName() + ")");
		bOk = false;
	}

	// Test si la partition est renseignee
	if (bOk and GetDimensionNumber() == 0)
	{
		AddError("No variable specified");
		bOk = false;
	}

	// Test des dimensions et de leur granularite
	if (bOk)
	{
		for (i = 0; i < GetDimensionNumber(); i++)
		{
			// Test de la dimension
			if (GetDimensionAt(i) == NULL)
			{
				AddError(sTmp + "No dimension specified at index " + IntToString(i + 1));
				bOk = false;
				break;
			}
			// Test de la granularite
			else if (GetGranularityAt(i) == 0)
			{
				AddError(sTmp + "No granularity specified at index " + IntToString(i + 1));
				bOk = false;
				break;
			}
			// Test de l'ordonnancement des dimensions
			else if (i > 0)
			{
				// Comparaison avec la dimension precente
				previousDimension = cast(KDConstructedPartitionDimension*, GetDimensionAt(i - 1));
				currentDimension = cast(KDConstructedPartitionDimension*, GetDimensionAt(i));
				nCompare = previousDimension->Compare(currentDimension);
				if (nCompare >= 0)
				{
					if (nCompare == 0)
						AddError(sTmp + "Dimension " + IntToString(i) + " (" +
							 previousDimension->GetObjectLabel() + ") " +
							 "is equal to dimension " + IntToString(i + 1) + " (" +
							 currentDimension->GetObjectLabel() + ")");
					else
						AddError(sTmp + "Dimension " + IntToString(i) + " (" +
							 previousDimension->GetObjectLabel() + ") " +
							 "is wrongly before dimension " + IntToString(i + 1) + " (" +
							 currentDimension->GetObjectLabel() + ")");
					bOk = false;
					break;
				}
			}
		}
	}

	// Test des parties
	if (bOk)
	{
		ExportParts(&oaParts);
		for (i = 0; i < oaParts.GetSize(); i++)
		{
			part = cast(KDConstructedPart*, oaParts.GetAt(i));
			bOk = part->Check();
			if (not bOk)
				break;
		}
	}
	return bOk;
}

int KDConstructedPartition::Compare(const KDConstructedPartition* partition) const
{
	int nDiff;
	int i;

	require(GetPartitionClass() != NULL);
	require(Check());
	require(partition != NULL);
	require(partition->GetPartitionClass() != NULL);
	require(partition->Check());

	// Comparaison des classes de partition
	nDiff = GetPartitionClass()->GetName().Compare(partition->GetPartitionClass()->GetName());

	// Comparaison des classes a l'origine des attributs d'acces aux tables
	if (nDiff == 0)
		nDiff = GetParentClass()->GetName().Compare(partition->GetParentClass()->GetName());

	// Comparaison des attributs d'acces aux tables
	if (nDiff == 0)
		nDiff = GetTableAttribute()->GetName().Compare(partition->GetTableAttribute()->GetName());

	// Comparaison des tailles
	if (nDiff == 0)
		nDiff = GetDimensionNumber() - partition->GetDimensionNumber();

	// Comparaison des dimensions et de leur granularite
	if (nDiff == 0)
	{
		for (i = 0; i < GetDimensionNumber(); i++)
		{
			// Comparaison des granularites
			nDiff = GetGranularityAt(i) - partition->GetGranularityAt(i);
			if (nDiff != 0)
				break;

			// Comparaison des dimensions
			nDiff = GetDimensionAt(i)->Compare(partition->GetDimensionAt(i));
			if (nDiff != 0)
				break;
		}
	}
	return nDiff;
}

void KDConstructedPartition::Write(ostream& ost) const
{
	int i;
	const KDConstructedPartitionDimension* dimension;

	// Nom de la classe utilisee
	if (GetPartitionClass() != NULL)
	{
		// Nom de la classe
		ost << GetPartitionClass()->GetName();

		// Attribut d'acces a la table
		if (GetTableAttribute() != NULL)
			ost << " " << GetTableAttribute()->GetName() << " ";

		// Operandes
		ost << "(";
		for (i = 0; i < GetDimensionNumber(); i++)
		{
			if (i > 0)
				ost << ", ";

			// Granularite
			ost << "[" << GetGranularityAt(i) << "]";

			// Dimension
			dimension = GetDimensionAt(i);
			if (dimension != NULL)
				ost << *dimension;
		}
		ost << ")";
	}
}

void KDConstructedPartition::WriteDimensionAt(int nIndex, ostream& ost)
{
	const KDConstructedPartitionDimension* dimension;

	// Si la classe est specifiee
	if (GetPartitionClass() != NULL)
	{
		assert(0 <= nIndex and nIndex < GetDimensionNumber());

		// Dimension de type Attribute
		dimension = GetDimensionAt(nIndex);
		if (dimension != NULL)
			ost << *dimension;
	}
}

longint KDConstructedPartition::GetUsedMemory() const
{
	longint lUsedMemory;

	// Memooire de base utilisee
	lUsedMemory = sizeof(KDConstructedPartition);
	lUsedMemory += oaPartitionDimensions.GetUsedMemory() - sizeof(ObjectArray);
	lUsedMemory += ivPartitionGranularities.GetUsedMemory() - sizeof(IntVector);
	lUsedMemory += slSortedParts.GetOverallUsedMemory() - sizeof(SortedList);
	return lUsedMemory;
}

const ALString KDConstructedPartition::GetObjectLabel() const
{
	ALString sLabel;
	const KDConstructedPartitionDimension* dimension;
	int i;

	// Chemin d'acces a la partition
	if (GetTableAttribute() != NULL)
	{
		sLabel += "[";
		sLabel += GetParentClass()->GetName();
		sLabel += ".";
		sLabel += GetTableAttribute()->GetName();
		sLabel += "]";
	}

	// Nom de la classe utilisee
	if (GetPartitionClass() != NULL)
	{
		sLabel += ':' + GetPartitionClass()->GetName();

		// Dimensions
		sLabel += "(";
		for (i = 0; i < GetDimensionNumber(); i++)
		{
			if (i > 0)
				sLabel += ", ";

			// Dimension de type Attribute
			dimension = GetDimensionAt(i);
			if (dimension != NULL)
				sLabel += dimension->GetObjectLabel();

			// Granularite
			sLabel += "(";
			sLabel += IntToString(GetGranularityAt(i));
			sLabel += ")";
		}
		sLabel += ")";
	}
	return sLabel;
}

const ALString KDConstructedPartition::GetClassLabel() const
{
	return "Partition";
}

int KDConstructedPartitionCompare(const void* elem1, const void* elem2)
{
	int nResult;
	KDConstructedPartition* partition1;
	KDConstructedPartition* partition2;

	// Acces aux partitions
	partition1 = cast(KDConstructedPartition*, *(Object**)elem1);
	partition2 = cast(KDConstructedPartition*, *(Object**)elem2);

	// Comparaison
	nResult = partition1->Compare(partition2);
	return nResult;
}

//////////////////////////////////////////////////////////////////////////
// Classe KDConstructedPartitionDimension

const KWAttribute* KDConstructedPartitionDimension::GetAttribute() const
{
	require(GetOrigin() == Attribute);
	return cast(const KWAttribute*, oDimension);
}

const KDConstructedRule* KDConstructedPartitionDimension::GetRule() const
{
	require(GetOrigin() == Rule);
	return cast(const KDConstructedRule*, oDimension);
}

int KDConstructedPartitionDimension::GetOrigin() const
{
	return cOrigin;
}

int KDConstructedPartitionDimension::GetType() const
{
	if (GetOrigin() == Attribute)
		return GetAttribute()->GetType();
	else if (GetOrigin() == Rule)
		return GetRule()->GetType();
	else
		return KWType::Unknown;
}

const KDClassSelectionOperandStats* KDConstructedPartitionDimension::GetClassSelectionOperandStats() const
{
	return classSelectionOperandStats;
}

const KWAttribute* KDConstructedPartitionDimension::GetSelectionAttribute() const
{
	return classSelectionOperandStats->GetSelectionAttribute();
}

longint KDConstructedPartitionDimension::GetUseCount() const
{
	return lUseCount;
}

void KDConstructedPartitionDimension::IncrementUseCount()
{
	lUseCount++;
	if (GetOrigin() == Rule)
		cast(KDConstructedRule*, oDimension)->IncrementUseCounts();
}

void KDConstructedPartitionDimension::ResetUseCount()
{
	lUseCount = 0;
}

boolean KDConstructedPartitionDimension::Check() const
{
	boolean bOk = true;

	if (cOrigin == None)
	{
		AddError("No dimension specified");
		bOk = false;
	}
	return bOk;
}

int KDConstructedPartitionDimension::Compare(const KDConstructedPartitionDimension* partitionDimension) const
{
	int nResult;

	require(partitionDimension != NULL);

	// Comparaison des types de dimension
	nResult = GetOrigin() - partitionDimension->GetOrigin();

	// Comparaison des contenu des dimensions
	if (nResult == 0)
	{
		// Comparaison des noms d'attribut
		if (GetOrigin() == Attribute)
			nResult = GetAttribute()->GetName().Compare(partitionDimension->GetAttribute()->GetName());
		// Comparaison des regles
		else if (GetOrigin() == Rule)
			nResult = GetRule()->Compare(partitionDimension->GetRule());
	}
	return nResult;
}

void KDConstructedPartitionDimension::Write(ostream& ost) const
{
	// Dimension de type Attribute
	if (GetOrigin() == Attribute)
		ost << GetAttribute()->GetName();
	// Dimension de type Rule
	else if (GetOrigin() == Rule)
		ost << *GetRule();
}

longint KDConstructedPartitionDimension::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KDConstructedPartitionDimension);
	if (GetOrigin() == Rule)
		lUsedMemory += GetRule()->GetUsedMemory();
	return lUsedMemory;
}

const ALString KDConstructedPartitionDimension::GetObjectLabel() const
{
	// Dimension de type Attribute
	if (GetOrigin() == Attribute)
		return GetAttribute()->GetName();
	// Dimension de type Rule
	else if (GetOrigin() == Rule)
		return GetRule()->GetObjectLabel();
	else
		return "";
}

const ALString KDConstructedPartitionDimension::GetClassLabel() const
{
	return "Dimension";
}

KDConstructedPartitionDimension::KDConstructedPartitionDimension()
{
	oDimension = NULL;
	cOrigin = None;
	lUseCount = 0;
	classSelectionOperandStats = NULL;
}

KDConstructedPartitionDimension::~KDConstructedPartitionDimension()
{
	if (cOrigin == Rule)
		delete oDimension;
}

void KDConstructedPartitionDimension::SetAttribute(const KWAttribute* attribute)
{
	require(attribute != NULL);
	require(cOrigin == None);
	oDimension = attribute;
	cOrigin = Attribute;
}

void KDConstructedPartitionDimension::SetRule(const KDConstructedRule* rule)
{
	require(rule != NULL);
	require(cOrigin == None);
	oDimension = rule;
	cOrigin = Rule;
}

ALString KDConstructedPartitionDimension::BuildInterpretableName() const
{
	ALString sDimensionName;

	if (GetOrigin() == Attribute)
	{
		sDimensionName = GetAttribute()->GetName();
	}
	else
	{
		assert(GetOrigin() == Rule);

		// On se base sur le nom fabrique a partir de la regle, meme si un attribut de selection
		// existait deja (GetSelectionAttribute()->GetName())
		// En effet, soit l'attribut de selection ayant meme regle de derivation qu'un attribut construit
		// etait en Used, et dans ce cas il est deja utilise en tant qu'attribut et non en tant que rehle.
		// Soit il etait en unused, et dans ce cas, on utilsie le nom construit base sur la regle, meme
		// si dans sa formule, on reutilise le nom de l'attribut existant pour eviter de regenerer
		// un attribut intermediaire
		sDimensionName = GetRule()->BuildInterpretableName(false);
	}
	return sDimensionName;
}

void KDConstructedPartitionDimension::Clean()
{
	oDimension = NULL;
	cOrigin = None;
}

void KDConstructedPartitionDimension::SetClassSelectionOperandStats(const KDClassSelectionOperandStats* operandStats)
{
	classSelectionOperandStats = operandStats;
}

int KDConstructedPartitionDimensionCompare(const void* elem1, const void* elem2)
{
	int nResult;
	KDConstructedPartitionDimension* partitionDimension1;
	KDConstructedPartitionDimension* partitionDimension2;

	// Acces aux partitions
	partitionDimension1 = cast(KDConstructedPartitionDimension*, *(Object**)elem1);
	partitionDimension2 = cast(KDConstructedPartitionDimension*, *(Object**)elem2);

	// Comparaison
	nResult = partitionDimension1->Compare(partitionDimension2);
	return nResult;
}

//////////////////////////////////////////////////////////////////////////
// Classe KDConstructedPart

KDConstructedPartition* KDConstructedPart::GetPartition() const
{
	return partition;
}

const IntVector* KDConstructedPart::GetPartilesIndexes() const
{
	return &ivPartilesIndexes;
}

void KDConstructedPart::SetCost(double dValue)
{
	require(dValue >= 0);
	dCost = dValue;
}

double KDConstructedPart::GetCost() const
{
	return dCost;
}

longint KDConstructedPart::GetUseCount() const
{
	return lUseCount;
}

void KDConstructedPart::IncrementUseCount()
{
	lUseCount++;
}

void KDConstructedPart::ResetUseCount()
{
	lUseCount = 0;
}

KWDerivationRule* KDConstructedPart::BuildDerivationRule() const
{
	KWDerivationRule* selectionRule;
	KWDerivationRule* selectionRuleOperand;
	int nOperand;
	const KDConstructedPartitionDimension* partitionDimension;
	ObjectArray oaSelectionOperandRules;

	require(Check());

	// Parcours des operandes de selection
	oaSelectionOperandRules.SetSize(partition->GetDimensionNumber());
	for (nOperand = 0; nOperand < partition->GetDimensionNumber(); nOperand++)
	{
		partitionDimension = partition->GetDimensionAt(nOperand);

		// Creation d'une regle par operande de selection
		selectionRuleOperand = BuildSelectionRuleOperand(
		    partitionDimension, partition->GetGranularityAt(nOperand), GetPartilesIndexes()->GetAt(nOperand));
		oaSelectionOperandRules.SetAt(nOperand, selectionRuleOperand);
	}

	// Creation d'une regle de type conjonction en cas de plusieurs operandes
	if (oaSelectionOperandRules.GetSize() > 1)
		selectionRule = BuildSelectionRuleFromOperands(&oaSelectionOperandRules);
	else
		selectionRule = cast(KWDerivationRule*, oaSelectionOperandRules.GetAt(0));
	return selectionRule;
}

const ALString KDConstructedPart::BuildPartAttributeName() const
{
	ALString sPartAttributeName;

	require(Check());

	// Chemin d'acces a la selection
	sPartAttributeName = partition->GetTableAttribute()->GetName();

	// Ajout du nom de la selection
	sPartAttributeName += " where ";
	sPartAttributeName += BuildInterpretableSelectionName();
	return sPartAttributeName;
}

boolean KDConstructedPart::Check() const
{
	boolean bOk = true;
	int i;
	int nPartileIndex;

	assert(partition != NULL);
	assert(partition->GetDimensionNumber() == ivPartilesIndexes.GetSize());

	// Parcours des index de partiles
	for (i = 0; i < ivPartilesIndexes.GetSize(); i++)
	{
		nPartileIndex = ivPartilesIndexes.GetAt(i);
		if (nPartileIndex < 0 or nPartileIndex >= partition->GetGranularityAt(i))
		{
			partition->AddError(GetClassLabel() + " " + GetObjectLabel() + ": Wrong partile at index " +
					    IntToString(i));
			bOk = false;
			break;
		}
	}
	return bOk;
}

int KDConstructedPart::Compare(const KDConstructedPart* part) const
{
	int nDiff;
	int i;

	require(part != NULL);
	require(partition == part->partition);
	require(GetPartilesIndexes()->GetSize() == part->GetPartilesIndexes()->GetSize());

	// Comparaison des index de partiles
	nDiff = 0;
	for (i = 0; i < GetPartilesIndexes()->GetSize(); i++)
	{
		nDiff = GetPartilesIndexes()->GetAt(i) - part->GetPartilesIndexes()->GetAt(i);
		if (nDiff != 0)
			break;
	}
	return nDiff;
}

int KDConstructedPart::CompareCost(const KDConstructedPart* part) const
{
	int nDiff;
	longint lSortValue1;
	longint lSortValue2;
	const KDConstructedPartitionDimension* partitionDimension1;
	const KDConstructedPartitionDimension* partitionDimension2;
	int i;

	require(part != NULL);

	// On se base sur un comparaison a sept decimales pres pour le critere de cout
	// Sept decimal semble le bon niveau piur assurer la stabilite de la comparaison
	lSortValue1 = longint(floor(GetCost() * 1e7 + 0.5));
	lSortValue2 = longint(floor(part->GetCost() * 1e7 + 0.5));

	// Comparaison sur le cout
	nDiff = CompareLongint(lSortValue1, lSortValue2);

	// Comparaison des tailles de partitions
	if (nDiff == 0)
		nDiff = GetPartilesIndexes()->GetSize() - part->GetPartilesIndexes()->GetSize();

	// On verifie que l'origines de dimension sont correctement ordonnes
	assert(KDConstructedPartitionDimension::Attribute < KDConstructedPartitionDimension::Rule);

	// Comparaison des dimensions de partition
	if (nDiff == 0)
	{
		for (i = 0; i < GetPartilesIndexes()->GetSize(); i++)
		{
			// Acces aux dimensions
			partitionDimension1 = partition->GetDimensionAt(i);
			partitionDimension2 = part->partition->GetDimensionAt(i);

			// Comparaison des dimensions
			nDiff = partitionDimension1->GetOrigin() - partitionDimension2->GetOrigin();
			if (nDiff == 0)
			{
				// Comparaison ddans le cas de dimension d'origine regle
				if (partitionDimension1->GetOrigin() == KDConstructedPartitionDimension::Rule)
					nDiff =
					    partitionDimension1->GetRule()->CompareCost(partitionDimension2->GetRule());
			}

			// Arret si operandes differents
			if (nDiff != 0)
				break;
		}
	}
	return nDiff;
}

void KDConstructedPart::Write(ostream& ost) const
{
	int i;
	ost << "(";
	for (i = 0; i < ivPartilesIndexes.GetSize(); i++)
	{
		if (i > 0)
			ost << ", ";
		ost << *partition->GetDimensionAt(i);
		ost << " in {";
		ost << partition->GetGranularityAt(i);
		ost << ":";
		ost << ivPartilesIndexes.GetAt(i);
		ost << "}";
	}
	ost << ")";
}

void KDConstructedPart::WriteCostDetails(ostream& ost, const ALString& sTreePrefix) const
{
	int i;
	const KDConstructedPartitionDimension* partitionDimension;

	// Information de cout et de nom
	ost << sTreePrefix << "\t" << GetCost() << "\t" << *this << "\n";

	// Information par dimension
	for (i = 0; i < ivPartilesIndexes.GetSize(); i++)
	{
		partitionDimension = partition->GetDimensionAt(i);

		if (partitionDimension->GetOrigin() == KDConstructedPartitionDimension::Attribute)
			ost << sTreePrefix << ".D" << i + 1 << "\t\t" << partitionDimension->GetAttribute()->GetName()
			    << "\n";
		else if (partitionDimension->GetOrigin() == KDConstructedPartitionDimension::Rule)
			partitionDimension->GetRule()->WriteCostDetails(ost, sTreePrefix + ".D" + IntToString(i + 1));
	}
}

longint KDConstructedPart::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = sizeof(KDConstructedPart);
	lUsedMemory += ivPartilesIndexes.GetUsedMemory() - sizeof(IntVector);
	return lUsedMemory;
}

const ALString KDConstructedPart::GetObjectLabel() const
{
	return "Part";
}

const ALString KDConstructedPart::GetClassLabel() const
{
	ALString sLabel;
	int i;

	sLabel = "(";
	for (i = 0; i < ivPartilesIndexes.GetSize(); i++)
	{
		if (i > 0)
			sLabel += ", ";
		sLabel += IntToString(ivPartilesIndexes.GetAt(i));
	}
	sLabel += ")";
	return sLabel;
}

KDConstructedPart::KDConstructedPart()
{
	partition = NULL;
	dCost = 0;
	lUseCount = 0;
}

KDConstructedPart::~KDConstructedPart() {}

void KDConstructedPart::SetPartition(KDConstructedPartition* constructedPartition)
{
	require(constructedPartition != NULL);
	partition = constructedPartition;
	ivPartilesIndexes.SetSize(partition->GetDimensionNumber());
}

KWDerivationRule*
KDConstructedPart::BuildSelectionRuleOperand(const KDConstructedPartitionDimension* partitionDimension,
					     int nGranularity, int nPartIndex) const
{
	KWDerivationRule* selectionOperandRule;
	const KDClassSelectionOperandStats* classSelectionOperandStats;
	const ALString sPartPrefix;
	int nGranularityIndex;
	const ObjectArray* oaSelectionParts;
	KDSelectionPart* selectionPart;
	KDSelectionInterval* selectionInterval;
	KDSelectionValue* selectionValue;
	KWDRValueGroup* valueGroup;
	KWDRInGroup* inGroupRule;
	KWDRIntervalBounds* intervalBounds;
	KWDRInInterval* inIntervalRule;
	int nValue;

	require(partitionDimension != NULL);
	require(2 <= nGranularity);
	require(0 <= nPartIndex and nPartIndex < nGranularity);

	// Acces aux stats de l'operande de selection correspondant a la dimension de partition
	classSelectionOperandStats = partitionDimension->GetClassSelectionOperandStats();

	// Recherche de l'index de la granularite
	nGranularityIndex = classSelectionOperandStats->SearchGranularityIndex(nGranularity);
	assert(nGranularityIndex != -1);

	// Recherche de la partie de selection concernee
	oaSelectionParts = classSelectionOperandStats->GetPartsAt(nGranularityIndex);
	selectionPart = classSelectionOperandStats->SearchPart(oaSelectionParts, nPartIndex);
	assert(selectionPart != NULL);

	// Cas d'un operande de selection Symbol
	selectionOperandRule = NULL;
	if (partitionDimension->GetType() == KWType::Symbol)
	{
		// Acces a la valeur de selection
		selectionValue = cast(KDSelectionValue*, selectionPart);

		// Cas d'une valeur de selection standard
		if (not selectionValue->IsGarbagePart())
		{
			// Creation de la regle resultat
			selectionOperandRule = new KWDRSymbolEQ;

			// Parametrage de l'operande de selection
			FillOperandFromPartitionDimension(selectionOperandRule->GetFirstOperand(), partitionDimension);

			// Parametrage de la partie de selection
			selectionOperandRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
			selectionOperandRule->GetSecondOperand()->SetSymbolConstant(selectionValue->GetValue());
		}
		// Cas de la valeur poubelle: on cree un groupement de valeurs pour la poubelle et on utilise
		// une regle de type Not(InGroup(PoubelleValueGroup, value))
		else
		{
			// Creation de la regle resultat
			selectionOperandRule = new KWDRNot;

			// Creation de la regle d'appartenance au groupe
			inGroupRule = new KWDRInGroup;

			// Creation du groupe de valeurs
			valueGroup = new KWDRValueGroup;
			valueGroup->SetValueNumber(selectionValue->GetOutsideValues()->GetSize());

			// Parametrage des valeurs
			for (nValue = 0; nValue < selectionValue->GetOutsideValues()->GetSize(); nValue++)
				valueGroup->SetValueAt(nValue, selectionValue->GetOutsideValues()->GetAt(nValue));

			// Parametrage de l'operande de selection, en deuxieme operande
			FillOperandFromPartitionDimension(inGroupRule->GetSecondOperand(), partitionDimension);

			// Parametrage de la partie de selection (groupement de valeurs), en premier operande
			inGroupRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
			inGroupRule->GetFirstOperand()->SetDerivationRule(valueGroup);

			// Parametrage de la partie dans la regle de selection
			selectionOperandRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
			selectionOperandRule->GetFirstOperand()->SetDerivationRule(inGroupRule);
		}
	}
	// Cas d'un operande de selection Continuous
	else
	{
		assert(partitionDimension->GetType() == KWType::Continuous);

		// Acces a l'intervalle de selection
		selectionInterval = cast(KDSelectionInterval*, selectionPart);
		assert(selectionInterval->GetLowerBound() != KWContinuous::GetMissingValue() or
		       selectionInterval->GetUpperBound() != KWContinuous::GetMaxValue() or
		       selectionInterval->GetIndex() > 0);

		// Cas d'un intervalle de type Missing
		if (selectionInterval->GetUpperBound() == KWContinuous::GetMissingValue())
		{
			selectionOperandRule = new KWDREQ;

			// Parametrage de l'operande de selection
			FillOperandFromPartitionDimension(selectionOperandRule->GetFirstOperand(), partitionDimension);

			// Parametrage de la partie de selection
			selectionOperandRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
			selectionOperandRule->GetSecondOperand()->SetContinuousConstant(
			    selectionInterval->GetUpperBound());
		}
		// Cas d'un intervalle de type not Missing
		else if (selectionInterval->GetLowerBound() == KWContinuous::GetMissingValue() and
			 selectionInterval->GetUpperBound() == KWContinuous::GetMaxValue())
		{
			selectionOperandRule = new KWDRNEQ;

			// Parametrage de l'operande de selection
			FillOperandFromPartitionDimension(selectionOperandRule->GetFirstOperand(), partitionDimension);

			// Parametrage de la partie de selection
			selectionOperandRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
			selectionOperandRule->GetSecondOperand()->SetContinuousConstant(
			    selectionInterval->GetLowerBound());
		}
		// Ajout d'un intervalle ouvert a droite
		else if (selectionInterval->GetLowerBound() != KWContinuous::GetMissingValue() and
			 selectionInterval->GetUpperBound() == KWContinuous::GetMaxValue())
		{
			selectionOperandRule = new KWDRG;

			// Parametrage de l'operande de selection
			FillOperandFromPartitionDimension(selectionOperandRule->GetFirstOperand(), partitionDimension);

			// Parametrage de la partie de selection
			selectionOperandRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
			selectionOperandRule->GetSecondOperand()->SetContinuousConstant(
			    selectionInterval->GetLowerBound());
		}
		// Ajout d'un intervalle ouvert a gauche (premier intervalle (index=0), donc pas de missinf)
		else if (selectionInterval->GetLowerBound() == KWContinuous::GetMissingValue() and
			 selectionInterval->GetUpperBound() != KWContinuous::GetMaxValue() and
			 selectionInterval->GetIndex() == 0)
		{
			selectionOperandRule = new KWDRLE;

			// Parametrage de l'operande de selection
			FillOperandFromPartitionDimension(selectionOperandRule->GetFirstOperand(), partitionDimension);

			// Parametrage de la partie de selection
			selectionOperandRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
			selectionOperandRule->GetSecondOperand()->SetContinuousConstant(
			    selectionInterval->GetUpperBound());
		}
		// Ajout d'un intervalle "standard"
		else
		{
			assert((selectionInterval->GetLowerBound() != KWContinuous::GetMissingValue() or
				selectionInterval->GetIndex() > 0) and
			       selectionInterval->GetUpperBound() != KWContinuous::GetMaxValue());
			inIntervalRule = new KWDRInInterval;
			selectionOperandRule = inIntervalRule;

			// Creation d'un intervalle avec les deux bornes
			intervalBounds = new KWDRIntervalBounds;
			intervalBounds->SetIntervalBoundNumber(2);
			intervalBounds->SetIntervalBoundAt(0, selectionInterval->GetLowerBound());
			intervalBounds->SetIntervalBoundAt(1, selectionInterval->GetUpperBound());

			// Parametrage de l'operande de selection
			FillOperandFromPartitionDimension(selectionOperandRule->GetSecondOperand(), partitionDimension);

			// Parametrage de la partie de selection (intervalle), en premier operande
			inIntervalRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
			inIntervalRule->GetFirstOperand()->SetDerivationRule(intervalBounds);
		}
	}
	ensure(selectionOperandRule != NULL);
	return selectionOperandRule;
}

void KDConstructedPart::FillOperandFromPartitionDimension(
    KWDerivationRuleOperand* operand, const KDConstructedPartitionDimension* partitionDimension) const
{
	const KWAttribute* ruleDerivedAttribute;

	require(operand != NULL);
	require(partitionDimension != NULL);

	// Cas d'un operande attribut
	if (partitionDimension->GetOrigin() == KDConstructedPartitionDimension::Attribute)
	{
		operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		operand->SetAttributeName(partitionDimension->GetAttribute()->GetName());
	}
	// Cas d'un operande regle
	else
	{
		assert(partitionDimension->GetOrigin() == KWDerivationRuleOperand::OriginRule);

		// Acces a l'attribut derive correspondant a la regle s'il existe
		ruleDerivedAttribute = partitionDimension->GetSelectionAttribute();

		// Utilisation de cet attribut s'il existe
		if (ruleDerivedAttribute != NULL)
		{
			operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
			operand->SetAttributeName(ruleDerivedAttribute->GetName());
		}
		// Creation d'une regle sinon
		else
		{
			operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
			operand->SetDerivationRule(partitionDimension->GetRule()->BuildDerivationRule());
		}
	}
	operand->SetType(partitionDimension->GetType());
}

KWDRAnd* KDConstructedPart::BuildSelectionRuleFromOperands(ObjectArray* oaOperandRules) const
{
	KWDRAnd* selectionRule;
	int nOperand;
	KWDerivationRuleOperand* andOperand;
	KWDerivationRule* operandRule;

	require(oaOperandRules != NULL);
	require(oaOperandRules->GetSize() >= 1);

	// Creation de la regle
	selectionRule = new KWDRAnd;
	selectionRule->SetOperandNumber(0);

	// Ajout des operandes
	for (nOperand = 0; nOperand < oaOperandRules->GetSize(); nOperand++)
	{
		operandRule = cast(KWDerivationRule*, oaOperandRules->GetAt(nOperand));

		// Ajout de l'operande base sur la regle en parametre
		andOperand = new KWDerivationRuleOperand;
		selectionRule->AddOperand(andOperand);
		andOperand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		andOperand->SetDerivationRule(operandRule);
		andOperand->SetType(operandRule->GetType());
	}
	ensure(selectionRule->GetOperandNumber() == oaOperandRules->GetSize());
	return selectionRule;
}

ALString KDConstructedPart::BuildInterpretableSelectionName() const
{
	ALString sSelectionName;
	int nOperand;
	const KDConstructedPartitionDimension* partitionDimension;

	// Parcours des operandes de selection
	for (nOperand = 0; nOperand < partition->GetDimensionNumber(); nOperand++)
	{
		partitionDimension = partition->GetDimensionAt(nOperand);

		// Ajout d'un and si plusieurs operandes
		if (nOperand > 0)
			sSelectionName += " and ";

		// Creation d'une regle par operande de selection
		sSelectionName += BuildInterpretableSelectionOperandName(
		    partitionDimension, partition->GetGranularityAt(nOperand), GetPartilesIndexes()->GetAt(nOperand));
	}
	return sSelectionName;
}

ALString
KDConstructedPart::BuildInterpretableSelectionOperandName(const KDConstructedPartitionDimension* partitionDimension,
							  int nGranularity, int nPartIndex) const
{
	ALString sSelectionOperandName;
	ALString sOperandName;
	ALString sValue;
	const KDClassSelectionOperandStats* classSelectionOperandStats;
	int nGranularityIndex;
	const ObjectArray* oaSelectionParts;
	KDSelectionPart* selectionPart;
	KDSelectionInterval* selectionInterval;
	KDSelectionValue* selectionValue;
	int nValue;

	require(partitionDimension != NULL);
	require(2 <= nGranularity);
	require(0 <= nPartIndex and nPartIndex < nGranularity);

	// Acces aux stats de l'operande de selection correspondant a la dimension de partition
	classSelectionOperandStats = partitionDimension->GetClassSelectionOperandStats();

	// Recherche de l'index de la granularite
	nGranularityIndex = classSelectionOperandStats->SearchGranularityIndex(nGranularity);
	assert(nGranularityIndex != -1);

	// Recherche de la partie de selection concernee
	oaSelectionParts = classSelectionOperandStats->GetPartsAt(nGranularityIndex);
	selectionPart = classSelectionOperandStats->SearchPart(oaSelectionParts, nPartIndex);
	assert(selectionPart != NULL);

	// Nom de l'operand de selection
	sOperandName = partitionDimension->BuildInterpretableName();

	// Cas d'un operande de selection Symbol
	if (partitionDimension->GetType() == KWType::Symbol)
	{
		// Acces a la valeur de selection
		selectionValue = cast(KDSelectionValue*, selectionPart);

		// Cas d'une valeur de selection standard
		if (not selectionValue->IsGarbagePart())
		{
			// Cas particulier d'une valeur vide
			sValue = CleanValue(selectionValue->GetValue());
			if (sValue == "")
				sSelectionOperandName = sOperandName + " is empty";
			// Cas general avec valeur non vide
			else
				sSelectionOperandName = sOperandName + " = " + sValue;
		}
		// Cas de la valeur poubelle: on cree un groupement de valeurs pour la poubelle et on utilise
		// une regle de type Not(InGroup(PoubelleValueGroup, value))
		else
		{
			// Nom de la selection: cas d'une seule valeur
			if (selectionValue->GetOutsideValues()->GetSize() == 1)
			{
				sValue = CleanValue(selectionValue->GetOutsideValues()->GetAt(0));

				// Cas particulier d'une valeur vide
				if (sValue == "")
					sSelectionOperandName = sOperandName + " not empty";
				else
					sSelectionOperandName = sOperandName + " <> " + sValue;
			}
			// Cas de plusieurs valeurs dans le groupe
			else
			{
				sSelectionOperandName = sOperandName + " not in {";
				for (nValue = 0; nValue < selectionValue->GetOutsideValues()->GetSize(); nValue++)
				{
					if (nValue > 0)
						sSelectionOperandName += ", ";
					sValue = CleanValue(selectionValue->GetOutsideValues()->GetAt(nValue));
					sSelectionOperandName += sValue;
				}
				sSelectionOperandName += "}";
			}
		}
	}
	// Cas d'un operande de selection Continuous
	else
	{
		assert(partitionDimension->GetType() == KWType::Continuous);

		// Acces a l'intervalle de selection
		selectionInterval = cast(KDSelectionInterval*, selectionPart);
		assert(selectionInterval->GetLowerBound() != KWContinuous::GetMissingValue() or
		       selectionInterval->GetUpperBound() != KWContinuous::GetMaxValue() or
		       selectionInterval->GetIndex() > 0);

		// Cas d'un intervalle de type Missing
		if (selectionInterval->GetUpperBound() == KWContinuous::GetMissingValue())
		{
			sSelectionOperandName = sOperandName + " is missing";
		}
		// Cas d'un intervalle de type not Missing
		else if (selectionInterval->GetLowerBound() == KWContinuous::GetMissingValue() and
			 selectionInterval->GetUpperBound() == KWContinuous::GetMaxValue())
		{
			sSelectionOperandName = sOperandName + " not missing";
		}
		// Ajout d'un intervalle ouvert a droite
		else if (selectionInterval->GetLowerBound() != KWContinuous::GetMissingValue() and
			 selectionInterval->GetUpperBound() == KWContinuous::GetMaxValue())
		{
			sSelectionOperandName =
			    sOperandName + " > " + KWContinuous::ContinuousToString(selectionInterval->GetLowerBound());
		}
		// Ajout d'un intervalle ouvert a gauche (premier intervalle (index=0), donc pas de missing)
		else if (selectionInterval->GetLowerBound() == KWContinuous::GetMissingValue() and
			 selectionInterval->GetUpperBound() != KWContinuous::GetMaxValue() and
			 selectionInterval->GetIndex() == 0)
		{
			sSelectionOperandName = sOperandName + " <= " +
						KWContinuous::ContinuousToString(selectionInterval->GetUpperBound());
		}
		// Ajout d'un intervalle "standard"
		else
		{
			assert((selectionInterval->GetLowerBound() != KWContinuous::GetMissingValue() or
				selectionInterval->GetIndex() > 0) and
			       selectionInterval->GetUpperBound() != KWContinuous::GetMaxValue());

			// Nom de la selection, si la borne inferieure n'est pas missing
			if (selectionInterval->GetLowerBound() != KWContinuous::GetMissingValue())
			{
				sSelectionOperandName =
				    sOperandName + " in ]" +
				    KWContinuous::ContinuousToString(selectionInterval->GetLowerBound()) + ", " +
				    KWContinuous::ContinuousToString(selectionInterval->GetUpperBound()) + "]";
			}
			// si la borne inferieure est missing
			else
			{
				sSelectionOperandName =
				    sOperandName +
				    " <= " + KWContinuous::ContinuousToString(selectionInterval->GetUpperBound()) +
				    " (and not missing)";
			}
		}
	}
	return sSelectionOperandName;
}

ALString KDConstructedPart::CleanValue(const char* sValue) const
{
	ALString sCleanedValue;
	int i;

	// Supression des tabulation, puis trim
	sCleanedValue = sValue;
	for (i = 0; i < sCleanedValue.GetLength(); i++)
	{
		if (sCleanedValue.GetAt(i) == '\t')
			sCleanedValue.SetAt(i, ' ');
	}
	sCleanedValue.TrimLeft();
	sCleanedValue.TrimRight();
	return sCleanedValue;
}

int KDConstructedPartCompare(const void* elem1, const void* elem2)
{
	int nResult;
	KDConstructedPart* part1;
	KDConstructedPart* part2;
	int i;

	// Acces aux parties
	part1 = cast(KDConstructedPart*, *(Object**)elem1);
	part2 = cast(KDConstructedPart*, *(Object**)elem2);
	assert(part1->GetPartition() == part2->GetPartition());
	assert(part1->GetPartilesIndexes()->GetSize() == part2->GetPartilesIndexes()->GetSize());

	// Comparaison des index de partiles
	nResult = 0;
	for (i = 0; i < part1->GetPartilesIndexes()->GetSize(); i++)
	{
		nResult = part1->GetPartilesIndexes()->GetAt(i) - part2->GetPartilesIndexes()->GetAt(i);
		if (nResult != 0)
			break;
	}
	return nResult;
}
