// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHHistogramVector_fp.h"

////////////////////////////////////////////////////////////////////////////
// Classe MHHistogramTable_fp

MHHistogramTable_fp::MHHistogramTable_fp()
{
	nCentralBinExponent = 0;
	nHierarchyLevel = 0;
	dMinBinLength = 0;

	// Redefinition du creator de vecteur d'effectif gere par la classe
	// (on detruit prealablement la version initialisee par la classe ancetre)
	delete kwfvFrequencyVectorCreator;
	kwfvFrequencyVectorCreator = new MHHistogramVector_fp;
}

MHHistogramTable_fp::~MHHistogramTable_fp() {}

void MHHistogramTable_fp::SetCentralBinExponent(int nValue)
{
	nCentralBinExponent = nValue;
}

int MHHistogramTable_fp::GetCentralBinExponent() const
{
	return nCentralBinExponent;
}

void MHHistogramTable_fp::SetHierarchyLevel(int nValue)
{
	require(nValue >= 0);
	nHierarchyLevel = nValue;
}

int MHHistogramTable_fp::GetHierarchyLevel() const
{
	return nHierarchyLevel;
}

void MHHistogramTable_fp::SetMinBinLength(double dValue)
{
	require(dValue >= 0);
	dMinBinLength = dValue;
}

double MHHistogramTable_fp::GetMinBinLength() const
{
	return dMinBinLength;
}

Continuous MHHistogramTable_fp::GetMinLowerBound() const
{
	if (GetFrequencyVectorNumber() == 0)
		return KWContinuous::GetMissingValue();
	else
		return cast(MHHistogramVector_fp*, GetFrequencyVectorAt(0))->GetLowerBound();
}

Continuous MHHistogramTable_fp::GetMaxUpperBound() const
{
	if (GetFrequencyVectorNumber() == 0)
		return KWContinuous::GetMissingValue();
	else
		return cast(MHHistogramVector_fp*, GetFrequencyVectorAt(GetFrequencyVectorNumber() - 1))
		    ->GetUpperBound();
}

////////////////////////////////////////////////////////////////////////////
// Classe MHHistogramVector_fp

MHHistogramVector_fp::MHHistogramVector_fp()
{
	nFrequency = 0;
	cLowerBound = 0;
	cUpperBound = 0;
}

MHHistogramVector_fp::~MHHistogramVector_fp() {}

KWFrequencyVector* MHHistogramVector_fp::Create() const
{
	return new MHHistogramVector_fp;
}

void MHHistogramVector_fp::CopyFrom(const KWFrequencyVector* kwfvSource)
{
	const MHHistogramVector_fp* kwhiSource;

	require(kwfvSource != NULL);

	// Appel de la methode ancetre
	KWFrequencyVector::CopyFrom(kwfvSource);

	// Cast du vecteur source dans le type de la sous-classe
	kwhiSource = cast(MHHistogramVector_fp*, kwfvSource);

	// Recopie des caracteristiques de l'intervalle
	nFrequency = kwhiSource->nFrequency;
	cLowerBound = kwhiSource->cLowerBound;
	cUpperBound = kwhiSource->cUpperBound;
}

KWFrequencyVector* MHHistogramVector_fp::Clone() const
{
	MHHistogramVector_fp* kwfvClone;

	kwfvClone = new MHHistogramVector_fp;
	kwfvClone->CopyFrom(this);
	return kwfvClone;
}

int MHHistogramVector_fp::ComputeTotalFrequency() const
{
	return nFrequency;
}

void MHHistogramVector_fp::WriteHeaderLineReport(ostream& ost) const
{
	ost << "Frequency\tLower bound\tUpper bound";
}

void MHHistogramVector_fp::WriteLineReport(ostream& ost) const
{
	ost << nFrequency << "\t" << cLowerBound << "\t" << cUpperBound;
}

void MHHistogramVector_fp::Write(ostream& ost) const
{
	WriteLineReport(ost);
}

const ALString MHHistogramVector_fp::GetClassLabel() const
{
	return "Histogram interval (fp)";
}
