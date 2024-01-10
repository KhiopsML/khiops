// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWLearningBenchmark.h"
#include "KWPredictorBivariate.h"

////////////////////////////////////////////////////////////
// Classe KWLearningBenchmarkBivariate
//    Benchmark de predicteurs bivaries, avec mesures
//     specifiques du nombre de groupes/intervalles...
class KWLearningBenchmarkBivariate : public KWLearningBenchmark
{
public:
	// Constructeur
	KWLearningBenchmarkBivariate();
	~KWLearningBenchmarkBivariate();

	// Filtrage des predicteurs specifiables pour l'evaluation
	// (ici: Bivariate uniquement)
	const ALString GetPredictorFilter() const;

	// Liste des criteres additionnels evalues, specifiques au cas bivarie
	//    CellNumber
	//    GridSize
	//    Attribute1PartNumber
	//    Attribute2PartNumber

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Ecriture d'un rapport, avec resume synthetique et detaille par jeux de donnees
	// Les resultats d'evaluation par attributs sont ici agreges par jeux de donnees
	void WriteDatasetEvaluationReport(ostream& ost) const;

	// Creation d'un tableau d'evaluation synthetique pour un jeu de donnees a partir
	// d'un tableau d'evaluation detaille par attribut
	KWStatisticalEvaluation* BuildSyntheticEvaluation(KWStatisticalEvaluation* detailedEvaluation,
							  int nBenchmark) const;

	// Reimplementation des methodes virtuelles
	void CreateCriterions();
	void WriteEvaluationReport(ostream& ost) const;
	KWStatisticalEvaluation* CreateTemplateEvaluation() const;
	void EvaluateExperiment(int nBenchmark, int nPredictor, int nValidation, int nFold, IntVector* ivFoldIndexes);
	void CollectAllResults(boolean bTrain, int nBenchmark, int nPredictor, int nExperiment, int nRun,
			       KWPredictor* trainedPredictor, KWPredictorEvaluation* predictorEvaluation);

	// Index d'experience pour le premier attribut d'un benchmark
	int GetBenchmarkExperimentStartIndex(int nBenchmark) const;

	// Tableau des nombre d'attributs par experience
	mutable IntVector ivBenchmarkExplanatoryAttributePairNumbers;

	// Parametrage de l'ecriture des rapport
	boolean bDatasetSyntheticReport;
};
