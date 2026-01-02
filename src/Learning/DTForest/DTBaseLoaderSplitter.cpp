// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTBaseLoaderSplitter.h"
#include "DTDecisionTreeDatabaseObject.h"

DTBaseLoaderSplitter::DTBaseLoaderSplitter()
{
	databaseloaderOrigine = NULL;
	// databaseloaderOutOfBag = NULL;
}

DTBaseLoaderSplitter::~DTBaseLoaderSplitter()
{
	CleanDaughterBaseloader();
}

DTBaseLoader* DTBaseLoaderSplitter::GetOrigineBaseLoader() const
{
	return databaseloaderOrigine;
}

void DTBaseLoaderSplitter::SetOrigineBaseLoader(DTBaseLoader* db)
{
	databaseloaderOrigine = db;
}

// DTBaseLoader* DTBaseLoaderSplitter::GetdatabaseloaderOutOfBag() const
//{
//	return databaseloaderOutOfBag;
// }
//
// void DTBaseLoaderSplitter::SetdatabaseloaderOutOfBag(DTBaseLoader* db)
//{
//	databaseloaderOutOfBag = db;
// }
ObjectArray* DTBaseLoaderSplitter::GetDaughterBaseloader()
{
	return &oaTrainDaughterBaseLoader;
}

int DTBaseLoaderSplitter::GetDaughterBaseloaderNumber()
{
	return oaTrainDaughterBaseLoader.GetSize();
}

DTBaseLoader* DTBaseLoaderSplitter::GetDaughterBaseloaderAt(int i)
{
	require(i >= 0);
	require(i < oaTrainDaughterBaseLoader.GetSize());

	return (DTBaseLoader*)oaTrainDaughterBaseLoader.GetAt(i);
}

boolean DTBaseLoaderSplitter::CreateDaughterBaseloaderFromSplitAttribute(KWAttributeStats* splitAttributeStats,
									 KWLearningSpec* learningSpec)
{
	require(splitAttributeStats != NULL);
	require(databaseloaderOrigine != NULL);
	require(databaseloaderOrigine->GetDatabaseObjects() != NULL);

	// ObjectArray oaOutOfBagDaughterBaseLoader;

	// Tableau de pointeurs des DTBaseLoader obtenues apres application de la regle de derivation
	// Les DTBaseLoader creees appartiennent a l'appelant
	ObjectArray oaTrainDaughterObjects;
	ObjectArray oaTrainDaughterTargetVector;
	ObjectArray oaTrainDaughterTargetTupleTable;
	ObjectArray oaTrainDaughterTableLoader;

	DTBaseLoader* db;
	KWTupleTableLoader* tlDaughter;
	ObjectArray* oaDaughter;
	SymbolVector* svDaughter;
	KWTupleTable* targetTupleTableDaughter;

	// Initialisation
	CleanDaughterBaseloader();

	// Extraction du nom de la base mere
	const ALString databaseloaderTrainName = databaseloaderOrigine->GetLearningSpec()->GetClass()->GetName();

	if (splitAttributeStats->GetPreparedDataGridStats()->GetAttributeNumber() == 1)
	{
		// a partir de learningEnv v8, les attributs a level nul ne sont plus prepares. Le seul attribut prepare
		// correspond ici a l'attribut cible
		// NVDELL AddWarning("CreateDaughterDatabasesFromSplitAttribute :
		// GetPreparedDataGridStats()->GetAttributeNumber() == 1");
		return true;
	}

	// Extraction de l'attribut de partitionnement
	const KWDGSAttributePartition* splitAttributePartition =
	    cast(KWDGSAttributePartition*, splitAttributeStats->GetPreparedDataGridStats()->GetAttributeAt(0));

	// Extraction du nombre de parties de l'attribut de partitionnement
	const int nPartNumber = splitAttributePartition->GetPartNumber();

	const KWAttribute* nativeAttribute =
	    learningSpec->GetClass()->LookupAttribute(splitAttributeStats->GetAttributeName());
	require(nativeAttribute != NULL);

	// Calcul de l'index de chargement de l'attribut natif
	const KWLoadIndex nLoadIndex = nativeAttribute->GetLoadIndex();

	// creation de 2 bases de donnees filles pour chaque intervalle de la partition
	for (int nPartIndex = 0; nPartIndex < nPartNumber; nPartIndex++)
	{
		db = new DTBaseLoader;
		oaTrainDaughterBaseLoader.Add(db);
		oaDaughter = new ObjectArray;
		oaTrainDaughterObjects.Add(oaDaughter);
		svDaughter = new SymbolVector;
		oaTrainDaughterTargetVector.Add(svDaughter);
	}

	// Cas du partitionnement d'un attribut Continuous
	if (splitAttributePartition->GetAttributeType() == KWType::Continuous)
	{
		// Parcours des objets de la base mere de Train
		for (int i = 0; i < databaseloaderOrigine->GetDatabaseObjects()->GetSize(); i++)
		{
			// Extraction de l'objet courant
			KWObject* kwo = cast(KWObject*, databaseloaderOrigine->GetDatabaseObjects()->GetAt(i));
			// Object * o = databaseObjects->Lookup(kwo);
			// assert(o != NULL);
			// DTDecisionTreeDatabaseObject * dbo = cast(DTDecisionTreeDatabaseObject *, o);
			// if (dbo->GetFrequency() == 0)
			//	continue;

			// Indice de l'intervalle de l'objet
			const int nIndex =
			    splitAttributePartition->ComputeContinuousPartIndex(kwo->GetContinuousValueAt(nLoadIndex));
			assert(nIndex >= 0 && nIndex < nPartNumber);

			// Ajout de l'objet dans sa DTBaseLoader fille
			oaDaughter = cast(ObjectArray*, oaTrainDaughterObjects.GetAt(nIndex));
			oaDaughter->Add(kwo);
			svDaughter = cast(SymbolVector*, oaTrainDaughterTargetVector.GetAt(nIndex));
			svDaughter->Add(
			    databaseloaderOrigine->GetTupleLoader()->GetInputExtraAttributeSymbolValues()->GetAt(i));
		}

		for (int nPartIndex = 0; nPartIndex < nPartNumber; nPartIndex++)
		{
			db = cast(DTBaseLoader*, oaTrainDaughterBaseLoader.GetAt(nPartIndex));
			oaDaughter = cast(ObjectArray*, oaTrainDaughterObjects.GetAt(nPartIndex));
			svDaughter = cast(SymbolVector*, oaTrainDaughterTargetVector.GetAt(nPartIndex));
			tlDaughter = new KWTupleTableLoader;
			tlDaughter->SetCheckDatabaseObjectClass(
			    databaseloaderOrigine->GetTupleLoader()->GetCheckDatabaseObjectClass());
			tlDaughter->SetInputClass(databaseloaderOrigine->GetLearningSpec()->GetClass());
			tlDaughter->SetInputExtraAttributeName(
			    databaseloaderOrigine->GetLearningSpec()->GetTargetAttributeName());
			tlDaughter->SetInputExtraAttributeType(
			    databaseloaderOrigine->GetLearningSpec()->GetTargetAttributeType());

			targetTupleTableDaughter = new KWTupleTable;
			db->LoadTupleTableFromSymbolValues(
			    databaseloaderOrigine->GetLearningSpec()->GetClass(),
			    databaseloaderOrigine->GetLearningSpec()->GetTargetAttributeName(), svDaughter,
			    targetTupleTableDaughter);
			tlDaughter->SetInputExtraAttributeTupleTable(targetTupleTableDaughter);

			if (tlDaughter->GetInputExtraAttributeType() == KWType::Continuous)
			{
				require(false);
			}
			else if (tlDaughter->GetInputExtraAttributeType() == KWType::Symbol)
			{
				assert(svDaughter->GetSize() ==
				       tlDaughter->GetInputExtraAttributeTupleTable()->GetTotalFrequency());
				tlDaughter->SetInputExtraAttributeSymbolValues(svDaughter);
			}

			db->Initialize(databaseloaderOrigine->GetLearningSpec(), tlDaughter, oaDaughter);
			// db->Write(cout);
		}

		return true;
	}

	// Cas d'un attribut Symbol
	else
	{
		// Parcours des objets de la base mere de Train

		for (int i = 0; i < databaseloaderOrigine->GetDatabaseObjects()->GetSize(); i++)
		{
			// Extraction de l'objet courant
			KWObject* kwo = cast(KWObject*, databaseloaderOrigine->GetDatabaseObjects()->GetAt(i));
			// Object * o = databaseObjects->Lookup(kwo);
			// assert(o != NULL);
			// DTDecisionTreeDatabaseObject * dbo = cast(DTDecisionTreeDatabaseObject *, o);
			// if (dbo->GetFrequency() == 0)
			//	continue;

			// Indice de l'intervalle de l'objet
			const int nIndex =
			    splitAttributePartition->ComputeSymbolPartIndex(kwo->GetSymbolValueAt(nLoadIndex));
			assert(nIndex >= 0 && nIndex < nPartNumber);

			// Ajout de l'objet dans sa DTBaseLoader fille
			oaDaughter = cast(ObjectArray*, oaTrainDaughterObjects.GetAt(nIndex));
			oaDaughter->Add(kwo);
			svDaughter = cast(SymbolVector*, oaTrainDaughterTargetVector.GetAt(nIndex));
			svDaughter->Add(
			    databaseloaderOrigine->GetTupleLoader()->GetInputExtraAttributeSymbolValues()->GetAt(i));
		}

		for (int nPartIndex = 0; nPartIndex < nPartNumber; nPartIndex++)
		{
			db = cast(DTBaseLoader*, oaTrainDaughterBaseLoader.GetAt(nPartIndex));
			oaDaughter = cast(ObjectArray*, oaTrainDaughterObjects.GetAt(nPartIndex));
			svDaughter = cast(SymbolVector*, oaTrainDaughterTargetVector.GetAt(nPartIndex));
			tlDaughter = new KWTupleTableLoader;
			tlDaughter->SetCheckDatabaseObjectClass(
			    databaseloaderOrigine->GetTupleLoader()->GetCheckDatabaseObjectClass());
			tlDaughter->SetInputClass(databaseloaderOrigine->GetLearningSpec()->GetClass());
			tlDaughter->SetInputExtraAttributeName(
			    databaseloaderOrigine->GetLearningSpec()->GetTargetAttributeName());
			tlDaughter->SetInputExtraAttributeType(
			    databaseloaderOrigine->GetLearningSpec()->GetTargetAttributeType());

			targetTupleTableDaughter = new KWTupleTable;
			db->LoadTupleTableFromSymbolValues(
			    databaseloaderOrigine->GetLearningSpec()->GetClass(),
			    databaseloaderOrigine->GetLearningSpec()->GetTargetAttributeName(), svDaughter,
			    targetTupleTableDaughter);
			tlDaughter->SetInputExtraAttributeTupleTable(targetTupleTableDaughter);

			if (tlDaughter->GetInputExtraAttributeType() == KWType::Continuous)
			{
				require(false);
			}
			else if (tlDaughter->GetInputExtraAttributeType() == KWType::Symbol)
			{
				assert(svDaughter->GetSize() ==
				       tlDaughter->GetInputExtraAttributeTupleTable()->GetTotalFrequency());
				tlDaughter->SetInputExtraAttributeSymbolValues(svDaughter);
			}

			db->Initialize(databaseloaderOrigine->GetLearningSpec(), tlDaughter, oaDaughter);
		}

		return true;
	}
	// Cas non Continuous non Symbol
	return false;
}

void DTBaseLoaderSplitter::CleanDaughterBaseloader()
{
	int nDatabaseIndex;
	DTBaseLoader* db;

	for (nDatabaseIndex = 0; nDatabaseIndex < oaTrainDaughterBaseLoader.GetSize(); nDatabaseIndex++)
	{
		db = cast(DTBaseLoader*, oaTrainDaughterBaseLoader.GetAt(nDatabaseIndex));
		db->DeleteAll();
		delete db;
	}

	oaTrainDaughterBaseLoader.RemoveAll();
	// oaTrainDaughterObjects.RemoveAll();
	// oaTrainDaughterTargetVector.RemoveAll();
}
