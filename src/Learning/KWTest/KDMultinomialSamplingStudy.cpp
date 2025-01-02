// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDMultinomialSamplingStudy.h"

KDMultinomialSamplingStudy::KDMultinomialSamplingStudy() {}

KDMultinomialSamplingStudy::~KDMultinomialSamplingStudy() {}

DoubleVector* KDMultinomialSamplingStudy::GetProbVector()
{
	return &dvProb;
}

int KDMultinomialSamplingStudy::GetProbVectorSize() const
{
	return dvProb.GetSize();
}

boolean KDMultinomialSamplingStudy::CheckProbVector() const
{
	boolean bOk = true;
	int i;
	double dProb;
	double dTotalProb;

	if (dvProb.GetSize() == 0)
		bOk = false;
	else
	{
		dTotalProb = 0;
		for (i = 0; i < dvProb.GetSize(); i++)
		{
			dProb = dvProb.GetAt(i);
			if (dProb < 0)
				bOk = false;
			if (dProb > 1)
				bOk = false;
			dTotalProb += dProb;
		}
		if (fabs(dTotalProb - 1) > 1e-5)
			bOk = false;
	}
	return bOk;
}

void KDMultinomialSamplingStudy::DistributionCompleteStudy(double dMaxTotalFrequency)
{
	boolean bDisplayAllDistributions = true;
	double dTotalFrequency;
	ObjectArray oaAllDistributions;
	int nBestDistribution;
	DoubleVector* dvFrequencies;
	DoubleVector dvEstimatedBestFrequencies;
	int i;
	int nRank;

	require(CheckProbVector());
	require(dMaxTotalFrequency >= 0);

	// Affichage des probas
	cout << "Probs";
	for (i = 0; i < dvProb.GetSize(); i++)
		cout << "\t" << dvProb.GetAt(i);
	cout << endl;

	// Ligne d'entete
	if (bDisplayAllDistributions)
	{
		cout << "Frequency\tDist nb";
		cout << "\tRank";
		cout << "\tEstimated";
		for (i = 0; i < dvProb.GetSize(); i++)
			cout << "\t"
			     << "CF" << i + 1;
		cout << "\tBest";
		for (i = 0; i < dvProb.GetSize(); i++)
			cout << "\t"
			     << "F" << i + 1;
		cout << "\n";
	}

	// Parcours de tous les effectifs
	for (dTotalFrequency = 0; dTotalFrequency <= dMaxTotalFrequency; dTotalFrequency++)
	{
		// On ne prend en compte qu'une sous-partie des cas possibles
		if (dTotalFrequency > 10 and fabs(dTotalFrequency - 10 * int(dTotalFrequency / 10)) > 0.1)
			continue;

		// Calcul de toutes les distributions
		ComputeAllDistributions(dTotalFrequency, &oaAllDistributions);

		// Estimation des meilleurs effectifs
		ComputeBestDistribution(dTotalFrequency, &dvEstimatedBestFrequencies);
		nRank = SearchDistributionIndex(&dvEstimatedBestFrequencies, &oaAllDistributions);

		// Affichage des resultats
		if (nRank > 0 or bDisplayAllDistributions)
		{
			// Effectif
			cout << dTotalFrequency << "\t";

			// Nombre de distributions
			cout << oaAllDistributions.GetSize() << "\t";

			// Effectifs estimes
			cout << nRank << "\t";
			cout << ComputeFrequencyVectorProb(&dvEstimatedBestFrequencies) << "\t";
			for (i = 0; i < dvProb.GetSize(); i++)
				cout << dvEstimatedBestFrequencies.GetAt(i) << "\t";

			// Affichage de la meilleure distribution
			nBestDistribution = SearchBestDistributionIndex(&oaAllDistributions);
			dvFrequencies = cast(DoubleVector*, oaAllDistributions.GetAt(nBestDistribution));
			WriteFrequencyVector(dvFrequencies, cout);
			cout << flush;
		}

		// Nettoyage
		oaAllDistributions.DeleteAll();
	}
}

void KDMultinomialSamplingStudy::DistributionStudy(double dTotalFrequency)
{
	boolean bDisplayAllDistribution = true;
	ObjectArray oaAllDistributions;
	int nBestDistribution;
	int nDistribution;
	DoubleVector* dvFrequencies;
	int i;

	require(CheckProbVector());
	require(dTotalFrequency >= 0);

	// Calcul de toutes les distributions
	ComputeAllDistributions(dTotalFrequency, &oaAllDistributions);

	// Affichage des probas
	cout << "\nFrequency\t" << dTotalFrequency << "\n";
	cout << "Probs";
	for (i = 0; i < dvProb.GetSize(); i++)
		cout << "\t" << dvProb.GetAt(i);
	cout << endl;

	// Affichage de la meilleure distribution
	nBestDistribution = SearchBestDistributionIndex(&oaAllDistributions);
	cout << "Best distribution\n";
	dvFrequencies = cast(DoubleVector*, oaAllDistributions.GetAt(nBestDistribution));
	WriteFrequencyVector(dvFrequencies, cout);

	// Affichage des distributions
	cout << "All distributions\t" << oaAllDistributions.GetSize() << "\n";
	if (bDisplayAllDistribution)
	{
		for (nDistribution = 0; nDistribution < oaAllDistributions.GetSize(); nDistribution++)
		{
			dvFrequencies = cast(DoubleVector*, oaAllDistributions.GetAt(nDistribution));
			WriteFrequencyVector(dvFrequencies, cout);
		}
	}

	// Nettoyage
	oaAllDistributions.DeleteAll();
}

void KDMultinomialSamplingStudy::ComputeAllDistributions(double dTotalFrequency, ObjectArray* oaAllDistributions)
{
	DoubleVector dvTemplateFrequencies;

	require(dTotalFrequency >= 0);
	require(oaAllDistributions != NULL);
	require(CheckProbVector());

	// Initialisation
	dvTemplateFrequencies.SetSize(GetProbVectorSize());
	oaAllDistributions->DeleteAll();

	// Constrution de tous les vecteurs d'effectif possibles
	ComputeAllDistributionsAt(&dvTemplateFrequencies, dTotalFrequency, 0, oaAllDistributions);
}

void KDMultinomialSamplingStudy::ComputeAllDistributionsAt(DoubleVector* dvTemplateFrequencies,
							   double dRemainingFrequency, int nStartIndex,
							   ObjectArray* oaAllDistributions)
{
	DoubleVector* dvFrequencies;
	double dFrequency;

	require(dvTemplateFrequencies != NULL);
	require(dvTemplateFrequencies->GetSize() > 0);
	require(dRemainingFrequency >= 0);
	require(0 <= nStartIndex and nStartIndex < dvTemplateFrequencies->GetSize());
	require(oaAllDistributions != NULL);

	// Si plus d'effectif, on ajoute la distribution courante
	if (dRemainingFrequency <= 1e-6)
	{
		dvFrequencies = dvTemplateFrequencies->Clone();
		oaAllDistributions->Add(dvFrequencies);
	}
	// Si on est sur le denier index, on y a joute l'effectif restant et on rend la distribution courante
	else if (nStartIndex == dvTemplateFrequencies->GetSize() - 1)
	{
		dvFrequencies = dvTemplateFrequencies->Clone();
		dvFrequencies->SetAt(nStartIndex, dRemainingFrequency);
		oaAllDistributions->Add(dvFrequencies);
	}
	// Sinon, on envisage tous le effectifs possible a cet index et on "recurse"
	else
	{
		for (dFrequency = 0; dFrequency <= dRemainingFrequency + 1e-6; dFrequency++)
		{
			dvTemplateFrequencies->SetAt(nStartIndex, dFrequency);
			ComputeAllDistributionsAt(dvTemplateFrequencies, dRemainingFrequency - dFrequency,
						  nStartIndex + 1, oaAllDistributions);
		}
		dvTemplateFrequencies->SetAt(nStartIndex, 0);
	}
}

int KDMultinomialSamplingStudy::SearchBestDistributionIndex(ObjectArray* oaAllDistributions)
{
	DoubleVector* dvFrequencies;
	int nDistribution;
	double dInfo;
	double dBestInfo;
	int nBest;

	require(oaAllDistributions != NULL);

	// Recherche de la distribution la plus probable
	dBestInfo = DBL_MAX;
	nBest = 0;
	for (nDistribution = 0; nDistribution < oaAllDistributions->GetSize(); nDistribution++)
	{
		dvFrequencies = cast(DoubleVector*, oaAllDistributions->GetAt(nDistribution));

		// Test si amelioration
		dInfo = ComputeFrequencyVectorInfo(dvFrequencies);
		if (dInfo < dBestInfo)
		{
			dBestInfo = dInfo;
			nBest = nDistribution;
		}
	}
	return nBest;
}

void KDMultinomialSamplingStudy::ComputeBestDistribution(double dTotalFrequency, DoubleVector* dvFrequencies) const
{
	require(CheckProbVector());
	require(dTotalFrequency >= 0);
	require(dvFrequencies != NULL);

	sampleGenerator.ComputeBestSample(dTotalFrequency, &dvProb, dvFrequencies);
}

int KDMultinomialSamplingStudy::SearchDistributionIndex(DoubleVector* dvSearchedFrequencies,
							ObjectArray* oaAllDistributions)
{
	int nSearchedIndex;
	DoubleVector dvAllCosts;
	int i;
	DoubleVector* dvFrequencies;
	double dSearchedCost;

	require(dvSearchedFrequencies != NULL);
	require(oaAllDistributions != NULL);
	require(CheckProbVector());
	require(dvSearchedFrequencies->GetSize() == dvProb.GetSize());

	// Collecte des probas de chaque distribution
	dvAllCosts.SetSize(oaAllDistributions->GetSize());
	for (i = 0; i < oaAllDistributions->GetSize(); i++)
	{
		dvFrequencies = cast(DoubleVector*, oaAllDistributions->GetAt(i));
		dvAllCosts.SetAt(i, ComputeFrequencyVectorInfo(dvFrequencies));
	}

	// Tri des couts par ordre croissant
	dvAllCosts.Sort();

	// Recherche du rang du cout de la distribution
	nSearchedIndex = 0;
	dSearchedCost = ComputeFrequencyVectorInfo(dvSearchedFrequencies);
	for (i = 0; i < dvAllCosts.GetSize(); i++)
	{
		if (fabs(dSearchedCost - dvAllCosts.GetAt(i)) < 1e-6)
		{
			nSearchedIndex = i;
			break;
		}
	}
	return nSearchedIndex;
}

double KDMultinomialSamplingStudy::ComputeFrequencyVectorProb(DoubleVector* dvFrequencies) const
{
	return sampleGenerator.ComputeFrequencyVectorProb(&dvProb, dvFrequencies);
}

double KDMultinomialSamplingStudy::ComputeFrequencyVectorInfo(DoubleVector* dvFrequencies) const
{
	return sampleGenerator.ComputeFrequencyVectorInfo(&dvProb, dvFrequencies);
}

double KDMultinomialSamplingStudy::ComputeTotalFrequency(DoubleVector* dvFrequencies) const
{
	int i;
	double dTotal;

	require(dvFrequencies != NULL);

	dTotal = 0;
	for (i = 0; i < dvFrequencies->GetSize(); i++)
		dTotal += dvFrequencies->GetAt(i);
	return dTotal;
}

void KDMultinomialSamplingStudy::WriteFrequencyVector(DoubleVector* dvFrequencies, ostream& ost) const
{
	int i;

	require(CheckProbVector());
	require(dvFrequencies != NULL);
	require(dvFrequencies->GetSize() == dvProb.GetSize());

	ost << ComputeFrequencyVectorProb(dvFrequencies);
	for (i = 0; i < dvFrequencies->GetSize(); i++)
		ost << "\t" << dvFrequencies->GetAt(i);
	ost << "\n";
}

void KDMultinomialSamplingStudy::Test()
{
	KDMultinomialSamplingStudy study;
	const int nMaxFrequency = 50;
	const int nTrialNumber = 20;
	int nTrial;
	double dProb;
	double dTotalProb;
	int i;

	// On force la graine aleatoire pour ameliorer la reproductivite des tests
	SetRandomSeed(1);

	// Initialisation d'un vecteur
	study.GetProbVector()->SetSize(3);
	study.GetProbVector()->SetAt(0, 0.5);
	study.GetProbVector()->SetAt(1, 0.3);
	study.GetProbVector()->SetAt(2, 0.2);

	// Etude de distribution des effectifs
	study.DistributionCompleteStudy(nMaxFrequency);

	// Initialisation d'un vecteur
	SetRandomSeed(1);
	study.GetProbVector()->SetSize(4);
	study.GetProbVector()->SetAt(0, 0.4);
	study.GetProbVector()->SetAt(1, 0.3);
	study.GetProbVector()->SetAt(2, 0.2);
	study.GetProbVector()->SetAt(3, 0.1);

	// Etude de distribution des effectifs
	// study.DistributionStudy(5);
	study.DistributionCompleteStudy(nMaxFrequency);

	// Boucle de test sur des vecteurs de probas aleatoires
	study.GetProbVector()->SetSize(5);
	for (nTrial = 0; nTrial < nTrialNumber; nTrial++)
	{
		// Creation d'un vecteur de probas aleatoire
		SetRandomSeed(nTrial);
		dTotalProb = 0;
		for (i = 0; i < study.GetProbVector()->GetSize() - 1; i++)
		{
			dProb = (1 - dTotalProb) * RandomDouble();
			dTotalProb += dProb;
			study.GetProbVector()->SetAt(i, dProb);
		}
		study.GetProbVector()->SetAt(study.GetProbVector()->GetSize() - 1, 1 - dTotalProb);

		// Etude
		study.DistributionCompleteStudy(nMaxFrequency);
	}
}
