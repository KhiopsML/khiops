// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "ALString.h"
#include "Object.h"
#include "Ermgt.h"

class StatObject;

// Prototype d'une fonction prenant un Object en entree, et rendant sous
// forme de chaine de caracteres la valeur d'un de ses attributs
// Permet d'offrir des services de facon generique
typedef const ALString (*GetterFunction)(const Object* object);

//////////////////////////////////////////////////////////////////////
// Classe de service de query
// Offre des services de query et de calcul de statistiques
// sur le contenu d'un container d'Object, en s'interessant a
// un attribut particulier et en prenant une fonction de comparaison
// d'Object basee sur cet attribut.
// Les containers utilisables sont ObjectArray, ObjectList et
// ObjectDictionary.
class QueryServices : public Object
{
public:
	// Constructeur
	QueryServices();
	~QueryServices();

	// Initialisation des services
	void Init(CompareFunction fCompare, GetterFunction fGetter, const ALString& sClassNameValue,
		  const ALString& sAttributeNameValue);

	// Chargement des objets
	void LoadObjectArray(const ObjectArray* oaObjects);
	void LoadObjectList(const ObjectList* olObjects);
	void LoadObjectDictionary(const ObjectDictionary* odObjects);
	void LoadNumericKeyDictionary(const NumericKeyDictionary* nkdObjects);

	// Verification de l'initialisation
	boolean IsInit() const;
	CompareFunction GetCompareFunction() const;
	GetterFunction GetGetterFunction() const;
	const ALString& GetClassName() const;
	const ALString& GetAttributeName() const;

	//// Services d'extraction

	// Renvoie tous les objets, tries correctement
	// Memoire: le tableau rendu devra etre libere par l'appelant
	ObjectArray* QueryAllObjects() const;
	int GetSize() const;
	Object* GetAt(int nIndex) const;

	// Renvoie tous les objets dont l'attribut de query corresponsant
	// a celui de l'objet en parametre
	// Memoire: le tableau rendu devra etre libere par l'appelant
	ObjectArray* QueryObjectsMatching(const Object* aKey) const;
	int ObjectsMatchingNumber(const Object* aKey) const;
	int ValuesNumber() const;

	// Renvoie un les objets dont l'attribut de query corresponsant
	// a celui de l'objet en parametre, NULL sinon
	Object* Lookup(const Object* aKey) const;

	// Verification des doublons
	// Renvoie false si 2 elements ont meme valeur pour la
	// fonction de comparaison, true sinon
	boolean CheckDoubles() const;

	//// Statistiques: liste ordonnee des valeur, avec nombre d'occurrences

	// Obtention d'un tableau de StatObject, representant les statistiques
	// Memoire: le tableau rendu et son contenu sont a liberer par l'appelant
	ObjectArray* QueryStatistics() const;

	// Ecriture des statistiques dans un fichier
	void WriteStatistics(ostream& ost) const;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Methode de test de classe
	static void Test(ostream& ost);

	////////// Implementation
protected:
	// Attributs de la classe
	ObjectArray oaSorted;
	CompareFunction fCompareFunction;
	GetterFunction fGetterFunction;
	ALString sClassName;
	ALString sAttributeName;
	boolean bIsInit;

	// Methodes de recherche de l'index d'un objet
	// Renvoie -1 si non trouve
	int GetMatchingObjectIndex(const Object* aKey) const;
	int GetMatchingObjectFirstIndex(const Object* aKey, int nStartingIndex) const;
	int GetMatchingObjectLastIndex(const Object* aKey, int nStartingIndex) const;
};

// Exemple de GetterFunction, pour la classe SampleObject, definie dans Object.h
// Implementation:
//    return IntToString( cast(SampleObject*, object)->GetInt()) );
const ALString SampleObjectGetInt(const Object* object);

//////////////////////////////////////////////////////////
// Classe StatObject
// Le premier champs represente la valeur d'un attribut
// le second champs le nombre d'items ayant cette valeur
class StatObject : public Object
{
public:
	// Constructeur
	StatObject();
	~StatObject();

	// Valeur
	void SetAttributeValue(const ALString& sValue);
	const ALString& GetAttributeValue() const;

	// Nombre d'item
	void SetItemNumber(int nValue);
	int GetItemNumber() const;

	// Display
	void Write(ostream& ost) const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///// Implementation
protected:
	ALString sAttributeValue;
	int nItemNumber;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation en inline

// Classe StatObject

inline StatObject::StatObject()
{
	nItemNumber = 0;
}

inline StatObject::~StatObject() {}

inline void StatObject::SetAttributeValue(const ALString& sValue)
{
	sAttributeValue = sValue;
}

inline const ALString& StatObject::GetAttributeValue() const
{
	return sAttributeValue;
}

inline void StatObject::SetItemNumber(int nValue)
{
	require(nValue >= 0);
	nItemNumber = nValue;
}

inline int StatObject::GetItemNumber() const
{
	return nItemNumber;
}

inline void StatObject::Write(ostream& ost) const
{
	ost << GetAttributeValue() << "\t" << GetItemNumber() << "\n";
}