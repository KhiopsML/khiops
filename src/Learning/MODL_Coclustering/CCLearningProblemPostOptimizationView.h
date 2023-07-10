// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"

#include "CCLearningProblem.h"
#include "CCCoclusteringReport.h"
#include "CCCoclusteringBuilder.h"

#include "CCLearningProblemToolView.h"
#include "CCPostProcessingSpecView.h"
#include "CCAnalysisResultsView.h"
#include "LMLicenseManager.h"

////////////////////////////////////////////////////////////////////////////////////////
// Prototype implemente rapidement dans le cadre du post-doc de Marirus Bartcus,
// disponible dans le menu Tools de Khiops coclustering:
//   . sous le nom "Post-optimize coclustering (expert mode)..."
//
// L'objectif est de permettre d'initialiser une solution de coclustering a partir
// d'une definitions de clusters existants, en offrant des fonctionnalites de:
//   . pre-optimisation rapide par deplacement de valeurs
//   . optimisation par fusions de clusters
//   . post-optimisation intense par deplacement de valeurs
// Chaque traitement peut etre effectue independamment.
// Les clusters sont specifie au moyen d'un fichier au format des fichier de coclustering .khc,
// ne contenant que:
//   . la ligne de version de Khiops
//   . la section d'entete "Dimensions"
//   . les sections "Composition" par variable categorielle
//     . les clusters doivent etre definis sur des ligne successives
//     . les valeurs par cluster doivent etre complete, et leur Frequency correcte
//     . les typicalites doivent juste etre entre 0 et 1 (OK si toutes a 1)

////////////////////////////////////////////////////////////
// Classe CCLearningProblemPostOptimizationView
class CCLearningProblemPostOptimizationView : public CCLearningProblemToolView
{
public:
	// Constructeur
	CCLearningProblemPostOptimizationView();
	~CCLearningProblemPostOptimizationView();

	// Redefinition de l'action de chargement du coclustering en entree, pour ne lire que la definition des clusters
	void SelectInputCoclustering();

	// Actions disponibles
	void PostOptimize();

	// Lecture des informations sur les clusters d'un coclustering a partir d'un fichier de rapport partiel
	// comportant les sections suivantes uniquement: Dimensions, Bounds, Composition
	// Permet d'obtenir les specification des clusters, mais sans aucun effectif par cellule, intervalle ou groupe
	boolean ReadReportClusters(const ALString& sFileName, CCHierarchicalDataGrid* coclusteringDataGrid);
	boolean ReadClusterComposition(CCCoclusteringReport* coclusteringReport,
				       CCHierarchicalDataGrid* coclusteringDataGrid);

	// Parametrage de l'objet edite
	void SetObject(Object* object) override;
};

////////////////////////////////////////////////////////////
// Classe CCCoclusteringPostOptimizer
class CCCoclusteringPostOptimizer : public CCCoclusteringBuilder
{
public:
	// Constructeur
	CCCoclusteringPostOptimizer();
	~CCCoclusteringPostOptimizer();

	// Parametrage de clusters existant au moyen d'une grille vide comportant la defintiions des clusters
	// Memoire: appartient a l'appelant
	void SetClusterDataGrid(const KWDataGrid* dataGrid);
	const KWDataGrid* GetClusterDataGrid() const;

	// Pre-optimisation rapide par deplacement de valeurs entre clusters
	boolean GetPreOptimize() const;
	void SetPreOptimize(boolean bValue);

	// Optimisation des clusters par fusions
	boolean GetOptimize() const;
	void SetOptimize(boolean bValue);

	// Pre-optimisation intense par deplacement de valeurs entre clusters
	boolean GetPostOptimize() const;
	void SetPostOptimize(boolean bValue);

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Methode virtuelle d'optimisation d'une grille
	void OptimizeDataGrid(const KWDataGrid* inputInitialDataGrid, KWDataGrid* optimizedDataGrid);

	// Optimisation greedy (DataGridMerger) (inspire de KWDataGridOptimizer::GreedyOptimize)
	double GreedyOptimize(const KWDataGrid* inputInitialDataGrid, KWDataGrid* optimizedDataGrid) const;

	// Grille contenant les clusters existants
	const KWDataGrid* clusterDataGrid;

	// Parametres d'optimisation
	boolean bPreOptimize;
	boolean bOptimize;
	boolean bPostOptimize;
};
