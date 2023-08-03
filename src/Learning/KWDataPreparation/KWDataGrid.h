// Copyright (c) 2023 Orange. All rights reserved.
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
// CH IV Begin
class KWDGVarPartSet;
class KWDGVarPartValue;
class KWDGInnerAttributes;
// CH IV End

#include "KWVersion.h"
#include "SortedList.h"
#include "KWSymbol.h"
#include "KWDataGridStats.h"
#include "KWSortableIndex.h"
#include "KWFrequencyVector.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGrid
//
// Deux famille de grille de donnees
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
//          - chaque variables descriptives est partitionnees en parties, intervalles ou groupes de valeurs
//          - chaque partie de variable est l'analogue d'un mot
//          - chaque instance est decrites par l'ensmeble des mots correspondant aux valeurs de ses variables
//          descriptives
//        - on a une grille � deux dimension, analogue au cas d'une grille textes x mots
//          - un attribut de la grille contient des groupes d'instances (textes), par leur identifiant
//          - l'autre attribut de la grille contients des groupes de parties de variables (mots)
//      - terminologie
//         VarPart
//           - nouveau type, pour un attribut de DataGrid
//           - extension du type Symbol, ou chaque partie de variable joue le r�le d�une valeur
//           - adjectif utilise pour specialiser les extensions des DataGrid quand elles ont un attribut de type VarPart
//         VarPartDataGrid
//           - extension d�un DataGrid, avec un attribut de type VarPart
//           - un et un seul attribut de type VarPart, plus un a plusieurs autres attributs
//           - le plus standard: un autre attribut de type Symbol, portant les identifiants des instances
//             - on se rapproche du modele standard �Instances x Variables�
//             - dans ce cas, on peut nommer cet attribut le IdentifierAttribute
//           - extensions, avec un a plusieurs attributs de type Symbol ou Continuous
//             - a explorer: IdSource x IdTarget dans le cas de l�analyse des arcs d�un graphe avec des variables par
//             arcs
//             - a explorer: Time * Identifier dans d�instances evoluant au cours du temps
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
//      - terminologie interne en fran�ais, pour des etudes/discussions techniques
//           - grille de donnees (ou coclustering)
//           - attribut de grille (ou axe, dimension)
//           - attribut interne (Inner)
//           - attribut proprietaire (Owner)
//      - terminologie externe, pour communiquer et expliquer aux utilisateurs
//           - coclustering, cluster, cocluster
//           - coclustering de variables
//             - chaque cluster est une Part, Interval ou Value group
//           - coclustering instances x variables
//             - clusters d�instances vs clusters de parties de variables
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
	//	 - son nombre de valeurs
	//   - sa liste de parties
	// Chaque partie est definie principalement par:
	//   - sa liste de valeurs
	//   - sa liste de cellules
	// Chaque cellule est defini principalement par:
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

	// CH IV Begin
	// CH IV Refactoring: question: pourquoi la gestion des VarPart attributes n'est elle pas
	//   porte par l'attribut de type VarPart qui les contient, plutot que par le DataGrid?
	//   MB: pourquoi pas, faire une etude d'impact et d'interet

	// Type de grille : standard ou VarPart (avec une attribut de type VarPart)
	boolean GetVarPartDataGrid() const;
	void SetVarPartDataGrid(boolean bValue);

	// Statut de la description des parties de variable des attributs internes : partagee ou proprietaire
	// Seules les descriptions proprietaires sont detruites lors de la destruction de la grille
	boolean GetVarPartsShared() const;
	void SetVarPartsShared(boolean bValue) const;

	// Acces aux attributs internes
	void SetInnerAttributes(KWDGInnerAttributes* attributes);
	KWDGInnerAttributes* GetInnerAttributes() const;
	// CH IV End

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

	// Recherche d'un attribut par son nom (recherche couteuse, par parcours exhaustif des attributs)
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

	// Nombre de cellules
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

	// Verification de la validite de la structure du DataGrid
	// Controle d'integrite global (attributs, parties, cellules...)
	// Attention: operation couteuse en O(k.n^2)
	boolean Check() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Tri des parties des attributs pour preparer l'affichage
	// Les intervalles sont tries par valeur croissante
	// Les groupes par effectif decroissant, et les valeurs dans les groupes par effectif decroissant
	// CH IV Begin
	// Les clusters de parties de variables sont tries de la meme facon que les groupes
	// CH IV End
	void SortAttributeParts();

	// Verification du tri des parties : couteuse, a utiliser essentiellement dans les assertions
	boolean AreAttributePartsSorted() const;

	// CH IV Begin
	// Verification du tri des parties des attributs internes : couteuse, a utiliser essentiellement dans les
	// assertions
	boolean AreInnerAttributePartsSorted() const;
	// CH IV End

	// Import/export avec les objets KWDataGridStats, qui permettent un stockage compact (mais fige)
	// de tout type de grilles de donnees
	// Les possibilites d'echange sont limitee par celles de la classe KWDataGrid, a savoir:
	//   - soit entierement non supervise
	//   - soit supervise avec un attribut cible symbolique implicite (GetTargetValueNumber() > 0)
	//   - soit supervise avec un attribut cible explicite (GetTargetAttribute() != NULL)
	// L'objet source doit etre valide, et l'objet cible doit etre vide
	void ImportDataGridStats(const KWDataGridStats* dataGridStats);
	void ExportDataGridStats(KWDataGridStats* dataGridStats) const;

	// Affichage
	void Write(ostream& ost) const override;
	void WriteTargetValues(ostream& ost) const;
	void WriteAttributes(ostream& ost) const;
	void WriteAttributeParts(ostream& ost) const;
	void WriteCells(ostream& ost) const;
	// CH IV Begin
	void WriteInnerAttributes(ostream& ost) const;
	// CH IV End

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

	////////////////////////////////////////////////////////////////////////////
	// Methodes de creations virtuelles, permettant de specialiser les entites
	// d'un DataGrid dans une sous-classe

	// Creation d'un attribut
	// Les creations de partie d'attribut se font depuis une methode NewPart dans les attributs
	virtual KWDGAttribute* NewAttribute() const;

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

	// CH IV Begin
	// Type VarPart d'une grille admettant un attribut de type VarPart
	// Par defaut a false
	boolean bVarPartDataGrid;

	// Attributs internes dans les attributs de type VarPart
	KWDGInnerAttributes* innerAttributes;

	// Statut proprietaire ou partage de la description des attributs internes
	// Mutable car modifie par HandleOptimizationStep
	mutable boolean bVarPartsShared;
	// CH IV End
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

	// CH IV Begin
	// Acces au nom d'attribut de grille dont l'attribut interne fait eventuellement partie
	// Dans le cas d'un attribut de grille simple (coclustering standard), renvoie vide
	// Dans le cas d'un attribut interne, renvoie le nom de l'attribut de grille de type VarPart qui contient
	// l'attribut
	ALString GetOwnerAttributeName() const;
	void SetOwnerAttributeName(ALString sName);

	// CH IV Refactoring: ajouter une methode boolean IsInnerAttribute() const? OK
	//   permettrait de remplacer tous les tests de type GetOwnerAttributeName() == "" par IsInVarParts() (idem pour
	//   !=)

	// CH IV End

	/////////////////////////////////////////////
	// Specifications de l'attribut operande

	// Nom de l'attribut
	void SetAttributeName(const ALString& sValue);
	const ALString& GetAttributeName() const;

	// CH IV Begin
	// Type de l'attribut (Symbol ou Continuous ou de type Parties de variables)
	// CH IV End
	// Le type n'est modifiable qu'une seule fois, pour l'initialisation
	void SetAttributeType(int nValue);
	int GetAttributeType() const;

	// Fonction de l'attribut (source ou cible, source par defaut)
	// Seul le dernier attribut d'une grille peut etre cible
	void SetAttributeTargetFunction(boolean bTargetAttribute);
	boolean GetAttributeTargetFunction() const;

	// Index de l'attribut dans le tableau des operandes
	int GetAttributeIndex() const;

	// CH V9 TODO :
	// Refactoring
	// - supprimer le GetTrueValueNumber qui est desormais redondant avec le GetInitialValueNumber (est encore
	// utilise dans les anciennes methodes de calcul de cout)
	// - nGranularizedValueNumber : nombre de valeurs apres granularisation
	// - nStoredValueNumber correspond aux modalites stockees
	// en plus des valeurs vues dans les donnes la StarValue
	// en moins les valeurs eventuellement nettoyees lors de la creation du fourre-tout en supervise

	// Nombre initial de valeurs de l'attribut
	// Attribut continu : nombre total d'instances
	// Attribut categoriel : nombre de valeurs distinctes (la StarValue n'est plus comptee ici)
	// On accede au nombre de valeurs distinctes avec la StarValue dans le cas categoriel
	// via la methode GetStoredValueNumber()
	// CH IV Begin
	// Attribut parties de variables : nombre de parties de variables distinctes
	// CH IV End
	void SetInitialValueNumber(int nValue);
	int GetInitialValueNumber() const;

	// Nombre de valeurs stockees pour l'attribut (avec la valeur speciale en plus dans le cas categoriel)
	int GetStoredValueNumber() const;

	// CH V9 TODO a supprimer a terme
	// Nombre de valeurs initiales reelles de l'attribut
	int GetTrueValueNumber() const;

	// Nomre de valeurs apres granularisation
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

	// CH IV Begin
	// CH IV Refactoring: redondant avec KWDGInnerAttributes?
	// Methodes specifiques aux attributs de type VarPart
	// Acces au nombre d'attributs interne dans un attribut de grille de type VarPart
	int GetInnerAttributeNumber() const;
	void SetInnerAttributeNumber(int nValue);

	// Acces au tableau des noms des attributs internes dans un attribut de grille de type VarPart
	const ALString& GetInnerAttributeNameAt(int nIndex) const;
	void SetInnerAttributeNameAt(int nAttributeIndex, const ALString& sInnerAttributeName);

	// Tri du tableau des noms des attributs internes
	// CH IV Refactoring: est-ce toujours utile si on deplacer la gestion des attributs internes vers l'atttribute
	// de grille de type VarPart???
	void SortInnerAttributeNames();

	// Initialisation des parties de l'attribut de type VarPart a partir des attributs internes initialises en
	// creant une partie par partie de variable
	void CreateVarPartsSet();
	// CH IV End

	////////////////////////////////////////////////
	// Gestion des parties de l'attribut
	// Memoire: les parties appartiennent a l'attribut

	// Creation d'une partie et ajout en fin de liste
	// Le type de l'attribut doit avoir ete specifie en prealable.
	// Renvoie la partie cree (avec le type de l'attribut)
	// CH IV Begin
	// Si l'attribut est de type VarPart, la partie renvoyee est de type KWDGVarPart*
	// CH IV End
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
	// Initialisation du fourre-tout
	// Sans recopie (creation a partir de la methode ConvertToCleanedValueSet)
	void SetCatchAllValueSet(KWDGValueSet* valueSet);
	// Avec recopie
	void InitializeCatchAllValueSet(KWDGValueSet* valueSet);

	// Acces au nombre de modalites du groupe fourre-tout
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

	// Recherche de la partie contenant une valeur numerique ou symbolique
	// (doit etre compatible avec le type de l'attribut)
	// Attention a ne pas modifier les valeurs (intervalles ou ensemble de valeurs)
	// pendant l'utilisation de l'indexation
	KWDGPart* LookupContinuousPart(Continuous cValue);
	KWDGPart* LookupSymbolPart(const Symbol& sValue);

	// CH IV Begin
	// Recherche de la partie contenant une partie de variables
	// (doit etre compatible avec le type de l'attribut)
	KWDGPart* LookupVarPart(KWDGPart* varPart);
	// CH IV End

	///////////////////////////////
	// Services divers

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
	// CH IV Begin
	// Les groupes (de valeurs ou de parties de variable) sont tries par effectif decroissant
	// Les valeurs dans les groupes sont tries par effectif decroissant
	// Les parties de variables dans les groupes sont tries par attribut puis en appliquant le tri des intervalles
	// ou des groupes au sein de chaque attribut CH IV End
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

	// Structure d'indexation des parties dans le cas symbolique
	// Dictionnaire des parties indexe par les valeurs (Symbol) des parties
	NumericKeyDictionary nkdParts;
	KWDGPart* starValuePart;
	boolean bIsIndexed;

	// CH IV Begin
	// Structure d'indexation, des parties dans le cas VarPart
	NumericKeyDictionary nkdVarPartSets;

	// Tableau des noms des attributs internes dans un attribut de type VarPart
	StringVector svInnerAttributeNames;

	// CH IV Refactoring: supprimer???
	// Attribut de type VarPart dont depend un attribut interne
	// Par defaut a NULL pour un attribut de type Simple (numerique ou categoriel)
	// KWDGAttribute* attributeAxis;

	// Nom de l'attribut de type VarPart dont depend un attribut interne
	// Par defaut a vide pour un attribut de type Simple (numerique ou categoriel)
	ALString sOwnerAttributeName;
	// CH IV End
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

	// Acces aux valeurs, selon le type
	KWDGInterval* GetInterval() const;
	KWDGValueSet* GetValueSet() const;
	// CH IV Begin
	KWDGVarPartSet* GetVarPartSet() const;
	// CH IV End

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
	// Attention, cela doit correspondre exactement au cumul des effectifs des cellules de la parties
	void SetPartFrequency(int nValue);

	// Controle d'integrite local a la partie (valeurs, cellules de la partie)
	boolean Check() const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Affichage
	void Write(ostream& ost) const override;
	void WriteValues(ostream& ost) const;
	void WriteCells(ostream& ost) const;

	// Libelle complet dans le cas d'une partie de variable, base sur le libelle de l'identifiant interne et celui
	// de la partie
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
	virtual KWDGValueSet* NewValueSet() const;
	// CH IV Begin
	virtual KWDGVarPartSet* NewVarPartSet() const;
	// CH IV End

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

	// Gestion des valeurs de la parties
	// Un seul objet, correspondant au type de la partie, doit etre non nul
	KWDGInterval* interval;
	KWDGValueSet* valueSet;
	// CH IV Begin
	KWDGVarPartSet* varPartSet;
	// CH IV End
};

// Comparaison de deux parties numeriques, sur la base de leur borne sup
int KWDGPartContinuousCompare(const void* elem1, const void* elem2);

// Comparaison de deux parties symboliques, sur la base de leur premiere valeur
int KWDGPartSymbolCompare(const void* elem1, const void* elem2);

// Comparaison de deux parties symboliques, par effectif decroissant
int KWDGPartSymbolCompareDecreasingFrequency(const void* elem1, const void* elem2);

// CH IV Begin
// Comparaison de deux parties de type parties de variables sur la base de leur premiere partie de variable :
// - le nom de l'attribut des premieres parties
// - la comparaison des premieres parties si l'attribut est semblable
int KWDGPartVarPartCompare(const void* elem1, const void* elem2);

// Comparaison de deux parties de parties de variables, par effectif decroissant
int KWDGPartVarPartCompareDecreasingFrequency(const void* elem1, const void* elem2);
// CH IV End
//////////////////////////////////////////////////////////////////////////////
// Classe KWDGInterval
// Intervalle de valeurs
class KWDGInterval : public Object
{
public:
	// Constructeur
	KWDGInterval();
	~KWDGInterval();

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

	// Controle d'integrite
	boolean Check() const override;

	// Import des valeurs d'un intervalle source, devant etre adjacent au premier
	// intervalle. L'intervalle source est reinitialise
	void Import(KWDGInterval* sourceInterval);

	// CH IV Begin
	// Mise a jour des valeurs de l'intervalle a partir des valeurs d'un intervalle source, devant etre adjacent au
	// premier intervalle. L'intervalle source n'est pas modifie
	void UpgradeFrom(const KWDGInterval* sourceInterval);
	// CH IV End
	// Copie
	void CopyFrom(const KWDGInterval* sourceInterval);

	// Affichage
	void Write(ostream& ost) const override;

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
// Classe KWDGValueSet
// Ensemble de valeurs d'une partie symbolique
class KWDGValueSet : public Object
{
public:
	// Constructeur
	KWDGValueSet();
	~KWDGValueSet();

	////////////////////////////////////////////////////////////////
	// Gestion des valeurs de la parties sous forme de liste
	// Memoire: les valeurs appartiennent a la partie

	// Creation d'une valeur et ajout en fin de liste
	// Renvoie la valeur cree
	KWDGValue* AddValue(const Symbol& sValue);

	// Destruction d'une valeur de la liste
	void DeleteValue(KWDGValue* value);

	// Destruction de toutes les valeurs
	void DeleteAllValues();

	// Test de validite d'une valeur (si elle appartient a la partie)
	boolean CheckValue(KWDGValue* value) const;

	// CH V9 TODO
	// Refactoring : distinguer nInitialValueNumber le nombre initial de valeurs
	// nGranularizedValueNumber : le nombre de modalites apres granularisation
	// nStoredValueNumber : le nombre de modalites stockees
	// Nombre de valeurs
	int GetValueNumber() const;

	// Test si la partie est la partie par defaut (si elle contient la valeur speciale)
	boolean IsDefaultPart() const;

	// Nombre de valeurs reelles (valeurs sauf l'eventuelle valeur speciale)
	int GetTrueValueNumber() const;

	// Parcours de tous les valeurs
	KWDGValue* GetHeadValue() const;
	KWDGValue* GetTailValue() const;
	void GetNextValue(KWDGValue*& value) const;
	void GetPrevValue(KWDGValue*& value) const;

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

	// Calue et renvoie un ValueSet compresse avec la modalite d'effectif le plus eleve et la StarValue
	// Ne modifie pas le ValueSet
	KWDGValueSet* ComputeCleanedValueSet() const;

	///////////////////////////////
	// Services divers

	// Controle d'integrite
	boolean Check() const override;

	// Calcul de l'effectif cumule des valeurs
	int ComputeTotalFrequency() const;

	// Import des valeurs d'une partie source, devant etre disjointe de la premiere
	// partie. La partie source est reinitialise
	void Import(KWDGValueSet* sourceValueSet);

	// Copie
	void CopyFrom(const KWDGValueSet* sourceValueSet);

	// Ajout de nouvelles valeurs recopiees depuis une source
	void UpgradeFrom(const KWDGValueSet* sourceValueSet);

	// Tri des valeurs par effectif decroissant, pour preparer l'affichage
	void SortValues();

	// Verification du tri des valeurs par effectif decroissant : couteux, a utiliser essentiellement dans les
	// assertions
	boolean AreValuesSorted() const;

	// Affichage
	void Write(ostream& ost) const override;
	void WriteValues(ostream& ost) const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////
	///// Implementation
protected:
	// Nom externe d'une valeur, de facon a assurer que l'unicite des ObjectLabel
	// Les valeurs sont entourees de quotes, si elle contiennent un des caracteres impliquee dans la construction du
	// ObjectLabel
	const ALString GetExternalValue(const Symbol& sValue) const;

	// Tri des valeurs selon une fonction de tri
	// La valeur speciale est toujours mise en dernier, independament du critere de tri
	void InternalSortValues(CompareFunction fCompare);

	// Methodes de creations virtuelles, permettant de specialiser la creation d'une valeur
	// dans une sous-classe
	virtual KWDGValue* NewValue(const Symbol& sValue) const;

	// Methode indiquant si les donnees sont emulee
	virtual boolean GetEmulated() const;

	// Gestion de la liste doublement chainee des cellules
	KWDGValue* headValue;
	KWDGValue* tailValue;
	int nValueNumber;
	boolean bIsDefaultPart;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGValue
// Valeur symbolique appartenant a une partie de valeur
class KWDGValue : public Object
{
public:
	// Constructeur
	KWDGValue(const Symbol& sValue);
	~KWDGValue();

	// Valeur
	Symbol& GetValue() const;

	// Effectif lie a la valeur
	void SetValueFrequency(int nFrequency);
	int GetValueFrequency() const;

	// Affichage
	void Write(ostream& ost) const override;

	///////////////////////////////
	///// Implementation
protected:
	friend class KWDGValueSet;

	// Attributs
	KWDGValue* prevValue;
	KWDGValue* nextValue;
	mutable Symbol sSymbolValue;
	int nValueFrequency;
};

// Comparaison de deux valeurs symboliques, par effectif decroissant
int KWDGValueCompareDecreasingFrequency(const void* elem1, const void* elem2);

//////////////////////////////////////////////////////////////////////////////
// CH IV Begin
// Classe KWDGVarPartSet
// Ensemble de parties de variables (constitue une partie d'un attribut de grille de type VarPart)
class KWDGVarPartSet : public Object
{
public:
	// Constructeur
	KWDGVarPartSet();
	~KWDGVarPartSet();

	////////////////////////////////////////////////////////////////
	// Gestion des parties de variable sous forme de liste
	// Memoire: les parties de variable appartiennent a la partie

	// Creation d'une partie de variable et ajout en fin de liste
	// Renvoie la partie de variable cree
	KWDGVarPartValue* AddVarPart(KWDGPart* varPart);

	// Destruction d'une partie de variable de la liste
	void DeleteVarPartValue(KWDGVarPartValue* varPartValue);

	// Destruction de toutes les parties de variable
	void DeleteAllVarPartValues();

	// Test de validite d'une partie de variable (si elle appartient a la partie)
	boolean CheckVarPart(KWDGVarPartValue* varPartValue) const;

	// Nombre de parties de variable
	int GetVarPartNumber() const;

	// Parcours de toutes les parties
	KWDGVarPartValue* GetHeadVarPart() const;
	KWDGVarPartValue* GetTailVarPart() const;
	void GetNextVarPart(KWDGVarPartValue*& varPartValue) const;
	void GetPrevVarPart(KWDGVarPartValue*& varPartValue) const;

	///////////////////////////////
	// Services divers

	// Controle d'integrite
	boolean Check() const override;

	// Calcul de l'effectif cumule des parties de variable
	int ComputeTotalFrequency() const;

	// Import des parties d'un ensemble  source, devant etre disjointe de la premiere
	// partie. La cible est reinitialisee
	void Import(KWDGVarPartSet* sourceVarPartSet);

	// Copie
	void CopyFrom(const KWDGVarPartSet* sourceVarPartSet);

	// Copie avec creation de nouvelles parties de variables qui alimente un nouveau KWDGInnerAttributes;
	void CopyWithNewVarPartsFrom(const KWDGVarPartSet* sourceVarPartSet,
				     KWDGInnerAttributes* targetInnerAttributes);

	// Ajout de nouvelles parties recopiees depuis une source
	void UpgradeFrom(const KWDGVarPartSet* sourceVarPartSet);

	// Tri des parties de variable par nom de variable puis tri des parties de la meme variable, pour preparer
	// l'affichage
	void SortVarPartValues();

	// Verification du tri des parties de variable : couteux, a utiliser essentiellement dans les assertions
	boolean AreVarPartValuesSorted() const;

	// Affichage
	void Write(ostream& ost) const override;
	void WriteVarParts(ostream& ost) const;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////
	///// Implementation
protected:
	// Tri des parties de variable selon une fonction de tri
	void InternalSortValues(CompareFunction fCompare);

	// Methodes de creations virtuelles, permettant de specialiser la creation d'une valeur
	// dans une sous-classe
	virtual KWDGVarPartValue* NewVarPartValue(KWDGPart* varPart) const;

	// Methode indiquant si les donnees sont emulees
	virtual boolean GetEmulated() const;

	// Gestion de la liste doublement chainee des cellules
	KWDGVarPartValue* headVarPart;
	KWDGVarPartValue* tailVarPart;
	int nVarPartNumber;
};

//////////////////////////////////////////////////////////////////////////////
// Classe KWDGVarPartValue
// Partie de variable appartenant a une partie de parties de variable
class KWDGVarPartValue : public Object
{
public:
	// Constructeur
	KWDGVarPartValue(KWDGPart* varPart);
	~KWDGVarPartValue();

	// Partie de variable
	KWDGPart* GetVarPart() const;

	// Effectif lie a la partie de variable
	void SetVarPartFrequency(int nFrequency);
	int GetVarPartFrequency() const;

	// Affichage
	void Write(ostream& ost) const override;

	///////////////////////////////
	///// Implementation
protected:
	friend class KWDGVarPartSet;

	// Attributs
	KWDGPart* varPart;
	KWDGVarPartValue* prevVarPartValue;
	KWDGVarPartValue* nextVarPartValue;
};

// Comparaison de deux parties de variable, par nom de variable puis par comparaison des parties si elles dependent de
// la meme variable
int KWDGVarPartValueCompareAttributeNameAndVarPart(const void* elem1, const void* elem2);

// Comparaison de deux objets KWSortableObject contenant des KWDGVarPartValue
int KWSortableObjectCompareVarPart(const void* elem1, const void* elem2);

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
	KWDGAttribute* LookupInnerAttribute(const ALString& sAttributeName);

	// Acces a la granularite des parties de variable, partages par tous les attributs
	int GetVarPartGranularity() const;
	void SetVarPartGranularity(int nValue);

	///////////////////////////////
	// Services divers

	// Controle d'integrite
	boolean Check() const override;

	// Nettoyage
	void DeleteAll();

	///////////////////////////////
	///// Implementation
protected:
	int nVarPartGranularity;
	ObjectDictionary odInnerAttributes;
	ObjectArray oaInnerAttributes;
};
// CH IV End

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
// CH IV Begin
inline boolean KWDataGrid::GetVarPartDataGrid() const
{
	return bVarPartDataGrid;
}

inline void KWDataGrid::SetVarPartDataGrid(boolean bValue)
{
	bVarPartDataGrid = bValue;
}

inline boolean KWDataGrid::GetVarPartsShared() const
{
	return bVarPartsShared;
}

inline void KWDataGrid::SetVarPartsShared(boolean bValue) const
{
	bVarPartsShared = bValue;
}

inline void KWDataGrid::SetInnerAttributes(KWDGInnerAttributes* attributes)
{
	innerAttributes = attributes;
}

inline KWDGInnerAttributes* KWDataGrid::GetInnerAttributes() const
{
	return innerAttributes;
}
// CH IV End
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
// CH IV Begin
inline ALString KWDGAttribute::GetOwnerAttributeName() const
{
	return sOwnerAttributeName;
}

inline void KWDGAttribute::SetOwnerAttributeName(ALString sName)
{
	sOwnerAttributeName = sName;
}

// CH IV End
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
	// CH IV Begin
	require(KWType::IsCoclusteringType(nValue));
	// CH IV End
	nAttributeType = nValue;
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

inline int KWDGAttribute::GetStoredValueNumber() const
{
	if (nAttributeType == KWType::Symbol)
		return nInitialValueNumber + 1;
	else
		return nInitialValueNumber;
}

inline int KWDGAttribute::GetTrueValueNumber() const
{
	// if (nAttributeType == KWType::Symbol)
	//	return nInitialValueNumber - 1;
	// else
	//  Desormais nInitialValueNumber est le TrueValueNumber
	return nInitialValueNumber;
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
// CH IV Begin
inline int KWDGAttribute::GetInnerAttributeNumber() const
{
	ensure(nAttributeType == KWType::VarPart or svInnerAttributeNames.GetSize() == 0);
	return svInnerAttributeNames.GetSize();
}

inline void KWDGAttribute::SetInnerAttributeNumber(int nValue)
{
	require(nAttributeType == KWType::VarPart or nValue == 0);
	svInnerAttributeNames.SetSize(nValue);
}

inline const ALString& KWDGAttribute::GetInnerAttributeNameAt(int nIndex) const
{
	require(nAttributeType == KWType::VarPart);
	require(0 <= nIndex and nIndex < svInnerAttributeNames.GetSize());

	return svInnerAttributeNames.GetAt(nIndex);
}

inline void KWDGAttribute::SetInnerAttributeNameAt(int nIndex, const ALString& sInnerAttributeName)
{
	require(nAttributeType == KWType::VarPart);
	require(0 <= nIndex and nIndex < svInnerAttributeNames.GetSize());

	svInnerAttributeNames.SetAt(nIndex, sInnerAttributeName);
}

inline void KWDGAttribute::SortInnerAttributeNames()
{
	require(nAttributeType == KWType::VarPart);
	svInnerAttributeNames.Sort();
}
// CH IV End

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
	// CH V9 MB TODO: expliquer pourquoi il n'y a pas de require(nAttributeType == KWType::Symbol)
	return garbagePart;
}

inline void KWDGAttribute::SetGarbagePart(KWDGPart* part)
{
	// CH IV Begin
	require(nAttributeType == KWType::Symbol or nAttributeType == KWType::VarPart);
	// CH IV End
	garbagePart = part;
}

inline int KWDGAttribute::GetGarbageModalityNumber() const
{
	if (GetGarbagePart() == NULL)
		return 0;
	// Sinon
	// CH IV Begin
	else if (nAttributeType == KWType::Symbol)
		return GetGarbagePart()->GetValueSet()->GetTrueValueNumber();
	else
		return GetGarbagePart()->GetVarPartSet()->GetVarPartNumber();
	// CH IV End
}

inline KWDGValueSet* KWDGAttribute::GetCatchAllValueSet() const
{
	// CH V9 MB TODO: expliquer pourquoi il n'y a pas de require(nAttributeType == KWType::Symbol)
	// require(nAttributeType == KWType::Symbol);
	return catchAllValueSet;
}

inline void KWDGAttribute::SetCatchAllValueSet(KWDGValueSet* valueSet)
{
	require(nAttributeType == KWType::Symbol);
	if (catchAllValueSet != NULL)
		delete catchAllValueSet;
	catchAllValueSet = valueSet;
}

inline void KWDGAttribute::InitializeCatchAllValueSet(KWDGValueSet* valueSet)
{
	require(nAttributeType == KWType::Symbol);

	if (catchAllValueSet != NULL)
		delete catchAllValueSet;
	catchAllValueSet = new KWDGValueSet;
	catchAllValueSet->CopyFrom(valueSet);
}

inline void KWDGAttribute::SetCatchAllValueNumber(int nValue)
{
	require(nValue >= 0);
	nCatchAllValueNumber = nValue;
}

inline int KWDGAttribute::GetCatchAllValueNumber() const
{
	// require(nAttributeType == KWType::Symbol);
	//  Cas ou il n'y a pas de groupe fourre-tout
	if (GetCatchAllValueSet() == NULL)
		return 0;
	// Cas d'une taille de fourre-tout memorise (suite a un nettoyage du fourre-tout)
	else if (nCatchAllValueNumber != -1)
		return nCatchAllValueNumber;
	else
		return GetCatchAllValueSet()->GetValueNumber();
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
	// CH IV Begin
	ensure(GetEmulated() or nPartFrequency == ComputeCellsTotalFrequency() or
	       GetAttribute()->GetOwnerAttributeName() != "");
	// CH IV End
	return nPartFrequency;
}

inline int KWDGPart::GetPartType() const
{
	require(interval == NULL or valueSet == NULL or varPartSet == NULL);
	require((interval == NULL) + (valueSet == NULL) + (varPartSet == NULL) >= 2);

	if (interval != NULL)
		return KWType::Continuous;
	else if (valueSet != NULL)
		return KWType::Symbol;
	// CH IV Begin
	else if (varPartSet != NULL)
		return KWType::VarPart;
	// CH IV End
	else
		return KWType::Unknown;
}

inline KWDGInterval* KWDGPart::GetInterval() const
{
	require(GetPartType() == KWType::Continuous);

	ensure(interval != NULL);
	return interval;
}

inline KWDGValueSet* KWDGPart::GetValueSet() const
{
	require(GetPartType() == KWType::Symbol);

	ensure(valueSet != NULL);
	return valueSet;
}
// CH IV Begin
inline KWDGVarPartSet* KWDGPart::GetVarPartSet() const
{
	require(GetPartType() == KWType::VarPart);

	ensure(varPartSet != NULL);
	return varPartSet;
}
// CH IV End
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

inline void KWDGInterval::CopyFrom(const KWDGInterval* sourceInterval)
{
	require(sourceInterval != NULL);

	cLowerBound = sourceInterval->cLowerBound;
	cUpperBound = sourceInterval->cUpperBound;
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

inline int KWDGValueSet::GetValueNumber() const
{
	return nValueNumber;
}

inline boolean KWDGValueSet::IsDefaultPart() const
{
	return bIsDefaultPart;
}

inline int KWDGValueSet::GetTrueValueNumber() const
{
	if (bIsDefaultPart and nValueNumber > 1)
		return nValueNumber - 1;
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
// CH IV Begin
// Classe KWDGVarPartValue

inline KWDGVarPartValue::KWDGVarPartValue(KWDGPart* varPartValue)
{
	varPart = varPartValue;
	prevVarPartValue = NULL;
	nextVarPartValue = NULL;
}

inline KWDGVarPartValue::~KWDGVarPartValue() {}

inline KWDGPart* KWDGVarPartValue::GetVarPart() const
{
	return varPart;
}

inline void KWDGVarPartValue::SetVarPartFrequency(int nFrequency)
{
	varPart->SetPartFrequency(nFrequency);
}

inline int KWDGVarPartValue::GetVarPartFrequency() const
{
	return varPart->GetPartFrequency();
}

inline KWDGVarPartSet::KWDGVarPartSet()
{
	headVarPart = NULL;
	tailVarPart = NULL;
	nVarPartNumber = 0;
}

inline KWDGVarPartSet::~KWDGVarPartSet()
{
	DeleteAllVarPartValues();
}

inline int KWDGVarPartSet::GetVarPartNumber() const
{
	return nVarPartNumber;
}

inline KWDGVarPartValue* KWDGVarPartSet::GetHeadVarPart() const
{
	return headVarPart;
}

inline KWDGVarPartValue* KWDGVarPartSet::GetTailVarPart() const
{
	return tailVarPart;
}

inline void KWDGVarPartSet::GetNextVarPart(KWDGVarPartValue*& varPartValue) const
{
	require(varPartValue != NULL);

	varPartValue = varPartValue->nextVarPartValue;
}

inline void KWDGVarPartSet::GetPrevVarPart(KWDGVarPartValue*& varPartValue) const
{
	require(varPartValue != NULL);

	varPartValue = varPartValue->prevVarPartValue;
}

// Classe KWDGInnerAttributes
inline KWDGInnerAttributes::KWDGInnerAttributes()
{
	oaInnerAttributes.SetSize(0);
	odInnerAttributes.DeleteAll();
	nVarPartGranularity = 0;
}

inline KWDGInnerAttributes::~KWDGInnerAttributes()
{
	DeleteAll();
}

inline int KWDGInnerAttributes::GetInnerAttributeNumber() const
{
	assert(odInnerAttributes.GetCount() == oaInnerAttributes.GetSize());
	return oaInnerAttributes.GetSize();
}

// CH IV End
// Classe KWDGValue

inline KWDGValue::KWDGValue(const Symbol& sValue)
{
	sSymbolValue = sValue;
	nValueFrequency = 0;
	prevValue = NULL;
	nextValue = NULL;
}

inline KWDGValue::~KWDGValue() {}

inline Symbol& KWDGValue::GetValue() const
{
	return sSymbolValue;
}

inline void KWDGValue::SetValueFrequency(int nFrequency)
{
	require(nFrequency >= 0);
	nValueFrequency = nFrequency;
}

inline int KWDGValue::GetValueFrequency() const
{
	return nValueFrequency;
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
