// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SNBPredictorSelectiveNaiveBayesTrainingTask.h"

SNBPredictorSNBTrainingTask::SNBPredictorSNBTrainingTask()
{
	// Initialisation des variables du maitre
	masterSnbPredictor = NULL;
	masterInitialClass = NULL;
	masterInitialDomain = NULL;
	masterInitialDatabase = NULL;
	masterBinarySliceSet = NULL;
	masterRandomAttribute = NULL;
	bMasterIsOnFastAddRun = true;
	dMasterModificationScore = DBL_MAX;
	dMasterModificationDataCost = DBL_MAX;
	dMasterModificationModelCost = DBL_MAX;
	dMasterCurrentScore = DBL_MAX;
	dMasterCurrentModelCost = DBL_MAX;
	dMasterCurrentDataCost = DBL_MAX;
	dMasterEmptySelectionScore = DBL_MAX;
	dMasterEmptySelectionModelCost = DBL_MAX;
	dMasterEmptySelectionDataCost = DBL_MAX;
	dMasterMapScore = DBL_MAX;
	dMasterPreviousRunScore = DBL_MAX;
	nMasterOuterIteration = -1;
	nMasterOuterIterationNumber = -1;
	nMasterFastForwardBackwardRun = -1;
	nMasterRandomAttribute = -1;
	bMasterUndoLastModification = false;
	dMasterPrecisionEpsilon = -1.0;
	bMasterInitializeSlaveScorers = false;
	nMasterRandomSeed = 1;
	nMasterFastRunStepFinishedTaskNumber = 0;
	dMasterTaskProgress = 0;
	bMasterIsTrainingSuccessfulWithoutRunningTask = false;

	// Initialisation des variables de l'esclave
	nSlaveProcessChunkIndex = -1;
	slaveRecoderClass = NULL;
	slaveDummyDatabase = NULL;
	slaveBinarySliceSet = NULL;

	// Declaration des variables partagees
	DeclareSharedParameter(&shared_learningSpec);
	DeclareSharedParameter(&shared_sRecoderClassName);
	DeclareSharedParameter(&shared_sRecoderClassDomainFileURI);
	DeclareSharedParameter(&shared_ivGrantedSlaveProcessIds);
	DeclareSharedParameter(&shared_nInstanceNumber);
	DeclareSharedParameter(&shared_nChunkNumber);
	DeclareSharedParameter(&shared_nAttributeNumber);
	DeclareSharedParameter(&shared_nSliceNumber);
	DeclareSharedParameter(&shared_nInitialAttributeNumber);
	shared_oaBinarySliceSetAttributes = new PLShared_ObjectArray(new PLShared_DataTableBinarySliceSetAttribute);
	DeclareSharedParameter(shared_oaBinarySliceSetAttributes);
	shared_odAttributeStats = new PLShared_ObjectDictionary(new PLShared_AttributeStats);
	DeclareSharedParameter(shared_odAttributeStats);
	shared_odAttributePairStats = new PLShared_ObjectDictionary(new PLShared_AttributePairStats);
	DeclareSharedParameter(shared_odAttributePairStats);
	DeclareSharedParameter(&shared_dataTableSliceSet);
	DeclareSharedParameter(&shared_ivTargetValueIndexes);
	DeclareSharedParameter(&shared_dPriorWeight);
	DeclareSharedParameter(&shared_bIsConstructionCostEnabled);
	DeclareSharedParameter(&shared_bIsPreparationCostEnabled);

	// Declaration des entrees et sortie des taches
	DeclareTaskInput(&input_nModificationAttribute);
	DeclareTaskInput(&input_bIsForwardModification);
	DeclareTaskInput(&input_bUndoLastModification);
	DeclareTaskInput(&input_bInitializeWorkingData);
	DeclareTaskOutput(&output_dDataCost);
}

SNBPredictorSNBTrainingTask::~SNBPredictorSNBTrainingTask()
{
	delete shared_oaBinarySliceSetAttributes;
	delete shared_odAttributeStats;
	delete shared_odAttributePairStats;
}

void SNBPredictorSNBTrainingTask::InternalTrain(SNBPredictorSelectiveNaiveBayes* snbPredictor)
{
	ObjectArray oaAllAttributeStats;
	int nTrainingAttributeNumber;
	KWDataPreparationClass dataPreparationClass;
	Timer timerTraining;
	ALString sTimeString;
	ALString sTmp;

	require(masterSnbPredictor == NULL);
	require(masterInitialClass == NULL);
	require(masterInitialDatabase == NULL);
	require(snbPredictor->GetClassStats() != NULL);
	require(snbPredictor->GetClassStats()->Check());
	require(snbPredictor->GetClassStats()->IsStatsComputed());
	require(snbPredictor->GetClassStats()->GetLearningSpec()->Check());
	require(snbPredictor->GetLearningSpec()->GetTargetAttributeType() == KWType::Symbol or
		snbPredictor->GetLearningSpec()->GetTargetAttributeType() == KWType::Continuous);
	require(snbPredictor->GetClassStats()->GetDataTableSliceSet() != NULL);
	require(snbPredictor->GetClassStats()->GetDataTableSliceSet()->Check());
	require(snbPredictor->GetClassStats()->GetDataTableSliceSet()->GetDeleteFilesAtClean());

	// Sauvegarde du SNBPredictorSelectiveNaiveBayes appelant
	masterSnbPredictor = snbPredictor;

	// Initialisation du flag d'entrainement rapide
	bMasterIsTrainingSuccessfulWithoutRunningTask = false;

	// Entrainement seulement s'il y a des attributs informatifs
	nTrainingAttributeNumber = masterSnbPredictor->GetTrainingAttributeNumber();
	if (nTrainingAttributeNumber > 0)
	{
		// Debut timer apprentissage
		timerTraining.Start();

		// Predicteur univarie s'il n'y a que un attribut informatif ou un seul attribut a evaluer
		if (nTrainingAttributeNumber == 1)
		{
			masterSnbPredictor->InternalTrainUnivariatePredictor();
			bMasterIsTrainingSuccessfulWithoutRunningTask = true;
		}
		// Execution effective de la tache d'entrainement s'il y a plus d'un attribut informatif
		else
		{
			// Sauvegarde de l'etat du KWLearningSpec
			masterInitialClass = snbPredictor->GetClass();
			masterInitialDatabase = snbPredictor->GetDatabase();

			// Deactivation du nettoyage des fichiers du SliceSet
			masterSnbPredictor->GetClassStats()->GetDataTableSliceSet()->SetDeleteFilesAtClean(false);

			// Initialisation d'une partie des variables partagees;
			// Le reste a besoin d'un SNBDataTableBinarySliceSet initialise ce qui n'est possible qu'une
			// fois execute ComputeResourceRequirements et se fait dans le MasterInitialize
			shared_learningSpec.SetLearningSpec(masterSnbPredictor->GetLearningSpec());
			shared_dataTableSliceSet.SetDataTableSliceSet(
			    masterSnbPredictor->GetClassStats()->GetDataTableSliceSet());
			shared_dPriorWeight = masterSnbPredictor->GetSelectionParameters()->GetPriorWeight();
			shared_bIsConstructionCostEnabled =
			    masterSnbPredictor->GetSelectionParameters()->GetConstructionCost();
			shared_bIsPreparationCostEnabled =
			    masterSnbPredictor->GetSelectionParameters()->GetPreparationCost();

			// Execution de la tache
			Run();

			// Nettoyage des variables partagees
			shared_learningSpec.RemoveObject();
			shared_dataTableSliceSet.RemoveObject();

			// Reactivation du nettoyage des fichiers du SliceSet (sa valeur par defaut)
			masterSnbPredictor->GetClassStats()->GetDataTableSliceSet()->SetDeleteFilesAtClean(true);

			// Remise a zero de l'etat du KWLearningSpec
			masterInitialClass = NULL;
			masterInitialDatabase = NULL;
		}

		// On informe le temps d'entrainement ecoule en succes ou pas
		timerTraining.Stop();
		sTimeString = SecondsToString(timerTraining.GetElapsedTime());
		if (IsTrainingSuccessful())
			AddSimpleMessage(masterSnbPredictor->GetPrefix() + " train time: " + sTimeString);
		else if (TaskProgression::IsInterruptionRequested())
			AddSimpleMessage(GetTaskName() + " interrupted by user after " + sTimeString);
		else
			AddSimpleMessage(GetTaskName() + " interrupted because of errors");
	}
	// S'il n'y a pas d'attribut informatif on entraine un predicteur vide
	else
	{
		masterSnbPredictor->InternalTrainEmptyPredictor();
		bMasterIsTrainingSuccessfulWithoutRunningTask = true;
	}

	// Nettoyage du predicteur appelant et du binary slice set
	masterSnbPredictor = NULL;

	ensure(masterSnbPredictor == NULL);
	ensure(masterInitialClass == NULL);
	ensure(masterInitialDatabase == NULL);
}

boolean SNBPredictorSNBTrainingTask::IsTrainingSuccessful() const
{
	return bMasterIsTrainingSuccessfulWithoutRunningTask or IsJobSuccessful();
}

boolean SNBPredictorSNBTrainingTask::ComputeResourceRequirements()
{
	const boolean bLocalTrace = false;
	const int nAbsoluteMaxSlaveProcessNumber = 10000;
	int nMaxSlaveProcessNumber;
	int nMaxSliceNumber;
	longint lSharedMemory;
	longint lMasterMemory;
	longint lGlobalSlaveMinMemory;
	longint lGlobalSlaveMaxMemory;
	longint lSlaveMinProcessesMinSlicesMemory;
	longint lSlaveMaxProcessesMaxSlicesMemory;
	longint lSlaveMemory;
	longint lMasterDisk;
	longint lSlaveDisk;
	longint lGlobalSlaveDisk;

	// Entete trace
	if (bLocalTrace)
		cout << "Tracing resource estimations\n";

	// Estimation du nombre optimal de processus esclaves pour l'apprentissage
	nMaxSlaveProcessNumber = ComputeMaxSlaveProcessNumber(nAbsoluteMaxSlaveProcessNumber);

	// Estimation du nombre minimal de slices
	nMaxSliceNumber = ComputeMaxSliceNumber();

	// Estimation de la memoire partagee
	lSharedMemory = ComputeSharedNecessaryMemory();

	// Estimation de la memoire pour le maitre
	lMasterMemory = ComputeMasterNecessaryMemory();

	// Estimation de la memoire globale des esclaves
	lGlobalSlaveMinMemory = ComputeGlobalSlaveNecessaryMemory(nMaxSliceNumber);
	lGlobalSlaveMaxMemory = ComputeGlobalSlaveNecessaryMemory(1);

	// Estimation de la memoire marginal pour chaque esclave
	lSlaveMinProcessesMinSlicesMemory = ComputeSlaveNecessaryMemory(1, 1);
	lSlaveMaxProcessesMaxSlicesMemory = ComputeSlaveNecessaryMemory(nMaxSlaveProcessNumber, nMaxSliceNumber);
	lSlaveMemory = max(lSlaveMinProcessesMinSlicesMemory, lSlaveMaxProcessesMaxSlicesMemory);

	// Estimation du disque du maitre
	lMasterDisk = ComputeMasterNecessaryDisk();

	// Estimation du disque marginal pour chaque esclave
	lSlaveDisk = ComputeSlaveNecessaryDisk();

	// Estimation du disque global des esclaves
	lGlobalSlaveDisk = ComputeGlobalSlaveNecessaryDisk();

	// Mise a jour des demandes de resources
	GetResourceRequirements()->SetMaxSlaveProcessNumber(nMaxSlaveProcessNumber);
	GetResourceRequirements()->GetSharedRequirement()->GetMemory()->Set(lSharedMemory);
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->Set(lMasterMemory);
	GetResourceRequirements()->GetGlobalSlaveRequirement()->GetMemory()->SetMin(lGlobalSlaveMinMemory);
	GetResourceRequirements()->GetGlobalSlaveRequirement()->GetMemory()->SetMax(lGlobalSlaveMaxMemory);
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->Set(lSlaveMemory);
	GetResourceRequirements()->GetMasterRequirement()->GetDisk()->Set(lMasterDisk);
	GetResourceRequirements()->GetGlobalSlaveRequirement()->GetDisk()->Set(lGlobalSlaveDisk);
	GetResourceRequirements()->GetSlaveRequirement()->GetDisk()->Set(lSlaveDisk);

	// Trace de deboggage
	if (bLocalTrace)
	{
		cout << "proc max                   = " << nMaxSlaveProcessNumber << "\n";
		cout << "slice max                  = " << nMaxSliceNumber << "\n";
		cout << "shared                 mem = " << LongintToHumanReadableString(lSharedMemory) << "\n";
		cout << "master                 mem = " << LongintToHumanReadableString(lMasterMemory) << "\n";
		cout << "global slave min       mem = " << LongintToHumanReadableString(lGlobalSlaveMinMemory) << "\n";
		cout << "global slave max       mem = " << LongintToHumanReadableString(lGlobalSlaveMaxMemory) << "\n";
		cout << "sl. min slice min proc mem = "
		     << LongintToHumanReadableString(lSlaveMinProcessesMinSlicesMemory) << "\n";
		cout << "sl. max slice max proc mem = "
		     << LongintToHumanReadableString(lSlaveMaxProcessesMaxSlicesMemory) << "\n";
		cout << "slave                  mem = " << LongintToHumanReadableString(lSlaveMemory) << "\n";
		cout << "global slave          disk = " << LongintToHumanReadableString(lGlobalSlaveDisk) << "\n";
		cout << "slave                 disk = " << LongintToHumanReadableString(lSlaveDisk) << "\n";
		cout << "\n";
	}

	return true;
}

int SNBPredictorSNBTrainingTask::ComputeMaxSlaveProcessNumber(int nAbsoluteMaxSlaveProcessNumber) const
{
	const double dAtomicProcessingTime = 1.4e-7;
	const double dCommunicationTime = 8.1e-5;
	const double dCommunicationTimeFactor = 2.0;
	const double dMaxTimeLossFactor = 0.05;
	longint nInstanceNumber;
	longint nIterationNumber;
	longint nAttributeNumber;
	longint nTargetValueNumber;
	double dEffectiveTargetValueNumber;
	double dSequentialTime;
	double dCommunicationTimePerProcess;
	double dOptimalTime;
	int nProcessNumber;
	double dCurrentTime;
	int nOptimalProcessNumber;
	double dMaxTimeLoss;
	int nBestProcessNumber;

	require(nAbsoluteMaxSlaveProcessNumber >= 1);

	// TODO: Etudier differentes parametrages et choisir le meilleur compromis
	// TODO: Ameliorer l'estimation pour la classif avec cible groupee
	// TODO: Incorporer le nombre de slices au modele et un cout d'IO au modele (?)

	// Memorisation des quantites pour la lisibilite des formules
	nInstanceNumber = (longint)masterSnbPredictor->GetInstanceNumber();
	nIterationNumber = (longint)(log2(nInstanceNumber + 1));
	nAttributeNumber = (longint)masterSnbPredictor->GetTrainingAttributeNumber();
	nTargetValueNumber = (longint)masterSnbPredictor->GetTargetDescriptiveStats()->GetValueNumber();

	// En regression groupee le nombre de parties de la cible est estime par sqrt(N)
	if (masterSnbPredictor->GetTargetAttributeType() == KWType::Symbol)
		dEffectiveTargetValueNumber = double(nTargetValueNumber);
	else
		dEffectiveTargetValueNumber = sqrt(nInstanceNumber);

	// Estimation des temps de traitement en sequentiel et du temps totaux de communication par processus
	// Les constantes dAtomicProcessingTime et dComunnicationTime ont ete estimes dans un etude
	dSequentialTime =
	    nIterationNumber * nInstanceNumber * nAttributeNumber * dEffectiveTargetValueNumber * dAtomicProcessingTime;
	dCommunicationTimePerProcess =
	    nIterationNumber * nAttributeNumber * dCommunicationTime * dCommunicationTimeFactor;

	// On cherche le meilleur temps en parallele
	dOptimalTime = DBL_MAX;
	nOptimalProcessNumber = 1;
	for (nProcessNumber = 2; nProcessNumber <= nAbsoluteMaxSlaveProcessNumber; nProcessNumber++)
	{
		dCurrentTime = dSequentialTime / nProcessNumber + dCommunicationTimePerProcess * nProcessNumber;

		// On memorise le temps courant si il y a une amelioration
		if (dCurrentTime < dOptimalTime)
		{
			dOptimalTime = dCurrentTime;
			nOptimalProcessNumber = nProcessNumber;
		}
		// Si le temps ne se ameliore pas on coupe l'optimisation (fn. convexe en le nombre de processus)
		else
			break;
	}

	// On reste en sequentiel si le meilleur temps parallele est pire que le temps sequentiel
	if (dSequentialTime < dOptimalTime)
		nOptimalProcessNumber = 1;

	// Optimum parallele: recherche en arriere d'un nombre de processus moindre sans trop de perte en temps
	if (nOptimalProcessNumber > 1)
	{
		nBestProcessNumber = nOptimalProcessNumber;
		dMaxTimeLoss = max(dOptimalTime * dMaxTimeLossFactor, 1.0);
		for (nProcessNumber = nOptimalProcessNumber - 1; nProcessNumber > 1; nProcessNumber--)
		{
			dCurrentTime = dSequentialTime / nProcessNumber + dCommunicationTimePerProcess * nProcessNumber;

			// On memorise le temps courant si il n'empire pas du maximum estipule
			if (dCurrentTime - dOptimalTime <= dMaxTimeLoss)
				nBestProcessNumber = nProcessNumber;
			else
				break;
		}
	}
	// Optimum sequentiel: On reste en sequentiel
	else
		nBestProcessNumber = 1;

	return nBestProcessNumber;
}

int SNBPredictorSNBTrainingTask::ComputeMaxSliceNumber() const
{
	return max(1, int(sqrt(masterSnbPredictor->GetTrainingAttributeNumber())));
}

longint SNBPredictorSNBTrainingTask::ComputeSharedNecessaryMemory()
{
	const boolean bLocalTrace = false;
	longint lBinarySliceSetSchemaMemory;
	longint lOverallAttributeStatsMemory;
	longint lRecodingObjectsMemory;
	longint lSharedMemory;

	// Memoire du SNBDataTableBinarySliceSetSchema
	lBinarySliceSetSchemaMemory =
	    SNBDataTableBinarySliceSetSchema::ComputeNecessaryMemory(masterSnbPredictor->GetClassStats());

	// Memoire des KWAttributeStats's
	lOverallAttributeStatsMemory = ComputeOverallAttributeStatsNecessaryMemory();

	// Memoire des objets de recodage
	lRecodingObjectsMemory = ComputeRecodingObjectsNecessaryMemory();

	lSharedMemory = lBinarySliceSetSchemaMemory + lOverallAttributeStatsMemory + lRecodingObjectsMemory;

	// Trace de deboggage
	if (bLocalTrace)
	{
		cout << "Shared memory estimation:\n";
		cout << "b.slice set schema        mem = " << LongintToHumanReadableString(lBinarySliceSetSchemaMemory)
		     << "\n";
		cout << "attribute stats           mem = " << LongintToHumanReadableString(lOverallAttributeStatsMemory)
		     << "\n";
		cout << "recoding objects          mem = " << LongintToHumanReadableString(lRecodingObjectsMemory)
		     << "\n";
		cout << "shared                    mem = " << LongintToHumanReadableString(lSharedMemory) << "\n";
		cout << "---------------------------------------------------------\n";
	}

	return lSharedMemory;
}

longint SNBPredictorSNBTrainingTask::ComputeGlobalSlaveNecessaryMemory(int nSliceNumber)
{
	const boolean bLocalTrace = false;
	int nInstanceNumber;
	int nAttributeNumber;
	longint lGlobalDataCostCalculatorMemory;
	longint lGlobalBinarySliceSetBufferMemory;
	longint lRecodingObjectsMemory;
	longint lGlobalSlaveMemory;

	// Aliases locales pour la lisibilite
	nInstanceNumber = masterSnbPredictor->GetInstanceNumber();
	nAttributeNumber = masterSnbPredictor->GetTrainingAttributeNumber();

	// La memoire global du buffer et du scorer s'estime avec les estimations des objets necessaires avec un seul
	// chunk
	lGlobalDataCostCalculatorMemory = ComputeGlobalSlaveScorerNecessaryMemory();
	lGlobalBinarySliceSetBufferMemory = SNBDataTableBinarySliceSetBuffer::ComputeNecessaryMemory(
	    nInstanceNumber, 1, nAttributeNumber, nSliceNumber);

	// Estimation de la memoire necessaire pour le recodage
	lRecodingObjectsMemory = ComputeRecodingObjectsNecessaryMemory();

	// Rationale de l'estimation : d'abord deux faits
	//   1) La memoire pour le recodage est deja demandee en shared
	//   2) La memoire pour le recodage et celle de la calculatrice ne cohabitent pas
	// Donc il faut seulement demander le delta necessaire pour la calculatrice
	lGlobalSlaveMemory =
	    lGlobalBinarySliceSetBufferMemory + max(lGlobalDataCostCalculatorMemory - lRecodingObjectsMemory, 0ll);

	// NB: Cette chiffre est sur-estimee dans le cas de plus d'un esclave. La vraie quantite necessaire est
	//
	//   lTrueGlobalSlaveMemory = lGlobalBinarySliceSetBufferMemory + max(lGlobalDataCostCalculatorMemory -
	//   nSlaveNumber * lRecodingObjectsMemory, 0ll)
	//
	// car chaque esclave demande un dictionnaire.
	// Neanmoins, si l'on prends notre l'estimation avec M slices en tant demande minimal de memoire on a la
	// garantie que pour n'importe quel nombre de processus il y a une nombre de slices ou on peut tourner la tache
	// (M slices dans le pire cas). La raison est que lTrueGlobalSlaveMemory <= lGlobalSlaveMemory pour n'importe
	// quel nombre d'esclaves et slices.
	//
	// Le compromis est que on interdit certains solutions avec un moindre nombre de slices.

	// Trace de deboggage
	if (bLocalTrace)
	{
		cout << "Global slave memory estimation (" << nSliceNumber << " slices):\n";
		cout << "binary slice set buffer   mem = "
		     << LongintToHumanReadableString(lGlobalBinarySliceSetBufferMemory) << "\n";
		cout << "data cost calculator      mem = "
		     << LongintToHumanReadableString(lGlobalDataCostCalculatorMemory) << "\n";
		cout << "recoding objects          mem = " << LongintToHumanReadableString(lRecodingObjectsMemory)
		     << "\n";
		cout << "global slave              mem = " << LongintToHumanReadableString(lGlobalSlaveMemory) << "\n";
		cout << "---------------------------------------------------------\n";
	}

	return lGlobalSlaveMemory;
}

longint SNBPredictorSNBTrainingTask::ComputeSlaveNecessaryMemory(int nSlaveProcessNumber, int nSliceNumber)
{
	const boolean bLocalTrace = false;
	int nInstanceNumber;
	int nAttributeNumber;
	longint lLayoutMemory;
	longint lTargetValuesMemory;
	longint lBinarySliceSetSelfMemory;
	longint lSelectionScorerMemory;
	longint lSlaveMemory;

	// Aliases locales pour la lisibilite
	nInstanceNumber = masterSnbPredictor->GetInstanceNumber();
	nAttributeNumber = masterSnbPredictor->GetTrainingAttributeNumber();

	// Memoire de toutes les parties de la SNBDataTableBinarySliceSet sauf le buffer et ses contenus
	lLayoutMemory = SNBDataTableBinarySliceSetLayout::ComputeNecessaryMemory(nInstanceNumber, nSlaveProcessNumber,
										 nAttributeNumber, nSliceNumber);
	lTargetValuesMemory = SNBDataTableBinarySliceSet::ComputeTargetValuesNecessaryMemory(nInstanceNumber);
	lBinarySliceSetSelfMemory = sizeof(SNBDataTableBinarySliceSet) - sizeof(SNBDataTableBinarySliceSetSchema) -
				    sizeof(SNBDataTableBinarySliceSetLayout) -
				    sizeof(SNBDataTableBinarySliceSetRandomizedAttributeIterator) -
				    sizeof(SNBDataTableBinarySliceSetBuffer);
	lSelectionScorerMemory = ComputeSlaveScorerNecessaryMemory();

	// Memoire du scorer sans ses contenus

	// La memoire de l'esclave est celle des
	lSlaveMemory = lLayoutMemory + lTargetValuesMemory + lBinarySliceSetSelfMemory;

	// Trace de deboggage
	if (bLocalTrace)
	{
		cout << "Slave memory estimation (" << nSlaveProcessNumber << " slave processes, " << nSliceNumber
		     << " slices):\n";
		cout << "layout                    mem = " << LongintToHumanReadableString(lLayoutMemory) << "\n";
		cout << "target values             mem = " << LongintToHumanReadableString(lTargetValuesMemory) << "\n";
		cout << "b. slice set (self)       mem = " << LongintToHumanReadableString(lBinarySliceSetSelfMemory)
		     << "\n";
		cout << "slave                     mem = " << LongintToHumanReadableString(lSlaveMemory) << "\n";
		cout << "---------------------------------------------------------\n";
	}

	return lSlaveMemory;
}

longint SNBPredictorSNBTrainingTask::ComputeOverallAttributeStatsNecessaryMemory()
{
	longint lOverallAttributeStatsMemory;
	ObjectDictionary dummyDictionary;
	ObjectArray* oaAllAttributeStats;
	int nAttribute;
	KWAttributeStats* attributeStats;
	ObjectArray* oaAllAttributePairStats;
	KWAttributePairStats* attributePairStats;

	// Memoire des conteneurs
	lOverallAttributeStatsMemory =
	    (longint)(masterSnbPredictor->GetClassStats()->GetInformativeAttributeNumber() +
		      masterSnbPredictor->GetClassStats()->GetInformativeCreatedAttributeNumber()) *
	    dummyDictionary.GetUsedMemoryPerElement();

	// Memoire des KWAttributeStats's informatifs
	oaAllAttributeStats = masterSnbPredictor->GetClassStats()->GetAttributeStats();
	for (nAttribute = 0; nAttribute < oaAllAttributeStats->GetSize(); nAttribute++)
	{
		attributeStats = cast(KWAttributeStats*, oaAllAttributeStats->GetAt(nAttribute));

		if (attributeStats->GetLevel() > 0)
			lOverallAttributeStatsMemory += attributeStats->GetUsedMemory();
	}

	// Memoire des KWAttributePairStats's informatifs
	oaAllAttributePairStats = masterSnbPredictor->GetClassStats()->GetAttributePairStats();
	for (nAttribute = 0; nAttribute < oaAllAttributePairStats->GetSize(); nAttribute++)
	{
		attributePairStats = cast(KWAttributePairStats*, oaAllAttributePairStats->GetAt(nAttribute));

		if (attributePairStats->GetLevel() > 0)
			lOverallAttributeStatsMemory += attributePairStats->GetUsedMemory();
	}

	return lOverallAttributeStatsMemory;
}

longint SNBPredictorSNBTrainingTask::ComputeDataPreparationClassNecessaryMemory()
{
	int nAttribute;
	ObjectArray* oaAllPreparedStats;
	KWDataPreparationStats* attributeStats;
	longint lDataPreparationClassMemory;

	// Estimation initale par la taille de la classe original d'apprentissage
	lDataPreparationClassMemory = masterInitialClass->GetDomain()->GetUsedMemory();

	// Estimation de memoire pour chacun des attributs
	oaAllPreparedStats = masterSnbPredictor->GetClassStats()->GetAllPreparedStats();
	for (nAttribute = 0; nAttribute < oaAllPreparedStats->GetSize(); nAttribute++)
	{
		attributeStats = cast(KWDataPreparationStats*, oaAllPreparedStats->GetAt(nAttribute));
		lDataPreparationClassMemory +=
		    ComputeDataPreparationAttributeNecessaryMemory(attributeStats->GetPreparedDataGridStats());
	}

	return lDataPreparationClassMemory;
}

longint SNBPredictorSNBTrainingTask::ComputeRecodingObjectsNecessaryMemory()
{
	const boolean bLocalTrace = false;
	const double dCompilationFactor = 1.5;
	const double dPhysicalClassFactor = 2.0;
	longint lDataPreparationClassMemory;
	KWDataTableSliceSet* sliceSet;
	longint lLargestSliceSetFileSize;
	KWDataTableSlice* slice;
	int nSlice;
	int nDataFile;
	longint lSliceSetReadBufferMemory;
	longint lRecodingObjectsMemory;

	// Memoire de la KWDataPreparationClass
	lDataPreparationClassMemory = ComputeDataPreparationClassNecessaryMemory();

	// Buffers pour la lecture depuis KWDataTableSliceSet
	sliceSet = shared_dataTableSliceSet.GetDataTableSliceSet();
	lLargestSliceSetFileSize = 0;
	for (nSlice = 0; nSlice < sliceSet->GetSliceNumber(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, sliceSet->GetSlices()->GetAt(nSlice));
		for (nDataFile = 0; nDataFile < slice->GetDataFileSizes()->GetSize(); nDataFile++)
		{
			if (lLargestSliceSetFileSize < slice->GetDataFileSizes()->GetAt(nDataFile))
				lLargestSliceSetFileSize = slice->GetDataFileSizes()->GetAt(nDataFile);
		}
	}
	lSliceSetReadBufferMemory = min((longint)BufferedFile::nDefaultBufferSize, lLargestSliceSetFileSize);

	// Total objets de recodage (avec des facteurs de slackness pour la compilation et la classe physique)
	lRecodingObjectsMemory = (longint)(lDataPreparationClassMemory * dCompilationFactor * dPhysicalClassFactor) +
				 lSliceSetReadBufferMemory;

	// Trace de deboggage
	if (bLocalTrace)
	{
		cout << "Recoding objects memory estimation\n";
		cout << "data prep class           mem = " << LongintToHumanReadableString(lDataPreparationClassMemory)
		     << "\n";
		cout << "slice set buffer          mem = " << LongintToHumanReadableString(lSliceSetReadBufferMemory)
		     << "\n";
		cout << "compilation factor            = " << dCompilationFactor << "\n";
		cout << "physical class factor         = " << dPhysicalClassFactor << "\n";
		cout << "recoding objects total    mem = " << LongintToHumanReadableString(lRecodingObjectsMemory)
		     << "\n";
		cout << "\n";
	}

	return lRecodingObjectsMemory;
}

longint
SNBPredictorSNBTrainingTask::ComputeDataPreparationAttributeNecessaryMemory(const KWDataGridStats* dataGridStats) const
{
	longint lDataPreparationAttributeMemory;
	int nAttribute;
	int nGroup;
	const KWDGSAttributePartition* attributePartition;
	const KWDGSAttributeDiscretization* attributeDiscretization;
	const KWDGSAttributeContinuousValues* attributeContinuousValues;
	const KWDGSAttributeGrouping* attributeGrouping;
	const KWDGSAttributeSymbolValues* attributeSymbolValues;

	// Memoire du pointeur
	lDataPreparationAttributeMemory = sizeof(KWDataPreparationAttribute*);

	if (dataGridStats != NULL)
	{
		// Memoire propre de l'objet
		lDataPreparationAttributeMemory = sizeof(KWDataPreparationAttribute);

		// Memoire de l'attribut associe
		lDataPreparationAttributeMemory += sizeof(KWAttribute*) + sizeof(KWAttribute) +
						   dataGridStats->ExportVariableNames().GetUsedMemory() +
						   sizeof(KWKeyValuePair*) + sizeof(KWKeyValuePair);

		// Memoire pour la regle "DataGrid" du attribut et l'attribut de recodage "CellIndex" associe
		lDataPreparationAttributeMemory +=
		    sizeof(KWDRDataGrid*) + sizeof(KWDRDataGrid) + sizeof(KWDRCellIndex*) + sizeof(KWDRCellIndex);
		for (nAttribute = 0; nAttribute < dataGridStats->GetAttributeNumber(); nAttribute++)
		{
			attributePartition = dataGridStats->GetAttributeAt(nAttribute);

			// Memoire des operands (1 pour la regle datagrid et 1 pour l'attribut de recodage)
			lDataPreparationAttributeMemory +=
			    2 * (sizeof(KWDerivationRuleOperand*) + sizeof(KWDerivationRuleOperand));

			// Memoire de l'attribut pour la regle de recodage
			lDataPreparationAttributeMemory +=
			    dataGridStats->GetAttributeAt(nAttribute)->GetAttributeName().GetUsedMemory();

			// Cas de la discretisation
			if (attributePartition->GetAttributeType() == KWType::Continuous and
			    not attributePartition->ArePartsSingletons())
			{
				attributeDiscretization = cast(const KWDGSAttributeDiscretization*, attributePartition);
				lDataPreparationAttributeMemory +=
				    sizeof(KWDRIntervalBounds*) + sizeof(KWDRIntervalBounds) +
				    (longint)attributeDiscretization->GetIntervalBoundNumber() * sizeof(Continuous);
			}
			// Cas d'un ensemble des singletons de valeurs continues
			else if (attributePartition->GetAttributeType() == KWType::Continuous and
				 attributePartition->ArePartsSingletons())
			{
				attributeContinuousValues =
				    cast(const KWDGSAttributeContinuousValues*, attributePartition);
				lDataPreparationAttributeMemory +=
				    sizeof(KWDRContinuousValueSet*) + sizeof(KWDRContinuousValueSet) +
				    (longint)attributeContinuousValues->GetValueNumber() * sizeof(Continuous);
			}
			// Cas d'un groupement de symboles
			else if (attributePartition->GetAttributeType() == KWType::Symbol and
				 not attributePartition->ArePartsSingletons())
			{
				attributeGrouping = cast(KWDGSAttributeGrouping*, attributePartition);
				lDataPreparationAttributeMemory += sizeof(KWDRValueGroups*) + sizeof(KWDRValueGroups);

				for (nGroup = 0; nGroup < attributeGrouping->GetGroupNumber(); nGroup++)
				{
					lDataPreparationAttributeMemory +=
					    sizeof(KWDRValueGroup*) + sizeof(KWDRValueGroup) +
					    sizeof(KWDerivationRuleOperand*) + sizeof(KWDerivationRuleOperand) +
					    (longint)attributeGrouping->GetGroupValueNumberAt(nGroup) * sizeof(Symbol);
				}
			}
			// Cas d'un ensemble de singletons de symboles
			else if (attributePartition->GetAttributeType() == KWType::Symbol and
				 attributePartition->ArePartsSingletons())
			{
				attributeSymbolValues = cast(const KWDGSAttributeSymbolValues*, attributePartition);
				lDataPreparationAttributeMemory +=
				    sizeof(KWDRSymbolValueSet*) + sizeof(KWDRSymbolValueSet) +
				    (longint)attributeSymbolValues->GetPartNumber() * sizeof(Symbol);
			}
		}

		// Memoire de la regle des effectifs des cellules
		lDataPreparationAttributeMemory += sizeof(KWDRFrequencies*) + sizeof(KWDRFrequencies) +
						   sizeof(KWDerivationRuleOperand*) + sizeof(KWDerivationRuleOperand) +
						   (longint)dataGridStats->ComputeTotalGridSize() * sizeof(int);
	}

	return lDataPreparationAttributeMemory;
}

longint SNBPredictorSNBTrainingTask::ComputeMasterNecessaryDisk()
{
	// On sur-estime le disque necessaire pour le fichier du dictionnaire de recodage avec son empreinte en memoire
	return ComputeDataPreparationClassNecessaryMemory();
}

longint SNBPredictorSNBTrainingTask::ComputeGlobalSlaveNecessaryDisk() const
{
	int nInstanceNumber;
	int nAttributeNumber;
	longint lGlobalSlaveDisk;

	// On estime la capacite totale du disque par la taille du SNBDataTableBinarySliceSetBuffer avec
	// un seul esclave et une seule slice (un peu sur-estimee)
	nInstanceNumber = masterSnbPredictor->GetInstanceNumber();
	nAttributeNumber = masterSnbPredictor->GetTrainingAttributeNumber();
	lGlobalSlaveDisk =
	    SNBDataTableBinarySliceSetBuffer::ComputeNecessaryMemory(nInstanceNumber, 1, nAttributeNumber, 1);

	return lGlobalSlaveDisk;
}

longint SNBPredictorSNBTrainingTask::ComputeSlaveNecessaryDisk()
{
	// On sur-estime le disque necessaire pour le fichier dictionnaire de recodage avec son empreinte en memoire
	return ComputeDataPreparationClassNecessaryMemory();
}

boolean SNBPredictorSNBTrainingTask::MasterInitialize()
{
	boolean bOk = true;

	// Activation du mode boost
	// Amelioration faible en pratique, de l'ordre de 10% dans les cas favorables
	SetBoostMode(true);

	// Initialisation du SNBDataTableBinarySliceSet du maitre
	bOk = MasterInitializeDataTableBinarySliceSet();

	// Initialisation du reste des variables
	if (bOk)
		bOk = bOk and MasterInitializeSharedVariables();
	if (bOk)
		MasterInitializeOptimizationVariables();

	// Estimation de l'unite de progression
	// Le nombre de taches est estme comme ceci :
	//   [#iterations externes] x [# max passes FFWBW] x [2 x #attributs] x [#eslaves]
	//                                                        ^^^^^
	//                                                (1 passe FFW et 1 FBW)
	// L'unite de progres est donc 1/#taches
	// Elle est sous-estime car le nombre de passes FFWBW n'atteint pas toujours le nombre max
	if (bOk)
	{
		dMasterTaskProgress = 1.0 / nMasterOuterIterationNumber;
		dMasterTaskProgress /= nMaxFastForwardBackwardRuns;
		dMasterTaskProgress /= 2.0 * masterBinarySliceSet->GetAttributeNumber();
		dMasterTaskProgress /= GetTaskResourceGrant()->GetSlaveNumber();
	}

	return bOk;
}

boolean SNBPredictorSNBTrainingTask::MasterInitializeDataTableBinarySliceSet()
{
	const boolean bLocalTrace = false;
	int nSlaveProcessNumber;
	boolean bOk = false;
	int nSliceNumber;
	int nMaxSliceNumber;
	longint lGrantedSlaveMemory;
	longint lSlaveNecessaryMemory;
	KWClass* recoderClass;

	require(masterBinarySliceSet == NULL);

	// Initialisation des variables locaux
	nSlaveProcessNumber = GetTaskResourceGrant()->GetSlaveNumber();
	lGrantedSlaveMemory = GetTaskResourceGrant()->GetMinSlaveMemory();
	nMaxSliceNumber = ComputeMaxSliceNumber();
	recoderClass = NULL;
	lSlaveNecessaryMemory = 0ll;

	// On recherche le moindre nombre de slices qui permet d'executer la tache
	for (nSliceNumber = 1; nSliceNumber <= nMaxSliceNumber; nSliceNumber++)
	{
		lSlaveNecessaryMemory = ComputeGlobalSlaveNecessaryMemory(nSliceNumber) / nSlaveProcessNumber +
					ComputeSlaveNecessaryMemory(nSlaveProcessNumber, nSliceNumber);
		if (lGrantedSlaveMemory >= lSlaveNecessaryMemory)
		{
			bOk = true;
			break;
		}
	}

	// Si le nombre de slices a ete trouve : Initialisation
	if (bOk)
	{
		// Initialisation du SNBDataTableBinarySliceSet du maitre
		masterBinarySliceSet = new SNBDataTableBinarySliceSet;
		masterBinarySliceSet->InitializeSchemaFromClassStats(
		    masterSnbPredictor->GetClassStats(),
		    masterSnbPredictor->GetTrainParameters()->GetMaxEvaluatedAttributeNumber());
		masterBinarySliceSet->layout.Initialize(masterSnbPredictor->GetInstanceNumber(), nSlaveProcessNumber,
							masterBinarySliceSet->GetAttributeNumber(), nSliceNumber);
		masterBinarySliceSet->randomizedAttributeIterator.Initialize(&masterBinarySliceSet->schema,
									     &masterBinarySliceSet->layout);
		masterBinarySliceSet->InitializeTargetValueIndexes(masterSnbPredictor->GetClassStats());

		// Changement le domaine de travail a celui de la classe de recodage
		recoderClass = masterBinarySliceSet->GetDataPreparationClass()->GetDataPreparationClass();
		masterInitialDomain = KWClassDomain::GetCurrentDomain();
		KWClassDomain::SetCurrentDomain(recoderClass->GetDomain());
		KWClassDomain::GetCurrentDomain()->Compile();
	}
	// Message d'erreur si pas assez de memoire, meme avec le nombre maximal de slices
	else
		AddError("not enough memory to run the task" +
			 RMResourceManager::BuildMissingMemoryMessage(lSlaveNecessaryMemory - lGrantedSlaveMemory));

	// Trace de deboggage
	if (bLocalTrace)
	{
		cout << "-------------------------------------------------------\n";
		cout << "status                 = " << (bOk ? "OK" : "KO") << "\n";
		cout << "granted slave proc     = " << GetTaskResourceGrant()->GetSlaveNumber() << "\n";
		cout << "granted master     mem = "
		     << LongintToHumanReadableString(GetTaskResourceGrant()->GetMasterMemory()) << " = "
		     << GetTaskResourceGrant()->GetMasterMemory() << " bytes\n";
		cout << "granted shared     mem = "
		     << LongintToHumanReadableString(GetTaskResourceGrant()->GetSharedMemory()) << " = "
		     << GetTaskResourceGrant()->GetSharedMemory() << " bytes\n";
		cout << "granted slave      mem = "
		     << LongintToHumanReadableString(GetTaskResourceGrant()->GetMinSlaveMemory()) << " = "
		     << GetTaskResourceGrant()->GetMinSlaveMemory() << " bytes\n";
		if (bOk)
		{
			cout << "reco dict real     mem = "
			     << LongintToHumanReadableString(recoderClass->GetUsedMemory()) << " = "
			     << recoderClass->GetUsedMemory() << " bytes\n";
			cout << masterBinarySliceSet->layout << "\n";
		}
		cout << "-------------------------------------------------------\n";
		cout << endl;
	}

	ensure(not bOk or IsMasterDataTableBinarySliceSetInitialized());
	ensure(not bOk or masterBinarySliceSet->Check());

	return bOk;
}

boolean SNBPredictorSNBTrainingTask::IsMasterDataTableBinarySliceSetInitialized() const
{
	boolean bOk = true;
	int nChunk;

	bOk = bOk and masterBinarySliceSet != NULL;
	bOk = bOk and masterBinarySliceSet->dataPreparationClass != NULL;
	bOk = bOk and masterBinarySliceSet->schema.IsInitialized();
	bOk = bOk and masterBinarySliceSet->layout.IsInitialized();
	bOk = bOk and masterBinarySliceSet->randomizedAttributeIterator.IsInitialized();
	bOk = bOk and masterBinarySliceSet->ivTargetValueIndexes.GetSize() > 0;
	bOk = bOk and masterBinarySliceSet->nInitializedChunkIndex < 0;
	bOk = bOk and not masterBinarySliceSet->dataBuffer.IsInitialized();
	if (bOk)
	{
		for (nChunk = 0; nChunk < masterBinarySliceSet->layout.GetChunkNumber(); nChunk++)
			bOk = bOk and not masterBinarySliceSet->dataBuffer.IsInitializedOnlyAtChunk(nChunk);
	}
	return bOk;
}

boolean SNBPredictorSNBTrainingTask::MasterInitializeSharedVariables()
{
	boolean bOk = true;
	int nAttribute;
	int nSlaveProcess;
	SNBDataTableBinarySliceSetAttribute* attribute;
	KWDataPreparationStats* dataPreparationStats;

	require(IsMasterDataTableBinarySliceSetInitialized());

	// Ids des esclaves attribues a la tache
	shared_ivGrantedSlaveProcessIds.GetIntVector()->SetSize(GetGrantedSlaveProcessIds()->GetSize());
	for (nSlaveProcess = 0; nSlaveProcess < GetGrantedSlaveProcessIds()->GetSize(); nSlaveProcess++)
		shared_ivGrantedSlaveProcessIds.SetAt(nSlaveProcess, GetGrantedSlaveProcessIds()->GetAt(nSlaveProcess));

	// Classe de recodage
	bOk = bOk and MasterInitializeRecoderClassSharedVariables();

	// Variables partagees du layout
	shared_nInstanceNumber = masterBinarySliceSet->GetInstanceNumber();
	shared_nChunkNumber = masterBinarySliceSet->layout.GetChunkNumber();
	shared_nAttributeNumber = masterBinarySliceSet->GetAttributeNumber();
	shared_nSliceNumber = masterBinarySliceSet->layout.GetSliceNumber();
	shared_nInitialAttributeNumber = masterBinarySliceSet->GetInitialAttributeNumber();

	// Tableau partage des attributs
	shared_oaBinarySliceSetAttributes->GetObjectArray()->CopyFrom(&masterBinarySliceSet->schema.oaAttributes);

	// Mise des KWDataPreparationStats' des attributs dans des hash partages differents (cas univarie ou bivarie)
	// Ceci permet de les initialiser dans l'esclave avec la bonne sous-classe de dataPreparationStats
	for (nAttribute = 0; nAttribute < masterBinarySliceSet->GetAttributeNumber(); nAttribute++)
	{
		attribute = masterBinarySliceSet->GetAttributeAt(nAttribute);
		dataPreparationStats = attribute->dataPreparationStats;
		assert(1 <= dataPreparationStats->GetAttributeNumber() and
		       dataPreparationStats->GetAttributeNumber() <= 2);
		if (dataPreparationStats->GetAttributeNumber() == 1)
			shared_odAttributeStats->GetObjectDictionary()->SetAt(attribute->GetNativeAttributeName(),
									      dataPreparationStats);
		else
			shared_odAttributePairStats->GetObjectDictionary()->SetAt(attribute->GetNativeAttributeName(),
										  dataPreparationStats);
	}

	// Tableau des indexes des valeurs cibles partagees
	shared_ivTargetValueIndexes.GetIntVector()->CopyFrom(&masterBinarySliceSet->ivTargetValueIndexes);

	return bOk;
}

boolean SNBPredictorSNBTrainingTask::MasterInitializeRecoderClassSharedVariables()
{
	boolean bOk = true;
	KWClass* recoderClass;
	ALString sRecoderClassTmpFilePath;

	require(IsMasterProcess());
	require(masterBinarySliceSet != NULL);
	require(masterBinarySliceSet->Check());

	// En parallele : On enregistre le domaine de la classe de recodage dans un fichier et on met son URI en shared
	recoderClass = masterBinarySliceSet->GetDataPreparationClass()->GetDataPreparationClass();
	assert(recoderClass->IsCompiled());
	shared_sRecoderClassName.SetValue(recoderClass->GetName());
	if (IsParallel())
	{
		sRecoderClassTmpFilePath =
		    FileService::CreateUniqueTmpFile(shared_sRecoderClassName.GetValue() + ".kdic", this);
		bOk = bOk and sRecoderClassTmpFilePath != "";
		if (not bOk)
			AddError("Error when creating temporary dictionary file for class " +
				 shared_sRecoderClassName.GetValue());

		if (bOk)
		{
			bOk = bOk and
			      recoderClass->GetDomain()->WriteFileFromClass(recoderClass, sRecoderClassTmpFilePath);
			if (not bOk)
			{
				AddError("Error when writing temporary dictionary file for class " +
					 shared_sRecoderClassName.GetValue());

				// Destruction du fichier
				FileService::RemoveFile(sRecoderClassTmpFilePath);
			}
		}

		if (bOk)
			shared_sRecoderClassDomainFileURI.SetValue(
			    FileService::BuildLocalURI(sRecoderClassTmpFilePath));
	}

	return bOk;
}

void SNBPredictorSNBTrainingTask::MasterInitializeOptimizationVariables()
{
	require(masterBinarySliceSet != NULL);

	// Memorisation et fixation de l'etat du RNG
	nMasterRandomSeed = GetRandomSeed();
	SetRandomSeed(1);

	// Variables de l'etat des iterations
	bMasterIsOnFastAddRun = true;
	bMasterUndoLastModification = false;
	nMasterOuterIteration = 0;
	nMasterFastForwardBackwardRun = 0;
	nMasterRandomAttribute = 0;
	masterRandomAttribute = masterBinarySliceSet->GetRandomAttributeAt(0);
	nMasterFastRunStepFinishedTaskNumber = 0;
	bMasterInitializeSlaveScorers = false;

	// Scores et couts
	dMasterModificationScore = 0.0;
	dMasterModificationModelCost = 0.0;
	dMasterModificationDataCost = 0.0;
	dMasterCurrentScore = 0.0;
	dMasterCurrentModelCost = 0.0;
	dMasterCurrentDataCost = 0.0;
	dMasterEmptySelectionScore = 0.0;
	dMasterEmptySelectionModelCost = 0.0;
	dMasterEmptySelectionDataCost = 0.0;
	dMasterPreviousRunScore = 0.0;
	dMasterMapScore = 0.0;

	// L'epsilon de precision a besoin d'une passe pour la base de donnees et se calcule au debut de la tache
	// dMasterPrecisionEpsilon = <apres la premiere passe>;

	ensure(CheckCurrentAttribute());
}

boolean SNBPredictorSNBTrainingTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	require(0 <= nMasterRandomAttribute and nMasterRandomAttribute < masterBinarySliceSet->GetAttributeNumber());
	require(masterRandomAttribute != NULL);

	if (not IsOuterIterationFinished())
	{
		// Premiere passe : Calcul du score de une selection vide pour estimer l'epsilon de precision
		if (not IsPrecisionEpsilonCalculated())
		{
			input_bUndoLastModification = false;
			input_bIsForwardModification = false;
			input_nModificationAttribute = -1;
			input_bInitializeWorkingData = false;
		}
		// Passe normal : Calcul du score d'une modification de la selection courant
		else
		{
			input_bIsForwardModification = IsOnFastForwardRun();
			input_bUndoLastModification = bMasterUndoLastModification;
			input_nModificationAttribute = masterRandomAttribute->GetIndex();
			input_bInitializeWorkingData = bMasterInitializeSlaveScorers;
			dTaskPercent = dMasterTaskProgress;
		}

		// Activation de la barriere de synchronisation pour les esclaves
		SetSlaveAtRestAfterProcess();
	}
	else
		bIsTaskFinished = true;

	return true;
}

boolean SNBPredictorSNBTrainingTask::MasterAggregateResults()
{
	// Mise a jour du compte de taches
	nMasterFastRunStepFinishedTaskNumber++;

	if (IsPrecisionEpsilonCalculated())
	{
		// Mise a jour du cout de donnes courant avec celui issu de l'esclave
		dMasterModificationDataCost += output_dDataCost;

		// Fin de toutes les tache d'un pas de la passe
		if (AllFastRunStepTasksAreFinished())
		{
			// Mise a jour de la selection s'il y a une amelioration
			UpdateSelection();

			// Mise a jour de l'attribut de l'iteration
			UpdateCurrentAttribute();

			// Si on est a la fin de la passe rapide on commence une autre (potentiellement une nouvelle
			// iteration externe)
			if (IsFastRunFinished())
				InitializeNextFastRun();

			// Remise a zero du score et couts de la modification courant
			dMasterModificationScore = 0.0;
			dMasterModificationModelCost = 0.0;
			dMasterModificationDataCost = 0.0;

			// Remise a zero du compteur des taches & liberation de la barriere de synchronization
			nMasterFastRunStepFinishedTaskNumber = 0;
			SetAllSlavesAtWork();
		}
	}
	else
	{
		// Mise a jour du cout de donnes de la selection vide avec celui issu de l'esclave
		dMasterEmptySelectionDataCost += output_dDataCost;

		// Fin du pas de la passe
		if (AllFastRunStepTasksAreFinished())
		{
			// Calcul d'epsilon de precision et initialisation des scores initiaux
			ComputeEmptySelectionScoreAndPrecisionEpsilon();

			// Remise a zero du compteur des taches & liberation de la barriere de synchronization
			nMasterFastRunStepFinishedTaskNumber = 0;
			SetAllSlavesAtWork();
		}
	}
	ensure(0 <= nMasterFastRunStepFinishedTaskNumber and nMasterFastRunStepFinishedTaskNumber < GetProcessNumber());
	ensure(CheckCurrentAttribute());
	return true;
}

boolean SNBPredictorSNBTrainingTask::IsOuterIterationFinished() const
{
	require(IsRunning());
	require(IsMasterProcess());
	return nMasterOuterIteration == nMasterOuterIterationNumber;
}

boolean SNBPredictorSNBTrainingTask::IsPrecisionEpsilonCalculated() const
{
	require(IsRunning());
	require(IsMasterProcess());
	return dMasterPrecisionEpsilon > 0;
}

boolean SNBPredictorSNBTrainingTask::IsOnFastForwardRun() const
{
	require(IsRunning());
	require(IsMasterProcess());
	return bMasterIsOnFastAddRun;
}

boolean SNBPredictorSNBTrainingTask::AllFastRunStepTasksAreFinished() const
{
	require(IsRunning());
	require(IsMasterProcess());
	return nMasterFastRunStepFinishedTaskNumber == GetProcessNumber();
}

boolean SNBPredictorSNBTrainingTask::IsFastRunFinished() const
{
	boolean bIsRunFinished;

	require(IsRunning());
	require(IsMasterProcess());

	if (IsOnFastForwardRun())
		bIsRunFinished = nMasterRandomAttribute == masterBinarySliceSet->GetAttributeNumber();
	else
		bIsRunFinished = nMasterRandomAttribute == -1;

	return bIsRunFinished;
}

void SNBPredictorSNBTrainingTask::InitializeNextFastRun()
{
	require(IsMasterProcess());

	// Fin d'une passe FastForward : On initialise une passe FastBackward
	if (IsOnFastForwardRun())
	{
		if (IsSelectionEmpty())
			InitializeNextFastForwardRun();
		else
		{
			dMasterPreviousRunScore = dMasterCurrentScore;
			bMasterIsOnFastAddRun = false;
			nMasterRandomAttribute = masterBinarySliceSet->GetAttributeNumber() - 1;
		}
	}
	// Fin d'une passe FastBackward : On initialise un passe FastForward (potentiellement une nouvelle iteration
	// externe)
	else
		InitializeNextFastForwardRun();

	UpdateCurrentAttribute();

	// Mise a jour de la progression
	UpdateTaskProgressionLabel();
}

void SNBPredictorSNBTrainingTask::ComputeEmptySelectionScoreAndPrecisionEpsilon()
{
	const boolean bLocalTrace = false;

	require(not IsPrecisionEpsilonCalculated());

	// Mise a jour du score de la selection vide
	dMasterEmptySelectionModelCost = ComputeSelectionModelCost();
	dMasterEmptySelectionScore = dMasterEmptySelectionModelCost + dMasterEmptySelectionDataCost;

	// Calcul du epsilon de precision permettant de se comparer de facon relative a ce cout par defaut
	// La comparaison en "absolu" pose des problemes numeriques
	dMasterPrecisionEpsilon = 1e-2 * (1 + fabs(dMasterEmptySelectionScore));
	dMasterPrecisionEpsilon /= (1 + shared_learningSpec.GetLearningSpec()->GetInstanceNumber());

	// Initialisation du reste de couts et scores
	dMasterCurrentScore = dMasterEmptySelectionScore;
	dMasterCurrentModelCost = dMasterEmptySelectionScore;
	dMasterCurrentDataCost = dMasterEmptySelectionDataCost;
	dMasterPreviousRunScore = dMasterEmptySelectionScore;
	dMasterMapScore = dMasterEmptySelectionScore;

	// Trace de debbogage
	if (bLocalTrace)
	{
		cout << "Empty selection model cost = " << dMasterEmptySelectionModelCost << "\n";
		cout << "Empty selection data cost  = " << dMasterEmptySelectionDataCost << "\n";
		cout << "Precision Epsilon          = " << dMasterPrecisionEpsilon << "\n";
		cout << "----------------------------------------------\n";
		cout << "Variable\tModif\tWeight\tModelCost\tDataCost\tDecision\tNewCost\n";
	}

	ensure(IsPrecisionEpsilonCalculated());
}

boolean SNBPredictorSNBTrainingTask::CheckCurrentAttribute() const
{
	boolean bOk = true;

	if (not IsFastRunFinished())
	{
		bOk = bOk and masterRandomAttribute != NULL;
		bOk = bOk and 0 <= nMasterRandomAttribute and
		      nMasterRandomAttribute < masterBinarySliceSet->GetAttributeNumber();
	}
	else
	{
		bOk = bOk and masterRandomAttribute == NULL;
		bOk = (IsOnFastForwardRun() and nMasterRandomAttribute == masterBinarySliceSet->GetAttributeNumber()) or
		      (not IsOnFastForwardRun() and nMasterRandomAttribute == -1);
	}

	return bOk;
}

boolean SNBPredictorSNBTrainingTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;
	ALString sTmp;

	require(masterSnbPredictor != NULL);
	require(masterInitialDatabase != NULL);
	require(masterInitialDatabase->Check());

	// Remise a l'etat initial du LearningSpec, du domaine de travail et du RNG
	shared_learningSpec.FinalizeSpecification(masterInitialClass, masterInitialDatabase);
	KWClassDomain::SetCurrentDomain(masterInitialDomain);
	SetRandomSeed(nMasterRandomSeed);

	// Sauvegarde des resultats de l'entrainement si tout c'est bien deroule
	if (bProcessEndedCorrectly)
		MasterFinalizeTrainingAndReports();

	// Nettoyage des conteneurs partagees
	shared_odAttributeStats->GetObjectDictionary()->RemoveAll();
	shared_odAttributePairStats->GetObjectDictionary()->RemoveAll();
	shared_oaBinarySliceSetAttributes->GetObjectArray()->RemoveAll();
	shared_ivTargetValueIndexes.GetIntVector()->SetSize(0);
	shared_ivGrantedSlaveProcessIds.GetIntVector()->SetSize(0);

	// Nettoyage du SNBDataTableBinarySliceSet
	if (masterBinarySliceSet != NULL)
	{
		if (bProcessEndedCorrectly)
			masterBinarySliceSet->CleanWorkingData(false);
		delete masterBinarySliceSet;
		masterBinarySliceSet = NULL;
	}

	// En parallele : Nettoyage du fichier dictionnaire auxilier
	if (IsParallel())
	{
		bOk = FileService::RemoveFile(
		    FileService::GetURIFilePathName(shared_sRecoderClassDomainFileURI.GetValue()));
		if (not bOk)
			AddWarning(sTmp + "Failed to remove temporary dictionary " +
				   shared_sRecoderClassDomainFileURI.GetValue());
	}

	ensure(shared_learningSpec.GetLearningSpec()->Check());
	ensure(masterSnbPredictor->GetClassStats() != NULL);
	ensure(masterSnbPredictor->GetClassStats()->Check());
	ensure(masterSnbPredictor->GetClass() != NULL);
	ensure(masterSnbPredictor->GetClass()->Check());
	ensure(masterSnbPredictor->GetDatabase() != NULL);
	ensure(masterSnbPredictor->GetDatabase()->Check());
	ensure(masterBinarySliceSet == NULL);

	return true;
}

boolean SNBPredictorSNBTrainingTask::SlaveInitialize()
{
	int nSlaveProcess;
	boolean bOk = true;

	// Initialisation de l'identifiant du chunk attribue a l'esclave
	nSlaveProcessChunkIndex = -1;
	for (nSlaveProcess = 0; nSlaveProcess < shared_ivGrantedSlaveProcessIds.GetSize(); nSlaveProcess++)
	{
		if (GetProcessId() == shared_ivGrantedSlaveProcessIds.GetAt(nSlaveProcess))
			nSlaveProcessChunkIndex = nSlaveProcess;
	}

	// Initialisation du KWLearningSpec et de la SNBDataTableBinarySliceSet
	bOk = bOk and SlaveInitializeLearningSpec();
	if (bOk)
		bOk = bOk and SlaveInitializeDataTableBinarySliceSet();

	ensure(0 <= nSlaveProcessChunkIndex and nSlaveProcessChunkIndex < shared_nChunkNumber);
	return bOk;
}

boolean SNBPredictorSNBTrainingTask::SlaveInitializeLearningSpec()
{
	boolean bOk = true;
	boolean bIsRecoderClassFileLocal;
	ALString sRecoderClassTmpFilePath;

	// En parallele: Le domain doit etre vide
	// En sequentiel : Le domain doit contenir la classe de recodage
	require((IsParallel() and KWClassDomain::GetCurrentDomain()->GetClassNumber() == 0) or
		(not IsParallel() and
		 KWClassDomain::GetCurrentDomain()->LookupClass(shared_sRecoderClassName.GetValue()) != NULL and
		 shared_sRecoderClassDomainFileURI.GetValue() == ""));

	// En parallele : Transfert du fichier de dictionnaire du domaine de la classe de recodage, parsing et
	// compilation
	if (IsParallel())
	{
		bIsRecoderClassFileLocal =
		    GetLocalHostName() == FileService::GetURIHostName(shared_sRecoderClassDomainFileURI.GetValue());
		if (bIsRecoderClassFileLocal)
			sRecoderClassTmpFilePath =
			    FileService::GetURIFilePathName(shared_sRecoderClassDomainFileURI.GetValue());
		else
		{
			sRecoderClassTmpFilePath =
			    FileService::CreateUniqueTmpFile(shared_sRecoderClassName.GetValue() + ".kdic", this);
			bOk = bOk and PLRemoteFileService::CopyFile(shared_sRecoderClassDomainFileURI.GetValue(),
								    sRecoderClassTmpFilePath);
		}

		// Parsing du fichier dictionnaire
		if (bOk)
			bOk = bOk and KWClassDomain::GetCurrentDomain()->ReadFile(sRecoderClassTmpFilePath);

		// Recherche de la classe a traiter
		if (bOk)
		{
			slaveRecoderClass =
			    KWClassDomain::GetCurrentDomain()->LookupClass(shared_sRecoderClassName.GetValue());
			bOk = bOk and slaveRecoderClass != NULL;
			if (not bOk)
				AddError("Dictionary " + shared_sRecoderClassName.GetValue() + " not found");
		}

		// Verfication d'integrite du dictionnaire
		if (bOk)
			bOk = bOk and slaveRecoderClass->Check();

		// Compilation du dictionnaire
		if (bOk)
		{
			KWClassDomain::GetCurrentDomain()->Compile();
			bOk = bOk and slaveRecoderClass->IsCompiled();
			if (not bOk)
				AddError("Failed to compile dictionary " + shared_sRecoderClassName.GetValue());
		}

		// Nettoyage du fichier dictionnaire temporaire
		if (not bIsRecoderClassFileLocal)
			FileService::RemoveFile(sRecoderClassTmpFilePath);
	}
	// En sequentiel : Recherche de la classe dans le domaine courant et compilation
	else
	{
		slaveRecoderClass = KWClassDomain::GetCurrentDomain()->LookupClass(shared_sRecoderClassName.GetValue());
		bOk = bOk and slaveRecoderClass != NULL;
		if (bOk)
		{
			KWClassDomain::GetCurrentDomain()->Compile();
			bOk = bOk and slaveRecoderClass->IsCompiled();
			if (not bOk)
				AddError("Failed to compile dictionary " + shared_sRecoderClassName.GetValue());
		}
		else
			AddError("Dictionary " + shared_sRecoderClassName.GetValue() + " not found in current domain");
	}

	// Finalization du LearningSpec de l'esclave avec une database bidon
	if (bOk)
	{
		slaveDummyDatabase = new KWSTDatabase;
		slaveDummyDatabase->SetClassName(slaveRecoderClass->GetName());
		slaveDummyDatabase->SetDatabaseName(slaveRecoderClass->GetName());
		shared_learningSpec.GetLearningSpec()->SetCheckTargetAttribute(false);
		shared_learningSpec.FinalizeSpecification(slaveRecoderClass, slaveDummyDatabase);
	}

	ensure(not bOk or slaveRecoderClass != NULL);
	ensure(not bOk or slaveRecoderClass->IsCompiled());
	ensure(not bOk or shared_learningSpec.GetLearningSpec()->Check());
	return bOk;
}

boolean SNBPredictorSNBTrainingTask::SlaveInitializeDataTableBinarySliceSet()
{
	boolean bOk = true;
	int nAttribute;
	ObjectArray* oaAttributes;
	SNBDataTableBinarySliceSetAttribute* attribute;
	ObjectDictionary* odAttributeStats;
	ObjectDictionary* odAttributePairStats;
	KWAttributeStats* attributeStats;
	KWAttributePairStats* attributePairStats;
	ALString sTmp;

	require(IsSlaveProcess());
	require(slaveBinarySliceSet == NULL);
	require(shared_learningSpec.GetLearningSpec()->Check());
	require(slaveRecoderClass != NULL);
	require(slaveRecoderClass->Check());

	// Alias locaux pour la lisibilite
	oaAttributes = shared_oaBinarySliceSetAttributes->GetObjectArray();
	odAttributeStats = shared_odAttributeStats->GetObjectDictionary();
	odAttributePairStats = shared_odAttributePairStats->GetObjectDictionary();

	// Creation du binary slice set
	slaveBinarySliceSet = new SNBDataTableBinarySliceSet;

	// Reconstituon du nombre d'attributs initial et des valeurs de la cible
	slaveBinarySliceSet->nInitialAttributeNumber = shared_nInitialAttributeNumber;
	slaveBinarySliceSet->ivTargetValueIndexes.CopyFrom(shared_ivTargetValueIndexes.GetConstIntVector());

	// Reconstition du layout
	slaveBinarySliceSet->layout.Initialize(shared_nInstanceNumber, shared_nChunkNumber, shared_nAttributeNumber,
					       shared_nSliceNumber);

	// Reconstitution des attributs a partir des objets attributs et stats transferes
	for (nAttribute = 0; nAttribute < oaAttributes->GetSize(); nAttribute++)
	{
		// Recuperation des objets attributs (ils viennent sans spec et sans stats)
		attribute = cast(SNBDataTableBinarySliceSetAttribute*, oaAttributes->GetAt(nAttribute));

		// Recuperation des stats de l'attribut et construction de la table de probabilites conditionnelles
		// (il doit avoir un et un seul KWDataPreparationStats associe a un nom)
		attributeStats = cast(KWAttributeStats*, odAttributeStats->Lookup(attribute->GetNativeAttributeName()));
		attributePairStats =
		    cast(KWAttributePairStats*, odAttributePairStats->Lookup(attribute->GetNativeAttributeName()));
		assert(not(attributeStats == NULL and attributePairStats == NULL));
		assert(not(attributeStats != NULL and attributePairStats != NULL));
		if (attributeStats != NULL)
			attribute->dataPreparationStats = attributeStats;
		else
			attribute->dataPreparationStats = attributePairStats;
		attribute->dataPreparationStats->SetLearningSpec(shared_learningSpec.GetLearningSpec());
		attribute->conditionalProbas.ImportDataGridStats(
		    attribute->dataPreparationStats->GetPreparedDataGridStats(), false, true);
	}

	// Reconstitution du SNBDataTableBinarySliceSetSchema a parter des attributs reconstruits
	slaveBinarySliceSet->schema.InitializeFromAttributes(oaAttributes);

	// Initialisation du buffer de lecture de donnees pour le chunk l'esclave
	slaveBinarySliceSet->dataBuffer.SetLayout(&slaveBinarySliceSet->layout);
	slaveBinarySliceSet->dataBuffer.InitializeBuffer();
	slaveBinarySliceSet->nInitializedChunkIndex = GetSlaveChunkIndex();
	bOk = slaveBinarySliceSet->dataBuffer.InitializeOnlyAtChunk(GetSlaveChunkIndex(), slaveRecoderClass,
								    shared_dataTableSliceSet.GetDataTableSliceSet(),
								    &slaveBinarySliceSet->schema);

	// En parallele on decharge la classe de recodage
	if (IsParallel())
		SlaveInitializeUnloadRecoderClass();

	ensure(not bOk or IsSlaveDataTableBinarySliceSetInitialized());
	ensure(not bOk or slaveBinarySliceSet->Check());
	return bOk;
}

boolean SNBPredictorSNBTrainingTask::IsSlaveDataTableBinarySliceSetInitialized() const
{
	return slaveBinarySliceSet != NULL and slaveBinarySliceSet->dataPreparationClass == NULL and
	       slaveBinarySliceSet->schema.IsInitialized() and slaveBinarySliceSet->layout.IsInitialized() and
	       not slaveBinarySliceSet->randomizedAttributeIterator.IsInitialized() and
	       slaveBinarySliceSet->ivTargetValueIndexes.GetSize() > 0 and
	       slaveBinarySliceSet->nInitializedChunkIndex >= 0 and
	       slaveBinarySliceSet->dataBuffer.IsInitializedOnlyAtChunk(GetSlaveChunkIndex());
}

void SNBPredictorSNBTrainingTask::SlaveInitializeUnloadRecoderClass()
{
	KWClass* dummyClass;
	KWAttribute* dummyAttribute;

	require(IsParallel());
	require(slaveRecoderClass != NULL);

	// Creation d'une classe bidon
	dummyClass = new KWClass;
	dummyAttribute = new KWAttribute;
	dummyAttribute->SetName("DummyAttribute");
	dummyAttribute->SetType(KWType::Symbol);
	dummyClass->SetName("DummyClass");
	dummyClass->InsertAttribute(dummyAttribute);
	slaveDummyDatabase->SetClassName(dummyClass->GetName());
	slaveDummyDatabase->SetDatabaseName(dummyClass->GetName());

	// Remplacement de la classe de recodage par la classe bidon dans le learning spec
	slaveRecoderClass = NULL;
	KWClassDomain::GetCurrentDomain()->DeleteAllClasses();
	KWClassDomain::GetCurrentDomain()->InsertClass(dummyClass);
	KWClassDomain::GetCurrentDomain()->Compile();
	shared_learningSpec.FinalizeSpecification(dummyClass, slaveDummyDatabase);

	ensure(slaveRecoderClass == NULL);
	ensure(shared_learningSpec.GetLearningSpec()->Check());
}

boolean SNBPredictorSNBTrainingTask::SlaveProcess()
{
	require(IsSlaveDataTableBinarySliceSetInitialized());
	require(slaveBinarySliceSet->Check());

	return true;
}

boolean SNBPredictorSNBTrainingTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	require(IsSlaveDataTableBinarySliceSetInitialized());
	require(slaveBinarySliceSet->Check());
	require(slaveRecoderClass != NULL or IsParallel());

	// Nettoyage des objets de travail l'esclave
	if (slaveBinarySliceSet != NULL)
	{
		slaveBinarySliceSet->schema.oaAttributes.RemoveAll();
		delete slaveBinarySliceSet;
		slaveBinarySliceSet = NULL;
	}
	if (slaveDummyDatabase != NULL)
	{
		delete slaveDummyDatabase;
		slaveDummyDatabase = NULL;
	}

	// Nettoyage du domaine en parallele
	if (IsParallel())
		KWClassDomain::GetCurrentDomain()->DeleteAllClasses();
	// Nettoyage que du pointeur en sequentiel
	else
		slaveRecoderClass = NULL;

	ensure(slaveBinarySliceSet == NULL);
	ensure(slaveRecoderClass == NULL);
	return true;
}

int SNBPredictorSNBTrainingTask::GetSlaveChunkIndex() const
{
	require(IsRunning());

	return nSlaveProcessChunkIndex;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBPredictorSNBEnsembleTrainingTask

SNBPredictorSNBEnsembleTrainingTask::SNBPredictorSNBEnsembleTrainingTask()
{
	masterHardSelectionScorer = NULL;
	masterMapSelection = NULL;
	masterWeightCalculator = NULL;
	slaveSelectionScorer = NULL;
}

SNBPredictorSNBEnsembleTrainingTask::~SNBPredictorSNBEnsembleTrainingTask() {}

PLParallelTask* SNBPredictorSNBEnsembleTrainingTask::Create() const
{
	return new SNBPredictorSNBEnsembleTrainingTask;
}

const ALString SNBPredictorSNBEnsembleTrainingTask::GetTaskName() const
{
	return "Selective Naive Bayes Training (Legacy)";
}

boolean SNBPredictorSNBEnsembleTrainingTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	boolean bOk;

	bOk = SNBPredictorSNBTrainingTask::MasterPrepareTaskInput(dTaskPercent, bIsTaskFinished);

	if (IsOuterIterationFinished())
	{
		masterWeightCalculator->AddSelectionOptimizationRecord(
		    SNBAttributeSelectionOptimizationRecord::Final, NULL, dMasterModificationModelCost,
		    dMasterModificationDataCost, masterHardSelectionScorer->GetAttributeSelection());
	}

	return bOk;
}

boolean SNBPredictorSNBEnsembleTrainingTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	// Appel a la methode ancetre
	bOk = SNBPredictorSNBTrainingTask::MasterFinalize(bProcessEndedCorrectly);

	// Nettoyage des objets de travail du maitre
	if (masterHardSelectionScorer != NULL)
	{
		delete masterHardSelectionScorer;
		masterHardSelectionScorer = NULL;
	}
	if (masterMapSelection != NULL)
	{
		delete masterMapSelection;
		masterMapSelection = NULL;
	}
	if (masterWeightCalculator != NULL)
	{
		delete masterWeightCalculator;
		masterWeightCalculator = NULL;
	}

	return bOk;
}

longint SNBPredictorSNBEnsembleTrainingTask::ComputeMasterNecessaryMemory() const
{
	require(masterSnbPredictor != NULL);
	require(masterSnbPredictor->Check());
	require(masterSnbPredictor->GetTargetValueStats() != NULL);
	require(masterSnbPredictor->GetTargetValueStats()->GetAttributeNumber() > 0);

	// NB : La methode appelle ignore tous les parametres sauf nAttributeNumber
	//      lorsque bIncludeDataCostCalculator == false
	return SNBHardAttributeSelectionScorer::ComputeNecessaryMemory(
	    masterSnbPredictor->GetInstanceNumber(), masterSnbPredictor->GetTrainingAttributeNumber(),
	    masterSnbPredictor->GetTargetValueStats()->GetAttributeAt(0)->GetPartNumber(),
	    masterSnbPredictor->GetTargetAttributeType(), masterSnbPredictor->IsTargetGrouped(), false);
}

longint SNBPredictorSNBEnsembleTrainingTask::ComputeGlobalSlaveScorerNecessaryMemory() const
{
	longint lFullScorerMemory;
	longint lSelectionScorerBufferMemory;
	require(masterSnbPredictor != NULL);
	require(masterSnbPredictor->Check());
	require(masterSnbPredictor->GetTargetValueStats() != NULL);
	require(masterSnbPredictor->GetTargetValueStats()->GetAttributeNumber() > 0);

	// NB : La methode appelle ignore tous les parametres sauf nAttributeNumber
	//      lorsque bIncludeDataCostCalculator == false
	lFullScorerMemory = SNBHardAttributeSelectionScorer::ComputeNecessaryMemory(
	    masterSnbPredictor->GetInstanceNumber(), masterSnbPredictor->GetTrainingAttributeNumber(),
	    masterSnbPredictor->GetTargetValueStats()->GetAttributeAt(0)->GetPartNumber(),
	    masterSnbPredictor->GetTargetAttributeType(), masterSnbPredictor->IsTargetGrouped(), true);

	lSelectionScorerBufferMemory =
	    lFullScorerMemory -
	    SNBHardAttributeSelectionScorer::ComputeNecessaryMemory(
		masterSnbPredictor->GetInstanceNumber(), masterSnbPredictor->GetTrainingAttributeNumber(),
		masterSnbPredictor->GetTargetValueStats()->GetAttributeAt(0)->GetPartNumber(),
		masterSnbPredictor->GetTargetAttributeType(), masterSnbPredictor->IsTargetGrouped(), false);

	return lSelectionScorerBufferMemory;
}

longint SNBPredictorSNBEnsembleTrainingTask::ComputeSlaveScorerNecessaryMemory() const
{
	require(masterSnbPredictor != NULL);
	require(masterSnbPredictor->Check());
	require(masterSnbPredictor->GetTargetValueStats() != NULL);
	require(masterSnbPredictor->GetTargetValueStats()->GetAttributeNumber() > 0);

	// NB : La methode appelle ignore tous les parametres sauf nAttributeNumber
	//      lorsque bIncludeDataCostCalculator == false
	return SNBHardAttributeSelectionScorer::ComputeNecessaryMemory(
	    masterSnbPredictor->GetInstanceNumber(), masterSnbPredictor->GetTrainingAttributeNumber(),
	    masterSnbPredictor->GetTargetValueStats()->GetAttributeAt(0)->GetPartNumber(),
	    masterSnbPredictor->GetTargetAttributeType(), masterSnbPredictor->IsTargetGrouped(), false);
}

void SNBPredictorSNBEnsembleTrainingTask::MasterInitializeOptimizationVariables()
{
	double dStartNumber;

	// Appel a la methode ancetre
	SNBPredictorSNBTrainingTask::MasterInitializeOptimizationVariables();

	// Calculatrice du score du maitre (calculatrice de couts de donnes non initialise)
	masterHardSelectionScorer = new SNBHardAttributeSelectionScorer;
	masterHardSelectionScorer->SetLearningSpec(shared_learningSpec.GetLearningSpec());
	masterHardSelectionScorer->SetDataTableBinarySliceSet(masterBinarySliceSet);
	masterHardSelectionScorer->SetPriorWeight(masterSnbPredictor->GetSelectionParameters()->GetPriorWeight());
	masterHardSelectionScorer->SetConstructionCostEnabled(
	    masterSnbPredictor->GetSelectionParameters()->GetConstructionCost());
	masterHardSelectionScorer->SetPreparationCostEnabled(
	    masterSnbPredictor->GetSelectionParameters()->GetPreparationCost());

	// Calculatrice de poids
	masterWeightCalculator = new SNBAttributeSelectionWeightCalculator;
	masterWeightCalculator->SetDataTableBinarySliceSet(masterBinarySliceSet);
	if (masterSnbPredictor->GetSelectionParameters()->GetSelectionCriterion() == "CMA")
		masterWeightCalculator->SetWeightingMethod(
		    SNBAttributeSelectionWeightCalculator::WeightingMethod::PredictorCompressionRate);
	else if (masterSnbPredictor->GetSelectionParameters()->GetSelectionCriterion() == "MA")
		masterWeightCalculator->SetWeightingMethod(
		    SNBAttributeSelectionWeightCalculator::WeightingMethod::PredictorProb);
	else
		masterWeightCalculator->SetWeightingMethod(
		    SNBAttributeSelectionWeightCalculator::WeightingMethod::None);
	masterWeightCalculator->SetTraceLevel(masterSnbPredictor->GetSelectionParameters()->GetTraceLevel());
	masterWeightCalculator->SetTraceSelectedAttributes(
	    masterSnbPredictor->GetSelectionParameters()->GetTraceSelectedAttributes());
	masterWeightCalculator->Reset();

	// Nombre de starts : Si le niveau d'optimisation a ete specifie par l'utilisateur (i.e > 0) on le prend
	if (masterSnbPredictor->GetSelectionParameters()->GetOptimizationLevel() > 0)
		nMasterOuterIterationNumber = masterSnbPredictor->GetSelectionParameters()->GetOptimizationLevel();
	// Sinon on calcule le nombre de starts par ~ log2(K+1) + log2(N+1) + 1
	// L'iteration additionnelle est pour tenir en compte la premiere passe sans randomisation
	else
	{
		dStartNumber = log2(masterBinarySliceSet->GetInstanceNumber() + 1.0);
		dStartNumber += log2(masterBinarySliceSet->GetInitialAttributeNumber() + 1.0);
		nMasterOuterIterationNumber = int(ceil(dStartNumber)) + 1;
	}

	ensure(masterHardSelectionScorer->Check());
	ensure(masterWeightCalculator->Check());
}

void SNBPredictorSNBEnsembleTrainingTask::UpdateTaskProgressionLabel() const
{
	ALString sLabel;

	require(IsRunning());
	require(IsMasterProcess());

	sLabel = "Model No.";
	sLabel += IntToString(nMasterOuterIteration + 1);
	if (IsOnFastForwardRun())
		sLabel += ": adding attributes";
	else
		sLabel += ": removing attributes";
	TaskProgression::DisplayLabel(sLabel);
}

void SNBPredictorSNBEnsembleTrainingTask::UpdateSelection()
{
	require(IsMasterProcess());

	// Passe fast forward (ajout)
	if (IsOnFastForwardRun())
	{
		// Mise a jour du score de la modification
		masterHardSelectionScorer->AddAttribute(masterRandomAttribute);
		dMasterModificationModelCost = masterHardSelectionScorer->ComputeSelectionModelCost();
		dMasterModificationScore += dMasterModificationModelCost + dMasterModificationDataCost;
		masterWeightCalculator->AddSelectionOptimizationRecord(
		    SNBAttributeSelectionOptimizationRecord::AddAttribute, masterRandomAttribute,
		    dMasterModificationModelCost, dMasterModificationDataCost,
		    masterHardSelectionScorer->GetAttributeSelection());

		// Mise a jour de la selection si le score est ameliore
		if (dMasterModificationScore < dMasterCurrentScore - dMasterPrecisionEpsilon)
		{
			dMasterCurrentScore = dMasterModificationScore;
			dMasterCurrentModelCost = dMasterModificationModelCost;
			dMasterCurrentDataCost = dMasterModificationDataCost;
			bMasterUndoLastModification = false;
			masterWeightCalculator->AddSelectionOptimizationRecord(
			    SNBAttributeSelectionOptimizationRecord::BestAdd, masterRandomAttribute,
			    dMasterModificationModelCost, dMasterModificationDataCost,
			    masterHardSelectionScorer->GetAttributeSelection());
		}
		else
		{
			masterHardSelectionScorer->UndoLastModification();
			bMasterUndoLastModification = true;
		}

		// L'initialisation du scorer s'execute une seule fois par start, apres le premiere passe FastAdd
		if (bMasterInitializeSlaveScorers)
			bMasterInitializeSlaveScorers = false;
	}
	// Passe fast backward (enlevement)
	else
	{
		// Mise a jour du score de la modification
		masterHardSelectionScorer->RemoveAttribute(masterRandomAttribute);
		dMasterModificationModelCost = ComputeSelectionModelCost();
		dMasterModificationScore += dMasterModificationModelCost + dMasterModificationDataCost;
		masterWeightCalculator->AddSelectionOptimizationRecord(
		    SNBAttributeSelectionOptimizationRecord::Remove, masterRandomAttribute,
		    dMasterModificationModelCost, dMasterModificationDataCost,
		    masterHardSelectionScorer->GetAttributeSelection());

		// Mise a jour de la selection si le score est ameliore
		if (dMasterModificationScore < dMasterCurrentScore + dMasterPrecisionEpsilon)
		{

			dMasterCurrentScore = dMasterModificationScore;
			dMasterCurrentModelCost = dMasterModificationModelCost;
			dMasterCurrentDataCost = dMasterModificationDataCost;
			bMasterUndoLastModification = false;
			masterWeightCalculator->AddSelectionOptimizationRecord(
			    SNBAttributeSelectionOptimizationRecord::BestRemove, masterRandomAttribute,
			    dMasterModificationModelCost, dMasterModificationDataCost,
			    masterHardSelectionScorer->GetAttributeSelection());
		}
		else
		{
			masterHardSelectionScorer->UndoLastModification();
			bMasterUndoLastModification = true;
		}
	}
}

void SNBPredictorSNBEnsembleTrainingTask::UpdateCurrentAttribute()
{
	require(IsMasterProcess());
	require(not IsFastRunFinished());

	// Passe fast forward (ajout) : Le prochain attribut aleatoire *ne doit pas* etre selectionne
	if (IsOnFastForwardRun())
	{
		if (masterRandomAttribute != NULL)
			nMasterRandomAttribute++;
		while (nMasterRandomAttribute < masterBinarySliceSet->GetAttributeNumber())
		{
			masterRandomAttribute = masterBinarySliceSet->GetRandomAttributeAt(nMasterRandomAttribute);
			if (not SelectionContainsAttribute(masterRandomAttribute))
				break;
			nMasterRandomAttribute++;
		}
	}
	// Passe fast backward (enlevement) : Le prochain attribut aleatoire *doit* etre selectionne
	else
	{
		if (masterRandomAttribute != NULL)
			nMasterRandomAttribute--;
		while (nMasterRandomAttribute >= 0)
		{
			masterRandomAttribute = masterBinarySliceSet->GetRandomAttributeAt(nMasterRandomAttribute);
			if (SelectionContainsAttribute(masterRandomAttribute))
				break;
			nMasterRandomAttribute--;
		}
	}

	// Shuffle de l'iterateur aleatoire si l'on arrive a la fin d'une passe fast forward ou backward
	if (IsFastRunFinished())
	{
		masterRandomAttribute = NULL;
		masterBinarySliceSet->ShuffleRandomAttributeIterator();
	}

	ensure(CheckCurrentAttribute());
}

boolean
SNBPredictorSNBEnsembleTrainingTask::SelectionContainsAttribute(SNBDataTableBinarySliceSetAttribute* attribute) const
{
	require(IsRunning());
	require(IsMasterProcess());
	return masterHardSelectionScorer->GetAttributeSelection()->Contains(attribute);
}

boolean SNBPredictorSNBEnsembleTrainingTask::IsSelectionEmpty() const
{
	require(IsRunning());
	require(IsMasterProcess());
	return masterHardSelectionScorer->GetAttributeSelection()->GetAttributeNumber() == 0;
}

void SNBPredictorSNBEnsembleTrainingTask::InitializeNextFastForwardRun()
{
	require(not IsOnFastForwardRun());

	bMasterIsOnFastAddRun = true;
	nMasterRandomAttribute = 0;

	// Nouvelle passe FastForwardBackward ssi le score est ameliore par rapport a la passe FastForward d'avant
	if (dMasterCurrentScore < dMasterPreviousRunScore - dMasterPrecisionEpsilon)
		nMasterFastForwardBackwardRun++;
	else
		nMasterFastForwardBackwardRun = nMaxFastForwardBackwardRuns;
	dMasterPreviousRunScore = dMasterCurrentScore;

	// Fin de la passe MultiStart
	if (nMasterFastForwardBackwardRun == nMaxFastForwardBackwardRuns)
	{
		masterWeightCalculator->AddSelectionOptimizationRecord(
		    SNBAttributeSelectionOptimizationRecord::LocalOptimum, NULL, dMasterCurrentModelCost,
		    dMasterCurrentDataCost, masterHardSelectionScorer->GetAttributeSelection());

		// Mise a jour du MAP en cas d'amelioration
		if (dMasterCurrentScore < dMasterMapScore - dMasterPrecisionEpsilon)
		{
			masterWeightCalculator->AddSelectionOptimizationRecord(
			    SNBAttributeSelectionOptimizationRecord::GlobalOptimum, NULL, dMasterCurrentModelCost,
			    dMasterCurrentDataCost, masterHardSelectionScorer->GetAttributeSelection());

			dMasterMapScore = dMasterCurrentScore;
			if (masterMapSelection != NULL)
				delete masterMapSelection;
			masterMapSelection = masterHardSelectionScorer->CollectAttributeSelection();
		}

		// Reinitialisation de la passe MultiStart
		nMasterOuterIteration++;
		nMasterFastForwardBackwardRun = 0;
		dMasterCurrentScore = dMasterEmptySelectionScore;
		dMasterCurrentModelCost = dMasterEmptySelectionModelCost;
		dMasterCurrentDataCost = dMasterEmptySelectionDataCost;
		dMasterPreviousRunScore = dMasterEmptySelectionScore;
		bMasterInitializeSlaveScorers = true;
		masterHardSelectionScorer->InitializeWorkingData();
		masterWeightCalculator->AddSelectionOptimizationRecord(
		    SNBAttributeSelectionOptimizationRecord::ForcedRemoveAll, NULL, dMasterEmptySelectionModelCost,
		    dMasterEmptySelectionDataCost, masterHardSelectionScorer->GetAttributeSelection());

		// Shuffle additionel pour la compatibilite backwards
		masterBinarySliceSet->ShuffleRandomAttributeIterator();
	}

	// Mise a jour du label de la progression
	UpdateTaskProgressionLabel();
}

void SNBPredictorSNBEnsembleTrainingTask::ComputeEmptySelectionScoreAndPrecisionEpsilon()
{
	// Appel a la methode ancetre
	SNBPredictorSNBTrainingTask::ComputeEmptySelectionScoreAndPrecisionEpsilon();

	// Initialisation des enregistrements de l'optimisation
	masterWeightCalculator->AddSelectionOptimizationRecord(
	    SNBAttributeSelectionOptimizationRecord::Start, NULL, dMasterEmptySelectionModelCost,
	    dMasterEmptySelectionDataCost, masterHardSelectionScorer->GetAttributeSelection());
}

double SNBPredictorSNBEnsembleTrainingTask::ComputeSelectionModelCost()
{
	require(IsRunning());
	require(IsMasterProcess());
	return masterHardSelectionScorer->ComputeSelectionModelCost();
}

void SNBPredictorSNBEnsembleTrainingTask::MasterFinalizeTrainingAndReports()
{
	int nAttribute;
	SNBDataTableBinarySliceSetAttribute* attribute;
	KWSelectedAttributeReport* attributeReport;
	KWPredictorSelectionReport* selectionReport;
	int nMaxSelectedAttributes;
	ContinuousVector* cvAttributeWeights;

	require(IsMasterProcess());

	// Calcul des attributs en fonction de l'historique d'evaluation
	masterWeightCalculator->ComputeAttributeWeigths();
	cvAttributeWeights = masterWeightCalculator->CollectAttributeWeights();

	// Nettoyage du rapport de selection d'attributs
	selectionReport = masterSnbPredictor->GetPredictorSelectionReport();
	selectionReport->GetSelectedAttributes()->DeleteAll();

	// Creation des rapports des attributs selectionnes et ajout au rapport de selection
	for (nAttribute = 0; nAttribute < masterBinarySliceSet->GetAttributeNumber(); nAttribute++)
	{
		// Ajout du rapport de l'attribut s'il est selectionne ou s'il a un poids non nul
		attribute = masterBinarySliceSet->GetAttributeAt(nAttribute);
		if (cvAttributeWeights->GetAt(attribute->GetDataPreparationClassIndex()) > 0)
		{
			attributeReport = new KWSelectedAttributeReport;
			attributeReport->SetPreparedAttributeName(attribute->GetPreparedAttributeName());
			attributeReport->SetNativeAttributeName(attribute->GetNativeAttributeName());
			attributeReport->SetUnivariateEvaluation(attribute->GetLevel());
			attributeReport->SetWeight(
			    cvAttributeWeights->GetAt(attribute->GetDataPreparationClassIndex()));
			selectionReport->GetSelectedAttributes()->Add(attributeReport);
		}
	}

	// Tri du rapport d'attributs selectionnes selon poids dans le predicteur et level
	selectionReport->GetSelectedAttributes()->SetCompareFunction(KWLearningReportCompareSortValue);
	selectionReport->GetSelectedAttributes()->Sort();

	// Supression des attributs avec moins importants si demande
	nMaxSelectedAttributes = masterSnbPredictor->GetSelectionParameters()->GetMaxSelectedAttributeNumber();
	if (nMaxSelectedAttributes > 0)
	{
		for (nAttribute = nMaxSelectedAttributes;
		     nAttribute < selectionReport->GetSelectedAttributes()->GetSize(); nAttribute++)
		{
			attributeReport = cast(KWSelectedAttributeReport*,
					       selectionReport->GetSelectedAttributes()->GetAt(nAttribute));
			delete attributeReport;
		}
		selectionReport->GetSelectedAttributes()->SetSize(nMaxSelectedAttributes);
	}

	// Creation de la classe du predicteur
	if (masterWeightCalculator->GetWeightingMethod() !=
	    SNBAttributeSelectionWeightCalculator::WeightingMethod::None)
	{
		masterSnbPredictor->InternalTrainWNB(
		    masterBinarySliceSet->GetDataPreparationClass(),
		    masterBinarySliceSet->GetDataPreparationClass()->GetDataPreparationAttributes(),
		    cvAttributeWeights);
	}
	else
		masterSnbPredictor->InternalTrainMAP(masterBinarySliceSet->GetDataPreparationClass(),
						     masterMapSelection);

	// Ajout de la meta-data des variables au rapport
	masterSnbPredictor->FillPredictorAttributeMetaData(
	    masterSnbPredictor->GetTrainedPredictor()->GetPredictorClass());

	// Nettoyage
	delete cvAttributeWeights;
}

boolean SNBPredictorSNBEnsembleTrainingTask::SlaveInitialize()
{
	boolean bOk;

	require(slaveSelectionScorer == NULL);

	// Appel a la methode ancetre
	bOk = SNBPredictorSNBTrainingTask::SlaveInitialize();

	// Initialisation de scorer
	if (bOk)
	{
		slaveSelectionScorer = new SNBHardAttributeSelectionScorer;
		slaveSelectionScorer->SetLearningSpec(shared_learningSpec.GetLearningSpec());
		slaveSelectionScorer->SetDataTableBinarySliceSet(slaveBinarySliceSet);
		slaveSelectionScorer->SetPriorWeight(shared_dPriorWeight);
		slaveSelectionScorer->SetConstructionCostEnabled(shared_bIsConstructionCostEnabled);
		slaveSelectionScorer->SetPreparationCostEnabled(shared_bIsPreparationCostEnabled);
		bOk = bOk and slaveSelectionScorer->CreateDataCostCalculator();
	}
	if (bOk)
		slaveSelectionScorer->InitializeWorkingData();

	ensure(not bOk or (slaveSelectionScorer->IsDataCostCalculatorCreated() and slaveSelectionScorer->Check()));
	return bOk;
}

boolean SNBPredictorSNBEnsembleTrainingTask::SlaveProcess()
{
	boolean bOk = true;

	// Appel a la methode ancetre
	bOk = bOk and SNBPredictorSNBTrainingTask::SlaveProcess();

	if (bOk and input_bInitializeWorkingData)
		slaveSelectionScorer->InitializeWorkingData();

	if (bOk and input_nModificationAttribute >= 0)
	{
		if (input_bUndoLastModification and not input_bInitializeWorkingData)
			bOk = bOk and slaveSelectionScorer->UndoLastModification();

		if (bOk)
		{
			if (input_bIsForwardModification)
				bOk = bOk and slaveSelectionScorer->AddAttribute(
						  slaveBinarySliceSet->GetAttributeAt(input_nModificationAttribute));
			else
				bOk = bOk and slaveSelectionScorer->RemoveAttribute(
						  slaveBinarySliceSet->GetAttributeAt(input_nModificationAttribute));
		}
	}
	output_dDataCost = slaveSelectionScorer->ComputeSelectionDataCost();

	return bOk;
}

boolean SNBPredictorSNBEnsembleTrainingTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	// Appel a la methode ancetre
	bOk = SNBPredictorSNBTrainingTask::SlaveFinalize(bProcessEndedCorrectly);

	// Nettoyage du scorer de l'esclave
	if (slaveSelectionScorer != NULL)
	{
		delete slaveSelectionScorer;
		slaveSelectionScorer = NULL;
	}

	return bOk;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SNBPredictorSNBDirectTrainingTask

SNBPredictorSNBDirectTrainingTask::SNBPredictorSNBDirectTrainingTask()
{
	masterWeightedSelectionScorer = NULL;
	slaveWeightedSelectionScorer = NULL;
	dMasterModificationDeltaWeight = DBL_MAX;
	DeclareTaskInput(&input_dModificationDeltaWeight);
}

SNBPredictorSNBDirectTrainingTask::~SNBPredictorSNBDirectTrainingTask() {}

PLParallelTask* SNBPredictorSNBDirectTrainingTask::Create() const
{
	return new SNBPredictorSNBDirectTrainingTask;
}

const ALString SNBPredictorSNBDirectTrainingTask::GetTaskName() const
{
	return "Selective Naive Bayes Training";
}

boolean SNBPredictorSNBDirectTrainingTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	boolean bOk;

	bOk = SNBPredictorSNBTrainingTask::MasterPrepareTaskInput(dTaskPercent, bIsTaskFinished);

	if (not IsOuterIterationFinished() and IsPrecisionEpsilonCalculated())
		input_dModificationDeltaWeight = dMasterModificationDeltaWeight;

	return bOk;
}

boolean SNBPredictorSNBDirectTrainingTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	// Appel a la methode ancetre
	bOk = SNBPredictorSNBTrainingTask::MasterFinalize(bProcessEndedCorrectly);

	// Nettoyage des variables de travail du maitre
	if (masterWeightedSelectionScorer != NULL)
	{
		delete masterWeightedSelectionScorer;
		masterWeightedSelectionScorer = NULL;
	}

	return bOk;
}

longint SNBPredictorSNBDirectTrainingTask::ComputeMasterNecessaryMemory() const
{
	require(masterSnbPredictor != NULL);
	require(masterSnbPredictor->Check());
	require(masterSnbPredictor->GetTargetValueStats() != NULL);
	require(masterSnbPredictor->GetTargetValueStats()->GetAttributeNumber() > 0);

	// NB : La methode appellee ignore tous les parametres sauf nAttributeNumber
	//      lorsque bIncludeDataCostCalculator == false
	return SNBWeightedAttributeSelectionScorer::ComputeNecessaryMemory(
	    1, masterSnbPredictor->GetTrainingAttributeNumber(), 1, masterSnbPredictor->GetTargetAttributeType(),
	    masterSnbPredictor->IsTargetGrouped(), false);
}

longint SNBPredictorSNBDirectTrainingTask::ComputeGlobalSlaveScorerNecessaryMemory() const
{
	longint lFullScorerMemory;
	longint lSelectionScorerBufferMemory;

	require(masterSnbPredictor != NULL);
	require(masterSnbPredictor->Check());
	require(masterSnbPredictor->GetTargetValueStats() != NULL);
	require(masterSnbPredictor->GetTargetValueStats()->GetAttributeNumber() > 0);

	// NB : La methode appelle ignore tous les parametres sauf nAttributeNumber
	//      lorsque bIncludeDataCostCalculator == false
	lFullScorerMemory = SNBWeightedAttributeSelectionScorer::ComputeNecessaryMemory(
	    masterSnbPredictor->GetInstanceNumber(), masterSnbPredictor->GetTrainingAttributeNumber(),
	    masterSnbPredictor->GetTargetValueStats()->GetAttributeAt(0)->GetPartNumber(),
	    masterSnbPredictor->GetTargetAttributeType(), masterSnbPredictor->IsTargetGrouped(), true);

	lSelectionScorerBufferMemory =
	    lFullScorerMemory -
	    SNBWeightedAttributeSelectionScorer::ComputeNecessaryMemory(
		masterSnbPredictor->GetInstanceNumber(), masterSnbPredictor->GetTrainingAttributeNumber(),
		masterSnbPredictor->GetTargetValueStats()->GetAttributeAt(0)->GetPartNumber(),
		masterSnbPredictor->GetTargetAttributeType(), masterSnbPredictor->IsTargetGrouped(), false);

	return lSelectionScorerBufferMemory;
}
longint SNBPredictorSNBDirectTrainingTask::ComputeSlaveScorerNecessaryMemory() const
{
	require(masterSnbPredictor != NULL);
	require(masterSnbPredictor->Check());
	require(masterSnbPredictor->GetTargetValueStats() != NULL);
	require(masterSnbPredictor->GetTargetValueStats()->GetAttributeNumber() > 0);

	// NB : La methode appellee ignore tous les parametres sauf nAttributeNumber
	//      lorsque bIncludeDataCostCalculator == false
	return SNBWeightedAttributeSelectionScorer::ComputeNecessaryMemory(
	    masterSnbPredictor->GetInstanceNumber(), masterSnbPredictor->GetTrainingAttributeNumber(),
	    masterSnbPredictor->GetTargetValueStats()->GetAttributeAt(0)->GetPartNumber(),
	    masterSnbPredictor->GetTargetAttributeType(), masterSnbPredictor->IsTargetGrouped(), false);
}

void SNBPredictorSNBDirectTrainingTask::MasterInitializeOptimizationVariables()
{
	// Appel a la methode ancetre
	SNBPredictorSNBTrainingTask::MasterInitializeOptimizationVariables();

	// Delta poids de la modification courante
	dMasterModificationDeltaWeight = 0.5;

	// Calculatrice du score du maitre (calculatrice de couts de donnes non initialise)
	masterWeightedSelectionScorer = new SNBWeightedAttributeSelectionScorer;
	masterWeightedSelectionScorer->SetLearningSpec(shared_learningSpec.GetLearningSpec());
	masterWeightedSelectionScorer->SetDataTableBinarySliceSet(masterBinarySliceSet);
	masterWeightedSelectionScorer->SetPriorWeight(masterSnbPredictor->GetSelectionParameters()->GetPriorWeight());
	masterWeightedSelectionScorer->SetConstructionCostEnabled(
	    masterSnbPredictor->GetSelectionParameters()->GetConstructionCost());
	masterWeightedSelectionScorer->SetPreparationCostEnabled(
	    masterSnbPredictor->GetSelectionParameters()->GetPreparationCost());
	masterWeightedSelectionScorer->SetPriorExponent(
	    masterSnbPredictor->GetSelectionParameters()->GetPriorExponent());

	// Nombre maximale d'iterations externes
	nMasterOuterIterationNumber = int(ceil(log(masterBinarySliceSet->GetInstanceNumber() + 1)) / log(2.0));

	// NB : Il y a un erreur de parenthese dans la derniere formule mais on la garde pour la compatibilite
	// ascendante La formule correcte est la suivante (elle donne +1 dans certaines cas) :
	//
	//  nMasterOuterIterationNumber = int(ceil(log2(masterBinarySliceSet->GetInstanceNumber() + 1)));
}

void SNBPredictorSNBDirectTrainingTask::UpdateTaskProgressionLabel() const
{
	ALString sLabel;

	require(IsRunning());
	require(IsMasterProcess());

	if (IsOnFastForwardRun())
		sLabel = "Increasing variable weights by ";
	else
		sLabel = "Decreasing variable weights by ";
	sLabel += DoubleToString(dMasterModificationDeltaWeight);
	TaskProgression::DisplayLabel(sLabel);
}

void SNBPredictorSNBDirectTrainingTask::UpdateSelection()
{
	const boolean bLocalTrace = false;

	require(IsMasterProcess());

	// Passe FastForward
	if (IsOnFastForwardRun())
	{
		// Mise a jour du score de la modification
		masterWeightedSelectionScorer->IncreaseAttributeWeight(masterRandomAttribute,
								       dMasterModificationDeltaWeight);
		dMasterModificationModelCost = masterWeightedSelectionScorer->ComputeSelectionModelCost();
		dMasterModificationScore += dMasterModificationModelCost + dMasterModificationDataCost;

		// Mise a jour de la selection ssi le score est ameliore
		if (dMasterModificationScore < dMasterCurrentScore - dMasterPrecisionEpsilon)
		{
			dMasterCurrentScore = dMasterModificationScore;
			dMasterCurrentModelCost = dMasterModificationModelCost;
			dMasterCurrentDataCost = dMasterModificationDataCost;
			bMasterUndoLastModification = false;
		}
		else
		{
			masterWeightedSelectionScorer->UndoLastModification();
			bMasterUndoLastModification = true;
		}

		// L'initialisation du scorer s'execute une seule fois par start, apres le premiere passe FastForward
		if (bMasterInitializeSlaveScorers)
			bMasterInitializeSlaveScorers = false;
	}
	// Passe FastBackward
	else
	{
		// Mise a jour du score de la modification
		masterWeightedSelectionScorer->DecreaseAttributeWeight(masterRandomAttribute,
								       dMasterModificationDeltaWeight);
		dMasterModificationModelCost = ComputeSelectionModelCost();
		dMasterModificationScore += dMasterModificationModelCost + dMasterModificationDataCost;

		// Mise a jour de la selection ssi le score est ameliore
		if (dMasterModificationScore < dMasterCurrentScore + dMasterPrecisionEpsilon)
		{
			dMasterCurrentScore = dMasterModificationScore;
			dMasterCurrentModelCost = dMasterModificationModelCost;
			dMasterCurrentDataCost = dMasterModificationDataCost;
			bMasterUndoLastModification = false;
		}
		else
		{
			masterWeightedSelectionScorer->UndoLastModification();
			bMasterUndoLastModification = true;
		}
	}

	// Trace de deboggage
	if (bLocalTrace)
	{
		cout << masterRandomAttribute->GetNativeAttributeName() << "\t"
		     << (IsOnFastForwardRun() ? "increase" : "decrease") << "\t" << dMasterModificationDeltaWeight
		     << "\t" << dMasterModificationModelCost << "\t" << dMasterModificationDataCost << "\t"
		     << (bMasterUndoLastModification ? "rejected" : "accepted");
		if (not bMasterUndoLastModification)
			cout << "\t" << dMasterCurrentScore;
		cout << "\n";
	}
}

void SNBPredictorSNBDirectTrainingTask::UpdateCurrentAttribute()
{
	require(IsMasterProcess());
	require(not IsFastRunFinished());

	// Passe FastForward
	if (IsOnFastForwardRun())
	{
		if (masterRandomAttribute != NULL)
			nMasterRandomAttribute++;

		if (nMasterRandomAttribute < masterBinarySliceSet->GetAttributeNumber())
			masterRandomAttribute = masterBinarySliceSet->GetRandomAttributeAt(nMasterRandomAttribute);
	}
	// Passe FastBackward : Le prochain attribut aleatoire *doit* etre selectionne
	else
	{
		if (masterRandomAttribute != NULL)
			nMasterRandomAttribute--;
		while (nMasterRandomAttribute >= 0)
		{
			masterRandomAttribute = masterBinarySliceSet->GetRandomAttributeAt(nMasterRandomAttribute);
			if (SelectionContainsAttribute(masterRandomAttribute))
				break;
			nMasterRandomAttribute--;
		}
	}

	// Shuffle de l'iterateur aleatoire si l'on arrive a la fin d'une passe FastForward ou FastBackward
	if (IsFastRunFinished())
	{
		masterRandomAttribute = NULL;
		masterBinarySliceSet->ShuffleRandomAttributeIterator();
	}

	ensure(CheckCurrentAttribute());
}

boolean
SNBPredictorSNBDirectTrainingTask::SelectionContainsAttribute(SNBDataTableBinarySliceSetAttribute* attribute) const
{
	require(IsRunning());
	require(IsMasterProcess());
	return masterWeightedSelectionScorer->GetAttributeSelection()->Contains(attribute);
}

boolean SNBPredictorSNBDirectTrainingTask::IsSelectionEmpty() const
{
	require(IsRunning());
	require(IsMasterProcess());
	return masterWeightedSelectionScorer->GetAttributeSelection()->GetAttributeNumber() == 0;
}

void SNBPredictorSNBDirectTrainingTask::InitializeNextFastForwardRun()
{
	bMasterIsOnFastAddRun = true;
	nMasterRandomAttribute = 0;

	// Nouvelle passe FastForwardBackward ssi le score est ameliore par rapport a la passe FastForward d'avant
	if (dMasterCurrentScore < dMasterPreviousRunScore - dMasterPrecisionEpsilon)
		nMasterFastForwardBackwardRun++;
	else
		nMasterFastForwardBackwardRun = nMaxFastForwardBackwardRuns;
	dMasterPreviousRunScore = dMasterCurrentScore;

	// Fin de l'iteration externe
	if (nMasterFastForwardBackwardRun == nMaxFastForwardBackwardRuns)
	{
		// Reinitialisation de l'iteration externe et mise a jour du poids marginal
		nMasterOuterIteration++;
		nMasterFastForwardBackwardRun = 0;
		dMasterModificationDeltaWeight = 1.0 / pow(2.0, nMasterOuterIteration + 1);
	}
}

double SNBPredictorSNBDirectTrainingTask::ComputeSelectionModelCost()
{
	require(IsRunning());
	require(IsMasterProcess());
	return masterWeightedSelectionScorer->ComputeSelectionModelCost();
}

void SNBPredictorSNBDirectTrainingTask::MasterFinalizeTrainingAndReports()
{
	ContinuousVector cvAttributeWeights;
	int nAttribute;
	SNBDataTableBinarySliceSetAttribute* attribute;
	KWSelectedAttributeReport* attributeReport;
	KWPredictorSelectionReport* selectionReport;
	int nMaxSelectedAttributes;
	KWDataPreparationAttribute* dataPreparationAttribute;
	ObjectArray oaSelectedAttributes;
	ObjectArray oaSelectedDataPreparationAttributes;
	ObjectArray* oaDataPreparationAttributes;
	NumericKeyDictionary nkdSelectedDataPreparationAttributeSet;

	require(IsMasterProcess());
	require(masterBinarySliceSet->Check());
	require(masterWeightedSelectionScorer->GetAttributeSelection()->GetAttributeNumber() > 0);

	// Nettoyage du rapport de selection d'attributs
	selectionReport = masterSnbPredictor->GetPredictorSelectionReport();
	selectionReport->GetSelectedAttributes()->DeleteAll();

	// Creation des rapports des attributs selectionnes et ajout au rapport de selection
	// Memorisation des noms d'attributs selectionnees
	for (nAttribute = 0; nAttribute < masterBinarySliceSet->GetAttributeNumber(); nAttribute++)
	{
		attribute = masterBinarySliceSet->GetAttributeAt(nAttribute);

		// Ajout du rapport de l'attribut s'il a un poids non nul
		if (masterWeightedSelectionScorer->GetAttributeSelection()->Contains(attribute))
		{
			attributeReport = new KWSelectedAttributeReport;
			attributeReport->SetPreparedAttributeName(attribute->GetPreparedAttributeName());
			attributeReport->SetNativeAttributeName(attribute->GetNativeAttributeName());
			attributeReport->SetUnivariateEvaluation(attribute->GetLevel());
			attributeReport->SetWeight(
			    masterWeightedSelectionScorer->GetAttributeSelection()->GetAttributeWeightAt(attribute));
			selectionReport->GetSelectedAttributes()->Add(attributeReport);
		}
	}
	selectionReport->SetUsedAttributeNumber(
	    masterWeightedSelectionScorer->GetAttributeSelection()->GetAttributeNumber());

	// Tri du rapport d'attributs selectionnes selon poids dans le predicteur et level
	selectionReport->GetSelectedAttributes()->SetCompareFunction(KWLearningReportCompareSortValue);
	selectionReport->GetSelectedAttributes()->Sort();

	// Supression des attributs avec les moins importants si demande
	nMaxSelectedAttributes = masterSnbPredictor->GetSelectionParameters()->GetMaxSelectedAttributeNumber();
	if (0 < nMaxSelectedAttributes and nMaxSelectedAttributes < selectionReport->GetSelectedAttributes()->GetSize())
	{
		for (nAttribute = nMaxSelectedAttributes;
		     nAttribute < selectionReport->GetSelectedAttributes()->GetSize(); nAttribute++)
		{
			attributeReport = cast(KWSelectedAttributeReport*,
					       selectionReport->GetSelectedAttributes()->GetAt(nAttribute));
			attribute =
			    masterBinarySliceSet->GetAttributeAtNativeName(attributeReport->GetNativeAttributeName());
			delete attributeReport;
		}
		selectionReport->GetSelectedAttributes()->SetSize(nMaxSelectedAttributes);
		selectionReport->SetUsedAttributeNumber(nMaxSelectedAttributes);
	}

	// Creation d'un tableau des SNBDataTableBinarySliceSetAttribute selectionnes
	oaSelectedAttributes.SetSize(selectionReport->GetUsedAttributeNumber());
	for (nAttribute = 0; nAttribute < selectionReport->GetUsedAttributeNumber(); nAttribute++)
	{
		attributeReport =
		    cast(KWSelectedAttributeReport*, selectionReport->GetSelectedAttributes()->GetAt(nAttribute));
		attribute = masterBinarySliceSet->GetAttributeAtNativeName(attributeReport->GetNativeAttributeName());
		oaSelectedAttributes.SetAt(nAttribute, attribute);
	}

	// Tri des SNBDataTableBinarySliceSetAtribute's par index de la classe de preparation
	oaSelectedAttributes.SetCompareFunction(SNBDataTableBinarySliceSetAttributeCompareDataPreparationClassIndex);
	oaSelectedAttributes.Sort();

	// Creation du tableau de KWDataPreparationAttribute's selectiones et ses poids
	oaSelectedDataPreparationAttributes.SetSize(selectionReport->GetUsedAttributeNumber());
	cvAttributeWeights.SetSize(selectionReport->GetUsedAttributeNumber());
	oaDataPreparationAttributes = masterBinarySliceSet->GetDataPreparationClass()->GetDataPreparationAttributes();
	for (nAttribute = 0; nAttribute < oaSelectedAttributes.GetSize(); nAttribute++)
	{
		attribute = cast(SNBDataTableBinarySliceSetAttribute*, oaSelectedAttributes.GetAt(nAttribute));
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*,
			 oaDataPreparationAttributes->GetAt(attribute->GetDataPreparationClassIndex()));
		oaSelectedDataPreparationAttributes.SetAt(nAttribute, dataPreparationAttribute);
		cvAttributeWeights.SetAt(
		    nAttribute,
		    masterWeightedSelectionScorer->GetAttributeSelection()->GetAttributeWeightAt(attribute));
	}

	// Creation de la classe du predicteur
	masterSnbPredictor->GetTrainedPredictor()->SetPredictorClass(
	    masterBinarySliceSet->GetDataPreparationClass()->GetDataPreparationClass(),
	    masterSnbPredictor->GetTargetAttributeType(), masterSnbPredictor->GetObjectLabel());
	if (masterSnbPredictor->GetTargetAttributeType() == KWType::Symbol)
	{
		masterSnbPredictor->InternalTrainWNBClassifier(
		    masterSnbPredictor->GetTrainedClassifier(), masterBinarySliceSet->GetDataPreparationClass(),
		    &oaSelectedDataPreparationAttributes, &cvAttributeWeights);
	}
	else if (masterSnbPredictor->GetTargetAttributeType() == KWType::Continuous)
	{
		masterSnbPredictor->InternalTrainWNBRegressor(
		    masterSnbPredictor->GetTrainedRegressor(), masterBinarySliceSet->GetDataPreparationClass(),
		    &oaSelectedDataPreparationAttributes, &cvAttributeWeights);
	}
	masterSnbPredictor->FillPredictorAttributeMetaData(
	    masterSnbPredictor->GetTrainedPredictor()->GetPredictorClass());
}

boolean SNBPredictorSNBDirectTrainingTask::SlaveInitialize()
{
	boolean bOk;

	// Appel a la methode ancetre
	bOk = SNBPredictorSNBTrainingTask::SlaveInitialize();

	// Initialisation de scorer
	if (bOk)
	{
		slaveWeightedSelectionScorer = new SNBWeightedAttributeSelectionScorer;
		slaveWeightedSelectionScorer->SetLearningSpec(shared_learningSpec.GetLearningSpec());
		slaveWeightedSelectionScorer->SetDataTableBinarySliceSet(slaveBinarySliceSet);
		slaveWeightedSelectionScorer->SetPriorWeight(shared_dPriorWeight);
		slaveWeightedSelectionScorer->SetConstructionCostEnabled(shared_bIsConstructionCostEnabled);
		slaveWeightedSelectionScorer->SetPreparationCostEnabled(shared_bIsPreparationCostEnabled);
		bOk = bOk and slaveWeightedSelectionScorer->CreateDataCostCalculator();
	}
	if (bOk)
		slaveWeightedSelectionScorer->InitializeWorkingData();

	ensure(not bOk or
	       (slaveWeightedSelectionScorer->IsDataCostCalculatorCreated() and slaveWeightedSelectionScorer->Check()));
	return bOk;
}

boolean SNBPredictorSNBDirectTrainingTask::SlaveProcess()
{
	boolean bOk = true;

	// Appel a la methode ancetre
	bOk = bOk and SNBPredictorSNBTrainingTask::SlaveProcess();

	// (Re)initialisation du scorer si demande
	if (bOk and input_bInitializeWorkingData)
		slaveWeightedSelectionScorer->InitializeWorkingData();

	// Modification d'un poids d'un attribut si demande
	if (bOk and input_nModificationAttribute >= 0)
	{
		// Annulation de la derniere modification si demande
		if (input_bUndoLastModification and not input_bInitializeWorkingData)
			bOk = bOk and slaveWeightedSelectionScorer->UndoLastModification();

		// Modification du poids de l'attribut
		if (bOk)
		{
			if (input_bIsForwardModification)
				bOk = bOk and slaveWeightedSelectionScorer->IncreaseAttributeWeight(
						  slaveBinarySliceSet->GetAttributeAt(input_nModificationAttribute),
						  input_dModificationDeltaWeight);
			else
				bOk = bOk and slaveWeightedSelectionScorer->DecreaseAttributeWeight(
						  slaveBinarySliceSet->GetAttributeAt(input_nModificationAttribute),
						  input_dModificationDeltaWeight);
		}
	}
	// Calcul du cout de donnees
	output_dDataCost = slaveWeightedSelectionScorer->ComputeSelectionDataCost();

	return bOk;
}

boolean SNBPredictorSNBDirectTrainingTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	require(slaveWeightedSelectionScorer != NULL);

	// Nettoyage du scorer de l'esclave
	delete slaveWeightedSelectionScorer;
	slaveWeightedSelectionScorer = NULL;

	// Appel a la methode ancetre
	bOk = SNBPredictorSNBTrainingTask::SlaveFinalize(bProcessEndedCorrectly);

	return bOk;
}
