// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWCDUniqueString;
class KWCDUniqueStringData;
class KWCDUniqueStringDictionary;

#include "Standard.h"
#include "ALString.h"
#include "Object.h"
#include "KWVersion.h"
#include "Vector.h"
#include "Timer.h"

///////////////////////////////////////////////////////////////////////////////////
// Classe KWCDUniqueString
//  Classe interne, utilisee pour la gestion efficace des identifiants et libelle
//  dans les dictionnaires:
//    KWClassDomain, KWClass, KWAttribute, KWDerivationRule, KWDerivatioRuleOperand...
// Cela permet de garder chaque identifiant ou libelle en exemplaire unique, tout en
// concervant un acces simple aux valeur de type ALString.
//
// C'est un enjeu important, parce les classes de gestion des dictionnaires ont
// plusieurs niveaux d'usage: specification, utilisation par instanciation d'un
// objet de ces classe, exploitation apres compilation.
// Par exemple, pour les regles de derivation, l'identifiant et le libelle sont
// des elements de specification, qui sont dupliques a chaque utilisation d'une regle.
// De meme, les variables d'un dictionnaire ont un identifiant qui est duplique
// dans chaque operande de regle l'utilisant. La gestion de ces element de specification
// en exemplaire unique permet un gain potentiel de memoire tres important, et une
// diminution du morcellement memoire.
//
// La gestion des KWCDUniqueString est similaire a celle des Symbol, mais elle differe
// sur les points suivants:
//  . les Symbol sont tres optimises en memoire, et ne donne acces qu'a des char*, alors
//    que les UniqueString contiennent directement un ALString, directement utilisable
//  . les Symbol sont dedies a la gestion des valeurs se trouvant dans une base de donnees,
//    alors que les UniqueString sont dedies a la specification des dictionnaire, ce qui
//    justifie qu'il soient dans des epscace de gestion separes, ce qui evitera a priori
//    le morcellement de la memoire allouee
//  . les Symbol ont un API evoluee, facilitant leur manipulation simple partout dans
//    les algorithmes de traitement de donnees, alors que les Unbique string ont une API
//    rudimentaire, parce qu'il ne sont utilisee que dans les classe de gestion des dictionnaires
//
// Techniquement, un KWCDUniqueString est un pointeur gere sur une structure KWCDUniqueStringData.
// La gestion des KWCDUniqueString consiste d'une part a assurer l'unicite des KWCDUniqueString
//  (par valeur chaine de caracteres) au moyen d'un dictionnaire KWCDUniqueStringDictionary,
//  et a gerer un compteur de reference sur chaque KWCDUniqueString, de facon a le desallouer
//  quand ce dernier n'est plus reference nulle part.
class KWCDUniqueString : public SystemObject
{
public:
	// Methode de test
	static void Test();

	///////////////////////////////////////////////////////////////////////////
	// Accessible uniquement pour les classes de gestion des dictionnaires
	// Les variable de type KWCDUniqueString sont prefixees pare us
	friend class KWClassDomain;
	friend class KWClass;
	friend class KWAttribute;
	friend class KWAttributeBlock;
	friend class KWDerivationRule;
	friend class KWDerivationRuleOperand;
	friend class KWMetaData;
	friend class KWKeyValuePair;
	friend class PLShared_MetaData;

protected:
	// Constructeur/destructeur
	KWCDUniqueString();
	~KWCDUniqueString();

	// Operateur d'affectation
	KWCDUniqueString& operator=(const KWCDUniqueString& sSymbol);

	// Acces a la valeur
	void SetValue(const ALString& sValue);
	const ALString& GetValue() const;

	// Estimation de la memoire necessaire au stockage complet de la valeur
	longint GetUsedMemory() const;

	// Nombre de symbols actuellement construits
	static int GetUniqueStringNumber();

	// Memoire (en octets) utilisee pour gerer l'ensemble des symbols
	static longint GetAllUniqueStringsUsedMemory();

	/////////////////////////////////////////////////////////
	//// Implementation
private:
	friend ostream& operator<<(ostream& ost, const KWCDUniqueString& sUniqueString);

	// Gestion dans le dictionnaire centralise des donnees des Symbol
	void NewSharedUniqueStringData(const ALString& sValue);
	void DeleteSharedUniqueStringData();

	// Donnees portee par la valeur d'un UniqueString
	KWCDUniqueStringData* uniqueStringData;

	// Chaine vide, pour toutes les UniqueString non initialisees
	static ALString sEmptyString;

	// Dictionnaire des valeurs des UniqueString
	static KWCDUniqueStringDictionary sdSharedUniqueStrings;
	friend class KWCDUniqueStringDictionary;
};

// Redefinition de l'operateur <<, pour les stream
inline ostream& operator<<(ostream& ost, const KWCDUniqueString& sUniqueString)
{
	ost << sUniqueString.GetValue();
	return ost;
}

///////////////////////////////////////////////////////////////////
// Classe KWCDUniqueStringData
// Cette classe est une classe privee, dont l'unique role est de definir
//  les donnees portee par un KWCDUniqueString pour sa gestion automatique
class KWCDUniqueStringData : public SystemObject
{
	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Constructeur/destructeur
	KWCDUniqueStringData();
	~KWCDUniqueStringData();

	// Acces a la valeur chaine de caracteres
	const ALString& GetValue() const;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Donnees portee par un KWCDUniqueString

	// Pointeur vers le symbole suivant, en cas de collision dans la tabke de hashage
	KWCDUniqueStringData* pNext;

	// Compteur de reference, la taille d'un int est suffisante pour le contexte d'utilisation dans les
	// dictionnaires
	int nRefCount;

	// Valeur de hashage, dependant de la valeur de la chaine de caracteres, independante de la taille de table de
	// hashage
	UINT nHashValue;

	// Valeur
	ALString sValue;

	// Classes friend pour l'implementation
	friend class KWCDUniqueString;
	friend class KWCDUniqueStringDictionary;
};

// Pointeur sur les donnees d'un KWCDUniqueString
// Contrairement a KWCDUniqueString, ce pointeur est un pointeur standard, non gere
// automatiquement, ce qui permet son utilisation par le dictionnaire
// KWSymbolDictionary sans interference avec la gestion automatique.
typedef KWCDUniqueStringData* KWCDUniqueStringDataPtr;

///////////////////////////////////////////////////////////////////////////
// Classe KWCDUniqueStringDictionary: dictionaire de KWCDUniqueStringData
// Inspire directement de KWStringDictionary
// Les valeurs de UniqueString servent a la fois de cle et de valeur dans le
// dictionnaire. Ce sont des objets KWUniqueStringDataPtr, directement integres
// dans le dictionnaire. Leur unicite correspond a l'unicite des UniqueStrings
class KWCDUniqueStringDictionary : public Object
{
public:
	~KWCDUniqueStringDictionary();

protected:
	// Constructeur
	KWCDUniqueStringDictionary();

	// Nombre d'elements
	int GetCount() const;
	boolean IsEmpty() const;

	// Recherche par cle
	// Renvoie NULL si non trouve
	KWCDUniqueStringDataPtr Lookup(const ALString& sValue) const;

	// Ajout d'un nouveau UniqueString, ou acces a un UniqueString existant
	KWCDUniqueStringDataPtr AsUniqueString(const ALString& sValue);

	// Supression des UniqueString
	void RemoveUniqueString(KWCDUniqueStringDataPtr uniqueStringData);
	void RemoveAll();

	// Parcours des UniqueString
	POSITION GetStartPosition() const;
	void GetNextUniqueStringData(POSITION& rNextPosition, KWCDUniqueStringDataPtr& uniqueStringData) const;

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

	// Hash d'une chaine de caracteres
	UINT HashKey(const ALString& sValue) const;

	// Recherche par cle
	KWCDUniqueStringDataPtr GetUniqueStringDataAt(const ALString& sValue, UINT& nHash) const;

	// Affichage d'un message d'erreur lie a un UniqueString non libere en fin de programme
	void ShowAllocErrorMessage(KWCDUniqueStringDataPtr uniqueStringData, int nMessageIndex);

	// Variables
	PointerVector pvUniqueStringDatas;
	int m_nCount;
	friend class KWCDUniqueString;
};

// /////////////////////
/// Methodes en inline

// Classe KWCDUniqueString

inline KWCDUniqueString::KWCDUniqueString()
{
	uniqueStringData = NULL;
}

inline KWCDUniqueString::~KWCDUniqueString()
{
	if (uniqueStringData and --uniqueStringData->nRefCount == 0)
		DeleteSharedUniqueStringData();
}

inline KWCDUniqueString& KWCDUniqueString::operator=(const KWCDUniqueString& sUniqueString)
{
	if (sUniqueString.uniqueStringData)
		++sUniqueString.uniqueStringData->nRefCount;
	if (uniqueStringData and --uniqueStringData->nRefCount == 0)
		DeleteSharedUniqueStringData();
	uniqueStringData = sUniqueString.uniqueStringData;
	return (*this);
}

inline void KWCDUniqueString::SetValue(const ALString& sValue)
{
	if (uniqueStringData and --uniqueStringData->nRefCount == 0)
		DeleteSharedUniqueStringData();
	if (sValue.GetLength() == 0)
		uniqueStringData = NULL;
	else
		NewSharedUniqueStringData(sValue);
}

inline const ALString& KWCDUniqueString::GetValue() const
{
	return (uniqueStringData ? uniqueStringData->sValue : sEmptyString);
}

inline longint KWCDUniqueString::GetUsedMemory() const
{
	return (uniqueStringData
		    ? sizeof(KWCDUniqueStringData) - sizeof(ALString) + uniqueStringData->GetValue().GetUsedMemory()
		    : 0);
}

inline void KWCDUniqueString::NewSharedUniqueStringData(const ALString& sValue)
{
	require(sValue.GetLength() > 0);
	uniqueStringData = sdSharedUniqueStrings.AsUniqueString(sValue);
	++uniqueStringData->nRefCount;
}

inline void KWCDUniqueString::DeleteSharedUniqueStringData()
{
	require(uniqueStringData != NULL);
	require(uniqueStringData->nRefCount == 0);
	sdSharedUniqueStrings.RemoveUniqueString(uniqueStringData);
}

// Classe KWCDUniqueStringData

inline KWCDUniqueStringData::KWCDUniqueStringData()
{
	nRefCount = 0;
	nHashValue = 0;
	pNext = NULL;
}

inline KWCDUniqueStringData::~KWCDUniqueStringData() {}

inline const ALString& KWCDUniqueStringData::GetValue() const
{
	return sValue;
}

// Classe KWCDUniqueStringDictionary

inline KWCDUniqueStringDataPtr KWCDUniqueStringDictionary::Lookup(const ALString& sValue) const
{
	UINT nHash;
	KWCDUniqueStringDataPtr pUniqueStringData = GetUniqueStringDataAt(sValue, nHash);
	return pUniqueStringData;
}

inline int KWCDUniqueStringDictionary::GetHashTableSize() const
{
	return pvUniqueStringDatas.GetSize();
}

inline void KWCDUniqueStringDictionary::InitHashTable(int nHashSize)
{
	assert(m_nCount == 0);
	assert(nHashSize > 0);

	pvUniqueStringDatas.SetSize(nHashSize);
}

inline UINT KWCDUniqueStringDictionary::HashKey(const ALString& sValue) const
{
	UINT nHash = 0;
	int i;

	require(sValue.GetLength() != 0);

	for (i = 0; i < sValue.GetLength(); i++)
		nHash = (nHash << 5) + nHash + sValue.GetAt(i);
	return nHash;
}
