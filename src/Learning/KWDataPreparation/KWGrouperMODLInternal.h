// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//// Implementation
//// Les classes ci-dessous sont reservees a l'implementation des classes
//// de groupage MODL

class KWGrouperMODLTwoClasses;
class KWMODLGroup;
class KWMODLGroupMerge;

#include "KWDiscretizerMODL.h"
#include "KWStat.h"
#include "KWUnivariatePartitionCost.h"
#include "KWFrequencyVector.h"
#include "KWDiscretizerMODLLine.h"

// Comparaison de deux objets KWMODLLine sur leur nombre de modalites
int KWMODLLineCompareModalityNumber(const void* elem1, const void* elem2);

//////////////////////////////////////////////////////////////////////////////////
// Algorithme de groupage MODL avec a priori a trois etage
// Cas particulier avec deux classes cibles, permettant de reutiliser les
// algorithmes de discretisation apres tri des valeurs descriptives
// par proportion croissante de la premiere classe cible
class KWGrouperMODLTwoClasses : public KWDiscretizerMODL
{
public:
	// Constructeur
	KWGrouperMODLTwoClasses();
	~KWGrouperMODLTwoClasses();

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
	//// Implementation
protected:
	friend class KWGrouperMODL;

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

///////////////////////////////////////////////////////////////
// Groupe MODL
// Cette classe est un support de travail pour des calculs
// algorithmiques sur les tables de contingence
class KWMODLGroup : public Object
{
public:
	// Constructeur
	KWMODLGroup(const KWFrequencyVector* kwfvFrequencyVectorCreator);
	~KWMODLGroup();

	// Vecteur de comptage des effectifs de la ligne fusionnee
	KWFrequencyVector* GetFrequencyVector();

	// Valeur d'un groupe
	void SetCost(double dValue);
	double GetCost() const;

	// Index
	void SetIndex(int nValue);
	int GetIndex() const;

	// Nombre de modalites d'un groupe (celui de son KWFrequencyVector)
	void SetModalityNumber(int nModality);
	int GetModalityNumber() const;

	// Position dans une liste triee par nombre de modalites
	void SetPosition(POSITION pos);
	POSITION GetPosition() const;

	// Duplication
	KWMODLGroup* Clone() const;

	// Rapport synthetique destine a rentrer dans un tableau (sans retour a la ligne)
	void WriteHeaderLineReport(ostream& ost) const;
	void WriteLineReport(ostream& ost) const;

	// Display
	void Write(ostream& ost) const override;

	///////////////////// Implementation ///////////////////////////
protected:
	KWFrequencyVector* kwfvFrequencyVector;
	double dCost;
	int nIndex;
};

// Comparaison des nombres de modalites des groupes
int KWMODLGroupModalityNumberCompare(const void* elem1, const void* elem2);

///////////////////////////////////////////////////////////////
// Fusion de groupes MODL
// Cette classe est un support de travail pour des calculs
// algorithmiques sur les tables de contingence
class KWMODLGroupMerge : public Object
{
public:
	// Constructeur
	KWMODLGroupMerge();
	~KWMODLGroupMerge();

	// Variation de valeur suite a la fusion (MergeGroupCost - Group1Cost - Group2Cost)
	void SetDeltaCost(double dValue);
	double GetDeltaCost() const;

	// Index du groupe source 1
	void SetIndex1(int nValue);
	int GetIndex1() const;

	// Index du groupe source 2
	void SetIndex2(int nValue);
	int GetIndex2() const;

	// Position dans une liste
	void SetPosition(POSITION pos);
	POSITION GetPosition() const;

	// Duplication
	KWMODLGroupMerge* Clone() const;

	// Display
	void Write(ostream& ost) const override;

	///////////////////// Implementation ///////////////////////////
protected:
	double dDeltaCost;
	int nIndex1;
	int nIndex2;
	POSITION position;
};

// Comparaison de la valeur de deux merges
int KWMODLGroupMergeCompare(const void* elem1, const void* elem2);

// Methodes en inline

/////

inline KWMODLGroup::KWMODLGroup(const KWFrequencyVector* kwfvFrequencyVectorCreator)
{
	require(kwfvFrequencyVector != NULL);
	dCost = 0;
	nIndex = 0;
	kwfvFrequencyVector = kwfvFrequencyVectorCreator->Create();
}

inline KWMODLGroup::~KWMODLGroup()
{
	delete kwfvFrequencyVector;
}

inline KWFrequencyVector* KWMODLGroup::GetFrequencyVector()
{
	return kwfvFrequencyVector;
}

inline void KWMODLGroup::SetCost(double dValue)
{
	dCost = dValue;
}

inline double KWMODLGroup::GetCost() const
{
	return dCost;
}

inline void KWMODLGroup::SetIndex(int nValue)
{
	nIndex = nValue;
}

inline int KWMODLGroup::GetIndex() const
{
	return nIndex;
}

inline void KWMODLGroup::SetModalityNumber(int nModality)
{
	kwfvFrequencyVector->SetModalityNumber(nModality);
}

inline int KWMODLGroup::GetModalityNumber() const
{
	return kwfvFrequencyVector->GetModalityNumber();
}

inline void KWMODLGroup::SetPosition(POSITION pos)
{
	kwfvFrequencyVector->SetPosition(pos);
}

inline POSITION KWMODLGroup::GetPosition() const
{
	return kwfvFrequencyVector->GetPosition();
}

/////

inline KWMODLGroupMerge::KWMODLGroupMerge()
{
	dDeltaCost = 0;
	nIndex1 = 0;
	nIndex2 = 0;
	position = NULL;
}

inline KWMODLGroupMerge::~KWMODLGroupMerge() {}

inline void KWMODLGroupMerge::SetDeltaCost(double dValue)
{
	dDeltaCost = dValue;
}

inline double KWMODLGroupMerge::GetDeltaCost() const
{
	return dDeltaCost;
}

inline void KWMODLGroupMerge::SetIndex1(int nValue)
{
	nIndex1 = nValue;
}

inline int KWMODLGroupMerge::GetIndex1() const
{
	return nIndex1;
}

inline void KWMODLGroupMerge::SetIndex2(int nValue)
{
	nIndex2 = nValue;
}

inline int KWMODLGroupMerge::GetIndex2() const
{
	return nIndex2;
}

inline void KWMODLGroupMerge::SetPosition(POSITION pos)
{
	position = pos;
}

inline POSITION KWMODLGroupMerge::GetPosition() const
{
	return position;
}
