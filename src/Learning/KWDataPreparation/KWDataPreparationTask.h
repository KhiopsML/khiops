// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataPreparationTask;

#include "PLParallelTask.h"
#include "KWLearningSpec.h"
#include "KWDataTableSliceSet.h"
#include "KWTupleTableLoader.h"
#include "KWDataGridOptimizer.h"
#include "MemoryStatsManager.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KWDataPreparationTask
// Preparation des donnees en parallele
// Classe virtuelle, ancetre des classes de preparation
class KWDataPreparationTask : public PLParallelTask
{
public:
	// Constructeur
	KWDataPreparationTask();
	~KWDataPreparationTask();

	// Calcul du nombre max d'attributs que l'on peut charger en memoire simultanement
	// en tenant compte de la memoire disponible et de la memoire necessaire pour le stockage
	// des resultats et de la memoire de travail
	// Retourne 0 si pas assez de memoire, avec message d'erreur sur la memoire manquante
	//
	// Cette methode est notamment utile pour le dimensionnement des ressources pour la parallelisation
	// Dans cette etape, on ne connait pas grande chose des donnees pour effectuer un dimensionnement fin.
	// On utilise ici une heuristique de dimensionnement "raisonnable" dans les cas des blocs d'attributs,
	// pour concilier la possibilite de les traiter efficacement et eviter une estimation trop conservatrice,
	// qui empecherait a priori le lancement des taches, alors que l'on ne connait pas encore le niveau de sparsite
	int ComputeMaxLoadableAttributeNumber(const KWLearningSpec* learningSpec, const KWTupleTable* targetTupleTable,
					      int nAttributePairNumber) const;

	// Verification des parametres d'entree
	boolean CheckInputParameters(const KWLearningSpec* learningSpec, const KWTupleTable* targetTupleTable) const;

	// Service de tri d'un ensemble d'attributs, de facon a preserver leur coherence vis a vis des blocs sparse
	// En sortie, deux attributs d'un meme bloc sont necessairement consecutifs
	void SortAttributesByBlock(ObjectArray* oaAttributes) const;
	boolean AreAttributesSortedByBlock(const ObjectArray* oaAttributes) const;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Methode principale a appeler pour lancer la tache de preparation avec
	// En entree:
	//  . learningSpec: les specifications d'apprentissage, comprenant la base source
	//  . tupleTableLoader: les statistiques sur le nombre d'enregistrements et les valeurs de l'eventuel attribut
	//  cible,
	//    incorporees en tant que ExtraAttribute dans le TupleTableLoader
	//  . dataTableSliceSet: la specification du decoupage de la base en tranche, avec les fichiers par tranche
	//  calcules
	// Dans une sous-classe, il faut avoir initialisee les eventuelles variables partagees
	// specifiques, puis apres la tache collecter les resultats de la taches
	// Methode interruptible, retourne false si erreur ou interruption (avec message), true sinon
	virtual boolean RunDataPreparationTask(KWLearningSpec* learningSpec, KWTupleTableLoader* tupleTableLoader,
					       KWDataTableSliceSet* dataTableSliceSet);

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Methodes de dimensionnement des taches
	// La tupleTable doit etre parametre correctement, avec les stats sur l'eventuel attribut cible

	// Estimation de la memoire necessaire pour stocker un resultat de preparation univariee
	// Prend en compte le type d'analyse et les statistiques cibles si elles sont disponibles
	longint ComputeNecessaryUnivariateStatsMemory(const KWLearningSpec* learningSpec,
						      const KWTupleTable* targetTupleTable) const;
	longint ComputeNecessaryBivariateStatsMemory(const KWLearningSpec* learningSpec,
						     const KWTupleTable* targetTupleTable) const;

	// Estimation de la memoire necessaire pour les informations sur l'attribut cible
	// Prend en compte a la fois la table de tuples cible, et les valeurs cibles si elle
	// sont associees via un attribut supplementaire de la table de tuples
	longint ComputeNecessaryTargetAttributeMemory(const KWLearningSpec* learningSpec,
						      const KWTupleTable* targetTupleTable) const;

	// Estimation de la memoire de travail minimale necessaire
	//  Base de de donnees chargee avec au minimum un attribut
	//  Memoire de travail pour proceder au pretraitement par grille
	// Prend en compte le type d'analyse et les statistiques cibles si elles sont disponibles
	longint ComputeNecessaryWorkingMemory(const KWLearningSpec* learningSpec, const KWTupleTable* targetTupleTable,
					      boolean bComputeAttributePairs) const;

	// Estimation de la memoire necessaire pour stocker un attribut de dictionnaire
	// On envisage une utilisation dans un bloc sparse pour ne pas sous-estimer la memoire necessaire
	longint ComputeNecessaryClassAttributeMemory() const;

	// Estimation de la memoire necessaire pour stocker un bloc vide de dictionnaire
	longint ComputeNecessaryClassAttributeBlockMemory() const;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Estimation heuristiques du nombre de valeurs par bloc, pour le dimensionnement des algorithme
	// Parametres en entree:
	//   . kwcClassToPartition: pour acceder au nombre total d'attribut par bloc, utilise pour estimer leur taux de
	//   sparsite
	//                          cf. KWAttributeBlock::GetEstimatedMeanValueNumber
	//   . kwcClassPart: pour acceder au nombre d'attribut du sous-bloc dans la partie, qui peut avoir un plus petit
	//   nombre d'attributs,
	//                   mais exploite la meme estimation du taux de sparsite
	//   . nMaxAttributeNumberPerBlock: parametre utilise pour borner eventuellement le nombre max d'attribut par
	//   bloc a prendre en compte
	//                                  (utile pour estimer les elements de dimensionnement avant la creation des
	//                                  sous-parties)
	//   . sTargetAttributeName: pour ignorer l'attribut cible si necessaire
	//   . nObjectNumber: nombre total d'instances de la base

	// Estimation heuristique des nombres max de valeurs par bloc pour une sous-partie d'une classe
	longint ComputeEstimatedMaxValueNumberPerBlock(const KWLearningSpec* learningSpec,
						       const KWTupleTable* targetTupleTable,
						       const KWClass* kwcCompleteClass, const KWClass* kwcPartialClass,
						       int nMaxAttributeNumberPerBlock) const;

	// Estimation heuristique du nombre total de valeurs dans les blocs pour une sous-partie d'une classe
	longint ComputeEstimatedTotalValueNumberInBlocks(const KWLearningSpec* learningSpec,
							 const KWTupleTable* targetTupleTable,
							 const KWClass* kwcCompleteClass,
							 const KWClass* kwcPartialClass) const;

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Methodes de dimensionnement pour gerer la memoire en cas de blocs d'attributs
	//
	// Une heuristique de dimensionnement pour les attributs de bloc est exploitee pour estimer
	// le taux de sparsite des blocs dependant de la taille des bloc

	// Calcul de la memoire de travail necessaire pour le chargement de toutes les tables de tuples d'un bloc
	longint ComputeBlockWorkingMemory(int nBlockAttributeNumber, longint lBlockValueNumber,
					  int nObjectNumber) const;

	// Calcul la place memoire totale minimum necessaire pour le stockage de toutes les valeurs de tous les
	// attributs charges d'une classe, hors attribut cible On prend egalement en compte la memoire additionnelle
	// necessaire a la gestion des valeurs des attributs dense Symbol, sur la base d'une estimation exploitantles
	// statistiques collectees dans une tranche
	longint ComputeDatabaseAllValuesMemory(KWDataTableSlice* slice, int nObjectNumber) const;

	// Calcul la place memoire totale minimum necessaire pour le stockage de toutes les valeurs de tous les
	// attributs charges d'une classe, hors attribut cible Cette estimation prend pas en compte l'overhead potentiel
	// lie aux valeurs Symbol en comptant au plus une variable avec autant de valeurs que d'instances
	longint ComputeDatabaseMinimumAllValuesMemory(int nDenseSymbolAttributeNumber,
						      int nDenseContinuousAttributeNumber, int nAttributeBlocNumber,
						      longint lTotalBlockValueNumber, int nObjectNumber) const;

	// Estimation de la place memoire additionnelle necessaire a la gestion des valeurs d'un attribut dense Symbol
	// dont on connait la place occupee sur disque. On reserve cette taille, et on utilise une heuristique
	// pour estimer le nombre de valeurs distinctes en fonction de la taille moyenne des Symbols
	// Cette estimation est approximative, car on peut reserver trop de place si peu de grande valeurs sont
	// en fait egales, ou pas assez en cas de grandes nombres de valeurs de petites tailles, mais distinctes
	longint ComputeEstimatedAttributeSymbolValuesMemory(longint lSymbolAttributeDiskSize, int nObjectNumber) const;

	// Taille moyenne escomptee pour les valeurs Symbol
	// On suppose qu'a partir de cette taille moyenne, il y aura autant de valeurs distinctes que d'individus
	longint GetExpectedMeanSymbolValueLength() const;

	// Memoire necessaire pour les structures de base
	longint GetNecessaryMemoryPerDenseValue() const;
	longint GetNecessaryMemoryPerEmptyValueBlock() const;
	longint GetNecessaryMemoryPerSparseValue() const;

	// Estimation de la memoire de travail necessaire pour le chargement de toutes les tables de tuples d'un bloc
	// On prend en compte le bloc le plus grand de la classe, eventuellement une sous partie en precisant
	// un nombre max d'attribut par bloc a prendre en compte
	longint ComputeEstimatedMaxBlockWorkingMemory(const KWLearningSpec* learningSpec,
						      const KWTupleTable* targetTupleTable,
						      const KWClass* kwcCompleteClass, const KWClass* kwcPartialClass,
						      int nMaxAttributeNumberPerBlock) const;

	// Estimation la place memoire totale necessaire pour le stockage de toutes les valeurs
	// de tous les attributs charges d'une classe, hors attribut cible
	longint ComputeEstimatedDatabaseAllValuesMemory(const KWLearningSpec* learningSpec,
							const KWTupleTable* targetTupleTable,
							const KWClass* kwcCompleteClass,
							const KWClass* kwcPartialClass) const;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Reimplementation des methodes virtuelles de tache

	// Reimplementation d'une partie des methodes, specialisables dans les sous-classes
	// Il s'agit essentiellement de la gestion des donnees en entree sous forme de variables partagees
	boolean MasterInitialize() override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveInitialize() override;
	boolean SlaveFinalize(boolean bProcessEndedCorrectly) override;

	///////////////////////////////////////////////////////////
	// Parametres partages par le maitre et les esclaves

	// Specification d'apprentissage
	// Ces specifications sont partagee pour parametrer le pretraitement des variables
	// par tranche de la base
	// Localement a chaque esclave, elles doivent etre reparametrees avec la classe et
	// la dabase locale a chaque tranche
	PLShared_LearningSpec shared_learningSpec;

	// Vecteur de valeurs numerique ou categorielles, et table de tuples de l'attribut cible
	// pour le parametrage d'un tupleTableLoader local a l'esclave
	PLShared_SymbolVector shared_svTargetValues;
	PLShared_ContinuousVector shared_cvTargetValues;
	PLShared_TupleTable shared_TargetTupleTable;

	//////////////////////////////////////////////////////
	// Variables de l'esclave

	// Chargeur de tuples de l'esclave
	KWTupleTableLoader slaveTupleTableLoader;

	//////////////////////////////////////////////////////
	// Variables du Master
	// Parametres de la methode principale, mis a disposition pour l'implementation des methodes de la tache

	// Memorisation de la classe et de la base des learningSpec partagees
	KWClass* masterClass;
	KWDatabase* masterDatabase;

	// Specification du decoupage de la base en tranches
	KWDataTableSliceSet* masterDataTableSliceSet;
};
