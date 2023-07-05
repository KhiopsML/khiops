// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHHistogram.h"

//////////////////////////////////////////////////////////
// Classe MHHistogram

MHHistogram::MHHistogram()
{
	Clean();
}

MHHistogram::~MHHistogram()
{
	oaIntervals.DeleteAll();
}

int MHHistogram::GetIntervalNumber() const
{
	return oaIntervals.GetSize();
}

MHHistogramInterval* MHHistogram::GetIntervalAt(int nIndex)
{
	return cast(MHHistogramInterval*, oaIntervals.GetAt(nIndex));
}

const MHHistogramInterval* MHHistogram::GetConstIntervalAt(int nIndex) const
{
	return cast(const MHHistogramInterval*, oaIntervals.GetAt(nIndex));
}

ObjectArray* MHHistogram::GetIntervals()
{
	return &oaIntervals;
}

int MHHistogram::ComputeEmptyIntervalNumber() const
{
	int nNumber;
	MHHistogramInterval* interval;
	int n;

	// Calcul du nombre d'intervalles vides
	nNumber = 0;
	for (n = 0; n < oaIntervals.GetSize(); n++)
	{
		interval = cast(MHHistogramInterval*, oaIntervals.GetAt(n));
		if (interval->GetFrequency() == 0)
			nNumber++;
	}
	return nNumber;
}

int MHHistogram::ComputeSingletonIntervalNumber() const
{
	int nNumber;
	MHHistogramInterval* interval;
	int n;

	// Calcul du nombre d'intervalles singletons
	nNumber = 0;
	for (n = 0; n < oaIntervals.GetSize(); n++)
	{
		interval = cast(MHHistogramInterval*, oaIntervals.GetAt(n));
		if (interval->GetFrequency() == 1)
			nNumber++;
	}
	return nNumber;
}

int MHHistogram::ComputeSingleValueIntervalNumber() const
{
	int nNumber;
	MHHistogramInterval* interval;
	int n;

	// Calcul du nombre d'intervalles avec valeur unique
	nNumber = 0;
	for (n = 0; n < oaIntervals.GetSize(); n++)
	{
		interval = cast(MHHistogramInterval*, oaIntervals.GetAt(n));
		if (interval->GetFrequency() > 0 and (interval->GetLowerValue() == interval->GetUpperValue()))
			nNumber++;
	}
	return nNumber;
}

int MHHistogram::ComputePICHIntervalNumber() const
{
	int nNumber;
	MHHistogramInterval* interval;
	int n;

	// Calcul du nombre d'intervalles PICH
	nNumber = 0;
	for (n = 0; n < oaIntervals.GetSize(); n++)
	{
		interval = cast(MHHistogramInterval*, oaIntervals.GetAt(n));
		if (interval->GetPICH())
			nNumber++;
	}
	return nNumber;
}

int MHHistogram::ComputeSingularIntervalNumber() const
{
	int nNumber;
	int n;

	// Calcul du nombre d'intervalles singuliers
	nNumber = 0;
	for (n = 0; n < oaIntervals.GetSize(); n++)
	{
		if (IsSingularIntervalAt(n))
			nNumber++;
	}
	return nNumber;
}

boolean MHHistogram::IsSingularIntervalAt(int nIntervalIndex) const
{
	boolean bIsSingular;

	require(0 <= nIntervalIndex and nIntervalIndex < GetIntervalNumber());

	// Il doit y avoir au moins trois intervalles
	// Avec deux intervalles, il ne peut y avoir un intervalle vide en debut ou fin
	bIsSingular = nIntervalIndex > 0 and nIntervalIndex < GetIntervalNumber() - 1;

	// Il doit etre non vide et ne comporter qu'une seul valeur
	bIsSingular = bIsSingular and GetConstIntervalAt(nIntervalIndex)->GetFrequency() > 0;
	bIsSingular = bIsSingular and GetConstIntervalAt(nIntervalIndex)->IsSingleValue();

	// Les intervalles precedent et suivant doivent etre vide
	bIsSingular = bIsSingular and GetConstIntervalAt(nIntervalIndex - 1)->GetFrequency() == 0;
	bIsSingular = bIsSingular and GetConstIntervalAt(nIntervalIndex + 1)->GetFrequency() == 0;
	return bIsSingular;
}

int MHHistogram::ComputeSpikeIntervalNumber() const
{
	int nNumber;
	int n;

	// Calcul du nombre d'intervalles a motif discret
	nNumber = 0;
	for (n = 0; n < oaIntervals.GetSize(); n++)
	{
		if (IsSpikeIntervalAt(n))
			nNumber++;
	}
	return nNumber;
}

boolean MHHistogram::IsSpikeIntervalAt(int nIntervalIndex) const
{
	boolean bIsSpike;

	require(0 <= nIntervalIndex and nIntervalIndex < GetIntervalNumber());

	// Il doit etre de type peak etsingleton
	bIsSpike = GetConstIntervalAt(nIntervalIndex)->IsSingleValue() and IsPeakIntervalAt(nIntervalIndex);

	return bIsSpike;
}

int MHHistogram::ComputePeakIntervalNumber() const
{
	int nNumber;
	int n;

	// Calcul du nombre d'intervalles a motif discret
	nNumber = 0;
	for (n = 0; n < oaIntervals.GetSize(); n++)
	{
		if (IsPeakIntervalAt(n))
			nNumber++;
	}
	return nNumber;
}

boolean MHHistogram::IsPeakIntervalAt(int nIntervalIndex) const
{

	boolean bIsSingular;

	require(0 <= nIntervalIndex and nIntervalIndex < GetIntervalNumber());

	// Il doit y avoir au moins trois intervalles
	// Avec deux intervalles, il ne peut y avoir un intervalle vide en debut ou fin
	bIsSingular = nIntervalIndex > 0 and nIntervalIndex < GetIntervalNumber() - 1;

	// Il doit etre non vide
	bIsSingular = bIsSingular and GetConstIntervalAt(nIntervalIndex)->GetFrequency() > 0;

	// Les intervalles precedent et suivant doivent etre moins denses
	bIsSingular = bIsSingular and GetConstIntervalAt(nIntervalIndex - 1)->GetFrequency() /
					      GetConstIntervalAt(nIntervalIndex - 1)->GetLength() <
					  GetConstIntervalAt(nIntervalIndex)->GetFrequency() /
					      GetConstIntervalAt(nIntervalIndex)->GetLength();
	bIsSingular = bIsSingular and GetConstIntervalAt(nIntervalIndex + 1)->GetFrequency() /
					      GetConstIntervalAt(nIntervalIndex + 1)->GetLength() <
					  GetConstIntervalAt(nIntervalIndex)->GetFrequency() /
					      GetConstIntervalAt(nIntervalIndex)->GetLength();
	return bIsSingular;
}

int MHHistogram::ComputeTotalFrequency() const
{
	int nTotalFrequency;
	int n;
	MHHistogramInterval* interval;

	// calcul de l'effectif total
	nTotalFrequency = 0;
	for (n = 0; n < oaIntervals.GetSize(); n++)
	{
		interval = cast(MHHistogramInterval*, oaIntervals.GetAt(n));
		nTotalFrequency += interval->GetFrequency();
	}
	return nTotalFrequency;
}

int MHHistogram::ComputeTotalBinLength() const
{
	int nTotalBinLength;
	int n;
	MHHistogramInterval* interval;

	// Calcul de l'effectif total
	nTotalBinLength = 0;
	for (n = 0; n < oaIntervals.GetSize(); n++)
	{
		interval = cast(MHHistogramInterval*, oaIntervals.GetAt(n));
		nTotalBinLength += interval->GetBinLength();
	}
	return nTotalBinLength;
}

double MHHistogram::ComputeLargeScaleTotalBinLength() const
{
	double dTotalBinLength;
	int n;
	MHHistogramInterval* interval;

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
			interval = cast(MHHistogramInterval*, oaIntervals.GetAt(n));
			dTotalBinLength += interval->GetBinLength();
		}
	}
	return dTotalBinLength;
}

void MHHistogram::SetDistinctValueNumber(int nValue)
{
	require(nValue >= 0);
	nDistinctValueNumber = nValue;
}

int MHHistogram::GetDistinctValueNumber() const
{
	return nDistinctValueNumber;
}

Continuous MHHistogram::GetMinValue() const
{
	if (GetIntervalNumber() == 0)
		return KWContinuous::GetMinValue();
	else
		return GetConstIntervalAt(0)->GetLowerValue();
}

Continuous MHHistogram::GetMaxValue() const
{
	if (GetIntervalNumber() == 0)
		return KWContinuous::GetMaxValue();
	else
		return GetConstIntervalAt(GetIntervalNumber() - 1)->GetUpperValue();
}

void MHHistogram::SetHistogramCriterion(const ALString& sValue)
{
	sHistogramCriterion = sValue;
}

const ALString& MHHistogram::GetHistogramCriterion() const
{
	return sHistogramCriterion;
}

void MHHistogram::SetCoarseningIndex(int nValue)
{
	require(nValue >= 0);
	nCoarseningIndex = nValue;
}

int MHHistogram::GetCoarseningIndex() const
{
	return nCoarseningIndex;
}

void MHHistogram::SetTruncationEpsilon(double dValue)
{
	require(dValue >= 0);
	dTruncationEpsilon = dValue;
}

double MHHistogram::GetTruncationEpsilon() const
{
	return dTruncationEpsilon;
}

void MHHistogram::SetRemovedSingularIntervalsNumber(int nValue)
{
	nRemovedSingularIntervalsNumber = nValue;
}

int MHHistogram::GetRemovedSingularIntervalsNumber() const
{
	return nRemovedSingularIntervalsNumber;
}

void MHHistogram::SetGranularity(int nValue)
{
	require(nValue >= 0);
	nGranularity = nValue;
}

int MHHistogram::GetGranularity() const
{
	return nGranularity;
}

void MHHistogram::SetNullCost(double dValue)
{
	require(dValue >= 0);
	dNullCost = dValue;
}

double MHHistogram::GetNullCost() const
{
	return dNullCost;
}

void MHHistogram::SetReferenceNullCost(double dValue)
{
	require(dValue >= 0);
	dReferenceNullCost = dValue;
}

double MHHistogram::GetReferenceNullCost() const
{
	return dReferenceNullCost;
}

void MHHistogram::SetCost(double dValue)
{
	require(dValue >= 0);
	dCost = dValue;
}

double MHHistogram::GetCost() const
{
	return dCost;
}

void MHHistogram::SetPartitionCost(double dValue)
{
	require(dValue >= 0);
	dPartitionCost = dValue;
}

double MHHistogram::GetPartitionCost() const
{
	return dPartitionCost;
}

double MHHistogram::GetGranularizedNullCost() const
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

double MHHistogram::GetGranularizedCost() const
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

double MHHistogram::GetLevel() const
{
	if (GetNullCost() == 0)
		return 0;
	else
		return 1 - GetCost() / GetNullCost();
}

double MHHistogram::GetGranularizedLevel() const
{
	double dGranularizedLevel;
	double dRefNullCost;

	// Cas des histogrammes standard
	if (GetHistogramCriterion() == "G-Enum")
	{
		if (GetGranularizedNullCost() == 0)
			return 0;
		else
			return 1 - GetGranularizedCost() / GetGranularizedNullCost();
	}
	// Cas des histogrammes avec representation virgule flotante
	else if (GetHistogramCriterion() == "G-Enum-fp")
	{
		// Cout du model null sur une plafge de valeur de reference [1, 2] avec 2^30 mantissa bins
		dRefNullCost =
		    2 * KWStat::NaturalNumbersUniversalCodeLength(1) + ComputeTotalFrequency() * 30.0 * log(2);

		// Tentative heuristique de critere normalise
		return (GetNullCost() - GetCost()) / GetReferenceNullCost();
	}
	// Cas autre
	else
		dGranularizedLevel = GetLevel();
	return dGranularizedLevel;
}

void MHHistogram::SetComputationTime(double dValue)
{
	require(dValue >= 0);
	dComputationTime = dValue;
}

double MHHistogram::GetComputationTime() const
{
	return dComputationTime;
}

void MHHistogram::SetDomainLowerBound(Continuous cValue)
{
	cDomainLowerBound = cValue;
}

Continuous MHHistogram::GetDomainLowerBound() const
{
	return cDomainLowerBound;
}

void MHHistogram::SetDomainUpperBound(Continuous cValue)
{
	cDomainUpperBound = cValue;
}

Continuous MHHistogram::GetDomainUpperBound() const
{
	return cDomainUpperBound;
}

void MHHistogram::SetDomainBoundsMantissaBitNumber(int nValue)
{
	require(nValue >= 0);
	nDomainBoundsMantissaBitNumber = nValue;
}

int MHHistogram::GetDomainBoundsMantissaBitNumber() const
{
	return nDomainBoundsMantissaBitNumber;
}

void MHHistogram::SetHierarchyLevel(int nValue)
{
	require(nValue >= 0);
	nHierarchyLevel = nValue;
}

int MHHistogram::GetHierarchyLevel() const
{
	return nHierarchyLevel;
}

void MHHistogram::SetMainBinHierarchyRootLevel(int nValue)
{
	nMainBinHierarchyRootLevel = nValue;
}

int MHHistogram::GetMainBinHierarchyRootLevel() const
{
	return nMainBinHierarchyRootLevel;
}

void MHHistogram::SetMaxHierarchyLevel(int nValue)
{
	require(nValue >= 0);
	nMaxHierarchyLevel = nValue;
}

int MHHistogram::GetMaxHierarchyLevel() const
{
	return nMaxHierarchyLevel;
}

void MHHistogram::SetMaxSafeHierarchyLevel(int nValue)
{
	require(nValue >= 0);
	nMaxSafeHierarchyLevel = nValue;
}

int MHHistogram::GetMaxSafeHierarchyLevel() const
{
	return nMaxSafeHierarchyLevel;
}

void MHHistogram::SetCentralBinExponent(int nValue)
{
	nCentralBinExponent = nValue;
}

int MHHistogram::GetCentralBinExponent() const
{
	return nCentralBinExponent;
}

void MHHistogram::SetMinCentralBinExponent(int nValue)
{
	nMinCentralBinExponent = nValue;
}

int MHHistogram::GetMinCentralBinExponent() const
{
	return nMinCentralBinExponent;
}

void MHHistogram::SetMaxCentralBinExponent(int nValue)
{
	nMaxCentralBinExponent = nValue;
}

int MHHistogram::GetMaxCentralBinExponent() const
{
	return nMaxCentralBinExponent;
}

void MHHistogram::SetMainBinNumber(int nValue)
{
	require(nValue >= 0);
	nMainBinNumber = nValue;
}

int MHHistogram::GetMainBinNumber() const
{
	return nMainBinNumber;
}

void MHHistogram::SetMinBinLength(double dValue)
{
	require(dValue >= 0);
	dMinBinLength = dValue;
}

double MHHistogram::GetMinBinLength() const
{
	return dMinBinLength;
}

int MHHistogram::GetOutlierDataSubsetNumber() const
{
	if (GetIntervalNumber() == 0)
		return 0;
	else if (GetConstIntervalAt(0)->GetDataSubsetIndex() == 0)
		return 0;
	else
		return GetConstIntervalAt(GetIntervalNumber() - 1)->GetDataSubsetIndex() -
		       GetConstIntervalAt(0)->GetDataSubsetIndex() + 1;
}

int MHHistogram::GetOutlierBoundaryIntervalNumber() const
{
	int nOutlierBoundaryIntervalNumber;
	int n;
	MHHistogramInterval* interval;

	// calcul de l'effectif total
	nOutlierBoundaryIntervalNumber = 0;
	for (n = 0; n < oaIntervals.GetSize(); n++)
	{
		interval = cast(MHHistogramInterval*, oaIntervals.GetAt(n));
		if (interval->GetPreviousDataSubsetBoundary())
			nOutlierBoundaryIntervalNumber++;
	}
	return nOutlierBoundaryIntervalNumber;
}

void MHHistogram::Clean()
{
	nDistinctValueNumber = 0;
	cDomainLowerBound = 0;
	cDomainUpperBound = 0;
	nDomainBoundsMantissaBitNumber = 0;
	nCoarseningIndex = 0;
	dTruncationEpsilon = 0;
	nRemovedSingularIntervalsNumber = 0;
	nGranularity = 0;
	dMinBinLength = 0;
	dNullCost = 0;
	dReferenceNullCost = 0;
	dCost = 0;
	dPartitionCost = 0;
	dComputationTime = 0;
	nHierarchyLevel = 0;
	nMainBinHierarchyRootLevel = 0;
	nMaxHierarchyLevel = 0;
	nMaxSafeHierarchyLevel = 0;
	nCentralBinExponent = 0;
	nMinCentralBinExponent = 0;
	nMaxCentralBinExponent = 0;
	nMainBinNumber = 0;
	dMinBinLength = 0;
	oaIntervals.DeleteAll();
}

void MHHistogram::CopyFrom(const MHHistogram* sourceHistogram)
{
	int i;

	require(sourceHistogram != NULL);

	// Recopie des caracteristique generales
	nDistinctValueNumber = sourceHistogram->GetDistinctValueNumber();
	sHistogramCriterion = sourceHistogram->sHistogramCriterion;
	cDomainLowerBound = sourceHistogram->cDomainLowerBound;
	cDomainUpperBound = sourceHistogram->cDomainUpperBound;
	nDomainBoundsMantissaBitNumber = sourceHistogram->nDomainBoundsMantissaBitNumber;
	nCoarseningIndex = sourceHistogram->nCoarseningIndex;
	dTruncationEpsilon = sourceHistogram->dTruncationEpsilon;
	nRemovedSingularIntervalsNumber = sourceHistogram->nRemovedSingularIntervalsNumber;
	nGranularity = sourceHistogram->nGranularity;
	dNullCost = sourceHistogram->dNullCost;
	dReferenceNullCost = sourceHistogram->dReferenceNullCost;
	dCost = sourceHistogram->dCost;
	dPartitionCost = sourceHistogram->dPartitionCost;
	dComputationTime = sourceHistogram->dComputationTime;

	// Recopie des caracteristique des histogrammes a virgule flottante
	nHierarchyLevel = sourceHistogram->nHierarchyLevel;
	nMainBinHierarchyRootLevel = sourceHistogram->nMainBinHierarchyRootLevel;
	nMaxHierarchyLevel = sourceHistogram->nMaxHierarchyLevel;
	nMaxSafeHierarchyLevel = sourceHistogram->nMaxSafeHierarchyLevel;
	nCentralBinExponent = sourceHistogram->nCentralBinExponent;
	nMinCentralBinExponent = sourceHistogram->nMinCentralBinExponent;
	nMaxCentralBinExponent = sourceHistogram->nMaxCentralBinExponent;
	nMainBinNumber = sourceHistogram->nMainBinNumber;
	dMinBinLength = sourceHistogram->dMinBinLength;

	// Recopie des intervalles
	oaIntervals.DeleteAll();
	for (i = 0; i < sourceHistogram->GetIntervalNumber(); i++)
		oaIntervals.Add(sourceHistogram->GetConstIntervalAt(i)->Clone());
}

MHHistogram* MHHistogram::Clone() const
{
	MHHistogram* cloneHistogram;

	cloneHistogram = new MHHistogram;
	cloneHistogram->CopyFrom(this);
	return cloneHistogram;
}

void MHHistogram::Write(ostream& ost) const
{
	int nTotalFrequency;
	int n;
	MHHistogramInterval* interval;

	// Titre
	ost << GetHistogramCriterion() << " histogram\n";

	// Information sur le modele
	ost << "\tDomain lower bound\t" << KWContinuous::ContinuousToString(GetDomainLowerBound()) << "\n";
	ost << "\tDomain upper bound\t" << KWContinuous::ContinuousToString(GetDomainUpperBound()) << "\n";
	ost << "\tDomain bounds mantissa bits\t" << GetDomainBoundsMantissaBitNumber() << "\n";
	ost << "\tCoarsening index\t" << GetCoarseningIndex() << "\n";
	ost << "\tTruncation epsilon\t" << GetTruncationEpsilon() << "\n";
	ost << "\tRemoved singular intervals number\t" << GetRemovedSingularIntervalsNumber() << "\n";
	ost << "\tGranularity\t" << GetGranularity() << "\n";
	ost << "\tHierarchy level\t" << GetHierarchyLevel() << "\n";
	ost << "\tMain bin hierarchy root level\t" << GetMainBinHierarchyRootLevel() << "\n";
	ost << "\tMax hierarchy level\t" << GetMaxHierarchyLevel() << "\n";
	ost << "\tMax safe hierarchy level\t" << GetMaxSafeHierarchyLevel() << "\n";
	ost << "\tCentral bin exponent\t" << GetCentralBinExponent() << "\n";
	ost << "\tMin central bin exponent\t" << GetMinCentralBinExponent() << "\n";
	ost << "\tMax central bin exponent\t" << GetMaxCentralBinExponent() << "\n";
	ost << "\tMain bin number\t" << GetMainBinNumber() << "\n";
	ost << "\tMin bin length\t" << GetMinBinLength() << "\n";
	ost << "\tOutlier data subsets\t" << GetOutlierDataSubsetNumber() << "\n";
	ost << "\tOutlier boundary intervals\t" << GetOutlierBoundaryIntervalNumber() << "\n";

	// Statistiques sur les intervalles
	ost << "\tIntervals\t" << GetIntervalNumber() << "\n";
	ost << "\tEmpty intervals\t" << ComputeEmptyIntervalNumber() << "\n";
	ost << "\tSingleton intervals\t" << ComputeSingletonIntervalNumber() << "\n";
	ost << "\tSingle value intervals\t" << ComputeSingleValueIntervalNumber() << "\n";
	ost << "\tSingular intervals\t" << ComputeSingularIntervalNumber() << "\n";
	ost << "\tSpike intervals\t" << ComputeSpikeIntervalNumber() << "\n";
	ost << "\tPeak intervals\t" << ComputePeakIntervalNumber() << "\n";
	ost << "\tOutlier PICH intervals\t" << ComputePICHIntervalNumber() << "\n";

	// Informations sur le jeu de donnees
	ost << "\tInstances\t" << ComputeTotalFrequency() << "\n";
	ost << "\tValues\t" << GetDistinctValueNumber() << "\n";
	ost << "\tBins\t" << KWContinuous::ContinuousToString(ComputeLargeScaleTotalBinLength()) << "\n";
	ost << "\tMin\t" << KWContinuous::ContinuousToString(GetMinValue()) << "\n";
	ost << "\tMax\t" << KWContinuous::ContinuousToString(GetMaxValue()) << "\n";

	// Informations sur les couts
	ost << "\tNull cost\t" << GetNullCost() << "\n";
	ost << "\tReference null cost\t" << GetReferenceNullCost() << "\n";
	ost << "\tCost\t" << GetCost() << "\n";
	ost << "\tLevel\t" << GetLevel() << "\n";
	ost << "\tNull cost(G)\t" << GetGranularizedNullCost() << "\n";
	ost << "\tCost(G)\t" << GetGranularizedCost() << "\n";
	ost << "\tLevel(G)\t" << GetGranularizedLevel() << "\n";
	ost << "\tPartition cost\t" << GetPartitionCost() << "\n";
	ost << "\tComputation time\t" << GetComputationTime() << "\n";
	ost << "\n";

	// Calcul de l'effectif total
	nTotalFrequency = ComputeTotalFrequency();

	// Parcours des intervalle pour les ecrire
	for (n = 0; n < oaIntervals.GetSize(); n++)
	{
		interval = cast(MHHistogramInterval*, oaIntervals.GetAt(n));

		// Ecriture de l'entete
		if (n == 0)
		{
			interval->WriteHeaderLineReport(ost);
			ost << "\n";
		}

		// Eciture d'un ligne
		interval->WriteLineReport(nTotalFrequency, ost);
		ost << "\n";
	}
}

void MHHistogram::WriteFile(const ALString& sFileName) const
{
	fstream fstOutput;
	boolean bOk;

	bOk = FileService::OpenOutputFile(sFileName, fstOutput);
	if (bOk)
	{
		Write(fstOutput);
		FileService::CloseOutputFile(sFileName, fstOutput);
	}
}

boolean MHHistogram::Check() const
{
	boolean bOk = true;
	int n;
	MHHistogramInterval* interval;
	MHHistogramInterval* previousInterval;
	ALString sTmp;

	// Verification de l'integrite des intervalles et de leur adjacence
	for (n = 0; n < oaIntervals.GetSize(); n++)
	{
		interval = cast(MHHistogramInterval*, oaIntervals.GetAt(n));
		bOk = interval->Check();
		if (not bOk)
			AddError(sTmp + "Wrong specification of interval " + IntToString(n + 1) + " " +
				 interval->GetObjectLabel());
		if (bOk and n > 0)
		{
			previousInterval = cast(MHHistogramInterval*, oaIntervals.GetAt(n - 1));
			bOk = interval->GetLowerBound() == previousInterval->GetUpperBound();
			if (not bOk)
				AddError(sTmp + "Upper bound of interval " + IntToString(n) + " (" +
					 KWContinuous::ContinuousToString(previousInterval->GetUpperBound()) +
					 ") is different from the lower bound of interval " + IntToString(n + 1) +
					 " (" + KWContinuous::ContinuousToString(interval->GetLowerBound()) + ")");
		}
		if (not bOk)
			break;
	}
	return bOk;
}

boolean MHHistogram::CheckValues(const ContinuousVector* cvValues) const
{
	boolean bOk = true;
	int n;
	int nTotalFrequency;
	MHHistogramInterval* interval;
	Continuous cLowerValue;
	Continuous cUpperValue;
	ALString sTmp;

	require(cvValues != NULL);

	// Verification du nombre total de valeurs
	nTotalFrequency = ComputeTotalFrequency();
	if (nTotalFrequency != cvValues->GetSize())
	{
		bOk = false;
		AddError(sTmp + "Number of values (" + IntToString(cvValues->GetSize()) +
			 ") is different from the totla frequency of the histogram (" + +IntToString(nTotalFrequency) +
			 ")");
	}
	// Verification des valeurs portees par les intervalles
	else
	{
		nTotalFrequency = 0;
		for (n = 0; n < oaIntervals.GetSize(); n++)
		{
			interval = cast(MHHistogramInterval*, oaIntervals.GetAt(n));

			// Acces aux valeurs a verifier
			if (interval->GetFrequency() > 0)
			{
				cLowerValue = cvValues->GetAt(nTotalFrequency);
				if (interval->GetFrequency() > 0)
					cUpperValue = cvValues->GetAt(nTotalFrequency + interval->GetFrequency() - 1);
				else
					cUpperValue = cLowerValue;
			}
			else
			{
				cLowerValue = KWContinuous::GetMissingValue();
				cUpperValue = KWContinuous::GetMissingValue();
			}

			// Mise a jour de l'effectif total
			nTotalFrequency += interval->GetFrequency();

			// Verification de la valeur inferieure de l'intervalle
			if (bOk)
			{
				bOk = interval->GetLowerValue() == cLowerValue;
				if (not bOk)
					AddError(sTmp + "Lower value of interval " + IntToString(n + 1) + " (" +
						 KWContinuous::ContinuousToString(interval->GetLowerValue()) +
						 ") is different from the expected lower value (" +
						 KWContinuous::ContinuousToString(cLowerValue) + ")");
			}

			// Verification de la valeur superieure de l'intervalle
			if (bOk)
			{
				bOk = interval->GetUpperValue() == cUpperValue;
				if (not bOk)
					AddError(sTmp + "Upper value of interval " + IntToString(n + 1) + " (" +
						 KWContinuous::ContinuousToString(interval->GetUpperValue()) +
						 ") is different from the expected upper value (" +
						 KWContinuous::ContinuousToString(cUpperValue) + ")");
			}
			if (not bOk)
				break;
		}
	}
	return bOk;
}

const ALString MHHistogram::GetClassLabel() const
{
	return "Histogram";
}

const ALString MHHistogram::GetObjectLabel() const
{
	return "";
}

//////////////////////////////////////////////////////////
// Classe MHHistogramInterval

MHHistogramInterval::MHHistogramInterval()
{
	cLowerBound = 0;
	cUpperBound = 0;
	nFrequency = 0;
	nBinLength = 0;
	cLowerValue = 0;
	cUpperValue = 0;
	nDataSubsetIndex = 0;
	bPreviousDataSubsetBoundary = false;
	bPICH = false;
	nPICHSplitIndex = 0;
	dCost = 0;
}

MHHistogramInterval::~MHHistogramInterval() {}

void MHHistogramInterval::SetLowerBound(Continuous cValue)
{
	cLowerBound = cValue;
}

Continuous MHHistogramInterval::GetLowerBound() const
{
	return cLowerBound;
}

void MHHistogramInterval::SetUpperBound(Continuous cValue)
{
	cUpperBound = cValue;
}

Continuous MHHistogramInterval::GetUpperBound() const
{
	return cUpperBound;
}

void MHHistogramInterval::SetFrequency(int nValue)
{
	require(nValue >= 0);
	nFrequency = nValue;
}

int MHHistogramInterval::GetFrequency() const
{
	return nFrequency;
}

void MHHistogramInterval::SetLowerValue(Continuous cValue)
{
	cLowerValue = cValue;
}

Continuous MHHistogramInterval::GetLowerValue() const
{
	return cLowerValue;
}

void MHHistogramInterval::SetUpperValue(Continuous cValue)
{
	cUpperValue = cValue;
}

Continuous MHHistogramInterval::GetUpperValue() const
{
	return cUpperValue;
}

boolean MHHistogramInterval::IsSingleValue() const
{
	return GetLowerValue() == GetUpperValue();
}

void MHHistogramInterval::SetBinLength(int nValue)
{
	require(nValue >= 0);
	nBinLength = nValue;
}

int MHHistogramInterval::GetBinLength() const
{
	return nBinLength;
}

void MHHistogramInterval::SetDataSubsetIndex(int nValue)
{
	require(nValue >= 0);
	nDataSubsetIndex = nValue;
}

int MHHistogramInterval::GetDataSubsetIndex() const
{
	return nDataSubsetIndex;
}

void MHHistogramInterval::SetPreviousDataSubsetBoundary(boolean bValue)
{
	bPreviousDataSubsetBoundary = bValue;
}

boolean MHHistogramInterval::GetPreviousDataSubsetBoundary() const
{
	return bPreviousDataSubsetBoundary;
}

void MHHistogramInterval::SetPICH(boolean bValue)
{
	bPICH = bValue;
}

boolean MHHistogramInterval::GetPICH() const
{
	return bPICH;
}

void MHHistogramInterval::SetPICHSplitIndex(int nValue)
{
	require(nValue >= 0);
	nPICHSplitIndex = nValue;
}

int MHHistogramInterval::GetPICHSplitIndex() const
{
	return nPICHSplitIndex;
}

void MHHistogramInterval::SetCost(double dValue)
{
	dCost = dValue;
}

double MHHistogramInterval::GetCost() const
{
	return dCost;
}

Continuous MHHistogramInterval::GetLength() const
{
	return GetUpperBound() - GetLowerBound();
}

double MHHistogramInterval::GetProbability(int nTotalFrequency) const
{
	return GetFrequency() * 1.0 / nTotalFrequency;
}

double MHHistogramInterval::GetDensity(int nTotalFrequency) const
{
	if (GetLength() > 0)
		return GetProbability(nTotalFrequency) / GetLength();
	else
		return 0;
}

int MHHistogramInterval::CompareDensity(const MHHistogramInterval* otherInterval) const
{
	int nCompare;

	// Cas ou un des intervalles est vide
	if (GetFrequency() == 0 or otherInterval->GetFrequency() == 0)
		nCompare = GetFrequency() - otherInterval->GetFrequency();
	// Cas ou les deux intervalles ne contiennent qu'une seule valeur
	else if (IsSingleValue() and otherInterval->IsSingleValue())
		nCompare = GetFrequency() - otherInterval->GetFrequency();
	// Cas chaque intervalle contient plusieurs valeurs
	else if (not IsSingleValue() and not otherInterval->IsSingleValue())
		nCompare = CompareDouble(GetFrequency() / (GetUpperValue() - GetLowerValue()),
					 otherInterval->GetFrequency() /
					     (otherInterval->GetUpperValue() - otherInterval->GetLowerValue()));
	// Cas ou seul le premier intervalle ne contient qu'une seule valeur
	else if (IsSingleValue())
	{
		// On choisit d'indiquer que le plus dense est celui ou il y a une densite
		nCompare = -1;
	}
	// Cas ou seul le second intervalle ne contient qu'une seule valeur
	else
	{
		assert(otherInterval->IsSingleValue());
		// On choisit d'indiquer que le plus dense est celui ou il y a une densite
		nCompare = 1;
	}
	return nCompare;
}

void MHHistogramInterval::CopyFrom(const MHHistogramInterval* sourceInterval)
{
	require(sourceInterval != NULL);

	cLowerBound = sourceInterval->cLowerBound;
	cUpperBound = sourceInterval->cUpperBound;
	nFrequency = sourceInterval->nFrequency;
	nBinLength = sourceInterval->nBinLength;
	cLowerValue = sourceInterval->cLowerValue;
	cUpperValue = sourceInterval->cUpperValue;
	nDataSubsetIndex = sourceInterval->nDataSubsetIndex;
	bPreviousDataSubsetBoundary = sourceInterval->bPreviousDataSubsetBoundary;
	bPICH = sourceInterval->bPICH;
	nPICHSplitIndex = sourceInterval->nPICHSplitIndex;
	dCost = sourceInterval->dCost;
}

MHHistogramInterval* MHHistogramInterval::Clone() const
{
	MHHistogramInterval* cloneInterval;

	cloneInterval = new MHHistogramInterval;
	cloneInterval->CopyFrom(this);
	return cloneInterval;
}

void MHHistogramInterval::WriteHeaderLineReport(ostream& ost) const
{
	ost << "Lower bound\tUpper bound\tFrequency\tLength\tProbability\tDensity\t";
	ost << "Lower value\tUpper value\tBin length\t";
	ost << "Data subset\tBoundary\tPICH\tPICH split\tInterval Cost";
}

void MHHistogramInterval::WriteLineReport(int nTotalFrequency, ostream& ost) const
{
	ost << KWContinuous::ContinuousToString(cLowerBound) << "\t";
	ost << KWContinuous::ContinuousToString(cUpperBound) << "\t";
	ost << nFrequency << "\t";
	ost << GetLength() << "\t";
	ost << KWContinuous::ContinuousToString(GetProbability(nTotalFrequency)) << "\t";
	if (GetDensity(nTotalFrequency) < KWContinuous::GetEpsilonValue())
		ost << GetDensity(nTotalFrequency) << "\t";
	else
		ost << KWContinuous::ContinuousToString(GetDensity(nTotalFrequency)) << "\t";
	ost << KWContinuous::ContinuousToString(cLowerValue) << "\t";
	ost << KWContinuous::ContinuousToString(cUpperValue) << "\t";
	ost << nBinLength << "\t";
	ost << nDataSubsetIndex << "\t";
	ost << bPreviousDataSubsetBoundary << "\t";
	ost << bPICH << "\t";
	ost << nPICHSplitIndex << "\t";
	ost << dCost;
}

void MHHistogramInterval::Write(ostream& ost) const
{
	ost << GetObjectLabel();
}

boolean MHHistogramInterval::CheckBounds() const
{
	boolean bOk = true;

	bOk =
	    bOk and (cLowerBound <= -KWContinuous::GetEpsilonValue() or KWContinuous::GetEpsilonValue() <= cUpperBound);
	bOk = bOk and cLowerBound < cUpperBound;
	return bOk;
}

boolean MHHistogramInterval::Check() const
{
	boolean bOk = true;

	// La borne inf peut execeptionnellement etre egale a la borne sup dans le cas d'un intervalle
	// ayant une seule valeur avec un epsilon min tres petit (value+epsilon=value)
	if (nFrequency > 0)
	{
		bOk = bOk and cLowerBound <= cLowerValue;
		bOk = bOk and cUpperValue <= cUpperBound;
	}
	else
	{
		bOk = bOk and cLowerValue == KWContinuous::GetMissingValue();
		bOk = bOk and cUpperValue == KWContinuous::GetMissingValue();
	}
	bOk =
	    bOk and (cLowerBound <= -KWContinuous::GetEpsilonValue() or KWContinuous::GetEpsilonValue() <= cUpperBound);
	bOk = bOk and cLowerBound < cUpperBound;
	bOk = bOk and cLowerValue <= cUpperValue;
	bOk = bOk and (not bPreviousDataSubsetBoundary or nDataSubsetIndex > 0);
	bOk = bOk and (not bPICH or nDataSubsetIndex > 0);
	return bOk;
}

const ALString MHHistogramInterval::GetClassLabel() const
{
	return "Histogram interval";
}

const ALString MHHistogramInterval::GetObjectLabel() const
{
	ALString sLabel;

	sLabel = "(";
	sLabel += KWContinuous::ContinuousToString(GetLowerBound());
	sLabel += ", ";
	sLabel += KWContinuous::ContinuousToString(GetUpperBound());
	sLabel += ", ";
	sLabel += IntToString(GetFrequency());
	sLabel += ")";
	return sLabel;
}