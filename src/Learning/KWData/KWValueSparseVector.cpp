// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWValueSparseVector.h"

// Inclusion dans le source plutot que le header pour eviter les reference cycliques
#include "KWObject.h"

//////////////////////////////////////////////////////////////////
// Classe KWValueSparseVector

KWValueSparseVector::KWValueSparseVector() {}

KWValueSparseVector::~KWValueSparseVector() {}

void KWValueSparseVector::RemoveAll()
{
	ivSparseIndexes.SetSize(0);
}

void KWValueSparseVector::Write(ostream& ost) const
{
	const int nMax = 10;
	int n;

	ost << GetClassLabel() << " [" << GetValueNumber() << "]:";
	for (n = 0; n < GetValueNumber(); n++)
	{
		if (n > 0)
			cout << ", ";
		ost << " (" << GetSparseIndexAt(n) << ":";
		ost << " [";
		WriteValueAt(ost, n);
		ost << "])";
		if (n >= nMax)
		{
			ost << "...";
			break;
		}
	}
	cout << "\n";
}

void KWValueSparseVector::WriteValueAt(ostream& ost, int nIndex) const {}

const IntVector* KWValueSparseVector::GetSparseIndexVector() const
{
	return &ivSparseIndexes;
}

longint KWValueSparseVector::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWValueSparseVector);
	lUsedMemory += ivSparseIndexes.GetUsedMemory() - sizeof(IntVector);
	return lUsedMemory;
}

const ALString KWValueSparseVector::GetClassLabel() const
{
	return "Sparse vector of " + KWType::ToString(GetType()) + " values";
}

//////////////////////////////////////////////////////////////////
// Classe KWContinuousValueSparseVector

KWContinuousValueSparseVector::KWContinuousValueSparseVector() {}

KWContinuousValueSparseVector::~KWContinuousValueSparseVector() {}

int KWContinuousValueSparseVector::GetType() const
{
	return KWType::Continuous;
}

void KWContinuousValueSparseVector::RemoveAll()
{
	KWValueSparseVector::RemoveAll();
	cvValues.SetSize(0);
}

void KWContinuousValueSparseVector::AddValueAt(int nSparseIndex, Continuous cValue)
{
	require(nSparseIndex >= 0);
	assert(GetValueNumber() == cvValues.GetSize());
	ivSparseIndexes.Add(nSparseIndex);
	cvValues.Add(cValue);
}

void KWContinuousValueSparseVector::WriteValueAt(ostream& ost, int nIndex) const
{
	require(0 <= nIndex and nIndex < GetValueNumber());
	ost << GetValueAt(nIndex);
}

void KWContinuousValueSparseVector::Test()
{
	const int nValueNumber = 1000;
	KWContinuousValueSparseVector vdTest;
	KWContinuousValueSparseVector vdCopy;
	int i;
	ALString sTmp;

	// Alimentation
	for (i = 0; i < nValueNumber; i++)
		vdTest.AddValueAt(i, i + 1);
	cout << "Init " << vdTest << endl;

	// Remove values
	vdTest.RemoveAll();
	cout << "Remove " << vdTest << endl;
}

//////////////////////////////////////////////////////////////////
// Classe KWSymbolValueSparseVector

KWSymbolValueSparseVector::KWSymbolValueSparseVector() {}

KWSymbolValueSparseVector::~KWSymbolValueSparseVector() {}

int KWSymbolValueSparseVector::GetType() const
{
	return KWType::Symbol;
}

void KWSymbolValueSparseVector::RemoveAll()
{
	KWValueSparseVector::RemoveAll();
	svValues.SetSize(0);
}

void KWSymbolValueSparseVector::AddValueAt(int nSparseIndex, const Symbol& sValue)
{
	require(nSparseIndex >= 0);
	assert(GetValueNumber() == svValues.GetSize());
	ivSparseIndexes.Add(nSparseIndex);
	svValues.Add(sValue);
}

void KWSymbolValueSparseVector::WriteValueAt(ostream& ost, int nIndex) const
{
	require(0 <= nIndex and nIndex < GetValueNumber());
	ost << GetValueAt(nIndex);
}

void KWSymbolValueSparseVector::Test()
{
	const int nValueNumber = 1000;
	KWSymbolValueSparseVector vdTest;
	KWSymbolValueSparseVector vdCopy;
	int i;
	ALString sTmp;

	// Alimentation
	for (i = 0; i < nValueNumber; i++)
		vdTest.AddValueAt(i, Symbol(sTmp + "v" + IntToString(i + 1)));
	cout << "Init " << vdTest << endl;

	// Remove values
	vdTest.RemoveAll();
	cout << "Remove " << vdTest << endl;
}

//////////////////////////////////////////////////////////////////
// Classe KWObjectArrayValueSparseVector

KWObjectArrayValueSparseVector::KWObjectArrayValueSparseVector() {}

KWObjectArrayValueSparseVector::~KWObjectArrayValueSparseVector() {}

int KWObjectArrayValueSparseVector::GetType() const
{
	return KWType::ObjectArray;
}

void KWObjectArrayValueSparseVector::RemoveAll()
{
	KWValueSparseVector::RemoveAll();
	oaValues.SetSize(0);
}

void KWObjectArrayValueSparseVector::AddValueAt(int nSparseIndex, ObjectArray* oaValue)
{
	require(nSparseIndex >= 0);
	require(oaValue != NULL);
	assert(GetValueNumber() == oaValues.GetSize());
	ivSparseIndexes.Add(nSparseIndex);
	oaValues.Add(oaValue);
}

void KWObjectArrayValueSparseVector::DeleteAll()
{
	oaValues.DeleteAll();
	RemoveAll();
}

void KWObjectArrayValueSparseVector::WriteValueAt(ostream& ost, int nIndex) const
{
	require(0 <= nIndex and nIndex < GetValueNumber());
	if (GetValueAt(nIndex) == NULL)
		ost << "NULL";
	else
		ost << "Part(" << GetValueAt(nIndex)->GetSize() << ")";
}

void KWObjectArrayValueSparseVector::Test()
{
	const int nValueNumber = 1000;
	KWClass* attributeClass;
	ObjectArray oaCreatedObjects;
	ObjectArray oaNewCreatedObjects;
	ObjectArray* oaPart;
	KWObjectArrayValueSparseVector vdTest;
	KWObjectArrayValueSparseVector vdCopy;
	int i;
	ALString sTmp;

	// Creation d'une classe de test
	attributeClass = KWClass::CreateClass("AttributeClass", 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, true, NULL);
	KWClassDomain::GetCurrentDomain()->InsertClass(attributeClass);
	KWClassDomain::GetCurrentDomain()->Compile();

	// Creation des tableaux d'objets
	for (i = 0; i < nValueNumber; i++)
		vdTest.AddValueAt(i, new ObjectArray);

	// Creation d'objets
	for (i = 0; i < nValueNumber; i++)
	{
		oaCreatedObjects.Add(KWObject::CreateObject(attributeClass, i + 1));
		oaNewCreatedObjects.Add(KWObject::CreateObject(attributeClass, nValueNumber + i + 1));
	}

	// Alimentation avec un objet par cle
	for (i = 0; i < nValueNumber; i++)
	{
		oaPart = vdTest.GetValueAt(i);
		oaPart->Add(oaCreatedObjects.GetAt(i));
	}
	cout << "Init " << vdTest << endl;

	// Ajout d'autres objets
	for (i = 0; i < nValueNumber; i++)
	{
		oaPart = vdTest.GetValueAt(i);
		oaPart->Add(oaNewCreatedObjects.GetAt(i));
	}
	cout << "With new objets " << vdTest << endl;

	// Remove values
	vdTest.DeleteAll();
	cout << "Remove " << vdTest << endl;

	// Destruction des objets
	oaCreatedObjects.DeleteAll();
	oaNewCreatedObjects.DeleteAll();

	// Destruction de toutes les classes enregistrees
	KWClassDomain::DeleteAllDomains();
}
