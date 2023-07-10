// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWLearningBenchmark.h"

KWLearningBenchmark::KWLearningBenchmark()
{
	nTargetAttributeType = KWType::Unknown;
	nCrossValidationNumber = 1;
	nFoldNumber = 10;
	bStratified = true;
	dSignificanceLevel = 0.05;
	sReportFileName = "benchmark.xls";
	bSyntheticReport = true;
	bExperimentReport = true;
	bRunReport = false;
	bExportBenchmarkDatabases = false;
}

KWLearningBenchmark::~KWLearningBenchmark()
{
	oaBenchmarkSpecs.DeleteAll();
	oaPredictorSpecs.DeleteAll();
	DeleteEvaluations();
	DeleteCriterions();
}

void KWLearningBenchmark::SetTargetAttributeType(int nValue)
{
	require(nTargetAttributeType == KWType::Unknown);
	require(nValue == KWType::Symbol or nValue == KWType::Continuous or nValue == KWType::None);
	nTargetAttributeType = nValue;
}

int KWLearningBenchmark::GetTargetAttributeType() const
{
	return nTargetAttributeType;
}

const ALString KWLearningBenchmark::GetPredictorFilter() const
{
	return "";
}

int KWLearningBenchmark::GetCrossValidationNumber() const
{
	return nCrossValidationNumber;
}

void KWLearningBenchmark::SetCrossValidationNumber(int nValue)
{
	require(nValue >= 0);
	nCrossValidationNumber = nValue;
}

int KWLearningBenchmark::GetFoldNumber() const
{
	return nFoldNumber;
}

void KWLearningBenchmark::SetFoldNumber(int nValue)
{
	require(nValue >= 2);
	nFoldNumber = nValue;
}

boolean KWLearningBenchmark::GetStratified() const
{
	return bStratified;
}

void KWLearningBenchmark::SetStratified(boolean bValue)
{
	bStratified = bValue;
}

void KWLearningBenchmark::SetSignificanceLevel(double dValue)
{
	require(0 <= dValue and dValue <= 1);

	dSignificanceLevel = dValue;
}

double KWLearningBenchmark::GetSignificanceLevel() const
{
	return dSignificanceLevel;
}

void KWLearningBenchmark::SetReportFileName(const ALString& sValue)
{
	sReportFileName = sValue;
}

const ALString& KWLearningBenchmark::GetReportFileName() const
{
	return sReportFileName;
}

void KWLearningBenchmark::SetRunReport(boolean bValue)
{
	bRunReport = bValue;
}

boolean KWLearningBenchmark::GetRunReport() const
{
	return bRunReport;
}

void KWLearningBenchmark::SetExportBenchmarkDatabases(boolean bValue)
{
	bExportBenchmarkDatabases = bValue;
}

boolean KWLearningBenchmark::GetExportBenchmarkDatabases() const
{
	return bExportBenchmarkDatabases;
}

ObjectArray* KWLearningBenchmark::GetBenchmarkSpecs()
{
	return &oaBenchmarkSpecs;
}

ObjectArray* KWLearningBenchmark::GetPredictorSpecs()
{
	return &oaPredictorSpecs;
}

void KWLearningBenchmark::Evaluate()
{
	KWClassDomain evaluationClassDomain;
	KWClassDomain* currentClassDomain;
	int nBenchmark;
	KWBenchmarkSpec* benchmarkSpec;
	int nPredictor;
	int nSeed;
	IntVector ivFoldIndexes;
	IntVector ivPhysicalRecordIndexes;
	int nValidation;
	int nFold;
	KWLearningSpec* learningSpec;
	IntVector ivSelectedInstances;
	ALString sTmp;

	require(Check());

	// Debut de suivi des taches
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Benchmark evaluation");

	// Mise en mode silencieux des erreur
	Global::SetSilentMode(true);

	// Initialisation du tableau des evaluation par criteres
	InitEvaluationResults();

	// Creation d'un nouveau domaine de classes pour les evaluations
	currentClassDomain = KWClassDomain::GetCurrentDomain();
	evaluationClassDomain.SetName("Evaluation");
	KWClassDomain::SetCurrentDomain(&evaluationClassDomain);

	// Evaluation par benchmark
	for (nBenchmark = 0; nBenchmark < GetBenchmarkSpecs()->GetSize(); nBenchmark++)
	{
		benchmarkSpec = cast(KWBenchmarkSpec*, GetBenchmarkSpecs()->GetAt(nBenchmark));

		// Fabrication des specifications d'apprentissage
		Global::SetSilentMode(false);
		benchmarkSpec->BuildLearningSpec();
		Global::SetSilentMode(true);

		// Exploitation si elles sont valides
		if (benchmarkSpec->IsLearningSpecValid())
		{
			learningSpec = benchmarkSpec->GetLearningSpec();

			// Calcul des statistiques sur le benchmark
			Global::SetSilentMode(false);
			benchmarkSpec->ComputeBenchmarkStats();
			Global::SetSilentMode(true);

			// Boucle de cross-validation
			for (nValidation = 0; nValidation < GetCrossValidationNumber(); nValidation++)
			{
				// Numerotation des parties auquelles seront affectees les instances
				// de la base
				// Cette etape etant couteuse, cette boucle est effectuee avant la
				// boucle sur les predictors pour n'etre effectuee qu'une seule fois
				nSeed = nValidation;
				benchmarkSpec->ComputeFoldIndexes(nSeed, GetFoldNumber(), GetStratified(),
								  &ivFoldIndexes);

				// Evaluation des predicteurs
				for (nPredictor = 0; nPredictor < GetPredictorSpecs()->GetSize(); nPredictor++)
				{
					// Boucle sur les parties de la cross-validation
					for (nFold = 0; nFold < GetFoldNumber(); nFold++)
					{
						// Export des base en apprentissage et test, si demande
						// Uniquement pour le premier predicteur pour ne pas repeter l'operation
						// inutilement
						if (GetExportBenchmarkDatabases() and nPredictor == 0)
						{
							ExportBenchmarkDatabase(benchmarkSpec, nValidation, nFold, true,
										&ivFoldIndexes);
							ExportBenchmarkDatabase(benchmarkSpec, nValidation, nFold,
										false, &ivFoldIndexes);
						}

						// Evaluation unitaire
						EvaluateExperiment(nBenchmark, nPredictor, nValidation, nFold,
								   &ivFoldIndexes);

						// Arret si interruption demandee
						if (TaskProgression::IsInterruptionRequested())
							break;
					}

					// Arret si interruption demandee
					if (TaskProgression::IsInterruptionRequested())
						break;
				}

				// Arret si interruption demandee
				if (TaskProgression::IsInterruptionRequested())
					break;
			}

			// Nettoyage des donnees de travail liees au benchmark
			benchmarkSpec->DeleteLearningSpec();

			// Ecriture d'un rapport intermdiaire
			sReportFileName = GetReportFileName();
			if (sReportFileName != "")
			{
				AddSimpleMessage(sTmp + "Write intermediate evaluation report (" +
						 IntToString(nBenchmark + 1) + "/" +
						 IntToString(GetBenchmarkSpecs()->GetSize()) + ") " + sReportFileName);
				WriteReportFile(sReportFileName);
			}

			// Arret si interruption demandee
			if (TaskProgression::IsInterruptionRequested())
				break;
		}

		// Nettoyage du domaine de classes
		evaluationClassDomain.DeleteAllClasses();

		// Arret si interruption demandee
		if (TaskProgression::IsInterruptionRequested())
			break;
	}

	// Destruction du domaine de classes d'evaluation
	KWClassDomain::SetCurrentDomain(currentClassDomain);
	evaluationClassDomain.DeleteAllClasses();

	// Restitution du mode d'affichage des erreurs
	Global::SetSilentMode(false);

	// Fin de suivi des taches
	TaskProgression::EndTask();
}

void KWLearningBenchmark::DeleteEvaluations()
{
	int nCriterion;
	ObjectArray* oaEvaluations;

	// Destruction des tableaux d'evaluation par critere
	for (nCriterion = 0; nCriterion < oaEvaluationArrays.GetSize(); nCriterion++)
	{
		oaEvaluations = cast(ObjectArray*, oaEvaluationArrays.GetAt(nCriterion));
		oaEvaluations->DeleteAll();
	}
	oaEvaluationArrays.DeleteAll();
}

int KWLearningBenchmark::GetCriterionNumber() const
{
	require(svCriterionNames.GetSize() == svCriterionLabels.GetSize());
	require(svCriterionNames.GetSize() == ivCriterionMaximizations.GetSize());

	return svCriterionNames.GetSize();
}

int KWLearningBenchmark::GetCriterionIndexAt(const ALString& sCriterionName)
{
	int nCriterion;

	// Recherche de l'index correspondant au nom du critere
	for (nCriterion = 0; nCriterion < GetCriterionNumber(); nCriterion++)
	{
		if (GetCriterionNameAt(nCriterion) == sCriterionName)
			return nCriterion;
	}

	// Si non trouve, on retourne -1
	return -1;
}

const ALString& KWLearningBenchmark::GetCriterionNameAt(int nCriterion)
{
	require(0 <= nCriterion and nCriterion < GetCriterionNumber());

	return svCriterionNames.GetAt(nCriterion);
}

const ALString& KWLearningBenchmark::GetCriterionLabelAt(int nCriterion)
{
	require(0 <= nCriterion and nCriterion < GetCriterionNumber());

	return svCriterionLabels.GetAt(nCriterion);
}

boolean KWLearningBenchmark::GetCriterionMaximizationAt(int nCriterion)
{
	require(0 <= nCriterion and nCriterion < GetCriterionNumber());

	return ivCriterionMaximizations.GetAt(nCriterion);
}

const ObjectArray* KWLearningBenchmark::GetAllEvaluationsAt(int nCriterion) const
{
	ObjectArray* oaEvaluations;

	require(0 <= nCriterion and nCriterion < GetCriterionNumber());

	oaEvaluations = cast(ObjectArray*, oaEvaluationArrays.GetAt(nCriterion));
	return oaEvaluations;
}

const KWStatisticalEvaluation* KWLearningBenchmark::GetEvaluationAt(int nCriterion, int nPredictor) const
{
	ObjectArray* oaEvaluations;

	require(0 <= nCriterion and nCriterion < GetCriterionNumber());

	oaEvaluations = cast(ObjectArray*, oaEvaluationArrays.GetAt(nCriterion));
	assert(0 <= nPredictor and nPredictor < oaEvaluations->GetSize());
	return cast(KWStatisticalEvaluation*, oaEvaluations->GetAt(nPredictor));
}

void KWLearningBenchmark::WriteReportFile(const ALString& sFileName) const
{
	fstream ost;
	boolean bOk;
	ALString sLocalFileName;

	// Preparation de la copie sur HDFS si necessaire
	bOk = PLRemoteFileService::BuildOutputWorkingFile(sFileName, sLocalFileName);
	if (bOk)
		bOk = FileService::OpenOutputFile(sLocalFileName, ost);
	if (bOk)
	{
		if (GetLearningReportHeaderLine() != "")
			ost << GetLearningReportHeaderLine() << "\n";
		WriteReport(ost);
		bOk = FileService::CloseOutputFile(sLocalFileName, ost);
	}

	if (bOk)
	{
		// Copie vers HDFS
		PLRemoteFileService::CleanOutputWorkingFile(sFileName, sLocalFileName);
	}
}

void KWLearningBenchmark::WriteReport(ostream& ost) const
{
	// Titre
	ost << "Predictor evaluation report"
	    << "\n\n\n";

	// Parametres du benchmark
	ost << "Cross validation\n";
	if (GetCrossValidationNumber() > 1)
		ost << GetCrossValidationNumber() << " * ";
	if (GetStratified())
		ost << "Stratified ";
	ost << GetFoldNumber() << " fold cross-validation\n";
	ost << "Test of significant differences at " << GetSignificanceLevel() << "%\n";

	// Rapport sur les benchmarks
	if (oaBenchmarkSpecs.GetSize() > 0)
	{
		ost << "\n\n";
		WriteBenchmarkReport(ost);
	}

	// Rapport sur les predictors
	if (oaPredictorSpecs.GetSize() > 0)
	{
		ost << "\n\n";
		WritePredictorReport(ost);
	}

	// Rapport synthetique d'evaluation
	if (oaEvaluationArrays.GetSize() > 0 and oaBenchmarkSpecs.GetSize() > 0 and oaPredictorSpecs.GetSize() > 0 and
	    nCrossValidationNumber > 0 and nFoldNumber > 0)
	{
		ost << "\n\n";
		WriteEvaluationReport(ost);
	}
}

boolean KWLearningBenchmark::Check() const
{
	boolean bOk = true;
	KWClassDomain evaluationClassDomain;
	KWClassDomain* currentClassDomain;
	int nBenchmark;
	KWBenchmarkSpec* benchmarkSpec;
	int nPredictor;
	KWPredictorSpec* predictorSpec;
	ALString sTmp;

	// Le type de predicteur doit etre initialise
	if (GetTargetAttributeType() != KWType::Symbol and GetTargetAttributeType() != KWType::Continuous and
	    GetTargetAttributeType() != KWType::None)
	{
		AddError("Prediction task not initialized");
		bOk = false;
	}

	// Controle des specifications de benchmark
	for (nBenchmark = 0; nBenchmark < oaBenchmarkSpecs.GetSize(); nBenchmark++)
	{
		benchmarkSpec = cast(KWBenchmarkSpec*, oaBenchmarkSpecs.GetAt(nBenchmark));

		// Validite "syntaxique" des specs
		if (not benchmarkSpec->Check())
			bOk = false;
		// Fabrication des specifications d'apprentissage pour tests pousses
		else
		{
			// Creation d'un nouveau domaine de classes pour les evaluations
			currentClassDomain = KWClassDomain::GetCurrentDomain();
			evaluationClassDomain.SetName("Evaluation");
			KWClassDomain::SetCurrentDomain(&evaluationClassDomain);

			// Construction des spec d'apprentissage
			benchmarkSpec->BuildLearningSpec();
			bOk = benchmarkSpec->IsLearningSpecValid();
			if (not bOk)
				benchmarkSpec->AddError("Wrong benchmark spec");
			else
			{
				// Calcul des statistiques du benchmark
				Global::SetSilentMode(true);
				benchmarkSpec->ComputeBenchmarkStats();
				Global::SetSilentMode(false);

				// Test du type de l'attribut cible
				if (benchmarkSpec->GetTargetAttributeType() != GetTargetAttributeType())
				{
					benchmarkSpec->AddError(
					    sTmp + "Type of target variable not consistent with a " +
					    KWType::GetPredictorLabel(GetTargetAttributeType()) + " task");
					bOk = false;
				}
			}

			// Nettoyage
			benchmarkSpec->DeleteLearningSpec();
			KWClassDomain::SetCurrentDomain(currentClassDomain);
			evaluationClassDomain.DeleteAllClasses();
		}
	}

	// Controle des specifications de predicteur
	for (nPredictor = 0; nPredictor < oaPredictorSpecs.GetSize(); nPredictor++)
	{
		predictorSpec = cast(KWPredictorSpec*, oaPredictorSpecs.GetAt(nPredictor));
		if (not predictorSpec->Check())
			bOk = false;

		// Test si le nom du predicteur est utilisable
		if (not KWPredictorSpec::IsPredictorUsable(predictorSpec->GetPredictorName(), GetPredictorFilter()))
		{
			predictorSpec->AddError("The predictor does not belong to the list of allowed predictors (" +
						GetPredictorFilter() + ")");
			bOk = false;
		}

		// Test si le predicteur est compatible avec la tache de prediction du benchmark
		if (predictorSpec->GetTargetAttributeType() != GetTargetAttributeType())
		{
			predictorSpec->AddError(sTmp + "Type of predictor not consistent with a " +
						KWType::GetPredictorLabel(GetTargetAttributeType()) + " task");
			bOk = false;
		}
	}

	return bOk;
}

const ALString KWLearningBenchmark::GetClassLabel() const
{
	return "Predictor benchmark";
}

const ALString KWLearningBenchmark::GetObjectLabel() const
{
	return "";
}

void KWLearningBenchmark::InitEvaluationResults()
{
	int nCriterion;
	int nPredictor;
	KWPredictorSpec* predictorSpec;
	ObjectArray* oaEvaluations;
	KWStatisticalEvaluation* firstEvaluation;
	KWStatisticalEvaluation* evaluation;

	require(Check());

	// Nettoyage
	DeleteEvaluations();
	DeleteCriterions();

	// Creation des criteres
	CreateCriterions();

	// Creation d'une premiere structure d'evaluation type
	firstEvaluation = CreateTemplateEvaluation();
	assert(firstEvaluation->GetSignificanceLevel() == GetSignificanceLevel());
	assert(firstEvaluation->GetExperimentNumber() >= GetBenchmarkSpecs()->GetSize());

	// Alimentation du tableau de toutes les evaluations par critere
	oaEvaluationArrays.SetSize(GetCriterionNumber());
	for (nCriterion = 0; nCriterion < GetCriterionNumber(); nCriterion++)
	{
		// Creation d'un tableau d'evaluation par methode
		oaEvaluations = new ObjectArray;
		oaEvaluations->SetSize(GetPredictorSpecs()->GetSize());
		oaEvaluationArrays.SetAt(nCriterion, oaEvaluations);

		// Creation des evaluations
		for (nPredictor = 0; nPredictor < GetPredictorSpecs()->GetSize(); nPredictor++)
		{
			predictorSpec = cast(KWPredictorSpec*, GetPredictorSpecs()->GetAt(nPredictor));

			// On clone l'evaluation type
			evaluation = firstEvaluation->Clone();
			oaEvaluations->SetAt(nPredictor, evaluation);

			// Parametrage de cette evaluation
			evaluation->SetCriterionName(GetCriterionLabelAt(nCriterion));
			evaluation->SetMaximization(GetCriterionMaximizationAt(nCriterion));
			evaluation->SetMethodName(predictorSpec->GetObjectLabel());

			// On peut nettoyer les libelles d'experimentation hormis pour la
			// premiere evaluation du tableau (afin de gagner de la place)
			if (nPredictor > 0)
				evaluation->InitializeAllLabels();
		}
	}

	// Destruction de l'evaluation type
	delete firstEvaluation;
}

KWStatisticalEvaluation* KWLearningBenchmark::CreateTemplateEvaluation() const
{
	KWStatisticalEvaluation* evaluation;
	int nBenchmark;
	KWBenchmarkSpec* benchmarkSpec;

	// Creation et parametrage
	evaluation = new KWStatisticalEvaluation;
	evaluation->SetExperimentNumber(oaBenchmarkSpecs.GetSize());
	evaluation->SetRunNumber(GetCrossValidationNumber() * GetFoldNumber());
	evaluation->SetSignificanceLevel(GetSignificanceLevel());

	// Parametrage des libelles des experiences
	evaluation->SetExperimentName("Dataset");
	for (nBenchmark = 0; nBenchmark < oaBenchmarkSpecs.GetSize(); nBenchmark++)
	{
		benchmarkSpec = cast(KWBenchmarkSpec*, oaBenchmarkSpecs.GetAt(nBenchmark));
		evaluation->SetExperimentLabelAt(nBenchmark, benchmarkSpec->GetClassName());
	}

	return evaluation;
}

KWClass* KWLearningBenchmark::BuildBenchmarkClass(KWBenchmarkSpec* benchmarkSpec) const
{
	KWClassDomain loadDomain;
	KWClassDomain* currentClassDomain;
	KWClass* kwcClass;

	require(benchmarkSpec != NULL);
	require(benchmarkSpec->Check());

	// On prepare un clone de la classe si celle-ci est deja calculee
	if (benchmarkSpec->IsLearningSpecValid())
		kwcClass = benchmarkSpec->GetLearningSpec()->GetClass()->Clone();
	// Sinon, on doit d'abord calculer les specifications d'apprentissage
	else
	{
		// Creation d'un nouveau domaine de classes pour le chargement
		currentClassDomain = KWClassDomain::GetCurrentDomain();
		loadDomain.SetName("LoadDomain");
		KWClassDomain::SetCurrentDomain(&loadDomain);

		// Fabrication des specifications d'apprentissage
		Global::SetSilentMode(false);
		benchmarkSpec->BuildLearningSpec();
		Global::SetSilentMode(true);

		// Exploitation si elles sont valides
		kwcClass = NULL;
		if (benchmarkSpec->IsLearningSpecValid())
			kwcClass = benchmarkSpec->GetLearningSpec()->GetClass()->Clone();

		// Destruction des specifications d'apprentissage
		benchmarkSpec->DeleteLearningSpec();

		// Destruction du domaine de classes d'evaluation
		loadDomain.DeleteAllClasses();
		KWClassDomain::SetCurrentDomain(currentClassDomain);
	}

	// Indexation de la classe
	if (kwcClass != NULL)
		kwcClass->IndexClass();
	return kwcClass;
}

void KWLearningBenchmark::CreateCriterions()
{
	require(GetCriterionNumber() == 0);

	// Ajout des criteres selon le type de predicteur
	if (GetTargetAttributeType() == KWType::Symbol)
		CreateClassifierCriterions();
	else if (GetTargetAttributeType() == KWType::Continuous)
		CreateRegressorCriterions();
	else if (GetTargetAttributeType() == KWType::Symbol)
		CreateClustererCriterions();

	// Ajout des criteres generiques
	AddCriterion("FeatureNumber", "Features", false);
	AddCriterion("ConstructedFeatureNumber", "Constr. att.", true);
	AddCriterion("InformativeFeatureNumber", "Inf. features", true);
	AddCriterion("UsedAttributeNumber", "Used attributes", false);
	AddCriterion("TotalComputingTime", "Total Time", false);
	AddCriterion("PreprocessingComputingTime", "Pre Time", false);
}

void KWLearningBenchmark::CreateClassifierCriterions()
{
	require(GetTargetAttributeType() == KWType::Symbol);
	require(GetCriterionNumber() == 0);

	AddCriterion("TrainAccuracy", "Train acc", true);
	AddCriterion("TestAccuracy", "Test acc", true);
	AddCriterion("RatioAccuracy", "Ratio acc", true);
	AddCriterion("TrainBalancedAccuracy", "Train Bacc", true);
	AddCriterion("TestBalancedAccuracy", "Test Bacc", true);
	AddCriterion("RatioBalancedAccuracy", "Ratio Bacc", true);
	AddCriterion("TrainAUC", "Train AUC", true);
	AddCriterion("TestAUC", "Test AUC", true);
	AddCriterion("RatioAUC", "Ratio AUC", true);
	AddCriterion("TrainCompressionRate", "Train CompR", true);
	AddCriterion("TestCompressionRate", "Test CompR", true);
	AddCriterion("RatioCompressionRate", "Ratio CompR", true);
}

void KWLearningBenchmark::CreateRegressorCriterions()
{
	require(GetTargetAttributeType() == KWType::Continuous);
	require(GetCriterionNumber() == 0);

	AddCriterion("TrainRMSE", "Train RMSE", false);
	AddCriterion("TestRMSE", "Test RMSE", false);
	AddCriterion("RatioRMSE", "Ratio RMSE", true);
	AddCriterion("TrainMAE", "Train MAE", false);
	AddCriterion("TestMAE", "Test MAE", false);
	AddCriterion("RatioMAE", "Ratio MAE", true);
	AddCriterion("TrainNLPD", "Train NLPD", false);
	AddCriterion("TestNLPD", "Test NLPD", false);
	AddCriterion("RatioNLPD", "Ratio NLPD", true);
	AddCriterion("TrainRankRMSE", "Train RankRMSE", false);
	AddCriterion("TestRankRMSE", "Test RankRMSE", false);
	AddCriterion("RatioRankRMSE", "Ratio RankRMSE", true);
	AddCriterion("TrainRankMAE", "Train RankMAE", false);
	AddCriterion("TestRankMAE", "Test RankMAE", false);
	AddCriterion("RatioRankMAE", "Ratio RankMAE", true);
	AddCriterion("TrainRankNLPD", "Train RankNLPD", false);
	AddCriterion("TestRankNLPD", "Test RankNLPD", false);
	AddCriterion("RatioRankNLPD", "Ratio RankNLPD", true);
}

void KWLearningBenchmark::CreateClustererCriterions()
{
	require(GetTargetAttributeType() == KWType::None);
	require(GetCriterionNumber() == 0);
}

void KWLearningBenchmark::AddCriterion(const ALString& sCriterionName, const ALString& sCriterionLabel,
				       boolean bCriterionMaximization)
{
	require(sCriterionName != "");
	require(sCriterionLabel != "");
	require(GetCriterionIndexAt(sCriterionName) == -1);
	require(svCriterionNames.GetSize() == svCriterionLabels.GetSize());
	require(svCriterionNames.GetSize() == ivCriterionMaximizations.GetSize());

	// Ajout des parametres du critere dans les tableaux
	svCriterionNames.Add(sCriterionName);
	svCriterionLabels.Add(sCriterionLabel);
	ivCriterionMaximizations.Add(bCriterionMaximization);
}

void KWLearningBenchmark::DeleteCriterions()
{
	svCriterionNames.SetSize(0);
	svCriterionLabels.SetSize(0);
	ivCriterionMaximizations.SetSize(0);
}

void KWLearningBenchmark::WriteBenchmarkReport(ostream& ost) const
{
	int nBenchmark;
	KWBenchmarkSpec* benchmarkSpec;

	// Rapport sur les benchmarks
	for (nBenchmark = 0; nBenchmark < oaBenchmarkSpecs.GetSize(); nBenchmark++)
	{
		benchmarkSpec = cast(KWBenchmarkSpec*, oaBenchmarkSpecs.GetAt(nBenchmark));

		// Ligne de titre pour la premiere ligne
		if (nBenchmark == 0)
		{
			ost << "\n";
			benchmarkSpec->WriteHeaderLineReport(ost);
			ost << "\n";
		}

		// Ligne de donnees
		benchmarkSpec->WriteLineReport(ost);
		ost << "\n";
	}
}

void KWLearningBenchmark::WritePredictorReport(ostream& ost) const
{
	int nPredictor;
	KWPredictorSpec* predictorSpec;
	KWPreprocessingSpec* preprocessingSpec;
	KWAttributeConstructionSpec* attributeConstructionSpec;

	// Rapport sur les predictors
	for (nPredictor = 0; nPredictor < oaPredictorSpecs.GetSize(); nPredictor++)
	{
		predictorSpec = cast(KWPredictorSpec*, oaPredictorSpecs.GetAt(nPredictor));

		// Acces au parametrage
		preprocessingSpec = predictorSpec->GetPreprocessingSpec();
		attributeConstructionSpec = predictorSpec->GetAttributeConstructionSpec();

		// Ligne de titre pour la premiere ligne
		if (nPredictor == 0)
		{
			ost << "\n";
			ost << "Predictor\t";
			ost << "Label\t";
			attributeConstructionSpec->WriteHeaderLineReport(ost);
			preprocessingSpec->WriteHeaderLineReport(ost);
			ost << "\n";
		}

		// Ligne de donnees
		ost << predictorSpec->GetPredictorName() << "\t";
		ost << predictorSpec->GetObjectLabel() << "\t";
		attributeConstructionSpec->WriteLineReport(ost);
		preprocessingSpec->WriteLineReport(GetTargetAttributeType(), ost);
		ost << "\n";
	}
}

void KWLearningBenchmark::WriteEvaluationReport(ostream& ost) const
{
	int nCriterion;
	ObjectArray* oaEvaluations;

	// Ecriture des rapport synthetiques
	if (bSyntheticReport)
	{
		for (nCriterion = 0; nCriterion < oaEvaluationArrays.GetSize(); nCriterion++)
		{
			oaEvaluations = cast(ObjectArray*, oaEvaluationArrays.GetAt(nCriterion));

			// Rapport par critere
			if (nCriterion > 0)
				ost << "\n";
			KWStatisticalEvaluation::WriteComparativeReport(ost, oaEvaluations);
		}
	}

	// Ecriture des rapports detailles par experimentation
	if (bExperimentReport)
	{
		for (nCriterion = 0; nCriterion < oaEvaluationArrays.GetSize(); nCriterion++)
		{
			oaEvaluations = cast(ObjectArray*, oaEvaluationArrays.GetAt(nCriterion));

			// Titre general pour les rapport detailles
			if (nCriterion == 0)
				ost << "\n\nExperiment details\n\n";

			// Rapport par critere
			if (nCriterion > 0)
				ost << "\n";
			KWStatisticalEvaluation::WriteExperimentComparativeReport(ost, oaEvaluations);
		}
	}

	// Ecriture des rapports detailles par experimentation et par run
	if (bRunReport)
	{
		for (nCriterion = 0; nCriterion < oaEvaluationArrays.GetSize(); nCriterion++)
		{
			oaEvaluations = cast(ObjectArray*, oaEvaluationArrays.GetAt(nCriterion));

			// Titre general pour les rapport detailles
			if (nCriterion == 0)
				ost << "\n\nRun details\n\n";

			// Rapport par critere
			if (nCriterion > 0)
				ost << "\n";
			KWStatisticalEvaluation::WriteValueComparativeReport(ost, oaEvaluations);
		}
	}
}

void KWLearningBenchmark::EvaluateExperiment(int nBenchmark, int nPredictor, int nValidation, int nFold,
					     IntVector* ivFoldIndexes)
{
	KWStatisticalEvaluation* totalComputingTimeEvaluation;
	KWStatisticalEvaluation* preprocessingComputingTimeEvaluation;
	KWBenchmarkSpec* benchmarkSpec;
	KWPredictorSpec* predictorSpec;
	KWLearningSpec* learningSpec;
	KWPredictor* predictor;
	KWPredictorEvaluation* predictorEvaluation;
	KWClass* constructedClass;
	ObjectDictionary odMultiTableConstructedAttributes;
	ObjectDictionary odTextConstructedAttributes;
	KWClassDomain* initialDomain;
	KWClassStats* classStats;
	int nRun;
	ALString sMainLabel;
	int nTotalExperimentNumber;
	int nExperimentIndex;
	double dTotalComputingTime;
	double dPreprocessingComputingTime;
	clock_t tBegin;
	clock_t tPreprocessingEnd;
	clock_t tTotalEnd;

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
	predictor = predictorSpec->GetPredictor();

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
	// Apprentissage

	// Parametrage des specifications d'apprentissage par le
	// preprocessing du predicteur
	TaskProgression::DisplayLabel("Train");
	learningSpec->GetPreprocessingSpec()->CopyFrom(predictorSpec->GetPreprocessingSpec());

	// Parametrage des instances a garder en apprentissage
	benchmarkSpec->ComputeDatabaseSelectedInstance(ivFoldIndexes, nFold, true);

	// Creation d'un objet de calcul des stats
	classStats = new KWClassStats;

	// Parametrage du nombre initial d'attributs
	learningSpec->SetInitialAttributeNumber(
	    learningSpec->GetClass()->ComputeInitialAttributeNumber(GetTargetAttributeType() != KWType::None));

	// Construction d'une classe avec de nouvelles variables si necessaire
	initialDomain = KWClassDomain::GetCurrentDomain();
	constructedClass =
	    BuildLearningSpecConstructedClass(learningSpec, predictorSpec, classStats->GetMultiTableConstructionSpec(),
					      classStats->GetTextConstructionSpec());

	// On prend le domaine de construction comme domaine de travail
	if (constructedClass != NULL)
	{
		KWClassDomain::SetCurrentDomain(constructedClass->GetDomain());
		learningSpec->SetClass(constructedClass);
	}
	// Sinon, on change quand meme de domaine de travail (pour la creation potentielle de nouvelles variables)
	else
	{
		KWClassDomain::SetCurrentDomain(KWClassDomain::GetCurrentDomain()->Clone());
		KWClassDomain::GetCurrentDomain()->Compile();
		learningSpec->SetClass(
		    KWClassDomain::GetCurrentDomain()->LookupClass(learningSpec->GetClass()->GetName()));
	}
	assert(learningSpec->Check());

	// Prise en compte des specification de construction d'arbres
	if (KDDataPreparationAttributeCreationTask::GetGlobalCreationTask())
	{
		// On recopie les specifications de creation d'attributs
		KDDataPreparationAttributeCreationTask::GetGlobalCreationTask()->CopyAttributeCreationSpecFrom(
		    predictorSpec->GetAttributeConstructionSpec()->GetAttributeCreationParameters());

		// On recopie le nombre d'attributs a construire, qui est specifie dans au niveau au dessus
		KDDataPreparationAttributeCreationTask::GetGlobalCreationTask()->SetMaxCreatedAttributeNumber(
		    predictorSpec->GetAttributeConstructionSpec()->GetMaxTreeNumber());
	}

	// Prise en compte des paires de variables demandees par le predicteur
	predictorSpec->GetAttributeConstructionSpec()->GetAttributePairsSpec()->SetClassName(
	    learningSpec->GetClass()->GetName());
	classStats->SetAttributePairsSpec(predictorSpec->GetAttributeConstructionSpec()->GetAttributePairsSpec());

	// Calcul des stats descriptives
	tBegin = clock();
	classStats->SetLearningSpec(learningSpec);
	classStats->ComputeStats();
	tPreprocessingEnd = clock();
	dPreprocessingComputingTime = (double)(tPreprocessingEnd - tBegin) / CLOCKS_PER_SEC;

	// Apprentissage
	if (classStats->IsStatsComputed())
	{
		// Debut de suivi de la tache d'apprentissage
		TaskProgression::BeginTask();
		TaskProgression::DisplayMainLabel(predictorSpec->GetObjectLabel());

		// Apprentissage
		predictor->SetLearningSpec(learningSpec);
		predictor->SetClassStats(classStats);
		predictor->Train();

		// Nettoyage de la classe du predicteur, en gardant tous les attributs de prediction
		if (predictor->IsTrained())
			predictor->GetTrainedPredictor()->CleanPredictorClass(initialDomain);

		// Fin de suivi de tache
		TaskProgression::EndTask();
	}
	tTotalEnd = clock();
	dTotalComputingTime = (double)(tTotalEnd - tBegin) / CLOCKS_PER_SEC;

	///////////////////////////////////////////////////////////////////
	// Collecte des resultats

	// Collecte si l'apprentissage a reussi
	if (classStats->IsStatsComputed() and predictor->IsTrained())
	{
		TaskProgression::DisplayLabel("Evaluation");

		// Index de l'experience
		nRun = nValidation * GetFoldNumber() + nFold;

		// Mise a jour des resultats d'evaluation sur tous les criteres en apprentissage
		if (not TaskProgression::IsInterruptionRequested())
		{
			assert(learningSpec->GetDatabase()->GetObjects()->GetSize() == 0);
			predictorEvaluation = predictor->Evaluate(learningSpec->GetDatabase());
			CollectAllResults(true, nBenchmark, nPredictor, nBenchmark, nRun, predictor,
					  predictorEvaluation);
			delete predictorEvaluation;
			assert(learningSpec->GetDatabase()->GetObjects()->GetSize() == 0);

			// Memorisation des temps de calcul
			totalComputingTimeEvaluation =
			    GetUpdatableEvaluationAt(GetCriterionIndexAt("TotalComputingTime"), nPredictor);
			preprocessingComputingTimeEvaluation =
			    GetUpdatableEvaluationAt(GetCriterionIndexAt("PreprocessingComputingTime"), nPredictor);
			totalComputingTimeEvaluation->SetResultAt(nBenchmark, nRun, dTotalComputingTime);
			preprocessingComputingTimeEvaluation->SetResultAt(nBenchmark, nRun,
									  dPreprocessingComputingTime);
		}

		// Mise a jour des resultats d'evaluation sur tous les criteres en test
		if (not TaskProgression::IsInterruptionRequested())
		{
			// Parametrage des instances a garder en test
			benchmarkSpec->ComputeDatabaseSelectedInstance(ivFoldIndexes, nFold, false);

			// Mise a jour des resultats
			assert(learningSpec->GetDatabase()->GetObjects()->GetSize() == 0);
			predictorEvaluation = predictor->Evaluate(learningSpec->GetDatabase());
			CollectAllResults(false, nBenchmark, nPredictor, nBenchmark, nRun, predictor,
					  predictorEvaluation);
			delete predictorEvaluation;
			assert(learningSpec->GetDatabase()->GetObjects()->GetSize() == 0);
		}
	}

	// Restitution du domaine initial
	if (initialDomain != KWClassDomain::GetCurrentDomain())
	{
		learningSpec->SetClass(initialDomain->LookupClass(learningSpec->GetClass()->GetName()));
		delete KWClassDomain::GetCurrentDomain();
		KWClassDomain::SetCurrentDomain(initialDomain);
	}

	// Nettoyage
	predictorSpec->GetPredictor()->SetClassStats(NULL);
	predictorSpec->GetPredictor()->SetLearningSpec(NULL);
	delete classStats;
}

KWClass* KWLearningBenchmark::BuildLearningSpecConstructedClass(KWLearningSpec* learningSpec,
								KWPredictorSpec* predictorSpec,
								ObjectDictionary* odMultiTableConstructedAttributes,
								ObjectDictionary* odTextConstructedAttributes)
{
	boolean bOk = true;
	KDMultiTableFeatureConstruction multiTableFeatureConstruction;
	KDTextFeatureConstruction textFeatureConstruction;
	boolean bIsTextConstructionPossible;
	KWClassDomain* constructedDomain;
	KWClass* constructedClass;

	require(learningSpec != NULL);
	require(learningSpec->GetInitialAttributeNumber() ==
		learningSpec->GetClass()->ComputeInitialAttributeNumber(learningSpec->GetTargetAttributeName() != ""));
	require(predictorSpec != NULL);
	require(odMultiTableConstructedAttributes != NULL);
	require(odMultiTableConstructedAttributes->GetCount() == 0);
	require(odTextConstructedAttributes != NULL);
	require(odTextConstructedAttributes->GetCount() == 0);

	//////////////////////////////////////////////////////////////////////////////////////
	// Le code suivant est largement inspire de  KWLearningProblem::BuildConstructedClass

	// Initialisation du domaine de connaissance
	multiTableFeatureConstruction.SetLearningSpec(learningSpec);
	multiTableFeatureConstruction.SetConstructionDomain(
	    predictorSpec->GetAttributeConstructionSpec()->GetConstructionDomain());
	multiTableFeatureConstruction.SetMaxRuleNumber(
	    predictorSpec->GetAttributeConstructionSpec()->GetMaxConstructedAttributeNumber());

	// Initialisation de la construction de variables a base de textes
	textFeatureConstruction.SetLearningSpec(learningSpec);
	textFeatureConstruction.SetConstructionDomain(
	    predictorSpec->GetAttributeConstructionSpec()->GetConstructionDomain());

	// Detection si des variable de type texte peuvent etre construites
	bIsTextConstructionPossible = textFeatureConstruction.ContainsTextAttributes(learningSpec->GetClass());

	// Parametrage des familles de construction de variables
	predictorSpec->GetAttributeConstructionSpec()->SpecifyLearningSpecConstructionFamilies(
	    learningSpec, true, bIsTextConstructionPossible);

	// Calcul de la nouvelle classe si demande
	constructedDomain = NULL;
	constructedClass = NULL;
	if (predictorSpec->GetAttributeConstructionSpec()->GetMaxConstructedAttributeNumber() > 0)
	{
		// Construction de variable
		multiTableFeatureConstruction.ComputeStats();
		bOk = multiTableFeatureConstruction.IsStatsComputed();
		if (bOk)
		{
			// Acces a la classe construite si la construction est effective
			if (multiTableFeatureConstruction.IsClassConstructed())
			{
				constructedClass = multiTableFeatureConstruction.GetConstructedClass();
				constructedDomain = constructedClass->GetDomain();
				multiTableFeatureConstruction.RemoveConstructedClass();

				// Restitution des couts initiaux si aucune variable n'a ete effectivement construite
				assert(constructedClass->GetAttributeNumber() >=
				       learningSpec->GetClass()->GetAttributeNumber());
				if (constructedClass->GetAttributeNumber() ==
				    learningSpec->GetClass()->GetAttributeNumber())
				{
					predictorSpec->GetAttributeConstructionSpec()
					    ->SpecifyLearningSpecConstructionFamilies(learningSpec, false,
										      bIsTextConstructionPossible);
					multiTableFeatureConstruction.ComputeInitialAttributeCosts(constructedClass);
				}

				// Collecte des attributs construits en multi-tables
				multiTableFeatureConstruction.CollectConstructedAttributes(
				    learningSpec->GetClass(), constructedClass, odMultiTableConstructedAttributes);
			}
			// Mise a jour des familles de construction de variable sinon
			else
				predictorSpec->GetAttributeConstructionSpec()->SpecifyLearningSpecConstructionFamilies(
				    learningSpec, false, bIsTextConstructionPossible);
		}
	}

	// Construction de variable en mode texte si necessaire
	if (bOk)
	{
		if (predictorSpec->GetAttributeConstructionSpec()->GetMaxTextFeatureNumber() > 0 and
		    bIsTextConstructionPossible)
		{
			// Duplication de la classe initiale si pas de construction demandee ou pas de classe
			// constructible
			if (bOk and constructedDomain == NULL)
			{
				assert(constructedDomain == NULL);
				constructedDomain =
				    learningSpec->GetClass()->GetDomain()->CloneFromClass(learningSpec->GetClass());
				constructedDomain->Compile();
				constructedClass = constructedDomain->LookupClass(learningSpec->GetClass()->GetName());

				// Calcul des couts de variable de facon standard
				multiTableFeatureConstruction.ComputeInitialAttributeCosts(constructedClass);
			}

			// Construction de variables de type texte
			textFeatureConstruction.ConstructTextFeatures(
			    constructedClass, predictorSpec->GetAttributeConstructionSpec()->GetMaxTextFeatureNumber(),
			    odTextConstructedAttributes);
		}
	}

	// Calcul uniquement des couts de construction des variables natives si pas de classe construite
	if (constructedClass == NULL)
		multiTableFeatureConstruction.ComputeInitialAttributeCosts(learningSpec->GetClass());
	return constructedClass;
}

void KWLearningBenchmark::CollectAllResults(boolean bTrain, int nBenchmark, int nPredictor, int nExperiment, int nRun,
					    KWPredictor* trainedPredictor, KWPredictorEvaluation* predictorEvaluation)
{
	KWStatisticalEvaluation* featureNumberEvaluation;
	KWStatisticalEvaluation* constructedFeatureNumberEvaluation;
	KWStatisticalEvaluation* informativeFeatureNumberEvaluation;
	KWStatisticalEvaluation* usedAttributeNumberEvaluation;

	require(0 <= nBenchmark and nBenchmark < GetBenchmarkSpecs()->GetSize());
	require(0 <= nPredictor and nPredictor < GetPredictorSpecs()->GetSize());
	require(trainedPredictor != NULL);
	require(trainedPredictor->IsTrained());
	require(predictorEvaluation != NULL);
	require(predictorEvaluation->GetTargetType() == GetTargetAttributeType());

	// Collecte des criteres
	featureNumberEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("FeatureNumber"), nPredictor);
	constructedFeatureNumberEvaluation =
	    GetUpdatableEvaluationAt(GetCriterionIndexAt("ConstructedFeatureNumber"), nPredictor);
	informativeFeatureNumberEvaluation =
	    GetUpdatableEvaluationAt(GetCriterionIndexAt("InformativeFeatureNumber"), nPredictor);
	usedAttributeNumberEvaluation =
	    GetUpdatableEvaluationAt(GetCriterionIndexAt("UsedAttributeNumber"), nPredictor);

	// Memorisation des nombres de variables construites, informatives ou non
	if (trainedPredictor->GetClassStats() != NULL)
	{
		featureNumberEvaluation->SetResultAt(nExperiment, nRun,
						     trainedPredictor->GetClassStats()->GetAttributeStats()->GetSize());
		constructedFeatureNumberEvaluation->SetResultAt(
		    nExperiment, nRun, trainedPredictor->GetClassStats()->GetConstructedAttributeNumber());
		informativeFeatureNumberEvaluation->SetResultAt(
		    nExperiment, nRun, trainedPredictor->GetClassStats()->GetInformativeAttributeNumber());
	}
	else
	{
		featureNumberEvaluation->SetResultAt(nExperiment, nRun, 0);
		informativeFeatureNumberEvaluation->SetResultAt(nExperiment, nRun, 0);
	}

	// Memorisation du nombre d'attributs utilises
	usedAttributeNumberEvaluation->SetResultAt(nExperiment, nRun,
						   trainedPredictor->GetPredictorReport()->GetUsedAttributeNumber());

	// Dispatching de l'evaluation
	if (GetTargetAttributeType() == KWType::Symbol)
		CollectAllClassifierResults(bTrain, nBenchmark, nPredictor, nExperiment, nRun, trainedPredictor,
					    predictorEvaluation);
	else if (GetTargetAttributeType() == KWType::Continuous)
		CollectAllRegressorResults(bTrain, nBenchmark, nPredictor, nExperiment, nRun, trainedPredictor,
					   predictorEvaluation);
	else if (GetTargetAttributeType() == KWType::None)
		CollectAllClustererResults(bTrain, nBenchmark, nPredictor, nExperiment, nRun, trainedPredictor,
					   predictorEvaluation);
}

void KWLearningBenchmark::CollectAllClassifierResults(boolean bTrain, int nBenchmark, int nPredictor, int nExperiment,
						      int nRun, KWPredictor* trainedPredictor,
						      KWPredictorEvaluation* predictorEvaluation)
{
	KWClassifierEvaluation* classifierEvaluation;
	KWStatisticalEvaluation* trainAccuracyEvaluation;
	KWStatisticalEvaluation* trainBalancedAccuracyEvaluation;
	KWStatisticalEvaluation* trainAUCEvaluation;
	KWStatisticalEvaluation* trainCompressionRateEvaluation;
	KWStatisticalEvaluation* testAccuracyEvaluation;
	KWStatisticalEvaluation* testBalancedAccuracyEvaluation;
	KWStatisticalEvaluation* testAUCEvaluation;
	KWStatisticalEvaluation* testCompressionRateEvaluation;
	KWStatisticalEvaluation* ratioAccuracyEvaluation;
	KWStatisticalEvaluation* ratioBalancedAccuracyEvaluation;
	KWStatisticalEvaluation* ratioAUCEvaluation;
	KWStatisticalEvaluation* ratioCompressionRateEvaluation;

	require(GetTargetAttributeType() == KWType::Symbol);

	// Acces a l'evaluation specialisee
	classifierEvaluation = cast(KWClassifierEvaluation*, predictorEvaluation);

	// Acces aux evaluations des criteres en train
	trainAccuracyEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("TrainAccuracy"), nPredictor);
	trainBalancedAccuracyEvaluation =
	    GetUpdatableEvaluationAt(GetCriterionIndexAt("TrainBalancedAccuracy"), nPredictor);
	trainAUCEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("TrainAUC"), nPredictor);
	trainCompressionRateEvaluation =
	    GetUpdatableEvaluationAt(GetCriterionIndexAt("TrainCompressionRate"), nPredictor);

	// Acces aux evaluations des criteres en test
	testAccuracyEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("TestAccuracy"), nPredictor);
	testBalancedAccuracyEvaluation =
	    GetUpdatableEvaluationAt(GetCriterionIndexAt("TestBalancedAccuracy"), nPredictor);
	testAUCEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("TestAUC"), nPredictor);
	testCompressionRateEvaluation =
	    GetUpdatableEvaluationAt(GetCriterionIndexAt("TestCompressionRate"), nPredictor);

	// Acces aux criteres de type ratio test/train
	ratioAccuracyEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("RatioAccuracy"), nPredictor);
	ratioBalancedAccuracyEvaluation =
	    GetUpdatableEvaluationAt(GetCriterionIndexAt("RatioBalancedAccuracy"), nPredictor);
	ratioAUCEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("RatioAUC"), nPredictor);
	ratioCompressionRateEvaluation =
	    GetUpdatableEvaluationAt(GetCriterionIndexAt("RatioCompressionRate"), nPredictor);

	// Memorisation des resultats d'evaluation en train
	if (bTrain)
	{
		trainAccuracyEvaluation->SetResultAt(nExperiment, nRun, classifierEvaluation->GetAccuracy());
		trainBalancedAccuracyEvaluation->SetResultAt(nExperiment, nRun,
							     classifierEvaluation->GetBalancedAccuracy());
		trainAUCEvaluation->SetResultAt(nExperiment, nRun, classifierEvaluation->GetAUC());
		trainCompressionRateEvaluation->SetResultAt(nExperiment, nRun,
							    classifierEvaluation->GetCompressionRate());
	}

	// Memorisation des resultats d'evaluation en test
	if (not bTrain)
	{
		testAccuracyEvaluation->SetResultAt(nExperiment, nRun, classifierEvaluation->GetAccuracy());
		testBalancedAccuracyEvaluation->SetResultAt(nExperiment, nRun,
							    classifierEvaluation->GetBalancedAccuracy());
		testAUCEvaluation->SetResultAt(nExperiment, nRun, classifierEvaluation->GetAUC());
		testCompressionRateEvaluation->SetResultAt(nExperiment, nRun,
							   classifierEvaluation->GetCompressionRate());
	}

	// Memorisation des criteres de type ratio test/train
	if (not bTrain)
	{
		if (trainAccuracyEvaluation->GetResultAt(nExperiment, nRun) != 0)
			ratioAccuracyEvaluation->SetResultAt(
			    nExperiment, nRun,
			    testAccuracyEvaluation->GetResultAt(nExperiment, nRun) /
				trainAccuracyEvaluation->GetResultAt(nExperiment, nRun));
		if (trainBalancedAccuracyEvaluation->GetResultAt(nExperiment, nRun) != 0)
			ratioBalancedAccuracyEvaluation->SetResultAt(
			    nExperiment, nRun,
			    testBalancedAccuracyEvaluation->GetResultAt(nExperiment, nRun) /
				trainBalancedAccuracyEvaluation->GetResultAt(nExperiment, nRun));
		if (trainAUCEvaluation->GetResultAt(nExperiment, nRun) != 0)
			ratioAUCEvaluation->SetResultAt(nExperiment, nRun,
							testAUCEvaluation->GetResultAt(nExperiment, nRun) /
							    trainAUCEvaluation->GetResultAt(nExperiment, nRun));
		if (trainCompressionRateEvaluation->GetResultAt(nExperiment, nRun) != 0)
			ratioCompressionRateEvaluation->SetResultAt(
			    nExperiment, nRun,
			    testCompressionRateEvaluation->GetResultAt(nExperiment, nRun) /
				trainCompressionRateEvaluation->GetResultAt(nExperiment, nRun));
	}
}

void KWLearningBenchmark::CollectAllRegressorResults(boolean bTrain, int nBenchmark, int nPredictor, int nExperiment,
						     int nRun, KWPredictor* trainedPredictor,
						     KWPredictorEvaluation* predictorEvaluation)
{
	KWRegressorEvaluation* regressorEvaluation;
	KWStatisticalEvaluation* trainRMSEEvaluation;
	KWStatisticalEvaluation* testRMSEEvaluation;
	KWStatisticalEvaluation* ratioRMSEEvaluation;
	KWStatisticalEvaluation* trainMAEEvaluation;
	KWStatisticalEvaluation* testMAEEvaluation;
	KWStatisticalEvaluation* ratioMAEEvaluation;
	KWStatisticalEvaluation* trainNLPDEvaluation;
	KWStatisticalEvaluation* testNLPDEvaluation;
	KWStatisticalEvaluation* ratioNLPDEvaluation;
	KWStatisticalEvaluation* trainRankRMSEEvaluation;
	KWStatisticalEvaluation* testRankRMSEEvaluation;
	KWStatisticalEvaluation* ratioRankRMSEEvaluation;
	KWStatisticalEvaluation* trainRankMAEEvaluation;
	KWStatisticalEvaluation* testRankMAEEvaluation;
	KWStatisticalEvaluation* ratioRankMAEEvaluation;
	KWStatisticalEvaluation* trainRankNLPDEvaluation;
	KWStatisticalEvaluation* testRankNLPDEvaluation;
	KWStatisticalEvaluation* ratioRankNLPDEvaluation;

	require(GetTargetAttributeType() == KWType::Continuous);

	// Acces a l'evaluation specialisee
	regressorEvaluation = cast(KWRegressorEvaluation*, predictorEvaluation);

	// Acces aux evaluations des criteres en train
	trainRMSEEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("TrainRMSE"), nPredictor);
	trainMAEEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("TrainMAE"), nPredictor);
	trainNLPDEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("TrainNLPD"), nPredictor);
	trainRankRMSEEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("TrainRankRMSE"), nPredictor);
	trainRankMAEEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("TrainRankMAE"), nPredictor);
	trainRankNLPDEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("TrainRankNLPD"), nPredictor);

	// Acces aux evaluations des criteres en test
	testRMSEEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("TestRMSE"), nPredictor);
	testMAEEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("TestMAE"), nPredictor);
	testNLPDEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("TestNLPD"), nPredictor);
	testRankRMSEEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("TestRankRMSE"), nPredictor);
	testRankMAEEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("TestRankMAE"), nPredictor);
	testRankNLPDEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("TestRankNLPD"), nPredictor);

	// Acces aux criteres de type ratio test/train
	ratioRMSEEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("RatioRMSE"), nPredictor);
	ratioMAEEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("RatioMAE"), nPredictor);
	ratioNLPDEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("RatioNLPD"), nPredictor);
	ratioRankRMSEEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("RatioRankRMSE"), nPredictor);
	ratioRankMAEEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("RatioRankMAE"), nPredictor);
	ratioRankNLPDEvaluation = GetUpdatableEvaluationAt(GetCriterionIndexAt("RatioRankNLPD"), nPredictor);

	// Memorisation des resultats d'evaluation en train
	if (bTrain)
	{
		trainRMSEEvaluation->SetResultAt(nExperiment, nRun, regressorEvaluation->GetRMSE());
		trainMAEEvaluation->SetResultAt(nExperiment, nRun, regressorEvaluation->GetMAE());
		trainNLPDEvaluation->SetResultAt(nExperiment, nRun, regressorEvaluation->GetNLPD());
		trainRankRMSEEvaluation->SetResultAt(nExperiment, nRun, regressorEvaluation->GetRankRMSE());
		trainRankMAEEvaluation->SetResultAt(nExperiment, nRun, regressorEvaluation->GetRankMAE());
		trainRankNLPDEvaluation->SetResultAt(nExperiment, nRun, regressorEvaluation->GetRankNLPD());
	}

	// Memorisation des resultats d'evaluation en test
	if (not bTrain)
	{
		testRMSEEvaluation->SetResultAt(nExperiment, nRun, regressorEvaluation->GetRMSE());
		testMAEEvaluation->SetResultAt(nExperiment, nRun, regressorEvaluation->GetMAE());
		testNLPDEvaluation->SetResultAt(nExperiment, nRun, regressorEvaluation->GetNLPD());
		testRankRMSEEvaluation->SetResultAt(nExperiment, nRun, regressorEvaluation->GetRankRMSE());
		testRankMAEEvaluation->SetResultAt(nExperiment, nRun, regressorEvaluation->GetRankMAE());
		testRankNLPDEvaluation->SetResultAt(nExperiment, nRun, regressorEvaluation->GetRankNLPD());
	}

	// Memorisation des criteres de type ratio test/train
	if (not bTrain)
	{
		if (trainRMSEEvaluation->GetResultAt(nExperiment, nRun) != 0)
			ratioRMSEEvaluation->SetResultAt(nExperiment, nRun,
							 testRMSEEvaluation->GetResultAt(nExperiment, nRun) /
							     trainRMSEEvaluation->GetResultAt(nExperiment, nRun));
		if (trainMAEEvaluation->GetResultAt(nExperiment, nRun) != 0)
			ratioMAEEvaluation->SetResultAt(nExperiment, nRun,
							testMAEEvaluation->GetResultAt(nExperiment, nRun) /
							    trainMAEEvaluation->GetResultAt(nExperiment, nRun));
		if (trainNLPDEvaluation->GetResultAt(nExperiment, nRun) != 0)
			ratioNLPDEvaluation->SetResultAt(nExperiment, nRun,
							 testNLPDEvaluation->GetResultAt(nExperiment, nRun) /
							     trainNLPDEvaluation->GetResultAt(nExperiment, nRun));
		if (trainRankRMSEEvaluation->GetResultAt(nExperiment, nRun) != 0)
			ratioRankRMSEEvaluation->SetResultAt(
			    nExperiment, nRun,
			    testRankRMSEEvaluation->GetResultAt(nExperiment, nRun) /
				trainRankRMSEEvaluation->GetResultAt(nExperiment, nRun));
		if (trainRankMAEEvaluation->GetResultAt(nExperiment, nRun) != 0)
			ratioRankMAEEvaluation->SetResultAt(nExperiment, nRun,
							    testRankMAEEvaluation->GetResultAt(nExperiment, nRun) /
								trainRankMAEEvaluation->GetResultAt(nExperiment, nRun));
		if (trainRankNLPDEvaluation->GetResultAt(nExperiment, nRun) != 0)
			ratioRankNLPDEvaluation->SetResultAt(
			    nExperiment, nRun,
			    testRankNLPDEvaluation->GetResultAt(nExperiment, nRun) /
				trainRankNLPDEvaluation->GetResultAt(nExperiment, nRun));
	}
}

void KWLearningBenchmark::CollectAllClustererResults(boolean bTrain, int nBenchmark, int nPredictor, int nExperiment,
						     int nRun, KWPredictor* trainedPredictor,
						     KWPredictorEvaluation* predictorEvaluation)
{
	require(GetTargetAttributeType() == KWType::None);
}

KWStatisticalEvaluation* KWLearningBenchmark::GetUpdatableEvaluationAt(int nCriterion, int nPredictor)
{
	ObjectArray* oaEvaluations;

	require(0 <= nCriterion and nCriterion < GetCriterionNumber());

	oaEvaluations = cast(ObjectArray*, oaEvaluationArrays.GetAt(nCriterion));
	assert(0 <= nPredictor and nPredictor < oaEvaluations->GetSize());
	return cast(KWStatisticalEvaluation*, oaEvaluations->GetAt(nPredictor));
}

void KWLearningBenchmark::ExportBenchmarkDatabase(KWBenchmarkSpec* benchmarkSpec, int nValidation, int nFold,
						  boolean bTrain, IntVector* ivFoldIndexes)
{
	KWLearningSpec* learningSpec;
	KWDatabase* exportedDatabase;
	ALString sExportSuffix;
	boolean bCurrentSilentMode;
	KWClassDomain* currentDomain;
	KWClassDomain* exportDomain;
	int i;
	KWClass* kwcClass;
	KWAttribute* attribute;

	require(benchmarkSpec != NULL);
	require(benchmarkSpec->Check());
	require(benchmarkSpec->IsLearningSpecValid());
	require(0 <= nValidation and nValidation < GetCrossValidationNumber());
	require(0 <= nFold and nFold < GetFoldNumber());
	require(ivFoldIndexes != NULL);

	// Probleme d'apprentissage
	learningSpec = benchmarkSpec->GetLearningSpec();

	// Parametrage des instances a garder en apprentissage/test
	benchmarkSpec->ComputeDatabaseSelectedInstance(ivFoldIndexes, nFold, bTrain);

	// Construction d'un dictionnaire pour l'export
	currentDomain = KWClassDomain::GetCurrentDomain();
	exportDomain = KWClassDomain::GetCurrentDomain()->CloneFromClass(learningSpec->GetClass());
	KWClassDomain::SetCurrentDomain(exportDomain);

	// On parametre les attributs pour n'exporter que les attributs natifs
	for (i = 0; i < exportDomain->GetClassNumber(); i++)
	{
		kwcClass = exportDomain->GetClassAt(i);
		attribute = kwcClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			attribute->SetUsed(attribute->GetAnyDerivationRule() == NULL and
					   KWType::IsData(attribute->GetType()));
			attribute->SetLoaded(attribute->GetUsed());
			kwcClass->GetNextAttribute(attribute);
		}
	}
	exportDomain->Compile();

	// Fabrication d'un suffixe d'export
	if (bTrain)
		sExportSuffix += "_Train";
	else
		sExportSuffix += "_Test";
	sExportSuffix += "_V";
	sExportSuffix += IntToString(nValidation + 1);
	sExportSuffix += "_F";
	sExportSuffix += IntToString(nFold + 1);

	// Parametrage de la base a exporter
	exportedDatabase = learningSpec->GetDatabase()->Clone();
	exportedDatabase->AddSuffixToUsedFiles(sExportSuffix);

	// Message d'export
	bCurrentSilentMode = Global::GetSilentMode();
	Global::SetSilentMode(false);
	AddMessage("Export database " + exportedDatabase->GetDatabaseName());
	Global::SetSilentMode(bCurrentSilentMode);

	// Export de la base
	learningSpec->GetDatabase()->ReadAll();
	exportedDatabase->WriteAll(learningSpec->GetDatabase());
	learningSpec->GetDatabase()->DeleteAll();
	delete exportedDatabase;

	// Nettoyage
	KWClassDomain::SetCurrentDomain(currentDomain);
	delete exportDomain;
}
