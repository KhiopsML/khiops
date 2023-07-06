// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHMODLHistogramVector.h"

////////////////////////////////////////////////////////////////////////////
// Classe MHMODLHistogramTable

MHMODLHistogramTable::MHMODLHistogramTable()
{
	nCentralBinExponent = 0;
	nHierarchyLevel = 0;
	dMinBinLength = 0;

	// Redefinition du creator de vecteur d'effectif gere par la classe
	// (on detruit prealablement la version initialisee par la classe ancetre)
	delete kwfvFrequencyVectorCreator;
	kwfvFrequencyVectorCreator = new MHMODLHistogramVector;
}

MHMODLHistogramTable::~MHMODLHistogramTable() {}

void MHMODLHistogramTable::SetCentralBinExponent(int nValue)
{
	nCentralBinExponent = nValue;
}

int MHMODLHistogramTable::GetCentralBinExponent() const
{
	return nCentralBinExponent;
}

void MHMODLHistogramTable::SetHierarchyLevel(int nValue)
{
	require(nValue >= 0);
	nHierarchyLevel = nValue;
}

int MHMODLHistogramTable::GetHierarchyLevel() const
{
	return nHierarchyLevel;
}

void MHMODLHistogramTable::SetMinBinLength(double dValue)
{
	require(dValue >= 0);
	dMinBinLength = dValue;
}

double MHMODLHistogramTable::GetMinBinLength() const
{
	return dMinBinLength;
}

Continuous MHMODLHistogramTable::GetMinLowerBound() const
{
	if (GetFrequencyVectorNumber() == 0)
		return KWContinuous::GetMissingValue();
	else
		return cast(MHMODLHistogramVector*, GetFrequencyVectorAt(0))->GetLowerBound();
}

Continuous MHMODLHistogramTable::GetMaxUpperBound() const
{
	if (GetFrequencyVectorNumber() == 0)
		return KWContinuous::GetMissingValue();
	else
		return cast(MHMODLHistogramVector*, GetFrequencyVectorAt(GetFrequencyVectorNumber() - 1))
		    ->GetUpperBound();
}

void MHMODLHistogramTable::Write(ostream& ost) const
{
	// Donnees specifiques
	ost << "Central bin exponent\t" << nCentralBinExponent << "\n";
	ost << "Hierarchy level\t" << nHierarchyLevel << "\n";
	ost << "Min bin length\t" << dMinBinLength << "\n";

	// Appel de la methode ancetre
	KWFrequencyTable::Write(ost);
}

////////////////////////////////////////////////////////////////////////////
// Classe MHMODLHistogramVector

MHMODLHistogramVector::MHMODLHistogramVector()
{
	nFrequency = 0;
	cLowerBound = 0;
	cUpperBound = 0;
}

MHMODLHistogramVector::~MHMODLHistogramVector() {}

KWFrequencyVector* MHMODLHistogramVector::Create() const
{
	return new MHMODLHistogramVector;
}

void MHMODLHistogramVector::CopyFrom(const KWFrequencyVector* kwfvSource)
{
	const MHMODLHistogramVector* kwhiSource;

	require(kwfvSource != NULL);

	// Appel de la methode ancetre
	KWFrequencyVector::CopyFrom(kwfvSource);

	// Cast du vecteur source dans le type de la sous-classe
	kwhiSource = cast(MHMODLHistogramVector*, kwfvSource);

	// Recopie des caracteristiques de l'intervalle
	nFrequency = kwhiSource->nFrequency;
	cLowerBound = kwhiSource->cLowerBound;
	cUpperBound = kwhiSource->cUpperBound;
}

KWFrequencyVector* MHMODLHistogramVector::Clone() const
{
	MHMODLHistogramVector* kwfvClone;

	kwfvClone = new MHMODLHistogramVector;
	kwfvClone->CopyFrom(this);
	return kwfvClone;
}

int MHMODLHistogramVector::ComputeTotalFrequency() const
{
	return nFrequency;
}

void MHMODLHistogramVector::WriteHeaderLineReport(ostream& ost) const
{
	ost << "Frequency\tLower bound\tUpper bound";
}

void MHMODLHistogramVector::WriteLineReport(ostream& ost) const
{
	ost << nFrequency << "\t" << cLowerBound << "\t" << cUpperBound;
}

void MHMODLHistogramVector::Write(ostream& ost) const
{
	WriteLineReport(ost);
}

const ALString MHMODLHistogramVector::GetClassLabel() const
{
	return "Histogram interval (fp)";
}