// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataTableSliceSet;
class KWDataTableSlice;
class KWDataTableDriverSlice;
class PLShared_DataTableSliceSet;
class PLShared_DataTableSlice;

#include "KWClass.h"
#include "KWSTDatabaseTextFile.h"
#include "Vector.h"
#include "PLSharedObject.h"
#include "PLFileConcatenater.h"

///////////////////////////////////////////////////////////////////////////////////
// Table de donnees textuelle decoupee en un ensemble de tranches
// Chaque tranche contient une partie exclusive des variables et contient les memes
// instances, et peut etre stockee au moyen de plusieurs fichiers au format predefini,
// sans ligne d'entete et avec separateur de champ tabulation.
// Les variables sont uniquement de type numerique ou categorielle, eventuellement
// par bloc sparse, et non calculee.
//
// Cette classe technique est utile en interne pour le stockage temporaire efficace
// de base de donnees tabulaire en un ensemble de tranches verticales par variables,
// et horizontale par instances (les fichiers de chaque tranche de variables devant
// etre synchronises par instances).
// Cette organisation permet alors de simplifier l'implementation de taches paralleles
// pour traiter une base par tranches verticales ou horizontales
class KWDataTableSliceSet : public Object
{
public:
	/// Constructeur
	KWDataTableSliceSet();
	~KWDataTableSliceSet();

	// Specification des tranches de la base en partitionnant les attributs en troncons
	// En entree:
	//  . un dictionaire, dont les attributs ou blocs d'attributs utilises sont a partitionner
	//    (seuls les attributs numeriques ou categoriels, denses ou sparse, sont pris en compte)
	//  . un eventuel attribut cible, a ne pas prendre en compte
	//  . le nombre max d'attributs denses par troncon
	// En sortie:
	//  . des tranches avec chacune:
	//      . un dictionnaire indexe mais non compile (manque le domaine)
	//      . une liste de fichiers vide
	// Les attributs et blocs d'attribut sont dispatches de facon aleatoires dans les troncon.
	// Certain blocs trop gros peuvent etre coupes et repartis sur plusieurs troncon, avec la seule
	// contrainte que chaque tranche contient une sous-partie contigue des attributs d'un bloc
	// selon un tri par VarKey, et que les sous-partie successives se trouvent dans des tranches
	// dans le meme ordre.
	// En cas de blocs sparse, une heuristique de dimensionnement est utilisee pour potentiellement
	// prendre en compte plus d'attributs par tranche que le max du cas dense
	void ComputeSpecification(const KWClass* kwcClassToPartition, const ALString& sTargetAttributeName,
				  int nInstanceNumber, int nMaxDenseAttributeNumberPerSlice);

	// Partition d'une classe en tranche en appliquant des specification imposee en entree
	// (seuls les attributs numeriques ou categoriels, denses ou sparse, doivent etre pris en compte)
	// En entree:
	//  . un dictionaire, dont les attributs ou blocs d'attributs utilises sont a partitionner
	//  . un eventuel attribut cible, a ne pas prendre en compte
	//  . le nombre max d'attributs denses par troncon
	//  . le nombre de tranches a creer
	//  . un index lexicographique, eventuellement vide, servant aux index lixicographique des tranches crees
	//  . un vecteur d'index indiquant pour chaque attribut en entree l'index de la tranche
	//    dans laquelle il doit se trouver (-1 si l'attribut ne se retrouve dans aucune tranche)
	// En sortie:
	//  . des tranches avec chacune:
	//      . un dictionnaire indexe mais non compile (manque le domaine)
	//      . une liste de fichiers vide
	void BuildSpecificationFromClassPartition(const KWClass* kwcClassToPartition,
						  const ALString& sTargetAttributeName, int nInstanceNumber,
						  int nSliceNumber, const IntVector* ivBaseLexicographicIndex,
						  const IntVector* ivAttributeSliceIndexes);

	// Calcul des informations de chargement des attributs de chaque slice pour la classe a partitionner du sliceSet
	// En sortie, les indexes de chargement de chaque attribut de la classe a partitionner sont specifies dans les
	// slices
	void ComputeSlicesLoadIndexes(const KWClass* kwcClassToPartition);

	// Nettoyage des informations de chargement des attributs de chaque slice pour la classe a partitionner du
	// sliceSet
	void CleanSlicesLoadIndexes();

	///////////////////////////////////////////////////////////////
	// Acces a la specification des tranches

	// Acces au nom de la classe partitionnee en tranches
	// Memoire: la classe appartient a l'appelant
	const ALString& GetClassName() const;

	// Acces a l'optionnel attribut cible, ne faisant partie d'aucune tranche
	const ALString& GetTargetAttributeName() const;

	// Nombre de tranches
	int GetSliceNumber() const;

	// Acces a une tranche de la table
	// La tranche appartient a l'appele
	KWDataTableSlice* GetSliceAt(int nIndex);

	// Nombre de chunks, c'est a dire de fichiers par tranche
	int GetChunkNumber() const;

	// Nombre total d'instances sur l'ensemble des chunk
	int GetTotalInstanceNumber() const;

	// Nombre d'instances par chunk
	// Ce nombre d'instances est le meme pour chaque tranche de variables, par fichier corrrespondant au chunk
	IntVector* GetChunkInstanceNumbers();

	// Mise a jour du nombre total d'instances par cumul du nombre d'instances par chunk
	// Methode avancee
	void UpdateTotalInstanceNumber();

	// Nettoyage: suppression de toutes les tranches et des fichiers correspondants
	void Clean();

	// Indique s'il faut detruire les fichiers du slice set au moment du clean (defaut : true)
	// Methode avancee
	void SetDeleteFilesAtClean(boolean bValue);
	boolean GetDeleteFilesAtClean() const;

	////////////////////////////////////////////////////////////////////////////////
	// Calcul de statistiques sur le contenu des tranches

	// Nombre total d'attributs dans les tranches
	int GetTotalAttributeNumber() const;

	// Nombre total d'attributs denses dans les tranches
	int GetTotalDenseAttributeNumber() const;

	// Nombre total de blocs d'attributs dans les tranches
	int GetTotalAttributeBlockNumber() const;

	// Taille totale cumulee sur disque pour l'ensemble des attributs denses Symbol charges en memoire
	longint GetTotalDenseSymbolAttributeDiskSize() const;

	// Taille totale cumulee sur l'ensemble des fichiers des tranches
	longint GetTotalDataFileSize() const;

	// Total cumule des nombres de valeurs par bloc d'attribut sur l'ensemble des blocs des tranches
	longint GetTotalAttributeBlockValueNumber() const;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Methodes pour la lecture sequentielle des objets du sliceset, similaires a celle de KWDatabase
	//
	// On traite le sliceset comme une base physique, que l'on va lire au moyen d'une classe
	// s'appuyant sur tout ou partie des attributs disponibles dans les tranches, en permettant
	// egalement d'avoir des attributs calcules, soit deja existant dans les tranches
	// (et ils ne seront pas recalcules), soit nouveau (et qui serotn calcules)

	// Verification si une classe peut etre utilisee pour lire des objets depuis les tranches (pour les assertions)
	// . les variables utilisees et chargees en memoire, natives ou calculees, doivent toutes etre de type numerique
	// ou categoriel . quand elle sont de meme nom que le variables de la base de cochunks, elles doivent etre de
	// meme type:
	//   eles seront lues telles quelles a partir de celle ci (y compris si elles sont calculees)
	// . il peut y avoir de nouvelles variables utilisees calculees, pourvu qu'elles se basent uniquement sur
	//   d'autre variables calculees ou sur des variables de la base de cochunks
	// . les variables calculees, en sortie ou intermediaires, ne doivent pas etre de type relation vers une autre
	// table . il peut y avoir des variables natives absentes de la base de cochunks, pourvue qu'elles ne soit
	//   pas utilisees, ni directement, ni en entree de calcul d'autres variables
	boolean CheckReadClass(const KWClass* kwcInputClass) const;

	// Parametrage de la classe a utiliser pour  a la classe utilisee pour la lecture en cours
	void SetReadClass(const KWClass* kwcInputClass);
	const KWClass* GetReadClass() const;

	// Parametrage de la taille totale des buffers des slices utilises pour la lecture de la base (defaut: 0 pour
	// automatique)
	void SetTotalBufferSize(longint lValue);
	longint GetTotalBufferSize() const;

	// Ouverture de la base de donnees pour lecture
	boolean OpenForRead();

	// Test etat de la base
	boolean IsOpenedForRead() const;

	// Test de fin de base, ou d'interruption utilisateur
	boolean IsEnd() const;

	// Lecture d'une instance
	// Renvoie NULL en cas d'erreur ou d'interruption utilisateur
	KWObject* Read();

	// Saut d'un enregistrement
	void Skip();

	// Saut d'un ensemble d'enregistrements
	// Cette methode est optimisee en exploitant GetChunkInstanceNumbers, ce qui permet
	// de se placer si necessaire directement sur le bon chunk
	// On peut le faire des l'ouverture pour se positionner a une ligne donnes d'un chunk particulier
	void SkipMultiple(int nSkipNumber);

	// Test si erreur grave
	boolean IsError() const;

	// Estimation du pourcentage d'avancement de la lecture
	double GetReadPercentage();

	// Index du chunk en cours
	// Attention, dans le cas de fichiers vides, on ne passera pas par tous les index possible en cours de lecture
	int GetChunkIndex() const;

	// Fermeture de la base
	boolean Close();

	////////////////////////////////////////////////////////////////////
	// Lecture/ecriture globale pour gestion de tous les objets en memoire

	// Lecture d'objets a partir d'une classe et du contenu de toutes les tranches
	// Cette methode exploite les methode precedente de lecture sequentielle du sliceset
	// pour lire tous les objets et les ranger dans un tableau, a exploiter par l'appelant
	// Methode avec suivi de tache
	// En cas d'interruption, d'erreur ou de memoire insuffisante, on renvoie
	// false en emettant un warning et on detruits les objets ayant deja ete lus
	boolean ReadAllObjectsWithClass(const KWClass* kwcInputClass, ObjectArray* oaReadObjects);

	////////////////////////////////////////////////////////////////////////////////
	// Gestion avancee des tranches
	// On peut specifier directement des tranches, pour utiliser la lecture des objets
	// en se basant sur toute ou partie de tranches existant par ailleurs
	// Cela est notamment utile si l'on dispose d'un ensemble de tranche complet,
	// dont on veut exploiter des sous-parties

	// Construction d'une classe a partir d'un ensemble de noms d'attributs des tranches
	// La classe construite sera compatible avec les attributs des tranches, vis
	// a vis des noms, types, et blocs d'attributs, et son nom sera specifie dans l'appele
	// Memoire: la classe construite en retour appartient a l'appelant
	KWClass* BuildClassFromAttributeNames(const ALString& sInputClassName,
					      const StringVector* svInputAttributeNames);

	// Acces au tableau des tranches
	// Memoire: le tableau et son contenu appartiennent a l'appelant, mais peuvent etre modifies sous la
	// responsabilite de l'appele
	ObjectArray* GetSlices();

	// Destruction de tous les fichiers des tranches, et reinitialisation des vecteurs de noms et taille de fichier
	// par tranche ainsi que des nombres de valeur par attributs, et en fin des nombres d'instances par chunk
	void DeleteAllSliceFiles();

	////////////////////////////////////////////////////////////////////////////////
	// Fonctionnalites avancees sur la gestion des attributs

	// Alimentation d'un dictionnaire, dont les cle sont les noms des attributs utilises des tranches
	// et les objets sont les attributs (KWAttribute)
	// Le contenu du dictionnaire en sortie appartient a l'appele
	void FillAttributes(ObjectDictionary* odAttributes) const;

	// Alimentation d'un dictionnaire, dont les cle sont les noms des attributs utilises des tranches
	// et les objets sont les tranches (KWDataTableSlice) les contenant
	// Le contenu du dictionnaire en sortie appartient a l'appele
	void FillSliceAttributes(ObjectDictionary* odSliceAttributes) const;

	// Ecriture de tous les dictionnaires de tranche
	void SliceDictionariesWrite(ostream& ost) const;
	void SliceDictionariesWriteFile(const ALString& sFileName) const;

	////////////////////////////////////////////////////////////////////////////////
	// Fonctionnalites de base

	// Verification de l'integrite
	// Principalement, les tranches doivent constituer une partition des attributs simples utilises du dictionnaire
	// en entree. Les blocs doivent se trouver par plages contigues, dans des tranches dont l'ordre est le meme que
	// celui des plages d'attributs des blocs, ce qui permet de reconstituer des blocs presents dans plusieurs
	// tranches par simple concatenation Cette methode est tolerante s'il n'y a pas de dictionnaire en entree
	// specifie
	boolean Check() const override;

	// Copie et duplication
	void CopyFrom(const KWDataTableSliceSet* aSource);
	KWDataTableSliceSet* Clone() const;

	// Memoire utilisee par le driver pour son fonctionnement
	longint GetUsedMemory() const override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////////////////////////////
	// Methodes de test

	// Creation d'une table de test
	// Decoupage de la classe en entree en tranche et creation physique d'autant de fichiers que necessaire
	// selon le nombre d'objet demandees
	static KWDataTableSliceSet* CreateDataTableSliceSet(const KWClass* kwcClassToPartition,
							    const ALString& sTargetAttributeName, int nObjectNumber,
							    int nMaxAttributeNumberPerSlice, int nChunkNumber);

	// Test de lecture des objet d'une table avec un dictionnaire
	static void TestReadDataTableSliceSet(const ALString& sTestLabel, KWDataTableSliceSet* inputDataTableSliceSet,
					      const KWClass* kwcInputClass);

	// Test
	static void Test();

	/////////////////////////////////////////////////
	///// Implementation
protected:
	// Calcul des informations necessaires a la lecture de la base avec la methode void ReadAllObjectFromClass
	// En entree:
	//  . kwcInputClass: classe a lire, valide pour la lecture (passant le CheckReadClass)
	// En sortie:
	//  . physicalClass: classe physique, comportant au debut tous les attributs principaux de la classe a lire,
	//                   et en fin de classe, les attributs necessaires au calcul des attributs principaux
	//  . oaPhysicalSlices: tranches impliquees dans la lecture physique des bases, avec leur dictionnaire
	//              correctement parametre pour les attribut a lire (Loaded)
	// Les classes physiques (physicalClass et classes de tranches impliquees) sont valides et indexees, mais ne
	// sont pas dans un domaine de classe Memoire: la physicalClass en sortie appartient a l'appelant, le contenu du
	// tableau oaPhysicalSlices a l'appele
	void ComputeReadInformation(const KWClass* kwcInputClass, KWClass*& physicalClass,
				    ObjectArray* oaPhysicalSlices);

	// Recherche des attributs du sliceset utilises recursivement dans les operandes d'une regle de derivation
	// On arrete la recursion si l'on arrive sur un attribut du sliceset, meme si l'attribut etait calcule
	// On retourne une erreur si on arrive sur un attribut natif qui n'est pas dans le sliceset
	// Entrees:
	//  . rule: regle a analyser
	//  . odSliceAttributes: dictionnaire des attributs des slices, avec chaque attribut en cle associe a sa slice
	// Sorties:
	//  . nkdAllUsedSliceAttributes: attributs trouve dans les tranches
	//  . nkdAllUsedDerivedAttributes: attributs calcules intermediaires, absent des tranches
	//  . sErrorAttribute: attribut natif absent des tranche, provoquant une erreur
	boolean BuildAllUsedSliceAttributes(const KWDerivationRule* rule, const ObjectDictionary* odSliceAttributes,
					    NumericKeyDictionary* nkdAllUsedSliceAttributes,
					    NumericKeyDictionary* nkdAllUsedDerivedAttributes,
					    ALString& sErrorAttribute) const;

	// Lecture physique d'une instance pour aller chercher les valeur des attributs utilises presents dans les
	// tranches Renvoie NULL en cas d'erreur
	KWObject* PhysicalRead();

	// Calcul du nombre d'attributs utilises de type simple (hors attribut cible eventuel)
	int GetUsedSimpleAttributeNumber() const;

	// Attributs de specification du sliceSet
	ALString sClassName;
	ALString sClassTargetAttributeName;
	int nTotalInstanceNumber;
	ObjectArray oaSlices;
	IntVector ivChunkInstanceNumbers;
	boolean bDeleteFilesAtClean;
	friend class PLShared_DataTableSliceSet;

	// Variables pour la gestion de la lecture au moyen d'une classe s'appuyant sur les attributs du sliceSet
	// Toutes ces variables, prefixees par _read, ne sont actives que le temps de l'utilisation des
	// methodes de lecture, entre l'ouverture et la fermeture
	longint lTotalBufferSize;
	const KWClass* read_Class;
	KWClass* read_PhysicalClass;
	ObjectArray read_oaPhysicalSlices;
	KWDataTableSlice* read_FirstPhysicalSlice;
	KWClassDomain* read_CurrentDomain;
	KWClassDomain read_PhysicalDomain;
	KWLoadIndexVector read_livDerivedDataItems;
};

///////////////////////////////////////////////////////////////////////////////////
// Tranche d'une table de donnees textuelle, definie par son dictionnaire contenant
// l'ensemble de ses variables et l'ensemble des fichier textuelle stockant ses
// instances
class KWDataTableSlice : public Object
{
public:
	/// Constructeur
	KWDataTableSlice();
	~KWDataTableSlice();

	// Index de la tranche, sur la base d'un vecteur d'entier traite selon un ordre lexicographique
	// L'ordre des tranches dans un sliceset est important, et cet index permet de le controler et le maintenir
	// y compris dans la cas ou une tranche doit etre decoupee en sous-tranches
	IntVector* GetLexicographicIndex();

	// Methode de comparaison de deux tranches selon leur index lexicographique
	int CompareLexicographicOrder(KWDataTableSlice* otherSlice) const;

	// Classe associee pour les lectures/ecritures
	// Son nom est unique au sein d'un sliceset et peut donc servir d'identifiant de la tranche
	// Il est a noter qu'en lecture/ecriture, seuls les attributs  de type Loaded seront geres.
	// La classe correspondante doit etre valide, ne contenir aucune formule de calcul,
	// uniquement des variables numeriques ou categorielles, eventuellement organisees par bloc sparse.
	// Elle n'est pas dans un domaine de classe. Elle devra etre insere dans un domaine et
	// compilee uniquement le temps de son utilisation
	// Memoire: la classe appartient a l'appele
	KWClass* GetClass();

	////////////////////////////////////////////////////////////////////////
	// Informations sur les fichiers de la tranche et leur contenu
	// Ces informations sur le contenu exact des tranches sont collectees
	// lors la creation effective des tranche par la tache KWDatabaseSlicerTask

	// Acces aux noms des fichiers de donnees gerant les instances de la base
	// Ces fichier sont sans ligne d'entete, avec separateur tabulation
	StringVector* GetDataFileNames();

	// Taille par fichier
	LongintVector* GetDataFileSizes();

	// Taille occupee sur disque par chaque attribut dense de type Symbol
	// L'index du vecteur est celui des attributs denses dans la classe, utilises ou non
	// Le vecteur contient une taille sur fichier pour les attribut Symbol, et 0 pour oles attributs numeriques
	// Attention: tous les attributs utilises ne sont pas necessaire charge en memoire (Loaded)
	LongintVector* GetDenseSymbolAttributeDiskSizes();

	// Nombre total de valeurs presentes par bloc d'attribut de la classe, sur l'ensemble des fichiers
	// L'index dans le vecteur est celui des blocs d'attributs dans la classe, utilises ou non
	// Attention: tous les blocs utilises ne sont pas necessaire charge en memoire (Loaded)
	LongintVector* GetAttributeBlockValueNumbers();

	// Destruction de tous les fichiers de la tranche, et reinitialisation des vecteurs de noms et taille de fichier
	// et des nombre de valeurs par bloc
	void DeleteSliceFiles();

	////////////////////////////////////////////////////////////////////////////////
	// Calcul de statistiques sur le contenu de la tranche

	// Taille totale cumulee sur l'ensemble des fichiers
	longint GetTotalDataFileSize() const;

	// Taille totale cumulee sur disque pour l'ensemble des attributs denses Symbol charges en memoire
	longint GetTotalDenseSymbolAttributeDiskSize() const;

	// Nombre d'attributs du bloc de plus grande taille, pour les blocs charges en memoire
	int GetMaxBlockAttributeNumber() const;

	// Nombre max de valeurs du plus gros bloc charge en memoire
	longint GetMaxAttributeBlockValueNumber() const;

	// Totale cumule des nombres de valeur par blocs d'attribut sur l'ensemble des blocs charges en memoire
	longint GetTotalAttributeBlockValueNumber() const;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Methodes pour la lecture sequentielle des objets de la tranche, similaires a celle de KWDatabase

	// Ouverture de la base de donnees pour lecture
	boolean OpenForRead();

	// Test etat de la base
	boolean IsOpenedForRead() const;

	// Test de fin de base, ou d'interruption utilisateur
	boolean IsEnd() const;

	// Lecture d'une instance
	// Renvoie NULL en cas d'erreur
	KWObject* Read();

	// Saut d'un enregistrement
	void Skip();

	// Test si erreur grave
	boolean IsError() const;

	// Estimation du pourcentage d'avancement de la lecture
	double GetReadPercentage();

	// Index du chunk en cours
	// Attention, dans le cas de fichiers vides, on ne passera pas par tous les index possible en cours de lecture
	int GetChunkIndex() const;

	// Fermeture de la base
	boolean Close();

	////////////////////////////////////////////////////////////////////
	// Lecture/ecriture globale pour gestion de tous les objets en memoire

	// Lecture de tous les objets de tous les fichiers
	// Le resultat de la lecture est accessible par la methode GetObjects
	// La classe associee a la tranche doit etre compilee
	// Methode avec suivi de tache
	// En cas d'interruption, d'erreur ou de memoire insuffisante, on renvoie
	// false en emettant un warning et on detruits les objets ayant deja ete lus
	boolean ReadAll();

	// Acces aux objets (KWObject)
	// Ce tableau a pour role principal d'accueillir le resultat d'un ReadAll ou ReadOneFile
	// Il peut aussi etre modifie explicitement.
	// Memoire: le tableau et son contenu sont gere par l'appele
	ObjectArray* GetObjects();

	// Supression/destruction de tous les objets charges en memoire
	void RemoveAll();
	void DeleteAll();

	////////////////////////////////////////////////////////////////////////////////
	// Gestion des noms de fichier d'une tranche, sans les paths

	// Construction d'un prefixe base sur un index lexicographic pour la tranche et un index pour le chunk
	// Si l'index de chunk vaut -1, seul la partie tranche du prefix est prise en compte
	static const ALString BuildPrefix(const IntVector* ivSliceLexicographicIndex, int nChunkIndex);

	// Construction d'un nom de fichier de tranche a partir d'un nom de fichier, sur la base
	// d'un prefixe exploitant la paire d'index (slice, chunk)
	static const ALString BuildSliceFileName(int nSliceIndex, int nChunkIndex, const ALString& sFilePathName);
	static const ALString BuildSubSliceFileName(const IntVector* ivSliceLexicographicIndex, int nChunkIndex,
						    const ALString& sFilePathName);

	// Recherche de la partie prefix et de la partie nom de fichier de tranche a partir de son chemin
	static const ALString GetSliceFilePrefix(const ALString& sSliceFilePathName);
	static const ALString GetSliceFileBaseName(const ALString& sSliceFilePathName);

	// Caracteres utilises pour prefixer les tranches et les chunk, et en fin de prefix
	static char GetSlicePrefix();
	static char GetSubSlicePrefix();
	static char GetChunkPrefix();
	static char GetPrefixEnd();

	////////////////////////////////////////////////////////////////////////////////
	// Variable de travail utilisable depuis d'autres classes pour personnaliser des
	// criteres de tri lexicographique

	// Vecteur de critere pour un tri lexicographique utilisateur
	// La gestion de la taille et des valeur de ce vecteur est entierement a la charge de l'appelant
	// Attention, ce tri permet de prioriser les tranche seon un criter utilisateurn quelconque.
	// Le sliceset complet doit lui toujours etretrie selon son ordre par defaut (selon GetLexicographicIndex)
	// pour etre utilisable
	DoubleVector* GetLexicographicSortCriterion();

	// Methode de comparaison de deux tranches selon leur critere lexicographique
	int CompareLexicographicSortCriterion(KWDataTableSlice* otherSlice) const;

	// Affichage des valeurs de tri d'une tranche sous forme d'une ligne
	void DisplayLexicographicSortCriterionHeaderLineReport(ostream& ost, const ALString& sSortCriterion) const;
	void DisplayLexicographicSortCriterionLineReport(ostream& ost) const;

	////////////////////////////////////////////////////////////////////////////////
	// Fonctionnalites de base

	// Verification de l'integrite
	boolean Check() const override;

	// Copie et duplication
	void CopyFrom(const KWDataTableSlice* aSource);
	KWDataTableSlice* Clone() const;

	// Memoire utilisee
	// On prend en compte la memoire utilisee par la classe, mais pas celle des KWObject
	longint GetUsedMemory() const override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	friend class KWDataTableSliceSet;
	friend class KWDatabaseSlicerOutputBufferedFile;
	friend class PLShared_DataTableSlice;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Parametrage des index de chargement des data items de la tranche dans la classe a partitionner
	// Methode avancee utilisees par la classe KWDatabaseSlicerTask pour piloter le decoupage
	// des objets de la classe principale du slice set en sous objets par classe de la slice courante
	friend class KWDatabaseSlicerTask;

	// Index de chargement dans la classe du slice set des data item de la slice courante
	KWLoadIndexVector* GetDataItemLoadIndexes();

	// Index du premier et du dernier attribut sparse par data item de type bloc sparse
	// En effet, les bloc de la classes du slice set sont potentiellement decoupes en sous-blocs
	// geres par chaque slice, et ces index de debut et de fin delimite la sous-partie reservee
	// au bloc de la slice courante
	IntVector* GetValueBlockFirstSparseIndexes();
	IntVector* GetValueBlockLastSparseIndexes();

	// Ouverture physique de la base de donnees pour lecture en passant en parametre une classe de driver
	// potentiellement differente de la classe de la tranche
	// Cela permet de cree des objets avec une classe plus grande, et de demander a chaque tranche d'alimenter
	// la partie des attributs qui sont dans la tranche
	// Le parametre bOpenOnDemandMode peut etre mis a true dans le cas ou de nombeuses tranches doivent etre
	// ouvertes simultanement Le parametre nBufferSize est pris en compte s'il est different de 0 pour avoir  une
	// taille specifique si necessaire
	boolean PhysicalOpenForRead(KWClass* driverClass, boolean bOpenOnDemandMode, int nBufferSize);

	// Lecture d'un objet dans son chunk courant
	// L'objet peut etre soit a creer, soit a completer avec les attributs provenant de la tranche
	// En cas d'erreur, l'objet cree ou complete reste disponible: sa destruction eventuelle est a la charge de
	// l'appeleant
	boolean PhysicalReadObject(KWObject*& kwoObject, boolean bCreate);

	// Ouverture du chunk courant en lecture
	boolean PhysicalOpenCurrentChunk();

	// Fermeture du chunk courant en lecture
	boolean PhysicalCloseCurrentChunk();

	// Saut des chunks vides
	void PhysicalSkipEmptyChunks();

	// Index lexicographique
	IntVector ivLexicographicIndex;

	// Dictionnaire de la tranche
	KWClass kwcClass;

	// Fichier de la tranche, et leur taille
	StringVector svDataFileNames; // URI
	LongintVector lvDataFileSizes;

	// Nombre effectif total de valeurs presentes par bloc de la tranche
	LongintVector lvAttributeBlockValueNumbers;

	// Taille occupee sur disque par chaque attribut Symbol dense dans les fichiers de la tranche
	LongintVector lvDenseSymbolAttributeDiskSizes;

	// Critere de tri lexicographique
	DoubleVector dvLexicographicSortCriterion;

	// Liste des objet lus de la tranche
	ObjectArray oaObjects;

	// Information d'index pour la tache slicer
	KWLoadIndexVector livDataItemLoadIndexes;
	IntVector ivValueBlockFirstSparseIndexes;
	IntVector ivValueBlockLastSparseIndexes;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Variables pour la gestion de la lecture au moyen de la classe associee a la tranche
	// Toutes ces variables, prefixees par _read, ne sont actives que le temps de l'utilisation des
	// methodes de lecture, entre l'ouverture et la fermeture

	// Driver associe a la tranche
	KWDataTableDriverSlice* read_SliceDataTableDriver;

	// Index du fichier courant en cours de traitement
	int read_nDataFileIndex;
};

// Methode de comparaison basee sur l'index lexicographique
int KWDataTableSliceCompareLexicographicIndex(const void* elem1, const void* elem2);

// Methode de comparaison basee sur le critere de tri lexicographique
int KWDataTableSliceCompareLexicographicSortCriterion(const void* elem1, const void* elem2);

///////////////////////////////////////////////////
// Classe KWDataTableDriverSlice
// Specialisation de KWDataTableDriverTextFile pour la gestion efficace des tranches d'une table
class KWDataTableDriverSlice : public KWDataTableDriverTextFile
{
	// Classe interne uniquemment pour la gestion des tranches
	friend class KWDataTableSliceSet;
	friend class KWDataTableSlice;

protected:
	// Constructeur
	KWDataTableDriverSlice();
	~KWDataTableDriverSlice();

	////////////////////////////////////////////////////////////////////
	// Methodes dediees a la lecture des tranches d'une base
	// On peut lire la tranche pour remplir tout ou partie des champs
	// des objets de la classe associee au driver
	//
	// Il faut d'abord calculer les informations d'ouverture de la tranche

	// Calcul des informations d'ouverture, parametree par la classe associee a la tranche
	// qui decrit la structure des fichiers de la tranche et choisit les champs a lire
	// Le calcul des liens entre les champs de la tranche et les champs de la classe
	// associee au driver se fera une fois pour toute
	void ComputeOpenInformation(const KWClass* kwcSliceClass);
	boolean IsOpenInformationComputed() const;

	// Fermeture global de la tranche
	void CleanOpenInformation();

	//////////////////////////////////////////////////////////////////////
	// Gestion d'un chunk d'une tranche ouverte en lecture
	// Une fois les information d'ouverture calculees, on peut lire
	// un ou plusieurs chunks, en ayant correctement specifie le DataTableName

	// Ouverture a la demande (defaut: false)
	void SetOpenOnDemandMode(boolean bValue);
	boolean GetOpenOnDemandMode() const;

	// Ouverture d'un chunk de la tranche
	// URI: un fichier de chunk est potentiellement distant
	// La taille en entree est utilisee pour verifier que la taille du chunk lu correspond a celle enregistree
	boolean OpenChunkForRead(const ALString& sDataFileName, longint lDataFileSize);
	boolean IsOpenedForRead() const override;

	// Fermeture d'un chunk de la tranche
	boolean Close() override;
	boolean IsClosed() const override;

	//////////////////////////////////////////////////////////////////////
	// Gestion des objets d'un chunk de tranche

	// Methode avancee pour lire les objets d'un chunk d'une tranche
	// Alimentation des champs de l'objet (de la classe du driver) qui sont presents dans la tranche
	// Un chunk de tranche, ecrit par l'outil, a necessairement un format correct: tout probleme de
	// lecture devient alors une error (et non un warning, comme dans certains cas pour le
	// KWDataTableDriverTextFile)
	boolean ReadObject(KWObject* kwoObject);

	/////////////////////////////////////////////////////////////////
	// Methode interne
private:
	// Calcul des index de chargement des attribut et blocs de la tranche en fonction de
	// classe de la tranche, qui peut avoir tout ou partie de ses attributs en Unused
	void ComputeSliceDataItemLoadIndexes(const KWClass* kwcSliceClass);

	// Methodes redefinies, dont l'utilisation est interdite
	boolean BuildDataTableClass(KWClass* kwcDataTableClass) override;
	boolean OpenForRead(const KWClass* kwcLogicalClass) override;
	boolean OpenForWrite() override;
	boolean IsOpenedForWrite() const override;
	KWObject* Read() override;
	void Write(const KWObject* kwoObject) override;

	// Mode d'ouverture a la demande
	boolean bOpenOnDemandMode;
};

///////////////////////////////////////////////////
// Classe PLShared_DataTableSliceSet
// Serialisation de la classe KWDataTableSliceSet
class PLShared_DataTableSliceSet : public PLSharedObject
{
public:
	// Constructeur
	PLShared_DataTableSliceSet();
	~PLShared_DataTableSliceSet();

	// Acces a l'objet serialise
	void SetDataTableSliceSet(KWDataTableSliceSet* dataTableSliceSet);
	KWDataTableSliceSet* GetDataTableSliceSet();

	// Reimplementation des methodes virtuelles, avec transfer des specifications des bases
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

///////////////////////////////////////////////////
// Classe PLShared_DataTableSlice
// Serialisation de la classe KWDataTableSlice
class PLShared_DataTableSlice : public PLSharedObject
{
public:
	// Constructeur
	PLShared_DataTableSlice();
	~PLShared_DataTableSlice();

	// Acces a l'objet serialise
	void SetDataTableSlice(KWDataTableSlice* dataTableSlice);
	KWDataTableSlice* GetDataTableSlice();

	// Reimplementation des methodes virtuelles, avec transfer des specifications de la base
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

/////////////////////////////////////////
// Methodes en inline

inline LongintVector* KWDataTableSlice::GetDenseSymbolAttributeDiskSizes()
{
	return &lvDenseSymbolAttributeDiskSizes;
}

inline LongintVector* KWDataTableSlice::GetAttributeBlockValueNumbers()
{
	return &lvAttributeBlockValueNumbers;
}

inline KWLoadIndexVector* KWDataTableSlice::GetDataItemLoadIndexes()
{
	return &livDataItemLoadIndexes;
}

inline IntVector* KWDataTableSlice::GetValueBlockFirstSparseIndexes()
{
	return &ivValueBlockFirstSparseIndexes;
}

inline IntVector* KWDataTableSlice::GetValueBlockLastSparseIndexes()
{
	return &ivValueBlockLastSparseIndexes;
}
