// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRTablePartition.h"

void KWDRRegisterTablePartitionRules()
{
	KWDerivationRule::RegisterDerivationRule(new KWDRPartition);
	KWDerivationRule::RegisterDerivationRule(new KWDRTablePartition);
	KWDerivationRule::RegisterDerivationRule(new KWDRTablePartitionCount);
	KWDerivationRule::RegisterDerivationRule(new KWDRTablePartitionCountDistinct);
	KWDerivationRule::RegisterDerivationRule(new KWDRTablePartitionEntropy);
	KWDerivationRule::RegisterDerivationRule(new KWDRTablePartitionMode);
	KWDerivationRule::RegisterDerivationRule(new KWDRTablePartitionModeAt);
	KWDerivationRule::RegisterDerivationRule(new KWDRTablePartitionMean);
	KWDerivationRule::RegisterDerivationRule(new KWDRTablePartitionStandardDeviation);
	KWDerivationRule::RegisterDerivationRule(new KWDRTablePartitionMedian);
	KWDerivationRule::RegisterDerivationRule(new KWDRTablePartitionMin);
	KWDerivationRule::RegisterDerivationRule(new KWDRTablePartitionMax);
	KWDerivationRule::RegisterDerivationRule(new KWDRTablePartitionSum);
	KWDerivationRule::RegisterDerivationRule(new KWDRTablePartitionCountSum);
	KWDerivationRule::RegisterDerivationRule(new KWDRTablePartitionTrend);
	KWDerivationRule::RegisterDerivationRule(new KWDRTablePartitionConcat);
}

///////////////////////////////////////////////////////////////
// Classe KWDRPartition

KWDRPartition::KWDRPartition()
{
	SetName("Partition");
	SetLabel("Partition");
	SetType(KWType::Structure);
	SetStructureName("Partition");
	SetOperandNumber(1);
	SetVariableOperandNumber(true);

	// Les operandes contiennent des regles de type Structure
	GetFirstOperand()->SetType(KWType::Unknown);
	GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);

	// Nombre totale de parties
	nTotalPartNumber = 0;
}

KWDRPartition::~KWDRPartition() {}

void KWDRPartition::AddUnivariatePartitionOperand(KWDRUnivariatePartition* univariatePartitionRule)
{
	KWDerivationRuleOperand* operand;

	require(univariatePartitionRule != NULL);

	// Ajout d'un operande base sur la regle en parametre
	operand = new KWDerivationRuleOperand;
	operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
	operand->SetType(KWType::Structure);
	operand->SetDerivationRule(univariatePartitionRule);
	operand->SetStructureName(univariatePartitionRule->GetStructureName());
	AddOperand(operand);
}

int KWDRPartition::ComputeUncheckedTotalPartNumber() const
{
	int nUncheckedTotalPartNumber;
	int nPartNumber;
	int nOperand;

	// Calcul du nombre de cellules de la partition
	nUncheckedTotalPartNumber = 1;
	for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
	{
		nPartNumber = GetUncheckedAttributePartNumberAt(nOperand);
		nUncheckedTotalPartNumber *= nPartNumber;
	}
	return nUncheckedTotalPartNumber;
}

KWDerivationRule* KWDRPartition::Create() const
{
	return new KWDRPartition;
}

Object* KWDRPartition::ComputeStructureResult(const KWObject* kwoObject) const
{
	require(Check());
	require(IsCompiled());
	return (Object*)this;
}

void KWDRPartition::Compile(KWClass* kwcOwnerClass)
{
	int nPartNumber;
	int nOperand;

	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Calcul du nombre de cellules de la partition
	nTotalPartNumber = 1;
	for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
	{
		nPartNumber =
		    cast(KWDRUnivariatePartition*, GetOperandAt(nOperand)->GetDerivationRule())->GetPartNumber();
		nTotalPartNumber *= nPartNumber;
	}
}

boolean KWDRPartition::CheckOperandsFamily(const KWDerivationRule* ruleFamily) const
{
	boolean bOk = true;
	KWDRIntervalBounds intervalBoundsRule;
	KWDRContinuousValueSet continuousValueSetRule;
	KWDRValueGroups valueGroupsRule;
	KWDRSymbolValueSet symbolValueSetRule;
	int nOperand;
	KWDerivationRuleOperand* operand;
	int nPartNumber;
	int nTotalCellNumber;
	double dTotalCellNumber;
	ALString sTmp;

	require(ruleFamily != NULL);
	require(ruleFamily->CheckDefinition());

	// Verification d'un nombre d'operande minimal
	if (GetOperandNumber() < 1)
	{
		bOk = false;
		AddError("The number of operands should be at least 1");
	}

	// Verification du type de structure de toutes les operandes
	for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
	{
		operand = GetOperandAt(nOperand);

		// Verification du type Structure
		if (operand->GetType() != KWType::Structure)
		{
			bOk = false;
			AddError(sTmp + "Operand " + IntToString(nOperand + 1) + " must be of type " +
				 KWType::ToString(KWType::Structure));
			break;
		}
		// Verification de la presence d'une regle de derivation
		else if (operand->GetDerivationRule() == NULL)
		{
			bOk = false;
			AddError(sTmp + "Operand " + IntToString(nOperand + 1) +
				 " must be produced by a derivation rule");
			break;
		}
	}

	// Verification du type de structure des operandes de partitionnement univarie
	if (bOk)
	{
		for (nOperand = 0; nOperand < GetOperandNumber(); nOperand++)
		{
			operand = GetOperandAt(nOperand);
			assert(operand->GetDerivationRule() != NULL);

			// Verification d'une structure de partition univariee
			if (operand->GetStructureName() != intervalBoundsRule.GetName() and
			    operand->GetStructureName() != valueGroupsRule.GetName() and
			    operand->GetStructureName() != continuousValueSetRule.GetName() and
			    operand->GetStructureName() != symbolValueSetRule.GetName())
			{
				bOk = false;
				AddError(sTmp + "Incorrect structure(" + operand->GetStructureName() +
					 ") for operand " + IntToString(nOperand + 1) + " (must be " +
					 intervalBoundsRule.GetName() + " or " + valueGroupsRule.GetName() + " or " +
					 continuousValueSetRule.GetName() + " or " + symbolValueSetRule.GetName() +
					 ")");
				break;
			}
		}
	}

	// Calcul du nombre total de cellules a partir des partitions univariees
	if (bOk)
	{
		nTotalCellNumber = 1;
		dTotalCellNumber = 1;
		for (nOperand = 0; nOperand < GetOperandNumber() - 1; nOperand++)
		{
			nPartNumber = cast(KWDRUnivariatePartition*, GetOperandAt(nOperand)->GetDerivationRule())
					  ->GetPartNumber();
			nTotalCellNumber *= nPartNumber;
			dTotalCellNumber *= nPartNumber;
		}

		// Test si on ne depasse pas la capacite des entiers
		if (dTotalCellNumber >= INT_MAX)
		{
			bOk = false;
			AddError(sTmp + "Total cell number too large (" + DoubleToString(dTotalCellNumber) + ")");
		}
	}
	return bOk;
}

boolean KWDRPartition::SilentCheckUnivariatePartitionOperand(const KWDerivationRuleOperand* operand) const
{
	boolean bOk;
	KWDRIntervalBounds intervalBoundsRule;
	KWDRContinuousValueSet continuousValueSetRule;
	KWDRValueGroups valueGroupsRule;
	KWDRSymbolValueSet symbolValueSetRule;

	require(operand != NULL);

	bOk = operand->GetType() == KWType::Structure and operand->GetDerivationRule() != NULL and
	      (operand->GetStructureName() != valueGroupsRule.GetName() or
	       operand->GetStructureName() != continuousValueSetRule.GetName() or
	       operand->GetStructureName() != symbolValueSetRule.GetName() or
	       operand->GetStructureName() != symbolValueSetRule.GetName());
	return bOk;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartition

KWDRTablePartition::KWDRTablePartition()
{
	SetName("TablePartition");
	SetLabel("Table partition");
	SetType(KWType::ObjectArrayValueBlock);
	SetMultipleScope(true);
	SetOperandNumber(3);
	SetVariableOperandNumber(true);
	GetFirstOperand()->SetType(KWType::ObjectArray);
	GetSecondOperand()->SetType(KWType::Structure);
	GetSecondOperand()->SetStructureName("Partition");

	// A partir du troisieme operande, le type est guide par les attributs de la grille
	// On doit neanmoins preciser un type valide pour la regle generique
	GetOperandAt(2)->SetType(KWType::Continuous);

	// Donnees issues de la compilation de la regle
	partitionRule = NULL;
}

KWDRTablePartition::~KWDRTablePartition()
{
	assert(oaSparseParts.OnlyNulls());
}

int KWDRTablePartition::GetVarKeyType() const
{
	return KWType::Continuous;
}

KWDerivationRule* KWDRTablePartition::Create() const
{
	return new KWDRTablePartition;
}

KWObjectArrayValueBlock*
KWDRTablePartition::ComputeObjectArrayValueBlockResult(const KWObject* kwoObject,
						       const KWIndexedKeyBlock* indexedKeyBlock) const

{
	KWObjectArrayValueBlock* valueBlockPartition;
	IntVector ivUsedSparseIndexes;
	ObjectArray* oaPart;
	ObjectArray* oaObjectArrayOperand;
	KWObject* kwoContainedObject;
	int nObject;
	int nCellIndex;
	int nOperand;
	int nAttributeIndex;
	int nPartIndex;
	int nFactor;
	Continuous cValue;
	Symbol sValue;
	int nNKey;
	int nSparseIndex;
	int i;

	require(IsCompiled());
	require(partitionRule != NULL);

	///////////////////////////////////////////////////////////////////////////////
	// Implementation aussi efficace que possible en tenant compte de la sparsite
	// des donnees
	// En entree:
	//  . N objects
	//  . P parties potentielles, correspondant a la taille de la partition (avec 1 <= NKey <= P)
	//  . S parties selectionnes (S <= P), chacune ayant un index sparse entre 0 et S-1
	// En sortie
	//  . un bloc sparse de parties, de taille T (potentiellement, T << S)
	//
	// Variables de travail
	//  . oaSparseParts: tableau de travail de taille P, memorisant pour chaque index sparse de partie
	//       la partie si elle a	 ete pas retenue;
	//       ce vecteur de taille P est preinitialise avec des NULL, qui servent d'indicateur de non utilisation
	//  . ivUsedSparseIndexes: vecteur des index sparse de partie utilises;
	//       ce vecteur est utilise temporairement, et atteint sa taille maximum T en fin de traitement
	//       quand toutes les parties utilises ont ete identifiees
	//
	// Algorithme: O(N + T log(T)) en temps, O(S) en memoire
	//  . analyse des objets: O(N)
	//     . calcul de la cle
	//     . calcul de l'index sparse
	//        . si necessaire, creation et memorisation d'une partie dans oaSparseParts
	//        . memorisation de l'index sparse dans ivUsedSparseIndexes
	// . tri des cles vue (ivUsedSparseIndexes): O(T log(T))
	// . creation du bloc sparse de partie: O(T)
	//    . a partir des index sparses tries (ivUsedSparseIndexes)
	//    . initialisation des paires (sparse index, partie), avec la partie retrouvee depuis oaSparseParts
	//    . reinitialisation de la variable oaSparseParts avec NULL pour la fois suivante

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Retaillage si necessaire du tableau sparse des parties
	if (oaSparseParts.GetSize() < indexedKeyBlock->GetKeyNumber())
		oaSparseParts.SetSize(indexedKeyBlock->GetKeyNumber());

	// Partitionnement de la table en fonction du parametre de partition
	oaObjectArrayOperand = GetFirstOperand()->GetObjectArrayValue(kwoObject);
	if (oaObjectArrayOperand != NULL and oaObjectArrayOperand->GetSize() > 0)
	{
		// Parcours des objets du container
		for (nObject = 0; nObject < oaObjectArrayOperand->GetSize(); nObject++)
		{
			kwoContainedObject = cast(KWObject*, oaObjectArrayOperand->GetAt(nObject));

			// Calcul de l'index dans la partition
			if (kwoContainedObject != NULL)
			{
				// Calcul de l'index de la cellule
				nCellIndex = 0;
				nFactor = 1;
				for (nOperand = 2; nOperand < GetOperandNumber(); nOperand++)
				{
					nAttributeIndex = nOperand - 2;

					// Recherche de l'index de la partie selon son type
					if (partitionRule->GetAttributeTypeAt(nAttributeIndex) == KWType::Continuous)
					{
						assert(GetOperandAt(nOperand)->GetType() == KWType::Continuous);
						cValue = GetOperandAt(nOperand)->GetContinuousValue(kwoContainedObject);
						nPartIndex = partitionRule->GetContinuousAttributePartIndexAt(
						    nAttributeIndex, cValue);
					}
					else
					{
						assert(partitionRule->GetAttributeTypeAt(nAttributeIndex) ==
						       KWType::Symbol);
						assert(GetOperandAt(nOperand)->GetType() == KWType::Symbol);
						sValue = GetOperandAt(nOperand)->GetSymbolValue(kwoContainedObject);
						nPartIndex = partitionRule->GetSymbolAttributePartIndexAt(
						    nAttributeIndex, sValue);
					}

					// Calcul de l'index de cellule
					nCellIndex += nFactor * nPartIndex;
					nFactor *= partitionRule->GetAttributePartNumberAt(nAttributeIndex);
				}

				// Memorisation d'une paire (index, tableau) si cle non deja rencontree
				nNKey = nCellIndex + 1;

				// Recherche de l'index sparse correspondant
				nSparseIndex = cast(KWIndexedNKeyBlock*, indexedKeyBlock)->GetKeyIndex(nNKey);

				// Test s'il faut memoriser la partie
				if (nSparseIndex != -1)
				{
					// Si necessaire, creation et memorisation d'une nouvelle partie
					oaPart = cast(ObjectArray*, oaSparseParts.GetAt(nSparseIndex));
					if (oaPart == NULL)
					{
						oaPart = new ObjectArray;
						oaSparseParts.SetAt(nSparseIndex, oaPart);

						// Memorisation de l'index sparse utilise
						ivUsedSparseIndexes.Add(nSparseIndex);
					}

					// Memorisation de l'objet dans sa partie
					oaPart->Add(kwoContainedObject);
				}
			}
		}
	}

	// Tri des index sparses de partie utilises
	ivUsedSparseIndexes.Sort();

	// Creation d'un vecteur sparse de partie de bonne taille
	valueBlockPartition = KWObjectArrayValueBlock::NewValueBlock(ivUsedSparseIndexes.GetSize());

	// Initialisation du vecteur sparse
	// On exploite ici le fait que les index sparse sont necessairment ordonnes de la
	// meme facon que les index de parties (NKey)
	for (i = 0; i < ivUsedSparseIndexes.GetSize(); i++)
	{
		nSparseIndex = ivUsedSparseIndexes.GetAt(i);

		// Memorisation de la paire (index sparse, partie)
		valueBlockPartition->SetAttributeSparseIndexAt(i, nSparseIndex);
		valueBlockPartition->SetValueAt(i, cast(ObjectArray*, oaSparseParts.GetAt(nSparseIndex)));
		assert(i == 0 or valueBlockPartition->GetAttributeSparseIndexAt(i) >
				     valueBlockPartition->GetAttributeSparseIndexAt(i - 1));

		// Reinitialisation du tableau de travail
		oaSparseParts.SetAt(nSparseIndex, NULL);
	}

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return valueBlockPartition;
}

void KWDRTablePartition::Compile(KWClass* kwcOwnerClass)
{
	// Appel de la methode ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Memorisation de la regle de partition
	partitionRule = cast(KWDRPartition*, GetSecondOperand()->GetReferencedDerivationRule(kwcOwnerClass));

	// Reinitialisation du tableau de travail sparse des parties
	// Ce dernier sera retaille dynamiquement lors de l'execution de la regle
	oaSparseParts.SetSize(0);
}

boolean KWDRTablePartition::CheckOperandsFamily(const KWDerivationRule* ruleFamily) const
{
	boolean bOk = true;
	KWDerivationRuleOperand* operand;
	int i;
	ALString sTmp;

	require(ruleFamily != NULL);
	require(ruleFamily->CheckDefinition());
	require(ruleFamily->GetOperandNumber() > 0);
	require(ruleFamily->GetFirstOperand()->GetType() == KWType::ObjectArray);
	require(ruleFamily->GetSecondOperand()->GetType() == KWType::Structure);
	require(ruleFamily->GetSecondOperand()->GetStructureName() == "Partition");

	// Il faut au moins un operande
	if (GetOperandNumber() < 3)
	{
		AddError("Missing operands");
		bOk = false;
	}

	// Verification du premier operande: table
	if (bOk)
	{
		operand = GetOperandAt(0);
		if (not operand->CheckFamily(ruleFamily->GetOperandAt(0)))
		{
			AddError(sTmp + "Type of first operand" +
				 " inconsistent with that of the registered rule (Table)");
			bOk = false;
		}
	}

	// Verification du second operande: partition
	if (bOk)
	{
		operand = GetOperandAt(1);
		if (not operand->CheckFamily(ruleFamily->GetOperandAt(1)))
		{
			AddError(sTmp + "Type of second operand" +
				 " inconsistent with that of the registered rule (Structure(Partition))");
			bOk = false;
		}
	}

	// Les deux premiers operandes sont deja verifies en precondition de la methode
	// Les autres operandes doivent etre de type Continuous ou Symbol
	if (bOk)
	{
		for (i = 2; i < GetOperandNumber(); i++)
		{
			operand = GetOperandAt(i);

			// Test de type simple
			if (not KWType::IsSimple(operand->GetType()))
			{
				AddError(sTmp + "Type of operand " + IntToString(i + 1) +
					 " inconsistent with that of the registered rule");
				bOk = false;
				break;
			}
		}
	}
	return bOk;
}

boolean KWDRTablePartition::CheckOperandsCompleteness(const KWClass* kwcOwnerClass) const
{
	boolean bOk = true;
	int nPartitionAttributeNumber;
	const KWDRPartition refPartition;
	KWDRPartition* referencedPartition;
	KWDRUnivariatePartition* univariatePartitionRule;
	int nPartitionAttributeType;
	KWDerivationRuleOperand* operand;
	int i;
	ALString sTmp;

	require(GetOperandNumber() > 0);
	require(kwcOwnerClass != NULL);

	// Methode ancetre
	bOk = KWDerivationRule::CheckOperandsCompleteness(kwcOwnerClass);

	// Verification que le premier operande reference une regle de type partition
	if (bOk)
		bOk = CheckReferencedDerivationRuleAt(1, kwcOwnerClass, refPartition.GetName());
	if (bOk)
	{
		referencedPartition = cast(KWDRPartition*, GetOperandAt(1)->GetReferencedDerivationRule(kwcOwnerClass));

		// Calcul du nombre d'attributs de la partition
		// La methode GetAttributeNumber n'est pas encore disponible (il faut une compilation)
		nPartitionAttributeNumber = referencedPartition->GetOperandNumber();

		// Les autres operandes doivent etre compatibles avec la partition
		if (GetOperandNumber() - 2 != nPartitionAttributeNumber)
		{
			AddError(sTmp +
				 "Wrong operand number with respect to the number of dimensions of the partition in "
				 "the second operand (" +
				 IntToString(nPartitionAttributeNumber) + " dimensions)");
			bOk = false;
		}
		else
		{
			// Verification du type des operandes
			for (i = 2; i < GetOperandNumber(); i++)
			{
				operand = GetOperandAt(i);

				// Recherche du type de l'attribut de la grille
				// La methode GetAttributeType n'est pas encore disponible (il faut une compilation)
				assert(referencedPartition->CheckReferencedDerivationRuleAt(i - 2, kwcOwnerClass, ""));
				univariatePartitionRule =
				    cast(KWDRUnivariatePartition*,
					 referencedPartition->GetOperandAt(i - 2)->GetReferencedDerivationRule(
					     kwcOwnerClass));
				check(univariatePartitionRule);
				nPartitionAttributeType = univariatePartitionRule->GetAttributeType();

				// Test de validite du type
				if (operand->GetType() != nPartitionAttributeType)
				{
					AddError(sTmp + "Type " + KWType::ToString(operand->GetType()) +
						 " of operand " + IntToString(i + 1) + " inconsistent with type " +
						 KWType::ToString(nPartitionAttributeType) + " of dimension " +
						 IntToString(i - 1) + " in the related partition");
					bOk = false;
					break;
				}
			}
		}
	}
	return bOk;
}

boolean KWDRTablePartition::IsSecondaryScopeOperand(int nOperandIndex) const
{
	return (nOperandIndex >= 2);
}

boolean KWDRTablePartition::CheckBlockAttributes(const KWClass* kwcOwnerClass,
						 const KWAttributeBlock* attributeBlock) const
{
	boolean bResult = true;
	KWDRPartition* referencedPartition;
	int nTotalPartNumber;
	KWAttribute* checkedAttribute;
	int nVarKey;
	ALString sTmp;

	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);
	require(attributeBlock != NULL);
	require(attributeBlock->GetFirstAttribute()->GetParentClass()->GetDomain() == kwcOwnerClass->GetDomain());
	require(CheckOperandsCompleteness(kwcOwnerClass));
	require(attributeBlock->GetVarKeyType() == KWType::Continuous);

	// Appel de la methode ancetre
	bResult = KWDerivationRule::CheckBlockAttributes(kwcOwnerClass, attributeBlock);

	// Analyse des cles du bloc d'attribut par rapport a la partition
	if (bResult)
	{
		// Acces a la partition
		referencedPartition = cast(KWDRPartition*, GetOperandAt(1)->GetReferencedDerivationRule(kwcOwnerClass));

		// Calcul du nombre total de partie de la partition
		nTotalPartNumber = referencedPartition->ComputeUncheckedTotalPartNumber();

		// Parcours des attributs du bloc pour verifier leur VarKey par rapport a la taille de la partition
		checkedAttribute = attributeBlock->GetFirstAttribute();
		while (checkedAttribute != NULL)
		{
			// VarKey de l'attribut
			nVarKey = attributeBlock->GetContinuousVarKey(checkedAttribute);
			assert(nVarKey >= 1);

			// Erreur si VarKey plus grand que le nombre de parties de la partition
			if (nVarKey > nTotalPartNumber)
			{
				// Messages d'erreur
				attributeBlock->AddError(
				    sTmp + "Variable " + checkedAttribute->GetName() +
				    +" has its VarKey=" + IntToString(nVarKey) +
				    " greater than the total number of parts (" + IntToString(nTotalPartNumber) +
				    ") in the input partition (second operand of rule " + GetName() + ")");
				bResult = false;
				break;
			}

			// Arret si derniere variable du bloc trouvee
			if (checkedAttribute == attributeBlock->GetLastAttribute())
				break;
			// Sinon, attribut suivant
			else
				checkedAttribute->GetParentClass()->GetNextAttribute(checkedAttribute);
		}
	}
	return bResult;
}

longint KWDRTablePartition::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRTablePartition) - sizeof(KWDerivationRule);
	lUsedMemory += oaSparseParts.GetUsedMemory();
	return lUsedMemory;
}

void KWDRTablePartition::InternalCompleteTypeInfo(const KWClass* kwcOwnerClass,
						  NumericKeyDictionary* nkdCompletedAttributes)
{
	require(kwcOwnerClass != NULL);
	require(nkdCompletedAttributes != NULL);

	// Appel de la methode ancetre
	KWDerivationRule::InternalCompleteTypeInfo(kwcOwnerClass, nkdCompletedAttributes);

	// Completion du tu type d'ObjectArray du bloc a partir du premier operande
	if (GetOperandNumber() > 1 and GetFirstOperand()->GetType() == KWType::ObjectArray)
	{
		SetObjectClassName(GetFirstOperand()->GetObjectClassName());
	}
}

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionStats

KWDRTablePartitionStats::KWDRTablePartitionStats()
{
	SetMultipleScope(true);
	SetOperandNumber(1);
	GetFirstOperand()->SetType(KWType::ObjectArrayValueBlock);

	// Regle permettant d'effectuer le calcul par partie
	tableStatsRule = NULL;

	// Donnees issues de la compilation de la regle
	tablePartitionIndexedKeyBlock = NULL;
}

KWDRTablePartitionStats::~KWDRTablePartitionStats()
{
	// Attention, on supprime tous les operande de la regle de stats par table
	// car ceux ci peuvent appartenir a la regle de stats par partition
	assert(tableStatsRule != NULL);
	tableStatsRule->RemoveAllOperands();
	delete tableStatsRule;
}

int KWDRTablePartitionStats::GetVarKeyType() const
{
	return KWType::Continuous;
}

void KWDRTablePartitionStats::Compile(KWClass* kwcOwnerClass)
{
	KWAttributeBlock* tablePartitionAttributeBlock;
	KWDRTablePartition* tablePartitionRule;
	int i;
	KWDerivationRuleOperand* operand;

	// Compilation de la classe ancetre
	KWDerivationRule::Compile(kwcOwnerClass);

	// Memorisation du bloc d'index associe a la partition
	tablePartitionAttributeBlock = GetFirstOperand()->GetOriginAttributeBlock();
	tablePartitionIndexedKeyBlock =
	    cast(const KWIndexedNKeyBlock*, tablePartitionAttributeBlock->GetLoadedAttributesIndexedKeyBlock());

	// Acces a la regle de partition de la table
	tablePartitionRule = cast(KWDRTablePartition*, GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass));
	assert(tablePartitionRule != NULL and tablePartitionRule->IsCompiled());

	// On parametre les operandes de la regle de stats par table a partir
	// de la regle de stats pour la partition
	check(tableStatsRule);
	tableStatsRule->RemoveAllOperands();
	tableStatsRule->AddOperand(tablePartitionRule->GetFirstOperand());
	for (i = 1; i < GetOperandNumber(); i++)
	{
		operand = GetOperandAt(i);
		tableStatsRule->AddOperand(operand);
	}
	assert(tableStatsRule->Check());
}

boolean KWDRTablePartitionStats::CheckFirstMultiScopeOperand() const
{
	return true;
}

KWClass* KWDRTablePartitionStats::LookupSecondaryScopeClass(const KWClass* kwcOwnerClass) const
{
	KWClass* secondaryScopeClass;
	KWDRTablePartition tablePartitionRuleRef;
	KWDerivationRule* firstOperandRule;
	KWDerivationRuleOperand* operandObjectArray;

	require(kwcOwnerClass != NULL);
	require(kwcOwnerClass->GetDomain() != NULL);

	// Recherche seulement si necessaire
	secondaryScopeClass = NULL;
	if (GetOperandNumber() > 0)
	{
		// Acces a la regle du premier operande
		firstOperandRule = GetFirstOperand()->GetReferencedDerivationRule(kwcOwnerClass);

		// Recherche de l'operande contenant les informations de la classe de l'ObjectArray
		operandObjectArray = NULL;
		if (firstOperandRule != NULL and firstOperandRule->GetName() == tablePartitionRuleRef.GetName() and
		    firstOperandRule->GetOperandNumber() > 0)
			operandObjectArray = firstOperandRule->GetFirstOperand();

		// Recherche de la classe
		if (operandObjectArray != NULL and KWType::IsRelation(operandObjectArray->GetType()) and
		    operandObjectArray->GetObjectClassName() != "")
			secondaryScopeClass =
			    kwcOwnerClass->GetDomain()->LookupClass(operandObjectArray->GetObjectClassName());
	}
	return secondaryScopeClass;
}

longint KWDRTablePartitionStats::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDerivationRule::GetUsedMemory();
	lUsedMemory += sizeof(KWDRTablePartitionStats) - sizeof(KWDerivationRule);
	assert(tableStatsRule != NULL);
	lUsedMemory += tableStatsRule->GetUsedMemory();
	return lUsedMemory;
}

void KWDRTablePartitionStats::InitializeFromTableStatsRule(KWDRTableStats* rule)
{
	const ALString sNamePrefix = "Table";
	const ALString sLabelSuffix = "in a table";
	int i;
	KWDerivationRuleOperand* operand;

	require(tableStatsRule == NULL);
	require(rule != NULL);
	require(rule->GetName().Find(sNamePrefix) >= 0);
	require(rule->GetLabel().Find(sLabelSuffix) >= 0);
	require(KWType::GetValueBlockType(rule->GetType()) == GetType());
	require(rule->GetOperandNumber() >= 1);
	require(rule->GetFirstOperand()->GetType() == KWType::ObjectArray);
	require(GetOperandNumber() == 1);
	require(GetFirstOperand()->GetType() == KWType::ObjectArrayValueBlock);

	// Memorisation de la regle
	tableStatsRule = rule;

	// Parametrage du nom et du libelle a partir de la regle de type TableStats
	SetName("TablePartition" + rule->GetName().Right(rule->GetName().GetLength() - sNamePrefix.GetLength()));
	SetLabel(rule->GetLabel().Left(rule->GetLabel().Find(sLabelSuffix)) + "by part in a partition");

	// On muni la regle de partition des meme operandes additionnels de calcul de stats
	// que la regle en parametre
	for (i = 1; i < rule->GetOperandNumber(); i++)
	{
		operand = rule->GetOperandAt(i);
		AddOperand(operand->Clone());
	}

	// Destruction des operandes de la regle de stats par table
	// qui seront alimentes a chaque compilation
	rule->DeleteAllOperands();
}

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionStatsContinuous

KWDRTablePartitionStatsContinuous::KWDRTablePartitionStatsContinuous()
{
	SetType(KWType::ContinuousValueBlock);
}

KWDRTablePartitionStatsContinuous::~KWDRTablePartitionStatsContinuous() {}

KWContinuousValueBlock*
KWDRTablePartitionStatsContinuous::ComputeContinuousValueBlockResult(const KWObject* kwoObject,
								     const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWContinuousValueBlock* resultValueBlock;
	KWContinuousValueSparseVector valueSparseVectorResults;
	KWObjectArrayValueBlock* valueBlockPartition;
	int n;
	int nPartSparseIndex;
	int nNKey;
	ObjectArray* oaPart;
	int nSparseIndex;
	Continuous cResult;

	require(IsCompiled());
	require(kwoObject != NULL);
	require(indexedKeyBlock != NULL);

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Calcul de la partition
	assert(GetFirstOperand()->GetOrigin() == KWDerivationRuleOperand::OriginAttribute);
	valueBlockPartition = GetFirstOperand()->GetObjectArrayValueBlock(kwoObject);

	// Parcours des tableaux d'objet de la partition
	for (n = 0; n < valueBlockPartition->GetValueNumber(); n++)
	{
		// Index sparse de la partie du bloc de partie
		nPartSparseIndex = valueBlockPartition->GetAttributeSparseIndexAt(n);

		// Cle de partie correspondante a cet index sparse
		nNKey = tablePartitionIndexedKeyBlock->GetKeyAt(nPartSparseIndex);

		// Traitement de la partie uniquement si necessaire
		nSparseIndex = cast(KWIndexedNKeyBlock*, indexedKeyBlock)->GetKeyIndex(nNKey);
		if (nSparseIndex != -1)
		{
			// Calcul de la stats associee a la partie
			oaPart = valueBlockPartition->GetValueAt(n);
			cResult = cast(KWDRTableStatsContinuous*, tableStatsRule)->ComputeContinuousStats(oaPart);

			// Memorisation si valeur utile
			if (cResult != GetValueBlockContinuousDefaultValue())
			{
				assert(valueSparseVectorResults.GetValueNumber() == 0 or
				       valueSparseVectorResults.GetSparseIndexAt(
					   valueSparseVectorResults.GetValueNumber() - 1) < nSparseIndex);
				valueSparseVectorResults.AddValueAt(nSparseIndex, cResult);
			}
		}
	}

	// Transformation en bloc de valeurs sparses (les index sparses sont dans le bon ordre)
	resultValueBlock = KWContinuousValueBlock::BuildBlockFromSparseValueVector(&valueSparseVectorResults);

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return resultValueBlock;
}

Continuous KWDRTablePartitionStatsContinuous::GetValueBlockContinuousDefaultValue() const
{
	return cast(KWDRTableStatsContinuous*, tableStatsRule)->GetDefaultContinuousStats();
}

longint KWDRTablePartitionStatsContinuous::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRTablePartitionStats::GetUsedMemory();
	lUsedMemory += sizeof(KWDRTablePartitionStatsContinuous) - sizeof(KWDRTablePartitionStats);
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionStatsSymbol

KWDRTablePartitionStatsSymbol::KWDRTablePartitionStatsSymbol()
{
	SetType(KWType::SymbolValueBlock);
}

KWDRTablePartitionStatsSymbol::~KWDRTablePartitionStatsSymbol() {}

KWSymbolValueBlock*
KWDRTablePartitionStatsSymbol::ComputeSymbolValueBlockResult(const KWObject* kwoObject,
							     const KWIndexedKeyBlock* indexedKeyBlock) const
{
	KWSymbolValueBlock* resultValueBlock;
	KWSymbolValueSparseVector valueSparseVectorResults;
	KWObjectArrayValueBlock* valueBlockPartition;
	int n;
	int nPartSparseIndex;
	int nNKey;
	ObjectArray* oaPart;
	int nSparseIndex;
	Symbol sResult;

	require(IsCompiled());
	require(kwoObject != NULL);
	require(indexedKeyBlock != NULL);

	// Evaluation des operandes secondaires de scope principal
	EvaluateMainScopeSecondaryOperands(kwoObject);

	// Calcul de la partition
	assert(GetFirstOperand()->GetOrigin() == KWDerivationRuleOperand::OriginAttribute);
	valueBlockPartition = GetFirstOperand()->GetObjectArrayValueBlock(kwoObject);

	// Parcours des tableaux d'objet de la partition
	for (n = 0; n < valueBlockPartition->GetValueNumber(); n++)
	{
		// Index sparse de la partie du bloc de partie
		nPartSparseIndex = valueBlockPartition->GetAttributeSparseIndexAt(n);

		// Cle de partie correspondante a cet index sparse
		nNKey = tablePartitionIndexedKeyBlock->GetKeyAt(nPartSparseIndex);

		// Traitement de la partie uniquement si necessaire
		nSparseIndex = cast(KWIndexedNKeyBlock*, indexedKeyBlock)->GetKeyIndex(nNKey);
		if (nSparseIndex != -1)
		{
			// Calcul de la stats associee a la partie
			oaPart = valueBlockPartition->GetValueAt(n);
			sResult = cast(KWDRTableStatsSymbol*, tableStatsRule)->ComputeSymbolStats(oaPart);

			// Memorisation si valeur utile
			if (sResult != GetValueBlockSymbolDefaultValue())
			{
				assert(valueSparseVectorResults.GetValueNumber() == 0 or
				       valueSparseVectorResults.GetSparseIndexAt(
					   valueSparseVectorResults.GetValueNumber() - 1) < nSparseIndex);
				valueSparseVectorResults.AddValueAt(nSparseIndex, sResult);
			}
		}
	}

	// Transformation en bloc de valeurs sparses (les index sparses sont dans le bon ordre)
	resultValueBlock = KWSymbolValueBlock::BuildBlockFromSparseValueVector(&valueSparseVectorResults);

	// Nettoyage des operandes secondaires de scope principal
	CleanMainScopeSecondaryOperands();
	return resultValueBlock;
}

Symbol& KWDRTablePartitionStatsSymbol::GetValueBlockSymbolDefaultValue() const
{
	static Symbol sDefaultValue;
	return sDefaultValue;
}

longint KWDRTablePartitionStatsSymbol::GetUsedMemory() const
{
	longint lUsedMemory;
	lUsedMemory = KWDRTablePartitionStats::GetUsedMemory();
	lUsedMemory += sizeof(KWDRTablePartitionStatsSymbol) - sizeof(KWDRTablePartitionStats);
	return lUsedMemory;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionCount

KWDRTablePartitionCount::KWDRTablePartitionCount()
{
	InitializeFromTableStatsRule(new KWDRTableCount);
}

KWDRTablePartitionCount::~KWDRTablePartitionCount() {}

KWDerivationRule* KWDRTablePartitionCount::Create() const
{
	return new KWDRTablePartitionCount;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionCountDistinct

KWDRTablePartitionCountDistinct::KWDRTablePartitionCountDistinct()
{
	InitializeFromTableStatsRule(new KWDRTableCountDistinct);
}

KWDRTablePartitionCountDistinct::~KWDRTablePartitionCountDistinct() {}

KWDerivationRule* KWDRTablePartitionCountDistinct::Create() const
{
	return new KWDRTablePartitionCountDistinct;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionEntropy

KWDRTablePartitionEntropy::KWDRTablePartitionEntropy()
{
	InitializeFromTableStatsRule(new KWDRTableEntropy);
}

KWDRTablePartitionEntropy::~KWDRTablePartitionEntropy() {}

KWDerivationRule* KWDRTablePartitionEntropy::Create() const
{
	return new KWDRTablePartitionEntropy;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionMode

KWDRTablePartitionMode::KWDRTablePartitionMode()
{
	InitializeFromTableStatsRule(new KWDRTableMode);
}

KWDRTablePartitionMode::~KWDRTablePartitionMode() {}

KWDerivationRule* KWDRTablePartitionMode::Create() const
{
	return new KWDRTablePartitionMode;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionModeAt

KWDRTablePartitionModeAt::KWDRTablePartitionModeAt()
{
	InitializeFromTableStatsRule(new KWDRTableModeAt);
}

KWDRTablePartitionModeAt::~KWDRTablePartitionModeAt() {}

KWDerivationRule* KWDRTablePartitionModeAt::Create() const
{
	return new KWDRTablePartitionModeAt;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionMean

KWDRTablePartitionMean::KWDRTablePartitionMean()
{
	InitializeFromTableStatsRule(new KWDRTableMean);
}

KWDRTablePartitionMean::~KWDRTablePartitionMean() {}

KWDerivationRule* KWDRTablePartitionMean::Create() const
{
	return new KWDRTablePartitionMean;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionStandardDeviation

KWDRTablePartitionStandardDeviation::KWDRTablePartitionStandardDeviation()
{
	InitializeFromTableStatsRule(new KWDRTableStandardDeviation);
}

KWDRTablePartitionStandardDeviation::~KWDRTablePartitionStandardDeviation() {}

KWDerivationRule* KWDRTablePartitionStandardDeviation::Create() const
{
	return new KWDRTablePartitionStandardDeviation;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionMedian

KWDRTablePartitionMedian::KWDRTablePartitionMedian()
{
	InitializeFromTableStatsRule(new KWDRTableMedian);
}

KWDRTablePartitionMedian::~KWDRTablePartitionMedian() {}

KWDerivationRule* KWDRTablePartitionMedian::Create() const
{
	return new KWDRTablePartitionMedian;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionMin

KWDRTablePartitionMin::KWDRTablePartitionMin()
{
	InitializeFromTableStatsRule(new KWDRTableMin);
}

KWDRTablePartitionMin::~KWDRTablePartitionMin() {}

KWDerivationRule* KWDRTablePartitionMin::Create() const
{
	return new KWDRTablePartitionMin;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionMax

KWDRTablePartitionMax::KWDRTablePartitionMax()
{
	InitializeFromTableStatsRule(new KWDRTableMax);
}

KWDRTablePartitionMax::~KWDRTablePartitionMax() {}

KWDerivationRule* KWDRTablePartitionMax::Create() const
{
	return new KWDRTablePartitionMax;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionSum

KWDRTablePartitionSum::KWDRTablePartitionSum()
{
	InitializeFromTableStatsRule(new KWDRTableSum);
}

KWDRTablePartitionSum::~KWDRTablePartitionSum() {}

KWDerivationRule* KWDRTablePartitionSum::Create() const
{
	return new KWDRTablePartitionSum;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionCountSum

KWDRTablePartitionCountSum::KWDRTablePartitionCountSum()
{
	InitializeFromTableStatsRule(new KWDRTableCountSum);
}

KWDRTablePartitionCountSum::~KWDRTablePartitionCountSum() {}

KWDerivationRule* KWDRTablePartitionCountSum::Create() const
{
	return new KWDRTablePartitionCountSum;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionTrend

KWDRTablePartitionTrend::KWDRTablePartitionTrend()
{
	InitializeFromTableStatsRule(new KWDRTableTrend);
}

KWDRTablePartitionTrend::~KWDRTablePartitionTrend() {}

KWDerivationRule* KWDRTablePartitionTrend::Create() const
{
	return new KWDRTablePartitionTrend;
}

///////////////////////////////////////////////////////////////
// Classe KWDRTablePartitionConcat

KWDRTablePartitionConcat::KWDRTablePartitionConcat()
{
	InitializeFromTableStatsRule(new KWDRTableConcat);
}

KWDRTablePartitionConcat::~KWDRTablePartitionConcat() {}

KWDerivationRule* KWDRTablePartitionConcat::Create() const
{
	return new KWDRTablePartitionConcat;
}
