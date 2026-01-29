// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHTruncationFloatingPointFrequencyTableBuilder.h"

MHTruncationFloatingPointFrequencyTableBuilder::MHTruncationFloatingPointFrequencyTableBuilder()
{
	dTruncationEpsilon = 0;
	dTruncationBinaryEpsilon = 0;
	nTruncationBinaryEpsilonExponent = 0;
	initialBinsTableBuilder = NULL;
}

MHTruncationFloatingPointFrequencyTableBuilder::~MHTruncationFloatingPointFrequencyTableBuilder()
{
	assert(initialBinsTableBuilder == NULL);
}

void MHTruncationFloatingPointFrequencyTableBuilder::SetTruncationEpsilon(double dValue)
{
	require(dValue >= 0);

	// Reinitialisation
	dTruncationEpsilon = 0;
	dTruncationBinaryEpsilon = 0;
	nTruncationBinaryEpsilonExponent = 0;

	// Initialisation de la gestion de la troncature si necessaire
	if (dValue > 0)
	{
		dTruncationEpsilon = dValue;

		// Recherche de l'exposant correspondant a l'epsilon de troncature
		MHFloatingPointFrequencyTableBuilder::ExtractMantissa(dTruncationEpsilon,
								      nTruncationBinaryEpsilonExponent);
		if (ldexp(1, nTruncationBinaryEpsilonExponent) < dTruncationEpsilon)
			nTruncationBinaryEpsilonExponent++;
		dTruncationBinaryEpsilon = ldexp(1, nTruncationBinaryEpsilonExponent);
		assert(dTruncationBinaryEpsilon >= dTruncationEpsilon);
		assert(dTruncationBinaryEpsilon / 2 < dTruncationEpsilon);
	}
}

double MHTruncationFloatingPointFrequencyTableBuilder::GetTruncationEpsilon() const
{
	return dTruncationEpsilon;
}

double MHTruncationFloatingPointFrequencyTableBuilder::GetTruncationBinaryEpsilon() const
{
	return dTruncationBinaryEpsilon;
}

int MHTruncationFloatingPointFrequencyTableBuilder::GetTruncationBinaryEpsilonExponent() const
{
	return nTruncationBinaryEpsilonExponent;
}

void MHTruncationFloatingPointFrequencyTableBuilder::InitializeBins(const ContinuousVector* cvSourceBinLowerValues,
								    const ContinuousVector* cvSourceBinUpperValues,
								    const IntVector* ivSourceBinFrequencies)
{
	ContinuousVector cvTruncationTransformedBinLowerValues;
	ContinuousVector cvTruncationTransformedBinUpperValues;

	require(dTruncationEpsilon > 0);

	// Transformation des valeurs
	if (cvSourceBinLowerValues != NULL)
		TransformValues(cvSourceBinLowerValues, &cvTruncationTransformedBinLowerValues);
	TransformValues(cvSourceBinUpperValues, &cvTruncationTransformedBinUpperValues);

	// Appel de la methode ancetre avec les valeurs transformees
	// L'appel de SetCentralBinExponent par InitializeValues sera celui par defaut, car a ce moment
	// on n'a pas encore initialise cvTruncationInitialInputValues
	if (cvSourceBinLowerValues != NULL)
		MHFloatingPointFrequencyTableBuilder::InitializeBins(&cvTruncationTransformedBinLowerValues,
								     &cvTruncationTransformedBinUpperValues,
								     ivSourceBinFrequencies);
	else
		MHFloatingPointFrequencyTableBuilder::InitializeBins(NULL, &cvTruncationTransformedBinUpperValues,
								     ivSourceBinFrequencies);
	assert(cMaxNegativeValue == 0 or cMaxNegativeValue <= -dTruncationBinaryEpsilon);
	assert(cMinPositiveValue == 0 or cMinPositiveValue >= dTruncationBinaryEpsilon);

	// Creation et initialisation d'un tableBuilder pour les valeurs initiales
	initialBinsTableBuilder = new MHFloatingPointFrequencyTableBuilder;
	initialBinsTableBuilder->InitializeBins(cvSourceBinLowerValues, cvSourceBinUpperValues, ivSourceBinFrequencies);
	ensure(Check());
}

void MHTruncationFloatingPointFrequencyTableBuilder::Clean()
{
	// Methode ancetre
	MHFloatingPointFrequencyTableBuilder::Clean();

	// Specialisation pour la troncature
	delete initialBinsTableBuilder;
	initialBinsTableBuilder = NULL;
}

void MHTruncationFloatingPointFrequencyTableBuilder::SetCentralBinExponent(int nValue)
{
	Continuous cMinValueLowerBound;
	Continuous cMinValueUpperBound;
	Continuous cMaxValueLowerBound;
	Continuous cMaxValueUpperBound;

	require(dTruncationEpsilon > 0);

	// Appel de la methode ancetre
	MHFloatingPointFrequencyTableBuilder::SetCentralBinExponent(nValue);

	// Cas particulier du premier appel depuis InitializeBins
	if (initialBinsTableBuilder == NULL)
	{
		// Correction temporaire du central bin exponent, pour qu'il soit au moins de la valeur du epsilon de
		// troncature
		assert(GetCentralBinExponent() == nValue);
		if (GetCentralBinExponent() == nTruncationBinaryEpsilonExponent - 1)
			MHFloatingPointFrequencyTableBuilder::SetCentralBinExponent(nValue + 1);
	}

	// Calcul de la longueur du plus petit bin, sur la base du plus petit central bin
	// Attention, ce calcul est fait une fois pour toutes pour le min central bin exponent
	// Par contre, on ne corrige pas le niveau de precision maximale (nExtremeValueMantissaBinBitNumber)
	// qui n'a d'impact que sur le cout de prior des hyper-parametres
	dMinBinLength = ldexp(1, nTruncationBinaryEpsilonExponent);

	// Attention, si la valeur positive min est 2^nTruncationBinaryEpsilonExponent, le central bin exponent positive
	// est ]0, 2^(nTruncationBinaryEpsilonExponent-1)]
	assert(GetCentralBinExponent() - nTruncationBinaryEpsilonExponent >= -1 or
	       dTruncationBinaryEpsilon >= GetMaxValue() - GetMinValue());

	// Correction dans le cas d'une seule valeur: on se place cette fois a la precision minimale
	if (GetMinValue() == GetMaxValue())
	{
		nMainBinHierarchyRootLevel = 0;
		nMinCentralBinExponent = nTruncationBinaryEpsilonExponent;
		nCentralBinExponent = nMinCentralBinExponent;
	}

	// Correction du niveau max de la hierarchie dans le cas d'un epsilon de troncature
	// Plutot que de trouver une formule analytique complexe, on recherche empiriquement le max garantissant des
	// mantissa bin plus grand que le epsilon de troncature On se base ici sur les valeurs Min et Max plutot que sur
	// les bornes du domaines, car cela revient ici au meme, sans avoir a gerer l'effet de bord de la borne inf du
	// domain qui est strictement inferieur a Min
	nMaxHierarchyLevel = 0;
	if (dTruncationBinaryEpsilon <= GetMaxValue() - GetMinValue())
	{
		// Recherche du niveau de hierarchie max
		nMaxHierarchyLevel = -1;
		while (nMaxHierarchyLevel < GetDefaultTotalHierarchyLevel())
		{
			nMaxHierarchyLevel++;

			// Extraction des bornes du bin pour les valeurs min et max
			ExtractFloatingPointBinBounds(GetMinValue(), nMaxHierarchyLevel, cMinValueLowerBound,
						      cMinValueUpperBound);
			ExtractFloatingPointBinBounds(GetMaxValue(), nMaxHierarchyLevel, cMaxValueLowerBound,
						      cMaxValueUpperBound);

			// Arret si necessaire
			if ((cMinValueUpperBound - cMinValueLowerBound < dTruncationBinaryEpsilon) and
			    (cMaxValueUpperBound - cMaxValueLowerBound < dTruncationBinaryEpsilon))
			{
				nMaxHierarchyLevel--;
				break;
			}
			else if ((cMinValueUpperBound - cMinValueLowerBound == dTruncationBinaryEpsilon) and
				 (cMaxValueUpperBound - cMaxValueLowerBound == dTruncationBinaryEpsilon))
			{
				break;
			}
		}
	}
	ensure(nMaxHierarchyLevel >= 0);

	// Correction du niveau max de hierarchie sur vis a vis des limites de la precision numerique
	nMaxSafeHierarchyLevel = min(nMaxSafeHierarchyLevel, nMaxHierarchyLevel);
	ensure(Check());
}

int MHTruncationFloatingPointFrequencyTableBuilder::GetMinOptimizedCentralBinExponent() const
{
	int nMinOptimizedCentralBinExponent;

	require(dTruncationEpsilon > 0);

	// Initialisation dans le cas standard
	if (dTruncationBinaryEpsilon <= GetMaxValue() - GetMinValue())
		nMinOptimizedCentralBinExponent = max(nTruncationBinaryEpsilonExponent, nMinCentralBinExponent);
	// Et dans le cas ou on depasse l'etendues de donnees
	else
		nMinOptimizedCentralBinExponent = nMinCentralBinExponent;
	return nMinOptimizedCentralBinExponent;
}

int MHTruncationFloatingPointFrequencyTableBuilder::GetMaxOptimizedCentralBinExponent() const
{
	return MHFloatingPointFrequencyTableBuilder::GetMaxOptimizedCentralBinExponent();
}

int MHTruncationFloatingPointFrequencyTableBuilder::GetTotalBinNumberAt(int nHierarchyLevel) const
{
	int nTotalBinNumber;
	int nMantissaBitNumber;
	int nMainBinMantissaBinNumber;
	int nTruncatedMantissaBitNumber;
	int nBinIndex;
	int nSign;
	boolean bIsCentralBin;
	int nExponent;
	Continuous cLowerBound;
	Continuous cUpperBound;
	int nBinNumber;
	int nFirstMainBinMantissaBinIndex;
	int nLastMainBinMantissaBinIndex;

	require(IsInitialized());
	require(AreDomainBoundsInitialized());
	require(0 <= nHierarchyLevel and nHierarchyLevel <= GetMaxHierarchyLevel());
	require(dTruncationEpsilon > 0);
	require(initialBinsTableBuilder != NULL);

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
		// Il faut ici traiter les main bin un a un pour gere le cas ou les mantissa bin devient plus
		// petit que le truncation epsilon
		// Ici, il n'y a plus de probleme avec l'intervalle de singularite ]-EpsilonValue, EpsilonValue[,
		// car le epsilon de troncature nous garanti d'e^tre en dehors
		assert(dTruncationBinaryEpsilon > KWContinuous::GetMinValue());

		// Nombre de mantissa bins par main bins, par defaut
		nMantissaBitNumber = nHierarchyLevel - GetMainBinHierarchyRootLevel();
		nMainBinMantissaBinNumber = 1 << nMantissaBitNumber;

		// Prise en compte des mantissa bins pour les main bins
		nTotalBinNumber = 0;
		for (nBinIndex = 0; nBinIndex < GetMainBinNumber(); nBinIndex++)
		{
			GetMainBinSpecAt(nBinIndex, nSign, nExponent, bIsCentralBin);
			GetMainBinBoundsAt(nBinIndex, cLowerBound, cUpperBound);

			// On evite de passer dans les bins plus petits que le truncation epsilon, qui sont des
			// artefacts la representation virgule flotante
			if (nExponent >= nTruncationBinaryEpsilonExponent)
			{
				// Calcul du nombre de bits utilisables pour assurer la contrainte du epsilon de
				// troncature
				nTruncatedMantissaBitNumber =
				    min(nMantissaBitNumber, nExponent - nTruncationBinaryEpsilonExponent);
				nBinNumber = 1 << nTruncatedMantissaBitNumber;
				assert((cUpperBound - cLowerBound) / nBinNumber >= dTruncationBinaryEpsilon);
				nTotalBinNumber += nBinNumber;

				// Correction pour le premier main bin couvrant la borne inf du domaine
				if (nBinIndex == 0)
				{
					nFirstMainBinMantissaBinIndex =
					    (int)floor(nBinNumber * (GetDomainLowerBound() - cLowerBound) /
						       (cUpperBound - cLowerBound));
					nTotalBinNumber -= nFirstMainBinMantissaBinIndex;
				}

				// Correction pour le dernier main bin couvrant la borne sup du domaine
				if (nBinIndex == GetMainBinNumber() - 1)
				{
					nLastMainBinMantissaBinIndex =
					    (int)ceil(nBinNumber * (GetDomainUpperBound() - cLowerBound) /
						      (cUpperBound - cLowerBound));
					nTotalBinNumber -= nBinNumber;
					nTotalBinNumber += nLastMainBinMantissaBinIndex;
				}
			}
		}
		assert(nTotalBinNumber >= 1);
		assert(nTotalBinNumber <= GetMainBinNumber() * nMainBinMantissaBinNumber);
	}
	ensure(1 <= nTotalBinNumber and nTotalBinNumber <= (1 << nHierarchyLevel));
	return nTotalBinNumber;
}

void MHTruncationFloatingPointFrequencyTableBuilder::ExtractFloatingPointBinBounds(Continuous cValue,
										   int nHierarchyBitNumber,
										   Continuous& cLowerBound,
										   Continuous& cUpperBound) const
{
	Continuous cIntervalLength;
	int nIntervalLengthExponent;
	Continuous cMantissa;

	require(dTruncationEpsilon > 0);

	// Appel de la methode ancetre
	MHFloatingPointFrequencyTableBuilder::ExtractFloatingPointBinBounds(cValue, nHierarchyBitNumber, cLowerBound,
									    cUpperBound);
	assert(fabs(cLowerBound) >= KWContinuous::GetEpsilonValue() or cLowerBound == 0);
	assert(fabs(cUpperBound) >= KWContinuous::GetEpsilonValue() or cUpperBound == 0);

	// Redefinition si la longueur de l'intervalle est plus petit que le epsilon binaire de troncature
	cIntervalLength = (cUpperBound - cLowerBound);
	assert(cIntervalLength < dTruncationBinaryEpsilon or
	       (longint)(cIntervalLength / dTruncationBinaryEpsilon) == cIntervalLength / dTruncationBinaryEpsilon or
	       nHierarchyBitNumber < GetMainBinHierarchyRootLevel() or
	       cIntervalLength / dTruncationBinaryEpsilon >= LONG_MAX or
	       cIntervalLength == KWContinuous::GetEpsilonValue());
	if (cIntervalLength < dTruncationBinaryEpsilon)
	{
		assert((longint)(dTruncationBinaryEpsilon / cIntervalLength) ==
			   dTruncationBinaryEpsilon / cIntervalLength or
		       dTruncationBinaryEpsilon / cIntervalLength >= LLONG_MAX or
		       cIntervalLength <= KWContinuous::GetEpsilonValue());

		// On ne traite que le cas d'un intervalle plus grand que le epsilon de la singularite en 0
		// Ce cas arrive rarement, dans le cas d'un bin englobant le epsilon de singularite, et tronque de ce
		// fait. Ces cas seront filtres dans les traitement suivants
		if (fabs(cLowerBound) != KWContinuous::GetEpsilonValue() and
		    fabs(cUpperBound) != KWContinuous::GetEpsilonValue())
		{
			assert(fabs(cLowerBound) > KWContinuous::GetEpsilonValue() or cLowerBound == 0);
			assert(fabs(cUpperBound) > KWContinuous::GetEpsilonValue() or cUpperBound == 0);

			// Recherche de la puissance de 2 correspondant au rapport entre cette longueur et la longueur
			// de l'intervalle
			cMantissa = frexp(cIntervalLength, &nIntervalLengthExponent);
			assert(cMantissa == 0.5 or cIntervalLength == KWContinuous::GetEpsilonValue());
			nIntervalLengthExponent--;
			assert(ldexp(1, nIntervalLengthExponent) == cIntervalLength or
			       cIntervalLength == KWContinuous::GetEpsilonValue());
			assert(nIntervalLengthExponent < nTruncationBinaryEpsilonExponent);

			// On re-extrait les bornes avec moins de bits
			MHFloatingPointFrequencyTableBuilder::ExtractFloatingPointBinBounds(
			    cValue, nHierarchyBitNumber + nIntervalLengthExponent - nTruncationBinaryEpsilonExponent,
			    cLowerBound, cUpperBound);
			assert(cUpperBound - cLowerBound == dTruncationBinaryEpsilon or
			       cUpperBound - cLowerBound == KWContinuous::GetEpsilonValue());
		}
	}
	ensure(cUpperBound - cLowerBound >= dTruncationBinaryEpsilon or
	       cUpperBound - cLowerBound <= KWContinuous::GetEpsilonValue());
}

const MHFloatingPointFrequencyTableBuilder*
MHTruncationFloatingPointFrequencyTableBuilder::GetInitialBinsTableBuilder() const
{
	return initialBinsTableBuilder;
}

Continuous MHTruncationFloatingPointFrequencyTableBuilder::TransformValue(Continuous cValue) const
{
	Continuous cTransformedValue;
	int nExponent;
	int nMantissaBitNumber;
	Continuous cLowerBound;
	Continuous cUpperBound;

	require(dTruncationEpsilon > 0);
	require(dTruncationBinaryEpsilon >= dTruncationEpsilon);
	require(dTruncationBinaryEpsilon == ldexp(1, nTruncationBinaryEpsilonExponent));

	// Transformation de la valeur
	cTransformedValue = (cValue * dTruncationBinaryEpsilon) / dTruncationEpsilon - dTruncationBinaryEpsilon / 2;

	// On evite les valeurs plus petites que le epsilon de troncature
	if (-dTruncationBinaryEpsilon < cTransformedValue and cTransformedValue <= 0)
		cTransformedValue = 0;
	else if (0 < cTransformedValue and cTransformedValue <= dTruncationBinaryEpsilon)
		cTransformedValue = dTruncationBinaryEpsilon;
	// Sinon, on projecte la valeur sur un multiple de epsilon de troncature
	else
	{
		// Recherche de l'exposant associe a la valeur
		MHFloatingPointFrequencyTableBuilder::ExtractMantissa(cTransformedValue, nExponent);
		assert(nExponent >= nTruncationBinaryEpsilonExponent);

		// Calcul du nombre de bits de mantissa pour obtenir le bon niveau de troncature
		nMantissaBitNumber = nExponent - nTruncationBinaryEpsilonExponent;
		nMantissaBitNumber =
		    min(nMantissaBitNumber, MHFloatingPointFrequencyTableBuilder::GetMaxMantissaBinBitNumber());

		// Recherche des bornes du mantissa bin pour se projeter sur la valeur superieure
		MHFloatingPointFrequencyTableBuilder::ExtractMantissaExponentBinBounds(
		    cTransformedValue, nMantissaBitNumber, nExponent, cLowerBound, cUpperBound);
		assert(cUpperBound - cLowerBound >= dTruncationBinaryEpsilon);
		assert(cUpperBound - cLowerBound == dTruncationBinaryEpsilon or
		       nMantissaBitNumber == MHFloatingPointFrequencyTableBuilder::GetMaxMantissaBinBitNumber());
		cTransformedValue = cUpperBound;
	}
	assert(cTransformedValue >= GetSystemMinValue());
	assert(cTransformedValue <= GetSystemMaxValue());

	// Verification de coherence avec la transformation inverse
	assert(fabs(cValue - InverseTransformValue(cTransformedValue)) <= (1 + 1e-5) * dTruncationEpsilon or
	       fabs(cValue) >= ldexp(1, MHFloatingPointFrequencyTableBuilder::GetMaxMantissaBinBitNumber() +
					    nTruncationBinaryEpsilonExponent) or
	       fabs(cValue) >= KWContinuous::GetMaxValue());
	assert(fabs(cValue - InverseTransformValue(cTransformedValue)) <=
	       (1 + 1e-5) * (fabs(cValue) + dTruncationEpsilon));
	return cTransformedValue;
}

Continuous MHTruncationFloatingPointFrequencyTableBuilder::InverseTransformValue(Continuous cTransformedValue) const
{
	Continuous cValue;
	int nExponent;
	int nMantissaBitNumber;
	Continuous cLowerBound;
	Continuous cUpperBound;

	require(dTruncationEpsilon > 0);
	require(dTruncationBinaryEpsilon >= dTruncationEpsilon);
	require(dTruncationBinaryEpsilon == ldexp(1, nTruncationBinaryEpsilonExponent));

	// Cas des valeurs extremes
	if (cTransformedValue >= KWContinuous::GetMaxValue())
		cValue = KWContinuous::GetMaxValue();
	else if (cTransformedValue <= KWContinuous::GetMinValue())
		cValue = KWContinuous::GetMinValue();
	// Transformation inverse de la valeur autour de zero
	else if (-dTruncationBinaryEpsilon <= cTransformedValue and cTransformedValue <= dTruncationBinaryEpsilon)
		cValue = (cTransformedValue * dTruncationEpsilon) / dTruncationBinaryEpsilon + dTruncationEpsilon / 2;
	// Sinon, on passe par l'exposant pour s'adapter a l'ordre de grandeur de la valeur
	else
	{
		// Recherche de l'exposant associe a la valeur
		MHFloatingPointFrequencyTableBuilder::ExtractMantissa(cTransformedValue, nExponent);
		assert(nExponent >= nTruncationBinaryEpsilonExponent);

		// Calcul du nombre de bits de mantissa pour obtenir le bon niveau de troncature
		nMantissaBitNumber = nExponent - nTruncationBinaryEpsilonExponent;
		nMantissaBitNumber =
		    min(nMantissaBitNumber, MHFloatingPointFrequencyTableBuilder::GetMaxMantissaBinBitNumber());

		// Recherche des bornes du mantissa bin pour se projeter sur la valeur la plus proche
		MHFloatingPointFrequencyTableBuilder::ExtractMantissaExponentBinBounds(
		    cTransformedValue, nMantissaBitNumber, nExponent, cLowerBound, cUpperBound);
		assert(cUpperBound - cLowerBound >= dTruncationBinaryEpsilon);
		assert(cUpperBound - cLowerBound == dTruncationBinaryEpsilon or
		       nMantissaBitNumber == MHFloatingPointFrequencyTableBuilder::GetMaxMantissaBinBitNumber());
		cValue =
		    ((cUpperBound + (cUpperBound - cLowerBound) / 2) * dTruncationEpsilon) / dTruncationBinaryEpsilon;
	}
	return cValue;
}

void MHTruncationFloatingPointFrequencyTableBuilder::TransformValues(const ContinuousVector* cvSourceValues,
								     ContinuousVector* cvTransformedValues) const
{
	boolean bDisplay = false;
	int n;
	Continuous cValue;
	Continuous cTransformedValue;

	require(cvSourceValues != NULL);
	require(cvTransformedValues != NULL);

	// Transformations des valeurs
	cvTransformedValues->SetSize(cvSourceValues->GetSize());
	for (n = 0; n < cvSourceValues->GetSize(); n++)
	{
		cValue = cvSourceValues->GetAt(n);

		// Verification
		assert(n == 0 or cValue >= cvSourceValues->GetAt(n - 1));
		assert(cValue <= KWContinuous::GetMaxValue());
		assert(cValue >= KWContinuous::GetMinValue());
		assert(cValue != KWContinuous::GetMissingValue());
		assert(cValue >= 0 or cValue <= -KWContinuous::GetEpsilonValue());
		assert(cValue <= 0 or cValue >= KWContinuous::GetEpsilonValue());

		// On ne transforme la valeur que pour les nouvelles valeurs
		if (n == 0 or cValue > cvSourceValues->GetAt(n - 1))
		{
			cTransformedValue = TransformValue(cValue);

			// Memorisation de la valeur
			assert(cTransformedValue == 0 or fabs(cTransformedValue) >= dTruncationBinaryEpsilon);
			cvTransformedValues->SetAt(n, cTransformedValue);
		}
		else
			cvTransformedValues->SetAt(n, cvTransformedValues->GetAt(n - 1));
		assert(n == 0 or cvTransformedValues->GetAt(n) >= cvTransformedValues->GetAt(n - 1));

		// Affichage
		if (bDisplay)
		{
			if (n == 0)
				cout << "X\tbX\n";
			if (n == 0 or cvSourceValues->GetAt(n) > cvSourceValues->GetAt(n - 1))
				cout << cvSourceValues->GetAt(n) << "\t" << cvTransformedValues->GetAt(n) << "\n";
		}
	}
}

Continuous MHTruncationFloatingPointFrequencyTableBuilder::GetSystemMinValue() const
{
	Continuous cSystemMinValue;

	cSystemMinValue = KWContinuous::GetMinValue();
	if (dTruncationBinaryEpsilon > 0)
		cSystemMinValue =
		    (cSystemMinValue * dTruncationBinaryEpsilon) / dTruncationEpsilon - dTruncationBinaryEpsilon / 2;
	return cSystemMinValue;
}

Continuous MHTruncationFloatingPointFrequencyTableBuilder::GetSystemMaxValue() const
{
	Continuous cSystemMaxValue;

	cSystemMaxValue = KWContinuous::GetMaxValue();
	if (dTruncationBinaryEpsilon > 0)
		cSystemMaxValue =
		    (cSystemMaxValue * dTruncationBinaryEpsilon) / dTruncationEpsilon + dTruncationBinaryEpsilon / 2;
	return cSystemMaxValue;
}

void MHTruncationFloatingPointFrequencyTableBuilder::UpdateMaxSafeHierarchyLevel()
{
	// On ne fait rien dans le cas de la troncature
}

void MHTruncationFloatingPointFrequencyTableBuilder::InitializeDomainBounds()
{
	Continuous cBound;
	int nHierarchyLevel;

	require(GetMaxHierarchyLevel() >= 0);
	require(GetCentralBinExponent() == GetMinCentralBinExponent() or
		GetCentralBinExponent() == GetMinCentralBinExponent() + 1);
	require(dTruncationEpsilon > 0);

	// Les valeurs ont ete transformee avec TransformValues, et il faut proceder de facon similaire pour calculer la
	// borne inf, en se deplacant d'un epsilon de troncature vers la gauche (code est proche de celui de
	// TransformValue) Plutot que de trouver une formule analytique complexe, on recherche empiriquement le max
	// garantissant des mantissa bin plus grand que le epsilon de troncature
	nDomainBoundsMantissaBitNumber = 0;

	// Recherche de la borne inf du domaine
	if (0 < cMinValue and cMinValue <= dTruncationBinaryEpsilon)
		cDomainLowerBound = 0;
	else if (-dTruncationBinaryEpsilon < cMinValue and cMinValue <= 0)
		cDomainLowerBound = -dTruncationBinaryEpsilon;
	else
	{
		nHierarchyLevel = nMaxHierarchyLevel;
		while (nHierarchyLevel >= 0)
		{
			ExtractFloatingPointBinBounds(cMinValue, nMaxHierarchyLevel, cDomainLowerBound, cBound);

			// Arret si on a atteint le epsilon de troncature
			if (cMinValue - cDomainLowerBound >= dTruncationBinaryEpsilon)
			{
				nDomainBoundsMantissaBitNumber = max(nDomainBoundsMantissaBitNumber,
								     nHierarchyLevel - GetMainBinHierarchyRootLevel());
				break;
			}
			nHierarchyLevel--;
		}
	}

	// Recherche de la borne sup du domaine
	if (0 < cMaxValue and cMaxValue <= dTruncationBinaryEpsilon)
		cDomainUpperBound = dTruncationBinaryEpsilon;
	else if (-dTruncationBinaryEpsilon < cMaxValue and cMaxValue <= 0)
		cDomainUpperBound = 0;
	else
	{
		nHierarchyLevel = nMaxHierarchyLevel;
		while (nHierarchyLevel >= 0)
		{
			ExtractFloatingPointBinBounds(cMaxValue, nMaxHierarchyLevel, cBound, cDomainUpperBound);

			// Arret si on a atteint le epsilon de troncature
			if (cDomainUpperBound - cMaxValue >= dTruncationBinaryEpsilon)
			{
				nDomainBoundsMantissaBitNumber = max(nDomainBoundsMantissaBitNumber,
								     nHierarchyLevel - GetMainBinHierarchyRootLevel());
				break;
			}
			nHierarchyLevel--;
		}
	}

	// Le nDomainBoundsMantissaBitNumber qui conditionne le cout des hyper-parametres n'est ici qu'approximativement
	// adapte
	assert(nDomainBoundsMantissaBitNumber <= GetMaxMantissaBinBitNumber());
	assert(GetDomainLowerBound() <= GetMinValue());
	assert(GetMaxValue() <= GetDomainUpperBound());
	assert((GetMinValue() == GetDomainLowerBound()) or
	       (GetMinValue() - GetDomainLowerBound() >= dTruncationBinaryEpsilon) or
	       (GetMinValue() == KWContinuous::GetMinValue()));
	assert((GetDomainUpperBound() - GetMaxValue() == 0) or
	       (GetDomainUpperBound() - GetMaxValue() >= dTruncationBinaryEpsilon) or
	       (GetMaxValue() == KWContinuous::GetMaxValue()));
}
