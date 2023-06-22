// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorEvaluationTask.h"

////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////
// Classe KWClassifierEvaluationTask

KWClassifierEvaluationTask::KWClassifierEvaluationTask()
{
	// Initialisation des variables du maitre
	classifierEvaluation = NULL;
	masterConfMatrixEvaluation = NULL;
	aucEvaluation = NULL;
	instanceEvaluationSampler = NULL;
	nNextEvaluationBatch = -1;
	bIsAucEvaluated = false;
	bIsAucFromSubsample = false;

	// Initialisation des variables de l'esclave
	slaveConfMatrixEvaluation = NULL;
	nSlaveInstanceEvaluationCapacity = 0;

	// Initialisation des variables partagees
	DeclareTaskInput(&input_bIsAucEvaluated);
	DeclareSharedParameter(&shared_liTargetAttribute);
	DeclareSharedParameter(&shared_liPredictionAttribute);
	DeclareSharedParameter(&shared_livProbAttributes);
	DeclareSharedParameter(&shared_nTargetValueNumber);
	DeclareSharedParameter(&shared_svPredictedModalities);
	DeclareTaskOutput(&output_confusionMatrix);
	DeclareTaskOutput(&output_dCompressionRate);
	DeclareTaskOutput(&output_bAreSlaveInstanceEvaluationsOverflowed);
	DeclareTaskOutput(&output_nSlaveCapacityOverflowSize);
	output_slaveEvaluationInstances = new PLShared_ObjectArray(new PLShared_ClassifierInstanceEvaluation);
	DeclareTaskOutput(output_slaveEvaluationInstances);
}

KWClassifierEvaluationTask::~KWClassifierEvaluationTask()
{
	assert(masterConfMatrixEvaluation == NULL);
	assert(slaveConfMatrixEvaluation == NULL);
	assert(aucEvaluation == NULL);
	assert(instanceEvaluationSampler == NULL);
	assert(oaSlaveEvaluationBatches.GetSize() == 0);

	// Nettoyage du tableau de sortie des esclaves (force a etre en reference)
	delete output_slaveEvaluationInstances;
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

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::ComputeResourceRequirements();

	// Estimation du nombre total d'instance de la base en tenant compte du taux d'achantillonnage
	lEstimatedTotalObjectNumber =
	    shared_sourceDatabase.GetPLDatabase()->GetDatabase()->GetSampleEstimatedObjectNumber();

	// Estimation de la memoire totale necessaire pour le calcul de toutes les courbes de lift
	lMaxRequiredEvaluationMemory =
	    lEstimatedTotalObjectNumber * ComputeInstanceEvaluationNecessaryMemory() + lMB / 2;

	// Estimation de la memoire totale restante pour en donner au max la moitie au maitre
	lRemainingMemory = RMResourceManager::GetRemainingAvailableMemory();

	// On diminue potentiellement la memoire max du maitre, en tenant compte de la memoire disponible restante
	lMaxMasterMemoryRequirement = min(2 * lGB, 32 * lMB + lRemainingMemory / 2);
	lMaxMasterMemoryRequirement = min(lMaxMasterMemoryRequirement, 2 * lMaxRequiredEvaluationMemory);

	// Memoire Maitre: on demande un min et un max "raisonnable" permettant d'avoir une
	// tres bonne estimation des courbes de lift, du critere d'AUC, avec dans le pire des cas
	// un temps de calcul raisonnable. Ce ne serait pas raisonnable de trier des tres
	// nombreux vecteurs de score de tres grande taille, pour gagner une precision
	// supplementaire negligeable par rapport a la variance des resultats.
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->UpgradeMin(
	    min(32 * lMB, lMaxRequiredEvaluationMemory));
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->UpgradeMax(lMaxMasterMemoryRequirement);

	// Memoire esclave: meme logique que pour le maitre, et un peu moins pour le max
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMin(
	    min(32 * lMB, lMaxRequiredEvaluationMemory));
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMax(
	    min(lGB / 2, 2 * lMaxRequiredEvaluationMemory));

	// En priorite, on attribut la memoire au maitre, qui collecte l'ensemble des scores
	// calcules par les esclaves
	GetResourceRequirements()->SetMemoryAllocationPolicy(RMTaskResourceRequirement::masterPreferred);
	return bOk;
}

boolean KWClassifierEvaluationTask::MasterInitialize()
{
	boolean bOk;
	int i;
	longint lInstanceEvaluationSize;
	longint lGrantedMemory;
	ALString sTmp = "";

	require(masterConfMatrixEvaluation == NULL);
	require(slaveConfMatrixEvaluation == NULL);
	require(aucEvaluation == NULL);
	require(instanceEvaluationSampler == NULL);

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::MasterInitialize();

	// Memorisation de la specialisation du rapport d'evaluation demandeur
	classifierEvaluation = cast(KWClassifierEvaluation*, predictorEvaluation);

	// Initialisation du service d'evaluation de la matrice de confusion
	masterConfMatrixEvaluation = new KWConfusionMatrixEvaluation;
	masterConfMatrixEvaluation->Initialize();
	for (i = 0; i < shared_nTargetValueNumber; i++)
		masterConfMatrixEvaluation->AddPredictedTarget(shared_svPredictedModalities.GetAt(i));

	// Initialisation des services d'echantillonage et de calcul de l'AUC
	bIsAucFromSubsample = false;
	bIsAucEvaluated = shared_livProbAttributes.GetSize() > 0 and shared_nTargetValueNumber > 0;
	nNextEvaluationBatch = 0;
	instanceEvaluationSampler = new KWReservoirSampler;
	aucEvaluation = new KWAucEvaluation;
	aucEvaluation->SetTargetValueNumber(shared_nTargetValueNumber);

	// Dimensionnement de la capacite du reservoir sampler
	lInstanceEvaluationSize = ComputeInstanceEvaluationNecessaryMemory();
	lGrantedMemory = GetMasterResourceGrant()->GetMemory();
	if (lGrantedMemory / lInstanceEvaluationSize < INT_MAX)
		instanceEvaluationSampler->SetCapacity((int)(lGrantedMemory / lInstanceEvaluationSize));
	else
		instanceEvaluationSampler->SetCapacity(INT_MAX);

	// Initialisation des courbes de lift pour l'ensemble des modalites
	assert(classifierEvaluation->oaAllLiftCurveValues.GetSize() == 0);
	for (i = 0; i < shared_nTargetValueNumber; i++)
	{
		// Arret et warning si le maximum de courbes est atteint
		if (i == nMaxLiftEvaluationNumber)
		{
			AddWarning(sTmp + "The lift curves will be computed only for " +
				   IntToString(nMaxLiftEvaluationNumber) + " values (among " +
				   IntToString(shared_nTargetValueNumber) + ")");
			break;
		}
		classifierEvaluation->oaAllLiftCurveValues.Add(new DoubleVector);
	}

	// Verification de l'initialisation
	assert(Check());

	return bOk;
}

boolean KWClassifierEvaluationTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	boolean bOk;

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::MasterPrepareTaskInput(dTaskPercent, bIsTaskFinished);

	// Ajout d'une reference nulle dans la place du batch pour les evaluations de l'esclave en cours
	oaSlaveEvaluationBatches.Add(NULL);
	input_bIsAucEvaluated = bIsAucEvaluated;
	return bOk;
}

boolean KWClassifierEvaluationTask::MasterAggregateResults()
{
	boolean bOk;
	ObjectArray* oaSlaveEvaluations;
	int i;
	ALString sTmp = "";

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::MasterAggregateResults();

	// Ajout des evaluations du esclave au celui du maitre
	masterConfMatrixEvaluation->AddEvaluatedMatrix(output_confusionMatrix.GetDataGridStats());
	classifierEvaluation->dCompressionRate += output_dCompressionRate;

	// Si les esclaves overflowent, l'AUC n'est plus evaluee
	// On nettoie les batchs et met un warning a l'utilisateur
	if (bIsAucEvaluated and output_bAreSlaveInstanceEvaluationsOverflowed)
	{
		// Nettoyage des donnees collectees en cours pour l'AUC
		bIsAucEvaluated = false;
		for (i = 0; i < oaSlaveEvaluationBatches.GetSize(); i++)
		{
			oaSlaveEvaluations = cast(ObjectArray*, oaSlaveEvaluationBatches.GetAt(i));
			if (oaSlaveEvaluations != NULL)
				oaSlaveEvaluations->DeleteAll();
		}
		output_slaveEvaluationInstances->GetObjectArray()->DeleteAll();
		instanceEvaluationSampler->DeleteAll();

		// Warning utilsateur (les autres criteres sont toujours calcules)
		AddWarning(sTmp + "Not enough memory in slave to compute AUC (needs extra " +
			   RMResourceManager::ActualMemoryToString(output_nSlaveCapacityOverflowSize *
								   ComputeInstanceEvaluationNecessaryMemory()) +
			   ") (maybe too many modalities in target)");
	}

	// Si l'AUC est evalue, on echantillonne les evaluations d'instance
	// Ceci se fait dans l'ordre d'expedition des esclaves via le tableau de batches
	if (bIsAucEvaluated)
	{
		// On transfert les instances d'evaluation de l'esclave en cours au tableau de batches
		// Memoire: Responsabilite transferee aux tableaux de oaSlaveInstancesBatches
		assert(oaSlaveEvaluationBatches.GetAt(GetTaskIndex()) == NULL);
		oaSlaveEvaluations = new ObjectArray;
		oaSlaveEvaluations->CopyFrom(output_slaveEvaluationInstances->GetObjectArray());
		oaSlaveEvaluationBatches.SetAt(GetTaskIndex(), oaSlaveEvaluations);
		output_slaveEvaluationInstances->GetObjectArray()->RemoveAll();

		// Echantillonage d'instances d'evaluation du tableau de batches au sampler du maitre
		// Ceci se fait en respectant l'ordre ou les esclaves ont ete expedies
		// Memoire: Responsabilite transferee vers masterInstanceSampler
		while (nNextEvaluationBatch < oaSlaveEvaluationBatches.GetSize())
		{
			oaSlaveEvaluations = cast(ObjectArray*, oaSlaveEvaluationBatches.GetAt(nNextEvaluationBatch));

			// Si le prochain batch a traiter est NUL => il n'est pas pret => on pause le echantillonage
			if (oaSlaveEvaluations == NULL)
			{
				break;
			}
			else
			{
				instanceEvaluationSampler->AddArray(oaSlaveEvaluations);
				oaSlaveEvaluations->RemoveAll();
				nNextEvaluationBatch++;
			}
		}

		// Warning la premiere fois que l'on atteint la capacite maximale du sampler
		if (instanceEvaluationSampler->IsOverflowed() and not bIsAucFromSubsample)
		{
			bIsAucFromSubsample = true;
			AddWarning(
			    sTmp +
			    "Not enough memory to compute the exact AUC (estimation made on a sub-sample of size " +
			    IntToString(instanceEvaluationSampler->GetCapacity()) + ")");
		}
	}

	return bOk;
}

boolean KWClassifierEvaluationTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	const int nPartileNumber = 1000;
	boolean bOk;
	int i;
	int nPredictorTarget;
	DoubleVector* dvLiftCurveValues;

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
				classifierEvaluation->dCompressionRate = 0;
		}

		// Calcul de l'AUC
		if (bIsAucEvaluated)
		{
			aucEvaluation->SetInstanceEvaluations(instanceEvaluationSampler->GetSampledObjects());
			if (shared_livProbAttributes.GetSize() > 0 and aucEvaluation->GetTargetValueNumber() > 0)
				classifierEvaluation->dAUC = aucEvaluation->ComputeGlobalAUCValue();

			// Calcul des courbes de lift
			for (i = 0; i < classifierEvaluation->oaAllLiftCurveValues.GetSize(); i++)
			{
				dvLiftCurveValues =
				    cast(DoubleVector*, classifierEvaluation->oaAllLiftCurveValues.GetAt(i));

				// L'index de lift de la modalite est celui de la modalite directement, sauf si la
				// derniere courbe memorise la courbe pour la modalite cible principale
				nPredictorTarget = GetPredictorTargetIndexAtLiftCurveIndex(i);

				// Si l'on a avant la modalite cible est au debut
				aucEvaluation->ComputeLiftCurveAt(nPredictorTarget, nPartileNumber, dvLiftCurveValues);
			}
		}
	}

	// Nettoyage
	delete masterConfMatrixEvaluation;
	delete aucEvaluation;
	delete instanceEvaluationSampler;
	masterConfMatrixEvaluation = NULL;
	aucEvaluation = NULL;
	instanceEvaluationSampler = NULL;
	oaSlaveEvaluationBatches.DeleteAll();
	classifierEvaluation = NULL;

	return bOk;
}

boolean KWClassifierEvaluationTask::SlaveInitialize()
{
	boolean bOk;
	longint lInstanceEvaluationSize;
	longint lGrantedMemory;

	require(slaveConfMatrixEvaluation == NULL);

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::SlaveInitialize();

	// Initialisation pour l'evaluation
	slaveConfMatrixEvaluation = new KWConfusionMatrixEvaluation;

	// Dimensionnement de la capacite du vecteur de score de l'esclave
	lInstanceEvaluationSize = ComputeInstanceEvaluationNecessaryMemory();
	lGrantedMemory =
	    ComputeSlaveGrantedMemory(GetSlaveResourceRequirement(), GetSlaveResourceGrant()->GetMemory(), false);
	if (lGrantedMemory / lInstanceEvaluationSize < INT_MAX)
		nSlaveInstanceEvaluationCapacity = (int)(lGrantedMemory / lInstanceEvaluationSize);
	else
		nSlaveInstanceEvaluationCapacity = INT_MAX;
	return bOk;
}

boolean KWClassifierEvaluationTask::SlaveProcessExploitDatabase()
{
	boolean bOk;
	longint lCapacityOverflowSize;

	// Initialisation des resultats de l'esclave
	output_dCompressionRate = 0;
	slaveConfMatrixEvaluation->Initialize();
	output_bAreSlaveInstanceEvaluationsOverflowed = false;
	output_nSlaveCapacityOverflowSize = 0;

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::SlaveProcessExploitDatabase();

	// En cas d'overflow, on calcule sa taille pour la renvoyer au maitre
	if (output_bAreSlaveInstanceEvaluationsOverflowed)
	{
		lCapacityOverflowSize = output_lReadObjects - nSlaveInstanceEvaluationCapacity;
		if (lCapacityOverflowSize > INT_MAX)
			output_nSlaveCapacityOverflowSize = INT_MAX;
		else
			output_nSlaveCapacityOverflowSize = (int)lCapacityOverflowSize;
	}

	// Remplisement des sorties de l'esclave
	slaveConfMatrixEvaluation->ExportDataGridStats(output_confusionMatrix.GetDataGridStats());

	return bOk;
}

boolean KWClassifierEvaluationTask::SlaveProcessExploitDatabaseObject(const KWObject* kwoObject)
{
	const Continuous cEpsilon = (Continuous)1e-6;
	boolean bOk;
	int nActualValueIndex;
	int i;
	Symbol sActualTargetValue;
	Symbol sPredictedTargetValue;
	Continuous cActualTargetValueProb;
	KWClassifierInstanceEvaluation* instanceEvaluation;

	require(kwoObject != NULL);

	// Appel de la methode ancetre
	bOk = KWPredictorEvaluationTask::SlaveProcessExploitDatabaseObject(kwoObject);

	// Obtention des modalites predites et effectives
	assert(shared_liTargetAttribute.GetValue().IsValid());
	assert(shared_liPredictionAttribute.GetValue().IsValid());
	sActualTargetValue = kwoObject->GetSymbolValueAt(shared_liTargetAttribute.GetValue());
	sPredictedTargetValue = kwoObject->GetSymbolValueAt(shared_liPredictionAttribute.GetValue());

	// Mise a jour de la matrice de confusion
	slaveConfMatrixEvaluation->AddInstanceEvaluation(sPredictedTargetValue, sActualTargetValue);

	// Recherche de l'index en apprentissage de la modalite effective
	// Par defaut: le nombre de modalites cible en apprentissage
	// (signifie valeur cible inconnue en apprentissage)
	nActualValueIndex = shared_nTargetValueNumber;
	if (shared_livProbAttributes.GetSize() > 0)
	{
		for (i = 0; i < shared_nTargetValueNumber; i++)
		{
			if (shared_svPredictedModalities.GetAt(i) == sActualTargetValue)
			{
				nActualValueIndex = i;
				break;
			}
		}
	}

	// Mise a jour du taux de compression si pertinente
	if (shared_livProbAttributes.GetSize() > 0)
	{
		// Recherche de la probabilite predite pour la valeur cible reelle
		cActualTargetValueProb = 0;
		if (nActualValueIndex < shared_livProbAttributes.GetSize())
		{
			assert(kwoObject->GetSymbolValueAt(shared_liTargetAttribute.GetValue()) ==
			       shared_svPredictedModalities.GetAt(nActualValueIndex));
			cActualTargetValueProb =
			    kwoObject->GetContinuousValueAt(shared_livProbAttributes.GetAt(nActualValueIndex));

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
	if (input_bIsAucEvaluated and not output_bAreSlaveInstanceEvaluationsOverflowed)
	{
		// On arrete tout si l'on a va depasser la capacite de l'esclave
		if (output_slaveEvaluationInstances->GetObjectArray()->GetSize() == nSlaveInstanceEvaluationCapacity)
		{
			output_slaveEvaluationInstances->GetObjectArray()->DeleteAll();
			output_bAreSlaveInstanceEvaluationsOverflowed = true;
		}
		// Sinon, on ajoute l'instance
		else
		{
			instanceEvaluation = new KWClassifierInstanceEvaluation;
			instanceEvaluation->SetTargetValueNumber(shared_nTargetValueNumber);
			instanceEvaluation->SetActualTargetIndex(nActualValueIndex);
			for (i = 0; i < shared_nTargetValueNumber; i++)
			{
				instanceEvaluation->SetTargetProbAt(
				    i, kwoObject->GetContinuousValueAt(shared_livProbAttributes.GetAt(i)));
			}
			output_slaveEvaluationInstances->GetObjectArray()->Add(instanceEvaluation);
		}
	}

	return bOk;
}

boolean KWClassifierEvaluationTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	// Appel a la methode ancetre
	bOk = KWPredictorEvaluationTask::SlaveFinalize(bProcessEndedCorrectly);

	// Nettoyage
	nSlaveInstanceEvaluationCapacity = 0;
	delete slaveConfMatrixEvaluation;
	slaveConfMatrixEvaluation = NULL;
	return bOk;
}

void KWClassifierEvaluationTask::InitializePredictorSharedVariables(KWPredictor* predictor)
{
	int nTargetIndex;
	KWTrainedClassifier* classifier;
	KWLoadIndex liTarget;
	KWLoadIndex liPrediction;
	KWLoadIndex liProbAttribute;
	ALString sTmp = "";

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

	// Warning si pas de modalites cibles specifiees
	if (shared_nTargetValueNumber == 0)
	{
		AddWarning(sTmp + "The AUC value will not be evaluated " +
			   "as the target value probabilities are not available from the predictor");
	}
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

int KWClassifierEvaluationTask::GetPredictorTargetIndexAtLiftCurveIndex(int nLiftCurveIndex) const
{
	require(classifierEvaluation != NULL);
	require(0 <= nLiftCurveIndex and nLiftCurveIndex < GetComputedLiftCurveNumber());

	if (nLiftCurveIndex == classifierEvaluation->oaAllLiftCurveValues.GetSize() - 1 and
	    GetMainTargetModalityLiftIndex() == classifierEvaluation->oaAllLiftCurveValues.GetSize() - 1)
		return classifierEvaluation->GetMainTargetModalityIndex();
	else
		return nLiftCurveIndex;
}

longint KWClassifierEvaluationTask::ComputeInstanceEvaluationNecessaryMemory() const
{
	return sizeof(KWClassifierInstanceEvaluation) + sizeof(KWClassifierInstanceEvaluation*) +
	       shared_nTargetValueNumber * sizeof(Continuous);
}

////////////////////////////////////////////////////////////////////////////////
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
	GetResourceRequirements()->SetMemoryAllocationPolicy(RMTaskResourceRequirement::masterPreferred);
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
	int i;

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
	// On nettoie les batchs et met un warning a l'utilisateur
	if (bIsRecCurveCalculated and output_bIsRecCurveVectorOverflowed)
	{
		// Nettoyage des donnees collectees en cours pour l'AUC
		bIsRecCurveCalculated = false;
		dvRankAbsoluteErrors.SetSize(0);

		// Warning utilisteur (les autres criteres sont toujours calcules)
		AddWarning("Not enough memory in slave to compute REC curve (needs extra " +
			   RMResourceManager::ActualMemoryToString(output_nSlaveCapacityOverflowSize * sizeof(double)) +
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
			AddWarning(
			    "Not enough memory in master to compute REC curve (needs extra " +
			    RMResourceManager::ActualMemoryToString(nMasterCapacityOverflowSize * sizeof(double)) +
			    ")");

			// Nettoyage
			dvRankAbsoluteErrors.SetSize(0);
			bIsRecCurveCalculated = false;
		}
		// Collecte des erreur veannt de l'esclave
		else
		{
			for (i = 0; i < output_dvRankAbsoluteErrors.GetSize(); i++)
				dvRankAbsoluteErrors.Add(output_dvRankAbsoluteErrors.GetAt(i));
		}
	}
	return bOk;
}

boolean KWRegressorEvaluationTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	const int nRankRECPartileNumber = 1000;
	boolean bOk;
	int i;
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
					i = 0;
					for (nPartile = 1; nPartile <= nRankRECPartileNumber; nPartile++)
					{
						dRankAbsoluteErrorThreshold = nPartile * 1.0 / nRankRECPartileNumber;

						// Calcul du nombre d'instance ayant une erreur inferieure au seuil
						while (i < dvRankAbsoluteErrors.GetSize() and
						       dvRankAbsoluteErrors.GetAt(i) <= dRankAbsoluteErrorThreshold)
							i++;

						// Memorisation de la valeur de la courbe de REC
						regressorEvaluation->dvRankRECCurveValues.SetAt(
						    nPartile, (1.0 * i) / dvRankAbsoluteErrors.GetSize());
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

	// Initialisation des resultats de l'eclave
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

////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////
// Classe KWAucEvaluation

int KWClassifierEvaluationInstanceCompare(const void* elem1, const void* elem2)
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
	int nTargetValueIndex;
	IntVector ivTargetValueFrequencies;
	int nUnknownTargetValueIndex;
	KWClassifierInstanceEvaluation* evaluationInstance;
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
		nUnknownTargetValueIndex = GetTargetValueNumber();
		for (nInstance = 0; nInstance < oaInstanceEvaluations->GetSize(); nInstance++)
		{
			evaluationInstance =
			    cast(KWClassifierInstanceEvaluation*, oaInstanceEvaluations->GetAt(nInstance));
			nTargetValueIndex = evaluationInstance->GetActualTargetIndex();
			assert(0 <= nTargetValueIndex and nTargetValueIndex <= GetTargetValueNumber());

			// On ignore les instances des classes non vues en apprentissage
			if (nTargetValueIndex < GetTargetValueNumber())
				ivTargetValueFrequencies.UpgradeAt(nTargetValueIndex, 1);
			else
				ivTargetValueFrequencies.UpgradeAt(nUnknownTargetValueIndex, 1);
		}

		// Evaluation: On table sur une AUC de 0.5 pour les valeurs cibles inconnues
		dEvaluation = 0;
		for (nTargetValueIndex = 0; nTargetValueIndex < GetTargetValueNumber(); nTargetValueIndex++)
			dEvaluation +=
			    ivTargetValueFrequencies.GetAt(nTargetValueIndex) * ComputeAUCValueAt(nTargetValueIndex);
		dEvaluation += ivTargetValueFrequencies.GetAt(nUnknownTargetValueIndex) * 0.5;
		dEvaluation /= oaInstanceEvaluations->GetSize();
	}
	return dEvaluation;
}

double KWAucEvaluation::ComputeAUCValueAt(int nTargetValueIndex)
{
	boolean bTrace = false;
	KWClassifierInstanceEvaluation* evaluationInstance;
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
	if (bTrace)
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
		evaluationInstance = cast(KWClassifierInstanceEvaluation*, oaInstanceEvaluations->GetAt(nInstance));

		// Acces aux valeurs
		cScore = evaluationInstance->GetTargetProbAt(nTargetValueIndex);
		nModalityIndex = evaluationInstance->GetActualTargetIndex();

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
			if (bTrace)
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
	if (bTrace)
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
	if (bTrace)
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
	oaInstanceEvaluations->SetCompareFunction(KWClassifierEvaluationInstanceCompare);
	oaInstanceEvaluations->Sort();
}

////////////////////////////////////////////////////////////////////////////////
// Classe KWReservoirSampler

KWReservoirSampler::KWReservoirSampler()
{
	nCapacity = 0;
	nSeenObjects = 0;
}

KWReservoirSampler::~KWReservoirSampler()
{
	oaSampledObjects.DeleteAll();
}

ObjectArray* KWReservoirSampler::GetSampledObjects()
{
	return &oaSampledObjects;
}

void KWReservoirSampler::Add(Object* object)
{
	int nRandomIndex;

	require(object != NULL);
	require(nCapacity > 0);

	nSeenObjects++;
	assert(oaSampledObjects.GetSize() <= nCapacity);

	// 1) si le reservoir n'est pas rempli, on ajoute une evaluation (nValueIndex == Size)
	if (oaSampledObjects.GetSize() < nCapacity)
		oaSampledObjects.Add(object);
	// 2) sinon, on tire au hasard une position d'insertion (RandomIndex) de la valeur
	else
	{
		nRandomIndex = RandomInt(nSeenObjects - 1);

		// 2.a) on memorise l'evaluation si l'index est valide pour le tableau
		if (nRandomIndex < nCapacity)
		{
			delete oaSampledObjects.GetAt(nRandomIndex);
			oaSampledObjects.SetAt(nRandomIndex, object);
		}
		// 2.b) sinon, on l'efface
		else
			delete object;
	}
}

void KWReservoirSampler::AddArray(const ObjectArray* oaObjects)
{
	int i;

	require(oaObjects != NULL);
	require(nCapacity > 0);

	for (i = 0; i < oaObjects->GetSize(); i++)
		Add(oaObjects->GetAt(i));
}

void KWReservoirSampler::RemoveAll()
{
	oaSampledObjects.RemoveAll();
	nSeenObjects = 0;
}

void KWReservoirSampler::DeleteAll()
{
	oaSampledObjects.DeleteAll();
	nSeenObjects = 0;
}

boolean KWReservoirSampler::IsOverflowed()
{
	return (nSeenObjects > nCapacity);
}

const ALString KWReservoirSampler::GetClassLabel() const
{
	return "Reservoir Sampler";
}

////////////////////////////////////////////////////////////////////////////////
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

void PLShared_ClassifierInstanceEvaluation::DeserializeObject(PLSerializer* serializer, Object* object) const
{
	KWClassifierInstanceEvaluation* evaluation;
	PLShared_ContinuousVector scvHelper;

	require(serializer != NULL);
	require(serializer->IsOpenForRead());
	require(object != NULL);

	// Deserialisation du nombre de modalites cibles, l'index de la reelle et les probas de chacune
	evaluation = cast(KWClassifierInstanceEvaluation*, object);
	scvHelper.DeserializeObject(serializer, &(evaluation->cvTargetProbs));
	evaluation->SetActualTargetIndex(serializer->GetInt());
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

Object* PLShared_ClassifierInstanceEvaluation::Create() const
{
	return new KWClassifierInstanceEvaluation;
}