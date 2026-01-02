// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWLoadIndex.h"

void KWLoadIndex::Test()
{
	KWLoadIndexVector livLoadIndexes;
	KWLoadIndex index0;
	KWLoadIndex index;
	int nIndex0;
	longint lIndex0;
	int nIndex;
	longint lIndex;
	Timer timer;
	int nMax = 1000000000;
	int nMaxAccess = 100000000;
	debug(nMaxAccess /= 10);
	int i;
	DoubleVector dvValues;
	double dValue;

	// Test des operateuurs de comparaison
	index.Reset();
	index0.SetDenseIndex(0);
	cout << index << " == " << index << ":" << (index == index) << endl;
	cout << index << " != " << index << ":" << (index != index) << endl;
	cout << index << " == " << index0 << ":" << (index == index0) << endl;
	cout << index << " != " << index0 << ":" << (index != index0) << endl;

	// Tests de base
	cout << "sizeof(KWLoadIndex) " << sizeof(KWLoadIndex) << endl;
	index.Reset();
	cout << "Full(-1) " << index << endl;
	index.SetDenseIndex(-1);
	index.SetSparseIndex(-1);
	cout << "(-1, -1) " << index << endl;
	index.SetDenseIndex(0);
	index.SetSparseIndex(-1);
	cout << "(0, -1) " << index << endl;
	index.SetDenseIndex(-1);
	index.SetSparseIndex(0);
	cout << "(-1, 0) " << index << endl;

	// Utilisation d'un vecteur d'index
	livLoadIndexes.SetSize(1);
	livLoadIndexes.SetAt(0, index);
	cout << "To vector of indexes: " << livLoadIndexes.GetAt(0) << endl;
	index.Reset();
	index = livLoadIndexes.GetAt(0);
	cout << "From vector of indexes: " << index << endl;
	index.Reset();
	cout << "Full(-1) " << index << endl;
	livLoadIndexes.SetAt(0, index);
	cout << "To vector of indexes: " << livLoadIndexes.GetAt(0) << endl;
	index.Reset();
	index = livLoadIndexes.GetAt(0);
	cout << "From vector of indexes: " << index << endl;

	// Performance sur des entiers
	cout << "Performance sur des entiers: " << flush;
	nIndex0 = 128;
	nIndex = 0;
	timer.Reset();
	timer.Start();
	for (i = 0; i < nMax; i++)
		nIndex = nIndex0;
	timer.Stop();
	cout << nIndex << " " << timer.GetElapsedTime() << endl;

	// Performance sur des entiers longs
	cout << "Performance sur des entiers longs: " << flush;
	lIndex0 = 1024;
	lIndex0 *= lIndex0;
	lIndex0 *= lIndex0;
	lIndex = 0;
	timer.Reset();
	timer.Start();
	for (i = 0; i < nMax; i++)
		lIndex = lIndex0;
	timer.Stop();
	cout << lIndex << " " << timer.GetElapsedTime() << endl;

	// Performance sur des LoadIndex
	cout << "Performance sur des LoadIndex: " << flush;
	index0.SetDenseIndex(256);
	index0.SetSparseIndex(1024);
	index.SetDenseIndex(0);
	timer.Reset();
	timer.Start();
	for (i = 0; i < nMax; i++)
		index = index0;
	timer.Stop();
	cout << index << " " << timer.GetElapsedTime() << endl;

	// Initialisation d'un vecteur de valeurs
	dvValues.SetSize(10000);
	for (i = 0; i < dvValues.GetSize(); i++)
		dvValues.SetAt(i, i);

	// Performance sur l'acces dense
	cout << "Performance sur l'acces dense: " << flush;
	index.SetDenseIndex(256);
	index.SetSparseIndex(-1);
	dValue = 0;
	timer.Reset();
	timer.Start();
	for (i = 0; i < nMaxAccess; i++)
	{
		// Simulation d'un acces dense
		dValue += dvValues.GetAt(index.GetDenseIndex() % dvValues.GetSize());
	}
	timer.Stop();
	cout << dValue << " " << timer.GetElapsedTime() << endl;

	// Performance sur l'acces sparse
	cout << "Performance sur l'acces sparse: " << flush;
	index.SetDenseIndex(256);
	index.SetSparseIndex(1024);
	dValue = 0;
	timer.Reset();
	timer.Start();
	for (i = 0; i < nMaxAccess; i++)
	{
		// Simulation d'un acces sparse (en deux temps)
		dValue += dvValues.GetAt(index.GetDenseIndex() % dvValues.GetSize()) +
			  sqrt(dvValues.GetAt(index.GetSparseIndex() % dvValues.GetSize()));
	}
	timer.Stop();
	cout << dValue << " " << timer.GetElapsedTime() << endl;

	// Performance sur l'acces generique dense
	cout << "Performance sur l'acces generique dense: " << flush;
	index.SetDenseIndex(256);
	index.SetSparseIndex(-1);
	dValue = 0;
	timer.Reset();
	timer.Start();
	for (i = 0; i < nMaxAccess; i++)
	{
		// Simulation d'un acces generique, aboutissant a un acces dense
		dValue += (index.IsDense() ? dvValues.GetAt(index.GetDenseIndex() % dvValues.GetSize())
					   : dvValues.GetAt(index.GetDenseIndex() % dvValues.GetSize()) +
						 sqrt(dvValues.GetAt(index.GetSparseIndex() % dvValues.GetSize())));
	}
	timer.Stop();
	cout << dValue << " " << timer.GetElapsedTime() << endl;

	// Performance sur l'acces generique sparse
	cout << "Performance sur l'acces generique sparse: " << flush;
	index.SetDenseIndex(256);
	index.SetSparseIndex(1024);
	dValue = 0;
	timer.Reset();
	timer.Start();
	for (i = 0; i < nMaxAccess; i++)
	{
		// Simulation d'un acces generique, aboutissant a un acces sparse
		dValue += (index.IsDense() ? dvValues.GetAt(index.GetDenseIndex() % dvValues.GetSize())
					   : dvValues.GetAt(index.GetDenseIndex() % dvValues.GetSize()) +
						 sqrt(dvValues.GetAt(index.GetSparseIndex() % dvValues.GetSize())));
	}
	timer.Stop();
	cout << dValue << " " << timer.GetElapsedTime() << endl;
}

/////////////////////////////////////////////
// Implementation de la classe PLShared_LoadIndexVector

PLShared_LoadIndexVector::PLShared_LoadIndexVector() {}

PLShared_LoadIndexVector::~PLShared_LoadIndexVector() {}

void PLShared_LoadIndexVector::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	require(serializer != NULL);
	require(serializer->IsOpenForWrite());

	serializer->PutLongintVector(&(cast(KWLoadIndexVector*, o)->lvLoadIndexes));
}

void PLShared_LoadIndexVector::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	require(serializer != NULL);
	require(serializer->IsOpenForRead());

	serializer->GetLongintVector(&(cast(KWLoadIndexVector*, o)->lvLoadIndexes));
}

Object* PLShared_LoadIndexVector::Create() const
{
	return new KWLoadIndexVector;
}

/////////////////////////////////////////////
// Implementation de la classe PLShared_LoadIndex

PLShared_LoadIndex::PLShared_LoadIndex() {}

PLShared_LoadIndex::~PLShared_LoadIndex() {}

void PLShared_LoadIndex::Write(ostream& ost) const
{
	ost << GetValue();
}

void PLShared_LoadIndex::SerializeValue(PLSerializer* serializer) const
{
	serializer->PutLongint(liValue.lFullIndex);
}

void PLShared_LoadIndex::DeserializeValue(PLSerializer* serializer)
{
	liValue.lFullIndex = serializer->GetLongint();
}
