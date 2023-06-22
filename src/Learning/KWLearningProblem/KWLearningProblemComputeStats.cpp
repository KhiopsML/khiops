// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWLearningProblem.h"

void KWLearningProblem::ComputeStats()
{
	KWLearningSpec learningSpec;
	KWClassStats* classStats;
	boolean bStatsOk;
	KWClass* kwcClass;
	KWClass* constructedClass;
	KWClassDomain* constructedClassDomain;
	KWClassDomain* initialClassDomain;
	KWClassDomain trainedClassDomain;
	boolean bIsSpecificRegressionLearningSpecNecessary;
	KWDatabase initialDatabase;
	KWDatabase specificRegressionDatabase;
	ObjectArray oaTrainedPredictors;
	ObjectArray oaTrainedPredictorReports;
	ALString sModelingDictionaryFileName;
	ALString sModelingReportName;
	KWPredictor* predictor;
	KWPredictorReport* predictorReport;
	KWPredictorEvaluator localPredictorEvaluator;
	ObjectArray oaTrainPredictorEvaluations;
	ObjectArray oaTestPredictorEvaluations;
	int i;
	ALString sTmp;

	require(FileService::CheckApplicationTmpDir());
	require(CheckClass());
	require(CheckTargetAttribute());
#ifdef DEPRECATED_V10
	require(CheckMandatoryAttributeInPairs());
#endif // DEPRECATED_V10
	require(CheckTrainDatabaseName());
	require(CheckResultFileNames());
	require(GetTrainDatabase()->CheckSelectionValue(GetTrainDatabase()->GetSelectionValue()));
	require(GetTestDatabase()->CheckSelectionValue(GetTestDatabase()->GetSelectionValue()));
	require(GetAnalysisSpec()->GetRecoderSpec()->GetRecodingSpec()->Check());
	require(CheckRecodingSpecs());
	require(CheckPreprocessingSpecs());
	require(
	    GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec()->GetMaxConstructedAttributeNumber() ==
		0 or
	    not GetAnalysisSpec()
		    ->GetModelingSpec()
		    ->GetAttributeConstructionSpec()
		    ->GetConstructionDomain()
		    ->GetImportAttributeConstructionCosts());
	require(not TaskProgression::IsStarted());

	// Demarage du suivi de la tache
	TaskProgression::SetTitle("Train model " + GetClassName() + " " + GetTargetAttributeName());
	TaskProgression::SetDisplayedLevelNumber(2);
	TaskProgression::Start();

	////////////////////////////////////////////////////////////////////////////////////////////
	// Initialisations

	// Initialisation des specifications d'apprentissage avec la classe de depart
	InitializeLearningSpec(&learningSpec, KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName()));

	// Debut de la gestion des erreurs dediees a l'apprentissage
	KWLearningErrorManager::BeginErrorCollection();
	KWLearningErrorManager::AddTask("Data preparation");

	// Destruction de tous les rapports potentiels
	DeleteAllOutputFiles();

	// Memorisation du domaine initial
	initialClassDomain = KWClassDomain::GetCurrentDomain();

	// Import des couts des attributs stockes dans les meta-donnees du dictionnaire si demande
	if (GetAnalysisSpec()
		->GetModelingSpec()
		->GetAttributeConstructionSpec()
		->GetConstructionDomain()
		->GetImportAttributeConstructionCosts())
		bStatsOk = ImportAttributeMetaDataCosts(&learningSpec, constructedClass);
	// Creation d'une classe avec prise en compte eventuelle de construction de variables
	else
		bStatsOk = BuildConstructedClass(&learningSpec, constructedClass);
	constructedClassDomain = NULL;
	if (bStatsOk)
	{
		assert(constructedClass != NULL);
		constructedClassDomain = constructedClass->GetDomain();
		KWClassDomain::SetCurrentDomain(constructedClassDomain);
	}
	assert(bStatsOk or constructedClass == NULL);

	// Recherche de la classe
	kwcClass = learningSpec.GetClass();
	check(kwcClass);
	assert(constructedClass == NULL or kwcClass == constructedClass);
	assert(kwcClass == KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName()));

	// Initialisation de domaine des predcicteurs appris
	trainedClassDomain.SetName("Train");

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Calcul des statistiques et ecriture des rapports de preparation

	// Creation d'un objet de calcul des stats
	classStats = new KWClassStats;

	// Initialisation des objets de calculs des statistiques
	if (bStatsOk and not TaskProgression::IsInterruptionRequested())
		InitializeClassStats(classStats, &learningSpec);

	// Calcul des statistiques
	if (bStatsOk and not TaskProgression::IsInterruptionRequested())
		classStats->ComputeStats();

	// Memorisation de l'eventuelle selection en cours, dont on a besoin potentiellement par la suite
	if (bStatsOk)
	{
		initialDatabase.CopySamplingAndSelectionFrom(learningSpec.GetDatabase());
		specificRegressionDatabase.CopySamplingAndSelectionFrom(learningSpec.GetDatabase());
	}

	// Test du cas particulier de la regression, si la classe cible contient des valeurs Missing
	// Dans ce cas, la variable cible est disponible, mais on a pas pu calcule les stats, et
	// on va le faire cette fois en filtrant les valeur cible manquantes
	bIsSpecificRegressionLearningSpecNecessary = false;
	if (bStatsOk and not classStats->IsStatsComputed() and not TaskProgression::IsInterruptionRequested())
		bIsSpecificRegressionLearningSpecNecessary = IsSpecificRegressionLearningSpecNecessary(&learningSpec);
	if (bIsSpecificRegressionLearningSpecNecessary)
	{
		// Parametrage des learning spec en creant un attribut de filtrage
		PrepareLearningSpecForRegression(&learningSpec);
		specificRegressionDatabase.CopySamplingAndSelectionFrom(learningSpec.GetDatabase());

		// Ajout d'un message indiquant  que l'on va filtrer les valeurs cibles manquantes
		Global::AddWarning("", "",
				   "The missing values of target variable " + GetTargetAttributeName() +
				       " are now filtered in a new attempt to train a model");

		// Recalcul des stats avec les valeurs cibles manquantes filtrees
		classStats->ComputeStats();
		bStatsOk = classStats->IsStatsComputed();
		assert(not learningSpec.IsTargetStatsComputed() or
		       cast(KWDescriptiveContinuousStats*, learningSpec.GetTargetDescriptiveStats())
			       ->GetMissingValueNumber() == 0);

		// On remet la selection de base initiale, pour ne pas perturber l'ecriture des rapports
		learningSpec.GetDatabase()->CopySamplingAndSelectionFrom(&initialDatabase);
	}

	// On conditionne la suite par la validite des classStats, maintenant que l'on a essaye de le calcule deux fois
	// si necessaire
	if (bStatsOk)
		bStatsOk = classStats->IsStatsComputed();

	// Ecriture des rapports de preparation
	if (bStatsOk and not TaskProgression::IsInterruptionRequested())
		WritePreparationReports(classStats);

	// Creation d'une classe de recodage
	if (bStatsOk and analysisSpec->GetRecoderSpec()->GetRecoder() and
	    not TaskProgression::IsInterruptionRequested())
		BuildRecodingClass(initialClassDomain, classStats, &trainedClassDomain);

	// Cas particulier de la regression: on remet la selection de base specific si necessaire, le temps de
	// l'apprentissage
	if (bIsSpecificRegressionLearningSpecNecessary)
		learningSpec.GetDatabase()->CopySamplingAndSelectionFrom(&specificRegressionDatabase);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Apprentissage

	// Apprentissage
	KWLearningErrorManager::AddTask("Modeling");
	if (bStatsOk and not TaskProgression::IsInterruptionRequested())
	{
		// Base vide
		if (classStats->GetInstanceNumber() == 0)
			Global::AddWarning("", "", sTmp + "No training: database is empty");
		// Apprentissage non supervise
		else if (classStats->GetTargetAttributeType() == KWType::None)
			TrainPredictors(initialClassDomain, classStats, &oaTrainedPredictors);
		// L'attribut cible n'a qu'une seule valeur
		else if (classStats->GetTargetDescriptiveStats()->GetValueNumber() < 2)
		{
			if (learningSpec.GetTargetAttributeType() == KWType::Continuous and
			    cast(KWDescriptiveContinuousStats*, learningSpec.GetTargetDescriptiveStats())
				    ->GetMissingValueNumber() > 0)
				Global::AddWarning("", "",
						   sTmp + "No training: target variable has only missing values");
			else
				Global::AddWarning("", "", sTmp + "No training: target variable has only one value");
		}
		// Apprentissage en classification
		else if (classStats->GetTargetAttributeType() == KWType::Symbol)
			TrainPredictors(initialClassDomain, classStats, &oaTrainedPredictors);
		// Apprentissage en regression
		else if (classStats->GetTargetAttributeType() == KWType::Continuous)
			TrainPredictors(initialClassDomain, classStats, &oaTrainedPredictors);
	}

	// Cas particulier de la regression: restitution des learning spec initiales si necessaire
	if (bIsSpecificRegressionLearningSpecNecessary)
		RestoreInitialLearningSpec(&learningSpec, &initialDatabase);

	// Ecriture du rapport de modelisation
	if (bStatsOk and not TaskProgression::IsInterruptionRequested() and
	    analysisResults->GetModelingFileName() != "" and oaTrainedPredictors.GetSize() > 0)
	{
		// Collecte des rapports d'apprentissage
		for (i = 0; i < oaTrainedPredictors.GetSize(); i++)
		{
			predictor = cast(KWPredictor*, oaTrainedPredictors.GetAt(i));
			if (predictor->IsTrained())
				oaTrainedPredictorReports.Add(predictor->GetPredictorReport());
		}

		// Ecriture du rapport
		if (oaTrainedPredictorReports.GetSize() == 0)
			Global::AddWarning("", "", "Modeling report is not written since no predictor was trained");
		else
		{
			sModelingReportName = BuildOutputFilePathName(analysisResults->GetModelingFileName());
			AddSimpleMessage("Write modeling report " + sModelingReportName);
			predictorReport = cast(KWPredictorReport*, oaTrainedPredictorReports.GetAt(0));
			predictorReport->WriteFullReportFile(sModelingReportName, &oaTrainedPredictorReports);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Evaluation

	// Evaluation dans le cas supervise
	if (bStatsOk and not TaskProgression::IsInterruptionRequested() and
	    classStats->GetTargetAttributeType() != KWType::None and oaTrainedPredictors.GetSize() > 0)
	{
		// Evaluation des predicteurs sur la base d'apprentissage
		if (analysisResults->GetTrainEvaluationFileName() != "" and
		    GetTrainDatabase()->GetDatabaseName() != "" and not GetTrainDatabase()->IsEmptySampling())
		{
			KWLearningErrorManager::AddTask("Train evaluation");
			localPredictorEvaluator.EvaluatePredictors(&oaTrainedPredictors, GetTrainDatabase(), "Train",
								   &oaTrainPredictorEvaluations);

			// Ecriture du rapport d'evaluation
			localPredictorEvaluator.WriteEvaluationReport(
			    BuildOutputFilePathName(analysisResults->GetTrainEvaluationFileName()), "Train",
			    &oaTrainPredictorEvaluations);
		}

		// Evaluation des predicteurs sur la base de test
		if (analysisResults->GetTestEvaluationFileName() != "" and
		    GetTestDatabase()->GetDatabaseName() != "" and not GetTestDatabase()->IsEmptySampling())
		{
			KWLearningErrorManager::AddTask("Test evaluation");
			localPredictorEvaluator.EvaluatePredictors(&oaTrainedPredictors, GetTestDatabase(), "Test",
								   &oaTestPredictorEvaluations);

			// Ecriture du rapport d'evaluation
			localPredictorEvaluator.WriteEvaluationReport(
			    BuildOutputFilePathName(analysisResults->GetTestEvaluationFileName()), "Test",
			    &oaTestPredictorEvaluations);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Gestion du fichier de dictionnaires appris

	// Collecte des classes de prediction de predicteurs dans un domaine de classe
	// Les classes des predicteurs sont transferees dans le domaine de classe en sortie, et dereferencees des
	// predicteurs
	CollectTrainedPredictorClasses(&oaTrainedPredictors, &trainedClassDomain);

	// Sauvegarde du fichier des dictionnaires appris si necessaire
	if (trainedClassDomain.GetClassNumber() > 0)
	{
		// Ecriture du fichier des dictionnaires de modelisation
		if (GetAnalysisResults()->GetModelingDictionaryFileName() != "" and
		    not TaskProgression::IsInterruptionRequested())
		{
			KWLearningErrorManager::AddTask("Write modeling dictionary file");
			sModelingDictionaryFileName =
			    BuildOutputFilePathName(GetAnalysisResults()->GetModelingDictionaryFileName());
			AddSimpleMessage("Write modeling dictionary file " + sModelingDictionaryFileName);

			// Sauvegarde des dictionnaires de modelisation
			trainedClassDomain.WriteFile(sModelingDictionaryFileName);
		}
	}

	// Ecriture du rapport JSON
	if (bStatsOk and not TaskProgression::IsInterruptionRequested())
		WriteJSONAnalysisReport(classStats, &oaTrainedPredictorReports, &oaTrainPredictorEvaluations,
					&oaTestPredictorEvaluations);

	// Ajout de log memoire
	MemoryStatsManager::AddLog("ComputeStats .Clean Begin");

	// Nettoyage des predicteurs et des classes apprises, et des evaluations
	trainedClassDomain.DeleteAllClasses();
	oaTrainedPredictors.DeleteAll();
	oaTrainPredictorEvaluations.DeleteAll();
	oaTestPredictorEvaluations.DeleteAll();

	// Nettoyage du domaine
	if (constructedClassDomain != NULL)
	{
		delete constructedClassDomain;
		KWClassDomain::SetCurrentDomain(initialClassDomain);
	}

	// Nettoyage
	delete classStats;

	// Fin de la gestion des erreurs dediees a l'apprentissage
	KWLearningErrorManager::EndErrorCollection();

	// Ajout de log memoire
	MemoryStatsManager::AddLog("ComputeStats .Clean End");

	// Fin du suivi de la tache
	TaskProgression::Stop();

	ensure(not TaskProgression::IsStarted());
}

void KWLearningProblem::BuildConstructedDictionary()
{
	boolean bStatsOk;
	KWClass* constructedClass;
	KWClassDomain* constructedClassDomain;
	KWLearningSpec learningSpec;
	ALString sConstructedDictionaryFileName;
	int nRefMaxTreeNumber;
	int nRefMaxAttributePairNumber;

	require(FileService::CheckApplicationTmpDir());
	require(CheckTrainDatabaseName());
	require(GetTrainDatabase()->Check());
	require(GetTrainDatabase()->CheckSelectionValue(GetTrainDatabase()->GetSelectionValue()));
	require(CheckClass());
	require(CheckTargetAttribute());
	require(not GetAnalysisSpec()
			->GetModelingSpec()
			->GetAttributeConstructionSpec()
			->GetConstructionDomain()
			->GetImportAttributeConstructionCosts());
	require(CheckResultFileNames());
	require(not TaskProgression::IsStarted());

	// Demarage du suivi de la tache
	TaskProgression::SetTitle("Build constructed dictionary " + GetClassName());
	TaskProgression::SetDisplayedLevelNumber(2);
	TaskProgression::Start();

	// On memorise les specification initiales des nombre d'arbres et nombre de paires a construire
	nRefMaxTreeNumber = GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec()->GetMaxTreeNumber();
	nRefMaxAttributePairNumber =
	    GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec()->GetMaxAttributePairNumber();

	// On les positionne temporairement a 0 le temps de la construction de la classe
	GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec()->SetMaxTreeNumber(0);
	GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec()->SetMaxAttributePairNumber(0);

	// Initialisation des specifications d'apprentissage avec la classe de depart
	InitializeLearningSpec(&learningSpec, KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName()));

	// Prise en compte eventuelle de construction de variables
	bStatsOk = BuildConstructedClass(&learningSpec, constructedClass);
	constructedClassDomain = NULL;
	if (bStatsOk and constructedClass != NULL)
		constructedClassDomain = constructedClass->GetDomain();
	assert(bStatsOk or constructedClass == NULL);

	// Sauvegarde du dictionnaire construit
	if (bStatsOk and GetAnalysisResults()->GetModelingDictionaryFileName() != "" and
	    not TaskProgression::IsInterruptionRequested())
	{
		sConstructedDictionaryFileName =
		    BuildOutputFilePathName(GetAnalysisResults()->GetModelingDictionaryFileName());

		// Sauvegarde du dictionnaire construit
		check(constructedClassDomain);
		AddSimpleMessage("Write constructed dictionary file " + sConstructedDictionaryFileName);
		constructedClassDomain->WriteFile(sConstructedDictionaryFileName);
	}

	// Nettoyage du domaine
	if (constructedClassDomain != NULL)
		delete constructedClassDomain;

	// On restitue les valeurs initiales des nombre d'arbres et nombre de paires a construire
	GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec()->SetMaxTreeNumber(nRefMaxTreeNumber);
	GetAnalysisSpec()->GetModelingSpec()->GetAttributeConstructionSpec()->SetMaxAttributePairNumber(
	    nRefMaxAttributePairNumber);

	// Fin du suivi de la tache
	TaskProgression::Stop();

	ensure(not TaskProgression::IsStarted());
}