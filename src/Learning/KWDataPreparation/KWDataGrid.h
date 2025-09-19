// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataGrid;
class KWDGAttribute;
class KWDGPart;
class KWDGInterval;
class KWDGValueSet;
class KWDGValue;
class KWDGCell;
class KWDGPartValues;
class KWDGSymbolValueSet;
class KWDGSymbolValue;
class KWDGVarPartSet;
class KWDGVarPartValue;
class KWDGInnerAttributes;

#include "KWVersion.h"
#include "SortedList.h"
#include "KWSymbol.h"
#include "KWDataGridStats.h"
#include "KWSortableIndex.h"
#include "KWFrequencyVector.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGrid
//
// Deux familles de grille de donnees
//   - coclustering de variables "standard"
//      - estimateur de densite jointe ou conditionnelle entre variables
//      - multivarie, avec variable numeriques ou categorielles
//      - Structure de donnees permettant de representer l'ensemble des enregistrements
//        d'une base de donnees, sous forme d'un produit cartesien des attributs.
//        - Ce produit cartesien correspond a un tableau multi-dimensionnel creux.
//        - la DataGrid permet un stockage efficace des n-uplets instancies, en maintenant
//          le lien entre les attributs, leurs valeurs, et les n-uplets de valeurs.
//        - cette structure est particulierement adaptee aux algorithmes de groupage multidimensionnels.
//      - terminologie
//         - modele en grile ou grille de donnees, variable, partie (intervalles, groupe de valeurs), valeur, cellule
//         - DataGrid, Attribute, part (Interval, ValueGroup), Value, Cell
//
//   - coclustering instances x variables (cf. these de Aichetou)
//      - pour l'analyse exploratoire d'une base de donnees, en groupa
//      - extension des data grid, avec un des attribut de type VarPart (extension sous forme de parametrage additionnel
//      de la meme classe)
//        - un attribut de la grille peut etre soit numerique, soit categoriel, soit constitue de parties de variables
//        - dans la these d'Aichetou
//          - dedie au cas instances x variables
//          - chaque variable descriptive est partitionnee en parties, intervalles ou groupes de valeurs
//          - chaque partie de variable est l'analogue d'un mot
//          - chaque instance est decrite par l'ensemble des mots correspondant aux valeurs de ses variables
//          descriptives
//        - on a une grille a deux dimensions, analogue au cas d'une grille textes x mots
//          - un attribut de la grille contient des groupes d'instances (textes), par leur identifiant
//          - l'autre attribut de la grille contient des groupes de parties de variables (mots)
//      - terminologie
//         VarPart
//           - nouveau type, pour un attribut de DataGrid
//           - extension du type Symbol, ou chaque partie de variable joue le role d'une valeur
//           - adjectif utilise pour specialiser les extensions des DataGrid quand elles ont un attribut de type VarPart
//         VarPartDataGrid
//           - extension d'un DataGrid, avec un attribut de type VarPart
//           - un et un seul attribut de type VarPart, plus un a plusieurs autres attributs
//           - le plus standard: un autre attribut de type Symbol, portant les identifiants des instances
//             - on se rapproche du modele standard "Instances x Variables"
//             - dans ce cas, on peut nommer cet attribut le IdentifierAttribute
//           - extensions, avec un a plusieurs attributs de type Symbol ou Continuous
//             - a explorer: IdSource x IdTarget dans le cas de l'analyse des arcs d'un graphe avec des variables par
//             arcs
//             - a explorer: Time * Identifier dans d'instances evoluant au cours du temps
//             - pas de terminologie particuliere pour ces extensions (autre que attributs de Symbol ou Continuous)
//         Attribute
//           - cas general: un attribut de DataGrid
//         VarPartAttribute
//           - attribut de type VarPart
//             - ses valeurs sont des VarPartValues
//             - ses groupes de valeurs sont des VarPartGroups
//             - les attributs correspondant aux VarPartValues sont ses InnerAttributes
//               - un InnerAttribute est de type Symbol ou Continuous
//               - pour un InnerAttribute, son attribut de type VarPart de la DataGrid est son OwnerAttribute
//      - terminologie interne en francais, pour des etudes/discussions techniques
//           - grille de donnees (ou coclustering)
//           - attribut de grille (ou axe, dimension)
//           - attribut interne (Inner)
//           - attribut proprietaire (Owner)
//      - terminologie externe, pour communiquer et expliquer aux utilisateurs
//           - coclustering, cluster, cocluster
//           - coclustering de variables
//             - chaque cluster est une Part, Interval ou Value group
//           - coclustering instances x variables
//             - clusters d'instances vs clusters de parties de variables
//             - on peut parler de dimension instances et dimension partie de variables
//             - les parties de variables sont des Part, Interval ou Value group
//
// Coclustering dans la litterature
//    - coclustering de matrice
//      - bivarie, en general categoriel, lie aux matrices
//      - terminologie
//        - coclustering, block clustering, two-mode clustering, cross-clustering, bi-clustering
//        - biclusters, coclusters, block, sub-matrix
//        - bipartite, array, matrix
//        - rows, columns
//    - coclustering variable x variable
//       - peu courant hormis le coclustering de matrice
//       - rare pour des variables de type mixte, ou en multivarie au dela de deux variables
//    - coclustering instances x variables
//      - plus connu et courant que le coclustering de variables, souvent confondu
//      - toujours avec des variables d'un seul type, souvent numeriques
//      - souvent meme terminologie que coclustering de matrice, en considerant la matrice instances x variables
//      - terminologie supplementaire
//        - object, instance, variable
//        - cluster, subset, group
//        - dimension (of instances, of variables)
class KWDataGrid : public Object
{
public:
	// Constructeur
	KWDataGrid();
	~KWDataGrid();

	/////////////////////////////////////////////////////////////////////////
	// Specification des donnees d'un DataGrid
	// Un DataGrid est representee sous forme d'un graphe d'objets:
	//   - au premier niveau, un tableau d'attributs
	//   - un DataGrid est un VarPartDataGrid si l'un de ses attributs est de type VarPart
	// Chaque attribut est defini principalement par:
	//   - son type: numerique, categoriele ou partie de variables
	//      - specification des variables internes dans le cas partie de variables
	//	 - son nombre de valeurs
	//   - sa liste de parties
	// Chaque partie est definie principalement par:
	//   - sa liste de valeurs
	//   - sa liste de cellules
	// Chaque cellule est definie principalement par:
	//   - le vecteur des effectifs des classes cibles
	//   - le tableau (par attribut) des parties dont il est le produit cartesien
	//
	// La specification d'un DataGrid se fait selon les etapes suivantes:
	//   - nombre d'attributs et de classes cibles: Initialize
	//   - specification des attributs: GetAttributeAt, puis methodes des attributs
	//   - specification des parties: ajout et specifications depuis les attributs
	//   - specification des cellules: AddCell
	// La recherche du N-uplet de parties correspondant a un N-uplet de valeurs
	// est possible au moyen de methodes utilitaires des attributs permettant
	// d'indexer les parties par les valeurs
	//
	// Memoire: l'integralite de la structure du DataGrid est geree par l'objet courant

	// Initialisation du nombre d'attribut du DataGrid
	// Le nombre de classe cible est strictement positif uniquement dans le cas
	// de la classification supervisee sans groupement de valeur.
	// Il prend la valeur 0 sinon, dans le cas non supervise ou dans le cas
	// d'un attribut cible avec groupement de valeur (regression ou classification
	// avec valeurs cibles a grouper).
	// Les structures precedentes eventuelles sont detruites
	// Les operandes sont creees et initialisee dans un etat coherent
	void Initialize(int nAttributeNumber, int nTargetValueNumber);
	int GetAttributeNumber() const;
	int GetTargetValueNumber() const;

	// Ajout d'un attribut si la grille est vide, sans aucune cellule
	void AddAttribute();

	// Test si le DataGrid est dans un etat vide (identique a l'etat initial)
	boolean IsEmpty() const;

	// Parametrage des valeurs cibles (uniquement pour le reporting)
	void SetTargetValueAt(int nIndex, const Symbol& sValue);
	Symbol& GetTargetValueAt(int nIndex) const;

	// Acces aux attributs du DataGrid
	// L'appelant se sert de ces point d'entree pour creer la structure du DataGrid
	KWDGAttribute* GetAttributeAt(int nAttributeIndex) const;

	// Acces a l'attribut cible (s'il existe, c'est le dernier attribut de la grille, NULL sinon)
	// Cas d'un attribut cible avec groupement de valeur (regression ou classification
	// avec valeurs cibles a grouper).
	KWDGAttribute* GetTargetAttribute() const;

	// Indique si la grille est de type instances x variables, car un de ses attribut est de type VarPart
	boolean IsVarPartDataGrid() const;

	// Acces a l'attribut de type VarPart d'un grille si elle de type instances x variables, NULL sinon
	KWDGAttribute* GetVarPartAttribute() const;

	// Acces direct au parametrage des attributs internes de l'attribut de type VarPart
	const KWDGInnerAttributes* GetInnerAttributes() const;

	// Methode ancee pour acceder aux attributs internes en mode editable
	// A utiliser avec precaution
	KWDGInnerAttributes* GetEditableInnerAttributes() const;

	// Recherche d'un attribut de la grille par son nom (recherche couteuse, par parcours exhaustif des attributs)
	// Renvoie NULL si non trouve
	KWDGAttribute* SearchAttribute(const ALString& sAttributeName) const;

	// Nettoyage des attributs non informatifs (reduits a une seules partie)
	// Prerequis: les attributs doivent etre initialises avec leur partition en intervalles/groupes,
	// mais aucune cellule ne doit etre cree
	// En sortie: les attributs non informatifs sont detruits, les autres etant reindexes
	void DeleteNonInformativeAttributes();

	// Nettoyage complet
	virtual void DeleteAll();

	// Nettoyage des cellules uniquement (en preservant les attributs et leur definition
	void DeleteAllCells();

	// Methode avancee: acces a la granularite du DataGrid
	// Vaut 0 si pas d'usage de la granularite
	// Est fixee lors de la creation de la grille par granularisation sinon
	int GetGranularity() const;
	void SetGranularity(int nIndex);

	////////////////////////////////////////////////////////////////
	// Mise-a-jour des cellules du DataGrid
	// Il s'agit de la liste des N-uplets du produit cartesien des attributs partitionnes

	// Mode edition des cellules
	// Il est necessaire de se mettre en mode mise-a-jour pour toutes les operations
	// de creation, destruction et acces directes aux cellules
	// En mode edition, les parties des attributs ne doivent pas etre modifies
	// Quand on passe CellUpdateMode a false, les statistiques globales sur la grille et les effectifs
	// par partie d'attributs sont recalculees.
	// Ces statistiques peuvent ne pas etre valides en mode edition des cellules.
	void SetCellUpdateMode(boolean bValue);
	boolean GetCellUpdateMode() const;

	// Construction/destruction des structures d'indexation des attributs,
	// qui permettent de trouver les parties a partir des valeurs dans chaque attribut
	void BuildIndexingStructure();
	void DeleteIndexingStructure();

	// Creation d'une cellule en specifiant le tableau des parties dont il est le N-uplet
	// La cellule est ajoutes en fin de liste de chacune des parties d'attribut
	// Renvoie la cellule cree
	KWDGCell* AddCell(ObjectArray* oaParts);

	// Recherche d'une cellule par son tableau de parties
	// Renvoie la cellule, ou NULL s'il n'existe pas
	KWDGCell* LookupCell(ObjectArray* oaParts) const;

	// Destruction d'une cellule du DataGrid
	void DeleteCell(KWDGCell* cell);

	// Test de validite d'une cellule (s'il appartient au DataGrid)
	// Attention: operation couteuse en O(k.n)
	boolean CheckCell(KWDGCell* cell) const;

	// Verification de la validite d'un tableau de parties d'attributs
	boolean CheckCellParts(ObjectArray* oaParts) const;

	////////////////////////////////////////////////////////////////
	// Acces aux cellules du DataGrid

	// Nombre de cellules non vides
	int GetCellNumber() const;

	// Parcours de tous les cellules
	KWDGCell* GetHeadCell() const;
	KWDGCell* GetTailCell() const;
	void GetNextCell(KWDGCell*& cell) const;
	void GetPrevCell(KWDGCell*& cell) const;

	///////////////////////////////////////////////////////////////
	// Statistiques globales sur la grille
	// Resultats valide uniquement quand la grille est entierement initialisee

	// Recalcul des statistiques globales sur la grille
	// (statistiques globales sur la grille, effectif par partie d'attribut)
	void UpdateAllStatistics();

	// Effectif de la grille
	int GetGridFrequency() const;

	// Taille de la grille en nombre total de cellules, donne par son log
	// Il s'agit du produit des nombre de parties par attribut
	// Ce nombre etant potentiellement tres grand, on renvoie son log pour
	// ne pas depasser les limites informatiques
	double GetLnGridSize() const;

	// Calcul du nombre d'attribut informatifs (ayant strictement plus de une partie)
	int GetInformativeAttributeNumber() const;

	// Nombre total de parties par attribut (par cumul sur tous les attributs)
	int GetTotalPartNumber() const;

	// Calcul du nombre max de partie par attribut
	int ComputeMaxPartNumber() const;

	// Calcul a des valeurs d'entropie dans les cellules (source), les valeurs cibles (target) ou mutuelle
	// L'entropie permet de mesurer la quantite d'information (en bits)
	// pour coder une variable
	// Entropie = somme( -p(x) log(p(x)), avec p(x) = e(x)/e
	double ComputeSourceEntropy();
	double ComputeTargetEntropy();
	double ComputeMutualEntropy();

	///////////////////////////////
	// Services divers

	// Creation d'un attribut
	// Les creations de partie d'attribut se font depuis une methode NewPart dans les attributs
	virtual KWDGAttribute* NewAttribute() const;

	// Verification de la validite de la structure du DataGrid
	// Controle d'integrite global (attributs, parties, cellules...)
	// Attention: operation couteuse en O(k.n^2)
	boolean Check() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Tri des parties des attributs pour preparer l'affichage
	// Les intervalles sont tries par valeur croissante
	// Les groupes par effectif decroissant, et les valeurs dans les groupes par effectif decroissant
	// Les clusters de parties de variables sont tries de la meme facon que les groupes
	void SortAttributeParts();

	// Verification du tri des parties : couteuse, a utiliser essentiellement dans les assertions
	boolean AreAttributePartsSorted() const;

	// Import/export avec les objets KWDataGridStats, qui permettent un stockage compact (mais fige)
	// de tout type de grilles de donnees excepte les grilles issues de coclustering individus * variables
	// qui sont traites par une methode specifique (cf ExportVarPartDataGridStats)
	// Les possibilites d'echange sont limitees par celles de la classe KWDataGrid, a savoir:
	//   - soit entierement non supervise
	//   - soit supervise avec un attribut cible symbolique implicite (GetTargetValueNumber() > 0)
	//   - soit supervise avec un attribut cible explicite (GetTargetAttribute() != NULL)
	// L'objet source doit etre valide, et l'objet cible doit etre vide
	void ImportDataGridStats(const KWDataGridStats* dataGridStats);
	void ExportDataGridStats(KWDataGridStats* dataGridStats) const;

	// Methode d'export d'une grille de coclustering instances x variables
	// Cet export considere exporte l'attribut VarPart en un attribut de type KWDGSGrouping
	// pour lequel les valeurs des groupes de valeurs sont les libelles des parties de variables
	// Cet export est partiel dans le sens ou les InnerAttributes ne sont pas exportes
	// Le dataGridStats obtenu peut etre utilise pour construire la RDD de type KWDRDataGrid dans le dictionnaire de deploiement
	// Seul la methode d'Export est specifique dans le cas VarPart.
	// La methode d'Import est commune a celle des autres grilles : l'import du KWDGSGrouping se fait de la meme facon,
	// qu'il provienne d'un attribut de type Symbol ou VarPart
	void ExportVarPartDataGridStats(KWDataGridStats* dataGridStats) const;

	// Affichage
	void Write(ostream& ost) const override;
	void WriteTargetValues(ostream& ost) const;
	void WriteAttributes(ostream& ost) const;
	void WriteAttributeParts(ostream& ost) const;
	void WriteCells(ostream& ost) const;
	void WriteInnerAttributes(ostream& ost) const;

	// Affichage des statistiques par classe cible dans un tableau croise
	// Dans chaque cellule du tableau, on affichage la proportion de la classe en parametre, ou
	// l'effectif de la cellule si TargetIndex est egal au nombre de valeurs cibles
	// Methode utilisable uniquement dans le cas a deux attributs
	void WriteCrossTableStats(ostream& ost, int nTargetIndex) const;

	// Valeur utilisateur permettant d'indexer ou de trier des grilles de donness
	void SetSortValue(int nValue);
	int GetSortValue() const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////
	// Methodes de test de la classe

	// Creation d'un DataGrid
	static KWDataGrid* CreateTestDataGrid(int nSymbolAttributeNumber, int nContinuousAttributeNumber,
					      int nAttributePartNumber, int nTargetValueNumber, int nInstanceNumber);

	// Methode de test
	static void Test();

	///////////////////////////////
	//// Implementation

protected:
	friend class KWDGAttribute;

	// Modification de l'attribut cible
	void SetTargetAttribute(KWDGAttribute* attribute);

	// Calcul de statistiques globales sur la grille
	int ComputeGridFrequency() const;
	double ComputeLnGridSize() const;
	int ComputeInformativeAttributeNumber() const;
	int ComputeTotalPartNumber() const;

	// Methode interne d'export d'une KWDataGrid en KWDataGridStats en fonction du type de grille : VarPart ou non
	void InternalExportDataGridStats(KWDataGridStats* dataGridStats, boolean bVarPartMode) const;

	////////////////////////////////////////////////////////////////////////////
	// Methodes de creations virtuelles, permettant de specialiser les entites
	// d'un DataGrid dans une sous-classe

	// Creation d'une cellule
	// Le tableau d'effectif par classe cible doit etre initialise selon GetTagretValueNumber
	virtual KWDGCell* NewCell() const;

	////////////////////////////////////////////////////////////////////////////
	// Attributs du DataGrid

	// Methode indiquant si les donnees sont emulee
	// En cas non emule (par defaut), il doit y avoir validite complete des donnees pour acceder
	// au caracteristiques principales (effectif,...). En cas emule (utile par exemple pour
	// la classe KWDataGridUnivariateCosts), ces caracteristique principales (permettant de
	// parametrer les couts) peuvent etre modifiee directement et acceder sans etre coherente
	// dans la la structure
	virtual boolean GetEmulated() const;

	// Statistiques globales sur la grille
	int nGridFrequency;
	double dLnGridSize;
	int nInformativeAttributeNumber;
	int nTotalPartNumber;
	int nGranularity;

	// Tableau des attributs du DataGrid
	ObjectArray oaAttributes;

	// Attribut cible
	KWDGAttribute* targetAttribute;

	// Tableau des valeurs cibles
	SymbolVector svTargetValues;

	// Gestion de la liste doublement chainee des cellules
	KWDGCell* headCell;
	KWDGCell* tailCell;
	int nCellNumber;

	// Gestion du mode mise-a-jour des cellule au moyen d'une liste triee dont
	// la cle de tri est le tableau des parties des cellules. On peut ainsi rechercher
	// si une cellule existe deja en O(k.n.log(n))
	SortedList* slCells;

	// Valeur de tri
	int nSortValue;

	// Attribut de la grille de type VarPart
	// Positionne par l'attribut lors de l'initialisation de son type s'il est de type VarPart
	// Permet egalement de savoir si la grille est de type VarPart
	KWDGAttribute* varPartAttribute;
};

// Comparaison de deux grilles de donnees, sur la valeur de tri (SortValue),
// puis en cas d'egalite sur les nom des attributs de la grille
int KWDataGridCompare(const void* elem1, const void* elem2);

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGAttribute
// Attribut d'un DataGrid
// L'attribut est caracterise principalement par:
//			Type de l'attribut (Symbol ou Continuous)
//			Nombre de valeurs de l'attribut
//			Liste des parties de l'attribut
//			Fonction de l'attribut (source ou cible, par defaut source)
class KWDGAttribute : public Object
{
public:
	// Constructeur
	KWDGAttribute();
	~KWDGAttribute();

	// DataGrid dont l'attribut fait partie
	KWDataGrid* GetDataGrid() const;

	/////////////////////////////////////////////
	// Specifications de l'attribut operande

	// Nom de l'attribut
	void SetAttributeName(const ALString& sValue);
	const ALString& GetAttributeName() const;

	// Type de l'attribut (Symbol ou Continuous ou de type Parties de variables)
	// Le type n'est modifiable qu'une seule fois, pour l'initialisation
	void SetAttributeType(int nValue);
	int GetAttributeType() const;

	// Fonction de l'attribut (source ou cible, source par defaut)
	// Seul le dernier attribut d'une grille peut etre cible
	void SetAttributeTargetFunction(boolean bTargetAttribute);
	boolean GetAttributeTargetFunction() const;

	// Test si un attribut est interne
	boolean IsInnerAttribute() const;

	// Nom de l'attribut de grille de type VarPart dont l'attribut interne fait eventuellement partie
	// Renvoie vide dans le cas d'un coclustering standard
	ALString GetOwnerAttributeName() const;
	void SetOwnerAttributeName(ALString sName);

	// Parametrage des attributs internes dans le cas d'un attribut de grille de type VarPart
	// Note sur la gestion memoire des attributs internes
	// - les attributs internes peuvent etre partagee entre plusieurs grilles
	// - grace a un comptage de reference propre aux attributs internes, ceux-ci sont automatiquement
	//   detruits quand ils ne sont plus utilises
	void SetInnerAttributes(const KWDGInnerAttributes* attributes);
	const KWDGInnerAttributes* GetInnerAttributes() const;

	// Nombre d'attributs internes
	int GetInnerAttributeNumber() const;

	// Acces a un attribut interne par index
	KWDGAttribute* GetInnerAttributeAt(int nIndex) const;

	// Index de l'attribut dans le tableau des attributs de la grille
	// Renvoie -1 si attribut interne
	int GetAttributeIndex() const;

	// Nombre initial de valeurs de l'attribut
	// Attribut continu : nombre total d'instances
	// Attribut categoriel : nombre de valeurs distinctes (sans la StarValue)
	// Attribut parties de variables : nombre de parties de variables distinctes
	// (le nombre de PV distinctes depend du niveau de tokenisation, pour les innerAttributes et pour l'attribut VarPart)
	void SetInitialValueNumber(int nValue);
	int GetInitialValueNumber() const;

	// Nombre de valeurs apres granularisation
	// Attribut continu : nombre theorique de partiles Ng=2^G
	// Attribute categoriel : nombre de partiles obtenus suite a la granularisation Vg
	void SetGranularizedValueNumber(int nValue);
	int GetGranularizedValueNumber() const;

	// Cout d'un attribut, utilise pour la regularisation des regles de construction (sans cout du choix de modele
	// nul) Usage avance; par defaut 0
	void SetCost(double dValue);
	double GetCost() const;

	////////////////////////////////////////////////
	// Service pour les attributs de type VarPart

	// Initialisation des parties de l'attribut de type VarPart a partir des attributs internes initialises en
	// creant une partie par partie de variable
	void CreateVarPartsSet();

	////////////////////////////////////////////////
	// Gestion des parties de l'attribut
	// Memoire: les parties appartiennent a l'attribut

	// Creation d'une partie et ajout en fin de liste
	// Le type de l'attribut doit avoir ete specifie en prealable.
	// Renvoie la partie cree (avec le type de l'attribut)
	// Si l'attribut est de type VarPart, la partie renvoyee est de type KWDGVarPart*
	KWDGPart* AddPart();

	// Destruction d'une partie et de son contenu, de facon coherente le DataGrid
	// (sans neanmoins gerer les cellules de la partie)
	void DeletePart(KWDGPart* part);

	// Destruction de tous les parties
	void DeleteAllParts();

	// Test de validite d'une partie (s'il appartient a l'attribut)
	// Attention: operation couteuse en O(n)
	boolean CheckPart(KWDGPart* part) const;

	// Nombre de parties
	int GetPartNumber() const;

	// Parcours de tous les parties
	KWDGPart* GetHeadPart() const;
	KWDGPart* GetTailPart() const;
	void GetNextPart(KWDGPart*& part) const;
	void GetPrevPart(KWDGPart*& part) const;

	// Acces a la partie poubelle
	// Renvoie NULL si pas de poubelle
	KWDGPart* GetGarbagePart() const;
	void SetGarbagePart(KWDGPart* part);

	// Acces au nombre de modalites du groupe poubelle
	// Renvoie 0 si le groupe poubelle n'existe pas et sa taille s'il existe
	virtual int GetGarbageModalityNumber() const;

	// Ensemble des valeurs ignorees lors de la creation du fourre-out
	// Memoire:  cet ensemble de valeurs appartient a l'appele
	// Renvoie NULL si pas de fourre tout
	KWDGValueSet* GetCatchAllValueSet() const;

	// Initialisation du fourre-tout en le recopiant s'il est non NULL
	void InitializeCatchAllValueSet(KWDGValueSet* valueSet);

	// Acces au nombre de modalites du groupe fourre-tout
	// Permet de memoriser sa taille, meme si on l'a nettoye
	// Renvoie 0 si le groupe fourre-tout n'existe pas et sa taille s'il existe
	void SetCatchAllValueNumber(int nValue);
	int GetCatchAllValueNumber() const;

	// Export des parties dans un tableau (initialement vide)
	void ExportParts(ObjectArray* oaParts) const;

	///////////////////////////////////////////////////////////////////////////
	// Methodes utilitaire de recherche de la partie contenant une valeur particuliere
	// Ces methodes sont interessantes pour ajouter ou rechercher des cellules
	// de parties a partir d'une cellule de valeurs.
	// Ces methodes ne sont operationnelles que quand les parties et leurs
	// valeurs sont entierement specifies. Il faut alors creer une structure
	// d'indexation des parties par les valeurs a fins d'optimisation.
	// Cette structure doit etre cree explicitement, puis utilisee (si possible
	// plusieurs fois), et enfin detruite

	// Construction des structures d'indexation
	void BuildIndexingStructure();

	// Destruction des structures d'indexation
	void DeleteIndexingStructure();

	// Indicateur d'indexation
	boolean IsIndexed() const;

	// Recherche de la partie contenant une valeur numerique, symbolique ou VarPart
	// (doit etre compatible avec le type de l'attribut)
	// Attention a ne pas modifier les valeurs (intervalles ou ensemble de valeurs)
	// pendant l'utilisation de l'indexation
	KWDGPart* LookupContinuousPart(Continuous cValue);
	KWDGPart* LookupSymbolPart(const Symbol& sValue);
	KWDGPart* LookupVarPart(KWDGPart* varPart);

	// Recherche generique de la partie contenant une valeur de ValueSet dans le cas
	// d'un attribut de type groupable, symbolique ou VarPart
	KWDGPart* LookupGroupablePart(const KWDGValue* value);

	///////////////////////////////
	// Services divers

	// Calcul de l'effectif cumule sur l'ensemble des parties
	int ComputeTotalPartFrequency() const;

	// Test si l'attribut contient des sous-parties de l'autre attribut en parametre
	// Dans le cas d'un attribut de type VarPart, les deux attributs doivent exploiter les meme variables internes
	boolean ContainsSubParts(const KWDGAttribute* otherAttribute) const;

	// Controle d'integrite local a l'attribut (parties, valeurs, cellules de l'attribut)
	// Dans le cas continus, les intervalles doivent former une partition.
	// Dans le cas symbolique, les parties doivent former une partiton des valeurs,
	// les valeurs autres etant representees par une modalite speciale definie
	// par Symbol::GetStarValue()
	boolean Check() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Tri des parties pour preparer l'affichage
	// Les intervalles sont tries par valeur croissante
	// Les groupes (de valeurs ou de parties de variable) sont tries par effectif decroissant
	// Danc chaque partie:
	// - Symbol: les valeurs dans les groupes sont tries par effectif decroissant
	// - VarPart: les parties de variables dans les groupes sont tries par attribut puis
	//    par valeur de la partie, intervalles, ou valeurs du groupe
	void SortParts();

	// Verification du tri des parties : couteux, a utiliser essentiellement dans les assertions
	boolean ArePartsSorted() const;

	// Affichage
	void Write(ostream& ost) const override;
	void WriteParts(ostream& ost) const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////
	// Methodes de test de la classe

	// Creation d'un attribut numerique de test
	static KWDGAttribute* CreateTestContinuousAttribute(Continuous cFirstBound, Continuous cIntervalWidth,
							    int nIntervalNumber);

	// Creation d'un attribut symbolique de test (dont les valeurs seront
	// nommees V1, V2..., et d'effectif nul)
	static KWDGAttribute* CreateTestSymbolAttribute(int nValueNumber, int nPartNumber);

	// Methode de test
	static void Test();

	///////////////////////////////
	///// Implementation
protected:
	friend class KWDataGrid;
	friend class KWEvaluatedDataGrid;
	friend class KWDataGridMerger;

	// Tri des parties selon une fonction de tri
	void InternalSortParts(CompareFunction fCompare);

	// Methodes de creations virtuelles, permettant de specialiser les entites
	// d'un KWDGAttribute dans une sous-classe
	virtual KWDGPart* NewPart() const;

	// Methode indiquant si les donnees sont emulee
	virtual boolean GetEmulated() const;

	// Memorisation de l'attribut associee a une partie
	// (Pour rendre accessible cette fonctionnalite dans les sous classes)
	void InitializePartOwner(KWDGPart* part);

	// Attributs
	KWDataGrid* dataGrid;
	ALString sAttributeName;
	int nAttributeType;
	int nAttributeIndex;

	// Nombre initial de valeurs
	int nInitialValueNumber;

	// Nombre de valeurs apres granularisation
	int nGranularizedValueNumber;

	// Pointeur vers la partie poubelle, a NULL par defaut
	KWDGPart* garbagePart;

	// Pointeur vers les valeurs initiales du fourre-tout, a NULL par defaut
	int nCatchAllValueNumber;
	KWDGValueSet* catchAllValueSet;
	double dCost;
	boolean bTargetAttribute;

	// Gestion de la liste doublement chainee des parties
	KWDGPart* headPart;
	KWDGPart* tailPart;
	int nPartNumber;

	// Structure d'indexation des parties dans le cas numerique
	// Tableau des parties (intervalles) tries de facon croissante
	ObjectArray oaIntervals;

	// Structure d'indexation des parties dans le cas groupable
	// Dictionnaire des parties indexe par les valeurs des parties, et partie par defaut
	NumericKeyDictionary nkdParts;
	KWDGPart* defaultPart;

	// Indicateur d'indexation
	boolean bIsIndexed;

	// Nom de l'attribut de type VarPart dont depend un attribut interne
	// Par defaut a vide pour un attribut de type Simple (numerique ou categoriel)
	ALString sOwnerAttributeName;

	// Attributs internes dans les attributs de type VarPart
	const KWDGInnerAttributes* innerAttributes;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGPart
// Partie d'un attribut de DataGrid
// Chaque partie est caracterise principalement par
//   - sa liste de valeurs (intervalle ou ensemble de valeur selon le type
//     numerique ou symbolique, parties de variable (de type KWDGPart)
//     si l'attribut contient des parties de variables
//   - sa liste de cellules
class KWDGPart : public Object
{
public:
	// Constructeur
	KWDGPart();
	~KWDGPart();

	// Attribut auquel appartient la partie
	KWDGAttribute* GetAttribute() const;

	// Effectif total de la partie, cumul des effectif des cellules
	// On ne peut utiliser cette methode que lorsque toutes les cellules du DataGrid
	// ont ete correctement initialisees
	int GetPartFrequency() const;

	// Type de partie (Symbol ou Continuous ou Partie de parties de variables)
	// Le type n'est modifiable qu'une seule fois, pour l'initialisation
	virtual void SetPartType(int nValue);
	int GetPartType() const;

	// Acces specialise aux valeurs, selon le type d'attribut
	KWDGInterval* GetInterval() const;
	KWDGSymbolValueSet* GetSymbolValueSet() const;
	KWDGVarPartSet* GetVarPartSet() const;

	// Acces generique au groupe de valeur, dans les cas Symbol ou VarPart
	KWDGValueSet* GetValueSet() const;

	// Acces generique aux valeurs via la classe ancetre commune a tous les types
	KWDGPartValues* GetPartValues() const;

	////////////////////////////////////////////////////////////////
	// Acces aux cellules liee a la partie courante,
	// Il s'agit de la liste des N-uplets du produit cartesien des
	// attributs partitionnes, ayant leur composante egale a la partie courante
	// Memoire: les cellules sont uniquement references par les parties,
	// ils appartiennent au DataGrid directement

	// Test de validite d'une cellule (s'il appartient a la liste des cellules de la partie)
	// Attention: operation couteuse en O(n)
	boolean CheckCell(KWDGCell* cell) const;

	// Nombre de cellules
	int GetCellNumber() const;

	// Parcours de tous les cellules
	KWDGCell* GetHeadCell() const;
	KWDGCell* GetTailCell() const;
	void GetNextCell(KWDGCell*& cell) const;
	void GetPrevCell(KWDGCell*& cell) const;

	///////////////////////////////
	// Services divers

	// Methode avancee: mise a jour directe de l'effectif de la partie
	// Attention, cela doit correspondre exactement au cumul des effectifs des cellules de la partie
	// Sauf pour les attributs interne ou l'effectif est possitionne directement
	void SetPartFrequency(int nValue);

	// Test si la partie est une sous-partie de l'autre partie en parametre
	boolean IsSubPart(const KWDGPart* otherPart) const;

	// Comparaison de deux parties en exploitant la taille des parties, sauf dans le cas numerique
	// - par intervalle si numerique
	// - par effectif decroissant puis valeur si ensemble de valeurs
	int ComparePart(const KWDGPart* otherPart) const;

	// Comparaison des valeurs des parties
	// - par intervalle si numerique
	// - par premiere valeur si groupe de valeur categoriel ou VarPart
	int ComparePartValues(const KWDGPart* otherPart) const;

	// Controle d'integrite local a la partie (valeurs, cellules de la partie)
	boolean Check() const override;

	// Memoire utilisee par la partie
	longint GetUsedMemory() const override;

	// Affichage
	void Write(ostream& ost) const override;
	void WriteValues(ostream& ost) const;
	void WriteCells(ostream& ost) const;

	// Libelle complet dans le cas d'une partie de variable, base sur le libelle de l'identifiant interne
	// et celui de la partie
	const ALString GetVarPartLabel() const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////
	///// Implementation
protected:
	friend class KWDataGrid;
	friend class KWDataGridMerger;
	friend class KWDGAttribute;

	// Methodes de creations virtuelles, permettant de specialiser la composition d'une partie
	// dans une sous-classe
	virtual KWDGInterval* NewInterval() const;
	virtual KWDGSymbolValueSet* NewSymbolValueSet() const;
	virtual KWDGVarPartSet* NewVarPartSet() const;

	// Acces a l'index de l'attribut (0 si NULL)
	int GetAttributeIndex() const;

	// Calcul de l'effectif total des cellules de la partie
	virtual int ComputeCellsTotalFrequency() const;

	// Methode indiquant si les donnees sont emulees
	virtual boolean GetEmulated() const;

	// Memorisation de l'attribut
	KWDGAttribute* attribute;

	// Chainage des parties
	KWDGPart* prevPart;
	KWDGPart* nextPart;

	// Gestion de la liste doublement chainee des cellules
	KWDGCell* headCell;
	KWDGCell* tailCell;
	int nCellNumber;

	// Effectif total de la partie
	int nPartFrequency;

	// Gestion des valeurs de la parties via a un objet ancetre generique,
	// dont le type de valeur doit etre le meme que celui de l'attribut
	KWDGPartValues* partValues;
};

// Comparaison de deux parties, par effectif decroissant puis valeur, ou par intervalle (cf. ComparePart)
int KWDGPartCompare(const void* elem1, const void* elem2);

// Comparaison de deux parties, par valeur ou par intervalle (cf. ComparePartValues)
int KWDGPartCompareValues(const void* elem1, const void* elem2);

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGPartValues
// Composant de KWDGPart dedie a la gestion des valeurs effectives d'une partie
// Classe abstraite, ancetre des intervalles et groupes de valeur
class KWDGPartValues : public Object
{
public:
	// Type de valeur gere par la partie
	virtual int GetValueType() const = 0;

	// Test si la partie  est un sous-ensemble de la partie en parametre
	virtual boolean IsSubPart(const KWDGPartValues* otherPartValues) const = 0;

	// Import des valeurs d'une autre partie
	// La partie source est reinitialisee
	virtual void Import(KWDGPartValues* sourcePartValues) = 0;

	// Mise a jour des valeurs a partir des valeurs d'une partie source, devant etre adjacente
	// La partie source n'est pas modifiee
	virtual void UpgradeFrom(const KWDGPartValues* sourcePartValues) = 0;

	// Copie
	virtual void CopyFrom(const KWDGPartValues* sourcePartValues) = 0;

	// Comparaison sur les valeurs de la partie
	virtual int ComparePartValues(const KWDGPartValues* otherPartValues) const = 0;

	// Controle d'integrite
	boolean Check() const override = 0;

	// Affichage
	void Write(ostream& ost) const override = 0;

	// Memoire utilisee par la partie, sans ses valeurs
	longint GetUsedMemory() const override = 0;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGValueSet
// Sous-classe virtuelle de KWDGPartValues dediee aux groupes de valeurs
// pour mutualiser la gestion des valeurs de type Symbol ou VarPart
class KWDGValueSet : public KWDGPartValues
{
public:
	// Constructeur
	KWDGValueSet();
	~KWDGValueSet();

	// Type de valeur gere
	int GetValueType() const override = 0;

	// Creation
	virtual KWDGValueSet* Create() const = 0;

	// Duplication
	KWDGValueSet* Clone() const;

	////////////////////////////////////////////////////////////////
	// Gestion des valeurs de la parties sous forme de liste
	// Memoire: les valeurs appartiennent a la partie

	// Creation d'une valeur a partir de la copie d'une valeur existante et ajout en fin de liste
	// Renvoie la valeur cree
	virtual KWDGValue* AddValueCopy(const KWDGValue* sourceValue) = 0;

	// Destruction d'une valeur de la liste
	void DeleteValue(KWDGValue* value);

	// Destruction de toutes les valeurs
	void DeleteAllValues();

	// Test de validite d'une valeur (si elle appartient a la partie)
	boolean CheckValue(KWDGValue* value) const;

	// Test si la partie est la partie par defaut
	boolean IsDefaultPart() const;

	// Nombre de valeurs
	int GetValueNumber() const;

	// Parcours de tous les valeurs
	KWDGValue* GetHeadValue() const;
	KWDGValue* GetTailValue() const;
	void GetNextValue(KWDGValue*& value) const;
	void GetPrevValue(KWDGValue*& value) const;

	///////////////////////////////
	// Services divers

	// Export des valeurs dans un tableau (initialement vide)
	void ExportValues(ObjectArray* oaValues) const;

	// Calcul de l'effectif cumule des valeurs
	int ComputeTotalFrequency() const;

	// Tri des valeurs
	void SortValues();

	// Tri des valeurs par effectif decroissant, pour preparer l'affichage
	void SortValueByDecreasingFrequencies();

	// Tri des valeurs par typicalite decroissante
	void SortValuesByDecreasingTypicalities();

	// Verification du tri des valeurs par effectif decroissant
	// Couteux, a utiliser essentiellement dans les assertions
	boolean AreValuesSorted() const;
	boolean AreValuesSortedByDecreasingFrequencies() const;
	boolean AreValuesSortedByDecreasingTypicalities() const;

	// Redefinition des methodes virtuelles
	boolean IsSubPart(const KWDGPartValues* otherPartValues) const override;
	void Import(KWDGPartValues* sourcePartValues) override;
	void UpgradeFrom(const KWDGPartValues* sourcePartValues) override;
	void CopyFrom(const KWDGPartValues* sourcePartValues) override;

	// Comparaison de deux ensembles de valeurs d'apres leur premiere valeur
	// - valeur Symbol si categoriel
	// - nom d'attribut, puis valeur de partie si VarPart
	int ComparePartValues(const KWDGPartValues* otherPartValues) const override;

	// Controle d'integrite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;
	void WriteValues(ostream& ost) const;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////
	///// Implementation

	// Modification du nombre de valeurs
	// Methode avancee utilisable uniquement en mode emule
	void SetValueNumber(int nValue);

protected:
	// Ajout d'une valeur en fin de liste
	void AddTailValue(KWDGValue* value);

	// Tri des valeurs selon une fonction de tri
	// La valeur speciale est toujours mise en dernier, independament du critere de tri
	void InternalSortValues(CompareFunction fCompare);

	// Test si des valeurs sont triees selon une fonction de tri
	// La valeur speciale est toujours mise en dernier, independament du critere de tri
	boolean InternalAreValuesSorted(CompareFunction fCompare) const;

	// Methode indiquant si les donnees sont emulees
	virtual boolean GetEmulated() const;

	// Gestion de la liste doublement chainee des cellules
	KWDGValue* headValue;
	KWDGValue* tailValue;
	int nValueNumber;
	boolean bIsDefaultPart;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGValue
// Classe virtuelle pour mutualiser la gestion des valeurs de type Symbol ou VarPart
class KWDGValue : public Object
{
public:
	// Constructeur
	KWDGValue();
	~KWDGValue();

	// Type de valeur
	virtual int GetValueType() const = 0;

	// Valeur accesssible depuis la classe ancetre, pour simplifier l'ecriture de code generique
	// Implementation par defaut en assert false
	// Chaque sous-classe doit reimplementer la methode correspondante a son type de valeur
	virtual Symbol& GetSymbolValue() const;
	virtual KWDGPart* GetVarPart() const;

	// Valeur en tant que cle numerique
	virtual NUMERIC GetNumericKeyValue() const = 0;

	// Test si on est la valeur par defaut, qui fait partie de la partie par defaut
	virtual boolean IsDefaultValue() const = 0;

	// Effectif de la valeur
	virtual void SetValueFrequency(int nFrequency) = 0;
	virtual int GetValueFrequency() const = 0;

	// Typicalite de la valeur
	void SetTypicality(double dValue);
	double GetTypicality() const;

	// Comparaison par valeur
	// - valeur Symbol si categoriel
	// - nom d'attribut, puis valeur de partie si VarPart
	virtual int CompareValue(const KWDGValue* otherValue) const = 0;

	// Compare par effectif decroissant, puis par valeur
	virtual int CompareFrequency(const KWDGValue* otherValue) const;

	// Comparaison par typicalite, puis
	// - par effectif si categoriel
	// - par valeur si VarPart
	virtual int CompareTypicality(const KWDGValue* otherValue) const = 0;

	// Affichage
	void Write(ostream& ost) const override = 0;

	// Libelle externe base sur la valeur
	virtual const ALString GetExternalValueLabel() const = 0;

	// Memoire utilisee
	virtual longint GetUsedMemory() const override = 0;

	// Libelles utilisateur
	const ALString GetClassLabel() const override = 0;
	const ALString GetObjectLabel() const override = 0;

	///////////////////////////////
	///// Implementation
protected:
	friend class KWDGValueSet;

	// Attributs
	double dTypicality;
	KWDGValue* prevValue;
	KWDGValue* nextValue;
};

// Comparaison de deux valeurs
int KWDGValueCompareValue(const void* elem1, const void* elem2);

// Comparaison de deux valeurs, par effectif decroissant
int KWDGValueCompareFrequency(const void* elem1, const void* elem2);

// Comparaison de deux typicalite
int KWDGValueCompareTypicality(const void* elem1, const void* elem2);

// Comparaison de deux objets KWSortableObject contenant des KWDGValue, par valeur puis par index
// Permet un comparaison generique pour des valeurs de type Symbol ou VarPart
int KWSortableObjectCompareValue(const void* elem1, const void* elem2);

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGSymbolValueSet
// Ensemble de valeurs d'une partie symbolique
class KWDGSymbolValueSet : public KWDGValueSet
{
public:
	// Constructeur
	KWDGSymbolValueSet();
	~KWDGSymbolValueSet();

	// Type de valeur gere par la partie
	int GetValueType() const override;

	// Creation
	KWDGValueSet* Create() const override;

	// Creation d'une valeur et ajout en fin de liste
	// Renvoie la valeur cree
	KWDGValue* AddSymbolValue(const Symbol& sValue);

	// Redefinition de la methode virtuelle d'ajout de la copie d'une valeur existante
	KWDGValue* AddValueCopy(const KWDGValue* sourceValue) override;

	///////////////////////////////
	// Services avances

	// Compression des valeurs (pour raison de memoire)
	// La liste des valeurs est remplacee par une seule valeur (la valeur speciale StarValue)
	// Les statistiques (ValueNumber et Frequency) sont conservees
	void CompressValueSet();

	// Compression des valeurs
	// La liste des valeurs est nettoyee des modalites de plus faible effectif
	// Les valeurs conservees sont la valeur de tete (la plus frequente) et la valeur speciale StarValue
	// Frequency est conservee
	// ValueNumber est mise a 1
	// En sortie : un KWDGValueSet qui contient les valeurs autres que celles conservees
	KWDGValueSet* ConvertToCleanedValueSet();

	// Calcule et renvoie un ValueSet compresse avec la modalite d'effectif le plus eleve et la StarValue
	// Ne modifie pas le ValueSet
	KWDGValueSet* ComputeCleanedValueSet() const;

	///////////////////////////////
	///// Implementation
protected:
	// Methode de creation virtuelle, permettant de specialiser la creation d'une valeur dans une sous-classe
	virtual KWDGValue* NewSymbolValue(const Symbol& sValue) const;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGSymbolValue
// Valeur symbolique appartenant a une partie de valeur
class KWDGSymbolValue : public KWDGValue
{
public:
	// Constructeur
	KWDGSymbolValue(const Symbol& sValue);
	~KWDGSymbolValue();

	// Type de valeur
	int GetValueType() const override;

	// Valeur
	Symbol& GetSymbolValue() const override;

	// Valeur en tant que cle numerique
	NUMERIC GetNumericKeyValue() const override;

	// Test si on est la valeur par defaut, qui fait partie de la partie par defaut
	boolean IsDefaultValue() const override;

	// Effectif lie a la valeur
	void SetValueFrequency(int nFrequency) override;
	int GetValueFrequency() const override;

	// Comparaison de valeur Symbol
	int CompareValue(const KWDGValue* otherValue) const override;

	// Comparaison part typicalite, puis par effectif decroissant
	int CompareTypicality(const KWDGValue* otherValue) const override;

	// Affichage
	void Write(ostream& ost) const override;

	// Libelle externe base sur la valeur
	const ALString GetExternalValueLabel() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////
	///// Implementation
protected:
	mutable Symbol sSymbolValue;
	int nValueFrequency;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGInterval
// Intervalle de valeurs
class KWDGInterval : public KWDGPartValues
{
public:
	// Constructeur
	KWDGInterval();
	~KWDGInterval();

	// Type de valeur gere par la partie
	int GetValueType() const override;

	// Borne inf de l'intervalle
	// (doit etre egal a KWDGInterval::GetMinLowerBound pour le premier intervalle)
	void SetLowerBound(Continuous cValue);
	Continuous GetLowerBound() const;

	// Borne sup de l'intervalle
	// (doit etre egal a KWDGInterval::GetMaxUpperBound pour le dernier intervalle)
	void SetUpperBound(Continuous cValue);
	Continuous GetUpperBound() const;

	// Valeurs extremes des bornes des intervalles
	static Continuous GetMinLowerBound();
	static Continuous GetMaxUpperBound();

	///////////////////////////////
	// Services divers

	// Redefinition des methodes virtuelles
	boolean IsSubPart(const KWDGPartValues* otherPartValues) const override;
	void Import(KWDGPartValues* sourcePartValues) override;
	void UpgradeFrom(const KWDGPartValues* sourcePartValues) override;
	void CopyFrom(const KWDGPartValues* sourcePartValues) override;

	// Comparaison de deux intervalles d'apres leur bornes
	int ComparePartValues(const KWDGPartValues* otherPartValues) const override;

	// Controle d'integrite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////
	///// Implementation
protected:
	// Bornes de l'intervalle
	Continuous cLowerBound;
	Continuous cUpperBound;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGVarPartSet
// Ensemble de parties de variables (constitue une partie d'un attribut de grille de type VarPart)
class KWDGVarPartSet : public KWDGValueSet
{
public:
	// Constructeur
	KWDGVarPartSet();
	~KWDGVarPartSet();

	// Type de valeur gere par la partie
	int GetValueType() const override;

	// Creation
	KWDGValueSet* Create() const override;

	// Creation d'une valeur de type partie de variable et ajout en fin de liste
	// Renvoie la valeur cree
	KWDGValue* AddVarPart(KWDGPart* varPart);

	// Redefinition de la methode virtuelle d'ajout de la copie d'une valeur existante
	KWDGValue* AddValueCopy(const KWDGValue* sourceValue) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	///////////////////////////////
	///// Implementation
protected:
	// Methode de creation virtuelle, permettant de specialiser la creation d'une valeur dans une sous-classe
	virtual KWDGVarPartValue* NewVarPartValue(KWDGPart* varPart) const;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGVarPartValue
// Partie de variable appartenant a une partie de parties de variable
class KWDGVarPartValue : public KWDGValue
{
public:
	// Constructeur
	KWDGVarPartValue(KWDGPart* varPartValue);
	~KWDGVarPartValue();

	// Type de valeur
	int GetValueType() const override;

	// Partie de variable
	KWDGPart* GetVarPart() const override;

	// Valeur en tant que cle numerique
	NUMERIC GetNumericKeyValue() const override;

	// Test si on est la valeur par defaut, qui fait partie de la partie par defaut
	boolean IsDefaultValue() const override;

	// Effectif lie a la valeur
	void SetValueFrequency(int nFrequency) override;
	int GetValueFrequency() const override;

	// Comparaison de valeur, par attribut, puis selon les valeurs de la partie, intervalles ou premiere valeur des groupe
	int CompareValue(const KWDGValue* otherValue) const override;

	// Comparaison part typicalite, puis par valeur
	int CompareTypicality(const KWDGValue* otherValue) const override;

	// Affichage
	void Write(ostream& ost) const override;

	// Libelle externe base sur la valeur
	const ALString GetExternalValueLabel() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////
	///// Implementation
protected:
	// Attributs
	KWDGPart* varPart;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGInnerAttributes
// Ensemble des attributs internes d'un attribut de grille de type VarPart
class KWDGInnerAttributes : public Object
{
public:
	// Constructeur
	KWDGInnerAttributes();
	~KWDGInnerAttributes();

	// Ajout d'un attribut interne
	void AddInnerAttribute(KWDGAttribute* innerAttribute);

	// Nombre attribut internes
	int GetInnerAttributeNumber() const;

	// Acces a un attribut interne par index
	KWDGAttribute* GetInnerAttributeAt(int nAttributeIndex) const;

	// Acces a un attribut interne par nom
	KWDGAttribute* LookupInnerAttribute(const ALString& sAttributeName) const;

	// Acces a la granularite des parties de variable, partages par tous les attributs
	int GetVarPartGranularity() const;
	void SetVarPartGranularity(int nValue);

	///////////////////////////////
	// Services divers

	// Nettoyage
	void DeleteAll();

	// Nettoyage des attributs qui ne contiennent que des valeurs manquantes
	void CleanEmptyInnerAttributes();

	// Export de toutes les parties parties de variables des attributs internes dans un tableau (initialement vide)
	void ExportAllInnerAttributeVarParts(ObjectArray* oaInnerAttributeVarParts) const;

	// Calcul du nombre total de parties de variable sur l'ensemble des attributs internes
	int ComputeTotalInnerAttributeVarParts() const;

	// Calcul de l'effectif total cumule sur les parties de variable sur l'ensemble des attributs internes
	int ComputeTotalInnerAttributeFrequency() const;

	// Tri des parties des attributs internes
	void SortInnerAttributeParts() const;

	// Verification du tri des parties des attributs internes
	// Test couteux, a utiliser essentiellement dans les assertions
	boolean AreInnerAttributePartsSorted() const;

	// Test si les attributs internes sont constitues des memes attributs, ne contenant que des sous-parties
	// des VarPart des autres attributs internes en parametressi Verification du tri des parties des attributs internes :
	// Test couteux, a utiliser essentiellement dans les assertions
	boolean ContainsSubVarParts(const KWDGInnerAttributes* otherInnerAttributes) const;

	// Controle d'integrite
	boolean Check() const override;

	// Affichage
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////
	///// Implementation
protected:
	// Gestion du compteur de reference par la classe KWDGAttribute, permettant de partager des InnerAttributes
	// entre plusieurs grilles et les desallouer automatiquement quand elle ne sont plus utilisees
	// Cela permet de mutualiser une structure lourde sans se soucier de la dynamique des utilisations
	// lors des algorithmes d'optimisation
	friend class KWDGAttribute;

	// Gestion des attributs internes
	int nVarPartGranularity;
	ObjectDictionary odInnerAttributes;
	ObjectArray oaInnerAttributes;

	// Compteur de reference
	mutable int nRefCount;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGCell
// Cellule: N-uplet de parties d'attribut
// Chaque cellule est caracterise principalement par
//   - l'effectif total de la cellule
//   - le vecteur des effectifs des classes cible (dans le cas de la classification supervisee)
//   - le tableau (par attribut) des parties monovaries dont il est le produit cartesien
class KWDGCell : public Object
{
public:
	// Constructeur
	KWDGCell();
	~KWDGCell();

	////////////////////////////////////////////////////////////////////////////////
	// Contenu de la cellule en nombre d'instance
	// Dans le cas non supervise, on a acces a l'effectif total de la cellule (Frequency)
	// Dans le cas supervise (GetTargetValueNumber() > 0), on a acces aux effectifs par
	// classe cible (on ne peut plus appeler directement SetFrequency, mais le GetFrequency
	// reste coherent)

	// Effectif de la cellule
	int GetCellFrequency() const;
	void SetCellFrequency(int nFrequency);

	// Nombre de classes cible (0 dans le cas non supervise)
	int GetTargetValueNumber() const;

	// Effectif par classe cible
	int GetTargetFrequencyAt(int nTarget) const;
	void SetTargetFrequencyAt(int nTarget, int nFrequency);
	void UpgradeTargetFrequencyAt(int nTarget, int nDeltaFrequency);

	// Mise a jour du contenu d'une cellule en prenant en compte le contenu d'une autre cellule
	void AddFrequenciesFrom(const KWDGCell* cell);
	void RemoveFrequenciesFrom(const KWDGCell* cell);

	// Initialisation du contenu d'une cellule par fusion du contenu de deux cellules
	void MergeFrequenciesFrom(const KWDGCell* cell1, const KWDGCell* cell2);

	////////////////////////////////////////////////////////////////////////
	// Partie d'attribut definissant la cellule

	// Nombre d'attribut dont la cellule est un N-uplet
	int GetAttributeNumber() const;

	// Acces aux parties d'attribut de la cellule
	KWDGPart* GetPartAt(int nAttributeIndex) const;

	///////////////////////////////
	// Services divers

	// Controle d'integrite local a la cellule
	boolean Check() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Affichage (sous forme d'une ligne de tableau de contingence multi-attributs)
	void Write(ostream& ost) const override;

	// Affichage des statistiques synthetiques sur la classe cible
	void WriteTargetStats(ostream& ost) const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////
	///// Implementation
protected:
	friend class KWDataGrid;
	friend class KWEvaluatedDataGrid;
	friend class KWDataGridMerger;
	friend class KWDGMPartMergeAction;
	friend class KWDGPart;

	// Effectif total de la cellule par cumul de l'effectif par classe cible (coherent avec l'effectif de la
	// cellule)
	int ComputeTotalFrequency() const;

	// Methode indiquant si les donnees sont emulee
	virtual boolean GetEmulated() const;

	// Effectif de la cellule
	int nCellFrequency;

	// Vecteur d'effectif par classe cible
	IntVector ivFrequencyVector;

	// Tableau des parties de la cellule
	ObjectArray oaParts;

	// Tableau des chainage des cellules dans les listes chainee de chacune de ses parties
	// En effet, une meme cellule appartient a autant de liste chainees que de parties
	// dont elle est un N-uplet
	ObjectArray oaPrevCells;
	ObjectArray oaNextCells;

	// Chainage pour la gestion de la liste globale dans la structure principale du DataGrid
	KWDGCell* prevCell;
	KWDGCell* nextCell;
};

// Comparaison de deux cellules, sur la base des parties referencees, pour la performance (acces par pointeur)
int KWDGCellCompare(const void* elem1, const void* elem2);

// Comparaison de deux cellules, sur la base des parties referencees, pour l'affichage (acces par valeur)
int KWDGCellCompareValue(const void* elem1, const void* elem2);

// Comparaison de deux cellules par effectif decroissant (puis sur la base des parties referencees si egalite)
int KWDGCellCompareDecreasingFrequency(const void* elem1, const void* elem2);

/////////////////////////////////////////////////
// Methodes en inline

// Classe KWDataGrid

inline int KWDataGrid::GetAttributeNumber() const
{
	return oaAttributes.GetSize();
}

inline int KWDataGrid::GetTargetValueNumber() const
{
	return svTargetValues.GetSize();
}

inline boolean KWDataGrid::IsEmpty() const
{
	return GetAttributeNumber() == 0 and GetTargetValueNumber() == 0;
}

inline void KWDataGrid::SetTargetValueAt(int nIndex, const Symbol& sValue)
{
	require(0 <= nIndex and nIndex < GetTargetValueNumber());

	svTargetValues.SetAt(nIndex, sValue);
}

inline Symbol& KWDataGrid::GetTargetValueAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetTargetValueNumber());

	return svTargetValues.GetAt(nIndex);
}

inline KWDGAttribute* KWDataGrid::GetAttributeAt(int nAttributeIndex) const
{
	require(0 <= nAttributeIndex and nAttributeIndex < GetAttributeNumber());
	ensure(oaAttributes.GetAt(nAttributeIndex) != NULL);

	return cast(KWDGAttribute*, oaAttributes.GetAt(nAttributeIndex));
}

inline KWDGAttribute* KWDataGrid::GetTargetAttribute() const
{
	return targetAttribute;
}

inline boolean KWDataGrid::IsVarPartDataGrid() const
{
	return varPartAttribute != NULL;
}

inline KWDGAttribute* KWDataGrid::GetVarPartAttribute() const
{
	ensure(varPartAttribute == NULL or varPartAttribute->GetAttributeType() == KWType::VarPart);
	return varPartAttribute;
}

inline const KWDGInnerAttributes* KWDataGrid::GetInnerAttributes() const
{
	require(IsVarPartDataGrid());
	return GetVarPartAttribute()->GetInnerAttributes();
}

inline KWDGInnerAttributes* KWDataGrid::GetEditableInnerAttributes() const
{
	require(IsVarPartDataGrid());
	return cast(KWDGInnerAttributes*, GetVarPartAttribute()->GetInnerAttributes());
}

inline int KWDataGrid::GetCellNumber() const
{
	return nCellNumber;
}

inline KWDGCell* KWDataGrid::GetHeadCell() const
{
	return headCell;
}

inline KWDGCell* KWDataGrid::GetTailCell() const
{
	return tailCell;
}

inline void KWDataGrid::GetNextCell(KWDGCell*& cell) const
{
	require(cell != NULL);
	cell = cell->nextCell;
}

inline void KWDataGrid::GetPrevCell(KWDGCell*& cell) const
{
	require(cell != NULL);
	cell = cell->prevCell;
}

inline int KWDataGrid::GetGridFrequency() const
{
	ensure(GetEmulated() or nGridFrequency == ComputeGridFrequency());
	return nGridFrequency;
}

inline double KWDataGrid::GetLnGridSize() const
{
	require(GetEmulated() or fabs(dLnGridSize - ComputeLnGridSize()) < 1e-5);
	return dLnGridSize;
}

inline int KWDataGrid::GetInformativeAttributeNumber() const
{
	require(GetEmulated() or nInformativeAttributeNumber == ComputeInformativeAttributeNumber());
	return nInformativeAttributeNumber;
}

inline int KWDataGrid::GetTotalPartNumber() const
{
	require(GetEmulated() or nTotalPartNumber == ComputeTotalPartNumber());
	return nTotalPartNumber;
}

inline void KWDataGrid::SetSortValue(int nValue)
{
	nSortValue = nValue;
}

inline int KWDataGrid::GetSortValue() const
{
	return nSortValue;
}

// Classe KWDGAttribute

inline KWDataGrid* KWDGAttribute::GetDataGrid() const
{
	return dataGrid;
}

inline void KWDGAttribute::SetAttributeName(const ALString& sValue)
{
	sAttributeName = sValue;
}

inline const ALString& KWDGAttribute::GetAttributeName() const
{
	return sAttributeName;
}

inline void KWDGAttribute::SetAttributeType(int nValue)
{
	require(nAttributeType == KWType::Unknown);
	require(KWType::IsCoclusteringType(nValue));
	require(dataGrid == NULL or dataGrid->varPartAttribute == NULL or nValue != KWType::VarPart);

	nAttributeType = nValue;

	// On memorise l'attribut de type VarPart dans la grille
	if (nAttributeType == KWType::VarPart)
	{
		assert(dataGrid != NULL and dataGrid->varPartAttribute == NULL);
		dataGrid->varPartAttribute = this;
	}
}

inline int KWDGAttribute::GetAttributeType() const
{
	return nAttributeType;
}

inline void KWDGAttribute::SetAttributeTargetFunction(boolean bValue)
{
	bTargetAttribute = bValue;

	// Memorisation d'un pointeur vers l'attribut cible si c'est le cas
	if (bValue and GetDataGrid() != NULL)
	{
		GetDataGrid()->SetTargetAttribute(this);
	}
}

inline boolean KWDGAttribute::GetAttributeTargetFunction() const
{
	return bTargetAttribute;
}

inline boolean KWDGAttribute::IsInnerAttribute() const
{
	assert(dataGrid == NULL or
	       (sOwnerAttributeName == "" and
		((not dataGrid->IsVarPartDataGrid()) or
		 dataGrid->GetInnerAttributes()->LookupInnerAttribute(sAttributeName) == NULL)) or
	       (sOwnerAttributeName != "" and
		(dataGrid->IsVarPartDataGrid() and
		 dataGrid->GetInnerAttributes()->LookupInnerAttribute(sAttributeName) == this)));
	return sOwnerAttributeName != "";
}

inline ALString KWDGAttribute::GetOwnerAttributeName() const
{
	return sOwnerAttributeName;
}

inline void KWDGAttribute::SetOwnerAttributeName(ALString sName)
{
	sOwnerAttributeName = sName;
}

inline void KWDGAttribute::SetInnerAttributes(const KWDGInnerAttributes* attributes)
{
	require(GetAttributeType() == KWType::VarPart);

	// Decrementation des references sur les attributs internes d'origine, et desallocation si necessaire
	if (innerAttributes != NULL)
	{
		innerAttributes->nRefCount--;
		if (innerAttributes->nRefCount == 0)
			delete innerAttributes;
	}

	// Incrementation des references sur les nouveaux attributs internes
	if (attributes != NULL)
		attributes->nRefCount++;
	innerAttributes = attributes;
}

inline const KWDGInnerAttributes* KWDGAttribute::GetInnerAttributes() const
{
	require(GetAttributeType() == KWType::VarPart);
	ensure(innerAttributes != NULL);
	return innerAttributes;
}

inline int KWDGAttribute::GetInnerAttributeNumber() const
{
	require(nAttributeType == KWType::VarPart);
	return GetInnerAttributes()->GetInnerAttributeNumber();
}

inline KWDGAttribute* KWDGAttribute::GetInnerAttributeAt(int nIndex) const
{
	require(nAttributeType == KWType::VarPart);
	return GetInnerAttributes()->GetInnerAttributeAt(nIndex);
}

inline int KWDGAttribute::GetAttributeIndex() const
{
	return nAttributeIndex;
}

inline void KWDGAttribute::SetInitialValueNumber(int nValue)
{
	require(nValue >= 0);
	nInitialValueNumber = nValue;
}

inline int KWDGAttribute::GetInitialValueNumber() const
{
	return nInitialValueNumber;
}

inline void KWDGAttribute::SetGranularizedValueNumber(int nValue)
{
	require(nValue >= 0);
	nGranularizedValueNumber = nValue;
}

inline int KWDGAttribute::GetGranularizedValueNumber() const
{
	return nGranularizedValueNumber;
}

inline void KWDGAttribute::SetCost(double dValue)
{
	require(dValue >= 0);
	dCost = dValue;
}

inline double KWDGAttribute::GetCost() const
{
	return dCost;
}

inline int KWDGAttribute::GetPartNumber() const
{
	return nPartNumber;
}

inline KWDGPart* KWDGAttribute::GetHeadPart() const
{
	return headPart;
}

inline KWDGPart* KWDGAttribute::GetTailPart() const
{
	return tailPart;
}

inline void KWDGAttribute::GetNextPart(KWDGPart*& part) const
{
	require(part != NULL);
	part = part->nextPart;
}

inline void KWDGAttribute::GetPrevPart(KWDGPart*& part) const
{
	require(part != NULL);
	part = part->prevPart;
}

inline KWDGPart* KWDGAttribute::GetGarbagePart() const
{
	// Pas de require strict sur le type d'attribut, pour simplifier l'usage de cette methode
	require(KWType::IsCoclusteringGroupableType(nAttributeType) or garbagePart == NULL);
	return garbagePart;
}

inline void KWDGAttribute::SetGarbagePart(KWDGPart* part)
{
	require(KWType::IsCoclusteringGroupableType(nAttributeType));
	garbagePart = part;
}

inline int KWDGAttribute::GetGarbageModalityNumber() const
{
	if (GetGarbagePart() == NULL)
		return 0;
	// Sinon
	else
	{
		assert(KWType::IsCoclusteringGroupableType(nAttributeType));
		return GetGarbagePart()->GetValueSet()->GetValueNumber();
	}
}

inline KWDGValueSet* KWDGAttribute::GetCatchAllValueSet() const
{
	// Pas de require strict sur le type d'attribut, pour simplifier l'usage de cette methode
	require(nAttributeType == KWType::Symbol or catchAllValueSet == NULL);
	return catchAllValueSet;
}

inline void KWDGAttribute::InitializeCatchAllValueSet(KWDGValueSet* valueSet)
{
	// Pas de require strict sur le type d'attribut, pour simplifier l'usage de cette methode
	require(nAttributeType == KWType::Symbol or catchAllValueSet == NULL);
	require(nAttributeType == KWType::Symbol or valueSet == NULL);

	if (catchAllValueSet != NULL)
		delete catchAllValueSet;
	if (valueSet == NULL)
		catchAllValueSet = NULL;
	else
		catchAllValueSet = valueSet->Clone();
}

inline void KWDGAttribute::SetCatchAllValueNumber(int nValue)
{
	require(nValue >= 0);
	nCatchAllValueNumber = nValue;
}

inline int KWDGAttribute::GetCatchAllValueNumber() const
{
	// Pas de require sur le type pour simplifier l'ecriture du code
	//  Cas ou il n'y a pas de groupe fourre-tout
	if (GetCatchAllValueSet() == NULL)
		return 0;
	// Cas d'une taille de fourre-tout memorise (suite a un nettoyage du fourre-tout)
	else if (nCatchAllValueNumber != -1)
		return nCatchAllValueNumber;
	else
	{
		assert(not GetCatchAllValueSet()->IsDefaultPart());
		return GetCatchAllValueSet()->GetValueNumber();
	}
}

inline boolean KWDGAttribute::IsIndexed() const
{
	return bIsIndexed;
}

// Classe KWDGPart

inline KWDGAttribute* KWDGPart::GetAttribute() const
{
	return attribute;
}

inline int KWDGPart::GetPartFrequency() const
{
	ensure(GetEmulated() or nPartFrequency == ComputeCellsTotalFrequency() or GetAttribute()->IsInnerAttribute());
	return nPartFrequency;
}

inline int KWDGPart::GetPartType() const
{
	require(partValues == NULL or KWType::IsCoclusteringType(partValues->GetValueType()));

	if (partValues == NULL)
		return KWType::Unknown;
	else
		return partValues->GetValueType();
}

inline KWDGInterval* KWDGPart::GetInterval() const
{
	require(GetPartType() == KWType::Continuous);
	return cast(KWDGInterval*, partValues);
}

inline KWDGSymbolValueSet* KWDGPart::GetSymbolValueSet() const
{
	require(GetPartType() == KWType::Symbol);
	return cast(KWDGSymbolValueSet*, partValues);
}

inline KWDGVarPartSet* KWDGPart::GetVarPartSet() const
{
	require(GetPartType() == KWType::VarPart);
	return cast(KWDGVarPartSet*, partValues);
}

inline KWDGValueSet* KWDGPart::GetValueSet() const
{
	require(KWType::IsCoclusteringGroupableType(GetPartType()));
	return cast(KWDGValueSet*, partValues);
}

inline KWDGPartValues* KWDGPart::GetPartValues() const
{
	require(KWType::IsCoclusteringType(GetPartType()));
	return partValues;
}

inline int KWDGPart::GetCellNumber() const
{
	return nCellNumber;
}

inline KWDGCell* KWDGPart::GetHeadCell() const
{
	return headCell;
}

inline KWDGCell* KWDGPart::GetTailCell() const
{
	return tailCell;
}

inline void KWDGPart::GetNextCell(KWDGCell*& cell) const
{
	require(cell != NULL);
	require(cell->GetAttributeNumber() > GetAttributeIndex());
	require(cell->Check());
	require(cell->GetPartAt(GetAttributeIndex()) == this);

	cell = cast(KWDGCell*, cell->oaNextCells.GetAt(GetAttributeIndex()));
}

inline void KWDGPart::GetPrevCell(KWDGCell*& cell) const
{
	require(cell != NULL);
	require(cell->GetAttributeNumber() > GetAttributeIndex());
	require(cell->Check());
	require(cell->GetPartAt(GetAttributeIndex()) == this);

	cell = cast(KWDGCell*, cell->oaPrevCells.GetAt(GetAttributeIndex()));
}

inline int KWDGPart::GetAttributeIndex() const
{
	return (attribute == NULL ? 0 : attribute->GetAttributeIndex());
}

// Classe KWDGInterval

inline KWDGInterval::KWDGInterval()
{
	cLowerBound = 0;
	cUpperBound = 0;
}

inline KWDGInterval::~KWDGInterval() {}

inline int KWDGInterval::GetValueType() const
{
	return KWType::Continuous;
}

inline void KWDGInterval::SetLowerBound(Continuous cValue)
{
	cLowerBound = cValue;
}

inline Continuous KWDGInterval::GetLowerBound() const
{
	return cLowerBound;
}

inline void KWDGInterval::SetUpperBound(Continuous cValue)
{
	cUpperBound = cValue;
}

inline Continuous KWDGInterval::GetUpperBound() const
{
	return cUpperBound;
}

inline Continuous KWDGInterval::GetMinLowerBound()
{
	assert(KWContinuous::GetMissingValue() < KWContinuous::GetMinValue());
	return KWContinuous::GetMissingValue();
}

inline Continuous KWDGInterval::GetMaxUpperBound()
{
	return KWContinuous::GetMaxValue();
}

// Classe KWDGValueSet

inline KWDGValueSet::KWDGValueSet()
{
	headValue = NULL;
	tailValue = NULL;
	nValueNumber = 0;
	bIsDefaultPart = false;
}

inline KWDGValueSet::~KWDGValueSet()
{
	DeleteAllValues();
}

inline KWDGValueSet* KWDGValueSet::Clone() const
{
	KWDGValueSet* cloneValueSet;
	cloneValueSet = Create();
	cloneValueSet->CopyFrom(this);
	return cloneValueSet;
}

inline boolean KWDGValueSet::IsDefaultPart() const
{
	return bIsDefaultPart;
}

inline int KWDGValueSet::GetValueNumber() const
{
	assert(nValueNumber > 0 or IsDefaultPart() or GetHeadValue() == NULL or GetValueType() != KWType::Symbol);

	// On assure que le seul cas sans aucune valeur est le cas de la partie par defaut reduite a la StarValue
	// Cela peut arriver si la partie a ete "nettoyee" pour gagner de la place, notamment quand on exporte
	// une grille dans un fichier de dictionnaire .kdic. Le modele exporte n'est pas tout a fait valide,
	// puisque l'on a perdu le compte des valeurs du groupe poubelle (bug a corriger plus tard).
	// Quand on reimporte le model pour le deploiement, il faut contourner ces incoherences.
	// Dans ce cas, ou rend une nombre de valeurs egal 1 a, ce qui a le merite d'avoir des couts de grille
	// valides numeriquement, meme s'ils ne corrrespondent pas au vrai model qui aurait du memoriser
	// le nombre exacte de valeurs du groupe
	// Ce cas peut egalement se produire dans certaines phases algorithmiques ou
	// la liste de valeurs est effectivement vide
	if (nValueNumber == 0 and IsDefaultPart())
		return 1;
	else
		return nValueNumber;
}

inline KWDGValue* KWDGValueSet::GetHeadValue() const
{
	return headValue;
}

inline KWDGValue* KWDGValueSet::GetTailValue() const
{
	return tailValue;
}

inline void KWDGValueSet::GetNextValue(KWDGValue*& value) const
{
	require(value != NULL);
	value = value->nextValue;
}

inline void KWDGValueSet::GetPrevValue(KWDGValue*& value) const
{
	require(value != NULL);
	value = value->prevValue;
}

// Classe KWDGSymbolValueSet

inline KWDGSymbolValueSet::KWDGSymbolValueSet() {}

inline KWDGSymbolValueSet::~KWDGSymbolValueSet() {}

inline int KWDGSymbolValueSet::GetValueType() const
{
	return KWType::Symbol;
}

inline KWDGValueSet* KWDGSymbolValueSet::Create() const
{
	return new KWDGSymbolValueSet;
}

inline KWDGValue* KWDGSymbolValueSet::AddSymbolValue(const Symbol& sValue)
{
	KWDGValue* value;

	// Creation de la valeur
	value = NewSymbolValue(sValue);

	// Ajout en fin de la liste des valeurs
	AddTailValue(value);

	// On retourne la valeur cree
	return value;
}

inline KWDGValue* KWDGSymbolValueSet::AddValueCopy(const KWDGValue* sourceValue)
{
	return AddSymbolValue(cast(KWDGSymbolValue*, sourceValue)->GetSymbolValue());
}

inline KWDGValue* KWDGSymbolValueSet::NewSymbolValue(const Symbol& sValue) const
{
	return new KWDGSymbolValue(sValue);
}

// Class KWDGVarPartSet

inline KWDGVarPartSet::KWDGVarPartSet() {}

inline KWDGVarPartSet::~KWDGVarPartSet() {}

inline int KWDGVarPartSet::KWDGVarPartSet::GetValueType() const
{
	return KWType::VarPart;
}

inline KWDGValueSet* KWDGVarPartSet::Create() const
{
	return new KWDGVarPartSet;
}

inline KWDGValue* KWDGVarPartSet::AddVarPart(KWDGPart* varPart)
{
	KWDGValue* value;

	require(varPart != NULL);

	// Creation de la valeur
	value = NewVarPartValue(varPart);

	// Ajout en fin de la liste des valeurs
	AddTailValue(value);

	// On retourne la valeur cree
	return value;
}

inline KWDGValue* KWDGVarPartSet::AddValueCopy(const KWDGValue* sourceValue)
{
	return AddVarPart(cast(KWDGVarPartValue*, sourceValue)->GetVarPart());
}

inline const ALString KWDGVarPartSet::GetClassLabel() const
{
	return "VarPart set";
}

inline KWDGVarPartValue* KWDGVarPartSet::NewVarPartValue(KWDGPart* varPart) const
{
	require(varPart != NULL);
	return new KWDGVarPartValue(varPart);
}

// Classe KWDGVarPartValue

inline KWDGVarPartValue::KWDGVarPartValue(KWDGPart* varPartValue)
{
	require(varPartValue != NULL);
	varPart = varPartValue;
}

inline KWDGVarPartValue::~KWDGVarPartValue() {}

inline int KWDGVarPartValue::GetValueType() const
{
	return KWType::VarPart;
}

inline KWDGPart* KWDGVarPartValue::GetVarPart() const
{
	return varPart;
}

inline NUMERIC KWDGVarPartValue::GetNumericKeyValue() const
{
	return varPart;
}

inline boolean KWDGVarPartValue::IsDefaultValue() const
{
	return false;
}

inline void KWDGVarPartValue::SetValueFrequency(int nFrequency)
{
	require(varPart != NULL);
	varPart->SetPartFrequency(nFrequency);
}

inline int KWDGVarPartValue::GetValueFrequency() const
{
	require(varPart != NULL);
	return varPart->GetPartFrequency();
}

// Classe KWDGInnerAttributes
inline KWDGInnerAttributes::KWDGInnerAttributes()
{
	nVarPartGranularity = 0;
	nRefCount = 0;
}

inline KWDGInnerAttributes::~KWDGInnerAttributes()
{
	assert(nRefCount == 0);
	DeleteAll();
}

inline int KWDGInnerAttributes::GetInnerAttributeNumber() const
{
	assert(odInnerAttributes.GetCount() == oaInnerAttributes.GetSize());
	return oaInnerAttributes.GetSize();
}

// Classe KWDGValue

inline KWDGValue::KWDGValue()
{
	dTypicality = 0;
	prevValue = NULL;
	nextValue = NULL;
}

inline KWDGValue::~KWDGValue() {}

inline Symbol& KWDGValue::GetSymbolValue() const
{
	static Symbol sEmpty;
	assert(false);
	return sEmpty;
}

inline KWDGPart* KWDGValue::GetVarPart() const
{
	assert(false);
	return NULL;
}

inline int KWDGValue::CompareFrequency(const KWDGValue* otherValue) const
{
	int nCompare;

	require(otherValue != NULL);
	require(otherValue->GetValueType() == GetValueType());

	nCompare = -GetValueFrequency() + otherValue->GetValueFrequency();
	if (nCompare == 0)
		nCompare = CompareValue(otherValue);
	return nCompare;
}

inline void KWDGValue::SetTypicality(double dValue)
{
	require(0 <= dValue and dValue <= 1);
	dTypicality = dValue;
}

inline double KWDGValue::GetTypicality() const
{
	return dTypicality;
}

// Classe KWDGSymbolValue

inline KWDGSymbolValue::KWDGSymbolValue(const Symbol& sValue)
{
	sSymbolValue = sValue;
	nValueFrequency = 0;
}

inline KWDGSymbolValue::~KWDGSymbolValue() {}

inline int KWDGSymbolValue::GetValueType() const
{
	return KWType::Symbol;
}

inline Symbol& KWDGSymbolValue::GetSymbolValue() const
{
	return sSymbolValue;
}

inline NUMERIC KWDGSymbolValue::GetNumericKeyValue() const
{
	return sSymbolValue.GetNumericKey();
}

inline boolean KWDGSymbolValue::IsDefaultValue() const
{
	return sSymbolValue == Symbol::GetStarValue();
}

inline void KWDGSymbolValue::SetValueFrequency(int nFrequency)
{
	require(nFrequency >= 0);
	nValueFrequency = nFrequency;
}

inline int KWDGSymbolValue::GetValueFrequency() const
{
	return nValueFrequency;
}

inline int KWDGSymbolValue::CompareValue(const KWDGValue* otherValue) const
{
	require(otherValue != NULL);
	return GetSymbolValue().CompareValue(otherValue->GetSymbolValue());
}

// Classe KWDGCell

inline KWDGCell::KWDGCell()
{
	prevCell = NULL;
	nextCell = NULL;
	nCellFrequency = 0;
}

inline KWDGCell::~KWDGCell()
{
	// Reinitialisation pour faciliter le debug
	debug(prevCell = NULL);
	debug(nextCell = NULL);
	debug(nCellFrequency = 0);
	debug(ivFrequencyVector.SetSize(0));
	debug(oaParts.SetSize(0));
	debug(oaNextCells.SetSize(0));
	debug(oaPrevCells.SetSize(0));
}

inline int KWDGCell::GetCellFrequency() const
{
	ensure(ivFrequencyVector.GetSize() == 0 or ComputeTotalFrequency() == nCellFrequency);
	return nCellFrequency;
}

inline void KWDGCell::SetCellFrequency(int nFrequency)
{
	require(nFrequency >= 0);
	require(GetTargetValueNumber() == 0);
	nCellFrequency = nFrequency;
}

inline int KWDGCell::GetTargetValueNumber() const
{
	return ivFrequencyVector.GetSize();
}

inline int KWDGCell::GetTargetFrequencyAt(int nTarget) const
{
	require(0 <= nTarget and nTarget < GetTargetValueNumber());
	ensure(ivFrequencyVector.GetAt(nTarget) >= 0);
	return ivFrequencyVector.GetAt(nTarget);
}

inline void KWDGCell::SetTargetFrequencyAt(int nTarget, int nFrequency)
{
	require(0 <= nTarget and nTarget < GetTargetValueNumber());
	require(0 <= nFrequency);
	nCellFrequency += nFrequency - ivFrequencyVector.GetAt(nTarget);
	ivFrequencyVector.SetAt(nTarget, nFrequency);
}

inline void KWDGCell::UpgradeTargetFrequencyAt(int nTarget, int nDeltaFrequency)
{
	require(0 <= nTarget and nTarget < GetTargetValueNumber());
	require(0 <= nDeltaFrequency);
	nCellFrequency += nDeltaFrequency;
	ivFrequencyVector.UpgradeAt(nTarget, nDeltaFrequency);
	assert(nCellFrequency >= 0);
	assert(ivFrequencyVector.GetAt(nTarget) >= 0);
}

inline int KWDGCell::GetAttributeNumber() const
{
	return oaParts.GetSize();
}

inline KWDGPart* KWDGCell::GetPartAt(int nAttributeIndex) const
{
	require(0 <= nAttributeIndex and nAttributeIndex < GetAttributeNumber());
	return cast(KWDGPart*, oaParts.GetAt(nAttributeIndex));
}
