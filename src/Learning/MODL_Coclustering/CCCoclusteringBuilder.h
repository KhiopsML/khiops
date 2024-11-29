// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class CCCoclusteringBuilder;

#include "PLDatabaseTextFile.h"
#include "KWTupleTable.h"
#include "KWTupleTableLoader.h"
#include "KWAttributeSubsetStats.h"
#include "KWDataGridMerger.h"
#include "KWDataGridPostOptimizer.h"
#include "KWDataPreparationClass.h"
#include "CCHierarchicalDataGrid.h"
#include "CCAnalysisSpec.h"
#include "CCCoclusteringReport.h"
#include "CCCoclusteringSpec.h"
#include "CCVarPartCoclusteringSpec.h"

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

	// Type de coclustering
	// A true pour un coclustering de type VarPart, instances * variables
	// A false sinon pour un coclustering de variables (valeur par defaut)
	boolean GetVarPartCoclustering() const;
	void SetVarPartCoclustering(boolean bValue);

	/////////////////////////////////////////////////////////////////////////////
	// Specification dans le cas d'un coclustering de variables
	// Les variables sont specifiees dans la classe ancetre KWAttributeSubsetStats

	// Variable d'effectif (optionnelle)
	// Chaque enregistrement est pondere (selon un nombre entier positif) par le contenu de cette variable
	// lors de la creation de la grille initiale
	const ALString& GetFrequencyAttributeName() const;
	void SetFrequencyAttributeName(const ALString& sValue);

	/////////////////////////////////////////////////////////////////////////////
	// Specification dans le cas d'un coclustering instances x variables

	// Variable d'identifiant (optionnelle)
	// Pour un coclustering instances * variables, permet de renseigner la variable d'identifiant des instances
	// Sinon, cette variable est creee automatiquement
	const ALString& GetIdentifierAttributeName() const;
	void SetIdentifierAttributeName(const ALString& sValue);

	// Variable de type VarPart, proprietaire des variables internes,
	// dans le cas d'un coclustering instances * variables
	const ALString& GetVarPartAttributeName() const;
	void SetVarPartAttributeName(const ALString& sValue);

	// Nombre d'attributs de grille dans le cas instances x variables, c'est a dire deux
	int GetVarPartCoclusteringAttributeNumber() const;

	// Vecteur des noms de variables internes, exploitee par la variable de type VarPart
	// dans le cas d'un coclustering instances * variables
	StringVector* GetInnerAttributesNames();

	/////////////////////////////////////////////////////////////////////////////
	// Exploitation des specifications du coclustering, principalement le calcul du modele

	// Verification de la validite des specifications
	boolean CheckSpecifications() const override;

	// Verification de la validite des specifications dans le cas standard coclustering de variables
	boolean CheckStandardSpecifications() const;

	// Verification de la validite des specifications dans le cas coclustering instances x variables
	boolean CheckVarPartSpecifications() const;

	// Calcul du coclustering, renvoie false en cas d'erreur ou d'interruption utilisateur
	boolean ComputeCoclustering();
	boolean IsCoclusteringComputed() const;

	// Test si le coclustering est calcule et informatif (au moins deux dimensions)
	boolean IsCoclusteringInformative() const;

	// CH IV Begin
	// Creation d'une structure de cout pour le probleme de coclustering, standard ou VarPart
	// Memoire: appartient a l'appelant
	KWDataGridCosts* CreateDataGridCost() const override;

	// Creation d'une grille avec une dimension de type VarPart
	// La dimension VarPart contient un cluster de parties de variable pour chaque partie de
	// variable de chaque attribut interne
	// L'effectif de la variable identifiant est alimente par le vecteur ivObservationNumbers
	KWDataGrid* CreateVarPartDataGrid(const KWTupleTable* tupleTable, ObjectDictionary& odObservationNumbers);

	// Nettoyage des eventuelles parties de variables vides du fait d'observations manquantes
	void CleanVarPartDataGrid(KWDataGrid* dataGrid);

	// Alimentation des cellules d'un VarPartDataGrid dont les attributs et parties sont correctement initialises,
	// Renvoie true si cellule correctement initialisee, false sinon (sans nettoyage des celulles crees)
	// Pour la dimension VarPart, on parcourt l'ensemble des attributs internes pour alimenter les cellules associees a chaque observation
	boolean CreateVarPartDataGridCells(const KWTupleTable* tupleTable, KWDataGrid* dataGrid);

	/////////////////////////////////////////////////////////////////////////////
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

	// Supression du dernier fichier temporaire sauvegarde
	void RemoveLastSavedReportFile() const;

	// Methode redefinie, appelee lors de l'optimisation a chaque etape d'optimisation
	// A chaque amelioration, un nouveau fichier de sauvegarde est cree avec un index croissant,
	// le fichier precedent etant detruit.
	// Un message utilisateur est egalement emis.
	// Transmission de la grille initiale granularisee
	// bIsLastSaving : si true, la sauvegarde est effectue meme s'il n'y a pas amelioration
	// Permet de recalculer la hierarchie du coclustering apres l'atteinte de la granularite maximale
	void HandleOptimizationStep(const KWDataGrid* optimizedDataGrid, const KWDataGrid* initialGranularizedDataGrid,
				    boolean bIsLastSaving) const override;

	// Libelles utilisateur: nom du module de l'application (GetLearningModuleName())
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Methode virtuelle d'optimisation d'une grille
	virtual void OptimizeDataGrid(const KWDataGrid* inputInitialDataGrid, KWDataGrid* optimizedDataGrid);

	// Initialisation d'un optimiseur de grille dedie coclustering
	void InitializeDataGridOptimizer(const KWDataGrid* inputInitialDataGrid,
					 KWDataGridOptimizer* dataGridOptimizer);

	// CH IV Begin
	// Methode d'optimisation d'une grille dediee au cas instances x variables
	void OptimizeVarPartDataGrid(const KWDataGrid* inputInitialDataGrid, KWDataGrid* optimizedDataGrid);
	void PROTO_OptimizeVarPartDataGrid(const KWDataGrid* inputInitialDataGrid, KWDataGrid* optimizedDataGrid);
	// CH IV End

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

	// CH IV Begin
	// Cas du coclustering avec attribut de type VarPart
	// En plus d'une table de tuples comme dans FillTupleTableFromDatabase, on alimente egalement un vecteur
	// qui associe a chaque tuple le nombre d'observations dans l'attribut de type VarPart
	// Un tuple est ecarte si son attribut identifiant n' est pas renseigne ou si aucune valeur n'est renseignee
	// pour les attributs internes En sortie, le dictionnaire odObservationNumbers contient pour chaque modalite de
	// l'identifiant, le nombre d'observations stocke dans un IntObject
	boolean FillVarPartTupleTableFromDatabase(KWDatabase* database, KWTupleTable* tupleTable,
						  ObjectDictionary& odObservationNumbers);

	// Creation de la partition d'un attribut de DataGrid de type Identifiant dans un coclustering Identifiant *
	// Parties de variables En entree, le dictionnaire odObservationNumbers contient pour chaque modalite de
	// l'identifiant, le nombre d'observations Ces effectifs permettent d'initialiser les effectifs de l'attribut
	// Cas d'un attribut Identifier de type Symbol
	boolean CreateIdentifierAttributeValueSets(const KWTupleTable* tupleTable, KWDGAttribute* dgAttribute,
						   ObjectDictionary& odObservationNumbers);

	// Cas d'un attribut Identifier de type Continuous
	boolean CreateIdentifierAttributeIntervals(const KWTupleTable* tupleTable, KWDGAttribute* dgAttribute,
						   ObjectDictionary& odObservationNumbers);

	// Renvoie le nombre d'observations associe a un enregistrement, avec eventuellement affichage de warning
	// Renvoie 0 si l'enregistrement est non utilisable (valeur manquante pour l'attribut Identifiant ou aucune
	// observation) L'attribut Identifiant est exclu du calcul du nombre d'observations
	int GetDatabaseObjectObservationNumber(const KWObject* kwoObject, longint lRecordIndex,
					       const KWAttribute* identifierAttribute,
					       const ObjectArray* oaInnerAttributes);
	// CH IV End

	// Renvoie l'effectif associe a un enregistrement, avec eventuellement affichage de warning
	// Renvoie 1 si l'attribut d'effectif est NULL
	// Renvoie 0 si erreur dans la specification de l'effectif
	int GetDatabaseObjectFrequency(const KWObject* kwoObject, longint lRecordIndex,
				       const KWAttribute* frequencyAttribute);

	// Verification de la memoire necessaire pour construire une grille initiale a partir d'un nombre de tuples
	// La base en entree peut etre entierement traitee:
	//   . dans ce cas, on dispose des statistique descriptives et donc du nombre de valeurs par attribut
	// ou en cours de lecture, pour une alimnettaion partielle de la table de tuples
	//   . dans ce cas, les nombres de valeurs par attribut sont estimes
	//   . cela permet d'avoir un estimation "anytime" de la memoire necessaire pour le coclustering
	// On renvoie en sortie le nombre max de cellules de la grille initiale
	boolean CheckMemoryForDataGridInitialization(KWDatabase* database, int nTupleNumber, int& nMaxCellNumber) const;

	// CH IV Begin
	// Verification de la memoire necessaire pour construire une grille initiale de type VarPart a partir d'un nombre de tuples
	// On renvoie en sortie le nombre max de cellules de la grille initiale
	boolean CheckMemoryForVarPartDataGridInitialization(KWDatabase* database, int nTupleNumber,
							    int& nMaxCellNumber) const;
	// CH IV End

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

	// Type de coclustering
	boolean bVarPartCoclustering;

	// Attributs d'effectif
	ALString sFrequencyAttributeName;

	// Attribut d'identifiant
	ALString sIdentifierAttributeName;

	// Nom de la variable de type VarPart
	ALString sVarPartAttributeName;

	// Nom des variables internes
	StringVector svInnerAttributeNames;

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
};
