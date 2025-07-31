// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWStat.h"

DoubleVector KWStat::dvLnFactorial;

DoubleVector KWStat::dvLnBell;

DoubleVector KWStat::dvLnStar;
DoubleVector KWStat::dvC0Max;

double KWStat::Min(const DoubleVector* dvValues)
{
	int i;
	double dMin;

	require(dvValues != NULL);

	// Premiere valeur
	dMin = 0;
	if (dvValues->GetSize() > 0)
		dMin = dvValues->GetAt(0);

	// Comparaison avecc les valeurs restantes
	for (i = 1; i < dvValues->GetSize(); i++)
	{
		if (dvValues->GetAt(i) < dMin)
			dMin = dvValues->GetAt(i);
	}

	return dMin;
}

double KWStat::Max(const DoubleVector* dvValues)
{
	int i;
	double dMax;

	require(dvValues != NULL);

	// Premiere valeur
	dMax = 0;
	if (dvValues->GetSize() > 0)
		dMax = dvValues->GetAt(0);

	// Comparaison avecc les valeurs restantes
	for (i = 1; i < dvValues->GetSize(); i++)
	{
		if (dvValues->GetAt(i) > dMax)
			dMax = dvValues->GetAt(i);
	}

	return dMax;
}

double KWStat::Mean(const DoubleVector* dvValues)
{
	int i;
	double dValueSum;

	require(dvValues != NULL);

	dValueSum = 0;
	for (i = 0; i < dvValues->GetSize(); i++)
		dValueSum += dvValues->GetAt(i);
	if (dvValues->GetSize() > 0)
		return dValueSum / dvValues->GetSize();
	else
		return 0;
}

double KWStat::GeometricMean(const DoubleVector* dvValues)
{
	int i;
	double dLogValueSum;

	require(dvValues != NULL);

	// On va passer par la moyenne arithmetique des log des valeurs
	dLogValueSum = 0;
	for (i = 0; i < dvValues->GetSize(); i++)
	{
		assert(dvValues->GetAt(i) >= 0);

		// La moyenne geometrique est nulle si une des valeurs est nulle
		if (dvValues->GetAt(i) <= 0)
			return 0;
		else
			dLogValueSum += log(dvValues->GetAt(i));
	}

	// S'il y a au moins une valeur, on retourne l'exponentielle de
	// la moyenne arithmetique des log des valeurs
	if (dvValues->GetSize() > 0)
		return exp(dLogValueSum / dvValues->GetSize());
	else
		return 0;
}

double KWStat::StandardDeviation(const DoubleVector* dvValues)
{
	int i;
	double dValue;
	double dValueSum;
	double dValueSquareSum;
	int nNumber;

	require(dvValues != NULL);

	// Calcul de l'ecart type
	dValueSum = 0;
	dValueSquareSum = 0;
	nNumber = dvValues->GetSize();
	for (i = 0; i < nNumber; i++)
	{
		dValue = dvValues->GetAt(i);
		dValueSum += dValue;
		dValueSquareSum += dValue * dValue;
	}
	if (nNumber > 0)
		return sqrt(fabs((dValueSquareSum - dValueSum * dValueSum / nNumber) / nNumber));
	else
		return 0;
}

double KWStat::TValue(const DoubleVector* dvValues1, const DoubleVector* dvValues2)
{
	int i;
	double dMean;
	double dStandardDeviation;
	double dValue;
	double dValueSum;
	double dValueSquareSum;
	int nNumber;

	require(dvValues1 != NULL);
	require(dvValues2 != NULL);
	require(dvValues1->GetSize() == dvValues2->GetSize());

	// Calcul de la moyenne, de l'ecart type des differences de resultats
	dMean = 0;
	dStandardDeviation = 0;
	dValue = 0;
	dValueSum = 0;
	dValueSquareSum = 0;
	nNumber = dvValues1->GetSize();
	for (i = 0; i < nNumber; i++)
	{
		dValue = dvValues1->GetAt(i) - dvValues2->GetAt(i);
		dValueSum += dValue;
		dValueSquareSum += dValue * dValue;
	}
	if (nNumber > 0)
	{
		dMean = dValueSum / nNumber;
		dStandardDeviation = sqrt(fabs((dValueSquareSum - dValueSum * dValueSum / nNumber) / nNumber));
	}

	// Calcul de la valeur de Student
	if (nNumber > 0)
		return dMean * sqrt(1.0 * nNumber) / (dStandardDeviation + 1e-5);
	else
		return 0;
}

double KWStat::Normal(double dX, double dMean, double dStandardDeviation)
{
	require(dStandardDeviation > 0);
	return StandardNormal((dX - dMean) / dStandardDeviation);
}

double KWStat::StandardNormal(double dX)
{
	return 1 - 0.5 * Erfc(dX / sqrt(2.));
}

double KWStat::InvNormal(double dProb, double dMean, double dStandardDeviation)
{
	const double dTolerance = 1e-7;
	const double dMax = 1e20;
	double dLowerX;
	double dUpperX;
	double dNewX;
	double dNewXVal;

	require(0 <= dProb and dProb <= 1);
	require(dStandardDeviation > 0);

	// Initialisation des bornes de l'intervalle de recherche
	dLowerX = -dMax;
	dUpperX = dMax;

	// Recherche par dichotomie de la valeur la plus proche
	while ((dUpperX - dLowerX) > dTolerance * (fabs(dLowerX) + fabs(dUpperX)))
	{
		// Calcul d'une nouvelle valeur
		dNewX = (dLowerX + dUpperX) / 2;
		dNewXVal = Normal(dNewX, dMean, dStandardDeviation);

		// Changement des bornes en fonction du resultat
		if (dNewXVal < dProb)
		{
			dLowerX = dNewX;
		}
		else
		{
			dUpperX = dNewX;
		}
	}
	return dLowerX;
}

double KWStat::InvStandardNormal(double dProb)
{
	const double dTolerance = 1e-7;
	const double dMax = 1e20;
	double dLowerX;
	double dUpperX;
	double dLowerXVal;
	double dUpperXVal;
	double dNewX;
	double dNewXVal;

	require(0 <= dProb and dProb <= 1);

	// Initialisation des bornes de l'intervalle de recherche
	dLowerX = -dMax;
	dLowerXVal = 0;
	dUpperX = dMax;
	dUpperXVal = 1;

	// Recherche par dichotomie de la valeur la plus proche
	while ((dUpperX - dLowerX) > dTolerance * (fabs(dLowerX) + fabs(dUpperX)))
	{
		// Calcul d'une nouvelle valeur
		dNewX = (dLowerX + dUpperX) / 2;
		dNewXVal = StandardNormal(dNewX);

		// Changement des bornes en fonction du resultat
		if (dNewXVal < dProb)
		{
			dLowerX = dNewX;
			dLowerXVal = dNewXVal;
		}
		else
		{
			dUpperX = dNewX;
			dUpperXVal = dNewXVal;
		}
	}
	return dLowerX;
}

double KWStat::BinomialProb(int n, double dProb, int k)
{
	double dResult;
	int i;

	require(0 <= n);
	require(0 <= k and k <= n);
	require(0 <= dProb and dProb <= 1);

	// Calcul de c(n,k)pow(p,k)pow(p,n-k)
	dResult = 1;
	if (k <= n - k)
	{
		for (i = 0; i < k; i++)
			dResult *= (n - i) * dProb * (1 - dProb) / (k - i);
		for (i = 0; i < n - 2 * k; i++)
			dResult *= 1 - dProb;
	}
	else
	{
		for (i = 0; i < n - k; i++)
			dResult *= (n - i) * dProb * (1 - dProb) / (n - k - i);
		for (i = 0; i < 2 * k - n; i++)
			dResult *= dProb;
	}

	return dResult;
}

double KWStat::Erf(double dX)
{
	// Computation of the error function erf(x).
	//
	//--- NvE 14-nov-1998 UU-SAP Utrecht

	return (1 - Erfc(dX));
}

double KWStat::Erfc(double dX)
{
	// Computation of the complementary error function erfc(x).
	//
	// The algorithm is based on a Chebyshev fit as denoted in
	// Numerical Recipes 2nd ed. on p. 214 (W.H.Press et al.).
	//
	// The fractional error is always less than 1.2e-7.
	//
	//--- Nve 14-nov-1998 UU-SAP Utrecht

	// The parameters of the Chebyshev fit
	const double a1 = -1.26551223, a2 = 1.00002368, a3 = 0.37409196, a4 = 0.09678418, a5 = -0.18628806,
		     a6 = 0.27886807, a7 = -1.13520398, a8 = 1.48851587, a9 = -0.82215223, a10 = 0.17087277;

	double v = 1; // The return value
	double z = fabs(dX);

	if (z <= 0)
		return v; // erfc(0)=1

	double t = 1 / (1 + 0.5 * z);

	v = t * exp((-z * z) + a1 +
		    t * (a2 + t * (a3 + t * (a4 + t * (a5 + t * (a6 + t * (a7 + t * (a8 + t * (a9 + t * a10)))))))));

	if (dX < 0)
		v = 2 - v; // erfc(-x)=2-erfc(x)

	return v;
}

double KWStat::Student(double dTValue, int ndf)
{
	require(dTValue >= 0);
	require(ndf >= 1);

	return BetaI(ndf / 2.0, 0.5, ndf / (ndf + dTValue * dTValue));
}

double KWStat::InvStudent(double dProb, int ndf)
{
	const double dTolerance = 1e-7;
	const double dMax = 1e20;
	double dLowerX;
	double dUpperX;
	double dLowerXVal;
	double dUpperXVal;
	double dNewX;
	double dNewXVal;

	require(0 <= dProb and dProb <= 1);
	require(ndf >= 1);

	// Cas particulier: probabilite nulle ou probabilite unite
	if (dProb == 0)
		return 0;
	if (dProb == 1)
		return dMax;

	// Initialisation des bornes de l'intervalle de recherche
	dLowerX = 0;
	dLowerXVal = 0;
	dUpperX = dMax;
	dUpperXVal = 1;

	// Recherche par dichotomie de la valeur la plus proche
	while ((dUpperX - dLowerX) > dTolerance * (fabs(dLowerX) + fabs(dUpperX)))
	{
		// Calcul d'une nouvelle valeur
		dNewX = (dLowerX + dUpperX) / 2;
		dNewXVal = Student(dNewX, ndf);

		// Changement des bornes en fonction du resultat
		if (dNewXVal > dProb)
		{
			dLowerX = dNewX;
			dLowerXVal = dNewXVal;
		}
		else
		{
			dUpperX = dNewX;
			dUpperXVal = dNewXVal;
		}
	}
	return dLowerX;
}

double KWStat::LnFactorial(int nValue)
{
	int i;

	require(nValue >= 0);

	// Renvoie de la valeur tabulee si possible
	if (nValue < nLnFactorialTableSize)
	{
		// Calcul si necessaire du tableau des valeurs des factorielles
		if (dvLnFactorial.GetSize() == 0)
		{
			// Taillage du vecteur des valeurs
			dvLnFactorial.SetSize(nLnFactorialTableSize);

			// Calcul des valeurs
			for (i = 1; i < nLnFactorialTableSize; i++)
			{
				dvLnFactorial.SetAt(i, dvLnFactorial.GetAt(i - 1) + log(1.0 * i));
			}
		}
		assert(fabs(dvLnFactorial.GetAt(nValue) - LnGamma(nValue + 1)) < (nValue + 1) * 1e-9);
		assert(nValue < 60 or
		       fabs(dvLnFactorial.GetAt(nValue) - LnGammaRamanujan(nValue + 1)) < (nValue + 1) * 1e-9);
		return dvLnFactorial.GetAt(nValue);
	}
	// Sinon, utilisation de la loi Gamma
	else
		return LnGamma(nValue + 1);
}

// Pour les explications sur le calcul des nombre de Bell generalises, se
// referer a la note technique FTR&D sur le groupage MODL
double KWStat::LnBell(int n, int k)
{
	require(n >= 1);
	require(1 <= k and k <= n);

	// Calcul si necessaire du tableau des valeurs de Bell generalisees
	if (dvLnBell.GetSize() == 0)
		ComputeLnBellTable();

	// Renvoie de la valeur tabulee si possible
	if (n < nLnBellTableMaxN)
		return dvLnBell.GetAt((n - 1) * nLnBellTableMaxN + k - 1);
	// Sinon, utilisation de l'approximation
	else
		return ComputeLnBellValue(n, k);
}

// Longueur de code universel des entier selon Rissanen
// Ref: A Universal Prior for Integers and Estimation by Minimum Description Length
//    Rissanen 1983
// Presente par exemple dans Hansen et Yu (1998) "Model Selection and the
//   Principle of Minimum Description Length"
double KWStat::NaturalNumbersUniversalCodeLength(int n)
{
	// const double dC0Rissanen = 2.865064; // indique dans article de Rissanen
	const double dC0 = 2.86511; // re calcule a partir de la valeur exacte en e(3)=65536 + dLog2^5 / (1-dLog2) selon
				    // l'estimation donnee par Rissanen
	const double dLog2 = log(2.0);
	double dCost;

	require(1 <= n);

	// Initialisation a log_2(C0)
	dCost = log(dC0) / dLog2;

	// Ajout du log_2*(n)
	dCost += LnStar(n);

	// Passage en log naturel
	dCost *= dLog2;
	return dCost;
}

double KWStat::BoundedNaturalNumbersUniversalCodeLength(int n, int nMax)
{
	const double dLog2 = log(2.0);
	double dCost;

	require(1 <= n);

	// Initialisation a log_2(C0(nMax))
	dCost = log(C0Max(nMax)) / dLog2;

	// Ajout du log_2*(n)
	dCost += LnStar(n);

	// Passage en log naturel
	dCost *= dLog2;
	return dCost;
}

double KWStat::LnStar(int n)
{
	const double dLog2 = log(2.0);
	double dCost;
	double dLogI;

	require(n > 0);

	// Calcul si necessaire du tableau des valeurs des LnStar et des C0Max
	if (dvLnStar.GetSize() == 0 or dvC0Max.GetSize() == 0)
		ComputeLnStarAndC0MaxTables();

	// Renvoi de la valeur tabulee si possible
	if (n < dvLnStar.GetSize())
		return dvLnStar.GetAt(n - 1);

	// Sinon on calcule
	else
	{
		// Codage de log*(n) en base 2
		dCost = 0;
		dLogI = log(1.0 * n) / dLog2;
		while (dLogI > 0)
		{
			dCost += dLogI;
			dLogI = log(dLogI) / dLog2;
		}
		return dCost;
	}
}

double KWStat::C0Max(int nMax)
{
	const double dLog2 = log(2.0);
	const int nE3 = 65536;
	double dC0Max;

	require(nMax >= 1);

	// Calcul si necessaire du tableau des valeurs des LnStar et des C0Max
	if (dvLnStar.GetSize() == 0 or dvC0Max.GetSize() == 0)
		ComputeLnStarAndC0MaxTables();

	// Renvoi de la valeur tabulee si possible
	if (nMax < dvC0Max.GetSize())
	{
		dC0Max = dvC0Max.GetAt(nMax - 1);
	}
	// Sinon, on approxime le reste de la somme entre la derniere valeur tabulee et C0(Max)
	// par le calcul de l'integrale associee
	// Voir article de Rissanen pour l'estimation de la borne de l'erreur
	// Somme_{a}^{b} < Integrale_{a}^{b} 2^(-log*(x)) dx
	// Or, quand x est dans l'intervalle [65536, 2^65536], log*(x) est la somme avec au max 5 compositions log(5)
	// On a alors 2^(-log*(x)) = dLog2^5 Dlog(5)(x)
	// Cette majoration est bornee par 2^(-log*(taille du tableau) - 2^(-log*(nMax)).
	// Cette borne croit quand la taille du tableau diminue et quand nMax croit. Elle vaut environ 7.38*10^-7 pour
	// TabMax=2000 et nMax=10^8
	else
	{
		// Initialisation de la somme a la derniere valeur tabulee
		dC0Max = dvC0Max.GetAt(dvC0Max.GetSize() - 1);

		// Cas ou la derniere valeur tabulee est < e(3) = 65536
		if (dvC0Max.GetSize() < nE3)
			// Cas ou nMax < e3
			if (nMax < nE3)
				dC0Max += pow(dLog2, 4) *
					  (log(log(log(log(nMax * 1.0) / dLog2) / dLog2) / dLog2) / dLog2 -
					   log(log(log(log(dvC0Max.GetSize() * 1.0) / dLog2) / dLog2) / dLog2) / dLog2);
			// Cas ou e3 < nMax
			else
				dC0Max += pow(dLog2, 4) *
					      (1 - log(log(log(log(dvC0Max.GetSize() * 1.0) / dLog2) / dLog2) / dLog2) /
						       dLog2) +
					  pow(dLog2, 5) *
					      log(log(log(log(log(nMax * 1.0) / dLog2) / dLog2) / dLog2) / dLog2) /
					      dLog2;
		// Cas ou e(3) < derniere valeur tabulee < nMax
		else
		{
			// Ajout de l'estimation du reste de la somme au dela de e(3)
			dC0Max +=
			    pow(dLog2, 5) *
			    (log(log(log(log(log(nMax * 1.0) / dLog2) / dLog2) / dLog2) / dLog2) / dLog2 -
			     log(log(log(log(log(dvC0Max.GetSize() * 1.0) / dLog2) / dLog2) / dLog2) / dLog2) / dLog2);
		}
	}
	return dC0Max;
}

void KWStat::ComputeLnStarAndC0MaxTables()
{
	const double dLog2 = log(2.0);
	double dLogI;
	int i;
	double dCost;

	// Taillage du vecteur des valeurs
	dvLnStar.SetSize(nLnStarTableMaxN);
	dvLnStar.SetAt(0, 0.0);
	dvC0Max.SetSize(nLnStarTableMaxN);
	dvC0Max.SetAt(0, 1.0);

	// On remplit le tableau (on commence a n=2 pour LnStar)
	for (i = 1; i < dvLnStar.GetSize(); i++)
	{
		// Calcul de log*(i+1) base 2 = somme_{j>1} max(log^(j) (i+1),0)
		dCost = 0;
		dLogI = log(1.0 * (i + 1)) / dLog2;
		while (dLogI > 0)
		{
			dCost += dLogI;
			dLogI = log(dLogI) / dLog2;
		}
		dvLnStar.SetAt(i, dCost);
		dvC0Max.SetAt(i, dvC0Max.GetAt(i - 1) + pow(2.0, -dCost));
	}
}

void KWStat::ComputeLnBellTable()
{
	int nMaxSize = nLnBellTableMaxN * nLnBellTableMaxN;
	boolean bPrintStirling = false;
	boolean bPrintBell = false;
	DoubleVector dvStirling;
	int i;
	int j;
	double dBell;

	// Calcul si necessaire du tableau des valeurs de Bell generalisees
	require(dvLnBell.GetSize() == 0);

	// Taillage du vecteur des valeurs
	dvLnBell.SetSize(nMaxSize);

	// Calcul des nombres de Stirling de seconde espece
	// (nombre de partition de n element en exactement k classes)
	dvStirling.SetSize(nMaxSize);

	// Initialisation des valeurs particulieres
	// (attention: l'index i correspond a 'n-1')
	for (i = 0; i < nLnBellTableMaxN; i++)
	{
		dvStirling.SetAt(i * nLnBellTableMaxN + 0, 1);
		if (i > 0)
			dvStirling.SetAt(i * nLnBellTableMaxN + 1, pow(2.0, 1.0 * i) - 1);
		if (i > 1)
			dvStirling.SetAt(i * nLnBellTableMaxN + i, 1);
		if (i > 2)
			dvStirling.SetAt(i * nLnBellTableMaxN + i - 1, i * (i + 1) / 2);
	}

	// Calcul par la formule de recurrence des valeurs suivantes
	for (i = 1; i < nLnBellTableMaxN; i++)
	{
		for (j = 2; j < i - 1; j++)
		{
			dvStirling.SetAt(i * nLnBellTableMaxN + j,
					 dvStirling.GetAt((i - 1) * nLnBellTableMaxN + j - 1) +
					     (j + 1) * dvStirling.GetAt((i - 1) * nLnBellTableMaxN + j));
		}
	}

	// Affichage des nombres de Stirling
	if (bPrintStirling)
	{
		for (i = 1; i <= nLnBellTableMaxN; i++)
		{
			for (j = 1; j <= i; j++)
			{
				cout << dvStirling.GetAt((i - 1) * nLnBellTableMaxN + j - 1) << "\t";
			}
			cout << endl;
		}
	}

	// Calcul des nombres de Bell generalises, par sommation des nombres de Stirling
	// pour les nombres de partition inferieurs (puis passage au logarithme)
	for (i = 1; i <= nLnBellTableMaxN; i++)
	{
		dBell = 0;
		for (j = 1; j <= i; j++)
		{
			dBell += dvStirling.GetAt((i - 1) * nLnBellTableMaxN + j - 1);
			dvLnBell.SetAt((i - 1) * nLnBellTableMaxN + j - 1, log(dBell));
		}
	}

	// Affichage des nombres de Bell
	if (bPrintBell)
	{
		for (i = 1; i <= nLnBellTableMaxN; i++)
		{
			for (j = 1; j <= i; j++)
			{
				cout << exp(dvLnBell.GetAt((i - 1) * nLnBellTableMaxN + j - 1)) << "\t";
			}
			cout << endl;
		}
	}

	ensure(dvLnBell.GetSize() > 0);
}

double KWStat::ComputeLnBellValue(int n, int k)
{
	double dEpsilon = 1e-6 / k;
	const int nSerieSize = 20;
	static DoubleVector dvInvExpSerie;
	double dInvExp;
	double dLnBell;
	double dBellSerie;
	double dFactor;
	double dInvExpFactor;
	double dTerm;
	int i0;
	double dLnTermI0;
	int i;

	require(n >= 1);
	require(1 <= k and k <= n);

	// Gestion des tres petites valeurs de n
	if (n == 1)
		return 0;
	else if (n == 2)
		return log(1.0 * k);

	// Calcul si necessaire des termes du developpement en serie de exp(-1)
	dInvExp = exp(-1.0);
	if (dvInvExpSerie.GetSize() == 0)
	{
		// Initialisation du vecteur
		dvInvExpSerie.SetSize(nSerieSize);

		// Calcul des termes de la series
		dFactor = 1;
		dTerm = 1;
		dvInvExpSerie.SetAt(0, 1);
		for (i = 1; i < nSerieSize; i++)
		{
			dFactor *= -i;
			dTerm += 1 / dFactor;
			dvInvExpSerie.SetAt(i, dTerm);
		}
		assert(fabs(dvInvExpSerie.GetAt(nSerieSize - 1) - dInvExp) < 1e-12);
	}

	// Recherche de la valeur max du facteur i^n/i! dans la serie permettant de
	// calculer le nombre de Bell
	i0 = ComputeMainBellTermIndex(n, k);
	dLnTermI0 = n * log(1.0 * i0) - LnFactorial(i0);

	// Calcul des termes de la serie du nombre de Bell, autour de l'index i0
	dBellSerie = 0;
	for (i = i0; i <= k; i++)
	{
		// Terme du developpement en serie de exp(-1)
		if (k - i >= nSerieSize)
			dInvExpFactor = dInvExp;
		else
			dInvExpFactor = dvInvExpSerie.GetAt(k - i);

		// Terme de la serie
		// On resout les problemes de precision numerique en passant par le log
		dTerm = exp(n * log(1.0 * i) - LnFactorial(i) - dLnTermI0);
		assert(dTerm < 1e3);

		// Ajout du terme, et arret si le terme est negligeable
		dBellSerie += dTerm * dInvExpFactor;
		if (dTerm < dEpsilon)
			break;
	}
	for (i = i0 - 1; i >= 1; i--)
	{
		// Terme du developpement en serie de exp(-1)
		if (k - i >= nSerieSize)
			dInvExpFactor = dInvExp;
		else
			dInvExpFactor = dvInvExpSerie.GetAt(k - i);

		// Terme de la serie
		// On resout les problemes de precision numerique en passant par le log
		dTerm = exp(n * log(i * 1.0 / i0) + LnFactorial(i0) - LnFactorial(i));
		assert(dTerm < 1e3);

		// Ajout du terme, et arret si le terme est negligeable
		dBellSerie += dTerm * dInvExpFactor;
		if (dTerm < dEpsilon)
			break;
	}

	// Calcul du logarithme du nombre de Bell generalise
	dLnBell = log(dBellSerie) + dLnTermI0;
	return dLnBell;
}

// Recherche de l'index i maximisant i^n/i! sur l'intervalle [1, k]
// Cela revient a d'abord inverser l'equation i.ln(i) = n
int KWStat::ComputeMainBellTermIndex(int n, int k)
{
	int iMin;
	int iMax;
	int iNew;
	double dNewValue;

	require(n > 2);
	require(1 <= k and k <= n);

	// Initialisation des bornes
	iMin = (int)floor(n / log(1.0 * n));
	iMax = (int)ceil(n / (log(1.0 * n) - log(log(1.0 * n))));
	assert(iMin <= iMax);
	assert(iMin * log(1.0 * iMin) <= n);
	assert(iMax * log(1.0 * iMax) >= n);

	// Recherche dichotomique
	while (iMax - iMin > 1)
	{
		// Evaluation du milieu de l'intervalle de recherche
		iNew = (iMin + iMax) / 2;
		dNewValue = iNew * log(1.0 * iNew);

		// Changement des bornes de l'intervalle
		if (dNewValue > n)
			iMax = iNew;
		else
			iMin = iNew;
	}

	// Si le max est avant k, on retorune le max
	if (iMin < k)
		return iMin;
	// Sinon, on retourne k
	else
		return k;
}

///////////////////////////////////////////////////////////////////////////

double KWStat::LnProb(double dChi2, int ndf)
{
	require(dChi2 >= 0);
	require(ndf >= 1);

	// Si ndf=1: fonction d'erreur
	if (ndf == 1)
		return LnErfc(sqrt(dChi2 / 2));
	// Si ndf=2: exponentielle
	else if (ndf == 2)
		return -dChi2 / 2;
	else
		return LnGamma1(ndf / 2.0, dChi2 / 2.0);
}

double KWStat::ProbLevel(double dChi2, int ndf)
{
	require(dChi2 >= 0);
	require(ndf >= 1);

	return -LnProb(dChi2, ndf) / log(10.0);
}

double KWStat::Chi2(double dChi2, int ndf)
{
	require(dChi2 >= 0);
	require(ndf >= 1);

	return exp(LnProb(dChi2, ndf));
}

double KWStat::InvChi2(double dProb, int ndf)
{
	const double dTolerance = 1e-7;
	const double dMax = 1e20;
	double dLowerX;
	double dUpperX;
	double dLowerXVal;
	double dUpperXVal;
	double dNewX;
	double dNewXVal;
	double dLnProb;

	require(0 <= dProb and dProb <= 1);
	require(ndf >= 1);

	// Cas particulier: probabilite nulle
	if (dProb == 0)
		return 0;

	// Initialisation des bornes de l'intervalle de recherche
	dLowerX = 0;
	dLowerXVal = 0;
	dUpperX = dMax;
	dUpperXVal = 1;

	// Recherche par dichotomie de la valeur la plus proche
	dLnProb = log(dProb);
	while ((dUpperX - dLowerX) > dTolerance * (fabs(dLowerX) + fabs(dUpperX)))
	{
		// Calcul d'une nouvelle valeur
		dNewX = (dLowerX + dUpperX) / 2;
		dNewXVal = LnProb(dNewX, ndf);

		// Changement des bornes en fonction du resultat
		if (dNewXVal > dLnProb)
		{
			dLowerX = dNewX;
			dLowerXVal = dNewXVal;
		}
		else
		{
			dUpperX = dNewX;
			dUpperXVal = dNewXVal;
		}
	}
	return dLowerX;
}

////////////////////////////////////////////////////////////////////////////

void KWStat::Test()
{
	boolean bSkipIt = false;
	IntVector ivNdfs;
	DoubleVector dvChi2Ratios;
	int i;
	double dResult;
	int j;
	IntVector ivC0Max;

	// Table des Student
	if (not bSkipIt)
	{
		cout << "Table de Student\n";
		cout << "ndf\t0.10\t0.05\t0.01\n";
		for (i = 1; i < 11; i++)
		{
			cout << i << "\t" << InvStudent(0.10, i) << "\t" << InvStudent(0.05, i) << "\t"
			     << InvStudent(0.01, i) << "\n";
		}
		cout << endl;
	}

	// Log(Factorielle)
	if (not bSkipIt)
	{
		cout << "Log(Factorielle)\n";
		dResult = 0;
		for (j = 0; j < 100; j++)
		{
			for (i = 0; i < 10000; i++)
			{
				dResult += LnFactorial(i);
			}
		}
		cout << dResult << endl;
	}

	// LnBell
	if (not bSkipIt)
	{
		cout << "Ln(Bell)\n";
		cout << "I";
		for (j = 1; j <= 20; j++)
			cout << "\tK=" << j;
		cout << "\n";
		for (i = 1; i <= 20; i++)
		{
			cout << i;
			for (j = 1; j <= 20; j++)
			{
				cout << "\t";
				if (i <= j)
					cout << LnBell(j, i);
			}
			cout << "\n";
		}
	}

	// Codage universel de Rissanen
	if (not bSkipIt)
	{
		cout << "Rissanen universal code for integer\n";

		// Bornes de normalisation
		cout << "N\tC0Max(N)\tln(C0Max(N))\n";
		for (i = 1; i <= 9; i++)
			cout << i << "\t" << C0Max(i) << "\t" << log(C0Max(i)) << "\n";
		i = 10;
		for (j = 1; j < 9; j++)
		{
			cout << i << "\t" << C0Max(i) << "\t" << log(C0Max(i)) << "\n";
			i *= 10;
		}

		// Cout de codage pour differentes bornes
		ivC0Max.Add(1);
		ivC0Max.Add(2);
		ivC0Max.Add(3);
		ivC0Max.Add(4);
		ivC0Max.Add(5);
		ivC0Max.Add(6);
		ivC0Max.Add(7);
		ivC0Max.Add(8);
		ivC0Max.Add(9);
		ivC0Max.Add(10);
		ivC0Max.Add(100);
		ivC0Max.Add(1000);

		// Entete
		cout << "N\tLn2*(N)";
		for (j = 0; j < ivC0Max.GetSize(); j++)
			cout << "\tUCL(N, " << ivC0Max.GetAt(j) << ")";
		cout << "\tUCL(N)\n";

		// Valeurs
		for (i = 1; i <= 100; i++)
		{
			cout << i << "\t";
			cout << LnStar(i) * log(2.0) << "\t";
			for (j = 0; j < ivC0Max.GetSize(); j++)
			{
				if (i <= ivC0Max.GetAt(j))
					cout << BoundedNaturalNumbersUniversalCodeLength(i, ivC0Max.GetAt(j));
				cout << "\t";
			}
			cout << NaturalNumbersUniversalCodeLength(i) << "\n";
		}
	}
}

////////////////////////////////////////////////////////////////////////////

double KWStat::LnErfc(double x)
{
	// Computation of the complementary error function erfc(x).
	//
	// The algorithm is based on a Chebyshev fit as denoted in
	// Numerical Recipes 2nd ed. on p. 214 (W.H.Press et al.).
	//
	// The fractional error is always less than 1.2e-7.
	//
	//--- Nve 14-nov-1998 UU-SAP Utrecht

	// The parameters of the Chebyshev fit
	const double a1 = -1.26551223, a2 = 1.00002368, a3 = 0.37409196, a4 = 0.09678418, a5 = -0.18628806,
		     a6 = 0.27886807, a7 = -1.13520398, a8 = 1.48851587, a9 = -0.82215223, a10 = 0.17087277;
	double v = 1; // The return value
	double z = fabs(x);

	if (z <= 0)
		return 0; // erfc(0)=1

	double t = 1 / (1 + 0.5 * z);

	v = log(t) + ((-z * z) + a1 +
		      t * (a2 + t * (a3 + t * (a4 + t * (a5 + t * (a6 + t * (a7 + t * (a8 + t * (a9 + t * a10)))))))));

	if (x < 0)
		v = log(2 - exp(v));

	return v;
}

double KWStat::LnGamma(double z)
{
	// Computation of ln[gamma(z)] for all z>0.
	//
	// The algorithm is based on the article by C.Lanczos [1] as denoted in
	// Numerical Recipes 2nd ed. on p. 207 (W.H.Press et al.).
	//
	// [1] C.Lanczos, SIAM Journal of Numerical Analysis B1 (1964), 86.
	//
	// The accuracy of the result is better than 2e-10.
	//
	//--- Nve 14-nov-1998 UU-SAP Utrecht

	require(z > 0);

	// Coefficients for the series expansion
	double c[7] = {2.5066282746310005, 76.18009172947146,     -86.50532032941677, 24.01409824083091,
		       -1.231739572450155, 0.1208650973866179e-2, -0.5395239384953e-5};

	double x = z;
	double y = x;
	double tmp = x + 5.5;
	tmp = (x + 0.5) * log(tmp) - tmp;
	double ser = 1.000000000190015;
	for (int i = 1; i < 7; i++)
	{
		y += 1;
		ser += c[i] / y;
	}
	double v = tmp + log(c[0] * ser / x);
	return v;
}

double KWStat::LnGammaRamanujan(double z)
{
	// Approximation de Ramanujan
	// S.Raghavan, S.S.Rangachari(Eds.), S.Ramanujan: The Lost Notebook and Other Unpublished Papers, Springer, New
	// York(1988), p. 339 Selon wikipedia, l'approximation de ln n! a unne erreur asymptotique de 1/(1400 n^3)
	// Empiriquement, l'approximation est meilleure que la methode alternative des que n > 60
	double dResult;
	const double dHalfLogPi = log(3.14159265358979323846) / 2.0;
	double dLogTerm;

	// Calcul du terme sous le log
	z -= 1.0;
	dLogTerm = ((8.0 * z + 4.0) * z + 1) * z + 1.0 / 30.0;

	// Aprpoximation avec tous ses termes
	dResult = z * log(z) - z + log(dLogTerm) / 6.0 + dHalfLogPi;
	return dResult;
}

double KWStat::BetaI(double a, double b, double x)
{
	double bt;

	require(0 <= x and x <= 1);

	// Cas limite
	if (x == 0 or x == 1)
		bt = 0;
	// Cas standard
	else
		bt = exp(LnGamma(a + b) - LnGamma(a) - LnGamma(b) + a * log(x) + b * log(1.0 - x));

	// Utilisation de la fraction continue directement
	if (x < (a + 1.0) / (a + b + 2.0))
		return bt * BetaCf(a, b, x) / a;
	// Utilisation de la fraction continue apres transformation symetrique
	else
		return 1.0 - bt * BetaCf(b, a, 1.0 - x) / b;
}

// Evaluation de la fraction continue pour la fonction gamma incomplete par
// la methode modifiee de Lentz
double KWStat::BetaCf(double a, double b, double x)
{
	const int nMaxIt = 100;
	const double dEps = 3.0e-7;
	const double dFPMin = 1.0e-30;
	int m, m2;
	double aa, c, d, del, h, qab, qam, qap;

	// These q's will be used in factors that occur in the coefs
	qab = a + b;
	qap = a + 1.0;
	qam = a - 1.0;

	// First step of Lentz's method
	c = 1.0;
	d = 1.0 - qab * x / qap;
	if (fabs(d) < dFPMin)
		d = dFPMin;
	d = 1.0 / d;
	h = d;
	for (m = 1; m <= nMaxIt; m++)
	{
		m2 = 2 * m;
		aa = m * (b - m) * x / ((qam + m2) * (a + m2));

		// One step (the even one) of the recurrence.
		d = 1.0 + aa * d;
		if (fabs(d) < dFPMin)
			d = dFPMin;
		c = 1.0 + aa / c;
		if (fabs(c) < dFPMin)
			c = dFPMin;
		d = 1.0 / d;
		h *= d * c;
		aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2));

		// Next step of the recurrence (the odd one)
		d = 1.0 + aa * d;
		if (fabs(d) < dFPMin)
			d = dFPMin;
		c = 1.0 + aa / c;
		if (fabs(c) < dFPMin)
			c = dFPMin;
		d = 1.0 / d;
		del = d * c;
		h *= del;

		// Are we done?
		if (fabs(del - 1.0) < dEps)
			break;
	}
	// if (m > nMaxIt) ErrMessage("a or b too big, or nMaxIt too small in betacf");
	return h;
}

double KWStat::LnGamma1(double a, double x)
{
	// Computation of the incomplete gamma function P(a,x)
	//
	// The algorithm is based on the formulas and code as denoted in
	// Numerical Recipes 2nd ed. on p. 210-212 (W.H.Press et al.).
	//
	//--- Nve 14-nov-1998 UU-SAP Utrecht

	require(a > 0 and x >= 0);

	if (x <= 0)
		return 0;

	if (x < (a + 1))
		return LnGammaSer1(a, x);
	else
		return LnGammaCf1(a, x);
}

double KWStat::LnGammaCf1(double a, double x)
{
	// Computation of the incomplete gamma function P(a,x)
	// via its continued fraction representation.
	//
	// The algorithm is based on the formulas and code as denoted in
	// Numerical Recipes 2nd ed. on p. 210-212 (W.H.Press et al.).
	//
	//--- Nve 14-nov-1998 UU-SAP Utrecht

	require(a > 0 and x >= 0);

	double gln = LnGamma(a);
	double h = GammaCf1Term(a, x);
	double v = (-x + a * log(x) - gln) + log(h);
	return v;
}

double KWStat::GammaCf1Term(double a, double x)
{
	int itmax;             // Maximum number of iterations
	double eps = 3.e-7;    // Relative accuracy
	double fpmin = 1.e-30; // Smallest double value allowed here

	require(a > 0 and x >= 0);

	itmax = (int)(log(a) * sqrt(a));
	if (itmax < 100)
		itmax = 100;
	double b = x + 1 - a;
	double c = 1 / fpmin;
	double d = 1 / b;
	double h = d;
	double an, del;
	for (int i = 1; i <= itmax; i++)
	{
		an = double(-i) * (double(i) - a);
		b += 2;
		d = an * d + b;
		if (fabs(d) < fpmin)
			d = fpmin;
		c = b + an / c;
		if (fabs(c) < fpmin)
			c = fpmin;
		d = 1 / d;
		del = d * c;
		h = h * del;
		if (fabs(del - 1) < eps)
			break;
		// if (i==itmax) cout << "*GamCf(a,x)* a too large or itmax too small" << endl;
	}
	return h;
}

double KWStat::LnGammaSer1(double a, double x)
{
	// Computation of the incomplete gamma function P(a,x)
	// via its series representation.
	//
	// The algorithm is based on the formulas and code as denoted in
	// Numerical Recipes 2nd ed. on p. 210-212 (W.H.Press et al.).
	//
	//--- Nve 14-nov-1998 UU-SAP Utrecht

	require(a > 0 and x >= 0);

	double gln = LnGamma(a);
	double sum = GammaSer1Term(a, x);
	double v = sum * exp(-x + a * log(x) - gln);
	return log(1 - v);
}

double KWStat::GammaSer1Term(double a, double x)
{
	int itmax;          // Maximum number of iterations
	double eps = 3.e-7; // Relative accuracy

	require(a > 0 and x >= 0);

	itmax = (int)(log(a) * sqrt(a));
	if (itmax < 100)
		itmax = 100;
	double ap = a;
	double sum = 1 / a;
	double del = sum;
	for (int n = 1; n <= itmax; n++)
	{
		ap += 1;
		del = del * x / ap;
		sum += del;
		if (fabs(del) < fabs(sum * eps))
			break;
		// if (n==itmax) cout << "*GamSer(a,x)* a too large or itmax too small" << endl;
	}
	return sum;
}
