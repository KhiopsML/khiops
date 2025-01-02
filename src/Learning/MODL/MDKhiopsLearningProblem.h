// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWPredictorNaiveBayes.h"
#include "SNBPredictorSelectiveNaiveBayes.h"
#include "KWPredictorDataGrid.h"
#include "KWLearningProblem.h"
#include "KWLearningBenchmark.h"
#include "KWLearningBenchmarkUnivariate.h"
#include "KWLearningBenchmarkBivariate.h"
#include "KWAttributeSubsetStats.h"
#include "MDKhiopsModelingSpec.h"

////////////////////////////////////////////////////////////
// Classe MDKhiopsLearningProblem
//    Khiops: preparation des donnees et modelisation
// Extension de la classe KWLearningProblem avec les
// familles de predicteurs naive Bayes et data grid,
// ainsi que des actions etendues sur les benchmarks
class MDKhiopsLearningProblem : public KWLearningProblem
{
public:
	// Constructeur
	MDKhiopsLearningProblem();
	~MDKhiopsLearningProblem();

	// Benchmark de classifier
	KWLearningBenchmark* GetClassifierBenchmark();
	KWLearningBenchmark* GetRegressorBenchmark();
	KWLearningBenchmarkUnivariate* GetClassifierBenchmarkUnivariate();
	KWLearningBenchmarkBivariate* GetClassifierBenchmarkBivariate();

	// Recherche des predicteurs a utiliser
	// Redefinition de la methode pour rechercher les predicteurs SNB, NB et DG
	void CollectPredictors(KWClassStats* classStats, ObjectArray* oaPredictors) override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Extension des sous-parties du probleme d'apprentissage
	KWLearningBenchmark* classifierBenchmark;
	KWLearningBenchmark* regressorBenchmark;
	KWLearningBenchmarkUnivariate* classifierBenchmarkUnivariate;
	KWLearningBenchmarkBivariate* classifierBenchmarkBivariate;
};

////////////////////////////////////////////////////////////
// Classe MDKhiopsAnalysisSpec
//    Specialisation des Analysis specs
//    notamment pour avoir  une version etendue des Modeling specs
//    avec parametrage des predicteurs SNB, NB et DG
class MDKhiopsAnalysisSpec : public KWAnalysisSpec
{
public:
	// Constructeur
	MDKhiopsAnalysisSpec();
	~MDKhiopsAnalysisSpec();
};
