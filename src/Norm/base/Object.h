// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class Object;
class SampleObject;
class StringObject;
class DoubleObject;
class IntObject;
class LongObject;
class ObjectArray;
class ObjectList;
class SortedList;
class ObjectDictionary;
class NumericKeyDictionary;
class LongintDictionary;
class LongintNumericKeyDictionary;

#include "Standard.h"
#include "ALString.h"
#include "MemVector.h"

// Position d'un element dans une container
typedef void* POSITION;

// Classes techniques pour l'implementation des containers
struct ListNode;
struct ODAssoc;
struct NKDAssoc;
struct CPlex;
#define BEFORE_START_POSITION ((void*)-1L)
#define HEAD_POSITION ((void*)0L)
#define TAIL_POSITION ((void*)0L)
typedef unsigned int UINT; // Pour les valeurs de hashage servant d'index

// Prototype des fonctions de comparaisons
typedef int (*CompareFunction)(const void* first, const void* second);

/////////////////////////////////////
// Classe Object
// Toute classe doit en heriter
// Est utilisee pour les container
class Object : public SystemObject
{
public:
	// Construction
	Object();
	virtual ~Object();

	// Affichage
	virtual void Write(ostream& ost) const;

	// Obtention du resultat d'affichage dans une string
	const ALString WriteString() const;

	// Test d'integrite d'un objet
	// A reimplementer dans les sous-classe la
	// ou cela est necessaire
	// Emet des messages d'erreur en cas de probleme
	// Renvoie true par defaut
	virtual boolean Check() const;

	// Estimation de la memoire utilisee
	// Permet une gestion preventive de la memoire dans les applications intensives en donnees
	// Implementation par defaut: avec assert(false), pour forcer la reimplementation si necessaire
	virtual longint GetUsedMemory() const;

	///////////////////////////////////////////
	// Gestion des erreurs
	// Il suffit de reimplementer les methodes
	// renvoyant un libelle associe a une classe
	// un un objet, pour pouvoir disposer de
	// methodes de gestion d'erreur personnalisees

	// Libelle utilisateur associe a la classe
	virtual const ALString GetClassLabel() const;

	// Libelle utilisateur associe a un objet
	virtual const ALString GetObjectLabel() const;

	// Methodes simplifiee de gestion des erreurs
	// (redirigees vers la classe Global)
	virtual void AddSimpleMessage(const ALString& sLabel) const;
	virtual void AddMessage(const ALString& sLabel) const;
	virtual void AddWarning(const ALString& sLabel) const;
	virtual void AddError(const ALString& sLabel) const;
	virtual void AddFatalError(const ALString& sLabel) const;

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
private:
	// On interdit les operateurs suivant generes par defaut par le C++:
	//   . le constructeur par copie (Object myObject(sourceObject) et
	//   . l'operateur = (Object = sourceObject),
	// Ces operateurs sont tres dangereux, car il recopient le bloc memoire de l'objet source tel quel,
	// y compris dans le cas de pointeurs, ce qui fait que les memes sous-objets peuvent etre partages
	// par la source et la cible de la copie (et donc detruits deux fois, par exemple).
	// Si on a besoin de copier un objet explicitement, il faut implementer une methode de type
	//  void CopyFrom(const MyClass* sourceObject)
	Object(const Object&) = delete;
	Object& operator=(const Object&) = delete;
};

// Ecriture dans un stream
inline ostream& operator<<(ostream& ost, const Object& value)
{
	value.Write(ost);
	return ost;
}

// Comparaison de Object, base sur leur adresse
int ObjectCompare(const void* elem1, const void* elem2);

///////////////////////////////////////////////////////
// Classe SampleObject
// Classe tres simple, permettant de faire des tests
class SampleObject : public Object
{
public:
	// Constructeur
	SampleObject();
	SampleObject(int nValue, ALString sValue);
	~SampleObject();

	// Attributs entier et chaine de caractere
	void SetInt(int nValue);
	int GetInt() const;
	void SetString(const ALString& sValue);
	const ALString& GetString() const;

	// Duplication
	SampleObject* Clone() const;

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	int nInt;
	ALString sString;
};

// Comparaison de 2 SampleObject
// Pour valeur d'exemple, l'implementation est:
//   return
//     ( cast(SampleObject*, *(Object**)elem1)->GetInt() -
//       cast(SampleObject*,*(Object**)elem2)->GetInt() );
int SampleObjectCompare(const void* elem1, const void* elem2);

/////////////////////////////////////////////////
// Classe StringObject
// Permet d'utiliser des containers de strings
class StringObject : public Object
{
public:
	// Destructeur
	StringObject();
	~StringObject();

	void SetString(const ALString& sValue);
	const ALString& GetString() const;

	// Duplication
	StringObject* Clone() const;

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	ALString sString;
};

// Comparaison de 2 StringObject
int StringObjectCompare(const void* elem1, const void* elem2);

/////////////////////////////////////////////////
// Classe DoubleObject
// Permet d'utiliser des containers d'entiers
class DoubleObject : public Object
{
public:
	// Destructeur
	DoubleObject();
	~DoubleObject();

	void SetDouble(double dValue);
	double GetDouble() const;

	// Duplication
	DoubleObject* Clone() const;

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	double dDouble;
};

// Comparaison de 2 DoubleObject
int DoubleObjectCompare(const void* elem1, const void* elem2);

/////////////////////////////////////////////////
// Classe IntObject
// Permet d'utiliser des containers d'entiers
class IntObject : public Object
{
public:
	// Destructeur
	IntObject();
	~IntObject();

	void SetInt(int nValue);
	int GetInt() const;

	// Duplication
	IntObject* Clone() const;

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	int nInt;
};

// Comparaison de 2 IntObject
int IntObjectCompare(const void* elem1, const void* elem2);

/////////////////////////////////////////////////
// Classe LongintObject
// Permet d'utiliser des containers d'entiers
class LongintObject : public Object
{
public:
	// Destructeur
	LongintObject();
	~LongintObject();

	void SetLongint(longint lValue);
	longint GetLongint() const;

	// Duplication
	LongintObject* Clone() const;

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	longint lLongint;
};

// Comparaison de 2 LongintObject
int LongintObjectCompare(const void* elem1, const void* elem2);

//////////////////////////////////////////////////////////////////
// Tableau d'objet
// Tableau se rataillant automatiquement lors des insertions
// Fonctionnalite de tris disponibles
//
// Memoire
//   Les objets n'appartiennent pas au tableau: leur liberation
//   n'est pas geree par cette classe
class ObjectArray : public Object
{
public:
	// Constructeur
	ObjectArray();
	~ObjectArray();

	// Taille
	// Lors d'un retaillage, la partie commune entre l'ancien et le nouveau
	// tableau est preservee, la partie supplementaire est initialisee a NULL
	void SetSize(int nValue);
	int GetSize() const;

	// Acces aux elements du vecteur
	void SetAt(int nIndex, Object* oElement);
	Object* GetAt(int nIndex) const;

	// Ajout d'un element en fin et retaillage du tableau
	void Add(Object* oElement);

	// Nettoyage du tableau sans destruction du contenu
	void RemoveAll();

	// Nettoyage du tableau avec destruction du contenu
	void DeleteAll();

	////////////////////////////////////////////////////////////////////
	// Acces avec deplacement des elements suivant l'element inseres
	// Attention: peu efficace algorithmiquement

	// Insertion d'un element
	// L'index peut etre egal a la taille du tableau, ce qui revient a faire un Add()
	void InsertAt(int nIndex, Object* oElement);

	// Supression d'un element
	void RemoveAt(int nIndex);

	// Insertion d'un tableau d'element
	void InsertObjectArrayAt(int nStartIndex, ObjectArray* oaArray);

	////////////////////////////////////////////////////////
	// Services de tri

	// Tri utilisateur avec la fonction de comparaison
	void SetCompareFunction(CompareFunction fCompare);
	CompareFunction GetCompareFunction() const;
	boolean IsSortable() const;
	void Sort();

	// Recherche du premier element du tableau correspondant a searchedKey pour le critere de comparaison
	// Renvoie l'object trouve, NULL sinon
	Object* Lookup(const Object* searchedKey) const;

	// Recherche de l'index du premier element du tableau correspondant a searchedKey pour le critere de comparaison
	// Renvoie -1 si non trouve
	int FindSortIndex(const Object* searchedKey) const;

	// Recherche de l'index du dernier element du tableau plus petit ou egal a searchedKey pour le critere de
	// comparaison
	//  . renvoie l'index du premier element correspondant a la cle si cle trouvee
	//  . renvoie l'index du dernier element strictement plus petit que la cle si cle non trouvee, mais qu'il existe
	//  des element plus grand . renvoie -1 si searchedKey est plus petit que le premier element du tableau.
	int FindPrecedingSortIndex(const Object* searchedKey) const;

	// Tri systeme, base sur les adresses des objets
	void SystemSort();

	/////////////////////////////////////////////////////////
	// Services avances

	// Perturbation aleatoire de l'ordre des elements
	void Shuffle();

	// Retaillage avec potentiellement une grande taille, sans risque d'erreur d'allocation
	// Renvoie false si echec de retaillage (et le vecteur garde sa taille initiale)
	boolean SetLargeSize(int nValue);

	// Initialisation a partir d'un ou deux tableaux source
	// Retaillage eventuel
	void CopyFrom(const ObjectArray* oaSource);
	void ConcatFrom(const ObjectArray* oaSource1, const ObjectArray* oaSource2);

	// Clone: alloue et retourne le Clone
	ObjectArray* Clone() const;

	// Conversions vers les autres containers
	// Memoire: le contenu precedent du container resultat n'est plus reference (mais pas detruit)
	void ExportObjectList(ObjectList* olResult) const;

	// Conversion vers une liste triable si le tableau est associe
	// a une fonction de comparaison (la meme que celle de la liste)
	void ExportSortedList(SortedList* slResult) const;

	// Operations ensemblistes
	// Les operations suivantes sont base sur la comparaison entre les
	// objets eux-memes (pointeurs)
	void Union(ObjectArray* oaFirst, ObjectArray* oaSecond);
	void Intersection(ObjectArray* oaFirst, ObjectArray* oaSecond);
	void Difference(ObjectArray* oaFirst, ObjectArray* oaSecond);

	// Test de contenance d'objets a NULL
	boolean NoNulls() const;
	boolean OnlyNulls() const;

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Estimation de la memoire utilisee avec prise en compte des objet contenus
	longint GetOverallUsedMemory() const;

	// Estimation de la memoire utilisee par element, pour le dimensionnement a prior des containers
	longint GetUsedMemoryPerElement() const;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	/// Test de la classe
	static void Test();

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Constantes specifiques a la classe (permet une compilation optimisee pour les methodes SetAt et GetAt)
	static const int nElementSize = (int)sizeof(Object*);
	static const int nBlockSize = (int)((MemSegmentByteSize) / nElementSize);

	// Taille effective et taille allouee
	int nSize;
	int nAllocSize;

	// Donnees: soit directement un block de valeurs, soit un tableau de blocks
	// selon la taille allouee, soit un vecteur de grande taille generique pour beneficier
	// des methodes de MemVector
	union
	{
		Object** pValues;
		Object*** pValueBlocks;
		MemHugeVector hugeVector;
	} pData;

	// Fonction de comparaison
	CompareFunction fCompareFunction;

	// Concatenation de deux tableaux
	ObjectArray* GetConcatenatedArray(ObjectArray* oaFirst, ObjectArray* oaSecond);
};

//////////////////////////////////////////////////////////////////////
// Liste doublement chainee d'objets
//
// Memoire
//   Les objets n'appartiennent pas a la liste: leur liberation
//   n'est pas geree par cette classe
class ObjectList : public Object
{
public:
	// Constructeur
	ObjectList();
	~ObjectList();

	// Nombre d'elements
	int GetCount() const;
	boolean IsEmpty() const;

	// Tete et queue de liste
	// Ne pas appeler sur une liste vide
	Object* GetHead() const;
	Object* GetTail() const;

	// Suppression de la tete ou de la queue de liste, en renvoyant l'objet correspondant
	Object* RemoveHead();
	Object* RemoveTail();

	// Ajout avant la tete ou apres la queue de liste
	// Retourne la position du nouvel element
	POSITION AddHead(Object* newElement);
	POSITION AddTail(Object* newElement);

	// Ajout d'une autre liste avant la tete ou apres la queue de liste
	void AddHead(ObjectList* pNewList);
	void AddTail(ObjectList* pNewList);

	// Supression ou destruction de tous les elements
	void RemoveAll();
	void DeleteAll();

	// Parcours de lal liste
	// Apres chaque appel de methode, la position contient la nouvelle position
	// Example:
	//   position = myList->GetHeadPosition();
	//   while (position != NULL)
	//       myObject = myList->GetNext(position);
	POSITION GetHeadPosition() const;
	POSITION GetTailPosition() const;
	Object* GetNext(POSITION& rPosition) const; // return *Position++
	Object* GetPrev(POSITION& rPosition) const; // return *Position--

	// Acces a un element pour une position donnee
	// La position doit etre non NULL et valide
	Object* GetAt(POSITION position) const;
	void SetAt(POSITION pos, Object* newElement);
	void RemoveAt(POSITION position);

	// Insertion apres ou avant une position donnee
	// Retourne la position du nouvel element
	POSITION InsertBefore(POSITION position, Object* newElement);
	POSITION InsertAfter(POSITION position, Object* newElement);

	// Recherche d'un element dans la liste (complexite en O(n))
	// Par defaut, on part de la tete de liste
	// Retourne NULL si non trouve
	POSITION Find(Object* searchValue, POSITION startAfter = NULL) const;

	// Recherche du ieme element
	// Retourne NULL si non trouve
	POSITION FindIndex(int nIndex) const;

	// Copie du contenu d'une liste source
	void CopyFrom(const ObjectList* olSource);

	// Clone: alloue et retourne le Clone
	ObjectList* Clone() const;

	// Conversions vers les autres containers
	// Memoire: le contenu precedent du container resultat n'est plus reference (mais pas detruit)
	void ExportObjectArray(ObjectArray* oaResult) const;

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Estimation de la memoire utilisee avec prise en compte des objet contenus
	longint GetOverallUsedMemory() const;

	// Estimation de la memoire utilisee par element, pour le dimensionnement a prior des containers
	longint GetUsedMemoryPerElement() const;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Test de la classe
	static void Test();

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	ListNode* pNodeHead;
	ListNode* pNodeTail;
	int nCount;
	ListNode* pNodeFree;
	struct CPlex* pBlocks;
	static const int nBlockSize = 16;

	ListNode* NewNode(ListNode* pPrev, ListNode* pNext);
	void FreeNode(ListNode* pNode);
};

// Definition d'un cle generique
union GenericKey
{
	GenericKey() : genericKey(0){};
	GenericKey(char* s) : sKey(s){};
	GenericKey(const char* s) : sKey((char*)s){};
	GenericKey(longint l) : genericKey(l){};
	char* sKey;
	ulongint unsignedKey; // Type le plus long a la fois en 32 et 64 bits
	longint genericKey;   // Type le plus long a la fois en 32 et 64 bits
};

// Definition d'un valeur generique
union GenericValue
{
	GenericValue() : lValue(0){};
	GenericValue(Object* o) : oValue(o){};
	GenericValue(longint l) : lValue(l){};
	Object* oValue;
	longint lValue;
	longint genericValue; // Type le plus long a la fois en 32 et 64 bits
};

// Association pour GenericDictionary
struct GDAssoc : public SystemObject
{
	GDAssoc* pNext;
	UINT nHashValue;
	GenericKey key;
	GenericValue value;
};

//////////////////////////////////////////////////////////////////////
// Dictionaire generique
// Classe ancetre de toutes les classe dictionnaires
// Les dictionnaires sont differencies selon:
//  . le type de cle: String ou NUMERIC
//  . le type de valeur: Object* ou longint
class GenericDictionary : public Object
{
public:
	// Destructeur generique
	~GenericDictionary();

	// Nombre d'elements
	int GetCount() const;
	boolean IsEmpty() const;

	// Supression ou destruction de tous les elements
	void RemoveAll();
	void DeleteAll();

	// Indique si les cles sont de type chaine de caracteres (NUMERIC sinon)
	boolean IsStringKey() const;

	// Indique si les valeur sont de type Object* (longint sinon)
	boolean IsObjectValue() const;

	// Supression de toutes les cles correspondant a des valeurs NULL (Object*) ou 0 (longint)
	// On renvoie le nombre de cle supprimees
	int RemoveAllNullValues();

	/////////////////////////////////////////////////////////////////////////
	// Services dans le cas de dictionnaire le cas de valeurs de type Object*

	// Conversions vers les autres containers
	// Memoire: le contenu precedent du container resultat n'est plus reference (mais pas detruit)
	void ExportObjectArray(ObjectArray* oaResult) const;
	void ExportObjectList(ObjectList* olResult) const;

	/////////////////////////////////////////////////////////////////////////
	// Services dans le cas de dictionnaire le cas de valeurs de type longint

	// Modification par addition d'une valeur pour toutes les cles
	void UpgradeAll(longint liDeltaValue);

	// Modification par addition d'une valeur pour toutes les cles, puis se limitant au bornes passees en parametres
	void BoundedUpgradeAll(longint liDeltaValue, longint lLowerBound, longint lUpperBound);

	// Calcul de la valeur min pour toutes les cles
	longint ComputeMinValue() const;

	// Calcul de la valeur max pour toutes les cles
	longint ComputeMaxValue() const;

	// Calcul de la valeur totale pour toutes les cles
	longint ComputeTotalValue() const;

	/////////////////////////////////////////////////////////////
	// Services generiques

	// Affichage du contenu du dictionaire
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Estimation de la memoire utilisee avec prise en compte des objet contenus
	longint GetOverallUsedMemory() const;

	// Estimation de la memoire utilisee par element (sans la cle), pour le dimensionnement a prior des containers
	longint GetUsedMemoryPerElement() const;

	// Libelle de la classe, pour rendre la classe virtuelle
	const ALString GetClassLabel() const override = 0;

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
	//
	// Les cles, gerees par des GenericKey, peuvent soit de type chaine de caractere, soit du type NUMERIC
	// (int, longintint, pointeur), ce qui est plus efficace en memoire et temps de calul en evitant la
	// gestion memoire des chaines de caracteres.
	// Les valeurs, geree par des GenericValue, peuvent etre soit des Object, soit des longint, ce qui est
	// plus efficace si on ne doit memoriser qu'un entier (un compteur par exemple). On pourra passer
	// a des valeurs d'autre type elementaire (double par exemple) si necessaire.
	// La specialisation des traitements selon le type de cle et de caleur se fait dans le constructeur
	// des sous-classes a l'aide des boolen bIsStringKey et bIsObjectValue, ce qui est plus efficace en temps
	// de calcul que via des methodes virtielles reimplementees, et plus simple a specialiser.
	// Sinon, les sous-classe n'ont a redefinir que les methodes specifiques liees aux type des cles et
	// des valeurs en reutilisant les methodes de type Generic*
protected:
	// Constructeur en protected, pour rendre la classe non instanciable
	GenericDictionary();

	///////////////////////////////////////////////////////////////////////////////////////
	// Implementation generique permettant une redefinition simple avec
	// le bon type de cle et de valeur

	// Recherche par cle
	// Renvoie 0 si non trouve
	GenericValue GenericLookup(GenericKey genericKey) const;

	// Ajout d'une nouvelle cle ou recherche de d'une cle existante
	GDAssoc* GenericGetAssocAt(GenericKey genericKey);

	// Ajout d'une nouvelle paire (key, value)
	// (ou remplacement d'une valeur pour une cle donnee)
	void GenericSetAt(GenericKey genericKey, GenericValue genericNewValue);

	// Supression d'une cle
	// Retourne true si la cle existait
	boolean GenericRemoveKey(GenericKey genericKey);

	// Parcours de toutes les paires (key, value)
	// Example:
	//  position = myDic->GetStartPosition();
	//	while (position != NULL)
	//	{
	//		myDic->GetNextAssoc(position, sKey, oElement);
	//      myObject = cast(MyClass*, oElement);
	//		cout << sKey << ": " << *myObject << "\n";
	//	}
	POSITION GenericGetStartPosition() const;
	void GenericGetNextAssoc(POSITION& rNextPosition, GenericKey& genericKey, GenericValue& genericValue) const;

	// Copie du contenu d'un dictionnaire source
	void GenericCopyFrom(const GenericDictionary* gdSource);

	// Taille de la table de hashage
	int GetHashTableSize() const;

	// Initialisation de la table de hashage
	void InitHashTable(int hashSize);

	// Pour le retaillage dynamique, preservant le contenu
	void ReinitHashTable(int nNewHashSize);

	// Fonction de hashage
	UINT HashKey(GenericKey genericKey) const;

	// Gestion des associations
	GDAssoc* NewAssoc();
	void FreeAssoc(GDAssoc* pAssoc);
	GDAssoc* GetAssocAt(GenericKey genericKey, UINT& nHash) const;

	// Variables de gestion du dictionnaire
	PointerVector pvGDAssocs;
	GDAssoc* pFreeList;
	struct CPlex* pBlocks;
	static const int nBlockSize = 16;
	int nCount;

	// Indique si les cles sont de type chaine de caracteres
	boolean bIsStringKey;

	// Indique si les valeur sont de type Object*
	boolean bIsObjectValue;
};

//////////////////////////////////////////////////////////////////////
// Dictionaire d'objet
// Les objets sont identifies par cle de type chaine de caracteres
//
// Memoire
//   Les objets n'appartiennent pas au dictionaire: leur liberation
//   n'est pas geree par cette classe
class ObjectDictionary : public GenericDictionary
{
public:
	// Constructeur
	ObjectDictionary();
	~ObjectDictionary();

	// Recherche par cle
	// Renvoie NULL si non trouve
	Object* Lookup(const char* sKey) const;

	// Ajout d'une nouvelle paire (key, value)
	// (ou remplacement d'une valeur pour une cle donnee)
	void SetAt(const char* sKey, Object* newValue);

	// Supression d'une cle
	boolean RemoveKey(const char* sKey);

	// Parcours de toutes les paires (key, value)
	// Example:
	//  position = myDic->GetStartPosition();
	//	while (position != NULL)
	//	{
	//		myDic->GetNextAssoc(position, sKey, oElement);
	//      myObject = cast(MyClass*, oElement);
	//		cout << sKey << ": " << *myObject << "\n";
	//	}
	POSITION GetStartPosition() const;
	void GetNextAssoc(POSITION& rNextPosition, ALString& sKey, Object*& oValue) const;

	// Copie du contenu d'un dictionnaire source
	void CopyFrom(const ObjectDictionary* odSource);

	// Clone: alloue et retourne le Clone
	ObjectDictionary* Clone() const;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Test de la classe
	static void Test();
};

// Type NUMERIC: type generique servant de cle pour les NumericKeyDictionary
class NUMERIC
{
public:
	// Constructeurs, pour avoir des conversion automatique avec tous les types numeriques
	NUMERIC();
	NUMERIC(const void* p);
	NUMERIC(const int n);
	NUMERIC(const longint l);

	////////////////////////////////////////////////////////////////
	// Acces au contenu de la valeur NUMERIC
	// Attention, ces methodes sont potentiellement risquees

	// Acces au contenu sous la forme d'un pointeur
	void* ToPointer() const;

	// Acces au contenu sous la forme d'un pointeur vers un Object*
	Object* ToObjectPointer() const;

	// Acces au contenu sous la forme d'un longint
	longint ToLongint() const;

	//////////////////////////////////////////////////////////
	///// Implementation
protected:
	friend int operator==(const NUMERIC num1, const NUMERIC num2);
	friend int operator!=(const NUMERIC num1, const NUMERIC num2);
	friend ostream& operator<<(ostream& ost, const NUMERIC value);
	longint lValue;
};

//////////////////////////////////////////////////////////////////////
// Dictionaire d'objet, a cle numerique
// Les objets sont identifies par cle numerique
// Cette cle peut etre un entier, un pointeur, un Object*...
//
// Memoire
//   Les objets n'appartiennent pas au dictionaire: leur liberation
//   n'est pas geree par cette classe
class NumericKeyDictionary : public GenericDictionary
{
public:
	// Constructeur
	NumericKeyDictionary();
	~NumericKeyDictionary();

	// Recherche par cle
	// Renvoie NULL si non trouve
	Object* Lookup(NUMERIC key) const;

	// Ajout d'une nouvelle paire (key, value)
	// (ou remplacement d'une valeur pour une cle donnee)
	void SetAt(NUMERIC key, Object* newValue);

	// Supression d'une cle
	boolean RemoveKey(NUMERIC key);

	// Parcours de toutes les paires (key, value)
	// Example:
	//  position = myDic->GetStartPosition();
	//	while (position != NULL)
	//	{
	//		myDic->GetNextAssoc(position, sKey, oElement);
	//      myObject = cast(MyClass*, oElement);
	//		cout << sKey << ": " << *myObject << "\n";
	//	}
	POSITION GetStartPosition() const;
	void GetNextAssoc(POSITION& rNextPosition, NUMERIC& key, Object*& oValue) const;

	// Copie du contenu d'un dictionnaire source
	void CopyFrom(const NumericKeyDictionary* odSource);

	// Clone: alloue et retourne le Clone
	NumericKeyDictionary* Clone() const;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Test de la classe
	static void Test();
};

//////////////////////////////////////////////////////////////////////
// Dictionaire de longint
class LongintDictionary : public GenericDictionary
{
public:
	// Constructeur
	LongintDictionary();
	~LongintDictionary();

	// Recherche par cle
	// Renvoie 0 si non trouve
	longint Lookup(const char* sKey) const;

	// Ajout d'une nouvelle paire (key, value)
	// (ou remplacement d'une valeur pour une cle donnee)
	void SetAt(const char* sKey, longint liNewValue);

	// Modification par addition d'une valeur pour une cle donnee
	// Si une cle n'existait pas, c'est comme si la valeur precedente etait egale a 0
	void UpgradeAt(const char* sKey, longint liDeltaValue);

	// Supression d'une cle
	boolean RemoveKey(const char* sKey);

	// Parcours de toutes les paires (key, value)
	// Example:
	//  position = myDic->GetStartPosition();
	//	while (position != NULL)
	//	{
	//		myDic->GetNextAssoc(position, sKey, lValue);
	//		cout << sKey << ": " << lValue << "\n";
	//	}
	POSITION GetStartPosition() const;
	void GetNextAssoc(POSITION& rNextPosition, ALString& sKey, longint& lValue) const;

	// Copie du contenu d'un dictionnaire source
	void CopyFrom(const LongintDictionary* ldSource);

	// Clone: alloue et retourne le Clone
	LongintDictionary* Clone() const;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Test de la classe
	static void Test();
};

//////////////////////////////////////////////////////////////////////
// Dictionaire de longint a cle numerique
class LongintNumericKeyDictionary : public GenericDictionary
{
public:
	// Constructeur
	LongintNumericKeyDictionary();
	~LongintNumericKeyDictionary();

	// Recherche par cle
	// Renvoie 0 si non trouve
	longint Lookup(NUMERIC key) const;

	// Ajout d'une nouvelle paire (key, value)
	// (ou remplacement d'une valeur pour une cle donnee)
	void SetAt(NUMERIC key, longint lNewValue);

	// Modification par addition d'une valeur pour une cle donnee
	// Si une cle n'existait pas, c'est comme si la valeur precedente etait egale a 0
	void UpgradeAt(NUMERIC key, longint lDeltaValue);

	// Supression d'une cle
	boolean RemoveKey(NUMERIC key);

	// Parcours de toutes les paires (key, value)
	// Example:
	//  position = myDic->GetStartPosition();
	//	while (position != NULL)
	//	{
	//		myDic->GetNextAssoc(position, sKey, lValue);
	//		cout << sKey << ": " << lValue << "\n";
	//	}
	POSITION GetStartPosition() const;
	void GetNextAssoc(POSITION& rNextPosition, NUMERIC& key, longint& lValue) const;

	// Copie du contenu d'un dictionnaire source
	void CopyFrom(const LongintNumericKeyDictionary* linkdSource);

	// Clone: alloue et retourne le Clone
	LongintNumericKeyDictionary* Clone() const;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Test de la classe
	static void Test();
};

///////////////////////////////////////////////////////////
// Classes technique pour l'implementation des containers

// Noeud de liste doublement chainee
struct ListNode : public SystemObject
{
	ListNode* pNext;
	ListNode* pPrev;
	Object* data;
};

struct CPlex : public SystemObject // Attention: structure de longueur variable
{
	CPlex* pNext;
	UINT nMax;
	UINT nCur;

	void* data()
	{
		return this + 1;
	}

	// Comme calloc, mais sans initialisation a 0
	static CPlex* Create(CPlex*& head, UINT nMax, UINT cbElement);

	// Liberation de pToDelete et de toute la liste chainee qui suit
	static void FreeDataChain(CPlex* pToDelete);
};

// Rend la taille de table superieure ou egale a une taille donnee
// Utile potentiellement pour reimplementer d'autres dictionnaires specialises
int DictionaryGetNextTableSize(int nSize);

// Implementations en inline

inline Object::Object() {}

inline Object::~Object() {}

inline SampleObject::SampleObject()
{
	nInt = 0;
}

inline SampleObject::~SampleObject() {}

inline void SampleObject::SetInt(int nValue)
{
	nInt = nValue;
}

inline int SampleObject::GetInt() const
{
	return nInt;
}

inline void SampleObject::SetString(const ALString& sValue)
{
	sString = sValue;
}

inline const ALString& SampleObject::GetString() const
{
	return sString;
}

inline StringObject::StringObject() {}

inline StringObject::~StringObject() {}

inline void StringObject::SetString(const ALString& sValue)
{
	sString = sValue;
}

inline const ALString& StringObject::GetString() const
{
	return sString;
}

inline void StringObject::Write(ostream& ost) const
{
	ost << sString;
}

inline DoubleObject::DoubleObject()
{
	dDouble = 0;
}

inline DoubleObject::~DoubleObject() {}

inline void DoubleObject::SetDouble(double dValue)
{
	dDouble = dValue;
}

inline double DoubleObject::GetDouble() const
{
	return dDouble;
}

inline void DoubleObject::Write(ostream& ost) const
{
	ost << dDouble;
}

inline IntObject::IntObject()
{
	nInt = 0;
}

inline IntObject::~IntObject() {}

inline void IntObject::SetInt(int nValue)
{
	nInt = nValue;
}

inline int IntObject::GetInt() const
{
	return nInt;
}

inline void IntObject::Write(ostream& ost) const
{
	ost << nInt;
}

inline LongintObject::LongintObject()
{
	lLongint = 0;
}

inline LongintObject::~LongintObject() {}

inline void LongintObject::SetLongint(longint lValue)
{
	lLongint = lValue;
}

inline longint LongintObject::GetLongint() const
{
	return lLongint;
}

inline void LongintObject::Write(ostream& ost) const
{
	ost << lLongint;
}

inline ObjectArray::ObjectArray()
{
	MemVector::Create(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
	fCompareFunction = NULL;
}

inline int ObjectArray::GetSize() const
{
	return nSize;
}

inline void ObjectArray::SetAt(int nIndex, Object* oElement)
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		pData.pValues[nIndex] = oElement;
	else
		(pData.pValueBlocks[nIndex / nBlockSize])[nIndex % nBlockSize] = oElement;
}

inline Object* ObjectArray::GetAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		return pData.pValues[nIndex];
	else
		return (pData.pValueBlocks[nIndex / nBlockSize])[nIndex % nBlockSize];
}

inline void ObjectArray::Add(Object* oElement)
{
	if (nSize < nAllocSize)
		nSize++;
	else
		SetSize(nSize + 1);
	SetAt(nSize - 1, oElement);
}

inline void ObjectArray::RemoveAll()
{
	SetSize(0);
}

inline int ObjectList::GetCount() const
{
	return nCount;
}

inline boolean ObjectList::IsEmpty() const
{
	return nCount == 0;
}

inline Object* ObjectList::GetHead() const
{
	require(not IsEmpty());
	assert(pNodeHead != NULL);
	return pNodeHead->data;
}

inline Object* ObjectList::GetTail() const
{
	require(not IsEmpty());
	assert(pNodeTail != NULL);
	return pNodeTail->data;
}

inline POSITION ObjectList::GetHeadPosition() const
{
	return (POSITION)pNodeHead;
}

inline POSITION ObjectList::GetTailPosition() const
{
	return (POSITION)pNodeTail;
}

inline Object* ObjectList::GetNext(POSITION& rPosition) const
{
	ListNode* pNode = (ListNode*)rPosition;
	check(pNode);
	rPosition = (POSITION)pNode->pNext;
	return pNode->data;
}

inline Object* ObjectList::GetPrev(POSITION& rPosition) const
{
	ListNode* pNode = (ListNode*)rPosition;
	check(pNode);
	rPosition = (POSITION)pNode->pPrev;
	return pNode->data;
}

inline Object* ObjectList::GetAt(POSITION position) const
{
	ListNode* pNode = (ListNode*)position;
	check(pNode);
	return pNode->data;
}

inline void ObjectList::SetAt(POSITION pos, Object* newElement)
{
	ListNode* pNode = (ListNode*)pos;
	check(pNode);
	pNode->data = newElement;
}

inline void ObjectList::FreeNode(ListNode* pNode)
{
	pNode->pNext = pNodeFree;
	pNodeFree = pNode;
	nCount--;
	ensure(nCount >= 0);
}

inline GenericDictionary::~GenericDictionary()
{
	RemoveAll();
	assert(nCount == 0);
}

inline int GenericDictionary::GetCount() const
{
	return nCount;
}

inline boolean GenericDictionary::IsEmpty() const
{
	return nCount == 0;
}

inline boolean GenericDictionary::IsStringKey() const
{
	return bIsStringKey;
}

inline boolean GenericDictionary::IsObjectValue() const
{
	return bIsObjectValue;
}

inline GenericValue GenericDictionary::GenericLookup(GenericKey genericKey) const
{
	UINT nHash;

	GDAssoc* pAssoc = GetAssocAt(genericKey, nHash);
	if (pAssoc == NULL)
		return {(longint)0};
	return pAssoc->value;
}

inline void GenericDictionary::GenericSetAt(GenericKey genericKey, GenericValue genericNewValue)
{
	GenericGetAssocAt(genericKey)->value = genericNewValue;
}

inline POSITION GenericDictionary::GenericGetStartPosition() const
{
	return (nCount == 0) ? NULL : BEFORE_START_POSITION;
}

inline int GenericDictionary::GetHashTableSize() const
{
	return pvGDAssocs.GetSize();
}

inline void GenericDictionary::InitHashTable(int nHashSize)
{
	assert(nCount == 0);
	assert(nHashSize > 0);
	pvGDAssocs.SetSize(nHashSize);
}

inline UINT GenericDictionary::HashKey(GenericKey genericKey) const
{
	if (bIsStringKey)
		return (UINT)HashValue(genericKey.sKey);
	else
		return (UINT)IthRandomUnsignedLongint(genericKey.unsignedKey);
}

inline ObjectDictionary::ObjectDictionary()
{
	bIsStringKey = true;
	bIsObjectValue = true;
}

inline ObjectDictionary::~ObjectDictionary() {}

inline Object* ObjectDictionary::Lookup(const char* sKey) const
{
	return GenericLookup((GenericKey)sKey).oValue;
}

inline void ObjectDictionary::SetAt(const char* sKey, Object* newValue)
{
	GenericSetAt((GenericKey)sKey, (GenericValue)newValue);
}

inline boolean ObjectDictionary::RemoveKey(const char* sKey)
{
	return GenericRemoveKey((GenericKey)sKey);
}

inline POSITION ObjectDictionary::GetStartPosition() const
{
	return GenericGetStartPosition();
}

inline void ObjectDictionary::GetNextAssoc(POSITION& rNextPosition, ALString& sKey, Object*& oValue) const
{
	GenericKey genericKey;
	GenericValue genericValue;

	GenericGetNextAssoc(rNextPosition, genericKey, genericValue);
	sKey = genericKey.sKey;
	oValue = genericValue.oValue;
}

inline NUMERIC::NUMERIC()
{
	lValue = 0;
}

inline NUMERIC::NUMERIC(const void* p)
{
	lValue = (longint)p;
}

inline NUMERIC::NUMERIC(const int n)
{
	lValue = (longint)n;
}

inline NUMERIC::NUMERIC(const longint l)
{
	lValue = (longint)l;
}

inline void* NUMERIC::ToPointer() const
{
	return (void*)lValue;
}

inline Object* NUMERIC::ToObjectPointer() const
{
	return (Object*)lValue;
}

inline longint NUMERIC::ToLongint() const
{
	return lValue;
}

inline int operator==(const NUMERIC num1, const NUMERIC num2)
{
	return num1.lValue == num2.lValue;
}

inline int operator!=(const NUMERIC num1, const NUMERIC num2)
{
	return num1.lValue != num2.lValue;
}

inline ostream& operator<<(ostream& ost, const NUMERIC value)
{
	ost << '[' << (ulongint)value.lValue << ']';
	return ost;
}

inline NumericKeyDictionary::NumericKeyDictionary()
{
	bIsStringKey = false;
	bIsObjectValue = true;
}

inline NumericKeyDictionary::~NumericKeyDictionary() {}

inline Object* NumericKeyDictionary::Lookup(NUMERIC key) const
{
	return GenericLookup((GenericKey)key.ToLongint()).oValue;
}

inline void NumericKeyDictionary::SetAt(NUMERIC key, Object* newValue)
{
	GenericSetAt((GenericKey)key.ToLongint(), (GenericValue)newValue);
}

inline boolean NumericKeyDictionary::RemoveKey(NUMERIC key)
{
	return GenericRemoveKey((GenericKey)key.ToLongint());
}

inline POSITION NumericKeyDictionary::GetStartPosition() const
{
	return GenericGetStartPosition();
}

inline void NumericKeyDictionary::GetNextAssoc(POSITION& rNextPosition, NUMERIC& key, Object*& oValue) const
{
	GenericKey genericKey;
	GenericValue genericValue;

	GenericGetNextAssoc(rNextPosition, genericKey, genericValue);
	key = genericKey.genericKey;
	oValue = genericValue.oValue;
}

inline LongintDictionary::LongintDictionary()
{
	bIsStringKey = true;
	bIsObjectValue = false;
}

inline LongintDictionary::~LongintDictionary() {}

inline longint LongintDictionary::Lookup(const char* sKey) const
{
	return GenericLookup((GenericKey)sKey).genericValue;
}

inline void LongintDictionary::SetAt(const char* sKey, longint liNewValue)
{
	GenericSetAt((GenericKey)sKey, (GenericValue)liNewValue);
}

inline void LongintDictionary::UpgradeAt(const char* sKey, longint liDeltaValue)
{
	GDAssoc* pAssoc;
	pAssoc = GenericGetAssocAt((GenericKey)sKey);
	pAssoc->value.genericValue += liDeltaValue;
}

inline boolean LongintDictionary::RemoveKey(const char* sKey)
{
	return GenericRemoveKey((GenericKey)sKey);
}

inline POSITION LongintDictionary::GetStartPosition() const
{
	return GenericGetStartPosition();
}

inline void LongintDictionary::GetNextAssoc(POSITION& rNextPosition, ALString& sKey, longint& lValue) const
{
	GenericKey genericKey;
	GenericValue genericValue;

	GenericGetNextAssoc(rNextPosition, genericKey, genericValue);
	sKey = genericKey.sKey;
	lValue = genericValue.genericValue;
}

inline LongintNumericKeyDictionary::LongintNumericKeyDictionary()
{
	bIsStringKey = false;
	bIsObjectValue = false;
}

inline LongintNumericKeyDictionary::~LongintNumericKeyDictionary() {}

inline longint LongintNumericKeyDictionary::Lookup(NUMERIC key) const
{
	return GenericLookup((GenericKey)key.ToLongint()).genericValue;
}

inline void LongintNumericKeyDictionary::SetAt(NUMERIC key, longint liNewValue)
{
	GenericSetAt((GenericKey)key.ToLongint(), (GenericValue)liNewValue);
}

inline void LongintNumericKeyDictionary::UpgradeAt(NUMERIC key, longint liDeltaValue)
{
	GDAssoc* pAssoc;
	pAssoc = GenericGetAssocAt((GenericKey)key.ToLongint());
	pAssoc->value.genericValue += liDeltaValue;
}

inline boolean LongintNumericKeyDictionary::RemoveKey(NUMERIC key)
{
	return GenericRemoveKey((GenericKey)key.ToLongint());
}

inline POSITION LongintNumericKeyDictionary::GetStartPosition() const
{
	return GenericGetStartPosition();
}

inline void LongintNumericKeyDictionary::GetNextAssoc(POSITION& rNextPosition, NUMERIC& key, longint& lValue) const
{
	GenericKey genericKey;
	GenericValue genericValue;

	GenericGetNextAssoc(rNextPosition, genericKey, genericValue);
	key = genericKey.genericKey;
	lValue = genericValue.genericValue;
}
