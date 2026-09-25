// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataGridInitialSolutionSearcherIV;

#include "KWClassStats.h"
#include "KWDataGrid.h"
#include "KWDataGridCosts.h"
#include "KWDataGridMerger.h"
#include "KWDataGridManager.h"
#include "KWAttributeSubsetStats.h"
#include "KWTupleTable.h"
#include "KWLearningSpec.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridInitialSolutionSearcherIV
// Recherche d'une solution initiale de qualite pour une coclustering IxV
class KWDataGridInitialSolutionSearcherIV : public Object
{
public:
	// Constructeur
	KWDataGridInitialSolutionSearcherIV();
	~KWDataGridInitialSolutionSearcherIV();

	// Parametrage par les specifications d'apprentissage
	// Memoire: les specifications sont referencees uniquement
	void SetLearningSpec(KWLearningSpec* specification);
	KWLearningSpec* GetLearningSpec() const;

	//////////////////////////////////////////////////////////////////////////////////////
	// Calcul d'une soution initiale

	// Calcul d'une solution initiale  par analyse bivariee des attributs internes
	// pour obtenir des partitions en VarPart pertinentes
	// Cette grille devra ensuite etre optimisee de facon classique, comme le son les grilles
	// issue d'une partition aleatoire.
	// On renvoie la grille la plus fine possible compatible avec les contraintes d'optimisation
	// en exploitant un nombre maximum de grille bivariee informatives
	// La methode se deroule en mode non verbeux.
	// Elle peut echouer en cas d'erreur, d'interruption utilisateur, ou d'absence de paires informatives
	boolean ComputeInitialSolution(const KWDataGrid* initialDataGrid, KWDataGrid* initialDataGridSolution);

	// Indique si la solution initiale a ete calculee
	boolean IsInitialSolutionComputed() const;

	// Nettoyage de toutes les donnee de calcul
	void Clean();

	// Indique le nombre de paires utilisee pour le calcul de la soliution initiale
	int GetInitialSolutionUsedPairNumber() const;

	// Construction d'une solution initiale specifique exploitant un nombre de paires passees en parametres
	void BuildSpecificInitialSolution(const KWDataGrid* initialDataGrid, int nPairNumber,
					  KWDataGrid* initialDataGridSolution) const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Constructtion d'une solution a partir des k permieres paires du table de pair passe en parametre
	// La solution exploite ces paire pour partitionner les attributs internes impliques dans au moins une paire
	// avec des cluster de VarPart singleton, plus un dernier cluster regroupant tous les attributs internes restants.
	// Les instances sont partitionnee de la facon a etre compatible avec les clusters de VarPart
	void BuildInitialSolutionFromBestPairs(const KWDataGrid* initialDataGrid,
					       const ObjectArray* oaInformativeAttributePairStats, int nPairNumber,
					       KWDataGrid* initialDataGridSolution) const;

	// Test si une solution est de taille compatible avec les contraintes d'optimisation d'une grille
	boolean IsInitialSolutionOptimizable(const KWDataGrid* initialDataGrid,
					     KWDataGrid* initialDataGridSolution) const;

	// Analyse bivariee des paires d'attributs internes
	// Le resultats est disponible dans bivariateClassStats
	// La methode peut echouer en cas d'erreur ou d'interruption utilisateur
	boolean ComputeInternalAttributesBivariateStats(const KWDataGrid* initialDataGrid);

	// Acces aux analyses bivariees
	const KWClassStats* GetInternalAttributesBivariateStats() const;

	// Nettoyage des analyse bivariees
	void CleanInternalAttributesBivariateStats();

	// Filtrage des attributs utilisables pour l'analyse bivariee, en supprimant ceux ne comportant qu'une seule valeur
	// Le tableau en sortie contient des KWDGAttribute
	void FilterInnerAttributes(const KWDataGrid* initialDataGrid, ObjectArray* oaFilteredInnerAttributes) const;

	// Tri d'un tableau d'attribut interne par complexite d'optimisation croissante
	void SortInnerAttributesByIncreasingComplexity(const KWDataGrid* initialDataGrid,
						       ObjectArray* oaInnerAttributes) const;

	// Selection des paires a utiliser, en prenant les plus informatives en priorite, et en s'arretant quand le nombre
	// total de parties de variables interne resultant atteint un seuil de complexite maximum
	// Le tableau en sortie contient les KWAttributePairStats selectionnes
	void SelectAttributePairStats(const KWClassStats* classStats, ObjectArray* oaSelectedAttributePairStats) const;

	// Calcul de l'intersection des discretisations a partir d'un tableau de partition de type KWDGSAttributeDiscretization
	void ComputeIntersectionDiscretizations(const KWDGAttribute* innerAttribute,
						const KWAttributeStats* attributeStats,
						const ObjectArray* oaAttributeDiscretizations,
						KWDGSAttributeDiscretization* resultDiscretization) const;

	// Calcul de l'intersection des groupes de valeurs a partir d'un tableau de partition de type KWDGSAttributeGrouping
	void ComputeIntersectionGroupings(const KWDGAttribute* innerAttribute, const KWAttributeStats* attributeStats,
					  const ObjectArray* oaAttributeGroupings,
					  KWDGSAttributeGrouping* resultGrouping) const;

	// Ecriture d'un rapport JSON a partir des stats bivariee calculees
	void WriteJSONAnalysisReport(KWClassStats* classStats, const ALString& sReportFileName) const;

	// Complexite algorithmique pour une variable interne impliquee dans une paire
	// Estimation de la complexite algorithmique en tenant compte du nombre
	// de valeurs disinctes dans le cas numerique ou categoriel
	// Cette stimation est fortement heuristique: ce qui est important est ici
	// d'avoir des valeurs comparables pour tire les attribut par complexite croissante
	static int ComputeAttributeOptimizationComplexity(const KWDGAttribute* attribute);

	// Comparaison de la complexite d'optmimisation de deux attributs
	static int CompareAttributeOptimizationComplexity(const void* elem1, const void* elem2);

	////////////////////////////////////////////////////////////////////////////
	// Variables de la classe

	// Specifications d'apprentissage
	KWLearningSpec* learningSpec;

	// Indicateur de calcul de la solution initiale
	boolean bIsInitialSolutionComputed;

	// Nombre de paires utilisees pour le calcul de la soliution initiale
	int nInitialSolutionUsedPairNumber;

	// Selection des paires d'attributs utilisees tries par informativite decroissante
	ObjectArray oaSelectedAttributePairStats;

	// Variables de travail pour l'apprentissage des analyses bivariees
	KWClassStats bivariateClassStats;
	KWLearningSpec bivariateLearningSpec;
};

//////////////////////////////////////////////////////////////////////////////////
// Classe KWValueSignature
// Classe technique de gestion des valeurs, impliquees dans un ensemble de partition
// La signature d'une valeur est le vecteur des index de parties dans chacune
// des partitions. Cela permet de calculer l'intersection des partitions, dont chaque
// partie regroupe les valeurs de meme signature
class KWValueSignature : public Object
{
public:
	// Constructeur
	KWValueSignature();
	~KWValueSignature();

	// Valeur
	void SetValue(Symbol sGroupedValue);
	Symbol GetValue() const;

	// Signature
	// Memoire: appartient a l'appele
	IntVector* GetSignature();

	// Comparaison de la partie signature, puis de la valeur
	int Compare(const KWValueSignature* aSource) const;

	// Comparaison de la partie signature uniquement
	int CompareSignature(const KWValueSignature* aSource) const;

	// Affichage, ecriture dans un fichier
	void Write(ostream& ost) const override;

	//////////////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Symbol sValue;
	IntVector ivSignature;
};

// Comparaison de deux signatures, en ignorant la valeur
int KWValueSignatureCompare(const void* elem1, const void* elem2);

// Implementation en inline

inline KWValueSignature::KWValueSignature() {}

inline KWValueSignature::~KWValueSignature() {}

inline void KWValueSignature::SetValue(Symbol sGroupedValue)
{
	sValue = sGroupedValue;
}

inline Symbol KWValueSignature::GetValue() const
{
	return sValue;
}

inline IntVector* KWValueSignature::GetSignature()
{
	return &ivSignature;
}
