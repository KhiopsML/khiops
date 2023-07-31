// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class DTGrouperMODL;
class DTGrouperMODLBasic;

#include "KWGrouper.h"
#include "KWGrouperMODL.h"
#include "KWStat.h"
#include "KWFrequencyVector.h"
#include "DTUnivariatePartitionCost.h"
#include "DTGrouperMODLInternal.h"
#include "SortedList.h"
#include "TaskProgression.h"

//////////////////////////////////////////////////////////////////////////////////
// Algorithme MODL de fusion des lignes quelconque d'une table de contingence
class DTGrouperMODL : public KWGrouperMODLFamily
{
public:
	// Constructeur
	DTGrouperMODL();
	~DTGrouperMODL();

	// Nom de l'algorithme
	const ALString GetName() const override;

	// Constructeur generique
	KWGrouper* Create() const override;

	// Parametrage de la structure des couts de la grille de donnees
	// Par defaut: KWDataGridCosts (avec couts a 0)
	// Memoire: l'objet appartient a l'appele (le Set remplace et detruit le parametre precedent)
	void SetGroupingCosts(KWUnivariatePartitionCosts* kwupcCosts);
	KWUnivariatePartitionCosts* GetGroupingCosts() const override;

	//////////////////////////////////////////////////////////////////////////////
	// Methode de calcul de cout d'un groupage (cout MODL)

	// Cout de codage d'un groupage (definie par sa table d'effectifs
	// et le nombre de valeurs descriptives groupees)
	// Dans le cas d'une table associee a une granularite > 0, le nombre de valeurs descriptives groupees
	// est le nombre de groupes de la table initiale apres granularisation (modalites en effectif suffisant
	// pour rester en singletons + groupe fourre-tout)
	double ComputeGroupingCost(KWFrequencyTable* kwftGroupedTable, int nInitialModalityNumber) const override;
	double ComputeGroupingModelCost(KWFrequencyTable* kwftGroupedTable, int nInitialModalityNumber) const override;
	double ComputeGroupingConstructionCost(KWFrequencyTable* kwftGroupedTable,
					       int nInitialModalityNumber) const override;
	double ComputeGroupingPreparationCost(KWFrequencyTable* kwftGroupedTable,
					      int nInitialModalityNumber) const override;
	double ComputeGroupingDataCost(KWFrequencyTable* kwftGroupedTable, int nInitialModalityNumber) const override;

	// Verification des parametres
	boolean Check() const override;

	// Test
	static void TestGranularizeFrequencyTable();

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class KWDataGridOptimizer;
	friend class KWDataGridManager;

	/////////////////////////////////////////////////////////////////////
	// Cout de la partition en groupes
	// Methodes de cout redirigee sur l'objet GroupingCost
	// (cf classe DTUnivariatePartitionCost)

	// Cout de decoupage en groupes
	// Ajout du nombre de modalites du groupe poubelle en parametre
	// nGroupNumber est le total de groupes (groupe poubelle inclus)
	double ComputePartitionCost(int nGroupNumber, int GarbageModalityNumber) const;

	// Calcul de la variation de cout de partition suite a decrementation du nombre de groupes
	// Ajout du nombre de modalites du groupe poubelle en paramettre
	// A utiliser uniquement si le groupe poubelle eventuel n'est pas affectee par cette decrementation
	double ComputePartitionDeltaCost(int nGroupNumber, int nGarbageModalityNumber) const;

	// Calcul du cout local d'un groupe
	double ComputeGroupCost(const KWFrequencyVector* groupFrequencyVector) const;

	// Calcul du cout global de la partition, definie par le tableau de ses parties
	// On accede via le tableau a la granularite de la partition et au nombre de modalites du groupe poubelle
	// eventuel
	double ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const;

	// Nombre de valeurs initiales
	int GetInitialValueNumber() const;

	//////////////////////////////////////////////////////////////////////////////
	// Partition d'une table d'effectifs selon une granularite N_G.
	// La table fournie en entree doit etre triee par frequence decroissante.
	// Pour la granularite 0, la table en sortie est identique a la table fournie en entree (singletons conserves)
	// Pour une granularite G de 1 a GMax, partition en au plus N_G parties
	// - on cree des parties singletons pour les modalites dont l'effectif est superieur a N/N_G
	// - la partie "fourre-tout" contient l'ensemble des modalites dont l'effectif est inf a N/N_G
	// Les modalites-singletons sont mises dans le fourre-tout pour toute granularite G >= 1
	void GranularizeFrequencyTable(KWFrequencyTable* kwctSource, KWFrequencyTable*& kwctTarget, int nGranularity,
				       KWQuantileGroupBuilder* quantileBuilder) const;

	//////////////////////////////////////////////////////////////////
	// Methodes pour le groupage dans des cas speciaux

	// Groupage des modalites pour une table d'effectifs preprocessee
	// Modification de l'algorithme pour parcourir les differentes granularites et inserer eventuellement un groupe
	// poubelle Reimplementation d'une methode virtuellle de KWGrouper
	// CH V9 TODO Renommer et modifier description de la methode :
	// avec la granularite, il n'y a plus de preprocessing manuel donc cette methode effectue un
	// groupage sur la table initiale par parcours des granularites
	void GroupPreprocessedTable(KWFrequencyTable* kwctSource, KWFrequencyTable*& kwctTarget,
				    IntVector*& ivGroups) const override;

	// Groupage des modalites pour une table d'effectifs
	void GroupFrequencyTable(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget,
				 IntVector*& ivGroups) const;

	// Cas particulier ou il y a tres peu de valeurs a grouper
	// Dans ce cas, on implemente la solution optimale "en dur" sans
	// utiliser les structures algorithmiques, a fins d'optimisation
	// Seuil: moins de trois valeurs sources a grouper
	void SmallSourceNumberGroup(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget,
				    IntVector*& ivGroups) const;

	// Groupage dans le cas ou il y a deux classes cibles
	void TwoClassesGroup(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget, IntVector*& ivGroups) const;

	//////////////////////////////////////////////////////////////////
	// Algorithme de groupage dans le cas ou il y a plusieurs classe
	// cibles
	// On part de toutes les valeurs initiales constituant des groupes
	// elementaires, et on envisage toutes les fusions de groupes possible.
	// En bufferisant les calculs, et en memorisant les fusions par ordre
	// decroissant d'interet dans une liste triee, on peut garantir une
	// complexite algorithmique en O(n^2.log(n))

	// Methode de pilotage de l'algorithme dans le cas a plusieurs classes
	void MultipleClassesGroup(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget,
				  IntVector*& ivGroups) const;

	// Methode de pilotage de l'algorithme dans le cas a plusieurs classes
	// Recherche d'une partition sans et avec poubelle
	void MultipleClassesGroupWithGarbageSearch(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget,
						   IntVector*& ivGroups) const;

	// Algorithme principal d'optimisation du groupage
	// Le vecteur ivGroups contiendra l'index des groupes terminaux pour
	// chaque groupe initial
	// Les groupes et fusions initiales sont geres par l'appele, et
	// detruits par la methode (pas leur container).
	// On peut demander un nombre exact de groupes (inferieur au MaxGroupNumber et
	// au nombre de groupes initial et superieur a 1), ou laisser le choix a la
	// methode en mettant le parametre ExactGroupNumber a 0.
	// En sortie, on obtient le nombre de groupes correspondant rencontre
	// d'evalaution optimale
	int OptimizeGroups(int nExactGroupNumber, ObjectArray* oaInitialGroups, ObjectArray* oaInitialGroupMerges,
			   ObjectArray*& oaNewGroups, IntVector*& ivGroups) const;

	// Algorithme inspire de celui d'OptimizeGroups
	// Les groupes et fusions initiales sont geres par l'appele.
	// Les fusions initiales sont detruites par la methode (pas leur container).
	// Les groupes initiaux NE sont PAS detruits par la methode contrairement a la methode OptimizeGroups d'origine
	// (le nombre de modalites par groupe n'est donc pas perdu)
	// On suit en plus l'evolution du cout de la partition considerant que le groupe contenant
	// le plus de modalites est le groupe poubelle
	// Si nExactGroupNumber = 0, arret des merges des qu'il y a degradation du cout de la partition SANS ou AVEC
	// poubelle Sinon, les merges sont effectues jusqu'a nExactGroupNumber En sortie, on obtient le nombre de
	// groupes correspondant rencontre d'evaluation optimale SANS puis AVEC poubelle
	IntVector* OptimizeGroupsWithGarbageSearch(int nExactGroupNumber, ObjectArray*& oaInitialGroups,
						   ObjectArray* oaInitialGroupMerges, ObjectArray*& oaNewGroups,
						   IntVector*& ivGroups) const;

	// Acces aux fusions de groupe par index
	KWMODLGroupMerge* GetGroupMergeAt(ObjectArray* oaGroupMerge, int nGroupNumber, int nIndex1, int nIndex2) const;
	void SetGroupMergeAt(ObjectArray* oaGroupMerge, int nGroupNumber, int nIndex1, int nIndex2,
			     KWMODLGroupMerge* groupMerge) const;

	/////////////////////////////////////////////////////////////////////////
	// Methode de preprocessing visant a reduire le nombre de valeurs
	// initiale pour diminuer la complexite algorithmique du groupage

	// Preprocessing des valeurs initiales, consistant a constituer des groupes
	// purs (avec une seule classe cible) ayant la meme classe cible
	// Le vecteur ivGroups contiendra l'index des groupes preprocesses pour
	// chaque valeur initiale
	// On construit une nouvelle table d'effectifs, qui sera utilisable
	// dans les post-optimisations (on peut deplacer toutes les modalites
	// d'un meme groupe pure directement, en restant compatble avec
	// la solution optimale)
	void MergePureSourceValues(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftPreprocessedSource,
				   IntVector*& ivGroups) const;

	// Preprocessing consistant a rechercher des sous-groupes partiels stable pour
	// tous les groupages bi-classes (une classe cible contre toutes les autres).
	// Ces groupages sont recherches sans cout de partition, de facon a partir
	// d'une solution initiale plus fine
	// Tous les groupages bi-classes sont effectues, et les sous-groupes presents
	// dans tous ces groupages sont identifies.
	ObjectArray* BuildReliableSubGroups(KWFrequencyTable* kwftSource, IntVector*& ivGroups) const;

	// Construction d'une cle d'encodage d'un vecteur d'index. Permet d'identifier
	// les sous-groupes stables, qui sont affectes aux memes groupes pour tous
	// les groupages bi-classes.
	// On passe d'abord le vecteur des valeurs max des index, permettant de
	// parametrer/optimiser l'encodage du vecteur d'index
	const ALString& BuildKeyFromIndexes(IntVector* ivIndexMaxValues, IntVector* ivIndexValues) const;

	// Construction de groupes a partir de la table d'effectifs initiale
	// Chaque groupe construit est un objet KWMODLGroup, entierement initialise
	// Memoire: le tableau et son contenu appartiennent a l'appelant
	ObjectArray* BuildGroups(KWFrequencyTable* kwftTable) const;

	// Construction d'une table d'effectifs a partir de groupes
	// Memoire: la table d'effectifs retournee appartiennent a l'appelant
	KWFrequencyTable* BuildTable(ObjectArray* oaGroups) const;

	// Construction du tableau carre des fusions possibles de groupes
	// Le tableau rendu est carre (pour en faciliter l'indexation), mais seule sa
	// diagonale inferieure contient des objets KWMODLGroupMerge representant les
	// fusions de groupes
	// Memoire: le tableau et son contenu appartiennent a l'appelant
	ObjectArray* BuildGroupMerges(ObjectArray* oaGroups) const;

	// Groupage majoritaire des valeurs descriptives peu frequentes
	// On cree au plus un groupe par classe cible pour agglomerer les valeurs
	// descriptives d'effectif strictement inferieur ou egal a un seuil donne
	// en parametre.
	// Si le parametre bOneSingleGarbageGroup est a true, il n'y aura qu'un seul
	// groupe pour les valeurs peu frequentes.
	// Le vecteur ivGroups contiendra l'index des groupes preprocesses pour
	// chaque groupe initial
	// Les groupes initiaux sont transformes apres processing, et le
	// tableau des groupes est retaille si des groupes ont disparus par fusion
	// (ils sont alors correctement detruits par la methode)
	void MergeSmallGroups(ObjectArray* oaInitialGroups, int nMinFrequency, boolean bOneSingleGarbageGroup,
			      IntVector*& ivGroups) const;

	// Calcul du seuil de frequence minimum des groupes a fusionner pour obtenir
	// au final un nombre limite de groupes
	int ComputeMinGroupFrequency(ObjectArray* oaInitialGroups, int nTotalFrequency,
				     int nOutputMaxGroupNumber) const;

	// En entree : les vecteurs de correspondance associes aux pretraitements envisages, NULL si pas de
	// pretraitement En sortie : le vecteur de correspondance entre la table initiale et la table avant groupage qui
	// reprend tous les pretraitements de regroupement de modalites
	//				Destruction a la charge de l'appelant
	void ComputeIndexesFromSourceToEndProcesses(IntVector*& ivSourceToPreprocessedIndexes,
						    IntVector*& ivPreprocessedToSubGroupsIndexes,
						    IntVector*& ivFewerInitialIndexes,
						    IntVector*& ivSourceToEndPreprocesses) const;

	/////////////////////////////////////////////////////////////////////////////
	// Algorithmes de post-optimisation des groupes
	// La table source ne contient pas forcement les valeurs initiales, mais des
	// des sous-groupes inseccables a passer globalement d'un groupe a un autre.
	// En entree, une solution de groupage initiale est presentee avec affectation
	// de ces sous-groupes dans les groupes de la table cible.
	// En sortie, la table d'effectifs finale est modifiee, ainsi que le vecteur
	// d'affectation des groupes

	// Post-optimisation du groupage, en effectuant une succession
	// de merge force suivi de post-optimisation (a nombre de groupes egal),
	// et en retenant la meilleure solution
	void PostOptimizeGrouping(KWFrequencyTable* kwftSource, KWFrequencyTable* kwftTarget,
				  IntVector* ivGroups) const;

	// Post-optimisation effectuee sur les tableaux de KWMODLGroup* plutot que sur les tables de contingence
	// afin d'avoir acces au nombre de modalites par groupe
	void PostOptimizeGroupingWithGarbageSearch(ObjectArray* oaInitialGroups, ObjectArray*& oaPostOptimizedGroups,
						   IntVector* ivPostOptimizedGroups, KWFrequencyTable*& kwftSource,
						   KWFrequencyTable*& kwftTarget) const;

	// Algorithme ExhaustiveMerge
	// Cette heuristique consiste a forcer les merges jusqu'a obtenir le nombre
	// minimum de groupe desire, puis a retenir le meilleur groupage rencontre
	void ExhaustiveMergeGrouping(int nMinGroupNumber, KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget,
				     IntVector*& ivGroups) const;

	// Idem ExhaustiveMergeGrouping avec en entree sortie les tables de KWMODLGroup* plutot que les tables de
	// contingence afin de prendre en compte et de mettre a jour le nombre de modalites par KWMODLGroup* En sortie,
	// meilleure partition SANS et AVEC poubelle
	void ExhaustiveMergeGroupingWithGarbageSearch(int nMinGroupNumber, ObjectArray* oaSource,
						      ObjectArray*& oaTarget, IntVector*& ivGroups,
						      ObjectArray*& oaTargetWithGarbage,
						      IntVector*& ivGroupsWithGarbage) const;

	// Post-optimisation des groupes
	// A nombre de groupes constant, on essaye les changements d'affectation
	// des valeurs descriptives vers de nouveaux groupes, tant qu'il y a amelioration
	// du critere d'evaluation du groupage
	void PostOptimizeGroups(KWFrequencyTable* kwftSource, KWFrequencyTable* kwftTarget, IntVector* ivGroups) const;

	// Effectue les memes changement d'affectation que dans PostOptimizeGroups
	// en mettant a jour le nombre de modalites par groupe.
	// Les objets manipules ne sont pas des tables de contingence mais des tableaux
	// de KWMODLGroup pour maintenir le nombre de modalites par groupe
	void PostOptimizeGroupsWithGarbageSearch(ObjectArray* oaInitialGroups, ObjectArray* oaNewGroups,
						 IntVector* ivGroups, SortedList* frequencyList) const;

	// Post-optimisation rapide des groupes
	// On effectue les changements des que amelioration, au lieu de recherche le meilleur
	// la meilleure amelioration a chaque changement
	// Parametre: nombre max de passes d'ameliorations
	void FastPostOptimizeGroups(KWFrequencyTable* kwftSource, KWFrequencyTable* kwftTarget, IntVector* ivGroups,
				    int nMaxStepNumber) const;
	void FastPostOptimizeGroupsWithGarbage(KWFrequencyTable* kwftSource, KWFrequencyTable* kwftTarget,
					       IntVector* ivGroups, int nMaxStepNumber,
					       SortedList* frequencyList) const;

	// On force le meilleur merge parmi les groupes deja constitues
	// Une nouvelle table est constituee en sortie, avec un groupe en moins
	// Le vecteur d'affectation des groupes correspond alors a la nouvelle table
	void ForceBestGroupMerge(KWFrequencyTable* kwftSource, KWFrequencyTable* kwftTarget,
				 KWFrequencyTable*& kwftNewTarget, IntVector* ivGroups) const;

	// Idem methode ForceBestGroupMerge avec manipulation de tableaux de KWMODLGroup afin
	// de suivre le nombre de modalites par groupe au cours des merges
	void ForceBestGroupMergeWithGarbageSearch(ObjectArray* oaSource, ObjectArray* oaTarget,
						  ObjectArray*& oaNewTarget, IntVector* ivGroups,
						  SortedList* frequencyList) const;

	// Groupage a un seul groupe: ce groupage particulier devra toujours etre
	// compare avec les meilleurs groupages issus des heuristiques
	void BuildSingletonGrouping(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget) const;

	//////////////////////////////////////////////////////////////////////////
	// Gestion des classes de travail pour les fusions et coupures de groupes
	// Les methodes sont implementees avec des vecteurs pleins (KWDenseFrequencyVector),
	// mais sont redefinissable pour s'adapter a des vecteur creux par exemple

	// Initialisation des variables de travail (notament, parametrage de la structure des couts)
	virtual void InitializeWorkingData(const KWFrequencyTable* kwftSource, int nInitialValueNumber) const;
	virtual void CleanWorkingData() const;

	// Acces a l'objet createur d'instance de vecteur d'effectif
	const KWFrequencyVector* GetFrequencyVectorCreator() const;

	// Initialisation du bon nombre de classe d'un vecteur d'effectifs
	virtual void InitializeFrequencyVector(KWFrequencyVector* kwfvFrequencyVector) const;

	// Verification de l'integrite d'un vecteur d'effectifs
	virtual boolean CheckFrequencyVector(const KWFrequencyVector* kwfvFrequencyVector) const;

	// Ajout d'un groupe source dans un groupe cible
	//  Input: groupes source et operande
	//  Output: groupe source mis a jour
	virtual void AddFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
					const KWFrequencyVector* kwfvAddedFrequencyVector) const;

	// Suppression d'un groupe source dans un groupe cible
	//  Input: groupes source et operande
	//  Output: groupe source mis a jour
	virtual void RemoveFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
					   const KWFrequencyVector* kwfvRemovedFrequencyVector) const;

	// Fusion de deux groupes
	//  Input: groupe source et deux groupes a merger
	//  Output: groupe source, fusion des deux autres
	virtual void MergeFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector,
					   const KWFrequencyVector* kwfvMergedFrequencyVector1,
					   const KWFrequencyVector* kwfvMergedFrequencyVector2) const;

	// Calcul du cout de l'union ou de la difference de deux groupes
	// Permet d'optimiser les calculs dans les reimplementations
	virtual double ComputeGroupUnionCost(const KWFrequencyVector* sourceGroup1,
					     const KWFrequencyVector* sourceGroup2) const;
	virtual double ComputeGroupDiffCost(const KWFrequencyVector* sourceGroup,
					    const KWFrequencyVector* removedGroup) const;

	// Donnee de travail pour la methode BuildKeyFromIndexes
	mutable ALString sKey;

	// Couts de partitionnement pour le groupement de valeur
	KWUnivariatePartitionCosts* groupingCosts;

	// Epsilon pour gerer le probleme de precision numerique
	static double dEpsilon;
};

//////////////////////////////////////////////////////////////////////////////////
// Algorithme de groupage MODL basique, en optimisant uniquement
// la taille du groupe "fourre-tout" contenant les modalites de faible effectif
class DTGrouperMODLBasic : public KWGrouper
{
public:
	// Constructeur
	DTGrouperMODLBasic();
	~DTGrouperMODLBasic();

	// Nom de l'algorithme
	const ALString GetName() const override;

	// Constructeur generique
	KWGrouper* Create() const override;

	// Calcul de la loi agregee pour des regroupements de lignes
	// Cette methode gere egalement la contrainte du nombre max de groupes et
	// de l'effectif minimum par groupe
	void Group(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget, IntVector*& ivGroups) const override;

	// Verification des parametres
	boolean Check() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	//////////////////////////////////////////////////////
	// Redefinition de methodes de la classe mere

	// Cout du choix de la taille min des modalites source du groupe fourre-tout
	double ComputeGarbageCost(int nGarbageMinSize) const;

	// Cout du choix du nombre de groupes
	double ComputeModelCost(int nGroupNumber, int nValueNumber) const;

	// Calcul du cout d'un tableau d'effectifs (exceptions au modele)
	double ComputeGroupCost(IntVector* ivFrequencyVector) const;

	// Epsilon pour gerer le probleme de precision numerique
	static double dEpsilon;
};

///////////////////////////////////////////////////////////////////////
// Methodes en inline

inline KWMODLGroupMerge* DTGrouperMODL::GetGroupMergeAt(ObjectArray* oaGroupMerge, int nGroupNumber, int nIndex1,
							int nIndex2) const
{
	require(oaGroupMerge != NULL);
	require(nGroupNumber >= 0);
	require(oaGroupMerge->GetSize() == nGroupNumber * nGroupNumber);
	require(0 <= nIndex1 and nIndex1 < nGroupNumber);
	require(0 <= nIndex2 and nIndex2 < nGroupNumber);
	require(nIndex1 != nIndex2);

	if (nIndex1 < nIndex2)
		return cast(KWMODLGroupMerge*, oaGroupMerge->GetAt(nIndex1 * nGroupNumber + nIndex2));
	else
		return cast(KWMODLGroupMerge*, oaGroupMerge->GetAt(nIndex2 * nGroupNumber + nIndex1));
}

inline void DTGrouperMODL::SetGroupMergeAt(ObjectArray* oaGroupMerge, int nGroupNumber, int nIndex1, int nIndex2,
					   KWMODLGroupMerge* groupMerge) const
{
	require(oaGroupMerge != NULL);
	require(nGroupNumber >= 0);
	require(oaGroupMerge->GetSize() == nGroupNumber * nGroupNumber);
	require(0 <= nIndex1 and nIndex1 < nGroupNumber);
	require(0 <= nIndex2 and nIndex2 < nGroupNumber);
	require(nIndex1 != nIndex2);

	if (nIndex1 < nIndex2)
		oaGroupMerge->SetAt(nIndex1 * nGroupNumber + nIndex2, groupMerge);
	else
		oaGroupMerge->SetAt(nIndex2 * nGroupNumber + nIndex1, groupMerge);
}

inline const KWFrequencyVector* DTGrouperMODL::GetFrequencyVectorCreator() const
{
	return groupingCosts->GetFrequencyVectorCreator();
}

inline double DTGrouperMODL::ComputePartitionCost(int nGroupNumber, int nGarbageModalityNumber) const
{
	return groupingCosts->ComputePartitionCost(nGroupNumber, nGarbageModalityNumber);
}

inline double DTGrouperMODL::ComputePartitionDeltaCost(int nGroupNumber, int nGarbageModalityNumber) const
{
	return groupingCosts->ComputePartitionDeltaCost(nGroupNumber, nGarbageModalityNumber);
}

inline double DTGrouperMODL::ComputeGroupCost(const KWFrequencyVector* groupFrequencyVector) const
{
	return groupingCosts->ComputePartCost(groupFrequencyVector);
}

inline double DTGrouperMODL::ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const
{
	return groupingCosts->ComputePartitionGlobalCost(partTable);
}

inline int DTGrouperMODL::GetInitialValueNumber() const
{
	ensure(groupingCosts->GetValueNumber() > 0);
	return groupingCosts->GetValueNumber();
}
