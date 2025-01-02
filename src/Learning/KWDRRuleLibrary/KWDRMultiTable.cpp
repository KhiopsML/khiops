// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRMultiTable.h"

void KWDRRegisterMultiTableRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRExist);
	KWDerivationRule::RegisterDerivationRule(new KWDRGetContinuousValue);
	KWDerivationRule::RegisterDerivationRule(new KWDRGetSymbolValue);
	KWDerivationRule::RegisterDerivationRule(new KWDRGetDateValue);
	KWDerivationRule::RegisterDerivationRule(new KWDRGetTimeValue);
	KWDerivationRule::RegisterDerivationRule(new KWDRGetTimestampValue);
	KWDerivationRule::RegisterDerivationRule(new KWDRGetTimestampTZValue);
	KWDerivationRule::RegisterDerivationRule(new KWDRGetTextValue);
	KWDerivationRule::RegisterDerivationRule(new KWDRGetObjectValue);
	KWDerivationRule::RegisterDerivationRule(new KWDRGetObjectArrayValue);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableAt);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableAtKey);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableSelectFirst);
	//
	KWDerivationRule::RegisterDerivationRule(new KWDRGetContinuousValueBlock);
	KWDerivationRule::RegisterDerivationRule(new KWDRGetSymbolValueBlock);
	//
	KWDerivationRule::RegisterDerivationRule(new KWDRTableUnion);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableIntersection);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableDifference);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableSubUnion);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableSubIntersection);
	KWDerivationRule::RegisterDerivationRule(new KWDREntitySet);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableExtraction);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableSelection);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableSort);
	//
	KWDerivationRule::RegisterDerivationRule(new KWDRTableCount);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableCountDistinct);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableEntropy);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableMode);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableModeAt);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableMean);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableStandardDeviation);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableMedian);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableMin);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableMax);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableSum);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableCountSum);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableTrend);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableConcat);
	//
	KWDerivationRule::RegisterDerivationRule(new KWDRTableSymbolVector);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableContinuousVector);
	//
	KWDerivationRule::RegisterDerivationRule(new KWDRTableSymbolHashMap);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableContinuousHashMap);
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRExist

KWDRExist::KWDRExist()
{
	SetName("Exist");
	SetLabel("Check if a sub-entity exists");
	SetType(KWType::Continuous);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Object);
}

KWDRExist::~KWDRExist() {}

KWDerivationRule* KWDRExist::Create() const
{
	return new KWDRExist;
}

Continuous KWDRExist::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KWObject* kwoContainedObject;

	require(IsCompiled());

	// Test de la presence
	kwoContainedObject = GetFirstOperand()->GetObjectValue(kwoObject);
	if (kwoContainedObject == NULL)
		return 0;
	else
		return 1;
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetContinuousValue

KWDRGetContinuousValue::KWDRGetContinuousValue()
{
	SetName("GetValue");
	SetLabel("Numerical value in a sub-entity");
	SetType(KWType::Continuous);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Object);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRGetContinuousValue::~KWDRGetContinuousValue() {}

KWDerivationRule* KWDRGetContinuousValue::Create() const
{
	return new KWDRGetContinuousValue;
}

Continuous KWDRGetContinuousValue::ComputeContinuousResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* valueOperand;
	KWObject* kwoContainedObject;
	Continuous cResult;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche de la valeur Continuous dans le sous-objet
	kwoContainedObject = GetFirstOperand()->GetObjectValue(kwoObject);
	if (kwoContainedObject == NULL)
		cResult = KWContinuous::GetMissingValue();
	else
	{
		valueOperand = GetSecondOperand();
		cResult = valueOperand->GetContinuousValue(kwoContainedObject);
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return cResult;
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetSymbolValue

KWDRGetSymbolValue::KWDRGetSymbolValue()
{
	SetName("GetValueC");
	SetLabel("Categorical value in a sub-entity");
	SetType(KWType::Symbol);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Object);
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRGetSymbolValue::~KWDRGetSymbolValue() {}

KWDerivationRule* KWDRGetSymbolValue::Create() const
{
	return new KWDRGetSymbolValue;
}

Symbol KWDRGetSymbolValue::ComputeSymbolResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* valueOperand;
	KWObject* kwoContainedObject;
	Symbol sResult;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche de la valeur Symbol dans le sous-objet
	kwoContainedObject = GetFirstOperand()->GetObjectValue(kwoObject);
	if (kwoContainedObject != NULL)
	{
		valueOperand = GetSecondOperand();
		sResult = valueOperand->GetSymbolValue(kwoContainedObject);
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return sResult;
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetDateValue

KWDRGetDateValue::KWDRGetDateValue()
{
	SetName("GetValueD");
	SetLabel("Date value in a sub-entity");
	SetType(KWType::Date);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Object);
	GetSecondOperand()->SetType(KWType::Date);
}

KWDRGetDateValue::~KWDRGetDateValue() {}

KWDerivationRule* KWDRGetDateValue::Create() const
{
	return new KWDRGetDateValue;
}

Date KWDRGetDateValue::ComputeDateResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* valueOperand;
	KWObject* kwoContainedObject;
	Date dResult;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche de la valeur Date dans le sous-objet
	kwoContainedObject = GetFirstOperand()->GetObjectValue(kwoObject);
	if (kwoContainedObject == NULL)
	{
		dResult.Reset();
	}
	else
	{
		valueOperand = GetSecondOperand();
		dResult = valueOperand->GetDateValue(kwoContainedObject);
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return dResult;
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetTimeValue

KWDRGetTimeValue::KWDRGetTimeValue()
{
	SetName("GetValueT");
	SetLabel("Time value in a sub-entity");
	SetType(KWType::Time);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Object);
	GetSecondOperand()->SetType(KWType::Time);
}

KWDRGetTimeValue::~KWDRGetTimeValue() {}

KWDerivationRule* KWDRGetTimeValue::Create() const
{
	return new KWDRGetTimeValue;
}

Time KWDRGetTimeValue::ComputeTimeResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* valueOperand;
	KWObject* kwoContainedObject;
	Time tResult;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche de la valeur Time dans le sous-objet
	kwoContainedObject = GetFirstOperand()->GetObjectValue(kwoObject);
	if (kwoContainedObject == NULL)
	{
		tResult.Reset();
	}
	else
	{
		valueOperand = GetSecondOperand();
		tResult = valueOperand->GetTimeValue(kwoContainedObject);
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return tResult;
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetTimestampValue

KWDRGetTimestampValue::KWDRGetTimestampValue()
{
	SetName("GetValueTS");
	SetLabel("Timestamp value in a sub-entity");
	SetType(KWType::Timestamp);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Object);
	GetSecondOperand()->SetType(KWType::Timestamp);
}

KWDRGetTimestampValue::~KWDRGetTimestampValue() {}

KWDerivationRule* KWDRGetTimestampValue::Create() const
{
	return new KWDRGetTimestampValue;
}

Timestamp KWDRGetTimestampValue::ComputeTimestampResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* valueOperand;
	KWObject* kwoContainedObject;
	Timestamp tsResult;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche de la valeur Timestamp dans le sous-objet
	kwoContainedObject = GetFirstOperand()->GetObjectValue(kwoObject);
	if (kwoContainedObject == NULL)
	{
		tsResult.Reset();
	}
	else
	{
		valueOperand = GetSecondOperand();
		tsResult = valueOperand->GetTimestampValue(kwoContainedObject);
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return tsResult;
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetTimestampTZValue

KWDRGetTimestampTZValue::KWDRGetTimestampTZValue()
{
	SetName("GetValueTSTZ");
	SetLabel("TimestampTZ value in a sub-entity");
	SetType(KWType::TimestampTZ);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Object);
	GetSecondOperand()->SetType(KWType::TimestampTZ);
}

KWDRGetTimestampTZValue::~KWDRGetTimestampTZValue() {}

KWDerivationRule* KWDRGetTimestampTZValue::Create() const
{
	return new KWDRGetTimestampTZValue;
}

TimestampTZ KWDRGetTimestampTZValue::ComputeTimestampTZResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* valueOperand;
	KWObject* kwoContainedObject;
	TimestampTZ tstzResult;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche de la valeur TimestampTZ dans le sous-objet
	kwoContainedObject = GetFirstOperand()->GetObjectValue(kwoObject);
	if (kwoContainedObject == NULL)
	{
		tstzResult.Reset();
	}
	else
	{
		valueOperand = GetSecondOperand();
		tstzResult = valueOperand->GetTimestampTZValue(kwoContainedObject);
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return tstzResult;
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetTextValue

KWDRGetTextValue::KWDRGetTextValue()
{
	SetName("GetText");
	SetLabel("Text value in a sub-entity");
	SetType(KWType::Text);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Object);
	GetSecondOperand()->SetType(KWType::Text);
}

KWDRGetTextValue::~KWDRGetTextValue() {}

KWDerivationRule* KWDRGetTextValue::Create() const
{
	return new KWDRGetTextValue;
}

Symbol KWDRGetTextValue::ComputeTextResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* valueOperand;
	KWObject* kwoContainedObject;
	Symbol sResult;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche de la valeur Symbol dans le sous-objet
	kwoContainedObject = GetFirstOperand()->GetObjectValue(kwoObject);
	if (kwoContainedObject != NULL)
	{
		valueOperand = GetSecondOperand();
		sResult = valueOperand->GetTextValue(kwoContainedObject);
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return sResult;
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetContinuousValueBlock

KWDRGetContinuousValueBlock::KWDRGetContinuousValueBlock()
{
	SetName("GetBlock");
	SetLabel("Numerical sparse value block in a sub-entity");
	SetType(KWType::ContinuousValueBlock);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Object);
	GetSecondOperand()->SetType(KWType::ContinuousValueBlock);
	nSourceValueBlockOperandIndex = 1;
}

KWDRGetContinuousValueBlock::~KWDRGetContinuousValueBlock() {}

KWDerivationRule* KWDRGetContinuousValueBlock::Create() const
{
	return new KWDRGetContinuousValueBlock;
}

KWContinuousValueBlock*
KWDRGetContinuousValueBlock::ComputeContinuousValueBlockResult(const KWObject* kwoObject,
							       const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWContinuousValueBlock* cvbResult;
	KWDerivationRuleOperand* valueOperand;
	KWObject* kwoContainedObject;

	require(IsCompiled());

	// Compilation dynamique
	DynamicCompile(indexedKeyBlock);

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche du bloc dans le sous-objet
	kwoContainedObject = GetFirstOperand()->GetObjectValue(kwoObject);
	if (kwoContainedObject == NULL)
	{
		cvbResult = KWContinuousValueBlock::NewValueBlock(0);
	}
	else
	{
		valueOperand = GetSecondOperand();
		assert(valueOperand->GetOrigin() != KWDerivationRuleOperand::OriginRule);

		// Extraction du sous-bloc
		if (bSameValueIndexes)
			cvbResult = valueOperand->GetContinuousValueBlock(kwoContainedObject)->Clone();
		else
			cvbResult = KWContinuousValueBlock::ExtractBlockSubset(
			    valueOperand->GetContinuousValueBlock(kwoContainedObject), &ivNewValueIndexes);
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return cvbResult;
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetSymbolValueBlock

KWDRGetSymbolValueBlock::KWDRGetSymbolValueBlock()
{
	SetName("GetBlockC");
	SetLabel("Categorical sparse value block in a sub-entity");
	SetType(KWType::SymbolValueBlock);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Object);
	GetSecondOperand()->SetType(KWType::SymbolValueBlock);
	nSourceValueBlockOperandIndex = 1;
}

KWDRGetSymbolValueBlock::~KWDRGetSymbolValueBlock() {}

KWDerivationRule* KWDRGetSymbolValueBlock::Create() const
{
	return new KWDRGetSymbolValueBlock;
}

KWSymbolValueBlock*
KWDRGetSymbolValueBlock::ComputeSymbolValueBlockResult(const KWObject* kwoObject,
						       const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWDerivationRuleOperand* valueOperand;
	KWObject* kwoContainedObject;
	KWSymbolValueBlock* svbResult;

	require(IsCompiled());

	// Compilation dynamique
	DynamicCompile(indexedKeyBlock);

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche du bloc dans le sous-objet
	kwoContainedObject = GetFirstOperand()->GetObjectValue(kwoObject);
	if (kwoContainedObject == NULL)
	{
		svbResult = KWSymbolValueBlock::NewValueBlock(0);
	}
	else
	{
		valueOperand = GetSecondOperand();
		assert(valueOperand->GetOrigin() != KWDerivationRuleOperand::OriginRule);

		// Extraction du sous-bloc
		if (bSameValueIndexes)
			svbResult = valueOperand->GetSymbolValueBlock(kwoContainedObject)->Clone();
		else
			svbResult = KWSymbolValueBlock::ExtractBlockSubset(
			    valueOperand->GetSymbolValueBlock(kwoContainedObject), &ivNewValueIndexes);
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return svbResult;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRObjectRule

boolean KWDRObjectRule::CheckCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	KWDerivationRuleOperand* valueOperand;

	require(kwcOwnerClass != NULL);

	// Methode ancetre
	bOk = KWDerivationRule::CheckCompleteness(kwcOwnerClass);

	// On verifie en plus la coherence avec le type de l'attribut retourne
	if (bOk and GetOperandNumber() >= 1)
	{
		valueOperand = GetFirstOperand();
		assert(KWType::IsRelation(GetType()));
		assert(KWType::IsRelation(valueOperand->GetType()));
		if (GetObjectClassName() != valueOperand->GetObjectClassName())
		{
			bOk = false;
			AddError("Dictionary (" + GetObjectClassName() + ") of " + KWType::ToString(GetType()) +
				 " returned by rule inconsistent with that of the first operand (" +
				 valueOperand->GetObjectClassName() + ")");
		}
	}

	return bOk;
}

void KWDRObjectRule::InternalCompleteTypeInfo(const KWClass* kwcOwnerClass,
					      NumericKeyDictionary* nkdCompletedAttributes)
{
	require(kwcOwnerClass != NULL);
	require(nkdCompletedAttributes != NULL);

	// Appel de la methode ancetre
	KWDerivationRule::InternalCompleteTypeInfo(kwcOwnerClass, nkdCompletedAttributes);

	// Le type retourne par la regle est le type Object du deuxieme operande
	if (GetOperandNumber() >= 1 and GetObjectClassName() == "")
		SetObjectClassName(GetFirstOperand()->GetObjectClassName());
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRObjectArrayRule

boolean KWDRObjectArrayRule::CheckObjectArray(const ObjectArray* oaObjects) const
{
	int nObject;
	KWObject* kwoObject;
	NumericKeyDictionary nkdObjects;

	require(oaObjects != NULL);

	// Parcours des elements du tableau
	for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
	{
		kwoObject = cast(KWObject*, oaObjects->GetAt(nObject));

		// Erreur si NULL
		if (kwoObject == NULL)
			return false;

		// Erreur si doublon
		if (nkdObjects.Lookup((NUMERIC)kwoObject) != NULL)
			return false;
		// Sinon, insertion dans un dictionnaire
		else
			nkdObjects.SetAt((NUMERIC)kwoObject, kwoObject);
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRObjectSetRule

boolean KWDRObjectSetRule::CheckCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	KWDerivationRuleOperand* valueOperandRef;
	KWDerivationRuleOperand* valueOperand;
	int nOperand;

	require(kwcOwnerClass != NULL);

	// Methode ancetre
	bOk = KWDerivationRule::CheckCompleteness(kwcOwnerClass);

	// On verifie en plus la coherence avec le type de l'attribut retourne
	if (bOk and GetOperandNumber() >= 1)
	{
		// Acces au premier operande donnant le classe de reference a utiliser pour le controle d'integrite
		valueOperandRef = GetFirstOperand();
		assert(KWType::IsRelation(GetType()));
		assert(KWType::IsRelation(valueOperandRef->GetType()));
		assert(GetObjectClassName() == valueOperandRef->GetObjectClassName());

		// Controle d'integrite des autres operandes
		for (nOperand = 1; nOperand < GetOperandNumber(); nOperand++)
		{
			valueOperand = GetOperandAt(nOperand);
			assert(KWType::IsRelation(valueOperand->GetType()));

			// Test si la classe d'Object est la meme
			if (valueOperand->GetObjectClassName() != valueOperandRef->GetObjectClassName())
			{
				bOk = false;
				AddError("Dictionary (" + valueOperand->GetObjectClassName() + ") of " +
					 KWType::ToString(GetType()) + " for operand " + IntToString(nOperand + 1) +
					 " inconsistent with that of the first operand (" +
					 valueOperandRef->GetObjectClassName() + ")");
				break;
			}
		}
	}

	return bOk;
}

int KWSortableObjectCompareKeyIndex(const void* elem1, const void* elem2)
{
	KWSortableObject* sortableObject1;
	KWSortableObject* sortableObject2;
	KWObject* kwoObject1;
	KWObject* kwoObject2;
	int nCompare;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux objets
	sortableObject1 = cast(KWSortableObject*, *(Object**)elem1);
	sortableObject2 = cast(KWSortableObject*, *(Object**)elem2);

	// Comparaison selon lma cle des objets
	kwoObject1 = cast(KWObject*, sortableObject1->GetSortValue());
	kwoObject2 = cast(KWObject*, sortableObject2->GetSortValue());
	nCompare = KWObjectCompareKey(&kwoObject1, &kwoObject2);

	// En cas d'agalite, tri selon l'index
	if (nCompare == 0)
		nCompare = sortableObject1->GetIndex() - sortableObject2->GetIndex();
	return nCompare;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRSubObjectRule

boolean KWDRSubObjectRule::CheckCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	KWDerivationRuleOperand* valueOperand;

	require(kwcOwnerClass != NULL);

	// Methode ancetre
	bOk = KWDerivationRule::CheckCompleteness(kwcOwnerClass);

	// On verifie en plus la coherence avec le type de l'attribut retourne
	if (bOk and GetOperandNumber() >= 2)
	{
		valueOperand = GetSecondOperand();
		assert(KWType::IsRelation(GetType()));
		assert(KWType::IsRelation(valueOperand->GetType()));
		if (GetObjectClassName() != valueOperand->GetObjectClassName())
		{
			bOk = false;
			AddError("Dictionary (" + GetObjectClassName() + ") of " + KWType::ToString(GetType()) +
				 " returned by rule inconsistent with that of the second operand (" +
				 valueOperand->GetObjectClassName() + ")");
		}
	}

	return bOk;
}

void KWDRSubObjectRule::InternalCompleteTypeInfo(const KWClass* kwcOwnerClass,
						 NumericKeyDictionary* nkdCompletedAttributes)
{
	require(kwcOwnerClass != NULL);
	require(nkdCompletedAttributes != NULL);

	// Appel de la methode ancetre
	KWDerivationRule::InternalCompleteTypeInfo(kwcOwnerClass, nkdCompletedAttributes);

	// Le type retourne par la regle est le type Object du deuxieme operande
	if (GetOperandNumber() >= 2 and GetObjectClassName() == "")
		SetObjectClassName(GetSecondOperand()->GetObjectClassName());
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetObjectValue

KWDRGetObjectValue::KWDRGetObjectValue()
{
	SetName("GetEntity");
	SetLabel("Entity value in a sub-entity");
	SetType(KWType::Object);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Object);
	GetSecondOperand()->SetType(KWType::Object);
}

KWDRGetObjectValue::~KWDRGetObjectValue() {}

KWDerivationRule* KWDRGetObjectValue::Create() const
{
	return new KWDRGetObjectValue;
}

KWObject* KWDRGetObjectValue::ComputeObjectResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* valueOperand;
	KWObject* kwoContainedObject;
	KWObject* kwoResult;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche de la valeur Object dans le sous-objet
	kwoContainedObject = GetFirstOperand()->GetObjectValue(kwoObject);
	if (kwoContainedObject == NULL)
		kwoResult = NULL;
	else
	{
		valueOperand = GetSecondOperand();
		kwoResult = valueOperand->GetObjectValue(kwoContainedObject);
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return kwoResult;
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRGetObjectArrayValue

KWDRGetObjectArrayValue::KWDRGetObjectArrayValue()
{
	SetName("GetTable");
	SetLabel("Table value in a sub-entity");
	SetType(KWType::ObjectArray);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::Object);
	GetSecondOperand()->SetType(KWType::ObjectArray);
}

KWDRGetObjectArrayValue::~KWDRGetObjectArrayValue() {}

KWDerivationRule* KWDRGetObjectArrayValue::Create() const
{
	return new KWDRGetObjectArrayValue;
}

ObjectArray* KWDRGetObjectArrayValue::ComputeObjectArrayResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* valueOperand;
	KWObject* kwoContainedObject;
	ObjectArray* oaResult;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche de la valeur ObjectArray dans le sous-objet
	kwoContainedObject = GetFirstOperand()->GetObjectValue(kwoObject);
	if (kwoContainedObject == NULL)
		oaResult = NULL;
	else
	{
		valueOperand = GetSecondOperand();
		oaResult = valueOperand->GetObjectArrayValue(kwoContainedObject);
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return oaResult;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableAt

KWDRTableAt::KWDRTableAt()
{
	SetName("TableAt");
	SetLabel("Extraction of one entity from a table from its rank");
	SetType(KWType::Object);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRTableAt::~KWDRTableAt() {}

KWDerivationRule* KWDRTableAt::Create() const
{
	return new KWDRTableAt;
}

KWObject* KWDRTableAt::ComputeObjectResult(const KWObject* kwoObject) const
{
	ObjectArray* oaContainedObjects;
	int nIndex;

	require(IsCompiled());

	// Recherche de la valeur ObjectArray
	oaContainedObjects = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaContainedObjects != NULL)
	{
		// Recherche du rang
		nIndex = (int)floor(GetSecondOperand()->GetContinuousValue(kwoObject) + 0.5);

		// On renvoie l'objet si son rang est valide
		if (1 <= nIndex and nIndex <= oaContainedObjects->GetSize())
			return (KWObject*)oaContainedObjects->GetAt(nIndex - 1);
		// On retourne NULL sinon
		else
			return NULL;
	}

	// On renvoie NULL si aucun objet n'est trouve
	return NULL;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableAtKey

KWDRTableAtKey::KWDRTableAtKey()
{
	SetName("TableAtKey");
	SetLabel("Extraction of one entity from a table from its key");
	SetType(KWType::Object);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::Symbol);
	SetVariableOperandNumber(true);
}

KWDRTableAtKey::~KWDRTableAtKey() {}

KWDerivationRule* KWDRTableAtKey::Create() const
{
	return new KWDRTableAtKey;
}

KWObject* KWDRTableAtKey::ComputeObjectResult(const KWObject* kwoObject) const
{
	ObjectArray* oaContainedObjects;
	int nObject;
	KWObject* kwoContainedObject;
	const KWClass* kwcSecondaryScopeClass;
	int nKey;
	Symbol sKey;
	boolean bKeyMatch;

	require(IsCompiled());

	// Recherche de la valeur ObjectArray
	oaContainedObjects = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaContainedObjects != NULL)
	{
		// Parcours des objets du tableau
		for (nObject = 0; nObject < oaContainedObjects->GetSize(); nObject++)
		{
			kwoContainedObject = cast(KWObject*, oaContainedObjects->GetAt(nObject));
			if (kwoContainedObject != NULL)
			{
				// Test si matching de la cle
				kwcSecondaryScopeClass = kwoContainedObject->GetClass();
				bKeyMatch = true;
				for (nKey = 0; nKey < kwcSecondaryScopeClass->GetKeyAttributeNumber(); nKey++)
				{
					sKey = GetOperandAt(nKey + 1)->GetSymbolValue(kwoObject);
					if (sKey != kwoContainedObject->GetSymbolValueAt(
							kwcSecondaryScopeClass->GetKeyAttributeLoadIndexAt(nKey)))
					{
						bKeyMatch = false;
						break;
					}
				}

				// On renvoie le premier objet contenu ayant la bonne valeur de cle
				if (bKeyMatch)
					return kwoContainedObject;
			}
		}
	}

	// On renvoie NULL si aucun objet n'a la bonne valeur de cle
	return NULL;
}

boolean KWDRTableAtKey::CheckCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk;
	KWDerivationRuleOperand* tableOperand;
	KWClass* kwcSecondaryScopeClass;
	int nKeyAttributeNumber;
	ALString sTmp;

	require(kwcOwnerClass != NULL);

	// Methode ancetre
	bOk = KWDRObjectRule::CheckCompleteness(kwcOwnerClass);

	// Test du nombre d'operande de la cle
	if (bOk)
	{
		assert(GetOperandNumber() >= 1);

		// Acces au premier operand de type ObjectArray
		tableOperand = GetFirstOperand();
		assert(KWType::IsRelation(GetType()));
		assert(KWType::IsRelation(tableOperand->GetType()));
		assert(tableOperand->GetObjectClassName() != "");

		// Recherche de la classe de l'ObjectArray
		kwcSecondaryScopeClass = kwcOwnerClass->GetDomain()->LookupClass(tableOperand->GetObjectClassName());
		assert(kwcSecondaryScopeClass != NULL);

		// Test de la compatibilite entre la cle de cette classe et les valeurs de la cle en operandes
		nKeyAttributeNumber = kwcSecondaryScopeClass->GetKeyAttributeNumber();
		if (nKeyAttributeNumber == 0)
		{
			bOk = false;
			AddError("Dictionary (" + tableOperand->GetObjectClassName() + ") of " +
				 KWType::ToString(tableOperand->GetType()) + " of first operand should have a key");
		}
		else if (nKeyAttributeNumber != GetOperandNumber() - 1)
		{
			bOk = false;
			AddError(sTmp + "Number of key value operands (" + IntToString(GetOperandNumber() - 1) +
				 ") should match that of the key (length=" + IntToString(nKeyAttributeNumber) +
				 ") of dictionary (" + tableOperand->GetObjectClassName() + ") of " +
				 KWType::ToString(tableOperand->GetType()) + " of the first operand");
		}
	}
	return bOk;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableSelectFirst

KWDRTableSelectFirst::KWDRTableSelectFirst()
{
	SetName("TableSelectFirst");
	SetLabel("Select first element of a table for a given selection criterion");
	SetType(KWType::Object);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRTableSelectFirst::~KWDRTableSelectFirst() {}

KWDerivationRule* KWDRTableSelectFirst::Create() const
{
	return new KWDRTableSelectFirst;
}

KWObject* KWDRTableSelectFirst::ComputeObjectResult(const KWObject* kwoObject) const
{
	KWObject* kwoResult;
	ObjectArray* oaObjectArrayOperand;
	KWDerivationRuleOperand* selectionOperand;
	KWObject* kwoContainedObject;
	int nObject;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Extraction des Object verifiant la selection
	kwoResult = NULL;
	oaObjectArrayOperand = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0)
	{
		// Acces a l'operande de selection
		selectionOperand = GetSecondOperand();

		// Extraction des objets du container
		for (nObject = 0; nObject < oaObjectArrayOperand->GetSize(); nObject++)
		{
			kwoContainedObject = cast(KWObject*, oaObjectArrayOperand->GetAt(nObject));

			// Utilisation ou non de l'operande de selection
			if (kwoContainedObject != NULL and
			    selectionOperand->GetContinuousValue(kwoContainedObject) != 0)
			{
				kwoResult = kwoContainedObject;
				break;
			}
		}
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return kwoResult;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableUnion

KWDRTableUnion::KWDRTableUnion()
{
	SetName("TableUnion");
	SetLabel("Union of tables");
	SetType(KWType::ObjectArray);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::ObjectArray);
	SetVariableOperandNumber(true);
}

KWDRTableUnion::~KWDRTableUnion() {}

KWDerivationRule* KWDRTableUnion::Create() const
{
	return new KWDRTableUnion;
}

ObjectArray* KWDRTableUnion::ComputeObjectArrayResult(const KWObject* kwoObject) const
{
	int nOperand;
	ObjectArray* oaObjectArrayOperand;
	int nObject;
	KWObject* kwoContainedObject;
	NumericKeyDictionary nkdAllObjects;

	require(IsCompiled());

	// Calcul de l'Union
	oaResult.SetSize(0);

	// Cas d'un seul operande
	if (GetOperandNumber() == 1)
	{
		oaObjectArrayOperand = GetOperandAt(0)->GetObjectArrayValue(kwoObject);

		// Recopie du tableau d'entree
		if (oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0)
		{
			assert(CheckObjectArray(oaObjectArrayOperand));
			oaResult.CopyFrom(oaObjectArrayOperand);
		}
	}
	// Cas de plusieurs operandes
	else
	{
		// Collecte des objets participants a l'union
		for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
		{
			oaObjectArrayOperand = GetOperandAt(nOperand)->GetObjectArrayValue(kwoObject);

			// Ajout si non null et non vide
			if (oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0)
			{
				assert(CheckObjectArray(oaObjectArrayOperand));

				// Parcours des objets du tableau
				for (nObject = 0; nObject < oaObjectArrayOperand->GetSize(); nObject++)
				{
					kwoContainedObject = cast(KWObject*, oaObjectArrayOperand->GetAt(nObject));

					// Prise en compte sauf si doublon
					if (nkdAllObjects.Lookup((NUMERIC)kwoContainedObject) == NULL)
					{
						// Ajout dans le tableau resultat
						oaResult.Add(kwoContainedObject);

						// Memorisation dans le dictionnaire assurant l'unicite des elements
						nkdAllObjects.SetAt((NUMERIC)kwoContainedObject, kwoContainedObject);
					}
				}
			}
		}
	}
	ensure(CheckObjectArray(&oaResult));
	return &oaResult;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableIntersection

KWDRTableIntersection::KWDRTableIntersection()
{
	SetName("TableIntersection");
	SetLabel("Intersection of tables");
	SetType(KWType::ObjectArray);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::ObjectArray);
	SetVariableOperandNumber(true);
}

KWDRTableIntersection::~KWDRTableIntersection() {}

KWDerivationRule* KWDRTableIntersection::Create() const
{
	return new KWDRTableIntersection;
}

ObjectArray* KWDRTableIntersection::ComputeObjectArrayResult(const KWObject* kwoObject) const
{
	int nOperand;
	ObjectArray* oaObjectArrayOperand;
	int nObject;
	KWObject* kwoContainedObject;
	NumericKeyDictionary nkdObjects1;
	NumericKeyDictionary nkdObjects2;
	NumericKeyDictionary* nkdReferenceObjects;
	NumericKeyDictionary* nkdSelectedObjects;
	NumericKeyDictionary* nkdTmpObjects;

	require(IsCompiled());

	// Calcul de l'Union
	oaResult.SetSize(0);

	// Cas d'un seul operande
	if (GetOperandNumber() == 1)
	{
		oaObjectArrayOperand = GetOperandAt(0)->GetObjectArrayValue(kwoObject);

		// Recopie du tableau d'entree
		if (oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0)
		{
			assert(CheckObjectArray(oaObjectArrayOperand));
			oaResult.CopyFrom(oaObjectArrayOperand);
		}
	}
	// Cas de plusieurs operandes
	else
	{
		// Parametrage des dictionnaires de travail
		nkdReferenceObjects = &nkdObjects1;
		nkdSelectedObjects = &nkdObjects2;

		// Initialisation du dictionnaire de reference avec les objets du premier operande
		oaObjectArrayOperand = GetOperandAt(0)->GetObjectArrayValue(kwoObject);
		if (oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0)
		{
			assert(CheckObjectArray(oaObjectArrayOperand));

			// Memorisation des objets dans le dictionnaire de reference
			for (nObject = 0; nObject < oaObjectArrayOperand->GetSize(); nObject++)
			{
				kwoContainedObject = cast(KWObject*, oaObjectArrayOperand->GetAt(nObject));
				nkdReferenceObjects->SetAt((NUMERIC)kwoContainedObject, kwoContainedObject);
			}
		}

		// Parcours des tableau operandes suivants pour determiner les objets de l'intersection
		for (nOperand = 1; nOperand < GetOperandNumber(); nOperand++)
		{
			oaObjectArrayOperand = GetOperandAt(nOperand)->GetObjectArrayValue(kwoObject);

			// Arret si intersection vide
			if (nkdReferenceObjects->GetCount() == 0)
				break;

			// Prise en compte si non null et non vide
			if (oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0)
			{
				assert(CheckObjectArray(oaObjectArrayOperand));

				// Parcours des objets du tableau
				nkdSelectedObjects->RemoveAll();
				for (nObject = 0; nObject < oaObjectArrayOperand->GetSize(); nObject++)
				{
					kwoContainedObject = cast(KWObject*, oaObjectArrayOperand->GetAt(nObject));

					// Prise en compte si existant dans le dictionnaire de reference
					if (nkdReferenceObjects->Lookup((NUMERIC)kwoContainedObject) != NULL)
						nkdSelectedObjects->SetAt((NUMERIC)kwoContainedObject,
									  kwoContainedObject);
				}

				// On change de dictionnaire de reference
				nkdTmpObjects = nkdReferenceObjects;
				nkdReferenceObjects = nkdSelectedObjects;
				nkdSelectedObjects = nkdTmpObjects;
			}
			// Sinon, l'intersection est vide
			else
				nkdReferenceObjects->RemoveAll();
		}

		// Rangement des objets de l'intersection dans le tableau resultat, en parcourant le premier container
		if (nkdReferenceObjects->GetCount() > 0)
		{
			oaObjectArrayOperand = GetOperandAt(0)->GetObjectArrayValue(kwoObject);
			assert(oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0);

			// Memorisation des objets de l'intersection dans l'ordre du premier tableau d'entree
			for (nObject = 0; nObject < oaObjectArrayOperand->GetSize(); nObject++)
			{
				kwoContainedObject = cast(KWObject*, oaObjectArrayOperand->GetAt(nObject));
				if (nkdReferenceObjects->Lookup((NUMERIC)kwoContainedObject) != NULL)
					oaResult.Add(kwoContainedObject);
			}
		}
		assert(oaResult.GetSize() == nkdReferenceObjects->GetCount());
	}

	ensure(CheckObjectArray(&oaResult));
	return &oaResult;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableDifference

KWDRTableDifference::KWDRTableDifference()
{
	SetName("TableDifference");
	SetLabel("Difference of two tables");
	SetType(KWType::ObjectArray);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::ObjectArray);
}

KWDRTableDifference::~KWDRTableDifference() {}

KWDerivationRule* KWDRTableDifference::Create() const
{
	return new KWDRTableDifference;
}

ObjectArray* KWDRTableDifference::ComputeObjectArrayResult(const KWObject* kwoObject) const
{
	ObjectArray* oaObjectArrayOperand1;
	ObjectArray* oaObjectArrayOperand2;
	int nObject;
	KWObject* kwoContainedObject;
	NumericKeyDictionary nkdObjects1;
	NumericKeyDictionary nkdIntersectionObjects;

	require(IsCompiled());

	// Calcul de la difference
	oaResult.SetSize(0);
	oaObjectArrayOperand1 = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	oaObjectArrayOperand2 = GetSecondOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjectArrayOperand1 != NULL and oaObjectArrayOperand2 != NULL)
	{
		assert(CheckObjectArray(oaObjectArrayOperand1));
		assert(CheckObjectArray(oaObjectArrayOperand2));

		// Calcul des elements de l'intersection
		for (nObject = 0; nObject < oaObjectArrayOperand1->GetSize(); nObject++)
		{
			kwoContainedObject = cast(KWObject*, oaObjectArrayOperand1->GetAt(nObject));
			nkdObjects1.SetAt((NUMERIC)kwoContainedObject, kwoContainedObject);
		}
		for (nObject = 0; nObject < oaObjectArrayOperand2->GetSize(); nObject++)
		{
			kwoContainedObject = cast(KWObject*, oaObjectArrayOperand2->GetAt(nObject));
			if (nkdObjects1.Lookup((NUMERIC)kwoContainedObject) != NULL)
				nkdIntersectionObjects.SetAt((NUMERIC)kwoContainedObject, kwoContainedObject);
		}
		nkdObjects1.RemoveAll();

		// Collecte des objets de la difference (ne participant pas a l'intersection)
		for (nObject = 0; nObject < oaObjectArrayOperand1->GetSize(); nObject++)
		{
			kwoContainedObject = cast(KWObject*, oaObjectArrayOperand1->GetAt(nObject));
			if (nkdIntersectionObjects.Lookup((NUMERIC)kwoContainedObject) == NULL)
				oaResult.Add(kwoContainedObject);
		}
		for (nObject = 0; nObject < oaObjectArrayOperand2->GetSize(); nObject++)
		{
			kwoContainedObject = cast(KWObject*, oaObjectArrayOperand2->GetAt(nObject));
			if (nkdIntersectionObjects.Lookup((NUMERIC)kwoContainedObject) == NULL)
				oaResult.Add(kwoContainedObject);
		}
	}
	ensure(CheckObjectArray(&oaResult));
	return &oaResult;
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableSubUnion

KWDRTableSubUnion::KWDRTableSubUnion()
{
	SetName("TableSubUnion");
	SetLabel("Table union over the sub-tables of a table");
	SetType(KWType::ObjectArray);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::ObjectArray);
}

KWDRTableSubUnion::~KWDRTableSubUnion() {}

KWDerivationRule* KWDRTableSubUnion::Create() const
{
	return new KWDRTableSubUnion;
}

ObjectArray* KWDRTableSubUnion::ComputeObjectArrayResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* valueOperand;
	ObjectArray* oaContainedObjects;
	ObjectArray* oaObjectArrayOperand;
	int nObject;
	int nSubObject;
	KWObject* kwoContainedObject;
	KWObject* kwoContainedSubObject;
	NumericKeyDictionary nkdAllObjects;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche de la table
	oaResult.SetSize(0);
	oaContainedObjects = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaContainedObjects != NULL and oaContainedObjects->GetSize() > 0)
	{
		valueOperand = GetSecondOperand();

		// Cas particulier d'un seul renregistrement dans la table
		if (oaContainedObjects->GetSize() == 1)
		{
			// Recopie directe dans la table resultat
			kwoContainedObject = cast(KWObject*, oaContainedObjects->GetAt(0));
			if (kwoContainedObject != NULL)
			{
				oaObjectArrayOperand = valueOperand->GetObjectArrayValue(kwoContainedObject);
				if (oaObjectArrayOperand != NULL)
					oaResult.CopyFrom(oaObjectArrayOperand);
			}
		}
		// Sinon, parcours des sous-tables associe a chaque enregistrement
		else
		{
			// Parcours des objets de la table
			for (nObject = 0; nObject < oaContainedObjects->GetSize(); nObject++)
			{
				kwoContainedObject = cast(KWObject*, oaContainedObjects->GetAt(nObject));

				// Acces a la sous-table
				if (kwoContainedObject != NULL)
				{
					oaObjectArrayOperand = valueOperand->GetObjectArrayValue(kwoContainedObject);

					// Parcours des objets du tableau
					if (oaObjectArrayOperand != NULL)
					{
						for (nSubObject = 0; nSubObject < oaObjectArrayOperand->GetSize();
						     nSubObject++)
						{
							kwoContainedSubObject =
							    cast(KWObject*, oaObjectArrayOperand->GetAt(nSubObject));

							// Prise en compte sauf si doublon
							if (nkdAllObjects.Lookup((NUMERIC)kwoContainedSubObject) ==
							    NULL)
							{
								// Ajout dans le tableau resultat
								oaResult.Add(kwoContainedSubObject);

								// Memorisation dans le dictionnaire assurant l'unicite
								// des elements
								nkdAllObjects.SetAt((NUMERIC)kwoContainedSubObject,
										    kwoContainedSubObject);
							}
						}
					}
				}
			}
		}
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return &oaResult;
}

//////////////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableSubIntersection

KWDRTableSubIntersection::KWDRTableSubIntersection()
{
	SetName("TableSubIntersection");
	SetLabel("Table intersection over the sub-tables of a table");
	SetType(KWType::ObjectArray);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::ObjectArray);
}

KWDRTableSubIntersection::~KWDRTableSubIntersection() {}

KWDerivationRule* KWDRTableSubIntersection::Create() const
{
	return new KWDRTableSubIntersection;
}

ObjectArray* KWDRTableSubIntersection::ComputeObjectArrayResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* valueOperand;
	ObjectArray* oaContainedObjects;
	ObjectArray* oaObjectArrayOperand;
	int nObject;
	int nSubObject;
	KWObject* kwoContainedObject;
	KWObject* kwoContainedSubObject;
	NumericKeyDictionary nkdSubObjects1;
	NumericKeyDictionary nkdSubObjects2;
	NumericKeyDictionary* nkdReferenceSubObjects;
	NumericKeyDictionary* nkdSelectedSubObjects;
	NumericKeyDictionary* nkdTmpSubObjects;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche de la table
	oaResult.SetSize(0);
	oaContainedObjects = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaContainedObjects != NULL and oaContainedObjects->GetSize() > 0)
	{
		valueOperand = GetSecondOperand();

		// Cas particulier d'un seul renregistrement dans la table
		if (oaContainedObjects->GetSize() == 1)
		{
			// Recopie directe dans la table resultat
			kwoContainedObject = cast(KWObject*, oaContainedObjects->GetAt(0));
			if (kwoContainedObject != NULL)
			{
				oaObjectArrayOperand = valueOperand->GetObjectArrayValue(kwoContainedObject);
				if (oaObjectArrayOperand != NULL)
					oaResult.CopyFrom(oaObjectArrayOperand);
			}
		}
		// Cas de plusieurs operandes
		else
		{
			// Parametrage des dictionnaires de travail
			nkdReferenceSubObjects = &nkdSubObjects1;
			nkdSelectedSubObjects = &nkdSubObjects2;

			// Initialisation du dictionnaire de reference avec les objets du premier operande
			kwoContainedObject = cast(KWObject*, oaContainedObjects->GetAt(0));
			if (kwoContainedObject != NULL)
			{
				oaObjectArrayOperand = valueOperand->GetObjectArrayValue(kwoContainedObject);
				if (oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0)
				{
					// Memorisation des sous-objets dans le dictionnaire de reference
					for (nSubObject = 0; nSubObject < oaObjectArrayOperand->GetSize(); nSubObject++)
					{
						kwoContainedSubObject =
						    cast(KWObject*, oaObjectArrayOperand->GetAt(nSubObject));
						nkdReferenceSubObjects->SetAt((NUMERIC)kwoContainedSubObject,
									      kwoContainedSubObject);
					}
				}
			}

			// Parcours des objets suivants de la table
			for (nObject = 1; nObject < oaContainedObjects->GetSize(); nObject++)
			{
				kwoContainedObject = cast(KWObject*, oaContainedObjects->GetAt(nObject));

				// Arret si intersection vide
				if (nkdReferenceSubObjects->GetCount() == 0)
					break;

				// Acces a la sous-table
				if (kwoContainedObject != NULL)
				{
					oaObjectArrayOperand = valueOperand->GetObjectArrayValue(kwoContainedObject);

					// Parcours des objets du tableau
					nkdSelectedSubObjects->RemoveAll();
					if (oaObjectArrayOperand != NULL)
					{
						for (nSubObject = 0; nSubObject < oaObjectArrayOperand->GetSize();
						     nSubObject++)
						{
							kwoContainedSubObject =
							    cast(KWObject*, oaObjectArrayOperand->GetAt(nSubObject));

							// Prise en compte si existant dans le dictionnaire de reference
							if (nkdReferenceSubObjects->Lookup(
								(NUMERIC)kwoContainedSubObject) != NULL)
								nkdSelectedSubObjects->SetAt(
								    (NUMERIC)kwoContainedSubObject,
								    kwoContainedSubObject);
						}
					}

					// On change de dictionnaire de reference
					nkdTmpSubObjects = nkdReferenceSubObjects;
					nkdReferenceSubObjects = nkdSelectedSubObjects;
					nkdSelectedSubObjects = nkdTmpSubObjects;
				}
				// Sinon, l'intersection est vide
				else
					nkdReferenceSubObjects->RemoveAll();
			}

			// Rangement des objets de l'intersection dans le tableau resultat, en parcourant le premier
			// container
			if (nkdReferenceSubObjects->GetCount() > 0)
			{
				kwoContainedObject = cast(KWObject*, oaContainedObjects->GetAt(0));
				assert(kwoContainedObject != NULL);
				oaObjectArrayOperand = valueOperand->GetObjectArrayValue(kwoContainedObject);
				assert(oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0);

				// Memorisation des objets de l'intersection dans l'ordre du premier tableau d'entree
				for (nSubObject = 0; nSubObject < oaObjectArrayOperand->GetSize(); nSubObject++)
				{
					kwoContainedSubObject =
					    cast(KWObject*, oaObjectArrayOperand->GetAt(nSubObject));
					if (nkdReferenceSubObjects->Lookup((NUMERIC)kwoContainedSubObject) != NULL)
						oaResult.Add(kwoContainedSubObject);
				}
			}
			assert(oaResult.GetSize() == nkdReferenceSubObjects->GetCount());
		}
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return &oaResult;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDREntitySet

KWDREntitySet::KWDREntitySet()
{
	SetName("EntitySet");
	SetLabel("Table from a set of entities");
	SetType(KWType::ObjectArray);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::Object);
	SetVariableOperandNumber(true);
}

KWDREntitySet::~KWDREntitySet() {}

KWDerivationRule* KWDREntitySet::Create() const
{
	return new KWDREntitySet;
}

ObjectArray* KWDREntitySet::ComputeObjectArrayResult(const KWObject* kwoObject) const
{
	int nOperand;
	KWObject* kwoObjectOperand;
	NumericKeyDictionary nkdAllObjects;

	require(IsCompiled());

	// Calcul de l'Union
	oaResult.SetSize(0);

	// Collecte des objets participants a l'union
	for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
	{
		kwoObjectOperand = GetOperandAt(nOperand)->GetObjectValue(kwoObject);

		// Ajout si non null
		if (kwoObjectOperand != NULL)
		{
			// Prise en compte sauf si doublon
			if (nkdAllObjects.Lookup((NUMERIC)kwoObjectOperand) == NULL)
			{
				// Memorisation dans le dictionnaire assurant l'unicite des elements
				nkdAllObjects.SetAt((NUMERIC)kwoObjectOperand, kwoObjectOperand);

				// Ajout dans le tableau resultat
				oaResult.Add(kwoObjectOperand);
			}
		}
	}
	return &oaResult;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableExtraction

KWDRTableExtraction::KWDRTableExtraction()
{
	SetName("TableExtraction");
	SetLabel("Extraction of a sub-table from a table from a range of ranks");
	SetType(KWType::ObjectArray);
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::Continuous);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KWDRTableExtraction::~KWDRTableExtraction() {}

KWDerivationRule* KWDRTableExtraction::Create() const
{
	return new KWDRTableExtraction;
}

ObjectArray* KWDRTableExtraction::ComputeObjectArrayResult(const KWObject* kwoObject) const
{
	ObjectArray* oaContainedObjects;
	Continuous cBeginIndex;
	Continuous cEndIndex;
	int nBeginIndex;
	int nEndIndex;
	int nObject;

	require(IsCompiled());

	// Recherche de la valeur ObjectArray
	oaResult.SetSize(0);
	oaContainedObjects = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaContainedObjects != NULL)
	{
		assert(CheckObjectArray(oaContainedObjects));

		// Recherche de la plage de rang a extraire
		cBeginIndex = GetOperandAt(1)->GetContinuousValue(kwoObject);
		cEndIndex = GetOperandAt(2)->GetContinuousValue(kwoObject);

		// On continu si les rang ne sont pas missing
		if (cBeginIndex != KWContinuous::GetMissingValue() and cEndIndex != KWContinuous::GetMissingValue())
		{
			// Extraction des rangs
			nBeginIndex = (int)floor(cBeginIndex + 0.5);
			nEndIndex = (int)floor(cEndIndex + 0.5);

			// Recadrage sur une plage compatible avec la taille du container
			nBeginIndex--;
			if (nBeginIndex < 0)
				nBeginIndex = 0;
			nEndIndex--;
			if (nEndIndex >= oaContainedObjects->GetSize())
				nEndIndex = oaContainedObjects->GetSize() - 1;

			// Extraction des objets concernes
			if (nBeginIndex <= nEndIndex)
			{
				oaResult.SetSize(nEndIndex - nBeginIndex + 1);
				for (nObject = 0; nObject < oaResult.GetSize(); nObject++)
					oaResult.SetAt(nObject, oaContainedObjects->GetAt(nBeginIndex + nObject));
			}
		}
	}

	ensure(CheckObjectArray(&oaResult));
	return &oaResult;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableSelection

KWDRTableSelection::KWDRTableSelection()
{
	SetName("TableSelection");
	SetLabel("Selection from a table for a given selection criterion");
	SetType(KWType::ObjectArray);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRTableSelection::~KWDRTableSelection() {}

KWDerivationRule* KWDRTableSelection::Create() const
{
	return new KWDRTableSelection;
}

ObjectArray* KWDRTableSelection::ComputeObjectArrayResult(const KWObject* kwoObject) const
{
	ObjectArray* oaObjectArrayOperand;
	KWDerivationRuleOperand* selectionOperand;
	KWObject* kwoContainedObject;
	int nObject;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Extraction des Object verifiant la selection
	oaResult.SetSize(0);
	oaObjectArrayOperand = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0)
	{
		assert(CheckObjectArray(oaObjectArrayOperand));

		// Acces a l'operande de selection
		selectionOperand = GetSecondOperand();

		// Extraction des objets du container
		for (nObject = 0; nObject < oaObjectArrayOperand->GetSize(); nObject++)
		{
			kwoContainedObject = cast(KWObject*, oaObjectArrayOperand->GetAt(nObject));

			// Utilisation ou non de l'operande de selection
			if (kwoContainedObject != NULL and
			    selectionOperand->GetContinuousValue(kwoContainedObject) != 0)
				oaResult.Add(kwoContainedObject);
		}
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	ensure(CheckObjectArray(&oaResult));
	return &oaResult;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableSort

KWDRTableSort::KWDRTableSort()
{
	SetName("TableSort");
	SetLabel("Sort of a table for given sort variables");
	SetType(KWType::ObjectArray);
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	SetVariableOperandNumber(true);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRTableSort::~KWDRTableSort() {}

KWDerivationRule* KWDRTableSort::Create() const
{
	return new KWDRTableSort;
}

// Variable statique globale pour parametrer la methode de comparaison des objet d'une table de KWObject
// Les operande de tri (KWKWDerivationRuleOperand) sont a partir de l'index 1
static const ObjectArray* KWDRTableSortRuleOperands = NULL;

// methode de comparaison de deux objets KWObject, selon les operandes de tri KWDRTableSortRuleOperands
int KWDRTableSortCompareObjects(const void* elem1, const void* elem2)
{
	KWObject* object1;
	KWObject* object2;
	int i;
	KWDerivationRuleOperand* sortOperand;
	int nDiff;

	require(elem1 != NULL);
	require(elem2 != NULL);
	require(KWDRTableSortRuleOperands != NULL);
	require(KWDRTableSortRuleOperands->GetSize() > 1);

	// Acces aux objets a comparer
	object1 = cast(KWObject*, *(Object**)elem1);
	object2 = cast(KWObject*, *(Object**)elem2);
	check(object1);
	check(object2);

	// Comparaison hierarchique operande par operande
	nDiff = 0;
	for (i = 1; i < KWDRTableSortRuleOperands->GetSize(); i++)
	{
		sortOperand = cast(KWDerivationRuleOperand*, KWDRTableSortRuleOperands->GetAt(i));
		assert(KWType::IsStored(sortOperand->GetType()));

		// Tri selon le type
		switch (sortOperand->GetType())
		{
		case KWType::Symbol:
			nDiff = sortOperand->GetSymbolValue(object1).CompareValue(sortOperand->GetSymbolValue(object2));
			break;
		case KWType::Continuous:
			nDiff = KWContinuous::Compare(sortOperand->GetContinuousValue(object1),
						      sortOperand->GetContinuousValue(object2));
			break;
		case KWType::Date:
			nDiff = sortOperand->GetDateValue(object1).Compare(sortOperand->GetDateValue(object2));
			break;
		case KWType::Time:
			nDiff = sortOperand->GetTimeValue(object1).Compare(sortOperand->GetTimeValue(object2));
			break;
		case KWType::Timestamp:
			nDiff =
			    sortOperand->GetTimestampValue(object1).Compare(sortOperand->GetTimestampValue(object2));
			break;
		case KWType::TimestampTZ:
			nDiff = sortOperand->GetTimestampTZValue(object1).Compare(
			    sortOperand->GetTimestampTZValue(object2));
			break;
		default:
			nDiff = 0;
			assert(false);
			break;
		}

		// Arret si le tri est decide
		if (nDiff != 0)
			break;
	}
	return nDiff;
}

ObjectArray* KWDRTableSort::ComputeObjectArrayResult(const KWObject* kwoObject) const
{
	ObjectArray* oaObjectArrayOperand;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Extraction des Object verifiant la selection
	oaResult.SetSize(0);
	oaObjectArrayOperand = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0)
	{
		assert(CheckObjectArray(oaObjectArrayOperand));

		// Recopie prealable des objet du container
		oaResult.CopyFrom(oaObjectArrayOperand);

		// Tri des objets, en parametrant temporairement la fonction de tri par les operandes de tri
		assert(KWDRTableSortRuleOperands == NULL);
		KWDRTableSortRuleOperands = &oaOperands;
		oaResult.SetCompareFunction(KWDRTableSortCompareObjects);
		oaResult.Sort();
		KWDRTableSortRuleOperands = NULL;
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	ensure(CheckObjectArray(&oaResult));
	return &oaResult;
}

boolean KWDRTableSort::CheckOperandsFamily(const KWDerivationRule* ruleFamily) const
{
	boolean bOk = true;
	KWDerivationRuleOperand* operand;
	int i;
	ALString sTmp;

	require(ruleFamily != NULL);
	require(ruleFamily->CheckDefinition());
	require(ruleFamily->GetOperandNumber() > 0);
	require(ruleFamily->GetFirstOperand()->GetType() == KWType::ObjectArray);

	// Il faut au moins un operande
	if (GetOperandNumber() == 0)
	{
		AddError("Missing operand");
		bOk = false;
	}
	// Verification du premier operande: ObjectArray
	else
	{
		operand = GetOperandAt(0);
		if (not operand->CheckFamily(ruleFamily->GetOperandAt(0)))
		{
			AddError(sTmp + "Type of first operand" + " inconsistent with that of the registered rule (" +
				 KWType::ToString(ruleFamily->GetOperandAt(0)->GetType()) + ")");
			bOk = false;
		}
	}

	// Le premier operande est deja verifie en precondition de la methode
	// Les autres operandes doivent etre de type simple
	if (bOk)
	{
		for (i = 1; i < GetOperandNumber(); i++)
		{
			operand = GetOperandAt(i);

			// Test de type stocke (simple ou complexe)
			if (not KWType::IsStored(operand->GetType()))
			{
				AddError(sTmp + "Type (" + KWType::ToString(operand->GetType()) + ") of operand " +
					 IntToString(i + 1) +
					 " inconsistent with that of the registered rule (sort value)");
				bOk = false;
				break;
			}
		}
	}
	return bOk;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableStats

KWDRTableStats::KWDRTableStats()
{
	SetMultipleScope(true);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::ObjectArray);
}

KWDRTableStats::~KWDRTableStats() {}

Continuous KWDRTableStats::ComputeContinuousStatsFromContinuousVector(int nRecordNumber, Continuous cDefaultValue,
								      const ContinuousVector* cvValues) const
{
	assert(false);
	return KWContinuous::GetMissingValue();
}

Continuous KWDRTableStats::ComputeContinuousStatsFromSymbolVector(int nRecordNumber, Symbol sDefaultValue,
								  const SymbolVector* svValues) const
{
	assert(false);
	return KWContinuous::GetMissingValue();
}

Symbol KWDRTableStats::ComputeSymbolStatsFromContinuousVector(int nRecordNumber, Continuous cDefaultValue,
							      const ContinuousVector* cvValues) const
{
	assert(false);
	return Symbol();
}

Symbol KWDRTableStats::ComputeSymbolStatsFromSymbolVector(int nRecordNumber, Symbol sDefaultValue,
							  const SymbolVector* svValues) const
{
	assert(false);
	return Symbol();
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableStatsContinuous

KWDRTableStatsContinuous::KWDRTableStatsContinuous()
{
	SetType(KWType::Continuous);
}

KWDRTableStatsContinuous::~KWDRTableStatsContinuous() {}

Continuous KWDRTableStatsContinuous::ComputeContinuousResult(const KWObject* kwoObject) const
{
	Continuous cResult;
	ObjectArray* oaObjects;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Calcul du resultat avec le tableau du premier operande et
	// la valeur accessible avec le deuxieme operande
	oaObjects = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjects == NULL or oaObjects->GetSize() == 0)
		cResult = GetDefaultContinuousStats();
	else
		cResult = ComputeContinuousStats(oaObjects);

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return cResult;
}

Continuous KWDRTableStatsContinuous::GetDefaultContinuousStats() const
{
	return KWContinuous::GetMissingValue();
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableStatsSymbol

KWDRTableStatsSymbol::KWDRTableStatsSymbol()
{
	SetType(KWType::Symbol);
}

KWDRTableStatsSymbol::~KWDRTableStatsSymbol() {}

Symbol KWDRTableStatsSymbol::ComputeSymbolResult(const KWObject* kwoObject) const
{
	Symbol sResult;
	ObjectArray* oaObjects;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Calcul du resultat avec le tableau du premier operande et
	// la valeur accessible avec le deuxieme operande
	oaObjects = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjects != NULL and oaObjects->GetSize() > 0)
		sResult = ComputeSymbolStats(oaObjects);

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return sResult;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableCount

KWDRTableCount::KWDRTableCount()
{
	SetName("TableCount");
	SetLabel("Number of instances in a table");
}

KWDRTableCount::~KWDRTableCount() {}

KWDerivationRule* KWDRTableCount::Create() const
{
	return new KWDRTableCount;
}

Continuous KWDRTableCount::ComputeContinuousResult(const KWObject* kwoObject) const
{
	ObjectArray* oaObjects;

	require(IsCompiled());

	// Calcul du nombre d'instances verifiant la selection
	oaObjects = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjects == NULL)
		return 0;
	else
		return (Continuous)oaObjects->GetSize();
}

Continuous KWDRTableCount::ComputeContinuousStats(const ObjectArray* oaObjects) const
{
	require(oaObjects != NULL);
	return (Continuous)oaObjects->GetSize();
}

Continuous KWDRTableCount::GetDefaultContinuousStats() const
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableCountDistinct

KWDRTableCountDistinct::KWDRTableCountDistinct()
{
	SetName("TableCountDistinct");
	SetLabel("Number of distinct values in a table");
	AddOperand(new KWDerivationRuleOperand);
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRTableCountDistinct::~KWDRTableCountDistinct() {}

KWDerivationRule* KWDRTableCountDistinct::Create() const
{
	return new KWDRTableCountDistinct;
}

Continuous KWDRTableCountDistinct::ComputeContinuousStats(const ObjectArray* oaObjects) const
{
	KWDerivationRuleOperand* valueOperand;
	int nObject;
	KWObject* kwoContainedObject;
	Symbol sValue;
	NumericKeyDictionary nkdDistinctValues;
	SymbolVector svDistinctValues;
	Continuous cResult;

	require(oaObjects != NULL);

	// Calcul du nombre de valeurs distinctes parmi les sous-objets
	valueOperand = GetSecondOperand();
	for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
	{
		kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));
		sValue = valueOperand->GetSymbolValue(kwoContainedObject);

		// Ajout dans le dictionnaire a cle numerique
		nkdDistinctValues.SetAt(sValue.GetNumericKey(), kwoContainedObject);

		// Attention, on doit memoriser les symbol du dictionnaire a cle numerique, car ceux-ci peuvent etre
		// temporaires s'il sont issus de calcul par regle de derivation, et pourraient ainsi etre dessalloues,
		// donnant lieu a a la reutilisation d'un meme bloc memoire (NumericKey) pour un nouveau symbole On
		// prefere cette solution a l'utilisation d'un ObjectDictionary, moains efficace en temps et memoire
		if (svDistinctValues.GetSize() < nkdDistinctValues.GetCount())
			svDistinctValues.Add(sValue);
		assert(svDistinctValues.GetSize() == nkdDistinctValues.GetCount());
	}
	cResult = (Continuous)nkdDistinctValues.GetCount();
	return cResult;
}

Continuous KWDRTableCountDistinct::ComputeContinuousStatsFromSymbolVector(int nRecordNumber, Symbol sDefaultValue,
									  const SymbolVector* svValues) const
{
	Continuous cResult;
	int nValue;
	Symbol sValue;
	NumericKeyDictionary nkdDistinctValues;

	require(nRecordNumber >= 0);
	require(svValues != NULL);

	// Initialisation avec la valeur par defaut si necessaire
	if (svValues->GetSize() < nRecordNumber)
		nkdDistinctValues.SetAt(sDefaultValue.GetNumericKey(), &nkdDistinctValues);

	// Parcours des valeurs du vecteur
	for (nValue = 0; nValue < svValues->GetSize(); nValue++)
	{
		sValue = svValues->GetAt(nValue);

		// Ajout dans le dictionnaire a cle numerique
		// (ici, les valeur sont deja memorisees dans un vecteur)
		nkdDistinctValues.SetAt(sValue.GetNumericKey(), &nkdDistinctValues);
	}
	cResult = (Continuous)nkdDistinctValues.GetCount();
	return cResult;
}

Continuous KWDRTableCountDistinct::GetDefaultContinuousStats() const
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableEntropy

KWDRTableEntropy::KWDRTableEntropy()
{
	SetName("TableEntropy");
	SetLabel("Entropy of the values values in a table");
	AddOperand(new KWDerivationRuleOperand);
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRTableEntropy::~KWDRTableEntropy() {}

KWDerivationRule* KWDRTableEntropy::Create() const
{
	return new KWDRTableEntropy;
}

Continuous KWDRTableEntropy::ComputeContinuousStats(const ObjectArray* oaObjects) const
{
	KWDerivationRuleOperand* valueOperand;
	int nObject;
	KWObject* kwoContainedObject;
	Symbol sValue;
	NumericKeyDictionary nkdDistinctValues;
	KWSortableSymbol* symbolCount;
	POSITION position;
	NUMERIC rKey;
	Object* oElement;
	double dProb;
	Continuous cResult;

	require(oaObjects != NULL);

	// Recherche de toutes les valeurs distinctes et comptage de leur effectif
	// On utilise des objets KWSortableSymbol, avec le champ Index pour l'effectif et
	// le champ SortValue pour la valeur du symbol
	// Attention, on doit en effet memoriser les symbol du dictionnaire a cle numerique, car ceux-ci peuvent etre
	// temporaires s'il sont issus de calcul par regle de derivation, et pourraient ainsi etre dessalloues, donnant
	// lieu a a la reutilisation d'un meme bloc memoire (NumericKey) pour un nouveau symbole On prefere cette
	// solution a l'utilisation d'un ObjectDictionary, moains efficace en temps et memoire
	valueOperand = GetSecondOperand();
	for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
	{
		kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));
		sValue = valueOperand->GetSymbolValue(kwoContainedObject);

		// Creation si necessaire d'un nouvel objet de comptage dans le dictionnaire a cle numerique
		symbolCount = cast(KWSortableSymbol*, nkdDistinctValues.Lookup(sValue.GetNumericKey()));
		if (symbolCount == NULL)
		{
			symbolCount = new KWSortableSymbol;
			symbolCount->SetSortValue(sValue);
			nkdDistinctValues.SetAt(sValue.GetNumericKey(), symbolCount);
		}

		// Mise a jour de l'effectif
		symbolCount->SetIndex(symbolCount->GetIndex() + 1);
	}

	// Calcul de l'entropie
	cResult = 0;
	position = nkdDistinctValues.GetStartPosition();
	while (position != NULL)
	{
		nkdDistinctValues.GetNextAssoc(position, rKey, oElement);
		symbolCount = cast(KWSortableSymbol*, oElement);

		// Calcul de la probabilite de la valeur
		dProb = symbolCount->GetIndex() * 1.0 / oaObjects->GetSize();

		// Mise a jour de l'entropie
		cResult -= dProb * log(dProb);
	}

	// Nettoyage
	nkdDistinctValues.DeleteAll();
	return cResult;
}

Continuous KWDRTableEntropy::ComputeContinuousStatsFromSymbolVector(int nRecordNumber, Symbol sDefaultValue,
								    const SymbolVector* svValues) const
{
	Continuous cResult;
	int nValue;
	Symbol sValue;
	NumericKeyDictionary nkdDistinctValues;
	KWSortableSymbol* symbolCount;
	POSITION position;
	NUMERIC rKey;
	Object* oElement;
	double dProb;
	int nDefaultValueNumber;

	require(nRecordNumber >= 0);
	require(svValues != NULL);

	// Prise en compte de la valeur par defaut si necessaire
	nDefaultValueNumber = nRecordNumber - svValues->GetSize();
	if (nDefaultValueNumber > 0)
	{
		symbolCount = new KWSortableSymbol;
		symbolCount->SetSortValue(sDefaultValue);
		nkdDistinctValues.SetAt(sDefaultValue.GetNumericKey(), symbolCount);
		symbolCount->SetIndex(nDefaultValueNumber);
	}

	// Parcours des valeurs du vecteur
	for (nValue = 0; nValue < svValues->GetSize(); nValue++)
	{
		sValue = svValues->GetAt(nValue);

		// Creation si necessaire d'un nouvel objet de comptage dans le dictionnaire a cle numerique
		symbolCount = cast(KWSortableSymbol*, nkdDistinctValues.Lookup(sValue.GetNumericKey()));
		if (symbolCount == NULL)
		{
			symbolCount = new KWSortableSymbol;
			symbolCount->SetSortValue(sValue);
			nkdDistinctValues.SetAt(sValue.GetNumericKey(), symbolCount);
		}

		// Mise a jour de l'effectif
		symbolCount->SetIndex(symbolCount->GetIndex() + 1);
	}

	// Calcul de l'entropie
	cResult = 0;
	position = nkdDistinctValues.GetStartPosition();
	while (position != NULL)
	{
		nkdDistinctValues.GetNextAssoc(position, rKey, oElement);
		symbolCount = cast(KWSortableSymbol*, oElement);

		// Calcul de la probabilite de la valeur
		dProb = symbolCount->GetIndex() * 1.0 / svValues->GetSize();

		// Mise a jour de l'entropie
		cResult -= dProb * log(dProb);
	}

	// Nettoyage
	nkdDistinctValues.DeleteAll();
	return cResult;
}

Continuous KWDRTableEntropy::GetDefaultContinuousStats() const
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableMode

KWDRTableMode::KWDRTableMode()
{
	SetName("TableMode");
	SetLabel("Most frequent value in a table");
	AddOperand(new KWDerivationRuleOperand);
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRTableMode::~KWDRTableMode() {}

KWDerivationRule* KWDRTableMode::Create() const
{
	return new KWDRTableMode;
}

Symbol KWDRTableMode::ComputeSymbolStats(const ObjectArray* oaObjects) const
{
	KWDerivationRuleOperand* valueOperand;
	int nObject;
	KWObject* kwoContainedObject;
	Symbol sValue;
	NumericKeyDictionary nkdValues;
	KWSortableSymbol* kwssValueFrequency;
	Symbol sMode;
	int nModeFrequency;

	require(oaObjects != NULL);

	// Recherche du mode parmi les sous-objets verifiant la selection
	nModeFrequency = 0;
	valueOperand = GetSecondOperand();
	for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
	{
		kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));

		// Calcul des effectifs par valeur, et recherche du mode
		sValue = valueOperand->GetSymbolValue(kwoContainedObject);

		// Memorisation de l'effectif de la valeur
		// On memorise la cle (Symbol) pour que le symbol ne soit pas detruit
		kwssValueFrequency = cast(KWSortableSymbol*, nkdValues.Lookup(sValue.GetNumericKey()));
		if (kwssValueFrequency == NULL)
		{
			kwssValueFrequency = new KWSortableSymbol;
			kwssValueFrequency->SetSortValue(sValue);
			nkdValues.SetAt(sValue.GetNumericKey(), kwssValueFrequency);
		}
		kwssValueFrequency->SetIndex(kwssValueFrequency->GetIndex() + 1);

		// Test si on obtient un nouveau mode
		if (kwssValueFrequency->GetIndex() > nModeFrequency)
		{
			nModeFrequency = kwssValueFrequency->GetIndex();
			sMode = sValue;
		}
		// En cas d'egalite, on prendre la valeur la plus petite selon l'ordre lexicographique
		else if (kwssValueFrequency->GetIndex() == nModeFrequency and sValue.CompareValue(sMode) < 0)
			sMode = sValue;
	}

	// Nettoyage
	nkdValues.DeleteAll();
	return sMode;
}

Symbol KWDRTableMode::ComputeSymbolStatsFromSymbolVector(int nRecordNumber, Symbol sDefaultValue,
							 const SymbolVector* svValues) const
{
	int nValue;
	Symbol sValue;
	NumericKeyDictionary nkdValues;
	KWSortableSymbol* kwssValueFrequency;
	Symbol sMode;
	int nModeFrequency;
	int nDefaultValueNumber;

	require(nRecordNumber >= 0);
	require(svValues != NULL);

	// On comptabilise d'abord le nombre de valeur manquantes
	nDefaultValueNumber = nRecordNumber - svValues->GetSize();
	nModeFrequency = nDefaultValueNumber;
	sMode = sDefaultValue;

	// Parcours des valeurs du vecteur, uniquement s'il y a au moins autant de valeur presentes
	// que de valeurs manquantes
	if (nDefaultValueNumber <= nRecordNumber / 2)
	{
		// Initialisation de la valeur par defaut avec son effectif
		kwssValueFrequency = new KWSortableSymbol;
		kwssValueFrequency->SetSortValue(sDefaultValue);
		kwssValueFrequency->SetIndex(nDefaultValueNumber);
		nkdValues.SetAt(sValue.GetNumericKey(), kwssValueFrequency);

		// Parcours de valeurs presentes
		for (nValue = 0; nValue < svValues->GetSize(); nValue++)
		{
			sValue = svValues->GetAt(nValue);

			// Memorisation de l'effectif de la valeur
			// On memorise la cle (Symbol) pour que le symbol ne soit pas detruit
			kwssValueFrequency = cast(KWSortableSymbol*, nkdValues.Lookup(sValue.GetNumericKey()));
			if (kwssValueFrequency == NULL)
			{
				kwssValueFrequency = new KWSortableSymbol;
				kwssValueFrequency->SetSortValue(sValue);
				nkdValues.SetAt(sValue.GetNumericKey(), kwssValueFrequency);
			}
			kwssValueFrequency->SetIndex(kwssValueFrequency->GetIndex() + 1);

			// Test si on obtient un nouveau mode
			if (kwssValueFrequency->GetIndex() > nModeFrequency)
			{
				nModeFrequency = kwssValueFrequency->GetIndex();
				sMode = sValue;
			}
			// En cas d'egalite, on prendre la valeur la plus petite selon l'ordre lexicographique
			else if (kwssValueFrequency->GetIndex() == nModeFrequency and sValue.CompareValue(sMode) < 0)
				sMode = sValue;
		}

		// Nettoyage
		nkdValues.DeleteAll();
	}
	return sMode;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableModeAt

// Comparaison de deux objets KWSortableValue par effectif decroissant puis par valeur
int KWSortableSymbolCompareIndexValue(const void* elem1, const void* elem2);

int KWSortableSymbolCompareIndexValue(const void* elem1, const void* elem2)
{
	int nResult;

	// Comparaison sur le critere de tri, du plus petitau plus grand
	nResult = -cast(KWSortableSymbol*, *(Object**)elem1)->GetIndex() +
		  cast(KWSortableSymbol*, *(Object**)elem2)->GetIndex();
	if (nResult != 0)
		return nResult;
	// En cas d'egalite, on se rabat sur une comparaison lexicographique
	else
		return cast(KWSortableSymbol*, *(Object**)elem1)
		    ->GetSortValue()
		    .CompareValue(cast(KWSortableSymbol*, *(Object**)elem2)->GetSortValue());
}

KWDRTableModeAt::KWDRTableModeAt()
{
	SetName("TableModeAt");
	SetLabel("Ith most frequent value in a table");
	AddOperand(new KWDerivationRuleOperand);
	GetSecondOperand()->SetType(KWType::Symbol);
	AddOperand(new KWDerivationRuleOperand);
	GetOperandAt(2)->SetType(KWType::Continuous);
	GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
}

KWDRTableModeAt::~KWDRTableModeAt() {}

KWDerivationRule* KWDRTableModeAt::Create() const
{
	return new KWDRTableModeAt;
}

Symbol KWDRTableModeAt::ComputeSymbolStats(const ObjectArray* oaObjects) const
{
	KWDerivationRuleOperand* valueOperand;
	int nObject;
	KWObject* kwoContainedObject;
	Symbol sValue;
	NumericKeyDictionary nkdValues;
	ObjectArray oaValues;
	KWSortableSymbol* kwssValueFrequency;
	int nIndex;
	Symbol sIthMode;

	require(oaObjects != NULL);

	// Index du mode a rechercher
	assert(GetOperandAt(2)->GetOrigin() == KWDerivationRuleOperand::OriginConstant);
	nIndex = (int)floor(GetOperandAt(2)->GetContinuousConstant() + 0.5);
	if (nIndex < 1)
		nIndex = 1;

	// Parcours des objets du container
	valueOperand = GetSecondOperand();
	for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
	{
		kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));
		sValue = valueOperand->GetSymbolValue(kwoContainedObject);

		// Memorisation de l'effectif de la valeur
		kwssValueFrequency = cast(KWSortableSymbol*, nkdValues.Lookup(sValue.GetNumericKey()));
		if (kwssValueFrequency == NULL)
		{
			kwssValueFrequency = new KWSortableSymbol;
			kwssValueFrequency->SetSortValue(sValue);
			nkdValues.SetAt(sValue.GetNumericKey(), kwssValueFrequency);
		}
		kwssValueFrequency->SetIndex(kwssValueFrequency->GetIndex() + 1);
	}

	// Recherche du ieme mode
	if (nIndex > nkdValues.GetCount())
		sIthMode = Symbol();
	else
	{
		// Stockage des couples (valeur, effectif) dans un tableau et tri par effectif
		nkdValues.ExportObjectArray(&oaValues);
		oaValues.SetCompareFunction(KWSortableSymbolCompareIndexValue);
		oaValues.Sort();

		// Recherche de la ieme valeur la plus frequente
		kwssValueFrequency = cast(KWSortableSymbol*, oaValues.GetAt(nIndex - 1));
		sIthMode = kwssValueFrequency->GetSortValue();
	}

	// Nettoyage
	nkdValues.DeleteAll();
	return sIthMode;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableMean

KWDRTableMean::KWDRTableMean()
{
	SetName("TableMean");
	SetLabel("Mean of values in a table");
	AddOperand(new KWDerivationRuleOperand);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRTableMean::~KWDRTableMean() {}

KWDerivationRule* KWDRTableMean::Create() const
{
	return new KWDRTableMean;
}

Continuous KWDRTableMean::ComputeContinuousStats(const ObjectArray* oaObjects) const
{
	KWDerivationRuleOperand* valueOperand;
	int nObject;
	KWObject* kwoContainedObject;
	Continuous cValue;
	int nObjectNumber;
	double dMean;
	Continuous cMean;

	require(oaObjects != NULL);

	// Calcul de la moyenne parmi les objets ayant une valeur non manquante
	dMean = 0;
	nObjectNumber = 0;
	valueOperand = GetSecondOperand();
	for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
	{
		kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));
		cValue = valueOperand->GetContinuousValue(kwoContainedObject);
		if (cValue != KWContinuous::GetMissingValue())
		{
			dMean += cValue;
			nObjectNumber++;
		}
	}
	if (nObjectNumber > 0)
		cMean = (Continuous)(dMean / nObjectNumber);
	else
		cMean = KWContinuous::GetMissingValue();
	return cMean;
}

Continuous KWDRTableMean::ComputeContinuousStatsFromContinuousVector(int nRecordNumber, Continuous cDefaultValue,
								     const ContinuousVector* cvValues) const
{
	int nValue;
	Continuous cValue;
	int nValueNumber;
	double dMean;
	Continuous cMean;
	int nDefaultValueNumber;

	require(nRecordNumber >= 0);
	require(cvValues != NULL);

	// Prise en compte de la valeur par defaut si elle n'est pas missing
	dMean = 0;
	nValueNumber = 0;
	if (cDefaultValue != KWContinuous::GetMissingValue())
	{
		nDefaultValueNumber = nRecordNumber - cvValues->GetSize();
		dMean = nDefaultValueNumber * cDefaultValue;
		nValueNumber = nDefaultValueNumber;
	}

	// Calcul de la moyenne parmi les valeurs non manquantes
	for (nValue = 0; nValue < cvValues->GetSize(); nValue++)
	{
		cValue = cvValues->GetAt(nValue);
		if (cValue != KWContinuous::GetMissingValue())
		{
			dMean += cValue;
			nValueNumber++;
		}
	}
	if (nValueNumber > 0)
		cMean = (Continuous)(dMean / nValueNumber);
	else
		cMean = KWContinuous::GetMissingValue();
	return cMean;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableStandardDeviation

KWDRTableStandardDeviation::KWDRTableStandardDeviation()
{
	SetName("TableStdDev");
	SetLabel("Standard deviation of values in a table");
	AddOperand(new KWDerivationRuleOperand);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRTableStandardDeviation::~KWDRTableStandardDeviation() {}

KWDerivationRule* KWDRTableStandardDeviation::Create() const
{
	return new KWDRTableStandardDeviation;
}

Continuous KWDRTableStandardDeviation::ComputeContinuousStats(const ObjectArray* oaObjects) const
{
	KWDerivationRuleOperand* valueOperand;
	int nObject;
	KWObject* kwoContainedObject;
	Continuous cValue;
	double dValue;
	int nObjectNumber;
	double dSum;
	double dSquareSum;
	double dStandardDeviation;
	Continuous cStandardDeviation;

	require(oaObjects != NULL);

	// Calcul de la moyenne parmi les sous-objets avec valeur non manquante
	nObjectNumber = 0;
	dSum = 0;
	dSquareSum = 0;
	valueOperand = GetSecondOperand();
	for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
	{
		kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));
		cValue = valueOperand->GetContinuousValue(kwoContainedObject);
		if (cValue != KWContinuous::GetMissingValue())
		{
			// Mise a jour des sommes
			dValue = cValue;
			dSum += dValue;
			dSquareSum += dValue * dValue;
			nObjectNumber++;
		}
	}
	if (nObjectNumber > 0)
	{
		// Calcul de l'ecart type
		dStandardDeviation = sqrt(fabs((dSquareSum - dSum * dSum / nObjectNumber) / nObjectNumber));
		assert(dStandardDeviation >= 0);
		cStandardDeviation = (Continuous)dStandardDeviation;
	}
	else
		cStandardDeviation = KWContinuous::GetMissingValue();
	return cStandardDeviation;
}

Continuous
KWDRTableStandardDeviation::ComputeContinuousStatsFromContinuousVector(int nRecordNumber, Continuous cDefaultValue,
								       const ContinuousVector* cvValues) const
{
	int nValue;
	Continuous cValue;
	double dValue;
	int nValueNumber;
	double dSum;
	double dSquareSum;
	double dStandardDeviation;
	Continuous cStandardDeviation;
	int nDefaultValueNumber;

	require(nRecordNumber >= 0);
	require(cvValues != NULL);

	// Prise en compte de la valeur par defaut si elle n'est pas missing
	dSum = 0;
	dSquareSum = 0;
	nValueNumber = 0;
	if (cDefaultValue != KWContinuous::GetMissingValue())
	{
		nDefaultValueNumber = nRecordNumber - cvValues->GetSize();
		dSum = nDefaultValueNumber * cDefaultValue;
		dSquareSum = nDefaultValueNumber * cDefaultValue * cDefaultValue;
		nValueNumber = nDefaultValueNumber;
	}

	// Calcul de la moyenne parmi les valeurs non manquantes
	for (nValue = 0; nValue < cvValues->GetSize(); nValue++)
	{
		cValue = cvValues->GetAt(nValue);
		if (cValue != KWContinuous::GetMissingValue())
		{
			// Mise a jour des sommes
			dValue = cValue;
			dSum += dValue;
			dSquareSum += dValue * dValue;
			nValueNumber++;
		}
	}
	if (nValueNumber > 0)
	{
		// Calcul de l'ecart type
		dStandardDeviation = sqrt(fabs((dSquareSum - dSum * dSum / nValueNumber) / nValueNumber));
		assert(dStandardDeviation >= 0);
		cStandardDeviation = (Continuous)dStandardDeviation;
	}
	else
		cStandardDeviation = KWContinuous::GetMissingValue();
	return cStandardDeviation;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableMedian

KWDRTableMedian::KWDRTableMedian()
{
	SetName("TableMedian");
	SetLabel("Median of values in a table");
	AddOperand(new KWDerivationRuleOperand);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRTableMedian::~KWDRTableMedian() {}

KWDerivationRule* KWDRTableMedian::Create() const
{
	return new KWDRTableMedian;
}

Continuous KWDRTableMedian::ComputeContinuousStats(const ObjectArray* oaObjects) const
{
	KWDerivationRuleOperand* valueOperand;
	int nObject;
	KWObject* kwoContainedObject;
	Continuous cValue;
	ContinuousVector cvValues;
	Continuous cMedian;

	require(oaObjects != NULL);

	// Colllecte des valeurs non manquantes
	valueOperand = GetSecondOperand();
	for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
	{
		kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));
		cValue = valueOperand->GetContinuousValue(kwoContainedObject);
		if (cValue != KWContinuous::GetMissingValue())
			cvValues.Add(cValue);
	}

	// Calcul de la valeur mediane
	if (cvValues.GetSize() == 0)
		cMedian = KWContinuous::GetMissingValue();
	else if (cvValues.GetSize() == 1)
		cMedian = cvValues.GetAt(0);
	// Cas ou il y a au moins deux valeurs
	else
	{
		// Tri des valeurs
		cvValues.Sort();

		// Calcul de la valeur mediane, selon la parite de la taille du tableau de valeurs
		if (cvValues.GetSize() % 2 == 0)
			cMedian =
			    (cvValues.GetAt(cvValues.GetSize() / 2 - 1) + cvValues.GetAt(cvValues.GetSize() / 2)) / 2;
		else
			cMedian = cvValues.GetAt(cvValues.GetSize() / 2);
	}
	return cMedian;
}

Continuous KWDRTableMedian::ComputeContinuousStatsFromContinuousVector(int nRecordNumber, Continuous cDefaultValue,
								       const ContinuousVector* cvValues) const
{
	int nValue;
	Continuous cValue;
	ContinuousVector cvCollectedValues;
	Continuous cMedian;
	int nDefaultValueNumber;
	int nTotalValueNumber;
	int nMedianValueIndex;

	require(nRecordNumber >= 0);
	require(cvValues != NULL);

	// Cas particulier ou la valeur par defaut n'est pas manquante et correspond a plus de la moitie des valeurs
	if (cDefaultValue != KWContinuous::GetMissingValue() and cvValues->GetSize() < nRecordNumber / 2)
	{
		cMedian = cDefaultValue;
	}
	// Cas plus general
	else
	{
		// Collecte des valeurs non manquantes
		for (nValue = 0; nValue < cvValues->GetSize(); nValue++)
		{
			cValue = cvValues->GetAt(nValue);
			if (cValue != KWContinuous::GetMissingValue())
				cvCollectedValues.Add(cValue);
		}

		// Cas standard, ou la valeur par defaut est la valeur manquente
		if (cDefaultValue == KWContinuous::GetMissingValue() or nRecordNumber == cvValues->GetSize())
		{
			// Calcul de la valeur mediane
			if (cvCollectedValues.GetSize() == 0)
				cMedian = KWContinuous::GetMissingValue();
			else if (cvCollectedValues.GetSize() == 1)
				cMedian = cvCollectedValues.GetAt(0);
			// Cas ou il y a au moins deux valeurs
			else
			{
				// Tri des valeurs
				cvCollectedValues.Sort();

				// Calcul de la valeur mediane, selon la parite de la taille du tableau de valeurs
				nMedianValueIndex = cvCollectedValues.GetSize() / 2;
				if (cvCollectedValues.GetSize() % 2 == 0)
					cMedian = (cvCollectedValues.GetAt(nMedianValueIndex - 1) +
						   cvCollectedValues.GetAt(nMedianValueIndex)) /
						  2;
				else
					cMedian = cvCollectedValues.GetAt(nMedianValueIndex);
			}
		}
		// Cas ou il faut prendre en compte la valeur par defaut, qui n'est pas manquante
		else
		{
			assert(cDefaultValue != KWContinuous::GetMissingValue());

			// Nombre de valeurs par defaut
			nDefaultValueNumber = nRecordNumber - cvValues->GetSize();
			assert(nDefaultValueNumber > 0);

			// Calcul de la valeur mediane
			if (cvCollectedValues.GetSize() == 0)
				cMedian = cDefaultValue;
			else if (cvCollectedValues.GetSize() == 1)
			{
				if (nDefaultValueNumber > 1)
					cMedian = cDefaultValue;
				else
					cMedian = (cDefaultValue + cvCollectedValues.GetAt(0)) / 2;
			}
			// Cas ou il y a au moins deux valeurs
			else
			{
				// Si le nombre de valeurs manquante est superieurs au nombre de valeurs presente,
				// la mediane est necessaire la valeur manquante
				if (nDefaultValueNumber > cvCollectedValues.GetSize())
					cMedian = cDefaultValue;
				// Sinon, si la valeur manquante est plus petite queye la premiere valeur presente
				else if (cDefaultValue <= cvCollectedValues.GetAt(0))
				{
					assert(nDefaultValueNumber <= cvCollectedValues.GetSize());

					// Si taille egale, on prend la mediane entre la valeur par defaut et la plus
					// petite valeur presente
					if (nDefaultValueNumber == cvCollectedValues.GetSize())
						cMedian = (cDefaultValue + cvCollectedValues.GetAt(0)) / 2;
					// Sinon, ce sera necessaire un valeur presente, au dela du nombre de valeur
					// manquantes
					else
					{
						assert(nDefaultValueNumber < cvCollectedValues.GetSize());
						nTotalValueNumber = nDefaultValueNumber + cvCollectedValues.GetSize();

						// Calcul de la valeur mediane, selon la parite de la taille du tableau
						// de valeurs
						nMedianValueIndex = nTotalValueNumber / 2 - nDefaultValueNumber;
						assert(nMedianValueIndex >= 0);
						if (nTotalValueNumber % 2 == 0)
							cMedian = (cvCollectedValues.GetAt(nMedianValueIndex - 1) +
								   cvCollectedValues.GetAt(nMedianValueIndex)) /
								  2;
						else
							cMedian = cvCollectedValues.GetAt(nMedianValueIndex);
					}
				}
				// Cas general
				{
					// On insere prealablement la valeur manquante dans les valeur presente, autant
					// de fois que necessaire Ce n'est pas efficace, mais c'est trop penible a coder
					// sinon
					for (nValue = 0; nValue < nDefaultValueNumber; nValue++)
						cvCollectedValues.Add(cDefaultValue);

					// Tri des valeurs
					cvCollectedValues.Sort();

					// Calcul de la valeur mediane, selon la parite de la taille du tableau de
					// valeurs
					nMedianValueIndex = cvCollectedValues.GetSize() / 2;
					if (cvCollectedValues.GetSize() % 2 == 0)
						cMedian = (cvCollectedValues.GetAt(nMedianValueIndex - 1) +
							   cvCollectedValues.GetAt(nMedianValueIndex)) /
							  2;
					else
						cMedian = cvCollectedValues.GetAt(nMedianValueIndex);
				}
			}
		}
	}
	return cMedian;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableMin

KWDRTableMin::KWDRTableMin()
{
	SetName("TableMin");
	SetLabel("Min of values in a table");
	AddOperand(new KWDerivationRuleOperand);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRTableMin::~KWDRTableMin() {}

KWDerivationRule* KWDRTableMin::Create() const
{
	return new KWDRTableMin;
}

Continuous KWDRTableMin::ComputeContinuousStats(const ObjectArray* oaObjects) const
{
	KWDerivationRuleOperand* valueOperand;
	int nObject;
	KWObject* kwoContainedObject;
	Continuous cValue;
	int nObjectNumber;
	Continuous cMin;

	require(oaObjects != NULL);

	// Calcul du min parmi les sous-objets par les valeurs non manquantes
	cMin = KWContinuous::GetMaxValue();
	nObjectNumber = 0;
	valueOperand = GetSecondOperand();
	for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
	{
		kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));
		cValue = valueOperand->GetContinuousValue(kwoContainedObject);
		if (cValue != KWContinuous::GetMissingValue())
		{
			if (cValue < cMin)
				cMin = cValue;
			nObjectNumber++;
		}
	}
	if (nObjectNumber == 0)
		cMin = KWContinuous::GetMissingValue();
	return cMin;
}

Continuous KWDRTableMin::ComputeContinuousStatsFromContinuousVector(int nRecordNumber, Continuous cDefaultValue,
								    const ContinuousVector* cvValues) const
{
	int nValue;
	Continuous cValue;
	int nValueNumber;
	Continuous cMin;

	require(nRecordNumber >= 0);
	require(cvValues != NULL);

	// Prise en compte de la valeur par defaut si elle n'est pas manquante
	cMin = KWContinuous::GetMaxValue();
	nValueNumber = 0;
	if (cDefaultValue != KWContinuous::GetMissingValue() and nRecordNumber > cvValues->GetSize())
	{
		cMin = cDefaultValue;
		nValueNumber = nRecordNumber - cvValues->GetSize();
	}

	// Calcul du min parmi les valeurs non manquantes
	for (nValue = 0; nValue < cvValues->GetSize(); nValue++)
	{
		cValue = cvValues->GetAt(nValue);
		if (cValue != KWContinuous::GetMissingValue())
		{
			if (cValue < cMin)
				cMin = cValue;
			nValueNumber++;
		}
	}
	if (nValueNumber == 0)
		cMin = KWContinuous::GetMissingValue();
	return cMin;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableMax

KWDRTableMax::KWDRTableMax()
{
	SetName("TableMax");
	SetLabel("Max of values in a table");
	AddOperand(new KWDerivationRuleOperand);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRTableMax::~KWDRTableMax() {}

KWDerivationRule* KWDRTableMax::Create() const
{
	return new KWDRTableMax;
}

Continuous KWDRTableMax::ComputeContinuousStats(const ObjectArray* oaObjects) const
{
	KWDerivationRuleOperand* valueOperand;
	int nObject;
	KWObject* kwoContainedObject;
	Continuous cValue;
	int nObjectNumber;
	Continuous cMax;

	require(oaObjects != NULL);

	// Calcul du max parmi les sous-objets par les valeurs non manquantes
	cMax = KWContinuous::GetMinValue();
	nObjectNumber = 0;
	valueOperand = GetSecondOperand();
	for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
	{
		kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));
		cValue = valueOperand->GetContinuousValue(kwoContainedObject);
		if (cValue != KWContinuous::GetMissingValue())
		{
			if (cValue > cMax)
				cMax = cValue;
			nObjectNumber++;
		}
	}
	if (nObjectNumber == 0)
		cMax = KWContinuous::GetMissingValue();
	return cMax;
}

Continuous KWDRTableMax::ComputeContinuousStatsFromContinuousVector(int nRecordNumber, Continuous cDefaultValue,
								    const ContinuousVector* cvValues) const
{
	int nValue;
	Continuous cValue;
	int nValueNumber;
	Continuous cMax;

	require(nRecordNumber >= 0);
	require(cvValues != NULL);

	// Prise en compte de la valeur par defaut si elle n'est pas manquante
	cMax = KWContinuous::GetMinValue();
	nValueNumber = 0;
	if (cDefaultValue != KWContinuous::GetMissingValue() and nRecordNumber > cvValues->GetSize())
	{
		cMax = cDefaultValue;
		nValueNumber = nRecordNumber - cvValues->GetSize();
	}

	// Calcul du min parmi les valeurs non manquantes
	for (nValue = 0; nValue < cvValues->GetSize(); nValue++)
	{
		cValue = cvValues->GetAt(nValue);
		if (cValue != KWContinuous::GetMissingValue())
		{
			if (cValue > cMax)
				cMax = cValue;
			nValueNumber++;
		}
	}
	if (nValueNumber == 0)
		cMax = KWContinuous::GetMissingValue();
	return cMax;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableSum

KWDRTableSum::KWDRTableSum()
{
	SetName("TableSum");
	SetLabel("Sum of values in a table");
	AddOperand(new KWDerivationRuleOperand);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRTableSum::~KWDRTableSum() {}

KWDerivationRule* KWDRTableSum::Create() const
{
	return new KWDRTableSum;
}

Continuous KWDRTableSum::ComputeContinuousStats(const ObjectArray* oaObjects) const
{
	KWDerivationRuleOperand* valueOperand;
	int nObject;
	KWObject* kwoContainedObject;
	Continuous cValue;
	int nObjectNumber;
	Continuous cSum;

	require(oaObjects != NULL);

	// Calcul de la somme parmi les sous-objets avec valeur non manquante
	cSum = 0;
	nObjectNumber = 0;
	valueOperand = GetSecondOperand();
	for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
	{
		kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));
		cValue = valueOperand->GetContinuousValue(kwoContainedObject);
		if (cValue != KWContinuous::GetMissingValue())
		{
			cSum += cValue;
			nObjectNumber++;
		}
	}
	if (nObjectNumber == 0)
		cSum = KWContinuous::GetMissingValue();
	return cSum;
}

Continuous KWDRTableSum::ComputeContinuousStatsFromContinuousVector(int nRecordNumber, Continuous cDefaultValue,
								    const ContinuousVector* cvValues) const
{
	int nValue;
	Continuous cValue;
	int nValueNumber;
	Continuous cSum;
	int nDefaultValueNumber;

	require(nRecordNumber >= 0);
	require(cvValues != NULL);

	// Prise en compte de la valeur par defaut si elle n'est pas manquante
	cSum = 0;
	nValueNumber = 0;
	nDefaultValueNumber = nRecordNumber - cvValues->GetSize();
	if (cDefaultValue != KWContinuous::GetMissingValue() and nRecordNumber > cvValues->GetSize())
	{
		cSum = nDefaultValueNumber * cDefaultValue;
		nValueNumber = nDefaultValueNumber;
	}

	// Calcul de la somme parmi les sous-objets avec valeur non manquante
	for (nValue = 0; nValue < cvValues->GetSize(); nValue++)
	{
		cValue = cvValues->GetAt(nValue);
		if (cValue != KWContinuous::GetMissingValue())
		{
			cSum += cValue;
			nValueNumber++;
		}
	}
	if (nValueNumber == 0)
		cSum = KWContinuous::GetMissingValue();
	return cSum;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableCountSum

KWDRTableCountSum::KWDRTableCountSum()
{
	SetName("TableCountSum");
	SetLabel("Sum of counts in a table");
	AddOperand(new KWDerivationRuleOperand);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRTableCountSum::~KWDRTableCountSum() {}

KWDerivationRule* KWDRTableCountSum::Create() const
{
	return new KWDRTableCountSum;
}

Continuous KWDRTableCountSum::ComputeContinuousStats(const ObjectArray* oaObjects) const
{
	KWDerivationRuleOperand* valueOperand;
	int nObject;
	KWObject* kwoContainedObject;
	Continuous cValue;
	int nObjectNumber;
	Continuous cCountSum;

	require(oaObjects != NULL);

	// Calcul de la somme parmi les sous-objets avec valeur non manquante
	cCountSum = 0;
	nObjectNumber = 0;
	valueOperand = GetSecondOperand();
	for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
	{
		kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));
		cValue = valueOperand->GetContinuousValue(kwoContainedObject);
		if (cValue != KWContinuous::GetMissingValue())
		{
			cCountSum += cValue;
			nObjectNumber++;
		}
	}
	return cCountSum;
}

Continuous KWDRTableCountSum::ComputeContinuousStatsFromContinuousVector(int nRecordNumber, Continuous cDefaultValue,
									 const ContinuousVector* cvValues) const
{
	int nValue;
	Continuous cValue;
	int nValueNumber;
	Continuous cCountSum;
	int nDefaultValueNumber;

	require(nRecordNumber >= 0);
	require(cvValues != NULL);

	// Prise en compte de la valeur par defaut si elle n'est pas manquante
	cCountSum = 0;
	nValueNumber = 0;
	nDefaultValueNumber = nRecordNumber - cvValues->GetSize();
	if (cDefaultValue != KWContinuous::GetMissingValue() and nRecordNumber > cvValues->GetSize())
	{
		cCountSum = nDefaultValueNumber * cDefaultValue;
		nValueNumber = nDefaultValueNumber;
	}

	// Calcul de la somme parmi les valeurs non manquantes
	for (nValue = 0; nValue < cvValues->GetSize(); nValue++)
	{
		cValue = cvValues->GetAt(nValue);
		if (cValue != KWContinuous::GetMissingValue())
		{
			cCountSum += cValue;
			nValueNumber++;
		}
	}
	return cCountSum;
}

Continuous KWDRTableCountSum::GetDefaultContinuousStats() const
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableTrend

KWDRTableTrend::KWDRTableTrend()
{
	SetName("TableTrend");
	SetLabel("Trend of values in a table");
	AddOperand(new KWDerivationRuleOperand);
	GetSecondOperand()->SetType(KWType::Continuous);
	AddOperand(new KWDerivationRuleOperand);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KWDRTableTrend::~KWDRTableTrend() {}

KWDerivationRule* KWDRTableTrend::Create() const
{
	return new KWDRTableTrend;
}

Continuous KWDRTableTrend::ComputeContinuousStats(const ObjectArray* oaObjects) const
{
	KWDerivationRuleOperand* valueXOperand;
	KWDerivationRuleOperand* valueYOperand;
	int nObject;
	KWObject* kwoContainedObject;
	Continuous cXValue;
	Continuous cYValue;
	int nObjectNumber;
	double dSumX;
	double dSumY;
	double dSumXX;
	double dSumXY;
	double dTrendDenominator;
	double dTrend;
	Continuous cTrend;

	require(oaObjects != NULL);

	// Calcul de la tendance parmi les sous-objets avec valeurs non manquantes
	valueYOperand = GetSecondOperand();
	valueXOperand = GetOperandAt(2);
	nObjectNumber = 0;
	dSumX = 0;
	dSumY = 0;
	dSumXX = 0;
	dSumXY = 0;
	dTrend = 0;
	for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
	{
		kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));
		cXValue = valueXOperand->GetContinuousValue(kwoContainedObject);
		cYValue = valueYOperand->GetContinuousValue(kwoContainedObject);
		if (cXValue != KWContinuous::GetMissingValue() and cYValue != KWContinuous::GetMissingValue())
		{
			// Mise a jour des sommes
			dSumX += cXValue;
			dSumY += cYValue;
			dSumXX += cXValue * cXValue;
			dSumXY += cXValue * cYValue;
			nObjectNumber++;
		}
	}
	if (nObjectNumber > 0)
	{
		// Pente de la droite de regression lineaire des points (X,Y) (moindres carres)
		dTrendDenominator = (nObjectNumber * dSumXX - dSumX * dSumX);
		if (fabs(dTrendDenominator) > KWContinuous::GetEpsilonValue())
		{
			dTrend = (nObjectNumber * dSumXY - dSumX * dSumY) / dTrendDenominator;
			cTrend = (Continuous)dTrend;
		}
		else
			cTrend = KWContinuous::GetMissingValue();
	}
	else
		cTrend = KWContinuous::GetMissingValue();
	return cTrend;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableConcat

KWDRTableConcat::KWDRTableConcat()
{
	SetName("TableConcat");
	SetLabel("Concatenation of values in a table");
	AddOperand(new KWDerivationRuleOperand);
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRTableConcat::~KWDRTableConcat() {}

KWDerivationRule* KWDRTableConcat::Create() const
{
	return new KWDRTableConcat;
}

Symbol KWDRTableConcat::ComputeSymbolStats(const ObjectArray* oaObjects) const
{
	KWDerivationRuleOperand* valueOperand;
	int nObject;
	KWObject* kwoContainedObject;
	Symbol sValue;
	ALString sConcat;

	require(oaObjects != NULL);

	// Concatenation du second operand des sous-objets
	valueOperand = GetSecondOperand();
	for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
	{
		kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));
		sValue = valueOperand->GetSymbolValue(kwoContainedObject);
		sConcat += sValue;
	}
	return Symbol(sConcat);
}

Symbol KWDRTableConcat::ComputeSymbolStatsFromSymbolVector(int nRecordNumber, Symbol sDefaultValue,
							   const SymbolVector* svValues) const
{
	int nValue;
	Symbol sValue;
	ALString sConcat;

	require(nRecordNumber >= 0);
	require(svValues != NULL);

	// Attention, il est necessaire que la valeur par defaut soit "", sinon on ne peut calculer la concatenation
	// qui serait basee sur des valeur par defaut pouvant s'inserer entre n'importe quelle valeur du
	// vecteur de valeurs presentes
	require(sDefaultValue == Symbol());

	// Parcours des valeurs du vecteur
	for (nValue = 0; nValue < svValues->GetSize(); nValue++)
	{
		sValue = svValues->GetAt(nValue);
		sConcat += sValue;
	}
	return Symbol(sConcat);
}

///////////////////////////////////////////////////////////////////////////
// Classe KWDRTableSymbolVector

KWDRTableSymbolVector::KWDRTableSymbolVector()
{
	SetName("TableVectorC");
	SetLabel("Categorical vector from a table");
	SetType(KWType::Structure);
	SetStructureName("VectorC");
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::Symbol);
}

KWDRTableSymbolVector::~KWDRTableSymbolVector() {}

KWDerivationRule* KWDRTableSymbolVector::Create() const
{
	return new KWDRTableSymbolVector;
}

Object* KWDRTableSymbolVector::ComputeStructureResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* valueOperand;
	ObjectArray* oaObjects;
	int nObject;
	KWObject* kwoContainedObject;
	Symbol sValue;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche des valeurs du container
	oaObjects = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjects == NULL)
		symbolVector.SetValueNumber(0);
	else
	{
		// Parcours des objets du container
		symbolVector.SetValueNumber(oaObjects->GetSize());
		valueOperand = GetSecondOperand();
		for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
		{
			kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));

			// Memorisation de la valeurs
			sValue = valueOperand->GetSymbolValue(kwoContainedObject);
			symbolVector.SetValueAt(nObject, sValue);
		}
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return &symbolVector;
}

longint KWDRTableSymbolVector::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRTableSymbolVector) - sizeof(KWDerivationRule);
	lUsedMemory += symbolVector.GetUsedMemory();
	return lUsedMemory;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableContinuousVector

KWDRTableContinuousVector::KWDRTableContinuousVector()
{
	SetName("TableVector");
	SetLabel("Numerical vector from a table");
	SetType(KWType::Structure);
	SetStructureName("Vector");
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::Continuous);
}

KWDRTableContinuousVector::~KWDRTableContinuousVector() {}

KWDerivationRule* KWDRTableContinuousVector::Create() const
{
	return new KWDRTableContinuousVector;
}

Object* KWDRTableContinuousVector::ComputeStructureResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* valueOperand;
	ObjectArray* oaObjects;
	int nObject;
	KWObject* kwoContainedObject;
	Continuous cValue;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Recherche des valeurs du container
	oaObjects = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjects == NULL)
		continuousVector.SetValueNumber(0);
	else
	{
		// Parcours des objets du container
		continuousVector.SetValueNumber(oaObjects->GetSize());
		valueOperand = GetSecondOperand();
		for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
		{
			kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));

			// Memorisation de la valeurs
			cValue = valueOperand->GetContinuousValue(kwoContainedObject);
			continuousVector.SetValueAt(nObject, cValue);
		}
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return &continuousVector;
}

longint KWDRTableContinuousVector::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRTableContinuousVector) - sizeof(KWDerivationRule);
	lUsedMemory += continuousVector.GetUsedMemory();
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////////////////
// Classe KWDRTableSymbolHashMap

KWDRTableSymbolHashMap::KWDRTableSymbolHashMap()
{
	SetName("TableHashMapC");
	SetLabel("Categorical hash map from a table");
	SetType(KWType::Structure);
	SetStructureName("HashMapC");
	SetMultipleScope(true);
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Symbol);
}

KWDRTableSymbolHashMap::~KWDRTableSymbolHashMap() {}

KWDerivationRule* KWDRTableSymbolHashMap::Create() const
{
	return new KWDRTableSymbolHashMap;
}

Object* KWDRTableSymbolHashMap::ComputeStructureResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* keyOperand;
	KWDerivationRuleOperand* valueOperand;
	ObjectArray* oaObjects;
	int nObject;
	KWObject* kwoContainedObject;
	int nKey;
	Symbol sKey;
	Symbol sValue;
	SymbolVector svKeys;
	NumericKeyDictionary nkdObjects;
	KWDRSymbolVector* keyVector;
	KWDRSymbolVector* valueVector;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Nettoyage prealable de la hashmap
	if (symbolHashMap.GetFirstOperand()->GetDerivationRule() != NULL)
		delete symbolHashMap.GetFirstOperand()->GetDerivationRule();
	if (symbolHashMap.GetSecondOperand()->GetDerivationRule() != NULL)
		delete symbolHashMap.GetSecondOperand()->GetDerivationRule();

	// Initialisation du vecteur de cle
	keyVector = new KWDRSymbolVector;
	keyVector->SetValueNumber(0);
	symbolHashMap.GetFirstOperand()->SetDerivationRule(keyVector);

	// Initialisation du vecteur de valeurs
	valueVector = new KWDRSymbolVector;
	valueVector->SetValueNumber(0);
	symbolHashMap.GetSecondOperand()->SetDerivationRule(valueVector);

	// Alimentation du dictionnaire de valeurs
	oaObjects = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjects != NULL)
	{
		keyOperand = GetSecondOperand();
		valueOperand = GetOperandAt(2);

		// Parcours des objets du container pour determiner les cles a garder
		for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
		{
			kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));

			// Memorisation de la cle si nouvelle cle
			sKey = keyOperand->GetSymbolValue(kwoContainedObject);
			if (nkdObjects.Lookup(sKey.GetNumericKey()) == NULL)
			{
				svKeys.Add(sKey);
				nkdObjects.SetAt(sKey.GetNumericKey(), kwoContainedObject);
			}
		}

		// Memorisation des cle et des valeurs, en taille prealablement les vecteurs
		keyVector->SetValueNumber(svKeys.GetSize());
		valueVector->SetValueNumber(svKeys.GetSize());
		for (nKey = 0; nKey < svKeys.GetSize(); nKey++)
		{
			// Memorisation de la cle
			sKey = svKeys.GetAt(nKey);
			keyVector->SetValueAt(nKey, sKey);

			// Memorisation de la valeur
			kwoContainedObject = cast(KWObject*, nkdObjects.Lookup(sKey.GetNumericKey()));
			check(kwoContainedObject);
			sValue = valueOperand->GetSymbolValue(kwoContainedObject);
			valueVector->SetValueAt(nKey, sValue);
		}
	}

	// Optimisation de la regle de hashmap
	symbolHashMap.Optimize();

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return &symbolHashMap;
}

longint KWDRTableSymbolHashMap::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRTableSymbolHashMap) - sizeof(KWDerivationRule);
	lUsedMemory += symbolHashMap.GetUsedMemory();
	return lUsedMemory;
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDRTableContinuousHashMap

KWDRTableContinuousHashMap::KWDRTableContinuousHashMap()
{
	SetName("TableHashMap");
	SetLabel("Numerical hash map from a table");
	SetType(KWType::Structure);
	SetStructureName("HashMap");
	SetMultipleScope(true);
	SetOperandNumber(3);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::Symbol);
	GetOperandAt(2)->SetType(KWType::Continuous);
}

KWDRTableContinuousHashMap::~KWDRTableContinuousHashMap() {}

KWDerivationRule* KWDRTableContinuousHashMap::Create() const
{
	return new KWDRTableContinuousHashMap;
}

Object* KWDRTableContinuousHashMap::ComputeStructureResult(const KWObject* kwoObject) const
{
	KWDerivationRuleOperand* keyOperand;
	KWDerivationRuleOperand* valueOperand;
	ObjectArray* oaObjects;
	int nObject;
	KWObject* kwoContainedObject;
	int nKey;
	Symbol sKey;
	Continuous cValue;
	SymbolVector svKeys;
	NumericKeyDictionary nkdObjects;
	KWDRSymbolVector* keyVector;
	KWDRContinuousVector* valueVector;

	require(IsCompiled());

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Nettoyage prealable de la hashmap
	if (continuousHashMap.GetFirstOperand()->GetDerivationRule() != NULL)
		delete continuousHashMap.GetFirstOperand()->GetDerivationRule();
	if (continuousHashMap.GetSecondOperand()->GetDerivationRule() != NULL)
		delete continuousHashMap.GetSecondOperand()->GetDerivationRule();

	// Initialisation du vecteur de cle
	keyVector = new KWDRSymbolVector;
	keyVector->SetValueNumber(0);
	continuousHashMap.GetFirstOperand()->SetDerivationRule(keyVector);

	// Initialisation du vecteur de valeurs
	valueVector = new KWDRContinuousVector;
	valueVector->SetValueNumber(0);
	continuousHashMap.GetSecondOperand()->SetDerivationRule(valueVector);

	// Alimentation du dictionnaire de valeurs
	oaObjects = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjects != NULL)
	{
		keyOperand = GetSecondOperand();
		valueOperand = GetOperandAt(2);

		// Parcours des objets du container pour determiner les cles a garder
		for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
		{
			kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));

			// Memorisation de la cle si nouvelle cle
			sKey = keyOperand->GetSymbolValue(kwoContainedObject);
			if (nkdObjects.Lookup(sKey.GetNumericKey()) == NULL)
			{
				svKeys.Add(sKey);
				nkdObjects.SetAt(sKey.GetNumericKey(), kwoContainedObject);
			}
		}

		// Memorisation des cle et des valeurs, en taille prealablement les vecteurs
		keyVector->SetValueNumber(svKeys.GetSize());
		valueVector->SetValueNumber(svKeys.GetSize());
		for (nKey = 0; nKey < svKeys.GetSize(); nKey++)
		{
			// Memorisation de la cle
			sKey = svKeys.GetAt(nKey);
			keyVector->SetValueAt(nKey, sKey);

			// Memorisation de la valeur
			kwoContainedObject = cast(KWObject*, nkdObjects.Lookup(sKey.GetNumericKey()));
			check(kwoContainedObject);
			cValue = valueOperand->GetContinuousValue(kwoContainedObject);
			valueVector->SetValueAt(nKey, cValue);
		}
	}

	// Optimisation de la regle de hashmap
	continuousHashMap.Optimize();

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return &continuousHashMap;
}

longint KWDRTableContinuousHashMap::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRTableContinuousHashMap) - sizeof(KWDerivationRule);
	lUsedMemory += continuousHashMap.GetUsedMemory();
	return lUsedMemory;
}
