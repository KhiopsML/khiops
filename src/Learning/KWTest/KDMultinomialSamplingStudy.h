// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "ALString.h"
#include "Object.h"
#include "Vector.h"
#include "KWStat.h"
#include "KWSortableIndex.h"
#include "SortedList.h"
#include "KDMultinomialSampleGenerator.h"

//////////////////////////////////////////////////////////////////////
// Etude de l'echantillonnage d'une distribution multinomiale
class KDMultinomialSamplingStudy : public Object
{
public:
	// Constructeur
	KDMultinomialSamplingStudy();
	~KDMultinomialSamplingStudy();

	// Parametrage du vecteur de probabilite de la multionomial (doit sommer a 1)
	DoubleVector* GetProbVector();

	// Verification des probas
	boolean CheckProbVector() const;

	// Taille du vecteur de proba
	int GetProbVectorSize() const;

	// Etude sur le tirage d'un echantillon
	void DistributionCompleteStudy(double dMaxTotalFrequency);
	void DistributionStudy(double dTotalFrequency);
	void ComputeAllDistributions(double dTotalFrequency, ObjectArray* oaAllDistributions);
	void ComputeAllDistributionsAt(DoubleVector* dvTemplateFrequencies, double dRemainingFrequency, int nStartIndex,
				       ObjectArray* oaAllDistributions);

	// Recherche de la distribution la plus probable dans un ensemble de distribution
	int SearchBestDistributionIndex(ObjectArray* oaAllDistributions);

	// Estimation heuristique de la distribution la plus probable pour un effectif donne
	void ComputeBestDistribution(double dTotalFrequency, DoubleVector* dvFrequencies) const;

	// Calcul du rang (par proba decroissante) d'un distribution parmi un ensemble
	int SearchDistributionIndex(DoubleVector* dvSearchedFrequencies, ObjectArray* oaAllDistributions);

	// Proba (et info=-log(prob)) d'un vecteur d'effectif
	double ComputeFrequencyVectorProb(DoubleVector* dvFrequencies) const;
	double ComputeFrequencyVectorInfo(DoubleVector* dvFrequencies) const;

	// Effectif total d'un vecteur
	double ComputeTotalFrequency(DoubleVector* dvFrequencies) const;

	// Affichage d'un vecteur d'effectif
	void WriteFrequencyVector(DoubleVector* dvFrequencies, ostream& ost) const;

	// Test
	static void Test();

protected:
	DoubleVector dvProb;
	KDMultinomialSampleGenerator sampleGenerator;
};
