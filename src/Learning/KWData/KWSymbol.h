// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class Symbol;
class SymbolObject;
class SymbolVector;
class KWSymbolData;
class KWSymbolDictionary;
class PLShared_Symbol;
class PLShared_SymbolVector;

#include "Standard.h"
#include "ALString.h"
#include "Object.h"
#include "MemVector.h"
#include "Timer.h"
#include "PLSharedVariable.h"
#include "PLSharedObject.h"
#include "KWVersion.h"

///////////////////////////////////////////////////////////////////
// Classe Symbol
//  Un Symbol est un identifiant, associe  une chaine de caractere.
//  La classe Symbol est faite pour gerer les Symbol comme un type
//   simple (int, float), permettant des declaration, affectation,
//   comparaisons tres efficaces, sans se soucier de la gestion
//   memoire de la valeur chaine de caracteres associee.
//  On declare ainsi un Symbol comme en int (il aura une valeur
//   initiale vide coherente), on peut lors de sa creation
//   l'initialiser a partir d'un autre Symbol ou d'une chaine de
//   caracteres, par la suite lui affecter un autre Symbol ou une
//   chaine de caracteres. On peut egalement comparer des Symbol par
//   les operateurs de comparaisons. Ces comparaisons porte sur
//   l'identifiant du Symbol et non sa valeur chaine de caracteres.
//
// Techniquement, un Symbol est un pointeur gere sur une structure
//  KWSymbolData. La gestion des Symbol consiste d'une part a assurer
//  l'unicite des Symbol (par valeur chaine de caracteres) au moyen
//  d'un dictionnaire KWSymbolDataDictionary, et a gerer un compteur
//  de reference sur chaque Symbol, de facon a desallouer la valeur
//  d'un Symbol quand ce dernier n'est plus reference nulle part.
class Symbol : public SystemObject
{
public:
	// Constructeur/destructeur
	Symbol();
	Symbol(const char* sString);
	Symbol(const char* sString, int nLength);
	Symbol(const Symbol& sSymbol);
	~Symbol();

	// Test si vide
	boolean IsEmpty() const;

	// Remise a vide
	void Reset();

	// Operateurs d'affectation
	Symbol& operator=(const char* sString);
	Symbol& operator=(const Symbol& sSymbol);

	// Operateurs de comparaison
	boolean operator==(const Symbol& sSymbol) const;
	boolean operator!=(const Symbol& sSymbol) const;
	boolean operator<=(const Symbol& sSymbol) const;
	boolean operator>=(const Symbol& sSymbol) const;
	boolean operator<(const Symbol& sSymbol) const;
	boolean operator>(const Symbol& sSymbol) const;
	int Compare(const Symbol& sSymbol) const;

	/////////////////////////////////////////////////////////
	// Methodes portant sur la valeur chaine de caracteres

	// Conversion vers chaine de caracteres
	operator const char*() const;
	const char* GetValue() const;

	// Longueur de la chaine de caracteres associee
	int GetLength() const;

	// Acces aux caracteres de la valeur du Symbol
	char GetAt(int nIndex) const;

	// Comparaison portant sur la valeur du Symbol
	int CompareValue(const Symbol& sSymbol) const;

	///////////////////////////////////////////////////////////////
	// Methodes avancees

	// Renvoie d'une cle de type void*, identifiant unique du Symbol
	// Utile par exemple pour se servir d'un Symbol comme cle dans
	// un dictionnaire a cle numerique
	void* GetNumericKey() const;

	// Estimation de la memoire necessaire au stockage du symbole
	longint GetUsedMemory() const;

	// Valeur speciale de symbole, signifiant: n'importe quelle valeur
	// La valeur reelle du Symbol representant " * " est basee en interne
	// sur une valeur tres speciale, afin d'eviter toute collision
	// avec une modalite existante
	// En externe, le caractere '*' est entoure de blancs, ce qui permet
	// d'eviter les collisions avec les valeurs utilisateurs (base ou dictionnaire)
	// qui sont purgees de leur blancs de debut et fin
	static Symbol& GetStarValue();

	// Calcul d'un symbol inexistant a partir d'un nom de base.
	// Ce nom sera utilise s'il n'y a pas de symbol existant correspondant,
	// sinon, on ajoutera par incrementations successives un numero au
	// prefixe du nom de base jusqu'a trouver un symbol inexistant
	static Symbol BuildNewSymbol(const char* sBaseName);

	// Nombre de symbols actuellement construits
	static int GetSymbolNumber();

	// Memoire (en octets) utilisee pour gerer l'ensemble des symbols
	static longint GetAllSymbolsUsedMemory();

	// Estimation de la memoire necessaire par Symbol (vide)
	static longint GetUsedMemoryPerSymbol();

	// Methode de test
	static void Test();

	/////////////////////////////////////////////////////////
	//// Implementation

#ifdef __C11__
	// Constructeur et affectation de deplacement, pour l'optimisation de la
	// gestion des variables temporaires par le compilateur
	Symbol(Symbol&& sSymbol) noexcept;
	Symbol& operator=(Symbol&& sSymbol) noexcept;
#endif

protected:
	// Implementation specifique en protected pour interdire l'utilisation de *
	int* operator*() const;

	// Gestion dans le dictionnaire centralise des donnees des Symbol
	void NewSharedSymbolData(const char* sValue, int nLength);
	void DeleteSharedSymbolData();

	// Donnees portee par la valeur d'un Symbol
	KWSymbolData* symbolData;

	// Dictionnaire des valeurs des Symbol
	static KWSymbolDictionary sdSharedSymbols;

	friend class KWSymbolDictionary;

	// La classe (union) KWValue a un acces direct sur les internes de
	// la classe Symbol pour reimplementer ses acces a la valeur Symbol
	// de facon efficace
	friend union KWValue;
};

// Redefinition de l'operateur <<, pour les stream
inline ostream& operator<<(ostream& ost, const Symbol& sSymbol)
{
	ost << (const char*)sSymbol;
	return ost;
}

/////////////////////////////////////////////////
// Classe SymbolObject
// Permet d'utiliser des containers de Symbol
class SymbolObject : public Object
{
public:
	// Destructeur
	SymbolObject();
	~SymbolObject();

	// Valeur
	void SetSymbol(const Symbol& sValue);
	Symbol& GetSymbol() const;

	// Duplication
	SymbolObject* Clone() const;

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///// Implementation
protected:
	mutable Symbol sSymbol;
};

// Comparaison de deux SymbolObject
int SymbolObjectCompare(const void* elem1, const void* elem2);

//////////////////////////////////////////////////////////
// Classe SymbolVector
// Vecteur de Symbol, de taille quelconque.
// Les acces aux vecteurs sont controles par assertions.
class SymbolVector : public Object
{
public:
	// Constructeur
	SymbolVector();
	~SymbolVector();

	// Taille
	// Lors d'un retaillage, la partie commune entre l'ancien et le nouveau
	// vecteur est preservee, la partie supplementaire est initialisee a '0'
	void SetSize(int nValue);
	int GetSize() const;

	// (Re)initialisation a 0
	void Initialize();

	// Acces aux elements du vecteur
	void SetAt(int nIndex, const Symbol& sValue);
	Symbol& GetAt(int nIndex) const;

	// Ajout d'un element en fin et retaillage du vecteur
	void Add(const Symbol& sValue);

	// Copie a partir d'un vecteur source
	// (retaillage si necessaire)
	void CopyFrom(const SymbolVector* svSource);

	// Perturbation aleatoire de l'ordre des valeurs
	void Shuffle();

	// Duplication
	SymbolVector* Clone() const;

	// Retaillage avec potentiellement une grande taille, sans risque d'erreur d'allocation
	// Renvoie false si echec de retaillage (et le vecteur garde sa taille initiale)
	boolean SetLargeSize(int nValue);

	// Tri des valeur par ordre croissant, soit par valeur alphabetique,
	// soit par cle numerique (rapide, mais non intelligible)
	void SortValues();
	void SortKeys();

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Test
	static void Test();

	///////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attention: l'implementation des methodes de SymbolVector doit tenir compte des compteurs
	// de reference des Symbol pour permettre leur desallocation automatique

	// Constantes specifiques a la classe (permet une compilation optimisee pour les methodes SetAt et GetAt)
	static const int nElementSize = (int)sizeof(Symbol);
	static const int nBlockSize = (int)((MemSegmentByteSize) / nElementSize);

	// Taille effective et taille allouee
	int nSize;
	int nAllocSize;

	// Donnees: soit directement un block de valeurs, soit un tableau de blocks
	// selon la taille allouee, soit un vecteur de grande taille generique pour beneficier
	// des methodes de MemVector
	union
	{
		Symbol* pValues;
		Symbol** pValueBlocks;
		MemHugeVector hugeVector;
	} pData;
};

// Tri par taille croissante, puis selon l'ordre lexicographique de la premiere valeur
int SymbolVectorCompare(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////// Implementation

///////////////////////////////////////////////////////////////////
// Classe KWSymbolAsString
// Sous-classe de ALString dediee a la manipulation des Symbol
// Permet d'optimiser la creation d'une ALString temporaire a partir d'un Symbol en
// recuperant son contenu (chaine de caracteres et longueur) sans allocation ni calcul.
// Le destructeur dereference ce contenu pour ne pas le desallouer.
// Attention, usage expert. Les chaines ainsi constituees ne doivent etre utilisees
// que pour acceder aux methodes const de ALString qui ne modifient pas sont contenu.
// Sinon, cela entrainera des corruptions dans la memoire.
// Le bon usage est de declarer une variable locale de travail en const:
//   const KWSymbolAsString sTmpValue(sMySymbol);
class KWSymbolAsString : public ALString
{
public:
	// Constructeur, referencant les donnees internes d'un Symbol
	inline KWSymbolAsString(Symbol sValue)
	{
		nDataLength = sValue.GetLength();
		nAllocLength = nDataLength;
		pchData = (char*)sValue.GetValue();
	}

	// Destructeur, dereferencant ses donnees internes pour ne pas les desallouer
	inline ~KWSymbolAsString()
	{
		Init();
	};
};

///////////////////////////////////////////////////////////////////
// Classe KWSymbolData
// Cette classe est une classe privee, dont l'unique role est de definir
//  les donnees portee par un Symbol pour sa gestion automatique
//   Donnees portees par un Symbol:
//    Valeur
//    Nombre de references, permetttant de gerer la memoire automatiquement
//    HashValue, pour la gestion efficace dans un dictionnaire
class KWSymbolData : public SystemObject
{
	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Longueur de la chaine de caracteres
	inline int GetLength()
	{
		return nLength;
	}

	// Acces a la valeur chaine de caracteres
	inline char* GetString()
	{
		return cFirstStringChar;
	}
	inline char GetAt(int nIndex)
	{
		return cFirstStringChar[nIndex];
	}

	// Gestion des donnees des symboles
	static KWSymbolData* NewSymbolData(const char* sValue, int nLength);
	static void DeleteSymbolData(KWSymbolData* pSymbolData);

	// Acces au symbol special StarValue (gere a part, different des symboles standards)
	inline static KWSymbolData* GetStarValue()
	{
		static KWSymbolData* sStarValue = NULL;
		if (sStarValue == NULL)
			sStarValue = InitStarValue();
		return sStarValue;
	}
	static KWSymbolData* InitStarValue();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Donnees portee par un Symbol
	// Lors de l'allocation, on alloue la taille de la structure KWSymbolData plus
	// celle de la valeur, pour tout soit gere dans un seul bloc memoire
	// (cela revient a gere une structure de taille variable, selon la longueur de la chaine de caracteres)
	// On prevoit une taille minimum (que de toute facon le compilateur prevoit pour des raisons d'alignement
	// processeur)

	// Compteur de reference, de grande taille pour ne pas etre limite a la taille des int
	unsigned long long int lRefCount;

	// Pointeur vers le symbole suivant, en cas de collision
	KWSymbolData* pNext;

	// Valeur de hashage, dependant de la valeur de la chaine de caracteres, independante de la taille de table de
	// hashage
	UINT nHashValue;

	// Taille de la chaine de caractere correspondant a la valeur du Symbol
	// Permet d'exploiter plus directement les Symbol via des ALString
	int nLength;

	// Stockage de la valeur du symbole (de taille quelconque)
	// On utilise une taille de 4 pour que la memoire total soit un multiple de 8 octets en 64 bits
	static const int nMinStringSize = 8;
	char cFirstStringChar[nMinStringSize]; // Pour acceder au debut de la chaine

	friend class Symbol;
	friend class KWSymbolDictionary;
	friend union KWValue;
};

////////////////////////////////////////////////////////
// Implementation

// Pointeur sur la valeur d'un Symbol
// Contrairement a Symbol, ce pointeur est un pointeur standard, non gere
// automatiquement, ce qui permet son utilisation par le dictionnaire
// KWSymbolDictionary sans interference avec la gestion automatique.
typedef KWSymbolData* KWSymbolDataPtr;

///////////////////////////////////////////////////////////////////////////
// Dictionaire de KWSymbolData
// Inspire directement de ObjectDictionary
// Les valeurs de Symbol servent a la fois de cle et de valeur dans le
// dictionnaire. Ce sont des objets KWSymbolDataPtr, directement integres
// dans le dictionnaire. Leur unicite correspond a l'unicite des Symbols
class KWSymbolDictionary : public Object
{
public:
	~KWSymbolDictionary();

protected:
	// Constructeur
	KWSymbolDictionary();

	// Nombre d'elements
	int GetCount() const;
	boolean IsEmpty() const;

	// Recherche par cle
	// Renvoie NULL si non trouve
	KWSymbolDataPtr Lookup(const char* key) const;

	// Ajout d'un nouveau Symbol, ou acces a un Symbol existant
	KWSymbolDataPtr AsSymbol(const char* key, int nLength);

	// Supression des Symbol
	void RemoveSymbol(KWSymbolDataPtr symbolData);
	void RemoveAll();

	// Parcours des Symbol
	POSITION GetStartPosition() const;
	void GetNextSymbolData(POSITION& rNextPosition, KWSymbolDataPtr& sSymbol) const;

	// Estimation de la memoire necessaire au stockage du dictionnaire et de tous ses symboles
	longint GetUsedMemory() const override;

	// Affichage du contenu du dictionaire
	void Write(ostream& ost) const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Test de la classe
	static void Test();

	////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Taille de la table de hashage
	int GetHashTableSize() const;

	// Initialisation de la table de hashage
	void InitHashTable(int hashSize);

	// Pour le retaillage dynamique, preservant le contenu
	void ReinitHashTable(int nNewHashSize);

	// Fonction de hashage
	UINT HashKey(const char* key) const;

	// Recherche par cle
	KWSymbolDataPtr GetSymbolDataAt(const char* key, UINT& nHash) const;

	// Affichage d'un message d'erreur lie a un Symbol non libere en fin de programme
	void ShowAllocErrorMessage(KWSymbolDataPtr symbolData, int nMessageIndex);

	// Variables
	PointerVector pvSymbolDatas;
	int m_nCount;
	friend class Symbol;
	friend union KWValue;
};

///////////////////////////////////////////////////////
// Classe PLShared_Symbol
// Serialisation du type Symbol
class PLShared_Symbol : public PLSharedVariable
{
public:
	PLShared_Symbol();
	~PLShared_Symbol();

	// Reimplementation des methodes virtuelles
	void Clean() override;

	// Ecriture
	void Write(ostream& ost) const override;

	// Methode de test
	static void Test();

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation

	// Operateurs d'affectation entre symbol et symbol partage
	const PLShared_Symbol& operator=(const PLShared_Symbol& sString);
	const PLShared_Symbol& operator=(Symbol& sSymbol);
	operator Symbol() const;

protected:
	Symbol sSymbolValue;

	// Acces a la valeur
	void SetValue(Symbol sValue);
	Symbol GetValue() const;

	// Comparaison de valeur
	int Compare(Symbol sOtherValue) const;

	// Reimplementation des methodes virtuelles
	void SerializeValue(PLSerializer*) const override;
	void DeserializeValue(PLSerializer*) override;

	// Operateurs en friend
	friend boolean operator==(const PLShared_Symbol& v1, const PLShared_Symbol& v2);
	friend boolean operator==(const PLShared_Symbol& v1, Symbol v2);
	friend boolean operator==(Symbol v1, const PLShared_Symbol& v2);
	friend boolean operator!=(const PLShared_Symbol& v1, const PLShared_Symbol& v2);
	friend boolean operator!=(const PLShared_Symbol& v1, Symbol v2);
	friend boolean operator!=(Symbol v1, const PLShared_Symbol& v2);
	friend boolean operator<(const PLShared_Symbol& v1, const PLShared_Symbol& v2);
	friend boolean operator<(const PLShared_Symbol& v1, Symbol v2);
	friend boolean operator<(Symbol v1, const PLShared_Symbol& v2);
	friend boolean operator>(const PLShared_Symbol& v1, const PLShared_Symbol& v2);
	friend boolean operator>(const PLShared_Symbol& v1, Symbol v2);
	friend boolean operator>(Symbol v1, const PLShared_Symbol& v2);
	friend boolean operator<=(const PLShared_Symbol& v1, const PLShared_Symbol& v2);
	friend boolean operator<=(const PLShared_Symbol& v1, Symbol v2);
	friend boolean operator<=(Symbol v1, const PLShared_Symbol& v2);
	friend boolean operator>=(const PLShared_Symbol& v1, const PLShared_Symbol& v2);
	friend boolean operator>=(const PLShared_Symbol& v1, Symbol v2);
	friend boolean operator>=(Symbol v1, const PLShared_Symbol& v2);
};

///////////////////////////////////////////////////////
// Classe PLShared_SymbolVector
// Serialisation des vecteurs de Continuous
class PLShared_SymbolVector : public PLSharedObject
{
public:
	// Constructeur
	PLShared_SymbolVector();
	~PLShared_SymbolVector();

	// Acces au vecteur (appartient a l'appele, jamais NULL)
	void SetSymbolVector(SymbolVector* sv);
	SymbolVector* GetSymbolVector();
	const SymbolVector* GetConstSymbolVector() const;

	// Acces aux elements du vecteur
	void SetAt(int nIndex, const Symbol& sValue);
	Symbol& GetAt(int nIndex) const;
	void Add(const Symbol& sValue);

	// Taille du vecteur
	int GetSize() const;

	// Reimplementation des methodes virtuelles
	void SerializeObject(PLSerializer* serializer, const Object* o) const override;
	void DeserializeObject(PLSerializer* serializer, Object* o) const override;

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Object* Create() const override;
};

// Implementations en inline
inline int KWSymbolDictionary::GetCount() const
{
	return m_nCount;
}

inline boolean KWSymbolDictionary::IsEmpty() const
{
	return m_nCount == 0;
}

inline POSITION KWSymbolDictionary::GetStartPosition() const
{
	return (m_nCount == 0) ? NULL : BEFORE_START_POSITION;
}

inline int KWSymbolDictionary::GetHashTableSize() const
{
	return pvSymbolDatas.GetSize();
}

////////////////////////////////////////////////////////////

inline Symbol::Symbol()
{
	symbolData = NULL;
}

inline Symbol::Symbol(const char* sString)
{
	require(sString != NULL);

	if (sString[0] == '\0')
		symbolData = NULL;
	else
		NewSharedSymbolData(sString, (int)strlen(sString));
}

inline Symbol::Symbol(const char* sString, int nLength)
{
	require(sString != NULL);

	if (sString[0] == '\0')
		symbolData = NULL;
	else
		NewSharedSymbolData(sString, nLength);
}

inline Symbol::Symbol(const Symbol& sSymbol)
{
	symbolData = sSymbol.symbolData;
	if (symbolData)
		++symbolData->lRefCount;
}

inline Symbol::~Symbol()
{
	if (symbolData and --symbolData->lRefCount == 0)
		DeleteSharedSymbolData();
}

inline boolean Symbol::IsEmpty() const
{
	return symbolData == NULL;
}

inline void Symbol::Reset()
{
	if (symbolData and --symbolData->lRefCount == 0)
		DeleteSharedSymbolData();
	symbolData = NULL;
}

#ifdef __C11__
inline Symbol::Symbol(Symbol&& sSymbol) noexcept
{
	symbolData = sSymbol.symbolData;
	sSymbol.symbolData = NULL;
}

inline Symbol& Symbol::operator=(Symbol&& sSymbol) noexcept
{
	if (this != &sSymbol)
	{
		if (symbolData and --symbolData->lRefCount == 0)
			DeleteSharedSymbolData();
		symbolData = sSymbol.symbolData;
		sSymbol.symbolData = NULL;
	}
	return (*this);
}
#endif // defined __C11__

inline Symbol& Symbol::operator=(const char* sString)
{
	require(sString != NULL);

	if (symbolData and --symbolData->lRefCount == 0)
		DeleteSharedSymbolData();
	if (sString[0] == '\0')
		symbolData = NULL;
	else
		NewSharedSymbolData(sString, int(strlen(sString)));
	return (*this);
}

inline Symbol& Symbol::operator=(const Symbol& sSymbol)
{
	if (sSymbol.symbolData)
		++sSymbol.symbolData->lRefCount;
	if (symbolData and --symbolData->lRefCount == 0)
		DeleteSharedSymbolData();
	symbolData = sSymbol.symbolData;
	return (*this);
}

inline boolean Symbol::operator==(const Symbol& sSymbol) const
{
	return (symbolData == sSymbol.symbolData);
}

inline boolean Symbol::operator!=(const Symbol& sSymbol) const
{
	return (symbolData != sSymbol.symbolData);
}

inline boolean Symbol::operator<=(const Symbol& sSymbol) const
{
	return (symbolData <= sSymbol.symbolData);
}

inline boolean Symbol::operator>=(const Symbol& sSymbol) const
{
	return (symbolData >= sSymbol.symbolData);
}

inline boolean Symbol::operator<(const Symbol& sSymbol) const
{
	return (symbolData < sSymbol.symbolData);
}

inline boolean Symbol::operator>(const Symbol& sSymbol) const
{
	return (symbolData > sSymbol.symbolData);
}

inline int Symbol::Compare(const Symbol& sSymbol) const
{
	// Comparaison de deux pointeurs: attention une simple difference ne rend pas necessairement un int (en 64 bits)
	if (symbolData == sSymbol.symbolData)
		return 0;
	else if (symbolData > sSymbol.symbolData)
		return 1;
	else
		return -1;
}

inline int Symbol::CompareValue(const Symbol& sSymbol) const
{
	return strcmp(GetValue(), sSymbol.GetValue());
}

inline Symbol::operator const char*() const
{
	return (symbolData ? symbolData->GetString() : "");
}

inline const char* Symbol::GetValue() const
{
	return (symbolData ? symbolData->GetString() : "");
}

inline int Symbol::GetLength() const
{
	return (symbolData ? symbolData->GetLength() : 0);
}

inline char Symbol::GetAt(int nIndex) const
{
	require(nIndex >= 0);
	require(nIndex < GetLength());

	return symbolData->GetAt(nIndex);
}

inline void* Symbol::GetNumericKey() const
{
	return symbolData;
}

inline void Symbol::NewSharedSymbolData(const char* sValue, int nLength)
{
	require(sValue != NULL);
	require(sValue[0] != '\0');
	symbolData = sdSharedSymbols.AsSymbol(sValue, nLength);
	++symbolData->lRefCount;
}

inline void Symbol::DeleteSharedSymbolData()
{
	require(symbolData != NULL);
	require(symbolData->lRefCount == 0);
	sdSharedSymbols.RemoveSymbol(symbolData);
}

inline Symbol& Symbol::GetStarValue()
{
	static KWSymbolData* sStarValue = KWSymbolData::GetStarValue();
	return *(Symbol*)&sStarValue;
}

// Classe SymbolObject

inline SymbolObject::SymbolObject() {}

inline SymbolObject::~SymbolObject() {}

inline void SymbolObject::SetSymbol(const Symbol& sValue)
{
	sSymbol = sValue;
}

inline Symbol& SymbolObject::GetSymbol() const
{
	return sSymbol;
}

inline void SymbolObject::Write(ostream& ost) const
{
	ost << sSymbol;
}

// Classe SymbolVector

inline SymbolVector::SymbolVector()
{
	MemVector::Create(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

inline int SymbolVector::GetSize() const
{
	return nSize;
}

inline void SymbolVector::SetAt(int nIndex, const Symbol& sValue)
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		pData.pValues[nIndex] = sValue;
	else
		(pData.pValueBlocks[nIndex / nBlockSize])[nIndex % nBlockSize] = sValue;
}

inline Symbol& SymbolVector::GetAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		return pData.pValues[nIndex];
	else
		return (pData.pValueBlocks[nIndex / nBlockSize])[nIndex % nBlockSize];
}

inline void SymbolVector::Add(const Symbol& sValue)
{
	// Attention: on force le retaillage explicitement pour avoir une Symbol NULL au nouvel index
	// afin de gerer correctement le dereferencement des Symbol
	SetSize(nSize + 1);
	SetAt(nSize - 1, sValue);
}

// Classe PLShared_Symbol

inline void PLShared_Symbol::SetValue(Symbol sValue)
{
	require(bIsWritable);
	sSymbolValue = sValue;
}
inline Symbol PLShared_Symbol::GetValue() const
{
	require(bIsReadable);
	require(bIsDeclared);
	return sSymbolValue;
}

inline void PLShared_Symbol::Clean()
{
	sSymbolValue = "";
}

inline const PLShared_Symbol& PLShared_Symbol::operator=(const PLShared_Symbol& shared_SymbolValue)
{
	SetValue(shared_SymbolValue.GetValue());
	return *this;
}

inline const PLShared_Symbol& PLShared_Symbol::operator=(Symbol& sValue)
{
	SetValue(sValue);
	return *this;
}

inline PLShared_Symbol::operator Symbol() const
{
	return GetValue();
}

inline int PLShared_Symbol::Compare(Symbol lOtherValue) const
{
	return GetValue().Compare(lOtherValue);
}

inline boolean operator==(const PLShared_Symbol& v1, const PLShared_Symbol& v2)
{
	int nCompare;
	nCompare = v1.GetValue().Compare(v2.GetValue());
	return nCompare == 0;
}

inline boolean operator==(const PLShared_Symbol& v1, Symbol v2)
{
	int nCompare;
	nCompare = v1.GetValue().Compare(v2);
	return nCompare == 0;
}

inline boolean operator==(Symbol v1, const PLShared_Symbol& v2)
{
	int nCompare;
	nCompare = v1.Compare(v2.GetValue());
	return nCompare == 0;
}

inline boolean operator!=(const PLShared_Symbol& v1, const PLShared_Symbol& v2)
{
	int nCompare;
	nCompare = v1.GetValue().Compare(v2.GetValue());
	return nCompare != 0;
}

inline boolean operator!=(const PLShared_Symbol& v1, Symbol v2)
{
	int nCompare;
	nCompare = v1.GetValue().Compare(v2);
	return nCompare != 0;
}

inline boolean operator!=(Symbol v1, const PLShared_Symbol& v2)
{
	int nCompare;
	nCompare = v1.Compare(v2.GetValue());
	return nCompare != 0;
}

inline boolean operator<(const PLShared_Symbol& v1, const PLShared_Symbol& v2)
{
	int nCompare;
	nCompare = v1.GetValue().Compare(v2.GetValue());
	return nCompare < 0;
}

inline boolean operator<(const PLShared_Symbol& v1, Symbol v2)
{
	int nCompare;
	nCompare = v1.GetValue().Compare(v2);
	return nCompare < 0;
}

inline boolean operator<(Symbol v1, const PLShared_Symbol& v2)
{
	int nCompare;
	nCompare = v1.Compare(v2.GetValue());
	return nCompare < 0;
}

inline boolean operator>(const PLShared_Symbol& v1, const PLShared_Symbol& v2)
{
	int nCompare;
	nCompare = v1.GetValue().Compare(v2.GetValue());
	return nCompare > 0;
}

inline boolean operator>(const PLShared_Symbol& v1, Symbol v2)
{
	int nCompare;
	nCompare = v1.GetValue().Compare(v2);
	return nCompare > 0;
}

inline boolean operator>(Symbol v1, const PLShared_Symbol& v2)
{
	int nCompare;
	nCompare = v1.Compare(v2.GetValue());
	return nCompare > 0;
}

inline boolean operator<=(const PLShared_Symbol& v1, const PLShared_Symbol& v2)
{
	int nCompare;
	nCompare = v1.GetValue().Compare(v2.GetValue());
	return nCompare <= 0;
}

inline boolean operator<=(const PLShared_Symbol& v1, Symbol v2)
{
	int nCompare;
	nCompare = v1.GetValue().Compare(v2);
	return nCompare <= 0;
}

inline boolean operator<=(Symbol v1, const PLShared_Symbol& v2)
{
	int nCompare;
	nCompare = v1.Compare(v2.GetValue());
	return nCompare <= 0;
}

inline boolean operator>=(const PLShared_Symbol& v1, const PLShared_Symbol& v2)
{
	int nCompare;
	nCompare = v1.GetValue().Compare(v2.GetValue());
	return nCompare >= 0;
}

inline boolean operator>=(const PLShared_Symbol& v1, Symbol v2)
{
	int nCompare;
	nCompare = v1.GetValue().Compare(v2);
	return nCompare >= 0;
}

inline boolean operator>=(Symbol v1, const PLShared_Symbol& v2)
{
	int nCompare;
	nCompare = v1.Compare(v2.GetValue());
	return nCompare >= 0;
}

// Classe PLShared_SymbolVector

inline void PLShared_SymbolVector::SetSymbolVector(SymbolVector* sv)
{
	require(bIsWritable);
	SetObject(sv);
}

inline SymbolVector* PLShared_SymbolVector::GetSymbolVector()
{
	require(bIsReadable);
	require(bIsDeclared);

	return cast(SymbolVector*, GetObject());
}

inline const SymbolVector* PLShared_SymbolVector::GetConstSymbolVector() const
{
	require(bIsReadable);
	require(bIsDeclared);

	return cast(SymbolVector*, GetObject());
}

inline Symbol& PLShared_SymbolVector::GetAt(int nIndex) const
{
	require(bIsReadable);
	require(bIsDeclared);

	return GetConstSymbolVector()->GetAt(nIndex);
}

inline void PLShared_SymbolVector::SetAt(int nIndex, const Symbol& sValue)
{
	require(bIsWritable);
	GetSymbolVector()->SetAt(nIndex, sValue);
}

inline void PLShared_SymbolVector::Add(const Symbol& sValue)
{
	require(bIsWritable);
	GetSymbolVector()->Add(sValue);
}

inline int PLShared_SymbolVector::GetSize() const
{
	require(bIsReadable);
	require(bIsDeclared);

	return GetConstSymbolVector()->GetSize();
}

inline Object* PLShared_SymbolVector::Create() const
{
	return new SymbolVector;
}
