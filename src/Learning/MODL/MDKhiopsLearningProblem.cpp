// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MDKhiopsLearningProblem.h"

////////////////////////////////////////////////////////////
// Classe MDKhiopsLearningProblem

MDKhiopsLearningProblem::MDKhiopsLearningProblem()
{
	// Creation explicite des sous-objets supplementaire,
	// ce qui permet de creer des sous-objets specifiques dans des sous-classes
	classifierBenchmark = new KWLearningBenchmark;
	regressorBenchmark = new KWLearningBenchmark;
	classifierBenchmarkUnivariate = new KWLearningBenchmarkUnivariate;
	classifierBenchmarkBivariate = new KWLearningBenchmarkBivariate;

	// Specilisation du parametrage des benchmarks, principalement pour les classifieurs
	classifierBenchmark->SetTargetAttributeType(KWType::Symbol);
	regressorBenchmark->SetTargetAttributeType(KWType::Continuous);
	classifierBenchmarkUnivariate->SetTargetAttributeType(KWType::Symbol);
}

MDKhiopsLearningProblem::~MDKhiopsLearningProblem()
{
	delete classifierBenchmark;
	delete regressorBenchmark;
	delete classifierBenchmarkUnivariate;
	delete classifierBenchmarkBivariate;
}

KWLearningBenchmark* MDKhiopsLearningProblem::GetClassifierBenchmark()
{
	return classifierBenchmark;
}

KWLearningBenchmark* MDKhiopsLearningProblem::GetRegressorBenchmark()
{
	return regressorBenchmark;
}

KWLearningBenchmarkUnivariate* MDKhiopsLearningProblem::GetClassifierBenchmarkUnivariate()
{
	return classifierBenchmarkUnivariate;
}

KWLearningBenchmarkBivariate* MDKhiopsLearningProblem::GetClassifierBenchmarkBivariate()
{
	return classifierBenchmarkBivariate;
}
