// Copyright (c) 2023-2026 Orange. All rights reserved.
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
