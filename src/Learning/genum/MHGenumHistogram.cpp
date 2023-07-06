// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHGenumHistogram.h"

//////////////////////////////////////////////////////////
// Classe MHGenumHistogram

MHGenumHistogram::MHGenumHistogram() {}

MHGenumHistogram::~MHGenumHistogram() {}

double MHGenumHistogram::GetGranularizedNullCost() const
{
	double dGranularizedNullCost;
	int nTotalFrequency;
	int nTotalBinLength;

	dGranularizedNullCost = GetNullCost();
	if (GetHistogramCriterion() == "G-Enum")
	{
		nTotalFrequency = ComputeTotalFrequency();
		nTotalBinLength = ComputeTotalBinLength();
		if (GetGranularity() > 0 and nTotalBinLength > 0)
			dGranularizedNullCost += nTotalFrequency * (log(GetGranularity()) - log(nTotalBinLength));
	}
	return dGranularizedNullCost;
}

double MHGenumHistogram::GetGranularizedCost() const
{
	double dGranularizedCost;
	int nTotalFrequency;
	int nTotalBinLength;

	dGranularizedCost = GetCost();
	if (GetHistogramCriterion() == "G-Enum")
	{
		nTotalFrequency = ComputeTotalFrequency();
		nTotalBinLength = ComputeTotalBinLength();
		if (GetGranularity() > 0 and nTotalBinLength > 0)
			dGranularizedCost += nTotalFrequency * (log(GetGranularity()) - log(nTotalBinLength));
	}
	return dGranularizedCost;
}

double MHGenumHistogram::GetGranularizedLevel() const
{
	double dGranularizedLevel;

	// Cas des histogrammes standard
	dGranularizedLevel = 0;
	if (GetHistogramCriterion() == "G-Enum")
	{
		if (GetGranularizedNullCost() != 0)
			dGranularizedLevel = 1 - GetGranularizedCost() / GetGranularizedNullCost();
	}
	// Cas autre
	else
		dGranularizedLevel = GetLevel();
	ensure(dGranularizedLevel >= 0);
	return dGranularizedLevel;
}

int MHGenumHistogram::ComputeTotalBinLength() const
{
	int nTotalBinLength;
	int n;
	MHGenumHistogramInterval* interval;

	// Calcul de l'effectif total
	nTotalBinLength = 0;
	for (n = 0; n < oaIntervals.GetSize(); n++)
	{
		interval = cast(MHGenumHistogramInterval*, oaIntervals.GetAt(n));
		nTotalBinLength += interval->GetBinLength();
	}
	return nTotalBinLength;
}

int MHGenumHistogram::ComputePICHIntervalNumber() const
{
	int nNumber;
	MHGenumHistogramInterval* interval;
	int n;

	// Calcul du nombre d'intervalles PICH
	nNumber = 0;
	for (n = 0; n < oaIntervals.GetSize(); n++)
	{
		interval = cast(MHGenumHistogramInterval*, oaIntervals.GetAt(n));
		if (interval->GetPICH())
			nNumber++;
	}
	return nNumber;
}

double MHGenumHistogram::ComputeLargeScaleTotalBinLength() const
{
	double dTotalBinLength;
	int n;
	MHGenumHistogramInterval* interval;

	// Cas de la representation a virgule flottante
	if (GetMinBinLength() > 0)
		dTotalBinLength = floor(0.9 + (GetConstIntervalAt(GetIntervalNumber() - 1)->GetUpperBound() -
					       GetConstIntervalAt(0)->GetLowerBound()) /
						  GetMinBinLength());
	// Calcul de l'effectif total sinon
	else
	{
		dTotalBinLength = 0;
		for (n = 0; n < oaIntervals.GetSize(); n++)
		{
			interval = cast(MHGenumHistogramInterval*, oaIntervals.GetAt(n));
			dTotalBinLength += interval->GetBinLength();
		}
	}
	return dTotalBinLength;
}

int MHGenumHistogram::GetOutlierDataSubsetNumber() const
{
	if (GetIntervalNumber() == 0)
		return 0;
	else if (cast(const MHGenumHistogramInterval*, GetConstIntervalAt(0))->GetDataSubsetIndex() == 0)
		return 0;
	else
		return cast(const MHGenumHistogramInterval*, GetConstIntervalAt(GetIntervalNumber() - 1))
			   ->GetDataSubsetIndex() -
		       cast(const MHGenumHistogramInterval*, GetConstIntervalAt(0))->GetDataSubsetIndex() + 1;
}

int MHGenumHistogram::GetOutlierBoundaryIntervalNumber() const
{
	int nOutlierBoundaryIntervalNumber;
	int n;
	MHGenumHistogramInterval* interval;

	// calcul de l'effectif total
	nOutlierBoundaryIntervalNumber = 0;
	for (n = 0; n < oaIntervals.GetSize(); n++)
	{
		interval = cast(MHGenumHistogramInterval*, oaIntervals.GetAt(n));
		if (interval->GetPreviousDataSubsetBoundary())
			nOutlierBoundaryIntervalNumber++;
	}
	return nOutlierBoundaryIntervalNumber;
}

void MHGenumHistogram::Clean()
{
	// Appel de la methode ancetre
	MHHistogram::Clean();
}

MHHistogram* MHGenumHistogram::Create() const
{
	return new MHGenumHistogram;
}

void MHGenumHistogram::CopyFrom(const MHHistogram* sourceHistogram)
{
	// Appel de la methode ancetre
	MHHistogram::CopyFrom(sourceHistogram);
}

MHHistogram* MHGenumHistogram::Clone() const
{
	return MHHistogram::Clone();
}

void MHGenumHistogram::WriteSummary(ostream& ost) const
{
	// Appel de la methode ancetre
	MHHistogram::WriteSummary(ost);

	// Indicateurs specifiques
	ost << "\tNull cost(G)\t" << KWContinuous::ContinuousToString(GetGranularizedNullCost()) << "\n";
	ost << "\tCost(G)\t" << KWContinuous::ContinuousToString(GetGranularizedCost()) << "\n";
	ost << "\tLevel(G)\t" << KWContinuous::ContinuousToString(GetGranularizedLevel()) << "\n";
	ost << "\tBins\t" << KWContinuous::ContinuousToString(ComputeLargeScaleTotalBinLength()) << "\n";
	ost << "\tOutlier data subsets\t" << GetOutlierDataSubsetNumber() << "\n";
	ost << "\tOutlier boundary intervals\t" << GetOutlierBoundaryIntervalNumber() << "\n";
	ost << "\tOutlier PICH intervals\t" << ComputePICHIntervalNumber() << "\n";
}

boolean MHGenumHistogram::CheckIntervalType(const MHHistogramInterval* interval) const
{
	require(interval != NULL);
	return cast(MHGenumHistogramInterval*, interval) != NULL;
}

//////////////////////////////////////////////////////////
// Classe MHGenumHistogram

MHGenumHistogramInterval::MHGenumHistogramInterval()
{
	nBinLength = 0;
	nDataSubsetIndex = 0;
	bPreviousDataSubsetBoundary = false;
	bPICH = false;
	nPICHSplitIndex = 0;
}

MHGenumHistogramInterval::~MHGenumHistogramInterval() {}

void MHGenumHistogramInterval::SetBinLength(int nValue)
{
	require(nValue >= 0);
	nBinLength = nValue;
}

int MHGenumHistogramInterval::GetBinLength() const
{
	return nBinLength;
}

void MHGenumHistogramInterval::SetDataSubsetIndex(int nValue)
{
	require(nValue >= 0);
	nDataSubsetIndex = nValue;
}

int MHGenumHistogramInterval::GetDataSubsetIndex() const
{
	return nDataSubsetIndex;
}

void MHGenumHistogramInterval::SetPreviousDataSubsetBoundary(boolean bValue)
{
	bPreviousDataSubsetBoundary = bValue;
}

boolean MHGenumHistogramInterval::GetPreviousDataSubsetBoundary() const
{
	return bPreviousDataSubsetBoundary;
}

void MHGenumHistogramInterval::SetPICH(boolean bValue)
{
	bPICH = bValue;
}

boolean MHGenumHistogramInterval::GetPICH() const
{
	return bPICH;
}

void MHGenumHistogramInterval::SetPICHSplitIndex(int nValue)
{
	require(nValue >= 0);
	nPICHSplitIndex = nValue;
}

int MHGenumHistogramInterval::GetPICHSplitIndex() const
{
	return nPICHSplitIndex;
}

MHHistogramInterval* MHGenumHistogramInterval::Create() const
{
	return new MHGenumHistogramInterval;
}

void MHGenumHistogramInterval::CopyFrom(const MHHistogramInterval* sourceInterval)
{
	MHGenumHistogramInterval* sourceGenumInterval;

	require(sourceInterval != NULL);

	// Appel de la methode ancetre
	MHHistogramInterval::CopyFrom(sourceInterval);

	// Copie specifique
	sourceGenumInterval = cast(MHGenumHistogramInterval*, sourceInterval);
	nBinLength = sourceGenumInterval->nBinLength;
	nDataSubsetIndex = sourceGenumInterval->nDataSubsetIndex;
	bPreviousDataSubsetBoundary = sourceGenumInterval->bPreviousDataSubsetBoundary;
	bPICH = sourceGenumInterval->bPICH;
	nPICHSplitIndex = sourceGenumInterval->nPICHSplitIndex;
}

void MHGenumHistogramInterval::WriteHeaderLineReport(ostream& ost) const
{
	MHHistogramInterval::WriteHeaderLineReport(ost);
	ost << "\tBin length\tData subset\tBoundary\tPICH\tPICH split";
}

void MHGenumHistogramInterval::WriteLineReport(int nTotalFrequency, ostream& ost) const
{
	MHHistogramInterval::WriteLineReport(nTotalFrequency, ost);
	ost << "\t";
	ost << nBinLength << "\t";
	ost << nDataSubsetIndex << "\t";
	ost << bPreviousDataSubsetBoundary << "\t";
	ost << bPICH << "\t";
	ost << nPICHSplitIndex;
}

void MHGenumHistogramInterval::Write(ostream& ost) const
{
	MHHistogramInterval::Write(ost);
}

boolean MHGenumHistogramInterval::Check() const
{
	boolean bOk;

	bOk = MHHistogramInterval::Check();
	bOk = bOk and (not bPreviousDataSubsetBoundary or nDataSubsetIndex > 0);
	bOk = bOk and (not bPICH or nDataSubsetIndex > 0);
	return bOk;
}