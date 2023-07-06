// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class CCCoclusteringBuilder;
class CCCoclusteringOptimizer;

#include "PLDatabaseTextFile.h"
#include "KWTupleTable.h"
#include "KWTupleTableLoader.h"
#include "KWAttributeSubsetStats.h"
#include "KWDataGridMerger.h"
#include "KWDataGridPostOptimizer.h"
#include "KWDataPreparationClass.h"
#include "CCHierarchicalDataGrid.h"
#include "CCCoclusteringReport.h"

/////////////////////////////////////////////////////////////////////////////////
// Construction et services autour du coclustering
// Le parametrage est celui de la classe ancetre, a savoir un probleme d'apprentissage
// ainsi que les noms des attributs prenant part au coclustering
class CCCoclusteringBuilder : public KWAttributeSubsetStats
{
public:
	// Constructeur
	CCCoclusteringBuilder();
	~CCCoclusteringBuilder();

	// Variable d'effectif (optionnelle)
	// Chaque enregistrement est pondere (selon un nombre entier positif) par le contenu de cette variable
	// lors de la creation de la grille initiale
	const ALString& GetFrequencyAttribute() const;
	void SetFrequencyAttribute(const ALString& sValue);

	// Verification de la validite des specifications
	boolean CheckSpecifications() const override;

	// Calcul du coclustering, renvoie false en cas d'erreur ou d'interruption utilisateur
	boolean ComputeCoclustering();
	boolean IsCoclusteringComputed() const;

	// Test si le coclustering est calcule et informatif (au moins deux dimensions)
	boolean IsCoclusteringInformative() const;

	/////////////////////////////////////////////////////
	// Acces aux resultats de coclustering

	// Grille de coclustering
	// Peut etre NULL si la grille n'est pas informative
	const CCHierarchicalDataGrid* GetCoclusteringDataGrid() const;

	// Structure de cout de la grille
	const KWDataGridCosts* GetCoclusteringDataGridCosts() const;

	//////////////////////////////////////////////////////////////////
	// Parametrage avance pour la gestion anytime de l'optimisation

	// Nom du fichier de sauvegarde des solutions intermediaires
	void SetReportFileName(const ALString& sFileName);
	const ALString& GetReportFileName() const;

	// Export Khc (defaut: true)
	boolean GetExportAsKhc() const;
	void SetExportAsKhc(boolean bValue);

	// Supression du dernier fichier temporaire sauvegarde
	void RemoveLastSavedReportFile() const;

	// Methode appelee lors de l'optimisation a chaque etape d'optimisation
	// A chaque amelioration, un nouveau fichier de sauvegarde est cree avec un index croissant,
	// le fichier precedent etant detruit.
	// Un message utilisateur est egalement emis.
	// Transmission de la grille initiale granularisee
	// bIsLastSaving : si true, la sauvegarde est effectue meme s'il n'y a pas amelioration
	// Permet de recalculer la hierarchie du coclustering apres l'atteinte de la granularite maximale
	void HandleOptimizationStep(const KWDataGrid* optimizedDataGrid, const KWDataGrid* initialGranularizedDataGrid,
				    boolean bIsLastSaving) const;

	// Libelles utilisateur: nom du module de l'application (GetLearningModuleName())
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Methode virtuelle d'optimisation d'une grille
	virtual void OptimizeDataGrid(const KWDataGrid* inputInitialDataGrid, KWDataGrid* optimizedDataGrid);

	///////////////////////////////////////////////////////////////////////////////////////
	// Gestion preventive de l'utilisation des ressources memoire, avec message d'erreur
	// On procede selon les etapes suivantes:
	//   . estimation si on peut lire la base a partir du fichier
	//   . lecture de la base
	//   . transformation de la base en table de tuples
	//   . transformation de la table de tuples en grille initiale
	//   . initialisation d'une grille a optimiser

	// Verification de la memoire necessaire pour charger la base
	// Renvoie false si verification possible (pas de variable de selection) et si memoire insuffisante
	boolean CheckMemoryForDatabaseRead(KWDatabase* database) const;

	// Alimentation d'une table de tuples comportant les attribut a analyser a partir de la base
	// La table de tuples est remplie au fur et a mesure de la lecture de la base, et des verification
	// de depasssement des capacites memoire sont effectuees regulierement
	// On renvoie true en cas de succes, false sinon avec un message d'erreur
	boolean FillTupleTableFromDatabase(KWDatabase* database, KWTupleTable* tupleTable);

	// Renvoie l'effectif associe a un enregistrement, avec eventuellement affichage de warning
	// Renvoie 1 si l'index de l'attribut d'eefctif est invalide
	// Renvoie 0 si erreur dans la specification de l'effectif
	int GetDatabaseObjectFrequency(KWObject* kwoObject, KWLoadIndex liFrequencyAttributeLoadIndex,
				       longint lRecordIndex);

	// Verification de la memoire necessaire pour construire une grille initiale a partir d'un nombre de tuples
	// La base en entree peut etre entierement traitee:
	//   . dans ce cas, on dispose des statistique descriptives et donc du nombre de valeurs par attribut
	// ou en cours de lecture, pour une alimnettaion partielle de la table de tuples
	//   . dans ce cas, les nombres de valeurs par attribut sont estimes
	//   . cela permet d'avoir un estimation "anytime" de la memoire necessaire pour le coclustering
	// On renvoie en sortie le nombre max de cellules de la grille initiale
	boolean CheckMemoryForDataGridInitialization(KWDatabase* database, int nTupleNumber, int& nMaxCellNumber) const;

	// Verification de la memoire necessaire pour optimiser le coclustering, la grille initiale etant construite
	boolean CheckMemoryForDataGridOptimization(KWDataGrid* inputInitialDataGrid) const;

	///////////////////////////////////////////////////////////////////////////
	// Pilotage de l'optimisation anytime

	// Debut et fin du pilotage anytime
	void AnyTimeStart() const;
	void AnyTimeStop() const;

	// Construction d'un nom de fichier de sauvegarde temporaire
	const ALString AnyTimeBuildTemporaryReportFileName(int nIndex) const;

	///////////////////////////////////////////////////////////////////////////
	// Gestion des resultats principaux du coclustering en vue de constitution
	// d'un rapport de coclustering

	// Nettoyage des resultats de coclustering
	void CleanCoclusteringResults();

	// Calcul de statistiques descriptives par attribut (KWDescriptiveStats)
	// stockees par nom d'attribut dans le dictionnaire en sortie
	// Memoire: le dictionnaire en sortie est passe par l'appelant et son
	// contenu, cree par l'appele, appartient a l'appelant
	void ComputeDescriptiveAttributeStats(const KWTupleTable* tupleTable,
					      ObjectDictionary* odOutputDescriptiveStats) const;

	// Calcul de toutes les infos de hierarchie
	// Pilotage de toutes les methodes detaillees
	void ComputeHierarchicalInfo(const KWDataGrid* inputInitialDataGrid, const KWDataGridCosts* dataGridCosts,
				     CCHierarchicalDataGrid* optimizedDataGrid) const;

	// Calcul de la typicalite des attributs
	void ComputeAttributeTypicalities(CCHierarchicalDataGrid* optimizedDataGrid) const;

	// Memorisation des bornes des attributs Continuous
	void ComputeContinuousAttributeBounds(CCHierarchicalDataGrid* optimizedDataGrid) const;

	// Calcul de la typicalite des valeurs des attributs
	void ComputeValueTypicalities(const KWDataGrid* inputInitialDataGrid, const KWDataGridCosts* dataGridCosts,
				      CCHierarchicalDataGrid* optimizedDataGrid) const;
	void ComputeValueTypicalitiesAt(const KWDataGrid* inputInitialDataGrid, const KWDataGridCosts* dataGridCosts,
					CCHierarchicalDataGrid* optimizedDataGrid, int nAttribute) const;

	// Calcul de l'interet des parties
	// Le mergeur de grille en entree est a cet effet initialise avec le calcul de toutes les distances intra-partie
	// Le contenu des partie de la grille optimisee est mis a jour
	void ComputePartInterests(const KWDataGridMerger* optimizedDataGridMerger, const KWDataGridCosts* dataGridCosts,
				  CCHierarchicalDataGrid* optimizedDataGrid) const;

	// Calcul des hierarchies des parties parties, en creant de nouvelles parties pour les coder les hierarchies
	// La grille optimisee en entree sera enrichie avec les nouvelles parties des noeuds intermediaires de la
	// hierarchie Le mergeur de grille en entree est initialise avec le calcul de toutes les distances intra-partie,
	// et sera utilise pour effectuer des fusions recursives jusqu'a un grille terminale mono-cellule
	void ComputePartHierarchies(KWDataGridMerger* optimizedDataGridMerger, const KWDataGridCosts* dataGridCosts,
				    CCHierarchicalDataGrid* optimizedDataGrid) const;
	const ALString BuildHierachicalPartName(const CCHDGAttribute* hdgAttribute, int nHierarchicalIndex) const;

	// Calcul des rangs des parties, en minimisant un critere de distance entre parties adjacentes
	void ComputePartRanks(const KWDataGridMerger* optimizedDataGridMerger, const KWDataGridCosts* dataGridCosts,
			      CCHierarchicalDataGrid* optimizedDataGrid) const;

	// Numerotation (Ranks) des noeuds d'un arbre de partie de coclustering par parcours infixe
	void ComputePartInfixRanks(CCHDGPart* hdgRootgPart) const;

	// Tri des valeurs par typicalite decroissante pour les attributs categoriels, et initialisation du nom des
	// parties feuilles
	void SortAttributePartsAndValues(CCHierarchicalDataGrid* optimizedDataGrid) const;

	// Attributs d'effectif
	ALString sFrequencyAttribute;

	// Structure de cout de la grille
	KWDataGridCosts* coclusteringDataGridCosts;

	// Dictionnaire des statistiques descriptives par attribut
	ObjectDictionary odDescriptiveStats;

	// Grille de de donnees initiale au niveau de grain le plus fin
	KWDataGrid* initialDataGrid;

	// Grille de coclustering
	mutable CCHierarchicalDataGrid* coclusteringDataGrid;

	// Gestion des sauvegardes en mode anytime
	ALString sAnyTimeReportFileName;
	mutable ALString sLastActualAnyTimeReportFileName;
	mutable int nAnyTimeOptimizationIndex;
	mutable Timer tAnyTimeTimer;
	mutable double dAnyTimeDefaultCost;
	mutable double dAnyTimeBestCost;
	mutable boolean bIsDefaultCostComputed;

	// Export des rapport au format Khc
	boolean bExportAsKhc;
};

//////////////////////////////////////////////////////////////////////////////////
// Classe CCCoclusteringOptimizer
// Specialisation de l'optimisation des grille pour passer en mode anytime
class CCCoclusteringOptimizer : public KWDataGridOptimizer
{
public:
	// Constructeur
	CCCoclusteringOptimizer();
	~CCCoclusteringOptimizer();

	// Parametrage du contexte de gestion de la partie anytime de l'optimisation
	void SetCoclusteringBuilder(const CCCoclusteringBuilder* builder);
	const CCCoclusteringBuilder* GetCoclusteringBuilder();

	// Methode appelee lors de l'optimisation a chaque etape d'optimisation ameliorant la solution
	// Reimplementation de la methode virtuelle de la classe mere
	void HandleOptimizationStep(const KWDataGrid* optimizedDataGrid, const KWDataGrid* initialGranularizedDataGrid,
				    boolean bIsLastSaving) const override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	const CCCoclusteringBuilder* coclusteringBuilder;
};