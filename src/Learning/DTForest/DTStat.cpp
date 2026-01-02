// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTStat.h"
#include "KDMultinomialSampleGenerator.h"

DoubleVector DTStat::vLnCatalan;
DoubleVector DTStat::vLnSchroder;
DoubleVector DTStat::vSchroder;
DoubleVector DTStat::vProbaRissanen;

// const int DTStat::nLnBellTableMaxN = 100;

int DTStat::RissanenMaxNumber(int nMax)
{
	double dsum, dval;
	if (nMax < 1)
		return -1;

	if (nMax != vProbaRissanen.GetSize())
	{
		KDMultinomialSampleGenerator msg;
		msg.ComputeNaturalNumbersUniversalPriorProbs(nMax, &vProbaRissanen);

		dsum = 0.0;
		for (int i = 0; i < vProbaRissanen.GetSize(); i++)
		{
			dsum += vProbaRissanen.GetAt(i);
			if (i > 0)
			{
				vProbaRissanen.SetAt(i, vProbaRissanen.GetAt(i - 1) + vProbaRissanen.GetAt(i));
			}
		}

		for (int i = 0; i < vProbaRissanen.GetSize(); i++)
		{
			vProbaRissanen.SetAt(i, vProbaRissanen.GetAt(i) / dsum);
		}
	}

	dval = RandomDouble();

	for (int i = 0; i < vProbaRissanen.GetSize() - 1; i++)
	{
		if (dval <= vProbaRissanen.GetAt(i))
		{
			return i + 1;
		}
	}

	return (vProbaRissanen.GetSize() + 1);
}

double DTStat::LnCatalan(int nValue)
{
	const int nMaxSize = 5000;
	int i;

	require(nValue >= 0);

	// Calcul si necessaire du tableau des valeurs des factorielles
	if (vLnCatalan.GetSize() == 0)
	{
		// Taillage du vecteur des valeurs
		vLnCatalan.SetSize(nMaxSize);
		vLnCatalan.SetAt(0, 0.);

		// Calcul des valeurs
		for (i = 1; i < nMaxSize; i++)
		{
			vLnCatalan.SetAt(i, KWStat::LnFactorial(2 * i) - KWStat::LnFactorial(i + 1) -
						KWStat::LnFactorial(i));
		}
	}

	// Renvoie de la valeur tabulee si possible
	if (nValue < nMaxSize)
	{
		// assert(fabs(vLnFactorial.GetAt(nValue)-LnGamma(nValue+1)) < 1e-9);
		return vLnCatalan.GetAt(nValue);
	}
	// Sinon, utilisation de la loi Gamma
	else
		return nValue * log(4.) - 1.5 * log(double(nValue)) - 0.5 * log(3.141592);
}

double DTStat::LnSchroder(int nValue)
{
	const int nMaxSize = 5000;
	double dval;
	int i;

	require(nValue >= 0);

	// Calcul si necessaire du tableau des valeurs des factorielles
	if (vLnSchroder.GetSize() == 0)
	{
		// Taillage du vecteur des valeurs
		vLnSchroder.SetSize(nMaxSize);

		vLnSchroder.SetAt(0, 0.);
		vLnSchroder.SetAt(1, 0.);
		vLnSchroder.SetAt(2, log(2.));
		vLnSchroder.SetAt(3, log(6.));

		// Calcul des valeurs
		for (i = 4; i < nMaxSize; i++)
		{
			dval = -1. + 3. * (2. * i - 3.) * exp((vLnSchroder.GetAt(i - 1) - vLnSchroder.GetAt(i - 2))) /
					 (i - 3.);
			dval = log(i - 3.) - log(double(i)) + vLnSchroder.GetAt(i - 2) + log(dval);
			vLnSchroder.SetAt(i, dval);
		}

		vLnSchroder.SetAt(0, 0.);
		vLnSchroder.SetAt(1, 0.);
		vLnSchroder.SetAt(2, 0.);
		for (i = 3; i < nMaxSize; i++)
		{
			dval = vLnSchroder.GetAt(i);
			dval -= log(2.0);
			vLnSchroder.SetAt(i, dval);
		}
	}
	// Renvoie de la valeur tabulee si possible
	if (nValue < nMaxSize)
	{
		// assert(fabs(vLnFactorial.GetAt(nValue)-LnGamma(nValue+1)) < 1e-9);
		return vLnSchroder.GetAt(nValue + 1);
	}
	// Sinon, utilisation de la loi Gamma
	else
		return 0.;
}

double DTStat::NbSchroder(int nValue)
{
	const int nMaxSize = 50;
	double dval;
	int i;

	require(nValue >= 0);

	// Calcul si necessaire du tableau des valeurs des factorielles
	if (vSchroder.GetSize() == 0)
	{
		// Taillage du vecteur des valeurs
		vSchroder.SetSize(nMaxSize);

		vSchroder.SetAt(0, 1);
		vSchroder.SetAt(1, 1.);
		vSchroder.SetAt(2, 2.);

		// Calcul des valeurs
		for (i = 3; i < nMaxSize; i++)
		{
			dval = (3. * (2. * i - 3.) * vSchroder.GetAt(i - 1) - (i - 3.) * vSchroder.GetAt(i - 2)) /
			       double(i);
			vSchroder.SetAt(i, dval);
		}
	}
	// Renvoie de la valeur tabulee si possible
	if (nValue < nMaxSize)
	{
		// assert(fabs(vLnFactorial.GetAt(nValue)-LnGamma(nValue+1)) < 1e-9);
		return vSchroder.GetAt(nValue);
	}
	// Sinon, utilisation de la loi Gamma
	else
		return 0.;
}

// Longueur de code universel des entier selon Rissanen
// Presente par exemple dans Hansen et Yu (1998) "Model Selection and the
//   Principle of Minimum Description Length"

double DTStat::NumbersCodeLength1(int n)
{
	const double dLog2 = log(2.0);
	double dCost;
	double dLogI;

	require(n > 0);

	dLogI = log(1.0 * n) / dLog2;

	dCost = 2 * dLogI + 1;

	return dCost;
}

double DTStat::NumbersCodeLength2(int n)
{
	const double dLog2 = log(2.0);
	double dCost;
	double dLogI;
	double dLogI2;

	require(n > 0);

	if (n == 1)
		return 1.;

	dLogI = log(1.0 * n) / dLog2;
	dLogI2 = log(dLogI) / dLog2;

	dCost = dLogI + 2 * dLogI2 + 1;

	return dCost;
}

////////////////////////////////////////////////////////////////////////////
