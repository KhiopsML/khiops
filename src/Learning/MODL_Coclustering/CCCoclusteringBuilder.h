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
#include "CCCoclusteringSpec.h"
// CH IV Refactoring: rennommer en CCVarPartCoclusteringSpec
#include "CCInstancesVariablesCoclusteringSpec.h"

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

	// CH IV Begin
	// Variable d'identifiant (optionnelle)
	// Pour un coclustering individus * variables, permet de renseigner la variable d'identifiant des individus
	// Sinon, cette variable est creee automatiquement
	const ALString& GetIdentifierAttribute() const;
	void SetIdentifierAttribute(const ALString& sValue);

	// Type de coclustering
	// A true pour un coclustering generique de type individus * varialbes
	// A false sinon pour un coclustering variable * variable (valeur par defaut)
	// CH IV Refactoring: rennommer en Set|GetVarPartCoclustering
	boolean GetGenericCoclustering() const;
	void SetGenericCoclustering(boolean bValue);
	// CH IV End

	// Verification de la validite des specifications
	boolean CheckSpecifications() const override;

	// Calcul du coclustering, renvoie false en cas d'erreur ou d'interruption utilisateur
	boolean ComputeCoclustering();
	boolean IsCoclusteringComputed() const;

	// Test si le coclustering est calcule et informatif (au moins deux dimensions)
	boolean IsCoclusteringInformative() const;

	// CH IV Begin

	// CH IV Refactoring: a clarifier (uniquement les attributs VarPart, ceux la plus l'Identifier, quid du
	// coclustering classique?)

	// Nombre d'attributs
	// Redefinition qui permet de definir la taille du vecteur des noms d'attributs et du vecteur des noms d'axes de
	// chaque attribut A ce stade, les noms d'axes sont redondants si plusieurs attributs partagent le meme axe
	void SetAttributeNumber(int nValue);

	// Parametrage des axes
	// Memoire: les specifications des axes sont a gerer par l'appelant
	// Acces au nom de l'axe d'un attribut
	// Pour un attribut Simple (numérique ou catégoriel), le nom est vide
	// Pour un attribut de type Parties de variables, contient le nom de l'axe
	void SetAxisForAttributeNameAt(int nIndex, const ALString& sAxisName);
	const ALString& GetAxisForAttributeNameAt(int nIndex) const;

	// Acces aux noms d'axes (sans redondance)
	const ALString& GetAxisNameAt(int nIndex) const;

	// Acces au nombre d'attributs impliques par axe
	// CH IV Refactoring: renommer en GetVarPartAttributeNumberAt
	// CH IV Refactoring: clarifier, il suffit d'indiquer quel est l'index de l'aaxe portant les VarPart attributes?
	int GetImpliedAttributeNumberAt(int nIndex) const;

	// Extraction et verification des noms des axes
	// L'extraction est effectuee en supprimant les noms redondants
	// Les noms vides sont consideres comme correspondant a des axes de type numerique ou categoriel
	// Un axe avec un nom non vide doit etre partage par au moins deux attributs successifs
	// Renvoie faux si le Check a echoue
	boolean ExtractAndCheckAxisNames();

	// Nombre d'axes distincts
	int GetAxisNumber() const;

	// Creation d'une structure de cout pour le probleme de coclustering generique
	// Memoire: appartient a l'appelant
	KWDataGridCosts* CreateDataGridCost() const;

	// Traite le cas generique ou les attributs sont de type Simple ou de type parties de variables
	// L'axe VarParts cree contient un cluster de parties de variable pour chaque partie de variable de chaque
	// attribut implique L'effectif de la variable identifiant est alimente par le vecteur ivObservationNumbers CH
	// IV Refactoring: renommer en CreateVarPartDataGrid
	KWDataGrid* CreateGenericDataGrid(const KWTupleTable* tupleTable, ObjectDictionary& odObservationNumbers);

	// Nettoyage des eventuelles parties de variables vides du fait d'observations manquantes
	// CH IV Refactoring: renommer en CleanVarPartDataGrid
	void CleanGenericDataGrid(KWDataGrid* dataGrid);

	// Alimentation des cellules d'un DataGrid dont les attributs et parties
	// sont correctement initialises
	// Renvoie true si cellule correctement initialisee, false sinon (sans nettoyage des celulles crees)
	// Pour les attributs de type VarParts, on parcourt l'ensemble des attributs impliques de l'ensemble de ces
	// attributs pour alimenter les cellules associees a chaque observation (l'observation d'un attribut implique
	// par axe de type VarParts) CH IV Refactoring: passer en override de la methode ancetre? OK
	//   verifier egalement pour les autres methodes CreateGenericDataGrid, CreateGenericDataGrid
	//    qui pourrait etre des redefintions des methodes de la classe ancetre?
	boolean CreateDataGridCells(const KWTupleTable* tupleTable, KWDataGrid* dataGrid);
	// CH IV End

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

	// CH IV Begin
	// CH IV Refactoring: renommer en OptimizeVarPartDataGrid
	void OptimizeGenericDataGrid(const KWDataGrid* inputInitialDataGrid, KWDataGrid* optimizedDataGrid,
				     CCCoclusteringOptimizer dataGridOptimizer);
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
	// Cas du coclustering generique avec attribut de type VarPart
	// En plus d'une table de tuples comme dans FillTupleTableFromDatabase, on alimente egalement un vecteur
	// qui associe a chaque tuple le nombre d'observations dans l'attribut de type VarPart
	// Un tuple est ecarte si son attribut identifiant n' est pas renseigne ou si aucune valeur n'est renseignee
	// pour les attributs impliques En sortie, le dictionnaire odObservationNumbers contient pour chaque modalite de
	// l'identifiant, le nombre d'observations stocke dans un IntObject CH IV Refactoring: renommer en
	// FillVarPartTupleTableFromDatabase
	boolean FillGenericTupleTableFromDatabase(KWDatabase* database, KWTupleTable* tupleTable,
						  ObjectDictionary& odObservationNumbers);

	// CH IV Refactoring: terminologie Instances x Variables a unifier
	//   VarPart: type d'un axe de grille, et terme generique de typa adjectif
	//   VarPartDataGrid: grille ayant un axe de type VarPart
	//   VarPartAttribute: attribute simple utilise dans un axe de type VarPart
	//   VarPartAttributePart: partie d'un VarPartAttribute,
	//   Identifier (InstanceIdentifier?)
	//   Axis: attribut d'un grille, terminologie nouvelle depuis l'introduction des VarPart
	//         pour differencier les attributs de type VarPart, et les VarPartAttributes qui les composent
	//   VarPartAxis: Axis de type VarPart
	//   Observation: lie a un couple (instance, valeur de variable)
	//     pour chaque instance, on a autant d'observations que de variables npn manquantes
	//   ...
	// CH IV Refactoring: OK, a deplacer et unifier dans l'entete de KWDataGrid

	// Creation de la partition d'un attribut de DataGrid de type Identifiant dans un coclustering Identifiant *
	// Parties de variables En entree, le dictionnaire odObservationNumbers contient pour chaque modalite de
	// l'identifiant, le nombre d'observations Ces effectifs permettent d'initialiser les effectifs de l'attribut
	// Cas d'un attribut Identifier de type Symbol
	boolean CreateIdentifierAttributeValueSets(const KWTupleTable* tupleTable, KWDGAttribute* dgAttribute,
						   ObjectDictionary& odObservationNumbers);

	// Cas d'un attribut Identifier de type Continuous
	boolean CreateIdentifierAttributeIntervals(const KWTupleTable* tupleTable, KWDGAttribute* dgAttribute,
						   ObjectDictionary& odObservationNumbers);

	// CH AB AF obsolete quand le calcul exact du nombre d'observations sera effectue
	// Alimentation d'un tableau de vecteur d'indexes decrivant toutes les observations generees
	// Traite le cas de plusieurs axes de type VarParts
	// CH IV Refactoring: obsolete???
	// CH IV Refactoring: renommer en FillObjectArrayVarPartAttributesIndexes
	void FillObjectArrayVarPartsAttributesIndexes(ObjectArray* oaIndexes, int nFirstVarPartsAttributeIndex,
						      KWDataGrid* dataGrid);

	// Renvoie le nombre d'observations associe a un enregistrement, avec eventuellement affichage de warning
	// Renvoie 0 si l'enregistrement est non utilisable (valeur manquante pour l'attribut Identifiant ou aucune
	// observation) L'attribut Identifiant est exclu du calcul du nombre d'observations
	int GetDatabaseObjectObservationNumber(KWObject* kwoObject, KWLoadIndex liFrequencyAttributeLoadIndex,
					       longint lRecordIndex, KWLoadIndexVector& livLoadIndexes);
	// CH IV End

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

	// CH IV Begin
	// Attribut d'identifiant
	ALString sIdentifierAttribute;

	// Type de coclustering
	// CH IV Refactoring: renommer en bVarPartCoclustering
	boolean bGenericCoclustering;
	// CH IV End

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

	// Export des rapports au format Khc
	boolean bExportAsKhc;

	// CH IV Begin
	// Nom des axes (sans redondance)
	StringVector svAxisNames;

	// Nombre d'attributs impliques par axe
	// Egal a 1 pour un axe de type autre que Parties de variables
	// Est > 1 pour un axe de type Parties de variables
	// CH IV Refactoring: renommer en ivVarPartAttributesNumber
	IntVector ivImpliedAttributesNumber;

	// Nom d'axe pour chaque attribut (avec redondance)
	StringVector svAxisForAttributeNames;

	// Index du premier attribut de type VarParts
	// CH IV Refactoring: cf clarification du cas d'un axe (le seul) avec des VarParts
	int nFirstVarPartsAttributeIndex;
	// CH IV End
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
