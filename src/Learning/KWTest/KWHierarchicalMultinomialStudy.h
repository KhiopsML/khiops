// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "ALString.h"
#include "Object.h"
#include "Vector.h"
#include "KWStat.h"
#include "KWClassDomain.h"
#include "KWLearningProject.h"
#include "KWDataGridStats.h"
#include "KWClassDomain.h"
#include "KWSTDatabaseTextFile.h"
#include "KWClassStats.h"
#include "KWQuantileBuilder.h"

///////////////////////////////////////////////////////////////////
// Etude d'un prior hierarchique pour la distribution multinomiale
// On se base sur l'estiamtion de la densite jointe de deux variables
// numeriques identiques X=Y au moyen d'une grille numerique bivariee
class KWHierarchicalMultinomialStudy : public Object
{
public:
	// Constructeur
	KWHierarchicalMultinomialStudy();
	~KWHierarchicalMultinomialStudy();

	// Test
	static void TestNewPrior();
	static void TestNewPriorOne();
	static void TestNewPriorOld();
	static void TestDataset();

	//////////////////////////////////////////////////////////////
	///// Implementation
protected:
	//////////////////////////////////////////////////////////////
	// Etude de correlation bivariee

	// Etude de la correlation entre toutes les paires de variables numeriques d'un jeu de donnees
	static void StudyDatasetBivariate(const ALString& sClassFileName, const ALString& sClassName,
					  const ALString& sDatasetName, const ALString& sResultDirectory);

	// Etude de la correlation entre deux variables, avec le prior multinomial standard ou hierarchique
	// et ecriture dans un fichier de resultat
	static void StudyBivariateSample(int nSize, double dSigma, int nSeed, fstream& fstResults);
	static void StudyWriteHeader(fstream& fstResults);

	// Calcul de la divergence de Kullback-Leibler entre la densite jointe estimee par une grille bivariee
	// et la densite jointe reelle pour Y = X + Gaussian(0, Sigma), pour uniforme sur [0;1]
	static double ComputeDKL(KWDataGridStats* pairDataGridStats, KWAttributeStats* attributeStats1,
				 KWAttributeStats* attributeStats2, double dSigma);

	// Acces a la classe de test des examples numeriques bivaries Sample(X1, X2)
	static KWClass* GetBivariateClass();

	//////////////////////////////////////////////////////////////
	// Generation d'un jeu de donnees de regression bruitee
	// Y = X + Noise, X in [0;1], Noise = Gaussian(0, S)

	// Creation d'un jeu de donnees
	static void CreateRegressionSample(int nSize, double dSigma, int nSeed, DoubleVector* dvSampleX,
					   DoubleVector* dvSampleY);

	// Sauvegarde d'un jeu de donnees dans un fichier
	static void SaveSampleInFile(const DoubleVector* dvSampleX, const DoubleVector* dvSampleY,
				     const ALString& sFileName);

	//////////////////////////////////////////////////////////////
	//  Calcul des couts d'un du prior multinomial

	// Type de prior pour la multinomial
	enum
	{
		Standard,
		Hierarchical1,
		Hierarchical2,
		None
	};

	// Calcul du cout du modele null
	static double ComputeNullCost(int n);

	// Calcul du nombre optimal d'intervalles au moyen de chaque prior
	// (standard, hierarchique version 1 et 2)
	static int ComputeBestMultinomialModel(int nPrior, int nFrequency, boolean bVerbose);

	// Calcul du cout d'une grille en 10 intervalles J equirepartis
	static double ComputeMultinomialCost(int nPrior, int nFrequency, int nJ);
	static double ComputeIPriorCost(int nPrior, int nFrequency, int nJ);
	static double ComputeMultinomialPriorCost(int nPrior, int nFrequency, int nJ);
	static double ComputeLikelihoodCost(int nPrior, int nFrequency, int nJ);

	// Calcul du level d'une grille en 10 intervalles J equirepartis
	static double ComputeMultinomialLevel(int nPrior, int nFrequency, int nJ);

	// Directory racine des etude
	static ALString sRootDir;
};

///////////////////////////////////////////////////////
// Classe KWAttributePairStatsStudy
// Specialisation de KWAttributePairStats pour etudier l'impact des nouveaux priors
class KWAttributePairStatsStudy : public KWAttributePairStats
{
public:
	// Constructeur
	KWAttributePairStatsStudy();
	~KWAttributePairStatsStudy();

	// Parametrage de l'utilisation des nouveaux prior (defaut: false)
	void SetHierarchicalMultinomialPrior(boolean bValue);
	boolean GetHierarchicalMultinomialPrior() const;

	// Redefinition de la methode d'optimisation
	// CH IV Begin
	//Le parcours exhaustif des granularites entieres remplace par un parcours des granularites par puissance de 2
	// Les methodes specifiques d'export de la grille granularisee de la classe KWHierarchicalMultinomialStudy sont supprimees au profit de l'utilisation
	// des methodes de KWDataGridManager pour simplifier la maintenance du code de KWTest
	// CH IV End
	boolean ComputeStats(const KWTupleTable* tupleTable) override;

	// Redefinition de la structure des cout pour l'etude des prior
	KWDataGridCosts* CreateDataGridCost() const override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Variables
	boolean bHierarchicalMultinomialPrior;
	KWDataGridManager dataGridManager;
};

////////////////////////////////////////////////////////////////////////////
// Structure des couts d'une grille de donnees dans le cas bivarie continu
// Cas standard de l'article sur l'etude du codage hierarchique multinomial
class KWDataGridClusteringCostsBivariate : public KWDataGridCosts
{
public:
	// Constructeur
	KWDataGridClusteringCostsBivariate();
	~KWDataGridClusteringCostsBivariate();

	// Duplication
	KWDataGridCosts* Clone() const override;

	/////////////////////////////////////////////////////////////////////////
	// Calcul des couts locaux par entite de la grille des donnees

	// Calcul du cout local d'un DataGrid, en fonction de taille de la grille (en cellules) et
	// du nombre d'attribut informatifs
	//  La taille de la grille est donnee par son log, pour ne pas depasser les limites informatiques
	//  Les attributs informatifs sont ceux ayant strictement plus de une partie
	double ComputeDataGridCost(const KWDataGrid* dataGrid, double dLnGridSize,
				   int nInformativeAttributeNumber) const override;

	// Calcul du cout local d'un attribut, pour un nombre de parties donnees
	// Le nombre de parties est le nombre total de parties.
	// En cas de presence d'une poubelle il s'agit du nombre de parties informatives de l'attribut + 1
	double ComputeAttributeCost(const KWDGAttribute* attribute, int nPartitionSize) const override;

	// Calcul du cout local d'une partie
	double ComputePartCost(const KWDGPart* part) const override;

	// Calcul du cout local de l'union de deux parties
	double ComputePartUnionCost(const KWDGPart* part1, const KWDGPart* part2) const override;

	// Calcul du cout local d'une cellule
	double ComputeCellCost(const KWDGCell* cell) const override;

	// Calcul du cout local d'une valeur d'un attribut symbolique
	double ComputeValueCost(const KWDGValue* value) const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;
};

////////////////////////////////////////////////////////////////////////////
// Structure des couts d'une grille de donnees dans le cas bivarie continu
// Cas standard de l'article sur l'etude du codage hierarchique multinomial
class KWDataGridClusteringCostsBivariateH : public KWDataGridClusteringCostsBivariate
{
public:
	// Constructeur
	KWDataGridClusteringCostsBivariateH();
	~KWDataGridClusteringCostsBivariateH();

	// Duplication
	KWDataGridCosts* Clone() const override;

	// Calcul du cout local d'un DataGrid, selon le prior hierarchique
	double ComputeDataGridCost(const KWDataGrid* dataGrid, double dLnGridSize,
				   int nInformativeAttributeNumber) const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Export des effectifs des cellules dans un vecteur, trie par effectif croissant
	void ExportCellFrequencies(const KWDataGrid* dataGrid, IntVector* ivCellFrequencies) const;

	// Calcul du meilleur prior multinomial a partir du nombre d'instances et du nombre total de cellules
	// On a aussi le nombre total de cellules courant et les effectifs de ces cellules
	// Le calcul est alors effectue sur cet etat courant, et evalue de facon heuristique en fonction
	// du changement de nombre total de cellules
	// Ce probleme est lie au fait que l'evaluation d'une nouvelle taille de grille est effectuee
	// avant la meilleurs fusion du DataGridMerger, alors que le vrai cout devrait etre calcule apres
	// la fusion. La solution actuelle est approximativen mais donne le bonne ordre de grandeur
	// des solutions pour evaluer le comoportement du nouveau prior
	double ComputeBestHierarchicalMultinomialPrior(int nInstanceNumber, int nTotalCellNumber,
						       int nCurrentTotalCellNumber,
						       const IntVector* ivCurrentCellFrequencies) const;
};
