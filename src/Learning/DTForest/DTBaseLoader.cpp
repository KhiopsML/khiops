// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTBaseLoader.h"

DTBaseLoader::DTBaseLoader()
{
	tupleTableLoader = NULL;
	database = NULL;
	learningSpec = NULL;
}

void DTBaseLoader::Initialize(KWLearningSpec* lSpec, KWTupleTableLoader* maintupleTableLoader, ObjectArray* oadb)
{
	require(lSpec != NULL);
	require(maintupleTableLoader != NULL);
	require(oadb != NULL);
	require(maintupleTableLoader->GetInputExtraAttributeSymbolValues() != NULL);

	learningSpec = lSpec;
	tupleTableLoader = maintupleTableLoader;
	database = oadb;

	assert(Check());
}

boolean DTBaseLoader::Check()
{
	boolean bOk = false;

	if (tupleTableLoader == NULL && database == NULL)
		return true;

	if (tupleTableLoader != NULL && database != NULL)
		if (tupleTableLoader->GetInputExtraAttributeSymbolValues()->GetSize() == database->GetSize())
			bOk = true;

	return bOk;
}

DTBaseLoader::~DTBaseLoader() {}

void DTBaseLoader::DeleteAll()
{
	tupleTableLoader->DeleteExtraAttributeInputs();
	delete tupleTableLoader;
	delete database;

	tupleTableLoader = NULL;
	database = NULL;
	learningSpec = NULL;
}

KWTupleTableLoader* DTBaseLoader::GetTupleLoader() const
{
	return tupleTableLoader;
}

ObjectArray* DTBaseLoader::GetDatabaseObjects() const
{
	return database;
}

KWLearningSpec* DTBaseLoader::GetLearningSpec() const
{
	return learningSpec;
}

void DTBaseLoader::BuildTrainOutOfBagBaseLoader(DTBaseLoader* blTrain, DTBaseLoader* blOutOfBag)
{
	require(blTrain != NULL);
	require(blOutOfBag != NULL);
	require(tupleTableLoader->GetInputExtraAttributeSymbolValues()->GetSize() == database->GetSize());
	Object* kwo;
	int i, newInstanceId;

	Object* obj;

	KWTupleTableLoader* tlTrain = new KWTupleTableLoader;
	KWTupleTableLoader* tlOutOfBag = new KWTupleTableLoader;
	ObjectArray* oaTrain = new ObjectArray;
	ObjectArray* oaOutOfBag = new ObjectArray;
	SymbolVector* svTrain = new SymbolVector;
	SymbolVector* svOutOfBag = new SymbolVector;
	KWTupleTable* targetTupleTableTrain = new KWTupleTable;
	KWTupleTable* targetTupleTableOutOfBag = new KWTupleTable;
	int instancesNumber = this->database->GetSize();
	NumericKeyDictionary nkdDatabaseObjects;

	// creation train

	for (i = 0; i < database->GetSize(); i++)
	{
		newInstanceId = RandomInt(instancesNumber - 1);
		kwo = database->GetAt(newInstanceId);

		assert(kwo != NULL);

		oaTrain->Add(kwo);
		nkdDatabaseObjects.SetAt(kwo, kwo);
		svTrain->Add(tupleTableLoader->GetInputExtraAttributeSymbolValues()->GetAt(newInstanceId));
	}

	// creation de out of bag
	for (i = 0; i < database->GetSize(); i++)
	{
		kwo = database->GetAt(i);
		assert(kwo != NULL);
		obj = nkdDatabaseObjects.Lookup(kwo);
		if (obj == NULL)
		{
			oaOutOfBag->Add(kwo);
			svOutOfBag->Add(tupleTableLoader->GetInputExtraAttributeSymbolValues()->GetAt(i));
		}
	}

	// Initialisation de la partie attribut cible du chargeur de tuples de l'esclave pour le train

	tlTrain->SetInputClass(learningSpec->GetClass());
	tlTrain->SetInputExtraAttributeName(learningSpec->GetTargetAttributeName());
	tlTrain->SetInputExtraAttributeType(learningSpec->GetTargetAttributeType());

	DTBaseLoader::LoadTupleTableFromSymbolValues(learningSpec->GetClass(), learningSpec->GetTargetAttributeName(),
						     svTrain, targetTupleTableTrain);
	tlTrain->SetInputExtraAttributeTupleTable(targetTupleTableTrain);

	if (tlTrain->GetInputExtraAttributeType() == KWType::Continuous)
	{
		require(false);
	}
	else if (tlTrain->GetInputExtraAttributeType() == KWType::Symbol)
	{
		assert(svTrain->GetSize() == tlTrain->GetInputExtraAttributeTupleTable()->GetTotalFrequency());
		tlTrain->SetInputExtraAttributeSymbolValues(svTrain);
	}

	blTrain->Initialize(learningSpec, tlTrain, oaTrain);

	// Initialisation de la partie attribut cible du chargeur de tuples de l'esclave pour outofBag
	tlOutOfBag->SetInputClass(learningSpec->GetClass());
	tlOutOfBag->SetInputExtraAttributeName(learningSpec->GetTargetAttributeName());
	tlOutOfBag->SetInputExtraAttributeType(learningSpec->GetTargetAttributeType());

	DTBaseLoader::LoadTupleTableFromSymbolValues(learningSpec->GetClass(),
						     this->learningSpec->GetTargetAttributeName(), svOutOfBag,
						     targetTupleTableOutOfBag);
	tlOutOfBag->SetInputExtraAttributeTupleTable(targetTupleTableOutOfBag);

	if (tlOutOfBag->GetInputExtraAttributeType() == KWType::Continuous)
	{
		require(false);
	}
	else if (tlOutOfBag->GetInputExtraAttributeType() == KWType::Symbol)
	{
		assert(svOutOfBag->GetSize() == tlOutOfBag->GetInputExtraAttributeTupleTable()->GetTotalFrequency());
		tlOutOfBag->SetInputExtraAttributeSymbolValues(svOutOfBag);
	}

	blOutOfBag->Initialize(learningSpec, tlOutOfBag, oaOutOfBag);

	// Alimentation d'une table de tuples univariee a partir d'un vecteur de valeur
	// void LoadTupleTableFromContinuousValues(const ALString& sAttributeName, const ContinuousVector*
	// cvInputValues, KWTupleTable* outputTupleTable); void LoadTupleTableFromSymbolValues(const ALString&
	// sAttributeName, const SymbolVector* svInputValues, KWTupleTable* outputTupleTable); blTrain->Write(cout);
	// blOutOfBag->Write(cout);
}

void DTBaseLoader::LoadTupleTableFromSymbolValues(KWClass* kwcInputClass, const ALString& sAttributeName,
						  const SymbolVector* svInputValues, KWTupleTable* outputTupleTable)
{
	int nValue;
	KWTuple* inputTuple;

	require(svInputValues != NULL);
	require(outputTupleTable != NULL);

	// Specification de la table de tuples
	// NB. la presence et le type de l'attribut cible ont deja ete verifie en amont
	outputTupleTable->CleanAll();
	outputTupleTable->AddAttribute(sAttributeName, KWType::Symbol);

	// Passage de la table de tuples en mode edition
	outputTupleTable->SetUpdateMode(true);
	inputTuple = outputTupleTable->GetInputTuple();

	// Alimentation a partir du vecteur de valeurs
	for (nValue = 0; nValue < svInputValues->GetSize(); nValue++)
	{
		inputTuple->SetSymbolAt(0, svInputValues->GetAt(nValue));
		outputTupleTable->UpdateWithInputTuple();
	}

	// Passage de la table de tuples en mode consultation
	outputTupleTable->SetUpdateMode(false);
}

void DTBaseLoader::Write(ostream& ost)
{
	assert(tupleTableLoader != NULL);
	assert(tupleTableLoader->GetInputExtraAttributeTupleTable() != NULL);

	// assert(targetAttributeLoadIndex.IsValid());//MB

	KWTupleTable* targettupletable = cast(KWTupleTable*, tupleTableLoader->GetInputExtraAttributeTupleTable());
	KWTuple* tuple;

	// NumericKeyDictionary * targetModalitiesCount = new NumericKeyDictionary;

	for (int i = 0; i < targettupletable->GetSize(); i++)
	{
		tuple = cast(KWTuple*, targettupletable->GetAt(i));
		// svReferenceTargetModalities->Add(tuple->GetSymbolAt(0));

		const Symbol sInstanceModality = tuple->GetSymbolAt(0);

		ost << "class : " << sInstanceModality.GetValue() << "Freq : " << tuple->GetFrequency() << endl;
		// targetModalitiesCount->SetAt(sInstanceModality.GetNumericKey(), modalityCount);
	}
}
