// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "KWStat.h"
#include "Vector.h"
#include "Timer.h"

//////////////////////////////////////////////////////////////////////////
// Bibliotheque de methodes de calcul des criteres NML
class MHNMLStat : public KWStat
{
public:
	// Calcul exact du terme COMP pour un modele de Bernoulli
	static double BernoulliCOMP(int n);

	// Calcul approche du terme COMP pour un modele de Bernoulli
	static double ApproximationBernoulliCOMP(int n);

	// Calcul exact du terme COMP pour un modele multinomial
	// Formule de (Kontkanen et al, 2007)
	// Mononen et al("Computing the Multinomial Stochastic Complexity in Sub-Lineart Time"))
	// Normalisation pour eviter des erreur de math overflow
	static double MultinomialCOMP(int L, int n);

	// Calcul exact du terme COMP pour un modele multinomial
	// Formule de recurrence de (Kontkanen et al, 2008)
	static double RecurrenceMultinomialCOMP(int L, int n);

	// Calcul approche du terme COMP pour un modele multinomial selon la methode de Rissanen
	static double ApproximationRissanenMultinomialCOMP(int L, int n);

	// Calcul approche du terme COMP pour un modele multinomial selon la methode de Szpankowski
	static double ApproximationSzpankowskiMultinomialCOMP(int L, int n);

	// Calcul exact du terme COMP pour un modele multinomial selon l'approche MODL
	static double MODLMultinomialCOMP(int L, int n);

	// Methode de test
	static void Test();

	///////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Test de comparaison pour une multinomiale (L,n)
	static void TestMultinomial(int L, int n);

	// Coefficient de recurrence pour le calcul exact du terme COMP pour un modele multinomial
	static double RecurrenceCoefMultinomialCOMP(int L, int n);

	// Log de falling factorial
	// x(x - 1)...(x - k + 1)
	static double LnFallingFactorial(int nX, int nK);

	// Log de raising factorial
	// x(x + 1)...(x + k - 1)
	static double LnRisingFactorial(int nX, int nK);
};

///// Methodes en inline

inline double MHNMLStat::LnFallingFactorial(int nX, int nK)
{
	return LnFactorial(nX) - LnFactorial(nX - nK);
}

inline double MHNMLStat::LnRisingFactorial(int nX, int nK)
{
	return LnFactorial(nX + nK - 1) - LnFactorial(nX - 1);
}
