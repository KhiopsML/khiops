// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWObjectKey;
class KWObject;

#include "KWSymbol.h"
#include "KWKey.h"

//////////////////////////////////////////////////////////
// Classe KWObjectKey
// Champs cles d'un KWObject
class KWObjectKey : public Object
{
public:
	// Constructeur
	KWObjectKey();
	~KWObjectKey();

	// Taille
	// Lors d'un retaillage, la partie commune entre l'ancien et le nouveau
	// vecteur est preservee, la partie supplementaire est initialisee a '0'
	void SetSize(int nValue);
	int GetSize() const;
	boolean IsEmpty() const;

	// (Re)initialisation des champs existants
	void Initialize();

	// Initialisation a partir d'un objet
	// Prerequis: l'objet doit etre valide et les champs de sa cle doivent etre charges en memoire
	void InitializeFromObject(const KWObject* kwoObject);

	// Acces aux elements du vecteur
	void SetAt(int nIndex, const Symbol& sValue);
	Symbol& GetAt(int nIndex) const;

	// Copie a partir d'une cle source
	// (retaillage si necessaire)
	void CopyFrom(const KWObjectKey* kwokSource);

	// Duplication
	KWObjectKey* Clone() const;

	// Comparaison portant sur la valeur des champs de la cle, de facon lexicographique
	int Compare(const KWObjectKey* kwokKey) const;

	// Comparaison stricte pour deux cle devant avoir le meme nombre de champs
	int StrictCompare(const KWObjectKey* kwokKey) const;

	// Comparaison uniquement sur le nombre de champ de la cle courante, la deuxieme cle
	// en parametre devant avoir au moins le meme nombre de champs
	int SubCompare(const KWObjectKey* kwokKey) const;

	// Affichage
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Construction de libelles utilisateurs de deux cles garantis distincts l'un de l'autre
	void BuildDistinctObjectLabels(const KWObjectKey* otherObjectKey, ALString& sObjectLabel,
				       ALString& sOtherObjectLabel);

	// Test de la classe
	static void Test();

	//////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Creation et destruction du vecteur de Symbol
	// Ces methodes gerent correctement les references aux Symbol
	void NewKeyFields(int nInitialisationSize);
	void DeleteKeyFields();

	// On utilise directement un vecteur de Symbol, plutot qu'un SymbolVector pour des raisons d'optimisation
	// En effet, une cle ne sera jamais de grande taille et n'a pas besoin d'etre retaillable
	int nSize;
	Symbol* sKeyFields;
};

#include "KWObject.h"

// Fonction de comparaison de deux KWObject base sur leur cle
int KWObjectCompareKey(const void* elem1, const void* elem2);

// Implementations en inline

inline KWObjectKey::KWObjectKey()
{
	nSize = 0;
	sKeyFields = NULL;
}

inline int KWObjectKey::GetSize() const
{
	return nSize;
}

inline boolean KWObjectKey::IsEmpty() const
{
	return nSize == 0;
}

inline void KWObjectKey::SetAt(int nIndex, const Symbol& sValue)
{
	require(0 <= nIndex and nIndex < GetSize());
	sKeyFields[nIndex] = sValue;
}

inline Symbol& KWObjectKey::GetAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetSize());
	return sKeyFields[nIndex];
}
