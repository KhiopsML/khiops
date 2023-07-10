// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataPreparationBivariateTask;
class KWAttributePairsSlices;
class KWAttributePair;

#include "KWDataPreparationTask.h"
#include "KWAttributeSubsetStats.h"
#include "KWClassStats.h"
#include "KWAttributePairsSpec.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KWDataPreparationBivariateTask
// Preparation univariee des donnees en parallele
class KWDataPreparationBivariateTask : public KWDataPreparationTask
{
public:
	// Constructeur
	KWDataPreparationBivariateTask();
	~KWDataPreparationBivariateTask();

	// Calcul de la preparation des donnees en parallele
	// En entree:
	//  . learningSpec: les specifications d'apprentissage, comprenant la base source
	//  . classStats: specification de l'apprentissage bivarie, avec acces aux statistiques univariees calculees
	//  . tupleTableLoader: les statistiques sur le nombre d'enregistrements et les valeurs de l'eventuel attribut
	//  cible,
	//    incorporees en tant que ExtraAttribute dans le TupleTableLoader
	//  . dataTableSliceSet: la specification du decoupage de la base en tranche, avec les fichiers par tranche
	//  calcules
	// En sortie:
	//  . un KWAttributePairStats par paire d'attributs analysee, cree dans le tableau en sortie
	// Methode interruptible, retourne false si erreur ou interruption (avec message), true sinon
	//
	// Se referer a KWDataPreparationUnivariateTask::CollectPreparationStats pour la strategie de dimensionnement
	boolean CollectPreparationPairStats(KWLearningSpec* learningSpec, KWClassStats* classStats,
					    KWTupleTableLoader* tupleTableLoader,
					    KWDataTableSliceSet* dataTableSliceSet, ObjectArray* oaAttributePairStats);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	////////////////////////////////////////////////////////////////////////////////
	// Heuristique de dimensionnement
	//
	// La preparation univariee exploite des heuristiques de dimensionnement bases dans un premier temps
	// sur des dimensionnements speculatifs, et dans un deuxieme temps des dimensionnements affines
	// exploitant des statistiques collectes par tranche (taux de sparsites des blocs, tailles des valeurs
	// categorielles. Et cela peut reconduire au redecoupage des tranches trop grosses en sous-tranches.
	//
	// La preparation bivariee  exploite directement toutes les informations collectees dans les tranches.
	// Dans un premier temps, toutes les paires d'attributs sont regroupees par paire de tranches concernees.
	// Et le dimensionnement est effectue pour la plus grande de ces paires de tranches.
	// On n'envisage pas ici une heuristique plus fine permettant d'analyse les paires d'attributs meme
	// si leur paire de tranches est trop grosses, qui se baserait si plusieurs passe de lecture des tranches
	// concernees. Cela permettrait de repousser les limites pour l'analyse des paires, mais cela demande
	// un surcout de developpement non justifie par les besoins et usages actuels.
	//
	// De meme, a titre de simplification, on impose que toutes les preparations univariees impliquers
	// dans les paires a analyser soit traites en variables partagees par tous les esclaves, alors que
	// chaque esclave pourait receboir en input uniquement les preparations univariees dont il a besoin.

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Reimplementation des methodes virtuelles de tache

	// Reimplementation des methodes
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean ComputeResourceRequirements() override;
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;
	boolean MasterAggregateResults() override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveInitialize() override;
	boolean SlaveProcess() override;

	// Calcul des donnees de dimensionnement et de pilotage de la tache
	void ComputeTaskInputs();
	void CleanTaskInputs();

	// Initialisation du vecteur de tri d'une paire de tranches, permettant au maitre d'ordonner les paires de
	// tranches par complexite decroissante
	void
	InitializeAttributePairsSlicesLexicographicSortCriterion(KWAttributePairsSlices* attributePairsSlices) const;

	// Libelles des criteres de tri d'une paire de tranches, separes par des tabulations
	const ALString GetAttributePairsSlicesLexicographicSortCriterionLabels() const;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Specification des paires d'attributs a analyser

	// Acces aux specifications de l'analyse bivariee
	const KWAttributePairsSpec* GetAttributePairsSpec() const;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Partitionnement des paires d'attributs selon les tranches, pour preparer le pilotage de l'anayse bivariee
	// en groupant les paires d'attributs de facon a minimiser le nombre de lectures de tranches

	// Initialisation d'un tableau de KWAttributePairsSlices elementaires, contenant chacun une paire
	// Memoire: le contenu du tableau resultat appartient a l'appelant
	void InitializeSingletonAllAttributePairsSlices(const ObjectArray* oaAttributePairStats,
							ObjectArray* oaAttributePairsSlices) const;

	// Fusion des KWAttributePairsSlices associes aux memes ensemble de tranches
	// En entree, chaque paire d'attribut est dans sa paire de tranche individuelle
	// En sortie, les paires d'attributs associees aux memes tranches sont regroupees
	// Memoire: le tableau, en entree et sortie contient des paires qui seront fusionnees, les autres etant
	// detruites
	void MergeAttributePairsSlices(ObjectArray* oaAttributePairsSlices) const;

	// Affichage d'un tableau de KWAttributePairsSlices
	void WriteAttributePairsSlices(const ObjectArray* oaAttributePairsSlices, ostream& ost) const;

	///////////////////////////////////////////////////////////////////////////////////
	// Gestion des paires d'attributs a analyser dans un esclave
	// au moyen d'objet KWAttributePair
	// L'objectif est de grouper l'analysed es paires d'attributs quand elles
	// partagent les meme block

	// Initialisation d'un tableau de KWAttributePair a partir des noms des attributs des paires
	// Les paire cree ont un index correspondant a leur rang dans les vecteurs de nomz en entree
	// Memoire: le contenu du tableau en sortie appartient a l'appelant
	void InitializeAttributePairs(const KWClass* kwcClass, const StringVector* svAttributePairNames1,
				      const StringVector* svAttributePairNames2,
				      ObjectArray* oaOuputAttributePairs) const;

	// Tri d'un tableau de paires d'attributs, selon leur bloc
	void SortAttributePairsByBlocks(ObjectArray* oaAttributePairs) const;

	// Recherche dans un tableau de paires d'attributs tries par blocs,
	// d'un groupe de paires ayant le meme premier bloc (ou pas de bloc)
	// On calcule en sortie un dictionnaire des attributs du premier bloc (s'il sont dans un bloc)
	// On part d'un index de depart dans le tableau, et on renvoie l'index
	// suivant le groupe en court en code retour de la methode
	// Memoire: le dictionnaire en sortie contient des objets KWAttribute references par les paires, s'ils sont dans
	// un bloc
	int SearchNextGroupOfAttributePairs(const ObjectArray* oaAttributePairs, int nFirstIndex,
					    ObjectDictionary* odFirstBlockAttributes) const;

	///////////////////////////////////////////////////////////
	// Parametres partages par le maitre et les esclaves

	// Acces aux statistiques univariees, utilisees pour le calcul d'au moins une paire
	PLShared_ObjectDictionary* shared_odAttributeStats;

	// Nombre d'instances par chunk, pour parametrer correctement les DataTableSliceSet de chaque esclave
	PLShared_IntVector shared_ivDataTableSliceSetChunkInstanceNumbers;

	//////////////////////////////////////////////////////
	// Parametre en entree et sortie des esclaves
	// Chaque esclave traite un sous-ensemble des paires d'attributs,
	// stockes dans une ou deux tranche de la base

	// Ensemble des paires d'attribut a evaluer dans un esclave,
	// sous la forme de deux liste de noms de variables
	PLShared_StringVector input_svAttributePairNames1;
	PLShared_StringVector input_svAttributePairNames2;

	// Liste des attributs impliquees dans les paires
	PLShared_StringVector input_svAttributeNames;

	// Tranches de la base contenant les attributs impliquees dans les paires
	PLShared_ObjectArray* input_oaDataTableSlices;

	// Preparation des attributs d'une tranche de base
	// En sortie, les paires specifee en entree sont completees avec leur analyse
	PLShared_ObjectArray* output_oaAttributPairStats;

	//////////////////////////////////////////////////////
	// Variables de l'esclave

	//////////////////////////////////////////////////////
	// Variables du Master

	// Specification des pretraitements bivaries et acces aux statistiques univariees
	KWClassStats* masterClassStats;

	// Ensemble de toutes preparations univariee en entree de la tache
	ObjectArray* oaMasterInputAttributeStats;

	// Ensemble des resultats de preparations bivariees pour toutes les paires d'attribut analysees
	// Ces resultats seront rendu directement a l'appelant de la tache
	ObjectArray* oaMasterOutputAttributePairStats;

	//////////////////////////////////////////////////////
	// Variable du Master pour le pilotage des esclaves
	// Ces varables sont calculees avant le dimensionnement et le lancement de la tache

	// Specification de l'ensemble des paires d'attributs a analyser
	ObjectArray oaInputAttributePairStats;

	// Specification de l'ensemble des KWAttributePairsSlices, chacune contenant la specification
	// du travail d'un esclave
	ObjectArray oaInputAttributePairsSlices;
};

////////////////////////////////////////////////////////////////////
// Classe KWAttributePairsSlices
// Specification d'un ensemble de paires d'attributs, de leurs attributs, et des
// slices de KWDataTableSliceSet les contenant
// Memoire: le contenu des specifications appartient a l'appelant
class KWAttributePairsSlices : public Object
{
public:
	// Constructeur
	KWAttributePairsSlices();
	~KWAttributePairsSlices();

	////////////////////////////////////////////////
	// Parametrage du contenu

	// Parametrage des paires d'attributs (KWAttributePairStats)
	ObjectArray* GetAttributePairStats();

	// Parametrage des attributs des paires (KWAttribute)
	ObjectArray* GetAttributes();

	// Parametrage de slices contenant les attributs (KWDataTableSlice)
	ObjectArray* GetSlices();

	//////////////////////////////////////////////////////////////
	// Methodes avancees

	// Ajout du contenu d'un autre ensemble de paires
	void AddAttributePairs(const KWAttributePairsSlices* otherAttributePairsSlices);

	// Comparaison selon les slices utilisees
	int CompareSlices(const KWAttributePairsSlices* otherAttributePairsSlices) const;

	////////////////////////////////////////////////////////////////////////////////
	// Variable de travail utilisable depuis d'autres classes pour personnaliser des
	// criteres de tri lexicographique

	// Vecteur de critere pour un tri lexocographique
	// La gestion de la taille et des valeur de ce vecteur est entierement a la cahrge de l'appelant
	DoubleVector* GetLexicographicSortCriterion();

	// Methode de comparaison de deux tranches selon leur critere lexicographique
	int CompareLexicographicSortCriterion(const KWAttributePairsSlices* otherAttributePairsSlices) const;

	// Affichage des valeurs de tri d'une tranche sous forme d'une ligne
	void DisplayLexicographicSortCriterionHeaderLineReport(ostream& ost, const ALString& sSortCriterion) const;
	void DisplayLexicographicSortCriterionLineReport(ostream& ost) const;

	//////////////////////////////////////////////////////////////
	// Methodes standard

	// Verification de l'integrite
	// Les paires, attribut et slices doivent etre toutes distinctes
	// Les attributs doivent etre dans l'ordre de leurs nom
	// Les tranches doivent etre dans l'ordre de leur nom de classe
	boolean Check() const override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Methode utilitaire de fusion du contenu de deux tableaux, suppose tries de la meme facon, en
	// produisant un tableau resultat trie avec elimination des doublons
	// On suppose que chaque tableau initial est trie, sans doublons
	void MergeArrayContent(const ObjectArray* oaFirst, const ObjectArray* oaSecond, CompareFunction fCompare,
			       ObjectArray* oaMergedResult) const;

	// Attributs de la classe
	ObjectArray oaAttributePairStats;
	ObjectArray oaAttributes;
	ObjectArray oaSlices;

	// Critere de tri lexicographique
	DoubleVector dvLexicographicSortCriterion;
};

// Methode de comparaison sur les tranches (cf KWAttributePairsSlices::CompareSlices)
int KWAttributePairsSlicesCompareSlices(const void* elem1, const void* elem2);

// Methode de comparaison basee sur le critere de tri lexicographique
int KWAttributePairsSlicesCompareLexicographicSortCriterion(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////////////
// Classe KWAttributePair
// Specification d'une pair d'attributs
// Class interne pour l'implementation de l'analyse bivariee
// Memoire: le contenu des specifications appartient a l'appelant
class KWAttributePair : public Object
{
public:
	// Constructeur
	KWAttributePair();
	~KWAttributePair();

	// Index de la paire, dans la liste des paires a analyser
	void SetIndex(int nValue);
	int GetIndex() const;

	// Premier attribut de la paire
	void SetAttribute1(KWAttribute* attribute);
	KWAttribute* GetAttribute1() const;

	// Second attribut de la paire
	void SetAttribute2(KWAttribute* attribute);
	KWAttribute* GetAttribute2() const;

	// Acces aux blocs des deux attributs dans l'ordre des blocs
	// (donc pas forecemnt dans l'ordre des attributs)
	const KWAttributeBlock* GetFirstBlock() const;
	const KWAttributeBlock* GetSecondBlock() const;

	// Acces aux attributs du premier et du second bloc
	KWAttribute* GetFirstBlockAttribute() const;
	KWAttribute* GetSecondBlockAttribute() const;

	// Comparaison avec une autre paire, vis a vis de leurs blocs
	// Les paires sont comparees selon les criteres hierarchiques suivant
	//   . premier bloc (taille decroissante, puis nom)
	//   . second bloc (taille decroissante, puis nom)
	//   . attribut du second bloc
	//   . attribut du premier bloc
	// L'utilisation des trois premier criteres permet, apres tri, d'identifier
	// des groupes de paires de variables ayant meme premier bloc et meme attribut du
	// second groupe, en maximisant la taille du premier bloc.
	// On peut alors faire appel a la class KWTupleTableLoader en chargeant les
	// table de tuples par groupes pour beneficier de la represenattion sparse
	// des attribut, dans le cas de l'analyse bivariee
	// L'ensemble des 4 criteres assure la reproductibilite des traitement
	int CompareBlocks(const KWAttributePair* otherAttributePair);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	int nIndex;
	KWAttribute* attribute1;
	KWAttribute* attribute2;
};

// Methode de comparaison sur les blocs (cf KWAttributePairs::CompareBlocks)
int KWAttributePairCompareBlocks(const void* elem1, const void* elem2);
