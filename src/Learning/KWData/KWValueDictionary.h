// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWValueDictionary;
class KWContinuousValueDictionary;
class KWSymbolValueDictionary;
class KWObjectArrayValueDictionary;

#include "KWSymbol.h"
#include "KWContinuous.h"
#include "KWType.h"
#include "Object.h"
#include "Vector.h"
#include "KWIndexedKeyBlock.h"

//////////////////////////////////////////////////////////////////
// Classe KWValueDictionary
// Classe virtuelle, ancetre des classes specialisee par type de valeur
// Dictionnaire dediee aux valeurs, permettant d'associer
// des valeurs a des cles
// Utile en particulier pour preparer un bloc de valeurs sparses
class KWValueDictionary : public Object
{
public:
	// Constructeur
	KWValueDictionary();
	~KWValueDictionary();

	// Type de valeur
	virtual int GetType() const = 0;

	// Supression de toutes les valeurs
	void RemoveAll();

	// Nombre de valeurs
	int GetCount() const;

	// Test de presence d'une cle
	boolean IsKeyPresent(const Symbol& sKey) const;

	////////////////////////////////////////////////////////////////
	// Services divers

	// Copie du contenu d'un dictionnaire source du meme type
	void CopyFrom(const KWValueDictionary* sourceValueDictionary);

	// Methode avancee: export sous forme d'un tableau de KWKeyIndex<Value>
	void ExportKeyIndexValues(ObjectArray* oaKeyIndexValues) const;

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Methodes avancees: parcours generique de toutes les paires (cle, valeurs)
	// ou valeur est de la classe KWKeyIndex<Value>
	// Exemple:
	//  position = myDic->GetStartPosition();
	//	while (position != NULL)
	//	{
	//		myDic->GetNextAssoc(position, sKey, keyIndexValue);
	//		cout << sKey << ": " << *keyIndexValue << "\n";
	//	}
	POSITION GetStartPosition() const;
	void GetNextAssoc(POSITION& rNextPosition, Symbol& sKey, KWKeyIndex*& keyIndexValue) const;

	// Dictionnaire des valeurs par cle, contenant des KWKeyIndex<Value>
	NumericKeyDictionary nkdKeyValues;
};

//////////////////////////////////////////////////////////////////
// Classe KWContinuousValueDictionary
// Dictionnaire dediee aux valeurs Continuous
class KWContinuousValueDictionary : public KWValueDictionary
{
public:
	// Constructeur
	KWContinuousValueDictionary();
	~KWContinuousValueDictionary();

	// Type de valeur
	int GetType() const override;

	////////////////////////////////////////////////////////////////
	// Alimentation et acces aux valeurs

	// Recherche d'une valeur: renvoie Missing si non trouve
	Continuous Lookup(const Symbol& sKey) const;

	// Ajout d'une valeur
	// Remplacement de l'eventuelle valeur existante
	void SetAt(const Symbol& sKey, Continuous cNewValue);

	// Modification par addition d'une valeur a une valeur existante
	// Memorisation de la valeur si valeur non deja presente pour la cle
	void UpgradeAt(const Symbol& sKey, Continuous cDeltaValue);

	////////////////////////////////////////////////////////////////
	// Services divers

	// Clone: alloue et retourne le Clone
	KWContinuousValueDictionary* Clone() const;

	// Test
	static void Test();
};

//////////////////////////////////////////////////////////////////
// Classe KWSymbolValueDictionary
// Dictionnaire dediee aux valeurs Symbol
class KWSymbolValueDictionary : public KWValueDictionary
{
public:
	// Constructeur
	KWSymbolValueDictionary();
	~KWSymbolValueDictionary();

	// Type de valeur
	int GetType() const override;

	////////////////////////////////////////////////////////////////
	// Alimentation et acces aux valeurs

	// Recherche d'une valeur: renvoie "" si non trouve
	Symbol& Lookup(const Symbol& sKey) const;

	// Ajout d'une valeur
	// Remplacement de l'eventuelle valeur existante
	void SetAt(const Symbol& sKey, const Symbol& sNewValue);

	////////////////////////////////////////////////////////////////
	// Services divers

	// Clone: alloue et retourne le Clone
	KWSymbolValueDictionary* Clone() const;

	// Test
	static void Test();
};

//////////////////////////////////////////////////////////////////
// Classe KWObjectArrayValueDictionary
// Dictionnaire dediee aux valeurs ObjectArray
class KWObjectArrayValueDictionary : public KWValueDictionary
{
public:
	// Constructeur
	KWObjectArrayValueDictionary();
	~KWObjectArrayValueDictionary();

	// Type de valeur
	int GetType() const override;

	////////////////////////////////////////////////////////////////
	// Alimentation et acces aux valeurs

	// Recherche d'une valeur: renvoie NULL si non trouve
	// Le tableau retourne appartient a l'appele
	ObjectArray* Lookup(const Symbol& sKey) const;

	// Ajout d'un KWObject au tableau associe a la cle
	// L'objet ajoute appartient a l'appelant
	void AddObjectAt(const Symbol& sKey, KWObject* kwoObject);

	////////////////////////////////////////////////////////////////
	// Services divers

	// Clone: alloue et retourne le Clone
	KWObjectArrayValueDictionary* Clone() const;

	// Test
	static void Test();
};

/////////////////////////////////
// Methodes en inline

// Classe KWValueDictionary

inline KWValueDictionary::KWValueDictionary() {}

inline KWValueDictionary::~KWValueDictionary()
{
	nkdKeyValues.DeleteAll();
}

inline void KWValueDictionary::RemoveAll()
{
	nkdKeyValues.DeleteAll();
}

inline int KWValueDictionary::GetCount() const
{
	return nkdKeyValues.GetCount();
}

inline boolean KWValueDictionary::IsKeyPresent(const Symbol& sKey) const
{
	return (nkdKeyValues.Lookup(sKey.GetNumericKey()) != NULL);
}

// Classe KWContinuousValueDictionary

inline KWContinuousValueDictionary::KWContinuousValueDictionary() {}

inline KWContinuousValueDictionary::~KWContinuousValueDictionary() {}

inline Continuous KWContinuousValueDictionary::Lookup(const Symbol& sKey) const
{
	KWContinuousKeyIndex* keyIndexContinuous;
	keyIndexContinuous = cast(KWContinuousKeyIndex*, nkdKeyValues.Lookup(sKey.GetNumericKey()));
	if (keyIndexContinuous != NULL)
		return keyIndexContinuous->GetValue();
	else
		return KWContinuous::GetMissingValue();
}

// Classe KWSymbolValueDictionary

inline KWSymbolValueDictionary::KWSymbolValueDictionary() {}

inline KWSymbolValueDictionary::~KWSymbolValueDictionary() {}

inline Symbol& KWSymbolValueDictionary::Lookup(const Symbol& sKey) const
{
	static Symbol sEmptySymbol;
	KWSymbolKeyIndex* keyIndexSymbol;
	keyIndexSymbol = cast(KWSymbolKeyIndex*, nkdKeyValues.Lookup(sKey.GetNumericKey()));
	if (keyIndexSymbol != NULL)
		return keyIndexSymbol->GetValue();
	else
		return sEmptySymbol;
}

inline ObjectArray* KWObjectArrayValueDictionary::Lookup(const Symbol& sKey) const
{
	KWObjectArrayKeyIndex* keyIndexObjectArray;
	keyIndexObjectArray = cast(KWObjectArrayKeyIndex*, nkdKeyValues.Lookup(sKey.GetNumericKey()));
	if (keyIndexObjectArray != NULL)
		return keyIndexObjectArray->GetValue();
	else
		return NULL;
}

// Classe KWObjectArrayValueDictionary

inline KWObjectArrayValueDictionary::KWObjectArrayValueDictionary() {}

inline KWObjectArrayValueDictionary::~KWObjectArrayValueDictionary() {}