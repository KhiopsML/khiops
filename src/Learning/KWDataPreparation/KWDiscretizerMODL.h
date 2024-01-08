// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDiscretizerMODLFamily;
class KWMODLHistogramResults;
class KWDiscretizerMODL;

#include "KWDiscretizer.h"
#include "KWDiscretizerMODLLine.h"
#include "KWFrequencyVector.h"
#include "KWUnivariatePartitionCost.h"
#include "SortedList.h"
#include "TaskProgression.h"
#include "Timer.h"
#include "KWQuantileBuilder.h"

//////////////////////////////////////////////////////////////////////////////////
// Discretiseur de la famille MODL, avec gestion des cout de discretisation
class KWDiscretizerMODLFamily : public KWDiscretizer
{
public:
	// Indique que l'on fait partie de la famille MODL
	boolean IsMODLFamily() const override;

	// Cout de codage d'une discretisation (definie par sa table de contingence)
	virtual KWUnivariatePartitionCosts* GetDiscretizationCosts() const = 0;
	virtual double ComputeDiscretizationCost(KWFrequencyTable* kwftDiscretizedTable) const = 0;
	virtual double ComputeDiscretizationModelCost(KWFrequencyTable* kwftDiscretizedTable) const = 0;
	virtual double ComputeDiscretizationConstructionCost(KWFrequencyTable* kwftDiscretizedTable) const = 0;
	virtual double ComputeDiscretizationPreparationCost(KWFrequencyTable* kwftDiscretizedTable) const = 0;
	virtual double ComputeDiscretizationDataCost(KWFrequencyTable* kwftDiscretizedTable) const = 0;

	////////////////////////////////////////////////////////////////////////
	// Gestion des resultats de discretisation non supervise MODL

	// Construction des resultats de discretisation non supervise MODL
	// Par defaut: renvoie NULL
	// Memoire: l'objet retourne appartient a l'appelant
	virtual KWMODLHistogramResults* BuildMODLHistogramResults() const;

	// Creation generique d'une objet de resultats de discretisation non supervise MODL
	// Par defaut: renvoie NULL
	// Memoire: l'objet retourne appartient a l'appelant
	virtual KWMODLHistogramResults* CreateMODLHistogramResults() const;

	// Acces a un objet permettant de gerer la serialisation des resultats de discretisation
	// Par defaut: renvoie NULL
	// Memoire: l'objet retourne appartient a l'appele
	virtual const PLSharedObject* GetMODLHistogramResultsSharedObject() const;
};

/////////////////////////////////////////////////////////////////////////////////
// Specification generique des resultats de discretisation non supervise MODL
// Comme il ne peut y avoir de dependance cyclique entre bibliotheque, cette classe
// virtuelle permet de definir les quelques services de base utilisable de facon generique
// dans la librairie courante, tout en implementant les histogrammes MODL
// dans une librairie fille, pour modulariser les developpement
class KWMODLHistogramResults : public Object
{
public:
	// Bornes des histogrammes, qui ne sont pas necessairement les valeur min et max du jeux de donnees
	virtual Continuous GetDomainLowerBound() const = 0;
	virtual Continuous GetDomainUpperBound() const = 0;

	// Ecriture d'un rapport JSON
	virtual void WriteJSONKeyReport(JSONFile* fJSON, const ALString& sKey) = 0;

	// Nom du discretiseur a l'origine des results
	virtual const ALString GetDiscretizerName() const = 0;
};

//////////////////////////////////////////////////////////////////////////////////
// Algorithme MODL de fusion des lignes adjacentes d'une table de contingence
// Optimisation des discretisation pour l'a priori a trois etages
// Prise en compte de l'effectif minimum par intervalle dans le critere
class KWDiscretizerMODL : public KWDiscretizerMODLFamily
{
public:
	// Constructeur
	KWDiscretizerMODL();
	~KWDiscretizerMODL();

	// Nom de l'algorithme
	const ALString GetName() const override;

	// Constructeur generique
	KWDiscretizer* Create() const override;

	// Parametrage de la structure des couts de la grille de donnees
	// Par defaut: KWDataGridCosts (avec couts a 0)
	// Memoire: l'objet appartient a l'appele (le Set remplace et detruit le parametre precedent)
	void SetDiscretizationCosts(KWUnivariatePartitionCosts* kwupcCosts);
	KWUnivariatePartitionCosts* GetDiscretizationCosts() const override;

	// Parametrage de l'algorithme utilise (parametre des methodes Set/GetParam)
	// Par defaut: OptimizedGreedyMerge
	enum
	{
		OptimizedGreedyMerge, // Algorithme glouton ascendant, avec post-optimisation
		GreedyMerge,          // Algorithme glouton ascendant
		OptimizedGreedySplit, // Algorithme glouton descendant, avec post-optimisation
		GreedySplit,          // Algorithme glouton descendant
		Optimal
	}; // Algorithme optimal (attention: en N^3)

	// Calcul de la loi agregee pour des regroupements de lignes adjacentes
	// Renvoie 1-TauxCompression
	void Discretize(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget) const override;

	//////////////////////////////////////////////////////////////////////////////
	// Methode de calcul de cout d'une discretisation (cout MODL)

	// Cout de codage d'une discretisation (definie par sa table de contingence)
	double ComputeDiscretizationCost(KWFrequencyTable* kwftDiscretizedTable) const override;
	double ComputeDiscretizationModelCost(KWFrequencyTable* kwftDiscretizedTable) const override;
	double ComputeDiscretizationConstructionCost(KWFrequencyTable* kwftDiscretizedTable) const override;
	double ComputeDiscretizationPreparationCost(KWFrequencyTable* kwftDiscretizedTable) const override;
	double ComputeDiscretizationDataCost(KWFrequencyTable* kwftDiscretizedTable) const override;

	// Verification des parametres
	boolean Check() const override;

	// Test de la methode de granularisation des tables d'effectifs
	static void TestGranularizeFrequencyTable();

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	friend class KWDataGridManager;

	// Discretisation d'une table granularisee
	void DiscretizeGranularizedFrequencyTable(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget) const;

	// Granularisation d'une table d'effectifs associe a une granularite : en parametre ou attribut interne
	// Pour la granularite maximale, la table en sortie est identique a la table en entree
	void GranularizeFrequencyTable(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget, int nGranularity,
				       KWQuantileIntervalBuilder* quantileBuilder) const;

	// Post-optimisation de la granularite, en recherchant une granularite moindre aboutissant au meme decoupage en
	// intervalles
	void PostOptimizeGranularity(KWFrequencyTable* kwftTarget, KWQuantileIntervalBuilder* quantileBuilder,
				     int nLastExploredGranularity) const;

	// Fusion des intervalles purs de meme classe cible d'une table d'effectifs
	void MergeFrequencyTablePureIntervals(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget) const;

	// Discretisation avec des tables d'effectifs au lieux des table de contingence
	// Renvoie le cout de codage de la discretisation
	void DiscretizeFrequencyTable(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget) const;

	/////////////////////////////////////////////////////////////////////
	// Cout de la partition en intervalles
	// Methodes de cout redirigee sur l'objet DiscretizationCost
	// (cf classe KWUnivariatePartitionCost)

	// Cout de decoupage en intervalles
	double ComputePartitionCost(int nIntervalNumber) const;

	// Calcul de la variation de cout de partition suite a decrementation du nombre d'intervalles
	double ComputePartitionDeltaCost(int nIntervalNumber) const;

	// Cout du decoupage en intervalles en mode poubelle
	// Prise en compte du choix de la presence d'un groupe poubelle meme si la taille du groupe poubelle est nul
	// Ne peut etre utilise que lorsqu'on utilise un discretiseur pour effectuer un groupage (e.g.
	// KWGrouperMODLTwoClasses)
	double ComputePartitionCost(int nIntervalNumber, int nGarbageModalityNumber) const;

	// Cout de la variation en presence d'un groupe poubelle
	// Ne peut etre utilise que :
	// - lorsque l'on utilise un discretiseur pour effectuer un groupage (e.g. KWGrouperMODLTwoClasses)
	// - lorsque la taille du groupe poubelle est constant pendant cette decrementation
	double ComputePartitionDeltaCost(int nIntervalNumber, int nGarbageModalityNumber) const;

	// Calcul du cout local d'un intervalle
	double ComputeIntervalCost(const KWFrequencyVector* intervalFrequencyVector) const;

	// Calcul du cout global de la partition, definie par le tableau de ses parties
	// On accede via la table de frequence aux informations de granularite
	// de la partition et au nombre de modalites du groupe poubelle eventuel
	double ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const;

	///////////////////////////////////////////////////////////////////
	// Gestion de la liste des intervalles
	// Une liste d'intervalle est decrite par sa tete de liste (head).
	// Elle est compose d'elements de liste, sous-classe de la classe
	// KWMODLLine. On peut parametrer la sous-classe a utiliser pour
	// Les operations de construction/duplication sont parametree par
	// un creator, instance de l'element de liste a utiliser de
	// la sous-classe desiree

	// Construction d'une table d'effectifs a partir de la liste d'intervalles
	KWFrequencyTable* BuildFrequencyTableFromIntervalList(KWMODLLine* headLine) const;

	// Construction de la liste des intervalles a partir d'une table de contingence
	KWMODLLine* BuildIntervalListFromFrequencyTable(KWMODLLine* lineCreator,
							const KWFrequencyTable* kwftTable) const;

	// Construction de la liste des intervalles a partir d'une table de contingence et des
	// bornes des intervalles (index des dernieres lignes)
	KWMODLLine*
	BuildIntervalListFromFrequencyTableAndIntervalBounds(KWMODLLine* lineCreator, const KWFrequencyTable* kwftTable,
							     const IntVector* ivIntervalLastLineIndexes) const;

	// Construction d'un intervalle unique, fusion de toutes les lignes
	// d'une table de contingence
	KWMODLLine* BuildUniqueIntervalFromFrequencyTable(KWMODLLine* lineCreator,
							  const KWFrequencyTable* kwftTable) const;

	// Affichage (en mode tableau) de la liste des intervalles
	void WriteIntervalListReport(const KWMODLLine* headLine, ostream& ost) const;

	// Duplication de la liste des intervalles (renvoie la tete de la liste dupliquee)
	// La structure est dupliquee en se basant sur le creator, mais seule les
	// donnee de base (de type KWMODLine) sont transferees
	KWMODLLine* CloneIntervalList(KWMODLLine* lineCreator, KWMODLLine* headLine) const;

	// Cout de partition global calcule a partir d'une liste d'intervalles, dont on passe la tete,
	// et d'un tableau de vecteur d'effectifs (qui n'est utilise que pour initialiser la structure
	// de cout: nombre de classes cibles...)
	double ComputeIntervalListPartitionGlobalCost(KWMODLLine* headLine, const KWFrequencyTable* kwftTable) const;

	// Taille d'une liste d'intervalles, dont on passe la tete
	int GetIntervalListSize(const KWMODLLine* headLine) const;

	// Destruction de la liste des intervalles (NULL possible)
	void DeleteIntervalList(KWMODLLine* headLine) const;

	///////////////////////////////////////////////////////////////////
	// Optimisation de la liste des intervalles avec le critere MODL

	// Optimisation de la liste des intervalles
	// Renvoie la tete de la liste
	KWMODLLine* IntervalListOptimization(const KWFrequencyTable* kwftSource) const;

	// Optimisation basee sur une recherche ascendante menee jusqu'a un seul
	// intervalle terminal, et memorisation de la meilleure solution rencontree
	// La liste (dont la tete est passee en parametre) est modifiee par la methode
	// La liste initiale doit etre initialisee pour la recherche de Merges
	virtual void IntervalListBestMergeOptimization(KWMODLLineOptimization*& headInterval) const;

	// Optimisation basee sur une recherche descendante menee jusqu'aux intervalles
	// elementaires, et memorisation de la meilleure solution rencontree
	// La liste initiale doit etre initialisee pour la recherche de Splits
	virtual void IntervalListBestSplitOptimization(const KWFrequencyTable* kwftSource,
						       KWMODLLineDeepOptimization* headInterval) const;

	// Post-optimisation basee sur une recherche locale des ameliorations
	// baseee sur des MergeSplit, des Split et des MergeMergeSplit
	// La liste (dont la tete est passee en parametre) est modifiee par la methode
	virtual void IntervalListPostOptimization(const KWFrequencyTable* kwftSource,
						  KWMODLLineDeepOptimization*& headInterval) const;

	// Post-optimisation basee sur une recherche locale des ameliorations
	// baseee sur des MergeSplit, c'est a dire uniquement sur des deplacements de frontieres entre intervalles
	// La liste (dont la tete est passee en parametre) est modifiee par la methode
	virtual void IntervalListBoundaryPostOptimization(const KWFrequencyTable* kwftSource,
							  KWMODLLineDeepOptimization*& headInterval) const;

	// Mise a jour des listes triees de Split, MergeSplit et MergeMergeSplit gerant
	// la bufferisation des informations de post-optimisation, en fonction de la
	// provenance de l'amelioration (portee par interval).
	// Cette mise a jour globale est une optimisation complexe des bufferisations en
	// fusionnant les fonctionnalites des methodes UpdateSplitSortedList,
	// UpdateMergeSplitSortedList et UpdateMergeMergeSplitSortedList.
	// Mise a jour simultanement de la liste des intervalles globale
	// (dont la tete de liste peut etre remplacee)
	void UpdatePostOptimizationSortedListsWithSplit(const KWFrequencyTable* kwftSource, SortedList* splitList,
							SortedList* mergeSplitList, SortedList* mergeMergeSplitList,
							KWMODLLineDeepOptimization*& headInterval,
							KWMODLLineDeepOptimization* interval) const;
	void UpdatePostOptimizationSortedListsWithMergeSplit(const KWFrequencyTable* kwftSource, SortedList* splitList,
							     SortedList* mergeSplitList,
							     SortedList* mergeMergeSplitList,
							     KWMODLLineDeepOptimization*& headInterval,
							     KWMODLLineDeepOptimization* interval) const;
	void UpdateBoundaryPostOptimizationSortedListWithMergeSplit(const KWFrequencyTable* kwftSource,
								    SortedList* mergeSplitList,
								    KWMODLLineDeepOptimization*& headInterval,
								    KWMODLLineDeepOptimization* interval) const;
	void UpdatePostOptimizationSortedListsWithMergeMergeSplit(const KWFrequencyTable* kwftSource,
								  SortedList* splitList, SortedList* mergeSplitList,
								  SortedList* mergeMergeSplitList,
								  KWMODLLineDeepOptimization*& headInterval,
								  KWMODLLineDeepOptimization* interval) const;

	// Verification de la coherence des cardinaux des listes d'ameliorations
	boolean CheckPostOptimizationSortedLists(SortedList* splitList, SortedList* mergeSplitList,
						 SortedList* mergeMergeSplitList,
						 KWMODLLineDeepOptimization* headInterval) const;

	////////////////////////////////////////////////////////////////////////////
	// Methodes concernant le suivi du nombre de modalites a reimplementer dans les sous-classes dediees a des
	// groupeurs Les implementation par defaut ne font rien: pas de tel suivi dans le cadre d'un discretiseur

	// Methodes d'ajout/retrait d'un intervalle de la liste de travail triee par nombre de modalites de l'intervalle
	// Par defaut, dans le cas d'un discretiseur cette liste de travail n'est pas utilisee et ces methodes sont
	// vides Elles sont specialisees dans le KWGrouperMODLTwoClasses et sont utilisees pour mettre a jour cette
	// liste de travail en meme temps que les listes triees de Split, MergeSplit et MergeMergeSplit
	virtual void AddIntervalToWorkingFrequencyList(KWMODLLine* interval) const;
	virtual void RemoveIntervalFromWorkingFrequencyList(KWMODLLine* interval) const;

	// Calcul du nombre total de modalites des lignes d'une table de contingence
	// Par defaut dans le cas d'un discretiseur, renvoie 0
	// Specialise dans le cas du KwGrouperMODLTwoClasses
	virtual int ComputeModalityNumber(const KWFrequencyTable* kwftSource, int nFirstIndex, int LastIndex) const;

	////////////////////////////////////////////////////////////////
	// Optimisation basee sur la recherche de Merges

	// Optimisation basee sur une recherche locale des ameliorations de type Merge
	virtual void IntervalListMergeOptimization(KWMODLLineOptimization*& headInterval) const;

	// Calcul d'un merge de deux intervalles
	void ComputeIntervalMerge(KWMODLLineOptimization* interval) const;

	// Initialisation des Merges possibles d'une liste d'intervalles
	void InitializeMergeList(KWMODLLineOptimization* headInterval) const;

	// Rangement des merges possibles dans une liste triee ordonnee par cout decroissant
	void InitializeMergeSortedList(SortedList* mergeList, KWMODLLineOptimization* headInterval) const;

	// Mise a jour de la liste triee des merges en mergeant deux intervalles
	// Mise a jour simultanement de la liste des intervalles globale
	// (dont la tete de liste peut etre remplacee)
	void UpdateMergeSortedList(SortedList* mergeList, KWMODLLineOptimization*& headInterval,
				   KWMODLLineOptimization* interval) const;

	////////////////////////////////////////////////////////////////
	// Optimisation basee sur la recherche de Splits

	// Optimisation basee sur une recherche locale des ameliorations de type Split
	virtual void IntervalListSplitOptimization(const KWFrequencyTable* kwftSource,
						   KWMODLLineDeepOptimization*& headInterval) const;

	// Calcul d'un Split de deux intervalles
	void ComputeIntervalSplit(const KWFrequencyTable* kwftSource, KWMODLLineDeepOptimization* interval) const;

	// Initialisation des Splits possibles d'une liste d'intervalles
	void InitializeSplitList(const KWFrequencyTable* kwftSource, KWMODLLineDeepOptimization* headInterval) const;

	// Rangement des Splits possibles dans une liste triee ordonnee par cout decroissant
	void InitializeSplitSortedList(SortedList* splitList, KWMODLLineDeepOptimization* headInterval) const;

	// Mise a jour de la liste triee des Split en modifiant les intervalles concernes
	// Mise a jour simultanement de la liste des intervalles globale
	// (dont la tete de liste peut etre remplacee)
	void UpdateSplitSortedList(const KWFrequencyTable* kwftSource, SortedList* splitList,
				   KWMODLLineDeepOptimization*& headInterval,
				   KWMODLLineDeepOptimization* interval) const;

	////////////////////////////////////////////////////////////////
	// Optimisation basee sur la recherche de MergeSplits

	// Optimisation basee sur une recherche locale des ameliorations de type MergeSplit
	virtual void IntervalListMergeSplitOptimization(const KWFrequencyTable* kwftSource,
							KWMODLLineDeepOptimization*& headInterval) const;

	// Calcul d'un MergeSplit de deux intervalles
	void ComputeIntervalMergeSplit(const KWFrequencyTable* kwftSource, KWMODLLineDeepOptimization* interval) const;

	// Initialisation des MergeSplits possibles d'une liste d'intervalles
	void InitializeMergeSplitList(const KWFrequencyTable* kwftSource,
				      KWMODLLineDeepOptimization* headInterval) const;

	// Rangement des MergeSplits possibles dans une liste triee ordonnee par cout decroissant
	void InitializeMergeSplitSortedList(SortedList* mergeSplitList, KWMODLLineDeepOptimization* headInterval) const;

	// Mise a jour de la liste triee des MergeSplit en modifiant les intervalles concernes
	// Mise a jour simultanement de la liste des intervalles globale
	// (dont la tete de liste peut etre remplacee)
	void UpdateMergeSplitSortedList(const KWFrequencyTable* kwftSource, SortedList* mergeSplitList,
					KWMODLLineDeepOptimization*& headInterval,
					KWMODLLineDeepOptimization* interval) const;

	////////////////////////////////////////////////////////////////
	// Optimisation basee sur la recherche de MergeMergeSplits

	// Optimisation basee sur une recherche locale des ameliorations de type MergeMergeSplit
	virtual void IntervalListMergeMergeSplitOptimization(const KWFrequencyTable* kwftSource,
							     KWMODLLineDeepOptimization*& headInterval) const;

	// Calcul d'un MergeMergeSplit de deux intervalles
	void ComputeIntervalMergeMergeSplit(const KWFrequencyTable* kwftSource,
					    KWMODLLineDeepOptimization* interval) const;

	// Initialisation des MergeMergeSplits possibles d'une liste d'intervalles
	void InitializeMergeMergeSplitList(const KWFrequencyTable* kwftSource,
					   KWMODLLineDeepOptimization* headInterval) const;

	// Rangement des MergeMergeSplits possibles dans une liste triee ordonnee par cout decroissant
	void InitializeMergeMergeSplitSortedList(SortedList* mergeMergeSplitList,
						 KWMODLLineDeepOptimization* headInterval) const;

	// Mise a jour de la liste triee des MergeMergeSplit en modifiant les intervalles concernes
	// Mise a jour simultanement de la liste des intervalles globale
	// (dont la tete de liste peut etre remplacee)
	void UpdateMergeMergeSplitSortedList(const KWFrequencyTable* kwftSource, SortedList* mergeMergeSplitList,
					     KWMODLLineDeepOptimization*& headInterval,
					     KWMODLLineDeepOptimization* interval) const;

	//////////////////////////////////////////////////////////////////
	// Recherche d'une discretisation optimale en se basant sur un
	// algorithme de programmation dynamique

	// Recherche de la discretisation optimale
	virtual KWMODLLine* ComputeOptimalDiscretization(const KWFrequencyTable* kwftSource) const;

	// Initialisation des discretisations optimales a un seul intervalle
	// Les nombres max d'intervalles par sont initialise afin d'optimiser l'evaluation
	// des discretisation multi-intervalles
	// On renvoie la tete de la liste
	KWMODLLineOptimalDiscretization* InitializeOptimalDiscretizations(const KWFrequencyTable* kwftSource) const;

	// Mise a jour des discretisation optimale en ajoutant un intervalle
	// On envoie true si un intervalle a effectivement pu etre rajoute
	boolean UpdateOptimalDiscretizations(const KWFrequencyTable* kwftSource,
					     KWMODLLineOptimalDiscretization* headInterval) const;

	////////////////////////////////////////////////////////////////
	// Recherche du meilleur split

	// Recherche du meilleur split d'un intervalle de lignes de tableau
	// de contingence entre la premiere et la derniere ligne specifiee.
	// En entree, le vecteur FirstSubLineFrequencyVector du lineSplit doit contenir
	// tous les effectifs de l'intervalle complet, et dInitialCost le cout initial
	// (pour evaluer le DeltaCost).
	// En sortie, les caracteristiques du lineSplit sont correctement remplies avec
	// le meilleur Split (avec un DeltaCost InfiniteCost si aucun Split n'est
	// possible (par exemple pour des raisons d'effectif minimum).
	void ComputeBestSplit(const KWFrequencyTable* kwftSource, int nFirstIndex, int nLastIndex, double dInitialCost,
			      KWMODLLineSplit* lineSplit) const;

	//////////////////////////////////////////////////////////////////////////
	// Gestion des classes de travail pour les fusions et coupures d'intervalles
	// Les methodes sont implementees avec des vecteurs pleins (KWDenseFrequencyVector),
	// mais sont redefinissable pour s'adapter a des vecteur creux par exemple

	// Initialisation des variables de travail (notament, parametrage de la structure des couts)
	virtual void InitializeWorkingData(const KWFrequencyTable* kwftSource) const;
	virtual void CleanWorkingData() const;

	// Acces a l'objet createur d'instance de vecteur d'effectif
	const KWFrequencyVector* GetFrequencyVectorCreator() const;

	// Initialisation du bon nombre de classe d'un vecteur d'effectifs
	virtual void InitializeFrequencyVector(KWFrequencyVector* kwfvFrequencyVector) const;

	// Verification de l'integrite d'un vecteur d'effectifs
	virtual boolean CheckFrequencyVector(const KWFrequencyVector* kwfvFrequencyVector) const;

	// Ajout d'un intervalle source dans un intervalle cible
	//  Input: intervalles source et operande
	//  Output: intervalle source mis a jour
	virtual void AddFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
					const KWFrequencyVector* kwfvAddedFrequencyVector) const;

	// Suppression d'un intervalle source dans un intervalle cible
	//  Input: intervalles source et operande
	//  Output: intervalle source mis a jour
	virtual void RemoveFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
					   const KWFrequencyVector* kwfvRemovedFrequencyVector) const;

	// Fusion de deux intervalles
	//  Input: l'intervalle source et deux intervalles a merger
	//  Output: l'intervalle source, fusion des deux autres
	virtual void MergeTwoFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector,
					      const KWFrequencyVector* kwfvMergedFrequencyVector1,
					      const KWFrequencyVector* kwfvMergedFrequencyVector2) const;

	// Fusion de trois intervalles
	//  Input: l'intervalle source et trois intervalles a merger
	//  Output: l'intervalle source, fusion des trois autres
	virtual void MergeThreeFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector,
						const KWFrequencyVector* kwfvMergedFrequencyVector1,
						const KWFrequencyVector* kwfvMergedFrequencyVector2,
						const KWFrequencyVector* kwfvMergedFrequencyVector3) const;

	// Coupure d'un intervalle en deux partie (Split)
	//  Input: intervalle source, description du premier sous-intervalle
	//  Output: l'intervalle source devient le premier sous-intervalle (par recopie) et le nouvel interval
	//          devient le second sous-intervalle
	virtual void SplitFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
					  KWFrequencyVector* kwfvNewFrequencyVector,
					  const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const;

	// Coupure d'une paire d'intervalles en deux parties (MergeSplit)
	//  Input: deux intervalles source, description du premier sous-intervalle
	//  Output: les deux intervalles sources sont redecoupes
	virtual void MergeSplitFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector1,
						KWFrequencyVector* kwfvSourceFrequencyVector2,
						const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const;

	// Coupure d'un triplet d'intervalles en deux parties (MergeMergeSplit)
	//  Input: trois intervalles source, description du premier sous-intervalle
	//  Output: les deux intervalles sources extremites (1 et 3) sont redecoupes
	//          en se partageant les effectifs
	virtual void MergeMergeSplitFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector1,
						     const KWFrequencyVector* kwfvSourceFrequencyVector2,
						     KWFrequencyVector* kwfvSourceFrequencyVector3,
						     const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const;

	///////////////////////////////////////////////////////////////////////////
	// Donnees de travail
	// Ces donnees sont mutable car modifie par l'algorithme de discretisation
	// contrairement aux parametres de l'algorithme qui sont constant lors
	// d'une discretisation

	// Statistiques sur les optimisation effectuees
	mutable int nMergeNumber;
	mutable int nExtraMergeNumber;
	mutable int nSplitNumber;
	mutable int nExtraSplitNumber;
	mutable int nMergeSplitNumber;
	mutable int nMergeMergeSplitNumber;

	// Cout pour favoriser les optimisations tendant a reduire la contrainte
	// d'effectif minimum, et les rendre prioritaire
	static double dPriorityDeltaCost;

	// Cout infini pour gerer les optimisations impossibles
	static double dInfiniteCost;

	// Couts de partitionnement pour la discretisation
	KWUnivariatePartitionCosts* discretizationCosts;

	// Epsilon pour gerer le probleme de precision numerique
	double dEpsilon;
};

// Comparaison de deux objets KWMODLLineOptimization sur la variation de cout de Merge
int KWMODLLineOptimizationCompareMergeDeltaCost(const void* elem1, const void* elem2);

// Comparaison de deux objets KWMODLLineDeepOptimization sur la variation de cout de MergeSplit
int KWMODLLineDeepOptimizationCompareMergeSplitDeltaCost(const void* elem1, const void* elem2);

// Comparaison de deux objets KWMODLLineDeepOptimization sur la variation de cout de Split
int KWMODLLineDeepOptimizationCompareSplitDeltaCost(const void* elem1, const void* elem2);

// Comparaison de deux objets KWMODLLineDeepOptimization sur la variation de cout de MergeMergeSplit
int KWMODLLineDeepOptimizationCompareMergeMergeSplitDeltaCost(const void* elem1, const void* elem2);

///////////////////////////////////////////////////////////////////////
// Methodes en inline

inline const KWFrequencyVector* KWDiscretizerMODL::GetFrequencyVectorCreator() const
{
	return discretizationCosts->GetFrequencyVectorCreator();
}

inline double KWDiscretizerMODL::ComputePartitionCost(int nIntervalNumber) const
{
	return discretizationCosts->ComputePartitionCost(nIntervalNumber);
}

inline double KWDiscretizerMODL::ComputePartitionDeltaCost(int nIntervalNumber) const
{
	return discretizationCosts->ComputePartitionDeltaCost(nIntervalNumber);
}

inline double KWDiscretizerMODL::ComputePartitionCost(int nIntervalNumber, int nGarbageModalityNumber) const
{
	return discretizationCosts->ComputePartitionCost(nIntervalNumber, nGarbageModalityNumber);
}

inline double KWDiscretizerMODL::ComputePartitionDeltaCost(int nIntervalNumber, int nGarbageModalityNumber) const
{
	return discretizationCosts->ComputePartitionDeltaCost(nIntervalNumber, nGarbageModalityNumber);
}

inline double KWDiscretizerMODL::ComputeIntervalCost(const KWFrequencyVector* intervalFrequencyVector) const
{
	return discretizationCosts->ComputePartCost(intervalFrequencyVector);
}

inline double KWDiscretizerMODL::ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const
{
	return discretizationCosts->ComputePartitionGlobalCost(partTable);
}
