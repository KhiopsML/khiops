// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "Vector.h"
#include "KWVersion.h"

//////////////////////////////////////////////////////////////////////////
// Bibliotheque de fonctions statistiques
class KWStat : public Object
{
public:
	////////////////////////////////////////////////////////////
	// Statistiques sur des vecteurs

	// Minimum
	static double Min(DoubleVector* dvValues);

	// Maximum
	static double Max(DoubleVector* dvValues);

	// Moyenne
	static double Mean(DoubleVector* dvValues);

	// Moyenne geometrique (les valeurs doivent etre positives)
	static double GeometricMean(DoubleVector* dvValues);

	// Ecart type
	static double StandardDeviation(DoubleVector* dvValues);

	// T-Value de Student pour la comparaison par paires de deux vecteurs
	static double TValue(DoubleVector* dvValues1, DoubleVector* dvValues2);

	/////////////////////////////////////////////////////////////
	// Fonction statistique standard

	// Loi normale
	static double Normal(double dX, double dMean, double dStandardDeviation);

	// Loi normale standard
	static double StandardNormal(double dX);

	// Loi normale inverse
	static double InvNormal(double dProb, double dMean, double dStandardDeviation);

	// Loi normale standard inverse
	static double InvStandardNormal(double dProb);

	// Loi binomiale (calcul exact)
	static double BinomialProb(int n, double dProb, int k);

	// Fonction d'erreur et son complementaire
	static double Erf(double dX);
	static double Erfc(double dX);

	// Loi de Student
	static double Student(double dTValue, int ndf);

	// Loi de Student inverse
	static double InvStudent(double dProb, int ndf);

	// Logarithme de factorielle
	static double LnFactorial(int nValue);

	// Logarithme du nombre de Bell "generalise"
	// Nombre de partition de n elements en k classes (eventuellements vides)
	static double LnBell(int n, int k);

	// Codage "universel" des entiers naturels (selon Rissanen) pour n >= 1, en log nat
	static double NaturalNumbersUniversalCodeLength(int n);

	// Codage "universel" des entiers naturels (selon Rissanen) entre 1 et nMax, en log nat
	static double BoundedNaturalNumbersUniversalCodeLength(int n, int nMax);

	/////////////////////////////////////////////////////////////////
	// Fonctions statistiques Khiops
	// Pas de limites pour les valeurs de Khi2 et de degres de libertes

	// Logarithme de la probabilite du Khi2
	static double LnProb(double dChi2, int ndf);

	// Niveau de la probabilite du Khi2 (-log10(prob))
	static double ProbLevel(double dChi2, int ndf);

	// Probabilite du Khi2
	// Peut valoir 0 a cause de la precision numerique limitee a 10-300.
	// Passer au log dans ce cas
	static double Chi2(double dChi2, int ndf);

	// Loi du Khi2 inverse
	// Renvoie pour une probabilite donnee la valeur du Khi2
	static double InvChi2(double dProb, int ndf);

	////////////////////////////
	// Test
	static void Test();

	///////////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////////
	// Methodes internes

	// Calcul du nombre de Bell generalise
	static void ComputeLnBellTable();
	static double ComputeLnBellValue(int n, int k);
	static int ComputeMainBellTermIndex(int n, int k);

	// Approximation du logarithme de la fonction d'erreur erfc(x)
	static double LnErfc(double x);

	// Approximation du logarithme de la loi Gamma, avec une precision de l'ordre de 1e-10
	static double LnGamma(double z);

	// Approximation du logarithme de la loi Gamma, selon une formule de Ramanujan
	// La precision est meilleure qu'avec l'approximation precedente des que n > 60, et
	// le calcul est plus de deux fois plus rapide
	static double LnGammaRamanujan(double z);

	// Approximation de la loi beta incomplete
	static double BetaI(double a, double b, double x);
	static double BetaCf(double a, double b, double x);

	// Approximation du logarithme de la loi Gamma incomplete
	static double LnGamma1(double a, double x);
	static double LnGammaCf1(double a, double x);
	static double GammaCf1Term(double a, double x);
	static double LnGammaSer1(double a, double x);
	static double GammaSer1Term(double a, double x);

	// Calcul de log*(n) en base 2 = Somme_{j>1} max(0,log^(j)(n))
	static double LnStar(int n);
	static double C0Max(int nMax);
	static void ComputeLnStarAndC0MaxTables();

	// Tableau des valeurs de la fonction logarithme de factorielle
	static DoubleVector dvLnFactorial;
	static const int nLnFactorialTableSize = 128000;

	// Tableau des valeurs de la fonction logarithme de Bell
	static DoubleVector dvLnBell;
	static const int nLnBellTableMaxN = 100;

	// Tableau des valeurs de la fonction log_2*(n)=somme_{j >=0} max(log_2^(j)(x),0)
	// ou log_2^(j) est la jeme composition de la fonction log_2
	static DoubleVector dvLnStar;
	static const int nLnStarTableMaxN = 2000;

	// Tableau des valeurs de la somme finie exacte somme_{n=1}^Max 2^{-log_2*(n)}
	static DoubleVector dvC0Max;
};
