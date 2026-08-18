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
	//  Analyse bivariee des paires de'attributs internes
	// La methode cree des objets KWAttributePairStats par paire analysee dans le tgableau en sortie
	// Memoire: le tableau et son contenu appartienent a l'appelant
	void ComputeInternalAttributesBivariateStats(const KWDataGrid* initialDataGrid,
						     ObjectArray* oaAttributePairStats) const;

	// Ecriture d'un rapport JSON a partir des stats bivariee calculees
	void WriteJSONAnalysisReport(KWClassStats* classStats, const ALString& sReportFileName) const;

	// Specifications d'apprentissage
	KWLearningSpec* learningSpec;

	// Variables de travail pour l'apprentissage des analyses bivariees
	mutable KWLearningSpec bivariateLearningSpec;
	mutable KWAttributePairsSpec bivariatePairSpec;
	mutable KWClassStats bivariateClassStats;
};
