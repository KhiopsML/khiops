// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MDKhiopsLearningProblem.h"

////////////////////////////////////////////////////////////
// Classe MDKhiopsLearningProblem

MDKhiopsLearningProblem::MDKhiopsLearningProblem()
{
	KWAnalysisSpec* previousAnalysisSpec;

	// Specialisation des specifications d'analyse,
	// en detruisant le sous-objet cree dans la classe ancetre et en le remplacant par une version dediee,
	// apres avoir recupere ses principales valeurs par defaut
	previousAnalysisSpec = analysisSpec;
	analysisSpec = new MDKhiopsAnalysisSpec;
	analysisSpec->CopyFrom(previousAnalysisSpec);
	analysisSpec->GetModelingSpec()->CopyFrom(previousAnalysisSpec->GetModelingSpec());
	analysisSpec->GetModelingSpec()->GetAttributeConstructionSpec()->CopyFrom(
	    previousAnalysisSpec->GetModelingSpec()->GetAttributeConstructionSpec());
	delete previousAnalysisSpec;

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

void MDKhiopsLearningProblem::CollectPredictors(KWClassStats* classStats, ObjectArray* oaPredictors)
{
	const SNBPredictorSelectiveNaiveBayes refPredictorSelectiveNaiveBayes;
	const KWPredictorNaiveBayes refPredictorNaiveBayes;
	const KWPredictorDataGrid refPredictorDataGrid;
	KWPredictor* predictorSelectiveNaiveBayes;
	KWPredictorNaiveBayes* predictorNaiveBayes;
	KWPredictorDataGrid* predictorDataGrid;
	MDKhiopsModelingSpec* khiopsModelingSpec;

	require(classStats != NULL);
	require(classStats->IsStatsComputed());
	require(oaPredictors != NULL);

	// Acces a la version specialisee des specification de modelisation
	khiopsModelingSpec = cast(MDKhiopsModelingSpec*, analysisSpec->GetModelingSpec());

	// Predicteur Bayesien Naif Selectif
	if (khiopsModelingSpec->GetSelectiveNaiveBayesPredictor())
	{
		predictorSelectiveNaiveBayes = KWPredictor::ClonePredictor(refPredictorSelectiveNaiveBayes.GetName(),
									   classStats->GetTargetAttributeType());
		if (predictorSelectiveNaiveBayes != NULL)
		{
			predictorSelectiveNaiveBayes->CopyFrom(khiopsModelingSpec->GetPredictorSelectiveNaiveBayes());
			oaPredictors->Add(predictorSelectiveNaiveBayes);
		}
	}

#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10: le predicteur MAP n'existe plus en V10: on construit a la place un predicteur SNB
		if (khiopsModelingSpec->GetMAPNaiveBayesPredictor())
		{
			// Cas ou on avait pas demande de predicteur SNB
			if (not khiopsModelingSpec->GetSelectiveNaiveBayesPredictor())
			{
				// Warning utilisateur
				AddWarning("MAP selective naive bayes predictor is deprecated since Khiops V10 : a SNB "
					   "predictor is built instead");

				// On construit un predicteur SNB a la place
				predictorSelectiveNaiveBayes = KWPredictor::ClonePredictor(
				    refPredictorSelectiveNaiveBayes.GetName(), classStats->GetTargetAttributeType());
				if (predictorSelectiveNaiveBayes != NULL)
				{
					predictorSelectiveNaiveBayes->CopyFrom(
					    khiopsModelingSpec->GetPredictorSelectiveNaiveBayes());
					oaPredictors->Add(predictorSelectiveNaiveBayes);
				}
			}
			// Cas ou on avait deja demande le predicteur SNB
			else
				AddWarning("MAP selective naive bayes predictor is deprecated since Khiops V10 : the "
					   "built SNB predictor is used instead");
		}
	}
#endif // DEPRECATED_V10

	// Predicteur Bayesien Naif
	if (khiopsModelingSpec->GetNaiveBayesPredictor())
	{
		predictorNaiveBayes =
		    cast(KWPredictorNaiveBayes*, KWPredictor::ClonePredictor(refPredictorNaiveBayes.GetName(),
									     classStats->GetTargetAttributeType()));
		if (predictorNaiveBayes != NULL)
			oaPredictors->Add(predictorNaiveBayes);
		AddWarning("Naive bayes predictor is deprecated since Khiops V10");
	}

	// Predicteur Data Grid
	if (khiopsModelingSpec->GetDataGridPredictor())
	{
		predictorDataGrid =
		    cast(KWPredictorDataGrid*, KWPredictor::ClonePredictor(refPredictorDataGrid.GetName(),
									   classStats->GetTargetAttributeType()));
		if (predictorDataGrid != NULL)
		{
			predictorDataGrid->CopyFrom(khiopsModelingSpec->GetPredictorDataGrid());
			oaPredictors->Add(predictorDataGrid);
		}
	}

	// Appel de la methode ancetre pour completer la liste
	KWLearningProblem::CollectPredictors(classStats, oaPredictors);
}

////////////////////////////////////////////////////////////
// Classe MDKhiopsAnalysisSpec

MDKhiopsAnalysisSpec::MDKhiopsAnalysisSpec()
{
	// Specialisation des specifications dde modelisation
	// en detruisant le sous-objet cree dans la classe ancetre et en le remplacant par une version dediee
	delete modelingSpec;
	modelingSpec = new MDKhiopsModelingSpec;
}

MDKhiopsAnalysisSpec::~MDKhiopsAnalysisSpec() {}
