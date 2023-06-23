// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHNMLStat.h"

double MHNMLStat::BernoulliCOMP(int n)
{
	double dComp;
	double dLnTerm;
	int k;

	dComp = 0;
	for (k = 0; k <= n; k++)
	{
		dLnTerm = LnFactorial(n) - LnFactorial(k) - LnFactorial(n - k);
		if (k > 0)
			dLnTerm += k * log(k);
		if (n - k > 0)
			dLnTerm += (n - k) * log(n - k);
		if (n > 0)
			dLnTerm -= n * log(n);
		dComp += exp(dLnTerm);
	}
	return log(dComp);
}

double MHNMLStat::ApproximationBernoulliCOMP(int n)
{
	const double dPi = 3.14159265358979323846;
	return 0.5 * log(n * dPi / 2);
}

double MHNMLStat::MultinomialCOMP(int L, int n)
{
	double dComp;
	int k;
	DoubleVector dLogTerms;
	double dLogTerm;
	double dMaxLogTerm;
	double dLogCOMP;

	require(L >= 1);
	require(n >= 0);

	// Cas particulier ou L=1
	if (L == 1)
		return 0;

	// Use formula of(Kontkanen et al, 2007)
	// (Mononen et al("Computing the Multinomial Stochastic Complexity in Sub-Linear Time"))
	// Use normalisation to avoid math overflow error
	dComp = 0;
	dMaxLogTerm = 0;
	// Collect all log terms
	for (k = 0; k <= n; k++)
	{
		dLogTerm = LnRisingFactorial(L - 1, k);
		dLogTerm = dLogTerm + LnFallingFactorial(n, k);
		dLogTerm = dLogTerm - k * log(n);
		dLogTerm = dLogTerm - LnFactorial(k);
		dLogTerms.Add(dLogTerm);
		if (dLogTerm > dMaxLogTerm)
			dMaxLogTerm = dLogTerm;
	}
	// Compute normalized sum
	dComp = 0;
	for (k = 0; k <= n; k++)
	{
		dLogTerm = dLogTerms.GetAt(k);
		dComp = dComp + exp(dLogTerm - dMaxLogTerm);
	}
	dLogCOMP = log(dComp) + dMaxLogTerm;
	return dLogCOMP;
}

double MHNMLStat::RecurrenceMultinomialCOMP(int L, int n)
{
	double dCompCoef;
	dCompCoef = RecurrenceCoefMultinomialCOMP(L, n);
	dCompCoef = log(dCompCoef);
	return dCompCoef;
}

double MHNMLStat::ApproximationRissanenMultinomialCOMP(int L, int n)
{
	const double dPi = 3.14159265358979323846;
	return (L - 1) * 0.5 * log(n / (2 * dPi)) + (L / 2.0) * log(dPi) - LnGamma(L / 2.0);
}

double MHNMLStat::ApproximationSzpankowskiMultinomialCOMP(int L, int n)
{
	const double dPi = 3.14159265358979323846;
	double dApprox;
	double dFactorGamma;
	double dL;
	double dN;

	dL = L;
	dN = n;
	dApprox = (dL - 1) * 0.5 * log(dN / 2) + 0.5 * log(dPi) - LnGamma(dL / 2);
	dFactorGamma = exp(LnGamma(dL / 2) - LnGamma(dL / 2 - 0.5));
	dApprox += (1 / sqrt(dN)) * ((sqrt(2) * dL / 3) * dFactorGamma);
	dApprox += (1 / dN) * ((3 + dL * (dL - 2) * (2 * dL + 1)) / 36 - (dL * dL / 9) * (dFactorGamma * dFactorGamma));
	return dApprox;
}

double MHNMLStat::MODLMultinomialCOMP(int L, int n)
{
	return LnFactorial(L + n - 1) - LnFactorial(n) - LnFactorial(L - 1);
}

void MHNMLStat::Test()
{
	IntVector ivAllL;
	int i;
	int L;
	int n;
	Timer timer;

	// List des taille de multinomiale
	ivAllL.Add(2);
	ivAllL.Add(10);
	ivAllL.Add(100);
	ivAllL.Add(1000);
	ivAllL.Add(10000);
	ivAllL.Add(100000);

	// Ligne d'entete
	timer.Start();
	cout << "L\tn\tEnum\tNML\tNML(dApprox S)\tNML(dApprox R)\tBIC\n";

	// Parcours des tailles de multimoniale
	for (i = 0; i < ivAllL.GetSize(); i++)
	{
		L = ivAllL.GetAt(i);
		for (n = 1; n <= 1000; n++)
			TestMultinomial(L, n);
		n = 1000;
		while (n <= 1e6)
		{
			n = (n * 11) / 10;
			TestMultinomial(L, n);
		}
	}
	timer.Stop();
	cout << "Time\t" << timer.GetElapsedTime() << endl;
}

void MHNMLStat::TestMultinomial(int L, int n)
{
	cout << L << "\t";
	cout << n << "\t";
	cout << MODLMultinomialCOMP(L, n) << "\t";
	cout << MultinomialCOMP(L, n) << "\t";
	cout << ApproximationSzpankowskiMultinomialCOMP(L, n) << "\t";
	cout << ApproximationRissanenMultinomialCOMP(L, n) << "\t";
	cout << (L - 1) * 0.5 * log(n) << "\n";
}

double MHNMLStat::RecurrenceCoefMultinomialCOMP(int L, int n)
{
	double dCompCoef;
	// Use reccurrence formula of paper(Kontkanen et al, 2008)
	dCompCoef = 0;
	if (L > 2.5)
		dCompCoef =
		    RecurrenceCoefMultinomialCOMP(L - 1, n) + (n * RecurrenceCoefMultinomialCOMP(L - 2, n)) / (L - 2);
	else if (L < 1.5)
		dCompCoef = 1;
	else
		dCompCoef = exp(BernoulliCOMP(n));
	return dCompCoef;
}
