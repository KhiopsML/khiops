// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHGenumHistogramVector.h"

////////////////////////////////////////////////////////////////////////////
// Classe MHGenumHistogramVector

MHGenumHistogramVector::MHGenumHistogramVector()
{
	nFrequency = 0;
	nLength = 0;
}

MHGenumHistogramVector::~MHGenumHistogramVector() {}

KWFrequencyVector* MHGenumHistogramVector::Create() const
{
	return new MHGenumHistogramVector;
}

void MHGenumHistogramVector::CopyFrom(const KWFrequencyVector* kwfvSource)
{
	const MHGenumHistogramVector* kwhiSource;

	require(kwfvSource != NULL);

	// Appel de la methode ancetre
	KWFrequencyVector::CopyFrom(kwfvSource);

	// Cast du vecteur source dans le type de la sous-classe
	kwhiSource = cast(MHGenumHistogramVector*, kwfvSource);

	// Recopie des caracteristiques de l'intervalle
	nFrequency = kwhiSource->nFrequency;
	nLength = kwhiSource->nLength;
}

KWFrequencyVector* MHGenumHistogramVector::Clone() const
{
	MHGenumHistogramVector* kwfvClone;

	kwfvClone = new MHGenumHistogramVector;
	kwfvClone->CopyFrom(this);
	return kwfvClone;
}

int MHGenumHistogramVector::ComputeTotalFrequency() const
{
	return nFrequency;
}

void MHGenumHistogramVector::WriteHeaderLineReport(ostream& ost) const
{
	ost << "Frequency\tLength";
}

void MHGenumHistogramVector::WriteLineReport(ostream& ost) const
{
	ost << nFrequency << "\t" << nLength;
}

const ALString MHGenumHistogramVector::GetClassLabel() const
{
	return "Histogram interval";
}
