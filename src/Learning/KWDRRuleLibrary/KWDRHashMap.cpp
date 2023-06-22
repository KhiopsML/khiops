// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRHashMap.h"

void KWDRRegisterHashMapRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRSymbolHashMap);
	KWDerivationRule::RegisterDerivationRule(new KWDRContinuousHashMap);
	//
	KWDerivationRule::RegisterDerivationRule(new KWDRSymbolValueAtKey);
	KWDerivationRule::RegisterDerivationRule(new KWDRContinuousValueAtKey);
}

///////////////////////////////////////////////////////////////
// Classe KWDRSymbolHashMap

KWDRSymbolHashMap::KWDRSymbolHashMap()
{
	SetName("HashMapC");
	SetLabel("Hash map of categorical values");
	SetType(KWType::Structure);
	SetStructureName("HashMapC");
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("VectorC");
	GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
	GetFirstOperand()->SetDerivationRule(new KWDRSymbolVector);
	GetSecondOperand()->SetType(KWType::Structure);
	GetSecondOperand()->SetStructureName("VectorC");
	GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
	GetSecondOperand()->SetDerivationRule(new KWDRSymbolVector);
}

KWDRSymbolHashMap::~KWDRSymbolHashMap()
{
	nkdKeyIndexes.DeleteAll();
}

KWDerivationRule* KWDRSymbolHashMap::Create() const
{
	return new KWDRSymbolHashMap;
}

Object* KWDRSymbolHashMap::ComputeStructureResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsCompiled());
	return (Object*)this;
}

Symbol KWDRSymbolHashMap::LookupValue(const Symbol& sKey)
{
	KWDRSymbolVector* valueVector;
	KWSortableIndex* keyIndex;
	Symbol sValue;

	require(CheckDefinition() and
		nkdKeyIndexes.GetCount() ==
		    cast(KWDRSymbolVector*, GetFirstOperand()->GetDerivationRule())->GetValueNumber());

	// Recherche de l'association cle, index
	keyIndex = cast(KWSortableIndex*, nkdKeyIndexes.Lookup(sKey.GetNumericKey()));
	if (keyIndex != NULL)
	{
		valueVector = cast(KWDRSymbolVector*, GetSecondOperand()->GetDerivationRule());
		assert(0 <= keyIndex->GetIndex() and keyIndex->GetIndex() < valueVector->GetValueNumber());
		sValue = valueVector->GetValueAt(keyIndex->GetIndex());
	}
	return sValue;
}

boolean KWDRSymbolHashMap::CheckCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;

	bOk = KWDerivationRule::CheckCompleteness(kwcOwnerClass);

	// Verification des vecteurs de cle et valeurs
	if (bOk)
	{
		KWDRSymbolVector* keyVector;
		KWDRSymbolVector* valueVector;
		KWDRSymbolVector tmpKeyVector;
		KWDRSymbolVector tmpValueVector;
		KWDRSymbolVector* checkedKeyVector;
		KWDRSymbolVector* checkedValueVector;
		int nKey;
		Symbol sKey;
		NumericKeyDictionary nkdKeys;
		ALString sTmp;

		// Acces aux vecteurs de cle et valeur
		keyVector = cast(KWDRSymbolVector*, GetFirstOperand()->GetDerivationRule());
		valueVector = cast(KWDRSymbolVector*, GetSecondOperand()->GetDerivationRule());

		// Transfert des cles vers la representation structuree
		check(keyVector);
		if (keyVector->GetStructureInterface())
			checkedKeyVector = cast(KWDRSymbolVector*, GetFirstOperand()->GetDerivationRule());
		else
		{
			tmpKeyVector.BuildStructureFromBase(keyVector);
			checkedKeyVector = &tmpKeyVector;
		}

		// Transfert des valeurs vers la representation structuree
		check(valueVector);
		if (valueVector->GetStructureInterface())
			checkedValueVector = cast(KWDRSymbolVector*, GetSecondOperand()->GetDerivationRule());
		else
		{
			tmpValueVector.BuildStructureFromBase(valueVector);
			checkedValueVector = &tmpValueVector;
		}

		// Verification de la taille des vecteurs
		if (checkedKeyVector->GetValueNumber() != checkedValueVector->GetValueNumber())
		{
			bOk = false;
			AddError(sTmp + "Number of keys (" + IntToString(checkedKeyVector->GetValueNumber()) +
				 ") should be the same as the number of values (" +
				 IntToString(checkedValueVector->GetValueNumber()) + ")");
		}

		// Verification de l'unicite des cle
		for (nKey = 0; nKey < checkedKeyVector->GetValueNumber(); nKey++)
		{
			sKey = checkedKeyVector->GetValueAt(nKey);

			// Test si la cle a deja ete vue
			if (nkdKeys.Lookup(sKey.GetNumericKey()) != NULL)
			{
				bOk = false;
				AddError(sTmp + "Key " + sKey + " is used several times");
				break;
			}
			else
				nkdKeys.SetAt(sKey.GetNumericKey(), &nkdKeys);
		}
	}
	return bOk;
}

void KWDRSymbolHashMap::Compile(KWClass* kwcOwnerClass)
{
	require(CheckCompleteness(kwcOwnerClass));

	// Compilation ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Optimisation de la regle
	Optimize();
}

void KWDRSymbolHashMap::Optimize()
{
	KWDRSymbolVector* keyVector;
	int nKey;
	Symbol sKey;
	KWSortableIndex* keyIndex;

	require(CheckDefinition());

	// Memorisation de l'association entre cle et valeurs
	nkdKeyIndexes.DeleteAll();
	keyVector = cast(KWDRSymbolVector*, GetFirstOperand()->GetDerivationRule());
	for (nKey = 0; nKey < keyVector->GetValueNumber(); nKey++)
	{
		sKey = keyVector->GetValueAt(nKey);

		// Creation d'une association (cle, index) et memorisation dans un dictionnaire
		keyIndex = new KWSortableIndex;
		keyIndex->SetIndex(nKey);
		nkdKeyIndexes.SetAt(sKey.GetNumericKey(), keyIndex);
	}
}

longint KWDRSymbolHashMap::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRSymbolHashMap) - sizeof(KWDerivationRule);
	lUsedMemory += sizeof(nkdKeyIndexes) + nkdKeyIndexes.GetCount() * (3 * sizeof(void*) + sizeof(KWSortableIndex));
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////
// Classe KWDRContinuousHashMap

KWDRContinuousHashMap::KWDRContinuousHashMap()
{
	SetName("HashMap");
	SetLabel("Hash map of numerical values");
	SetType(KWType::Structure);
	SetStructureName("HashMap");
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("VectorC");
	GetFirstOperand()->SetDerivationRule(new KWDRSymbolVector);
	GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
	GetSecondOperand()->SetType(KWType::Structure);
	GetSecondOperand()->SetStructureName("Vector");
	GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
	GetSecondOperand()->SetDerivationRule(new KWDRContinuousVector);
}

KWDRContinuousHashMap::~KWDRContinuousHashMap()
{
	nkdKeyIndexes.DeleteAll();
}

KWDerivationRule* KWDRContinuousHashMap::Create() const
{
	return new KWDRContinuousHashMap;
}

Object* KWDRContinuousHashMap::ComputeStructureResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsCompiled());
	return (Object*)this;
}

Continuous KWDRContinuousHashMap::LookupValue(const Symbol& sKey)
{
	KWDRContinuousVector* valueVector;
	KWSortableIndex* keyIndex;
	Continuous cValue;

	require(CheckDefinition() and
		nkdKeyIndexes.GetCount() ==
		    cast(KWDRSymbolVector*, GetFirstOperand()->GetDerivationRule())->GetValueNumber());

	// Recherche de l'association cle, index
	keyIndex = cast(KWSortableIndex*, nkdKeyIndexes.Lookup(sKey.GetNumericKey()));
	if (keyIndex != NULL)
	{
		valueVector = cast(KWDRContinuousVector*, GetSecondOperand()->GetDerivationRule());
		assert(0 <= keyIndex->GetIndex() and keyIndex->GetIndex() < valueVector->GetValueNumber());
		cValue = valueVector->GetValueAt(keyIndex->GetIndex());
	}
	// Si pas trouve, on renvoie la valeur manquante
	else
		cValue = KWContinuous::GetMissingValue();
	return cValue;
}

boolean KWDRContinuousHashMap::CheckCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;

	bOk = KWDerivationRule::CheckCompleteness(kwcOwnerClass);

	// Verification des vecteurs de cle et valeurs
	if (bOk)
	{
		KWDRSymbolVector* keyVector;
		KWDRContinuousVector* valueVector;
		KWDRSymbolVector tmpKeyVector;
		KWDRContinuousVector tmpValueVector;
		KWDRSymbolVector* checkedKeyVector;
		KWDRContinuousVector* checkedValueVector;
		int nKey;
		Symbol sKey;
		NumericKeyDictionary nkdKeys;
		ALString sTmp;

		// Acces aux vecteurs de cle et valeur
		keyVector = cast(KWDRSymbolVector*, GetFirstOperand()->GetDerivationRule());
		valueVector = cast(KWDRContinuousVector*, GetSecondOperand()->GetDerivationRule());

		// Transfert des cles vers la representation structuree
		check(keyVector);
		if (keyVector->GetStructureInterface())
			checkedKeyVector = keyVector;
		else
		{
			tmpKeyVector.BuildStructureFromBase(keyVector);
			checkedKeyVector = &tmpKeyVector;
		}

		// Transfert des valeurs vers la representation structuree
		check(valueVector);
		if (valueVector->GetStructureInterface())
			checkedValueVector = valueVector;
		else
		{
			tmpValueVector.BuildStructureFromBase(valueVector);
			checkedValueVector = &tmpValueVector;
		}

		// Verification de la taille des vecteurs
		if (checkedKeyVector->GetValueNumber() != checkedValueVector->GetValueNumber())
		{
			bOk = false;
			AddError(sTmp + "Number of keys (" + IntToString(checkedKeyVector->GetValueNumber()) +
				 ") should be the same as the number of values (" +
				 IntToString(checkedValueVector->GetValueNumber()) + ")");
		}

		// Verification de l'unicite des cle
		for (nKey = 0; nKey < checkedKeyVector->GetValueNumber(); nKey++)
		{
			sKey = checkedKeyVector->GetValueAt(nKey);

			// Test si la cle a deja ete vue
			if (nkdKeys.Lookup(sKey.GetNumericKey()) != NULL)
			{
				bOk = false;
				AddError(sTmp + "Key " + sKey + " is used several times");
				break;
			}
			else
				nkdKeys.SetAt(sKey.GetNumericKey(), &nkdKeys);
		}
	}
	return bOk;
}

void KWDRContinuousHashMap::Compile(KWClass* kwcOwnerClass)
{
	require(CheckCompleteness(kwcOwnerClass));

	// Compilation ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Optimisation de la regle
	Optimize();
}

void KWDRContinuousHashMap::Optimize()
{
	KWDRSymbolVector* keyVector;
	int nKey;
	Symbol sKey;
	KWSortableIndex* keyIndex;

	require(CheckDefinition());

	// Memorisation de l'association entre cle et valeurs
	nkdKeyIndexes.DeleteAll();
	keyVector = cast(KWDRSymbolVector*, GetFirstOperand()->GetDerivationRule());
	for (nKey = 0; nKey < keyVector->GetValueNumber(); nKey++)
	{
		sKey = keyVector->GetValueAt(nKey);

		// Creation d''une association (cle, index) et memorisation dans un dictionnaire
		keyIndex = new KWSortableIndex;
		keyIndex->SetIndex(nKey);
		nkdKeyIndexes.SetAt(sKey.GetNumericKey(), keyIndex);
	}
}

longint KWDRContinuousHashMap::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRContinuousHashMap) - sizeof(KWDerivationRule);
	lUsedMemory += sizeof(nkdKeyIndexes) + nkdKeyIndexes.GetCount() * (3 * sizeof(void*) + sizeof(KWSortableIndex));
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////
// Classe KWDRSymbolValueAtKey

KWDRSymbolValueAtKey::KWDRSymbolValueAtKey()
{
	SetName("ValueAtKeyC");
	SetLabel("Categorical value at key");
	SetType(KWType::Symbol);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("HashMapC");
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRSymbolValueAtKey::~KWDRSymbolValueAtKey() {}

KWDerivationRule* KWDRSymbolValueAtKey::Create() const
{
	return new KWDRSymbolValueAtKey;
}

Symbol KWDRSymbolValueAtKey::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KWDRSymbolHashMap* SymbolHashMap;
	Symbol sKey;
	Symbol sValue;

	require(Check());
	require(IsCompiled());

	// Recherche de la partition
	SymbolHashMap = cast(KWDRSymbolHashMap*, GetFirstOperand()->GetStructureValue(kwoObject));

	// Recherche de la cle et de la valeur associee
	sKey = GetSecondOperand()->GetSymbolValue(kwoObject);
	sValue = SymbolHashMap->LookupValue(sKey);
	return sValue;
}

///////////////////////////////////////////////////////////////
// Classe KWDRContinuousValueAtKey

KWDRContinuousValueAtKey::KWDRContinuousValueAtKey()
{
	SetName("ValueAtKey");
	SetLabel("Numerical value at key");
	SetType(KWType::Continuous);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Structure);
	GetFirstOperand()->SetStructureName("HashMap");
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRContinuousValueAtKey::~KWDRContinuousValueAtKey() {}

KWDerivationRule* KWDRContinuousValueAtKey::Create() const
{
	return new KWDRContinuousValueAtKey;
}

Continuous KWDRContinuousValueAtKey::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KWDRContinuousHashMap* continuousHashMap;
	Symbol sKey;
	Continuous cValue;

	require(Check());
	require(IsCompiled());

	// Recherche de la partition
	continuousHashMap = cast(KWDRContinuousHashMap*, GetFirstOperand()->GetStructureValue(kwoObject));

	// Recherche de la cle et de la valeur associee
	sKey = GetSecondOperand()->GetSymbolValue(kwoObject);
	cValue = continuousHashMap->LookupValue(sKey);
	return cValue;
}