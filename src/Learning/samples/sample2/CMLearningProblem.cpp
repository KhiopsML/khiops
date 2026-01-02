// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CMLearningProblem.h"

CMLearningProblem::CMLearningProblem()
{
	// Specialisation des specifications d'analyse,
	// en detruisant le sous-objet cree dans la classe ancetre et en le remplacant par une version dediee
	delete analysisSpec;
	analysisSpec = new CMAnalysisSpec;

	// Specialisation des noms de fichier resultats en ajoutant un prefixe DT
	analysisResults->SetResultFilesPrefix("Maj_");
}

CMLearningProblem::~CMLearningProblem() {}

// void CMLearningProblem::ActionTrain()
//{
//	KWLearningSpec learningSpec;
//	KWClassStats classStats;
//	KWClass* kwcClass; //Dico
//	ALString className; //Nom du dico
//	ALString sTargetAttributeName; //Nom de la cible (un des champs de la database)
//	ALString sNewName;
//	ALString sReportEvalName;
//	ALString sReportName;
//	ObjectArray oaAllEvaluatedClassifierEvaluations;
//	IntegerVector* ivShuffleIndexes;
//	fstream ost;
//	boolean bStatsOk;
//	Continuous cTrainPercentage;
//
//	require(CheckClass());
//	require(CheckTrainDatabaseName());
//	require(CheckTargetAttribute());
//	require(Check());
//
//	// Demarage du suivi de la tache
//	TaskProgression::SetTitle("Train multi layer perceptron");
//	TaskProgression::SetDisplayedLevelNumber(2);
//	TaskProgression::Start();
//
//	if(!Check())
//	{
//		AddSimpleMessage("Errors in the classifier specification");
//		return;
//	}
//
//	// Acceder au nom du dico
//	className = GetClassManagement()->GetClassName();
//
//	// Acceder au dico (dans le domaine, s'il y en avait plusieurs)
//	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(className);
//	check(kwcClass);
//     // a remettre? kwcClassOld = NULL;
//
//	// Acces a l'attribut cible
//	sTargetAttributeName = GetAnalysisSpec()->GetTargetAttributeName();
//
//	InitializeLearningSpec(&learningSpec, kwcClass);
//	InitializeClassStats(&classStats, &learningSpec);
//
//	// Test integrite des specifications d'apprentissage
//	learningSpec.Check();
//	Check();
//
//	// On calcule les stat de discretisation et de groupage (sur toute la base)
//	classStats.ComputeStats();
//     bStatsOk = classStats.IsStatsComputed();
//
//	// Memorisation du type Forward ou non de l'algo d'apprentissage
//	//GetAnalysisSpec()->GetPredictorSpec()->GetClassifierMultiLayerPerceptron()->GetMLPParameters()->SetForwardClassifier(GetAnalysisSpec()->GetPredictorSpec()->IsForwardClassifier());
//
//	// Memorisation du type Logit ou non de l'apprentissage
//	// Logit : on modelise les ratios P(Y_j | X) / P(Y_J |X)
//	// Sinon : on modelise toutes les probas conditionelles P(Y_j |X)
//	//GetAnalysisSpec()->GetPredictorSpec()->GetClassifierMultiLayerPerceptron()->GetMLPParameters()->SetLogit(GetAnalysisSpec()->GetPredictorSpec()->IsLogitClassifier());
//
//	// Extraction du pourcentage d'individus utilise pour l'apprentissage du reseau de neurones
//	cTrainPercentage =
// GetAnalysisSpec()->GetPredictorSpec()->GetClassifierMultiLayerPerceptron()->GetMLPParameters()->GetTrainPercentage();
//
//	// Calcul et melange des indices marquant les indices des instances utilisees pour le Train
//	ivShuffleIndexes = ComputeDatabaseSelectedInstances(cTrainPercentage);
//
//	// Nettoyage de l'ancien vecteur puis
//	// Memorisation du vecteur d'indices au niveau des parametres du classifieur
//	delete
// GetAnalysisSpec()->GetPredictorSpec()->GetClassifierMultiLayerPerceptron()->GetMLPParameters()->GetShuffleIndexes();
//	GetAnalysisSpec()->GetPredictorSpec()->GetClassifierMultiLayerPerceptron()->GetMLPParameters()->SetShuffleIndexes(ivShuffleIndexes);
//
//	// On enleve le marquage pour la prochaine lecture de la base
//	// (on a memorise les indices des instances Train dans le vecteur ivShuffleIndexes)
//	//GetTrainDatabase()->GetMarkedInstances()->SetSize(0);
//
//	if(bStatsOk)
//	{
//		// Lancement de l'apprentissage
//		TrainPredictors(&classStats);
//	}
//
//	// Fin du suivi de la tache
//	TaskProgression::Stop();
//
//	ensure(not TaskProgression::IsStarted());
// }

// void CMLearningProblem::TrainPredictors(KWClassStats* classStats)
//{
//	//ObjectArray oaPredictors;
//	//ObjectArray oaUnivariatePredictors;
//	ObjectArray oaAllPredictors;
//	int i;
//	KWPredictor* predictor;
//	KWPredictorReport* predictorReport;
//	ObjectArray oaTrainedPredictorReports;
//	ALString sModelingReportName;
//
//	require(classStats != NULL);
//	require(classStats->IsStatsComputed());
//
//	// Recherche des predicteurs a utiliser
//	CollectPredictors(classStats, &oaAllPredictors);
//	//CollectUnivariatePredictors(classStats, &oaUnivariatePredictors);
//	//oaAllPredictors.ConcatFrom(&oaPredictors, &oaUnivariatePredictors);
//
//	// Apprentissage des predicteurs
//	TaskProgression::SetTitle("Train predictors");
//	for (i = 0; i < oaAllPredictors.GetSize(); i++)
//	{
//		predictor = cast(KWPredictor*, oaAllPredictors.GetAt(i));
//         TrainPredictor(classStats, predictor);
//		if (predictor->IsTrained())
//			oaTrainedPredictorReports.Add(predictor->GetPredictorReport());
//	}
//
//	// Ecriture du rapport de modelisation
//	if (analysisResults.GetModelingFileName() != "" and oaAllPredictors.GetSize() > 0)
//	{
//		if (oaTrainedPredictorReports.GetSize() == 0)
//			Global::AddWarning("", "", "Modeling report is not written since no predictor was trained");
//		else
//		{
//			sModelingReportName = BuildOutputFilePathName(analysisResults.GetModelingFileName(), "");
//			AddSimpleMessage("Write modeling report " + sModelingReportName);
//			predictorReport = cast(KWPredictorReport*, oaTrainedPredictorReports.GetAt(0));
//			predictorReport->WriteFullReportFile(sModelingReportName, &oaTrainedPredictorReports);
//		}
//	}
//
//	// Pas d'evaluation dans le cas non supervise
//	//if (classStats->GetTargetAttributeType() == KWType::None)
//	//	Global::AddWarning("", "", "No evaluation in case of unsupervised learning");
//	// Evaluation dans le cas supervise
//	//else
//	{
//		// Evaluation des predicteurs sur la base d'apprentissage
//		EvaluatePredictors(classStats, &oaAllPredictors,
//			GetTrainDatabase(), analysisResults.GetTrainEvaluationFileName(), false);
//
//		// Evaluation des predicteurs sur la base de test
//		EvaluatePredictors(classStats, &oaAllPredictors,
//			GetTestDatabase(), analysisResults.GetTestEvaluationFileName(), true);
//	}
//
//	// Nettoyage
//	oaAllPredictors.DeleteAll();
// }

void CMLearningProblem::CollectPredictors(KWClassStats* classStats, ObjectArray* oaPredictors)
{
	CMMajorityClassifier* predictorMajoritaire;
	CMModelingSpec* cmModelingSpec;

	require(classStats != NULL);
	require(classStats->IsStatsComputed());
	require(oaPredictors != NULL);
	require(oaPredictors->GetSize() == 0);

	// Dans tous les cas, on souhaite apprendre un reseau de neurones

	// Predicteur Connected Machine
	predictorMajoritaire =
	    cast(CMMajorityClassifier*, KWPredictor::ClonePredictor("Majority", classStats->GetTargetAttributeType()));

	if (predictorMajoritaire != NULL)
	{
		// Acces a la version specialisee des specification de modelisation
		cmModelingSpec = cast(CMModelingSpec*, analysisSpec->GetModelingSpec());

		// predictorConnectedMachine->CopyFrom(nnModelingSpec->GetClassifierMultiLayerPerceptron());

		oaPredictors->Add(predictorMajoritaire);
	}
	else
		AddWarning("Predicteur majoritaire " + KWType::GetPredictorLabel(classStats->GetTargetAttributeType()) +
			   " is not available");
}

////////////////////////////////////////////////////////////////////////
//// Classe CMAnalysisSpec
//
CMAnalysisSpec::CMAnalysisSpec()
{
	// Specialisation des specifications dde modelisation
	// en detruisant le sous-objet cree dans la classe ancetre et en le remplacant par une version dediee
	delete modelingSpec;
	modelingSpec = new CMModelingSpec;
}

CMAnalysisSpec::~CMAnalysisSpec() {}
