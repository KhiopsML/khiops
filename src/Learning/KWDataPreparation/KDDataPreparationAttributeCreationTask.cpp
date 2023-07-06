// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDDataPreparationAttributeCreationTask.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KDDataPreparationAttributeCreationTask

KDDataPreparationAttributeCreationTask::KDDataPreparationAttributeCreationTask()
{
	nMaxCreatedAttributeNumber = 0;
	classStats = NULL;
}

KDDataPreparationAttributeCreationTask::~KDDataPreparationAttributeCreationTask() {}

void KDDataPreparationAttributeCreationTask::SetMaxCreatedAttributeNumber(int nValue)
{
	require(nValue >= 0);
	nMaxCreatedAttributeNumber = nValue;
}

int KDDataPreparationAttributeCreationTask::GetMaxCreatedAttributeNumber() const
{
	return nMaxCreatedAttributeNumber;
}

void KDDataPreparationAttributeCreationTask::SetClassStats(KWClassStats* stats)
{
	classStats = stats;
}

KWClassStats* KDDataPreparationAttributeCreationTask::GetClassStats() const
{
	return classStats;
}

boolean KDDataPreparationAttributeCreationTask::CreatePreparedAttributes(KWLearningSpec* learningSpec,
									 KWTupleTableLoader* tupleTableLoader,
									 KWDataTableSliceSet* dataTableSliceSet,
									 ObjectDictionary* odInputAttributeStats,
									 ObjectArray* oaOutputAttributeStats)
{
	return false;
}

KWLearningReport* KDDataPreparationAttributeCreationTask::GetCreationReport()
{
	return NULL;
}

ALString KDDataPreparationAttributeCreationTask::GetReportPrefix() const
{
	return "Created";
}

void KDDataPreparationAttributeCreationTask::CopyAttributeCreationSpecFrom(
    const KDDataPreparationAttributeCreationTask* attributeCreationTask)
{
	require(attributeCreationTask != NULL);
	SetMaxCreatedAttributeNumber(attributeCreationTask->GetMaxCreatedAttributeNumber());
}

void KDDataPreparationAttributeCreationTask::SetGlobalCreationTask(
    KDDataPreparationAttributeCreationTask* attributeCreationTask)
{
	if (globalAttributeCreationTask != NULL)
		delete globalAttributeCreationTask;
	globalAttributeCreationTask = attributeCreationTask;
}

KDDataPreparationAttributeCreationTask* KDDataPreparationAttributeCreationTask::GetGlobalCreationTask()
{
	return globalAttributeCreationTask;
}

KDDataPreparationAttributeCreationTask* KDDataPreparationAttributeCreationTask::CloneGlobalCreationTask()
{
	KDDataPreparationAttributeCreationTask* attributeCreationTask;

	if (globalAttributeCreationTask == NULL)
		return NULL;
	else
	{
		attributeCreationTask =
		    cast(KDDataPreparationAttributeCreationTask*, globalAttributeCreationTask->Create());
		attributeCreationTask->CopyAttributeCreationSpecFrom(globalAttributeCreationTask);
		return attributeCreationTask;
	}
}

void KDDataPreparationAttributeCreationTask::SetConstructedAttributeCost(KWAttribute* attribute, double dCost) const
{
	require(attribute != NULL);
	require(dCost > 0);

	// Parametrage du cout de construction de l'attribut
	attribute->SetCost(dCost);

	// Meta-data sur le cout de construction
	attribute->SetMetaDataCost(dCost);
}

boolean KDDataPreparationAttributeCreationTask::CheckPreparedAttributes(KWLearningSpec* learningSpec,
									ObjectDictionary* odInputAttributeStats,
									ObjectArray* oaOutputAttributeStats) const
{
	boolean bOk = true;
	ObjectDictionary odOutputAttributeStats;
	KWAttributeStats* attributeStats;
	KWAttribute* attribute;
	int i;
	int nPreparedAttributeNumber;
	ALString sTmp;

	require(learningSpec != NULL);
	require(learningSpec->Check());
	require(odInputAttributeStats != NULL);
	require(oaOutputAttributeStats != NULL);

	// On commence par stocker les preparation en sortie dans un dictionnaire
	for (i = 0; i < oaOutputAttributeStats->GetSize(); i++)
	{
		attributeStats = cast(KWAttributeStats*, oaOutputAttributeStats->GetAt(i));
		assert(attributeStats->GetAttributeNumber() == 1);
		assert(odOutputAttributeStats.Lookup(attributeStats->GetAttributeName()) == NULL);
		odOutputAttributeStats.SetAt(attributeStats->GetAttributeName(), attributeStats);
	}

	// Parcours des attributs en entree de la classe
	nPreparedAttributeNumber = 0;
	for (i = 0; i < learningSpec->GetClass()->GetUsedAttributeNumber(); i++)
	{
		attribute = learningSpec->GetClass()->GetUsedAttributeAt(i);

		// On regarde uniquement les attribut concernes
		if (KWType::IsSimple(attribute->GetType()) and
		    attribute->GetName() != learningSpec->GetTargetAttributeName())
		{
			// Recherche d'une preparation dans les dictionnaire en entree et sortie
			// Erreur si presente deux fois
			if (odInputAttributeStats->Lookup(attribute->GetName()) != NULL and
			    odOutputAttributeStats.Lookup(attribute->GetName()) != NULL)
			{
				AddError("Variable " + attribute->GetName() +
					 " has data preparation both in input and out prepared variables");
				bOk = false;
				break;
			}
			// On le compte s'il est present dans un des deux containers
			// Il peut etre absent, si on ne veut pas l'utiliser pour la creation de nouveaux attributs
			else if (odInputAttributeStats->Lookup(attribute->GetName()) != NULL or
				 odOutputAttributeStats.Lookup(attribute->GetName()) != NULL)
				nPreparedAttributeNumber++;
		}
	}

	// Test s'il n'y a pas trop de preparations
	if (bOk and nPreparedAttributeNumber != odInputAttributeStats->GetCount() + odOutputAttributeStats.GetCount())
	{
		AddError(sTmp + "Number of used input variables (" + IntToString(nPreparedAttributeNumber) +
			 ") should be equal to number of prepared variables (" +
			 IntToString(odInputAttributeStats->GetCount() + odOutputAttributeStats.GetCount()) + ")");
		bOk = false;
	}
	return bOk;
}

KDDataPreparationAttributeCreationTask* KDDataPreparationAttributeCreationTask::globalAttributeCreationTask = NULL;

/////////////////////////////////////////////////////////////////////////////////
// Classe KDDPBivariateCrossProductsCreationTask

KDDPBivariateCrossProductsCreationTask::KDDPBivariateCrossProductsCreationTask() {}

KDDPBivariateCrossProductsCreationTask::~KDDPBivariateCrossProductsCreationTask() {}

void KDDPBivariateCrossProductsCreationTask::SetCreatedAttributePrefix(const ALString& sValue)
{
	sCreatedAttributePrefix = sValue;
}

const ALString& KDDPBivariateCrossProductsCreationTask::GetCreatedAttributePrefix() const
{
	return sCreatedAttributePrefix;
}

boolean KDDPBivariateCrossProductsCreationTask::CreatePreparedAttributes(KWLearningSpec* learningSpec,
									 KWTupleTableLoader* tupleTableLoader,
									 KWDataTableSliceSet* dataTableSliceSet,
									 ObjectDictionary* odInputAttributeStats,
									 ObjectArray* oaOutputAttributeStats)
{
	boolean bOk = true;
	int nAttribute;
	KWClass* kwcUpdatedClass;
	KWAttribute* createdAttribute;
	ObjectArray oaCreatedAttributes;
	ObjectArray oaInputAttributeStats;
	IntVector ivPairFirstIndexes;
	IntVector ivPairSecondIndexes;
	int i;
	int j;
	KWAttribute* attribute1;
	KWAttribute* attribute2;
	KWAttributeStats* attributeStats1;
	KWAttributeStats* attributeStats2;
	ObjectArray oaObjects;
	KWDataPreparationUnivariateTask dataPreparationUnivariateTask;
	double dAttributeCost;

	require(CheckInputParameters(learningSpec, tupleTableLoader->GetInputExtraAttributeTupleTable()));
	require(CheckPreparedAttributes(learningSpec, odInputAttributeStats, oaOutputAttributeStats));

	// Debut de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel(GetTaskName());
	AddSimpleMessage(GetTaskName() + ": " + IntToString(nMaxCreatedAttributeNumber));

	/////////////////////////////////////////////////////////////////////////////////////////////////
	// Identification des paires de variables a traiter

	// Collecte des preparations d'attributs dans un tableau
	odInputAttributeStats->ExportObjectArray(&oaInputAttributeStats);

	// Collecte d'un ensemple aleatoire de paires d'indices
	CollectRandomPairIndexes(oaInputAttributeStats.GetSize(), nMaxCreatedAttributeNumber, &ivPairFirstIndexes,
				 &ivPairSecondIndexes);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Creation des attributs et insertion de ces attributs dans la classe

	// Creation d'attributs
	for (nAttribute = 0; nAttribute < ivPairFirstIndexes.GetSize(); nAttribute++)
	{
		// Recherche des index de la paire d'attributs
		i = ivPairFirstIndexes.GetAt(nAttribute);
		j = ivPairSecondIndexes.GetAt(nAttribute);

		// Recherche des preparations de la paire
		attributeStats1 = cast(KWAttributeStats*, oaInputAttributeStats.GetAt(i));
		attributeStats2 = cast(KWAttributeStats*, oaInputAttributeStats.GetAt(j));

		// Recherche des attributs de la paire
		attribute1 = learningSpec->GetClass()->LookupAttribute(attributeStats1->GetAttributeName());
		attribute2 = learningSpec->GetClass()->LookupAttribute(attributeStats2->GetAttributeName());

		// Creation d'un nouvel attribut
		dAttributeCost =
		    learningSpec->GetSelectionCost() + attribute1->GetCost() + attribute2->GetCost() - log(2.0);
		createdAttribute =
		    CreateAttributeFrom(attribute1, attribute2, attributeStats1, attributeStats2, dAttributeCost);
		oaCreatedAttributes.Add(createdAttribute);
	}

	// On commence a passer tous les attributs de la classe en not loaded
	kwcUpdatedClass = learningSpec->GetClass();
	kwcUpdatedClass->SetAllAttributesLoaded(false);

	// Ajout des attributs dans la classe
	for (nAttribute = 0; nAttribute < oaCreatedAttributes.GetSize(); nAttribute++)
	{
		createdAttribute = cast(KWAttribute*, oaCreatedAttributes.GetAt(nAttribute));
		createdAttribute->SetName(kwcUpdatedClass->BuildAttributeName(createdAttribute->GetName()));
		createdAttribute->CompleteTypeInfo(kwcUpdatedClass);
		kwcUpdatedClass->InsertAttribute(createdAttribute);
	}

	// Compilation de la classe
	kwcUpdatedClass->Compile();

	/////////////////////////////////////////////////////////////////////////////////////////////
	// Lecture de la base pour preparer les attributs creees

	// Lecture des objets avec le sliceSet
	bOk = not TaskProgression::IsInterruptionRequested();
	if (bOk)
	{
		TaskProgression::DisplayMainLabel(GetTaskName() + ": Read data table");
		bOk = dataTableSliceSet->ReadAllObjectsWithClass(kwcUpdatedClass, &oaObjects);

		// Parametrage du chargeur de tuple
		tupleTableLoader->SetInputClass(kwcUpdatedClass);
		tupleTableLoader->SetInputDatabaseObjects(&oaObjects);
	}

	// Preparation des nouveaux attributs
	if (bOk)
	{
		TaskProgression::DisplayMainLabel(GetTaskName() + ": Variable preparation");
		bOk = dataPreparationUnivariateTask.BasicCollectPreparationStats(
		    learningSpec, tupleTableLoader, &oaCreatedAttributes, true, oaOutputAttributeStats);
	}

	// Destruction des objets
	oaObjects.DeleteAll();

	// On repasse tous les attributs de la classe en loaded
	kwcUpdatedClass = learningSpec->GetClass();
	kwcUpdatedClass->SetAllAttributesLoaded(true);
	kwcUpdatedClass->Compile();

	/////////////////////////////////////////////////////////////////////////////
	// Finalisation

	// Nettoyage si echec
	if (not bOk)
	{
		assert(oaOutputAttributeStats->GetSize() == 0);

		// Suppression des attributs de la classe
		for (nAttribute = 0; nAttribute < oaCreatedAttributes.GetSize(); nAttribute++)
		{
			createdAttribute = cast(KWAttribute*, oaCreatedAttributes.GetAt(nAttribute));
			kwcUpdatedClass->DeleteAttribute(createdAttribute->GetName());
		}
		oaCreatedAttributes.SetSize(0);

		// On recompile la classe
		kwcUpdatedClass->Compile();
	}

	// Message si erreur
	if (not bOk)
	{
		if (TaskProgression::IsInterruptionRequested())
			AddWarning("Interrupted by user");
		else
			AddError("Interrupted because of errors");
	}

	// Fin de tache
	TaskProgression::EndTask();

	ensure(CheckPreparedAttributes(learningSpec, odInputAttributeStats, oaOutputAttributeStats));
	ensure(bOk or oaCreatedAttributes.GetSize() == 0);
	ensure(oaCreatedAttributes.GetSize() == oaOutputAttributeStats->GetSize());
	return bOk;
}

void KDDPBivariateCrossProductsCreationTask::CopyAttributeCreationSpecFrom(
    const KDDataPreparationAttributeCreationTask* attributeCreationTask)
{
	// Appel de la methode ancetre
	KDDataPreparationAttributeCreationTask::CopyAttributeCreationSpecFrom(attributeCreationTask);

	// Recopie des attributs specifiques
	sCreatedAttributePrefix =
	    cast(KDDPBivariateCrossProductsCreationTask*, attributeCreationTask)->GetCreatedAttributePrefix();
}

ALString KDDPBivariateCrossProductsCreationTask::GetReportPrefix() const
{
	return "CrossProduct";
}

void KDDPBivariateCrossProductsCreationTask::CollectRandomPairIndexes(int nMaxIndex, int nMaxPairNumber,
								      IntVector* ivPairFirstIndexes,
								      IntVector* ivPairSecondIndexes) const
{
	const int nMaxPairNumberUpperBound = 100000;
	int nInitialRandomSeed;
	int nTotalPairNumber;
	IntVector ivPairIndexes;
	int i;
	int j;
	int nPair;
	int n;

	require(nMaxIndex >= 0);
	require(nMaxPairNumber >= 0);
	require(nMaxPairNumber <= nMaxPairNumberUpperBound);
	require(ivPairFirstIndexes != NULL);
	require(ivPairSecondIndexes != NULL);

	// Calcul du nombre total de paires
	if (nMaxIndex < sqrt(INT_MAX * 1.0))
		nTotalPairNumber = nMaxIndex * (nMaxIndex - 1) / 2;
	// On se limite aux index de paires sans probleme de depassement de INT_MAX
	else
		nTotalPairNumber = int(sqrt(INT_MAX * 1.0) * (sqrt(INT_MAX * 1.0) - 1) / 2);

	// Cas ou on prend toutes les paires
	if (nTotalPairNumber <= nMaxPairNumber)
	{
		ivPairFirstIndexes->SetSize(nTotalPairNumber);
		ivPairSecondIndexes->SetSize(nTotalPairNumber);
		nPair = 0;
		for (i = 0; i < nMaxIndex; i++)
		{
			for (j = i + 1; j < nMaxIndex; j++)
			{
				ivPairFirstIndexes->SetAt(nPair, i);
				ivPairSecondIndexes->SetAt(nPair, j);
				nPair++;
			}
		}
		assert(nPair == nTotalPairNumber);
	}
	// Cas ou on prend des paires au hasard (il y aura peut-etre des doublons)
	else
	{
		// Creation de paires d'index aleatoires
		nInitialRandomSeed = GetRandomSeed();
		ivPairIndexes.SetSize(min(nMaxPairNumberUpperBound * 2, nTotalPairNumber));
		nPair = 0;
		for (n = 0; n < ivPairIndexes.GetSize(); n++)
		{
			nPair += 1 + RandomInt((nTotalPairNumber - nPair) / (ivPairIndexes.GetSize() - n) - 1);
			ivPairIndexes.SetAt(n, nPair - 1);
		}
		ivPairIndexes.Shuffle();
		SetRandomSeed(nInitialRandomSeed);

		// On prend les premieres paires aleatoires
		ivPairFirstIndexes->SetSize(nMaxPairNumber);
		ivPairSecondIndexes->SetSize(nMaxPairNumber);
		for (n = 0; n < nMaxPairNumber; n++)
		{
			nPair = ivPairIndexes.GetAt(n);
			i = int(floor(sqrt(nPair * 2)));
			j = nPair - i * (i - 1) / 2;
			while (j >= i)
			{
				i++;
				j = nPair - i * (i - 1) / 2;
			}
			assert(i < ivPairIndexes.GetSize() - 1);
			assert(j >= 0);
			ivPairFirstIndexes->SetAt(n, i);
			ivPairSecondIndexes->SetAt(n, j);
		}
		assert(n == nMaxPairNumber);
	}
}

KWAttribute* KDDPBivariateCrossProductsCreationTask::CreateAttributeFrom(KWAttribute* attribute1,
									 KWAttribute* attribute2,
									 KWAttributeStats* attributeStats1,
									 KWAttributeStats* attributeStats2,
									 double dCost) const
{
	KWAttribute* attribute;
	KWDRConcat* concatRule;
	KWDerivationRuleOperand* operand;
	ALString sName1;
	ALString sName2;

	require(attribute1 != NULL);
	require(attribute2 != NULL);
	require(attributeStats1 != NULL);
	require(attributeStats2 != NULL);
	require(attributeStats1->GetAttributeName() == attribute1->GetName());
	require(attributeStats2->GetAttributeName() == attribute2->GetName());
	require(dCost > 0);

	// On cree l'attribut avec une regle de concatenation
	attribute = new KWAttribute;
	sName1 = attribute1->GetName().Left(min(attribute1->GetName().GetLength(), 30));
	sName1.TrimRight();
	sName2 = attribute2->GetName().Left(min(attribute2->GetName().GetLength(), 30));
	sName2.TrimRight();
	attribute->SetName(sCreatedAttributePrefix + sName1 + "_" + sName2);
	concatRule = new KWDRConcat;
	concatRule->DeleteAllOperands();
	attribute->SetDerivationRule(concatRule);
	attribute->SetType(concatRule->GetType());

	// Ajout d'un operande pour le premier attribut
	concatRule->AddOperand(CreateConcatOperandFrom(attribute1, attributeStats1));

	// Ajout d'un operande pour le separateur de champ
	operand = new KWDerivationRuleOperand;
	operand->SetType(KWType::Symbol);
	operand->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	operand->SetSymbolConstant("_");
	concatRule->AddOperand(operand);

	// Ajout d'un operande pour le second attribut
	concatRule->AddOperand(CreateConcatOperandFrom(attribute2, attributeStats2));

	// Parametrage du cout de construction de l'attribut
	SetConstructedAttributeCost(attribute, dCost);
	return attribute;
}

KWDerivationRuleOperand*
KDDPBivariateCrossProductsCreationTask::CreateConcatOperandFrom(KWAttribute* attribute,
								KWAttributeStats* attributeStats) const
{
	KWDerivationRuleOperand* operand;
	KWDRIntervalBounds* intervalBoundsRule;
	KWDRIntervalId* intervalIdRule;
	const int nIntervalNumber = 10;
	int nBound;
	const KWDescriptiveContinuousStats* descriptitiveContinuousStats;
	Continuous cRange;

	require(attribute != NULL);
	require(attributeStats != NULL);
	require(attributeStats->GetAttributeName() == attribute->GetName());
	require(KWType::IsSimple(attribute->GetType()));

	// Creation de l'operand
	operand = new KWDerivationRuleOperand;
	operand->SetType(KWType::Symbol);

	// Cas d'un attribut Symbol: on le prend tel quel
	if (attribute->GetType() == KWType::Symbol)
	{
		operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		operand->SetAttributeName(attribute->GetName());
	}
	// Cas d'un attribut Continuous: on en prend une discretisation non supervisee
	else
	{
		// Acces aux stats decsriptives
		descriptitiveContinuousStats =
		    cast(const KWDescriptiveContinuousStats*, attributeStats->GetDescriptiveStats());

		// Creation d'une regle de type intervalle
		intervalBoundsRule = new KWDRIntervalBounds;

		// Parametrage des bornes
		cRange = descriptitiveContinuousStats->GetMax() - descriptitiveContinuousStats->GetMin();
		if (cRange > 0)
		{
			intervalBoundsRule->SetIntervalBoundNumber(nIntervalNumber - 1);
			for (nBound = 0; nBound < intervalBoundsRule->GetIntervalBoundNumber(); nBound++)
			{
				intervalBoundsRule->SetIntervalBoundAt(nBound,
								       descriptitiveContinuousStats->GetMin() +
									   (nBound + 1) * cRange / nIntervalNumber);
			}
		}
		else
		{
			intervalBoundsRule->SetIntervalBoundNumber(1);
			intervalBoundsRule->SetIntervalBoundAt(0, descriptitiveContinuousStats->GetMax());
		}

		// Creation d'une regle pour indexer l'intervalle
		intervalIdRule = new KWDRIntervalId;
		intervalIdRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
		intervalIdRule->GetFirstOperand()->SetDerivationRule(intervalBoundsRule);
		intervalIdRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		intervalIdRule->GetSecondOperand()->SetAttributeName(attribute->GetName());

		// Parametrage de l'operande
		operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		operand->SetDerivationRule(intervalIdRule);
	}
	return operand;
}

const ALString KDDPBivariateCrossProductsCreationTask::GetTaskName() const
{
	return "Bivariate cross-product variable creation";
}

PLParallelTask* KDDPBivariateCrossProductsCreationTask::Create() const
{
	return new KDDPBivariateCrossProductsCreationTask;
}

boolean KDDPBivariateCrossProductsCreationTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	return false;
}

boolean KDDPBivariateCrossProductsCreationTask::MasterAggregateResults()
{
	return false;
}

boolean KDDPBivariateCrossProductsCreationTask::SlaveProcess()
{
	return false;
}
