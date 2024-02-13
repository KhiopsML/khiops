// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWSortableIndex.h"

////////////////////////////////////////////////////////////////////////
// Classe KWSortableIndex

void KWSortableIndex::Write(ostream& ost) const
{
	ost << nIndex << "\n";
}

int KWSortableIndexCompare(const void* elem1, const void* elem2)
{
	// Comparaison sur le critere de tri
	return cast(KWSortableIndex*, *(Object**)elem1)->GetIndex() -
	       cast(KWSortableIndex*, *(Object**)elem2)->GetIndex();
}

////////////////////////////////////////////////////////////////////////
// Classe KWSortableValue

void KWSortableValue::Write(ostream& ost) const
{
	ost << dSortValue << "\t" << nIndex << "\n";
}

int KWSortableValueCompare(const void* elem1, const void* elem2)
{
	double dResult;

	// Comparaison sur le critere de tri
	dResult = cast(KWSortableValue*, *(Object**)elem1)->GetSortValue() -
		  cast(KWSortableValue*, *(Object**)elem2)->GetSortValue();
	if (dResult > 0)
		return 1;
	else if (dResult < 0)
		return -1;
	else
		return cast(KWSortableIndex*, *(Object**)elem1)->GetIndex() -
		       cast(KWSortableIndex*, *(Object**)elem2)->GetIndex();
}

int KWSortableValueCompareDecreasing(const void* elem1, const void* elem2)
{
	double dResult;

	// Comparaison sur le critere de tri
	dResult = cast(KWSortableValue*, *(Object**)elem1)->GetSortValue() -
		  cast(KWSortableValue*, *(Object**)elem2)->GetSortValue();
	if (dResult > 0)
		return -1;
	else if (dResult < 0)
		return 1;
	else
		return -cast(KWSortableIndex*, *(Object**)elem1)->GetIndex() +
		       cast(KWSortableIndex*, *(Object**)elem2)->GetIndex();
}

////////////////////////////////////////////////////////////////////////
// Classe KWSortableValueSymbol

void KWSortableValueSymbol::Write(ostream& ost) const
{
	ost << dSortValue << "\t" << sSymbol << "\t" << nIndex << "\n";
}

int KWSortableValueSymbolCompare(const void* elem1, const void* elem2)
{
	double dResult;

	// Comparaison sur le critere de tri
	dResult = cast(KWSortableValueSymbol*, *(Object**)elem1)->GetSortValue() -
		  cast(KWSortableValueSymbol*, *(Object**)elem2)->GetSortValue();
	if (dResult > 0)
		return 1;
	else if (dResult < 0)
		return -1;
	else
		return cast(KWSortableValueSymbol*, *(Object**)elem1)
		    ->GetSymbol()
		    .CompareValue(cast(KWSortableValueSymbol*, *(Object**)elem2)->GetSymbol());
}

////////////////////////////////////////////////////////////////////////
// Classe KWSortableContinuous

void KWSortableContinuous::Write(ostream& ost) const
{
	ost << cSortValue << "\t" << nIndex << "\n";
}

int KWSortableContinuousCompare(const void* elem1, const void* elem2)
{
	Continuous cResult;

	// Comparaison sur le critere de tri
	cResult = cast(KWSortableContinuous*, *(Object**)elem1)->GetSortValue() -
		  cast(KWSortableContinuous*, *(Object**)elem2)->GetSortValue();
	if (cResult > 0)
		return 1;
	else if (cResult < 0)
		return -1;
	else
		return 0;
}

////////////////////////////////////////////////////////////////////////
// Classe KWSortableContinuousSymbol

void KWSortableContinuousSymbol::Write(ostream& ost) const
{
	ost << cSortValue << "\t" << sSymbol << "\t" << nIndex << "\n";
}

int KWSortableContinuousSymbolCompare(const void* elem1, const void* elem2)
{
	Continuous cResult;

	// Comparaison sur le critere de tri
	cResult = cast(KWSortableContinuousSymbol*, *(Object**)elem1)->GetSortValue() -
		  cast(KWSortableContinuousSymbol*, *(Object**)elem2)->GetSortValue();
	if (cResult > 0)
		return 1;
	else if (cResult < 0)
		return -1;
	else
		return cast(KWSortableContinuousSymbol*, *(Object**)elem1)
		    ->GetSymbol()
		    .CompareValue(cast(KWSortableContinuousSymbol*, *(Object**)elem2)->GetSymbol());
}

int KWSortableContinuousSymbolCompareSymbolValue(const void* elem1, const void* elem2)
{
	int nResult;
	Continuous cResult;

	// Comparaison sur la valeur du Symbol
	nResult = cast(KWSortableContinuousSymbol*, *(Object**)elem1)
		      ->GetSymbol()
		      .CompareValue(cast(KWSortableContinuousSymbol*, *(Object**)elem2)->GetSymbol());

	// Comparaison sur le critere de tri
	if (nResult != 0)
		return nResult;
	else
	{
		cResult = cast(KWSortableContinuousSymbol*, *(Object**)elem1)->GetSortValue() -
			  cast(KWSortableContinuousSymbol*, *(Object**)elem2)->GetSortValue();
		if (cResult > 0)
			return 1;
		else if (cResult < 0)
			return -1;
		else
			return 0;
	}
}

////////////////////////////////////////////////////////////////////////
// Classe KWSortableSymbol

void KWSortableSymbol::Write(ostream& ost) const
{
	ost << sSortValue << "\t" << nIndex << "\n";
}

int KWSortableSymbolCompare(const void* elem1, const void* elem2)
{
	// Comparaison sur le critere de tri
	return cast(KWSortableSymbol*, *(Object**)elem1)
	    ->GetSortValue()
	    .Compare(cast(KWSortableSymbol*, *(Object**)elem2)->GetSortValue());
}

int KWSortableSymbolCompareValue(const void* elem1, const void* elem2)
{
	int nCompare;

	// Comparaison sur le critere de tri
	nCompare = cast(KWSortableSymbol*, *(Object**)elem1)
		       ->GetSortValue()
		       .CompareValue(cast(KWSortableSymbol*, *(Object**)elem2)->GetSortValue());

	// Comparaison sur l'index si egal
	if (nCompare == 0)
		nCompare = cast(KWSortableSymbol*, *(Object**)elem1)->GetIndex() -
			   cast(KWSortableSymbol*, *(Object**)elem2)->GetIndex();
	return nCompare;
}

int KWSortableSymbolCompareDecreasingIndexValue(const void* elem1, const void* elem2)
{
	int nCompare;

	// Comparaison sur l'index
	nCompare = -cast(KWSortableSymbol*, *(Object**)elem1)->GetIndex() +
		   cast(KWSortableSymbol*, *(Object**)elem2)->GetIndex();

	// Comparaison sur la valeur si index egal
	if (nCompare == 0)
	{
		nCompare = cast(KWSortableSymbol*, *(Object**)elem1)
			       ->GetSortValue()
			       .CompareValue(cast(KWSortableSymbol*, *(Object**)elem2)->GetSortValue());
	}
	return nCompare;
}

////////////////////////////////////////////////////////////////////////
// Classe KWSortableFrequencySymbol

void KWSortableFrequencySymbol::Write(ostream& ost) const
{
	ost << sSortValue << "\t" << nFrequency << "\t" << nIndex << "\n";
}

int KWSortableFrequencySymbolCompare(const void* elem1, const void* elem2)
{
	int nCompare;

	// Comparaison sur l'effectif
	nCompare = -cast(KWSortableFrequencySymbol*, *(Object**)elem1)->GetFrequency() +
		   cast(KWSortableFrequencySymbol*, *(Object**)elem2)->GetFrequency();

	// Comparaison sur la valeur si index egal
	if (nCompare == 0)
	{
		nCompare = cast(KWSortableFrequencySymbol*, *(Object**)elem1)
			       ->GetSortValue()
			       .CompareValue(cast(KWSortableFrequencySymbol*, *(Object**)elem2)->GetSortValue());
	}
	return nCompare;
}

////////////////////////////////////////////////////////////////////////
// Classe KWSortableObject

void KWSortableObject::Write(ostream& ost) const
{
	ost << oSortValue << "\t" << nIndex << "\n";
}

int KWSortableObjectCompare(const void* elem1, const void* elem2)
{
	int nCompare;

	// Comparaison sur le critere de tri
	if (cast(KWSortableObject*, *(Object**)elem1)->GetSortValue() ==
	    cast(KWSortableObject*, *(Object**)elem2)->GetSortValue())
		nCompare = 0;
	else if (cast(KWSortableObject*, *(Object**)elem1)->GetSortValue() >
		 cast(KWSortableObject*, *(Object**)elem2)->GetSortValue())
		nCompare = 1;
	else
		nCompare = -1;
	return nCompare;
}

////////////////////////////////////////////////////////////////////////
// Classe KWIntVectorSorter

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

void KWIntVectorSorter::SortVector(const IntVector* ivInputVector)
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

int KWIntVectorSorter::LookupInitialIndex(int nValue) const
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

boolean KWIntVectorSorter::CheckUniqueValues() const
{
	int i;

	for (i = 1; i < GetSize(); i++)
	{
		if (GetSortedValueAt(i) == GetSortedValueAt(i - 1))
			return false;
	}
	return true;
}

void KWIntVectorSorter::Clean()
{
	ipvSortedValueIndexPairs.SetSize(0);
}

void KWIntVectorSorter::Write(ostream& ost) const
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

longint KWIntVectorSorter::GetUsedMemory() const
{
	return sizeof(KWIntVectorSorter) + ipvSortedValueIndexPairs.GetUsedMemory() - sizeof(IntPairVector);
}
