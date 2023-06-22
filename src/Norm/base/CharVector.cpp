// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CharVector.h"

//////////////////////////////////////
// Implementation de la classe CharVector

int CharVectorCompareValue(const void* elem1, const void* elem2)
{
	return *(char*)elem1 - *(char*)elem2;
}

CharVector::~CharVector()
{
	MemVector::Delete(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

void CharVector::SetSize(int nValue)
{
	MemVector::SetSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);
}

void CharVector::Initialize()
{
	MemVector::Initialize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

void CharVector::Sort()
{
	MemVector::Sort(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, CharVectorCompareValue);
}

void CharVector::Shuffle()
{
	int i;
	int iSwap;
	char cSwappedValue;

	// Retour si pas assez d'elements
	if (GetSize() <= 1)
		return;

	// Boucle de swap d'elements du tableau
	for (i = 1; i < GetSize(); i++)
	{
		iSwap = RandomInt(i - 1);
		cSwappedValue = GetAt(iSwap);
		SetAt(iSwap, GetAt(i));
		SetAt(i, cSwappedValue);
	}
}

void CharVector::CopyFrom(const CharVector* cvSource)
{
	require(cvSource != NULL);
	MemVector::CopyFrom(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, cvSource->pData.hugeVector,
			    cvSource->nSize, cvSource->nAllocSize);
}

CharVector* CharVector::Clone() const
{
	CharVector* cvClone;

	cvClone = new CharVector;

	// Recopie
	cvClone->CopyFrom(this);
	return cvClone;
}

boolean CharVector::SetLargeSize(int nValue)
{
	return MemVector::SetLargeSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);
}

void CharVector::Write(ostream& ost) const
{
	const int nMaxSize = 10;

	ost << GetClassLabel() + " (size=" << GetSize() << ")\n";
	for (int i = 0; i < GetSize() and i < nMaxSize; i++)
		ost << "\t" << GetAt(i) << "\n";
	if (GetSize() > nMaxSize)
		ost << "\t"
		    << "..."
		    << "\n";
}

longint CharVector::GetUsedMemory() const
{
	return sizeof(CharVector) + nAllocSize * sizeof(char);
}

const ALString CharVector::GetClassLabel() const
{
	return "Char vector";
}

void CharVector::Test()
{
	CharVector* cvTest;
	CharVector* cvClone;
	int nSize;
	int i;
	char c;

	// Creation
	SetRandomSeed(1);
	nSize = AcquireRangedInt("Taille du vecteur", 0, 10000, 1000);
	cvTest = new CharVector;
	cvTest->SetSize(nSize);
	cout << *cvTest << endl;

	// Remplissage avec des 1
	cout << "Remplissage avec des 1" << endl;
	for (i = 0; i < cvTest->GetSize(); i++)
		cvTest->SetAt(i, '1');
	cout << *cvTest << endl;

	// Duplication
	cout << "Duplication" << endl;
	cvClone = cvTest->Clone();
	cout << *cvClone << endl;

	// Retaillage
	nSize = AcquireRangedInt("Nouvelle taille du vecteur, avec conservation des valeurs", 0, 10000, 2000);
	cvTest->SetSize(nSize);
	cout << *cvTest << endl;

	// Retaillage avec reinitialisation a '0'
	nSize = AcquireRangedInt("Nouvelle taille du vecteur", 0, 10000, 1000);
	cvTest->SetSize(nSize);
	cvTest->Initialize();
	cout << *cvTest << endl;

	// Retaillage par ajouts successifs
	nSize = AcquireRangedInt("Nombre d'agrandissements", 0, 1000000, 9000);
	for (i = 0; i < nSize; i++)
		cvTest->Add(0);
	cout << *cvTest << endl;

	// Remplissage aleatoire
	cout << "Remplissage aleatoire" << endl;
	for (i = 0; i < cvTest->GetSize(); i++)
	{
		c = (char)(int('0') + RandomInt('z' - '0'));
		cvTest->SetAt(i, c);
	}
	cout << *cvTest << endl;

	// Tri
	cout << "Tri" << endl;
	cvTest->Sort();
	cout << *cvTest << endl;

	// Perturbation aleatoire
	cout << "Perturbation aleatoire" << endl;
	cvTest->Shuffle();
	cout << *cvTest << endl;

	// Liberations
	delete cvClone;
	delete cvTest;
}