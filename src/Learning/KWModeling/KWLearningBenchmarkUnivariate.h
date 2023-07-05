// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWLearningBenchmark.h"
#include "KWPredictorUnivariate.h"

////////////////////////////////////////////////////////////
// Classe KWLearningBenchmarkUnivariate
//    Benchmark de predicteurs univaries, avec mesures
//     specifiques du nombre de groupes/intervalles...
class KWLearningBenchmarkUnivariate : public KWLearningBenchmark
{
public:
	// Constructeur
	KWLearningBenchmarkUnivariate();
	~KWLearningBenchmarkUnivariate();

	// Filtrage des predicteurs specifiables pour l'evaluation
	// (ici: Univariate uniquement)
	const ALString GetPredictorFilter() const;

	// Liste des criteres additionnels evalues, specifiques au cas univarie
	//    MonoCluster
	//    ClusterNumber

	// Evaluation de la valeur du critere a optimiser pour l'algorithme de
	//  discretisation ou groupage (defaut: false)
	// Cela n'a pas de sens pour comparer different criteres, mais cela
	//  permet de comparer differente heuristique pour un meme critere
	void SetEvaluateOptimizationCriterion(boolean bValue);
	boolean GetEvaluateOptimizationCriterion() const;

	// Liste des criteres additionnels evalues si l'option d'evaluation
	//  du critere d'optimisation est activee.
	// La premiere methode sert de reference
	//	  OptimizationCriterion
	//	  DiffOptimizationCriterion
	//	  IsEqualOptimizationCriterion

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
	void Evaluate();
	void WriteEvaluationReport(ostream& ost) const;
	KWStatisticalEvaluation* CreateTemplateEvaluation() const;
	void CreateCriterions();
	void EvaluateExperiment(int nBenchmark, int nPredictor, int nValidation, int nFold, IntVector* ivFoldIndexes);
	void CollectAllResults(boolean bTrain, int nBenchmark, int nPredictor, int nExperiment, int nRun,
			       KWPredictor* trainedPredictor, KWPredictorEvaluation* predictorEvaluation);

	// Collecte de la valeur du critere d'optimisation en apprentissage
	void CollectTrainOptimizationCriterion(int nBenchmark, int nPredictor, int nExperiment, int nRun,
					       KWPredictor* trainedPredictor,
					       KWPredictorEvaluation* predictorEvaluation);

	// Index d'experience pour le premier attribut d'un benchmark
	int GetBenchmarkExperimentStartIndex(int nBenchmark) const;

	// Tableau des nombre d'attributs par experience
	mutable IntVector ivBenchmarkExplanatoryAttributeNumbers;

	// Parametrage de l'ecriture des rapport
	boolean bDatasetSyntheticReport;
	boolean bEvaluateOptimizationCriterion;
};