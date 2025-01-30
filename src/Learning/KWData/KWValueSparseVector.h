// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWValueSparseVector;
class KWContinuousValueSparseVector;
class KWSymbolValueSparseVector;
class KWObjectArrayValueSparseVector;

#include "KWSymbol.h"
#include "KWContinuous.h"
#include "KWType.h"
#include "Object.h"
#include "Vector.h"
#include "KWIndexedKeyBlock.h"

//////////////////////////////////////////////////////////////////
// Classe KWValueSparseVector
// Classe virtuelle, ancetre des classes specialisee par type de valeur
// Vecteur de paires (SparseIndex, valeur)
// Attention, il n'y a pas de garantie de l'unicite ni de l'ordre des SparseIndex
// Utile en particulier pour preparer un bloc de valeurs sparses
class KWValueSparseVector : public Object
{
public:
	// Constructeur
	KWValueSparseVector();
	~KWValueSparseVector();

	// Type de valeur
	virtual int GetType() const = 0;

	// Supression de toutes les valeurs
	virtual void RemoveAll();

	// Nombre de paires (cle, valeurs)
	int GetValueNumber() const;

	// Acces aux index sparse
	int GetSparseIndexAt(int nIndex) const;

	////////////////////////////////////////////////////////////////
	// Services divers

	// Affichage
	void Write(ostream& ost) const override;

	// Affichage d'une valeur
	// A redefinir dans chaque sous-classe
	virtual void WriteValueAt(ostream& ost, int nIndex) const;

	// Methode avance d'acces au vecteur d'index sparse
	const IntVector* GetSparseIndexVector() const;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	///////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Vecteur des index sparse
	IntVector ivSparseIndexes;
};

//////////////////////////////////////////////////////////////////
// Classe KWContinuousValueSparseVector
// Dictionnaire dediee aux valeurs Continuous
class KWContinuousValueSparseVector : public KWValueSparseVector
{
public:
	// Constructeur
	KWContinuousValueSparseVector();
	~KWContinuousValueSparseVector();

	// Type de valeur
	int GetType() const override;

	// Supression de toutes les valeurs
	void RemoveAll() override;

	////////////////////////////////////////////////////////////////
	// Alimentation et acces aux valeurs

	// Ajout d'une paire (index sparse, valeur)
	void AddValueAt(int nSparseIndex, Continuous cValue);

	// Acces aux valeurs par index (entre 0 et le nombre de valeurs)
	Continuous GetValueAt(int nIndex) const;

	////////////////////////////////////////////////////////////////
	// Services divers

	// Affichage d'une valeur
	void WriteValueAt(ostream& ost, int nIndex) const override;

	// Test
	static void Test();

	///////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Vecteur des valeurs associees aux cle
	ContinuousVector cvValues;
};

//////////////////////////////////////////////////////////////////
// Classe KWSymbolValueSparseVector
// Dictionnaire dediee aux valeurs Symbol
class KWSymbolValueSparseVector : public KWValueSparseVector
{
public:
	// Constructeur
	KWSymbolValueSparseVector();
	~KWSymbolValueSparseVector();

	// Type de valeur
	int GetType() const override;

	// Supression de toutes les valeurs
	void RemoveAll() override;

	////////////////////////////////////////////////////////////////
	// Alimentation et acces aux valeurs

	// Ajout d'une paire (index sparse, valeur)
	void AddValueAt(int nSparseIndex, Symbol sValue);

	// Acces aux valeurs par index (entre 0 et le nombre de valeurs)
	Symbol& GetValueAt(int nIndex) const;

	////////////////////////////////////////////////////////////////
	// Services divers

	// Affichage d'une valeur
	void WriteValueAt(ostream& ost, int nIndex) const override;

	// Test
	static void Test();

	///////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Vecteur des valeurs associees aux cle
	SymbolVector svValues;
};

//////////////////////////////////////////////////////////////////
// Classe KWObjectArrayValueSparseVector
// Dictionnaire dediee aux valeurs ObjectArray
class KWObjectArrayValueSparseVector : public KWValueSparseVector
{
public:
	// Constructeur
	KWObjectArrayValueSparseVector();
	~KWObjectArrayValueSparseVector();

	// Type de valeur
	int GetType() const override;

	// Supression de toutes les valeurs
	void RemoveAll() override;

	////////////////////////////////////////////////////////////////
	// Alimentation et acces aux valeurs

	// Ajout d'une paire (index sparse, valeur)
	void AddValueAt(int nSparseIndex, ObjectArray* oaValue);

	// Acces aux valeurs par index (entre 0 et le nombre de valeurs)
	ObjectArray* GetValueAt(int nIndex) const;

	////////////////////////////////////////////////////////////////
	// Services divers

	// Destruction de toutes les valeurs et des tableaux indexes
	void DeleteAll();

	// Affichage d'une valeur
	void WriteValueAt(ostream& ost, int nIndex) const override;

	// Test
	static void Test();

	///////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Vecteur des valeurs associees aux cle
	ObjectArray oaValues;
};

/////////////////////////////////
// Methodes en inline

inline int KWValueSparseVector::GetValueNumber() const
{
	return ivSparseIndexes.GetSize();
}

inline int KWValueSparseVector::GetSparseIndexAt(int nIndex) const
{
	return ivSparseIndexes.GetAt(nIndex);
}

inline Continuous KWContinuousValueSparseVector::GetValueAt(int nIndex) const
{
	return cvValues.GetAt(nIndex);
}

inline Symbol& KWSymbolValueSparseVector::GetValueAt(int nIndex) const
{
	return svValues.GetAt(nIndex);
}

inline ObjectArray* KWObjectArrayValueSparseVector::GetValueAt(int nIndex) const
{
	return cast(ObjectArray*, oaValues.GetAt(nIndex));
}
