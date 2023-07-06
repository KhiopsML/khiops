// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHDataSubset.h"

MHDataSubset::MHDataSubset()
{
	cvDatasetValues = NULL;
	nFirstValueIndex = -1;
	nLastValueIndex = -1;
	bPWCH = false;
	nPICHSplitIndex = 0;
	bTerminalPrev = false;
	bTerminalNext = false;
	histogram = NULL;
	prevBoundaryHistogram = NULL;
}

MHDataSubset::~MHDataSubset()
{
	DeleteHistograms();
}

void MHDataSubset::SetDatasetValues(const ContinuousVector* cvValues)
{
	cvDatasetValues = cvValues;
}

const ContinuousVector* MHDataSubset::GetDatasetValues() const
{
	return cvDatasetValues;
}

void MHDataSubset::SetFirstValueIndex(int nValue)
{
	require(0 <= nValue and nValue < cvDatasetValues->GetSize());
	nFirstValueIndex = nValue;
}

int MHDataSubset::GetFirstValueIndex() const
{
	ensure(0 <= nFirstValueIndex and nFirstValueIndex < cvDatasetValues->GetSize());
	return nFirstValueIndex;
}

void MHDataSubset::SetLastValueIndex(int nValue)
{
	require(0 <= nValue and nValue <= cvDatasetValues->GetSize());
	nLastValueIndex = nValue;
}

int MHDataSubset::GetLastValueIndex() const
{
	ensure(0 <= nLastValueIndex and nLastValueIndex <= cvDatasetValues->GetSize());
	return nLastValueIndex;
}

int MHDataSubset::GetSize() const
{
	return GetLastValueIndex() - GetFirstValueIndex();
}

Continuous MHDataSubset::GetFirstValue() const
{
	return cvDatasetValues->GetAt(GetFirstValueIndex());
}

Continuous MHDataSubset::GetLastValue() const
{
	return cvDatasetValues->GetAt(GetLastValueIndex() - 1);
}

boolean MHDataSubset::IsSingleton() const
{
	return GetFirstValue() == GetLastValue();
}

void MHDataSubset::SetPWCH(boolean bValue)
{
	bPWCH = bValue;
}

boolean MHDataSubset::GetPWCH() const
{
	return bPWCH;
}

void MHDataSubset::SetPICHSplitIndex(int nValue)
{
	require(nValue >= 0);
	nPICHSplitIndex = nValue;
}

int MHDataSubset::GetPICHSplitIndex() const
{
	return nPICHSplitIndex;
}

void MHDataSubset::SetTerminalPrev(boolean bValue)
{
	bTerminalPrev = bValue;
}

boolean MHDataSubset::GetTerminalPrev() const
{
	return bTerminalPrev;
}

void MHDataSubset::SetTerminalNext(boolean bValue)
{
	bTerminalNext = bValue;
}

boolean MHDataSubset::GetTerminalNext() const
{
	return bTerminalNext;
}

boolean MHDataSubset::GetTerminal() const
{
	return GetTerminalPrev() and GetTerminalNext();
}

void MHDataSubset::SetHistogram(const KWFrequencyTable* histogramFrequencyTable)
{
	if (histogram != NULL)
		delete histogram;
	histogram = histogramFrequencyTable;
}

const KWFrequencyTable* MHDataSubset::GetHistogram() const
{
	return histogram;
}

void MHDataSubset::SetPrevBoundaryHistogram(const KWFrequencyTable* histogramFrequencyTable)
{
	if (prevBoundaryHistogram != NULL)
		delete prevBoundaryHistogram;
	prevBoundaryHistogram = histogramFrequencyTable;
}

const KWFrequencyTable* MHDataSubset::GetPrevBoundaryHistogram() const
{
	return prevBoundaryHistogram;
}

void MHDataSubset::DeleteHistograms()
{
	if (histogram != NULL)
		delete histogram;
	if (prevBoundaryHistogram != NULL)
		delete prevBoundaryHistogram;
	histogram = NULL;
	prevBoundaryHistogram = NULL;
}

void MHDataSubset::RemoveHistograms()
{
	histogram = NULL;
	prevBoundaryHistogram = NULL;
}

void MHDataSubset::CopyFrom(const MHDataSubset* aSource)
{
	require(aSource != NULL);

	cvDatasetValues = aSource->cvDatasetValues;
	nFirstValueIndex = aSource->nFirstValueIndex;
	nLastValueIndex = aSource->nLastValueIndex;
	bPWCH = aSource->bPWCH;
	nPICHSplitIndex = aSource->GetPICHSplitIndex();
	bTerminalPrev = aSource->bTerminalPrev;
	bTerminalNext = aSource->bTerminalNext;
	histogram = NULL;
	prevBoundaryHistogram = NULL;
}

MHDataSubset* MHDataSubset::Clone() const
{
	MHDataSubset* aClone;

	aClone = new MHDataSubset;
	aClone->CopyFrom(this);
	return aClone;
}

void MHDataSubset::WriteHeaderLine(ostream& ost) const
{
	ost << "First index\tLastIndex\tSize\tFirst value\tLastValue\tPWCH\tPICH split\tTerm prev\tTerm next";
}

void MHDataSubset::Write(ostream& ost) const
{
	cout << nFirstValueIndex << "\t";
	cout << nLastValueIndex << "\t";
	cout << GetSize() << "\t";
	cout << GetFirstValue() << "\t";
	cout << GetLastValue() << "\t";
	cout << bPWCH << "\t";
	cout << nPICHSplitIndex << "\t";
	cout << bTerminalPrev << "\t";
	cout << bTerminalNext;
}

const ALString MHDataSubset::GetClassLabel() const
{
	return "Data subset";
}

const ALString MHDataSubset::GetObjectLabel() const
{
	ALString sLabel;
	sLabel = "(";
	sLabel += IntToString(nFirstValueIndex);
	sLabel = ", ";
	sLabel += IntToString(nLastValueIndex);
	sLabel = ")";
	return sLabel;
}
