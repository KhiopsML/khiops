// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWBenchmarkSpec.h"

KWBenchmarkSpec::KWBenchmarkSpec()
{
	benchmarkClassSpec = new KWBenchmarkClassSpec;
	benchmarkDatabase = KWDatabase::CreateDefaultDatabaseTechnology();

	database = NULL;
	learningSpec = NULL;
	nLearningSpecFreshness = 0;
	nDatabaseInstanceNumber = 0;
	nSymbolAttributeNumber = 0;
	nContinuousAttributeNumber = 0;
	nTargetAttributeType = KWType::Unknown;
	nTargetValueNumber = 0;
	dTargetMajorityAccuracy = 0;
	dTargetMean = 0;
	dTargetStandardDeviation = 0;
}

KWBenchmarkSpec::~KWBenchmarkSpec()
{
	DeleteLearningSpec();
	delete benchmarkClassSpec;
	delete benchmarkDatabase;
}

KWBenchmarkClassSpec* KWBenchmarkSpec::GetBenchmarkClassSpec()
{
	return benchmarkClassSpec;
}

KWDatabase* KWBenchmarkSpec::GetBenchmarkDatabase()
{
	return benchmarkDatabase;
}

const ALString& KWBenchmarkSpec::GetClassFileName() const
{
	return benchmarkClassSpec->GetClassFileName();
}

const ALString& KWBenchmarkSpec::GetClassName() const
{
	return benchmarkClassSpec->GetClassName();
}

const ALString& KWBenchmarkSpec::GetTargetAttributeName() const
{
	return benchmarkClassSpec->GetTargetAttributeName();
}

const ALString& KWBenchmarkSpec::GetDatabaseName() const
{
	return benchmarkDatabase->GetDatabaseName();
}

void KWBenchmarkSpec::BuildLearningSpec()
{
	boolean bOk;
	KWClass* benchmarkClass;

	require(Check());

	// Reinitialisation des statistiques
	nDatabaseInstanceNumber = 0;
	nSymbolAttributeNumber = 0;
	nContinuousAttributeNumber = 0;
	nTargetAttributeType = KWType::Unknown;
	nTargetValueNumber = 0;
	dTargetMajorityAccuracy = 0;
	dTargetMean = 0;
	dTargetStandardDeviation = 0;

	// Nettoyage eventuel des resultats precedents
	DeleteLearningSpec();

	//   Chargement du fichier de dictionnaire et compilation
	KWClassDomain::GetCurrentDomain()->ReadFile(GetClassFileName());
	KWClassDomain::GetCurrentDomain()->Compile();

	// Creation et initialisation de la base de donnees
	database = benchmarkDatabase->Clone();

	// Creation et initialisation des specifications d'apprentissage
	learningSpec = new KWLearningSpec;
	benchmarkClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());
	if (benchmarkClass != NULL)
		learningSpec->SetClass(benchmarkClass);
	learningSpec->SetDatabase(database);
	learningSpec->SetTargetAttributeName(GetTargetAttributeName());

	// Verification des donnees par les objets eux-meme
	bOk = database->Check() and learningSpec->Check();

	// Si KO: destruction des resultats
	if (not bOk)
		DeleteLearningSpec();
}

boolean KWBenchmarkSpec::IsLearningSpecValid() const
{
	assert((database == NULL and learningSpec == NULL) or (database != NULL and learningSpec != NULL));
	assert(learningSpec == NULL or (learningSpec->Check() and learningSpec->GetDatabase() == database));
	return learningSpec != NULL;
}

KWLearningSpec* KWBenchmarkSpec::GetLearningSpec() const
{
	require(IsLearningSpecValid());
	return learningSpec;
}

KWDatabase* KWBenchmarkSpec::GetDatabase() const
{
	require(IsLearningSpecValid());
	return database;
}

void KWBenchmarkSpec::DeleteLearningSpec()
{
	if (learningSpec != NULL)
		delete learningSpec;
	if (database != NULL)
		delete database;
	learningSpec = NULL;
	database = NULL;
}

void KWBenchmarkSpec::ComputeBenchmarkStats()
{
	KWClass* learningClass;
	KWAttribute* attribute;
	KWTupleTableLoader tupleTableLoader;
	KWTupleTable targetTupleTable;
	KWDescriptiveContinuousStats* targetContinuousStats;
	KWDescriptiveSymbolStats* targetSymbolStats;
	KWObject* kwoObject;
	ALString sTmp;

	require(IsLearningSpecValid());

	// Statistiques sur la classe
	learningClass = GetLearningSpec()->GetClass();
	attribute = learningClass->GetHeadAttribute();
	nSymbolAttributeNumber = 0;
	nContinuousAttributeNumber = 0;
	while (attribute != NULL)
	{
		if (attribute->GetUsed() and attribute->GetName() != GetTargetAttributeName())
		{
			if (attribute->GetType() == KWType::Symbol)
				nSymbolAttributeNumber++;
			if (attribute->GetType() == KWType::Continuous)
				nContinuousAttributeNumber++;
		}
		learningClass->GetNextAttribute(attribute);
	}

	// Preparation de la classe pour ne charger que l'attribut cible
	learningClass->SetAllAttributesLoaded(false);
	attribute = learningClass->LookupAttribute(GetTargetAttributeName());
	if (attribute != NULL)
		attribute->SetLoaded(true);
	learningClass->GetDomain()->Compile();

	// Lecture de la base enregistrements par enregistrement, pour collecter les index physiques des records
	database->OpenForRead();
	ivPhysicalRecordIndexes.SetSize(0);
	if (database->IsOpenedForRead())
	{
		while (not database->IsEnd() and not database->IsError())
		{
			kwoObject = database->Read();

			// Memorisation de l'index de l'objet
			if (kwoObject != NULL)
			{
				database->GetObjects()->Add(kwoObject);
				ivPhysicalRecordIndexes.Add((int)database->GetLastReadMarkIndex());
			}
		}
		database->Close();
	}
	nDatabaseInstanceNumber = GetDatabase()->GetObjects()->GetSize();
	database->AddMessage(sTmp + "Read records: " + IntToString(nDatabaseInstanceNumber));

	// Calcul des statistiques sur l'attribut cible
	if (GetLearningSpec()->GetTargetAttributeType() == KWType::Continuous or
	    GetLearningSpec()->GetTargetAttributeType() == KWType::Symbol)
	{
		// Lecture des tuples pour l'attribut cible
		tupleTableLoader.SetInputClass(learningClass);
		tupleTableLoader.SetInputDatabaseObjects(GetDatabase()->GetObjects());
		tupleTableLoader.LoadUnivariate(GetTargetAttributeName(), &targetTupleTable);

		// Calcul des stats
		GetLearningSpec()->ComputeTargetStats(&targetTupleTable);
		targetTupleTable.CleanAll();
	}

	// Nettoyage de la base de donnees
	database->DeleteAll();

	// Collecte des statistiques
	nTargetAttributeType = GetLearningSpec()->GetTargetAttributeType();
	nTargetValueNumber = 0;
	dTargetMajorityAccuracy = 0;
	dTargetMean = 0;
	dTargetStandardDeviation = 0;
	if (GetLearningSpec()->IsTargetStatsComputed())
	{
		if (nTargetAttributeType == KWType::Symbol)
		{
			targetSymbolStats =
			    cast(KWDescriptiveSymbolStats*, GetLearningSpec()->GetTargetDescriptiveStats());
			nTargetValueNumber = targetSymbolStats->GetValueNumber();
			dTargetMajorityAccuracy = targetSymbolStats->GetModeFrequency();
			if (nDatabaseInstanceNumber > 0)
				dTargetMajorityAccuracy /= nDatabaseInstanceNumber;
		}
		else if (nTargetAttributeType == KWType::Continuous)
		{
			targetContinuousStats =
			    cast(KWDescriptiveContinuousStats*, GetLearningSpec()->GetTargetDescriptiveStats());
			dTargetMean = targetContinuousStats->GetMean();
			dTargetStandardDeviation = targetContinuousStats->GetStandardDeviation();
		}
	}

	// Restitution de l'etat initial de la classe
	learningClass->SetAllAttributesLoaded(true);
	learningClass->GetDomain()->Compile();
}

int KWBenchmarkSpec::GetDatabaseInstanceNumber() const
{
	return nDatabaseInstanceNumber;
}

int KWBenchmarkSpec::GetAttributeNumber() const
{
	return nSymbolAttributeNumber + nContinuousAttributeNumber;
}

int KWBenchmarkSpec::GetSymbolAttributeNumber() const
{
	return nSymbolAttributeNumber;
}

int KWBenchmarkSpec::GetContinuousAttributeNumber() const
{
	return nContinuousAttributeNumber;
}

int KWBenchmarkSpec::GetTargetAttributeType() const
{
	return nTargetAttributeType;
}

int KWBenchmarkSpec::GetTargetValueNumber() const
{
	require(GetTargetAttributeType() == KWType::Symbol);
	return nTargetValueNumber;
}

double KWBenchmarkSpec::GetTargetMajorityAccuracy() const
{
	require(GetTargetAttributeType() == KWType::Symbol);
	return dTargetMajorityAccuracy;
}

double KWBenchmarkSpec::GetTargetMean() const
{
	require(GetTargetAttributeType() == KWType::Continuous);
	return dTargetMean;
}

double KWBenchmarkSpec::GetTargetStandardDeviation() const
{
	require(GetTargetAttributeType() == KWType::Continuous);
	return dTargetStandardDeviation;
}

void KWBenchmarkSpec::WriteHeaderLineReport(ostream& ost)
{
	// Statistiques generales
	ost << "Database\tDictionary\tClass\t"
	    << "Variables\tCont. var.\tCat. var.\t"
	    << "Instances";

	// Statistiques selon type de prediction
	if (GetTargetAttributeType() == KWType::Symbol)
		ost << "\tValues\tMaj. acc.";
	else if (GetTargetAttributeType() == KWType::Continuous)
		ost << "\tMean\tStd. dev.";
}

void KWBenchmarkSpec::WriteLineReport(ostream& ost)
{
	// Statistiques generales
	ost << GetDatabaseName() << "\t" << GetClassName() << "\t" << GetTargetAttributeName() << "\t"
	    << GetAttributeNumber() << "\t" << GetContinuousAttributeNumber() << "\t" << GetSymbolAttributeNumber()
	    << "\t" << GetDatabaseInstanceNumber();

	// Statistiques selon type de prediction
	if (GetTargetAttributeType() == KWType::Symbol)
	{
		ost << "\t" << GetTargetValueNumber() << "\t" << GetTargetMajorityAccuracy();
	}
	else if (GetTargetAttributeType() == KWType::Continuous)
	{
		ost << "\t" << GetTargetMean() << "\t" << GetTargetStandardDeviation();
	}
}

void KWBenchmarkSpec::ComputeFoldIndexes(int nSeed, int nFoldNumber, boolean bStratified,
					 IntVector* ivResultFoldIndexes)
{
	KWClass* learningClass;
	KWDatabase* workingDatabase;
	KWAttribute* attribute;
	int i;
	KWObject* kwoObject;
	KWLoadIndex liTargetLoadIndex;
	KWSortableContinuousSymbol* indexedTargetValueRandom;
	ObjectArray oaIndexedTargetValues;
	Symbol sModalityRef;

	require(Check());
	require(IsLearningSpecValid());
	require(nFoldNumber >= 2);
	require(ivResultFoldIndexes != NULL);

	// Cas non stratifie (systematique si l'on n'est pas en classification)
	if (GetTargetAttributeType() != KWType::Symbol or not bStratified)
	{
		// Numerotation des parties (fold) de facon equilibree
		ivResultFoldIndexes->SetSize(GetDatabaseInstanceNumber());
		for (i = 0; i < ivResultFoldIndexes->GetSize(); i++)
			ivResultFoldIndexes->SetAt(i, i % nFoldNumber);

		// Permutation aleatoire des index d'instance dans les parties
		SetRandomSeed(nSeed);
		ivResultFoldIndexes->Shuffle();
	}
	// Cas stratifie
	else
	{
		assert(GetTargetAttributeType() == KWType::Symbol and bStratified);

		////////////////////////////////////////////////////////////////////
		// Recherche des index et valeurs cible de la base

		// Preparation de la classe pour ne charger que l'attribut cible
		learningClass = learningSpec->GetClass();
		learningClass->SetAllAttributesLoaded(false);
		attribute = learningClass->LookupAttribute(GetTargetAttributeName());
		if (attribute != NULL)
			attribute->SetLoaded(true);
		learningClass->GetDomain()->Compile();

		// Creation d'une base de travail
		assert(database == learningSpec->GetDatabase());
		workingDatabase = database->Clone();

		// Reinitialisation des eventuels marquages d'instances dans la base
		workingDatabase->GetMarkedInstances()->SetSize(0);

		// Lecture de la base d'origine et transfert vers la base de travail
		workingDatabase->SetVerboseMode(false);
		workingDatabase->ReadAll();
		assert(GetDatabaseInstanceNumber() == workingDatabase->GetObjects()->GetSize());
		workingDatabase->SetVerboseMode(true);

		// Memorisation dans un tableau des triplets (index, random value,valeur cible)
		SetRandomSeed(nSeed);
		liTargetLoadIndex = learningClass->LookupAttribute(GetTargetAttributeName())->GetLoadIndex();
		oaIndexedTargetValues.SetSize(workingDatabase->GetObjects()->GetSize());
		for (i = 0; i < workingDatabase->GetObjects()->GetSize(); i++)
		{
			kwoObject = cast(KWObject*, workingDatabase->GetObjects()->GetAt(i));

			// Memorisation de l'index, de la valeur cible de l'objet et d'une valeur aleatoire
			indexedTargetValueRandom = new KWSortableContinuousSymbol;
			oaIndexedTargetValues.SetAt(i, indexedTargetValueRandom);
			indexedTargetValueRandom->SetIndex(i);
			indexedTargetValueRandom->SetSymbol(kwoObject->GetSymbolValueAt(liTargetLoadIndex));
			indexedTargetValueRandom->SetSortValue(RandomDouble());
		}

		// Nettoyage
		workingDatabase->DeleteAll();
		delete workingDatabase;

		// Restitution de l'etat initial de la classe
		learningClass->SetAllAttributesLoaded(true);
		learningClass->GetDomain()->Compile();

		////////////////////////////////////////////////////////////////////
		// Calcul de l'echantillonage stratifie

		// Tri des instances par valeur cible, en tenant compte de la valeur aleatoire
		// (pour avoir un ordre aleatoire par valeur cible)
		oaIndexedTargetValues.SetCompareFunction(KWSortableContinuousSymbolCompareSymbolValue);
		oaIndexedTargetValues.Sort();

		// Numerotation des parties (fold) de facon stratifiee
		ivResultFoldIndexes->SetSize(GetDatabaseInstanceNumber());
		for (i = 0; i < ivResultFoldIndexes->GetSize(); i++)
		{
			indexedTargetValueRandom = cast(KWSortableContinuousSymbol*, oaIndexedTargetValues.GetAt(i));

			// Memorsation du fold
			ivResultFoldIndexes->SetAt(indexedTargetValueRandom->GetIndex(), i % nFoldNumber);
		}

		// Nettoyage
		oaIndexedTargetValues.DeleteAll();
	}
	ensure(ivResultFoldIndexes->GetSize() == ivPhysicalRecordIndexes.GetSize());
}

void KWBenchmarkSpec::ComputeDatabaseSelectedInstance(IntVector* ivFoldIndexes, int nFoldRefIndex, boolean bExcluding)
{
	int i;
	int nLastPhysicalRecordIndex;
	boolean bKeepInstance;

	require(Check());
	require(IsLearningSpecValid());
	require(database == learningSpec->GetDatabase());
	require(database != NULL);
	require(database->Check());
	require(ivFoldIndexes != NULL);
	require(nFoldRefIndex >= 0);
	require(ivPhysicalRecordIndexes.GetSize() == ivFoldIndexes->GetSize());

	// Parametrage de la taille du vecteur de marquages des instances a garder
	nLastPhysicalRecordIndex = 0;
	if (ivPhysicalRecordIndexes.GetSize() > 0)
		nLastPhysicalRecordIndex = ivPhysicalRecordIndexes.GetAt(ivPhysicalRecordIndexes.GetSize() - 1);
	database->GetMarkedInstances()->SetSize(nLastPhysicalRecordIndex + 1);
	database->GetMarkedInstances()->Initialize();

	// Parametrage des instances a garder dans la base
	for (i = 0; i < ivFoldIndexes->GetSize(); i++)
	{
		bKeepInstance = (bExcluding and ivFoldIndexes->GetAt(i) != nFoldRefIndex) or
				(not bExcluding and ivFoldIndexes->GetAt(i) == nFoldRefIndex);
		database->GetMarkedInstances()->SetAt(ivPhysicalRecordIndexes.GetAt(i), bKeepInstance);
	}
}

boolean KWBenchmarkSpec::Check() const
{
	boolean bOk = true;

	if (GetClassFileName() == "")
	{
		bOk = false;
		AddError("Missing dictionary file name");
	}
	if (GetClassName() == "")
	{
		bOk = false;
		AddError("Missing dictionary name");
	}
	if (GetDatabaseName() == "")
	{
		bOk = false;
		AddError("Missing database file name");
	}

	return bOk;
}

const ALString KWBenchmarkSpec::GetClassLabel() const
{
	return "Benchmark";
}

const ALString KWBenchmarkSpec::GetObjectLabel() const
{
	return GetClassName() + " " + GetTargetAttributeName() + " (" + GetDatabaseName() + ")";
}
