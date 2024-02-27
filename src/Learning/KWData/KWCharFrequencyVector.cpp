// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWCharFrequencyVector.h"

KWCharFrequencyVector::KWCharFrequencyVector()
{
	assert(sizeof(char) == 1);
	ivCharFrequencies.SetSize(nCharNumber);
	nTotalFrequency = 0;
}

KWCharFrequencyVector::~KWCharFrequencyVector() {}

void KWCharFrequencyVector::Initialize()
{
	ivCharFrequencies.Initialize();
	nTotalFrequency = 0;
}

void KWCharFrequencyVector::InitializeFrequencies(int nFrequency)
{
	int i;

	require(nFrequency >= 0);
	require(nFrequency == 0 or nFrequency <= INT_MAX / nFrequency);

	for (i = 0; i < GetSize(); i++)
		ivCharFrequencies.SetAt(i, nFrequency);
	nTotalFrequency = nFrequency * GetSize();
}

boolean KWCharFrequencyVector::IsZero() const
{
	int i;
	for (i = 0; i < GetSize(); i++)
	{
		if (ivCharFrequencies.GetAt(i) > 0)
			return false;
	}
	return true;
}

void KWCharFrequencyVector::InitializeFromString(const ALString& sValue)
{
	Initialize();
	UpgradeFromString(sValue);
}

void KWCharFrequencyVector::UpgradeFromString(const ALString& sValue)
{
	int i;
	require(nTotalFrequency <= INT_MAX - sValue.GetLength());
	for (i = 0; i < sValue.GetLength(); i++)
		ivCharFrequencies.UpgradeAt((unsigned char)sValue.GetAt(i), 1);
	nTotalFrequency += sValue.GetLength();
}

void KWCharFrequencyVector::InitializeFromStringVector(const StringVector* svStrings)
{
	Initialize();
	UpgradeFromStringVector(svStrings);
}

void KWCharFrequencyVector::UpgradeFromStringVector(const StringVector* svStrings)
{
	int i;
	for (i = 0; i < svStrings->GetSize(); i++)
		UpgradeFromString(svStrings->GetAt(i));
}

void KWCharFrequencyVector::InitializeFromBuffer(const CharVector* cvBuffer)
{
	Initialize();
	UpgradeFromBuffer(cvBuffer);
}

void KWCharFrequencyVector::UpgradeFromBuffer(const CharVector* cvBuffer)
{
	int i;
	require(nTotalFrequency <= INT_MAX - cvBuffer->GetSize());
	for (i = 0; i < cvBuffer->GetSize(); i++)
		ivCharFrequencies.UpgradeAt((unsigned char)cvBuffer->GetAt(i), 1);
	nTotalFrequency += cvBuffer->GetSize();
}

boolean KWCharFrequencyVector::IsEqual(const KWCharFrequencyVector* cfvComparedChars) const
{
	int i;
	require(cfvComparedChars != NULL);
	if (nTotalFrequency != cfvComparedChars->nTotalFrequency)
		return false;
	for (i = 0; i < GetSize(); i++)
	{
		if (ivCharFrequencies.GetAt(i) != cfvComparedChars->ivCharFrequencies.GetAt(i))
			return false;
	}
	return true;
}

boolean KWCharFrequencyVector::IsGreater(const KWCharFrequencyVector* cfvComparedChars) const
{
	int i;
	require(cfvComparedChars != NULL);
	if (nTotalFrequency <= cfvComparedChars->nTotalFrequency)
		return false;
	for (i = 0; i < GetSize(); i++)
	{
		if (ivCharFrequencies.GetAt(i) <= cfvComparedChars->ivCharFrequencies.GetAt(i))
			return false;
	}
	return true;
}

boolean KWCharFrequencyVector::IsGreaterOrEqual(const KWCharFrequencyVector* cfvComparedChars) const
{
	int i;
	require(cfvComparedChars != NULL);
	if (nTotalFrequency < cfvComparedChars->nTotalFrequency)
		return false;
	for (i = 0; i < GetSize(); i++)
	{
		if (ivCharFrequencies.GetAt(i) < cfvComparedChars->ivCharFrequencies.GetAt(i))
			return false;
	}
	return true;
}

boolean KWCharFrequencyVector::IsSmaller(const KWCharFrequencyVector* cfvComparedChars) const
{
	int i;
	require(cfvComparedChars != NULL);
	if (nTotalFrequency >= cfvComparedChars->nTotalFrequency)
		return false;
	for (i = 0; i < GetSize(); i++)
	{
		if (ivCharFrequencies.GetAt(i) >= cfvComparedChars->ivCharFrequencies.GetAt(i))
			return false;
	}
	return true;
}

boolean KWCharFrequencyVector::IsSmallerOrEqual(const KWCharFrequencyVector* cfvComparedChars) const
{
	int i;
	require(cfvComparedChars != NULL);
	if (nTotalFrequency > cfvComparedChars->nTotalFrequency)
		return false;
	for (i = 0; i < GetSize(); i++)
	{
		if (ivCharFrequencies.GetAt(i) > cfvComparedChars->ivCharFrequencies.GetAt(i))
			return false;
	}
	return true;
}

int KWCharFrequencyVector::GetUsedCharNumber() const
{
	int nUsedCharNumber;
	int i;

	nUsedCharNumber = 0;
	for (i = 0; i < GetSize(); i++)
	{
		if (ivCharFrequencies.GetAt(i) > 0)
			nUsedCharNumber++;
	}
	return nUsedCharNumber;
}

int KWCharFrequencyVector::GetFrequentCharNumber(int nFrequencyThreshold) const
{
	int nUsedCharNumber;
	int i;

	require(nFrequencyThreshold >= 0);

	nUsedCharNumber = 0;
	for (i = 0; i < GetSize(); i++)
	{
		if (ivCharFrequencies.GetAt(i) > nFrequencyThreshold)
			nUsedCharNumber++;
	}
	return nUsedCharNumber;
}

void KWCharFrequencyVector::FilterChars(const KWCharFrequencyVector* cfvFilteredChars)
{
	int i;

	require(Check());
	require(cfvFilteredChars != NULL);
	require(cfvFilteredChars->Check());

	// On remet a zero les efefctifs des caracteres a filtrer
	for (i = 0; i < GetSize(); i++)
	{
		if (cfvFilteredChars->ivCharFrequencies.GetAt(i) > 0)
		{
			nTotalFrequency -= ivCharFrequencies.GetAt(i);
			ivCharFrequencies.SetAt(i, 0);
		}
	}
	ensure(Check());
}

void KWCharFrequencyVector::FilterBelowFrequency(int nFrequencyThreshold)
{
	int i;

	require(Check());

	// On remet a zero les effectifs des caracteres a filtrer
	for (i = 0; i < GetSize(); i++)
	{
		if (ivCharFrequencies.GetAt(i) < nFrequencyThreshold)
		{
			nTotalFrequency -= ivCharFrequencies.GetAt(i);
			ivCharFrequencies.SetAt(i, 0);
		}
	}
	ensure(Check());
}

void KWCharFrequencyVector::Add(const KWCharFrequencyVector* cfvAddedChars)
{
	int i;

	require(Check());
	require(cfvAddedChars != NULL);
	require(cfvAddedChars->Check());

	// Addition des effectifs de chaque caractere
	for (i = 0; i < GetSize(); i++)
	{
		if (cfvAddedChars->ivCharFrequencies.GetAt(i) > 0)
		{
			ivCharFrequencies.UpgradeAt(i, cfvAddedChars->ivCharFrequencies.GetAt(i));
		}
	}
	nTotalFrequency += cfvAddedChars->GetTotalFrequency();
	ensure(Check());
}

void KWCharFrequencyVector::Substract(const KWCharFrequencyVector* cfvSubstractedChars)
{
	int i;

	require(Check());
	require(cfvSubstractedChars != NULL);
	require(cfvSubstractedChars->Check());
	require(IsGreaterOrEqual(cfvSubstractedChars));

	// Addition des effectifs de chaque caractere
	for (i = 0; i < GetSize(); i++)
	{
		if (cfvSubstractedChars->ivCharFrequencies.GetAt(i) > 0)
		{
			ivCharFrequencies.UpgradeAt(i, -cfvSubstractedChars->ivCharFrequencies.GetAt(i));
		}
	}
	nTotalFrequency -= cfvSubstractedChars->GetTotalFrequency();
	ensure(Check());
}

void KWCharFrequencyVector::Min(const KWCharFrequencyVector* cfvOtherChars)
{
	int i;

	require(Check());
	require(cfvOtherChars != NULL);
	require(cfvOtherChars->Check());

	// Addition des effectifs de chaque caractere
	nTotalFrequency = 0;
	for (i = 0; i < GetSize(); i++)
	{
		ivCharFrequencies.SetAt(i, min(ivCharFrequencies.GetAt(i), cfvOtherChars->ivCharFrequencies.GetAt(i)));
		nTotalFrequency += ivCharFrequencies.GetAt(i);
	}
	ensure(Check());
}

void KWCharFrequencyVector::Max(const KWCharFrequencyVector* cfvOtherChars)
{
	int i;

	require(Check());
	require(cfvOtherChars != NULL);
	require(cfvOtherChars->Check());

	// Addition des effectifs de chaque caractere
	nTotalFrequency = 0;
	for (i = 0; i < GetSize(); i++)
	{
		ivCharFrequencies.SetAt(i, max(ivCharFrequencies.GetAt(i), cfvOtherChars->ivCharFrequencies.GetAt(i)));
		nTotalFrequency += ivCharFrequencies.GetAt(i);
	}
	ensure(Check());
}

void KWCharFrequencyVector::Not(const KWCharFrequencyVector* cfvOtherChars)
{
	int i;

	require(Check());
	require(cfvOtherChars != NULL);
	require(cfvOtherChars->Check());

	// Addition des effectifs de chaque caractere
	for (i = 0; i < GetSize(); i++)
	{
		if (cfvOtherChars->ivCharFrequencies.GetAt(i) > 0)
		{
			nTotalFrequency -= ivCharFrequencies.GetAt(i);
			ivCharFrequencies.SetAt(i, 0);
		}
	}
	ensure(Check());
}

void KWCharFrequencyVector::CopyFrom(const KWCharFrequencyVector* cfvSource)
{
	require(Check());
	require(cfvSource != NULL);
	require(cfvSource != this);
	require(cfvSource->Check());

	ivCharFrequencies.CopyFrom(&cfvSource->ivCharFrequencies);
	nTotalFrequency = cfvSource->nTotalFrequency;

	ensure(Check());
}

KWCharFrequencyVector* KWCharFrequencyVector::Clone() const
{
	KWCharFrequencyVector* cfvClone;

	cfvClone = new KWCharFrequencyVector;
	cfvClone->CopyFrom(this);
	return cfvClone;
}

boolean KWCharFrequencyVector::Check() const
{
	boolean bOk;
	int i;
	int nComputedTotalFrequency;

	// Verification du contenu, en recomptant l'effectif total
	bOk = ivCharFrequencies.GetSize() == nCharNumber;
	nComputedTotalFrequency = 0;
	for (i = 0; i < ivCharFrequencies.GetSize(); i++)
	{
		assert(ivCharFrequencies.GetAt(i) >= 0);
		nComputedTotalFrequency += ivCharFrequencies.GetAt(i);
	}
	bOk = bOk and nComputedTotalFrequency == nTotalFrequency;
	return bOk;
}

void KWCharFrequencyVector::Write(ostream& ost) const
{
	const int nMaxDisplayedCharNumber = 10;
	int i;
	int nDisplayedCharNumber;
	char cValue;

	// Affichage du contenu dans la limite d'un nombre max de caracteres
	ost << '{';
	ost << '(';
	ost << GetUsedCharNumber();
	ost << "; ";
	ost << GetTotalFrequency();
	ost << ") ";
	nDisplayedCharNumber = 0;
	for (i = 0; i < GetSize(); i++)
	{
		cValue = GetCharAtIndex(i);

		// Affichage uniquement des cararteres d'effectif non nul
		if (GetFrequencyAt(cValue) > 0)
		{
			// Insertion d'un seperateur de valeur
			if (nDisplayedCharNumber > 0)
				ost << ", ";

			// Cas ou la limite est atteinte
			if (nDisplayedCharNumber == nMaxDisplayedCharNumber)
			{
				ost << "...";
				break;
			}
			// Affichage standard d'une paire "char:frequency"
			else
			{
				ost << CharToString(cValue) << ':' << GetFrequencyAt(cValue);
			}
			nDisplayedCharNumber++;
		}
	}
	ost << '}';
}

void KWCharFrequencyVector::FullWrite(ostream& ost) const
{
	int i;
	char cValue;

	// Affichage du contenu complet
	for (i = 0; i < GetSize(); i++)
	{
		cValue = GetCharAtIndex(i);

		// Affichage uniquement des cararteres d'effectif non nul
		if (GetFrequencyAt(cValue) > 0)
		{
			ost << CharToString(cValue) << '\t' << GetFrequencyAt(cValue) << '\n';
		}
	}
}

longint KWCharFrequencyVector::GetUsedMemory() const
{
	return sizeof(KWCharFrequencyVector) + ivCharFrequencies.GetUsedMemory() - sizeof(IntVector);
}

const ALString KWCharFrequencyVector::GetClassLabel() const
{
	return "Char frequencies";
}

const ALString KWCharFrequencyVector::GetObjectLabel() const
{
	ALString sLabel;
	const int nMaxDisplayedCharNumber = 3;
	int i;
	int nDisplayedCharNumber;
	char cValue;

	// Affichage du contenu dans la limite d'un nombre max de caracteres
	sLabel = '{';
	nDisplayedCharNumber = 0;
	for (i = 0; i < GetSize(); i++)
	{
		cValue = GetCharAtIndex(i);

		// Affichage uniquement des cararteres d'effectif non nul
		if (GetFrequencyAt(cValue) > 0)
		{
			// Insertion d'un seperateur de valeur
			if (nDisplayedCharNumber > 0)
				sLabel += ", ";

			// Cas ou la limite est atteinte
			if (nDisplayedCharNumber == nMaxDisplayedCharNumber)
			{
				sLabel += "...";
				break;
			}
			// Affichage standard d'une paire "char:frequency"
			else
			{
				sLabel += CharToString(cValue);
				sLabel += ':' << GetFrequencyAt(cValue);
			}
			nDisplayedCharNumber++;
		}
	}
	return sLabel;
}
