// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorEvaluationTask.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWPredictorEvaluationTask

KWPredictorEvaluationTask::KWPredictorEvaluationTask()
{
	predictorEvaluation = NULL;
}

KWPredictorEvaluationTask::~KWPredictorEvaluationTask() {}

boolean KWPredictorEvaluationTask::Evaluate(KWPredictor* predictor, KWDatabase* database,
					    KWPredictorEvaluation* requesterPredictorEvaluation)
{
	boolean bOk;

	require(predictor != NULL);
	require(database != NULL);
	require(requesterPredictorEvaluation != NULL);
	require(predictor->IsTrained());
	require(KWType::IsPredictorType(predictor->GetTargetAttributeType()));
	require(database->GetObjects()->GetSize() == 0);

	// Intialiazation des variables necessaires pour l'evaluation
	predictorEvaluation = requesterPredictorEvaluation;
	InitializePredictorSharedVariables(predictor);

	// On ne souhaite que les messages de fin de tache en cas d'arret
	SetDisplaySpecificTaskMessage(false);
	SetDisplayTaskTime(false);
	SetDisplayEndTaskMessage(true);

	// Lancement de la tache
	SetReusableDatabaseIndexer(predictor->GetLearningSpec()->GetDatabaseIndexer());
	bOk = RunDatabaseTask(database);

	// Verification de l'alimentation et nettoyage des variables partagees
	assert(Check());
	CleanPredictorSharedVariables();

	return bOk;
}

const ALString KWPredictorEvaluationTask::GetClassLabel() const
{
	return "Evaluation";
}

const ALString KWPredictorEvaluationTask::GetObjectLabel() const
{
	require(predictorEvaluation != NULL);
	return predictorEvaluation->GetPredictorName();
}

const ALString KWPredictorEvaluationTask::GetTaskName() const
{
	return "Predictor evaluation";
}

PLParallelTask* KWPredictorEvaluationTask::Create() const
{
	return new KWPredictorEvaluationTask;
}

void KWPredictorEvaluationTask::InitializePredictorSharedVariables(KWPredictor* predictor)
{
	// Les sous-classes ont besoin d'une implementation
}

void KWPredictorEvaluationTask::CleanPredictorSharedVariables()
{
	// Les sous-classes ont besoin d'une implementation
}

boolean KWPredictorEvaluationTask::MasterAggregateResults()
{
	boolean bOk;

	// Appel a la methode ancetre
	bOk = KWDatabaseTask::MasterAggregateResults();

	predictorEvaluation->lInstanceEvaluationNumber += output_lReadObjects;

	return bOk;
}

boolean KWPredictorEvaluationTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	// Appel a la methode ancetre
	bOk = KWDatabaseTask::MasterFinalize(bProcessEndedCorrectly);

	// Warning si la base est vide
	if (bOk and predictorEvaluation->GetEvaluationInstanceNumber() == 0)
		AddWarning("Empty evaluation database");
	return bOk;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWClassifierEvaluationTask

KWClassifierEvaluationTask::KWClassifierEvaluationTask()
{
	// Initialisation des variables du maitre
	classifierEvaluation = NULL;
	masterConfMatrixEvaluation = NULL;
	masterAucEvaluation = NULL;
	masterInstanceEvaluationSampler = NULL;
	dMasterSamplingProb = -1.0;
	nCurrentSample = -1;
	bIsAucEvaluated = false;

	// Initialisation des variables de l'esclave
	slaveInstanceEvaluationSampler = NULL;
	slaveConfusionMatrixEvaluation = NULL;

	// Initialisation des variables partagees
	DeclareTaskInput(&input_bIsAucEvaluated);
	DeclareSharedParameter(&shared_liTargetAttribute);
	DeclareSharedParameter(&shared_liPredictionAttribute);
	DeclareSharedParameter(&shared_livProbAttributes);
	DeclareSharedParameter(&shared_nTargetValueNumber);
	DeclareSharedParameter(&shared_svPredictedModalities);
	DeclareSharedParameter(&shared_nSlaveInstanceEvaluationCapacity);
	DeclareTaskOutput(&output_confusionMatrix);
	DeclareTaskOutput(&output_dCompressionRate);
	output_oaInstanceEvaluationSample = new PLShared_ObjectArray(new PLShared_ClassifierInstanceEvaluation);
	DeclareTaskOutput(output_oaInstanceEvaluationSample);
	DeclareTaskOutput(&output_lSeenInstanceEvaluationNumber);
}

KWClassifierEvaluationTask::~KWClassifierEvaluationTask()
{
	assert(masterConfMatrixEvaluation == NULL);
	assert(slaveConfusionMatrixEvaluation == NULL);
	assert(masterAucEvaluation == NULL);
	assert(masterInstanceEvaluationSampler == NULL);
	assert(oaAllInstanceEvaluationSamples.GetSize() == 0);

	// Nettoyage du tableau de sortie des esclaves (force a etre en reference)
	delete output_oaInstanceEvaluationSample;
}

const ALString KWClassifierEvaluationTask::GetTaskName() const
{
	return "Classifier evaluation";
}

PLParallelTask* KWClassifierEvaluationTask::Create() const
{
	return new KWClassifierEvaluationTask;
}

boolean KWClassifierEvaluationTask::ComputeResourceRequirements()
{
	boolean bOk;
	longint lEstimatedTotalObjectNumber;
	longint lMaxRequiredEvaluationMemory;
	longint lRemainingMemory;
	longint lMaxMasterMemoryRequirement;
	longint lMaxSlaveMemoryRequirement;

	// Appel a la methode ancetre et sauvegarde ses exigences en memoire
	bOk = KWPredictorEvaluationTask::ComputeResourceRequirements();
	databaseTaskMasterMemoryRequirement.CopyFrom(GetResourceRequirements()->GetMasterRequirement()->GetMemory());
	databaseTaskSlaveMemoryRequirement.CopyFrom(GetResourceRequirements()->GetSlaveRequirement()->GetMemory());

	// Estimation du nombre total d'instances de la base en tenant compte du taux d'achantillonnage
	lEstimatedTotalObjectNumber =
	    shared_sourceDatabase.GetPLDatabase()->GetDatabase()->GetSampleEstimatedObjectNumber();

	// Estimation de la memoire totale necessaire pour le calcul de toutes les courbes de lift
	lMaxRequiredEvaluationMemory =
	    lEstimatedTotalObjectNumber * ComputeInstanceEvaluationNecessaryMemory(shared_nTargetValueNumber) + lMB / 2;

	// Estimation de la memoire totale restante pour en donner au max la moitie au maitre
	lRemainingMemory = RMResourceManager::GetRemainingAvailableMemory();

	// On diminue potentiellement la memoire max du maitre, en tenant compte de la memoire disponible restante
	lMaxMasterMemoryRequirement = min({2 * lGB, 32 * lMB + lRemainingMemory / 2, 2 * lMaxRequiredEvaluationMemory});

	// On estime la memoire max du esclave avec un facteur 4
	lMaxSlaveMemoryRequirement = lMaxMasterMemoryRequirement / 4;

	// Memoire Maitre: on demande un min et un max "raisonnable" permettant d'avoir une bonne
	// estimation des courbes de lift, du critere d'AUC, et dans le pire cas un temps de calcul
	// raisonnable. Ce ne serait pas raisonnable de trier des grands vecteurs de score pour gagner une
	// precision negligeable par rapport a l'erreur statistique du calcul.
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->UpgradeMin(
	    min(32 * lMB, lMaxRequiredEvaluationMemory));
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->UpgradeMax(lMaxMasterMemoryRequirement);
	assert(GetResourceRequirements()->GetMasterRequirement()->GetMemory()->Check());

	// Memoire esclave: meme logique que pour le maitre, et un peu moins pour le max
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMin(
	    min(8 * lMB, lMaxSlaveMemoryRequirement));
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMax(lMaxSlaveMemoryRequirement);
	assert(GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->Check());

	// On demande un distribution balancee de la memoire entre maitre et esclaves
	GetResourceRequirements()->SetMemoryAllocationPolicy(RMTaskResourceRequirement::balanced);

	return bOk;
}

boolean KWClassifierEvaluationTask::MasterInitialize()
{
	boolean bLocalTrace = false;
	boolean bOk;
	int nTargetValue;
	longint lMasterGrantedMemory;
	longint lMasterSelfMemory;
	longint lSlaveGrantedMemory;
	longint lSlaveSelfMemory;
	ALString sTmp;

	require(masterConfMatrixEvaluation == NULL);
	require(slaveConfusionMatrixEvaluation == NULL);
	require(masterAucEvaluation == NULL);
	require(masterInstanceEvaluationSampler == NULL);

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::MasterInitialize();

	// Memorisation de la specialisation du rapport d'evaluation demandeur
	classifierEvaluation = cast(KWClassifierEvaluation*, predictorEvaluation);
	assert(classifierEvaluation->oaAllLiftCurveValues.GetSize() == 0);

	// Initialisation du service d'evaluation de la matrice de confusion
	masterConfMatrixEvaluation = new KWConfusionMatrixEvaluation;
	masterConfMatrixEvaluation->Initialize();
	for (nTargetValue = 0; nTargetValue < shared_nTargetValueNumber; nTargetValue++)
		masterConfMatrixEvaluation->AddPredictedTarget(shared_svPredictedModalities.GetAt(nTargetValue));

	// Initialisation des courbes de lift pour l'ensemble des modalites
	for (nTargetValue = 0; nTargetValue < shared_nTargetValueNumber; nTargetValue++)
	{
		// Arret et warning si le maximum de courbes est atteint
		if (nTargetValue == nMaxLiftEvaluationNumber)
		{
			AddWarning(sTmp + "The lift curves will be computed only for " +
				   IntToString(nMaxLiftEvaluationNumber) + " values (among " +
				   IntToString(shared_nTargetValueNumber) + ")");
			break;
		}
		classifierEvaluation->oaAllLiftCurveValues.Add(new DoubleVector);
	}

	// Initialisation du compteur des echantillons d'evaluation des esclaves pour le calcul d'AUC et courbes de lift
	nCurrentSample = 0;

	// Initialisation du service de calcul de l'AUC
	bIsAucEvaluated = shared_livProbAttributes.GetSize() > 0 and shared_nTargetValueNumber > 0;
	masterAucEvaluation = new KWAucEvaluation;
	masterAucEvaluation->SetTargetValueNumber(shared_nTargetValueNumber);

	// Initialisation de l'echantillonneur du maitre et son proba
	// Etat du RNG a 2^60 pour etre dans une plage differente des esclaves
	dMasterSamplingProb = 1.0;
	masterInstanceEvaluationSampler = new KWReservoirSampler;
	masterInstanceEvaluationSampler->SetSamplerRandomSeed(1ll << 60);

	// Dimensionnement de la capacite de l'echantillonneur du maitre et du esclave
	lMasterGrantedMemory = GetMasterResourceGrant()->GetMemory();
	lMasterSelfMemory =
	    ComputeTaskSelfMemory(lMasterGrantedMemory, GetResourceRequirements()->GetMasterRequirement()->GetMemory(),
				  &databaseTaskMasterMemoryRequirement);
	masterInstanceEvaluationSampler->SetCapacity(
	    ComputeSamplerCapacity(lMasterGrantedMemory, shared_nTargetValueNumber));

	// Dimensionnement de la capacite des echantillonneurs des esclaves
	// En parallele: La capacite de l'echantillonneur est celle de memoire propre de cette sous-classe
	//               C'est-a-dire en decomptant la memoire utilise par PLDatabaseTask
	if (GetTaskResourceGrant()->GetSlaveNumber() > 1)
	{
		lSlaveGrantedMemory = GetTaskResourceGrant()->GetSlaveMemory();
		lSlaveSelfMemory = ComputeTaskSelfMemory(lSlaveGrantedMemory,
							 GetResourceRequirements()->GetSlaveRequirement()->GetMemory(),
							 &databaseTaskSlaveMemoryRequirement);
		shared_nSlaveInstanceEvaluationCapacity =
		    ComputeSamplerCapacity(lSlaveGrantedMemory, shared_nTargetValueNumber);
	}
	// En sequentiel : La capacite est la meme du maitre parce qu'ils partangent le meme espace de memoire
	else
		shared_nSlaveInstanceEvaluationCapacity = masterInstanceEvaluationSampler->GetCapacity();

	// Trace de deboggage
	if (bLocalTrace)
	{
		cout << "Master Initialized\n";
		cout << "master      mem = " << LongintToHumanReadableString(lMasterGrantedMemory) << "\n";
		cout << "master capacity = " << masterInstanceEvaluationSampler->GetCapacity() << "\n";
		cout << "slave       mem = " << LongintToHumanReadableString(GetTaskResourceGrant()->GetSlaveMemory())
		     << "\n";
		cout << "slave capacity  =  " << shared_nSlaveInstanceEvaluationCapacity << "\n";
	}

	ensure(Check());
	return bOk;
}

boolean KWClassifierEvaluationTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	boolean bOk;

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::MasterPrepareTaskInput(dTaskPercent, bIsTaskFinished);

	// Ajout d'une reference nulle pour l'echantillon d'evaluation de l'esclave en cours
	oaAllInstanceEvaluationSamples.Add(NULL);
	lvSeenInstanceEvaluationNumberPerSample.Add(-1);
	input_bIsAucEvaluated = bIsAucEvaluated;
	return bOk;
}

boolean KWClassifierEvaluationTask::MasterAggregateResults()
{
	boolean bOk;
	ObjectArray* oaInstanceEvaluationSample;

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::MasterAggregateResults();

	// Ajout des evaluations du esclave au celui du maitre
	masterConfMatrixEvaluation->AddEvaluatedMatrix(output_confusionMatrix.GetDataGridStats());
	classifierEvaluation->dCompressionRate += output_dCompressionRate;

	// Cas de l'evaluation du AUC
	if (bIsAucEvaluated)
	{
		// On transfert les instances d'evaluation de l'esclave en cours au tableau des echantillons
		// Memoire: Responsabilite transferee au tableau d'achantillons du maitre
		assert(oaAllInstanceEvaluationSamples.GetAt(GetTaskIndex()) == NULL);
		assert(lvSeenInstanceEvaluationNumberPerSample.GetAt(GetTaskIndex()) == -1);
		oaInstanceEvaluationSample = new ObjectArray;
		oaInstanceEvaluationSample->CopyFrom(output_oaInstanceEvaluationSample->GetObjectArray());
		oaAllInstanceEvaluationSamples.SetAt(GetTaskIndex(), oaInstanceEvaluationSample);
		lvSeenInstanceEvaluationNumberPerSample.SetAt(GetTaskIndex(), output_lSeenInstanceEvaluationNumber);
		output_oaInstanceEvaluationSample->GetObjectArray()->RemoveAll();

		// Echantillonnage d'instances d'evaluation du tableau d'echantillons au sampler du maitre
		// Ceci se fait en respectant l'ordre ou les esclaves ont ete expedies
		// Memoire: Responsabilite transferee vers masterInstanceSampler
		while (nCurrentSample < oaAllInstanceEvaluationSamples.GetSize())
		{
			// Si le prochain echantillon a traiter est NULL => il n'est pas pret => on pause le
			// echantillonnage
			if (oaAllInstanceEvaluationSamples.GetAt(nCurrentSample) == NULL)
				break;
			// Sinon on met a jour l'echantillon
			else
			{
				MasterAggregateResultsUpdateSampler();
				nCurrentSample++;
			}
		}
	}

	return bOk;
}

void KWClassifierEvaluationTask::MasterAggregateResultsUpdateSampler()
{
	ObjectArray* oaCurrentInstanceEvaluationSample;
	longint lCurrentSampleSeenObjectNumber;
	int nMasterSampleSize;
	double dSlaveSamplingProb;
	double dMasterResamplingProb;
	int nExpectedMasterResampleSize;

	require(oaAllInstanceEvaluationSamples.GetAt(nCurrentSample) != NULL);
	require(lvSeenInstanceEvaluationNumberPerSample.GetAt(nCurrentSample) >= 0);
	require(0 <= dMasterSamplingProb and dMasterSamplingProb <= 1.0);

	// Acces a l'echantillon courant, le nombre d'objets vus et la taille de l'echantillon du maitre
	oaCurrentInstanceEvaluationSample = cast(ObjectArray*, oaAllInstanceEvaluationSamples.GetAt(nCurrentSample));
	lCurrentSampleSeenObjectNumber = lvSeenInstanceEvaluationNumberPerSample.GetAt(nCurrentSample);
	nMasterSampleSize = masterInstanceEvaluationSampler->GetSampleSize();

	// Calcul de la proba d'echantillonage de l'echantillon a traiter
	if ((longint)oaCurrentInstanceEvaluationSample->GetSize() == lCurrentSampleSeenObjectNumber)
		dSlaveSamplingProb = 1.0;
	else
		dSlaveSamplingProb =
		    (1.0 * oaCurrentInstanceEvaluationSample->GetSize()) / lCurrentSampleSeenObjectNumber;

	// Cas simple de l'echantillon vide:
	//  - Mise a jour la proba d'echantillonage du maitre si celle du esclave est plus petite
	//  - Ajout l'echantillon complet
	if (nMasterSampleSize == 0)
	{
		assert(dMasterSamplingProb == 1);
		if (dSlaveSamplingProb < dMasterSamplingProb)
			dMasterSamplingProb = dSlaveSamplingProb;
		masterInstanceEvaluationSampler->AddArray(oaCurrentInstanceEvaluationSample);
	}
	// Cas general
	else
	{
		// Si la proba d'echantilonnage du esclave est plus petite que celle du maitre :
		//  - Mise-a-jour de diminution de la proba du maitre pour l'ajuster a celle de l'esclave
		// Voir details ci-dessous
		if (dSlaveSamplingProb < dMasterSamplingProb)
		{
			// Calcul de la proba de reechantillonnage et de la taille esperee du reechantillon
			// Le reechantillonage avec proba dSlaveSamplingProb / dMasterSamplingProb est necessaire pour
			// diminuer la proba d'echantillonage du maitre de dMasterSamplingProb a dSlaveSamplingProb
			dMasterResamplingProb = dSlaveSamplingProb / dMasterSamplingProb;
			nExpectedMasterResampleSize = int(ceil((dMasterResamplingProb * nMasterSampleSize)));

			// Si le reechantillonnage diminue la taille esperee de l'echantillon, reechantillonnage de
			// reservoir
			if (nExpectedMasterResampleSize < nMasterSampleSize)
				masterInstanceEvaluationSampler->Resample(dMasterResamplingProb);

			// Mise a jour de la proba du maitre
			dMasterSamplingProb = dSlaveSamplingProb;

			// Ajout complet de l'echantillon du esclave, ce qui ne pose pas de probleme puisque
			// le maitre et l'esclave ont des echantillons a la meme proba
			masterInstanceEvaluationSampler->AddArray(oaCurrentInstanceEvaluationSample);
		}
		// Si la proba d'echantillonage du esclave est plus grande que celle du maitre :
		//   - Ajout avec une proba de reechantillonage dMasterSamplingProb / dSlaveSamplingProb.
		// Cette probabilite de reechantillonage permet de preserver proba d'echantillonage du maitre
		// lors de l'ajout de l'echantillon courant.
		// NB: Le cas le plus courant rentre ici (ie. dMasterSamplingProb == dSlaveSamplingProb == 1.0)
		else
		{
			dMasterResamplingProb = dMasterSamplingProb / dSlaveSamplingProb;
			masterInstanceEvaluationSampler->AddArrayWithProb(oaCurrentInstanceEvaluationSample,
									  dMasterResamplingProb);
		}
	}
	// Nettoyage
	oaCurrentInstanceEvaluationSample->RemoveAll();

	ensure(0 <= dMasterSamplingProb and dMasterSamplingProb <= 1.0);
	ensure(oaCurrentInstanceEvaluationSample->GetSize() == 0);
}

boolean KWClassifierEvaluationTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	const int nPartileNumber = 1000;
	boolean bOk;
	int nLiftCurve;
	int nPredictorTarget;
	DoubleVector* dvLiftCurveValues;
	int nSample;
	longint lSeenInstanceEvaluationNumber;
	ALString sTmp;

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::MasterFinalize(bProcessEndedCorrectly);

	// Calcul des criteres si ok
	if (bOk)
	{
		// Memorisation de la matrice de confusion
		assert(masterConfMatrixEvaluation->Check());
		masterConfMatrixEvaluation->ExportDataGridStats(&classifierEvaluation->dgsConfusionMatrix);
		classifierEvaluation->dgsConfusionMatrix.ExportAttributePartFrequenciesAt(
		    1, &classifierEvaluation->ivActualModalityFrequencies);

		// Calcul et memorisation des taux de prediction
		classifierEvaluation->dAccuracy = masterConfMatrixEvaluation->ComputeAccuracy();
		classifierEvaluation->dBalancedAccuracy = masterConfMatrixEvaluation->ComputeBalancedAccuracy();
		classifierEvaluation->dMajorityAccuracy = masterConfMatrixEvaluation->ComputeMajorityAccuracy();
		classifierEvaluation->dTargetEntropy = masterConfMatrixEvaluation->ComputeTargetEntropy();

		// Calcul et memorisation du taux de compression
		if (shared_livProbAttributes.GetSize() > 0 and classifierEvaluation->lInstanceEvaluationNumber > 0)
		{
			// Normalisation par rapport a l'entropie cible
			if (classifierEvaluation->dTargetEntropy > 0)
			{
				classifierEvaluation->dCompressionRate =
				    1.0 - classifierEvaluation->dCompressionRate /
					      (classifierEvaluation->lInstanceEvaluationNumber *
					       classifierEvaluation->dTargetEntropy);
			}
			else
				classifierEvaluation->dCompressionRate = 0;

			// On arrondit si necessaire a 0
			if (fabs(classifierEvaluation->dCompressionRate) <
			    classifierEvaluation->dTargetEntropy / classifierEvaluation->lInstanceEvaluationNumber)
			{
				classifierEvaluation->dCompressionRate = 0;
			}
		}

		// Calcul de l'AUC s'il y y a des instances en evaluation
		if (bIsAucEvaluated and masterInstanceEvaluationSampler->GetSampledObjects()->GetSize() > 0)
		{
			masterAucEvaluation->SetInstanceEvaluations(
			    masterInstanceEvaluationSampler->GetSampledObjects());
			if (shared_livProbAttributes.GetSize() > 0 and masterAucEvaluation->GetTargetValueNumber() > 0)
				classifierEvaluation->dAUC = masterAucEvaluation->ComputeGlobalAUCValue();

			// Calcul des courbes de lift
			for (nLiftCurve = 0; nLiftCurve < classifierEvaluation->oaAllLiftCurveValues.GetSize();
			     nLiftCurve++)
			{
				dvLiftCurveValues =
				    cast(DoubleVector*, classifierEvaluation->oaAllLiftCurveValues.GetAt(nLiftCurve));

				// L'index de lift de la modalite est celui de la modalite directement, sauf si la
				// derniere courbe memorise la courbe pour la modalite cible principale
				nPredictorTarget = GetPredictorTargetIndexAtLiftCurveIndex(nLiftCurve);

				// Si l'on a avant la modalite cible est au debut
				masterAucEvaluation->ComputeLiftCurveAt(nPredictorTarget, nPartileNumber,
									dvLiftCurveValues);
			}
		}
	}

	// Warning si du sampling a ete utilise
	if (bOk)
	{
		lSeenInstanceEvaluationNumber = 0;
		for (nSample = 0; nSample < lvSeenInstanceEvaluationNumberPerSample.GetSize(); nSample++)
			lSeenInstanceEvaluationNumber += lvSeenInstanceEvaluationNumberPerSample.GetAt(nSample);

		if ((longint)masterInstanceEvaluationSampler->GetSampleSize() < lSeenInstanceEvaluationNumber)
		{
			AddWarning(
			    sTmp +
			    "Not enough memory to compute the exact AUC: estimation made on a sub-sample of size " +
			    IntToString(masterInstanceEvaluationSampler->GetSampleSize()) + " taken from " +
			    LongintToString(lSeenInstanceEvaluationNumber));
		}
	}

	// Nettoyage
	classifierEvaluation = NULL;
	delete masterConfMatrixEvaluation;
	masterConfMatrixEvaluation = NULL;
	delete masterAucEvaluation;
	masterAucEvaluation = NULL;
	delete masterInstanceEvaluationSampler;
	masterInstanceEvaluationSampler = NULL;
	dMasterSamplingProb = -1.0;
	nCurrentSample = -1;
	oaAllInstanceEvaluationSamples.DeleteAll();

	return bOk;
}

boolean KWClassifierEvaluationTask::SlaveInitialize()
{
	boolean bOk;

	require(slaveConfusionMatrixEvaluation == NULL);
	require(slaveInstanceEvaluationSampler == NULL);

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::SlaveInitialize();

	// Initialisation des objets de travail de l'esclave
	slaveInstanceEvaluationSampler = new KWReservoirSampler;
	slaveConfusionMatrixEvaluation = new KWConfusionMatrixEvaluation;

	// Dimensionnement de la capacite du vecteur de score de l'esclave
	slaveInstanceEvaluationSampler->SetCapacity(shared_nSlaveInstanceEvaluationCapacity);

	return bOk;
}

boolean KWClassifierEvaluationTask::SlaveProcessExploitDatabase()
{
	boolean bOk;
	int nInstanceEvaluation;
	KWClassifierInstanceEvaluation* instanceEvaluation;

	require(slaveInstanceEvaluationSampler != NULL);
	require(slaveInstanceEvaluationSampler->GetSampleSize() == 0);

	// Initialisation des resultats de l'esclave
	output_dCompressionRate = 0;
	slaveConfusionMatrixEvaluation->Initialize();

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::SlaveProcessExploitDatabase();

	// Remplisement des sorties de l'esclave
	slaveConfusionMatrixEvaluation->ExportDataGridStats(output_confusionMatrix.GetDataGridStats());
	for (nInstanceEvaluation = 0; nInstanceEvaluation < slaveInstanceEvaluationSampler->GetSampleSize();
	     nInstanceEvaluation++)
	{
		instanceEvaluation = cast(KWClassifierInstanceEvaluation*,
					  slaveInstanceEvaluationSampler->GetSampledObjectAt(nInstanceEvaluation));
		output_oaInstanceEvaluationSample->GetObjectArray()->Add(instanceEvaluation);
	}
	output_lSeenInstanceEvaluationNumber = slaveInstanceEvaluationSampler->GetSeenObjectNumber();

	// Remise a zero de l'echantillonneur
	slaveInstanceEvaluationSampler->RemoveAll();

	return bOk;
}

boolean KWClassifierEvaluationTask::SlaveProcessExploitDatabaseObject(const KWObject* kwoObject)
{
	const Continuous cEpsilon = (Continuous)1e-6;
	boolean bOk;
	int nActualValue;
	int nTargetValue;
	Symbol sActualTargetValue;
	Symbol sPredictedTargetValue;
	Continuous cActualTargetValueProb;
	KWClassifierInstanceEvaluation* instanceEvaluation;

	require(kwoObject != NULL);
	require(shared_liTargetAttribute.GetValue().IsValid());
	require(shared_liPredictionAttribute.GetValue().IsValid());

	// Appel de la methode ancetre
	bOk = KWPredictorEvaluationTask::SlaveProcessExploitDatabaseObject(kwoObject);

	// Acces aux modalites predites et effectives
	sActualTargetValue = kwoObject->GetSymbolValueAt(shared_liTargetAttribute.GetValue());
	sPredictedTargetValue = kwoObject->GetSymbolValueAt(shared_liPredictionAttribute.GetValue());

	// Mise a jour de la matrice de confusion
	slaveConfusionMatrixEvaluation->AddInstanceEvaluation(sPredictedTargetValue, sActualTargetValue);

	// Recherche de l'index en apprentissage de la modalite effective
	// Par defaut: le nombre de modalites cible en apprentissage
	// (signifie valeur cible inconnue en apprentissage)
	nActualValue = shared_nTargetValueNumber;
	if (shared_livProbAttributes.GetSize() > 0)
	{
		for (nTargetValue = 0; nTargetValue < shared_nTargetValueNumber; nTargetValue++)
		{
			if (shared_svPredictedModalities.GetAt(nTargetValue) == sActualTargetValue)
			{
				nActualValue = nTargetValue;
				break;
			}
		}
	}

	// Mise a jour du taux de compression si pertinente
	if (shared_livProbAttributes.GetSize() > 0)
	{
		// Recherche de la probabilite predite pour la valeur cible reelle
		cActualTargetValueProb = 0;
		if (nActualValue < shared_livProbAttributes.GetSize())
		{
			assert(kwoObject->GetSymbolValueAt(shared_liTargetAttribute.GetValue()) ==
			       shared_svPredictedModalities.GetAt(nActualValue));
			cActualTargetValueProb =
			    kwoObject->GetContinuousValueAt(shared_livProbAttributes.GetAt(nActualValue));

			// On projete sur [0, 1] pour avoir une probabilite quoi qu'il arrive
			if (cActualTargetValueProb < cEpsilon)
				cActualTargetValueProb = cEpsilon;
			if (cActualTargetValueProb > 1)
				cActualTargetValueProb = 1;
		}
		// Si la valeur etait inconnue en apprentissage, on lui associe une probabilite minimale
		else
			cActualTargetValueProb = cEpsilon;

		// Ajout du log negatif de cette probabilite a l'evaluation des scores
		output_dCompressionRate -= log(cActualTargetValueProb);
	}

	// Collecte des informations necessaires a l'estimation de l'AUC et aux courbes de lift
	if (input_bIsAucEvaluated)
	{
		// Initialisation du RNG de l'echantillonneur avec l'index de creation du premier objet de la sous-tache
		if (slaveInstanceEvaluationSampler->GetSampleSize() == 0)
			slaveInstanceEvaluationSampler->SetSamplerRandomSeed(kwoObject->GetCreationIndex());

		// Ajout de l'evaluation de l'instance
		instanceEvaluation = new KWClassifierInstanceEvaluation;
		instanceEvaluation->SetTargetValueNumber(shared_nTargetValueNumber);
		instanceEvaluation->SetActualTargetIndex(nActualValue);
		for (nTargetValue = 0; nTargetValue < shared_nTargetValueNumber; nTargetValue++)
		{
			instanceEvaluation->SetTargetProbAt(
			    nTargetValue,
			    kwoObject->GetContinuousValueAt(shared_livProbAttributes.GetAt(nTargetValue)));
		}
		slaveInstanceEvaluationSampler->Add(instanceEvaluation);
	}

	return bOk;
}

boolean KWClassifierEvaluationTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	require(slaveInstanceEvaluationSampler != NULL);
	require(slaveConfusionMatrixEvaluation != NULL);

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::SlaveFinalize(bProcessEndedCorrectly);

	// Nettoyage
	delete slaveInstanceEvaluationSampler;
	slaveInstanceEvaluationSampler = NULL;
	delete slaveConfusionMatrixEvaluation;
	slaveConfusionMatrixEvaluation = NULL;
	return bOk;
}

void KWClassifierEvaluationTask::InitializePredictorSharedVariables(KWPredictor* predictor)
{
	int nTargetIndex;
	KWTrainedClassifier* classifier;
	KWLoadIndex liTarget;
	KWLoadIndex liPrediction;
	KWLoadIndex liProbAttribute;
	ALString sTmp;

	// Informations du predicteur necessaires pour l'evaluation: Globalement des LoadIndex
	classifier = predictor->GetTrainedClassifier();
	liTarget = GetLoadIndex(classifier->GetTargetAttribute());
	liPrediction = GetLoadIndex(classifier->GetPredictionAttribute());
	shared_liTargetAttribute.SetValue(liTarget);
	shared_liPredictionAttribute.SetValue(liPrediction);
	shared_nTargetValueNumber = classifier->GetTargetValueNumber();
	shared_livProbAttributes.GetLoadIndexVector()->SetSize(shared_nTargetValueNumber);
	for (nTargetIndex = 0; nTargetIndex < shared_livProbAttributes.GetSize(); nTargetIndex++)
	{
		liProbAttribute = GetLoadIndex(classifier->GetProbAttributeAt(nTargetIndex));
		shared_livProbAttributes.SetAt(nTargetIndex, liProbAttribute);
		shared_svPredictedModalities.Add(classifier->GetTargetValueAt(nTargetIndex));
	}

	// La capacite de l'echantillonneur du esclave est initialisee dans MasterInitialize apres dimensionnement
	shared_nSlaveInstanceEvaluationCapacity = 0;

	// Warning s'il n'y a pas de modalites cibles specifiees
	if (shared_nTargetValueNumber == 0)
		AddWarning(sTmp + "The AUC value will not be evaluated as the target value probabilities are not "
				  "available from the predictor");
}

void KWClassifierEvaluationTask::CleanPredictorSharedVariables()
{
	// Reinitialisation des index des attributs necessaires a l'evaluation
	shared_liTargetAttribute.GetValue().Reset();
	shared_liPredictionAttribute.GetValue().Reset();
	shared_livProbAttributes.GetLoadIndexVector()->SetSize(0);
	shared_svPredictedModalities.GetSymbolVector()->SetSize(0);
}

int KWClassifierEvaluationTask::GetMainTargetModalityLiftIndex() const
{
	require(classifierEvaluation != NULL);

	if (classifierEvaluation->GetMainTargetModalityIndex() == -1)
		return -1;
	else
	{
		// Si la modalite cible principale a un index dans les premiere courbe de lift
		if (classifierEvaluation->GetMainTargetModalityIndex() <
		    classifierEvaluation->oaAllLiftCurveValues.GetSize())
			return classifierEvaluation->GetMainTargetModalityIndex();
		// Sinon, c'est la derniere courbe de lift memorisee
		else
			return classifierEvaluation->oaAllLiftCurveValues.GetSize() - 1;
	}
}

int KWClassifierEvaluationTask::GetComputedLiftCurveNumber() const
{
	require(classifierEvaluation != NULL);
	return classifierEvaluation->oaAllLiftCurveValues.GetSize();
}

int KWClassifierEvaluationTask::GetPredictorTargetIndexAtLiftCurveIndex(int nLiftCurve) const
{
	require(classifierEvaluation != NULL);
	require(0 <= nLiftCurve and nLiftCurve < GetComputedLiftCurveNumber());

	if (nLiftCurve == classifierEvaluation->oaAllLiftCurveValues.GetSize() - 1 and
	    GetMainTargetModalityLiftIndex() == classifierEvaluation->oaAllLiftCurveValues.GetSize() - 1)
		return classifierEvaluation->GetMainTargetModalityIndex();
	else
		return nLiftCurve;
}

int KWClassifierEvaluationTask::ComputeSamplerCapacity(longint lMemory, int nTargetValueNumber) const
{
	int nCapacity;
	longint lInstanceEvaluationSize;

	lInstanceEvaluationSize = ComputeInstanceEvaluationNecessaryMemory(nTargetValueNumber);
	if (lMemory / lInstanceEvaluationSize < INT_MAX)
		nCapacity = (int)(lMemory / lInstanceEvaluationSize);
	else
		nCapacity = INT_MAX;

	return nCapacity;
}

longint KWClassifierEvaluationTask::ComputeTaskSelfMemory(longint lTaskGrantedMemory,
							  RMPhysicalResource* taskMemoryRequirement,
							  RMPhysicalResource* parentTaskMemoryRequirement) const
{
	longint lDeltaMinMemory;
	longint lDeltaMaxMemory;
	longint lTaskMemoryRequirementRange;
	longint lTaskSelfMemory;
	double dMemoryRatio;

	require(taskMemoryRequirement != NULL);
	require(taskMemoryRequirement->GetMin() <= taskMemoryRequirement->GetMax());
	require(taskMemoryRequirement->GetMin() <= lTaskGrantedMemory and
		lTaskGrantedMemory <= taskMemoryRequirement->GetMax());
	require(parentTaskMemoryRequirement != NULL);
	require(parentTaskMemoryRequirement->GetMin() <= parentTaskMemoryRequirement->GetMax());

	// Calcul "delta" exigences de cette classe par rapport aux exigences de la classe ancetre
	lDeltaMinMemory = taskMemoryRequirement->GetMin() - parentTaskMemoryRequirement->GetMin();
	lDeltaMaxMemory = taskMemoryRequirement->GetMax() - parentTaskMemoryRequirement->GetMax();
	assert(0 < lDeltaMinMemory and lDeltaMinMemory <= lDeltaMaxMemory);

	// Si l'on a attribue le min a la tache alors on rends le delta min avec la classe ancetre
	if (lTaskGrantedMemory == taskMemoryRequirement->GetMin())
		lTaskSelfMemory = lDeltaMinMemory;
	// Si l'on a attribue le max a la tache alors on rends le delta max avec la classe ancetre
	else if (lTaskGrantedMemory == taskMemoryRequirement->GetMax())
		lTaskSelfMemory = lDeltaMaxMemory;
	// Sinon on rends au prorata du rang la memoire propre
	else
	{
		lTaskMemoryRequirementRange = taskMemoryRequirement->GetMax() - taskMemoryRequirement->GetMin();
		if (lTaskMemoryRequirementRange > 0)
		{
			dMemoryRatio =
			    double(lTaskGrantedMemory - taskMemoryRequirement->GetMin()) / lTaskMemoryRequirementRange;
			lTaskSelfMemory =
			    (longint)(lDeltaMinMemory + dMemoryRatio * (lDeltaMaxMemory - lDeltaMinMemory));
		}
		else
			lTaskSelfMemory = lDeltaMinMemory;
	}
	ensure(lTaskSelfMemory > 0);
	return lTaskSelfMemory;
}

longint KWClassifierEvaluationTask::ComputeInstanceEvaluationNecessaryMemory(int nTargetValueNumber) const
{
	return (longint)sizeof(KWClassifierInstanceEvaluation) + sizeof(KWClassifierInstanceEvaluation*) +
	       (longint)nTargetValueNumber * sizeof(Continuous);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWRegressorEvaluationTask

KWRegressorEvaluationTask::KWRegressorEvaluationTask()
{
	regressorEvaluation = NULL;

	// Initialisation des variables du maitre
	bIsRecCurveCalculated = true;
	nMasterErrorVectorMaxCapacity = 0;
	nSlaveErrorVectorMaxCapacity = 0;

	// Declaration des variables partagees
	DeclareSharedParameter(&shared_liTargetAttribute);
	DeclareSharedParameter(&shared_liMeanAttribute);
	DeclareSharedParameter(&shared_liDensityAttribute);
	DeclareSharedParameter(&shared_liTargetRankAttribute);
	DeclareSharedParameter(&shared_liMeanRankAttribute);
	DeclareSharedParameter(&shared_liRankDensityAttribute);
	DeclareTaskInput(&input_bIsRecCurveCalculated);
	DeclareTaskOutput(&output_nTargetMissingValueNumber);
	DeclareTaskOutput(&output_dRMSE);
	DeclareTaskOutput(&output_dMAE);
	DeclareTaskOutput(&output_dNLPD);
	DeclareTaskOutput(&output_dRankRMSE);
	DeclareTaskOutput(&output_dRankMAE);
	DeclareTaskOutput(&output_dRankNLPD);
	DeclareTaskOutput(&output_dvRankAbsoluteErrors);
	DeclareTaskOutput(&output_bIsRecCurveVectorOverflowed);
	DeclareTaskOutput(&output_nSlaveCapacityOverflowSize);
}

KWRegressorEvaluationTask::~KWRegressorEvaluationTask() {}

const ALString KWRegressorEvaluationTask::GetTaskName() const
{
	return "Regressor evaluation";
}

PLParallelTask* KWRegressorEvaluationTask::Create() const
{
	return new KWRegressorEvaluationTask;
}

boolean KWRegressorEvaluationTask::ComputeResourceRequirements()
{
	boolean bOk;
	longint lEstimatedTotalObjectNumber;
	longint lMaxRequiredEvaluationMemory;
	longint lRemainingMemory;
	longint lMaxMasterMemoryRequirement;

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::ComputeResourceRequirements();

	// Estimation du nombre total d'instance de la base en tenant compte du taux d'achantillonnage
	lEstimatedTotalObjectNumber =
	    shared_sourceDatabase.GetPLDatabase()->GetDatabase()->GetSampleEstimatedObjectNumber();

	// Estimation de la memoire totale necessaire pour le calcul de la courbe de REC
	lMaxRequiredEvaluationMemory = lEstimatedTotalObjectNumber * sizeof(double) + lMB / 2;

	// Estimation de la memoire total restante pour en donner au max la moitie au maitre
	lRemainingMemory = RMResourceManager::GetRemainingAvailableMemory();

	// On diminue potentiellement la memoire max du maitre, en tenant compte de la memoire disponible restante
	lMaxMasterMemoryRequirement = min(2 * lGB, 16 * lMB + lRemainingMemory / 2);
	lMaxMasterMemoryRequirement = min(lMaxMasterMemoryRequirement, 2 * lMaxRequiredEvaluationMemory);

	// Memoire Maitre: on demande un min et un max "raisonnable" permettant d'avoir une
	// tres bonne estimation des courbes de lift, du critere d'AUC, avec dans le pire des cas
	// un temps de calcul raisonnable. Ce ne serait pas raisonnable de trier des tres
	// nombreux vecteurs de score de tres grande taille, pour gagner une precision
	// supplementaire negligeable par rapport a la variance des resultats.
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->UpgradeMin(
	    min(16 * lMB, lMaxRequiredEvaluationMemory));
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->UpgradeMax(lMaxMasterMemoryRequirement);

	// Memoire esclave: meme logique que pour le maitre, et un peu moins pour le max
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMin(
	    min(8 * lMB, lMaxRequiredEvaluationMemory));
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMax(
	    min(64 * lMB, 2 * lMaxRequiredEvaluationMemory));

	// En priorite, on attribut la memoire au maitre, qui collecte l'ensemble des scores
	// calcules par les esclaves
	GetResourceRequirements()->SetMemoryAllocationPolicy(RMTaskResourceRequirement::balanced);
	return bOk;
}

boolean KWRegressorEvaluationTask::MasterInitialize()
{
	boolean bOk;
	longint lGrantedMemory;

	require(predictorEvaluation != NULL);

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::MasterInitialize();

	// Memorisation de la specialisation du rapport d'evaluation demandeur
	regressorEvaluation = cast(KWRegressorEvaluation*, predictorEvaluation);
	bIsRecCurveCalculated = true;

	// Dimensionnement du vecteur des predictions du regresseur cote maitre
	lGrantedMemory = GetMasterResourceGrant()->GetMemory();
	if (lGrantedMemory / sizeof(double) < INT_MAX)
		nMasterErrorVectorMaxCapacity = (int)(lGrantedMemory / sizeof(double));
	else
		nMasterErrorVectorMaxCapacity = INT_MAX;
	return bOk;
}

boolean KWRegressorEvaluationTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	boolean bOk;

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::MasterPrepareTaskInput(dTaskPercent, bIsTaskFinished);

	// Parametrage de la collecte des erreurs de prediction
	input_bIsRecCurveCalculated = bIsRecCurveCalculated;
	return bOk;
}

boolean KWRegressorEvaluationTask::MasterAggregateResults()
{
	boolean bOk;
	int nMasterCapacityOverflowSize;
	int nRankAbsoluteError;

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::MasterAggregateResults();

	// Mise a jour des criteres d'evaluation
	regressorEvaluation->lTargetMissingValueNumber += output_nTargetMissingValueNumber;
	regressorEvaluation->dRMSE += output_dRMSE;
	regressorEvaluation->dMAE += output_dMAE;
	regressorEvaluation->dNLPD += output_dNLPD;
	regressorEvaluation->dRankRMSE += output_dRankRMSE;
	regressorEvaluation->dRankMAE += output_dRankMAE;
	regressorEvaluation->dRankNLPD += output_dRankNLPD;

	// Si les esclaves overflowent, la courbe de REC n'est plus evaluee
	// On nettoie les donnees collectees et on emet un warning a l'utilisateur
	if (bIsRecCurveCalculated and output_bIsRecCurveVectorOverflowed)
	{
		// Nettoyage des donnees collectees en cours
		bIsRecCurveCalculated = false;
		dvRankAbsoluteErrors.SetSize(0);

		// Warning utilisateur (les autres criteres sont toujours calcules)
		AddWarning("Not enough memory in slave to compute REC curve (needs extra " +
			   RMResourceManager::ActualMemoryToString((longint)output_nSlaveCapacityOverflowSize *
								   sizeof(double)) +
			   ")");
	}

	// Collecte des erreurs de prediction pour la courbe de REC
	if (bIsRecCurveCalculated)
	{
		// Test de depassement de capacite du vecteur d'erreurs
		nMasterCapacityOverflowSize = dvRankAbsoluteErrors.GetSize() + output_dvRankAbsoluteErrors.GetSize() -
					      nMasterErrorVectorMaxCapacity;
		nMasterCapacityOverflowSize = max(nMasterCapacityOverflowSize, 0);
		if (nMasterCapacityOverflowSize > 0)
		{
			// Warning utilisteur (les autres criteres sont toujours calcules)
			AddWarning("Not enough memory in master to compute REC curve (needs extra " +
				   RMResourceManager::ActualMemoryToString((longint)nMasterCapacityOverflowSize *
									   sizeof(double)) +
				   ")");

			// Nettoyage
			dvRankAbsoluteErrors.SetSize(0);
			bIsRecCurveCalculated = false;
		}
		// Collecte des erreur veannt de l'esclave
		else
		{
			for (nRankAbsoluteError = 0; nRankAbsoluteError < output_dvRankAbsoluteErrors.GetSize();
			     nRankAbsoluteError++)
				dvRankAbsoluteErrors.Add(output_dvRankAbsoluteErrors.GetAt(nRankAbsoluteError));
		}
	}
	return bOk;
}

boolean KWRegressorEvaluationTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	const int nRankRECPartileNumber = 1000;
	boolean bOk;
	int nRankAbsoluteError;
	int nPartile;
	double dRankAbsoluteErrorThreshold;
	ALString sMessage;
	ALString sTmp;

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::MasterFinalize(bProcessEndedCorrectly);

	// Finalisation du calcul si ok
	if (bOk)
	{
		if (regressorEvaluation->lInstanceEvaluationNumber > 0)
		{
			// Calcul du nombre d'instances utilisable pour le calcul des criteres sur les valeurs
			regressorEvaluation->lInstanceEvaluationNumber -=
			    regressorEvaluation->lTargetMissingValueNumber;

			// Warning en cas de valeur manquantes
			if (regressorEvaluation->lInstanceEvaluationNumber == 0)
			{
				sMessage = "Evaluation criteria cannot be computed, since all records have a missing "
					   "target value";
				AddWarning(sMessage);
			}
			else if (regressorEvaluation->lTargetMissingValueNumber > 0)
			{
				sMessage = "Evaluation criteria are computed using a subset of ";
				sMessage += sTmp +
					    LongintToReadableString(regressorEvaluation->lInstanceEvaluationNumber) +
					    " records, because of ";
				sMessage += sTmp +
					    LongintToReadableString(regressorEvaluation->lTargetMissingValueNumber) +
					    " missing target values";
				AddWarning(sMessage);
			}

			// Criteres, evalue uniquement en l'absence de valeurs manquantes
			if (regressorEvaluation->lInstanceEvaluationNumber > 0)
			{
				regressorEvaluation->dRMSE /= regressorEvaluation->lInstanceEvaluationNumber;
				regressorEvaluation->dRMSE = sqrt(regressorEvaluation->dRMSE);
				regressorEvaluation->dMAE /= regressorEvaluation->lInstanceEvaluationNumber;
				regressorEvaluation->dNLPD /= regressorEvaluation->lInstanceEvaluationNumber;
				regressorEvaluation->dRankRMSE /= regressorEvaluation->lInstanceEvaluationNumber;
				regressorEvaluation->dRankRMSE = sqrt(regressorEvaluation->dRankRMSE);
				regressorEvaluation->dRankMAE /= regressorEvaluation->lInstanceEvaluationNumber;
				regressorEvaluation->dRankNLPD /= regressorEvaluation->lInstanceEvaluationNumber;

				// Calcul de la courbe de REC
				if (bIsRecCurveCalculated and dvRankAbsoluteErrors.GetSize() > 0)
				{
					assert(dvRankAbsoluteErrors.GetSize() ==
					       regressorEvaluation->lInstanceEvaluationNumber);
					dvRankAbsoluteErrors.Sort();
					regressorEvaluation->dvRankRECCurveValues.SetSize(nRankRECPartileNumber + 1);
					nRankAbsoluteError = 0;
					for (nPartile = 1; nPartile <= nRankRECPartileNumber; nPartile++)
					{
						dRankAbsoluteErrorThreshold = nPartile * 1.0 / nRankRECPartileNumber;

						// Calcul du nombre d'instance ayant une erreur inferieure au seuil
						while (nRankAbsoluteError < dvRankAbsoluteErrors.GetSize() and
						       dvRankAbsoluteErrors.GetAt(nRankAbsoluteError) <=
							   dRankAbsoluteErrorThreshold)
							nRankAbsoluteError++;

						// Memorisation de la valeur de la courbe de REC
						regressorEvaluation->dvRankRECCurveValues.SetAt(
						    nPartile,
						    (1.0 * nRankAbsoluteError) / dvRankAbsoluteErrors.GetSize());
					}
				}
			}

			// Nettoyage
			dvRankAbsoluteErrors.SetSize(0);
			nMasterErrorVectorMaxCapacity = 0;
			regressorEvaluation = NULL;
		}
	}
	return bOk;
}

boolean KWRegressorEvaluationTask::SlaveInitialize()
{
	boolean bOk;
	longint lGrantedMemory;

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::SlaveInitialize();

	// Dimensionnement du vecteur des predictions du regresseur cote maitre
	lGrantedMemory =
	    ComputeSlaveGrantedMemory(GetSlaveResourceRequirement(), GetSlaveResourceGrant()->GetMemory(), false);
	if (lGrantedMemory / sizeof(double) < INT_MAX)
		nSlaveErrorVectorMaxCapacity = (int)(lGrantedMemory / sizeof(double));
	else
		nSlaveErrorVectorMaxCapacity = INT_MAX;
	return bOk;
}

boolean KWRegressorEvaluationTask::SlaveProcessExploitDatabase()
{
	boolean bOk;
	longint lCapacityOverflowSize;

	// Initialisation des resultats de l'esclave
	output_nTargetMissingValueNumber = 0;
	output_dRMSE = 0;
	output_dMAE = 0;
	output_dNLPD = 0;
	output_dRankRMSE = 0;
	output_dRankMAE = 0;
	output_dRankNLPD = 0;
	output_bIsRecCurveVectorOverflowed = false;
	output_nSlaveCapacityOverflowSize = 0;

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::SlaveProcessExploitDatabase();

	// En cas d'overflow, on calcule sa taille pour la renvoyer au maitre
	if (output_bIsRecCurveVectorOverflowed)
	{
		lCapacityOverflowSize = output_lReadObjects - nSlaveErrorVectorMaxCapacity;
		if (lCapacityOverflowSize > INT_MAX)
			output_nSlaveCapacityOverflowSize = INT_MAX;
		else
			output_nSlaveCapacityOverflowSize = (int)lCapacityOverflowSize;
	}

	return bOk;
}

boolean KWRegressorEvaluationTask::SlaveProcessExploitDatabaseObject(const KWObject* kwoObject)
{
	const double dEpsilonMin = DBL_MIN;
	boolean bOk;
	Continuous cTargetActualValue;
	Continuous cTargetPredictedValue;
	double dDiff;
	double dDensity;
	boolean bMissingValue;

	require(kwoObject != NULL);

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::SlaveProcessExploitDatabaseObject(kwoObject);

	// Evaluation des criteres sur les valeurs
	bMissingValue = false;
	if (shared_liTargetAttribute.GetValue().IsValid() and shared_liMeanAttribute.GetValue().IsValid())
	{
		// Acces aux valeurs cibles reelle et predite
		cTargetActualValue = kwoObject->GetContinuousValueAt(shared_liTargetAttribute.GetValue());
		cTargetPredictedValue = kwoObject->GetContinuousValueAt(shared_liMeanAttribute.GetValue());

		// Detection de presence de valeur manquante
		if (cTargetActualValue == KWContinuous::GetMissingValue())
		{
			output_nTargetMissingValueNumber += 1;
			bMissingValue = true;
		}
		if (cTargetPredictedValue == KWContinuous::GetMissingValue())
			bMissingValue = true;

		// Mise a jour des crieres sur les valeur si possible
		if (not bMissingValue)
		{
			// Calcul de la difference de valeur
			dDiff = cTargetPredictedValue - cTargetActualValue;

			// Mise a jour du RMSE et du MAE
			output_dRMSE += dDiff * dDiff;
			output_dMAE += fabs(dDiff);
		}
	}

	// Mise a jour de la somme des densites de valeurs
	if (shared_liDensityAttribute.GetValue().IsValid() and not bMissingValue)
	{
		dDensity = kwoObject->GetContinuousValueAt(shared_liDensityAttribute.GetValue());
		if (dDensity < dEpsilonMin)
			dDensity = dEpsilonMin;
		output_dNLPD -= log(dDensity);
	}

	// Evaluation des criteres sur les rangs
	if (shared_liTargetRankAttribute.GetValue().IsValid() and shared_liMeanRankAttribute.GetValue().IsValid() and
	    not bMissingValue)
	{
		// Calcul de la difference de rang
		dDiff = kwoObject->GetContinuousValueAt(shared_liMeanRankAttribute.GetValue()) -
			kwoObject->GetContinuousValueAt(shared_liTargetRankAttribute.GetValue());

		// Mise a jour du RankRMSE et du RankMAE
		output_dRankRMSE += dDiff * dDiff;
		output_dRankMAE += fabs(dDiff);

		// Ajout d'une valeur dans le vecteur des difference de rangs
		if (input_bIsRecCurveCalculated and not output_bIsRecCurveVectorOverflowed)
		{
			// On arrete tout si l'on a va depasser la capacite de l'esclave
			if (output_dvRankAbsoluteErrors.GetSize() == nSlaveErrorVectorMaxCapacity)
			{
				output_dvRankAbsoluteErrors.GetDoubleVector()->SetSize(0);
				output_bIsRecCurveVectorOverflowed = true;
			}
			// Sinon, on ajoute l'erreur
			else
				output_dvRankAbsoluteErrors.Add(fabs(dDiff));
		}
	}

	// Mise a jour de la somme des densites de rang
	if (shared_liRankDensityAttribute.GetValue().IsValid() and not bMissingValue)
	{
		dDensity = kwoObject->GetContinuousValueAt(shared_liRankDensityAttribute.GetValue());
		if (dDensity < dEpsilonMin)
			dDensity = dEpsilonMin;
		output_dRankNLPD -= log(dDensity);
	}

	return bOk;
}

boolean KWRegressorEvaluationTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::SlaveFinalize(bProcessEndedCorrectly);

	// Nettoyage
	nSlaveErrorVectorMaxCapacity = 0;
	return bOk;
}

void KWRegressorEvaluationTask::InitializePredictorSharedVariables(KWPredictor* predictor)
{
	KWTrainedRegressor* trainedRegressor;

	trainedRegressor = predictor->GetTrainedRegressor();
	shared_liTargetAttribute.SetValue(GetLoadIndex(trainedRegressor->GetTargetAttribute()));
	shared_liMeanAttribute.SetValue(GetLoadIndex(trainedRegressor->GetMeanAttribute()));
	shared_liDensityAttribute.SetValue(GetLoadIndex(trainedRegressor->GetDensityAttribute()));
	shared_liTargetRankAttribute.SetValue(GetLoadIndex(trainedRegressor->GetTargetAttributeRank()));
	shared_liMeanRankAttribute.SetValue(GetLoadIndex(trainedRegressor->GetMeanRankAttribute()));
	shared_liRankDensityAttribute.SetValue(GetLoadIndex(trainedRegressor->GetDensityRankAttribute()));
}

void KWRegressorEvaluationTask::CleanPredictorSharedVariables()
{
	shared_liTargetAttribute.GetValue().Reset();
	shared_liMeanAttribute.GetValue().Reset();
	shared_liDensityAttribute.GetValue().Reset();
	shared_liTargetRankAttribute.GetValue().Reset();
	shared_liMeanRankAttribute.GetValue().Reset();
	shared_liRankDensityAttribute.GetValue().Reset();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWConfusionMatrixEvaluation

int KWConfusionMatrixEvaluation::nMaxSize = 1000;

KWConfusionMatrixEvaluation::KWConfusionMatrixEvaluation()
{
	nTotalFrequency = 0;
}

KWConfusionMatrixEvaluation::~KWConfusionMatrixEvaluation()
{
	svPredictedValues.SetSize(0);
	nkdPredictedValues.RemoveAll();
	svActualValues.SetSize(0);
	nkdActualValues.RemoveAll();
	oaConfusionMatrixFreqs.DeleteAll();
	nkdPredictedValuesActualFreqs.DeleteAll();
}

void KWConfusionMatrixEvaluation::Initialize()
{
	nTotalFrequency = 0;
	svPredictedValues.SetSize(0);
	nkdPredictedValues.RemoveAll();
	svActualValues.SetSize(0);
	nkdActualValues.RemoveAll();
	oaConfusionMatrixFreqs.DeleteAll();
	nkdPredictedValuesActualFreqs.DeleteAll();
}

void KWConfusionMatrixEvaluation::AddPredictedTarget(const Symbol& sPredictedTarget)
{
	AddInstances(sPredictedTarget, sPredictedTarget, 0);
}

void KWConfusionMatrixEvaluation::AddInstanceEvaluation(const Symbol& sPredictedTarget, const Symbol& sActualTarget)
{
	AddInstances(sPredictedTarget, sActualTarget, 1);
}

void KWConfusionMatrixEvaluation::AddEvaluatedMatrix(const KWDataGridStats* dgsConfusionMatrix)
{
	const KWDGSAttributeSymbolValues* predicted;
	const KWDGSAttributeSymbolValues* actual;
	int nPredicted;
	int nActual;
	int nFrequency;

	require(dgsConfusionMatrix->GetAttributeNumber() == 2);

	predicted = cast(KWDGSAttributeSymbolValues*, dgsConfusionMatrix->GetAttributeAt(0));
	actual = cast(KWDGSAttributeSymbolValues*, dgsConfusionMatrix->GetAttributeAt(1));

	assert(actual->GetPartNumber() == predicted->GetPartNumber());

	for (nPredicted = 0; nPredicted < predicted->GetPartNumber(); nPredicted++)
	{
		for (nActual = 0; nActual < actual->GetPartNumber(); nActual++)
		{
			nFrequency = dgsConfusionMatrix->GetBivariateCellFrequencyAt(nPredicted, nActual);

			if (nFrequency > 0)
				AddInstances(predicted->GetValueAt(nPredicted), actual->GetValueAt(nActual),
					     nFrequency);
		}
	}
}

double KWConfusionMatrixEvaluation::ComputeAccuracy() const
{
	double dResultAccuracy;
	double dDiagonalFrequency;
	int nActual;
	Symbol sActualTarget;

	// Calcul des frequences sur la diagonale (bonnes predictions)
	dDiagonalFrequency = 0;
	for (nActual = 0; nActual < svActualValues.GetSize(); nActual++)
	{
		sActualTarget = svActualValues.GetAt(nActual);
		dDiagonalFrequency += ComputeFrequencyAt(sActualTarget, sActualTarget);
	}

	// Calcul du taux de bonne prediction
	dResultAccuracy = dDiagonalFrequency;
	if (nTotalFrequency > 0)
		dResultAccuracy /= nTotalFrequency;

	return dResultAccuracy;
}

double KWConfusionMatrixEvaluation::ComputeBalancedAccuracy() const
{
	int nTrueTargetNumber;
	double dResultAccuracy;
	int nCumulatedActualFrequency;
	int nActual;
	Symbol sActualTarget;

	// Sommation des taux de bonne prediction par classe cible
	dResultAccuracy = 0;
	nTrueTargetNumber = 0;
	for (nActual = 0; nActual < svActualValues.GetSize(); nActual++)
	{
		sActualTarget = svActualValues.GetAt(nActual);
		nCumulatedActualFrequency = ComputeCumulatedActualFrequencyAt(sActualTarget);
		if (nCumulatedActualFrequency > 0)
		{
			nTrueTargetNumber++;
			dResultAccuracy +=
			    ComputeFrequencyAt(sActualTarget, sActualTarget) * 1.0 / nCumulatedActualFrequency;
		}
	}

	// Calcul de la moyenne
	if (nTrueTargetNumber > 0)
		dResultAccuracy /= nTrueTargetNumber;

	return dResultAccuracy;
}

double KWConfusionMatrixEvaluation::ComputeMajorityAccuracy() const
{
	double dResultAccuracy;
	int nCumulatedActualFrequency;
	int nActual;
	Symbol sActualTarget;

	// Recherche de la frequence maximale des valeurs cibles
	dResultAccuracy = 0;
	for (nActual = 0; nActual < svActualValues.GetSize(); nActual++)
	{
		sActualTarget = svActualValues.GetAt(nActual);
		nCumulatedActualFrequency = ComputeCumulatedActualFrequencyAt(sActualTarget);
		if (nCumulatedActualFrequency > dResultAccuracy)
			dResultAccuracy = nCumulatedActualFrequency;
	}
	if (nTotalFrequency > 0)
		dResultAccuracy /= nTotalFrequency;
	return dResultAccuracy;
}

double KWConfusionMatrixEvaluation::ComputeTargetEntropy() const
{
	double dTargetEntropy;
	int nCumulatedActualFrequency;
	int nActual;
	Symbol sActualTarget;
	double dTargetProb;

	// Calcul de l'entropie cible
	dTargetEntropy = 0;
	for (nActual = 0; nActual < svActualValues.GetSize(); nActual++)
	{
		sActualTarget = svActualValues.GetAt(nActual);
		nCumulatedActualFrequency = ComputeCumulatedActualFrequencyAt(sActualTarget);
		if (nTotalFrequency > 0)
			dTargetProb = nCumulatedActualFrequency * 1.0 / nTotalFrequency;
		else
			dTargetProb = 0;

		// Mise a jour de l'entropie
		if (dTargetProb > 0)
			dTargetEntropy -= dTargetProb * log(dTargetProb);
	}

	// Arrondi a zero si neccessaire
	if (dTargetEntropy < pow(1.0 + nTotalFrequency, -2))
		dTargetEntropy = 0;

	return dTargetEntropy;
}

KWDGSAttributeSymbolValues* KWConfusionMatrixEvaluation::CreateModalities(SymbolVector* svModalities,
									  const ALString& sAttributeName) const
{
	boolean bStarValueIsAbsent = true;
	int nModalities;
	KWDGSAttributeSymbolValues* modalities;

	// Verification si la StarValue est absente
	for (nModalities = 0; nModalities < svModalities->GetSize(); nModalities++)
	{
		if (svModalities->GetAt(nModalities) == Symbol::GetStarValue())
			bStarValueIsAbsent = false;
	}

	// Ajout des modalites
	modalities = new KWDGSAttributeSymbolValues;
	modalities->SetAttributeName(sAttributeName);
	modalities->SetPartNumber(svModalities->GetSize());
	for (nModalities = 0; nModalities < svModalities->GetSize(); nModalities++)
		modalities->SetValueAt(nModalities, svModalities->GetAt(nModalities));

	// Si la StarValue est absente l'on l'ajoute a la fin
	if (bStarValueIsAbsent)
	{
		modalities->SetPartNumber(svModalities->GetSize() + 1);
		modalities->SetValueAt(svModalities->GetSize(), Symbol::GetStarValue());
	}

	// Initialiation de l'InitialValueNumber et GranularizedValeurNumber a 1 > 0 pour passer le Check
	modalities->SetInitialValueNumber(1);
	modalities->SetGranularizedValueNumber(1);

	return modalities;
}

void KWConfusionMatrixEvaluation::ExportDataGridStats(KWDataGridStats* dgsConfusionMatrix) const
{
	SymbolVector* svConfusionMatrixAllValues;
	int nActual;
	Symbol sActualTarget;
	NumericKeyDictionary* nkdActualFrequencies;
	int nPredicted;
	Symbol sPredictedTarget;
	IntObject* ioFrequency;
	int nFrequency;
	int nConfusionMatrixSize;
	boolean bTruncated = false;
	int nTruncatedFrequency;

	require(dgsConfusionMatrix != NULL);

	// Initialisation de la matrice de confusion
	dgsConfusionMatrix->DeleteAll();

	// Recherche de toutes les valeurs cibles, predites et effectives
	svConfusionMatrixAllValues = new SymbolVector;
	svConfusionMatrixAllValues->CopyFrom(&svPredictedValues);
	for (nActual = 0; nActual < svActualValues.GetSize(); nActual++)
	{
		sActualTarget = svActualValues.GetAt(nActual);
		if (nkdPredictedValues.Lookup((NUMERIC)sActualTarget.GetNumericKey()) == NULL)
			svConfusionMatrixAllValues->Add(sActualTarget);
	}

	// Tri de toutes les valeurs cibles
	svConfusionMatrixAllValues->SortValues();

	// On tronque si necessaire la taille de la matrice de confusion
	nConfusionMatrixSize = svConfusionMatrixAllValues->GetSize();
	if (nConfusionMatrixSize > GetMaxSize())
	{
		nConfusionMatrixSize = GetMaxSize();
		bTruncated = true;
	}

	// Preparation de la grille pour la matrice de confusion
	dgsConfusionMatrix->AddAttribute(CreateModalities(svConfusionMatrixAllValues, "Prediction"));
	dgsConfusionMatrix->AddAttribute(CreateModalities(svConfusionMatrixAllValues, "Actual"));
	dgsConfusionMatrix->CreateAllCells();

	// Alimentation des effectifs
	nTruncatedFrequency = 0;
	for (nPredicted = 0; nPredicted < nConfusionMatrixSize; nPredicted++)
	{
		sPredictedTarget = svConfusionMatrixAllValues->GetAt(nPredicted);
		for (nActual = 0; nActual < nConfusionMatrixSize; nActual++)
		{
			sActualTarget = svConfusionMatrixAllValues->GetAt(nActual);

			// Recherche de l'effectif correspondant
			nFrequency = 0;
			nkdActualFrequencies =
			    cast(NumericKeyDictionary*,
				 nkdPredictedValuesActualFreqs.Lookup((NUMERIC)sPredictedTarget.GetNumericKey()));
			if (nkdActualFrequencies != NULL)
			{
				ioFrequency = cast(
				    IntObject*, nkdActualFrequencies->Lookup((NUMERIC)sActualTarget.GetNumericKey()));
				if (ioFrequency != NULL)
					nFrequency = ioFrequency->GetInt();
				nTruncatedFrequency += nFrequency;
			}

			// Mise a jour de la matrice de confusion
			dgsConfusionMatrix->SetBivariateCellFrequencyAt(nPredicted, nActual, nFrequency);
		}
	}

	// On calcule si necessaire les effectifs dans la partie tronquee de la matrice de confusion
	if (bTruncated)
	{
		// Calcul dans la partie predite tronquee
		for (nPredicted = nConfusionMatrixSize; nPredicted < svConfusionMatrixAllValues->GetSize();
		     nPredicted++)
		{
			sPredictedTarget = svConfusionMatrixAllValues->GetAt(nPredicted);
			for (nActual = 0; nActual < nConfusionMatrixSize; nActual++)
			{
				sActualTarget = svConfusionMatrixAllValues->GetAt(nActual);

				// Recherche de l'effectif correspondant
				nFrequency = 0;
				nkdActualFrequencies = cast(
				    NumericKeyDictionary*,
				    nkdPredictedValuesActualFreqs.Lookup((NUMERIC)sPredictedTarget.GetNumericKey()));
				if (nkdActualFrequencies != NULL)
				{
					ioFrequency =
					    cast(IntObject*,
						 nkdActualFrequencies->Lookup((NUMERIC)sActualTarget.GetNumericKey()));
					if (ioFrequency != NULL)
						nFrequency = ioFrequency->GetInt();
					nTruncatedFrequency += nFrequency;
				}

				// Mise a jour de la matrice de confusion
				dgsConfusionMatrix->SetBivariateCellFrequencyAt(nConfusionMatrixSize, nActual,
										nFrequency);
			}
		}

		// Calcul dans la partie reelle tronquee
		for (nPredicted = 0; nPredicted < nConfusionMatrixSize; nPredicted++)
		{
			sPredictedTarget = svConfusionMatrixAllValues->GetAt(nPredicted);
			for (nActual = nConfusionMatrixSize; nActual < svConfusionMatrixAllValues->GetSize(); nActual++)
			{
				sActualTarget = svConfusionMatrixAllValues->GetAt(nActual);

				// Recherche de l'effectif correspondant
				nFrequency = 0;
				nkdActualFrequencies = cast(
				    NumericKeyDictionary*,
				    nkdPredictedValuesActualFreqs.Lookup((NUMERIC)sPredictedTarget.GetNumericKey()));
				if (nkdActualFrequencies != NULL)
				{
					ioFrequency =
					    cast(IntObject*,
						 nkdActualFrequencies->Lookup((NUMERIC)sActualTarget.GetNumericKey()));
					if (ioFrequency != NULL)
						nFrequency = ioFrequency->GetInt();
					nTruncatedFrequency += nFrequency;
				}

				// Mise a jour de la matrice de confusion
				dgsConfusionMatrix->SetBivariateCellFrequencyAt(nPredicted, nConfusionMatrixSize,
										nFrequency);
			}
		}

		// On range l'effectif manquant non deja traite
		dgsConfusionMatrix->SetBivariateCellFrequencyAt(nConfusionMatrixSize, nConfusionMatrixSize,
								nTotalFrequency - nTruncatedFrequency);
	}

	delete svConfusionMatrixAllValues;
	svConfusionMatrixAllValues = NULL;
}

int KWConfusionMatrixEvaluation::GetMaxSize()
{
	return nMaxSize;
}

void KWConfusionMatrixEvaluation::SetMaxSize(int nValue)
{
	require(nValue >= 0);
	nMaxSize = nValue;
}

boolean KWConfusionMatrixEvaluation::Check() const
{
	boolean bOk = true;
	int nCell;
	int nPredicted;
	int nActual;
	Symbol sPredicted;
	Symbol sActual;
	int nCheckedTotalFrequency;
	KWDataGridStats dgsConfusionMatrix;

	// Verification de l'effectif total par parcours du tableau d'effectifs
	nCheckedTotalFrequency = 0;
	for (nCell = 0; nCell < oaConfusionMatrixFreqs.GetSize(); nCell++)
		nCheckedTotalFrequency += cast(IntObject*, oaConfusionMatrixFreqs.GetAt(nCell))->GetInt();
	bOk = bOk and (nCheckedTotalFrequency == nTotalFrequency);
	assert(bOk);

	// Verification de l'effectif total par parcours des cellules
	nCheckedTotalFrequency = 0;
	for (nPredicted = 0; nPredicted < svPredictedValues.GetSize(); nPredicted++)
	{
		for (nActual = 0; nActual < svActualValues.GetSize(); nActual++)
		{
			sPredicted = svPredictedValues.GetAt(nPredicted);
			sActual = svActualValues.GetAt(nActual);
			nCheckedTotalFrequency += ComputeFrequencyAt(sPredicted, sActual);
		}
	}
	bOk = bOk and (nCheckedTotalFrequency == nTotalFrequency);
	assert(bOk);

	// Verification de l'effectif total par les totaux de valeurs predites
	nCheckedTotalFrequency = 0;
	for (nPredicted = 0; nPredicted < svPredictedValues.GetSize(); nPredicted++)
	{
		sPredicted = svPredictedValues.GetAt(nPredicted);
		nCheckedTotalFrequency += ComputeCumulatedPredictedFrequencyAt(sPredicted);
	}
	bOk = bOk and (nCheckedTotalFrequency == nTotalFrequency);
	assert(bOk);

	// Verification de l'effectif total par les totaux de valeurs reelles
	nCheckedTotalFrequency = 0;
	for (nActual = 0; nActual < svActualValues.GetSize(); nActual++)
	{
		sActual = svActualValues.GetAt(nActual);
		nCheckedTotalFrequency += ComputeCumulatedActualFrequencyAt(sActual);
	}
	bOk = bOk and (nCheckedTotalFrequency == nTotalFrequency);
	assert(bOk);

	// Verification de l'effectif global par la matrice de confusion
	ExportDataGridStats(&dgsConfusionMatrix);
	nCheckedTotalFrequency = dgsConfusionMatrix.ComputeGridFrequency();
	bOk = bOk and (nCheckedTotalFrequency == nTotalFrequency);
	assert(bOk);

	return bOk;
}

void KWConfusionMatrixEvaluation::Write(ostream& ost) const
{
	KWDataGridStats dataGridStats;
	ExportDataGridStats(&dataGridStats);
	dataGridStats.WriteFrequencyCrossTable(ost);
}

void KWConfusionMatrixEvaluation::AddInstances(const Symbol& sPredictedTarget, const Symbol& sActualTarget,
					       int nFrequency)
{
	NUMERIC predictedTargetKey;
	NUMERIC actualTargetKey;
	IntObject* ioFrequency;
	NumericKeyDictionary* actualFrequencies;

	require(nFrequency >= 0);

	predictedTargetKey = (NUMERIC)sPredictedTarget.GetNumericKey();
	actualTargetKey = (NUMERIC)sActualTarget.GetNumericKey();

	// Memorisation de la valeur predite si n'est pas encore enregistree
	if (nkdPredictedValues.Lookup(predictedTargetKey) == NULL)
	{
		nkdPredictedValues.SetAt(predictedTargetKey, &svPredictedValues);
		svPredictedValues.Add(sPredictedTarget);
	}

	// Memorisation de la valeur reelle si n'est pas encore enregistree
	if (nkdActualValues.Lookup(actualTargetKey) == NULL)
	{
		nkdActualValues.SetAt(actualTargetKey, &svActualValues);
		svActualValues.Add(sActualTarget);
	}

	// Recherche du dictionnaire d'effectifs pour la valeur predite
	actualFrequencies = cast(NumericKeyDictionary*, nkdPredictedValuesActualFreqs.Lookup(predictedTargetKey));
	if (actualFrequencies == NULL)
	{
		actualFrequencies = new NumericKeyDictionary;
		nkdPredictedValuesActualFreqs.SetAt(predictedTargetKey, actualFrequencies);
	}

	// Mise a jour de l'effectif
	ioFrequency = cast(IntObject*, actualFrequencies->Lookup(actualTargetKey));
	if (ioFrequency == NULL)
	{
		ioFrequency = new IntObject;
		actualFrequencies->SetAt(actualTargetKey, ioFrequency);
		oaConfusionMatrixFreqs.Add(ioFrequency);
	}
	ioFrequency->SetInt(ioFrequency->GetInt() + nFrequency);

	// Mise a jour de l'effectif total
	nTotalFrequency += nFrequency;
}

int KWConfusionMatrixEvaluation::ComputeFrequencyAt(const Symbol& sPredictedTarget, const Symbol& sActualTarget) const
{
	int nFrequency;
	NumericKeyDictionary* nkdActualFrequencies;
	IntObject* ioFrequency;
	NUMERIC actualTargetKey;
	NUMERIC predictedTargetKey;

	// Si la recherche echoue alors il n'y a pas d'effectifs dans la cellule [target, actual]
	nFrequency = 0;

	// Recherche de l'effectif de la cellule [target, actual]
	predictedTargetKey = (NUMERIC)sPredictedTarget.GetNumericKey();
	nkdActualFrequencies = cast(NumericKeyDictionary*, nkdPredictedValuesActualFreqs.Lookup(predictedTargetKey));
	if (nkdActualFrequencies != NULL)
	{
		actualTargetKey = (NUMERIC)sActualTarget.GetNumericKey();
		ioFrequency = cast(IntObject*, nkdActualFrequencies->Lookup(actualTargetKey));

		if (ioFrequency != NULL)
			nFrequency = ioFrequency->GetInt();
	}

	return nFrequency;
}

int KWConfusionMatrixEvaluation::ComputeCumulatedActualFrequencyAt(const Symbol& sActualTarget) const
{
	int nCumulatedFrequency;
	NumericKeyDictionary* nkdActualFrequencies;
	IntObject* ioFrequency;
	int nPredicted;
	Symbol sPredictedTarget;

	// Parcours des dictionnaires de modalites predites
	// Couteux algorithmiquement si la matrice est creuse
	nCumulatedFrequency = 0;
	for (nPredicted = 0; nPredicted < svPredictedValues.GetSize(); nPredicted++)
	{
		sPredictedTarget = svPredictedValues.GetAt(nPredicted);

		// Recherche de l'existance d'une cellule non vide pour la modalite reelle
		nkdActualFrequencies =
		    cast(NumericKeyDictionary*,
			 nkdPredictedValuesActualFreqs.Lookup((NUMERIC)sPredictedTarget.GetNumericKey()));
		if (nkdActualFrequencies != NULL)
		{
			ioFrequency =
			    cast(IntObject*, nkdActualFrequencies->Lookup((NUMERIC)sActualTarget.GetNumericKey()));
			if (ioFrequency != NULL)
				nCumulatedFrequency += ioFrequency->GetInt();
		}
	}
	return nCumulatedFrequency;
}

int KWConfusionMatrixEvaluation::ComputeCumulatedPredictedFrequencyAt(const Symbol& sPredictedTarget) const
{
	int nCumulatedFrequency;
	NumericKeyDictionary* nkdActualFrequencies;
	IntObject* ioFrequency;
	POSITION position;
	Object* oElement;
	NUMERIC rKey;

	// Recherche du dictionnaire de modalites relles correspondant a la modalite predite
	// Efficace algorithmiquement si la matrice est creuse
	nCumulatedFrequency = 0;
	nkdActualFrequencies = cast(NumericKeyDictionary*,
				    nkdPredictedValuesActualFreqs.Lookup((NUMERIC)sPredictedTarget.GetNumericKey()));
	if (nkdActualFrequencies != NULL)
	{
		// Somme des frequences de cellules non vides pour cette modalite predite
		position = nkdActualFrequencies->GetStartPosition();
		while (position != NULL)
		{
			nkdActualFrequencies->GetNextAssoc(position, rKey, oElement);
			ioFrequency = cast(IntObject*, oElement);
			nCumulatedFrequency += ioFrequency->GetInt();
		}
	}
	return nCumulatedFrequency;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWAucEvaluation

int KWClassifierInstanceEvaluationCompare(const void* elem1, const void* elem2)
{
	KWClassifierInstanceEvaluation* instanceA;
	KWClassifierInstanceEvaluation* instanceB;

	instanceA = cast(KWClassifierInstanceEvaluation*, *(Object**)elem1);
	instanceB = cast(KWClassifierInstanceEvaluation*, *(Object**)elem2);

	assert(instanceA != NULL and instanceB != NULL);

	return KWContinuous::Compare(instanceA->GetSortValue(), instanceB->GetSortValue());
}

// On utilise un epsilon = 0 pour gerer les egalites score de la courbe de ROC,
// calculee selon la methode des trapezes.
// Des differences non nulles (e.g 1e-5) menent a une sous-estimation parfois importante de l'AUC
double KWAucEvaluation::dEpsilon = 0;

KWAucEvaluation::KWAucEvaluation()
{
	nTargetValueNumber = 0;
	oaInstanceEvaluations = NULL;
}

KWAucEvaluation::~KWAucEvaluation() {}

void KWAucEvaluation::Initialize()
{
	nTargetValueNumber = 0;
	oaInstanceEvaluations = NULL;
}

void KWAucEvaluation::SetInstanceEvaluations(ObjectArray* instances)
{
	oaInstanceEvaluations = instances;
}

double KWAucEvaluation::ComputeGlobalAUCValue()
{
	double dEvaluation;
	int nTargetValue;
	IntVector ivTargetValueFrequencies;
	int nUnknownTargetValue;
	KWClassifierInstanceEvaluation* instanceEvaluation;
	int nInstance;

	require(oaInstanceEvaluations != NULL);

	// Calcul de l'evaluation globale en ponderant par les frequences de modalites cibles
	if (oaInstanceEvaluations->GetSize() == 0)
		dEvaluation = 1;
	// Cas general: ponderation des evaluations par les frequences des modalites cibles
	// On ne gere pas le cas particulier de deux valeurs cibles (le calcul de l'AUC n'est pas tout
	// a fait symetrique pour les deux AUC, pour des raison numeriques (la precision autour de p=0
	// est plus importante qu'autour de p=1).
	// Les modalite cibles non vues en apprentissage comptent pour une AUC de 0.5
	else
	{
		// Calcul des effectifs par valeurs cible
		// Le dernier effectif du vecteur (surnumeraire) sert a gerer les modalites cibles en test
		// non vues en apprentissage
		ivTargetValueFrequencies.SetSize(GetTargetValueNumber() + 1);
		nUnknownTargetValue = GetTargetValueNumber();
		for (nInstance = 0; nInstance < oaInstanceEvaluations->GetSize(); nInstance++)
		{
			instanceEvaluation =
			    cast(KWClassifierInstanceEvaluation*, oaInstanceEvaluations->GetAt(nInstance));
			nTargetValue = instanceEvaluation->GetActualTargetIndex();
			assert(0 <= nTargetValue and nTargetValue <= GetTargetValueNumber());

			// On ignore les instances des classes non vues en apprentissage
			if (nTargetValue < GetTargetValueNumber())
				ivTargetValueFrequencies.UpgradeAt(nTargetValue, 1);
			else
				ivTargetValueFrequencies.UpgradeAt(nUnknownTargetValue, 1);
		}

		// Evaluation: On table sur une AUC de 0.5 pour les valeurs cibles inconnues
		dEvaluation = 0;
		for (nTargetValue = 0; nTargetValue < GetTargetValueNumber(); nTargetValue++)
			dEvaluation += ivTargetValueFrequencies.GetAt(nTargetValue) * ComputeAUCValueAt(nTargetValue);
		dEvaluation += ivTargetValueFrequencies.GetAt(nUnknownTargetValue) * 0.5;
		dEvaluation /= oaInstanceEvaluations->GetSize();
	}
	return dEvaluation;
}

double KWAucEvaluation::ComputeAUCValueAt(int nTargetValueIndex)
{
	boolean bLocalTrace = false;
	KWClassifierInstanceEvaluation* instanceEvaluation;
	double dEvaluation;
	int nInstance;
	Continuous cBlockScore;
	double dBlockROCCurveArea;
	double dObservedROCCurveArea;
	Continuous cScore;
	int nModalityIndex;
	int nTruePositive;
	int nFalsePositive;
	int nPreviousTruePositive;
	int nPreviousFalsePositive;

	require(oaInstanceEvaluations != NULL);
	require(0 <= nTargetValueIndex and nTargetValueIndex < GetTargetValueNumber());

	// Tri des scores pour cette modalite
	SortInstanceEvaluationsAt(nTargetValueIndex);

	// Entete de la trace
	if (bLocalTrace)
		cout << "Instance\tScore\tTP\tFP\tROC area\tTotal AUC" << endl;

	// Calcul de la surface de la courbe de ROC observee
	dObservedROCCurveArea = 0;
	nTruePositive = 0;
	nFalsePositive = 0;
	nPreviousTruePositive = 0;
	nPreviousFalsePositive = 0;
	cBlockScore = 0;
	for (nInstance = 0; nInstance < oaInstanceEvaluations->GetSize(); nInstance++)
	{
		instanceEvaluation = cast(KWClassifierInstanceEvaluation*, oaInstanceEvaluations->GetAt(nInstance));

		// Acces aux valeurs
		cScore = instanceEvaluation->GetTargetProbAt(nTargetValueIndex);
		nModalityIndex = instanceEvaluation->GetActualTargetIndex();

		// Memorisation de la premiere valeur de lift
		if (nInstance == 0)
			cBlockScore = cScore;

		// Contribution a l'aire si changement de bloc
		assert(cScore <= cBlockScore);
		if (fabs(cScore - cBlockScore) > dEpsilon)
		{
			// Mise a jour de la valeur de la surface
			// Toutes les instances ayant meme score sont traitees en bloc, de
			// facon a ne pas dependre de leur ordre, aleatoire
			dBlockROCCurveArea = (nFalsePositive - nPreviousFalsePositive) * 1.0 *
					     (nTruePositive + nPreviousTruePositive) / 2.0;
			dObservedROCCurveArea += dBlockROCCurveArea;

			// Trace
			if (bLocalTrace)
			{
				cout << nInstance << "\t" << cBlockScore << "\t" << nTruePositive << "\t"
				     << nFalsePositive << "\t" << dBlockROCCurveArea << "\t" << dObservedROCCurveArea
				     << endl;
			}

			// Initialisation d'un nouveau bloc
			nPreviousTruePositive = nTruePositive;
			nPreviousFalsePositive = nFalsePositive;
			cBlockScore = cScore;
		}

		// Incrementation du nombre d'instances positive ou negative
		if (nModalityIndex == nTargetValueIndex)
			nTruePositive++;
		else
			nFalsePositive++;
	}

	// Prise en compte du dernier block
	dBlockROCCurveArea =
	    (nFalsePositive - nPreviousFalsePositive) * 1.0 * (nTruePositive + nPreviousTruePositive) / 2.0;
	dObservedROCCurveArea += dBlockROCCurveArea;

	// Trace du dernier bloc
	if (bLocalTrace)
	{
		cout << nInstance << "\t" << cBlockScore << "\t" << nTruePositive << "\t" << nFalsePositive << "\t"
		     << dBlockROCCurveArea << "\t" << dObservedROCCurveArea << endl;
	}

	// Normalisation dans el cas general
	if (dObservedROCCurveArea > 0)
		dObservedROCCurveArea /= nFalsePositive * 1.0 * nTruePositive;
	// Cas particulier ou il n'y a aucun faux-positif
	else if (nFalsePositive == 0)
		dObservedROCCurveArea = 1;
	dEvaluation = dObservedROCCurveArea;

	// Trace pour les calculs totaux
	if (bLocalTrace)
		cout << "EvaluatedAUC\t" << dEvaluation << endl;

	return dEvaluation;
}

void KWAucEvaluation::ComputeLiftCurveAt(int nTargetValueIndex, int nPartileNumber, DoubleVector* dvLiftValues)
{
	KWClassifierInstanceEvaluation* instanceEvaluation;
	int nTarget;
	double dSizeStep;
	int nFirstStep;
	int nLastStep;
	int nStep;
	int nInstance;
	int nTargetValueFrequency;
	int nCorrectInstanceNumber;
	int nBlockTrueInstanceNumber;
	int nBlockFirstInstanceIndex;
	Continuous cBlockScore;
	int nModalityIndex;
	Continuous cScore;
	double dLiftValue;

	require(oaInstanceEvaluations != NULL);
	require(0 <= nTargetValueIndex and nTargetValueIndex < nTargetValueNumber);
	require(nPartileNumber > 0);

	// Initialisation du vecteur de valeurs de lift
	dSizeStep = 1.0 / nPartileNumber;
	dvLiftValues->SetSize(nPartileNumber + 1);

	// Tri des scores pour cette modalite
	SortInstanceEvaluationsAt(nTargetValueIndex);

	// Memorisation du nombre d'instances correspondant a cette modalite cible
	nTargetValueFrequency = 0;
	for (nInstance = 0; nInstance < oaInstanceEvaluations->GetSize(); nInstance++)
	{
		instanceEvaluation = cast(KWClassifierInstanceEvaluation*, oaInstanceEvaluations->GetAt(nInstance));
		nTarget = instanceEvaluation->GetActualTargetIndex();
		assert(0 <= nTargetValueIndex and nTargetValueIndex <= GetTargetValueNumber());
		if (nTarget == nTargetValueIndex)
			nTargetValueFrequency++;
	}

	// Cas particulier d'une modalite de frequence nulle
	if (nTargetValueFrequency == 0)
	{
		for (nStep = 1; nStep < dvLiftValues->GetSize(); nStep++)
			dvLiftValues->SetAt(nStep, 1);
	}
	// Cas standard
	else
	{
		// Parcours des instances
		// On tiens compte des scores a egalite, pour lisser la courbe de lift et
		// la rendre independante de l'ordre de presentation des exemples
		nCorrectInstanceNumber = 0;
		nBlockTrueInstanceNumber = 0;
		nBlockFirstInstanceIndex = 0;
		cBlockScore = 0;
		for (nInstance = 0; nInstance < oaInstanceEvaluations->GetSize(); nInstance++)
		{
			instanceEvaluation =
			    cast(KWClassifierInstanceEvaluation*, oaInstanceEvaluations->GetAt(nInstance));

			// Acces aux valeurs
			cScore = instanceEvaluation->GetTargetProbAt(nTargetValueIndex);
			nModalityIndex = instanceEvaluation->GetActualTargetIndex();

			// Memorisation de la premiere valeur de lift
			if (nInstance == 0)
				cBlockScore = cScore;

			// Test si meme valeur de lift
			if (cScore == cBlockScore)
			{
				// Incrementation du nombre d'instances correctes du bloc
				if (nModalityIndex == nTargetValueIndex)
					nBlockTrueInstanceNumber++;
			}
			// Sinon, changement de bloc
			else
			{
				// Calcul des bornes des index de la courbe de lift inclus dans le bloc
				nFirstStep = (int)floor(
				    (nBlockFirstInstanceIndex * 1.0 / oaInstanceEvaluations->GetSize()) / dSizeStep);
				nLastStep = (int)ceil((nInstance * 1.0 / oaInstanceEvaluations->GetSize()) / dSizeStep);

				// Calcul du lift pour les index inclus dans le bloc
				for (nStep = nFirstStep; nStep <= nLastStep; nStep++)
				{
					// Calcul si index completement inclus dans le bloc
					if (nBlockFirstInstanceIndex <=
						nStep * dSizeStep * oaInstanceEvaluations->GetSize() and
					    nStep * dSizeStep * oaInstanceEvaluations->GetSize() <= nInstance)
					{
						dLiftValue = (nCorrectInstanceNumber +
							      (nStep * dSizeStep * oaInstanceEvaluations->GetSize() -
							       nBlockFirstInstanceIndex) *
								  nBlockTrueInstanceNumber /
								  (nInstance - nBlockFirstInstanceIndex)) /
							     nTargetValueFrequency;
						assert(0 <= dLiftValue and dLiftValue <= 1);
						dvLiftValues->SetAt(nStep, dLiftValue);
					}
				}

				// Initialisation d'un nouveau bloc
				nCorrectInstanceNumber += nBlockTrueInstanceNumber;
				nBlockTrueInstanceNumber = 0;
				if (nModalityIndex == nTargetValueIndex)
					nBlockTrueInstanceNumber++;
				nBlockFirstInstanceIndex = nInstance;
				cBlockScore = cScore;
			}
		}

		// Prise en compte de la fin de la courbe de lift, en partant de la fin
		nStep = nPartileNumber;
		while (nStep >= 0)
		{
			// Calcul si index completement inclus dans le bloc
			if (nBlockFirstInstanceIndex <= nStep * dSizeStep * oaInstanceEvaluations->GetSize())
			{
				dLiftValue =
				    (nCorrectInstanceNumber +
				     (nStep * dSizeStep * oaInstanceEvaluations->GetSize() - nBlockFirstInstanceIndex) *
					 nBlockTrueInstanceNumber / (nInstance - nBlockFirstInstanceIndex)) /
				    nTargetValueFrequency;
				assert(0 <= dLiftValue and dLiftValue <= 1);
				dvLiftValues->SetAt(nStep, dLiftValue);
			}
			else
				break;
			nStep--;
		}

		// On remplit les eventuelles valeurs non renseignees en partant du debut
		for (nStep = 1; nStep <= nPartileNumber; nStep++)
		{
			if (dvLiftValues->GetAt(nStep) == 0)
				dvLiftValues->SetAt(nStep, dvLiftValues->GetAt(nStep - 1));
			assert(dvLiftValues->GetAt(nStep) >= dvLiftValues->GetAt(nStep - 1));
		}
	}
}

void KWAucEvaluation::SortInstanceEvaluationsAt(int nTargetValueIndex)
{
	KWClassifierInstanceEvaluation* instance;
	int nInstanceIndex;

	require(oaInstanceEvaluations != NULL);
	require(0 <= nTargetValueIndex and nTargetValueIndex < GetTargetValueNumber());

	// On attribue l'oppose de la proba a la valeur de tri des instances pour un tri decroissant
	for (nInstanceIndex = 0; nInstanceIndex < oaInstanceEvaluations->GetSize(); nInstanceIndex++)
	{
		instance = cast(KWClassifierInstanceEvaluation*, oaInstanceEvaluations->GetAt(nInstanceIndex));
		instance->SetSortValue(-instance->GetTargetProbAt(nTargetValueIndex));
	}
	oaInstanceEvaluations->SetCompareFunction(KWClassifierInstanceEvaluationCompare);
	oaInstanceEvaluations->Sort();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWReservoirSampler

KWReservoirSampler::KWReservoirSampler()
{
	lRandomSeed = 0;
	nCapacity = 0;
	lSeenObjectNumber = 0;
}

KWReservoirSampler::~KWReservoirSampler()
{
	oaSampledObjects.DeleteAll();
}

void KWReservoirSampler::SetCapacity(int nSampleSize)
{
	require(nSampleSize >= 0);
	nCapacity = nSampleSize;
}

int KWReservoirSampler::GetCapacity()
{
	return nCapacity;
}

void KWReservoirSampler::SetSamplerRandomSeed(longint lSomeRandomState)
{
	require(lSomeRandomState >= 0);
	lRandomSeed = lSomeRandomState;
}

longint KWReservoirSampler::GetSamplerRandomSeed() const
{
	return lRandomSeed;
}

longint KWReservoirSampler::RandomLongint(longint lMax)
{
	longint lRightBound;
	longint lRemainder;
	longint lRandom;

	require(lMax >= 0);

	// Echantillonnage non-biase dans le rang [0, lMax]
	lRightBound = lMax + 1;
	lRemainder = LLONG_MAX % lRightBound;
	lRandom = IthRandomLongint(lRandomSeed++);
	if (lRandom < 0)
		lRandom = -lRandom;
	while (lRandom >= LLONG_MAX - lRemainder)
	{
		lRandom = IthRandomLongint(lRandomSeed++);
		if (lRandom < 0)
			lRandom = -lRandom;
	}

	return lRandom % lRightBound;
}

int KWReservoirSampler::GetSampleSize() const
{
	return oaSampledObjects.GetSize();
}

longint KWReservoirSampler::GetSeenObjectNumber() const
{
	return lSeenObjectNumber;
}

Object* KWReservoirSampler::GetSampledObjectAt(int nIndex) const
{
	return oaSampledObjects.GetAt(nIndex);
}

ObjectArray* KWReservoirSampler::GetSampledObjects()
{
	return &oaSampledObjects;
}

void KWReservoirSampler::Add(Object* object)
{
	longint lRandomIndex;

	require(object != NULL);
	require(nCapacity > 0);
	require(oaSampledObjects.GetSize() <= nCapacity);

	lSeenObjectNumber++;

	// 1) si le reservoir n'est pas rempli, on ajoute une evaluation (nValueIndex == Size)
	if (oaSampledObjects.GetSize() < nCapacity)
		oaSampledObjects.Add(object);
	// 2) sinon, on tire au hasard une position d'insertion (RandomIndex) de la valeur
	else
	{
		lRandomIndex = RandomLongint(lSeenObjectNumber - 1);

		// 2.a) on memorise l'evaluation si l'index est valide pour le tableau
		if (lRandomIndex < (longint)nCapacity)
		{
			delete oaSampledObjects.GetAt((int)lRandomIndex);
			oaSampledObjects.SetAt((int)lRandomIndex, object);
		}
		// 2.b) sinon, on l'efface
		else
			delete object;
	}

	ensure(oaSampledObjects.GetSize() <= nCapacity);
}

void KWReservoirSampler::AddWithProb(Object* object, double dProb)
{
	require(0 <= dProb and dProb <= 1);

	if (RandomDouble() <= dProb)
		Add(object);
}

void KWReservoirSampler::AddArray(const ObjectArray* oaObjects)
{
	int nObject;

	require(oaObjects != NULL);
	require(nCapacity > 0);

	for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
		Add(oaObjects->GetAt(nObject));
}

void KWReservoirSampler::AddArrayWithProb(const ObjectArray* oaObjects, double dProb)
{
	int nObject;

	require(oaObjects != NULL);
	require(0.0 <= dProb and dProb <= 1.0);
	require(nCapacity > 0);

	if (dProb == 1.0)
		AddArray(oaObjects);
	else
	{
		for (nObject = 0; nObject < oaObjects->GetSize(); nObject++)
			AddWithProb(oaObjects->GetAt(nObject), dProb);
	}
}

void KWReservoirSampler::Resample(double dProb)
{
	int nRandomSeed;
	int nObject;
	Object* object;
	int nNewSampleSize;

	require(0.0 <= dProb and dProb <= 1.0);

	// On n'agit que dans les cas non triviaux
	if (GetSampleSize() > 0 and dProb < 1.0)
	{
		// Estimation de la taille esperee apres echantillonnage
		nNewSampleSize = int(ceil(dProb * GetSampleSize()));

		// Si la taille espere est plus petite alors on procede au reechantillonnage
		if (nNewSampleSize < GetSampleSize())
		{
			// Randomisation de l'echantillon
			nRandomSeed = GetRandomSeed();
			SetRandomSeed(int(GetSamplerRandomSeed() % INT_MAX));
			oaSampledObjects.Shuffle();
			SetRandomSeed(nRandomSeed);

			// Retaillage du tableau d'echantillonnage a la taille esperee
			for (nObject = nNewSampleSize; nObject < GetSampleSize(); nObject++)
			{
				object = oaSampledObjects.GetAt(nObject);
				delete object;
			}
			oaSampledObjects.SetSize(nNewSampleSize);
		}
	}
}

void KWReservoirSampler::RemoveAll()
{
	oaSampledObjects.RemoveAll();
	lSeenObjectNumber = 0;
}

void KWReservoirSampler::DeleteAll()
{
	oaSampledObjects.DeleteAll();
	lSeenObjectNumber = 0;
}

boolean KWReservoirSampler::IsOverflowed()
{
	return (lSeenObjectNumber > (longint)nCapacity);
}

const ALString KWReservoirSampler::GetClassLabel() const
{
	return "Reservoir Sampler";
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe PLShared_ClassifierInstanceEvaluation

PLShared_ClassifierInstanceEvaluation::PLShared_ClassifierInstanceEvaluation() {}

PLShared_ClassifierInstanceEvaluation::~PLShared_ClassifierInstanceEvaluation() {}
void PLShared_ClassifierInstanceEvaluation::SetClassifierInstanceEvaluation(KWClassifierInstanceEvaluation* evaluation)
{
	require(evaluation != NULL);
	SetObject(evaluation);
}

KWClassifierInstanceEvaluation* PLShared_ClassifierInstanceEvaluation::GetClassifierInstanceEvaluation()
{
	return cast(KWClassifierInstanceEvaluation*, GetObject());
}

void PLShared_ClassifierInstanceEvaluation::SerializeObject(PLSerializer* serializer, const Object* object) const
{
	KWClassifierInstanceEvaluation* evaluation;
	PLShared_ContinuousVector scvHelper;

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());
	require(object != NULL);

	// Serialisation du nombre de modalites cibles, l'index de la reelle et les probas de chacune
	evaluation = cast(KWClassifierInstanceEvaluation*, object);
	scvHelper.SerializeObject(serializer, &(evaluation->cvTargetProbs));
	serializer->PutInt(evaluation->GetActualTargetIndex());
}

void PLShared_ClassifierInstanceEvaluation::DeserializeObject(PLSerializer* serializer, Object* oObject) const
{
	KWClassifierInstanceEvaluation* evaluation;
	PLShared_ContinuousVector scvHelper;

	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	require(oObject != NULL);

	// Deserialisation du nombre de modalites cibles, l'index de la reelle et les probas de chacune
	evaluation = cast(KWClassifierInstanceEvaluation*, oObject);
	scvHelper.DeserializeObject(serializer, &(evaluation->cvTargetProbs));
	evaluation->SetActualTargetIndex(serializer->GetInt());
}

Object* PLShared_ClassifierInstanceEvaluation::Create() const
{
	return new KWClassifierInstanceEvaluation;
}
