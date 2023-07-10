// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Standard.h"

//////////////////////////////////////////////////////////
// Bibliotheque de services generiques sur les vecteurs de
// tres grande taille
// Services techniques destines uniquement a l'implementation
// efficace des vecteurs et tableaux, non a l'utilisation
// directe dans des clasdses utilisateur

// Union pour le stockage d'un vecteur de tres grande taille
// Dans le cas des petites tailles (inferieur a MemSegmentByteSize),
// les valeurs sont stockees dans un seul block (pValues)
// Dans le cas des grandes tailles, les valeurs sont stockees dans un tableau
// de blocks (pValueBlocks)
union MemHugeVector
{
	char* pValues;
	char** pValueBlocks;
};

// Prototype des fonctions de comparaisons
typedef int (*MemVectorCompareFunction)(const void* first, const void* second);

//////////////////////////////////////////////////////////
// Classe MemVector
// Bibliotheque de methodes statiques travaillant sur
// les vecteurs de tres grande taille
class MemVector : public SystemObject
{
public:
	/////////////////////////////////////////////////////////////////////////////////////
	// Methodes generiques sur les vecteurs de grandes tailles
	// Ces methodes prennent en parametres les caracteristiques generiques des
	// vecteur de grande taille:
	//           MemHugeVector: tableau d'elements ou tableau de tableau d'elements (union)
	//           Size: nombre d'element effectifs
	//           AllocSize: taille allouee
	//           BlockSize: taille d'un tableau d'element
	//           ElementSize: taille d'un element
	// On passe d'un tableau d'element a un tableau de tableau de tableau d'element des
	// que la taille allouee depasse la taille (fixe) d'un tableau d'elements)
	//
	// Ces methodes sont implementee en static et utilisable comme les fonctions de type
	// memcpy ou memset de la bibliotheque C ansi.
	//
	// Les classes vecteurs de type IntVector declarent l'ensemble complet des methodes publiques
	// pour beneficier d'une interface lisible, utilisent les methodes generiques de MemVector pour
	// l'implementation des methodes complexes, ou beneficient d'implementation specifiques pour des raisons
	// d'optimisation (notamment, pour l'acces direct aux elements du tableaux, que le compilateur
	// peut particulierement optimiser quand il connait en dur la taille des elements).

	// Creation du vecteur
	static void Create(MemHugeVector& memHugeVector, int& nSize, int& nAllocSize, const int nBlockSize,
			   const int nElementSize);

	// Destruction du vecteur
	static void Delete(MemHugeVector& memHugeVector, int& nSize, int& nAllocSize, const int nBlockSize,
			   const int nElementSize);

	// Retaillage du vecteur
	static void SetSize(MemHugeVector& memHugeVector, int& nSize, int& nAllocSize, const int nBlockSize,
			    const int nElementSize, int nNewSize);

	// Initialisation a zero
	static void Initialize(MemHugeVector& memHugeVector, int& nSize, int& nAllocSize, const int nBlockSize,
			       const int nElementSize);

	// Tri
	static void Sort(MemHugeVector& memHugeVector, int& nSize, int& nAllocSize, const int nBlockSize,
			 const int nElementSize, MemVectorCompareFunction fCompareFunction);

	// Copie a partir d'un vecteur source
	// (retaillage si necessaire)
	static void CopyFrom(MemHugeVector& memHugeVector, int& nSize, int& nAllocSize, const int nBlockSize,
			     const int nElementSize, const MemHugeVector& memSourceHugeVector, const int& nSourceSize,
			     const int& nSourceAllocSize);

	// Import de nElementNumber elements du tableau cByteBuffer vers l'index nIndex du MemHugeVector
	static void ImportBuffer(MemHugeVector& memHugeVector, int nSize, int nAllocSize, int nBlockSize,
				 int nElementSize, int nIndex, int nElementNumber, const char* cByteBuffer);

	// Export de nElementNumber elements vers le tableau cByteBuffer depuis l'index nIndex du MemHugeVector
	// Le tableau cByteBuffer doit etre alloue et de taille nElementNumber*nElementSize
	static void ExportBuffer(const MemHugeVector& memHugeVector, int nSize, int nAllocSize, int nBlockSize,
				 int nElementSize, int nIndex, int nElementNumber, char* cByteBuffer);

	// Retaillage avec potentiellement une grande taille, sans risque d'erreur d'allocation (a appeler par
	// SetLargeSize) Renvoie false si echec de retaillage (et le vecteur garde sa taille initiale)
	static boolean SetLargeSize(MemHugeVector& memHugeVector, int& nSize, int& nAllocSize, const int nBlockSize,
				    const int nElementSize, int nNewSize);

	// Verification de coherence
	static boolean Check(const MemHugeVector& memHugeVector, const int& nSize, const int& nAllocSize,
			     const int nBlockSize, const int nElementSize);
};

//////////////////////////////////////////////////////////
// Classe PointerVector
// Vecteur de pointers, de taille quelconque.
// Les acces aux vecteurs sont controles par assertions.
// Attention, cette classe technique est a usage interne
// uniquement pour l'implementation efficace des
// classes de la librairie
// (elle n'herite pas de la classe Object)
class PointerVector : public SystemObject
{
public:
	// Constructeur
	PointerVector();
	~PointerVector();

	// Taille
	// Lors d'un retaillage, la partie commune entre l'ancien et le nouveau
	// vecteur est preservee, la partie supplementaire est initialisee a '0'
	void SetSize(int nValue);
	int GetSize() const;

	// (Re)initialisation a 0
	void Initialize();

	// Acces aux elements du vecteur
	void SetAt(int nIndex, void* pValue);
	void* GetAt(int nIndex) const;

	// Ajout d'un element en fin et retaillage du vecteur
	void Add(void* pValue);

	// Echange du contenu avec un vecteur source
	// A l'issue de cette methode (technique), les contenus des deux vecteurs
	// on ete interverti, de maniere tres efficace
	void SwapFrom(PointerVector* pvSource);

	// Copie a partir d'un vecteur source
	// (retaillage si necessaire)
	void CopyFrom(const PointerVector* pvSource);

	// Duplication
	PointerVector* Clone() const;

	// Retaillage avec potentiellement une grande taille, sans risque d'erreur d'allocation
	// Renvoie false si echec de retaillage (et le vecteur garde sa taille initiale)
	boolean SetLargeSize(int nValue);

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const;

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Constantes specifiques a la classe (permet une compilation optimisee pour les methodes SetAt et GetAt)
	static const int nElementSize = (int)sizeof(void*);
	static const int nBlockSize = (int)((MemSegmentByteSize) / nElementSize);

	// Taille effective et taille allouee
	int nSize;
	int nAllocSize;

	// Donnees: soit directement un block de valeurs, soit un tableau de blocks
	// selon la taille allouee, soit un vecteur de grande taille generique pour beneficier
	// des methodes de MemVector
	union
	{
		void** pValues;
		void*** pValueBlocks;
		MemHugeVector hugeVector;
	} pData;
};

/////////////////////////////
// Implementation en inline

inline void MemVector::Create(MemHugeVector& memHugeVector, int& nSize, int& nAllocSize, const int nBlockSize,
			      const int nElementSize)
{
	memHugeVector.pValues = NULL;
	nSize = 0;
	nAllocSize = 0;
	ensure(Check(memHugeVector, nSize, nAllocSize, nBlockSize, nElementSize));
}

// Classe PointerVector

inline PointerVector::PointerVector()
{
	MemVector::Create(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

inline PointerVector::~PointerVector()
{
	MemVector::Delete(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

inline void PointerVector::SetSize(int nValue)
{
	MemVector::SetSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);
}

inline int PointerVector::GetSize() const
{
	return nSize;
}

inline void PointerVector::Initialize()
{
	MemVector::Initialize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

inline void PointerVector::SetAt(int nIndex, void* pValue)
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		pData.pValues[nIndex] = pValue;
	else
		(pData.pValueBlocks[nIndex / nBlockSize])[nIndex % nBlockSize] = pValue;
}

inline void* PointerVector::GetAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		return pData.pValues[nIndex];
	else
		return (pData.pValueBlocks[nIndex / nBlockSize])[nIndex % nBlockSize];
}

inline void PointerVector::Add(void* pValue)
{
	if (nSize < nAllocSize)
		nSize++;
	else
		SetSize(nSize + 1);
	SetAt(nSize - 1, pValue);
}
