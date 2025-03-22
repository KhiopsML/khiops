// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataPreparationUnivariateTask;
class KWDataItemMemoryFootprint;

#include "KWDataPreparationTask.h"
#include "KWAttributeStats.h"
#include "KWDatabaseSlicerTask.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KWDataPreparationUnivariateTask
// Preparation univariee des donnees en parallele
class KWDataPreparationUnivariateTask : public KWDataPreparationTask
{
public:
	// Constructeur
	KWDataPreparationUnivariateTask();
	~KWDataPreparationUnivariateTask();

	// Calcul de la preparation des donnees en parallele
	// En entree:
	//  . learningSpec: les specifications d'apprentissage, comprenant la base source
	//  . tupleTableLoader: les statistiques sur le nombre d'enregistrements et les valeurs de l'eventuel attribut
	//  cible,
	//    incorporees en tant que ExtraAttribute dans le TupleTableLoader
	//  . dataTableSliceSet: la specification du decoupage de la base en tranche, avec les fichiers par tranche
	//  calcules
	// En sortie:
	//  . un KWAttributeStats par attribut source analyse, cree dans le dictionnaire en sortie
	// Attention, le dataTableSliceSet en sortie peut comporter plus de tranche que celui en entree
	// Methode interruptible, retourne false si erreur ou interruption (avec message), true sinon
	boolean CollectPreparationStats(KWLearningSpec* learningSpec, KWTupleTableLoader* tupleTableLoader,
					KWDataTableSliceSet* dataTableSliceSet,
					ObjectDictionary* odOutputAttributeStats);

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Services de base de preparation sur tout ou partie des attributs et des objets
	// Ces services sont appelable directement, et ne sont pas paralellises

	// Service basique de calcul de la preparation des donnees, non paralelise, pour une classe donnee et ses objets
	// En entree:
	//  . learningSpec: les specifications d'apprentissage
	//  . tupleTableLoader: les statistiques sur le nombre d'enregistrements et les valeurs de l'eventuel attribut
	//  cible,
	//    le tupleLoader doit etre correctement parametre avec son InputClass, InputDatabaseObjects et
	//    ExtraAttribute (eventuel)
	//  . oaInputAttributes: sous partie des attributs a analyser, tries de facon coherente avec les blocs sparse
	//  . bDisplayTaskProgression: parametrage du suivi ou non de la progression de la tache
	// En sortie:
	//  . oaOutputAttributeStats: un KWAttributeStats par attribut source analyse, cree dans le tableau en sortie,
	//  parametre par le LearningSpec
	// Methode interruptible, avec suivi de tache si necessaire
	// Retourne false interruption (avec message et destruction des resultats en sortie), true sinon
	boolean BasicCollectPreparationStats(KWLearningSpec* learningSpec, KWTupleTableLoader* tupleTableLoader,
					     ObjectArray* oaInputAttributes, boolean bDisplayTaskProgression,
					     ObjectArray* oaOutputAttributeStats) const;

	// Collecte de tous les attributs a analyser d'une classe
	// (tous ceux, hors attribut cible, qui sont Loaded)
	void CollectInputAttributes(KWLearningSpec* learningSpec, KWClass* kwcClass,
				    ObjectArray* oaCollectedAttributes) const;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	////////////////////////////////////////////////////////////////////////////////
	// Heuristique de dimensionnement
	//
	// Le SliceSet en entree de la preparation a ete dimensionne en utilisant des heuristiques, notamment en ce qui
	// concerne le taux de remplissage des blocs sparse. On exploite dans un  premier temps les memes heuristiques,
	// pour que les taches de preparation puissent etre lancee selon les memes ressources disponibles que pour la
	// calcul du SliceSet.
	//
	// Ensuite, on collecte les statistiques collectees suivante par tranche
	//   . le nombre d'attributs, sparse ou non: pour le dimensionnement des resultats de preparation
	//   . la memoire utilise par la tranche: principalement, celle pour la definition de la kwClass
	//   . la memoire de travail pour la gestion des blocs: pour charger simultanement toutes les tables de tuples
	//   du plus gros bloc de la tranche . le nombre maximum de tuple presents simultanement en memoire: correspond
	//   au calcul de la memoire de travail pour la gestion des blocs . la memoire de travail pour le stockage de
	//   toutes les valeurs de la tranche . nombre total de valeurs presents en memoire, correspondant a la  la
	//   memoire de travail pour le stockage de toutes les valeurs
	// Cela permet de dimensionner les esclaves, toujours en se basant sur les heuristique de dimensionnement.
	//
	// Au cas ou ces estimation serait sous-evaluees, se sera a la charge de chaque esclave d'adapter ses
	// algorithmes aux ressources disponibles. Pour cela, l'esclave dispose des elements de dimensionnement et des
	// estimations de nombre de tuples ou de valeurs a la base de ces estimations. En cas de depassement des
	// ressources disponibles, les esclaves pouront choisir:
	//   . de faire plusieurs passes de lecture de leur sliceSet
	//   . de faire plusieurs passe de traitement par bloc sparse s'il sont trop pleins
	// en exploitant:
	//   . la difference entre la ressource minimum demandee et la ressoruce allouee
	//   . la proportion entre le nombre de valeurs effectifs et les nombre de valeurs estimes
	//   . la proportion entre les nombres de tuples effectifs et les nombre de tuples estimes
	// Les information de dimensionnement basees sur les heuristiques sont passe a tous les esclaves via des
	// variables partagees
	//
	//
	// Alors que le DataTableSliceSet a ete partitionne en slices d'attributs et chunks d'instances selon des
	// heuristiques "raisonnables" de dimensionnement pour tenir compte de la sparsite des blocs d'attributs,
	// on beneficie ici de statistiques de volumetries collectee lors du calcul des slices et chunks
	// (variables par slices, nombres de valeurs effectives par bloc d'attributs, nombre d'instances par chunk,
	// taille des fichiers de co-chunk).
	// La tache de preparation va ici exploiter les meme elements de dimensionnement heuristique pour decouper
	// le travail par esclave selon les slices d'attribut. Charge ensuite a chaque esclave d'exploiter les
	// statistique de volumetrie pour soit calculer la preparation pour les attributs de la tranche, soit
	// decouper la tranche en sous-tranches plus petites, qui remplaceront la tranche initiale dans le
	// slice set initial et seront traitees par la suite
	// En cas de redecoupage d'une tranche, celle si est detruite et remplacee par NULL dans le sliceSet initial,
	// ce qui permet de preserver l'indexation du sliceSet. Les sous-tranche produites sont rajoutees en fin de
	// sliceSet, ce qui fait qu'elle seront traitees comme les autre en continuant le parcours sequentiel des
	// tranches a traiter. En fin de tache, le sliceSet en sortie est nettoye en supprimant les "trous"
	// correspondant au ancienne tranches detruites (reperees par des NULL).
	// Attention, la gestion de l'avancement est delicate, car un esclave lance pour de la preparation d'une tranche
	// d'attributs peut echouer a effectuer son travail, et au contraire creer d'autre sous-tranches a traiter.
	// Cette gestion de l'avancement est entierement traitee dans MasterPrepareTaskInput.
	// Il faut egalement faire attention a ce que la tache peut ne pas etre finie, meme s'il toutes les tranches
	// sont traitees ou en cours de traitement. Il faut attendre la fin des esclaves pour savoir s'il reste
	// des tranches d'attributs a preparer. Ceci est gere dans MasterPrepareTaskInput et MasterAggregateResults,
	// en utilisant les methodes avancees de PLParallelTask pour mettre en sommeil des esclaves ou les reveiller.

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Reimplementation des methodes virtuelles de tache

	// Reimplementation des methodes
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean ComputeResourceRequirements() override;
	boolean MasterInitialize() override;
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;
	boolean MasterAggregateResults() override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveProcess() override;

	// Initialisation du vecteur de tri d'une tranche, permettant au maitre d'ordonner les tranches par complexite
	// decroissante
	void InitializeSliceLexicographicSortCriterion(KWDataTableSlice* slice) const;

	// Libelles des criteres de tri d'une tranche, separes par des tabulations
	const ALString GetSliceLexicographicSortCriterionLabels() const;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Methodes de travail pour les esclave

	// Calcul et collecte des statistiques de preparation d'une tranche depuis un esclave correctement initialise
	boolean SlaveSliceCollectPreparationStats(KWDataTableSlice* slice, ObjectArray* oaOutputAttributeStats);

	// Calcul et collecte des statistiques de preparation d'un tableau de sous-tranches d'une tranche depuis un
	// esclave orrectement initialise
	boolean SlaveSliceArrayCollectPreparationStats(KWDataTableSlice* slice, ObjectArray* oaSubSlices,
						       ObjectArray* oaOutputAttributeStats);

	// Redecoupage d'une tranche en un ensemble de tranches plus petites, de facon a respecter les contraintes
	// memoire Entree:
	//   . slice: tranche avec son dictionnaire comportant tous les attributs utilises
	//   . lAvailableWorkingMemory: taille memoire disponible pour le chargement des valeurs ainsi que pour la
	//   preparation du
	//                              du pus gand bloc de la tranche, ce qui definit la contrainte memoire par
	//                              sous-tranche
	//   . lSplitAvailableMemory: memoire disponible pour effectuer le split de la tranche
	// Sortie:
	//   . oaSubSlices: tableau des sous-tranches crees, avec les fichiers corresppondant et les statistiques
	//   memoire collectees
	boolean SplitSlice(KWDataTableSlice* slice, int nObjectNumber, longint lAvailableWorkingMemory,
			   longint lSplitAvailableMemory, ObjectArray* oaSubSlices);

	// Partitionnement des attributs d'une tranche a charger selon une contrainte de taille memoire maximum
	// pour le stockage de l'ensemble des valeurs denses ou sparse (cf.
	// KWDataPreparationTask::ComputeDatabaseAllValuesMemory) Entree:
	//   . slice: tranche avec son dictionnaire comportant tous les attributs utilises
	//   . nObjectNumber: nombre d'instances
	//   . lAvailableWorkingMemory: taille memoire disponible pour le chargement des valeurs ainsi que pour la
	//   preparation du
	//                              du pus gand bloc de la tranche, ce qui definit la contrainte memoire par
	//                              sous-tranche
	// Sortie:
	//   . ivUsedAttributeStepIndexes: pour chaque attribut, index de l'etape de chargement (-1 si attribut en
	//   Unused)
	// Retourne le nombre de parties de la partition
	int ComputeSlicePartition(KWDataTableSlice* slice, int nObjectNumber, longint lAvailableWorkingMemory,
				  IntVector* ivUsedAttributePartIndexes);

	///////////////////////////////////////////////////////////
	// Parametres partages par le maitre et les esclaves

	//////////////////////////////////////////////////////
	// Parametre en entree et sortie des esclaves

	// Tranche de la base a traiter par un esclave
	PLShared_DataTableSlice input_DataTableSlice;

	// Identifiant de la tranche traitee
	PLShared_String output_sSliceClassName;

	// Preparation des attributs d'une tranche de base dans le cas ou il y a assez de ressource pour traiter la
	// tranche
	PLShared_ObjectArray* output_oaSliceAttributeStats;

	// Tableau de sous-tranches pretes a l'emploi si l'esclave a du redecoupe la tranche pour manque de ressource
	PLShared_ObjectArray* output_oaDataTableSubSlices;

	///////////////////////////////////////////////////////////////////
	// Variables partagees pour le indiquer aux esclaves les elements
	// de dimensionnement concernant la plus grande tranche
	// Ces elements de dimensionnement ont ete evalues selon les
	// heuristique initiales d'evaluation du taux de sparsite des blocs
	// Chaque esclave, connaissant les taux de sparsite reel, aura ainsi
	// les informations utiles pour evaluer si les ressources disponible
	// permettent de tout traiter en une seule passe, ou si plusieurs
	// passes sont necessaire

	// Nombre d'attributs, sparse ou non: pour le dimensionnement des resultats de preparation
	PLShared_Int shared_nLargestSliceAttributeNumber;

	// Memoire utilise par la tranche: principalement, celle pour la definition de la kwClass
	PLShared_Longint shared_lLargestSliceUsedMemory;

	// Memoire de travail pour la gestion des blocs: pour charger simultanement toutes les tables de tuples du plus
	// gros bloc de la tranche
	PLShared_Longint shared_lLargestSliceMaxBlockWorkingMemory;

	// Memoire de travail pour le stockage de toutes les valeurs de la tranche
	PLShared_Longint shared_lLargestSliceDatabaseAllValuesMemory;

	//////////////////////////////////////////////////////
	// Variables de l'esclave

	//////////////////////////////////////////////////////
	// Variables du Master

	// Nombre total d'attributs a traiter
	int nMasterTotalAttributeNumber;

	// Pourcentage total utilise dans la methode MasterPrepareTaskInput
	double dMasterCumulatedPercentage;

	// Liste trie des tranches selon le critere de tri lexicographique
	// Il s'agit de la liste des tranches, ou sous-tranches, restant a traiter, classes par complexite croissantes
	SortedList* slMasterSortedSlices;

	// Dictionnaire des tranches du sliceset
	// Au debut, ce dictiunnaire est initialisee par les tranches du sliceset en entree
	// Au fur et a mesure du traitement des tranches, quand une tranche est decoupee en sous-tranche, la tranche est
	// supprimee du dictionnaire et detruite, et les sous-tranches sont inseree dans le dictionnaire En fin de
	// traitement, les tranche du dictinnaire sont triees selon l'ordre lexicographique des tranches et
	// sous-tranches, et rempecnt les tranche initailes dans le sliceset en sortie
	ObjectDictionary* odMasterSlices;

	// Ensemble des preparations de tous les attributs de la base
	ObjectDictionary* odMasterDatabaseAttributeStats;
};

/////////////////////////////////////////////////////////////////////////////////
// Classe KWDataItemMemoryFootprint
// Empeinte memoire d'un attribut ou bloc d'attribut
// Class interne, pour le dimensionnement des etapes de lecture dans le SlaveProcess
// de la preparation des donnees
class KWDataItemMemoryFootprint : public Object
{
public:
	// Constructeur
	KWDataItemMemoryFootprint();
	~KWDataItemMemoryFootprint();

	// Data item concerne
	// Memoire: appartient a l'appelant
	void SetDataItem(KWDataItem* dataItemValue);
	KWDataItem* GetDataItem() const;

	// Index des attributs utilises concernes dans la classe
	// Un seul dans le cas d'un attribut, et potentiellement plusieurs pour un bloc d'attributs
	IntVector* GetUsedAttributeIndexes();

	// Memoire necessaire pour le stockage du DataItem sur toute la base
	// Dans le cas d'un bloc d'attribut, on prend les blocs a vide
	void SetDataItemNecessaryMemory(longint lValue);
	longint GetDataItemNecessaryMemory() const;

	// Memoire necessaire pour le stockage de tous les valeurs sparses en cas de bloc
	void SetAllSparseValuesNecessaryMemory(longint lValue);
	longint GetAllSparseValuesNecessaryMemory() const;

	// Memoire necessaire totale pour le stockage des valeurs
	longint GetDatabaseAllValuesMemory() const;

	// Memoire de travail necessaire pour le chargement de toutes les tables de tuples d'un bloc
	void SetBlockWorkingMemory(longint lValue);
	longint GetBlockWorkingMemory() const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	KWDataItem* dataItem;
	IntVector ivUsedAttributeIndexes;
	longint lDataItemNecessaryMemory;
	longint lAllSparseValuesNecessaryMemory;
	longint lBlockWorkingMemory;
};

// Methode de comparaison basee sur la memoire totale necessaire pour le stockage des valeurs
// La memoire de travail n'est pas ici prise en compte, car un seul bloc - le plus grand -
// par tranche devra etre pris en compte
int KWDataItemMemoryFootprintCompare(const void* elem1, const void* elem2);
