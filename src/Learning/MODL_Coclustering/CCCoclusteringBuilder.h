// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class CCCoclusteringBuilder;

#include "PLDatabaseTextFile.h"
#include "KWTupleTable.h"
#include "KWTupleTableLoader.h"
#include "KWAttributeSubsetStats.h"
#include "KWDataGridMerger.h"
#include "KWDataGridOptimizerVxV.h"
#include "KWDataGridOptimizerIxV.h"
#include "KWDataGridPostOptimizer.h"
#include "CCHierarchicalDataGrid.h"
#include "CCAnalysisSpec.h"
#include "CCCoclusteringReport.h"
#include "CCCoclusteringSpec.h"

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

	// Creation d'une structure de cout pour le probleme de coclustering, standard ou VarPart
	// Memoire: appartient a l'appelant
	KWDataGridCosts* CreateDataGridCost() const override;

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
	void HandleOptimizationStep(const KWDataGrid* optimizedDataGrid,
				    const KWDataGrid* initialGranularizedDataGrid) const override;

	// Libelles utilisateur: nom du module de l'application (GetLearningModuleName())
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Optimisation d'une grille
	// Le parametre optimizedDataGrid contient le meilleur resultat en sortie
	// A notrer que la meilleure grille coclusteringDataGrid est actualisee au fur et
	// a mesure de l'optimistaion de facon anytime via la methode HandleOptimizationStep
	void OptimizeDataGrid(const KWDataGrid* inputInitialDataGrid, KWDataGrid* optimizedDataGrid);

	// Initialisation d'un optimiseur de grille dedie coclustering
	void InitializeDataGridOptimizer(const KWDataGrid* inputInitialDataGrid,
					 KWDataGridOptimizer* dataGridOptimizer);

	///////////////////////////////////////////////////////////////////////////////////////
	// Service de creation de grille initiale dans le cas standard VxV
	//
	// Gestion des valeurs manquantes
	// - les valeurs manquantes sont gardees et traitees comme les autres valeurs
	//
	// Gestion de la memoire
	// - la premiere etape consiste en la lecture de la base, avec des verifications memoire au fil de l'eau
	// - les etapes suivante construisent la grille initiale progressivement (attributs, tuples, parties, cellules,
	//   avec des verification memoire au fil de l'eau
	//   - ces estimations sont  parfois dans le pire des cas, mais ce n'est pas un probleme etant donne
	//     que l'etape finale d'optimisation sera tres exigente en memoire
	// - l'etape finale, une fois la grille initiale construite, consiste a estimer finement une fois pour toutes
	//   la memoire necessaire pour l'ensemble des donnes de travail pour l'optimisation de la grille
	// Les methodes retournent false en cas d'erreur ou de probleme memoire, avec des message d'erreur

	// Creation d'une grille initiale de type variables x variables
	// Renvoie false avec message d'erreur si echec, avec initialDataGrid restant a NULL
	boolean CreateStandardInitialDataGrid();

	// Alimentation d'une table de tuples comportant les attributs a analyser a partir de la base
	// en tenant compte de l'eventuel attribut d'effectif par enregistrement
	// Les objets de la base doivent etre charges en memoire. Ils sont liberes au fur et a mesure de leur traitement
	// pour liberer la memoire des que leur contenu est transfere dans un tuple.
	boolean FillStandardTupleTableFromDatabase(KWDatabase* database, KWTupleTable* tupleTable);

	// Renvoie l'effectif associe a un enregistrement, avec eventuellement affichage de warning
	// Renvoie 1 si l'attribut d'effectif est NULL
	// Renvoie 0 si erreur dans la specification de l'effectif
	int GetDatabaseObjectFrequency(const KWObject* kwoObject, const KWAttribute* frequencyAttribute);

	///////////////////////////////////////////////////////////////////////////////////////
	// Service de creation de grille initiale dans le cas VarPart IxV
	//
	// Gestion des valeurs manquantes
	// - les valeurs manquantes sont ignorees et ne font pas partie des observations retenues
	// - pour les variables sparses, c'est la valeur par defaut du bloc qui est ignoree,
	//   qu'elle soit la valeur manquante ou tout autre valeur (e.g. 0)
	// - les variables internes vides sont ignorees

	// Creation d'une grille de type instances x variables
	// La dimension VarPart contient un cluster de parties de variable pour chaque partie de
	// variable de chaque attribut interne
	// Renvoie false avec message d'erreur si echec, avec initialDataGrid restant a NULL
	boolean CreateVarPartInitialDataGrid();

	// Creation d'une grille vide de type instances x variables, avec uniquement ses dimensions
	KWDataGrid* CreateVarPartEmptyDataGrid();

	// Creation des parties de l'attribut identifiant, sans mettre a jour les effectifs
	// Mise a jour des statistique descriptives correspondantes
	// La base entre doit etre chargee en memoire
	// On considere ici qu'il n'y a pas de risque de depassement memoire, suite au test ReadDatabaseAndCheckRemainingMemory
	boolean InitializeIdentifierAttributeParts(KWDatabase* database, KWDGAttribute* identifierAttribute,
						   ObjectDictionary* odOutputDescriptiveStats);

	// Creation des attributs internes et des parties elementaires de l'attribut de type VarPart,
	// Mise a jour des statistique descriptives correspondantes
	// La base entre doit etre chargee en memoire
	// Les blocs de variables sont exploites de facon sparse pour creer efficacement les attributs internes
	// Des verifications de depasssement des capacites memoire sont effectuees a chaque variable ou bloc de variables
	boolean InitializeVarPartAttributeParts(KWDatabase* database, KWDGAttribute* varPartAttribute,
						ObjectDictionary* odOutputDescriptiveStats);

	// Creation d'un attribut interne a partir d'un attribut et de la table de tuple correspondante chargee en memoire
	// Les statistiques descriptives sont mises a jour systematiquement
	// L'attribut interne n'est cree que s'il est non vide, et range dans le dictionnaire en sortie
	// La table de tuple est videe en sortie de methode
	// On renvoie l'attribut interne s'il est cree, NULL sinon
	KWDGAttribute* CreateInnerAttribute(const KWAttribute* attribute, KWDGAttribute* varPartAttribute,
					    KWTupleTable* tupleTable, ObjectDictionary* odInnerAttributes,
					    ObjectDictionary* odOutputDescriptiveStats);

	// Creation des attributs internes et des partie elementaire de l'attribut de type VarPart,
	// avec mise a jour des statistique descriptives correspondantes
	// La base entre doit etre chargee en memoire
	// Elle est detruite au fur et a mesure de la creation des cellule et est rendues vide
	// Les blocs de variables sont exploites de facon sparse pour creer efficacement les cellules
	// Des verifications de depasssement des capacites memoire sont effectuees regulierement
	boolean InitializeVarPartCells(KWDatabase* database, KWDataGrid* dataGrid);

	// Supression d'une table de tuples univariee de l'eventuel tuple comportant une valeur donnee
	void RemoveTupleWithMissingContinuousValue(KWTupleTable* tupleTable, Continuous cValue) const;
	void RemoveTupleWithMissingSymbolValue(KWTupleTable* tupleTable, Symbol sValue) const;

	///////////////////////////////////////////////////////////////////////////////////////
	// Services commun aux methodes de creation de la grille initiale
	// Gestion preventive de l'utilisation des ressources memoire, avec message d'erreur
	// Apres lecture de la base, chaque etape de construction de la grille initiale (creation des attributs, parties, cellules)
	// est effectuee avec suivi de tache, controle de la memoire utilisee au fil de l'eau, et gestion des interruptions utilisateurs

	// Lecture de la base de donnees
	//
	// La base est entierement lue avant la construction de la grille initiale.
	// Bien qu'il soit possible de construire la grille au fur et a mesure de la lecture
	// pour economiser de la memoire, cette approche n'est pas retenue pour les raisons suivantes :
	// - Estimer la memoire necessaire avant la lecture elle-meme est quasiment impossible
	//   dans le cas general en raison des nombreuses incertitudes
	//   - critere de selection
	//   - valeurs categrorielle ou texte de taille inconnue
	//   - donnees sparses
	//   - donnes multi-table avec table secondaires et table externes
	//   - valeur calculees par des regles de derivation
	//   - ...
	// - La majorite des bases occupent generalement moins de memoire que la grille elle-meme.
	//    - Il est rare qu'une base depasse la memoire necessaire pour la grille.
	//    - En pratique, la memoire utilisee par la base est souvent inferieure a celle de la grille,
	//      avec un facteur d'au plus 2.
	// - La lecture en une seule passe rend l'implementation plus modulaire et plus facile a maintenir.
	// - La lecture est deleguee a la classe KWDatabase, qui gere tous les traitements d'erreur
	//   et la gestion preventive de la memoire au fil de l'eau.
	//
	// Retourne false en cas d'erreur ou si la memoire disponible est insuffisante pour construire la grille initiale.
	boolean ReadDatabaseAndCheckRemainingMemory(KWDatabase* database) const;

	// Calcul du nombre de valeurs exacts restant dans la base, en tenant compte de blocs sparses
	// Attention: il s'agit des valeurs non forcement distinctes
	// Renvoie 0 en cas d'interruption utilisateur
	longint ComputeDatabaseTotalRemainingValueNumber(KWDatabase* database, int nStartObjectIndex) const;

	// Calcul du nombre de valeurs dans la base pour un bloc sparse donne
	// Attention: il s'agit des valeurs non forcement distinctes
	// Renvoie 0 en cas d'interruption utilisateur
	longint ComputeDatabaseBlockValueNumber(KWDatabase* database, KWAttributeBlock* attributeBlock) const;

	// Verification de la memoire necessaire pour construire une grille initiale a partir d'un nombre de tuples,
	// qui fournit l'effectif total et le nombre de cellules, dans le cas standard variables x variables
	boolean CheckMemoryForStandardDataGridInitialization(const KWTupleTable* tupleTable) const;

	// Verification de la memoire necessaire pour optimiser le coclustering, la grille initiale etant construite
	// Prise en compte des deux cas: variables x variables et instances x variables
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
	// Methode avec suivi de tache
	// En cas d'interruption ou d'erreur, on renvoie false
	// en emettant une erreur et on detruit les statistiques descriptives
	// Memoire: le dictionnaire en sortie est passe par l'appelant et son
	// contenu, cree par l'appele, appartient a l'appelant
	boolean ComputeDescriptiveAttributeStats(const KWTupleTable* tupleTable,
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

	// Grille de coclustering resultat
	mutable CCHierarchicalDataGrid* coclusteringDataGrid;

	// Gestion des sauvegardes en mode anytime
	ALString sAnyTimeReportFileName;
	mutable ALString sLastActualAnyTimeReportFileName;
	mutable int nAnyTimeOptimizationIndex;
	mutable Timer tAnyTimeTimer;
	mutable double dAnyTimeBestCost;
};
