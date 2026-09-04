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

	// Recherche d'une solution initiale meilleure que celle du modele null
	// Le parametre initialDataGridSolution en sortie contient une grille initiale fine
	// potentiellement intessante. Elle est de type KWDataGridMerger car elle doit ensuite
	// etre optimisee selon les algorithmes d'optimisation standard.
	// On exploite a cet effet des grilles bivariees entre attribut internes pour
	// obtenir des partitions en VarPart pertinentes
	void SearchInitialSolution(const KWDataGrid* initialDataGrid, KWDataGridMerger* initialDataGridSolution) const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	//  Analyse bivariee des paires d'attributs internes
	// Le resultats est disponible dans bivariateClassStats
	void ComputeInternalAttributesBivariateStats(const KWDataGrid* initialDataGrid) const;

	// Acces aux analyses bivariees
	const KWClassStats* GetInternalAttributesBivariateStats() const;

	// Nettoyage des analyse bivariees
	void CleanInternalAttributesBivariateStats() const;

	// Calcul de l'intersection des discretisations a partir d'un tableau de partition de type KWDGSAttributeDiscretization
	void ComputeIntersectionDiscretizations(const KWAttributeStats* attributeStats,
						const ObjectArray* oaAttributeDiscretizations,
						KWDGSAttributeDiscretization* resultDiscretization) const;

	// Calcul de l'intersection des groupes de valeurs a partir d'un tableau de partition de type KWDGSAttributeGrouping
	void ComputeIntersectionGroupings(const KWAttributeStats* attributeStats,
					  const ObjectArray* oaAttributeGroupings,
					  KWDGSAttributeGrouping* resultGrouping) const;

	// Ecriture d'un rapport JSON a partir des stats bivariee calculees
	void WriteJSONAnalysisReport(KWClassStats* classStats, const ALString& sReportFileName) const;

	// Specifications d'apprentissage
	KWLearningSpec* learningSpec;

	// Variables de travail pour l'apprentissage des analyses bivariees
	mutable KWClassStats bivariateClassStats;
	mutable KWLearningSpec bivariateLearningSpec;
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
