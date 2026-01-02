// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "IntVectorSorter.h"

////////////////////////////////////////////////////////////////////////
// Classe IntVectorSorter

int IntPairVectorCompareValue1(const void* elem1, const void* elem2)
{
	longint lValue1;
	longint lValue2;

	// Acces aux valeurs
	lValue1 = *(longint*)elem1;
	lValue2 = *(longint*)elem2;

	// Comparaison
	return IntPairVector::LongintToInt1(lValue1) - IntPairVector::LongintToInt1(lValue2);
}

int IntPairVectorCompareValue2(const void* elem1, const void* elem2)
{
	longint lValue1;
	longint lValue2;

	// Acces aux valeurs
	lValue1 = *(longint*)elem1;
	lValue2 = *(longint*)elem2;

	// Comparaison
	return IntPairVector::LongintToInt2(lValue1) - IntPairVector::LongintToInt2(lValue2);
}

void IntPairVector::SortByValue1()
{
	MemVector::Sort(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, IntPairVectorCompareValue1);
}

void IntPairVector::SortByValue2()
{
	MemVector::Sort(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, IntPairVectorCompareValue2);
}

void IntPairVector::Write(ostream& ost) const
{
	const int nMaxSize = 10;
	int i;

	ost << GetClassLabel() + " (size=" << GetSize() << ")\n";
	for (i = 0; i < GetSize() and i < nMaxSize; i++)
		ost << "\t(" << GetValue1At(i) << ", " << GetValue2At(i) << ")\n";
	if (GetSize() > nMaxSize)
		ost << "\t"
		    << "..."
		    << "\n";
}

const ALString IntPairVector::GetClassLabel() const
{
	return "Integer pair vector";
}

void IntPairVector::Test()
{
	IntPairVector ipvTest;
	int i;
	const int nSize = 10;
	int nValue;

	// Creation d'un vecteur avec des valeurs positives et negatives
	nValue = 1;
	for (i = 0; i < nSize; i++)
	{
		ipvTest.AddPair(nValue, i);
		nValue *= -2;
	}
	cout << "Initial pairs: " << ipvTest << endl;

	// Tri par valeur 1
	ipvTest.SortByValue1();
	cout << "Sorted by value 1: " << ipvTest << endl;

	// Tri par valeur 2
	ipvTest.SortByValue2();
	cout << "Sorted by value 2: " << ipvTest << endl;
}

void IntVectorSorter::SortVector(const IntVector* ivInputVector)
{
	int i;

	require(ivInputVector != NULL);

	// Initialisation du vecteur de paires (valeur, index)
	ipvSortedValueIndexPairs.SetSize(ivInputVector->GetSize());
	for (i = 0; i < ivInputVector->GetSize(); i++)
	{
		// Memorisation de la paire (valeur, index)
		ipvSortedValueIndexPairs.SetPairAt(i, ivInputVector->GetAt(i), i);
	}

	// Tri des entiers longs, donc des paires (valeur, index) par valeur
	ipvSortedValueIndexPairs.SortByValue1();
}

int IntVectorSorter::LookupInitialIndex(int nValue) const
{
	int nLeft;
	int nRight;
	int nMiddle;
	longint lSearchedValue;
	longint lValue;

	// Stockage de la cle au format longint
	lSearchedValue = IntPairVector::IntPairToLongint(nValue, 0);

	// Initialisation des index de recherche dichotomique
	nLeft = 0;
	nRight = GetSize() - 1;

	// Recherche dichotomique
	while (nLeft < nRight)
	{
		assert(ipvSortedValueIndexPairs.GetValue1At(nLeft) < ipvSortedValueIndexPairs.GetValue1At(nRight));
		nMiddle = (nLeft + nRight) / 2;
		lValue = ipvSortedValueIndexPairs.GetValue1At(nMiddle);
		if (lValue < lSearchedValue)
			nLeft = nMiddle + 1;
		else
			nRight = nMiddle;
	}

	// On teste la cle (poids fort de l'entier long) pour l'index final
	if (nLeft < GetSize() and GetSortedValueAt(nLeft) == nValue)
		return GetInitialIndexAt(nLeft);
	else
		return -1;
}

boolean IntVectorSorter::CheckUniqueValues() const
{
	int i;

	for (i = 1; i < GetSize(); i++)
	{
		if (GetSortedValueAt(i) == GetSortedValueAt(i - 1))
			return false;
	}
	return true;
}

void IntVectorSorter::Clean()
{
	ipvSortedValueIndexPairs.SetSize(0);
}

void IntVectorSorter::Write(ostream& ost) const
{
	const int nMaxSize = 10;
	int i;

	ost << "Sorted vector (size=" << GetSize() << ")\n";
	for (i = 0; i < GetSize() and i < nMaxSize; i++)
		ost << "\t(" << GetSortedValueAt(i) << ", " << GetInitialIndexAt(i) << ")\n";
	if (GetSize() > nMaxSize)
		ost << "\t"
		    << "..."
		    << "\n";
}

longint IntVectorSorter::GetUsedMemory() const
{
	return sizeof(IntVectorSorter) + ipvSortedValueIndexPairs.GetUsedMemory() - sizeof(IntPairVector);
}
