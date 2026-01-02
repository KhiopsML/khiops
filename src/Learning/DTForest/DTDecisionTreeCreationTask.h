// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KDDataPreparationAttributeCreationTask.h"
#include "DTDecisionTree.h"
#include "DTForestParameter.h"
#include "DTCreationReport.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe DTDecisionTreeCreationTask
// Preparation des donnees en parallele, utilisant des arbres de decision
class DTDecisionTreeCreationTask : public KDDataPreparationAttributeCreationTask
{
public:
	// Constructeur
	DTDecisionTreeCreationTask();
	~DTDecisionTreeCreationTask();

	// Prefixe utilise pour fabriquer un nom de rapport
	ALString GetReportPrefix() const override;

	// Prefixe des variables a cree (defaut: "")
	void SetCreatedAttributePrefix(const ALString& sValue);
	const ALString& GetCreatedAttributePrefix() const;

	void SetForestParameter(DTForestParameter* sValue);
	DTForestParameter* GetForestParameter();

	// Redefinition de la methode de creation d'attributs
	// En entree:
	//  . learningSpec: les specifications d'apprentissage, comprenant la base source
	//  . tupleTableLoader: les statistiques sur le nombre d'enregistrements et les valeurs de l'eventuel attribut
	//  cible,
	//    incorporees en tant que ExtraAttribute dans le TupleTableLoader
	//  . dataTableSliceSet: la specification du decoupage de la base en tranche, avec les fichiers par tranche
	//  calcules
	//	. odInputAttributeStats : dictionnaire contenant les KWAttributeStats a traiter
	// En sortie:
	//  . tableau de KWAttributeStats, et objet DTCreationReport contenant les DTDecisionTreeSpec crees par les
	//  esclaves
	// Methode interruptible, retourne false si erreur ou interruption (avec message), true sinon
	//
	boolean CreatePreparedAttributes(KWLearningSpec* learningSpec, KWTupleTableLoader* tupleTableLoader,
					 KWDataTableSliceSet* dataTableSliceSet,
					 ObjectDictionary* odInputAttributeStats,
					 ObjectArray* oaOutputAttributeStats) override;

	//
	// en sortie : cree des DTAttributeSelections, qui seront stockes dans le forestAttributeSelection passe en
	// parametre

	void BuildForestAttributeSelections(DTForestAttributeSelection& forestAttributeSelection,
					    int nMaxCreatedAttributeNumber);

	// Renvoie un rapport de creation dediee aux arbres construits
	KWLearningReport* GetCreationReport() override;

	// Recopie des specification de creation d'attributs
	void
	CopyAttributeCreationSpecFrom(const KDDataPreparationAttributeCreationTask* attributeCreationTask) override;

	boolean ComputeTree(KWLearningSpec* learningSpec, DTBaseLoader* blOrigine, DTDecisionTree* tree);
	ObjectArray* BuildRootAttributeStats(KWLearningSpec* learningSpec, DTBaseLoader* blOrigine,
					     ObjectDictionary* odInputAttributeStats);

	KWTupleTable* BuildTupleTableForClassification(DTDecisionTree* dttree, const ALString& svariablename) const;
	KWTupleTable* BuildTupleTableForRegression(const KWLearningSpec* learningSpec, DTDecisionTree* dttree,
						   const ALString& svariablename) const;

	// Calcul du nombre max d'attributs DecisionTree que l'on peut construire dans la memoire allouee a 1 esclave
	int ComputeOneSlaveMaxLoadableAttributeNumber(const int lGrantedlavesNumber,
						      const longint lGrantedMinSlaveMemory);

	///////////////////////////////////////////////////////////////////////////////
	// Implementation
protected:
	// creation d'un arbre de decision
	DTDecisionTree* CreateDecisionTree(KWLearningSpec* learningSpec, KWTupleTableLoader* tupleTableLoader,
					   ObjectArray* oaObjects, ObjectArray* oaInputAttributeStats,
					   DTAttributeSelection* attgenerator);

	// filtre pour un KWCLASS les attributs ayant un level=0
	void UnloadNonInformativeAttributes(KWClass* kwclass, ObjectDictionary* odInputAttributeStats);

	// filtre pour un KWCLASS les attributs les moins informatif
	void UnloadLessInformativeAttribute(KWClass* kwclass, ObjectDictionary* odInputAttributeStats,
					    int nloadattribut);

	// Filtre pour ne charger que les attributs les plus informatifs, tous les autres passant en unload
	// Necessaire pour utiliser correctment un slice set
	void LoadOnlyMostInformativeAttributes(KWClass* kwcClass, ObjectDictionary* odInputAttributeStats,
					       int nMaxAttributeNumber);

	void LoadOnlySelectedAttributes(KWClass* kwcClass, DTAttributeSelectionsSlices* attributeselectionsSlices);

	// Estimation de la memoire de travail minimale necessaire
	//  Base de de donnees chargee avec au minimum deux attributs
	//  Memoire de travail pour proceder au pretraitement par grille
	// Prend en compte le type d'analyse et les statistiques cibles si elles sont disponibles
	longint ComputeMedianStatMemory(ObjectDictionary* odInputAttributeStats) const;

	// Estimation la memoire de stockage de specification des arbres et des regles de derivation
	//  Base de de donnees chargee avec au minimum deux attributs
	//  Memoire de travail pour proceder au pretraitement par grille
	// Prend en compte le type d'analyse et les statistiques cibles si elles sont disponibles
	longint ComputeMeanTreeSpecMemory(ObjectDictionary* odInputAttributeStats) const;

	void BuildAttributeSelectionsSlices(const ObjectArray* oaAttributeSelections,
					    ObjectArray* oaAttributeSelectionsSlices, int nMaxloadVariables,
					    int nGrantedSlaveNumber);

	int ComputeSlavesNumberToFill(const ObjectArray* oaRemainingAttributesSelections,
				      const int nGrantedSlaveNumber) const;

	void ReferenceTargetIntervalValues(const ObjectDictionary* odAttributeStats);

	// transforme une regression en classification
	KWLearningSpec* InitializeRegressionLearningSpec(const KWLearningSpec*);

	// transforme une regression en classification en effectuant au prealable une discretisation EqualFreq
	void InitializeEqualFreqDiscretization(KWTupleTableLoader*, KWLearningSpec*);

	// Equal Freq Discretisation d'une target continue
	SymbolVector* EqualFreqDiscretizeContinuousTarget(KWTupleTableLoader*, int nQuantileNumber) const;

	// Discretisation 'MODL' d'une target continue
	SymbolVector* MODLDiscretizeContinuousTarget(KWTupleTableLoader*, int nMaxIntervalsNumber,
						     const ContinuousVector& cvIntervalValues, int nSplitIndex,
						     KWDataGridStats* targetStat) const;

	// transforme une regression en classification en effectuant au prealable une discretisation MODL
	void InitializeMODLDiscretization(KWTupleTableLoader*, KWLearningSpec*,
					  const ContinuousVector& cvIntervalValues, int nSplitIndex,
					  KWDataGridStats* targetStat);

	void InitializeBinaryEQFDiscretization(KWTupleTableLoader*, KWLearningSpec*,
					       const ContinuousVector& cvIntervalValues, int nSplitIndex,
					       KWDataGridStats* targetStat);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Reimplementation des methodes virtuelles de tache

	// Reimplementation des methodes
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean ComputeResourceRequirements() override;
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;
	boolean MasterAggregateResults() override;
	boolean MasterInitialize() override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveInitialize() override;
	boolean SlaveProcess() override;

	// Calcul des donnees de dimensionnement et de pilotage de la tache
	void ComputeTaskInputs();
	void CleanTaskInputs();

	// Affichage d'un tableau de DTAttributeSelectionsSlices
	void WriteAttributeSelectionsSlices(const ObjectArray* oaAttributeSelectionsSlices, ostream& ost) const;

	////////////////////////////////////////////////////////////
	// Implementation du ComputeResourceRequirements

	// Estimation de la memoire partagee
	longint ComputeSharedNecessaryMemory();

	// Estimation de la memoire du maitre
	longint ComputeMasterNecessaryMemory();

	// Estimation de la memoire du plus gros arbre possible
	longint ComputeBiggestTreeNecessaryMemory();

	// estimations memoire
	void InitializeMemoryEstimations();

	int GetNodeVariableNumber() const;

	///////////////////////////////////////////////////////////
	// Parametres partages par le maitre et les esclaves

	// Acces aux statistiques univariees
	PLShared_ObjectDictionary* shared_odAttributeStats;

	// Nombre d'instances par chunk, pour parametrer correctement les DataTableSliceSet de chaque esclave
	PLShared_IntVector shared_ivDataTableSliceSetChunkInstanceNumbers;

	//////////////////////////////////////////////////////
	// Parametre en entree et sortie des esclaves
	// Chaque esclave traite un sous-ensemble de selections d'attributs,
	// stockes dans une ou deux tranche de la base

	PLShared_ObjectArray* input_oaAttributeSelections;

	// Liste des attributs impliques dans les arbres
	PLShared_StringVector input_svAttributeNames;

	PLShared_ForestParameter* input_forestParameter;

	// Tranches de la base contenant les attributs impliquees dans les arbres
	PLShared_ObjectArray* input_oaDataTableSlices;

	// liste des coupures effectuees sur la cible continue (cas de la regression)
	PLShared_ContinuousVector input_cvIntervalValues;

	// list des valeurs de split par arbre (cas de la regression)
	PLShared_IntVector input_ivSplitValues;

	// Preparation des attributs d'une tranche de base
	// En sortie, les attributs arbres sont completes avec leur analyse
	PLShared_ObjectArray* output_oaAttributeStats;

	// liste des objets DTDecisionTreeSpec * crees par 1 esclave
	PLShared_ObjectArray* output_oaDecisionTreeSpecs;

	//////////////////////////////////////////////////////
	// Variables de l'esclave

	//////////////////////////////////////////////////////
	// Variables du Master

	ObjectDictionary* odMasterInputAttributeStats;
	KWTupleTableLoader* masterTupleTableLoader;

	// Ensemble des resultats de preparations des arbres (liste de KWAttributeStats *)
	// Ces resultats seront rendu directement a l'appelant de la tache
	ObjectArray* oaMasterOutputAttributeStats;

	// ensemble des resultats de preparations d'arbres de decision (liste de DTDecisionTreeSpec *)
	// ce sont les resultats aggreges par le process maitre, au fur et a mesure de la collecte des resultats
	// provenant des esclaves
	ObjectArray* oaMasterAllDecisionTreeSpecs;

	// Rapport de creation des arbres. Il est alimente en fin de tache, par les objets DTDecisionTreeSpec crees par
	// tous les esclaves
	DTCreationReport masterOutputCreationReport;

	int nMasterDatabaseObjectNumber;

	// Estimations memoire
	longint lMasterAllResultsMeanMemory; // estimation de la memoire prise par tous les arbres calcules par tous les
					     // esclaves
	longint lMasterMeanTreeSpecMemory;
	longint lMasterMedianStatMemory;
	longint lMasterTreeResultMeanMemory;
	longint lMasterOneAttributeValueMemory;
	longint lMasterTreeWorkingMemory;
	longint lMasterEmptyObjectSize;
	int nMasterForestMaxAttributesSelectionNumber;

	//////////////////////////////////////////////////////
	// Variable du Master pour le pilotage des esclaves
	// Ces varables sont calculees avant le dimensionnement et le lancement de la tache

	// Specification de l'ensemble des attributs a analyser
	ObjectArray oaInputAttributeSelectionStats;

	// Specification de l'ensemble des DTAttributeSelectionsSlices, chacune contenant la specification
	// du travail d'un esclave
	ObjectArray oaInputAttributeSelectionsSlices;

	// Prefixe des variables a cree (defaut: "")
	ALString sCreatedAttributePrefix;

	// parametre du random forest
	DTForestParameter randomForestParameter;

	// selection des attributs
	DTForestAttributeSelection* forestattributeselection;

	// indicateur d'apprentissag des arbres
	int bIsTraining;

	// trace d'execution du process maitre
	boolean bMasterTraceOn;
};

int DAttributeArrayCompareBlocks(const void* elem1, const void* elem2);

inline void DTDecisionTreeCreationTask::SetForestParameter(DTForestParameter* sValue)
{
	randomForestParameter.CopyFrom(sValue);
}

inline DTForestParameter* DTDecisionTreeCreationTask::GetForestParameter()
{
	return &randomForestParameter;
}
