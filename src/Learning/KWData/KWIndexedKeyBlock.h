// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWIndexedKeyBlock;
class KWKeyIndex;
class KWContinuousKeyIndex;
class KWSymbolKeyIndex;
class KWObjectArrayKeyIndex;

#include "KWSymbol.h"
#include "KWContinuous.h"
#include "KWType.h"
#include "Object.h"
#include "Vector.h"
#include "KWSortableIndex.h"

//////////////////////////////////////////////////////////////////
// Classe KWIndexedKeyBlock: gestion d'un ensemble de cles indexees
// Les cles sont ajoutees une a une et leur index d'insertion est
// memorise, et accessible efficacement
// Utile en particulier pour indexer un bloc de valeurs sparses
// dont les variables sont identifiees par des cles
class KWIndexedKeyBlock : public Object
{
public:
	// Type de cle utilisee pour les variables du bloc
	// Valeurs possibles:
	//   KWType::Symbol: cles categorielles (ex: <VarKey="dix">, <VarKey="1">
	//   KWType::Continuous: cles numeriques entieres, plus grande que 1 (ex: <VarKey=1>, <VarKey=10>
	virtual int GetVarKeyType() const = 0;

	// Nettoyage
	virtual void Clean() = 0;

	// Indexation des cles pour preparer l'acces efficace
	virtual boolean IndexKeys() = 0;

	// Nombre total de cles
	virtual int GetKeyNumber() const = 0;
};

//////////////////////////////////////////////////////////////////
// Classe KWIndexedCKeyBlock: gestion d'un ensemble de cles indexees
// dans le cas de cles categorielles
class KWIndexedCKeyBlock : public KWIndexedKeyBlock
{
public:
	// Constructeur
	KWIndexedCKeyBlock();
	~KWIndexedCKeyBlock();

	// Type de cle utilisee pour les variables du bloc
	int GetVarKeyType() const override;

	// Nettoyage
	void Clean() override;

	// Indexation des cles pour preparer l'acces efficace
	boolean IndexKeys() override;

	//////////////////////////////////////////////////////////////////
	// Alimentation des cles

	// Ajout d'une cle (ne doit pas etre presente ni vide)
	// Les cles doivent etre inserees dans le bon ordre
	void AddKey(const Symbol& sKey);

	// Test de presence d'une cle
	boolean IsKeyPresent(const Symbol& sKey) const;

	// Nombre total de cles
	int GetKeyNumber() const override;

	///////////////////////////////////////////////////////////////////
	// Acces aux cle et a leur index

	// Acces a l'index associe a une cle (-1 si cle absente)
	int GetKeyIndex(const Symbol& sKey) const;

	// Acces aux cles par index (entre 0 et KeyNumber)
	Symbol& GetKeyAt(int nIndex) const;

	////////////////////////////////////////////////////////////////
	// Services divers

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Test
	static void Test();

	///////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////////////
	// Les cles categorielles sont gerees au moyen d'un dictionnaire
	// a cles numerique, efficace en temps de calcul, mais avec une
	// empreinte memoire non negligeable

	// Tableau et dictionnaire des paires (index, cles) contenant des KWKeyIndex
	ObjectArray oaKeyIndexes;
	NumericKeyDictionary nkdKeyIndexes;
};

//////////////////////////////////////////////////////////////////
// Classe KWIndexedNKeyBlock: gestion d'un ensemble de cles indexees
// dans le cas de cles entieres
class KWIndexedNKeyBlock : public KWIndexedKeyBlock
{
public:
	// Constructeur
	KWIndexedNKeyBlock();
	~KWIndexedNKeyBlock();

	// Type de cle utilisee pour les variables du bloc
	int GetVarKeyType() const override;

	// Nettoyage
	void Clean() override;

	// Indexation des cles pour preparer l'acces efficace
	// Erreur en cas de doublons sur les cles
	boolean IndexKeys() override;

	//////////////////////////////////////////////////////////////////
	// Alimentation des cles

	// Ajout d'une cle (entier valide entre MinKey et MaxKey)
	// Les cles doivent etre inserees dans le bon ordre
	void AddKey(int nKey);

	// Test de presence d'une cle, une fois les cle triees
	boolean IsKeyPresent(int nKey) const;

	// Nombre total de cles
	int GetKeyNumber() const override;

	// Valeurr min et max des cles numerique
	static int GetMinKey();
	static int GetMaxKey();

	///////////////////////////////////////////////////////////////////
	// Acces aux cle et a leur index

	// Acces aux cles par index (entre 0 et KeyNumber)
	int GetKeyAt(int nIndex) const;

	// Acces a l'index associe a une cle (-1 si cle absente)
	int GetKeyIndex(int nKey) const;

	////////////////////////////////////////////////////////////////
	// Services divers

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Test
	static void Test();

	///////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// La classe de gestion de vecteurs de valeurs sparse avec des cle entiere a accees a l'implementation
	friend class KWValueSparseVector;

	//////////////////////////////////////////////////////////////////
	// Les cles sont ajoutees une a une, puis une fois indexees,
	// leur index d'insertion est accessible selon le SortedKeyMode,
	// soit en O(log(N)) si l'on passe par une recherche dichotomique
	// soit en O(1) si on utilise un vecteur d'index.
	// Le passage d'un mode a l'autre est automatiquement decide en fonction
	// de contraintes d'occupation memoire
	// La complexite en mode dichotomique est a priori moins interessante
	// qu'avec une table de hashage en O(1), mais la gestion d'entiers long
	// au lieu de symbol et d'objets(Key, Index) est tres avantageuse
	// a la fois en memoire et en temps.
	//
	// Cette fonctionnalite est utile en particulier pour indexer un bloc de
	// valeurs sparses dont les variables sont identifiees par des cles entieres

	// Indexation des cles selon un mode specifie, pour permettre de tester
	// et comparer les deux modes
	// Les cles doivent etre triees prealablement sans doublon
	//  AutomaticMode: determine automatiquement selon des consideration de taille emmoire
	//   SortedKeyMode: on utilise le vecteur des cles, qui est deja trie
	//     . la recherche de l'index d'une cle est effectuee par dichotomie sur la cle
	//   not SortedKeyMode: on utilise un vecteur d'entier de taille la valeur max des cles
	//     . le vecteur est de taille MaxKey+1
	//     . chaque cle occupe la case du vecteur en utilisant la cle comme index
	//     . la recherche de l'index d'une cle est effectuee directement
	//        . on rend l'index si la cle est utilise, -1 sinon
	// On renvoie true si OK, false s'il y avait un probleme d'uncite des cles
	boolean InternalIndexKeys(boolean bAutomaticMode, boolean bSortedKeyMode);

	// Acces par recherche dichotomique a l'index associe a une cle (-1 si cle absente)
	int GetDichotomicKeyIndex(int nKey) const;

	// Calcul de l'occupation memoire qui serait necessaire pour gerer les cles avec un dictionnaire
	longint ComputeDictionaryNecessaryMemory(int nKeyNumber) const;

	// Test selon chaque mode
	static void TestWithMode(boolean bSortedKeyMode);

	// Vecteur des cles entieres
	IntVector ivKeys;

	// Vecteur d'index pour chaque NKey potentielle
	// Pour chaque cles, on memorise son index dans ivKeys si elle est presente, -1 sinon
	IntVector ivKeyIndexes;
};

////////////////////////////////////////////////////////////////////////
// Classe KWKeyIndex: gestion de triplets (index, cle, valeurs)
// Classe de service pour la gestion des bloc sparse (ici: classe ancetre)
class KWKeyIndex : public Object
{
public:
	// Constructeur
	KWKeyIndex();
	~KWKeyIndex();

	// Type de valeur (cf KWType)
	virtual int GetType() const;

	// Cle
	void SetKey(const Symbol& sValue);
	Symbol& GetKey() const;

	// Index
	void SetIndex(int nValue);
	int GetIndex() const;

	// Duplication
	virtual KWKeyIndex* Clone() const;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Affichage (sans saut de ligne)
	void Write(ostream& ost) const override;
	virtual void WriteValue(ostream& ost) const;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	mutable Symbol sKey;
	int nIndex;
};

// Comparaison de deux objets KWKeyIndex par index
int KWKeyIndexCompareIndex(const void* elem1, const void* elem2);

// Comparaison de deux objets KWKeyIndex par valeur de cle
int KWKeyIndexCompareKeyValue(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////////////////
// Classe KWContinuousKeyIndex: specialisation pour les valeurs Continuous
class KWContinuousKeyIndex : public KWKeyIndex
{
public:
	// Constructeur
	KWContinuousKeyIndex();
	~KWContinuousKeyIndex();

	// Type de valeur
	int GetType() const override;

	// Valeur
	void SetValue(Continuous cValue);
	Continuous GetValue() const;

	// Duplication
	KWKeyIndex* Clone() const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Affichage
	void WriteValue(ostream& ost) const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	Continuous cStoredValue;
};

////////////////////////////////////////////////////////////////////////
// Classe KWSymbolKeyIndex: specialisation pour les valeurs Symbol
class KWSymbolKeyIndex : public KWKeyIndex
{
public:
	// Constructeur
	KWSymbolKeyIndex();
	~KWSymbolKeyIndex();

	// Type de valeur
	int GetType() const override;

	// Valeur
	void SetValue(const Symbol& sValue);
	Symbol& GetValue() const;

	// Duplication
	KWKeyIndex* Clone() const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Affichage
	void WriteValue(ostream& ost) const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	mutable Symbol sStoredValue;
};

////////////////////////////////////////////////////////////////////////
// Classe KWSymbolKeyIndex: specialisation pour les valeurs ObjectArray
class KWObjectArrayKeyIndex : public KWKeyIndex
{
public:
	// Constructeur
	KWObjectArrayKeyIndex();
	~KWObjectArrayKeyIndex();

	// Type de valeur
	int GetType() const override;

	// Valeur
	// Memoire: la tableau appartient a l'appele, mais pas son contenu
	ObjectArray* GetValue();

	// Duplication
	KWKeyIndex* Clone() const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Affichage
	void WriteValue(ostream& ost) const override;

	////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	ObjectArray oaStoredValue;
};

///////////////////////////////////////////////////////
// Methodes en inline

// Classe KWIndexedCKeyBlock

inline KWIndexedCKeyBlock::KWIndexedCKeyBlock() {}

inline boolean KWIndexedCKeyBlock::IsKeyPresent(const Symbol& sKey) const
{
	return nkdKeyIndexes.Lookup(sKey.GetNumericKey()) != NULL;
}

inline int KWIndexedCKeyBlock::GetKeyNumber() const
{
	return oaKeyIndexes.GetSize();
}

inline int KWIndexedCKeyBlock::GetKeyIndex(const Symbol& sKey) const
{
	KWKeyIndex* keyIndex;

	keyIndex = cast(KWKeyIndex*, nkdKeyIndexes.Lookup(sKey.GetNumericKey()));
	if (keyIndex == NULL)
		return -1;
	else
	{
		assert(keyIndex->GetKey() == sKey);
		return keyIndex->GetIndex();
	}
}

inline Symbol& KWIndexedCKeyBlock::GetKeyAt(int nIndex) const
{
	KWKeyIndex* keyIndex;

	require(0 <= nIndex and nIndex < GetKeyNumber());

	keyIndex = cast(KWKeyIndex*, oaKeyIndexes.GetAt(nIndex));
	return keyIndex->GetKey();
}

// Classe KWIndexedNKeyBlock

inline KWIndexedNKeyBlock::KWIndexedNKeyBlock() {}

inline KWIndexedNKeyBlock::~KWIndexedNKeyBlock() {}

inline void KWIndexedNKeyBlock::AddKey(int nKey)
{
	require(GetMinKey() <= nKey and nKey <= GetMaxKey());
	require(ivKeys.GetSize() == 0 or nKey > ivKeys.GetAt(ivKeys.GetSize() - 1));
	ivKeys.Add(nKey);
}

inline boolean KWIndexedNKeyBlock::IsKeyPresent(int nKey) const
{
	return (GetKeyIndex(nKey) != -1);
}

inline int KWIndexedNKeyBlock::GetKeyNumber() const
{
	return ivKeys.GetSize();
}

inline int KWIndexedNKeyBlock::GetMinKey()
{
	return 1;
}

inline int KWIndexedNKeyBlock::GetMaxKey()
{
	return 1000000000;
}

inline int KWIndexedNKeyBlock::GetKeyAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetKeyNumber());
	return ivKeys.GetAt(nIndex);
}

inline boolean KWIndexedNKeyBlock::IndexKeys()
{
	return InternalIndexKeys(true, false);
}

inline int KWIndexedNKeyBlock::GetKeyIndex(int nKey) const
{
	require(nKey >= 1);

	// Cas d'une liste triee des index
	if (ivKeyIndexes.GetSize() == 0)
	{
		return GetDichotomicKeyIndex(nKey);
	}
	// Cas par test de presence de la cle
	else
	{
		if (nKey < ivKeyIndexes.GetSize())
		{
			assert(ivKeyIndexes.GetAt(nKey) == -1 or ivKeys.GetAt(ivKeyIndexes.GetAt(nKey)) == nKey);
			assert(GetDichotomicKeyIndex(nKey) == ivKeyIndexes.GetAt(nKey));
			return ivKeyIndexes.GetAt(nKey);
		}
		else
		{
			assert(GetDichotomicKeyIndex(nKey) == -1);
			return -1;
		}
	}
}

// Classe KWKeyIndex

inline KWKeyIndex::KWKeyIndex()
{
	nIndex = 0;
}

inline KWKeyIndex::~KWKeyIndex() {}

inline void KWKeyIndex::SetKey(const Symbol& sValue)
{
	sKey = sValue;
}

inline Symbol& KWKeyIndex::GetKey() const
{
	return sKey;
}

inline void KWKeyIndex::SetIndex(int nValue)
{
	nIndex = nValue;
}

inline int KWKeyIndex::GetIndex() const
{
	return nIndex;
}

// Classe KWContinuousKeyIndex

inline KWContinuousKeyIndex::KWContinuousKeyIndex()
{
	cStoredValue = 0;
}

inline KWContinuousKeyIndex::~KWContinuousKeyIndex() {}

inline void KWContinuousKeyIndex::SetValue(Continuous cValue)
{
	cStoredValue = cValue;
}

inline Continuous KWContinuousKeyIndex::GetValue() const
{
	return cStoredValue;
}

// Classe KWSymbolKeyIndex

inline KWSymbolKeyIndex::KWSymbolKeyIndex() {}

inline KWSymbolKeyIndex::~KWSymbolKeyIndex() {}

inline void KWSymbolKeyIndex::SetValue(const Symbol& sValue)
{
	sStoredValue = sValue;
}

inline Symbol& KWSymbolKeyIndex::GetValue() const
{
	return sStoredValue;
}

// Classe KWObjectArrayKeyIndex

inline KWObjectArrayKeyIndex::KWObjectArrayKeyIndex() {}

inline KWObjectArrayKeyIndex::~KWObjectArrayKeyIndex() {}

inline ObjectArray* KWObjectArrayKeyIndex::GetValue()
{
	return &oaStoredValue;
}