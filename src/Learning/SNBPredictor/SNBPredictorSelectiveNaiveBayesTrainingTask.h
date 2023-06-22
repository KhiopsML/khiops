// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class SNBPredictorSNBTrainingTask;
class SNBPredictorSNBEnsembleTrainingTask;
class SNBPredictorSNBDirectTrainingTask;

#include "PLParallelTask.h"
#include "SNBPredictorSelectiveNaiveBayes.h"
#include "SNBDataTableBinarySliceSet.h"
#include "SNBAttributeSelectionScorer.h"
#include "SNBAttributeSelectionWeightCalculator.h"

class SNBPredictorSNBTrainingTask : public PLParallelTask
{
public:
	// Constructeur
	SNBPredictorSNBTrainingTask();
	~SNBPredictorSNBTrainingTask();

	// Point d'entree de la tache
	void InternalTrain(SNBPredictorSelectiveNaiveBayes* snbPredictor);

	// True si l'entrainement a finie
	boolean IsTrainingSuccessful() const;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	///  Implementation
protected:
	// Reimplementation des methodes privees de PLParallelTask
	boolean ComputeResourceRequirements() override;
	boolean MasterInitialize() override;
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;
	boolean MasterAggregateResults() override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveInitialize() override;
	boolean SlaveProcess() override;
	boolean SlaveFinalize(boolean bProcessEndedCorrectly) override;

	////////////////////////////////////////////////////////////
	// Implementation du ComputeResourceRequirements

	// Estimation du nombre optimal de processeurs
	int ComputeMaxSlaveProcessNumber(int nAbsoluteMaxSlaveProcesses) const;

	// Nombre maximal des slices (utilise aussi dans MasterInitialize)
	int ComputeMaxSliceNumber() const;

	// Estimation de la memoire partagee
	longint ComputeSharedNecessaryMemory();

	// Estimation de la memoire du maitre
	virtual longint ComputeMasterNecessaryMemory() const = 0;

	// Estimation de la memoire necessaire pour le recodage
	longint ComputeDataPreparationClassNecessaryMemory();

	// Estimation de l'empreinte memoire des objets pour le recodage du slice set
	longint ComputeRecodingObjectsNecessaryMemory();

	// Estimation de la memoire utilisee par les KWAttributeStats
	longint ComputeOverallAttributeStatsNecessaryMemory();

	// Estimation de la taille d'un attribut prepare
	longint ComputeDataPreparationAttributeNecessaryMemory(const KWDataGridStats* dataGridStats) const;

	// Estimation de la memoire globale necessaire pour tous les esclaves
	virtual longint ComputeGlobalSlaveNecessaryMemory(int nSliceNumber);

	// Estimation de la memoire globale des scores de tous les esclaves
	virtual longint ComputeGlobalSlaveScorerNecessaryMemory() const = 0;

	// Estimation heuristique de la memoire de l'esclave pour l'apprentissage
	virtual longint ComputeSlaveNecessaryMemory(int nProcessNumber, int nSliceNumber);

	// Estimation heuristique de la memoire de l'esclave pour le scorer des selections
	virtual longint ComputeSlaveScorerNecessaryMemory() const = 0;

	// Estimatio heuristique du disque du maitre
	longint ComputeMasterNecessaryDisk();

	// Estimation heuristique du disque global des esclaves pour l'apprentissage
	longint ComputeGlobalSlaveNecessaryDisk() const;

	// Estimation heuristique du disque global partagee
	longint ComputeSlaveNecessaryDisk();

	///////////////////////////////////////////////////////////////////
	// Implementation du MasterInitialize

	// Initialisation de la database d'apprentissage du maitre
	boolean MasterInitializeDataTableBinarySliceSet();

	// True si la database d'apprentissage du maitre est initialise
	boolean IsMasterDataTableBinarySliceSetInitialized() const;

	// Initialisation des variables partagees
	boolean MasterInitializeSharedVariables();

	// Initialisation des variables partagees de la classe de recodage
	boolean MasterInitializeRecoderClassSharedVariables();

	// Initialisation des variables de control de l'optimisation
	virtual void MasterInitializeOptimizationVariables();

	//////////////////////////////////////////////////////////////////
	// Implementation du SlaveInitialize

	// Initialisation du LearningSpec des esclaves
	boolean SlaveInitializeLearningSpec();

	// Initialisation de la database d'apprentissage des esclaves
	boolean SlaveInitializeDataTableBinarySliceSet();

	// True si la database d'apprentissage du maitre est initialise
	boolean IsSlaveDataTableBinarySliceSetInitialized() const;

	// Decharge la classe de recodage et la remplace par une classe bidon
	// Ceci s'utilise en parallele pour economiser de la memoire
	void SlaveInitializeUnloadRecoderClass();

	/////////////////////////////////////////////////////////////////////////
	// Implementation du MasterPrepareTaskInput & MasterAggregateResults

	// True si l'iteration externe est finie
	boolean IsOuterIterationFinished() const;

	// True si l'epsilon de precision est deja calcule
	boolean IsPrecisionEpsilonCalculated() const;

	// True si l'on est dans une iteration FastForward
	boolean IsOnFastForwardRun() const;

	// Mise a jour de la progression de la tache
	virtual void UpdateTaskProgressionLabel() const = 0;

	// True si toutes les taches d'une passe sont finies
	boolean AllFastRunStepTasksAreFinished() const;

	// Mise a jour de la selection apres l'evaluation d'une modification
	virtual void UpdateSelection() = 0;

	// Mise a jour de l'attribut courant de l'iteration
	virtual void UpdateCurrentAttribute() = 0;

	// True si la passe rapide est finie
	boolean IsFastRunFinished() const;

	// Initialise une nouvelle passe rapide : forward -> backward ou backward -> forward
	void InitializeNextFastRun();

	// Calcule le score de la selection vide et l'epsilon de precision
	virtual void ComputeEmptySelectionScoreAndPrecisionEpsilon();

	// True si l'attribut courant est valide
	boolean CheckCurrentAttribute() const;

	// True si la selection courant contient l'attribut specifie
	virtual boolean SelectionContainsAttribute(SNBDataTableBinarySliceSetAttribute* attribute) const = 0;

	// True si la selection est vide
	virtual boolean IsSelectionEmpty() const = 0;

	// Initialise une nouvelle passe rapide forward (potentiellement une nouvelle iteration externe)
	virtual void InitializeNextFastForwardRun() = 0;

	// Calcule le cout du modele courant
	virtual double ComputeSelectionModelCost() = 0;

	///////////////////////////////////////////////
	// Implementation du MasterFinalize

	// Finalisation de l'entrainement et enregistrement
	virtual void MasterFinalizeTrainingAndReports() = 0;

	////////////////////////////////////////////////
	// Methodes des esclaves

	// Index du Chunk de chaque esclave
	int GetSlaveChunkIndex() const;

	///////////////////////////////////
	// Parametres du maitre

	// Predicteur SNB appelant
	SNBPredictorSelectiveNaiveBayes* masterSnbPredictor;

	// Classe du KWLearningSpec initial du predicteur SNB appelant
	KWClass* masterInitialClass;

	// Database du KWLearningSpec initial du predicteur SNB appelant
	KWDatabase* masterInitialDatabase;

	// Domaine initial du predicteur SNB appelant
	KWClassDomain* masterInitialDomain;

	//////////////////////////////////////////
	// Objets de travail du maitre

	// SNBDataTableBinarySliceSet du maitre
	// Pas de buffer initalise
	SNBDataTableBinarySliceSet* masterBinarySliceSet;

	// Index de l'iteration externe
	int nMasterOuterIteration;

	// Nombre d'iterations externes
	int nMasterOuterIterationNumber;

	// Maximum de passes FastForwardBackward pour chaque iteration externe
	const int nMaxFastForwardBackwardRuns = 2;

	// Index de l'iteration Fast Forward Backward
	int nMasterFastForwardBackwardRun;

	// True si l'iteration FFBW est dans la phase Forward
	boolean bMasterIsOnFastAddRun;

	// Index de l'attribut aleatoire courant
	int nMasterRandomAttribute;

	// Attribut aleatoire courant
	SNBDataTableBinarySliceSetAttribute* masterRandomAttribute;

	// Nombre de taches qui finalise dans un pas d'une FastRun
	int nMasterFastRunStepFinishedTaskNumber;

	// True si le prochain pas les esclaves doivent defaire la derniere modification de la selection
	boolean bMasterUndoLastModification;

	// True si dans le prochain les esclaven doivent reinitialiser ses scores
	boolean bMasterInitializeSlaveScorers;

	// Meilleur score de l'iteration externe courante
	double dMasterCurrentScore;
	double dMasterCurrentModelCost;
	double dMasterCurrentDataCost;

	// Score de la modification courante
	double dMasterModificationScore;
	double dMasterModificationModelCost;
	double dMasterModificationDataCost;

	// Score de la derniere passe fast forward
	double dMasterPreviousRunScore;

	// Score de la selection vide
	double dMasterEmptySelectionScore;
	double dMasterEmptySelectionModelCost;
	double dMasterEmptySelectionDataCost;
	double dMasterMapScore;

	// Epsilon de precision pour les comparaisons des scores
	double dMasterPrecisionEpsilon;

	// Etat initial de la graine aleatoire
	int nMasterRandomSeed;

	// Progression de la tache
	double dMasterTaskProgress;

	// True si l'entrainement a finie avec sucess sans declencher la tache
	boolean bMasterIsTrainingSuccessfulWithoutRunningTask;

	//////////////////////////////////////////////////////
	// Objets de travail des esclaves

	// Identifiant du chunk attribue a l'esclave
	int nSlaveProcessChunkIndex;

	// Instance de la classe de recodage de l'esclave
	KWClass* slaveRecoderClass;

	// Instance de database bidon pour completer le LearningSpec de l'esclave
	KWSTDatabase* slaveDummyDatabase;

	// Database d'apprentissage de l'esclave
	SNBDataTableBinarySliceSet* slaveBinarySliceSet;

	////////////////////////////////////////////////////
	// Variables partagees

	// Learning spec de l'apprentissage
	PLShared_LearningSpec shared_learningSpec;

	// Nom de la classe de recodage
	PLShared_String shared_sRecoderClassName;

	// URI du fichier contenant le domaine de la classe de recodage
	PLShared_String shared_sRecoderClassDomainFileURI;

	// Identiifiants des processus esclaves
	PLShared_IntVector shared_ivGrantedSlaveProcessIds;

	// Indices de la cible pour toutes les instances
	PLShared_IntVector shared_ivTargetValueIndexes;

	// Relation [nom -> KWAttributeStats] pour les attributs simples
	PLShared_ObjectDictionary* shared_odAttributeStats;

	// Relation [nom -> KWAttributePairStats] pour les paires d'attributs
	PLShared_ObjectDictionary* shared_odAttributePairStats;

	// KWDataTableSliceSet de la base d'apprentissage
	PLShared_DataTableSliceSet shared_dataTableSliceSet;

	// Nombre initial d'attributs
	PLShared_Int shared_nInitialAttributeNumber;

	// Nombre d'instances
	PLShared_Int shared_nInstanceNumber;

	// Nombre de chunks du SNBDataTableBinarySliceSet de la base d'appretissage
	PLShared_Int shared_nChunkNumber;

	// Nombre d'attributs du SNBDataTableBinarySliceSet de la base d'appretissage
	PLShared_Int shared_nAttributeNumber;

	// Nombre de slices 'du SNBDataTableBinarySliceSet de la base d'appretissage
	PLShared_Int shared_nSliceNumber;

	// Attributes du SNBDataTableBinarySliceSet de la base d'apprentissage
	PLShared_ObjectArray* shared_oaBinarySliceSetAttributes;

	// Poids du prior du critere de selection
	PLShared_Double shared_dPriorWeight;

	// True si le cout de construction est considere dans le critere de selection
	PLShared_Boolean shared_bIsConstructionCostEnabled;

	// True si le cout de preparation est considere dans le critere de selection
	PLShared_Boolean shared_bIsPreparationCostEnabled;

	////////////////////////////////////////////
	// Entrees et sorties des taches

	// True si l'esclave doit defaire la derniere modification de la selection
	PLShared_Boolean input_bUndoLastModification;

	// Attribut de la derniere modification
	PLShared_Int input_nModificationAttribute;

	// True si la derniere modification etait faite sur une passe forward
	PLShared_Boolean input_bIsForwardModification;

	// True si l'esclave doit reinitiliser son scorer
	PLShared_Boolean input_bInitializeWorkingData;

	// Cout de donnes de la modification dans le chunk de l'esclave
	PLShared_Double output_dDataCost;
};

class SNBPredictorSNBEnsembleTrainingTask : public SNBPredictorSNBTrainingTask
{
public:
	// Constructeur
	SNBPredictorSNBEnsembleTrainingTask();
	~SNBPredictorSNBEnsembleTrainingTask();

	///////////////////////////////////////////////////////////////////////////////////////////////////
	///  Implementation
protected:
	// Reimplementation des methodes privees de PLParallelTask
	PLParallelTask* Create() const override;
	const ALString GetTaskName() const override;
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveInitialize() override;
	boolean SlaveProcess() override;
	boolean SlaveFinalize(boolean bProcessEndedCorrectly) override;

	// Reimplementation des methodes privees de SNBPredictorSNBTrainingTask
	longint ComputeMasterNecessaryMemory() const override;
	longint ComputeGlobalSlaveScorerNecessaryMemory() const override;
	longint ComputeSlaveScorerNecessaryMemory() const override;
	void MasterInitializeOptimizationVariables() override;
	void UpdateTaskProgressionLabel() const override;
	void UpdateSelection() override;
	void UpdateCurrentAttribute() override;
	boolean SelectionContainsAttribute(SNBDataTableBinarySliceSetAttribute* attribute) const override;
	boolean IsSelectionEmpty() const override;
	void InitializeNextFastForwardRun() override;
	void ComputeEmptySelectionScoreAndPrecisionEpsilon() override;
	double ComputeSelectionModelCost() override;
	void MasterFinalizeTrainingAndReports() override;

	/////////////////////////////////////////////////////
	// Objets de travail

	// Calculatrice de score de selection du maitre
	SNBHardAttributeSelectionScorer* masterHardSelectionScorer;

	// Calculatrice de poids des attributs
	SNBAttributeSelectionWeightCalculator* masterWeightCalculator;

	// Selection d'attributs MAP
	SNBHardAttributeSelection* masterMapSelection;

	// Calculatrice de score de selection de l'esclave
	SNBHardAttributeSelectionScorer* slaveSelectionScorer;
};

class SNBPredictorSNBDirectTrainingTask : public SNBPredictorSNBTrainingTask
{
public:
	// Constructeur
	SNBPredictorSNBDirectTrainingTask();
	~SNBPredictorSNBDirectTrainingTask();

	///////////////////////////////////////////////////////////////////////////////////////////////////
	/// Implementation
protected:
	// Reimplementation des methodes virtuelles privees de la classe ParallelTask
	PLParallelTask* Create() const override;
	const ALString GetTaskName() const override;
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveInitialize() override;
	boolean SlaveProcess() override;
	boolean SlaveFinalize(boolean bProcessEndedCorrectly) override;

	// Reimplementation des methodes virtuelles privees de SNBPredictorSNBTrainingTask
	longint ComputeMasterNecessaryMemory() const override;
	longint ComputeGlobalSlaveScorerNecessaryMemory() const override;
	longint ComputeSlaveScorerNecessaryMemory() const override;
	void MasterInitializeOptimizationVariables() override;
	void UpdateTaskProgressionLabel() const override;
	void UpdateSelection() override;
	void UpdateCurrentAttribute() override;
	boolean SelectionContainsAttribute(SNBDataTableBinarySliceSetAttribute* attribute) const override;
	boolean IsSelectionEmpty() const override;
	void InitializeNextFastForwardRun() override;
	double ComputeSelectionModelCost() override;
	void MasterFinalizeTrainingAndReports() override;

	// Maximum de passes FastForwardBackward pour chaque MultiStart
	const int nMaxFastForwardBackwardRuns = 2;

	// Delta poids de la modification courant
	double dMasterModificationDeltaWeight;

	// Calculatrice de score de selection du maitre
	SNBWeightedAttributeSelectionScorer* masterWeightedSelectionScorer;

	// Calculatrice de score de selection du esclave
	SNBWeightedAttributeSelectionScorer* slaveWeightedSelectionScorer;

	// Difference de poids de la modification courant (entree esclave)
	PLShared_Double input_dModificationDeltaWeight;
};
