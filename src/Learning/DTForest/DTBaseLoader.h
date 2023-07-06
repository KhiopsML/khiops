// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "Object.h"
#include "KWLearningSpec.h"
#include "KWTupleTableLoader.h"

class DTBaseLoader : public Object
{
public:
	DTBaseLoader();
	~DTBaseLoader();
	void Initialize(KWLearningSpec* lSpec, KWTupleTableLoader* maintupleTableLoader, ObjectArray* oadb);
	boolean Check();

	KWTupleTableLoader* GetTupleLoader() const;
	// void	SetTupleLoader(KWTupleTableLoader* tuple);
	ObjectArray* GetDatabaseObjects() const;
	// void	SetObjectArray(ObjectArray* oadb);
	KWLearningSpec* GetLearningSpec() const;
	// void	SetLearningSpec(KWLearningSpec* lSpec);

	// Ecriture d'un rapport abrege
	void Write(ostream& ost);

	void DeleteAll();

	void BuildTrainOutOfBagBaseLoader(DTBaseLoader* blTrain, DTBaseLoader* blOutOfBag);
	void LoadTupleTableFromSymbolValues(KWClass* kwcInputClass, const ALString& sAttributeName,
					    const SymbolVector* svInputValues, KWTupleTable* outputTupleTable);

protected:
	KWTupleTableLoader* tupleTableLoader;
	ObjectArray* database;
	KWLearningSpec* learningSpec;
};