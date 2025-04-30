// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SNBPredictorSelectiveNaiveBayesTrainingTask.h"

SNBPredictorSelectiveNaiveBayesTrainingTask::SNBPredictorSelectiveNaiveBayesTrainingTask()
{
	// Initialisation des variables du maitre
	masterSnbPredictor = NULL;
	masterInitialClass = NULL;
	masterInitialDomain = NULL;
	masterInitialDatabase = NULL;
	masterBinarySliceSet = NULL;
	masterRandomAttribute = NULL;
	dMasterModificationScore = DBL_MAX;
	dMasterModificationDataCost = DBL_MAX;
	dMasterModificationModelCost = DBL_MAX;
	dMasterModificationDeltaWeight = DBL_MAX;
	dMasterCurrentScore = DBL_MAX;
	dMasterCurrentModelCost = DBL_MAX;
	dMasterCurrentDataCost = DBL_MAX;
	dMasterEmptySelectionScore = DBL_MAX;
	dMasterEmptySelectionModelCost = DBL_MAX;
	dMasterEmptySelectionDataCost = DBL_MAX;
	dMasterMapScore = DBL_MAX;
	dMasterLastFFBWRunScore = DBL_MAX;
	nMasterOuterIteration = -1;
	nMasterOuterIterationNumber = -1;
	nMasterFastForwardBackwardRun = -1;
	nMasterRandomAttribute = -1;
	bMasterUndoLastModification = false;
	dMasterPrecisionEpsilon = -1.0;
	bMasterInitializeSlaveScorers = false;
	nMasterTaskState = TaskState::PrecisionEpsilonComputation;
	nMasterRandomSeed = 1;
	nMasterFastRunStepFinishedTaskNumber = 0;
	dMasterTaskProgress = 0;
	bMasterIsTrainingSuccessfulWithoutRunningTask = false;
	masterWeightedSelectionScorer = NULL;

	// Initialisation des variables de l'esclave
	nSlaveProcessChunkIndex = -1;
	slaveRecoderClass = NULL;
	slaveDummyDatabase = NULL;
	slaveBinarySliceSet = NULL;
	slaveWeightedSelectionScorer = NULL;

	// Declaration des variables partagees
	DeclareSharedParameter(&shared_learningSpec);
	DeclareSharedParameter(&shared_sRecoderClassName);
	DeclareSharedParameter(&shared_lMaxSparseValuesPerBlock);
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
	DeclareSharedParameter(&shared_dPriorExponent);
	DeclareSharedParameter(&shared_bIsConstructionCostEnabled);
	DeclareSharedParameter(&shared_bIsPreparationCostEnabled);

	// Declaration des entrees et sortie des taches
	DeclareTaskInput(&input_nTaskState);
	DeclareTaskInput(&input_nModificationAttribute);
	DeclareTaskInput(&input_dModificationDeltaWeight);
	DeclareTaskInput(&input_bUndoLastModification);
	DeclareTaskInput(&input_bInitializeWorkingData);
	DeclareTaskOutput(&output_dDataCost);
}

SNBPredictorSelectiveNaiveBayesTrainingTask::~SNBPredictorSelectiveNaiveBayesTrainingTask()
{
	delete shared_oaBinarySliceSetAttributes;
	delete shared_odAttributeStats;
	delete shared_odAttributePairStats;
}

PLParallelTask* SNBPredictorSelectiveNaiveBayesTrainingTask::Create() const
{
	return new SNBPredictorSelectiveNaiveBayesTrainingTask;
}

const ALString SNBPredictorSelectiveNaiveBayesTrainingTask::GetTaskName() const
{
	return "Selective Naive Bayes Training";
}

void SNBPredictorSelectiveNaiveBayesTrainingTask::InternalTrain(SNBPredictorSelectiveNaiveBayes* snbPredictor)
{
	int nTrainingAttributeNumber;
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
	nTrainingAttributeNumber = masterSnbPredictor->ComputeTrainingAttributeNumber();
	if (nTrainingAttributeNumber > 0)
	{
		// Debut timer apprentissage
		timerTraining.Start();

		// Predicteur univarie s'il n'y a que un attribut informatif ou un seul attribut a evaluer
		if (nTrainingAttributeNumber == 1)
		{
			masterSnbPredictor->InternalTrainFinalizeWithUnivariatePredictor();
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
			shared_dPriorExponent = masterSnbPredictor->GetSelectionParameters()->GetPriorExponent();
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
			AddWarning("Interrupted by user after " + sTimeString);
		else
			AddError("Interrupted because of errors");
	}
	// S'il n'y a pas d'attribut informatif on entraine un predicteur vide
	else
	{
		masterSnbPredictor->InternalTrainFinalizeWithEmptyPredictor();
		bMasterIsTrainingSuccessfulWithoutRunningTask = true;
	}

	// Nettoyage du predicteur appelant
	masterSnbPredictor = NULL;

	ensure(masterSnbPredictor == NULL);
	ensure(masterInitialClass == NULL);
	ensure(masterInitialDatabase == NULL);
}

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::IsTrainingSuccessful() const
{
	return bMasterIsTrainingSuccessfulWithoutRunningTask or IsJobSuccessful();
}

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeResourceRequirements()
{
	const boolean bDisplay = false;
	const int nAbsoluteMaxSlaveProcessNumber = 10000;
	int nMaxSlaveProcessNumber;
	int nMaxSliceNumber;
	longint lSharedMinMemory;
	longint lSharedMaxMemory;
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
	if (bDisplay)
		cout << "Tracing resource estimations\n";

	// Estimation du nombre optimal de processus esclaves pour l'apprentissage
	nMaxSlaveProcessNumber = ComputeMaxSlaveProcessNumber(nAbsoluteMaxSlaveProcessNumber);

	// Estimation du nombre minimal de slices
	nMaxSliceNumber = ComputeMaxSliceNumber();

	// Estimation de la memoire partagee
	// Elle est croissant avec la taille du buffer du sliceSet
	lSharedMinMemory = ComputeSharedNecessaryMemory(MemSegmentByteSize);
	lSharedMaxMemory = ComputeSharedNecessaryMemory(BufferedFile::nDefaultBufferSize);

	// Estimation de la memoire pour le maitre
	lMasterMemory = ComputeMasterNecessaryMemory();

	// Estimation de la memoire globale des esclaves
	// Elle est decroissante avec le nombre de slices et avec la taille du buffer du sliceSet
	lGlobalSlaveMinMemory = ComputeGlobalSlaveNecessaryMemory(nMaxSliceNumber, BufferedFile::nDefaultBufferSize);
	lGlobalSlaveMaxMemory = ComputeGlobalSlaveNecessaryMemory(1, MemSegmentByteSize);

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
	GetResourceRequirements()->GetSharedRequirement()->GetMemory()->SetMin(lSharedMinMemory);
	GetResourceRequirements()->GetSharedRequirement()->GetMemory()->SetMax(lSharedMaxMemory);
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->Set(lMasterMemory);
	GetResourceRequirements()->GetGlobalSlaveRequirement()->GetMemory()->SetMin(lGlobalSlaveMinMemory);
	GetResourceRequirements()->GetGlobalSlaveRequirement()->GetMemory()->SetMax(lGlobalSlaveMaxMemory);
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->Set(lSlaveMemory);
	GetResourceRequirements()->GetMasterRequirement()->GetDisk()->Set(lMasterDisk);
	GetResourceRequirements()->GetGlobalSlaveRequirement()->GetDisk()->Set(lGlobalSlaveDisk);
	GetResourceRequirements()->GetSlaveRequirement()->GetDisk()->Set(lSlaveDisk);
	GetResourceRequirements()->SetMemoryAllocationPolicy(RMTaskResourceRequirement::globalPreferred);

	// Trace de deboggage
	if (bDisplay)
	{
		cout << "proc max                   = " << nMaxSlaveProcessNumber << "\n";
		cout << "slice max                  = " << nMaxSliceNumber << "\n";
		cout << "shared min             mem = " << LongintToHumanReadableString(lSharedMinMemory) << "\n";
		cout << "shared max             mem = " << LongintToHumanReadableString(lSharedMaxMemory) << "\n";
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

int SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeMaxSlaveProcessNumber(int nAbsoluteMaxSlaveProcessNumber) const
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
	nAttributeNumber = (longint)masterSnbPredictor->ComputeTrainingAttributeNumber();
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

	// Optimum parallele : recherche en arriere d'un nombre de processus moindre sans trop de perte en temps
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
	// Optimum sequentiel : On reste en sequentiel
	else
		nBestProcessNumber = 1;

	return nBestProcessNumber;
}

int SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeMaxSliceNumber() const
{
	return max(1, int(sqrt(masterSnbPredictor->ComputeTrainingAttributeNumber())));
}

longint SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeSharedNecessaryMemory(longint lSliceSetBufferMemory)
{
	const boolean bDisplay = false;
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
	lRecodingObjectsMemory = ComputeRecodingObjectsNecessaryMemory(lSliceSetBufferMemory);

	// Total de memoire partagee
	lSharedMemory = lBinarySliceSetSchemaMemory + lOverallAttributeStatsMemory + lRecodingObjectsMemory;

	// Trace de deboggage
	if (bDisplay)
	{
		cout << "Shared memory estimation (s.set buffer size "
		     << LongintToHumanReadableString(lSliceSetBufferMemory) << ") :\n";
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

longint SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeMasterNecessaryMemory() const
{
	require(masterSnbPredictor != NULL);
	require(masterSnbPredictor->Check());
	require(masterSnbPredictor->GetTargetValueStats() != NULL);
	require(masterSnbPredictor->GetTargetValueStats()->GetAttributeNumber() > 0);

	// NB : La methode appellee ignore tous les parametres sauf nAttributeNumber
	//      lorsque bIncludeDataCostCalculator == false
	return SNBAttributeSelectionScorer::ComputeNecessaryMemory(
	    1, masterSnbPredictor->ComputeTrainingAttributeNumber(), 1, masterSnbPredictor->GetTargetAttributeType(),
	    masterSnbPredictor->IsTargetGrouped(), false);
}

longint SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeGlobalSlaveNecessaryMemory(int nSliceNumber,
										       longint lSliceSetBufferMemory)
{
	const boolean bDisplay = false;
	int nInstanceNumber;
	int nAttributeNumber;
	int nSparseAttributeNumber;
	longint lGlobalDataCostCalculatorMemory;
	longint lGlobalBinarySliceSetChunkBufferMemory;
	longint lRecodingObjectsMemory;
	longint lGlobalSlaveMemory;
	double dSparseMemoryFactor;
	IntVector* ivSparseMissingValueNumberPerAttribute;

	// Initialisation variables locales
	nInstanceNumber = masterSnbPredictor->GetInstanceNumber();
	nAttributeNumber = masterSnbPredictor->ComputeTrainingAttributeNumber();
	nSparseAttributeNumber = masterSnbPredictor->ComputeTrainingSparseAttributeNumber();
	dSparseMemoryFactor = masterSnbPredictor->ComputeSparseMemoryFactor();
	ivSparseMissingValueNumberPerAttribute =
	    masterSnbPredictor->ComputeTrainingSparseMissingValueNumberPerAttribute();

	// La memoire global du buffer et du scorer s'estime
	// avec les estimations des objets necessaires avec un seul chunk
	lGlobalDataCostCalculatorMemory = ComputeGlobalSlaveScorerNecessaryMemory();
	lGlobalBinarySliceSetChunkBufferMemory = SNBDataTableBinarySliceSetChunkBuffer::ComputeNecessaryMemory(
	    nInstanceNumber, 1, ivSparseMissingValueNumberPerAttribute, nAttributeNumber - nSparseAttributeNumber,
	    nSliceNumber, dSparseMemoryFactor, false);

	// Nettoyage vecteur des comptes des valeurs sparse
	delete ivSparseMissingValueNumberPerAttribute;
	ivSparseMissingValueNumberPerAttribute = NULL;

	// Estimation de la memoire necessaire pour le recodage
	lRecodingObjectsMemory = ComputeRecodingObjectsNecessaryMemory(lSliceSetBufferMemory);

	// Rationale de l'estimation : d'abord deux faits
	//   1) La memoire pour le recodage est deja demandee en shared
	//   2) La memoire pour le recodage et celle de la calculatrice ne cohabitent pas
	// Donc il faut seulement demander le delta necessaire pour la calculatrice
	lGlobalSlaveMemory =
	    lGlobalBinarySliceSetChunkBufferMemory + max(lGlobalDataCostCalculatorMemory - lRecodingObjectsMemory, 0ll);

	// NB: Cette chiffre est sur-estimee dans le cas de plus d'un esclave. La vraie quantite necessaire est
	//
	//   lTrueGlobalSlaveMemory = lGlobalBinarySliceSetBufferMemory
	//                            + max(lGlobalDataCostCalculatorMemory - nSlaveNumber * lRecodingObjectsMemory, 0ll)
	//
	// car chaque esclave demande un dictionnaire.
	// Neanmoins, si l'on prends notre l'estimation avec M slices en tant demande minimal de memoire on a la garantie que pour
	// n'importe quel nombre de processus il y a une nombre de slices ou on peut tourner la tache (M slices dans le pire cas).
	// La raison est que lTrueGlobalSlaveMemory <= lGlobalSlaveMemory pour n'importe quel nombre d'esclaves et slices.
	//
	// Le compromis est que on interdit certains solutions avec un moindre nombre de slices.

	// Trace de deboggage
	if (bDisplay)
	{
		cout << "Global slave memory estimation (" << nSliceNumber << " slices, s.set buffer "
		     << LongintToHumanReadableString(lSliceSetBufferMemory) << "):\n";
		cout << "sparse memory factor          = " << masterSnbPredictor->ComputeSparseMemoryFactor() << "\n";
		cout << "binary slice set buffer   mem = "
		     << LongintToHumanReadableString(lGlobalBinarySliceSetChunkBufferMemory) << "\n";
		cout << "data cost calculator      mem = "
		     << LongintToHumanReadableString(lGlobalDataCostCalculatorMemory) << "\n";
		cout << "recoding objects          mem = " << LongintToHumanReadableString(lRecodingObjectsMemory)
		     << "\n";
		cout << "global slave              mem = " << LongintToHumanReadableString(lGlobalSlaveMemory) << "\n";
		cout << "---------------------------------------------------------\n";
	}

	return lGlobalSlaveMemory;
}

longint SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeGlobalSlaveScorerNecessaryMemory() const
{
	longint lFullScorerMemory;
	longint lSelectionScorerBufferMemory;

	require(masterSnbPredictor != NULL);
	require(masterSnbPredictor->Check());
	require(masterSnbPredictor->GetTargetValueStats() != NULL);
	require(masterSnbPredictor->GetTargetValueStats()->GetAttributeNumber() > 0);

	// NB : La methode appelle ignore tous les parametres sauf nAttributeNumber lorsque bIncludeDataCostCalculator == false
	lFullScorerMemory = SNBAttributeSelectionScorer::ComputeNecessaryMemory(
	    masterSnbPredictor->GetInstanceNumber(), masterSnbPredictor->ComputeTrainingAttributeNumber(),
	    masterSnbPredictor->GetTargetValueStats()->GetAttributeAt(0)->GetPartNumber(),
	    masterSnbPredictor->GetTargetAttributeType(), masterSnbPredictor->IsTargetGrouped(), true);

	lSelectionScorerBufferMemory =
	    lFullScorerMemory -
	    SNBAttributeSelectionScorer::ComputeNecessaryMemory(
		masterSnbPredictor->GetInstanceNumber(), masterSnbPredictor->ComputeTrainingAttributeNumber(),
		masterSnbPredictor->GetTargetValueStats()->GetAttributeAt(0)->GetPartNumber(),
		masterSnbPredictor->GetTargetAttributeType(), masterSnbPredictor->IsTargetGrouped(), false);

	return lSelectionScorerBufferMemory;
}

longint SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeGlobalSlaveBinarySliceSetChunkBufferNecessaryMemory(
    int nSliceNumber) const
{
	int nInstanceNumber;
	int nAttributeNumber;
	int nSparseAttributeNumber;
	longint lSparseMissingValueNumber;
	double dSparseMemoryFactor;
	longint lNecessaryMemory;
	IntVector* ivTrainingSparseMissingValueNumberPerAttribute;

	// Aliases locales pour la lisibilite
	nInstanceNumber = masterSnbPredictor->GetInstanceNumber();
	nAttributeNumber = masterSnbPredictor->ComputeTrainingAttributeNumber();
	nSparseAttributeNumber = masterSnbPredictor->ComputeTrainingSparseAttributeNumber();
	lSparseMissingValueNumber = masterSnbPredictor->ComputeTrainingAttributesSparseMissingValueNumber();

	dSparseMemoryFactor = masterSnbPredictor->ComputeSparseMemoryFactor();
	ivTrainingSparseMissingValueNumberPerAttribute =
	    masterSnbPredictor->ComputeTrainingSparseMissingValueNumberPerAttribute();

	// La memoire global du buffer et du scorer s'estime
	// avec les estimations des objets necessaires avec un seul chunk
	lNecessaryMemory = SNBDataTableBinarySliceSetChunkBuffer::ComputeNecessaryMemory(
	    nInstanceNumber, 1, ivTrainingSparseMissingValueNumberPerAttribute,
	    nAttributeNumber - nSparseAttributeNumber, nSliceNumber, dSparseMemoryFactor, false);

	// Nettoyage
	delete ivTrainingSparseMissingValueNumberPerAttribute;
	ivTrainingSparseMissingValueNumberPerAttribute = NULL;

	return lNecessaryMemory;
}

longint SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeSlaveNecessaryMemory(int nSlaveProcessNumber,
										 int nSliceNumber)
{
	const boolean bDisplay = false;
	int nInstanceNumber;
	int nAttributeNumber;
	longint lLayoutMemory;
	longint lTargetValuesMemory;
	longint lBinarySliceSetSelfMemory;
	longint lSelectionScorerMemory;
	longint lSlaveMemory;

	// Aliases locales pour la lisibilite
	nInstanceNumber = masterSnbPredictor->GetInstanceNumber();
	nAttributeNumber = masterSnbPredictor->ComputeTrainingAttributeNumber();

	// Memoire de toutes les parties de la SNBDataTableBinarySliceSet sauf le buffer et ses contenus
	lLayoutMemory = SNBDataTableBinarySliceSetLayout::ComputeNecessaryMemory(nInstanceNumber, nSlaveProcessNumber,
										 nAttributeNumber, nSliceNumber);
	lTargetValuesMemory = SNBDataTableBinarySliceSet::ComputeTargetValuesNecessaryMemory(nInstanceNumber);
	lBinarySliceSetSelfMemory = sizeof(SNBDataTableBinarySliceSet) - sizeof(SNBDataTableBinarySliceSetSchema) -
				    sizeof(SNBDataTableBinarySliceSetLayout) -
				    sizeof(SNBDataTableBinarySliceSetRandomizedAttributeIterator) -
				    sizeof(SNBDataTableBinarySliceSetChunkBuffer);
	lSelectionScorerMemory = ComputeSlaveScorerNecessaryMemory();

	// La memoire de l'esclave est celle des
	lSlaveMemory = lLayoutMemory + lTargetValuesMemory + lBinarySliceSetSelfMemory;

	// Trace de deboggage
	if (bDisplay)
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

longint SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeSlaveScorerNecessaryMemory() const
{
	require(masterSnbPredictor != NULL);
	require(masterSnbPredictor->Check());
	require(masterSnbPredictor->GetTargetValueStats() != NULL);
	require(masterSnbPredictor->GetTargetValueStats()->GetAttributeNumber() > 0);

	// NB : La methode appellee ignore tous les parametres sauf nAttributeNumber
	//      lorsque bIncludeDataCostCalculator == false
	return SNBAttributeSelectionScorer::ComputeNecessaryMemory(
	    masterSnbPredictor->GetInstanceNumber(), masterSnbPredictor->ComputeTrainingAttributeNumber(),
	    masterSnbPredictor->GetTargetValueStats()->GetAttributeAt(0)->GetPartNumber(),
	    masterSnbPredictor->GetTargetAttributeType(), masterSnbPredictor->IsTargetGrouped(), false);
}

longint SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeOverallAttributeStatsNecessaryMemory()
{
	longint lOverallDataPreparatrionStatsMemory;
	ObjectDictionary dummyDictionary;
	ObjectArray* oaAllPreparedStats;
	int nAttribute;
	KWDataPreparationStats* dataPreparationStats;

	// Memoire des KWAttributeStats's informatifs
	lOverallDataPreparatrionStatsMemory = 0;
	oaAllPreparedStats = masterSnbPredictor->GetClassStats()->GetAllPreparedStats();
	for (nAttribute = 0; nAttribute < oaAllPreparedStats->GetSize(); nAttribute++)
	{
		dataPreparationStats = cast(KWDataPreparationStats*, oaAllPreparedStats->GetAt(nAttribute));

		if (dataPreparationStats->IsInformative())
			lOverallDataPreparatrionStatsMemory +=
			    dataPreparationStats->GetUsedMemory() + dummyDictionary.GetUsedMemoryPerElement();
	}
	return lOverallDataPreparatrionStatsMemory;
}

longint SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeDataPreparationClassNecessaryMemory()
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

longint
SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeRecodingObjectsNecessaryMemory(longint lSliceSetBufferMemory)
{
	const boolean bDisplay = false;
	const double dCompilationFactor = 1.5;
	const double dPhysicalClassFactor = 2.0;
	longint lDataPreparationClassMemory;
	longint lSliceSetTotalReadBufferMemory;
	longint lRecodingObjectsMemory;

	// Memoire de la KWDataPreparationClass
	lDataPreparationClassMemory = ComputeDataPreparationClassNecessaryMemory();

	// Memoire total pour les buffer du KWDataTableSliceSet en entree
	lSliceSetTotalReadBufferMemory = ComputeSliceSetTotalReadBufferNecessaryMemory(lSliceSetBufferMemory);

	// Total objets de recodage (avec des facteurs de slackness pour la compilation et la classe physique)
	lRecodingObjectsMemory = (longint)(lDataPreparationClassMemory * dCompilationFactor * dPhysicalClassFactor) +
				 lSliceSetTotalReadBufferMemory;

	// Trace de deboggage
	if (bDisplay)
	{
		cout << "Recoding objects memory  (s.set buffer " << LongintToHumanReadableString(lSliceSetBufferMemory)
		     << "):\n";
		cout << "data prep class           mem = " << LongintToHumanReadableString(lDataPreparationClassMemory)
		     << "\n";
		cout << "slice set tot buffer      mem = "
		     << LongintToHumanReadableString(lSliceSetTotalReadBufferMemory) << "\n";
		cout << "compilation factor            = " << dCompilationFactor << "\n";
		cout << "physical class factor         = " << dPhysicalClassFactor << "\n";
		cout << "total recoding objects    mem = " << LongintToHumanReadableString(lRecodingObjectsMemory)
		     << "\n";
		cout << "\n";
	}

	return lRecodingObjectsMemory;
}
longint SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeSliceSetTotalReadBufferNecessaryMemory(
    longint lSliceSetBufferMemory)
{
	// Sur-estimation donnee pour le max de:
	//   - taille du buffer * nombre de slices du KWDataTableSliceSet en entree
	//   - taille par defaut d'un buffer de lecture
	return max((longint)BufferedFile::nDefaultBufferSize,
		   shared_dataTableSliceSet.GetDataTableSliceSet()->GetSliceNumber() * lSliceSetBufferMemory);
}

longint SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeDataPreparationAttributeNecessaryMemory(
    const KWDataGridStats* dataGridStats) const
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

longint SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeMasterNecessaryDisk()
{
	// On sur-estime le disque necessaire pour le fichier du dictionnaire de recodage avec son empreinte en memoire
	return ComputeDataPreparationClassNecessaryMemory();
}

longint SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeGlobalSlaveNecessaryDisk() const
{
	int nInstanceNumber;
	int nAttributeNumber;
	int nSparseAttributeNumber;
	IntVector* ivSparseMissingValueNumberPerAttribute;
	longint lNecessaryDisk;

	// On estime la capacite totale du disque par la taille du SNBDataTableBinarySliceSetChunkBuffer avec
	// un seul esclave et une seule slice (un peu sur-estimee)
	nInstanceNumber = masterSnbPredictor->GetInstanceNumber();
	nAttributeNumber = masterSnbPredictor->ComputeTrainingAttributeNumber();
	nSparseAttributeNumber = masterSnbPredictor->ComputeTrainingSparseAttributeNumber();
	ivSparseMissingValueNumberPerAttribute =
	    masterSnbPredictor->ComputeTrainingSparseMissingValueNumberPerAttribute();
	lNecessaryDisk = SNBDataTableBinarySliceSetChunkBuffer::ComputeNecessaryMemory(
	    nInstanceNumber, 1, ivSparseMissingValueNumberPerAttribute, nAttributeNumber - nSparseAttributeNumber, 1,
	    1.0, true);

	// Nettoyage
	delete ivSparseMissingValueNumberPerAttribute;

	return lNecessaryDisk;
}

longint SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeSlaveNecessaryDisk()
{
	// On sur-estime le disque necessaire pour le fichier dictionnaire de recodage avec son empreinte en memoire
	return ComputeDataPreparationClassNecessaryMemory();
}

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::MasterInitialize()
{
	boolean bOk = true;

	// Activation du mode boost
	// Amelioration faible en pratique, de l'ordre de 10% dans les cas favorables
	SetBoostMode(true);

	// Initialisation du SNBDataTableBinarySliceSet du maitre
	bOk = MasterInitializeDataTableBinarySliceSet();

	// Initialisation du reste des variables
	bOk = bOk and MasterInitializeSharedVariables();
	if (bOk)
		MasterInitializeOptimizationVariables();

	// Estimation de l'unite de progression
	// Le nombre de taches est estime comme ceci :
	//   [#iterations externes] x [# max passes FFWBW] x [1.2 x #attributs] x [#eslaves]
	//                                                       ^^^^^
	//                                             (+1 pour FFW et + .2 pour FBW)
	// L'unite de progres est donc 1/#taches
	// Elle est n'est pas exacte car le nombre de taches FBW depends du nombre de variables embarquees
	// On dans ce cas la 20% qui est l'ordre magnitud de variables selectionnees a la fin
	if (bOk)
	{
		dMasterTaskProgress = 1.0 / nMasterOuterIterationNumber;
		dMasterTaskProgress /= nMaxFastForwardBackwardRuns;
		dMasterTaskProgress /= 1.2 * masterBinarySliceSet->GetAttributeNumber();
		dMasterTaskProgress /= GetTaskResourceGrant()->GetSlaveNumber();
	}

	return bOk;
}

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::MasterInitializeDataTableBinarySliceSet()
{
	const boolean bDisplay = false;
	int nSlaveProcessNumber;
	boolean bOk = false;
	int nSliceNumber;
	int nMaxSliceNumber;
	longint lSlaveGrantedMemory;
	longint lSlaveNecessaryMemory;
	longint lSlaveShareOfGlobalNecessaryMemory;
	longint lSlaveExtraMemory;
	longint lDataTableSliceSetTotalReadBufferMemory;
	longint lOneSliceExecutionExtraNecessaryMemory;
	int nDataTableSliceSetSliceNumber;
	int nDenseAttributeNumber;
	longint lNonBufferSlaveGlobalMemory;
	longint lSlaveDenseValuesMemoryPerBlock;
	longint lSlaveMaxSparseValuesMemoryPerBlock;
	KWClass* recoderClass;
	ALString sTmp;
	longint lGlobalSharedMemoryPerSlave;

	require(masterBinarySliceSet == NULL);

	// Initialisation des variables locaux
	nSlaveProcessNumber = GetTaskResourceGrant()->GetSlaveNumber();
	lSlaveGrantedMemory = GetTaskResourceGrant()->GetSlaveMemory();
	nMaxSliceNumber = ComputeMaxSliceNumber();
	lSlaveNecessaryMemory = 0;
	lSlaveShareOfGlobalNecessaryMemory = 0;
	lSlaveExtraMemory = -1;
	lDataTableSliceSetTotalReadBufferMemory = 0;
	lOneSliceExecutionExtraNecessaryMemory = 0;
	nDataTableSliceSetSliceNumber = 0;
	recoderClass = NULL;

	// Recherche d'un nombre de slices qui permet d'executer la tache
	// avec le minimum pour les buffers du KWDataTableSliceSet
	for (nSliceNumber = 1; nSliceNumber <= nMaxSliceNumber; nSliceNumber++)
	{
		// Calcul de la memoire necessaire pour l'esclave pour ce nombre de slices
		// NB: La memoire globale diminue avec la taille du buffer du slice set d'entree (2eme param).
		//     Donc on utilise BufferedFile::nDefaultBufferSize, qui est la plus grand taille utilisee
		//     lors de l'estimation de resources. Ceci assure que l'on atteint la borne-inf des
		//     ressources demandes.
		lGlobalSharedMemoryPerSlave =
		    ComputeGlobalSlaveNecessaryMemory(nSliceNumber, BufferedFile::nDefaultBufferSize) /
		    nSlaveProcessNumber;
		lSlaveNecessaryMemory =
		    ComputeSlaveNecessaryMemory(nSlaveProcessNumber, nSliceNumber) + lGlobalSharedMemoryPerSlave;

		// Finalisation de la recherche si la memoire attribuee est suffissante pour ce nombre de slices
		if (lSlaveGrantedMemory >= lSlaveNecessaryMemory)
		{
			bOk = true;
			lSlaveExtraMemory = lSlaveGrantedMemory - lSlaveNecessaryMemory;
			break;
		}
		// Si l'on ne peut pas executer avec une seule slice:
		//   Memorisation de la quantite extra de memoire pour pouvoir le faire
		else if (nSliceNumber == 1)
			lOneSliceExecutionExtraNecessaryMemory =
			    (lSlaveNecessaryMemory - lSlaveGrantedMemory) * nSlaveProcessNumber;
	}

	// Si une configuration de memoire a ete trouve :
	// - Warning en mode expert s'il y a eu du slicing
	// - Initialisation du buffer du slice set en entree
	// - Initialisation du binary slice set
	// - Estimation du nombre maximal de valeurs sparse dans un bloc
	if (bOk)
	{
		// Message en mode expert s'il y a eu du slicing
		if (GetLearningExpertMode() and nSliceNumber > 1)
		{
			assert(lOneSliceExecutionExtraNecessaryMemory > 0);
			AddMessage(sTmp + "Train database was sliced. Number of slices: " + IntToString(nSliceNumber) +
				   " (needs extra " +
				   RMResourceManager::ActualMemoryToString(lOneSliceExecutionExtraNecessaryMemory) +
				   ")");
		}

		// Calcul et parametrage de la memoire necessaire pour les buffer de lecture du KWDataTableSliceSet
		//
		// En gros on rajoute a la memoire deja pris en compte (config avec buffer de taille minimale) le minimum entre:
		//   - l'excedant entre la memoire necessaire avec un buffer de taille par defaut et celle avec un de taille minimale
		//   - l'excedant entre la memoire attribuee et la memoire necessaire avec un buffer de taille minimale
		//
		nDataTableSliceSetSliceNumber = shared_dataTableSliceSet.GetDataTableSliceSet()->GetSliceNumber();
		lDataTableSliceSetTotalReadBufferMemory =
		    ComputeSliceSetTotalReadBufferNecessaryMemory(MemSegmentByteSize) +
		    min(lSlaveExtraMemory,
			ComputeSliceSetTotalReadBufferNecessaryMemory(BufferedFile::nDefaultBufferSize) -
			    ComputeSliceSetTotalReadBufferNecessaryMemory(MemSegmentByteSize));
		shared_dataTableSliceSet.GetDataTableSliceSet()->SetTotalBufferSize(
		    lDataTableSliceSetTotalReadBufferMemory);

		// Calcul du nombre maximal de valeurs sparse a retenir en memoire
		// Calcule seulement quand il y a des variables sparse
		if (masterSnbPredictor->ComputeTrainingSparseAttributeNumber() > 0)
		{
			// Estimation du nombre de valeurs denses dans un bloc
			nDenseAttributeNumber = masterSnbPredictor->ComputeTrainingAttributeNumber() -
						masterSnbPredictor->ComputeTrainingSparseAttributeNumber();
			lSlaveDenseValuesMemoryPerBlock = longint(nDenseAttributeNumber) *
							  masterSnbPredictor->GetInstanceNumber() * sizeof(int) /
							  longint(nSlaveProcessNumber * nSliceNumber);

			// Estimation de la memoire max des valeurs sparse dans un bloc
			//
			// [grant esclave]
			//   - [estimation memoire esclave]
			//   - [estimation memoire global esclave hors buffer]
			//   - [memoire des attributs denses du bloc]
			//
			lNonBufferSlaveGlobalMemory =
			    max(0ll,
				ComputeGlobalSlaveScorerNecessaryMemory() -
				    ComputeRecodingObjectsNecessaryMemory(lDataTableSliceSetTotalReadBufferMemory)) /
			    nSlaveProcessNumber;
			lSlaveMaxSparseValuesMemoryPerBlock =
			    GetTaskResourceGrant()->GetSlaveMemory() -
			    ComputeSlaveNecessaryMemory(nSlaveProcessNumber, nSliceNumber) -
			    lNonBufferSlaveGlobalMemory - lSlaveDenseValuesMemoryPerBlock;

			// Calcul du nombre des valeurs sparse max dans un bloc, with a slack factor
			shared_lMaxSparseValuesPerBlock = lSlaveMaxSparseValuesMemoryPerBlock / sizeof(int);
		}
		else
			shared_lMaxSparseValuesPerBlock = 0;

		// Initialisation du SNBDataTableBinarySliceSet du maitre
		masterBinarySliceSet = new SNBDataTableBinarySliceSet;
		masterBinarySliceSet->Initialize(
		    masterSnbPredictor->GetClassStats(), nSlaveProcessNumber,
		    masterSnbPredictor->GetTrainParameters()->GetMaxEvaluatedAttributeNumber(), nSliceNumber);

		// Changement le domaine de travail a celui de la classe de recodage
		recoderClass = masterBinarySliceSet->GetDataPreparationClass()->GetDataPreparationClass();
		masterInitialDomain = KWClassDomain::GetCurrentDomain();
		KWClassDomain::SetCurrentDomain(recoderClass->GetDomain());
		KWClassDomain::GetCurrentDomain()->Compile();
	}
	// Message d'erreur si pas assez de memoire, meme avec le nombre maximal de slices
	else
		AddError("Not enough memory to run the task" +
			 RMResourceManager::BuildMissingMemoryMessage(lSlaveNecessaryMemory - lSlaveGrantedMemory));

	// Trace de deboggage
	if (bDisplay)
	{
		cout << "-------------------------------------------------------\n";
		cout << "status                      = " << (bOk ? "OK" : "KO") << "\n";
		cout << "granted slave proc          = " << GetTaskResourceGrant()->GetSlaveNumber() << "\n";
		cout << "sparse attr. number         = " << masterSnbPredictor->ComputeTrainingSparseAttributeNumber()
		     << "\n";
		cout << "slice set tot buffer    mem = "
		     << LongintToHumanReadableString(lDataTableSliceSetTotalReadBufferMemory) << " = "
		     << lDataTableSliceSetTotalReadBufferMemory << " bytes\n";
		cout << "granted master          mem = "
		     << LongintToHumanReadableString(GetTaskResourceGrant()->GetMasterMemory()) << " = "
		     << GetTaskResourceGrant()->GetMasterMemory() << " bytes\n";
		cout << "granted shared          mem = "
		     << LongintToHumanReadableString(GetTaskResourceGrant()->GetSharedMemory()) << " = "
		     << GetTaskResourceGrant()->GetSharedMemory() << " bytes\n";
		cout << "granted slave           mem = "
		     << LongintToHumanReadableString(GetTaskResourceGrant()->GetSlaveMemory()) << " = "
		     << GetTaskResourceGrant()->GetSlaveMemory() << " bytes\n";
		cout << "extra to run w/1 slice  mem = "
		     << LongintToHumanReadableString(lOneSliceExecutionExtraNecessaryMemory) << " = "
		     << lOneSliceExecutionExtraNecessaryMemory << " bytes\n";
		cout << "sparse block    max  values = " << shared_lMaxSparseValuesPerBlock << "\n";
		cout << "sparse block    max     mem = "
		     << LongintToHumanReadableString(shared_lMaxSparseValuesPerBlock * sizeof(int)) << " = "
		     << shared_lMaxSparseValuesPerBlock * 4 << " bytes\n";
		cout << "sparse block max     values = " << shared_lMaxSparseValuesPerBlock << "\n";
		if (bOk)
		{
			cout << "reco dict real          mem = "
			     << LongintToHumanReadableString(recoderClass->GetUsedMemory()) << " = "
			     << recoderClass->GetUsedMemory() << " bytes\n";
			cout << "-------\n";
			cout << masterBinarySliceSet->layout << "\n";
		}
		cout << "-------------------------------------------------------\n";
		cout << endl;
	}

	ensure(shared_lMaxSparseValuesPerBlock >= 0ll);
	ensure(not bOk or IsMasterDataTableBinarySliceSetInitialized());
	ensure(not bOk or masterBinarySliceSet->Check());

	return bOk;
}

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::IsMasterDataTableBinarySliceSetInitialized() const
{
	boolean bOk = true;

	bOk = bOk and masterBinarySliceSet != NULL;
	bOk = bOk and masterBinarySliceSet->IsInitialized();
	bOk = bOk and masterBinarySliceSet->dataPreparationClass != NULL;
	return bOk;
}

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::MasterInitializeSharedVariables()
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

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::MasterInitializeRecoderClassSharedVariables()
{
	boolean bOk = true;
	KWClass* recoderClass;
	ALString sRecoderClassTmpFilePath;
	ALString sAlphaNumClassName;
	int i;
	char c;

	require(IsMasterProcess());
	require(masterBinarySliceSet != NULL);
	require(masterBinarySliceSet->Check());

	// En parallele : On enregistre le domaine de la classe de recodage dans un fichier et on met son URI en shared
	recoderClass = masterBinarySliceSet->GetDataPreparationClass()->GetDataPreparationClass();
	assert(recoderClass->IsCompiled());
	shared_sRecoderClassName.SetValue(recoderClass->GetName());
	if (IsParallel())
	{
		// Le nom du dictionnaire de recodage ne doit contenir que des alphanumeriques. Sinon on remplace les char par '_'
		for (i = 0; i < shared_sRecoderClassName.GetValue().GetLength(); i++)
		{
			c = shared_sRecoderClassName.GetValue().GetAt(i);
			if (isalnum(c))
				sAlphaNumClassName += c;
			else
				sAlphaNumClassName += '_';
		}
		sRecoderClassTmpFilePath = FileService::CreateUniqueTmpFile(sAlphaNumClassName + ".kdic", this);
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

void SNBPredictorSelectiveNaiveBayesTrainingTask::MasterInitializeOptimizationVariables()
{
	require(masterBinarySliceSet != NULL);

	// Memorisation et fixation de l'etat du RNG
	nMasterRandomSeed = GetRandomSeed();
	SetRandomSeed(1);

	// Variables de l'etat des iterations
	bMasterUndoLastModification = false;
	nMasterOuterIteration = 0;
	nMasterFastForwardBackwardRun = 0;
	nMasterRandomAttribute = 0;
	masterRandomAttribute = masterBinarySliceSet->GetRandomAttributeAt(0);
	nMasterFastRunStepFinishedTaskNumber = 0;
	bMasterInitializeSlaveScorers = false;
	nMasterTaskState = TaskState::PrecisionEpsilonComputation;

	nMasterOuterIterationNumber = int(ceil(log2(masterBinarySliceSet->GetInstanceNumber() + 1)));

	// Scores et couts
	dMasterModificationScore = 0.0;
	dMasterModificationModelCost = 0.0;
	dMasterModificationDataCost = 0.0;
	dMasterCurrentScore = DBL_MAX;
	dMasterCurrentModelCost = 0.0;
	dMasterCurrentDataCost = 0.0;
	dMasterEmptySelectionScore = 0.0;
	dMasterEmptySelectionModelCost = 0.0;
	dMasterEmptySelectionDataCost = 0.0;
	dMasterLastFFBWRunScore = DBL_MAX;
	dMasterMapScore = 0.0;

	// L'epsilon de precision a besoin d'une passe pour la base de donnees et se calcule au debut de la tache
	dMasterPrecisionEpsilon = 0.0;

	// Delta poids de la modification courante
	dMasterModificationDeltaWeight = 1.0;

	// Calculatrice du score du maitre (calculatrice de couts de donnes non initialise)
	masterWeightedSelectionScorer = new SNBAttributeSelectionScorer;
	masterWeightedSelectionScorer->SetLearningSpec(shared_learningSpec.GetLearningSpec());
	masterWeightedSelectionScorer->SetDataTableBinarySliceSet(masterBinarySliceSet);
	masterWeightedSelectionScorer->SetPriorWeight(masterSnbPredictor->GetSelectionParameters()->GetPriorWeight());
	masterWeightedSelectionScorer->SetConstructionCostEnabled(
	    masterSnbPredictor->GetSelectionParameters()->GetConstructionCost());
	masterWeightedSelectionScorer->SetPreparationCostEnabled(
	    masterSnbPredictor->GetSelectionParameters()->GetPreparationCost());
	masterWeightedSelectionScorer->SetPriorExponent(
	    masterSnbPredictor->GetSelectionParameters()->GetPriorExponent());

	ensure(CheckCurrentAttribute());
}

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::MasterPrepareTaskInput(double& dTaskPercent,
									    boolean& bIsTaskFinished)
{
	require(0 <= nMasterRandomAttribute and nMasterRandomAttribute < masterBinarySliceSet->GetAttributeNumber());
	require(masterRandomAttribute != NULL);

	if (not IsOuterIterationFinished())
	{
		input_nTaskState = nMasterTaskState;

		// Premiere passe : Calcul du score de une selection vide pour estimer l'epsilon de precision
		if (nMasterTaskState == TaskState::PrecisionEpsilonComputation)
		{
			input_bUndoLastModification = false;
			input_nModificationAttribute = -1;
			input_bInitializeWorkingData = false;
		}
		// Passe normal : Calcul du score d'une modification de la selection courant
		else
		{
			assert(nMasterTaskState == TaskState::FastForwardRun or
			       nMasterTaskState == TaskState::FastBackwardRun);
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

	if (not IsOuterIterationFinished() and nMasterTaskState != TaskState::PrecisionEpsilonComputation)
		input_dModificationDeltaWeight = dMasterModificationDeltaWeight;

	return true;
}

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::MasterAggregateResults()
{
	// Mise a jour du compte de taches
	nMasterFastRunStepFinishedTaskNumber++;

	if (nMasterTaskState == TaskState::PrecisionEpsilonComputation)
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

			// Passage a l'etat FFW
			nMasterTaskState = TaskState::FastForwardRun;
		}
	}
	else
	{
		assert(nMasterTaskState == TaskState::FastForwardRun or nMasterTaskState == TaskState::FastBackwardRun);

		// Mise a jour du cout de donnes courant avec celui issu de l'esclave
		dMasterModificationDataCost += output_dDataCost;

		// Fin de toutes les tache d'un pas de la passe
		if (AllFastRunStepTasksAreFinished())
		{
			// Mise a jour de la selection s'il y a une amelioration
			UpdateSelection();

			// Mise a jour de l'attribut de l'iteration
			UpdateCurrentAttribute();

			// Si on est a la fin de la passe rapide on commence une autre
			// (potentiellement une nouvelle iteration externe)
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

	ensure(nMasterTaskState == TaskState::PrecisionEpsilonComputation or dMasterPrecisionEpsilon > 0);
	ensure(0 <= nMasterFastRunStepFinishedTaskNumber and nMasterFastRunStepFinishedTaskNumber < GetProcessNumber());
	ensure(CheckCurrentAttribute());
	return true;
}

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::IsOuterIterationFinished() const
{
	require(IsRunning());
	require(IsMasterProcess());
	return nMasterOuterIteration == nMasterOuterIterationNumber;
}

void SNBPredictorSelectiveNaiveBayesTrainingTask::UpdateTaskProgressionLabel() const
{
	ALString sLabel;

	require(IsRunning());
	require(IsMasterProcess());

	if (nMasterTaskState == TaskState::FastForwardRun)
		sLabel = "Increasing variable weights by ";
	else
	{
		assert(nMasterTaskState == TaskState::FastBackwardRun);
		sLabel = "Decreasing variable weights by ";
	}
	sLabel += DoubleToString(dMasterModificationDeltaWeight);
	TaskProgression::DisplayLabel(sLabel);
}

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::AllFastRunStepTasksAreFinished() const
{
	require(IsRunning());
	require(IsMasterProcess());
	return nMasterFastRunStepFinishedTaskNumber == GetProcessNumber();
}

void SNBPredictorSelectiveNaiveBayesTrainingTask::UpdateSelection()
{
	const boolean bDisplay = false;

	require(IsMasterProcess());

	// Passe FastForward
	if (nMasterTaskState == TaskState::FastForwardRun)
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
		assert(nMasterTaskState == TaskState::FastBackwardRun);

		// Mise a jour du score de la modification
		masterWeightedSelectionScorer->DecreaseAttributeWeight(masterRandomAttribute,
								       dMasterModificationDeltaWeight);
		dMasterModificationModelCost = masterWeightedSelectionScorer->ComputeSelectionModelCost();
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
	if (bDisplay)
	{
		cout << masterRandomAttribute->GetNativeAttributeName() << "\t"
		     << (nMasterTaskState == TaskState::FastForwardRun ? "increase" : "decrease") << "\t"
		     << dMasterModificationDeltaWeight << "\t" << dMasterModificationModelCost << "\t"
		     << dMasterModificationDataCost << "\t" << (bMasterUndoLastModification ? "rejected" : "accepted")
		     << "\t" << dMasterLastFFBWRunScore;
		if (not bMasterUndoLastModification)
			cout << "\t" << dMasterCurrentScore;
		cout << "\n";
	}
}

void SNBPredictorSelectiveNaiveBayesTrainingTask::UpdateCurrentAttribute()
{
	require(IsMasterProcess());
	require(not IsFastRunFinished());

	// Passe FastForward
	if (nMasterTaskState == TaskState::FastForwardRun)
	{
		if (masterRandomAttribute != NULL)
			nMasterRandomAttribute++;

		if (nMasterRandomAttribute < masterBinarySliceSet->GetAttributeNumber())
			masterRandomAttribute = masterBinarySliceSet->GetRandomAttributeAt(nMasterRandomAttribute);
	}
	// Passe FastBackward : Le prochain attribut aleatoire *doit* etre selectionne
	else
	{
		assert(nMasterTaskState == TaskState::FastBackwardRun);
		if (masterRandomAttribute != NULL)
			nMasterRandomAttribute--;
		while (nMasterRandomAttribute >= 0)
		{
			masterRandomAttribute = masterBinarySliceSet->GetRandomAttributeAt(nMasterRandomAttribute);
			if (masterWeightedSelectionScorer->GetAttributeSelection()->Contains(masterRandomAttribute))
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

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::IsFastRunFinished() const
{
	boolean bIsRunFinished;

	require(IsRunning());
	require(IsMasterProcess());

	if (nMasterTaskState == TaskState::FastForwardRun)
		bIsRunFinished = nMasterRandomAttribute == masterBinarySliceSet->GetAttributeNumber();
	else
		bIsRunFinished = nMasterRandomAttribute == -1;

	return bIsRunFinished;
}

void SNBPredictorSelectiveNaiveBayesTrainingTask::InitializeNextFastRun()
{
	require(IsMasterProcess());
	require(nMasterTaskState == TaskState::FastForwardRun or nMasterTaskState == TaskState::FastBackwardRun);

	// Fin d'une passe FastForward : On initialise une passe FastBackward
	if (nMasterTaskState == TaskState::FastForwardRun)
	{
		// Si la selection est vide : On saute a la prochaine passe FastForward
		if (masterWeightedSelectionScorer->GetAttributeSelection()->GetAttributeNumber() == 0)
			InitializeNextFastForwardRun();
		// Sinon on initialise une passe FastBackward
		else
		{
			nMasterTaskState = TaskState::FastBackwardRun;
			nMasterRandomAttribute = masterBinarySliceSet->GetAttributeNumber() - 1;
		}
	}
	// Fin d'une passe FastBackward : On initialise un passe FastForward et potentiellement une
	// nouvelle iteration externe
	else
		InitializeNextFastForwardRun();

	// Mise-a-jour de l'attribut courant
	UpdateCurrentAttribute();

	// Mise a jour de la progression
	UpdateTaskProgressionLabel();
}

void SNBPredictorSelectiveNaiveBayesTrainingTask::ComputeEmptySelectionScoreAndPrecisionEpsilon()
{
	const boolean bDisplay = false;

	require(nMasterTaskState == TaskState::PrecisionEpsilonComputation);
	require(dMasterPrecisionEpsilon == 0.0);

	// Mise a jour du score de la selection vide
	dMasterEmptySelectionModelCost = masterWeightedSelectionScorer->ComputeSelectionModelCost();
	dMasterEmptySelectionScore = dMasterEmptySelectionModelCost + dMasterEmptySelectionDataCost;

	// Calcul du epsilon de precision permettant de se comparer de facon relative a ce cout par defaut
	// La comparaison en "absolu" pose des problemes numeriques
	dMasterPrecisionEpsilon = 1e-2 * (1 + fabs(dMasterEmptySelectionScore));
	dMasterPrecisionEpsilon /= (1 + shared_learningSpec.GetLearningSpec()->GetInstanceNumber());

	// Initialisation du reste de couts et scores
	dMasterCurrentScore = dMasterEmptySelectionScore;
	dMasterCurrentModelCost = dMasterEmptySelectionModelCost;
	dMasterCurrentDataCost = dMasterEmptySelectionDataCost;
	dMasterLastFFBWRunScore = dMasterEmptySelectionScore;
	dMasterMapScore = dMasterEmptySelectionScore;

	// Trace de debbogage
	if (bDisplay)
	{
		cout << "Empty selection model cost = " << dMasterEmptySelectionModelCost << "\n";
		cout << "Empty selection data cost  = " << dMasterEmptySelectionDataCost << "\n";
		cout << "Precision Epsilon          = " << dMasterPrecisionEpsilon << "\n";
		cout << "----------------------------------------------\n";
		// Entete pour la trace de la methode UpdateSelection
		cout << "Variable\tModif\tWeight\tModelCost\tDataCost\tDecision\tLastCost\tNewCost\n";
	}

	ensure(dMasterPrecisionEpsilon > 0.0);
}

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::CheckCurrentAttribute() const
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
		bOk = (nMasterTaskState == TaskState::FastForwardRun and
		       nMasterRandomAttribute == masterBinarySliceSet->GetAttributeNumber()) or
		      (nMasterTaskState != TaskState::FastForwardRun and nMasterRandomAttribute == -1);
	}

	return bOk;
}

void SNBPredictorSelectiveNaiveBayesTrainingTask::InitializeNextFastForwardRun()
{
	// Refaite de la meme passe FFWBW ssi le score est ameliore par rapport a la passe FFBW d'avant
	nMasterRandomAttribute = 0;
	if (dMasterCurrentScore < dMasterLastFFBWRunScore - dMasterPrecisionEpsilon)
		nMasterFastForwardBackwardRun++;
	// Sinon on passe a la suivante passe FFWBW (diminution de poids)
	else
		nMasterFastForwardBackwardRun = nMaxFastForwardBackwardRuns;
	dMasterLastFFBWRunScore = dMasterCurrentScore;

	// Fin de l'iteration externe
	if (nMasterFastForwardBackwardRun == nMaxFastForwardBackwardRuns)
	{
		// Reinitialisation de l'iteration externe et mise a jour du poids marginal
		nMasterOuterIteration++;
		nMasterFastForwardBackwardRun = 0;
		dMasterModificationDeltaWeight /= 2.0;
	}

	// Changement a l'etat de la tache a FFW
	nMasterTaskState = TaskState::FastForwardRun;
}

void SNBPredictorSelectiveNaiveBayesTrainingTask::MasterFinalizeTrainingAndReports()
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
			if (attribute->GetPreparedDataGridStats()->GetAttributeNumber() == 3)
			{
				attributeReport->SetPair(true);
				attributeReport->SetNativeAttributeName1(
				    attribute->GetPreparedDataGridStats()->GetAttributeAt(0)->GetAttributeName());
				attributeReport->SetNativeAttributeName2(
				    attribute->GetPreparedDataGridStats()->GetAttributeAt(1)->GetAttributeName());
			}
			attributeReport->SetUnivariateEvaluation(attribute->GetLevel());
			attributeReport->SetWeight(
			    masterWeightedSelectionScorer->GetAttributeSelection()->GetAttributeWeightAt(attribute));
			assert(selectionReport->Check());
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

	// Tri des SNBDataTableBinarySliceSetAttribute's par index de la classe de preparation
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

	// Creation des regles du prediction dans la classe de preparation
	masterSnbPredictor->CreatePredictorAttributesInClass(masterBinarySliceSet->GetDataPreparationClass(),
							     &oaSelectedDataPreparationAttributes, &cvAttributeWeights);
}

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
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

	// Nettoyage des variables de travail du maitre
	if (masterBinarySliceSet != NULL)
	{
		if (bProcessEndedCorrectly)
			masterBinarySliceSet->CleanWorkingData(false);
		delete masterBinarySliceSet;
		masterBinarySliceSet = NULL;
	}
	if (masterWeightedSelectionScorer != NULL)
	{
		delete masterWeightedSelectionScorer;
		masterWeightedSelectionScorer = NULL;
	}

	// En parallele : Nettoyage du fichier dictionnaire auxilier
	if (IsParallel())
		FileService::RemoveFile(FileService::GetURIFilePathName(shared_sRecoderClassDomainFileURI.GetValue()));

	ensure(shared_learningSpec.GetLearningSpec()->Check());
	ensure(masterSnbPredictor->GetClassStats() != NULL);
	ensure(masterSnbPredictor->GetClassStats()->Check());
	ensure(masterSnbPredictor->GetClass() != NULL);
	ensure(masterSnbPredictor->GetClass()->Check());
	ensure(masterSnbPredictor->GetDatabase() != NULL);
	ensure(masterSnbPredictor->GetDatabase()->Check());
	ensure(masterWeightedSelectionScorer == NULL);
	ensure(masterBinarySliceSet == NULL);

	return true;
}

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::SlaveInitialize()
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

	// Initialisation de scorer
	if (bOk)
	{
		slaveWeightedSelectionScorer = new SNBAttributeSelectionScorer;
		slaveWeightedSelectionScorer->SetLearningSpec(shared_learningSpec.GetLearningSpec());
		slaveWeightedSelectionScorer->SetDataTableBinarySliceSet(slaveBinarySliceSet);
		slaveWeightedSelectionScorer->SetPriorWeight(shared_dPriorWeight);
		slaveWeightedSelectionScorer->SetPriorExponent(shared_dPriorExponent);
		slaveWeightedSelectionScorer->SetConstructionCostEnabled(shared_bIsConstructionCostEnabled);
		slaveWeightedSelectionScorer->SetPreparationCostEnabled(shared_bIsPreparationCostEnabled);
		bOk = bOk and slaveWeightedSelectionScorer->CreateDataCostCalculator();
	}
	if (bOk)
		slaveWeightedSelectionScorer->InitializeWorkingData();

	ensure(not bOk or
	       (slaveWeightedSelectionScorer->IsDataCostCalculatorCreated() and slaveWeightedSelectionScorer->Check()));
	ensure(0 <= nSlaveProcessChunkIndex and nSlaveProcessChunkIndex < shared_nChunkNumber);
	return bOk;
}

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::SlaveInitializeLearningSpec()
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

	// En parallele : Transfert du fichier de dict. du domaine de la classe de recodage, parsing et compilation
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

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::SlaveInitializeDataTableBinarySliceSet()
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

	// Reconstitution du nombre d'attributs initial et des valeurs de la cible
	slaveBinarySliceSet->nInitialAttributeNumber = shared_nInitialAttributeNumber;
	slaveBinarySliceSet->ivTargetValueIndexes.CopyFrom(shared_ivTargetValueIndexes.GetConstIntVector());

	// Reconstitution du layout
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
	//
	// Reconstitution du SNBDataTableBinarySliceSetSchema a parter des attributs reconstruits
	slaveBinarySliceSet->schema.InitializeFromAttributes(oaAttributes);

	// Initialisation du buffer de lecture de donnees pour le chunk l'esclave
	slaveBinarySliceSet->chunkBuffer.SetLayout(&slaveBinarySliceSet->layout);
	bOk = slaveBinarySliceSet->chunkBuffer.Initialize(
	    GetSlaveChunkIndex(), slaveRecoderClass, shared_dataTableSliceSet.GetDataTableSliceSet(),
	    &slaveBinarySliceSet->schema, shared_lMaxSparseValuesPerBlock);

	// En parallele on decharge la classe de recodage
	if (IsParallel())
		SlaveInitializeUnloadRecoderClass();

	ensure(not bOk or IsSlaveDataTableBinarySliceSetInitialized());
	ensure(not bOk or slaveBinarySliceSet->Check());
	return bOk;
}

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::IsSlaveDataTableBinarySliceSetInitialized() const
{
	return slaveBinarySliceSet != NULL and slaveBinarySliceSet->IsReadyToReadChunk();
}

void SNBPredictorSelectiveNaiveBayesTrainingTask::SlaveInitializeUnloadRecoderClass()
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

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::SlaveProcess()
{
	boolean bOk = true;

	require(IsSlaveDataTableBinarySliceSetInitialized());
	require(slaveBinarySliceSet->Check());

	// (Re)initialisation du scorer si demande
	if (bOk and input_bInitializeWorkingData)
		slaveWeightedSelectionScorer->InitializeWorkingData();

	// Annulation de la derniere modification si demande
	if (bOk and input_bUndoLastModification and not input_bInitializeWorkingData)
		bOk = bOk and slaveWeightedSelectionScorer->UndoLastModification();

	// Calcul du score pour les differents etats de la tache
	if (bOk)
	{
		// Calcul du epsilon : Calcul du cout
		if (input_nTaskState == TaskState::PrecisionEpsilonComputation)
		{
			assert(input_dModificationDeltaWeight == 0.0);
			assert(slaveWeightedSelectionScorer->GetAttributeSelection()->GetAttributeNumber() == 0);
			output_dDataCost = slaveWeightedSelectionScorer->GetSelectionDataCost();
		}
		// Passe FastForward : Increment du poids et calcul du cout
		else if (input_nTaskState == TaskState::FastForwardRun)
		{
			assert(input_nModificationAttribute >= 0);
			assert(input_dModificationDeltaWeight != 0.0);
			bOk = bOk and slaveWeightedSelectionScorer->IncreaseAttributeWeight(
					  slaveBinarySliceSet->GetAttributeAt(input_nModificationAttribute),
					  input_dModificationDeltaWeight);
			output_dDataCost = slaveWeightedSelectionScorer->GetSelectionDataCost();
		}
		// Passe FastBackward : Decrement du poids et calcul du cout
		else
		{
			assert(input_nTaskState == TaskState::FastBackwardRun);
			assert(input_nModificationAttribute >= 0);
			assert(input_dModificationDeltaWeight != 0.0);
			bOk = bOk and slaveWeightedSelectionScorer->DecreaseAttributeWeight(
					  slaveBinarySliceSet->GetAttributeAt(input_nModificationAttribute),
					  input_dModificationDeltaWeight);
			output_dDataCost = slaveWeightedSelectionScorer->GetSelectionDataCost();
		}
	}

	return bOk;
}

boolean SNBPredictorSelectiveNaiveBayesTrainingTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	require(not bProcessEndedCorrectly or IsSlaveDataTableBinarySliceSetInitialized());
	require(not bProcessEndedCorrectly or (slaveBinarySliceSet == NULL or slaveBinarySliceSet->Check()));
	require(not bProcessEndedCorrectly or (slaveRecoderClass != NULL or IsParallel()));
	require(not bProcessEndedCorrectly or (slaveWeightedSelectionScorer != NULL));

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

	// Nettoyage du scorer de l'esclave, qui peut etre nul s'il y a eu un probleme d'initialisation
	if (slaveWeightedSelectionScorer != NULL)
	{
		delete slaveWeightedSelectionScorer;
		slaveWeightedSelectionScorer = NULL;
	}

	ensure(slaveBinarySliceSet == NULL);
	ensure(slaveRecoderClass == NULL);
	ensure(slaveWeightedSelectionScorer == NULL);
	ensure(slaveDummyDatabase == NULL);

	return true;
}

int SNBPredictorSelectiveNaiveBayesTrainingTask::GetSlaveChunkIndex() const
{
	require(IsRunning());
	require(IsSlaveProcess());
	return nSlaveProcessChunkIndex;
}
