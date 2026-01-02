// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWLearningBenchmarkBivariate.h"

KWLearningBenchmarkBivariate::KWLearningBenchmarkBivariate()
{
	bDatasetSyntheticReport = true;
}

KWLearningBenchmarkBivariate::~KWLearningBenchmarkBivariate() {}

const ALString KWLearningBenchmarkBivariate::GetPredictorFilter() const
{
	return "Bivariate";
}

void KWLearningBenchmarkBivariate::WriteEvaluationReport(ostream& ost) const
{
	WriteDatasetEvaluationReport(ost);
	ost << "\n\n";
	ost << "All datasets\n\n";
	KWLearningBenchmark::WriteEvaluationReport(ost);
}

void KWLearningBenchmarkBivariate::WriteDatasetEvaluationReport(ostream& ost) const
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

void KWLearningBenchmarkBivariate::CreateCriterions()
{
	require(GetCriterionNumber() == 0);

	// Methode ancetre
	KWLearningBenchmark::CreateCriterions();

	// Ajout des criteres specifiques
	AddCriterion("CellNumber", "Cell number", false);
	AddCriterion("GridSize", "Grid size", false);
	AddCriterion("Attribute1PartNumber", "Att1 part nb", false);
	AddCriterion("Attribute2PartNumber", "Att2 part nb", false);
}

KWStatisticalEvaluation*
KWLearningBenchmarkBivariate::BuildSyntheticEvaluation(KWStatisticalEvaluation* detailedEvaluation,
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
	nAttributeNumber = ivBenchmarkExplanatoryAttributePairNumbers.GetAt(nBenchmark);

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

KWStatisticalEvaluation* KWLearningBenchmarkBivariate::CreateTemplateEvaluation() const
{
	KWStatisticalEvaluation* evaluation;
	int nBenchmark;
	KWBenchmarkSpec* benchmarkSpec;
	ObjectArray oaBenchmarkClasses;
	KWClass* kwcClass;
	KWAttribute* attribute1;
	int nAttribute1;
	KWAttribute* attribute2;
	int nAttribute2;
	int nTotalAttributePairNumber;
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

	// Comptage des paires d'attributs par benchmark
	nTotalAttributePairNumber = 0;
	ivBenchmarkExplanatoryAttributePairNumbers.SetSize(oaBenchmarkSpecs.GetSize());
	for (nBenchmark = 0; nBenchmark < oaBenchmarkSpecs.GetSize(); nBenchmark++)
	{
		// Classe du probleme d'apprentissage
		kwcClass = cast(KWClass*, oaBenchmarkClasses.GetAt(nBenchmark));

		// Memorisation du nombre d'attributs
		ivBenchmarkExplanatoryAttributePairNumbers.SetAt(nBenchmark, 0);
		if (kwcClass != NULL)
		{
			ivBenchmarkExplanatoryAttributePairNumbers.SetAt(
			    nBenchmark,
			    ((kwcClass->GetUsedAttributeNumber() - 1) * (kwcClass->GetUsedAttributeNumber() - 2)) / 2);
		}
		nTotalAttributePairNumber += ivBenchmarkExplanatoryAttributePairNumbers.GetAt(nBenchmark);
	}

	// Creation et parametrage
	evaluation = new KWStatisticalEvaluation;
	evaluation->SetExperimentNumber(nTotalAttributePairNumber);
	evaluation->SetRunNumber(GetCrossValidationNumber() * GetFoldNumber());
	evaluation->SetSignificanceLevel(GetSignificanceLevel());

	// Parametrage des libelles des experiences
	evaluation->SetExperimentName("Dataset\tAttribute1\tAttribute2");
	nExperiment = 0;
	for (nBenchmark = 0; nBenchmark < oaBenchmarkSpecs.GetSize(); nBenchmark++)
	{
		benchmarkSpec = cast(KWBenchmarkSpec*, oaBenchmarkSpecs.GetAt(nBenchmark));

		// Classe du probleme d'apprentissage
		kwcClass = cast(KWClass*, oaBenchmarkClasses.GetAt(nBenchmark));

		// Parcours des paires d'attributs explicatifs
		if (kwcClass != NULL)
		{
			// Premier attribut de la paire
			for (nAttribute1 = 0; nAttribute1 < kwcClass->GetUsedAttributeNumber(); nAttribute1++)
			{
				attribute1 = kwcClass->GetUsedAttributeAt(nAttribute1);

				// Deuxieme attribut de la paire
				for (nAttribute2 = nAttribute1 + 1; nAttribute2 < kwcClass->GetUsedAttributeNumber();
				     nAttribute2++)
				{
					attribute2 = kwcClass->GetUsedAttributeAt(nAttribute2);

					// Ajout des attributs explicatifs
					if (attribute1->GetName() != benchmarkSpec->GetTargetAttributeName() and
					    attribute2->GetName() != benchmarkSpec->GetTargetAttributeName())
					{
						evaluation->SetExperimentLabelAt(nExperiment,
										 benchmarkSpec->GetClassName() + "\t" +
										     attribute1->GetName() + "\t" +
										     attribute2->GetName());
						nExperiment++;
					}
				}
			}
		}
	}
	assert(nExperiment == nTotalAttributePairNumber);

	// Nettoyage
	oaBenchmarkClasses.DeleteAll();

	return evaluation;
}

void KWLearningBenchmarkBivariate::EvaluateExperiment(int nBenchmark, int nPredictor, int nValidation, int nFold,
						      IntVector* ivFoldIndexes)
{
	KWBenchmarkSpec* benchmarkSpec;
	KWPredictorSpec* predictorSpec;
	KWLearningSpec* learningSpec;
	KWPredictorBivariate* predictorBivariate;
	KWClassStats classStats;
	KWAttributePairsSpec attributePairsSpec;
	KWPredictorEvaluation* predictorEvaluation;
	int nRun;
	ALString sMainLabel;
	int nTotalExperimentNumber;
	int nExperimentIndex;
	int nExperimentAttributePairIndex;
	KWClass* kwcClass;
	KWAttribute* attribute1;
	int nAttribute1;
	KWAttribute* attribute2;
	int nAttribute2;

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
	predictorBivariate = cast(KWPredictorBivariate*, predictorSpec->GetPredictor());

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
	// Calcul des statistiques bivariees

	// Parametrage des specifications d'apprentissage par le
	// preprocessing du predicteur
	TaskProgression::DisplayLabel("Train");
	learningSpec->GetPreprocessingSpec()->CopyFrom(predictorSpec->GetPreprocessingSpec());

	// Parametrage des instances a garder en apprentissage
	benchmarkSpec->ComputeDatabaseSelectedInstance(ivFoldIndexes, nFold, true);

	// Calcul des stats descriptives
	classStats.SetLearningSpec(learningSpec);
	attributePairsSpec.SetClassName(learningSpec->GetClass()->GetName());
	attributePairsSpec.SetMaxAttributePairNumber(ivBenchmarkExplanatoryAttributePairNumbers.GetAt(nBenchmark));
	classStats.SetAttributePairsSpec(&attributePairsSpec);
	classStats.ComputeStats();

	// Parametrage de l'apprentissage
	predictorBivariate->SetLearningSpec(learningSpec);
	predictorBivariate->SetClassStats(&classStats);
	predictorBivariate->SetBestBivariate(false);

	///////////////////////////////////////////////////////////////////////
	// Collecte des resultats en apprentissage et test pour chaque attribut

	// Collecte si stats calculees
	if (classStats.IsStatsComputed())
	{
		// Index du run
		nRun = nValidation * GetFoldNumber() + nFold;

		// Boucle sur les paires d'attributs
		nExperimentAttributePairIndex = GetBenchmarkExperimentStartIndex(nBenchmark);
		kwcClass = learningSpec->GetClass();
		for (nAttribute1 = 0; nAttribute1 < kwcClass->GetUsedAttributeNumber(); nAttribute1++)
		{
			attribute1 = kwcClass->GetUsedAttributeAt(nAttribute1);

			// Deuxieme attribut de la paire
			for (nAttribute2 = nAttribute1 + 1; nAttribute2 < kwcClass->GetUsedAttributeNumber();
			     nAttribute2++)
			{
				attribute2 = kwcClass->GetUsedAttributeAt(nAttribute2);

				// Ajout des attributs explicatifs
				if (attribute1->GetName() != benchmarkSpec->GetTargetAttributeName() and
				    attribute2->GetName() != benchmarkSpec->GetTargetAttributeName())
				{
					// Parametrage des instances a garder en apprentissage
					benchmarkSpec->ComputeDatabaseSelectedInstance(ivFoldIndexes, nFold, true);

					// Parametrage de la paire d'attributs par ordre alphabetique
					if (attribute1->GetName() < attribute2->GetName())
					{
						predictorBivariate->SetAttributeName1(attribute1->GetName());
						predictorBivariate->SetAttributeName2(attribute2->GetName());
					}
					else
					{
						predictorBivariate->SetAttributeName1(attribute2->GetName());
						predictorBivariate->SetAttributeName2(attribute1->GetName());
					}

					// Apprentissage pour une paire d'attributs donnee
					TaskProgression::DisplayLabel("Evaluate variable pair " +
								      attribute1->GetName() + " x " +
								      attribute2->GetName());
					predictorBivariate->Train();

					// Collecte si l'apprentissage a reussi
					if (predictorBivariate->IsTrained())
					{
						// Mise a jour des resultats d'evaluation sur tous les criteres en
						// apprentissage
						if (not TaskProgression::IsInterruptionRequested())
						{
							// Mise a jour des resultats
							assert(learningSpec->GetDatabase()->GetObjects()->GetSize() ==
							       0);
							predictorEvaluation =
							    predictorBivariate->Evaluate(learningSpec->GetDatabase());
							CollectAllResults(true, nBenchmark, nPredictor,
									  nExperimentAttributePairIndex, nRun,
									  predictorBivariate, predictorEvaluation);
							delete predictorEvaluation;
							assert(learningSpec->GetDatabase()->GetObjects()->GetSize() ==
							       0);
						}

						// Mise a jour des resultats d'evaluation sur tous les criteres en test
						if (not TaskProgression::IsInterruptionRequested())
						{
							// Parametrage des instances a garder en test
							benchmarkSpec->ComputeDatabaseSelectedInstance(ivFoldIndexes,
												       nFold, false);

							// Mise a jour des resultats
							assert(learningSpec->GetDatabase()->GetObjects()->GetSize() ==
							       0);
							predictorEvaluation =
							    predictorBivariate->Evaluate(learningSpec->GetDatabase());
							CollectAllResults(false, nBenchmark, nPredictor,
									  nExperimentAttributePairIndex, nRun,
									  predictorBivariate, predictorEvaluation);
							delete predictorEvaluation;
							assert(learningSpec->GetDatabase()->GetObjects()->GetSize() ==
							       0);
						}
					}

					// Incrementation de l'index de l'experience
					nExperimentAttributePairIndex++;
				}
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

void KWLearningBenchmarkBivariate::CollectAllResults(boolean bTrain, int nBenchmark, int nPredictor, int nExperiment,
						     int nRun, KWPredictor* trainedPredictor,
						     KWPredictorEvaluation* predictorEvaluation)
{
	KWPredictorBivariate* trainedPredictorBivariate;
	const KWDGSAttributePartition* attribute1Partition;
	const KWDGSAttributePartition* attribute2Partition;
	int nAttribute1PartitionSize;
	int nAttribute2PartitionSize;
	KWStatisticalEvaluation* cellNumberEvaluation;
	KWStatisticalEvaluation* gridSizeEvaluation;
	KWStatisticalEvaluation* attribute1PartNumberEvaluation;
	KWStatisticalEvaluation* attribute2PartNumberEvaluation;
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
	trainedPredictorBivariate = cast(KWPredictorBivariate*, trainedPredictor);

	// Acces aux evaluations des criteres
	cellNumberEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("CellNumber"), nPredictor);
	gridSizeEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("GridSize"), nPredictor);
	attribute1PartNumberEvaluation =
	    GetUpdatableEvaluationAt(GetCriterionIndexAt("Attribute1PartNumber"), nPredictor);
	attribute2PartNumberEvaluation =
	    GetUpdatableEvaluationAt(GetCriterionIndexAt("Attribute2PartNumber"), nPredictor);
	preprocessingComputingTimeEvaluation =
	    GetUpdatableEvaluationAt(GetCriterionIndexAt("PreprocessingComputingTime"), nPredictor);
	totalComputingTimeEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("TotalComputingTime"), nPredictor);

	// Memorisation des resultats d'evaluation, uniquement en train
	if (bTrain)
	{
		// Recherche des attributs de partition de la grille bivariee
		attribute1Partition = trainedPredictorBivariate->GetTrainDataGridStats()->SearchAttribute(
		    trainedPredictorBivariate->GetAttributeName1());
		attribute2Partition = trainedPredictorBivariate->GetTrainDataGridStats()->SearchAttribute(
		    trainedPredictorBivariate->GetAttributeName2());

		// Calcul des taille des partitions
		nAttribute1PartitionSize = 1;
		if (attribute1Partition != NULL)
			nAttribute1PartitionSize = attribute1Partition->GetPartNumber();
		nAttribute2PartitionSize = 2;
		if (attribute2Partition != NULL)
			nAttribute2PartitionSize = attribute2Partition->GetPartNumber();

		// Memorisation des resultats d'evaluation
		cellNumberEvaluation->SetResultAt(
		    nExperiment, nRun, trainedPredictorBivariate->GetTrainDataGridStats()->ComputeSourceCellNumber());
		gridSizeEvaluation->SetResultAt(nExperiment, nRun, nAttribute1PartitionSize * nAttribute2PartitionSize);
		attribute1PartNumberEvaluation->SetResultAt(nExperiment, nRun, nAttribute1PartitionSize);
		attribute2PartNumberEvaluation->SetResultAt(nExperiment, nRun, nAttribute2PartitionSize);

		// Pour le temps de calcul, on prend le temps total pour le jeu d'essai
		// divise par le nombre d'attributs evalues
		preprocessingComputingTimeEvaluation->SetResultAt(
		    nExperiment, nRun,
		    trainedPredictorBivariate->GetClassStats()->GetTotalComputeTime() /
			ivBenchmarkExplanatoryAttributePairNumbers.GetAt(nBenchmark));
		totalComputingTimeEvaluation->SetResultAt(
		    nExperiment, nRun,
		    trainedPredictorBivariate->GetClassStats()->GetTotalComputeTime() /
			ivBenchmarkExplanatoryAttributePairNumbers.GetAt(nBenchmark));
	}
}

int KWLearningBenchmarkBivariate::GetBenchmarkExperimentStartIndex(int nBenchmark) const
{
	int i;
	int nBenchmarkExperimentStartIndex;

	require(0 <= nBenchmark and nBenchmark < oaBenchmarkSpecs.GetSize());
	require(ivBenchmarkExplanatoryAttributePairNumbers.GetSize() == oaBenchmarkSpecs.GetSize());

	// On calcul le nombre d'experience precedent le benchmark
	nBenchmarkExperimentStartIndex = 0;
	for (i = 0; i < nBenchmark; i++)
		nBenchmarkExperimentStartIndex += ivBenchmarkExplanatoryAttributePairNumbers.GetAt(i);
	return nBenchmarkExperimentStartIndex;
}
