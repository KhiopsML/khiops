// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHFloatingPointFrequencyTableBuilder.h"

MHFloatingPointFrequencyTableBuilder::MHFloatingPointFrequencyTableBuilder()
{
	Clean();
}

MHFloatingPointFrequencyTableBuilder::~MHFloatingPointFrequencyTableBuilder() {}

void MHFloatingPointFrequencyTableBuilder::InitializeBins(const ContinuousVector* cvSourceBinLowerValues,
							  const ContinuousVector* cvSourceBinUpperValues,
							  const IntVector* ivSourceBinFrequencies)
{
	int i;
	Continuous cBinLowerValue;
	Continuous cBinUpperValue;
	int nBinFrequency;
	Continuous cMantissa;

	require(cvSourceBinUpperValues != NULL);
	require(cvSourceBinUpperValues->GetSize() > 0);
	require(cvSourceBinLowerValues == NULL or
		cvSourceBinLowerValues->GetSize() == cvSourceBinUpperValues->GetSize());
	require(ivSourceBinFrequencies == NULL or
		ivSourceBinFrequencies->GetSize() == cvSourceBinUpperValues->GetSize());

	// Nettoyage
	Clean();

	// Calcul des statistiques sur les valeurs
	nDistinctValueNumber = 0;
	cMinValue = GetSystemMaxValue();
	cMaxValue = GetSystemMinValue();
	cMaxNegativeValue = GetSystemMinValue();
	cMinPositiveValue = GetSystemMaxValue();
	for (i = 0; i < cvSourceBinUpperValues->GetSize(); i++)
	{
		// Initialisation dans le cas minmaliste d'un seul vecteur en entree
		cBinUpperValue = cvSourceBinUpperValues->GetAt(i);

		// Initialisation par defaut des autres caracteristiques du bin
		cBinLowerValue = cBinUpperValue;
		nBinFrequency = 1;

		// Innitialisation complementaires si les autres vecteurs sont disponibles
		if (cvSourceBinLowerValues != NULL)
			cBinLowerValue = cvSourceBinLowerValues->GetAt(i);
		if (ivSourceBinFrequencies != NULL)
			nBinFrequency = ivSourceBinFrequencies->GetAt(i);
		assert(cBinLowerValue <= cBinUpperValue);
		assert(nBinFrequency >= 0);

		// Verification
		assert(i == 0 or cBinLowerValue >= cvSourceBinUpperValues->GetAt(i - 1));
		assert(cBinLowerValue >= GetSystemMinValue());
		assert(cBinUpperValue <= GetSystemMaxValue());
		assert(cBinLowerValue != KWContinuous::GetMissingValue());
		assert(cBinUpperValue != KWContinuous::GetMissingValue());
		assert(cBinLowerValue >= 0 or cBinLowerValue <= -KWContinuous::GetEpsilonValue());
		assert(cBinLowerValue <= 0 or cBinLowerValue >= KWContinuous::GetEpsilonValue());
		assert(cBinUpperValue >= 0 or cBinUpperValue <= -KWContinuous::GetEpsilonValue());
		assert(cBinUpperValue <= 0 or cBinUpperValue >= KWContinuous::GetEpsilonValue());

		// Mise a jour des statistiques
		if (i == 0 or cBinUpperValue > cvSourceBinUpperValues->GetAt(i - 1))
		{
			assert(i == 0 or cBinLowerValue > cvSourceBinUpperValues->GetAt(i - 1));
			cvBinLowerValues.Add(cBinLowerValue);
			cvBinUpperValues.Add(cBinUpperValue);
			ivBinFrequencies.Add(nBinFrequency);

			// Mise a jour du nombre de valeurs distinctes
			nDistinctValueNumber++;
			if (cBinUpperValue > cBinLowerValue)
				nDistinctValueNumber++;
		}
		else
		{
			assert(cBinLowerValue == cvBinLowerValues.GetAt(ivBinFrequencies.GetSize() - 1));
			assert(cBinUpperValue == cvBinUpperValues.GetAt(ivBinFrequencies.GetSize() - 1));
			ivBinFrequencies.UpgradeAt(ivBinFrequencies.GetSize() - 1, nBinFrequency);
		}
		cMinValue = min(cMinValue, cBinLowerValue);
		cMaxValue = max(cMaxValue, cBinUpperValue);
		if (cBinUpperValue < 0)
			cMaxNegativeValue = max(cMaxNegativeValue, cBinUpperValue);
		else if (cBinLowerValue > 0)
			cMinPositiveValue = min(cMinPositiveValue, cBinLowerValue);
	}

	// Calcul des effectifs cumules
	ivBinCumulatedFrequencies.SetSize(ivBinFrequencies.GetSize());
	ivBinCumulatedFrequencies.SetAt(0, ivBinFrequencies.GetAt(0));
	for (i = 1; i < ivBinFrequencies.GetSize(); i++)
		ivBinCumulatedFrequencies.SetAt(i, ivBinCumulatedFrequencies.GetAt(i - 1) + ivBinFrequencies.GetAt(i));

	// Correction pour les valeurs centrales
	if (cMinValue >= 0)
		cMaxNegativeValue = 0;
	if (cMaxValue <= 0)
		cMinPositiveValue = 0;

	// Calcul des exposants des bins extremes et centraux, et du nombre total de main bins
	// Cas d'une seule valeur: on exploite un seul bin
	if (cMinValue == cMaxValue)
	{
		// Cas des valeurs nulles uniquement
		if (cMinValue == 0)
		{
			// Un unique bin ]-1;0]
			assert(nMinValueExponent == 0);
			assert(nMaxValueExponent == 0);
			assert(nMinCentralBinExponent == 0);
		}
		// Cas d'une valeur non nulle
		else
		{
			cMantissa = ExtractMantissa(cMaxValue, nMaxValueExponent);
			nMinValueExponent = nMaxValueExponent;
			nMinCentralBinExponent = nMaxValueExponent;
		}
	}
	// Cas de valeurs positives
	else if (cMinValue >= 0)
	{
		// Exposant max
		cMantissa = ExtractMantissa(cMaxValue, nMaxValueExponent);

		// Exposant min si valeurs strictement positives
		if (cMinValue > 0)
			cMantissa = ExtractMantissa(cMinValue, nMinValueExponent);
		// Cas d'un central bin
		else
		{
			assert(cMinPositiveValue > 0);
			cMantissa = ExtractMantissa(cMinPositiveValue, nMinValueExponent);
		}

		// Exposant minimum du central bin
		nMinCentralBinExponent = nMinValueExponent;
	}
	// Cas de valeurs negatives ou nulles
	else if (cMaxValue <= 0)
	{
		// Exposant min
		cMantissa = ExtractMantissa(cMinValue, nMinValueExponent);

		// Exposant max si valeurs strictement negatives
		if (cMaxValue < 0)
			cMantissa = ExtractMantissa(cMaxValue, nMaxValueExponent);
		// Cas d'un central bin
		else
		{
			assert(cMaxNegativeValue < 0);
			cMantissa = ExtractMantissa(cMaxNegativeValue, nMaxValueExponent);
		}

		// Exposant minimum du central bin
		nMinCentralBinExponent = nMaxValueExponent;
	}
	// Cas general de valeurs de part et d'autre de zero
	else
	{
		assert(cMinValue < 0);
		assert(0 < cMaxValue);
		assert(cMaxNegativeValue < 0);
		assert(cMinPositiveValue > 0);

		// Exposant min
		cMantissa = ExtractMantissa(cMinValue, nMinValueExponent);

		// Exposant max
		cMantissa = ExtractMantissa(cMaxValue, nMaxValueExponent);

		// Exposant du bin central
		if (-cMaxNegativeValue < cMinPositiveValue)
			cMantissa = ExtractMantissa(cMaxNegativeValue, nMinCentralBinExponent);
		else
			cMantissa = ExtractMantissa(cMinPositiveValue, nMinCentralBinExponent);
	}

	// Memorisation de la valeur initiale du CentralBinExponent
	SetCentralBinExponent(GetMinCentralBinExponent());

	// Initialisation des bornes du domaines, avec une optimisation
	InitializeDomainBounds();
	ensure(Check());
}

boolean MHFloatingPointFrequencyTableBuilder::IsInitialized() const
{
	return nDistinctValueNumber != -1;
}

int MHFloatingPointFrequencyTableBuilder::GetBinNumber() const
{
	require(IsInitialized());
	return ivBinFrequencies.GetSize();
}

Continuous MHFloatingPointFrequencyTableBuilder::GetBinLowerValueAt(int nBinIndex) const
{
	require(IsInitialized());
	return cvBinLowerValues.GetAt(nBinIndex);
}

Continuous MHFloatingPointFrequencyTableBuilder::GetBinUpperValueAt(int nBinIndex) const
{
	require(IsInitialized());
	return cvBinUpperValues.GetAt(nBinIndex);
}

int MHFloatingPointFrequencyTableBuilder::GetBinFrequencyAt(int nBinIndex) const
{
	return ivBinFrequencies.GetAt(nBinIndex);
}

Continuous MHFloatingPointFrequencyTableBuilder::GetBinLowerValueAtFrequency(int nFrequencyIndex) const
{
	int nCumulatedFrequency;
	int nBinIndex;

	require(IsInitialized());

	nCumulatedFrequency = nFrequencyIndex + 1;
	nBinIndex = SearchBinIndex(nCumulatedFrequency);
	return cvBinLowerValues.GetAt(nBinIndex);
}

Continuous MHFloatingPointFrequencyTableBuilder::GetBinUpperValueAtFrequency(int nFrequencyIndex) const
{
	int nCumulatedFrequency;
	int nBinIndex;

	require(IsInitialized());

	nCumulatedFrequency = nFrequencyIndex + 1;
	nBinIndex = SearchBinIndex(nCumulatedFrequency);
	return cvBinUpperValues.GetAt(nBinIndex);
}

void MHFloatingPointFrequencyTableBuilder::Clean()
{
	cvBinLowerValues.SetSize(0);
	cvBinUpperValues.SetSize(0);
	ivBinFrequencies.SetSize(0);
	ivBinCumulatedFrequencies.SetSize(0);
	nDistinctValueNumber = -1;
	cMinValue = 0;
	cMaxValue = 0;
	cMaxNegativeValue = 0;
	cMinPositiveValue = 0;
	cDomainLowerBound = -DBL_MAX;
	cDomainUpperBound = DBL_MAX;
	nMinValueExponent = 0;
	nMaxValueExponent = 0;
	nMinCentralBinExponent = 0;
	nCentralBinExponent = 0;
	nMainBinNumber = 0;
	nZeroMainBinIndex = 0;
	nMainBinHierarchyRootLevel = 0;
	nMaxHierarchyLevel = 0;
	nMaxSafeHierarchyLevel = 0;
	nDomainBoundsMantissaBitNumber = 0;
	dMinBinLength = DBL_MIN;
	ensure(not AreDomainBoundsInitialized());
}

int MHFloatingPointFrequencyTableBuilder::GetTotalFrequency() const
{
	require(IsInitialized());
	return ivBinCumulatedFrequencies.GetAt(ivBinCumulatedFrequencies.GetSize() - 1);
}

int MHFloatingPointFrequencyTableBuilder::GetDistinctValueNumber() const
{
	require(IsInitialized());
	return nDistinctValueNumber;
}

Continuous MHFloatingPointFrequencyTableBuilder::GetMinValue() const
{
	require(IsInitialized());
	return cMinValue;
}

Continuous MHFloatingPointFrequencyTableBuilder::GetMaxValue() const
{
	require(IsInitialized());
	return cMaxValue;
}

Continuous MHFloatingPointFrequencyTableBuilder::GetMaxNegativeValue() const
{
	require(IsInitialized());
	return cMaxNegativeValue;
}

Continuous MHFloatingPointFrequencyTableBuilder::GetMinPositiveValue() const
{
	require(IsInitialized());
	return cMinPositiveValue;
}

int MHFloatingPointFrequencyTableBuilder::GetMinValueExponent() const
{
	require(IsInitialized());
	return nMinValueExponent;
}

int MHFloatingPointFrequencyTableBuilder::GetMaxValueExponent() const
{
	require(IsInitialized());
	return nMaxValueExponent;
}

int MHFloatingPointFrequencyTableBuilder::GetMinCentralBinExponent() const
{
	require(IsInitialized());
	return nMinCentralBinExponent;
}

int MHFloatingPointFrequencyTableBuilder::GetMaxCentralBinExponent() const
{
	require(IsInitialized());
	if (GetMinValue() == GetMaxValue())
		return nMinCentralBinExponent;
	else if (GetMaxValue() <= 0 and nMinValueExponent == nMaxValueExponent)
		return nMinCentralBinExponent;
	else if (GetMinValue() > 0 and nMinValueExponent == nMaxValueExponent)
		return nMinCentralBinExponent;
	else
		return 1 + max(nMinValueExponent, nMaxValueExponent);
}

int MHFloatingPointFrequencyTableBuilder::GetMinOptimizedCentralBinExponent() const
{
	return GetMinCentralBinExponent();
}

int MHFloatingPointFrequencyTableBuilder::GetMaxOptimizedCentralBinExponent() const
{
	return GetMaxCentralBinExponent();
}

void MHFloatingPointFrequencyTableBuilder::SetCentralBinExponent(int nValue)
{
	int nBinNumber;
	int nExponent;
	Continuous cMinValueBinLowerBound;
	Continuous cMinValueBinUpperBound;
	Continuous cMaxValueBinLowerBound;
	Continuous cMaxValueBinUpperBound;
	int nExtremeValueMantissaBinBitNumber;

	require(IsInitialized());
	require(GetMinCentralBinExponent() <= nValue and nValue <= GetMaxCentralBinExponent());

	// Memorisation de l'exposant
	nCentralBinExponent = nValue;

	// Initialisation de l'index du main bin contenant la valeur 0
	nZeroMainBinIndex = -1;

	// Calcul du nombre de bins principaux
	if (cMinValue == cMaxValue)
	{
		nMainBinNumber = 1;
		if (cMinValue == 0)
			nZeroMainBinIndex = 0;
	}
	// Cas de valeurs positives
	else if (cMinValue >= 0)
	{
		assert(nMinValueExponent <= nCentralBinExponent and nCentralBinExponent <= nMaxValueExponent + 1);

		// Cas du central bin minimum
		if (nCentralBinExponent == nMinValueExponent)
		{
			nMainBinNumber = nMaxValueExponent - nMinValueExponent + 1;

			// Plus les eventuels central bins ]-2^e, 0] et ]0, 2^e] si necessaire
			if (cMinValue == 0)
			{
				nMainBinNumber += 2;
				nZeroMainBinIndex = 0;
			}
		}
		// Cas general
		else
		{
			nMainBinNumber = nMaxValueExponent - nCentralBinExponent + 2;

			// Plus une eventuel central bins ]-2^e, 0] si necessaire
			if (cMinValue == 0)
			{
				nMainBinNumber += 1;
				nZeroMainBinIndex = 0;
			}
		}
	}
	// Cas de valeurs negatives ou nulles
	else if (cMaxValue <= 0)
	{
		assert(nMaxValueExponent <= nCentralBinExponent and nCentralBinExponent <= nMinValueExponent + 1);

		// Cas du central bin minimum
		if (nCentralBinExponent == nMaxValueExponent)
		{
			nMainBinNumber = nMinValueExponent - nMaxValueExponent + 1;

			// Plus l'eventuel central bins ]-2^e, 0] si necessaire
			if (cMaxValue == 0)
			{
				nMainBinNumber += 1;
				nZeroMainBinIndex = nMainBinNumber - 1;
			}
		}
		// Cas general
		else
		{
			nMainBinNumber = nMinValueExponent - nCentralBinExponent + 2;
			nZeroMainBinIndex = nMainBinNumber - 1;
		}
	}
	// Cas general de valeurs de part et d'autre de zero
	else
	{
		assert(nMinCentralBinExponent <= nCentralBinExponent and
		       nCentralBinExponent <= max(nMinValueExponent, nMaxValueExponent) + 1);

		// Nombre de main bins negatifs
		if (nCentralBinExponent > nMinValueExponent)
		{
			nMainBinNumber = 1;
			nZeroMainBinIndex = 0;
		}
		else
		{
			nMainBinNumber = nMinValueExponent - nCentralBinExponent + 2;
			nZeroMainBinIndex = nMainBinNumber - 1;
		}

		// Nombre de main bins positifs
		if (nCentralBinExponent > nMaxValueExponent)
			nMainBinNumber += 1;
		// Cas general
		else
			nMainBinNumber += nMaxValueExponent - nCentralBinExponent + 2;
	}

	// Calcul du niveau de reference de la racine de la hierarchie
	// Cas ou il ya plusieurs main bins
	if (nMainBinNumber >= 2)
	{
		nMainBinHierarchyRootLevel = 0;
		nBinNumber = 1;
		while (nBinNumber < nMainBinNumber)
		{
			nBinNumber *= 2;
			nMainBinHierarchyRootLevel++;
		}
	}
	// Cas avec une seule valeur: on se place a la precision maximale
	else if (cMinValue == cMaxValue)
		nMainBinHierarchyRootLevel = -min(GetDefaultTotalHierarchyLevel(), GetMaxMantissaBinBitNumber());
	// Cas avec un seul main bin
	else
	{
		assert(cMinValue > 0 or cMaxValue <= 0);

		// Recherche du plus grand nombre de digits tels que les valeurs min et max soient dans le meme mantissa
		// bin
		nMainBinHierarchyRootLevel = 0;
		while (nMainBinHierarchyRootLevel <= GetMaxMantissaBinBitNumber())
		{
			ExtractMantissaMainBinBounds(cMinValue, nMainBinHierarchyRootLevel, nCentralBinExponent,
						     nExponent, cMinValueBinLowerBound, cMinValueBinUpperBound);
			ExtractMantissaMainBinBounds(cMaxValue, nMainBinHierarchyRootLevel, nCentralBinExponent,
						     nExponent, cMaxValueBinLowerBound, cMaxValueBinUpperBound);

			// On continue si on est dans le meme mantissa bin
			if (cMinValueBinLowerBound == cMaxValueBinLowerBound)
			{
				assert(cMinValueBinUpperBound == cMaxValueBinUpperBound);
				if (nMainBinHierarchyRootLevel < GetMaxMantissaBinBitNumber())
					nMainBinHierarchyRootLevel++;
				else
					break;
			}
			// Arret si on est dans des mantissa bins differents
			else
			{
				nMainBinHierarchyRootLevel--;
				break;
			}
		}
		assert(nMainBinHierarchyRootLevel >= 0);

		// On est ici en niveau negatif, car on exploite qu'une partie de bin
		nMainBinHierarchyRootLevel = -nMainBinHierarchyRootLevel;
	}

	// Calcul du niveau de hierarchie max
	nMaxHierarchyLevel =
	    min(GetMaxMantissaBinBitNumber() + GetMainBinHierarchyRootLevel(), GetDefaultTotalHierarchyLevel());

	// Calcul de la longueur du plus petit bin, sur la base du plus petit central bin et du niveau de precision
	// maximale Attention, ce calcul est fait une fois pour toutes pour le min central bin exponent
	if (nCentralBinExponent == GetMinCentralBinExponent() and dMinBinLength > 0)
	{
		nExtremeValueMantissaBinBitNumber = GetMaxHierarchyLevel() - GetMainBinHierarchyRootLevel();
		assert(nExtremeValueMantissaBinBitNumber > 0);
		dMinBinLength = ldexp(1, GetMinCentralBinExponent() - nExtremeValueMantissaBinBitNumber);
	}
	assert(dMinBinLength > 0);

	// Mise a jour du niveau de hierarchie max "sur", en tenant compte des limites de la precision numerique,
	// sauf lors de l'initialisation quand les bornes du domaines numerique n'ont pas encore ete initialisees
	nMaxSafeHierarchyLevel = nMaxHierarchyLevel;
	if (AreDomainBoundsInitialized())
		UpdateMaxSafeHierarchyLevel();
	ensure(Check());
}

int MHFloatingPointFrequencyTableBuilder::GetCentralBinExponent() const
{
	require(IsInitialized());
	return nCentralBinExponent;
}

int MHFloatingPointFrequencyTableBuilder::GetMainBinNumber() const
{
	require(IsInitialized());
	return nMainBinNumber;
}

int MHFloatingPointFrequencyTableBuilder::GetZeroMainBinIndex() const
{
	require(IsInitialized());
	return nZeroMainBinIndex;
}

void MHFloatingPointFrequencyTableBuilder::GetMainBinSpecAt(int nIndex, int& nSign, int& nExponent,
							    boolean& bIsCentralBin) const
{
	require(IsInitialized());
	require(0 <= nIndex and nIndex < GetMainBinNumber());

	// Cas d'une plage de valeur negative ou nulle contenue dans un seul central bin
	if (GetMinValue() == 0 or (GetMinValue() <= 0 and GetCentralBinExponent() > GetMinValueExponent()))
	{
		if (nIndex == 0)
		{
			nSign = -1;
			nExponent = GetCentralBinExponent();
			bIsCentralBin = true;
		}
		else if (nIndex == 1)
		{
			nSign = 1;
			nExponent = GetCentralBinExponent();
			bIsCentralBin = true;
		}
		else
		{
			nSign = 1;
			nExponent = GetCentralBinExponent() + nIndex - 2;
			bIsCentralBin = false;
		}
	}
	// Cas d'une plage de valeurs comportant des valeurs negatives hors central bin
	else if (GetMinValue() < 0)
	{
		if (nIndex <= GetMinValueExponent() - GetCentralBinExponent())
		{
			nSign = -1;
			nExponent = GetMinValueExponent() - nIndex;
			bIsCentralBin = false;
		}
		else if (nIndex == GetMinValueExponent() - GetCentralBinExponent() + 1)
		{
			nSign = -1;
			nExponent = GetCentralBinExponent();
			bIsCentralBin = true;
		}
		else if (nIndex == GetMinValueExponent() - GetCentralBinExponent() + 2)
		{
			nSign = 1;
			nExponent = GetCentralBinExponent();
			bIsCentralBin = true;
		}
		else
		{
			nSign = 1;
			nExponent = GetMaxValueExponent() - (GetMainBinNumber() - 1 - nIndex);
			bIsCentralBin = false;
		}
	}
	// Cas d'une plage de valeur strictement positive
	else
	{
		nSign = 1;
		if (GetCentralBinExponent() == GetMinCentralBinExponent())
		{
			nExponent = GetMaxValueExponent() - (GetMainBinNumber() - 1 - nIndex);
			bIsCentralBin = false;
		}
		else
		{
			if (nIndex == 0)
			{
				nExponent = GetCentralBinExponent();
				bIsCentralBin = true;
			}
			else
			{
				nExponent = GetCentralBinExponent() + nIndex - 1;
				bIsCentralBin = false;
			}
		}
	}
}

void MHFloatingPointFrequencyTableBuilder::GetMainBinBoundsAt(int nIndex, Continuous& cLowerBound,
							      Continuous& cUpperBound) const
{
	int nSign;
	int nExponent;
	boolean bIsCentralBin;

	require(IsInitialized());

	// Extraction des specification du bin
	GetMainBinSpecAt(nIndex, nSign, nExponent, bIsCentralBin);

	// Extration des bornes
	if (bIsCentralBin)
		BuildCentralBinBounds(nSign, nExponent, cLowerBound, cUpperBound);
	else
		BuildExponentBinBounds(nSign, nExponent, cLowerBound, cUpperBound);
}

int MHFloatingPointFrequencyTableBuilder::GetMainBinHierarchyRootLevel() const
{
	require(IsInitialized());
	return nMainBinHierarchyRootLevel;
}

int MHFloatingPointFrequencyTableBuilder::GetTotalBinNumberAt(int nHierarchyLevel) const
{
	int nTotalBinNumber;
	int nMantissaBitNumber;
	int nMainBinMantissaBinNumber;
	int nFirstMainBinMantissaBinIndex;
	int nLastMainBinMantissaBinIndex;
	int nExponent;
	Continuous cLowerBound;
	Continuous cUpperBound;
	Continuous cSingularityLowerBound;
	Continuous cSingularityUpperBound;

	require(IsInitialized());
	require(AreDomainBoundsInitialized());
	require(0 <= nHierarchyLevel and nHierarchyLevel <= GetMaxHierarchyLevel());

	// Cas de bins hierarchique constitues de plusieurs main bins
	if (GetMainBinHierarchyRootLevel() > 0 and nHierarchyLevel <= GetMainBinHierarchyRootLevel())
	{
		// On se base ici sur des groupements de main bins selon leur niveau de hierarchie
		nTotalBinNumber = 1 << nHierarchyLevel;
		nTotalBinNumber = min(nTotalBinNumber, GetMainBinNumber());
	}
	// Cas ou on atteint le niveau de main bins
	else
	{
		// On a ici un main bin avec son mantissa bin, pour la borne inf du domaine et la borne sup du domaine
		// On se contente de faire ici la difference des index de main bin (MainBin Number-1) fois le nombre de
		// mantissa bins par main bin, plus la difference des index de mantissa bin, plus 1 pour inclure
		// la borne sup.
		// La borne inf du domaine est dans le meme main bin que la valeur min, et la borne sup du domaine est
		// dans le meme main bin que la valeur max: il n'y a pas de risque de changement de main bin en passant
		// des valeurs extremes des donnees au valeurs extremes du domaine.
		// La borne sup du domaine est incluse dans son mantissa bin, alors que la borne inf en est exclue.
		// Enfin, si un mantissa bin traverse l'intervalle de singularite ]-EpsilonValue, EpsilonValue] autour
		// de 0, il ne faut compter qu'un seul mantissa bin et reduire l'index a 1.

		// Nombre de mantissa bins par main bins
		nMantissaBitNumber = nHierarchyLevel - GetMainBinHierarchyRootLevel();
		nMainBinMantissaBinNumber = 1 << nMantissaBitNumber;

		// Prise en compte des mantissa bins pour les main bins
		nTotalBinNumber = (GetMainBinNumber() - 1) * nMainBinMantissaBinNumber;

		// Index du mantissa bin du dernier main bin, couvrant la borne sup du domaine
		if (GetDomainUpperBound() != 0)
		{
			// On extrait les bornes du mantissa bin correspondant
			ExtractMantissaMainBinBounds(GetDomainUpperBound(), nMantissaBitNumber, GetCentralBinExponent(),
						     nExponent, cLowerBound, cUpperBound);
			assert(GetDomainUpperBound() <= cUpperBound);

			// Cas particulier de l'intervalle de singularite ]-EpsilonValue,EpsilonValue[
			if (0 < cLowerBound and cLowerBound < KWContinuous::GetEpsilonValue())
			{
				assert(cUpperBound > KWContinuous::GetEpsilonValue());

				// On ne compte qu'un bin ]0,UpperBound]
				nLastMainBinMantissaBinIndex = 1;
			}
			else if (-KWContinuous::GetEpsilonValue() < cUpperBound and cUpperBound < 0)
			{
				assert(cLowerBound < -KWContinuous::GetEpsilonValue());

				// On compte tous les mantissa bin du main bin, comme dans le cas du DomainUpperBound=0
				nLastMainBinMantissaBinIndex = nMainBinMantissaBinNumber;
			} // Cas general
			else
			{
				assert(cLowerBound == 0 or cLowerBound > KWContinuous::GetEpsilonValue() or
				       cLowerBound < -KWContinuous::GetEpsilonValue());

				// On compte l'index, plus 1 pour englober la valeur sup
				nLastMainBinMantissaBinIndex =
				    (int)ExtractMantissaBinInMainBinIndex(GetDomainUpperBound(), nMantissaBitNumber,
									  GetCentralBinExponent()) +
				    1;
			}
		}
		// Cas de 0 qui qui est dans le central bin negatif, dont il est la derniere valeur
		else
			// On compte tous les mantissa bin du main bin
			nLastMainBinMantissaBinIndex = nMainBinMantissaBinNumber;

		// Index du mantissa bins du premier main bin, couvrant la borne inf du domaine
		if (GetDomainLowerBound() != 0)
		{
			// On extrait les bornes du mantissa bin correspondant
			ExtractMantissaMainBinBounds(GetDomainLowerBound(), nMantissaBitNumber, GetCentralBinExponent(),
						     nExponent, cLowerBound, cUpperBound);
			assert(cLowerBound < GetDomainLowerBound());

			// Cas particulier de l'intervalle de singularite ]-EpsilonValue,EpsilonValue[
			if (0 < cLowerBound and cLowerBound < KWContinuous::GetEpsilonValue())
			{
				assert(cUpperBound > KWContinuous::GetEpsilonValue());

				// On a pas de bin a compter, comme pour DomainLowerBound=0
				nFirstMainBinMantissaBinIndex = 0;
			}
			else if (-KWContinuous::GetEpsilonValue() < cUpperBound and cUpperBound < 0)
			{
				assert(cLowerBound < -KWContinuous::GetEpsilonValue());

				// On compte tous les mantissa bins, moins 1, par ce que l'on s'arrete un bin avant la
				// fin du central bin
				nFirstMainBinMantissaBinIndex = nMainBinMantissaBinNumber - 1;
			}
			// Cas general
			else
			{
				assert(cLowerBound == 0 or cLowerBound > KWContinuous::GetEpsilonValue() or
				       cLowerBound < -KWContinuous::GetEpsilonValue());

				// On compte l'index, ce qui exclu la valeur inf
				nFirstMainBinMantissaBinIndex = (int)ExtractMantissaBinInMainBinIndex(
				    GetDomainLowerBound(), nMantissaBitNumber, GetCentralBinExponent());

				// Correction si la valeur inf du domaine coincide avec le upper bound du mantissa bin:
				// dans cas cas on a decompte un bin en trop
				if (GetDomainLowerBound() == cUpperBound)
				{
					// On extrait les bornes pour la borne inf de la singularite en 0
					ExtractMantissaMainBinBounds(-KWContinuous::GetEpsilonValue(),
								     nMantissaBitNumber, GetCentralBinExponent(),
								     nExponent, cSingularityLowerBound,
								     cSingularityUpperBound);

					// Cas ou on est a la limite de la singularite
					if (cUpperBound == cSingularityLowerBound)
						// On compte tous les mantissa bins, moins 1, par ce que l'on s'arrete
						// un bin avant la fin du central bin
						nFirstMainBinMantissaBinIndex = nMainBinMantissaBinNumber - 1;
					// Cas general
					else
					{
						// On compense le bin decompte en trop
						nFirstMainBinMantissaBinIndex++;

						// On gere le cas d'un changement de main bin
						if (nFirstMainBinMantissaBinIndex == nMainBinMantissaBinNumber)
							nFirstMainBinMantissaBinIndex = 0;
					}
				}
			}
		}
		// Cas de 0 qui qui est dans le central bin negatif, dont il est la derniere valeur
		else
			// On n'a pas de bin a compter
			nFirstMainBinMantissaBinIndex = 0;

		// On fait la soustraction des index de mantissa bin entre le dernier (inclus) et le premier (exclus)
		nTotalBinNumber += nLastMainBinMantissaBinIndex - nFirstMainBinMantissaBinIndex;
	}
	ensure(1 <= nTotalBinNumber and nTotalBinNumber <= (1 << nHierarchyLevel));
	return nTotalBinNumber;
}

Continuous MHFloatingPointFrequencyTableBuilder::GetDomainLowerBound() const
{
	require(IsInitialized());
	return cDomainLowerBound;
}

Continuous MHFloatingPointFrequencyTableBuilder::GetDomainUpperBound() const
{
	require(IsInitialized());
	return cDomainUpperBound;
}

int MHFloatingPointFrequencyTableBuilder::GetDomainBoundsMantissaBitNumber() const
{
	require(IsInitialized());
	return nDomainBoundsMantissaBitNumber;
}

double MHFloatingPointFrequencyTableBuilder::GetMinBinLength() const
{
	require(IsInitialized());
	return dMinBinLength;
}

int MHFloatingPointFrequencyTableBuilder::GetMinDistinctValueNumberPerBin() const
{
	int nMinimumDistinctValueNumberPerBin;
	double dBalancedIntervalNumber;

	require(IsInitialized());

	// Nombre d'intervalle equilibre, avec effectif par intervalle egal au nombre d'intervalles
	dBalancedIntervalNumber = sqrt(1.0 + GetTotalFrequency());

	// Heuristique pour le nombre min de valeurs distinctes encodable par intervalle
	// (ne pas confondre avec le nombre d'instances par intervalles)
	// On corrige l'effectif en utilisant la loi normal inverse, pour tenir compte du fait que l'on peut s'ecarter
	// du nombre moyen de valeur par intervalle d'autant plus que le nombre d'intervalle est grand Attention: cette
	// heuristique reste tres "bidulique" est a ete en grande partie ajustee experimentalement sur la base de jeux
	// de donnees uniformes ou gaussiens, translates de plus en plus loisn de l'origines pour rarefier le nombre de
	// valeurs numeriques distinctes encodables par plage de valeurs
	nMinimumDistinctValueNumberPerBin =
	    int(ceil(dBalancedIntervalNumber * KWStat::InvStandardNormal(1 - 1 / dBalancedIntervalNumber)));
	return nMinimumDistinctValueNumberPerBin;
}

int MHFloatingPointFrequencyTableBuilder::GetMaxHierarchyLevel() const
{
	require(IsInitialized());
	return nMaxHierarchyLevel;
}

int MHFloatingPointFrequencyTableBuilder::GetMaxSafeHierarchyLevel() const
{
	require(IsInitialized());
	return nMaxSafeHierarchyLevel;
}

int MHFloatingPointFrequencyTableBuilder::GetDefaultTotalHierarchyLevel()
{
	return 30;
}

void MHFloatingPointFrequencyTableBuilder::BuildFrequencyTable(int nHierarchyLevel,
							       KWFrequencyTable*& histogramFrequencyTable) const
{
	boolean bDisplay = false;
	boolean bDisplayDetails = false;
	Continuous cLowerValue;
	Continuous cUpperValue;
	Continuous cLowerBound;
	Continuous cInnerBound;
	Continuous cUpperBound;
	ContinuousVector cvBounds;
	IntVector ivFrequencies;
	MHMODLHistogramVector* frequencyVector;
	int i;
	int nIndex;

	require(IsInitialized());
	require(0 <= nHierarchyLevel and nHierarchyLevel <= GetMaxHierarchyLevel());

	// Creation des intervalles
	if (bDisplay)
		cout << "Create intervals\t" << nHierarchyLevel << "\t" << GetTotalBinNumberAt(nHierarchyLevel) << "\n";

	// Cas particulier d'un seul intervalle
	if (nHierarchyLevel == 0)
	{
		// Utilisation des bornes du domaine
		cvBounds.Add(GetDomainLowerBound());
		cvBounds.Add(GetDomainUpperBound());

		// Memorisation de l'effectif global
		ivFrequencies.Add(GetTotalFrequency());
	}
	// Cas general
	else
	{
		// Creation des intervalles correspondant aux valeurs du domaine
		for (i = 0; i < GetBinNumber(); i++)
		{
			// Extrraction des bornes d'un bin elementairres
			cLowerValue = GetBinLowerValueAt(i);
			cUpperValue = GetBinUpperValueAt(i);
			assert(cLowerValue <= cUpperValue);

			// Incrementation de l'effectif du dernier intervalle s'il contient la valeur
			if (cvBounds.GetSize() > 0 and cUpperValue <= cvBounds.GetAt(cvBounds.GetSize() - 1))
			{
				assert(cUpperValue >= cvBounds.GetAt(cvBounds.GetSize() - 2));
				ivFrequencies.UpgradeAt(ivFrequencies.GetSize() - 1, GetBinFrequencyAt(i));
			}
			// Sinon, creation de nouveaux intervalles
			else
			{
				// Extraction des bornes de l'intervalle
				ExtractFloatingPointBinBounds(cLowerValue, nHierarchyLevel, cLowerBound, cUpperBound);
				if (cUpperValue > cLowerValue)
					ExtractFloatingPointBinBounds(cUpperValue, nHierarchyLevel, cInnerBound,
								      cUpperBound);
				assert(cLowerBound < cUpperBound);

				// Cas particulier d'une borne inf rentrant strictement dans le dernier intervalle
				// courant Cela peut arriver dans le cas d'un bin de longueur strictement positive
				if (cvBounds.GetSize() > 0 and (cLowerBound < cvBounds.GetAt(cvBounds.GetSize() - 1)))
				{
					assert(cLowerValue < cUpperValue);

					// On doit agrandir le dernier intervalle courant pour y incorporer le denier
					// bin
					cvBounds.SetAt(cvBounds.GetSize() - 1, cUpperBound);
					ivFrequencies.UpgradeAt(ivFrequencies.GetSize() - 1, GetBinFrequencyAt(i));
					if (bDisplayDetails)
						cout << "\tUpdate full\t" << i << "\t"
						     << KWContinuous::ContinuousToString(cUpperValue) << "\t,"
						     << KWContinuous::ContinuousToString(cUpperBound) << "\t]" << endl;
				}
				// Cas standard d'un bin arrivant apres le dernier intervalle courant
				else
				{
					// On doit depasser la borne de l'intervalle precedent, sauf dans le cas extreme
					// des valeurs autour de la limite de la precision
					assert(cvBounds.GetSize() == 0 or
					       cLowerBound >= cvBounds.GetAt(cvBounds.GetSize() - 1) or
					       (cLowerBound == 0 and
						cvBounds.GetAt(cvBounds.GetSize() - 1) ==
						    KWContinuous::GetEpsilonValue() and
						cUpperBound > KWContinuous::GetEpsilonValue()));

					// Memorisation de la toute premiere borne si necessaire
					if (cvBounds.GetSize() == 0)
						cvBounds.Add(cLowerBound);
					// Ajout d'un intervalle vide si necessaire
					else if (cLowerBound > cvBounds.GetAt(cvBounds.GetSize() - 1))
					{
						cvBounds.Add(cLowerBound);
						ivFrequencies.Add(0);
						if (bDisplayDetails)
							cout << "\tAdd empty\t" << i << "\t"
							     << KWContinuous::ContinuousToString(cUpperValue) << "\t]"
							     << KWContinuous::ContinuousToString(cLowerBound) << "\t,"
							     << endl;
					}

					// Ajout de l'intervalle contenant le nouveau bin
					cvBounds.Add(cUpperBound);
					ivFrequencies.Add(GetBinFrequencyAt(i));
					if (bDisplayDetails)
						cout << "\tAdd full\t" << i << "\t"
						     << KWContinuous::ContinuousToString(cUpperValue) << "\t,"
						     << KWContinuous::ContinuousToString(cUpperBound) << "\t]" << endl;
				}
			}
			assert(cvBounds.GetAt(cvBounds.GetSize() - 1) >= cUpperValue);
		}

		// Ajout d'un premier intervalle vide si necessaire, entre la borne inf du domain et la valeur min
		if (GetDomainLowerBound() < cvBounds.GetAt(0))
		{
			// Decallage des valeurs collectes d'un cran vers la droite
			// Cette recopie est innefficace, mais cela reste negligeable et cela permet d'ameliorer
			// la maintenabilite du code en searant la collecte des valeurs de base et la gestion des bornes
			// du domaine
			cvBounds.SetSize(cvBounds.GetSize() + 1);
			for (i = cvBounds.GetSize() - 1; i >= 1; i--)
				cvBounds.SetAt(i, cvBounds.GetAt(i - 1));

			// Decallage des effectifs collectes d'un grand vers la droite
			ivFrequencies.SetSize(ivFrequencies.GetSize() + 1);
			for (i = ivFrequencies.GetSize() - 1; i >= 1; i--)
				ivFrequencies.SetAt(i, ivFrequencies.GetAt(i - 1));

			// On positionne le nouvelle intervalle vide au debut
			cvBounds.SetAt(0, GetDomainLowerBound());
			ivFrequencies.SetAt(0, 0);
		}
		// Sinon, on remplace la borne du premier intervalle
		else
			cvBounds.SetAt(0, GetDomainLowerBound());
		assert(cvBounds.GetAt(0) <= GetMinValue());
		assert(cvBounds.GetAt(0) < cvBounds.GetAt(1));

		// Creation d'un dernier intervalle vide si necessaire, entre la valeur max et la borne sup du domain
		if (cvBounds.GetAt(cvBounds.GetSize() - 1) < GetDomainUpperBound())
		{
			cvBounds.Add(GetDomainUpperBound());
			ivFrequencies.Add(0);
		}
		// Sinon, on remplace la borne du dernier intervalle
		else
			cvBounds.SetAt(cvBounds.GetSize() - 1, GetDomainUpperBound());
		assert(cvBounds.GetAt(cvBounds.GetSize() - 1) >= GetMaxValue());
		assert(cvBounds.GetAt(cvBounds.GetSize() - 1) > cvBounds.GetAt(cvBounds.GetSize() - 2));
	}

	// Nettoyage potentiel pour traiter le cas de la singularite autour de zero
	// On fusionne tous les intervalles dont une borne commune est +-EpsilonValue
	// On ne le fait qu'apres la collecte des bornes, car c'est plus simple ici que de le faire au fur et a mesure
	nIndex = 1;
	i = 2;
	while (i < cvBounds.GetSize())
	{
		assert(i == cvBounds.GetSize() - 1 or cvBounds.GetAt(i) < cvBounds.GetAt(i + 1));

		// On fusionne les intervalles dont une des bornes est EpsilonValue
		if (cvBounds.GetAt(nIndex) == -KWContinuous::GetEpsilonValue() or
		    cvBounds.GetAt(nIndex) == KWContinuous::GetEpsilonValue())
		{
			cvBounds.SetAt(nIndex, cvBounds.GetAt(i));
			ivFrequencies.SetAt(nIndex - 1, ivFrequencies.GetAt(nIndex - 1) + ivFrequencies.GetAt(i - 1));
		}
		// On garde les autres tels quels
		else
		{
			nIndex++;
			cvBounds.SetAt(nIndex, cvBounds.GetAt(i));
			ivFrequencies.SetAt(nIndex - 1, ivFrequencies.GetAt(i - 1));
		}
		i++;
	}
	ivFrequencies.SetSize(nIndex);
	cvBounds.SetSize(nIndex + 1);
	assert(ivFrequencies.GetSize() <= 1 << nHierarchyLevel);
	assert(ivFrequencies.GetSize() == cvBounds.GetSize() - 1);

	// Creation de la table parametree pour la creation de MHMODLHistogramVector
	histogramFrequencyTable = new MHMODLHistogramTable;
	histogramFrequencyTable->SetFrequencyVectorNumber(ivFrequencies.GetSize());

	// Parametrage avec les specifications de granularisation
	histogramFrequencyTable->SetGranularizedValueNumber(GetTotalBinNumberAt(nHierarchyLevel));
	cast(MHMODLHistogramTable*, histogramFrequencyTable)->SetCentralBinExponent(GetCentralBinExponent());
	cast(MHMODLHistogramTable*, histogramFrequencyTable)->SetHierarchyLevel(nHierarchyLevel);
	cast(MHMODLHistogramTable*, histogramFrequencyTable)->SetMinBinLength(GetMinBinLength());

	// Alimentation de la table
	for (i = 0; i < ivFrequencies.GetSize(); i++)
	{
		frequencyVector = cast(MHMODLHistogramVector*, histogramFrequencyTable->GetFrequencyVectorAt(i));

		// Parametrage du vecteur
		assert(cvBounds.GetAt(i) < cvBounds.GetAt(i + 1));
		frequencyVector->SetFrequency(ivFrequencies.GetAt(i));
		frequencyVector->SetLowerBound(cvBounds.GetAt(i));
		frequencyVector->SetUpperBound(cvBounds.GetAt(i + 1));
		assert(frequencyVector->GetLowerBound() < frequencyVector->GetUpperBound());
	}

	// Affichage de la table
	if (bDisplay)
	{
		cout << "\tLower bound\tUpper bound\tFrequency\n";
		for (i = 0; i < ivFrequencies.GetSize(); i++)
		{
			frequencyVector =
			    cast(MHMODLHistogramVector*, histogramFrequencyTable->GetFrequencyVectorAt(i));
			cout << "\t" << frequencyVector->GetLowerBound();
			cout << "\t" << frequencyVector->GetUpperBound();
			cout << "\t" << frequencyVector->GetFrequency() << "\n";
		}
	}
	ensure(histogramFrequencyTable->GetTotalFrequency() == GetTotalFrequency());
	ensure(histogramFrequencyTable->GetGranularizedValueNumber() >=
	       histogramFrequencyTable->GetFrequencyVectorNumber());
	ensure(cast(MHMODLHistogramVector*, histogramFrequencyTable->GetFrequencyVectorAt(0))->GetLowerBound() <=
	       GetMinValue());
	ensure(cast(MHMODLHistogramVector*, histogramFrequencyTable->GetFrequencyVectorAt(
						histogramFrequencyTable->GetFrequencyVectorNumber() - 1))
		   ->GetUpperBound() >= GetMaxValue());
}

void MHFloatingPointFrequencyTableBuilder::BuildNulFrequencyTable(KWFrequencyTable*& histogramFrequencyTable) const
{
	BuildFrequencyTable(0, histogramFrequencyTable);
	cast(MHMODLHistogramTable*, histogramFrequencyTable)->SetCentralBinExponent(GetMaxCentralBinExponent());
}

void MHFloatingPointFrequencyTableBuilder::ExtractFloatingPointBinBounds(Continuous cValue, int nHierarchyBitNumber,
									 Continuous& cLowerBound,
									 Continuous& cUpperBound) const
{
	int nMantissaBitNumber;
	int nExponent;
	int nMainBinIndex;
	int nHierarchyBinNumber;
	int nHierarchyBinIndex;
	int nHierarchyMainBinIndex;
	Continuous cBound;

	require(IsInitialized());
	require(cValue != KWContinuous::GetMissingValue());
	require(GetMinValue() <= cValue and cValue <= GetMaxValue());
	require(cValue >= 0 or cValue <= GetMaxNegativeValue());
	require(cValue <= 0 or cValue >= GetMinPositiveValue());
	require(0 <= nHierarchyBitNumber and nHierarchyBitNumber <= GetMaxHierarchyLevel());

	// Cas ou on est au debut de la hierarchie et ou ne s'interesse qu'au main bin
	if (GetMainBinHierarchyRootLevel() > 0 and nHierarchyBitNumber < GetMainBinHierarchyRootLevel())
	{
		// Acces a l'index du main bin associe a la valeur
		nMainBinIndex = GetMainBinIndex(cValue);

		// Recherche de la borne inf du hierarchical bin
		nHierarchyBinNumber = 1 << nHierarchyBitNumber;
		nHierarchyBinIndex = nMainBinIndex * nHierarchyBinNumber / GetMainBinNumber();
		assert(nHierarchyBinIndex < nHierarchyBinNumber);
		nHierarchyMainBinIndex =
		    (nHierarchyBinIndex * GetMainBinNumber() + nHierarchyBinNumber - 1) / nHierarchyBinNumber;
		GetMainBinBoundsAt(nHierarchyMainBinIndex, cLowerBound, cBound);

		// Recherche de la borne sup du hierarchical bin
		nHierarchyBinIndex++;
		if (nHierarchyBinIndex > nHierarchyBinNumber - 1)
			GetMainBinBoundsAt(GetMainBinNumber() - 1, cBound, cUpperBound);
		else
		{
			nHierarchyMainBinIndex =
			    (nHierarchyBinIndex * GetMainBinNumber() + nHierarchyBinNumber - 1) / nHierarchyBinNumber;
			GetMainBinBoundsAt(nHierarchyMainBinIndex, cUpperBound, cBound);
		}
	}
	// Cas ou on est dans un mantissa bin
	else
	{
		// Nombre de bits de la mantisse
		nMantissaBitNumber = nHierarchyBitNumber - GetMainBinHierarchyRootLevel();
		assert(0 <= nMantissaBitNumber and nMantissaBitNumber <= GetMaxMantissaBinBitNumber());

		// Cas de la valeur 0
		if (cValue == 0)
		{
			nExponent = GetCentralBinExponent();
			ExtractMantissaCentralBinBounds(cValue, nMantissaBitNumber, nExponent, cLowerBound,
							cUpperBound);
			assert(cLowerBound < 0);
			assert(cUpperBound == 0);
		}
		// Cas d'un central bin
		else if (ldexp(-1, GetCentralBinExponent()) < cValue and cValue <= ldexp(1, GetCentralBinExponent()))
		{
			nExponent = GetCentralBinExponent();
			ExtractMantissaCentralBinBounds(cValue, nMantissaBitNumber, nExponent, cLowerBound,
							cUpperBound);
			assert(ldexp(-1, GetCentralBinExponent()) <= cLowerBound and
			       cUpperBound <= ldexp(1, GetCentralBinExponent()));
		}
		// Cas general d'un exponent bin
		else
		{
			ExtractMantissaExponentBinBounds(cValue, nMantissaBitNumber, nExponent, cLowerBound,
							 cUpperBound);
		}
	}
	assert(cLowerBound < cValue and cValue <= cUpperBound);

	// Traitement particulier des bornes depassant les limites de la precision des Continuous
	if ((fabs(cLowerBound) < KWContinuous::GetEpsilonValue() and cLowerBound != 0) or
	    (fabs(cUpperBound) < KWContinuous::GetEpsilonValue() and cUpperBound != 0))
	{
		// Cas tout negatif
		if (cUpperBound <= 0)
		{
			assert(cLowerBound < -KWContinuous::GetEpsilonValue() or cUpperBound == 0);
			if (cLowerBound > -KWContinuous::GetEpsilonValue())
			{
				cLowerBound = -KWContinuous::GetEpsilonValue();
				assert(cUpperBound == 0);
			}
			if (cUpperBound > -KWContinuous::GetEpsilonValue())
			{
				cUpperBound = 0;
				assert(cLowerBound <= -KWContinuous::GetEpsilonValue());
			}
		}
		// Cas tout positif
		else if (cLowerBound >= 0)
		{
			assert(cUpperBound > KWContinuous::GetEpsilonValue() or cLowerBound == 0);
			if (cLowerBound < KWContinuous::GetEpsilonValue())
			{
				if (cLowerBound > 0)
					cLowerBound = 0;
			}
			if (cUpperBound < KWContinuous::GetEpsilonValue())
			{
				cUpperBound = KWContinuous::GetEpsilonValue();
				assert(cLowerBound == 0);
			}
		}
		// Cas de part et d'autre de zero
		else
		{
			assert(cLowerBound < 0 and cUpperBound > 0);
			if (cLowerBound > -KWContinuous::GetEpsilonValue())
			{
				if (cValue > 0)
				{
					cLowerBound = 0;
					assert(cUpperBound > KWContinuous::GetEpsilonValue());
				}
				else
				{
					assert(cValue <= 0);
					cLowerBound = -KWContinuous::GetEpsilonValue();
				}
			}
			if (cUpperBound < KWContinuous::GetEpsilonValue())
			{
				assert(cValue <= 0);
				cUpperBound = 0;
				assert(cLowerBound <= -KWContinuous::GetEpsilonValue());
			}
		}
		assert(cLowerBound < cUpperBound);
	}
	assert(cLowerBound < cValue and cValue <= cUpperBound);
	assert(fabs(cLowerBound) >= KWContinuous::GetEpsilonValue() or cLowerBound == 0);
	assert(fabs(cUpperBound) >= KWContinuous::GetEpsilonValue() or cUpperBound == 0);
}

int MHFloatingPointFrequencyTableBuilder::GetMainBinIndex(Continuous cValue) const
{
	int nMainBinIndex;
	int nExponent;
	boolean bCheck = false;
	Continuous cLowerBound;
	Continuous cUpperBound;

	// Verifications poussees en mode debug
	debug(bCheck = true);

	// Cas avant un central bin
	if (cValue <= ldexp(-1, GetCentralBinExponent()))
	{
		ExtractMantissa(cValue, nExponent);
		nMainBinIndex = GetMinValueExponent() - nExponent;
	}
	// Cas apres un central bin
	else if (cValue > ldexp(1, GetCentralBinExponent()))
	{
		ExtractMantissa(cValue, nExponent);
		nMainBinIndex = GetMainBinNumber() - 1 - (GetMaxValueExponent() - nExponent);
	}
	// Cas dans un central bin
	else
	{
		if (cValue <= 0)
			nMainBinIndex = nZeroMainBinIndex;
		else
			nMainBinIndex = nZeroMainBinIndex + 1;
	}
	ensure(0 <= nMainBinIndex and nMainBinIndex < GetMainBinNumber());

	// Verification du main bin
	if (bCheck)
	{
		GetMainBinBoundsAt(nMainBinIndex, cLowerBound, cUpperBound);
		assert(cLowerBound < cValue and cValue <= cUpperBound);
	}
	return nMainBinIndex;
}

Continuous MHFloatingPointFrequencyTableBuilder::ExtractMantissa(Continuous cValue, int& nExponent)
{
	Continuous cMantissa;

	require(cValue != KWContinuous::GetMissingValue());

	// Obtention de la mantisse et de l'exposant avec l'API C, qui renvoie une mantisse normalisee 0.5 <= |m| < 1
	// dans [0.5, 1[
	cMantissa = frexp(cValue, &nExponent);

	// On se replace sur des intervalles fermes a gauche et ouvert a droite
	if (cMantissa != 0)
	{
		cMantissa = ldexp(cMantissa, 1);
		nExponent--;

		// Cas particulier d'une mantisse a 1: on se deplace de [1, 2[ vers ]1, 2]
		if (cMantissa == 1)
		{
			cMantissa = 2;
			nExponent--;
		}
	}
	return cMantissa;
}

longint MHFloatingPointFrequencyTableBuilder::ExtractMantissaBinInMainBinIndex(Continuous cValue,
									       int nMantissaBitNumber,
									       int nCentralExponent)
{
	longint lMantissaBinIndex;

	require(cValue != KWContinuous::GetMissingValue());
	require(0 <= nMantissaBitNumber and nMantissaBitNumber <= GetMaxMantissaBinBitNumber());

	// Cas d'une valeur dans un central bin
	if (ldexp(-1, nCentralExponent) < cValue and cValue <= ldexp(1, nCentralExponent))
		lMantissaBinIndex = ExtractMantissaBinInCentralBinIndex(cValue, nMantissaBitNumber, nCentralExponent);
	// Cas d'une valeur dans un exponent bin
	else
		lMantissaBinIndex = ExtractMantissaBinInExponentBinIndex(cValue, nMantissaBitNumber);
	return lMantissaBinIndex;
}

longint MHFloatingPointFrequencyTableBuilder::ExtractMantissaBinInExponentBinIndex(Continuous cValue,
										   int nMantissaBitNumber)
{
	longint liMantissaBinIndex;
	int nExponent;
	Continuous cMantissa;

	require(cValue != KWContinuous::GetMissingValue());
	require(0 <= nMantissaBitNumber and nMantissaBitNumber <= GetMaxMantissaBinBitNumber());

	// Cas particulier de la valeur 0
	if (cValue == 0)
		liMantissaBinIndex = 0;
	// Cas general
	else
	{
		// Extraction de la representation virgule flottante
		cMantissa = ExtractMantissa(cValue, nExponent);

		// Cas particulier d'un exponent bin
		if (nMantissaBitNumber == 0)
			liMantissaBinIndex = 0;
		// Cas d'un vrai mantissa bin
		else
		{
			assert(cMantissa != 0);

			// Cas d'une valeur positive
			if (cMantissa > 0)
			{
				assert(cMantissa > 1);
				liMantissaBinIndex = (longint)ceil(ldexp(cMantissa - 1, nMantissaBitNumber)) - 1;
			}
			// Cas d'une valeur negative
			else
			{
				assert(cMantissa > -2);
				liMantissaBinIndex = (longint)(ldexp(1, nMantissaBitNumber) -
							       floor(ldexp(-cMantissa - 1, nMantissaBitNumber))) -
						     1;
			}
		}
		ensure(0 <= liMantissaBinIndex and liMantissaBinIndex <= ldexp(1, nMantissaBitNumber) - 1);
		ensure(liMantissaBinIndex == 0 or
		       BuildValue(ceil(cMantissa) - 1 + liMantissaBinIndex * ldexp(1, -nMantissaBitNumber), nExponent) <
			   cValue);
		ensure(liMantissaBinIndex != 0 or cValue < 0 or BuildValue(2, nExponent - 1) < cValue);
		ensure(liMantissaBinIndex != 0 or cValue > 0 or BuildValue(-1, nExponent + 1) < cValue);
		ensure(cValue <=
		       BuildValue(ceil(cMantissa) - 1 + (liMantissaBinIndex + 1) * ldexp(1, -nMantissaBitNumber),
				  nExponent));
	}
	ensure(0 <= liMantissaBinIndex and liMantissaBinIndex < ldexp(1, nMantissaBitNumber));
	return liMantissaBinIndex;
}

longint MHFloatingPointFrequencyTableBuilder::ExtractMantissaBinInCentralBinIndex(Continuous cValue,
										  int nMantissaBitNumber,
										  int nCentralExponent)
{
	longint liMantissaBinIndex;
	int nExponent;
	Continuous cMantissa;

	require(cValue != KWContinuous::GetMissingValue());
	require(ldexp(-1, nCentralExponent) < cValue and cValue <= ldexp(1, nCentralExponent));
	require(0 <= nMantissaBitNumber and nMantissaBitNumber <= GetMaxMantissaBinBitNumber());

	// Cas particulier de la valeur 0
	if (cValue == 0)
	{
		// On est dans le dernier mantissa bin du central bin negatif
		liMantissaBinIndex = (int)ldexp(1, nMantissaBitNumber) - 1;
	}
	// Cas particulier d'un central bin
	else if (nMantissaBitNumber == 0)
	{
		// Il n'y a qu'un seul mantissa bin dans le central bin
		liMantissaBinIndex = 0;
	}
	// Cas general
	else
	{
		// Extraction de la representation virgule flottante
		cMantissa = ExtractMantissa(cValue, nExponent);
		assert(cMantissa != 0);
		assert(nExponent <= nCentralExponent);

		// On projete la mantisse calculee dans un exponent bin vers le central bin, d'exposant plus grand
		// Cas d'une valeur positive
		if (cMantissa > 0)
		{
			assert(cMantissa > 1);
			liMantissaBinIndex =
			    (longint)ceil(ldexp(cMantissa, nExponent - nCentralExponent + nMantissaBitNumber)) - 1;
			ensure(ldexp(liMantissaBinIndex, nCentralExponent - nMantissaBitNumber) < cValue);
			ensure(cValue <= ldexp(liMantissaBinIndex + 1, nCentralExponent - nMantissaBitNumber));
		}
		// Cas d'une valeur negative
		else
		{
			assert(cMantissa > -2);
			liMantissaBinIndex =
			    (longint)(ldexp(1, nMantissaBitNumber) -
				      floor(ldexp(-cMantissa, nExponent - nCentralExponent + nMantissaBitNumber))) -
			    1;
			ensure(-ldexp(1, nCentralExponent) +
				   ldexp(liMantissaBinIndex, nCentralExponent - nMantissaBitNumber) <
			       cValue);
			ensure(cValue <= -ldexp(1, nCentralExponent) +
					     ldexp(liMantissaBinIndex + 1, nCentralExponent - nMantissaBitNumber));
		}
		ensure(0 <= liMantissaBinIndex and liMantissaBinIndex <= ldexp(1, nMantissaBitNumber) - 1);
	}
	ensure(0 <= liMantissaBinIndex and liMantissaBinIndex < ldexp(1, nMantissaBitNumber));
	return liMantissaBinIndex;
}

Continuous MHFloatingPointFrequencyTableBuilder::BuildValue(Continuous cMantissa, int nExponent)
{
	Continuous cValue;

	require((-2 < cMantissa and cMantissa <= -1) or cMantissa >= 0);
	require((1 < cMantissa and cMantissa <= 2) or cMantissa <= 0);
	require(abs(nExponent) <= 1 + log(KWContinuous::GetMaxValue()) / log(2));
	require(cMantissa != 0 or nExponent == 0);

	cValue = ldexp(cMantissa, nExponent);
	return cValue;
}

void MHFloatingPointFrequencyTableBuilder::ExtractMantissaMainBinBounds(Continuous cValue, int nMantissaBitNumber,
									int nCentralExponent, int& nExponent,
									Continuous& cLowerBound,
									Continuous& cUpperBound)
{
	require(cValue != KWContinuous::GetMissingValue());
	require(0 <= nMantissaBitNumber and nMantissaBitNumber <= GetMaxMantissaBinBitNumber());

	// Cas d'une valeur dans un central bin
	if (ldexp(-1, nCentralExponent) < cValue and cValue <= ldexp(1, nCentralExponent))
	{
		ExtractMantissaCentralBinBounds(cValue, nMantissaBitNumber, nCentralExponent, cLowerBound, cUpperBound);
		nExponent = nCentralExponent;
	}
	// Cas d'une valeur dans un exponent bin
	else
		ExtractMantissaExponentBinBounds(cValue, nMantissaBitNumber, nExponent, cLowerBound, cUpperBound);
}

void MHFloatingPointFrequencyTableBuilder::ExtractMantissaExponentBinBounds(Continuous cValue, int nMantissaBitNumber,
									    int& nExponent, Continuous& cLowerBound,
									    Continuous& cUpperBound)
{
	Continuous cMantissa;
	Continuous cTruncatedMantissa;

	require(cValue != KWContinuous::GetMissingValue());
	require(0 <= nMantissaBitNumber and nMantissaBitNumber <= GetMaxMantissaBinBitNumber());

	// Cas particulier de la valeur 0
	if (cValue == 0)
	{
		cLowerBound = -1;
		cUpperBound = 1;
	}
	// Cas general
	else
	{
		// Extraction de la representation virgule flottante
		cMantissa = ExtractMantissa(cValue, nExponent);

		// Cas particulier d'un exponent bin
		if (nMantissaBitNumber == 0)
		{
			if (cValue >= 0)
				BuildExponentBinBounds(1, nExponent, cLowerBound, cUpperBound);
			else
				BuildExponentBinBounds(-1, nExponent, cLowerBound, cUpperBound);
		}
		// Cas d'un vrai mantissa bin
		else
		{
			// Troncature pour obtenir un index entier de mantissa bin parmi 2^nMantissaBitNumber
			cTruncatedMantissa = ldexp(cMantissa, nMantissaBitNumber);
			cTruncatedMantissa = trunc(cTruncatedMantissa);

			// Recherche des bornes ]lower, upper] dans le cas positif
			if (cMantissa >= 0)
			{
				cLowerBound = ldexp(cTruncatedMantissa, nExponent - nMantissaBitNumber);
				cUpperBound = ldexp(cTruncatedMantissa + 1, nExponent - nMantissaBitNumber);

				// Cas particulier ou la valeur tombe sur la borne inf
				if (cValue == cLowerBound)
				{
					cUpperBound = cLowerBound;
					cLowerBound = ldexp(cTruncatedMantissa - 1, nExponent - nMantissaBitNumber);
				}
			}
			// Recherche des bornes ]lower, upper] dans le cas negatif
			else
			{
				cLowerBound = ldexp(cTruncatedMantissa - 1, nExponent - nMantissaBitNumber);
				cUpperBound = ldexp(cTruncatedMantissa, nExponent - nMantissaBitNumber);
			}
		}
	}
	ensure(cLowerBound < cUpperBound);
	ensure(cLowerBound < cValue and cValue <= cUpperBound);
	ensure(cValue == 0 or cUpperBound - cLowerBound == ldexp(1, nExponent - nMantissaBitNumber));
	ensure(cValue != 0 or cUpperBound - cLowerBound == 2);
}

void MHFloatingPointFrequencyTableBuilder::ExtractMantissaCentralBinBounds(Continuous cValue, int nMantissaBitNumber,
									   int nCentralExponent,
									   Continuous& cLowerBound,
									   Continuous& cUpperBound)
{
	Continuous cMantissa;
	Continuous cTruncatedMantissa;
	int nExponent;

	require(cValue != KWContinuous::GetMissingValue());
	require(cValue == 0 or cValue <= -KWContinuous::GetEpsilonValue() or cValue >= KWContinuous::GetEpsilonValue());
	require(nCentralExponent >= GetMinBinExponent());
	require(ldexp(-1, nCentralExponent) < cValue and cValue <= ldexp(1, nCentralExponent));
	require(0 <= nMantissaBitNumber and nMantissaBitNumber <= GetMaxMantissaBinBitNumber());

	// Cas particulier de la valeur 0
	if (cValue == 0)
	{
		BuildCentralBinBounds(-1, nCentralExponent - nMantissaBitNumber, cLowerBound, cUpperBound);
	}
	// Cas particulier d'un central bin
	else if (nMantissaBitNumber == 0)
	{
		if (cValue > 0)
			BuildCentralBinBounds(1, nCentralExponent, cLowerBound, cUpperBound);
		else
			BuildCentralBinBounds(-1, nCentralExponent, cLowerBound, cUpperBound);
	}
	// Cas d'un vrai mantissa bin
	else
	{
		// Extraction de la representation virgule flottante
		cMantissa = ExtractMantissa(cValue, nExponent);
		assert(cMantissa != 0);
		assert(nExponent <= nCentralExponent);

		// Troncature pour obtenir un index entier de mantissa bin parmi 2^nMantissaBitNumber
		// apres avoir projete la mantisse calculee dans un exponent bin vers le central bin, d'exposant plus
		// grand
		cTruncatedMantissa = ldexp(cMantissa, nExponent - nCentralExponent + nMantissaBitNumber);
		cTruncatedMantissa = trunc(cTruncatedMantissa);

		// Recherche des bornes ]lower, upper] dans le cas positif
		if (cMantissa > 0)
		{
			cLowerBound = ldexp(cTruncatedMantissa, nCentralExponent - nMantissaBitNumber);
			cUpperBound = ldexp(cTruncatedMantissa + 1, nCentralExponent - nMantissaBitNumber);

			// Cas particulier ou la valeur tombe sur la borne inf
			if (cValue == cLowerBound)
			{
				cUpperBound = cLowerBound;
				cLowerBound = ldexp(cTruncatedMantissa - 1, nCentralExponent - nMantissaBitNumber);
			}
		}
		// Recherche des bornes ]lower, upper] dans le cas negatif
		else
		{
			cLowerBound = ldexp(cTruncatedMantissa - 1, nCentralExponent - nMantissaBitNumber);
			cUpperBound = ldexp(cTruncatedMantissa, nCentralExponent - nMantissaBitNumber);
		}
	}
	ensure(cLowerBound < cUpperBound);
	ensure(cLowerBound < cValue and cValue <= cUpperBound);
	ensure(cUpperBound - cLowerBound == ldexp(1, nCentralExponent - nMantissaBitNumber));
}

void MHFloatingPointFrequencyTableBuilder::BuildExponentBinBounds(int nSign, int nExponent, Continuous& cLowerBound,
								  Continuous& cUpperBound)
{
	require(nSign == -1 or nSign == 1);

	// Cas negatif
	if (nSign == -1)
	{
		cLowerBound = ldexp(-1, nExponent + 1);
		cUpperBound = ldexp(-1, nExponent);
	}
	// Cas positif
	else
	{
		cLowerBound = ldexp(1, nExponent);
		cUpperBound = ldexp(1, nExponent + 1);
	}
}

void MHFloatingPointFrequencyTableBuilder::BuildCentralBinBounds(int nSign, int nExponent, Continuous& cLowerBound,
								 Continuous& cUpperBound)
{
	require(nSign == -1 or nSign == 1);

	// Cas negatif
	if (nSign == -1)
	{
		cLowerBound = ldexp(-1, nExponent);
		cUpperBound = 0;
	}
	// Cas positif
	else
	{
		cLowerBound = 0;
		cUpperBound = ldexp(1, nExponent);
	}
	ensure(cLowerBound < cUpperBound);
	ensure(nSign == -1 or cLowerBound == 0);
	ensure(nSign == 1 or cUpperBound == 0);
}

int MHFloatingPointFrequencyTableBuilder::GetMaxMantissaBinBitNumber()
{
	// On prend un digit de moins pour que deux valeurs successives de mantissa bin soient discernables
	// une fois arrondies au nombre max de digits
	return (int)floor((KWContinuous::GetDigitNumber() - 1) * log(10) / log(2));
}

int MHFloatingPointFrequencyTableBuilder::GetMinBinExponent()
{
	int nMinBinExponent;
	ExtractMantissa(KWContinuous::GetEpsilonValue(), nMinBinExponent);
	return nMinBinExponent;
}

int MHFloatingPointFrequencyTableBuilder::GetMaxBinExponent()
{
	int nMaxBinExponent;
	ExtractMantissa(KWContinuous::GetMaxValue(), nMaxBinExponent);
	return nMaxBinExponent;
}

void MHFloatingPointFrequencyTableBuilder::Write(ostream& ost) const
{
	ost << "Floating point frequency table builder\n";
	if (IsInitialized())
	{
		ost << "Total frequency\t" << GetTotalFrequency() << "\n";
		ost << "Min value\t" << GetMinValue() << "\n";
		ost << "Max value\t" << GetMaxValue() << "\n";
		ost << "Max negative value\t" << GetMaxNegativeValue() << "\n";
		ost << "Min positive value\t" << GetMinPositiveValue() << "\n";
		ost << "Min value exponent\t" << GetMinValueExponent() << "\n";
		ost << "Max value exponent\t" << GetMaxValueExponent() << "\n";
		ost << "Min central bin exponent\t" << GetMinCentralBinExponent() << "\n";
		ost << "Max central bin exponent\t" << GetMaxCentralBinExponent() << "\n";
		ost << "Central bin exponent\t" << GetCentralBinExponent() << "\n";
		ost << "Main bin number\t" << GetMainBinNumber() << "\n";
		ost << "Zero main bin index\t" << GetZeroMainBinIndex() << "\n";
		ost << "Floating-point bin hierarchy root level\t" << GetMainBinHierarchyRootLevel() << "\n";
	}
}

void MHFloatingPointFrequencyTableBuilder::WriteMainBins(ostream& ost) const
{
	int nMainBinIndex;
	Continuous cLowerBound;
	Continuous cUpperBound;
	int nSign;
	int nExponent;
	boolean bIsCentralBin;

	if (IsInitialized())
	{
		ost << "Index\tSign\tExponent\tCentral\tLower\tUpper\n";
		for (nMainBinIndex = 0; nMainBinIndex < GetMainBinNumber(); nMainBinIndex++)
		{
			// Extraction des specifications et des bornes du bin
			GetMainBinSpecAt(nMainBinIndex, nSign, nExponent, bIsCentralBin);
			GetMainBinBoundsAt(nMainBinIndex, cLowerBound, cUpperBound);

			// Affchage
			cout << nMainBinIndex << "\t" << nSign << "\t" << nExponent << "\t" << bIsCentralBin << "\t"
			     << cLowerBound << "\t" << cUpperBound << "\n";
		}
	}
}

void MHFloatingPointFrequencyTableBuilder::WriteHierarchyTotalBinNumbers(ostream& ost) const
{
	int nHierarchyLevel;

	if (IsInitialized())
	{
		ost << "Hierarchy level\tTotal bin number\n";
		for (nHierarchyLevel = 0; nHierarchyLevel <= GetMaxHierarchyLevel(); nHierarchyLevel++)
		{
			ost << nHierarchyLevel << "\t";
			ost << GetTotalBinNumberAt(nHierarchyLevel) << "\n";
		}
	}
}

void MHFloatingPointFrequencyTableBuilder::WriteFrequencyTable(KWFrequencyTable* histogramFrequencyTable,
							       ostream& ost) const
{
	int i;
	MHMODLHistogramVector* frequencyVector;

	require(histogramFrequencyTable != NULL);

	// Entete
	ost << "Lower bound\tUpper bound\tLength\tFrequency\n";

	// Vecteurs
	for (i = 0; i < histogramFrequencyTable->GetFrequencyVectorNumber(); i++)
	{
		frequencyVector = cast(MHMODLHistogramVector*, histogramFrequencyTable->GetFrequencyVectorAt(i));
		ost << frequencyVector->GetLowerBound() << "\t";
		ost << frequencyVector->GetUpperBound() << "\t";
		ost << frequencyVector->GetUpperBound() - frequencyVector->GetLowerBound() << "\t";
		ost << frequencyVector->GetFrequency() << "\n";
	}
}

boolean MHFloatingPointFrequencyTableBuilder::Check() const
{
	boolean bOk = true;
	Continuous cLowerBound;
	Continuous cUpperBound;
	Continuous cPreviousLowerBound;
	Continuous cPreviousUpperBound;
	int nMainBinIndex;
	int nIndex;

	// Verification si donnee initialisees
	if (IsInitialized())
	{
		// Verification de base
		bOk = bOk and GetMinValue() <= GetMaxValue();
		bOk = bOk and GetDomainLowerBound() <= GetMinValue();
		bOk = bOk and GetMaxValue() <= GetDomainUpperBound();
		bOk = bOk and (GetMinValue() >= 0 or GetMaxNegativeValue() < 0);
		bOk = bOk and (GetMaxValue() <= 0 or GetMinPositiveValue() > 0);
		bOk = bOk and (GetMainBinNumber() > 0);
		bOk = bOk and (GetMainBinHierarchyRootLevel() >= 0 or
			       -GetMainBinHierarchyRootLevel() <= GetMaxMantissaBinBitNumber());
		bOk = bOk and GetMaxHierarchyLevel() >= 0;
		bOk = bOk and ((GetMinValue() < GetMaxValue() or GetMaxHierarchyLevel() == 0));
		bOk = bOk and GetMaxSafeHierarchyLevel() >= 0;
		bOk = bOk and GetMaxSafeHierarchyLevel() <= GetMaxHierarchyLevel();
		bOk = bOk and GetMinBinLength() > 0;
		assert(bOk);

		// Verification des main bins
		cPreviousLowerBound = 0;
		cPreviousUpperBound = 0;
		for (nMainBinIndex = 0; nMainBinIndex < GetMainBinNumber(); nMainBinIndex++)
		{
			GetMainBinBoundsAt(nMainBinIndex, cLowerBound, cUpperBound);
			assert(cLowerBound < cUpperBound);

			// Cas du premier bin
			if (nMainBinIndex == 0)
			{
				bOk = bOk and (cLowerBound < GetMinValue() and GetMinValue() <= cUpperBound);
				assert(bOk);

				// Verification de la borne inf du domain si elle a ete initialisee
				if (AreDomainBoundsInitialized())
				{
					bOk = bOk and GetDomainLowerBound() < GetMinValue();
					assert(bOk);

					// La borne inf est dans le meme bin que la valeur min, sauf dans de la limite
					// de la precision des Continuous
					bOk = bOk and
					      (cLowerBound <= GetDomainLowerBound() or
					       (0 < cLowerBound and cLowerBound < KWContinuous::GetEpsilonValue() and
						GetDomainLowerBound() == 0) or
					       (-KWContinuous::GetEpsilonValue() < cLowerBound and cLowerBound < 0 and
						GetDomainLowerBound() == -KWContinuous::GetEpsilonValue()));
					assert(bOk);
				}
			}
			assert(bOk);

			// Cas du dernier bin
			if (nMainBinIndex == GetMainBinNumber() - 1)
			{
				bOk = bOk and (cLowerBound < GetMaxValue() and GetMaxValue() <= cUpperBound);

				// Verification de la borne sup du domain si elle a ete initialisee
				if (AreDomainBoundsInitialized())
				{
					bOk = bOk and GetMaxValue() <= GetDomainUpperBound();

					// La borne sup est dans le meme bin que la valeur max, sauf dans de la limite
					// de a precision des Continuous
					bOk = bOk and (GetDomainUpperBound() <= cUpperBound or
						       (-KWContinuous::GetEpsilonValue() < cUpperBound and
							cUpperBound < 0 and GetDomainUpperBound() == 0));
				}
			}
			assert(bOk);

			// Cas du bin contenant 0
			if (nMainBinIndex == nZeroMainBinIndex)
				bOk = bOk and (cLowerBound < 0 and 0 <= cUpperBound);
			if (cLowerBound < 0 and 0 <= cUpperBound)
				bOk = bOk and nMainBinIndex == nZeroMainBinIndex;
			assert(bOk);

			// Verification de la recherche du main bin index d'une valeur
			nIndex = GetMainBinIndex((cLowerBound + cUpperBound) / 2);
			bOk = bOk and nIndex == nMainBinIndex;
			nIndex = GetMainBinIndex(cUpperBound);
			bOk = bOk and nIndex == nMainBinIndex;
			assert(bOk);

			// Comparaison avec le bin precedent
			if (nMainBinIndex > 0)
				bOk = bOk and cLowerBound == cPreviousUpperBound;
			assert(bOk);

			// Memorisation du bin precedent
			cPreviousLowerBound = cLowerBound;
			cPreviousUpperBound = cUpperBound;
		}
	}
	return bOk;
}

int MHFloatingPointFrequencyTableBuilder::ComputeDistinctValueNumber(const ContinuousVector* cvValues)
{
	int nNumber;
	Continuous cValue;
	int i;

	require(cvValues != NULL);

	// Calcul des statistiques sur les valeurs
	nNumber = 0;
	for (i = 0; i < cvValues->GetSize(); i++)
	{
		cValue = cvValues->GetAt(i);
		assert(i == 0 or cValue >= cvValues->GetAt(i - 1));

		// Mise a jour des statistiques
		if (i == 0 or cValue > cvValues->GetAt(i - 1))
			nNumber++;
	}
	return nNumber;
}

void MHFloatingPointFrequencyTableBuilder::Test()
{
	ContinuousVector cvValues;

	// Test simple avec une gaussienne et peu de points
	InitializeGaussianValues(&cvValues, 100);
	TestWithValues("Gaussian values", &cvValues);

	// Test de la representation virgule flotante
	InitializeTestValues(&cvValues);
	StudyFloatingPointRepresentation(&cvValues);

	// Initialisation d'un table de contingence avec les valeurs de test
	TestWithValues("Test values", &cvValues);

	// Initialisation d'un table de contingence avec des valeurs gaussiennes
	InitializeGaussianValues(&cvValues, 10000);
	TestWithValues("Gaussian values", &cvValues);

	// Initialisation d'un table de contingence avec des valeurs uniformes, dans tous les cas de figures
	// Valeurs singleton
	InitializeUniformValues(&cvValues, 0, 0, 0, true, 10000);
	TestWithValues("Uniform values (0, 0, 0, true)", &cvValues);
	InitializeUniformValues(&cvValues, -1, -1, 0, false, 10000);
	TestWithValues("Uniform values (-1, -1, 0, false)", &cvValues);
	InitializeUniformValues(&cvValues, 1, 1, 0, false, 10000);
	TestWithValues("Uniform values (1, 1, 0, false)", &cvValues);

	// Valeurs negatives
	InitializeUniformValues(&cvValues, -10, -1, 0, false, 10000);
	TestWithValues("Uniform values (-10, -1, 0, false)", &cvValues);
	InitializeUniformValues(&cvValues, -10, -1, 0, true, 10000);
	TestWithValues("Uniform values (-10, -1, 0, true)", &cvValues);

	// Valeurs positives
	InitializeUniformValues(&cvValues, 1, 10, 0, false, 10000);
	TestWithValues("Uniform values (1, 10, 0, false)", &cvValues);
	InitializeUniformValues(&cvValues, 1, 10, 0, true, 10000);
	TestWithValues("Uniform values (1, 10, 0, true)", &cvValues);

	// Valeurs de part et d'autre de 0
	InitializeUniformValues(&cvValues, -10, 10, 1, false, 10000);
	TestWithValues("Uniform values (-10, 10, 1, false)", &cvValues);
	InitializeUniformValues(&cvValues, -10, 10, 1, true, 10000);
	TestWithValues("Uniform values (-10, 10, 1, true)", &cvValues);
	InitializeUniformValues(&cvValues, -1, 10, 1, false, 10000);
	TestWithValues("Uniform values (-1, 10, 1, false)", &cvValues);
	InitializeUniformValues(&cvValues, -1, 10, 1, true, 10000);
	TestWithValues("Uniform values (-1, 10, 1, true)", &cvValues);
	InitializeUniformValues(&cvValues, -10, 1, 1, false, 10000);
	TestWithValues("Uniform values (-10, 1, 1, false)", &cvValues);
	InitializeUniformValues(&cvValues, -10, 1, 1, true, 10000);
	TestWithValues("Uniform values (-10, 1, 1, true)", &cvValues);

	// Valeurs d'une petite partie de bin
	InitializeUniformValues(&cvValues, -1.75, -1.5, 0, false, 10000);
	TestWithValues("Uniform values (-1.75, -1.5, 0, false)", &cvValues);
	InitializeUniformValues(&cvValues, -1.75, -1.5, 0, true, 10000);
	TestWithValues("Uniform values (-1.75, -1.5, 0, true)", &cvValues);
	InitializeUniformValues(&cvValues, 1.5, 1.75, 0, false, 10000);
	TestWithValues("Uniform values (1.5, 1.75, 0, false)", &cvValues);
	InitializeUniformValues(&cvValues, 1.5, 1.75, 0, true, 10000);
	TestWithValues("Uniform values (1.5, 1.75, 0, true)", &cvValues);
	InitializeUniformValues(&cvValues, 1200, 1201, 0, false, 10000);
	TestWithValues("Uniform values (1200, 1201, 0, false)", &cvValues);
	InitializeUniformValues(&cvValues, 1.1, 1.10000000000001, 0, false, 10000);
	TestWithValues("Uniform values (1.1, 1.10000000000001, 0, false)", &cvValues);
}

void MHFloatingPointFrequencyTableBuilder::TestWithValues(const ALString& sLabel, const ContinuousVector* cvValues)
{
	const int nMaxDisplayedHierarchicalLevel = 0;
	MHFloatingPointFrequencyTableBuilder frequencyTableBuilder;
	KWFrequencyTable* histogramFrequencyTable;
	int nHierarchicalLevel;
	int nLastSize;
	int nExponent;

	require(cvValues != NULL);

	// Initialisation du builder
	frequencyTableBuilder.InitializeBins(NULL, cvValues, NULL);
	cout << "\n" << sLabel << "\n" << frequencyTableBuilder << endl;

	// Calcul des tables d'intervalles a tous les niveaux de hierarchie
	cout << "Hierarchical level\tTotal bin number\tSize\n";
	nLastSize = 0;
	for (nHierarchicalLevel = 0; nHierarchicalLevel <= frequencyTableBuilder.GetMaxHierarchyLevel();
	     nHierarchicalLevel++)
	{
		frequencyTableBuilder.BuildFrequencyTable(nHierarchicalLevel, histogramFrequencyTable);

		// Affichage des resultats
		cout << nHierarchicalLevel << "\t";
		cout << frequencyTableBuilder.GetTotalBinNumberAt(nHierarchicalLevel) << "\t";
		cout << histogramFrequencyTable->GetFrequencyVectorNumber() << "\n";

		// Verification de la taille
		assert(histogramFrequencyTable->GetFrequencyVectorNumber() >= nLastSize);
		nLastSize = histogramFrequencyTable->GetFrequencyVectorNumber();

		// Affichage detaille de la table si demande
		if (nHierarchicalLevel <= nMaxDisplayedHierarchicalLevel)
		{
			frequencyTableBuilder.WriteFrequencyTable(histogramFrequencyTable, cout);
			cout << "\n";
		}
		delete histogramFrequencyTable;
	}

	// Calcul de la representation a tous les niveaux d'exposant de central bin
	cout << "CentralBinExponent\tMainBinNumber\tZeroMainBinIndex\tMainBinHierarchyRootLevel\tMinBinLength\tMaxHiera"
		"rchyLevel\tMaxSafeHierarchyLevel\n";
	for (nExponent = frequencyTableBuilder.GetMinCentralBinExponent();
	     nExponent <= frequencyTableBuilder.GetMaxCentralBinExponent(); nExponent++)
	{
		frequencyTableBuilder.SetCentralBinExponent(nExponent);

		// Affichage des resultats
		cout << frequencyTableBuilder.GetCentralBinExponent() << "\t";
		cout << frequencyTableBuilder.GetMainBinNumber() << "\t";
		cout << frequencyTableBuilder.GetZeroMainBinIndex() << "\t";
		cout << frequencyTableBuilder.GetMainBinHierarchyRootLevel() << "\t";
		cout << frequencyTableBuilder.GetMinBinLength() << "\t";
		cout << frequencyTableBuilder.GetMaxHierarchyLevel() << "\t";
		cout << frequencyTableBuilder.GetMaxSafeHierarchyLevel() << "\n";
	}
}

void MHFloatingPointFrequencyTableBuilder::StudyDomainBoundsOptimization(ContinuousVector* cvValues)
{
	MHFloatingPointFrequencyTableBuilder tableBuilder;
	int i;
	Continuous cBound;
	Continuous cLowerBound;
	Continuous cUpperBound;
	int nMantissaBitNumber;
	double dPrecisionCost;
	double dMantissaEncodingCost;
	double dLikelihoodCost;
	double dTotalCost;
	double dBestTotalCost;
	boolean bImproved;

	require(cvValues != NULL);

	// Initialisation= du table builder
	tableBuilder.InitializeBins(NULL, cvValues, NULL);

	// Affichage de quelques stats
	cout << "Instances\t" << tableBuilder.GetTotalFrequency() << "\n";
	cout << "Min\t" << tableBuilder.GetMinValue() << "\n";
	cout << "Max\t" << tableBuilder.GetMaxValue() << "\n";
	cout << "Main bin number\t" << tableBuilder.GetMainBinNumber() << "\n";
	cout << "Max hierarchy level\t" << tableBuilder.GetMaxHierarchyLevel() << "\n";

	// Optimisation des bornes
	dBestTotalCost = DBL_MAX;
	for (i = tableBuilder.GetMainBinHierarchyRootLevel(); i <= tableBuilder.GetMaxHierarchyLevel(); i++)
	{
		// Calcul des bornes inf et sup des premiers intervalles de l'histogramme, calculees a la precision
		// maximale pour le MinCentralBinExponent Ces bornes doivent en effet etre independante des choix de
		// modelisation (le choix du CentralBinExponent) pour avoir un modele nul identique et des couts de
		// modelisation comparables
		tableBuilder.ExtractFloatingPointBinBounds(tableBuilder.GetMinValue(), i, cLowerBound, cBound);
		tableBuilder.ExtractFloatingPointBinBounds(tableBuilder.GetMaxValue(), i, cBound, cUpperBound);

		// Calcul du cout total d'encodage
		nMantissaBitNumber = i - tableBuilder.GetMainBinHierarchyRootLevel();
		dPrecisionCost = KWStat::NaturalNumbersUniversalCodeLength(i + 1);
		dMantissaEncodingCost = 2 * i * log(2);
		dLikelihoodCost = tableBuilder.GetTotalFrequency() * log(cUpperBound - cLowerBound);
		dTotalCost = dPrecisionCost + dMantissaEncodingCost + dLikelihoodCost;

		// Test si amelioration
		bImproved = dTotalCost < dBestTotalCost;
		if (bImproved)
			dBestTotalCost = dTotalCost;

		// Affichage
		if (i == 0)
			cout << "d\tlb\tub\tCost\tC_p\tC_m\tC_l\tImproved\n";
		cout << nMantissaBitNumber << "\t";
		cout << cLowerBound << "\t";
		cout << cUpperBound << "\t";
		cout << dTotalCost << "\t";
		cout << dPrecisionCost << "\t";
		cout << dMantissaEncodingCost << "\t";
		cout << dLikelihoodCost << "\t";
		if (bImproved)
			cout << "*";
		cout << "\n";
	}
}

int MHFloatingPointFrequencyTableBuilder::SearchBinIndex(int nSearchedCumulativeFrequency) const
{
	int nIndex;
	int nLowerIndex;
	int nUpperIndex;

	require(IsInitialized());
	require(ivBinCumulatedFrequencies.GetSize() > 0);
	require(0 <= nSearchedCumulativeFrequency and nSearchedCumulativeFrequency <= GetTotalFrequency());

	// Recherche dichotomique
	nLowerIndex = 0;
	nUpperIndex = ivBinCumulatedFrequencies.GetSize() - 1;
	nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
	while (nLowerIndex + 1 < nUpperIndex)
	{
		// Deplacement des bornes de recherche en fonction
		// de la comparaison avec la borne courante
		if (nSearchedCumulativeFrequency <= ivBinCumulatedFrequencies.GetAt(nIndex))
			nUpperIndex = nIndex;
		else
			nLowerIndex = nIndex;

		// Modification du prochain intervalle teste
		nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
	}
	assert(nLowerIndex <= nUpperIndex);
	assert(nUpperIndex <= nLowerIndex + 1);

	// On compare par rapport aux deux index restant
	if (nSearchedCumulativeFrequency <= ivBinCumulatedFrequencies.GetAt(nLowerIndex))
		nIndex = nLowerIndex;
	else
		nIndex = nUpperIndex;

	ensure(nSearchedCumulativeFrequency <= ivBinCumulatedFrequencies.GetAt(nIndex));
	ensure(nIndex == 0 or ivBinCumulatedFrequencies.GetAt(nIndex - 1) < nSearchedCumulativeFrequency);
	return nIndex;
}

Continuous MHFloatingPointFrequencyTableBuilder::GetSystemMinValue() const
{
	return KWContinuous::GetMinValue();
}

Continuous MHFloatingPointFrequencyTableBuilder::GetSystemMaxValue() const
{
	return KWContinuous::GetMaxValue();
}

void MHFloatingPointFrequencyTableBuilder::InitializeDomainBounds()
{
	boolean bDisplay = false;
	const double dEpsilon = 1e-7;
	int nTotalFrequency;
	int i;
	double dBestCost;
	double dCost;
	Continuous cBound;
	Continuous cLowerBound;
	Continuous cUpperBound;

	require(not AreDomainBoundsInitialized());
	require(GetMaxHierarchyLevel() >= 0);
	require(GetCentralBinExponent() == GetMinCentralBinExponent());

	// Optimisation des bornes du domaine, en minimisant le cout global d'encodage des mantisses de ces bornes et de
	// la vraisemblance du modele null
	dBestCost = DBL_MAX;
	nTotalFrequency = GetTotalFrequency();
	for (i = 0; i <= GetMaxHierarchyLevel(); i++)
	{
		// Calcul des bornes inf et sup des premiers intervalles de l'histogramme, calculees a la precision
		// maximale pour le MinCentralBinExponent Ces bornes doivent en effet etre independante des choix de
		// modelisation (le choix du CentralBinExponent) pour avoir un modele nul identique et des couts de
		// modelisation comparables
		ExtractFloatingPointBinBounds(GetMinValue(), i, cLowerBound, cBound);
		ExtractFloatingPointBinBounds(GetMaxValue(), i, cBound, cUpperBound);
		assert(GetDomainLowerBound() <= GetMinValue());
		assert(GetMaxValue() <= GetDomainUpperBound());

		// Calcul du cout total d'encodage
		dCost = nTotalFrequency * log(cUpperBound - cLowerBound);
		dCost += MHMODLHistogramCosts::ComputeDomainBoundsMantissaCost(i);

		// Affichage
		if (bDisplay)
		{
			if (i == 0)
				cout << "i\tmantissa\teps\tlb\tub\tD lb\tD up\tCost\tBestCost\timprove\n";
			cout << i << "\t";
			cout << i - GetMainBinHierarchyRootLevel() << "\t";
			cout << pow(2, -(i - GetMainBinHierarchyRootLevel())) << "\t";
			cout << KWContinuous::ContinuousToString(cLowerBound) << "\t";
			cout << KWContinuous::ContinuousToString(cUpperBound) << "\t";
			cout << KWContinuous::ContinuousToString(GetMinValue() - cLowerBound) << "\t";
			cout << KWContinuous::ContinuousToString(cUpperBound - GetMaxValue()) << "\t";
			cout << dCost << "\t";
			cout << dBestCost << "\t";
			if (dCost < dBestCost - dEpsilon)
				cout << "Best";
			cout << "\n";
		}

		// Memorisation si amelioration
		if (dCost < dBestCost - dEpsilon)
		{
			dBestCost = dCost;

			// Memorisation des bornes
			cDomainLowerBound = cLowerBound;
			cDomainUpperBound = cUpperBound;

			// Memorisation des nombre de bits de mantisse utilises pour encoder les valeurs extremes
			nDomainBoundsMantissaBitNumber = i;
		}
	}
	ensure(AreDomainBoundsInitialized());
}

boolean MHFloatingPointFrequencyTableBuilder::AreDomainBoundsInitialized() const
{
	return (cDomainLowerBound > -DBL_MAX) and (cDomainUpperBound < DBL_MAX);
}

void MHFloatingPointFrequencyTableBuilder::UpdateMaxSafeHierarchyLevel()
{
	double dNumberDistinctValues;
	int nMinDistinctValueNumberPerBin;

	// On s'assure que le nombre de valeurs distinctes encodable par bin est suffisant
	dNumberDistinctValues = MHContinuousLimits::ComputeNumberDistinctValues(GetMinValue(), GetMaxValue());
	nMinDistinctValueNumberPerBin = GetMinDistinctValueNumberPerBin();
	nMaxSafeHierarchyLevel = nMaxHierarchyLevel;
	while (nMaxSafeHierarchyLevel > 0)
	{
		// Arret si le nombre de valeurs distinct encodables est suffisant (critere hautement heuristique)
		if (dNumberDistinctValues >
		    GetTotalBinNumberAt(nMaxSafeHierarchyLevel) * double(nMinDistinctValueNumberPerBin))
			break;

		// Diminution du niveau de hierarchie max
		nMaxSafeHierarchyLevel--;
	}
}

void MHFloatingPointFrequencyTableBuilder::InitializeTestValues(ContinuousVector* cvValues)
{
	require(cvValues != NULL);

	// Initialisation avec des valeurs speciales
	cvValues->SetSize(0);
	cvValues->Add(0);
	cvValues->Add(KWContinuous::GetMinValue());
	cvValues->Add(KWContinuous::GetMaxValue());
	cvValues->Add(-KWContinuous::GetEpsilonValue());
	cvValues->Add(KWContinuous::GetEpsilonValue());
	cvValues->Add(-10);
	cvValues->Add(-8);
	cvValues->Add(-4);
	cvValues->Add(-2);
	cvValues->Add(-1.5);
	cvValues->Add(-1);
	cvValues->Add(-0.5);
	cvValues->Add(-0.25);
	cvValues->Add(-0.1);
	cvValues->Add(0.1);
	cvValues->Add(0.25);
	cvValues->Add(0.5);
	cvValues->Add(1);
	cvValues->Add(1.5);
	cvValues->Add(1.25);
	cvValues->Add(1.1);
	cvValues->Add(1.2);
	cvValues->Add(1.3);
	cvValues->Add(1.4);
	cvValues->Add(1.5);
	cvValues->Add(1.6);
	cvValues->Add(1.7);
	cvValues->Add(1.8);
	cvValues->Add(1.9);
	cvValues->Add(2);
	cvValues->Add(4);
	cvValues->Add(8);
	cvValues->Add(3.14159265358979324);
	cvValues->Add(10);
	cvValues->Add(100);
	cvValues->Add(111.1111111111);

	// Tri du vecteur
	cvValues->Sort();
}

void MHFloatingPointFrequencyTableBuilder::InitializeUniformValues(ContinuousVector* cvValues, Continuous cMin,
								   Continuous cMax, Continuous cEpsilon, boolean bZero,
								   int nValueNumber)
{
	int i;
	int nRandomSeed;

	require(cvValues != NULL);
	require(cMin <= cMax);
	require(cEpsilon >= 0);
	require(cEpsilon == 0 or (cMin <= -cEpsilon and cEpsilon <= cMax));
	require(nValueNumber > 0);

	// Initialisation avec des valeurs gaussiennes
	nRandomSeed = GetRandomSeed();
	SetRandomSeed(0);
	cvValues->SetSize(0);
	for (i = 0; i < nValueNumber; i++)
	{
		// Generation de la valeur 0
		if (i == 0 and bZero)
			cvValues->Add(0);
		// Generation d'un valeur dans le cas d'un intervalle ne contenant pas 0
		else if (cEpsilon == 0)
		{
			assert(cMax <= 0 or cMin >= 0);
			cvValues->Add(cMin + RandomDouble() * (cMax - cMin));
		}
		// Generation d'un valeur dans le cas d'un intervalle contenant 0
		else
		{
			assert(cEpsilon > 0);
			assert(cMin <= -cEpsilon and cEpsilon <= cMax);

			// Valeur dans l'intervalle contenant la valeur min
			if (RandomDouble() < 0.5)
				cvValues->Add(cMin + RandomDouble() * (-cEpsilon - cMin));
			// Valeur dans l'intervalle contenant la valeur max
			else
				cvValues->Add(cEpsilon + RandomDouble() * (cMax - cEpsilon));
		}
	}
	SetRandomSeed(nRandomSeed);

	// Tri du vecteur
	cvValues->Sort();
}

void MHFloatingPointFrequencyTableBuilder::InitializeGaussianValues(ContinuousVector* cvValues, int nValueNumber)
{
	int i;
	int nRandomSeed;

	require(cvValues != NULL);
	require(nValueNumber > 0);

	// Initialisation avec des valeurs gaussiennes
	nRandomSeed = GetRandomSeed();
	SetRandomSeed(0);
	cvValues->SetSize(0);
	for (i = 0; i < nValueNumber; i++)
		cvValues->Add(KWStat::InvStandardNormal(RandomDouble()));
	SetRandomSeed(nRandomSeed);

	// Tri du vecteur
	cvValues->Sort();
}

void MHFloatingPointFrequencyTableBuilder::StudyFloatingPointRepresentation(ContinuousVector* cvValues)
{
	double cValue;
	double cTest;
	int i;
	Continuous cMantissaFP;
	int nExponentFP;
	double cMantissa;
	int nExponent;
	int nValueExponent;
	double dLower;
	double dUpper;
	double dWidth0;
	int n;

	require(cvValues != NULL);

	// Extraction de la mantisse et de l'exposant
	cout << "Value\tMantissa\tExponent\tDiff\tMantissaFP\tExponentFP\tDiffFP\tLower0\tUpper0\tWidth0\tLower1\tUpper"
		"1\tWidth1\tLower2\tUpper2\tWidth2\tLower3\tUpper3\tWidth3\tLower4\tUpper4\tWidth4\tRel width4\n";
	for (i = 0; i < cvValues->GetSize(); i++)
	{
		cValue = cvValues->GetAt(i);

		// Obtention de la mantisse et de l'exposant avec la representation virgule flottante
		cMantissaFP = ExtractMantissa(cValue, nExponentFP);

		// Obtention de la mantisse et de l'exposant avec l'API C, qui renvoie une mantisse normalisee 0.5 <=
		// |m| < 1 dans [0.5, 1[
		cMantissa = frexp(cValue, &nExponent);
		cTest = ldexp(cMantissa, nExponent);

		// Affichage des resultats de base
		cout << cValue << "\t";
		cout << cMantissa << "\t";
		cout << nExponent << "\t";
		cout << cValue - cTest << "\t";
		cout << cMantissaFP << "\t";
		cout << nExponentFP << "\t";
		cout << cValue - BuildValue(cMantissaFP, nExponentFP);

		// Calcul des exponent bins et mantissa bins
		dWidth0 = 0;
		for (n = 0; n <= 4; n++)
		{
			ExtractMantissaExponentBinBounds(cValue, n, nValueExponent, dLower, dUpper);

			// Largeur de l'exponent bin
			if (n == 0)
				dWidth0 = dUpper - dLower;
			assert(dWidth0 / (dUpper - dLower) == ldexp(1, n) or cValue == 0);

			// Affichage
			cout << "\t" << dLower;
			cout << "\t" << dUpper;
			cout << "\t" << dUpper - dLower;
			if (n == 4)
				cout << "\t" << (dUpper - dLower) / dWidth0;
		}
		cout << endl;
	}
}
