// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWLearningProblem.h"

boolean KWLearningProblem::TrainPredictors(const KWClassDomain* initialDomain, KWClassStats* classStats,
					   ObjectArray* oaTrainedPredictors)
{
	boolean bOk = true;
	ObjectArray oaPredictors;
	ObjectArray oaUnivariatePredictors;
	int i;
	KWPredictor* predictor;

	require(initialDomain != NULL);
	require(initialDomain->LookupClass(GetClassName()) != NULL);
	require(classStats != NULL);
	require(classStats->IsStatsComputed());
	require(oaTrainedPredictors != NULL);
	require(oaTrainedPredictors->GetSize() == 0);

	// Recherche des predicteurs a utiliser
	CollectPredictors(classStats, &oaPredictors);
	CollectUnivariatePredictors(classStats, &oaUnivariatePredictors);
	oaTrainedPredictors->ConcatFrom(&oaPredictors, &oaUnivariatePredictors);

	// Apprentissage des predicteurs
	TaskProgression::SetTitle("Train predictors " + GetClassName() + " " + GetTargetAttributeName());
	for (i = 0; i < oaTrainedPredictors->GetSize(); i++)
	{
		predictor = cast(KWPredictor*, oaTrainedPredictors->GetAt(i));
		bOk = bOk and TrainPredictor(initialDomain, classStats, predictor);
		if (not bOk)
			break;
	}
	return bOk;
}

void KWLearningProblem::CollectTrainedPredictorClasses(ObjectArray* oaTrainedPredictors,
						       KWClassDomain* trainedClassDomain)
{
	int i;
	KWPredictor* predictor;
	KWClass* kwcPredictorClass;
	int nIndex;
	KWAttribute* attribute;
	KWClassDomain* kwcPredictorDomain;
	ALString sPredictorName;
	ALString sPredictorPrefix;
	ALString sPredictorSuffix;

	require(oaTrainedPredictors != NULL);
	require(trainedClassDomain != NULL);

	// Collecte des classes de prediction dans le domaine de classe
	for (i = 0; i < oaTrainedPredictors->GetSize(); i++)
	{
		predictor = cast(KWPredictor*, oaTrainedPredictors->GetAt(i));
		if (predictor->IsTrained())
		{
			// Acces a la classe de prediction et a son domaine
			kwcPredictorClass = predictor->GetTrainedPredictor()->GetPredictorClass();
			kwcPredictorDomain = predictor->GetTrainedPredictor()->GetPredictorDomain();
			assert(KWClassDomain::LookupDomain(kwcPredictorDomain->GetName()) != kwcPredictorDomain);

			// On met les attributs de cle en Used
			for (nIndex = 0; nIndex < kwcPredictorClass->GetKeyAttributeNumber(); nIndex++)
			{
				attribute = kwcPredictorClass->LookupAttribute(
				    kwcPredictorClass->GetKeyAttributeNameAt(nIndex));
				check(attribute);
				attribute->SetUsed(true);
				attribute->SetLoaded(true);
			}

			// On dereference la classe de son predicteur pour qu'elle ne soit pas detruite avec lui
			predictor->GetTrainedPredictor()->RemovePredictor();

			// Transfert des classes de prediction (potentiellement plusieurs dans le cas multi-table)
			sPredictorPrefix = "";
			sPredictorSuffix = "";
			if (predictor->GetPrefix() != "")
				sPredictorPrefix = predictor->GetPrefix() + "_";
			if (predictor->GetSuffix() != "")
				sPredictorSuffix = "_" + predictor->GetSuffix();
			trainedClassDomain->ImportDomain(kwcPredictorDomain, sPredictorPrefix, sPredictorSuffix);
			delete kwcPredictorDomain;
		}
	}
	ensure(trainedClassDomain->Check());
}

void KWLearningProblem::CollectPredictors(KWClassStats* classStats, ObjectArray* oaPredictors)
{
	const KWPredictorBaseline refPredictorBaseline;
	KWPredictorBaseline* predictorBaseline;

	require(classStats != NULL);
	require(classStats->IsStatsComputed());
	require(oaPredictors != NULL);
	require(classStats != NULL);
	require(classStats->IsStatsComputed());

	// Predicteur Baseline
	if (analysisSpec->GetModelingSpec()->GetBaselinePredictor())
	{
		predictorBaseline =
		    cast(KWPredictorBaseline*, KWPredictor::ClonePredictor(refPredictorBaseline.GetName(),
									   classStats->GetTargetAttributeType()));
		if (predictorBaseline != NULL)
			oaPredictors->Add(predictorBaseline);
	}
}

void KWLearningProblem::CollectUnivariatePredictors(KWClassStats* classStats, ObjectArray* oaUnivariatePredictors)
{
	ObjectArray oaAllUnivariateStats;
	const KWPredictorUnivariate refPredictorUnivariate;
	KWPredictorUnivariate* predictorUnivariate;
	KWLearningReport* univariateLearningReport;
	int i;

	require(classStats != NULL);
	require(classStats->IsStatsComputed());
	require(oaUnivariatePredictors != NULL);
	require(oaUnivariatePredictors->GetSize() == 0);

	// Predicteurs univaries
	if (KWPredictor::LookupPredictor(refPredictorUnivariate.GetName(), classStats->GetTargetAttributeType()) and
	    analysisSpec->GetModelingSpec()->GetUnivariatePredictorNumber() > 0)
	{
		// Recherche de toutes les statistiques univariees
		oaAllUnivariateStats.CopyFrom(classStats->GetAttributeStats());
		oaAllUnivariateStats.SetCompareFunction(KWLearningReportCompareSortValue);
		oaAllUnivariateStats.Sort();

		// Creation des meilleurs predicteurs univaries
		for (i = 0; i < oaAllUnivariateStats.GetSize(); i++)
		{
			univariateLearningReport = cast(KWLearningReport*, oaAllUnivariateStats.GetAt(i));

			// Creation tant que le predicteur presente un interet, et que l'on
			// a pas atteint le quota demande
			if (univariateLearningReport->GetSortValue() > 0 and
			    i < analysisSpec->GetModelingSpec()->GetUnivariatePredictorNumber())
			{
				predictorUnivariate =
				    cast(KWPredictorUnivariate*,
					 KWPredictor::ClonePredictor(refPredictorUnivariate.GetName(),
								     classStats->GetTargetAttributeType()));
				if (predictorUnivariate != NULL)
				{
					predictorUnivariate->SetBestUnivariate(false);
					predictorUnivariate->SetAttributeName(univariateLearningReport->GetSortName());
					predictorUnivariate->SetUnivariateRank(i + 1);
					oaUnivariatePredictors->Add(predictorUnivariate);
				}
				else
					break;
			}
		}
	}
}

boolean KWLearningProblem::TrainPredictor(const KWClassDomain* initialDomain, KWClassStats* classStats,
					  KWPredictor* predictor)
{
	boolean bOk = true;
	KWLearningSpec* learningSpec;

	require(initialDomain != NULL);
	require(initialDomain->LookupClass(GetClassName()) != NULL);
	require(classStats != NULL);
	require(classStats->IsStatsComputed());
	require(predictor != NULL);

	bOk = not TaskProgression::IsInterruptionRequested();
	if (bOk)
	{
		// Debut de tache
		TaskProgression::BeginTask();
		TaskProgression::DisplayMainLabel("Train " + predictor->GetName());

		// Acces aux objets d'apprentissage
		learningSpec = classStats->GetLearningSpec();

		// Initialisation du predicteur
		predictor->SetLearningSpec(learningSpec);
		predictor->SetClassStats(classStats);
		predictor->Train();

		// Nettoyage de la classe du predicteur
		if (predictor->IsTrained())
			predictor->GetTrainedPredictor()->CleanPredictorClass(initialDomain);

		// Message si apprentissage effectif
		if (predictor->IsTrained() and not TaskProgression::IsInterruptionRequested())
			AddSimpleMessage("Build " + predictor->GetName() + " " + predictor->GetIdentifier());

		// Fin de tache, avec livbelle de fin en cas de memorisation dans le fichier de suivi des taches
		if (predictor->IsTrained())
			TaskProgression::DisplayLabel("Ok");
		else
			TaskProgression::DisplayLabel("Ko");
		TaskProgression::EndTask();

		// Indicateur de succes
		bOk = predictor->IsTrained() and not TaskProgression::IsInterruptionRequested();
	}
	return bOk;
}
