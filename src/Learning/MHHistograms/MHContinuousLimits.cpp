// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHContinuousLimits.h"

Continuous MHContinuousLimits::ComputeClosestLowerBound(Continuous cValue)
{
	Continuous cLowerBound;

	require(cValue != KWContinuous::GetMissingValue());

	// Cas des valeurs speciales
	if (cValue == KWContinuous::GetMinValue())
		cLowerBound = KWContinuous::GetMinValue();
	else if (cValue == 0)
		cLowerBound = -KWContinuous::GetEpsilonValue();
	else if (cValue == KWContinuous::GetEpsilonValue())
		cLowerBound = 0;
	// Cas general
	else
	{
		if (cValue < 0)
			cLowerBound = (1 + pow(10, -(KWContinuous::GetDigitNumber() - 1))) * cValue;
		else
			cLowerBound = (1 - pow(10, -(KWContinuous::GetDigitNumber() - 1))) * cValue;
		cLowerBound = KWContinuous::DoubleToContinuous(cLowerBound);
		assert(cLowerBound < cValue);
	}
	return cLowerBound;
}

Continuous MHContinuousLimits::ComputeClosestUpperBound(Continuous cValue)
{
	Continuous cUpperBound;

	require(cValue != KWContinuous::GetMissingValue());

	// Cas des valeurs speciales
	if (cValue == KWContinuous::GetMaxValue())
		cUpperBound = KWContinuous::GetMaxValue();
	else if (cValue == 0)
		cUpperBound = KWContinuous::GetEpsilonValue();
	else if (cValue == -KWContinuous::GetEpsilonValue())
		cUpperBound = 0;
	// Cas general
	else
	{
		if (cValue < 0)
			cUpperBound = (1 - pow(10, -(KWContinuous::GetDigitNumber() - 1))) * cValue;
		else
			cUpperBound = (1 + pow(10, -(KWContinuous::GetDigitNumber() - 1))) * cValue;
		cUpperBound = KWContinuous::DoubleToContinuous(cUpperBound);
		assert(cUpperBound > cValue);
	}
	return cUpperBound;
}

double MHContinuousLimits::ComputeNumberDistinctValues(Continuous cMinValue, Continuous cMaxValue)
{
	double dExpectedNumberDistinctValues;
	double dTotalDistinctValuesPerExponent;
	int nMinExponent;
	int nMaxExponent;

	require(CheckContinuousValueBounds(cMinValue));
	require(CheckContinuousValueBounds(cMaxValue));
	require(cMinValue <= cMaxValue);

	// Si les deux valeurs sont egales, cela fait 1
	if (cMinValue == cMaxValue)
		dExpectedNumberDistinctValues = 1;
	// Cas de valeurs dont une vaut 0
	else if (cMinValue == 0)
		dExpectedNumberDistinctValues =
		    1 + ComputeNumberDistinctValues(KWContinuous::GetEpsilonValue(), cMaxValue);
	else if (cMaxValue == 0)
		dExpectedNumberDistinctValues =
		    1 + ComputeNumberDistinctValues(cMinValue, -KWContinuous::GetEpsilonValue());
	// Cas de valeurs negatives
	else if (cMaxValue < 0)
		dExpectedNumberDistinctValues = ComputeNumberDistinctValues(-cMaxValue, -cMinValue);
	// Cas de valeurs positives
	else if (cMinValue > 0)
	{
		// On compte le nombre de valeurs par exposant: entre 1.000... et 9.999...
		dTotalDistinctValuesPerExponent =
		    pow(10, KWContinuous::GetDigitNumber()) - pow(10, KWContinuous::GetDigitNumber() - 1);

		// Exposant de la valeur min
		nMinExponent = (int)floor(log(cMinValue) / log(10));
		if (pow(10, nMinExponent) > 2 * cMinValue)
			nMinExponent--;

		// Exposant de la valeur max
		nMaxExponent = (int)floor(log(cMaxValue) / log(10));
		if (pow(10, nMaxExponent) > 2 * cMaxValue)
			nMaxExponent--;
		assert(nMinExponent <= nMaxExponent);

		// Nombre de valeurs par plage d'exposant
		dExpectedNumberDistinctValues = (nMaxExponent - nMinExponent) * dTotalDistinctValuesPerExponent;

		// On rajoute les valeur encodables par la mantisse entre la valeur max et l'exposant superieur
		dExpectedNumberDistinctValues += ceil((cMaxValue - pow(10, nMaxExponent)) *
						      pow(10, KWContinuous::GetDigitNumber() - 1 - nMaxExponent));

		// On supprime les valeur encodables par la mantisse entre la valeur min et l'exposant inferieur
		dExpectedNumberDistinctValues -= floor((cMinValue - pow(10, nMinExponent)) *
						       pow(10, KWContinuous::GetDigitNumber() - 1 - nMinExponent));
		assert(dExpectedNumberDistinctValues >=
		       (nMaxExponent - nMinExponent - 1) * dTotalDistinctValuesPerExponent);
		assert(dExpectedNumberDistinctValues <=
		       (nMaxExponent - nMinExponent + 1) * dTotalDistinctValuesPerExponent);
	}
	// Cas de valeurs de part et d'autre de 0
	else
	{
		assert(cMinValue < 0);
		assert(cMaxValue > 0);
		dExpectedNumberDistinctValues =
		    ComputeNumberDistinctValues(cMinValue, 0) + ComputeNumberDistinctValues(0, cMaxValue) - 1;
	}
	ensure(dExpectedNumberDistinctValues > 0);
	return dExpectedNumberDistinctValues;
}

boolean MHContinuousLimits::CheckContinuousValueBounds(Continuous cValue)
{
	boolean bOk = true;

	bOk = bOk and cValue != KWContinuous::GetMissingValue();
	if (bOk)
	{
		if (cValue > 0)
		{
			bOk = bOk and cValue >= KWContinuous::GetEpsilonValue();
			bOk = bOk and cValue <= KWContinuous::GetMaxValue();
		}
		else if (cValue < 0)
		{
			bOk = bOk and cValue <= -KWContinuous::GetEpsilonValue();
			bOk = bOk and cValue >= -KWContinuous::GetMaxValue();
		}
	}
	return bOk;
}
