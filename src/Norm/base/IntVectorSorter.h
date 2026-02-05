// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class IntPairVector;
class IntVectorSorter;

#include "Vector.h"

//////////////////////////////////////////////////////////
// Classe IntPairVector
// Vecteur de paires d'entiers, de taille quelconque.
// Sous classe de LongintVector, ou les valeurs du vecteur peuvent
// etre considerees soit comme de longint, soit comme des paires d'entier
// Utile pour l'implementationoptimisee de la classe KWIntVectorSorter
class IntPairVector : public LongintVector
{
public:
	// Constructeur
	IntPairVector();
	~IntPairVector();

	// Acces aux elements du vecteur
	void SetPairAt(int nIndex, int nValue1, int nValue2);
	int GetValue1At(int nIndex) const;
	int GetValue2At(int nIndex) const;

	// Ajout d'un element en fin et retaillage du vecteur
	void AddPair(int nValue1, int nValue2);

	// Tri des valeurs par ordre croissant, pour chaque entier de la pair
	void SortByValue1();
	void SortByValue2();

	// Affichage
	void Write(ostream& ost) const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Conversion entre longint et pair d'entiers
	static longint IntPairToLongint(int nValue1, int nValue2);
	static int LongintToInt1(longint lValue);
	static int LongintToInt2(longint lValue);

	// Test
	static void Test();
};

///////////////////////////////////////////////////////////////////////
// Classe IntVectorSorter
// Service de tri d'un vecteur d'entiers, permettant de conserver
// la correspondance entre les valeurs triees et leurs index initiaux
// Cette classe dedieee aux vecteur d'entier est optimise en temp et memoire
class IntVectorSorter : public Object
{
public:
	// Constructeur
	IntVectorSorter();
	~IntVectorSorter();

	// Tri d'un vecteur
	void SortVector(const IntVector* ivInputVector);

	////////////////////////////////////////
	// Acces aux resultats du tri

	// Taille du vecteur trie
	int GetSize() const;

	// Acces aux valeur triees
	int GetSortedValueAt(int nIndex) const;

	// Acces aux index initiaux des valeurs triees
	int GetInitialIndexAt(int nIndex) const;

	// Recherche de l'index initial correspondant a une valeur
	// Renvoie -1 si non trouve
	int LookupInitialIndex(int nValue) const;

	// Test si les valeurs sont uniques
	boolean CheckUniqueValues() const;

	///////////////////////////////////////////
	// Service divers

	// Nettoyage des resultats
	void Clean();

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	///////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Implementation optimisee au moyen d'un vecteur de longint contenant
	// des paires (valeur, index initial)
	// Cela permet une implementation efficace, a la fois en evitant de creer
	// un objet par paire et avec uin tri rapide

	// Vecteur des paires (cle, index) trie par cle
	IntPairVector ipvSortedValueIndexPairs;
};

////////////////////////////////////////////////////////////////////////////
// Methodes en inline

// Classe IntPairVector

inline IntPairVector::IntPairVector() {}

inline IntPairVector::~IntPairVector() {}

inline void IntPairVector::SetPairAt(int nIndex, int nValue1, int nValue2)
{
	LongintVector::SetAt(nIndex, IntPairToLongint(nValue1, nValue2));
}

inline int IntPairVector::GetValue1At(int nIndex) const
{
	return LongintToInt1(LongintVector::GetAt(nIndex));
}

inline int IntPairVector::GetValue2At(int nIndex) const
{
	return LongintToInt2(LongintVector::GetAt(nIndex));
}

inline void IntPairVector::AddPair(int nValue1, int nValue2)
{
	LongintVector::Add(IntPairToLongint(nValue1, nValue2));
}

#if defined NDEBUG && defined __GNUC__ && !defined __clang__
// pour GCC, en release, on force la compilation en O0 car il ya un bug de gcc (version  4.4.7 sur Centos 6) qui
// optimise un peu trop...
#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ <= 5) // Test si GCC <= 4.5
#pragma GCC push_options
#pragma GCC optimize("O0")
#endif //__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ <=5)
#endif // defined NDEBUG && defined __GNUC__ && !defined __clang__

inline longint IntPairVector::IntPairToLongint(int nValue1, int nValue2)
{
	longint lPair;
	((int*)&lPair)[0] = nValue1;
	((int*)&lPair)[1] = nValue2;
	DISABLE_WARNING_PUSH
	DISABLE_WARNING_UNINITIALIZED
	return lPair;
	DISABLE_WARNING_POP
}

inline int IntPairVector::LongintToInt1(longint lValue)
{
	return ((int*)&lValue)[0];
}

inline int IntPairVector::LongintToInt2(longint lValue)
{
	return ((int*)&lValue)[1];
}

#if defined NDEBUG && defined __GNUC__ && !defined __clang__
#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ <= 5)
#pragma GCC pop_options
#endif //__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ <=5)
#endif // defined NDEBUG && defined __GNUC__ && !defined __clang__

// Classe IntVectorSorter

inline IntVectorSorter::IntVectorSorter()
{
	assert(sizeof(longint) == 2 * sizeof(int));
}

inline IntVectorSorter::~IntVectorSorter() {}

inline int IntVectorSorter::GetSize() const
{
	return ipvSortedValueIndexPairs.GetSize();
}

inline int IntVectorSorter::GetSortedValueAt(int nIndex) const
{
	return ipvSortedValueIndexPairs.GetValue1At(nIndex);
}

inline int IntVectorSorter::GetInitialIndexAt(int nIndex) const
{
	return ipvSortedValueIndexPairs.GetValue2At(nIndex);
}
