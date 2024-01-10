// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class SNBDataTableBinarySliceSetLayout;
class SNBDataTableBinarySliceSetAttribute;
class PLShared_DataTableBinarySliceSetAttribute;
class SNBDataTableBinarySliceSetSchema;
class SNBDataTableBinarySliceSetRandomizedAttributeIterator;
class SNBDataTableBinarySliceSetBuffer;
class SNBDataTableBinarySliceSet;

#include "KWDataPreparationClass.h"
#include "HugeBuffer.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Layout logique du SNBDataTableBinarySliceSet
// Repond aux queries sur les dimensions, offsets du tableau et de slices et chunks
class SNBDataTableBinarySliceSetLayout : public Object
{
public:
	// Constructeur
	SNBDataTableBinarySliceSetLayout();
	~SNBDataTableBinarySliceSetLayout();

	//////////////////////////////////////////
	// Initialisation et Nettoyage

	// Initialisation/reinitialisation avec des dimensionements
	void Initialize(int nSomeInstanceNumber, int nSomeChunkNumber, int nSomeAttributeNumber, int nSomeSliceNumber);

	// True si initialise
	boolean IsInitialized() const;

	// Nettoyage des variables de travail
	void CleanWorkingData();

	/////////////////////////////////////////////////////////////////////
	// Acces aux dimensions; disponibles apres initialisation

	// Nombre d'instances
	int GetInstanceNumber() const;

	// Nombre de chunks (groupes d'instances)
	int GetChunkNumber() const;

	// Nombre maximal d'instances parmi les chunks;
	int GetMaxChunkInstanceNumber() const;

	// Nombre d'instances d'un chunk
	int GetInstanceNumberAtChunk(int nChunk) const;

	// Offset d'un chunk
	int GetInstanceOffsetAtChunk(int nChunk) const;

	// Nombre d'attributs
	int GetAttributeNumber() const;

	// Nombre de slices (groupes d'attributs)
	int GetSliceNumber() const;

	// Nombre maximal d'attributs parmi les slices
	int GetMaxSliceAttributeNumber() const;

	// Nombre d'attributs d'une slice
	int GetAttributeNumberAtSlice(int nSlice) const;

	// Offset de la slice dans la liste d'attributs
	int GetAttributeOffsetAtSlice(int nSlice) const;

	// Index de la slice d'un attribute
	int GetSliceIndexAtAttribute(int nAttribute) const;

	// Index d'un attribut dans sa slice
	int GetRelativeIndexAtAttribute(int nAttribute) const;

	// Nombre de `int`s contenus dans un bloc
	longint GetBlockSizeAt(int nChunk, int nSlice) const;

	// Offset en bytes d'un bloc dans son fichier de chunk
	longint GetBlockOffsetAt(int nChunk, int nSlice) const;

	//////////////////////////////
	// Services divers

	// Verification d'integrite
	boolean Check() const override;

	// Label de classe
	const ALString GetClassLabel() const override;

	// Ecriture vers un stream
	void Write(ostream& ost) const override;

	// Estimation de l'empreinte memoire a partir des dimensions
	static longint ComputeNecessaryMemory(int nInstanceNumber, int nChunkNumber, int nAttributeNumber,
					      int nSliceNumber);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Nombre d'instances
	int nInstanceNumber;

	// Nombre d'attributs
	int nAttributeNumber;

	// Nombre de chunks
	int nChunkNumber;

	// Nombres d'instances par chunk
	IntVector ivChunkInstanceNumbers;

	// Offsets de chunks
	IntVector ivChunkInstanceOffsets;

	// Nombre de slices
	int nSliceNumber;

	// Nombres d'attributs par slices
	IntVector ivSliceAttributeNumbers;

	// Offsets des slices
	IntVector ivSliceAttributeOffsets;

	// Indexes des slices de chaque attribut
	IntVector ivAttributeSliceIndexes;

	// Indexes des attributs relatifs a ses slices
	IntVector ivAttributeRelativeIndexes;

	// Tableau de LongintVectors's contenant les tailles des des blocs
	ObjectArray oaBlockSizes;

	// Tableau de LongintVector's contenant les offsets des blocs
	ObjectArray oaBlockOffsets;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Attribut d'un SNBDataTableBinarySliceSet
// Cette classe est (majoritairement) une facade d'un KWDataPreparationStats d'un attribut prepare
// avec une interface qui donne access aux informations necessaires pour l'apprentissage du modele
class SNBDataTableBinarySliceSetAttribute : public Object
{
public:
	// Constructeur
	SNBDataTableBinarySliceSetAttribute();
	~SNBDataTableBinarySliceSetAttribute();

	/////////////////////////////
	// Initialisation

	// Nom de l'attribut natif
	void SetNativeAttributeName(const ALString& sValue);
	const ALString& GetNativeAttributeName() const;

	// Nom de l'attribut prepare
	void SetPreparedAttributeName(const ALString& sValue);
	const ALString& GetPreparedAttributeName() const;

	// Nom de l'attribut recode
	void SetRecodedAttributeName(const ALString& sValue);
	const ALString& GetRecodedAttributeName() const;

	// Index de l'attribut dans le SNBDataTableBinarySliceSet qui le contient
	void SetIndex(int nValue);
	int GetIndex() const;

	// Index de l'attribut dans la DataPreparationClass
	void SetDataPreparationClassIndex(int nValue);
	int GetDataPreparationClassIndex() const;

	// Creation de la table de probabilites a partir d'un attribut prepare
	void InitializeFromDataPreparationAttribute(KWDataPreparationAttribute* dataPreparationAttribute);

	////////////////////////////////////////////////////////////////
	// Informations du KWDataPreparationAttribute source

	// Cout de construction
	double GetConstructionCost() const;

	// Cout du modele nul de l'attribut
	double GetNullConstructionCost() const;

	// Cout de preparation
	double GetPreparationCost() const;

	// Level (informativite)
	double GetLevel() const;

	// Calcul de Ln(P( X = x | Y = y )); modalites specifiees par des indexes de parties
	Continuous GetLnSourceConditionalProb(int nSourceModality, int nTargetModality) const;

	// DataGridStats issu de la preparation de donnees
	const KWDataGridStats* GetPreparedDataGridStats() const;

	// Partition de la cible de l'attribut prepare
	const KWDGSAttributePartition* GetTargetPartition() const;

	/////////////////////////////
	// Service Divers

	// Test d'integrite
	boolean Check() const override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Empreinte memoire
	longint GetUsedMemory() const override;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Nom de l'attribut
	ALString sNativeAttributeName;

	// Nom de l'attribut prepare
	ALString sPreparedAttributeName;

	// Nom de l'attribut recode associe
	ALString sRecodedAttributeName;

	// Indice dans la learning database qui le contient
	int nIndex;

	// Indice dans l'ordre de la classe de preparation original
	int nDataPreparationClassIndex;

	// Statistiques de preparation de l'attribut
	KWDataPreparationStats* dataPreparationStats;

	// Table de probabilites conditionelles
	KWProbabilityTable conditionalProbas;

	// Acces privee au conteneur de serialisatio de cette classe et a la tache d'apprentissage
	friend class PLShared_DataTableBinarySliceSetAttribute;
	friend class SNBPredictorSNBTrainingTask;
};

// Comparaison par le nom natif de l'attribut
int SNBDataTableBinarySliceSetAttributeCompareNativeAttributeName(const void* elem1, const void* elem2);

// Comparaison par l'index dans KWDataPreparationClass de l'attribut source
int SNBDataTableBinarySliceSetAttributeCompareDataPreparationClassIndex(const void* elem1, const void* elem2);

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Conteneur de serialisation d'un KWDataTableSliceSetAttribute
// Nota : L'objet KWDataPreparationStats encapsule n'est pas serialise
class PLShared_DataTableBinarySliceSetAttribute : public PLSharedObject
{
public:
	// Constructeur
	PLShared_DataTableBinarySliceSetAttribute();
	~PLShared_DataTableBinarySliceSetAttribute();

	// Parametrage de l'objet serialise
	void SetDataTableBinarySliceSetAttribute(SNBDataTableBinarySliceSetAttribute* attribute);
	SNBDataTableBinarySliceSetAttribute* GetDataTableBinarySliceSetAttribute() const;

protected:
	// Reimplementation des methodes de PLSharedObject
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;
	Object* Create() const override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Schema d'attributs de la SNBDataTableBinarySliceSet
// Donne acces aux attributs par index ou noms
class SNBDataTableBinarySliceSetSchema : public Object
{
public:
	// Constructeur
	SNBDataTableBinarySliceSetSchema();
	~SNBDataTableBinarySliceSetSchema();

	///////////////////////////////////////////////
	// Initialisation et Nettoyage

	// Initialisation
	void InitializeFromDataPreparationClass(KWDataPreparationClass* dataPreparationClass,
						ObjectArray* oaUsedDataPreparationAttributes,
						StringVector* svUsedDataPreparationAttributesNames);

	// Initialisation directe a partir un tableau de SNBDataTableBinarySliceSetAttribute's
	void InitializeFromAttributes(const ObjectArray* oaDataTableBinarySliceSetAttributes);

	// True si l'objet est initialise
	boolean IsInitialized() const;

	// Nettoyage des variables de travail
	void CleanWorkingData();

	//////////////////////////////////
	// Acces aux attributs

	// Nombre d'attributs
	int GetAttributeNumber() const;

	// Acces a un attribut par son index
	SNBDataTableBinarySliceSetAttribute* GetAttributeAt(int nAttribute) const;

	// Acces a un attribute par son nom
	SNBDataTableBinarySliceSetAttribute* GetAttributeAtNativeName(const ALString& sName) const;

	// Acces a un attribut par son nom recode
	SNBDataTableBinarySliceSetAttribute* GetAttributeAtRecodedAttribute(const KWAttribute* recodedAttribute) const;

	//////////////////////////////
	// Services Divers

	// True si l'objet est dans un etat valide
	boolean Check() const override;

	// Ecriture vers un stream
	void Write(ostream& ost) const override;

	// Empreinte memoire
	longint GetUsedMemory() const override;

	// Estimation heuristique de l'empreinte memoire
	// Ne tiens pas en compte les KWDataPreparationAttributes facades par les attributs
	static longint ComputeNecessaryMemory(KWClassStats* classStats);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Tableau de SNBDataTableBinarySliceSetAttribute's
	ObjectArray oaAttributes;

	// Relation [nom -> attribut]
	ObjectDictionary odAttributesByNativeAttributeName;

	// Relation [nom attribut recode -> attribut]
	ObjectDictionary odAttributesByRecodedAttributeName;

	// Acces prive a la tache parallele d'apprentissage pour reconstituer serialisation de l'objet
	friend class SNBPredictorSNBTrainingTask;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Iterateur aleatoire d'un SNBDataTableBinarySliceSetSchema
class SNBDataTableBinarySliceSetRandomizedAttributeIterator : public Object
{
public:
	// Constructeur
	SNBDataTableBinarySliceSetRandomizedAttributeIterator();
	~SNBDataTableBinarySliceSetRandomizedAttributeIterator();

	// Initialisation
	void Initialize(const SNBDataTableBinarySliceSetSchema* schema, const SNBDataTableBinarySliceSetLayout* layout);

	// True si l'objet est initialise
	boolean IsInitialized() const;

	// Nettoyage des variables travail
	void CleanWorkingData();

	// Obtention d'attribut par index dans un ordre aleatoire
	SNBDataTableBinarySliceSetAttribute* GetRandomAttributeAt(int nRandomAttribute) const;

	// Reordonnancement de facon aleatoire les attributs de la table
	void Shuffle();

	// Restoration de l'ordre initial des attributs
	void Restore();

	// Verification d'integrite
	boolean Check() const override;

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nAttributeNumber, int nSliceNumber);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
private:
	// Ensemble d'attributs de la database
	const SNBDataTableBinarySliceSetSchema* schema;

	// Layout de la database
	const SNBDataTableBinarySliceSetLayout* layout;

	// Tableaux des attributs pour acces aux attributs en ordre aleatoire
	ObjectArray oaRandomizedAttributes;

	// Tableau de tableaux des attributs pour chaque slice dans l'ordre initial
	ObjectArray oaInitialSliceAttributes;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Buffer de lecture de une SNBDataTableBinarySliceSet
// Son initialisation entraine l'ecriture des fichiers de donnees binaires a partir
// d'un KWDataTableSliceSet et une classe de recodage
// Apres initialisation les fichiers sont Read-Only jusqu'a une nouvelle initialisation
class SNBDataTableBinarySliceSetBuffer : public Object
{
public:
	// Constructeur
	SNBDataTableBinarySliceSetBuffer();
	~SNBDataTableBinarySliceSetBuffer();

	/////////////////////////////
	// Initialisation

	// Parametrage du layout de la learning database
	// SetLayout defait toute initialisation
	void SetLayout(const SNBDataTableBinarySliceSetLayout* someLayout);
	const SNBDataTableBinarySliceSetLayout* GetLayout() const;

	// Initialisation
	boolean Initialize(KWClass* recoderClass, KWDataTableSliceSet* sliceSet,
			   const SNBDataTableBinarySliceSetSchema* schema);

	// Initialisation pour un seul chunk
	boolean InitializeOnlyAtChunk(int nChunk, KWClass* recoderClass, KWDataTableSliceSet* sliceSet,
				      SNBDataTableBinarySliceSetSchema* schema);

	// True si tous les chunks sont initialises
	boolean IsInitialized() const;

	// True si le chunk specifie est initialise
	boolean IsInitializedAtChunk(int nChunk) const;

	// True si le buffer est initialize pour un seul chunk
	boolean IsInitializedOnlyAtChunk(int nChunk) const;

	// Nettoyage : libere la memoire et efface les fichiers de donnees
	void CleanWorkingData();

	////////////////////////////////
	// Acces aux donnees

	// Recollection des indices d'un chunk pour un attribut
	boolean CollectRecodedAttributeIndexes(int nChunk, int nAttribute, IntVector*& ivOutput);

	//////////////////////////////
	// Services divers

	// Verification d'integrite
	boolean Check() const override;

	// Label de classe
	const ALString GetClassLabel() const override;

	// Ecriture des contenus
	void WriteContentsAsTSV(ostream& ost, const SNBDataTableBinarySliceSetSchema* schema);

	// Estimation de l'empreinte memoire
	static longint ComputeNecessaryMemory(int nInstanceNumber, int nChunkNumber, int nAttributeNumber,
					      int nSliceNumber);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	/////////////////////////////
	// Initialisation

	// Initialisation du buffer de lecture
	boolean InitializeBuffer();

	// Initialization du fichier de donnees d'un chunk a partir d'un slice set et une classe de recodage
	boolean InitializeDataFileAtChunk(int nChunk, KWClass* recoderClass, KWDataTableSliceSet* sliceSet,
					  const SNBDataTableBinarySliceSetSchema* schema);

	// Charge un bloc en memoire depuis un KWDataTableSliceSet
	boolean LoadBlockFromSliceSetAt(int nChunk, int nSlice, KWClass* recoderClass, KWDataTableSliceSet* sliceSet,
					const SNBDataTableBinarySliceSetSchema* schema);

	// Charge dans la classe de recodage (recoderClass) les attributs de la slice indiquee
	// Les index de chargement sont stockes dans le vecteur livLoadedRecodedAttributeIndexes
	void LoadRecodedAttributesAtSlice(int nSlice, KWClass* recoderClass,
					  const SNBDataTableBinarySliceSetSchema* schema,
					  KWLoadIndexVector* livLoadedRecodedAttributeIndexes) const;

	// Creation du fichier temporaire de chunk; retourne le chemin du fichier cree
	const ALString CreateTempFileForChunk(int nChunk) const;

	// Mini API d'ecriture de fichier de donnees utilisee dans l'initialisation
	boolean OpenOutputDataFileAtChunk(int nChunk);
	boolean WriteToDataFile(int* writeBuffer, int nIntNumber) const;
	boolean CloseOutputDataFile();

	///////////////////////////////////////////////////////////
	// Chargement de fichiers de donnees en memoire

	// Chargement d'un bloc : si le bloc est deja en memoire on ne fait rien
	boolean LoadBlockAt(int nChunk, int nSlice);

	// Acces aux chemins des fichiers de stockage de la database
	ALString GetDataFilePathAtChunk(int nChunk) const;

	// Verifie la taille des fichiers en fonction du layout
	boolean CheckDataFileAtChunk(int nChunk) const;

	/////////////////////////
	// Parametres

	// Layout du SNBDataTableBinarySliceSet
	const SNBDataTableBinarySliceSetLayout* layout;

	///////////////////////////////////
	// Objets de travail

	// Chemins des fichiers pour chaque chunk initialisee
	StringVector svChunkFilePaths;

	// Tableau contenant les donnees en memoire
	ObjectArray oaLoadedBlock;

	// True si il n'y que un seul chunk initialise
	boolean bIsSingleChunkBuffer;

	// Indice du seul chunk initialize si bIsSingleChunkBuffer
	int nSingleInitializedChunkIndex;

	// Indice du chunk charge en memoire
	int nLoadedBlockChunkIndex;

	// Indice de la slice charge en memoire
	int nLoadedBlockSliceIndex;

	// File pointer pour la lecture/ecriture de fichiers
	FILE* fChunkDataFile;

	// TODO FOR FELIPE: a supprimer
	//// Buffer pour la lecture/ecriture de fichiers
	// int* buffer;

	// Indice du fichier ouvert pendant l'initialisation
	int nOpenFileChunkIndex;

	// TODO FOR FELIPE: a supprimer
	//// Taille du buffer en nombre de int's
	// static const int nIntBufferSize = MemSegmentByteSize / sizeof(int);

	// La tache parallele d'apprentissage est friend pour faire des initialisations partielles
	friend class SNBPredictorSNBTrainingTask;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Recodage d'un KWDataTableSliceSet vers un stockage de fichiers donnees binaires
// Il est construit a partir des attributs *predictifs* issus de la preparation de donnees
// Toutes les fonctionalite ont ete deleguees a des classes sous-traitants, celle-ci etant une
// classe composee
class SNBDataTableBinarySliceSet : public Object
{
public:
	// Constructeur
	SNBDataTableBinarySliceSet();
	~SNBDataTableBinarySliceSet();

	/////////////////////////////
	// Initialisation

	// Initialisation complete
	void Initialize(KWClassStats* classStats, int nChunkNumber, int nMaxAttributes, int nSliceNumber);

	// Initialisation du schema d'attributs a partir d'un KWClassStats
	void InitializeSchemaFromClassStats(KWClassStats* classStats, int nMaxAttributes);

	// True si le schema est specifie et tous les chunks initialises pour lire
	boolean IsInitialized() const;

	// True si le schema est specifie et il y au moins un chunk initialise pour lire
	boolean IsPartiallyInitialized() const;

	// True s'il y a des attributs utiles
	boolean HasUsableAttributes() const;

	// Nettoyage des variables de travail
	void CleanWorkingData(boolean bOnlyRemoveDataPreparation);

	//////////////////////////////////
	// Acces aux attributs

	// Nombre d'attributs
	int GetAttributeNumber() const;

	// Nombre d'attributs initiales (inclu les non predictifs)
	int GetInitialAttributeNumber() const;

	// Acces aux attributs par index
	SNBDataTableBinarySliceSetAttribute* GetAttributeAt(int nAttribute) const;

	// Acces aux attributs par nom
	SNBDataTableBinarySliceSetAttribute* GetAttributeAtNativeName(const ALString& sAttributeName) const;

	// Acces aux attributs par le nom d'attribut recode
	// Renvoie NULL si non trouve
	SNBDataTableBinarySliceSetAttribute* GetAttributeAtRecodedAttribute(KWAttribute* recodingAttribute) const;

	// Acces aux DataPreparationAttributs utilises
	KWDataPreparationAttribute* GetDataPreparationAttributeAt(int nAttribute);

	// Acces au noms des attributs
	const ALString GetAttributeNameAt(int nAttribute) const;

	// True si l'attribut appartient a la database
	boolean ContainsAttribute(const SNBDataTableBinarySliceSetAttribute* attribute) const;

	// Obtention d'attribut par index dans un ordre aleatoire
	SNBDataTableBinarySliceSetAttribute* GetRandomAttributeAt(int nRandomAttribute) const;

	// Reordonnancement de facon aleatoire les attributs de la table
	void ShuffleRandomAttributeIterator();

	// Restoration de l'ordre initial des attributs
	void RestoreRandomAttributeIterator();

	////////////////////////////////////////////////
	// Acces aux contenus de la database

	// Nombre d'instances
	int GetInstanceNumber() const;

	// Nombre d'instances dans les chunks initialises
	int GetActiveInstanceNumber() const;

	// Indice de la modalite cible pour une instance
	int GetTargetValueIndexAt(int nInstance) const;

	// Indice de la modalite cible pour une instance active (i.e. dans un chunk initialize)
	int GetTargetValueIndexAtActiveInstance(int nActiveInstance) const;

	// Exporte dans un vecteur de sortie les frequences des parties de la cible pour l'attribut specifie
	// Utilisee pour la regression et classification avec cible groupee
	void ExportTargetPartFrequencies(const SNBDataTableBinarySliceSetAttribute* attribute,
					 IntVector* ivOutput) const;

	// Met a jour un vecteur de scores avec le score de l'attribut pour une cible donnee
	// Pour un individu i dans le vecteur d'entree, valeur cible, attribut k et poids w
	// la transformation est donee par :
	//
	//   score_i -> score_i + w * log P( X_k = x_ik | Y = y_j )
	//
	boolean UpdateTargetValueScores(SNBDataTableBinarySliceSetAttribute* attribute, int nTarget, Continuous cWeight,
					ContinuousVector* cvScores);

	// Ecriture tabulaire des contenus
	void WriteContentsAsTSV(ostream& ost);

	// True s'il y a eu un de lecture dans un appel a CollectRecodedAttributeIndexes
	boolean IsError() const;

	//////////////////////////////
	// Services Divers

	// DataPreparationClass utilise pour construire la database
	// Memoire : Appartient a l'appelle
	KWDataPreparationClass* GetDataPreparationClass();

	// True si l'objet est dans un etat valide
	boolean Check() const override;

	// Label de classe
	const ALString GetClassLabel() const override;

	// Estimation de l'empreinte memoire des valeurs cibles
	static longint ComputeTargetValuesNecessaryMemory(int nInstanceNumber);

	// True si l'attribute est predictive de la cible
	static boolean IsPredictive(KWDataPreparationAttribute* dataPreparationAttribute);

	//////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	/////////////////////////////
	// Initialisation

	// Initialisation des indexes des parties de la target value
	void InitializeTargetValueIndexes(KWClassStats* classStats);

	// Initialisation des indexes des parties pour une target value categorielle
	void InitializeSymbolTargetValueIndexes(KWClassStats* classStats);

	// Initialisation des indexes des parties pour une target value numerique
	void InitializeContinuousTargetValueIndexes(KWClassStats* classStats);

	// Index de partie associe a une valeur d'une cible continue
	int ComputeContinuousTargetValuePartIndex(Continuous cTargetValue, const KWClassStats* classStats) const;

	// Initialisation du buffer et tous les fichiers de chunk
	boolean InitializeAllChunks(KWDataTableSliceSet* sliceSet);

	// Initialisation partielle du buffer pour le chunk specifie
	boolean InitializeBufferOnlyAtChunk(int nChunk, KWDataTableSliceSet* sliceSet);

	/////////////////////////////////
	// Objects de travail

	// KWDataPreparationClass propietaire des tous les KWDataPreparationAttribute's si initialise avec un
	// KWClassStats
	KWDataPreparationClass* dataPreparationClass;

	// Layout de la base de donnes
	SNBDataTableBinarySliceSetLayout layout;

	// Ensemble d'attributs de la database
	SNBDataTableBinarySliceSetSchema schema;

	// Iterateur randomise des attributs
	SNBDataTableBinarySliceSetRandomizedAttributeIterator randomizedAttributeIterator;

	// Buffer de lecture par blocs
	SNBDataTableBinarySliceSetBuffer dataBuffer;

	// Nombre d'attributs initiaux
	int nInitialAttributeNumber;

	// Indices des valeurs de la cible
	IntVector ivTargetValueIndexes;

	// Index du seul chunk initialise pour l'initialisation partielle du buffer
	int nInitializedChunkIndex;

	// True s'il a eu un erreur de lecture du disque
	boolean bIsError;

	// La tache parallele d'apprentissage est friend pour faire des initialisations partielles
	friend class SNBPredictorSNBTrainingTask;
};
