// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDMultinomialSampleGenerator.h"

///////////////////////////////////////////////////////////////////////////////
// Classe KDMultinomialSampleGenerator

KDMultinomialSampleGenerator::KDMultinomialSampleGenerator()
{
	lRankedRandomSeed = 0;
}

KDMultinomialSampleGenerator::~KDMultinomialSampleGenerator() {}

void KDMultinomialSampleGenerator::ComputeBestSample(double dTotalFrequency, const DoubleVector* dvProbs,
						     DoubleVector* dvFrequencies) const
{
	require(CheckPartialProbVector(dvProbs));
	require(dTotalFrequency >= 0);
	require(dvFrequencies != NULL);

	// Reinitialisation de la graine aleatoire
	lRankedRandomSeed = 0;

	// Calcul de la meilleure distribution sur la base des partie entieres superieure
	ComputeBestCeilSample(dTotalFrequency, dvProbs, dvFrequencies);

	// On le post-optimize
	PostOptimizeSample(dTotalFrequency, dvProbs, dvFrequencies);
	ensure(CheckFrequencies(dTotalFrequency, dvProbs, dvFrequencies));
}

void KDMultinomialSampleGenerator::ComputeBestEquidistributedSample(double dTotalFrequency, int nValueNumber,
								    DoubleVector* dvFrequencies) const
{
	double dBaseNumber;
	int nRestNumber;
	int i;

	require(dTotalFrequency >= 0);
	require(nValueNumber >= 0);
	require(dvFrequencies != NULL);

	// Reinitialisation de la graine aleatoire
	lRankedRandomSeed = 0;

	// Taillage du vecteur deffectif
	dvFrequencies->SetSize(nValueNumber);

	// Calcul des effectifs si au moins une valeur
	if (nValueNumber > 0)
	{
		// Cas des tres grand effectifs: on prend l'arrondi
		if (IsVeryLargeFrequency(dTotalFrequency))
		{
			dBaseNumber = floor(0.5 + dTotalFrequency / nValueNumber);
			for (i = 0; i < nValueNumber; i++)
				dvFrequencies->SetAt(i, dBaseNumber);
		}
		// Sinon, on prend la partie entiere inferieure, et on distribure le reste au hasard
		else
		{
			// Calcul du nombre de tirage par element du vecteur et du nombre de tirage restants
			dBaseNumber = floor((0.5 + dTotalFrequency) / nValueNumber);
			nRestNumber = (int)floor(0.5 + dTotalFrequency - dBaseNumber * nValueNumber);

			// Protection contre les problemes d'arrondi
			if (nRestNumber < 0)
				nRestNumber = 0;
			if (nRestNumber >= nValueNumber)
				nRestNumber = nValueNumber - 1;

			// On commence a repartir un nombre de base de tirages par element du vecteur
			for (i = 0; i < nValueNumber; i++)
				dvFrequencies->SetAt(i, dBaseNumber);

			// On attribut le reste aleatoirement, en les mettant en debut du vecteur avant le de le
			// melanger aleatoirement
			for (i = 0; i < nRestNumber; i++)
				dvFrequencies->UpgradeAt(i, 1);
			dvFrequencies->Shuffle();
		}
	}
	ensure(nValueNumber == 0 or CheckFrequencyVector(dTotalFrequency, dvFrequencies));
}

void KDMultinomialSampleGenerator::ComputeBestHierarchicalSamples(double dTotalFrequency, int nValueNumber,
								  int nSubValueNumber, DoubleVector* dvFrequencies,
								  DoubleVector* dvSubFrequencies) const
{
	double dSubValueRandomDrawingNumber;

	require(dTotalFrequency >= 0);
	require(nValueNumber >= 0);
	require(nSubValueNumber >= 0);
	require(nValueNumber + nSubValueNumber > 0);
	require(dvFrequencies != NULL);
	require(dvSubFrequencies != NULL);

	// Reinitialisation de la graine aleatoire
	lRankedRandomSeed = 0;

	// Initialisation des tableaux
	dvFrequencies->SetSize(nValueNumber);
	dvSubFrequencies->SetSize(nSubValueNumber);

	// Cas ou il n'y a pas de valeurs secondaires
	if (nSubValueNumber == 0)
	{
		ComputeBestEquidistributedSample(dTotalFrequency, nValueNumber, dvFrequencies);
		dvSubFrequencies->SetSize(0);
	}
	// Cas ou il n'y a pas de valeurs secondaires
	else if (nValueNumber == 0)
	{
		dvFrequencies->SetSize(0);
		ComputeBestEquidistributedSample(dTotalFrequency, nSubValueNumber, dvSubFrequencies);
	}
	// Cas ou il y a des valeurs principales et secondaires
	else
	{
		// Cas ou il y a moins de valeurs principales que de valeurs secondaires: on privilegie les valeurs
		// principales
		if (dTotalFrequency <= nValueNumber)
		{
			ComputeBestEquidistributedSample(dTotalFrequency, nValueNumber, dvFrequencies);

			// On ne mais aucun tirage pour les regles
			dvSubFrequencies->SetSize(0);
			dvSubFrequencies->SetSize(nSubValueNumber);
		}
		// Sinon, on repartit au mieux en fonction des regles tirables
		else
		{
			// Calcul du nombre de tirages attribues aux valeurs secondaires (potentiellement un de moins
			// que par valeur principale a cause de la division entiere)
			dSubValueRandomDrawingNumber = floor((0.5 + dTotalFrequency) / (nValueNumber + 1));

			// Disptach des tirages restant pour les valeurs principales
			ComputeBestEquidistributedSample(dTotalFrequency - dSubValueRandomDrawingNumber, nValueNumber,
							 dvFrequencies);

			// Dispatch des tirages pour les valeurs secondaires
			ComputeBestEquidistributedSample(dSubValueRandomDrawingNumber, nSubValueNumber,
							 dvSubFrequencies);
		}
	}
}

void KDMultinomialSampleGenerator::ComputeBestBaselSample(double dTotalFrequency, int nMaxIndex,
							  DoubleVector* dvFrequencies) const
{
	DoubleVector dvProbs;

	require(dTotalFrequency >= 0);
	require(nMaxIndex > 0);
	require(dvFrequencies != NULL);

	// Reinitialisation de la graine aleatoire
	lRankedRandomSeed = 0;

	// Calcul de l'echantillon
	ComputeBaselProbs(nMaxIndex, &dvProbs);
	ComputeBestSample(dTotalFrequency, &dvProbs, dvFrequencies);
}

void KDMultinomialSampleGenerator::ComputeBaselProbs(int nMaxIndex, DoubleVector* dvProbs) const
{
	int nIndex;

	require(nMaxIndex >= 0);
	require(dvProbs != NULL);

	// Distribution des probas selon la loi p(k) = 6/(pi^2 * k^2)
	dvProbs->SetSize(nMaxIndex);
	for (nIndex = 0; nIndex < nMaxIndex; nIndex++)
		dvProbs->SetAt(nIndex, ComputeBaselProbAt(nIndex));
}

double KDMultinomialSampleGenerator::ComputeBaselProbAt(int nIndex) const
{
	const double dPi = 3.14159265358979323846;
	double dProb;

	require(nIndex >= 0);

	// Distribution des probas selon la loi p(k) = 6/(pi^2 * k^2)
	dProb = dPi * (nIndex + 1);
	dProb *= dProb;
	dProb = 6.0 / dProb;
	return dProb;
}

double KDMultinomialSampleGenerator::ComputeBaselCodingLengthAt(int nIndex) const
{
	double dCodingLength;
	require(nIndex >= 0);
	dCodingLength = -log(ComputeBaselProbAt(nIndex));
	return dCodingLength;
}

void KDMultinomialSampleGenerator::ComputeBestNaturalNumbersUniversalPriorSample(double dTotalFrequency, int nMaxIndex,
										 DoubleVector* dvFrequencies) const
{
	DoubleVector dvProbs;

	require(dTotalFrequency >= 0);
	require(nMaxIndex > 0);
	require(dvFrequencies != NULL);

	// Reinitialisation de la graine aleatoire
	lRankedRandomSeed = 0;

	// Calcul de l'echantillon
	ComputeNaturalNumbersUniversalPriorProbs(nMaxIndex, &dvProbs);
	ComputeBestSample(dTotalFrequency, &dvProbs, dvFrequencies);
}

void KDMultinomialSampleGenerator::ComputeNaturalNumbersUniversalPriorProbs(int nMaxIndex, DoubleVector* dvProbs) const
{
	int nIndex;

	require(nMaxIndex >= 0);
	require(dvProbs != NULL);

	// Distribution des probas selon le prior universel des nombre entier
	dvProbs->SetSize(nMaxIndex);
	for (nIndex = 1; nIndex <= nMaxIndex; nIndex++)
		dvProbs->SetAt(nIndex - 1, exp(-KWStat::NaturalNumbersUniversalCodeLength(nIndex)));
}

void KDMultinomialSampleGenerator::ComputeBestProductSample(double dTotalFrequency, const DoubleVector* dvProbs1,
							    const DoubleVector* dvProbs2,
							    ObjectArray* oaIndexedFrequencies) const
{
	ObjectArray oaProbVectors;

	// Reinitialisation de la graine aleatoire
	lRankedRandomSeed = 0;

	// Appel du cas a plus de deux vecteurs de proabbilites
	oaProbVectors.SetSize(2);
	oaProbVectors.SetAt(0, cast(Object*, dvProbs1));
	oaProbVectors.SetAt(1, cast(Object*, dvProbs2));
	ComputeBestMultipleProductSample(dTotalFrequency, &oaProbVectors, oaIndexedFrequencies);
}

void KDMultinomialSampleGenerator::ComputeBestMultipleProductSample(double dTotalFrequency,
								    const ObjectArray* oaProbVectors,
								    ObjectArray* oaIndexedFrequencies) const
{
	KDIndexedFrequency* indexedFrequency;
	int nDimNumber;
	int nDim;
	ObjectArray oaSortedProbVectors;
	DoubleVector* dvProbs;
	ObjectArray* oaSortedProbs;
	int nIndex;
	KWSortableValue* sortedProb;
	SortedList slIndexedFrequencies(KDIndexedFrequencyCompareProb);
	KDIndexedFrequency currentIndexedFrequency;
	double dLargestProb;
	DoubleVector dvProductProbs;
	DoubleVector dvFrequencies;
	int i;

	require(dTotalFrequency >= 0);
	require(oaProbVectors != NULL);
	require(oaProbVectors->GetSize() > 0);
	require(oaIndexedFrequencies != NULL);

	// Reinitialisation de la graine aleatoire
	lRankedRandomSeed = 0;

	// Nettoyage
	oaIndexedFrequencies->SetSize(0);

	// Nombre de dimensions
	nDimNumber = oaProbVectors->GetSize();

	// Tri des probabilites par valeur decroissante
	oaSortedProbVectors.SetSize(nDimNumber);
	for (nDim = 0; nDim < nDimNumber; nDim++)
	{
		dvProbs = cast(DoubleVector*, oaProbVectors->GetAt(nDim));
		assert(CheckPartialProbVector(dvProbs));

		// Collecte des proba dans un tableau de proba indexes
		oaSortedProbs = new ObjectArray;
		oaSortedProbs->SetSize(dvProbs->GetSize());
		for (nIndex = 0; nIndex < dvProbs->GetSize(); nIndex++)
		{
			// Creation d'une paire (index, proba), avec perturbation aleatoire infinisetimale pour
			// rompre les cas d'egalite au hasard
			sortedProb = new KWSortableValue;
			sortedProb->SetIndex(nIndex);
			sortedProb->SetSortValue(EpsilonProbPerturbation(dvProbs->GetAt(nIndex)));
			oaSortedProbs->SetAt(nIndex, sortedProb);
		}

		// Tri d tableau de probas
		oaSortedProbs->SetCompareFunction(KWSortableValueCompareDecreasing);
		oaSortedProbs->Sort();

		// Memorisation
		oaSortedProbVectors.SetAt(nDim, oaSortedProbs);
	}

	// Calcul de la proba la plus grande
	dLargestProb = 1;
	for (nDim = 0; nDim < nDimNumber; nDim++)
	{
		oaSortedProbs = cast(ObjectArray*, oaSortedProbVectors.GetAt(nDim));
		sortedProb = cast(KWSortableValue*, oaSortedProbs->GetAt(0));
		dLargestProb *= sortedProb->GetSortValue();
	}

	// On continue si la proba la plus grande est suffisante
	if (dLargestProb > DBL_MIN)
	{
		// Initialisation d'un template d'effectif indexe
		currentIndexedFrequency.SetIndexSize(nDimNumber);
		currentIndexedFrequency.SetProb(1.0);

		// Calcul recursif des effectifs indexes pour les proba les plus importantes (les seules utiles)
		ComputeBestMultipleProductProbsAt(dTotalFrequency, &oaSortedProbVectors, 0, dLargestProb,
						  &currentIndexedFrequency, &slIndexedFrequencies);

		// Tri des effectifs indexes par proba decroissante
		slIndexedFrequencies.ExportObjectArray(oaIndexedFrequencies);

		// Collecte des proba dans un vecteur
		for (i = 0; i < oaIndexedFrequencies->GetSize(); i++)
		{
			indexedFrequency = cast(KDIndexedFrequency*, oaIndexedFrequencies->GetAt(i));
			dvProductProbs.Add(indexedFrequency->GetProb());
		}

		// Generation d'un echantillon selon ces probas
		if (dvProductProbs.GetSize() > 0)
			ComputeBestSample(dTotalFrequency, &dvProductProbs, &dvFrequencies);

		// Collecte des effectifs calcules
		for (i = 0; i < oaIndexedFrequencies->GetSize(); i++)
		{
			indexedFrequency = cast(KDIndexedFrequency*, oaIndexedFrequencies->GetAt(i));
			indexedFrequency->SetFrequency(dvFrequencies.GetAt(i));
		}
	}

	// Nettoyage
	for (nDim = 0; nDim < nDimNumber; nDim++)
	{
		oaSortedProbs = cast(ObjectArray*, oaSortedProbVectors.GetAt(nDim));
		oaSortedProbs->DeleteAll();
		delete oaSortedProbs;
	}

	// Verification du resultat
	ensure(CheckIndexedFrequencies(dTotalFrequency, oaProbVectors, oaIndexedFrequencies));
}

void KDMultinomialSampleGenerator::ComputeBestSelectionSample(double dTotalFrequency, int nSelectionSize,
							      const DoubleVector* dvProbs,
							      ObjectArray* oaIndexedFrequencies) const
{
	KDIndexedFrequency* indexedFrequency;
	ObjectArray oaSortedProbVectors;
	ObjectArray oaSortedProbs;
	int nIndex;
	KWSortableValue* sortedProb;
	SortedList slIndexedFrequencies(KDIndexedFrequencyCompareProb);
	KDIndexedFrequency currentIndexedFrequency;
	int nDim;
	double dLargestProb;
	DoubleVector dvSelectionProbs;
	DoubleVector dvFrequencies;
	int i;

	require(dTotalFrequency >= 0);
	require(nSelectionSize > 0);
	require(nSelectionSize <= dvProbs->GetSize());
	require(dvProbs != NULL);
	require(dvProbs->GetSize() > 0);
	require(CheckPartialProbVector(dvProbs));
	require(oaIndexedFrequencies != NULL);

	// Reinitialisation de la graine aleatoire
	lRankedRandomSeed = 0;

	// Nettoyage
	oaIndexedFrequencies->SetSize(0);

	// Collecte des proba dans un tableau de proba indexes
	oaSortedProbs.SetSize(dvProbs->GetSize());
	for (nIndex = 0; nIndex < dvProbs->GetSize(); nIndex++)
	{
		// Creation d'une paire (index, proba), avec perturbation aleatoire infinisetimale pour
		// rompre les cas d'egalite au hasard
		sortedProb = new KWSortableValue;
		sortedProb->SetIndex(nIndex);
		sortedProb->SetSortValue(EpsilonProbPerturbation(dvProbs->GetAt(nIndex)));
		oaSortedProbs.SetAt(nIndex, sortedProb);
	}

	// Tri du tableau de probas
	oaSortedProbs.SetCompareFunction(KWSortableValueCompareDecreasing);
	oaSortedProbs.Sort();

	// Calcul de la proba la plus grande
	// On prend compte du facteur de selection K! iterativement
	dLargestProb = 1;
	for (nDim = 0; nDim < nSelectionSize; nDim++)
	{
		sortedProb = cast(KWSortableValue*, oaSortedProbs.GetAt(nDim));
		dLargestProb *= sortedProb->GetSortValue() * (nDim + 1);
	}

	// On continue si la proba la plus grande est suffisante
	if (dLargestProb > DBL_MIN)
	{
		// Initialisation d'un template d'effectif indexe
		currentIndexedFrequency.SetIndexSize(nSelectionSize);
		currentIndexedFrequency.SetProb(1.0);

		// Calcul recursif des effectifs indexes pour les proba les plus importantes (les seules utiles)
		ComputeBestSelectionProbsAt(dTotalFrequency, nSelectionSize, &oaSortedProbs, 0, 0, dLargestProb,
					    &currentIndexedFrequency, &slIndexedFrequencies);

		// Tri des effectifs indexes par proba decroissante
		slIndexedFrequencies.ExportObjectArray(oaIndexedFrequencies);

		// Collecte des proba dans un vecteur, en tenant d'un facteur de selection
		for (i = 0; i < oaIndexedFrequencies->GetSize(); i++)
		{
			indexedFrequency = cast(KDIndexedFrequency*, oaIndexedFrequencies->GetAt(i));
			assert(indexedFrequency->Check());
			dvSelectionProbs.Add(indexedFrequency->GetProb());
		}

		// Generation d'un echantillon selon ces probas
		if (dvSelectionProbs.GetSize() > 0)
			ComputeBestSample(dTotalFrequency, &dvSelectionProbs, &dvFrequencies);

		// Collecte des effectifs calcules
		for (i = 0; i < oaIndexedFrequencies->GetSize(); i++)
		{
			indexedFrequency = cast(KDIndexedFrequency*, oaIndexedFrequencies->GetAt(i));
			indexedFrequency->SetFrequency(dvFrequencies.GetAt(i));
		}
	}

	// Nettoyage
	oaSortedProbs.DeleteAll();
}

double KDMultinomialSampleGenerator::ComputeFrequencyVectorProb(const DoubleVector* dvProbs,
								DoubleVector* dvFrequencies) const
{
	double dInfo;
	dInfo = ComputeFrequencyVectorInfo(dvProbs, dvFrequencies);
	return exp(-dInfo);
}

double KDMultinomialSampleGenerator::ComputeFrequencyVectorInfo(const DoubleVector* dvProbs,
								DoubleVector* dvFrequencies) const
{
	const double dPi = 3.14159265358979323846;
	int i;
	double dInfo;
	double dFrequency;
	double dTotalFrequency;
	double dProb;

	require(CheckPartialProbVector(dvProbs));
	require(dvFrequencies != NULL);
	require(dvFrequencies->GetSize() == dvProbs->GetSize());

	// Calcul de l'effectif total
	dTotalFrequency = 0;
	for (i = 0; i < dvFrequencies->GetSize(); i++)
	{
		dFrequency = dvFrequencies->GetAt(i);
		assert(0 <= dFrequency);
		dTotalFrequency += dFrequency;
	}
	assert(0 <= dTotalFrequency);

	// Calcul base sur le coefficient multinomial
	// Calcul exact si effectif total entier
	dInfo = 0;
	if (dTotalFrequency <= INT_MAX)
	{
		for (i = 0; i < dvFrequencies->GetSize(); i++)
		{
			dFrequency = dvFrequencies->GetAt(i);

			// Ajout d'un terme lie a la proba de la valeur
			dProb = dvProbs->GetAt(i);
			if (dProb > 0 and dFrequency > 0)
				dInfo -= dFrequency * log(dProb);

			// Pris en compte du terme de multinome
			dInfo += KWStat::LnFactorial((int)floor(0.5 + dFrequency));
		}
		assert(0 <= dTotalFrequency);
		dInfo -= KWStat::LnFactorial((int)floor(0.5 + dTotalFrequency));
	}
	// Calcul base sur une approximation de Stirling si on reste dans la limite de la precision des double
	else if (not IsVeryLargeFrequency(dTotalFrequency))
	{
		for (i = 0; i < dvFrequencies->GetSize(); i++)
		{
			dFrequency = dvFrequencies->GetAt(i);

			// Ajout d'un terme lie a la proba de la valeur
			dProb = dvProbs->GetAt(i);
			if (dProb > 0 and dFrequency > 0)
				dInfo -= dFrequency * log(dProb);

			// Calcul de l'entropie
			if (dFrequency > 0)
				dInfo += dFrequency * log(dFrequency) - dFrequency + 0.5 * log(dFrequency) +
					 0.5 * log(2 * dPi) + log(1 + (1.0 / 12 * dFrequency));
		}
		dInfo -= dTotalFrequency * log(dTotalFrequency) - dTotalFrequency + 0.5 * log(dTotalFrequency) +
			 0.5 * log(2 * dPi) + log(1 + (1.0 / 12 * dTotalFrequency));
	}
	// Sinon, on renvoie +inf
	else
		dInfo = DBL_MAX;
	return dInfo;
}

boolean KDMultinomialSampleGenerator::CheckFrequencies(double dTotalFrequency, const DoubleVector* dvProbs,
						       const DoubleVector* dvFrequencies) const
{
	boolean bOk = true;

	require(dTotalFrequency >= 0);
	require(dvProbs != NULL);
	require(CheckPartialProbVector(dvProbs));
	require(dvFrequencies != NULL);

	// Le nombre d'effectifs doit etre egal a celui de probabilites
	if (bOk)
		bOk = (dvProbs->GetSize() == dvFrequencies->GetSize());

	// Verification des effectifs
	if (bOk)
		bOk = CheckFrequencyVector(dTotalFrequency, dvFrequencies);
	return bOk;
}

boolean KDMultinomialSampleGenerator::CheckFrequencyVector(double dTotalFrequency,
							   const DoubleVector* dvFrequencies) const
{
	boolean bOk = true;
	int i;
	double dTotal;

	require(dTotalFrequency >= 0);
	require(dvFrequencies != NULL);

	// Verification des effectifs
	dTotal = 0;
	for (i = 0; i < dvFrequencies->GetSize(); i++)
	{
		if (dvFrequencies->GetAt(i) < 0)
		{
			bOk = false;
			break;
		}
		dTotal += dvFrequencies->GetAt(i);
	}

	// Verification du total, en tenant compte de la limite de precision des double
	if (IsVeryLargeFrequency(dTotalFrequency))
		bOk = fabs(dTotal - dTotalFrequency) < dTotalFrequency * 1e-5;
	else
		bOk = fabs(dTotal - dTotalFrequency) < 1e-5;
	return bOk;
}

boolean KDMultinomialSampleGenerator::CheckIndexedFrequencies(double dTotalFrequency, const ObjectArray* oaProbVectors,
							      ObjectArray* oaIndexedFrequencies) const
{
	boolean bOk = true;
	const KDIndexedFrequency* indexedFrequency;
	const DoubleVector* dvProbs;
	int i;
	int nDim;
	int nIndex;
	double dTotal;
	double dProb;
	double dTotalProb;

	require(dTotalFrequency >= 0);
	require(dTotalFrequency >= 0);
	require(oaProbVectors != NULL);
	require(oaProbVectors->GetSize() > 0);
	require(oaIndexedFrequencies != NULL);

	// Verification de chaque effectif indexe
	dTotal = 0;
	dTotalProb = 0;
	for (i = 0; i < oaIndexedFrequencies->GetSize(); i++)
	{
		indexedFrequency = cast(const KDIndexedFrequency*, oaIndexedFrequencies->GetAt(i));

		// Verification de la taille des index
		if (oaProbVectors->GetSize() != indexedFrequency->GetIndexSize())
		{
			bOk = false;
			break;
		}

		// Verification des index par dimension
		dProb = 1;
		for (nDim = 0; nDim < oaProbVectors->GetSize(); nDim++)
		{
			dvProbs = cast(const DoubleVector*, oaProbVectors->GetAt(nDim));
			nIndex = indexedFrequency->GetIndexAt(nDim);
			if (nIndex < 0 or nIndex >= dvProbs->GetSize())
			{
				bOk = false;
				break;
			}

			// Calcul de la probabilite produit
			dProb *= dvProbs->GetAt(nIndex);
		}

		// Test de validite de la probabilite
		if (bOk)
		{
			assert(0 <= indexedFrequency->GetProb() and indexedFrequency->GetProb() <= 1);
			if (fabs(dProb - indexedFrequency->GetProb()) > 1e-5 * dProb)
			{
				bOk = false;
				break;
			}
			else
				dTotalProb += dProb;
		}

		// Prise en compte de l'effectif
		assert(0 <= indexedFrequency->GetFrequency());
		if (bOk)
			dTotal += indexedFrequency->GetFrequency();

		// Arret si erreur
		if (not bOk)
			break;
	}

	// Test de la probabilite totale
	if (bOk)
		bOk = (dTotalProb < 1 + 1e-5);

	// Test de l'effectif total
	if (IsVeryLargeFrequency(dTotalFrequency))
		bOk = fabs(dTotal - dTotalFrequency) < dTotalFrequency * 1e-5;
	else
		bOk = fabs(dTotal - dTotalFrequency) < 1e-5;
	return bOk;
}

boolean KDMultinomialSampleGenerator::CheckProbVector(const DoubleVector* dvProbs) const
{
	return InternalCheckProbVector(dvProbs, true);
}

boolean KDMultinomialSampleGenerator::CheckPartialProbVector(const DoubleVector* dvProbs) const
{
	return InternalCheckProbVector(dvProbs, false);
}

void KDMultinomialSampleGenerator::WriteProbVector(const DoubleVector* dvProbs, ostream& ost) const
{
	int i;

	require(dvProbs != NULL);

	for (i = 0; i < dvProbs->GetSize(); i++)
		ost << "\t" << dvProbs->GetAt(i);
}

void KDMultinomialSampleGenerator::WriteFrequencyVector(const DoubleVector* dvFrequencies, ostream& ost) const
{
	int i;

	require(dvFrequencies != NULL);

	for (i = 0; i < dvFrequencies->GetSize(); i++)
		ost << "\t" << dvFrequencies->GetAt(i);
}

void KDMultinomialSampleGenerator::WriteIndexedFrequencyArray(const ObjectArray* oaIndexedFrequencies,
							      ostream& ost) const
{
	int i;
	KDIndexedFrequency* indexedFrequency;
	int nDim;
	int nDimNumber;

	require(oaIndexedFrequencies != NULL);

	// Premiere ligne: les probas
	cout << "Prob";
	for (i = 0; i < oaIndexedFrequencies->GetSize(); i++)
	{
		indexedFrequency = cast(KDIndexedFrequency*, oaIndexedFrequencies->GetAt(i));
		cout << "\t" << indexedFrequency->GetProb();
	}
	cout << "\n";

	// Recherche du nombre de dimension
	nDimNumber = 0;
	if (oaIndexedFrequencies->GetSize() > 0)
		nDimNumber = cast(KDIndexedFrequency*, oaIndexedFrequencies->GetAt(0))->GetIndexSize();

	// Lignes suivantes: les index par dimension
	for (nDim = 0; nDim < nDimNumber; nDim++)
	{
		cout << "I" << nDim + 1;
		for (i = 0; i < oaIndexedFrequencies->GetSize(); i++)
		{
			indexedFrequency = cast(KDIndexedFrequency*, oaIndexedFrequencies->GetAt(i));
			cout << "\t";
			if (indexedFrequency->GetIndexSize() >= nDimNumber)
				cout << indexedFrequency->GetIndexAt(nDim);
		}
		cout << "\n";
	}

	// Dernier ligne: les effectifs
	cout << "Frequency";
	for (i = 0; i < oaIndexedFrequencies->GetSize(); i++)
	{
		indexedFrequency = cast(KDIndexedFrequency*, oaIndexedFrequencies->GetAt(i));
		cout << "\t" << indexedFrequency->GetFrequency();
	}
	cout << "\n";
}

void KDMultinomialSampleGenerator::Test()
{
	DoubleVector dvProbs;
	DoubleVector dvBaselProbs;
	DoubleVector dvUniversalPriorProbs;
	KDMultinomialSampleGenerator sampleGenerator;
	DoubleVector dvFrequencies;
	double dTotalFrequency;
	int nMaxIndex;
	int i;

	// On force la graine aleatoire pour ameliorer la reproductivite des tests
	SetRandomSeed(1);

	// Test du generateur selon le piror universel des entier
	dTotalFrequency = 1;
	nMaxIndex = 100;
	cout << "Compute Best Natural Numbers Universal Prior Sample (max " << nMaxIndex << " numbers)" << endl;
	sampleGenerator.ComputeNaturalNumbersUniversalPriorProbs(nMaxIndex, &dvProbs);
	cout << "Probs";
	for (i = 0; i < nMaxIndex; i++)
		cout << "\t" << dvProbs.GetAt(i);
	cout << endl;
	cout << "Frequency";
	for (i = 0; i < nMaxIndex; i++)
		cout << "\t"
		     << "N" << i + 1;
	cout << endl;
	while (dTotalFrequency <= 1e100)
	{
		cout << dTotalFrequency;
		sampleGenerator.ComputeBestNaturalNumbersUniversalPriorSample(dTotalFrequency, nMaxIndex,
									      &dvFrequencies);
		sampleGenerator.WriteFrequencyVector(&dvFrequencies, cout);
		cout << endl;
		if (dTotalFrequency < 9.5)
			dTotalFrequency++;
		else if (dTotalFrequency < 9.5e9)
			dTotalFrequency *= 10;
		else
			dTotalFrequency *= 1e10;
	}
	cout << endl;

	// Test multinomiaux
	dvProbs.SetSize(4);
	dvProbs.SetAt(0, 0.4);
	dvProbs.SetAt(1, 0.3);
	dvProbs.SetAt(2, 0.2);
	dvProbs.SetAt(3, 0.1);

	// Test de generation du meilleur echantillon
	dTotalFrequency = 1;
	cout << "Compute Best Multinomial Sample (4 values)" << endl;
	cout << "Frequency";
	for (i = 0; i < dvProbs.GetSize(); i++)
		cout << "\t"
		     << "N(" << dvProbs.GetAt(i) << ")";
	cout << "\tProb" << endl;
	while (dTotalFrequency <= 1e100)
	{
		sampleGenerator.ComputeBestSample(dTotalFrequency, &dvProbs, &dvFrequencies);
		cout << dTotalFrequency;
		sampleGenerator.WriteFrequencyVector(&dvFrequencies, cout);
		cout << "\t" << sampleGenerator.ComputeFrequencyVectorProb(&dvProbs, &dvFrequencies) << "\n";
		if (dTotalFrequency < 9.5)
			dTotalFrequency++;
		else if (dTotalFrequency < 9.5e9)
			dTotalFrequency *= 10;
		else
			dTotalFrequency *= 1e10;
	}
	cout << endl;

	// Test multinomiaux
	TestMultinomial(50, &dvProbs);

	// Test multinomiaux equidistribues
	TestEquidistributed(20, 5);

	// Test multinomiaux hierarchique
	TestHierarchical(10, 3, 0);
	TestHierarchical(10, 0, 4);
	TestHierarchical(20, 3, 4);

	// Test pour la distribution de Basel
	cout << "Basel distribution\n";
	sampleGenerator.ComputeBaselProbs(10, &dvBaselProbs);
	TestMultinomial(10, &dvBaselProbs);

	// Test pour la distribution du prior universel des nombres entiers
	cout << "Natural numbers universal prior distribution\n";
	sampleGenerator.ComputeNaturalNumbersUniversalPriorProbs(20, &dvUniversalPriorProbs);
	TestMultinomial(100, &dvUniversalPriorProbs);

	// Test pour le produit de deux distributions
	TestProduct(100, &dvBaselProbs, &dvUniversalPriorProbs);

	// Test pour le produit de cinq distributions
	TestProducts(100, 5, &dvBaselProbs);

	// Test pour la selection de cinq valeurs d'une distribution
	TestSelection(100, 5, &dvBaselProbs);

	// Test pour des produits de taille croissante
	dvProbs.SetSize(10);
	for (i = 0; i < dvProbs.GetSize(); i++)
		dvProbs.SetAt(i, 1.0 / dvProbs.GetSize());
	for (i = 1; i < 10; i++)
		TestProducts(10, i, &dvProbs);

	// Test pour des selections de taille croissante
	dvProbs.SetSize(10);
	for (i = 0; i < dvProbs.GetSize(); i++)
		dvProbs.SetAt(i, 1.0 / dvProbs.GetSize());
	for (i = 1; i < 10; i++)
		TestSelection(10, i, &dvProbs);
}

void KDMultinomialSampleGenerator::TestMultinomial(int nMaxTotalFrequency, const DoubleVector* dvProbs)
{
	int nFrequency;
	int nValue;
	KDMultinomialSampleGenerator sampleGenerator;
	DoubleVector dvFrequencies;
	DoubleVector dvSubFrequencies;

	require(nMaxTotalFrequency > 0);
	require(dvProbs != NULL);
	require(dvProbs->GetSize() > 0);

	// Entete
	cout << "Multinomial\t" << dvProbs->GetSize() << "\n";
	for (nValue = 0; nValue < dvProbs->GetSize(); nValue++)
		cout << "\t" << dvProbs->GetAt(nValue);
	cout << "\n";
	cout << "Total";
	for (nValue = 0; nValue < dvProbs->GetSize(); nValue++)
		cout << "\t"
		     << "V" << nValue + 1;
	cout << "\tp(E)\n";

	// Generation des echantillons
	for (nFrequency = 0; nFrequency <= nMaxTotalFrequency; nFrequency++)
	{
		sampleGenerator.ComputeBestSample(nFrequency, dvProbs, &dvFrequencies);
		cout << nFrequency;
		sampleGenerator.WriteFrequencyVector(&dvFrequencies, cout);
		cout << "\t" << sampleGenerator.ComputeFrequencyVectorProb(dvProbs, &dvFrequencies) << "\n";
	}
	cout << "\n";
}

void KDMultinomialSampleGenerator::TestEquidistributed(int nMaxTotalFrequency, int nValueNumber)
{
	int nFrequency;
	int nValue;
	KDMultinomialSampleGenerator sampleGenerator;
	DoubleVector dvFrequencies;
	DoubleVector dvSubFrequencies;

	require(nMaxTotalFrequency > 0);
	require(nValueNumber > 0);

	// Entete
	cout << "Equidistributed multinomial\t" << nValueNumber << "\n";
	cout << "Total";
	for (nValue = 0; nValue < nValueNumber; nValue++)
		cout << "\t"
		     << "V" << nValue + 1;
	cout << "\n";

	// Generation des echantillons
	for (nFrequency = 0; nFrequency <= nMaxTotalFrequency; nFrequency++)
	{
		sampleGenerator.ComputeBestEquidistributedSample(nFrequency, nValueNumber, &dvFrequencies);
		cout << nFrequency;
		sampleGenerator.WriteFrequencyVector(&dvFrequencies, cout);
		cout << "\n";
	}
	cout << "\n";
}

void KDMultinomialSampleGenerator::TestHierarchical(int nMaxTotalFrequency, int nValueNumber, int nSubValueNumber)
{
	int nFrequency;
	int nValue;
	KDMultinomialSampleGenerator sampleGenerator;
	DoubleVector dvFrequencies;
	DoubleVector dvSubFrequencies;

	require(nMaxTotalFrequency > 0);
	require(nValueNumber >= 0);
	require(nSubValueNumber >= 0);
	require(nValueNumber + nSubValueNumber > 0);

	// Entete
	cout << "Hierarchical multinomial\t" << nValueNumber << "\t" << nSubValueNumber << "\n";
	cout << "Total";
	for (nValue = 0; nValue < nValueNumber; nValue++)
		cout << "\t"
		     << "V" << nValue + 1;
	for (nValue = 0; nValue < nSubValueNumber; nValue++)
		cout << "\t"
		     << "v" << nValue + 1;
	cout << "\n";

	// Generation des echantillons
	for (nFrequency = 1; nFrequency <= nMaxTotalFrequency; nFrequency++)
	{
		sampleGenerator.ComputeBestHierarchicalSamples(nFrequency, nValueNumber, nSubValueNumber,
							       &dvFrequencies, &dvSubFrequencies);
		cout << nFrequency;
		sampleGenerator.WriteFrequencyVector(&dvFrequencies, cout);
		sampleGenerator.WriteFrequencyVector(&dvSubFrequencies, cout);
		cout << "\n";
	}
	cout << "\n";
}

void KDMultinomialSampleGenerator::TestProduct(int nMaxTotalFrequency, const DoubleVector* dvProbs1,
					       const DoubleVector* dvProbs2)
{
	int nFrequency;
	int nValue;
	KDMultinomialSampleGenerator sampleGenerator;
	ObjectArray oaIndexedFrequencies;

	require(nMaxTotalFrequency > 0);
	require(dvProbs1 != NULL);
	require(dvProbs1->GetSize() > 0);
	require(dvProbs2 != NULL);
	require(dvProbs2->GetSize() > 0);

	// Entete
	cout << "Product\t" << dvProbs1->GetSize() << "\t" << dvProbs2->GetSize() << "\n";
	cout << "Index\tProb1\tProb2\n";
	for (nValue = 0; nValue < dvProbs1->GetSize() or nValue < dvProbs2->GetSize(); nValue++)
	{
		cout << nValue << "\t";
		if (nValue < dvProbs1->GetSize())
			cout << dvProbs1->GetAt(nValue);
		cout << "\t";
		if (nValue < dvProbs2->GetSize())
			cout << dvProbs2->GetAt(nValue);
		cout << "\n";
	}

	// Generation des echantillons
	nFrequency = 0;
	while (nFrequency <= nMaxTotalFrequency)
	{
		sampleGenerator.ComputeBestProductSample(nFrequency, dvProbs1, dvProbs2, &oaIndexedFrequencies);
		cout << "Total frequency\t" << nFrequency << "\n";
		sampleGenerator.WriteIndexedFrequencyArray(&oaIndexedFrequencies, cout);
		oaIndexedFrequencies.DeleteAll();
		if (nFrequency < 5)
			nFrequency++;
		else
			nFrequency *= 2;
	}
	cout << "\n";
}

void KDMultinomialSampleGenerator::TestProducts(int nMaxTotalFrequency, int nDimNumber, const DoubleVector* dvProbs)
{
	int nFrequency;
	int nValue;
	int nDim;
	KDMultinomialSampleGenerator sampleGenerator;
	ObjectArray oaProbVectors;
	ObjectArray oaIndexedFrequencies;

	require(nMaxTotalFrequency > 0);
	require(nDimNumber > 0);
	require(dvProbs != NULL);
	require(dvProbs->GetSize() > 0);

	// Entete
	cout << "Products\t" << nDimNumber << "\t" << dvProbs->GetSize() << "\n";
	cout << "Index\tProb\n";
	for (nValue = 0; nValue < dvProbs->GetSize(); nValue++)
		cout << nValue << "\t" << dvProbs->GetAt(nValue) << "\n";

	// Creation du tableau de vecteurs de probas
	oaProbVectors.SetSize(nDimNumber);
	for (nDim = 0; nDim < nDimNumber; nDim++)
		oaProbVectors.SetAt(nDim, cast(Object*, dvProbs));

	// Generation des echantillons
	nFrequency = 0;
	while (nFrequency <= nMaxTotalFrequency)
	{
		sampleGenerator.ComputeBestMultipleProductSample(nFrequency, &oaProbVectors, &oaIndexedFrequencies);
		cout << "Total frequency\t" << nFrequency << "\n";
		sampleGenerator.WriteIndexedFrequencyArray(&oaIndexedFrequencies, cout);
		oaIndexedFrequencies.DeleteAll();
		if (nFrequency < 5)
			nFrequency++;
		else
			nFrequency *= 2;
	}
	cout << "\n";
}

void KDMultinomialSampleGenerator::TestSelection(int nMaxTotalFrequency, int nSelectionSize,
						 const DoubleVector* dvProbs)
{
	int nFrequency;
	int nValue;
	KDMultinomialSampleGenerator sampleGenerator;
	ObjectArray oaProbVectors;
	ObjectArray oaIndexedFrequencies;
	Timer timerSelection;

	require(nMaxTotalFrequency > 0);
	require(nSelectionSize > 0);
	require(dvProbs != NULL);
	require(dvProbs->GetSize() > 0);

	// Entete
	cout << "Selection\t" << nSelectionSize << "\t" << dvProbs->GetSize() << "\n";
	cout << "Index\tProb\n";
	for (nValue = 0; nValue < dvProbs->GetSize(); nValue++)
		cout << nValue << "\t" << dvProbs->GetAt(nValue) << "\n";

	// Generation des echantillons
	nFrequency = 0;
	while (nFrequency <= nMaxTotalFrequency)
	{
		timerSelection.Reset();
		timerSelection.Start();
		sampleGenerator.ComputeBestSelectionSample(nFrequency, nSelectionSize, dvProbs, &oaIndexedFrequencies);
		timerSelection.Stop();
		cout << "Total frequency\t" << nFrequency << endl;
		cout << "TIME\tElapsed time\t" << timerSelection.GetElapsedTime() << endl;
		sampleGenerator.WriteIndexedFrequencyArray(&oaIndexedFrequencies, cout);
		oaIndexedFrequencies.DeleteAll();
		if (nFrequency < 5)
			nFrequency++;
		else
			nFrequency *= 2;
	}
	cout << endl;
}

void KDMultinomialSampleGenerator::ComputeBestCeilSample(double dTotalFrequency, const DoubleVector* dvProbs,
							 DoubleVector* dvFrequencies) const
{
	const double dEpsilon = 1e-5;
	DoubleVector dvRuleProbs;
	int i;
	double dFrequency;
	double dTotalProb;
	double dTotal;
	double dRemainingProb;
	KWSortableValue* svRemainingProb;
	SortedList slRemainingProbs(KWSortableValueCompare);
	boolean bVeryLargeTotalFrequency;

	require(dTotalFrequency >= 0);
	require(CheckPartialProbVector(dvProbs));

	// Reinitialisation de la graine aleatoire
	lRankedRandomSeed = 0;

	// Indicateur d'effectifs tres important (au dela de la precision numerique des double)
	bVeryLargeTotalFrequency = IsVeryLargeFrequency(dTotalFrequency);

	// Calcul de la proba totale, necessaire si le vecteur de proba est partiel
	dTotalProb = 0;
	for (i = 0; i < dvProbs->GetSize(); i++)
		dTotalProb += dvProbs->GetAt(i);
	assert(dTotalProb > 0);

	// Dispatch en fonction des probas
	// On attribue d'abord les parties entieres des effectifs les plus vraissemblables
	// selon la distribution multionomiale N!/N1!N2!...Nk! * p1^N1 p2^N2 ... Pk^NK
	// en prenant la partie entiere superieure les proba normalisees
	dvFrequencies->SetSize(dvProbs->GetSize());
	dTotal = 0;
	for (i = 0; i < dvProbs->GetSize(); i++)
	{
		// On prend l'entier le plus proche en cas d'effectifs importants
		if (bVeryLargeTotalFrequency)
			dFrequency = floor(0.5 + dTotalFrequency * dvProbs->GetAt(i) / dTotalProb);
		else
			dFrequency = ceil(dTotalFrequency * dvProbs->GetAt(i) / dTotalProb);
		dvFrequencies->SetAt(i, dFrequency);
		dTotal += dFrequency;
	}
	assert(bVeryLargeTotalFrequency or dTotal >= dTotalFrequency - dEpsilon);

	// S'il y a des effectifs en trop, on les supprime un a un en fonction du prochaine la prochaine instance la
	// plus probable selon la distribution multionomiale N/Ni * pi On arrete si on on est en limite de la precision
	// numerique des doubles
	if (not bVeryLargeTotalFrequency and dTotal > dTotalFrequency + dEpsilon)
	{
		assert(dTotal - dTotalFrequency <= dvProbs->GetSize() + dEpsilon);

		// Initialisation d'une liste trie par proba de supression d'instance
		for (i = 0; i < dvProbs->GetSize(); i++)
		{
			// Prise en compte si effectif non nul
			if (dvFrequencies->GetAt(i) > 0)
			{
				// Calcul de la proba de supression
				dRemainingProb = dvProbs->GetAt(i) * dTotal * 1.0 / dvFrequencies->GetAt(i);

				// Perturbation aleatoire infinisetimale pour de gerer les cas d'egalite de facon
				// aleatoire
				dRemainingProb = EpsilonPerturbation(dRemainingProb);

				// Insertion dans la liste triee
				svRemainingProb = new KWSortableValue;
				svRemainingProb->SetIndex(i);
				svRemainingProb->SetSortValue(dRemainingProb);
				slRemainingProbs.Add(svRemainingProb);
			}
		}

		// Supression d'instance jusqu'a atteindre l'effectif desire
		while (dTotal > dTotalFrequency + dEpsilon)
		{
			// Extraction de la tete de liste
			svRemainingProb = cast(KWSortableValue*, slRemainingProbs.GetHead());
			slRemainingProbs.RemoveHead();

			// Supression d'une instance
			i = svRemainingProb->GetIndex();
			dvFrequencies->UpgradeAt(i, -1);
			dTotal--;

			// Reinsertion dans la liste a pres avoir reactualiser le cout
			if (dvFrequencies->GetAt(i) > 0)
			{
				// Calcul de la nouvelle proba d'ajout
				dRemainingProb = dvProbs->GetAt(i) * dTotal * 1.0 / dvFrequencies->GetAt(i);

				// Perturbation aleatoire infinisetimale pour de gerer les cas d'egalite de facon
				// aleatoire
				dRemainingProb = EpsilonPerturbation(dRemainingProb);

				// Reinsertion dans la liste a pres avoir reactualiser le cout
				svRemainingProb->SetSortValue(dRemainingProb);
				slRemainingProbs.Add(svRemainingProb);
			}
			else
				delete svRemainingProb;
		}
		assert(fabs(dTotal - dTotalFrequency) < dEpsilon);

		// Nettoyage
		slRemainingProbs.DeleteAll();
	}
}

void KDMultinomialSampleGenerator::PostOptimizeSample(double dTotalFrequency, const DoubleVector* dvProbs,
						      DoubleVector* dvFrequencies) const
{
	int i;
	double dRemoveProb;
	KWSortableValue* svRemoveProb;
	SortedList slRemoveProbs(KWSortableValueCompare);
	double dAddProb;
	KWSortableValue* svAddProb;
	SortedList slAddProbs(KWSortableValueCompare);
	boolean bContinue;
	int nRemoveIndex;
	int nAddIndex;
	boolean bVeryLargeTotalFrequency;

	require(dTotalFrequency >= 0);
	require(CheckPartialProbVector(dvProbs));
	require(CheckFrequencies(dTotalFrequency, dvProbs, dvFrequencies));

	// Pas de post-optimisation si pas d'instance
	if (dTotalFrequency == 0)
		return;

	// Indicateur d'effectifs tres important (au dela de la precision numerique des double)
	bVeryLargeTotalFrequency = IsVeryLargeFrequency(dTotalFrequency);

	// Pas de post-optimisation si on est au dela de la precision numerique des double
	if (bVeryLargeTotalFrequency)
		return;

	// Initialisation d'une liste trie par proba de supression d'instance
	// et d'une liste triee par ajout d'instance
	for (i = 0; i < dvProbs->GetSize(); i++)
	{
		// Calcul de la proba d'ajout
		dAddProb = dvProbs->GetAt(i) / (dvFrequencies->GetAt(i) + 1.0);
		dAddProb = EpsilonPerturbation(dAddProb);

		// Insertion dans la liste triee
		svAddProb = new KWSortableValue;
		svAddProb->SetIndex(i);
		svAddProb->SetSortValue(dAddProb);
		slAddProbs.Add(svAddProb);

		// Prise en compte de la supression si effectif non nul
		if (dvFrequencies->GetAt(i) > 0)
		{
			// Calcul de la proba de supression
			dRemoveProb = dvProbs->GetAt(i) / dvFrequencies->GetAt(i);
			dRemoveProb = EpsilonPerturbation(dRemoveProb);

			// Insertion dans la liste triee
			svRemoveProb = new KWSortableValue;
			svRemoveProb->SetIndex(i);
			svRemoveProb->SetSortValue(dRemoveProb);
			slRemoveProbs.Add(svRemoveProb);
		}
	}

	// Optimisation tant que amelioration
	bContinue = true;
	while (bContinue)
	{
		assert(slAddProbs.GetCount() > 0 and slRemoveProbs.GetCount() > 0);

		// Extraction de la meilleure supression (tete de liste)
		svRemoveProb = cast(KWSortableValue*, slRemoveProbs.GetHead());
		nRemoveIndex = svRemoveProb->GetIndex();
		dRemoveProb = svRemoveProb->GetSortValue();

		// Extraction du meilleur ajout (queue de liste)
		svAddProb = cast(KWSortableValue*, slAddProbs.GetTail());
		nAddIndex = svAddProb->GetIndex();
		dAddProb = svAddProb->GetSortValue();

		// Test si amelioration
		bContinue = (nRemoveIndex != nAddIndex) and (dAddProb > dRemoveProb * (1.0 + 1e-5));

		// On effectue l'amelioration si necessaire
		if (bContinue)
		{
			// Mise a jour des listes
			slRemoveProbs.RemoveHead();
			slAddProbs.RemoveTail();

			// Supression d'une instance
			dvFrequencies->UpgradeAt(nRemoveIndex, -1);

			// Reinsertion dans la liste a pres avoir reactualiser le cout
			if (dvFrequencies->GetAt(nRemoveIndex) > 0)
			{
				// Calcul de la nouvelle proba d'ajout
				dRemoveProb = dvProbs->GetAt(nRemoveIndex) / dvFrequencies->GetAt(nRemoveIndex);
				dRemoveProb = EpsilonPerturbation(dRemoveProb);

				// Reinsertion dans la liste a pres avoir reactualiser le cout
				svRemoveProb->SetSortValue(dRemoveProb);
				slRemoveProbs.Add(svRemoveProb);
			}
			// Destruction de la supression
			else
				delete svRemoveProb;

			// Ajout d'une instance supplementaire
			dvFrequencies->UpgradeAt(nAddIndex, 1);

			// Calcul de la nouvelle proba d'ajout
			dAddProb = dvProbs->GetAt(nAddIndex) / (dvFrequencies->GetAt(nAddIndex) + 1.0);
			dAddProb = EpsilonPerturbation(dAddProb);

			// Reinsertion dans la liste a pres avoir reactualiser le cout
			svAddProb->SetSortValue(dAddProb);
			slAddProbs.Add(svAddProb);

			// Prise en compte d'une nouvelle supression d'instance si une seul instance
			if (dvFrequencies->GetAt(nAddIndex) == 1)
			{
				// Creation d'une supression
				svRemoveProb = new KWSortableValue;
				svRemoveProb->SetIndex(nAddIndex);

				// Calcul de la nouvelle proba d'ajout
				dRemoveProb = dvProbs->GetAt(nAddIndex);
				dRemoveProb = EpsilonPerturbation(dRemoveProb);

				// Reinsertion dans la liste a pres avoir reactualiser le cout
				svRemoveProb->SetSortValue(dRemoveProb);
				slRemoveProbs.Add(svRemoveProb);
			}
		}
	}

	// Nettoyage
	slRemoveProbs.DeleteAll();
	slAddProbs.DeleteAll();
	ensure(CheckFrequencies(dTotalFrequency, dvProbs, dvFrequencies));
}

void KDMultinomialSampleGenerator::ComputeBestMultipleProductProbsAt(double dTotalFrequency,
								     const ObjectArray* oaSortedProbVectors,
								     int nCurrentDim, double dLargestProb,
								     KDIndexedFrequency* currentIndexedFrequency,
								     SortedList* slIndexedFrequencies) const
{
	KDIndexedFrequency* indexedFrequency;
	ObjectArray* oaSortedProbs;
	KWSortableValue* sortedProb;
	IntVector ivIndexes;
	double dCurrentProb;
	DoubleVector dvProductProbs;
	IntVector dvFrequencies;
	int i;

	require(dTotalFrequency >= 0);
	require(oaSortedProbVectors != NULL);
	require(oaSortedProbVectors->GetSize() > 0);
	require(0 <= nCurrentDim and nCurrentDim < oaSortedProbVectors->GetSize());
	require(dLargestProb > 0);
	require(currentIndexedFrequency != NULL);
	require(slIndexedFrequencies != NULL);

	// Acces au vecteur de proba courant
	oaSortedProbs = cast(ObjectArray*, oaSortedProbVectors->GetAt(nCurrentDim));

	// Acces a la proba courante
	dCurrentProb = currentIndexedFrequency->GetProb();

	// Parcours des probas courante
	for (i = 0; i < oaSortedProbs->GetSize(); i++)
	{
		sortedProb = cast(KWSortableValue*, oaSortedProbs->GetAt(i));

		// Mise a jour de l'index
		currentIndexedFrequency->SetIndexAt(nCurrentDim, sortedProb->GetIndex());

		// Mise a jour de la proba courante
		currentIndexedFrequency->SetProb(dCurrentProb * sortedProb->GetSortValue());

		// Arret pour toute la dimension si proba en cours de calcul plus petite que la plus
		// petite des probas possibles
		if (currentIndexedFrequency->GetProb() * (dTotalFrequency + 2.0 - slIndexedFrequencies->GetCount()) <
		    dLargestProb)
			break;

		// Insertion dans la liste triee si on est sur la derniere dimension
		if (nCurrentDim == oaSortedProbVectors->GetSize() - 1)
		{
			// Insertion de la proba dans la liste
			indexedFrequency = currentIndexedFrequency->Clone();
			slIndexedFrequencies->Add(indexedFrequency);

			// Supression eventuelle de la proba la plus petite
			indexedFrequency = cast(KDIndexedFrequency*, slIndexedFrequencies->GetTail());
			if (indexedFrequency->GetProb() * (dTotalFrequency + 2.0 - slIndexedFrequencies->GetCount()) <
			    dLargestProb)
			{
				slIndexedFrequencies->RemoveTail();
				delete indexedFrequency;
			}
		}
		// Sinon, on passe a la dimension suivante
		else
		{
			ComputeBestMultipleProductProbsAt(dTotalFrequency, oaSortedProbVectors, nCurrentDim + 1,
							  dLargestProb, currentIndexedFrequency, slIndexedFrequencies);
		}

		// Arret si assez de probas jointes generes
		// Valide, parce que les proba sont generes par valeurs decroissante
		if (slIndexedFrequencies->GetCount() >= dTotalFrequency)
			break;
	}
}

void KDMultinomialSampleGenerator::ComputeBestSelectionProbsAt(double dTotalFrequency, int nSelectionSize,
							       ObjectArray* oaSortedProbs, int nCurrentDim,
							       int nStartIndex, double dLargestProb,
							       KDIndexedFrequency* currentIndexedFrequency,
							       SortedList* slIndexedFrequencies) const
{
	KDIndexedFrequency* indexedFrequency;
	KWSortableValue* sortedProb;
	IntVector ivIndexes;
	double dCurrentProb;
	DoubleVector dvProductProbs;
	IntVector dvFrequencies;
	int i;
	int nLastIndex;

	require(dTotalFrequency >= 0);
	require(oaSortedProbs != NULL);
	require(oaSortedProbs->GetSize() > 0);
	require(0 <= nCurrentDim and nCurrentDim < nSelectionSize);
	require(0 <= nStartIndex and nStartIndex < oaSortedProbs->GetSize());
	require(dLargestProb > 0);
	require(currentIndexedFrequency != NULL);
	require(slIndexedFrequencies != NULL);

	// Acces a la proba courante
	dCurrentProb = currentIndexedFrequency->GetProb();

	// Parcours des probas courante
	// On restreint le parcours des probas pour eviter les index invariants par permutation
	// en utilisant des index d'index croissant par dimension
	nLastIndex = oaSortedProbs->GetSize() - (nSelectionSize - 1 - nCurrentDim);
	for (i = nStartIndex; i < nLastIndex; i++)
	{
		sortedProb = cast(KWSortableValue*, oaSortedProbs->GetAt(i));

		// Mise a jour de l'index
		currentIndexedFrequency->SetIndexAt(nCurrentDim, sortedProb->GetIndex());

		// Mise a jour de la proba courante en tenant compte du facteur de selection K! de facon iterative
		// Chaque facteur p*k est inferieur a 1, car les probabilites sont triees par ordre decroissant
		assert(sortedProb->GetSortValue() * (nCurrentDim + 1) <= 1.0 + 1e-5);
		currentIndexedFrequency->SetProb(dCurrentProb * sortedProb->GetSortValue() * (nCurrentDim + 1));

		// Arret pour toute la dimension si proba en cours de calcul plus petite que la plus
		// petite des probas possibles
		if (currentIndexedFrequency->GetProb() * (dTotalFrequency + 2.0 - slIndexedFrequencies->GetCount()) <
		    dLargestProb)
			break;

		// Insertion dans la liste triee si on est sur la derniere dimension
		if (nCurrentDim == nSelectionSize - 1)
		{
			assert(CheckDifferentIndexes(currentIndexedFrequency));

			// Insertion de la proba dans la liste, apres avoir trie les index pour assurer l'uncite du
			// vecteur d'index independament de toute permutation
			indexedFrequency = currentIndexedFrequency->Clone();
			indexedFrequency->SortIndexes();
			slIndexedFrequencies->Add(indexedFrequency);
			assert(indexedFrequency->Check());

			// Supression eventuelle de la proba la plus petite
			indexedFrequency = cast(KDIndexedFrequency*, slIndexedFrequencies->GetTail());
			if (indexedFrequency->GetProb() * (dTotalFrequency + 2.0 - slIndexedFrequencies->GetCount()) <
			    dLargestProb)
			{
				slIndexedFrequencies->RemoveTail();
				delete indexedFrequency;
			}
		}
		// Sinon, on passe a la dimension suivante
		else
		{
			ComputeBestSelectionProbsAt(dTotalFrequency, nSelectionSize, oaSortedProbs, nCurrentDim + 1,
						    i + 1, dLargestProb, currentIndexedFrequency, slIndexedFrequencies);
		}

		// Arret si assez de probas jointes generes
		// Valide, parce que les proba sont generes par valeurs decroissante
		if (slIndexedFrequencies->GetCount() >= dTotalFrequency)
			break;
	}
}

boolean KDMultinomialSampleGenerator::CheckDifferentIndexes(const KDIndexedFrequency* indexedFrequency) const
{
	boolean bOk = true;
	int nDim;
	IntVector ivIndexes;

	require(indexedFrequency != NULL);

	// Recopie des index
	ivIndexes.SetSize(indexedFrequency->GetIndexSize());
	for (nDim = 0; nDim < indexedFrequency->GetIndexSize(); nDim++)
		ivIndexes.SetAt(nDim, indexedFrequency->GetIndexAt(nDim));

	// Tri des index, ce qui permet en suite d'en tester l'unicite
	ivIndexes.Sort();
	for (nDim = 1; nDim < ivIndexes.GetSize(); nDim++)
	{
		if (ivIndexes.GetAt(nDim - 1) >= ivIndexes.GetAt(nDim))
		{
			bOk = false;
			break;
		}
	}
	return bOk;
}

double KDMultinomialSampleGenerator::EpsilonPerturbation(double dProb) const
{
	const double dEpsilon = 1e-10;
	double dPerturbatedProb;

	lRankedRandomSeed++;
	dPerturbatedProb = dProb + dProb * dEpsilon * (IthRandomDouble(lRankedRandomSeed) - 0.5);
	ensure(fabs(dProb - dPerturbatedProb) < 1.01 * dProb * dEpsilon / 2.0);
	return dPerturbatedProb;
}

double KDMultinomialSampleGenerator::EpsilonProbPerturbation(double dProb) const
{
	double dPerturbatedProb;

	dPerturbatedProb = EpsilonPerturbation(dProb);
	if (dPerturbatedProb < 0)
		dPerturbatedProb = 0;
	if (dPerturbatedProb > 1)
		dPerturbatedProb = 1;
	return dPerturbatedProb;
}

boolean KDMultinomialSampleGenerator::InternalCheckProbVector(const DoubleVector* dvProbs, boolean bComplete) const
{
	boolean bOk = true;
	int i;
	double dProb;
	double dTotalProb;

	require(dvProbs != NULL);

	if (dvProbs->GetSize() == 0)
		bOk = false;
	else
	{
		// Chaque proba doit etre comprise entre 0 et 1
		dTotalProb = 0;
		for (i = 0; i < dvProbs->GetSize(); i++)
		{
			dProb = dvProbs->GetAt(i);
			if (dProb < 0)
				bOk = false;
			if (dProb > 1)
				bOk = false;
			dTotalProb += dProb;
		}

		// Le total doit inferieur ou egal a 1
		if (dTotalProb > 1 + 1e-5)
			bOk = false;
		if (bComplete and fabs(dTotalProb - 1) > 1e-5)
			bOk = false;
	}
	return bOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KDIndexedFrequency

boolean KDIndexedFrequency::Check() const
{
	boolean bOk = true;
	int i;
	ALString sTmp;

	// Les index doivent etre croissants
	for (i = 0; i < GetIndexSize(); i++)
	{
		if (i > 0 and ivIndexes.GetAt(i - 1) >= ivIndexes.GetAt(i))
		{
			bOk = false;
			AddError(sTmp + "Wrong order for index at " + IntToString(i + 1));
			break;
		}
	}
	return bOk;
}

void KDIndexedFrequency::Write(ostream& ost) const
{
	int i;

	// Index
	ost << "(";
	for (i = 0; i < GetIndexSize(); i++)
	{
		if (i > 0)
			ost << ", ";
		ost << ivIndexes.GetAt(i);
	}
	ost << ")\t";

	// Proba
	ost << dProb << "\t";

	// Effectif
	ost << dFrequency << "\n";
}

const ALString KDIndexedFrequency::GetObjectLabel() const
{
	ALString sLabel;
	int i;

	sLabel = "(";
	for (i = 0; i < GetIndexSize(); i++)
	{
		if (i > 0)
			sLabel += ", ";
		sLabel += IntToString(ivIndexes.GetAt(i));
	}
	sLabel += ")";
	return sLabel;
}

const ALString KDIndexedFrequency::GetClassLabel() const
{
	return "Indexed frequency";
}

int KDIndexedFrequencyCompareProb(const void* elem1, const void* elem2)
{
	double dResult;

	// Comparaison sur le critere de tri
	dResult = cast(KDIndexedFrequency*, *(Object**)elem1)->GetProb() -
		  cast(KDIndexedFrequency*, *(Object**)elem2)->GetProb();
	if (dResult > 0)
		return -1;
	else if (dResult < 0)
		return 1;
	else
		return 0;
}
