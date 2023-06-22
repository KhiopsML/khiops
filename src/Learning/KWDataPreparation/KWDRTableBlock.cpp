// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRTableBlock.h"

void KWDRRegisterTableBlockRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRTableBlockCountDistinct);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableBlockEntropy);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableBlockMode);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableBlockMean);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableBlockStandardDeviation);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableBlockMedian);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableBlockMin);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableBlockMax);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableBlockSum);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableBlockCountSum);
	KWDerivationRule::RegisterDerivationRule(new KWDRTableBlockConcat);
}

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockStats

KWDRTableBlockStats::KWDRTableBlockStats()
{
	SetMultipleScope(true);
	SetOperandNumber(2);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::Unknown);

	// Regle permettant d'effectuer le calcul par partie
	tableStatsRule = NULL;

	// Donnees issues de la compilation de la regle
	tableAttributeBlock = NULL;
	tableBlockIndexedKeyBlock = NULL;

	// On indique l'index de l'operande de type bloc, en tant que sous classe de KWDRValueBlockRule
	nSourceValueBlockOperandIndex = 1;

	// On indique qu'il ne faut pas deduire le type de bloc retourne de celui en operande
	bReturnTypeSameAsOperandType = false;
}

KWDRTableBlockStats::~KWDRTableBlockStats()
{
	assert(tableStatsRule != NULL);
	delete tableStatsRule;
}

int KWDRTableBlockStats::GetVarKeyType() const
{
	return KWType::Continuous;
}

const KWDRTableStats* KWDRTableBlockStats::GetTableStatsRule() const
{
	return tableStatsRule;
}

void KWDRTableBlockStats::Compile(KWClass* kwcOwnerClass)
{
	// Compilation de la classe ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Memorisation du bloc d'index associe a la partition
	tableAttributeBlock = GetSecondOperand()->GetOriginAttributeBlock();
	tableBlockIndexedKeyBlock =
	    cast(const KWIndexedNKeyBlock*, tableAttributeBlock->GetLoadedAttributesIndexedKeyBlock());
}

longint KWDRTableBlockStats::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRTableBlockStats) - sizeof(KWDerivationRule);
	assert(tableStatsRule != NULL);
	lUsedMemory += tableStatsRule->GetUsedMemory();
	return lUsedMemory;
}

void KWDRTableBlockStats::InitializeFromTableStatsRule(KWDRTableStats* rule)
{
	const ALString sNamePrefix = "Table";
	const ALString sLabelSuffix = "in a table";
	int nOperand;

	require(tableStatsRule == NULL);
	require(rule != NULL);
	require(rule->GetName().Find(sNamePrefix) >= 0);
	require(rule->GetLabel().Find(sLabelSuffix) >= 0);
	require(KWType::GetValueBlockType(rule->GetType()) == GetType());
	require(rule->GetOperandNumber() >= 2);
	require(rule->GetFirstOperand()->GetType() == KWType::ObjectArray);
	require(KWType::IsSimple(rule->GetSecondOperand()->GetType()));
	require(GetOperandNumber() == 2);
	require(GetSecondOperand()->GetType() == KWType::Unknown);

	// Memorisation de la regle
	tableStatsRule = rule;

	// Parametrage du nom et du libelle a partir de la regle de type TableStats
	SetName("TableBlock" + rule->GetName().Right(rule->GetName().GetLength() - sNamePrefix.GetLength()));
	SetLabel(rule->GetLabel().Left(rule->GetLabel().Find(sLabelSuffix)) + "by value in a sparse block");

	// On passe le type du deuxieme operande en block du meme type simple que le deuxieme operande de la regle
	GetSecondOperand()->SetType(KWType::GetValueBlockType(rule->GetSecondOperand()->GetType()));

	// On cree d'eventuele nouveaux operandes en fonction des operandes restant de la regle
	for (nOperand = 2; nOperand < rule->GetOperandNumber(); nOperand++)
	{
		AddOperand(new KWDerivationRuleOperand);
		GetOperandAt(nOperand)->SetType(rule->GetOperandAt(nOperand)->GetType());
	}
}

int KWDRTableBlockStats::ComputeContinuousVectors(const KWObject* kwoObject, const KWIndexedKeyBlock* indexedKeyBlock,
						  ObjectArray* oaOutputVectors) const
{
	int nRecordNumber;
	KWDerivationRuleOperand* valueOperand;
	ObjectArray* oaObjects;
	int nObject;
	KWObject* kwoContainedObject;
	KWContinuousValueBlock* containedValueBlock;
	int n;
	int nSourceSparseIndex;
	int nSparseIndex;
	debug(int nCheckedNKey);
	debug(int nCheckedSparseIndex);
	Continuous cValue;
	ContinuousVector* cvVector;

	require(IsCompiled());
	require(nDynamicCompileFreshness == nCompileFreshness);
	require(GetSecondOperand()->GetType() == KWType::ContinuousValueBlock);
	require(kwoObject != NULL);
	require(indexedKeyBlock != NULL);
	require(oaOutputVectors != NULL);
	require(oaOutputVectors->GetSize() == 0);

	// Initialisation du tableau resultat
	oaOutputVectors->SetSize(indexedKeyBlock->GetKeyNumber());

	// Calcul du resultat avec le tableau du premier operande et
	// la valeur accessible avec le deuxieme operande
	nRecordNumber = 0;
	oaObjects = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjects != NULL and oaObjects->GetSize() > 0)
	{
		valueOperand = GetSecondOperand();
		nRecordNumber = oaObjects->GetSize();

		// Parcours des valeurs du bloc sparse source, dans le cas ou les index source et cible sont identiques
		if (bSameValueIndexes)
		{
			// Extraction des valeurs des blocs secondaires
			for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
			{
				// Acecs au bloc de valeur de l'objet secondaire
				kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));
				containedValueBlock = valueOperand->GetContinuousValueBlock(kwoContainedObject);

				for (n = 0; n < containedValueBlock->GetValueNumber(); n++)
				{
					// Index sparse de la valeur du bloc source
					nSourceSparseIndex = containedValueBlock->GetAttributeSparseIndexAt(n);

					// Index sparse cible
					nSparseIndex = nSourceSparseIndex;

					// Verification que ce sont bien les meme index sparse
					debug(nCheckedNKey = tableBlockIndexedKeyBlock->GetKeyAt(nSourceSparseIndex));
					debug(
					    nCheckedSparseIndex =
						cast(KWIndexedNKeyBlock*, indexedKeyBlock)->GetKeyIndex(nCheckedNKey));
					debug(assert(nSparseIndex == nCheckedSparseIndex));

					// Recherche de la valeur du bloc sparse
					cValue = containedValueBlock->GetValueAt(n);

					// Creation si necessaire d'un vecteur de valeurs
					cvVector = cast(ContinuousVector*, oaOutputVectors->GetAt(nSparseIndex));
					if (cvVector == NULL)
					{
						cvVector = new ContinuousVector;
						oaOutputVectors->SetAt(nSparseIndex, cvVector);
					}

					// Memorisation dans le vecteur de valeurs
					cvVector->Add(cValue);
				}
			}
		}
		// Et dans le cas ou ces index sparse sont different
		else
		{
			for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
			{
				// Acecs au bloc de valeur de l'objet secondaire
				kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));
				containedValueBlock = valueOperand->GetContinuousValueBlock(kwoContainedObject);

				// Parcours des valeurs du bloc sparse
				for (n = 0; n < containedValueBlock->GetValueNumber(); n++)
				{
					// Index sparse de la partie du bloc de partie
					nSourceSparseIndex = containedValueBlock->GetAttributeSparseIndexAt(n);

					// Index sparse cible
					nSparseIndex = ivNewValueIndexes.GetAt(nSourceSparseIndex);

					// Verification que ce sont bien les meme index sparse
					debug(nCheckedNKey = tableBlockIndexedKeyBlock->GetKeyAt(nSourceSparseIndex));
					debug(
					    nCheckedSparseIndex =
						cast(KWIndexedNKeyBlock*, indexedKeyBlock)->GetKeyIndex(nCheckedNKey));
					debug(assert(nSparseIndex == nCheckedSparseIndex));

					// Traitement de la partie uniquement si necessaire
					if (nSparseIndex != -1)
					{
						// Recherche de la valeur du bloc sparse
						cValue = containedValueBlock->GetValueAt(n);

						// Creation si necessaire d'un vecteur de valeurs
						cvVector =
						    cast(ContinuousVector*, oaOutputVectors->GetAt(nSparseIndex));
						if (cvVector == NULL)
						{
							cvVector = new ContinuousVector;
							oaOutputVectors->SetAt(nSparseIndex, cvVector);
						}

						// Memorisation dans le vecteur de valeurs
						cvVector->Add(cValue);
					}
				}
			}
		}
	}
	return nRecordNumber;
}

int KWDRTableBlockStats::ComputeSymbolVectors(const KWObject* kwoObject, const KWIndexedKeyBlock* indexedKeyBlock,
					      ObjectArray* oaOutputVectors) const
{
	int nRecordNumber;
	KWDerivationRuleOperand* valueOperand;
	ObjectArray* oaObjects;
	int nObject;
	KWObject* kwoContainedObject;
	KWSymbolValueBlock* containedValueBlock;
	int n;
	int nSourceSparseIndex;
	int nSparseIndex;
	debug(int nCheckedNKey);
	debug(int nCheckedSparseIndex);
	Symbol sValue;
	SymbolVector* svVector;

	require(IsCompiled());
	require(nDynamicCompileFreshness == nCompileFreshness);
	require(GetSecondOperand()->GetType() == KWType::SymbolValueBlock);
	require(kwoObject != NULL);
	require(indexedKeyBlock != NULL);
	require(oaOutputVectors != NULL);
	require(oaOutputVectors->GetSize() == 0);

	// Initialisation du tableau resultat
	oaOutputVectors->SetSize(indexedKeyBlock->GetKeyNumber());

	// Calcul du resultat avec le tableau du premier operande et
	// la valeur accessible avec le deuxieme operande
	nRecordNumber = 0;
	oaObjects = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjects != NULL and oaObjects->GetSize() > 0)
	{
		valueOperand = GetSecondOperand();
		nRecordNumber = oaObjects->GetSize();

		// Parcours des valeurs du bloc sparse source, dans le cas ou les index source et cible sont identiques
		if (bSameValueIndexes)
		{
			// Extraction des valeurs des blocs secondaires
			for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
			{
				// Acecs au bloc de valeur de l'objet secondaire
				kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));
				containedValueBlock = valueOperand->GetSymbolValueBlock(kwoContainedObject);

				for (n = 0; n < containedValueBlock->GetValueNumber(); n++)
				{
					// Index sparse de la valeur du bloc source
					nSourceSparseIndex = containedValueBlock->GetAttributeSparseIndexAt(n);

					// Index sparse cible
					nSparseIndex = nSourceSparseIndex;

					// Verification que ce sont bien les meme index sparse
					debug(nCheckedNKey = tableBlockIndexedKeyBlock->GetKeyAt(nSourceSparseIndex));
					debug(
					    nCheckedSparseIndex =
						cast(KWIndexedNKeyBlock*, indexedKeyBlock)->GetKeyIndex(nCheckedNKey));
					debug(assert(nSparseIndex == nCheckedSparseIndex));

					// Recherche de la valeur du bloc sparse
					sValue = containedValueBlock->GetValueAt(n);

					// Creation si necessaire d'un vecteur de valeurs
					svVector = cast(SymbolVector*, oaOutputVectors->GetAt(nSparseIndex));
					if (svVector == NULL)
					{
						svVector = new SymbolVector;
						oaOutputVectors->SetAt(nSparseIndex, svVector);
					}

					// Memorisation dans le vecteur de valeurs
					svVector->Add(sValue);
				}
			}
		}
		// Et dans le cas ou ces index sparse sont different
		else
		{
			for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
			{
				// Acecs au bloc de valeur de l'objet secondaire
				kwoContainedObject = cast(KWObject*, oaObjects->GetAt(nObject));
				containedValueBlock = valueOperand->GetSymbolValueBlock(kwoContainedObject);

				// Parcours des valeurs du bloc sparse
				for (n = 0; n < containedValueBlock->GetValueNumber(); n++)
				{
					// Index sparse de la partie du bloc de partie
					nSourceSparseIndex = containedValueBlock->GetAttributeSparseIndexAt(n);

					// Index sparse cible
					nSparseIndex = ivNewValueIndexes.GetAt(nSourceSparseIndex);

					// Verification que ce sont bien les meme index sparse
					debug(nCheckedNKey = tableBlockIndexedKeyBlock->GetKeyAt(nSourceSparseIndex));
					debug(
					    nCheckedSparseIndex =
						cast(KWIndexedNKeyBlock*, indexedKeyBlock)->GetKeyIndex(nCheckedNKey));
					debug(assert(nSparseIndex == nCheckedSparseIndex));

					// Traitement de la partie uniquement si necessaire
					if (nSparseIndex != -1)
					{
						// Recherche de la valeur du bloc sparse
						sValue = containedValueBlock->GetValueAt(n);

						// Creation si necessaire d'un vecteur de valeurs
						svVector = cast(SymbolVector*, oaOutputVectors->GetAt(nSparseIndex));
						if (svVector == NULL)
						{
							svVector = new SymbolVector;
							oaOutputVectors->SetAt(nSparseIndex, svVector);
						}

						// Memorisation dans le vecteur de valeurs
						svVector->Add(sValue);
					}
				}
			}
		}
	}
	return nRecordNumber;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockStatsContinuous

KWDRTableBlockStatsContinuous::KWDRTableBlockStatsContinuous()
{
	SetType(KWType::ContinuousValueBlock);
}

KWDRTableBlockStatsContinuous::~KWDRTableBlockStatsContinuous() {}

KWContinuousValueBlock*
KWDRTableBlockStatsContinuous::ComputeContinuousValueBlockResult(const KWObject* kwoObject,
								 const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWContinuousValueBlock* resultValueBlock;
	int nRecordNumber;
	Continuous cRecordDefaultValue;
	Symbol sRecordDefaultValue;
	ObjectArray oaVectors;
	ContinuousVector* cvVector;
	SymbolVector* svVector;
	ContinuousVector cvEmptyVector;
	SymbolVector svEmptyVector;
	int nSparseIndex;
	IntVector ivSparseIndexes;
	ContinuousVector cvSparseValues;
	Continuous cResult;
	int i;

	require(IsCompiled());

	// Compilation dynamique
	DynamicCompile(indexedKeyBlock);

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Calcul du tableau de vecteurs pour chaque attribut du bloc sparse
	if (GetSecondOperand()->GetType() == KWType::ContinuousValueBlock)
	{
		nRecordNumber = ComputeContinuousVectors(kwoObject, indexedKeyBlock, &oaVectors);
		cRecordDefaultValue = tableAttributeBlock->GetContinuousDefaultValue();

		// Calcul des valeurs pour chaque vecteur non vide du vecteur
		for (nSparseIndex = 0; nSparseIndex < oaVectors.GetSize(); nSparseIndex++)
		{
			cvVector = cast(ContinuousVector*, oaVectors.GetAt(nSparseIndex));
			if (cvVector == NULL)
				cvVector = &cvEmptyVector;
			cResult = tableStatsRule->ComputeContinuousStatsFromContinuousVector(
			    nRecordNumber, cRecordDefaultValue, cvVector);
			if (cResult != GetValueBlockContinuousDefaultValue())
			{
				ivSparseIndexes.Add(nSparseIndex);
				cvSparseValues.Add(cResult);
			}
		}
	}
	else
	{
		nRecordNumber = ComputeSymbolVectors(kwoObject, indexedKeyBlock, &oaVectors);
		sRecordDefaultValue = tableAttributeBlock->GetSymbolDefaultValue();

		// Calcul des valeurs pour chaque vecteur non vide du vecteur
		for (nSparseIndex = 0; nSparseIndex < oaVectors.GetSize(); nSparseIndex++)
		{
			svVector = cast(SymbolVector*, oaVectors.GetAt(nSparseIndex));
			if (svVector == NULL)
				svVector = &svEmptyVector;
			cResult = tableStatsRule->ComputeContinuousStatsFromSymbolVector(nRecordNumber,
											 sRecordDefaultValue, svVector);
			if (cResult != GetValueBlockContinuousDefaultValue())
			{
				ivSparseIndexes.Add(nSparseIndex);
				cvSparseValues.Add(cResult);
			}
		}
	}

	// Creation du bloc sparse
	resultValueBlock = KWContinuousValueBlock::NewValueBlock(ivSparseIndexes.GetSize());
	for (i = 0; i < ivSparseIndexes.GetSize(); i++)
	{
		resultValueBlock->SetAttributeSparseIndexAt(i, ivSparseIndexes.GetAt(i));
		resultValueBlock->SetValueAt(i, cvSparseValues.GetAt(i));
	}

	// Nettoyage
	oaVectors.DeleteAll();

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return resultValueBlock;
}

Continuous KWDRTableBlockStatsContinuous::GetValueBlockContinuousDefaultValue() const
{
	return cast(KWDRTableStatsContinuous*, tableStatsRule)->GetDefaultContinuousStats();
}

longint KWDRTableBlockStatsContinuous::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRTableBlockStats::GetUsedMemory();
	lUsedMemory += sizeof(KWDRTableBlockStatsContinuous) - sizeof(KWDRTableBlockStats);
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockStatsSymbol

KWDRTableBlockStatsSymbol::KWDRTableBlockStatsSymbol()
{
	SetType(KWType::SymbolValueBlock);
}

KWDRTableBlockStatsSymbol::~KWDRTableBlockStatsSymbol() {}

KWSymbolValueBlock*
KWDRTableBlockStatsSymbol::ComputeSymbolValueBlockResult(const KWObject* kwoObject,
							 const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWSymbolValueBlock* resultValueBlock;
	int nRecordNumber;
	Continuous cRecordDefaultValue;
	Symbol sRecordDefaultValue;
	ObjectArray oaVectors;
	ContinuousVector* cvVector;
	SymbolVector* svVector;
	ContinuousVector cvEmptyVector;
	SymbolVector svEmptyVector;
	int nSparseIndex;
	IntVector ivSparseIndexes;
	SymbolVector svSparseValues;
	Symbol sResult;
	int i;

	require(IsCompiled());

	// Compilation dynamique
	DynamicCompile(indexedKeyBlock);

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Calcul du tableau de vecteurs pour chaque attribut du bloc sparse
	if (GetSecondOperand()->GetType() == KWType::ContinuousValueBlock)
	{
		nRecordNumber = ComputeContinuousVectors(kwoObject, indexedKeyBlock, &oaVectors);
		cRecordDefaultValue = tableAttributeBlock->GetContinuousDefaultValue();

		// Calcul des valeurs pour chaque vecteur non vide du vecteur
		for (nSparseIndex = 0; nSparseIndex < oaVectors.GetSize(); nSparseIndex++)
		{
			cvVector = cast(ContinuousVector*, oaVectors.GetAt(nSparseIndex));
			if (cvVector == NULL)
				cvVector = &cvEmptyVector;
			sResult = tableStatsRule->ComputeSymbolStatsFromContinuousVector(nRecordNumber,
											 cRecordDefaultValue, cvVector);
			if (sResult != GetValueBlockSymbolDefaultValue())
			{
				ivSparseIndexes.Add(nSparseIndex);
				svSparseValues.Add(sResult);
			}
		}
	}
	else
	{
		nRecordNumber = ComputeSymbolVectors(kwoObject, indexedKeyBlock, &oaVectors);
		sRecordDefaultValue = tableAttributeBlock->GetSymbolDefaultValue();

		// Calcul des valeurs pour chaque vecteur non vide du vecteur
		for (nSparseIndex = 0; nSparseIndex < oaVectors.GetSize(); nSparseIndex++)
		{
			svVector = cast(SymbolVector*, oaVectors.GetAt(nSparseIndex));
			if (svVector == NULL)
				svVector = &svEmptyVector;
			sResult = tableStatsRule->ComputeSymbolStatsFromSymbolVector(nRecordNumber, sRecordDefaultValue,
										     svVector);
			if (sResult != GetValueBlockSymbolDefaultValue())
			{
				ivSparseIndexes.Add(nSparseIndex);
				svSparseValues.Add(sResult);
			}
		}
	}

	// Creation du bloc sparse
	resultValueBlock = KWSymbolValueBlock::NewValueBlock(ivSparseIndexes.GetSize());
	for (i = 0; i < ivSparseIndexes.GetSize(); i++)
	{
		resultValueBlock->SetAttributeSparseIndexAt(i, ivSparseIndexes.GetAt(i));
		resultValueBlock->SetValueAt(i, svSparseValues.GetAt(i));
	}

	// Nettoyage
	oaVectors.DeleteAll();

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return resultValueBlock;
}

Symbol& KWDRTableBlockStatsSymbol::GetValueBlockSymbolDefaultValue() const
{
	static Symbol sDefaultValue;
	return sDefaultValue;
}

longint KWDRTableBlockStatsSymbol::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRTableBlockStats::GetUsedMemory();
	lUsedMemory += sizeof(KWDRTableBlockStatsSymbol) - sizeof(KWDRTableBlockStats);
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockCountDistinct

KWDRTableBlockCountDistinct::KWDRTableBlockCountDistinct()
{
	InitializeFromTableStatsRule(new KWDRTableCountDistinct);
}

KWDRTableBlockCountDistinct::~KWDRTableBlockCountDistinct() {}

KWDerivationRule* KWDRTableBlockCountDistinct::Create() const
{
	return new KWDRTableBlockCountDistinct;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockEntropy

KWDRTableBlockEntropy::KWDRTableBlockEntropy()
{
	InitializeFromTableStatsRule(new KWDRTableEntropy);
}

KWDRTableBlockEntropy::~KWDRTableBlockEntropy() {}

KWDerivationRule* KWDRTableBlockEntropy::Create() const
{
	return new KWDRTableBlockEntropy;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockMode

KWDRTableBlockMode::KWDRTableBlockMode()
{
	InitializeFromTableStatsRule(new KWDRTableMode);
}

KWDRTableBlockMode::~KWDRTableBlockMode() {}

KWDerivationRule* KWDRTableBlockMode::Create() const
{
	return new KWDRTableBlockMode;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockMean

KWDRTableBlockMean::KWDRTableBlockMean()
{
	InitializeFromTableStatsRule(new KWDRTableMean);
}

KWDRTableBlockMean::~KWDRTableBlockMean() {}

KWDerivationRule* KWDRTableBlockMean::Create() const
{
	return new KWDRTableBlockMean;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockStandardDeviation

KWDRTableBlockStandardDeviation::KWDRTableBlockStandardDeviation()
{
	InitializeFromTableStatsRule(new KWDRTableStandardDeviation);
}

KWDRTableBlockStandardDeviation::~KWDRTableBlockStandardDeviation() {}

KWDerivationRule* KWDRTableBlockStandardDeviation::Create() const
{
	return new KWDRTableBlockStandardDeviation;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockMedian

KWDRTableBlockMedian::KWDRTableBlockMedian()
{
	InitializeFromTableStatsRule(new KWDRTableMedian);
}

KWDRTableBlockMedian::~KWDRTableBlockMedian() {}

KWDerivationRule* KWDRTableBlockMedian::Create() const
{
	return new KWDRTableBlockMedian;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockMin

KWDRTableBlockMin::KWDRTableBlockMin()
{
	InitializeFromTableStatsRule(new KWDRTableMin);
}

KWDRTableBlockMin::~KWDRTableBlockMin() {}

KWDerivationRule* KWDRTableBlockMin::Create() const
{
	return new KWDRTableBlockMin;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockMax

KWDRTableBlockMax::KWDRTableBlockMax()
{
	InitializeFromTableStatsRule(new KWDRTableMax);
}

KWDRTableBlockMax::~KWDRTableBlockMax() {}

KWDerivationRule* KWDRTableBlockMax::Create() const
{
	return new KWDRTableBlockMax;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockSum

KWDRTableBlockSum::KWDRTableBlockSum()
{
	InitializeFromTableStatsRule(new KWDRTableSum);
}

KWDRTableBlockSum::~KWDRTableBlockSum() {}

KWDerivationRule* KWDRTableBlockSum::Create() const
{
	return new KWDRTableBlockSum;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockCountSum

KWDRTableBlockCountSum::KWDRTableBlockCountSum()
{
	InitializeFromTableStatsRule(new KWDRTableCountSum);
}

KWDRTableBlockCountSum::~KWDRTableBlockCountSum() {}

KWDerivationRule* KWDRTableBlockCountSum::Create() const
{
	return new KWDRTableBlockCountSum;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTableBlockConcat

KWDRTableBlockConcat::KWDRTableBlockConcat()
{
	InitializeFromTableStatsRule(new KWDRTableConcat);
}

KWDRTableBlockConcat::~KWDRTableBlockConcat() {}

KWDerivationRule* KWDRTableBlockConcat::Create() const
{
	return new KWDRTableBlockConcat;
}