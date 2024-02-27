// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRPreprocessing.h"

void KWDRRegisterPreprocessingRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRIntervalBounds);
	KWDerivationRule::RegisterDerivationRule(new KWDRContinuousValueSet);
	KWDerivationRule::RegisterDerivationRule(new KWDRValueGroup);
	KWDerivationRule::RegisterDerivationRule(new KWDRValueGroups);
	KWDerivationRule::RegisterDerivationRule(new KWDRSymbolValueSet);
	//
	KWDerivationRule::RegisterDerivationRule(new KWDRInInterval);
	KWDerivationRule::RegisterDerivationRule(new KWDRInGroup);
	//
	KWDerivationRule::RegisterDerivationRule(new KWDRIntervalId);
	KWDerivationRule::RegisterDerivationRule(new KWDRContinuousValueId);
	KWDerivationRule::RegisterDerivationRule(new KWDRGroupId);
	KWDerivationRule::RegisterDerivationRule(new KWDRSymbolValueId);
	//
	KWDerivationRule::RegisterDerivationRule(new KWDRIntervalIndex);
	KWDerivationRule::RegisterDerivationRule(new KWDRContinuousValueIndex);
	KWDerivationRule::RegisterDerivationRule(new KWDRGroupIndex);
	KWDerivationRule::RegisterDerivationRule(new KWDRSymbolValueIndex);
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRUnivariatePartition

int KWDRUnivariatePartition::GetContinuousPartIndex(Continuous cValue) const
{
	assert(false);
	return 0;
}

int KWDRUnivariatePartition::GetSymbolPartIndex(const Symbol& sValue) const
{
	assert(false);
	return 0;
}

const ALString KWDRUnivariatePartition::GetPartLabelAt(int nIndex) const
{
	return "";
}

///////////////////////////////////////////////////////////////
// Classe KWDRIntervalBounds

KWDRIntervalBounds::KWDRIntervalBounds()
{
	SetName("IntervalBounds");
	SetLabel("Interval bounds");
	SetStructureName("IntervalBounds");
	SetOperandNumber(1);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Continuous);
	GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRIntervalBounds::~KWDRIntervalBounds() {}

void KWDRIntervalBounds::SetIntervalBoundNumber(int nValue)
{
	require(nValue >= 0);

	cvIntervalBounds.SetSize(nValue);
	bStructureInterface = true;
	nFreshness++;
}

int KWDRIntervalBounds::GetIntervalBoundNumber() const
{
	if (bStructureInterface)
		return cvIntervalBounds.GetSize();
	else
		return GetOperandNumber();
}

void KWDRIntervalBounds::SetIntervalBoundAt(int nIndex, Continuous cBound)
{
	require(0 <= nIndex and nIndex < cvIntervalBounds.GetSize());
	require(bStructureInterface);

	cvIntervalBounds.SetAt(nIndex, cBound);
	nFreshness++;
}

Continuous KWDRIntervalBounds::GetIntervalBoundAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < cvIntervalBounds.GetSize());
	require(bStructureInterface);

	return cvIntervalBounds.GetAt(nIndex);
}

int KWDRIntervalBounds::GetPartNumber() const
{
	return GetIntervalBoundNumber() + 1;
}

int KWDRIntervalBounds::GetAttributeType() const
{
	return KWType::Continuous;
}

int KWDRIntervalBounds::GetContinuousPartIndex(Continuous cValue) const
{
	return ComputeIntervalIndex(cValue);
}

const ALString KWDRIntervalBounds::GetPartLabelAt(int nIndex) const
{
	ALString sPartLabel;
	boolean bMissingValue;

	require(Check());
	require(IsCompiled());
	require(bStructureInterface);

	// Renvoie vide si index hors bornes
	if (0 <= nIndex and nIndex < GetPartNumber())
	{
		// Flag de presence de la valeur manquante dans la discretisation
		bMissingValue =
		    cvIntervalBounds.GetSize() >= 1 and cvIntervalBounds.GetAt(0) == KWContinuous::GetMissingValue();

		// Cas particulier d'un intervalle reduit a la valeur manquante
		if (nIndex == 0 and bMissingValue)
			sPartLabel = "Missing";
		// Cas standard
		else
		{
			if (nIndex == 0 or (nIndex == 1 and bMissingValue))
				sPartLabel = sPartLabel + "]-inf;";
			else
				sPartLabel = sPartLabel + "]" +
					     KWContinuous::ContinuousToString(cvIntervalBounds.GetAt(nIndex - 1)) + ";";
			if (nIndex == GetPartNumber() - 1)
				sPartLabel = sPartLabel + "+inf[";
			else
				sPartLabel =
				    sPartLabel + KWContinuous::ContinuousToString(cvIntervalBounds.GetAt(nIndex)) + "]";
		}
		return sPartLabel;
	}
	return "";
}

int KWDRIntervalBounds::ComputeIntervalIndex(Continuous cValue) const
{
	int nIndex;

	require(Check());
	require(IsCompiled());
	require(bStructureInterface);

	// Recherche de l'index de l'intervalle de discretization
	// Recherche sequentielle s'il y a peu d'intervalles
	if (cvIntervalBounds.GetSize() <= 10)
	{
		for (nIndex = 0; nIndex < cvIntervalBounds.GetSize(); nIndex++)
		{
			if (cValue <= cvIntervalBounds.GetAt(nIndex))
				return nIndex;
		}
		assert(nIndex == cvIntervalBounds.GetSize());
		return nIndex;
	}
	// Recherche dichotomique sinon
	else
	{
		int nLowerIndex;
		int nUpperIndex;

		// Initialisation des index extremites
		nLowerIndex = 0;
		nUpperIndex = cvIntervalBounds.GetSize() - 1;

		// Recherche dichotomique de l'intervalle
		nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
		while (nLowerIndex + 1 < nUpperIndex)
		{
			// Deplacement des bornes de recherche en fonction
			// de la comparaison avec la borne courante
			if (cValue <= cvIntervalBounds.GetAt(nIndex))
				nUpperIndex = nIndex;
			else
				nLowerIndex = nIndex;

			// Modification du prochain intervalle teste
			nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
		}
		assert(nLowerIndex <= nUpperIndex);
		assert(nUpperIndex <= nLowerIndex + 1);

		// On compare par rapport aux deux index restant
		if (cValue <= cvIntervalBounds.GetAt(nLowerIndex))
			nIndex = nLowerIndex;
		else if (cValue > cvIntervalBounds.GetAt(nUpperIndex))
			nIndex = nUpperIndex + 1;
		else
			nIndex = nUpperIndex;

		// On retourne le resultat
		assert(nIndex == cvIntervalBounds.GetSize() or cValue <= cvIntervalBounds.GetAt(nIndex));
		assert(nIndex == 0 or cValue > cvIntervalBounds.GetAt(nIndex - 1));
		return nIndex;
	}
}

void KWDRIntervalBounds::ImportAttributeDiscretization(const KWDGSAttributeDiscretization* attributeDiscretization)
{
	int nBoundNumber;
	int nBound;

	require(attributeDiscretization != NULL);

	// Reinitialisation
	DeleteAllOperands();

	// Parametrage des intervalles
	nBoundNumber = attributeDiscretization->GetIntervalBoundNumber();
	SetIntervalBoundNumber(nBoundNumber);
	for (nBound = 0; nBound < nBoundNumber; nBound++)
		SetIntervalBoundAt(nBound, attributeDiscretization->GetIntervalBoundAt(nBound));
}

void KWDRIntervalBounds::ExportAttributeDiscretization(KWDGSAttributeDiscretization* attributeDiscretization) const
{
	int nBoundNumber;
	int nBound;

	require(IsCompiled());
	require(attributeDiscretization != NULL);

	// Parametrage des intervalles
	nBoundNumber = GetIntervalBoundNumber();
	attributeDiscretization->SetPartNumber(nBoundNumber + 1);
	for (nBound = 0; nBound < nBoundNumber; nBound++)
		attributeDiscretization->SetIntervalBoundAt(nBound, GetIntervalBoundAt(nBound));
}

KWDerivationRule* KWDRIntervalBounds::Create() const
{
	return new KWDRIntervalBounds;
}

void KWDRIntervalBounds::CleanCompiledBaseInterface()
{
	DeleteAllOperands();
}

void KWDRIntervalBounds::CopyStructureFrom(const KWDerivationRule* kwdrSource)
{
	const KWDRIntervalBounds* kwdribSource = cast(KWDRIntervalBounds*, kwdrSource);

	// Copie de la version optimisee du parametrage des bornes de discretisation
	cvIntervalBounds.CopyFrom(&kwdribSource->cvIntervalBounds);
}

void KWDRIntervalBounds::BuildStructureFromBase(const KWDerivationRule* kwdrSource)
{
	int i;
	KWDerivationRuleOperand* operand;

	require(kwdrSource->KWDerivationRule::CheckDefinition());

	// Recopie des operandes
	cvIntervalBounds.SetSize(kwdrSource->GetOperandNumber());
	for (i = 0; i < kwdrSource->GetOperandNumber(); i++)
	{
		operand = kwdrSource->GetOperandAt(i);
		cvIntervalBounds.SetAt(i, operand->GetContinuousConstant());
	}
	bStructureInterface = true;
}

boolean KWDRIntervalBounds::CheckStructureDefinition() const
{
	boolean bOk = true;
	int i;

	// Test de l'ordre des bornes
	for (i = 1; i < cvIntervalBounds.GetSize(); i++)
	{
		if (cvIntervalBounds.GetAt(i - 1) > cvIntervalBounds.GetAt(i))
		{
			bOk = false;
			AddError("Discretization bounds are not correctly ordered");
			break;
		}
	}
	return bOk;
}

void KWDRIntervalBounds::WriteStructureUsedRule(ostream& ost) const
{
	int i;

	// Nom de la regle utilisee
	ost << KWClass::GetExternalName(GetName());

	// Operandes
	ost << "(";
	for (i = 0; i < cvIntervalBounds.GetSize(); i++)
	{
		if (i > 0)
			ost << ", ";
		ost << KWClass::GetExternalContinuousConstant(cvIntervalBounds.GetAt(i));
	}
	ost << ")";
}

int KWDRIntervalBounds::FullCompare(const KWDerivationRule* rule) const
{
	int nDiff;
	KWDRIntervalBounds* ruleIntervalBounds;
	int i;

	require(rule != NULL);

	// Comparaison sur la classe sur laquelle la regle est applicable
	nDiff = GetClassName().Compare(rule->GetClassName());

	// Comparaison sur le nom de la regle
	if (nDiff == 0)
		nDiff = GetName().Compare(rule->GetName());

	// En cas d'egalite, comparaison en utilisant l'interface de structure
	if (nDiff == 0)
	{
		ruleIntervalBounds = cast(KWDRIntervalBounds*, rule);
		assert(GetStructureInterface());
		assert(ruleIntervalBounds->GetStructureInterface());

		// Taille du vecteur
		nDiff = cvIntervalBounds.GetSize() - ruleIntervalBounds->cvIntervalBounds.GetSize();

		// Si egalite, comparaison sur les valeurs
		if (nDiff == 0)
		{
			for (i = 0; i < cvIntervalBounds.GetSize(); i++)
			{
				nDiff = KWContinuous::Compare(cvIntervalBounds.GetAt(i),
							      ruleIntervalBounds->cvIntervalBounds.GetAt(i));
				if (nDiff != 0)
					break;
			}
		}
	}

	return nDiff;
}

longint KWDRIntervalBounds::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRUnivariatePartition::GetUsedMemory();
	lUsedMemory += sizeof(KWDRIntervalBounds) - sizeof(KWDRUnivariatePartition);
	lUsedMemory += cvIntervalBounds.GetUsedMemory();
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////
// Classe KWDRContinuousValueSet

KWDRContinuousValueSet::KWDRContinuousValueSet()
{
	SetName("ValueSet");
	SetLabel("Numerical value set");
	SetStructureName("ValueSet");
	SetOperandNumber(1);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Continuous);
	GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRContinuousValueSet::~KWDRContinuousValueSet() {}

void KWDRContinuousValueSet::SetValueNumber(int nValue)
{
	require(nValue >= 0);

	cvContinuousValueSet.SetSize(nValue);
	bStructureInterface = true;
	nFreshness++;
}

int KWDRContinuousValueSet::GetValueNumber() const
{
	if (bStructureInterface)
		return cvContinuousValueSet.GetSize();
	else
		return GetOperandNumber();
}

void KWDRContinuousValueSet::SetValueAt(int nIndex, Continuous cBound)
{
	require(0 <= nIndex and nIndex < cvContinuousValueSet.GetSize());
	require(bStructureInterface);

	cvContinuousValueSet.SetAt(nIndex, cBound);
	nFreshness++;
}

Continuous KWDRContinuousValueSet::GetValueAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < cvContinuousValueSet.GetSize());
	require(bStructureInterface);

	return cvContinuousValueSet.GetAt(nIndex);
}

int KWDRContinuousValueSet::GetPartNumber() const
{
	return GetValueNumber();
}

int KWDRContinuousValueSet::GetAttributeType() const
{
	return KWType::Continuous;
}

int KWDRContinuousValueSet::GetContinuousPartIndex(Continuous cValue) const
{
	return ComputeValueIndex(cValue);
}

const ALString KWDRContinuousValueSet::GetPartLabelAt(int nIndex) const
{
	ALString sPartLabel;

	require(Check());
	require(IsCompiled());
	require(bStructureInterface);

	// Calcul du libelle si index valide
	if (0 <= nIndex and nIndex < GetPartNumber())
	{
		if (cvContinuousValueSet.GetAt(nIndex) == KWContinuous::GetMissingValue())
			sPartLabel = "Missing";
		else
			sPartLabel = KWContinuous::ContinuousToString(cvContinuousValueSet.GetAt(nIndex));
		return sPartLabel;
	}
	return "";
}

int KWDRContinuousValueSet::ComputeValueIndex(Continuous cValue) const
{
	int nIndex;

	require(Check());
	require(IsCompiled());
	require(bStructureInterface);

	// On utilise ici la methode GetLowerMeanValue (plutot que GetHumanReadableLowerMeanValue), car les valeurs des
	// bornes ne sont pas montrees aux utilisateurs, et que l'on ne s'en sert que pour les calculs dans une regle de
	// derivation (exigence de rapidite) Recherche sequentielle s'il y a peu de valeurs
	if (cvContinuousValueSet.GetSize() <= 10)
	{
		for (nIndex = 0; nIndex < cvContinuousValueSet.GetSize() - 1; nIndex++)
		{
			if (cValue <= KWContinuous::GetLowerMeanValue(cvContinuousValueSet.GetAt(nIndex),
								      cvContinuousValueSet.GetAt(nIndex + 1)))
				return nIndex;
		}
		assert(nIndex == cvContinuousValueSet.GetSize() - 1);
		return nIndex;
	}
	// Recherche dichotomique sinon
	else
	{
		int nLowerIndex;
		int nUpperIndex;

		// Initialisation des index extremites
		nLowerIndex = 0;
		nUpperIndex = cvContinuousValueSet.GetSize() - 2;

		// Recherche dichotomique de l'index de la valeur
		nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
		while (nLowerIndex + 1 < nUpperIndex)
		{
			// Deplacement des bornes de recherche en fonction
			// de la comparaison avec la borne courante
			if (cValue <= KWContinuous::GetLowerMeanValue(cvContinuousValueSet.GetAt(nIndex),
								      cvContinuousValueSet.GetAt(nIndex + 1)))
				nUpperIndex = nIndex;
			else
				nLowerIndex = nIndex;

			// Modification du prochain intervalle teste
			nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
		}
		assert(nLowerIndex <= nUpperIndex);
		assert(nUpperIndex <= nLowerIndex + 1);

		// On compare par rapport aux deux index restant
		if (cValue <= KWContinuous::GetLowerMeanValue(cvContinuousValueSet.GetAt(nLowerIndex),
							      cvContinuousValueSet.GetAt(nLowerIndex + 1)))
			nIndex = nLowerIndex;
		else if (cValue > KWContinuous::GetLowerMeanValue(cvContinuousValueSet.GetAt(nUpperIndex),
								  cvContinuousValueSet.GetAt(nUpperIndex + 1)))
			nIndex = nUpperIndex + 1;
		else
			nIndex = nUpperIndex;

		// On retourne le resultat
		assert(nIndex == cvContinuousValueSet.GetSize() - 1 or
		       cValue <= KWContinuous::GetLowerMeanValue(cvContinuousValueSet.GetAt(nIndex),
								 cvContinuousValueSet.GetAt(nIndex + 1)));
		assert(nIndex == 0 or cValue > KWContinuous::GetLowerMeanValue(cvContinuousValueSet.GetAt(nIndex - 1),
									       cvContinuousValueSet.GetAt(nIndex)));
		return nIndex;
	}
}

void KWDRContinuousValueSet::ImportAttributeContinuousValues(
    const KWDGSAttributeContinuousValues* attributeContinuousValues)
{
	int nValue;

	require(attributeContinuousValues != NULL);

	// Reinitialisation
	DeleteAllOperands();

	// Parametrage des valeurs
	SetValueNumber(attributeContinuousValues->GetValueNumber());
	for (nValue = 0; nValue < GetValueNumber(); nValue++)
		SetValueAt(nValue, attributeContinuousValues->GetValueAt(nValue));
}

void KWDRContinuousValueSet::ExportAttributeContinuousValues(
    KWDGSAttributeContinuousValues* attributeContinuousValues) const
{
	int nValue;

	require(IsCompiled());
	require(attributeContinuousValues != NULL);

	// Parametrage des valeurs
	attributeContinuousValues->SetPartNumber(GetValueNumber());
	for (nValue = 0; nValue < GetValueNumber(); nValue++)
		attributeContinuousValues->SetValueAt(nValue, GetValueAt(nValue));
}

KWDerivationRule* KWDRContinuousValueSet::Create() const
{
	return new KWDRContinuousValueSet;
}

void KWDRContinuousValueSet::CleanCompiledBaseInterface()
{
	DeleteAllOperands();
}

void KWDRContinuousValueSet::CopyStructureFrom(const KWDerivationRule* kwdrSource)
{
	const KWDRContinuousValueSet* kwdribSource = cast(KWDRContinuousValueSet*, kwdrSource);

	// Copie de la version optimisee du parametrage des bornes de discretisation
	cvContinuousValueSet.CopyFrom(&kwdribSource->cvContinuousValueSet);
}

void KWDRContinuousValueSet::BuildStructureFromBase(const KWDerivationRule* kwdrSource)
{
	int i;
	KWDerivationRuleOperand* operand;

	require(kwdrSource->KWDerivationRule::CheckDefinition());

	// Recopie des operandes
	cvContinuousValueSet.SetSize(kwdrSource->GetOperandNumber());
	for (i = 0; i < kwdrSource->GetOperandNumber(); i++)
	{
		operand = kwdrSource->GetOperandAt(i);
		cvContinuousValueSet.SetAt(i, operand->GetContinuousConstant());
	}
	bStructureInterface = true;
}

boolean KWDRContinuousValueSet::CheckStructureDefinition() const
{
	boolean bOk = true;
	int i;
	ALString sTmp;

	// Test de l'existence d'au moins une valeur
	if (cvContinuousValueSet.GetSize() == 0)
	{
		bOk = false;
		AddError("Empty value set");
	}

	// Test de l'ordre des valeurs
	for (i = 1; i < cvContinuousValueSet.GetSize(); i++)
	{
		if (cvContinuousValueSet.GetAt(i - 1) > cvContinuousValueSet.GetAt(i))
		{
			bOk = false;
			AddError("Values are not correctly ordered");
			break;
		}
		else if (cvContinuousValueSet.GetAt(i - 1) == cvContinuousValueSet.GetAt(i))
		{
			bOk = false;
			AddError(sTmp + "Value " + KWContinuous::ContinuousToString(cvContinuousValueSet.GetAt(i)) +
				 " is not unique in the set");
			break;
		}
	}
	return bOk;
}

void KWDRContinuousValueSet::WriteStructureUsedRule(ostream& ost) const
{
	int i;

	// Nom de la regle utilisee
	ost << KWClass::GetExternalName(GetName());

	// Operandes
	ost << "(";
	for (i = 0; i < cvContinuousValueSet.GetSize(); i++)
	{
		if (i > 0)
			ost << ", ";
		ost << KWClass::GetExternalContinuousConstant(cvContinuousValueSet.GetAt(i));
	}
	ost << ")";
}

int KWDRContinuousValueSet::FullCompare(const KWDerivationRule* rule) const
{
	int nDiff;
	KWDRContinuousValueSet* ruleContinuousValueSet;
	int i;

	require(rule != NULL);

	// Comparaison sur la classe sur laquelle la regle est applicable
	nDiff = GetClassName().Compare(rule->GetClassName());

	// Comparaison sur le nom de la regle
	if (nDiff == 0)
		nDiff = GetName().Compare(rule->GetName());

	// En cas d'egalite, comparaison en utilisant l'interface de structure
	if (nDiff == 0)
	{
		ruleContinuousValueSet = cast(KWDRContinuousValueSet*, rule);
		assert(GetStructureInterface());
		assert(ruleContinuousValueSet->GetStructureInterface());

		// Taille du vecteur
		nDiff = cvContinuousValueSet.GetSize() - ruleContinuousValueSet->cvContinuousValueSet.GetSize();

		// Si egalite, comparaison sur les valeurs
		if (nDiff == 0)
		{
			for (i = 0; i < cvContinuousValueSet.GetSize(); i++)
			{
				nDiff = KWContinuous::Compare(cvContinuousValueSet.GetAt(i),
							      ruleContinuousValueSet->cvContinuousValueSet.GetAt(i));
				if (nDiff != 0)
					break;
			}
		}
	}

	return nDiff;
}

longint KWDRContinuousValueSet::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRUnivariatePartition::GetUsedMemory();
	lUsedMemory += sizeof(KWDRContinuousValueSet) - sizeof(KWDRUnivariatePartition);
	lUsedMemory += cvContinuousValueSet.GetUsedMemory();
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////
// Classe KWDRValueGroup

KWDRValueGroup::KWDRValueGroup()
{
	SetName("ValueGroup");
	SetLabel("Value group");
	SetStructureName("ValueGroup");
	GetFirstOperand()->SetSymbolConstant(Symbol::GetStarValue());
}

KWDRValueGroup::~KWDRValueGroup() {}

KWDerivationRule* KWDRValueGroup::Create() const
{
	return new KWDRValueGroup;
}

void KWDRValueGroup::BuildStructureFromBase(const KWDerivationRule* kwdrSource)
{
	int i;
	KWDerivationRuleOperand* operand;
	Symbol sValue;
	ALString sStarString;

	require(kwdrSource->KWDerivationRule::CheckDefinition());

	// Acces au caractere codant la valeur speciale
	sStarString = Symbol::GetStarValue().GetValue();

	// Recopie des operandes
	svValues.SetSize(kwdrSource->GetOperandNumber());
	for (i = 0; i < kwdrSource->GetOperandNumber(); i++)
	{
		operand = kwdrSource->GetOperandAt(i);
		sValue = operand->GetSymbolConstant();

		// Traitement particulier de la valeur speciale
		// C'est la seule valeur ayant des blancs non supprimes en debut et fin
		if (sValue == sStarString)
			svValues.SetAt(i, Symbol::GetStarValue());
		else
			svValues.SetAt(i, sValue);
	}
	bStructureInterface = true;
}

void KWDRValueGroup::WriteStructureUsedRule(ostream& ost) const
{
	int i;
	Symbol sValue;
	ALString sStarString;

	// Acces au caractere codant la valeur speciale
	sStarString = Symbol::GetStarValue().GetValue();

	// Nom de la regle utilisee
	ost << KWClass::GetExternalName(GetName());

	// Operandes
	ost << "(";
	for (i = 0; i < svValues.GetSize(); i++)
	{
		sValue = svValues.GetAt(i);
		if (i > 0)
			ost << ", ";

		// Traitement particulier de la valeur speciale:
		// on ajoute des blancs pour l'entourer, ce qui evite les collisions avec les valeurs
		// qui sont purgees des blancs en debut et fin
		if (sValue == sStarString)
			ost << KWClass::GetExternalSymbolConstant(Symbol::GetStarValue());
		else
			ost << KWClass::GetExternalSymbolConstant(sValue);
	}
	ost << ")";
}

///////////////////////////////////////////////////////////////
// Classe KWDRValueGroups

KWDRValueGroups::KWDRValueGroups()
{
	KWDRValueGroup* valueGroup;

	SetName("ValueGroups");
	SetLabel("Value groups");
	SetType(KWType::Structure);
	SetStructureName("ValueGroups");
	SetOperandNumber(1);
	SetVariableOperandNumber(true);

	// Les operandes contiennent des regles de type ensemble de valeurs
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("ValueGroup");
	GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);

	// Par defaut, il y a un seul ensemble de valeur pour la modalite speciale
	valueGroup = new KWDRValueGroup;
	GetFirstOperand()->SetDerivationRule(valueGroup);

	// Gestion de la compilation dynamique
	nStarValueTargetIndex = 0;
	nDynamicCompileFreshness = 0;
}

KWDRValueGroups::~KWDRValueGroups() {}

void KWDRValueGroups::SetValueGroupNumber(int nValue)
{
	int nPreviousValueGroupNumber;
	int nGroup;
	KWDerivationRuleOperand* operand;

	require(nValue >= 0);

	// Changement du nombre d'operandes
	// avec destruction eventuelle en cas de diminuation du nombre de groupes
	nPreviousValueGroupNumber = GetOperandNumber();
	SetOperandNumber(nValue);

	// Parametrage des eventuels nouveaux operandes
	for (nGroup = nPreviousValueGroupNumber; nGroup < nValue; nGroup++)
	{
		operand = GetOperandAt(nGroup);
		operand->SetType(KWType::Structure);
		operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		operand->SetDerivationRule(new KWDRValueGroup);
		operand->SetStructureName(operand->GetDerivationRule()->GetStructureName());
	}
}

int KWDRValueGroups::GetValueGroupNumber() const
{
	return GetOperandNumber();
}

KWDRValueGroup* KWDRValueGroups::GetValueGroupAt(int nIndex)
{
	require(0 <= nIndex and nIndex < GetOperandNumber());
	assert(GetOperandAt(nIndex)->GetDerivationRule() != NULL);
	return cast(KWDRValueGroup*, GetOperandAt(nIndex)->GetDerivationRule());
}

int KWDRValueGroups::GetPartNumber() const
{
	return GetValueGroupNumber();
}

int KWDRValueGroups::GetAttributeType() const
{
	return KWType::Symbol;
}

int KWDRValueGroups::ComputeTotalPartValueNumber() const
{
	int nTotalPartValueNumber;
	int nIndex;
	KWDRValueGroup* valueGroup;

	// Calcul du nombre total de valeurs
	nTotalPartValueNumber = 0;
	for (nIndex = 0; nIndex < GetOperandNumber(); nIndex++)
	{
		assert(GetOperandAt(nIndex)->GetDerivationRule() != NULL);
		valueGroup = cast(KWDRValueGroup*, GetOperandAt(nIndex)->GetDerivationRule());
		nTotalPartValueNumber += valueGroup->GetValueNumber();
	}

	// On enleve la StarValue, presente necessauirement dans un des groupes
	if (nTotalPartValueNumber > 0)
		nTotalPartValueNumber--;
	return nTotalPartValueNumber;
}

int KWDRValueGroups::GetSymbolPartIndex(const Symbol& sValue) const
{
	return ComputeGroupIndex(sValue);
}

const ALString KWDRValueGroups::GetPartLabelAt(int nIndex) const
{
	ALString sPartLabel;
	KWDRValueGroup* valueGroup;
	int nValue;
	Symbol sValue;
	int nNumber;

	require(Check());
	require(IsCompiled());

	// Renvoie vide si index hors bornes
	if (0 <= nIndex and nIndex < GetPartNumber())
	{
		// Acces au groupe
		valueGroup = cast(KWDRValueGroup*, GetOperandAt(nIndex)->GetDerivationRule());

		// Fabrication du libelle a partir des valeurs du groupes
		nNumber = 0;
		sPartLabel = "{";
		for (nValue = 0; nValue < valueGroup->GetValueNumber(); nValue++)
		{
			sValue = valueGroup->GetValueAt(nValue);

			// On n'utilise la modalite speciale pour fabriquer le libelle
			if (sValue != Symbol::GetStarValue())
			{
				// Prise en compte si moins de trois valeurs
				if (nNumber < 3)
				{
					if (nNumber > 0)
						sPartLabel += ", ";
					sPartLabel += sValue;
					nNumber++;
				}
				// Arret si au moins quatre valeurs
				else
				{
					sPartLabel += ", ...";
					break;
				}
			}
		}
		sPartLabel += "}";
		return sPartLabel;
	}
	return "";
}

int KWDRValueGroups::ComputeGroupIndex(const Symbol& sValue) const
{
	int nIndex;

	require(Check());
	require(IsCompiled());

	// Compilation dynamique si necessaire
	if (nDynamicCompileFreshness != nCompileFreshness)
		DynamicCompile();

	// Recherche de la modalite codee correspondante
	// Recherche sequentielle s'il y a peu de modalites
	if (svSortedValues.GetSize() <= 10)
	{
		for (nIndex = 0; nIndex < svSortedValues.GetSize(); nIndex++)
		{
			if (sValue == svSortedValues.GetAt(nIndex))
				return ivGroupIndexes.GetAt(nIndex);
		}
		assert(nIndex == svSortedValues.GetSize());

		// Retour de l'index de la modalite cible
		return nStarValueTargetIndex;
	}
	// Recherche dichotomique sinon
	else
	{
		int nLowerIndex;
		int nUpperIndex;

		// Initialisation des index extremites
		nLowerIndex = 0;
		nUpperIndex = svSortedValues.GetSize() - 1;

		// Recherche dichotomique de la modalite
		nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
		while (nLowerIndex + 1 < nUpperIndex)
		{
			assert(svSortedValues.GetAt(nLowerIndex) <= svSortedValues.GetAt(nUpperIndex));

			// Deplacement des bornes de recherche en fonction
			// de la comparaison avec la borne courante
			if (sValue <= svSortedValues.GetAt(nIndex))
				nUpperIndex = nIndex;
			else
				nLowerIndex = nIndex;

			// Modification du prochain intervalle teste
			nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
		}
		assert(nLowerIndex <= nUpperIndex);
		assert(nUpperIndex <= nLowerIndex + 1);

		// On compare par rapport aux deux index restant
		if (sValue == svSortedValues.GetAt(nLowerIndex))
			nIndex = nLowerIndex;
		else if (sValue == svSortedValues.GetAt(nUpperIndex))
			nIndex = nUpperIndex;
		else
			nIndex = -1;

		// On retourne le resultat
		if (nIndex == -1)
			return nStarValueTargetIndex;
		else
			return ivGroupIndexes.GetAt(nIndex);
	}
}

int KWDRValueGroups::GetTotalPartValueNumber() const
{
	require(IsCompiled());

	// Compilation dynamique si necessaire
	if (nDynamicCompileFreshness != nCompileFreshness)
		DynamicCompile();
	ensure(ComputeTotalPartValueNumber() == svSortedValues.GetSize());
	return svSortedValues.GetSize();
}

void KWDRValueGroups::ImportAttributeGrouping(const KWDGSAttributeGrouping* attributeGrouping)
{
	KWDRValueGroup* valueGroupRule;
	KWDerivationRuleOperand* valueGroupOperand;
	int nGroup;
	int nValue;

	// Acces a la partition source
	require(attributeGrouping != NULL);

	// Reinitialisation
	DeleteAllOperands();

	// Parametrage des groupes de valeurs
	for (nGroup = 0; nGroup < attributeGrouping->GetPartNumber(); nGroup++)
	{
		// Creation d'un operande pour le groupe
		valueGroupOperand = new KWDerivationRuleOperand;
		AddOperand(valueGroupOperand);
		valueGroupOperand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		valueGroupOperand->SetType(KWType::Structure);

		// Creation d'un nouveau groupe
		valueGroupRule = new KWDRValueGroup;
		valueGroupOperand->SetDerivationRule(valueGroupRule);
		valueGroupOperand->SetStructureName(valueGroupRule->GetStructureName());

		// Ajout des valeurs du groupe
		valueGroupRule->DeleteAllOperands();
		valueGroupRule->SetValueNumber(attributeGrouping->GetGroupValueNumberAt(nGroup));
		for (nValue = 0; nValue < attributeGrouping->GetGroupValueNumberAt(nGroup); nValue++)
		{
			valueGroupRule->SetValueAt(nValue,
						   attributeGrouping->GetValueAt(
						       attributeGrouping->GetGroupFirstValueIndexAt(nGroup) + nValue));
		}
	}
}

void KWDRValueGroups::ExportAttributeGrouping(KWDGSAttributeGrouping* attributeGrouping) const
{
	const KWDRValueGroup* valueGroupRule;
	int nValueNumber;
	int nGroup;
	int nValue;
	int i;

	require(IsCompiled());
	require(attributeGrouping != NULL);

	// Calcul du nombre de valeurs total sur l'ensemble des groupes
	nValueNumber = 0;
	for (nGroup = 0; nGroup < GetValueGroupNumber(); nGroup++)
	{
		valueGroupRule = cast(const KWDRValueGroup*, GetOperandAt(nGroup)->GetDerivationRule());
		nValueNumber += valueGroupRule->GetValueNumber();
	}

	// Parametrage des groupes et des valeurs
	attributeGrouping->SetPartNumber(GetValueGroupNumber());
	attributeGrouping->SetKeptValueNumber(nValueNumber);
	nValue = 0;
	for (nGroup = 0; nGroup < GetValueGroupNumber(); nGroup++)
	{
		valueGroupRule = cast(const KWDRValueGroup*, GetOperandAt(nGroup)->GetDerivationRule());

		// Parametrage de l'index de la premiere valeur du groupe
		attributeGrouping->SetGroupFirstValueIndexAt(nGroup, nValue);

		// Parametrage des valeurs du groupe
		for (i = 0; i < valueGroupRule->GetValueNumber(); i++)
		{
			attributeGrouping->SetValueAt(nValue, valueGroupRule->GetValueAt(i));
			nValue++;
		}
	}
}

KWDerivationRule* KWDRValueGroups::Create() const
{
	return new KWDRValueGroups;
}

Object* KWDRValueGroups::ComputeStructureResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsCompiled());
	return (Object*)this;
}

void KWDRValueGroups::Compile(KWClass* kwcOwnerClass)
{
	// Bien que l'on herite indirectement de KWDRStructure, une compilation standard
	// est ici necessaire, et suffisante
	KWDerivationRule::Compile(kwcOwnerClass);
}

boolean KWDRValueGroups::CheckDefinition() const
{
	boolean bOk;

	bOk = KWDerivationRule::CheckDefinition();

	// Verification des groupes
	if (bOk)
	{
		KWDRValueGroup* valueGroup;
		int nGroup;
		KWDRValueGroup tempValueGroup;
		KWDRValueGroup* checkedValueGroup;
		int nValue;
		Symbol sValue;
		NumericKeyDictionary nkdValues;
		ALString sTmp;

		// Verification de la presence d'au moins un groupe
		if (GetValueGroupNumber() == 0)
		{
			bOk = false;
			AddError("No value group is specified");
		}

		// Verification de l'unicite des modalites sources
		Global::ActivateErrorFlowControl();
		for (nGroup = 0; nGroup < GetValueGroupNumber(); nGroup++)
		{
			valueGroup = cast(KWDRValueGroup*, GetOperandAt(nGroup)->GetDerivationRule());

			// Transfert des valeurs vers la representation structuree
			if (valueGroup->GetStructureInterface())
				checkedValueGroup = valueGroup;
			else
			{
				tempValueGroup.BuildStructureFromBase(valueGroup);
				checkedValueGroup = &tempValueGroup;
			}

			// Test d'existence d'au moins une valeur
			if (checkedValueGroup->GetValueNumber() == 0)
			{
				bOk = false;
				AddError(sTmp + "No value is specified in group " + IntToString(nGroup + 1));
			}

			// Parcours des valeurs du groupe
			for (nValue = 0; nValue < checkedValueGroup->GetValueNumber(); nValue++)
			{
				sValue = checkedValueGroup->GetValueAt(nValue);
				if (nkdValues.Lookup(sValue.GetNumericKey()) != NULL)
				{
					bOk = false;
					AddError(sTmp + "Input value " + sValue + " is specified several times");
				}
				else
					nkdValues.SetAt(sValue.GetNumericKey(), valueGroup);
			}
		}
		Global::DesactivateErrorFlowControl();

		// Test de la presence de la modalite speciale StarValue
		if (nkdValues.Lookup(Symbol::GetStarValue().GetNumericKey()) == NULL)
		{
			bOk = false;
			AddError(sTmp + "Special value " + Symbol::GetStarValue() +
				 " does not belong to any value group");
		}
	}
	return bOk;
}

int KWDRValueGroups::FullCompare(const KWDerivationRule* rule) const
{
	int nDiff;

	// Appel de la methode standard, pour court-circuiter la methode de Structure
	nDiff = KWDerivationRule::FullCompare(rule);
	return nDiff;
}

longint KWDRValueGroups::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRUnivariatePartition::GetUsedMemory();
	lUsedMemory += sizeof(KWDRValueGroups) - sizeof(KWDRUnivariatePartition);
	lUsedMemory += svSortedValues.GetUsedMemory();
	lUsedMemory += ivGroupIndexes.GetUsedMemory();
	return lUsedMemory;
}

void KWDRValueGroups::DynamicCompile() const
{
	int nGroup;
	int nValue;
	int nCompiledValue;
	KWDRValueGroup* valueGroup;
	KWSortableSymbol* sortableValue;
	ObjectArray oaSortableValues;
	debug(boolean bFound = false);

	require(Check());
	require(IsCompiled());

	// Compilation dynamique si necessaire
	if (nDynamicCompileFreshness < nCompileFreshness)
	{
		// Preparation d'un tableau des valeurs sources, associes a l'index
		// de leur groupe
		for (nGroup = 0; nGroup < GetValueGroupNumber(); nGroup++)
		{
			valueGroup = cast(KWDRValueGroup*, GetOperandAt(nGroup)->GetDerivationRule());

			// On cree une paire (modalite, index de groupes) par modalite
			for (nValue = 0; nValue < valueGroup->GetValueNumber(); nValue++)
			{
				// Creation, initialisation et memorisation d'une paire triable
				sortableValue = new KWSortableSymbol;
				sortableValue->SetSortValue(valueGroup->GetValueAt(nValue));
				sortableValue->SetIndex(nGroup);
				oaSortableValues.Add(sortableValue);
			}
		}

		// Tri des modalites
		oaSortableValues.SetCompareFunction(KWSortableSymbolCompare);
		oaSortableValues.Sort();

		// Initialisation de la taille des vecteurs de modalites et codes
		svSortedValues.SetSize(oaSortableValues.GetSize() - 1);
		ivGroupIndexes.SetSize(svSortedValues.GetSize());

		// Initialisation des caracteristique du groupe de la modalite speciale
		nStarValueTargetIndex = -1;

		// Compilation de ces vecteurs: on range les modalites (et leur code)
		// par ordre de references des modalites, ce qui permettra une recherche
		// rapide par dichotomie lors de l'utilisation de la regle
		// On profite de cette passe pour identifier la modalite speciale
		nCompiledValue = 0;
		for (nValue = 0; nValue < oaSortableValues.GetSize(); nValue++)
		{
			sortableValue = cast(KWSortableSymbol*, oaSortableValues.GetAt(nValue));

			// Memorisation eventuelle du code de la modalite speciale
			if (sortableValue->GetSortValue() == Symbol::GetStarValue())
			{
				nStarValueTargetIndex = sortableValue->GetIndex();
				debug(bFound = true);
			}
			// Sinon, rangement de la modalite et de son code dans les vecteurs
			else
			{
				assert(nCompiledValue == 0 or
				       svSortedValues.GetAt(nCompiledValue - 1).Compare(sortableValue->GetSortValue()) <
					   0);

				// Memorisation de la correspondance entre modalite source et index de groupe
				svSortedValues.SetAt(nCompiledValue, sortableValue->GetSortValue());
				ivGroupIndexes.SetAt(nCompiledValue, sortableValue->GetIndex());
				nCompiledValue++;
			}
		}
		debug(assert(bFound));
		assert(0 <= nStarValueTargetIndex and nStarValueTargetIndex < GetValueGroupNumber());
		assert(nCompiledValue == svSortedValues.GetSize());

		// Nettoyage
		oaSortableValues.DeleteAll();

		// Memorisation de la fraicheur
		nDynamicCompileFreshness = nCompileFreshness;
	}
}

///////////////////////////////////////////////////////////////
// Classe KWDRSymbolValueSet

KWDRSymbolValueSet::KWDRSymbolValueSet()
{
	SetName("ValueSetC");
	SetLabel("Categorical value set");
	SetStructureName("ValueSetC");
	SetOperandNumber(1);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::Symbol);
	GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	nStarValueIndex = 0;
	nDynamicCompileFreshness = 0;
}

KWDRSymbolValueSet::~KWDRSymbolValueSet() {}

void KWDRSymbolValueSet::SetValueNumber(int nValue)
{
	require(nValue >= 0);

	svSymbolValueSet.SetSize(nValue);
	bStructureInterface = true;
	nFreshness++;
}

int KWDRSymbolValueSet::GetValueNumber() const
{
	if (bStructureInterface)
		return svSymbolValueSet.GetSize();
	else
		return GetOperandNumber();
}

void KWDRSymbolValueSet::SetValueAt(int nIndex, const Symbol& sBound)
{
	require(0 <= nIndex and nIndex < svSymbolValueSet.GetSize());
	require(bStructureInterface);

	svSymbolValueSet.SetAt(nIndex, sBound);
	nFreshness++;
}

Symbol& KWDRSymbolValueSet::GetValueAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < svSymbolValueSet.GetSize());
	require(bStructureInterface);

	return svSymbolValueSet.GetAt(nIndex);
}

int KWDRSymbolValueSet::GetPartNumber() const
{
	return GetValueNumber();
}

int KWDRSymbolValueSet::GetAttributeType() const
{
	return KWType::Symbol;
}

int KWDRSymbolValueSet::GetSymbolPartIndex(const Symbol& sValue) const
{
	return ComputeValueIndex(sValue);
}

const ALString KWDRSymbolValueSet::GetPartLabelAt(int nIndex) const
{
	ALString sPartLabel;

	require(Check());
	require(IsCompiled());
	require(bStructureInterface);

	// Renvoie vide si index hors bornes
	if (0 <= nIndex and nIndex < GetPartNumber())
	{
		sPartLabel = svSymbolValueSet.GetAt(nIndex);
		return sPartLabel;
	}
	return "";
}

int KWDRSymbolValueSet::ComputeValueIndex(const Symbol& sValue) const
{
	int nIndex;

	require(Check());
	require(IsCompiled());
	require(bStructureInterface);

	// Compilation dynamique si necessaire
	if (nDynamicCompileFreshness != nCompileFreshness)
		DynamicCompile();

	// Recherche de la modalite codee correspondante
	// Recherche sequentielle s'il y a peu de modalites
	if (svSortedValues.GetSize() <= 10)
	{
		for (nIndex = 0; nIndex < svSortedValues.GetSize(); nIndex++)
		{
			if (sValue == svSortedValues.GetAt(nIndex))
				return ivValueIndexes.GetAt(nIndex);
		}
		assert(nIndex == svSortedValues.GetSize());

		// Index de la StarValue (ou 1) en cas d'echec
		return nStarValueIndex;
	}
	// Recherche dichotomique sinon
	else
	{
		int nLowerIndex;
		int nUpperIndex;

		// Initialisation des index extremites
		nLowerIndex = 0;
		nUpperIndex = svSortedValues.GetSize() - 1;

		// Recherche dichotomique de la modalite
		nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
		while (nLowerIndex + 1 < nUpperIndex)
		{
			assert(svSortedValues.GetAt(nLowerIndex) <= svSortedValues.GetAt(nUpperIndex));

			// Deplacement des bornes de recherche en fonction
			// de la comparaison avec la borne courante
			if (sValue <= svSortedValues.GetAt(nIndex))
				nUpperIndex = nIndex;
			else
				nLowerIndex = nIndex;

			// Modification du prochain intervalle teste
			nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
		}
		assert(nLowerIndex <= nUpperIndex);
		assert(nUpperIndex <= nLowerIndex + 1);

		// On compare par rapport aux deux index restant
		if (sValue == svSortedValues.GetAt(nLowerIndex))
			nIndex = nLowerIndex;
		else if (sValue == svSortedValues.GetAt(nUpperIndex))
			nIndex = nUpperIndex;
		else
			nIndex = -1;

		// On retourne le resultat
		if (nIndex == -1)
			return nStarValueIndex;
		else
			return ivValueIndexes.GetAt(nIndex);
	}
}

void KWDRSymbolValueSet::ImportAttributeSymbolValues(const KWDGSAttributeSymbolValues* attributeSymbolValues)
{
	int nValue;

	require(attributeSymbolValues != NULL);

	// Reinitialisation
	DeleteAllOperands();

	// Parametrage des valeurs
	SetValueNumber(attributeSymbolValues->GetValueNumber());
	for (nValue = 0; nValue < GetValueNumber(); nValue++)
		SetValueAt(nValue, attributeSymbolValues->GetValueAt(nValue));
}

void KWDRSymbolValueSet::ExportAttributeSymbolValues(KWDGSAttributeSymbolValues* attributeSymbolValues) const
{
	int nValue;

	require(IsCompiled());
	require(attributeSymbolValues != NULL);

	// Parametrage des valeurs
	attributeSymbolValues->SetPartNumber(GetValueNumber());
	for (nValue = 0; nValue < GetValueNumber(); nValue++)
		attributeSymbolValues->SetValueAt(nValue, GetValueAt(nValue));
}

KWDerivationRule* KWDRSymbolValueSet::Create() const
{
	return new KWDRSymbolValueSet;
}

void KWDRSymbolValueSet::CleanCompiledBaseInterface()
{
	DeleteAllOperands();
}

void KWDRSymbolValueSet::CopyStructureFrom(const KWDerivationRule* kwdrSource)
{
	const KWDRSymbolValueSet* kwdribSource = cast(KWDRSymbolValueSet*, kwdrSource);

	// Copie de la version optimisee du parametrage des bornes de discretisation
	svSymbolValueSet.CopyFrom(&kwdribSource->svSymbolValueSet);
}

void KWDRSymbolValueSet::BuildStructureFromBase(const KWDerivationRule* kwdrSource)
{
	int i;
	KWDerivationRuleOperand* operand;
	Symbol sValue;
	ALString sStarString;

	require(kwdrSource->KWDerivationRule::CheckDefinition());

	// Acces au caractere codant la valeur speciale
	sStarString = Symbol::GetStarValue().GetValue();

	// Recopie des operandes
	svSymbolValueSet.SetSize(kwdrSource->GetOperandNumber());
	for (i = 0; i < kwdrSource->GetOperandNumber(); i++)
	{
		operand = kwdrSource->GetOperandAt(i);
		sValue = operand->GetSymbolConstant();

		// Traitement particulier de la valeur speciale
		// C'est la seule valeur ayant des blancs non supprimes en debut et fin
		if (sValue == sStarString)
			svSymbolValueSet.SetAt(i, Symbol::GetStarValue());
		else
			svSymbolValueSet.SetAt(i, sValue);
	}
	bStructureInterface = true;
}

boolean KWDRSymbolValueSet::CheckStructureDefinition() const
{
	boolean bOk = true;
	SymbolVector svSortedCheckValues;
	int i;
	ALString sTmp;

	// Test de l'existence d'au moins une valeur
	if (svSymbolValueSet.GetSize() == 0)
	{
		bOk = false;
		AddError("Empty value set");
	}

	// Test de l'unicite des valeurs, apres les avoir triees
	svSortedCheckValues.CopyFrom(&svSymbolValueSet);
	svSortedCheckValues.SortKeys();
	for (i = 1; i < svSortedCheckValues.GetSize(); i++)
	{
		if (svSortedCheckValues.GetAt(i - 1) == svSortedCheckValues.GetAt(i))
		{
			bOk = false;
			AddError(sTmp + "Value " + svSortedCheckValues.GetAt(i) + " is not unique in the set");
			break;
		}
	}
	return bOk;
}

void KWDRSymbolValueSet::WriteStructureUsedRule(ostream& ost) const
{
	int i;
	Symbol sValue;
	ALString sStarString;

	// Acces au caractere codant la valeur speciale
	sStarString = Symbol::GetStarValue().GetValue();

	// Nom de la regle utilisee
	ost << KWClass::GetExternalName(GetName());

	// Operandes
	ost << "(";
	for (i = 0; i < svSymbolValueSet.GetSize(); i++)
	{
		sValue = svSymbolValueSet.GetAt(i);
		if (i > 0)
			ost << ", ";

		// Traitement particulier de la valeur speciale:
		// on ajoute des blancs pour l'entourer, ce qui evite les collisions avec les valeurs
		// qui sont purgees des blancs en debut et fin
		if (sValue == sStarString)
			ost << KWClass::GetExternalSymbolConstant(Symbol::GetStarValue());
		else
			ost << KWClass::GetExternalSymbolConstant(sValue);
	}
	ost << ")";
}

int KWDRSymbolValueSet::FullCompare(const KWDerivationRule* rule) const
{
	int nDiff;
	KWDRSymbolValueSet* ruleSymbolValueSet;
	int i;

	require(rule != NULL);

	// Comparaison sur la classe sur laquelle la regle est applicable
	nDiff = GetClassName().Compare(rule->GetClassName());

	// Comparaison sur le nom de la regle
	if (nDiff == 0)
		nDiff = GetName().Compare(rule->GetName());

	// En cas d'egalite, comparaison en utilisant l'interface de structure
	if (nDiff == 0)
	{
		ruleSymbolValueSet = cast(KWDRSymbolValueSet*, rule);
		assert(GetStructureInterface());
		assert(ruleSymbolValueSet->GetStructureInterface());

		// Taille du vecteur
		nDiff = svSymbolValueSet.GetSize() - ruleSymbolValueSet->svSymbolValueSet.GetSize();

		// Si egalite, comparaison sur les valeurs
		if (nDiff == 0)
		{
			for (i = 0; i < svSymbolValueSet.GetSize(); i++)
			{
				nDiff = svSymbolValueSet.GetAt(i).CompareValue(
				    ruleSymbolValueSet->svSymbolValueSet.GetAt(i));
				if (nDiff != 0)
					break;
			}
		}
	}

	return nDiff;
}

longint KWDRSymbolValueSet::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRUnivariatePartition::GetUsedMemory();
	lUsedMemory += sizeof(KWDRSymbolValueSet) - sizeof(KWDRUnivariatePartition);
	lUsedMemory += svSortedValues.GetUsedMemory();
	lUsedMemory += ivValueIndexes.GetUsedMemory();
	lUsedMemory += svSymbolValueSet.GetUsedMemory();
	return lUsedMemory;
}

void KWDRSymbolValueSet::DynamicCompile() const
{
	int nValue;
	KWSortableSymbol* sortableValue;
	ObjectArray oaSortableValues;

	require(Check());
	require(IsCompiled());

	// Compilation dynamique si necessaire
	if (nDynamicCompileFreshness < nCompileFreshness)
	{
		// Preparation d'un tableau des valeurs sources, associes a leur index
		// Calcul de l'index de la StarValue (par defaut, premier index)
		nStarValueIndex = 0;
		for (nValue = 0; nValue < svSymbolValueSet.GetSize(); nValue++)
		{
			// Creation, initialisation et memorisation d'une paire triable
			sortableValue = new KWSortableSymbol;
			sortableValue->SetSortValue(svSymbolValueSet.GetAt(nValue));
			sortableValue->SetIndex(nValue);
			oaSortableValues.Add(sortableValue);

			// Test si StarValue
			if (svSymbolValueSet.GetAt(nValue) == Symbol::GetStarValue())
				nStarValueIndex = nValue;
		}

		// Tri des modalites
		oaSortableValues.SetCompareFunction(KWSortableSymbolCompare);
		oaSortableValues.Sort();

		// Initialisation de la taille des vecteurs de modalites et codes
		svSortedValues.SetSize(oaSortableValues.GetSize());
		ivValueIndexes.SetSize(svSortedValues.GetSize());

		// Compilation de ces vecteurs: on range les valeurs (et leur index)
		// par ordre de references des valeurs, ce qui permettra une recherche
		// rapide par dichotomie lors de l'utilisation de la regle
		for (nValue = 0; nValue < oaSortableValues.GetSize(); nValue++)
		{
			sortableValue = cast(KWSortableSymbol*, oaSortableValues.GetAt(nValue));

			// Rangement de la modalite et de son code dans les vecteurs
			svSortedValues.SetAt(nValue, sortableValue->GetSortValue());
			ivValueIndexes.SetAt(nValue, sortableValue->GetIndex());
		}

		// Nettoyage
		oaSortableValues.DeleteAll();

		// Memorisation de la fraicheur
		nDynamicCompileFreshness = nCompileFreshness;
	}
}

///////////////////////////////////////////////////////////////
// Classe KWDRInInterval

KWDRInInterval::KWDRInInterval()
{
	SetName("InInterval");
	SetLabel("In interval");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("IntervalBounds");
	GetSecondOperand()->SetType(KWType::Continuous);
	cLowerBound = KWContinuous::GetMissingValue();
	cUpperBound = KWContinuous::GetMissingValue();
}

KWDRInInterval::~KWDRInInterval() {}

KWDerivationRule* KWDRInInterval::Create() const
{
	return new KWDRInInterval;
}

Continuous KWDRInInterval::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cValue;

	require(Check());
	require(IsCompiled());

	cValue = GetSecondOperand()->GetContinuousValue(kwoObject);
	if (cValue > cLowerBound and cValue <= cUpperBound)
		return 1;
	else
		return 0;
}

boolean KWDRInInterval::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	KWDRIntervalBounds* intervalBounds;

	// Verification de la classe mere
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification des bornes
	if (bOk)
	{
		intervalBounds = cast(KWDRIntervalBounds*, GetFirstOperand()->GetDerivationRule());
		if (intervalBounds->GetIntervalBoundNumber() != 2)
		{
			bOk = false;
			AddError("Expected two interval bounds in first operand of the rule");
		}
	}
	return bOk;
}

void KWDRInInterval::Compile(KWClass* kwcOwnerClass)
{
	KWDRIntervalBounds* intervalBounds;

	require(CheckCompleteness(kwcOwnerClass));

	// Compilation ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Recherche des bornes
	intervalBounds = cast(KWDRIntervalBounds*, GetFirstOperand()->GetDerivationRule());
	cLowerBound = intervalBounds->GetIntervalBoundAt(0);
	cUpperBound = intervalBounds->GetIntervalBoundAt(1);
}

longint KWDRInInterval::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRInInterval) - sizeof(KWDerivationRule);
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////
// Classe KWDRInGroup

KWDRInGroup::KWDRInGroup()
{
	SetName("InGroup");
	SetLabel("In group");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("ValueGroup");
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRInGroup::~KWDRInGroup() {}

KWDerivationRule* KWDRInGroup::Create() const
{
	return new KWDRInGroup;
}

Continuous KWDRInGroup::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Symbol sValue;
	int nIndex;

	require(Check());
	require(IsCompiled());

	// Recherche de la valeur
	sValue = GetSecondOperand()->GetSymbolValue(kwoObject);

	// Recherche de l'appartenance de la valeur au groupe
	// Recherche sequentielle s'il y a peu de modalites
	if (svSortedValues.GetSize() <= 10)
	{
		for (nIndex = 0; nIndex < svSortedValues.GetSize(); nIndex++)
		{
			if (sValue == svSortedValues.GetAt(nIndex))
				return 1;
		}
		assert(nIndex == svSortedValues.GetSize());

		// Retourne 0 si non trouve
		return 0;
	}
	// Recherche dichotomique sinon
	else
	{
		int nLowerIndex;
		int nUpperIndex;

		// Initialisation des index extremites
		nLowerIndex = 0;
		nUpperIndex = svSortedValues.GetSize() - 1;

		// Recherche dichotomique de la modalite
		nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
		while (nLowerIndex + 1 < nUpperIndex)
		{
			assert(svSortedValues.GetAt(nLowerIndex) <= svSortedValues.GetAt(nUpperIndex));

			// Deplacement des bornes de recherche en fonction
			// de la comparaison avec la borne courante
			if (sValue <= svSortedValues.GetAt(nIndex))
				nUpperIndex = nIndex;
			else
				nLowerIndex = nIndex;

			// Modification du prochain intervalle teste
			nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
		}
		assert(nLowerIndex <= nUpperIndex);
		assert(nUpperIndex <= nLowerIndex + 1);

		// On compare par rapport aux deux index restant
		if (sValue == svSortedValues.GetAt(nLowerIndex))
			return 1;
		else if (sValue == svSortedValues.GetAt(nUpperIndex))
			return 1;
		// Retourne 0 si non trouve
		else
			return 0;
	}
}

void KWDRInGroup::Compile(KWClass* kwcOwnerClass)
{
	KWDRValueGroup* valueGroup;

	require(CheckCompleteness(kwcOwnerClass));

	// Compilation ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Recherche des valeurs pour les trier
	valueGroup = cast(KWDRValueGroup*, GetFirstOperand()->GetDerivationRule());
	svSortedValues.CopyFrom(valueGroup->GetValues());
	svSortedValues.SortKeys();
}

longint KWDRInGroup::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRInGroup) - sizeof(KWDerivationRule);
	lUsedMemory += svSortedValues.GetUsedMemory();
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////
// Classe KWDRPartId

KWDRPartId::KWDRPartId()
{
	SetName("PartId");
	SetLabel("Part Id of a value");
	SetType(KWType::Symbol);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);

	// Gestion de la compilation dynamique
	univariatePartition = NULL;
	nDynamicCompileFreshness = 0;
}

KWDRPartId::~KWDRPartId() {}

KWDerivationRule* KWDRPartId::Create() const
{
	return new KWDRPartId;
}

Symbol KWDRPartId::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Continuous cValue;
	Symbol sValue;
	int nIndex;

	require(Check());
	require(IsCompiled());
	require(KWType::IsSimple(GetSecondOperand()->GetType()));

	// Compilation dynamique si necessaire
	if (nDynamicCompileFreshness < nCompileFreshness)
	{
		// Recherche de la partition univariee
		univariatePartition = cast(KWDRUnivariatePartition*, GetFirstOperand()->GetStructureValue(kwoObject));
		assert(univariatePartition->GetAttributeType() == GetSecondOperand()->GetType());

		// Calcul des Ids de la partition
		svPartIds.SetSize(univariatePartition->GetPartNumber());
		for (nIndex = 0; nIndex < svPartIds.GetSize(); nIndex++)
			svPartIds.SetAt(nIndex, KWDRUnivariatePartition::ComputeId(nIndex));

		// Memorisation de la fraicheur
		nDynamicCompileFreshness = nCompileFreshness;
	}

	// Calcul de l'identifiant de la partie
	nIndex = -1;
	if (GetSecondOperand()->GetType() == KWType::Continuous)
	{
		cValue = GetSecondOperand()->GetContinuousValue(kwoObject);
		nIndex = univariatePartition->GetContinuousPartIndex(cValue);
	}
	else
	{
		sValue = GetSecondOperand()->GetSymbolValue(kwoObject);
		nIndex = univariatePartition->GetSymbolPartIndex(sValue);
	}
	assert(0 <= nIndex and nIndex < svPartIds.GetSize());
	return svPartIds.GetAt(nIndex);
}

longint KWDRPartId::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRPartId) - sizeof(KWDerivationRule);
	lUsedMemory += svPartIds.GetUsedMemory();
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////
// Classe KWDRIntervalId

KWDRIntervalId::KWDRIntervalId()
{
	SetName("IntervalId");
	SetLabel("Interval Id of a numerical value");
	GetFirstOperand()->SetStructureName("IntervalBounds");
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRIntervalId::~KWDRIntervalId() {}

KWDerivationRule* KWDRIntervalId::Create() const
{
	return new KWDRIntervalId;
}

///////////////////////////////////////////////////////////////
// Classe KWDRContinuousValueId

KWDRContinuousValueId::KWDRContinuousValueId()
{
	SetName("ValueId");
	SetLabel("Id of a numerical value in a set");
	GetFirstOperand()->SetStructureName("ValueSet");
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRContinuousValueId::~KWDRContinuousValueId() {}

KWDerivationRule* KWDRContinuousValueId::Create() const
{
	return new KWDRContinuousValueId;
}

///////////////////////////////////////////////////////////////
// Classe KWDRGroupId

KWDRGroupId::KWDRGroupId()
{
	SetName("GroupId");
	SetLabel("Group Id of a categorical value");
	GetFirstOperand()->SetStructureName("ValueGroups");
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRGroupId::~KWDRGroupId() {}

KWDerivationRule* KWDRGroupId::Create() const
{
	return new KWDRGroupId;
}

///////////////////////////////////////////////////////////////
// Classe KWDRSymbolValueId

KWDRSymbolValueId::KWDRSymbolValueId()
{
	SetName("ValueIdC");
	SetLabel("Id of a categorical value in a set");
	GetFirstOperand()->SetStructureName("ValueSetC");
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRSymbolValueId::~KWDRSymbolValueId() {}

KWDerivationRule* KWDRSymbolValueId::Create() const
{
	return new KWDRSymbolValueId;
}

///////////////////////////////////////////////////////////////
// Classe KWDRPartIndex

KWDRPartIndex::KWDRPartIndex()
{
	SetName("PartIndex");
	SetLabel("Part index of a value");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
}

KWDRPartIndex::~KWDRPartIndex() {}

KWDerivationRule* KWDRPartIndex::Create() const
{
	return new KWDRPartIndex;
}

Continuous KWDRPartIndex::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KWDRUnivariatePartition* univariatePartition;
	Continuous cValue;
	Symbol sValue;
	int nIndex;

	require(Check());
	require(IsCompiled());
	require(KWType::IsSimple(GetSecondOperand()->GetType()));

	// Recherche de la partition
	univariatePartition = cast(KWDRUnivariatePartition*, GetFirstOperand()->GetStructureValue(kwoObject));
	assert(univariatePartition->GetAttributeType() == GetSecondOperand()->GetType());

	// Calcul de l'identifiant de la partie
	nIndex = -1;
	if (GetSecondOperand()->GetType() == KWType::Continuous)
	{
		cValue = GetSecondOperand()->GetContinuousValue(kwoObject);
		nIndex = univariatePartition->GetContinuousPartIndex(cValue);
	}
	else
	{
		sValue = GetSecondOperand()->GetSymbolValue(kwoObject);
		nIndex = univariatePartition->GetSymbolPartIndex(sValue);
	}
	assert(0 <= nIndex and nIndex < univariatePartition->GetPartNumber());
	return (Continuous)(nIndex + 1);
}

///////////////////////////////////////////////////////////////
// Classe KWDRIntervalIndex

KWDRIntervalIndex::KWDRIntervalIndex()
{
	SetName("IntervalIndex");
	SetLabel("Interval index of a numerical value");
	GetFirstOperand()->SetStructureName("IntervalBounds");
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRIntervalIndex::~KWDRIntervalIndex() {}

KWDerivationRule* KWDRIntervalIndex::Create() const
{
	return new KWDRIntervalIndex;
}

///////////////////////////////////////////////////////////////
// Classe KWDRContinuousValueIndex

KWDRContinuousValueIndex::KWDRContinuousValueIndex()
{
	SetName("ValueIndex");
	SetLabel("Index of a numerical value in a set");
	GetFirstOperand()->SetStructureName("ValueSet");
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRContinuousValueIndex::~KWDRContinuousValueIndex() {}

KWDerivationRule* KWDRContinuousValueIndex::Create() const
{
	return new KWDRContinuousValueIndex;
}

///////////////////////////////////////////////////////////////
// Classe KWDRGroupIndex

KWDRGroupIndex::KWDRGroupIndex()
{
	SetName("GroupIndex");
	SetLabel("Group index of a categorical value");
	GetFirstOperand()->SetStructureName("ValueGroups");
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRGroupIndex::~KWDRGroupIndex() {}

KWDerivationRule* KWDRGroupIndex::Create() const
{
	return new KWDRGroupIndex;
}

///////////////////////////////////////////////////////////////
// Classe KWDRSymbolValueIndex

KWDRSymbolValueIndex::KWDRSymbolValueIndex()
{
	SetName("ValueIndexC");
	SetLabel("Index of a categorical value in a set");
	GetFirstOperand()->SetStructureName("ValueSetC");
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRSymbolValueIndex::~KWDRSymbolValueIndex() {}

KWDerivationRule* KWDRSymbolValueIndex::Create() const
{
	return new KWDRSymbolValueIndex;
}
