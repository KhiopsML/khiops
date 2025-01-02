// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHHistogramVector.h"

////////////////////////////////////////////////////////////////////////////
// Classe MHHistogramVector

MHHistogramVector::MHHistogramVector()
{
	nFrequency = 0;
	nLength = 0;
}

MHHistogramVector::~MHHistogramVector() {}

KWFrequencyVector* MHHistogramVector::Create() const
{
	return new MHHistogramVector;
}

void MHHistogramVector::CopyFrom(const KWFrequencyVector* kwfvSource)
{
	const MHHistogramVector* kwhiSource;

	require(kwfvSource != NULL);

	// Appel de la methode ancetre
	KWFrequencyVector::CopyFrom(kwfvSource);

	// Cast du vecteur source dans le type de la sous-classe
	kwhiSource = cast(MHHistogramVector*, kwfvSource);

	// Recopie des caracteristiques de l'intervalle
	nFrequency = kwhiSource->nFrequency;
	nLength = kwhiSource->nLength;
}

KWFrequencyVector* MHHistogramVector::Clone() const
{
	MHHistogramVector* kwfvClone;

	kwfvClone = new MHHistogramVector;
	kwfvClone->CopyFrom(this);
	return kwfvClone;
}

int MHHistogramVector::ComputeTotalFrequency() const
{
	return nFrequency;
}

void MHHistogramVector::WriteHeaderLineReport(ostream& ost) const
{
	ost << "Frequency\tLength";
}

void MHHistogramVector::WriteLineReport(ostream& ost) const
{
	ost << nFrequency << "\t" << nLength;
}

const ALString MHHistogramVector::GetClassLabel() const
{
	return "Histogram interval";
}
