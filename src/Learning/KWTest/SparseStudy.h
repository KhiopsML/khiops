// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWSymbol.h"
#include "Object.h"
#include "Vector.h"
#include "Timer.h"
#include "KWType.h"

//////////////////////////////////////////////////////////////////
// Classe de gestion des index de chargement, avec version dense et sparse
// Classe interne, geree comme un type simple depuis l'exterieur,
// avec des services avances uniquement pour les classe devant gerer
// l'acces indexe a des valeurs denses ou sparse
class LoadIndex : public SystemObject
{
private:
	// Creation et reinitialisation, dans un etat invalide
	LoadIndex();
	void Reset();

	// Acces a l'index de donnee dense
	void SetDenseIndex(int nDenseIndex);
	int GetDenseIndex() const;

	// Acces a l'index de donnee sparse
	void SetSparseIndex(int nSparseIndex);
	int GetSparseIndex() const;

	// Test si l'index correspond a une valeur valide, dense ou sparse
	boolean IsValid() const;
	boolean IsDense() const;
	boolean IsSparse() const;

	// Utilisation en temps qu'entier long
	LoadIndex& operator=(const longint lValue);
	operator const longint() const;

	// Utilisation en temps qu'entier interdite par des assertions
	LoadIndex& operator=(const int nValue);
	operator const int() const;

	// Fonction et classes amies, ayant acces aux services
	friend ostream& operator<<(ostream& ost, const LoadIndex& value);
	friend class KWAttribute;
	friend class KWDerivationRuleOperand;
	friend class SparseStudy;

	// Donnees de gestion optimise d'un index
	// L'index complet est compose d'un index dense (-1) si invalide, permettant
	// d'acceder a une donnee dense (type de donnees dense, ou bloc de donnees sparse).
	// Si la donnees est sparse, l'index sparse permet d'acceder a la donnee sparse
	// au sein de son bloc
	// L'index complet (lFull) permet de traiter les index dense et sparse en une
	// seule operation
	union
	{
		longint lFullIndex;
		struct
		{
			int nDenseIndex;
			int nSparseIndex;
		} IndexParts;
	};
};

// Ecriture dans un stream
inline ostream& operator<<(ostream& ost, const LoadIndex& value)
{
	ost << "(" << value.GetDenseIndex() << ", " << value.GetSparseIndex() << ")";
	return ost;
}

// Classe d'etude sur l'implementation de donnees sparse
class SparseStudy : public Object
{
public:
	// Constructeur
	SparseStudy();
	~SparseStudy();

	// Affichage
	void Write(ostream& ost) const override;

	// Methode de test
	static void Test();

	/////////////////////////////////
	///// Implementation
	double GetValue() const;
	double GetDenseValue() const;
	double GetSparseValue() const;

	DoubleVector dvValues;
	int nIndex;
	longint lIndex;
	LoadIndex index;
};

void BasicVectorAccessTest();
void DoubleVectorAccessTest();

////////////////////////////////////////////////////////
// Classe KWValueIndexPair
// Class technique, utilisee uniquement pour l'implementation des blocs sparse
// Pas de "packing" des attribut pour optimiser la place memoire
#ifdef _WIN32
#pragma pack(push)
#pragma pack(1)
class KWValueIndexPairTest : public SystemObject
{
	KWValue value;
	int nIndex;
	friend class KWContinuousValueBlockTest;
};
#pragma pack(pop)
#else // Linux
class KWValueIndexPairTest : public SystemObject
{
	KWValue value;
	int nIndex;
	friend class KWContinuousValueBlockTest;
} __attribute__((packed));
#endif

/////////////////////////////////////////////////////////////////////
// Classe KWContinuousValueBlock: gestion d'un bloc de valeurs sparse
// de type Continuous
// Class optimisee pour une empreinte memoire minimale
// On n'herite pas de Object, ce qui empeche l'utilisation des objets de
// cette classe dans des contianer de type ObjectArray.
// Les blocs de valeurs ne sont pas retaillable: la taille est decide
// une fois pour toutes lors de la creation de l'objet
class KWContinuousValueBlockTest : public SystemObject
{
public:
	// Creation d'un bloc de taille donnees
	// Methode statique a utiliser pour la creation d'un bloc de taille fixe donnee
	static KWContinuousValueBlockTest* NewValueBlock(int nSize);
	~KWContinuousValueBlockTest();

	// Nombre de valeurs disponibles
	int GetValueNumber() const;

	///////////////////////////////////////////////////////////////////
	// Acces aux valeurs du bloc, de la premiere a la derniere
	// (entre 0 et ValueNumber)

	// Acces a l'index de chargement sparse de la variable associee a un index de valeur
	int GetAttributeSparseIndexAt(int nValueIndex) const;
	void SetAttributeSparseIndexAt(int nValueIndex, int nSparseIndex);

	// Acces a une valeur par son index
	Continuous GetValueAt(int nValueIndex) const;
	void SetValueAt(int nValueIndex, Continuous cValue);

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const;

	// Test
	static void TestPerformance();

	//////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Constantes specifiques a la classe (permet une compilation optimisee pour les methodes SetAt et GetAt)
	static const int nElementSize = (int)sizeof(KWValueIndexPairTest);
	static const int nSegmentSize = (int)((MemSegmentByteSize - sizeof(int)) / nElementSize);

	// Constructeur/destructeur par defaut
	KWContinuousValueBlockTest();

	// Nombre de valeurs du bloc
	int nValueNumber;

	// Caractere dont l'adresse permettra de trouver le debut du bloc
	// En effet, 'ors de l'allocation, on allouera la taille necessaire pour stocker
	// un entier (nValueNumber), ainsi que le nombres de paires (value, index)
	// (KWValueIndexPair) du bloc
	char cStartBlock;
};

inline KWContinuousValueBlockTest::KWContinuousValueBlockTest()
{
	cStartBlock = '\0';
}
inline int KWContinuousValueBlockTest::GetValueNumber() const
{
	return nValueNumber;
}
inline int KWContinuousValueBlockTest::GetAttributeSparseIndexAt(int nValueIndex) const
{
	require(0 <= nValueIndex and nValueIndex < nValueNumber);
	return (nValueNumber <= nSegmentSize
		    ? ((KWValueIndexPairTest*)&cStartBlock)[nValueIndex].nIndex
		    : ((KWValueIndexPairTest**)&cStartBlock)[nValueIndex / nSegmentSize][nValueIndex % nSegmentSize]
			  .nIndex);
}
inline void KWContinuousValueBlockTest::SetAttributeSparseIndexAt(int nValueIndex, int nSparseIndex)
{
	require(0 <= nValueIndex and nValueIndex < nValueNumber);
	require(1 <= nSparseIndex and nSparseIndex <= 1000000000);
	if (nValueNumber <= nSegmentSize)
		((KWValueIndexPairTest*)&cStartBlock)[nValueIndex].nIndex = nSparseIndex;
	else
		((KWValueIndexPairTest**)&cStartBlock)[nValueIndex / nSegmentSize][nValueIndex % nSegmentSize].nIndex =
		    nSparseIndex;
}
inline Continuous KWContinuousValueBlockTest::GetValueAt(int nValueIndex) const
{
	require(0 <= nValueIndex and nValueIndex < nValueNumber);
	return (nValueNumber <= nSegmentSize
		    ? ((KWValueIndexPairTest*)&cStartBlock)[nValueIndex].value.GetContinuous()
		    : ((KWValueIndexPairTest**)&cStartBlock)[nValueIndex / nSegmentSize][nValueIndex % nSegmentSize]
			  .value.GetContinuous());
}
inline void KWContinuousValueBlockTest::SetValueAt(int nValueIndex, Continuous cValue)
{
	require(0 <= nValueIndex and nValueIndex < nValueNumber);
	if (nValueNumber <= nSegmentSize)
		((KWValueIndexPairTest*)&cStartBlock)[nValueIndex].value.SetContinuous(cValue);
	else
		((KWValueIndexPairTest**)&cStartBlock)[nValueIndex / nSegmentSize][nValueIndex % nSegmentSize]
		    .value.SetContinuous(cValue);
}

/////////////////////////////////////////////////////////
// Class KWIndexedValue
// Classe technique pour des paire (index, valeur)
// permettant un implementation optimisee des classe KWValueBlock
#pragma pack(push)
#pragma pack(1)
class KWIndexedValueTest : public SystemObject
{
	KWValue value;
	int nIndex;
};
// __attribute__((packed));
#pragma pack(pop)

// Methodes en inline de LoadIndex

inline LoadIndex::LoadIndex()
{
	lFullIndex = -1;
	ensure(not IsValid() and not IsSparse());
}

inline void LoadIndex::Reset()
{
	lFullIndex = -1;
	ensure(not IsValid() and not IsSparse());
}

inline void LoadIndex::SetDenseIndex(int nDenseIndex)
{
	require(nDenseIndex >= -1);
	IndexParts.nDenseIndex = nDenseIndex;
}

inline int LoadIndex::GetDenseIndex() const
{
	return IndexParts.nDenseIndex;
}

inline void LoadIndex::SetSparseIndex(int nSparseIndex)
{
	require(nSparseIndex >= -1);
	IndexParts.nSparseIndex = nSparseIndex;
}

inline int LoadIndex::GetSparseIndex() const
{
	return IndexParts.nSparseIndex;
}

inline boolean LoadIndex::IsValid() const
{
	return IndexParts.nDenseIndex != -1;
}

inline boolean LoadIndex::IsDense() const
{
	return IndexParts.nSparseIndex == -1;
}

inline boolean LoadIndex::IsSparse() const
{
	return IndexParts.nSparseIndex != -1;
}

inline LoadIndex& LoadIndex::operator=(const longint lValue)
{
	lFullIndex = lValue;
	return *this;
}

inline LoadIndex::operator const longint() const
{
	return lFullIndex;
}

inline LoadIndex& LoadIndex::operator=(const int nValue)
{
	assert(false);
	IndexParts.nDenseIndex = nValue;
	return *this;
}

inline LoadIndex::operator const int() const
{
	assert(false);
	return IndexParts.nDenseIndex;
}

// Methodes en inline de SparseStudy

inline double SparseStudy::GetValue() const
{
	require(index.IsValid());
	return (index.IsDense() ? GetDenseValue() : GetSparseValue());
}

inline double SparseStudy::GetDenseValue() const
{
	require(index.IsValid() and index.IsDense());
	return dvValues.GetAt(index.GetDenseIndex() % dvValues.GetSize());
}

inline double SparseStudy::GetSparseValue() const
{
	require(index.IsValid() and index.IsSparse());
	return dvValues.GetAt(index.GetDenseIndex() % dvValues.GetSize()) +
	       sqrt(dvValues.GetAt(index.GetSparseIndex() % dvValues.GetSize()));
}
