// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWSNBStudy.h"

KWSNBStudy::KWSNBStudy()
{
	nProbBinNumber = 12;
	dP000 = 0;
	dP001 = 0;
	dP010 = 0;
	dP011 = 0;
	dP100 = 0;
	dP101 = 0;
	dP110 = 0;
	dP111 = 0;

	// Initialisation des tailles des vecteu'rs de probabilites
	dvTrueConditionalProbs.SetSize(8);
	dvNullConditionalProbs.SetSize(8);
	dvX1ConditionalProbs.SetSize(8);
	dvX2ConditionalProbs.SetSize(8);
	dvNBConditionalProbs.SetSize(8);
	dvMAPConditionalProbs.SetSize(8);
	dvBMAConditionalProbs.SetSize(8);
	dvCMAConditionalProbs.SetSize(8);
}

KWSNBStudy::~KWSNBStudy() {}

void KWSNBStudy::Test()
{
	KWSNBStudy test;

	test.Study();
}

void KWSNBStudy::Study()
{
	boolean bDisplayCase = false;
	int nP000;
	int nP001;
	int nP010;
	int nP011;
	int nP100;
	int nP101;
	int nP110;
	int nP111;
	double dEpsilon;
	int nCaseNumber;

	//
	dP000 = 0.25 / 2;
	dP001 = 0.75 / 2;
	dP010 = 0;
	dP011 = 0;
	dP100 = 0;
	dP101 = 0;
	dP110 = 0.75 / 2;
	dP111 = 0.25 / 2;
	//
	dP000 = 0.0;
	dP001 = 0.0;
	dP010 = 1 / 12.0;
	dP011 = 3 / 12.0;
	dP100 = 3 / 12.0;
	dP101 = 1 / 12.0;
	dP110 = 2 / 12.0;
	dP111 = 2 / 12.0;
	//
	dP000 = 2 / 16.0;
	dP001 = 2 / 16.0;
	dP010 = 1 / 16.0;
	dP011 = 3 / 16.0;
	dP100 = 3 / 16.0;
	dP101 = 1 / 16.0;
	dP110 = 2 / 16.0;
	dP111 = 2 / 16.0;
	//
	StudyOneCase(0);

	// Nombre de cas theoriques etudies
	cout << "Nombre de cas theoriques etudies: "
	     << exp(KWStat::LnFactorial(nProbBinNumber + 7) - KWStat::LnFactorial(nProbBinNumber) -
		    KWStat::LnFactorial(7))
	     << endl;

	// Parcours de tous les cas
	nCaseNumber = 0;
	dEpsilon = 1.0 / (8 * nProbBinNumber);
	for (nP000 = 0; nP000 <= nProbBinNumber; nP000++)
		for (nP001 = 0; nP001 <= nProbBinNumber - (nP000); nP001++)
			for (nP010 = 0; nP010 <= nProbBinNumber - (nP000 + nP001); nP010++)
				for (nP011 = 0; nP011 <= nProbBinNumber - (nP000 + nP001 + nP010); nP011++)
					for (nP100 = 0; nP100 <= nProbBinNumber - (nP000 + nP001 + nP010 + nP011);
					     nP100++)
						for (nP101 = 0;
						     nP101 <= nProbBinNumber - (nP000 + nP001 + nP010 + nP011 + nP100);
						     nP101++)
							for (nP110 = 0;
							     nP110 <= nProbBinNumber - (nP000 + nP001 + nP010 + nP011 +
											nP100 + nP101);
							     nP110++)
							{
								nP111 =
								    nProbBinNumber - (nP000 + nP001 + nP010 + nP011 +
										      nP100 + nP101 + nP110);
								{
									if (bDisplayCase)
									{
										cout << nCaseNumber << "\t" << nP000
										     << "\t" << nP001 << "\t" << nP010
										     << "\t" << nP011 << "\t" << nP100
										     << "\t" << nP101 << "\t" << nP110
										     << "\t" << nP111 << endl;
									}

									// Calcul des probabilites
									dP000 = (nP000 + dEpsilon) /
										(nProbBinNumber + 8 * dEpsilon);
									dP001 = (nP001 + dEpsilon) /
										(nProbBinNumber + 8 * dEpsilon);
									dP010 = (nP010 + dEpsilon) /
										(nProbBinNumber + 8 * dEpsilon);
									dP011 = (nP011 + dEpsilon) /
										(nProbBinNumber + 8 * dEpsilon);
									dP100 = (nP100 + dEpsilon) /
										(nProbBinNumber + 8 * dEpsilon);
									dP101 = (nP101 + dEpsilon) /
										(nProbBinNumber + 8 * dEpsilon);
									dP110 = (nP110 + dEpsilon) /
										(nProbBinNumber + 8 * dEpsilon);
									dP111 = 1.0 - (dP000 + dP001 + dP010 + dP011 +
										       dP100 + dP101 + dP110);
									assert(dP111 > 0);
									assert(fabs(dP111 - (nP111 + dEpsilon) /
												(nProbBinNumber +
												 8 * dEpsilon)) < 1e-5);

									// Etude d'un cas
									StudyOneCase(nCaseNumber);

									nCaseNumber++;
								}
							}

	// Nombre de cas etudies
	cout << "Nombre de cas etudies: " << nCaseNumber << endl;
}

void KWSNBStudy::StudyOneCase(int nCaseIndex)
{
	int nDisplayedCase = 0;
	boolean bDisplayDKL = false;

	// Calcul des probabilites conditionnelles
	ComputeTrueConditionalProbs();
	ComputeNullConditionalProbs();
	ComputeX1ConditionalProbs();
	ComputeX2ConditionalProbs();
	ComputeNBConditionalProbs();
	ComputeMAPConditionalProbs();
	ComputeBMAConditionalProbs();
	ComputeCMAConditionalProbs();

	// Affichage d'un cas
	if (nDisplayedCase == nCaseIndex)
	{
		cout << "Case " << nCaseIndex << "\t" << dP000 << "\t" << dP001 << "\t" << dP010 << "\t" << dP011
		     << "\t" << dP100 << "\t" << dP101 << "\t" << dP110 << "\t" << dP111 << endl;
		DisplayProbs("True", &dvTrueConditionalProbs, cout);
		DisplayProbs("Null", &dvNullConditionalProbs, cout);
		DisplayProbs("X1", &dvX1ConditionalProbs, cout);
		DisplayProbs("X2", &dvX2ConditionalProbs, cout);
		DisplayProbs("NB", &dvNBConditionalProbs, cout);
		DisplayProbs("MAP", &dvMAPConditionalProbs, cout);
		DisplayProbs("BMA", &dvBMAConditionalProbs, cout);
		DisplayProbs("CMA", &dvCMAConditionalProbs, cout);
	}

	// Afficage des divergences
	if (bDisplayDKL)
	{
		// Entete
		if (nCaseIndex == 0)
		{
			cout << "Case\tP000\tP001\tP010\tP011\tP100\tP101\tP110\tP111";
			cout << "\tTrueNLPD\tNull\tX1\tX2\tNB\tMAP\tBMA\tCMA" << endl;
		}

		// Probabilites
		cout << "Case " << nCaseIndex << "\t" << dP000 << "\t" << dP001 << "\t" << dP010 << "\t" << dP011
		     << "\t" << dP100 << "\t" << dP101 << "\t" << dP110 << "\t" << dP111;

		// Divergences
		cout << "\t" << ComputeMeanNLPD(&dvTrueConditionalProbs);
		cout << "\t" << ComputeDKL(&dvNullConditionalProbs);
		cout << "\t" << ComputeDKL(&dvX1ConditionalProbs);
		cout << "\t" << ComputeDKL(&dvX2ConditionalProbs);
		cout << "\t" << ComputeDKL(&dvNBConditionalProbs);
		cout << "\t" << ComputeDKL(&dvMAPConditionalProbs);
		cout << "\t" << ComputeDKL(&dvBMAConditionalProbs);
		cout << "\t" << ComputeDKL(&dvCMAConditionalProbs);
		cout << endl;
	}
}

void KWSNBStudy::ComputeTrueConditionalProbs()
{
	require(fabs(dP000 + dP001 + dP010 + dP011 + dP100 + dP101 + dP110 + dP111 - 1.0) < 1e-5);

	dvTrueConditionalProbs.Initialize();
	if (dP000 + dP001 > 0)
		dvTrueConditionalProbs.SetAt(I000, dP000 / (dP000 + dP001));
	if (dP000 + dP001 > 0)
		dvTrueConditionalProbs.SetAt(I001, dP001 / (dP000 + dP001));
	if (dP010 + dP011 > 0)
		dvTrueConditionalProbs.SetAt(I010, dP010 / (dP010 + dP011));
	if (dP010 + dP011 > 0)
		dvTrueConditionalProbs.SetAt(I011, dP011 / (dP010 + dP011));
	if (dP100 + dP101 > 0)
		dvTrueConditionalProbs.SetAt(I100, dP100 / (dP100 + dP101));
	if (dP100 + dP101 > 0)
		dvTrueConditionalProbs.SetAt(I101, dP101 / (dP100 + dP101));
	if (dP110 + dP111 > 0)
		dvTrueConditionalProbs.SetAt(I110, dP110 / (dP110 + dP111));
	if (dP110 + dP111 > 0)
		dvTrueConditionalProbs.SetAt(I111, dP111 / (dP110 + dP111));
}

void KWSNBStudy::ComputeNullConditionalProbs()
{
	double dP__0;
	double dP__1;

	require(fabs(dP000 + dP001 + dP010 + dP011 + dP100 + dP101 + dP110 + dP111 - 1.0) < 1e-5);

	dP__0 = dP000 + dP010 + dP100 + dP110;
	dP__1 = dP001 + dP011 + dP101 + dP111;
	dvNullConditionalProbs.SetAt(I000, dP__0);
	dvNullConditionalProbs.SetAt(I001, dP__1);
	dvNullConditionalProbs.SetAt(I010, dP__0);
	dvNullConditionalProbs.SetAt(I011, dP__1);
	dvNullConditionalProbs.SetAt(I100, dP__0);
	dvNullConditionalProbs.SetAt(I101, dP__1);
	dvNullConditionalProbs.SetAt(I110, dP__0);
	dvNullConditionalProbs.SetAt(I111, dP__1);
}

void KWSNBStudy::ComputeX1ConditionalProbs()
{
	require(fabs(dP000 + dP001 + dP010 + dP011 + dP100 + dP101 + dP110 + dP111 - 1.0) < 1e-5);

	dvX1ConditionalProbs.Initialize();
	if (dP000 + dP001 + dP010 + dP011 > 0)
		dvX1ConditionalProbs.SetAt(I000, (dP000 + dP010) / (dP000 + dP001 + dP010 + dP011));
	if (dP000 + dP001 + dP010 + dP011 > 0)
		dvX1ConditionalProbs.SetAt(I001, (dP001 + dP011) / (dP000 + dP001 + dP010 + dP011));
	if (dP000 + dP001 + dP010 + dP011 > 0)
		dvX1ConditionalProbs.SetAt(I010, (dP000 + dP010) / (dP000 + dP001 + dP010 + dP011));
	if (dP000 + dP001 + dP010 + dP011 > 0)
		dvX1ConditionalProbs.SetAt(I011, (dP001 + dP011) / (dP000 + dP001 + dP010 + dP011));
	if (dP100 + dP101 + dP110 + dP111 > 0)
		dvX1ConditionalProbs.SetAt(I100, (dP100 + dP110) / (dP100 + dP101 + dP110 + dP111));
	if (dP100 + dP101 + dP110 + dP111 > 0)
		dvX1ConditionalProbs.SetAt(I101, (dP101 + dP111) / (dP100 + dP101 + dP110 + dP111));
	if (dP100 + dP101 + dP110 + dP111 > 0)
		dvX1ConditionalProbs.SetAt(I110, (dP100 + dP110) / (dP100 + dP101 + dP110 + dP111));
	if (dP100 + dP101 + dP110 + dP111 > 0)
		dvX1ConditionalProbs.SetAt(I111, (dP101 + dP111) / (dP100 + dP101 + dP110 + dP111));
}

void KWSNBStudy::ComputeX2ConditionalProbs()
{
	require(fabs(dP000 + dP001 + dP010 + dP011 + dP100 + dP101 + dP110 + dP111 - 1.0) < 1e-5);

	dvX2ConditionalProbs.Initialize();
	if (dP000 + dP001 + dP100 + dP101 > 0)
		dvX2ConditionalProbs.SetAt(I000, (dP000 + dP100) / (dP000 + dP001 + dP100 + dP101));
	if (dP000 + dP001 + dP100 + dP101 > 0)
		dvX2ConditionalProbs.SetAt(I001, (dP001 + dP101) / (dP000 + dP001 + dP100 + dP101));
	if (dP010 + dP011 + dP110 + dP111 > 0)
		dvX2ConditionalProbs.SetAt(I010, (dP010 + dP110) / (dP010 + dP011 + dP110 + dP111));
	if (dP010 + dP011 + dP110 + dP111 > 0)
		dvX2ConditionalProbs.SetAt(I011, (dP011 + dP111) / (dP010 + dP011 + dP110 + dP111));
	if (dP000 + dP001 + dP100 + dP101 > 0)
		dvX2ConditionalProbs.SetAt(I100, (dP000 + dP100) / (dP000 + dP001 + dP100 + dP101));
	if (dP000 + dP001 + dP100 + dP101 > 0)
		dvX2ConditionalProbs.SetAt(I101, (dP001 + dP101) / (dP000 + dP001 + dP100 + dP101));
	if (dP010 + dP011 + dP110 + dP111 > 0)
		dvX2ConditionalProbs.SetAt(I110, (dP010 + dP110) / (dP010 + dP011 + dP110 + dP111));
	if (dP010 + dP011 + dP110 + dP111 > 0)
		dvX2ConditionalProbs.SetAt(I111, (dP011 + dP111) / (dP010 + dP011 + dP110 + dP111));
}

void KWSNBStudy::ComputeNBConditionalProbs()
{
	double dP__0;
	double dP__1;
	double dTotalProb;

	require(fabs(dP000 + dP001 + dP010 + dP011 + dP100 + dP101 + dP110 + dP111 - 1.0) < 1e-5);

	// Calcul initiaux
	dvNBConditionalProbs.Initialize();
	dP__0 = dP000 + dP010 + dP100 + dP110;
	dP__1 = dP001 + dP011 + dP101 + dP111;
	if (dP000 + dP001 > 0)
		dvNBConditionalProbs.SetAt(I000, (dP000 + dP010) * (dP000 + dP100) / (dP__0 * (dP000 + dP001)));
	if (dP000 + dP001 > 0)
		dvNBConditionalProbs.SetAt(I001, (dP001 + dP011) * (dP001 + dP101) / (dP__1 * (dP000 + dP001)));
	if (dP010 + dP011 > 0)
		dvNBConditionalProbs.SetAt(I010, (dP000 + dP010) * (dP010 + dP110) / (dP__0 * (dP010 + dP011)));
	if (dP010 + dP011 > 0)
		dvNBConditionalProbs.SetAt(I011, (dP001 + dP011) * (dP011 + dP111) / (dP__1 * (dP010 + dP011)));
	if (dP100 + dP101 > 0)
		dvNBConditionalProbs.SetAt(I100, (dP100 + dP110) * (dP000 + dP100) / (dP__0 * (dP100 + dP101)));
	if (dP100 + dP101 > 0)
		dvNBConditionalProbs.SetAt(I101, (dP101 + dP111) * (dP001 + dP101) / (dP__1 * (dP100 + dP101)));
	if (dP110 + dP111 > 0)
		dvNBConditionalProbs.SetAt(I110, (dP100 + dP110) * (dP010 + dP110) / (dP__0 * (dP110 + dP111)));
	if (dP110 + dP111 > 0)
		dvNBConditionalProbs.SetAt(I111, (dP101 + dP111) * (dP011 + dP111) / (dP__1 * (dP110 + dP111)));

	// Normalisation
	dTotalProb = dvNBConditionalProbs.GetAt(I000) + dvNBConditionalProbs.GetAt(I001);
	if (dTotalProb > 0)
		dvNBConditionalProbs.SetAt(I000, dvNBConditionalProbs.GetAt(I000) / dTotalProb);
	if (dTotalProb > 0)
		dvNBConditionalProbs.SetAt(I001, dvNBConditionalProbs.GetAt(I001) / dTotalProb);
	dTotalProb = dvNBConditionalProbs.GetAt(I010) + dvNBConditionalProbs.GetAt(I011);
	if (dTotalProb > 0)
		dvNBConditionalProbs.SetAt(I010, dvNBConditionalProbs.GetAt(I010) / dTotalProb);
	if (dTotalProb > 0)
		dvNBConditionalProbs.SetAt(I011, dvNBConditionalProbs.GetAt(I011) / dTotalProb);
	dTotalProb = dvNBConditionalProbs.GetAt(I100) + dvNBConditionalProbs.GetAt(I101);
	if (dTotalProb > 0)
		dvNBConditionalProbs.SetAt(I100, dvNBConditionalProbs.GetAt(I100) / dTotalProb);
	if (dTotalProb > 0)
		dvNBConditionalProbs.SetAt(I101, dvNBConditionalProbs.GetAt(I101) / dTotalProb);
	dTotalProb = dvNBConditionalProbs.GetAt(I110) + dvNBConditionalProbs.GetAt(I111);
	if (dTotalProb > 0)
		dvNBConditionalProbs.SetAt(I110, dvNBConditionalProbs.GetAt(I110) / dTotalProb);
	if (dTotalProb > 0)
		dvNBConditionalProbs.SetAt(I111, dvNBConditionalProbs.GetAt(I111) / dTotalProb);
}

void KWSNBStudy::ComputeMAPConditionalProbs()
{
	double dNullMeanNLPD;
	double dX1MeanNLPD;
	double dX2MeanNLPD;
	double dNBMeanNLPD;
	double dMAPMeanNLPD;

	require(fabs(dP000 + dP001 + dP010 + dP011 + dP100 + dP101 + dP110 + dP111 - 1.0) < 1e-5);

	// Calcul des log vraisemblances negatives de chaque modele
	dNullMeanNLPD = ComputeMeanNLPD(&dvNullConditionalProbs);
	dX1MeanNLPD = ComputeMeanNLPD(&dvX1ConditionalProbs);
	dX2MeanNLPD = ComputeMeanNLPD(&dvX2ConditionalProbs);
	dNBMeanNLPD = ComputeMeanNLPD(&dvNBConditionalProbs);

	// Recherche du MAP
	dMAPMeanNLPD = DBL_MAX;
	if (dNullMeanNLPD < dMAPMeanNLPD)
	{
		dMAPMeanNLPD = dNullMeanNLPD;
		dvMAPConditionalProbs.CopyFrom(&dvNullConditionalProbs);
	}
	if (dX1MeanNLPD < dMAPMeanNLPD)
	{
		dMAPMeanNLPD = dX1MeanNLPD;
		dvMAPConditionalProbs.CopyFrom(&dvX1ConditionalProbs);
	}
	if (dX2MeanNLPD < dMAPMeanNLPD)
	{
		dMAPMeanNLPD = dX2MeanNLPD;
		dvMAPConditionalProbs.CopyFrom(&dvX2ConditionalProbs);
	}
	if (dNBMeanNLPD < dMAPMeanNLPD)
	{
		dMAPMeanNLPD = dNBMeanNLPD;
		dvMAPConditionalProbs.CopyFrom(&dvNBConditionalProbs);
	}
}

void KWSNBStudy::ComputeBMAConditionalProbs()
{
	dvBMAConditionalProbs.CopyFrom(&dvMAPConditionalProbs);
}

void KWSNBStudy::ComputeCMAConditionalProbs()
{
	double dP__0;
	double dP__1;
	double dTotalProb;
	double dNullMeanNLPD;
	double dX1MeanNLPD;
	double dX2MeanNLPD;
	double dNBMeanNLPD;
	double dNullCoef;
	double dX1Coef;
	double dX2Coef;
	double dNBCoef;
	double dX1Weight;
	double dX2Weight;

	require(fabs(dP000 + dP001 + dP010 + dP011 + dP100 + dP101 + dP110 + dP111 - 1.0) < 1e-5);

	// Calcul des log vraisemblances negatives de chaque modele
	dNullMeanNLPD = ComputeMeanNLPD(&dvNullConditionalProbs);
	dX1MeanNLPD = ComputeMeanNLPD(&dvX1ConditionalProbs);
	dX2MeanNLPD = ComputeMeanNLPD(&dvX2ConditionalProbs);
	dNBMeanNLPD = ComputeMeanNLPD(&dvNBConditionalProbs);

	// Calcul des taux de compression des modeles
	dNullCoef = 1 - dNullMeanNLPD / dNullMeanNLPD;
	if (dNullCoef < 1e-10)
		dNullCoef = 0;
	dX1Coef = 1 - dX1MeanNLPD / dNullMeanNLPD;
	if (dX1Coef < 1e-10)
		dX1Coef = 0;
	dX2Coef = 1 - dX2MeanNLPD / dNullMeanNLPD;
	if (dX2Coef < 1e-10)
		dX2Coef = 0;
	dNBCoef = 1 - dNBMeanNLPD / dNullMeanNLPD;
	if (dNBCoef < 1e-10)
		dNBCoef = 0;

	// Calcul des poids des variables
	dX1Weight = 0;
	dX2Weight = 0;
	if (dNullCoef + dX1Coef + dX2Coef + dNBCoef > 1e-10)
	{
		dX1Weight = (dX1Coef + dNBCoef) / (dNullCoef + dX1Coef + dX2Coef + dNBCoef);
		dX2Weight = (dX2Coef + dNBCoef) / (dNullCoef + dX1Coef + dX2Coef + dNBCoef);
	}

	// Calcul initiaux
	dvCMAConditionalProbs.Initialize();
	dP__0 = dP000 + dP010 + dP100 + dP110;
	dP__1 = dP001 + dP011 + dP101 + dP111;
	dP__0 = pow(dP__0, dX1Weight + dX2Weight - 1);
	dP__1 = pow(dP__1, dX1Weight + dX2Weight - 1);
	if (dP000 + dP001 > 0)
		dvCMAConditionalProbs.SetAt(I000, pow(dP000 + dP010, dX1Weight) * pow(dP000 + dP100, dX2Weight) /
						      (dP__0 * (dP000 + dP001)));
	if (dP000 + dP001 > 0)
		dvCMAConditionalProbs.SetAt(I001, pow(dP001 + dP011, dX1Weight) * pow(dP001 + dP101, dX2Weight) /
						      (dP__1 * (dP000 + dP001)));
	if (dP010 + dP011 > 0)
		dvCMAConditionalProbs.SetAt(I010, pow(dP000 + dP010, dX1Weight) * pow(dP010 + dP110, dX2Weight) /
						      (dP__0 * (dP010 + dP011)));
	if (dP010 + dP011 > 0)
		dvCMAConditionalProbs.SetAt(I011, pow(dP001 + dP011, dX1Weight) * pow(dP011 + dP111, dX2Weight) /
						      (dP__1 * (dP010 + dP011)));
	if (dP100 + dP101 > 0)
		dvCMAConditionalProbs.SetAt(I100, pow(dP100 + dP110, dX1Weight) * pow(dP000 + dP100, dX2Weight) /
						      (dP__0 * (dP100 + dP101)));
	if (dP100 + dP101 > 0)
		dvCMAConditionalProbs.SetAt(I101, pow(dP101 + dP111, dX1Weight) * pow(dP001 + dP101, dX2Weight) /
						      (dP__1 * (dP100 + dP101)));
	if (dP110 + dP111 > 0)
		dvCMAConditionalProbs.SetAt(I110, pow(dP100 + dP110, dX1Weight) * pow(dP010 + dP110, dX2Weight) /
						      (dP__0 * (dP110 + dP111)));
	if (dP110 + dP111 > 0)
		dvCMAConditionalProbs.SetAt(I111, pow(dP101 + dP111, dX1Weight) * pow(dP011 + dP111, dX2Weight) /
						      (dP__1 * (dP110 + dP111)));

	// Normalisation
	dTotalProb = dvCMAConditionalProbs.GetAt(I000) + dvCMAConditionalProbs.GetAt(I001);
	if (dTotalProb > 0)
		dvCMAConditionalProbs.SetAt(I000, dvCMAConditionalProbs.GetAt(I000) / dTotalProb);
	if (dTotalProb > 0)
		dvCMAConditionalProbs.SetAt(I001, dvCMAConditionalProbs.GetAt(I001) / dTotalProb);
	dTotalProb = dvCMAConditionalProbs.GetAt(I010) + dvCMAConditionalProbs.GetAt(I011);
	if (dTotalProb > 0)
		dvCMAConditionalProbs.SetAt(I010, dvCMAConditionalProbs.GetAt(I010) / dTotalProb);
	if (dTotalProb > 0)
		dvCMAConditionalProbs.SetAt(I011, dvCMAConditionalProbs.GetAt(I011) / dTotalProb);
	dTotalProb = dvCMAConditionalProbs.GetAt(I100) + dvCMAConditionalProbs.GetAt(I101);
	if (dTotalProb > 0)
		dvCMAConditionalProbs.SetAt(I100, dvCMAConditionalProbs.GetAt(I100) / dTotalProb);
	if (dTotalProb > 0)
		dvCMAConditionalProbs.SetAt(I101, dvCMAConditionalProbs.GetAt(I101) / dTotalProb);
	dTotalProb = dvCMAConditionalProbs.GetAt(I110) + dvCMAConditionalProbs.GetAt(I111);
	if (dTotalProb > 0)
		dvCMAConditionalProbs.SetAt(I110, dvCMAConditionalProbs.GetAt(I110) / dTotalProb);
	if (dTotalProb > 0)
		dvCMAConditionalProbs.SetAt(I111, dvCMAConditionalProbs.GetAt(I111) / dTotalProb);
}

double KWSNBStudy::ComputeDKL(DoubleVector* dvEstimatedProbs)
{
	double dDKL;

	require(dvEstimatedProbs != NULL);

	dDKL = ComputeMeanNLPD(dvEstimatedProbs) - ComputeMeanNLPD(&dvTrueConditionalProbs);

	return dDKL;
}

double KWSNBStudy::ComputeMeanNLPD(DoubleVector* dvEstimatedProbs)
{
	double dMeanNLPD;

	require(dvEstimatedProbs != NULL);

	dMeanNLPD = 0;
	if (dP000 > 0)
		dMeanNLPD -= dP000 * log(dvEstimatedProbs->GetAt(I000));
	if (dP001 > 0)
		dMeanNLPD -= dP001 * log(dvEstimatedProbs->GetAt(I001));
	if (dP010 > 0)
		dMeanNLPD -= dP010 * log(dvEstimatedProbs->GetAt(I010));
	if (dP011 > 0)
		dMeanNLPD -= dP011 * log(dvEstimatedProbs->GetAt(I011));
	if (dP100 > 0)
		dMeanNLPD -= dP100 * log(dvEstimatedProbs->GetAt(I100));
	if (dP101 > 0)
		dMeanNLPD -= dP101 * log(dvEstimatedProbs->GetAt(I101));
	if (dP110 > 0)
		dMeanNLPD -= dP110 * log(dvEstimatedProbs->GetAt(I110));
	if (dP111 > 0)
		dMeanNLPD -= dP111 * log(dvEstimatedProbs->GetAt(I111));

	return dMeanNLPD;
}

void KWSNBStudy::DisplayProbs(const ALString& sLabel, DoubleVector* dvProbs, ostream& ost)
{
	int n;

	require(dvProbs != NULL);

	ost << sLabel;
	for (n = 0; n < dvProbs->GetSize(); n++)
		ost << "\t" << dvProbs->GetAt(n);
	ost << "\t" << ComputeDKL(dvProbs);
	ost << endl;
}
