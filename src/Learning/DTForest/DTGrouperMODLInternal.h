// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
// Implementation
// Les classes ci-dessous sont reservees a l'implementation des classes
// de groupage MODL

class DTGrouperMODLTwoClasses;
class KWMODLGroup;
class KWMODLGroupMerge;

#include "DTDiscretizerMODL.h"
#include "KWStat.h"
#include "DTUnivariatePartitionCost.h"
#include "KWFrequencyVector.h"
// #include "DTDiscretizerMODLLine.h"

//////////////////////////////////////////////////////////////////////////////////
// Algorithme de groupage MODL avec a priori a trois etage
// Cas particulier avec deux classes cibles, permettant de reutiliser les
// algorithmes de discretisation apres tri des valeurs descriptives
// par proportion croissante de la premiere classe cible
class DTGrouperMODLTwoClasses : public KWDiscretizerMODL // public DTDiscretizerMODL
{
public:
	// Constructeur
	DTGrouperMODLTwoClasses();
	~DTGrouperMODLTwoClasses();

	// Calcul de la loi agregee pour des regroupements de lignes
	// On peut passer une table source contenant des sous-groupes initiaux
	// isus d'un preprocessing, ce qui oblige a indiquer le nombre
	// de valeur sources initial
	// Cette methode gere egalement la contrainte du nombre max de groupes
	// Renvoie 1-TauxCompression
	void Group(KWFrequencyTable* kwftSource, int nInitialSourceValueNumber, KWFrequencyTable*& kwftTarget,
		   IntVector*& ivGroups) const;

	// Variante de la methode ou la table source est deja triee par frequence
	// cible croissante, ce qui evite de le refaire
	void GroupSortedTable(KWFrequencyTable* kwftSortedSource, int nInitialSourceValueNumber,
			      KWFrequencyTable*& kwftTarget, IntVector*& ivGroups) const;

	// Variante de la methode ou on propose un groupage initial, a ameliorer
	// par post-optimisation
	// Cette methode n'a d'interet que pour des raisons d'optimisation en
	// temps de calcul
	void PostOptimizeGrouping(KWFrequencyTable* kwftSortedSource, int nInitialSourceValueNumber,
				  KWFrequencyTable* kwftInitialTarget, KWFrequencyTable*& kwftTarget,
				  IntVector*& ivGroups) const;

	// Nom de l'algorithme
	const ALString GetName() const override;

	/////////////////////////////////////////////////////////////////
	// Implementation
protected:
	friend class DTGrouperMODL;

	// Initialisation des variables de travail
	// Reimplementation vide de ces methodes virtuelle pour les inhiber
	// (le parametrage est fait par l'appelant)
	void InitializeWorkingData(const KWFrequencyTable* kwftSource) const override;
	void CleanWorkingData() const override;

	// Discretisation d'une table granularisee avec recherche d'une table optimale avec groupe poubelle
	void DiscretizeFrequencyTable(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget) const;

	///////////////////////////////////////////////////////////////////////////
	// Gestion du nombre de modalites par ligne de contingence

	// Acces au mode de suivi actif ou non du nombre de modalites
	boolean GetModalityNumberMonitoring() const;
	void SetModalityNumberMonitoring(boolean bValue) const;

	// Acces a la liste utilisee comme variable de travail
	SortedList* GetWorkingFrequencyList() const;
	void SetWorkingFrequencyList(SortedList* frequencyList) const;
	void ResetWorkingFrequencyList() const;

	// Ajout et retrait d'un intervalle de la liste de travail triee par nombre de modalites de l'intervalle
	// Necessite que la liste soit initialisee
	void AddIntervalToWorkingFrequencyList(KWMODLLine* interval) const override;
	void RemoveIntervalFromWorkingFrequencyList(KWMODLLine* interval) const override;

	// Rangement des intervalles dans une liste triee ordonnee par effectif decroissant du nombre de modalites
	// En entree : frequencyList est NULL
	// En sortie : frequencyList est initialisee
	// La memoire appartient a l'appelant
	void InitializeFrequencySortedList(SortedList* frequencyList, KWMODLLine* headInterval) const;

	// Calcul du nombre de modalites d'une ligne de contingence decrite par ses index de debut et de fin par rapport
	// a une table kwftSource En entree, la table kwftSource est initialisee En sortie, le nombre de modalites
	int ComputeModalityNumber(const KWFrequencyTable* kwftSource, int nFirstIndex, int LastIndex) const override;

	// Calcul du nombre de modalites du groupe poubelle d'une partition
	// En entree, la table kwftTargetWithGarbage dont les KWFrequencyVector contiennent le nombre de modalites par
	// ligne de contingence En sortie, le nombre de modalites du groupe poubelle egal a l'argmax du nombre de
	// modalites par ligne de contingence
	int ComputeGarbageModalityNumberFromTable(KWFrequencyTable* kwftTargetWithGarbage) const;

	// Gestion de la liste des intervalles
	// Nombre total de modalites d'une liste d'intervalles, dont on passe la tete
	int GetIntervalListTotalModalityNumber(const KWMODLLine* headLine) const;

	///////////////////////////////////////////////////////////////////
	// Optimisation de la liste des intervalles avec le critere MODL
	// Renvoie la tete de la liste issue de l'optimisation sans poubelle.
	// Pilotage de l'optimisation avec poubelle pour l'algo glouton descendant (parametragre par defaut
	// OptimizedGreedyMerge) Structure analogue a IntervalListOptimization avec, en parametre supplementaire, la
	// tete de liste issue de l'optimisation AVEC poubelle et bGarbageImproveBestMerge
	KWMODLLine*
	IntervalListOptimizationWithGarbage(const KWFrequencyTable* kwftSource,
					    KWMODLLineDeepOptimization*& headIntervalDeepOptimizationWithGarbage,
					    boolean& bGarbageImproveBestMerge) const;

	// Adaptation de la methode IntervalListBestMergeOptimization a la recherche d'une partition avec poubelle
	// La liste dont la tete headInterval est passee en parametre doit etre initialisee pour la recherche de
	// poubelles c'est a dire que chaque element doit contenir son nombre de modalites En sortie :
	// - headInterval : tete de la liste sans poubelle
	// - headIntervalWithGarbage : tete de la liste avec poubelle
	// CH V9 TODO : a mettre en return
	// - nGarbageModalityNumber : la taille de la poubelle associee a la meilleure liste avec poubelle
	// - bGarbageImproveBestMerge : renvoie vraie si la meilleure liste avec poubelle est de cout inferieur a la
	// meilleure liste sans poubelle
	virtual void IntervalListBestMergeOptimizationWithGarbage(KWMODLLineOptimization*& headInterval,
								  KWMODLLineOptimization*& headIntervalWithGarbage,
								  int& nGarbageModalityNumber,
								  boolean& bGarbageImproveBestMerge) const;

	// Post-optimisation d'une partition qui contient un groupe poubelle
	// La taille de la poubelle est supposee constante au cours des deplacements envisages
	// Cette taille est passee en dernier argument
	// Meme schema que dans IntervalListPostOptimization avec la prise en compte
	// de nGarbageModalityNumber lors des calculs de variation de cout
	virtual void IntervalListPostOptimizationWithGarbage(const KWFrequencyTable* kwftSource,
							     KWMODLLineDeepOptimization*& headInterval,
							     int nGarbageModalityNumber) const;

	////////////////////////////////////////////////////////////////
	// Optimisation basee sur la recherche de Merges
	// Optimisation basee sur une recherche locale des ameliorations de type Merge
	// Arret des Merges des qu'une fusion d'intervalles degrade le cout sans poubelle
	// ou degrade le cout avec poubelle lorsque celui est plus avantageux que le cout sans poubelle
	// Suivi du nombre de modalites de chaque groupe
	// Suivi du cout de la partition avec un groupe poubelle egal a celui qui contient le plus de modalites
	virtual void IntervalListMergeOptimizationWithGarbagePartitionCost(KWMODLLineOptimization*& headInterval) const;

	////////////////////////////////////////////////////////////////
	// Tri des lignes d'une table d'effectifs par proportion de la ieme cible
	// (applicable uniquement dans le cas d'une table d'effectifs creuse).
	// La table d'effectifs est triee directement.
	// Pour faire le lien entre table initiale et table triee, les methodes
	// alimentent sur option le vecteur des index initiaux (pour la table triee)
	// ou des index des lignes triees (pour la table initiale). Ces vecteurs sont
	// alimentes que s'il ne sont pas a NULL. Dans ce cas, ils sont retailles et
	// alimentes correctement.
	// Maintenance: ces trois methodes sont dupliques des methodes correspondantes de KWFrequencyTable
	static void SortFrequencyTableByTargetRatio(KWFrequencyTable* kwftFrequencyTable, boolean bAscending,
						    int nTargetIndex, IntVector* ivInitialLineIndexes,
						    IntVector* ivSortedLineIndexes);

	// Methodes de verification de tri
	// Cette methode est couteuse en temps de calcul, mais interessante dans le cadre d'assertions
	static boolean IsFrequencyTableSortedByTargetRatio(KWFrequencyTable* kwftFrequencyTable, boolean bAscending,
							   int nTargetIndex);

	// Donnees de travail
	// Booleen indiquant si le suivi du nombre de modalites est actif ou non
	// Par defaut a false
	// A true dans les methodes dediees a la recherche du groupe poubelle
	mutable boolean bSearchGarbage;
	// Liste triee de KWMODLLine en fonction du nModalityNumber de leur KWFrequencyVector
	// mutable car modifiee par methode Set/Reset-WorkingFrequencyList qui sont const
	mutable SortedList* workingFrequencyList;
};

// Methodes en inline

/////
