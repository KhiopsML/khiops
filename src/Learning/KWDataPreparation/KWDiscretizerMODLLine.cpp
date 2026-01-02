// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDiscretizerMODLLine.h"

/////////////////////////////////////////////////////////////////////
// Classe KWMODLLineMerge

void KWMODLLineMerge::CopyFrom(KWMODLLineMerge* lineMerge)
{
	require(lineMerge != NULL);

	kwfvFrequencyVector->CopyFrom(lineMerge->GetFrequencyVector());
	dCost = lineMerge->GetCost();
	dDeltaCost = lineMerge->GetDeltaCost();
	dTruncatedDeltaCost = lineMerge->GetTruncatedDeltaCost();
	position = lineMerge->GetPosition();
}

void KWMODLLineMerge::WriteHeaderLineReport(ostream& ost) const
{
	// Cout et variation du cout
	ost << "Cost\tDeltaCost\t";

	// Libelle des valeurs cibles
	kwfvFrequencyVector->WriteHeaderLineReport(ost);
}

void KWMODLLineMerge::WriteLineReport(ostream& ost) const
{
	// Cout et variation du cout
	ost << dCost << "\t" << dDeltaCost << "\t";

	// Frequence des des valeurs cibles
	kwfvFrequencyVector->WriteLineReport(ost);
}

void KWMODLLineMerge::Write(ostream& ost) const
{
	WriteHeaderLineReport(ost);
	ost << "\n";
	WriteLineReport(ost);
	ost << "\n";
}

/////////////////////////////////////////////////////////////////////
// Classe KWMODLLineSplit

void KWMODLLineSplit::CopyFrom(KWMODLLineSplit* lineSplit)
{
	require(lineSplit != NULL);

	kwfvFirstSubLineFrequencyVector->CopyFrom(lineSplit->GetFirstSubLineFrequencyVector());
	nFirstSubLineIndex = lineSplit->GetFirstSubLineIndex();
	dFirstSubLineCost = lineSplit->GetFirstSubLineCost();
	dSecondSubLineCost = lineSplit->GetSecondSubLineCost();
	dDeltaCost = lineSplit->GetDeltaCost();
	dTruncatedDeltaCost = lineSplit->GetTruncatedDeltaCost();
	position = lineSplit->GetPosition();
}

void KWMODLLineSplit::WriteHeaderLineReport(ostream& ost) const
{
	// Index de la premiere sous partie
	ost << "Index1\t";

	// Couts et variation du cout
	ost << "Cost1\tCost2\tDeltaCost\t";

	// Libelle des valeurs cibles
	kwfvFirstSubLineFrequencyVector->WriteHeaderLineReport(ost);
}

void KWMODLLineSplit::WriteLineReport(ostream& ost) const
{
	// Index de la premiere sous partie
	ost << nFirstSubLineIndex << "\t";

	// Cout et variation du cout
	ost << dFirstSubLineCost << "\t" << dSecondSubLineCost << "\t" << dDeltaCost << "\t";

	// Frequence des valeurs cibles
	kwfvFirstSubLineFrequencyVector->WriteLineReport(ost);
}

void KWMODLLineSplit::Write(ostream& ost) const
{
	WriteHeaderLineReport(ost);
	ost << "\n";
	WriteLineReport(ost);
	ost << "\n";
}

/////////////////////////////////////////////////////////////////////
// Classe KWMODLLine

void KWMODLLine::CopyFrom(KWMODLLine* line)
{
	require(line != NULL);

	kwfvFrequencyVector->CopyFrom(line->GetFrequencyVector());
	nIndex = line->GetIndex();
	prevLine = line->GetPrev();
	nextLine = line->GetNext();
	dCost = line->GetCost();
	position = line->GetPosition();
}

void KWMODLLine::WriteHeaderLineReport(ostream& ost) const
{
	// Index
	ost << "Index";

	// Cout
	ost << "\tCost\t";

	// Libelle des valeurs cibles
	kwfvFrequencyVector->WriteHeaderLineReport(ost);
}

void KWMODLLine::WriteLineReport(ostream& ost) const
{
	// Index
	ost << GetIndex();

	// Cout
	ost << "\t" << dCost << "\t";

	// Frequence des valeurs cibles
	kwfvFrequencyVector->WriteLineReport(ost);
}

void KWMODLLine::Write(ostream& ost) const
{
	WriteHeaderLineReport(ost);
	ost << "\n";
	WriteLineReport(ost);
	ost << "\n";
}

/////////////////////////////////////////////////////////////////////
// Classe KWMODLLineOptimization

void KWMODLLineOptimization::WriteHeaderLineReport(ostream& ost) const
{
	// Entete de base
	KWMODLLine::WriteHeaderLineReport(ost);

	// Informations de merge
	ost << "\tMerge\t";
	merge.WriteHeaderLineReport(ost);
}

void KWMODLLineOptimization::WriteLineReport(ostream& ost) const
{
	// Informations de base
	KWMODLLine::WriteLineReport(ost);

	// Informations de merge
	ost << "\t\t";
	merge.WriteLineReport(ost);
}

/////////////////////////////////////////////////////////////////////
// Classe KWMODLLineDeepOptimization

void KWMODLLineDeepOptimization::WriteHeaderLineReport(ostream& ost) const
{
	// Entete de base
	KWMODLLine::WriteHeaderLineReport(ost);

	// Informations de Split
	ost << "\tSplit\t";
	split.WriteHeaderLineReport(ost);

	// Informations de MergeSplit
	ost << "\tMergeSplit\t";
	mergeSplit.WriteHeaderLineReport(ost);

	// Informations de MergeMergeSplit
	ost << "\tMergeMergeSplit\t";
	mergeMergeSplit.WriteHeaderLineReport(ost);
}

void KWMODLLineDeepOptimization::WriteLineReport(ostream& ost) const
{
	// Informations de base
	KWMODLLine::WriteLineReport(ost);

	// Informations de Split
	ost << "\t\t";
	split.WriteLineReport(ost);

	// Informations de MergeSplit
	ost << "\t\t";
	mergeSplit.WriteLineReport(ost);

	// Informations de MergeMergeSplit
	ost << "\t\t";
	mergeMergeSplit.WriteLineReport(ost);
}

/////////////////////////////////////////////////////////////////////
// Classe KWMODLLineOptimalDiscretization

KWMODLLineOptimalDiscretization::KWMODLLineOptimalDiscretization(const KWFrequencyVector* kwfvFrequencyVectorCreator)
    : KWMODLLine(kwfvFrequencyVectorCreator)
{
	nIntervalNumber = 0;
	dDiscretizationCost = 0;
}

KWMODLLineOptimalDiscretization::~KWMODLLineOptimalDiscretization() {}

KWMODLLine* KWMODLLineOptimalDiscretization::Create() const
{
	return new KWMODLLineOptimalDiscretization(kwfvFrequencyVector);
}

int KWMODLLineOptimalDiscretization::GetIntervalNumber() const
{
	return nIntervalNumber;
}

void KWMODLLineOptimalDiscretization::SetIntervalNumber(int nValue)
{
	require(nValue >= 0);
	nIntervalNumber = nValue;
}

int KWMODLLineOptimalDiscretization::GetMaxIntervalNumber() const
{
	return ivLastLineIndexes.GetSize();
}

void KWMODLLineOptimalDiscretization::SetMaxIntervalNumber(int nValue)
{
	require(nValue >= 0);
	ivLastLineIndexes.SetSize(nValue);
}

double KWMODLLineOptimalDiscretization::GetDiscretizationCost() const
{
	return dDiscretizationCost;
}

void KWMODLLineOptimalDiscretization::SetDiscretizationCost(double dValue)
{
	dDiscretizationCost = dValue;
}

void KWMODLLineOptimalDiscretization::SetLastLineIndexAt(int nIntervalIndex, int nLineIndex)
{
	require(GetIntervalNumber() <= GetMaxIntervalNumber());
	require(0 <= nIntervalIndex and nIntervalIndex < nIntervalNumber);
	require(nLineIndex >= 0);
	ivLastLineIndexes.SetAt(nIntervalIndex, nLineIndex);
}

int KWMODLLineOptimalDiscretization::GetLastLineIndexAt(int nIntervalIndex) const
{
	require(GetIntervalNumber() <= GetMaxIntervalNumber());
	require(0 <= nIntervalIndex and nIntervalIndex < nIntervalNumber);
	return ivLastLineIndexes.GetAt(nIntervalIndex);
}

void KWMODLLineOptimalDiscretization::WriteHeaderLineReport(ostream& ost) const
{
	int i;

	// Entete de base
	KWMODLLine::WriteHeaderLineReport(ost);

	// Entete
	ost << "\tOptimal\t";
	ost << "Interval number\tDisc cost";

	// Bornes des intervalles
	for (i = 0; i < GetIntervalNumber(); i++)
		ost << "\tb" << i + 1;
}

void KWMODLLineOptimalDiscretization::WriteLineReport(ostream& ost) const
{
	int i;

	// Informations de base
	KWMODLLine::WriteLineReport(ost);

	// Entete
	ost << "\t\t";
	ost << GetIntervalNumber() << "\t" << GetDiscretizationCost();

	// Bornes des intervalles
	for (i = 0; i < GetIntervalNumber(); i++)
		ost << "\t" << GetLastLineIndexAt(i);
}
