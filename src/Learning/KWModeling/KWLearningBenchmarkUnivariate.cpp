// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWLearningBenchmarkUnivariate.h"

KWLearningBenchmarkUnivariate::KWLearningBenchmarkUnivariate()
{
	bDatasetSyntheticReport = true;
	bEvaluateOptimizationCriterion = false;
}

KWLearningBenchmarkUnivariate::~KWLearningBenchmarkUnivariate() {}

const ALString KWLearningBenchmarkUnivariate::GetPredictorFilter() const
{
	return "Univariate";
}

void KWLearningBenchmarkUnivariate::SetEvaluateOptimizationCriterion(boolean bValue)
{
	bEvaluateOptimizationCriterion = bValue;
}

boolean KWLearningBenchmarkUnivariate::GetEvaluateOptimizationCriterion() const
{
	return bEvaluateOptimizationCriterion;
}

void KWLearningBenchmarkUnivariate::Evaluate()
{
	const double dEpsilon = 1e-6;
	const ObjectArray* oaOptimizationCriterionEvaluationArray;
	const ObjectArray* oaDiffOptimizationCriterionEvaluationArray;
	const ObjectArray* oaIsEqualOptimizationCriterionEvaluationArray;
	KWStatisticalEvaluation* referenceOptimizationCriterionEvaluation;
	KWStatisticalEvaluation* optimizationCriterionEvaluation;
	KWStatisticalEvaluation* diffOptimizationCriterionEvaluation;
	KWStatisticalEvaluation* isEqualOptimizationCriterionEvaluation;
	double dDifference;
	int nPredictor;
	int nExperiment;
	int nRun;

	// Mode de test avance et cache pour forcer un crash de l'outil
	// Permet de tester les methodologie de debugage avancee avec gestion de fichier dump
	if (GetReportFileName() == ":CRASH:")
	{
		cout << "CRASH" << endl;
		KWLearningBenchmarkUnivariate* crashBench = NULL;

		// Appel d'une methode sur un pointeur NULL
		crashBench->Evaluate();
	}

	// Evaluation standard
	KWLearningBenchmark::Evaluate();

	// Calculs additionnels si evaluation du critere d'optimisation
	if (bEvaluateOptimizationCriterion)
	{
		// Recherche des tableau d'evaluation concernes
		oaOptimizationCriterionEvaluationArray =
		    GetAllEvaluationsAt(GetCriterionIndexAt("OptimizationCriterion"));
		oaDiffOptimizationCriterionEvaluationArray =
		    GetAllEvaluationsAt(GetCriterionIndexAt("DiffOptimizationCriterion"));
		oaIsEqualOptimizationCriterionEvaluationArray =
		    GetAllEvaluationsAt(GetCriterionIndexAt("IsEqualOptimizationCriterion"));

		// Recherche de l'evaluation de reference
		referenceOptimizationCriterionEvaluation = NULL;
		if (oaOptimizationCriterionEvaluationArray->GetSize() > 0)
			referenceOptimizationCriterionEvaluation =
			    cast(KWStatisticalEvaluation*, oaOptimizationCriterionEvaluationArray->GetAt(0));

		// On renseigne les valeurs des criteres relatif a la reference,
		// par methode, experience et run
		for (nPredictor = 0; nPredictor < oaOptimizationCriterionEvaluationArray->GetSize(); nPredictor++)
		{
			// Acces aux evaluations
			optimizationCriterionEvaluation =
			    cast(KWStatisticalEvaluation*, oaOptimizationCriterionEvaluationArray->GetAt(nPredictor));
			diffOptimizationCriterionEvaluation = cast(
			    KWStatisticalEvaluation*, oaDiffOptimizationCriterionEvaluationArray->GetAt(nPredictor));
			isEqualOptimizationCriterionEvaluation = cast(
			    KWStatisticalEvaluation*, oaIsEqualOptimizationCriterionEvaluationArray->GetAt(nPredictor));

			// Calcul des valeurs des criteres
			for (nExperiment = 0;
			     nExperiment < referenceOptimizationCriterionEvaluation->GetExperimentNumber();
			     nExperiment++)
			{
				for (nRun = 0; nRun < referenceOptimizationCriterionEvaluation->GetRunNumber(); nRun++)
				{
					// Difference
					dDifference =
					    referenceOptimizationCriterionEvaluation->GetResultAt(nExperiment, nRun) -
					    optimizationCriterionEvaluation->GetResultAt(nExperiment, nRun);
					diffOptimizationCriterionEvaluation->SetResultAt(nExperiment, nRun,
											 dDifference);

					// Egalite (difference relative nulle a epsilon pres)
					isEqualOptimizationCriterionEvaluation->SetResultAt(
					    nExperiment, nRun,
					    fabs(dDifference) <=
						dEpsilon * fabs(referenceOptimizationCriterionEvaluation->GetResultAt(
							       nExperiment, nRun)));
				}
			}
		}
	}
}

void KWLearningBenchmarkUnivariate::WriteEvaluationReport(ostream& ost) const
{
	WriteDatasetEvaluationReport(ost);
	ost << "\n\n";
	ost << "All datasets\n\n";
	KWLearningBenchmark::WriteEvaluationReport(ost);
}

void KWLearningBenchmarkUnivariate::WriteDatasetEvaluationReport(ostream& ost) const
{
	int nBenchmark;
	int nCriterion;
	ObjectArray oaDatasetEvaluations;
	ObjectArray* oaEvaluations;
	int nEvaluation;
	KWStatisticalEvaluation* detailedEvaluation;
	KWStatisticalEvaluation* syntheticEvaluation;

	// Ecriture des rapport synthetiques par jeu de donnees
	if (bDatasetSyntheticReport)
	{
		// Ecriture des rapport synthetiques par jeu de donnees
		for (nBenchmark = 0; nBenchmark < oaBenchmarkSpecs.GetSize(); nBenchmark++)
		{
			// Titre pour cette partie du rapport
			ost << "\n\nDataset\t"
			    << cast(KWBenchmarkSpec*, oaBenchmarkSpecs.GetAt(nBenchmark))->GetClassName() << "\n\n";

			// Ecriture des rapport synthetiques par critere
			for (nCriterion = 0; nCriterion < oaEvaluationArrays.GetSize(); nCriterion++)
			{
				oaEvaluations = cast(ObjectArray*, oaEvaluationArrays.GetAt(nCriterion));

				// Construction d'un tableau d'evaluation pour le critere
				for (nEvaluation = 0; nEvaluation < oaEvaluations->GetSize(); nEvaluation++)
				{
					detailedEvaluation =
					    cast(KWStatisticalEvaluation*, oaEvaluations->GetAt(nEvaluation));
					syntheticEvaluation = BuildSyntheticEvaluation(detailedEvaluation, nBenchmark);
					oaDatasetEvaluations.Add(syntheticEvaluation);
				}

				// Rapport par critere
				if (nCriterion > 0)
					ost << "\n";
				KWStatisticalEvaluation::WriteComparativeReport(ost, &oaDatasetEvaluations);

				// Nettoyage
				oaDatasetEvaluations.DeleteAll();
			}
		}
	}
}

KWStatisticalEvaluation*
KWLearningBenchmarkUnivariate::BuildSyntheticEvaluation(KWStatisticalEvaluation* detailedEvaluation,
							int nBenchmark) const
{
	KWStatisticalEvaluation* syntheticEvaluation;
	KWBenchmarkSpec* benchmarkSpec;
	int nStartIndex;
	int nAttributeNumber;
	int nAttribute;
	int nRun;

	require(detailedEvaluation != NULL);
	require(0 <= nBenchmark and nBenchmark < oaBenchmarkSpecs.GetSize());

	// Recherche des caracteristiques du benchmark
	benchmarkSpec = cast(KWBenchmarkSpec*, oaBenchmarkSpecs.GetAt(nBenchmark));
	nStartIndex = GetBenchmarkExperimentStartIndex(nBenchmark);
	nAttributeNumber = ivBenchmarkExplanatoryAttributeNumbers.GetAt(nBenchmark);

	// Creation de l'evaluation, et initialisation des ses caracteristiques principales
	syntheticEvaluation = new KWStatisticalEvaluation;
	syntheticEvaluation->SetCriterionName(detailedEvaluation->GetCriterionName());
	syntheticEvaluation->SetMethodName(detailedEvaluation->GetMethodName());
	syntheticEvaluation->SetExperimentName(benchmarkSpec->GetClassName());
	syntheticEvaluation->SetMaximization(detailedEvaluation->GetMaximization());
	syntheticEvaluation->SetExperimentNumber(nAttributeNumber);
	syntheticEvaluation->SetRunNumber(detailedEvaluation->GetRunNumber());
	syntheticEvaluation->SetSignificanceLevel(detailedEvaluation->GetSignificanceLevel());

	// Initialisation des libelles des attributs du jeux de donnees
	for (nAttribute = 0; nAttribute < nAttributeNumber; nAttribute++)
		syntheticEvaluation->SetExperimentLabelAt(
		    nAttribute, detailedEvaluation->GetExperimentLabelAt(nStartIndex + nAttribute));

	// Recopie des valeurs des experimentation concernees
	for (nAttribute = 0; nAttribute < nAttributeNumber; nAttribute++)
	{
		for (nRun = 0; nRun < syntheticEvaluation->GetRunNumber(); nRun++)
		{
			syntheticEvaluation->SetResultAt(
			    nAttribute, nRun, detailedEvaluation->GetResultAt(nStartIndex + nAttribute, nRun));
		}
	}

	return syntheticEvaluation;
}

KWStatisticalEvaluation* KWLearningBenchmarkUnivariate::CreateTemplateEvaluation() const
{
	KWStatisticalEvaluation* evaluation;
	int nBenchmark;
	KWBenchmarkSpec* benchmarkSpec;
	ObjectArray oaBenchmarkClasses;
	KWClass* kwcClass;
	KWAttribute* attribute;
	int nAttribute;
	int nTotalAttributeNumber;
	int nExperiment;

	require(Check());

	// Recherche des classes des problemes d'apprentissage
	oaBenchmarkClasses.SetSize(oaBenchmarkSpecs.GetSize());
	for (nBenchmark = 0; nBenchmark < oaBenchmarkSpecs.GetSize(); nBenchmark++)
	{
		benchmarkSpec = cast(KWBenchmarkSpec*, oaBenchmarkSpecs.GetAt(nBenchmark));

		// Classe du probleme d'apprentissage
		kwcClass = BuildBenchmarkClass(benchmarkSpec);
		oaBenchmarkClasses.SetAt(nBenchmark, kwcClass);
	}

	// Comptage des attributs par benchmark
	nTotalAttributeNumber = 0;
	ivBenchmarkExplanatoryAttributeNumbers.SetSize(oaBenchmarkSpecs.GetSize());
	for (nBenchmark = 0; nBenchmark < oaBenchmarkSpecs.GetSize(); nBenchmark++)
	{
		// Classe du probleme d'apprentissage
		kwcClass = cast(KWClass*, oaBenchmarkClasses.GetAt(nBenchmark));

		// Memorisation du nombre d'attributs
		ivBenchmarkExplanatoryAttributeNumbers.SetAt(nBenchmark, 0);
		if (kwcClass != NULL)
		{
			ivBenchmarkExplanatoryAttributeNumbers.SetAt(nBenchmark,
								     kwcClass->GetUsedAttributeNumber() - 1);
		}
		nTotalAttributeNumber += ivBenchmarkExplanatoryAttributeNumbers.GetAt(nBenchmark);
	}

	// Creation et parametrage
	evaluation = new KWStatisticalEvaluation;
	evaluation->SetExperimentNumber(nTotalAttributeNumber);
	evaluation->SetRunNumber(GetCrossValidationNumber() * GetFoldNumber());
	evaluation->SetSignificanceLevel(GetSignificanceLevel());

	// Parametrage des libelles des experiences
	evaluation->SetExperimentName("Dataset\tAttribute");
	nExperiment = 0;
	for (nBenchmark = 0; nBenchmark < oaBenchmarkSpecs.GetSize(); nBenchmark++)
	{
		benchmarkSpec = cast(KWBenchmarkSpec*, oaBenchmarkSpecs.GetAt(nBenchmark));

		// Classe du probleme d'apprentissage
		kwcClass = cast(KWClass*, oaBenchmarkClasses.GetAt(nBenchmark));

		// Parcours des attributs explicatifs
		if (kwcClass != NULL)
		{
			for (nAttribute = 0; nAttribute < kwcClass->GetUsedAttributeNumber(); nAttribute++)
			{
				attribute = kwcClass->GetUsedAttributeAt(nAttribute);

				// Ajout des attributs explicatifs
				if (attribute->GetName() != benchmarkSpec->GetTargetAttributeName())
				{
					evaluation->SetExperimentLabelAt(nExperiment, benchmarkSpec->GetClassName() +
											  "\t" + attribute->GetName());
					nExperiment++;
				}
			}
		}
	}
	assert(nExperiment == nTotalAttributeNumber);

	// Nettoyage
	oaBenchmarkClasses.DeleteAll();

	return evaluation;
}

void KWLearningBenchmarkUnivariate::CreateCriterions()
{
	require(GetCriterionNumber() == 0);

	KWLearningBenchmark::CreateCriterions();

	// Ajout des criteres specifiques
	AddCriterion("ClusterNumber", "Clusters", false);
	AddCriterion("MonoCluster", "Mono", true);

	// Ajout des criteres pour l'evaluation du critere d'optimisation
	if (bEvaluateOptimizationCriterion)
	{
		AddCriterion("OptimizationCriterion", "Criterion", false);
		AddCriterion("DiffOptimizationCriterion", "Diff", false);
		AddCriterion("IsEqualOptimizationCriterion", "Opt", true);
	}
}

void KWLearningBenchmarkUnivariate::EvaluateExperiment(int nBenchmark, int nPredictor, int nValidation, int nFold,
						       IntVector* ivFoldIndexes)
{
	KWBenchmarkSpec* benchmarkSpec;
	KWPredictorSpec* predictorSpec;
	KWLearningSpec* learningSpec;
	KWPredictorUnivariate* predictorUnivariate;
	KWPredictorEvaluation* predictorEvaluation;
	KWClassStats classStats;
	KWAttributePairsSpec attributePairsSpec;
	int nRun;
	ALString sMainLabel;
	int nTotalExperimentNumber;
	int nExperimentIndex;
	int nExperimentAttributeIndex;
	KWClass* kwcClass;
	KWAttribute* attribute;
	int nAttribute;

	require(0 <= nBenchmark and nBenchmark < GetBenchmarkSpecs()->GetSize());
	require(0 <= nPredictor and nPredictor < GetPredictorSpecs()->GetSize());
	require(0 <= nValidation and nValidation < GetCrossValidationNumber());
	require(0 <= nFold and nFold < GetFoldNumber());
	require(ivFoldIndexes != NULL);

	//////////////////////////////////////////////////////////////////
	// Acces aux parametres de l'experience

	// Specifications du benchmark
	benchmarkSpec = cast(KWBenchmarkSpec*, GetBenchmarkSpecs()->GetAt(nBenchmark));
	assert(benchmarkSpec->Check());
	assert(benchmarkSpec->IsLearningSpecValid());

	// Probleme d'apprentissage
	learningSpec = benchmarkSpec->GetLearningSpec();

	// Specifications du predicteur
	predictorSpec = cast(KWPredictorSpec*, GetPredictorSpecs()->GetAt(nPredictor));
	assert(predictorSpec->Check());

	// Classifieur
	predictorUnivariate = cast(KWPredictorUnivariate*, predictorSpec->GetPredictor());

	// Suivi de tache
	sMainLabel = benchmarkSpec->GetClassName();
	if (GetCrossValidationNumber() > 1)
		sMainLabel = sMainLabel + " Iter " + IntToString(nValidation);
	sMainLabel = sMainLabel + " " + predictorSpec->GetObjectLabel();
	sMainLabel = sMainLabel + " Fold " + IntToString(nFold + 1);
	TaskProgression::DisplayMainLabel(sMainLabel);
	nTotalExperimentNumber = GetBenchmarkSpecs()->GetSize() * GetPredictorSpecs()->GetSize() *
				 GetCrossValidationNumber() * GetFoldNumber();
	nExperimentIndex = nBenchmark * GetCrossValidationNumber() * GetPredictorSpecs()->GetSize() * GetFoldNumber() +
			   nValidation * GetPredictorSpecs()->GetSize() * GetFoldNumber() +
			   nPredictor * GetFoldNumber() + nFold + 1;
	TaskProgression::DisplayProgression((nExperimentIndex * 100) / nTotalExperimentNumber);

	//////////////////////////////////////////////////////////
	// Calcul des statistiques univariees

	// Parametrage des specifications d'apprentissage par le
	// preprocessing du predicteur
	TaskProgression::DisplayLabel("train");
	learningSpec->GetPreprocessingSpec()->CopyFrom(predictorSpec->GetPreprocessingSpec());

	// Parametrage des instances a garder en apprentissage
	benchmarkSpec->ComputeDatabaseSelectedInstance(ivFoldIndexes, nFold, true);

	// Parametrage du nombre initial d'attributs
	learningSpec->SetInitialAttributeNumber(
	    learningSpec->GetClass()->ComputeInitialAttributeNumber(GetTargetAttributeType() != KWType::None));

	// Calcul des stats descriptives
	classStats.SetLearningSpec(learningSpec);
	attributePairsSpec.SetClassName(learningSpec->GetClass()->GetName());
	classStats.SetAttributePairsSpec(&attributePairsSpec);
	classStats.ComputeStats();

	// Parametrage de l'apprentissage
	predictorUnivariate->SetLearningSpec(learningSpec);
	predictorUnivariate->SetClassStats(&classStats);
	predictorUnivariate->SetBestUnivariate(false);

	///////////////////////////////////////////////////////////////////////
	// Collecte des resultats en apprentissage et test pour chaque attribut

	// Collecte si les stats descriptives sont calculees
	if (classStats.IsStatsComputed())
	{
		// Index du run
		nRun = nValidation * GetFoldNumber() + nFold;

		// Boucle sur les attributs
		nExperimentAttributeIndex = GetBenchmarkExperimentStartIndex(nBenchmark);
		kwcClass = learningSpec->GetClass();
		for (nAttribute = 0; nAttribute < kwcClass->GetUsedAttributeNumber(); nAttribute++)
		{
			attribute = kwcClass->GetUsedAttributeAt(nAttribute);

			// Apprentissage pour un attribut donne
			if (attribute->GetName() != benchmarkSpec->GetTargetAttributeName())
			{
				// Parametrage des instances a garder en apprentissage
				benchmarkSpec->ComputeDatabaseSelectedInstance(ivFoldIndexes, nFold, true);

				// Apprentissage pour un attribut donne
				TaskProgression::DisplayLabel("Evaluate variable " + attribute->GetName());
				predictorUnivariate->SetAttributeName(attribute->GetName());
				predictorUnivariate->Train();

				// Collecte si l'apprentissage a reussi
				if (predictorUnivariate->IsTrained())
				{
					// Mise a jour des resultats d'evaluation sur tous les criteres en apprentissage
					if (not TaskProgression::IsInterruptionRequested())
					{
						// Mise a jour des resultats
						assert(learningSpec->GetDatabase()->GetObjects()->GetSize() == 0);
						predictorEvaluation =
						    predictorUnivariate->Evaluate(learningSpec->GetDatabase());
						CollectAllResults(true, nBenchmark, nPredictor,
								  nExperimentAttributeIndex, nRun, predictorUnivariate,
								  predictorEvaluation);
						delete predictorEvaluation;
						assert(learningSpec->GetDatabase()->GetObjects()->GetSize() == 0);
					}

					// Mise a jour des resultats d'evaluation sur tous les criteres en test
					if (not TaskProgression::IsInterruptionRequested())
					{
						// Parametrage des instances a garder en test
						benchmarkSpec->ComputeDatabaseSelectedInstance(ivFoldIndexes, nFold,
											       false);

						// Mise a jour des resultats
						assert(learningSpec->GetDatabase()->GetObjects()->GetSize() == 0);
						assert(learningSpec->GetDatabase()->GetObjects()->GetSize() == 0);
						predictorEvaluation =
						    predictorUnivariate->Evaluate(learningSpec->GetDatabase());
						CollectAllResults(false, nBenchmark, nPredictor,
								  nExperimentAttributeIndex, nRun, predictorUnivariate,
								  predictorEvaluation);
						delete predictorEvaluation;
					}
				}

				// Incrementation de l'index de l'experience
				nExperimentAttributeIndex++;
			}

			// Arret  si demande
			if (TaskProgression::IsInterruptionRequested())
				break;
		}
	}

	// Nettoyage
	predictorSpec->GetPredictor()->SetClassStats(NULL);
	predictorSpec->GetPredictor()->SetLearningSpec(NULL);
}

void KWLearningBenchmarkUnivariate::CollectAllResults(boolean bTrain, int nBenchmark, int nPredictor, int nExperiment,
						      int nRun, KWPredictor* trainedPredictor,
						      KWPredictorEvaluation* predictorEvaluation)
{
	KWPredictorUnivariate* trainedPredictorUnivariate;
	KWStatisticalEvaluation* monoClusterEvaluation;
	KWStatisticalEvaluation* clusterNumberEvaluation;
	KWStatisticalEvaluation* preprocessingComputingTimeEvaluation;
	KWStatisticalEvaluation* totalComputingTimeEvaluation;

	require(0 <= nBenchmark and nBenchmark < GetBenchmarkSpecs()->GetSize());
	require(0 <= nPredictor and nPredictor < GetPredictorSpecs()->GetSize());
	require(trainedPredictor != NULL);
	require(trainedPredictor->IsTrained());
	require(predictorEvaluation != NULL);
	require(predictorEvaluation->GetTargetType() == GetTargetAttributeType());

	// Methode ancetre
	KWLearningBenchmark::CollectAllResults(bTrain, nBenchmark, nPredictor, nExperiment, nRun, trainedPredictor,
					       predictorEvaluation);

	////////////////////////////////////////////
	// Evaluation des criteres additionnels

	// Acces au predicteur specialise
	trainedPredictorUnivariate = cast(KWPredictorUnivariate*, trainedPredictor);

	// Acces aux evaluations des criteres
	clusterNumberEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("ClusterNumber"), nPredictor);
	monoClusterEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("MonoCluster"), nPredictor);
	preprocessingComputingTimeEvaluation =
	    GetUpdatableEvaluationAt(GetCriterionIndexAt("PreprocessingComputingTime"), nPredictor);
	totalComputingTimeEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("TotalComputingTime"), nPredictor);

	// Memorisation des resultats d'evaluation, uniquement en train
	if (bTrain)
	{
		// Memorisation des resultats d'evaluation
		clusterNumberEvaluation->SetResultAt(
		    nExperiment, nRun, trainedPredictorUnivariate->GetTrainDataGridStats()->ComputeSourceCellNumber());
		monoClusterEvaluation->SetResultAt(
		    nExperiment, nRun,
		    trainedPredictorUnivariate->GetTrainDataGridStats()->ComputeSourceCellNumber() == 1);

		// Pour le temps de calcul, on prend le temps total pour le jeu d'essai
		// divise par le nombre d'attributs evalues
		preprocessingComputingTimeEvaluation->SetResultAt(
		    nExperiment, nRun,
		    trainedPredictorUnivariate->GetClassStats()->GetTotalComputeTime() /
			ivBenchmarkExplanatoryAttributeNumbers.GetAt(nBenchmark));
		totalComputingTimeEvaluation->SetResultAt(
		    nExperiment, nRun,
		    trainedPredictorUnivariate->GetClassStats()->GetTotalComputeTime() /
			ivBenchmarkExplanatoryAttributeNumbers.GetAt(nBenchmark));

		// Memorisation du critere d'optimisation
		if (bEvaluateOptimizationCriterion)
		{
			CollectTrainOptimizationCriterion(nBenchmark, nPredictor, nExperiment, nRun, trainedPredictor,
							  predictorEvaluation);
		}
	}
}

void KWLearningBenchmarkUnivariate::CollectTrainOptimizationCriterion(int nBenchmark, int nPredictor, int nExperiment,
								      int nRun, KWPredictor* trainedPredictor,
								      KWPredictorEvaluation* predictorEvaluation)
{
	KWPredictorUnivariate* predictorUnivariate;
	KWStatisticalEvaluation* optimizationCriterionEvaluation;
	KWClassStats* classStats;
	ALString sAttributeName;
	KWAttributeStats* attributeStats;

	require(bEvaluateOptimizationCriterion);
	require(0 <= nBenchmark and nBenchmark < GetBenchmarkSpecs()->GetSize());
	require(0 <= nPredictor and nPredictor < GetPredictorSpecs()->GetSize());
	require(trainedPredictor != NULL);
	require(trainedPredictor->IsTrained());
	require(predictorEvaluation != NULL);
	require(predictorEvaluation->GetTargetType() == GetTargetAttributeType());

	// Acces au predictor univarie
	predictorUnivariate = cast(KWPredictorUnivariate*, trainedPredictor);

	// Acces a l'evaluation du critere d'optimisation
	optimizationCriterionEvaluation =
	    GetUpdatableEvaluationAt(GetCriterionIndexAt("OptimizationCriterion"), nPredictor);

	// Recherche de la valeur du critere d'optimisation dans les statistiques
	// de preparation correspondant a l'attribut evalue
	sAttributeName = predictorUnivariate->GetAttributeName();
	classStats = predictorUnivariate->GetClassStats();
	attributeStats = classStats->LookupAttributeStats(sAttributeName);
	if (attributeStats != NULL)
		optimizationCriterionEvaluation->SetResultAt(nExperiment, nRun, attributeStats->GetLevel());
}

int KWLearningBenchmarkUnivariate::GetBenchmarkExperimentStartIndex(int nBenchmark) const
{
	int i;
	int nBenchmarkExperimentStartIndex;

	require(0 <= nBenchmark and nBenchmark < oaBenchmarkSpecs.GetSize());
	require(ivBenchmarkExplanatoryAttributeNumbers.GetSize() == oaBenchmarkSpecs.GetSize());

	// On calcul le nombre d'experience precedent le benchmark
	nBenchmarkExperimentStartIndex = 0;
	for (i = 0; i < nBenchmark; i++)
		nBenchmarkExperimentStartIndex += ivBenchmarkExplanatoryAttributeNumbers.GetAt(i);
	return nBenchmarkExperimentStartIndex;
}
