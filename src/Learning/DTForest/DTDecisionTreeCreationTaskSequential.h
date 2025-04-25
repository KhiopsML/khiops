// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDataPreparationTask.h"
#include "KWDataPreparationUnivariateTask.h"
#include "KWAttributeStats.h"
#include "KWDRMath.h"
#include "KWDRString.h"
#include "KDDataPreparationAttributeCreationTask.h"
#include "DTDecisionTree.h"
#include "DTForestParameter.h"
#include "DTCreationReport.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe DTDecisionTreeCreationTaskSequential
// Exemple simple de service de creation de nouveaux attributs et de leur preparation
// Creation de paire d'attributs basee sur une discretisation non supervisee prealable
// des attributs numeriques, et sur la concatenation des valeurs par paire d'attributs
class DTDecisionTreeCreationTaskSequential : public KDDataPreparationAttributeCreationTask
{
public:
	// Constructeur
	DTDecisionTreeCreationTaskSequential();
	~DTDecisionTreeCreationTaskSequential();

	// Prefixe utilise pour fabriquer un nom de rapport
	ALString GetReportPrefix() const override;

	// Prefixe des variables a cree (defaut: "")
	void SetCreatedAttributePrefix(const ALString& sValue);
	const ALString& GetCreatedAttributePrefix() const;

	// Prefixe des variables a cree (defaut: "")
	void SetForestParameter(DTForestParameter* sValue);
	DTForestParameter* GetForestParameter();

	// Redefinition de la methode de creation d'attributs
	boolean CreatePreparedAttributes(KWLearningSpec* learningSpec, KWTupleTableLoader* tupleTableLoader,
					 KWDataTableSliceSet* dataTableSliceSet,
					 ObjectDictionary* odInputAttributeStats,
					 ObjectArray* oaOutputAttributeStats) override;

	boolean BluidForestAttributeSelections(DTForestAttributeSelection& forestAttributeSelection,
					       int nMaxCreatedAttributeNumber);

	// Renvoie un rapport de creation dediee aux arbres construits
	KWLearningReport* GetCreationReport() override;

	// Recopie des specification de creation d'attributs
	void
	CopyAttributeCreationSpecFrom(const KDDataPreparationAttributeCreationTask* attributeCreationTask) override;

	// selection de varibale pour la creation d'un arbre
	// void SelectTreeAttributes(DTDecisionTree* tree, DTAttributeSelection* attgenerator, longint nObject);
	boolean ComputeTree(KWLearningSpec* learningSpec, DTBaseLoader* blOrigine, DTDecisionTree* tree);
	ObjectArray* BuildRootAttributeStats(KWLearningSpec* learningSpec, DTBaseLoader* blOrigine,
					     ObjectDictionary* odInputAttributeStats);
	KWAttributeStats* BuildAttributeStats(KWLearningSpec* learningSpec, DTDecisionTree* dttee,
					      const ALString& svariablename) const;
	KWTupleTable* BuildTupleTableForClassification(DTDecisionTree* dttree, const ALString& svariablename) const;
	KWTupleTable* BuildTupleTableForRegression(const KWLearningSpec* learningSpec, DTDecisionTree* dttree,
						   const ALString& svariablename) const;

	// Calcul du nombre max d'attributs DecisionTree que l'on peut construire en memoire simultanement
	// en tenant compte de la memoire disponible et de la memoire necessaire pour le stockage
	// des resultats et de la memoire de travail
	// Cette methode est notamment utile pour le dimensionnement des ressources pur la parallelisation
	// Retourne 0 si pas assez de memoire, avec message d'erreur sur la memoire manquante
	int ComputeMaxLoadableAttributeNumber(const KWLearningSpec* learningSpec, const KWTupleTable* targetTupleTable,
					      ObjectDictionary* odInputAttributeStats, int nDecisionTreeMaxNumber,
					      DTForestAttributeSelection& forestAttributeSelection) const;

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
	longint ComputeMeanStatMemory(ObjectDictionary* odInputAttributeStats) const;
	longint ComputeMedianStatMemory(ObjectDictionary* odInputAttributeStats) const;

	// Estimation la memoire de stockage de specification des arbres et des regles de derivation
	//  Base de de donnees chargee avec au minimum deux attributs
	//  Memoire de travail pour proceder au pretraitement par grille
	// Prend en compte le type d'analyse et les statistiques cibles si elles sont disponibles
	longint ComputeMeanTreeSpecMemory(ObjectDictionary* odInputAttributeStats) const;

	void BuildAttributeSelectionsSlices(const ObjectArray* oaAttributeSelections,
					    ObjectArray* oaAttributeSelectionsSlices,
					    KWDataTableSliceSet* dataTableSliceSet, int nMaxloadVariables) const;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Reimplementation des methodes virtuelles de tache

	// Reimplementation des methodes
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;
	boolean MasterAggregateResults() override;
	boolean SlaveProcess() override;

	// transforme une regression en classification et retourne un nouveau learningSpec
	KWLearningSpec* InitializeEqualFreqDiscretization(KWTupleTableLoader*, const KWLearningSpec*);

	// Equal Freq Discretisation d'une target continue
	SymbolVector* EqualFreqDiscretizeContinuousTarget(KWTupleTableLoader*, const int nQuantileNumber) const;

	// Prefixe des variables a cree (defaut: "")
	ALString sCreatedAttributePrefix;

	// parametre du random forest
	DTForestParameter randomForestParameter;

	// Rapport de creation
	DTCreationReport creationReport;

	// indicateur d'apprentissag des arbres
	int bIsTraining;
};

int DAttributeArrayCompareBlocks(const void* elem1, const void* elem2);

// Prefixe des variables a cree (defaut: "")
inline void DTDecisionTreeCreationTaskSequential::SetForestParameter(DTForestParameter* sValue)
{
	randomForestParameter.CopyFrom(sValue);
}

inline DTForestParameter* DTDecisionTreeCreationTaskSequential::GetForestParameter()
{
	return &randomForestParameter;
}
