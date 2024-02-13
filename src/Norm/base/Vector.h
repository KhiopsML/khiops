// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "ALString.h"
#include "MemVector.h"

class DoubleVector;
class IntVector;
class LongintVector;
class StringVector;

//////////////////////////////////////////////////////////
// Classe DoubleVector
// Vecteur de reel, de taille quelconque.
// Les acces aux vecteurs sont controles par assertions.
class DoubleVector : public Object
{
public:
	// Constructeur
	DoubleVector();
	~DoubleVector();

	// Taille
	// Lors d'un retaillage, la partie commune entre l'ancien et le nouveau
	// vecteur est preservee, la partie supplementaire est initialisee a '0'
	void SetSize(int nValue);
	int GetSize() const;

	// (Re)initialisation a 0
	void Initialize();

	// Acces aux elements du vecteur
	void SetAt(int nIndex, double dValue);
	double GetAt(int nIndex) const;

	// Modification par addition d'une valeur a un element
	void UpgradeAt(int nIndex, double dDeltaValue);

	// Ajout d'un element en fin et retaillage du vecteur
	void Add(double dValue);

	// Tri des valeur par ordre croissant
	void Sort();

	// Perturbation aleatoire de l'ordre des valeurs
	void Shuffle();

	// Copie a partir d'un vecteur source
	// (retaillage si necessaire)
	void CopyFrom(const DoubleVector* cvSource);

	// Duplication
	DoubleVector* Clone() const;

	// Import de nElementNumber elements du tableau cByteBuffer vers l'index nIndex du vecteur
	void ImportBuffer(int nIndex, int nElementNumber, const char* cByteBuffer);

	// Export de nElementNumber elements vers le tableau cByteBuffer depuis l'index nIndex du vecteur
	// Le tableau cByteBuffer doit etre alloue et de taille nElementNumber*sizeof(double)
	void ExportBuffer(int nIndex, int nElementNumber, char* cByteBuffer) const;

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Retaillage avec potentiellement une grande taille, sans risque d'erreur d'allocation
	// Renvoie false si echec de retaillage (et le vecteur garde sa taille initiale)
	boolean SetLargeSize(int nValue);

	// Test
	static void Test();

	// Test ImportBuffer et ExportBuffer
	static void Test2();

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Constantes specifiques a la classe (permet une compilation optimisee pour les methodes SetAt et GetAt)
	static const int nElementSize = (int)sizeof(double);
	static const int nBlockSize = (int)((MemSegmentByteSize) / nElementSize);

	// Taille effective et taille allouee
	int nSize;
	int nAllocSize;

	// Donnees: soit directement un block de valeurs, soit un tableau de blocks
	// selon la taille allouee, soit un vecteur de grande taille generique pour beneficier
	// des methodes de MemVector
	union
	{
		double* pValues;
		double** pValueBlocks;
		MemHugeVector hugeVector;
	} pData;
	friend class PLSerializer;

	// Test des methodes ImportBuffer et ExportBuffer
	static void TestImportExport(int nVectorSize, int nIndexSource, int nIndexDest);
};

//////////////////////////////////////////////////////////
// Classe IntVector
// Vecteur d'entiers, de taille quelconque.
// Les acces aux vecteurs sont controles par assertions.
class IntVector : public Object
{
public:
	// Constructeur
	IntVector();
	~IntVector();

	// Taille
	// Lors d'un retaillage, la partie commune entre l'ancien et le nouveau
	// vecteur est preservee, la partie supplementaire est initialisee a '0'
	void SetSize(int nValue);
	int GetSize() const;

	// (Re)initialisation a 0
	void Initialize();

	// Acces aux elements du vecteur
	void SetAt(int nIndex, int nValue);
	int GetAt(int nIndex) const;

	// Modification par addition d'une valeur a un element
	void UpgradeAt(int nIndex, int nDeltaValue);

	// Ajout d'un element en fin et retaillage du vecteur
	void Add(int nValue);

	// Copie a partir d'un vecteur source
	// (retaillage si necessaire)
	void CopyFrom(const IntVector* ivSource);

	// Tri des valeur par ordre croissant
	void Sort();

	// Perturbation aleatoire de l'ordre des valeurs
	void Shuffle();

	// Duplication
	IntVector* Clone() const;

	// Import de nElementNumber elements du tableau cByteBuffer vers l'index nIndex du vecteur
	void ImportBuffer(int nIndex, int nElementNumber, const char* cByteBuffer);

	// Export de nElementNumber elements vers le tableau cByteBuffer depuis l'index nIndex du vecteur
	// Le tableau cByteBuffer doit etre alloue et de taille nElementNumber*sizeof(int)
	void ExportBuffer(int nIndex, int nElementNumber, char* cByteBuffer) const;

	// Retaillage avec potentiellement une grande taille, sans risque d'erreur d'allocation
	// Renvoie false si echec de retaillage (et le vecteur garde sa taille initiale)
	boolean SetLargeSize(int nValue);

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Test
	static void Test();

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Constantes specifiques a la classe (permet une compilation optimisee pour les methodes SetAt et GetAt)
	static const int nElementSize = (int)sizeof(int);
	static const int nBlockSize = (int)((MemSegmentByteSize) / nElementSize);

	// Taille effective et taille allouee
	int nSize;
	int nAllocSize;

	// Donnees: soit directement un block de valeurs, soit un tableau de blocks
	// selon la taille allouee, soit un vecteur de grande taille generique pour beneficier
	// des methodes de MemVector
	union
	{
		int* pValues;
		int** pValueBlocks;
		MemHugeVector hugeVector;
	} pData;
	friend class PLSerializer;
};

//////////////////////////////////////////////////////////
// Classe LongintVector
// Vecteur d'entiers longs, de taille quelconque.
// Les acces aux vecteurs sont controles par assertions.
class LongintVector : public Object
{
public:
	// Constructeur
	LongintVector();
	~LongintVector();

	// Taille
	// Lors d'un retaillage, la partie commune entre l'ancien et le nouveau
	// vecteur est preservee, la partie supplementaire est initialisee a '0'
	void SetSize(int nValue);
	int GetSize() const;

	// (Re)initialisation a 0
	void Initialize();

	// Acces aux elements du vecteur
	void SetAt(int nIndex, longint lValue);
	longint GetAt(int nIndex) const;

	// Modification par addition d'une valeur a un element
	void UpgradeAt(int nIndex, longint lDeltaValue);

	// Ajout d'un element en fin et retaillage du vecteur
	void Add(longint lValue);

	// Tri des valeur par ordre croissant
	void Sort();

	// Perturbation aleatoire de l'ordre des valeurs
	void Shuffle();

	// Copie a partir d'un vecteur source
	// (retaillage si necessaire)
	void CopyFrom(const LongintVector* lvSource);

	// Duplication
	LongintVector* Clone() const;

	// Import de nElementNumber elements du tableau cByteBuffer vers l'index nIndex du vecteur
	void ImportBuffer(int nIndex, int nElementNumber, const char* cByteBuffer);

	// Export de nElementNumber elements vers le tableau cByteBuffer depuis l'index nIndex du vecteur
	// Le tableau cByteBuffer doit etre alloue et de taille nElementNumber*sizeof(longint)
	void ExportBuffer(int nIndex, int nElementNumber, char* cByteBuffer) const;

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Retaillage avec potentiellement une grande taille, sans risque d'erreur d'allocation
	// Renvoie false si echec de retaillage (et le vecteur garde sa taille initiale)
	boolean SetLargeSize(int nValue);

	// Test
	static void Test();

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Constantes specifiques a la classe (permet une compilation optimisee pour les methodes SetAt et GetAt)
	static const int nElementSize = (int)sizeof(longint);
	static const int nBlockSize = (int)((MemSegmentByteSize) / nElementSize);

	// Taille effective et taille allouee
	int nSize;
	int nAllocSize;

	// Donnees: soit directement un block de valeurs, soit un tableau de blocks
	// selon la taille allouee, soit un vecteur de grande taille generique pour beneficier
	// des methodes de MemVector
	union
	{
		longint* pValues;
		longint** pValueBlocks;
		MemHugeVector hugeVector;
	} pData;
	friend class PLSerializer;
};

//////////////////////////////////////////////////////////
// Classe StringVector
// Vecteur de chaines de caracteres, de taille quelconque.
// Les acces aux vecteurs sont controles par assertions.
class StringVector : public Object
{
public:
	// Constructeur
	StringVector();
	~StringVector();

	// Taille
	// Lors d'un retaillage, la partie commune entre l'ancien et le nouveau
	// vecteur est preservee, la partie supplementaire est initialisee a ""'
	void SetSize(int nValue);
	int GetSize() const;

	// (Re)initialisation a 0
	void Initialize();

	// Acces aux elements du vecteur
	void SetAt(int nIndex, const ALString& sValue);
	const ALString& GetAt(int nIndex) const;

	// Ajout d'un element en fin et retaillage du vecteur
	void Add(const ALString& sValue);

	// Copie a partir d'un vecteur source
	// (retaillage si necessaire)
	void CopyFrom(const StringVector* svSource);

	// Tri des valeur par ordre croissant
	void Sort();

	// Perturbation aleatoire de l'ordre des valeurs
	void Shuffle();

	// Duplication
	StringVector* Clone() const;

	// Retaillage avec potentiellement une grande taille, sans risque d'erreur d'allocation
	// Renvoie false si echec de retaillage (et le vecteur garde sa taille initiale)
	boolean SetLargeSize(int nValue);

	// Affichage
	void Write(ostream& ost) const override;

	// Estimation de la memoire utilisee
	longint GetUsedMemory() const override;

	// Libelle de la classe
	const ALString GetClassLabel() const override;

	// Test
	static void Test();

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Acces aux elements du vecteur en tant que pointeurs sur chaine de caractere
	void InternalSetAt(int nIndex, ALString* sValue);
	ALString* InternalGetAt(int nIndex) const;

	// Constantes specifiques a la classe (permet une compilation optimisee pour les methodes SetAt et GetAt)
	static const int nElementSize = (int)sizeof(ALString*);
	static const int nBlockSize = (int)((MemSegmentByteSize) / nElementSize);

	// Taille effective et taille allouee
	int nSize;
	int nAllocSize;

	// Donnees: soit directement un block de valeurs, soit un tableau de blocks
	// selon la taille allouee, soit un vecteur de grande taille generique pour beneficier
	// des methodes de MemVector
	union
	{
		ALString** pValues;
		ALString*** pValueBlocks;
		MemHugeVector hugeVector;
	} pData;
	friend class PLSerializer;
};

/////////////////////////////////////////////////////////////////
// Implementations en inline

// Classe DoubleVector

inline DoubleVector::DoubleVector()
{
	MemVector::Create(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

inline int DoubleVector::GetSize() const
{
	return nSize;
}

inline void DoubleVector::SetAt(int nIndex, double dValue)
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		pData.pValues[nIndex] = dValue;
	else
		(pData.pValueBlocks[nIndex / nBlockSize])[nIndex % nBlockSize] = dValue;
}

inline double DoubleVector::GetAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		return pData.pValues[nIndex];
	else
		return (pData.pValueBlocks[nIndex / nBlockSize])[nIndex % nBlockSize];
}

inline void DoubleVector::UpgradeAt(int nIndex, double dDeltaValue)
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		pData.pValues[nIndex] += dDeltaValue;
	else
		pData.pValueBlocks[nIndex / nBlockSize][nIndex % nBlockSize] += dDeltaValue;
}

inline void DoubleVector::Add(double dValue)
{
	if (nSize < nAllocSize)
		nSize++;
	else
		SetSize(nSize + 1);
	SetAt(nSize - 1, dValue);
}

// Classe IntVector

inline IntVector::IntVector()
{
	MemVector::Create(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

inline int IntVector::GetSize() const
{
	return nSize;
}

inline void IntVector::SetAt(int nIndex, int nValue)
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		pData.pValues[nIndex] = nValue;
	else
		(pData.pValueBlocks[nIndex / nBlockSize])[nIndex % nBlockSize] = nValue;
}

inline int IntVector::GetAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		return pData.pValues[nIndex];
	else
		return (pData.pValueBlocks[nIndex / nBlockSize])[nIndex % nBlockSize];
}

inline void IntVector::UpgradeAt(int nIndex, int nDeltaValue)
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		pData.pValues[nIndex] += nDeltaValue;
	else
		pData.pValueBlocks[nIndex / nBlockSize][nIndex % nBlockSize] += nDeltaValue;
}

inline void IntVector::Add(int nValue)
{
	if (nSize < nAllocSize)
		nSize++;
	else
		SetSize(nSize + 1);
	SetAt(nSize - 1, nValue);
}

// Classe LongintVector

inline LongintVector::LongintVector()
{
	MemVector::Create(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

inline int LongintVector::GetSize() const
{
	return nSize;
}

inline void LongintVector::SetAt(int nIndex, longint lValue)
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		pData.pValues[nIndex] = lValue;
	else
		(pData.pValueBlocks[nIndex / nBlockSize])[nIndex % nBlockSize] = lValue;
}

inline longint LongintVector::GetAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		return pData.pValues[nIndex];
	else
		return (pData.pValueBlocks[nIndex / nBlockSize])[nIndex % nBlockSize];
}

inline void LongintVector::UpgradeAt(int nIndex, longint lDeltaValue)
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		pData.pValues[nIndex] += lDeltaValue;
	else
		pData.pValueBlocks[nIndex / nBlockSize][nIndex % nBlockSize] += lDeltaValue;
}

inline void LongintVector::Add(longint lValue)
{
	if (nSize < nAllocSize)
		nSize++;
	else
		SetSize(nSize + 1);
	SetAt(nSize - 1, lValue);
}

// Classe StringVector

inline StringVector::StringVector()
{
	MemVector::Create(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

inline int StringVector::GetSize() const
{
	return nSize;
}

inline void StringVector::SetAt(int nIndex, const ALString& sValue)
{
	ALString* sVectorValue;
	require(0 <= nIndex and nIndex < GetSize());
	sVectorValue = InternalGetAt(nIndex);
	(*sVectorValue) = sValue;
}

inline const ALString& StringVector::GetAt(int nIndex) const
{
	ALString* sVectorValue;
	require(0 <= nIndex and nIndex < GetSize());
	sVectorValue = InternalGetAt(nIndex);
	return (*sVectorValue);
}

inline void StringVector::Add(const ALString& sValue)
{
	if (nSize < nAllocSize)
	{
		nSize++;
		InternalSetAt(nSize - 1, new ALString(sValue));
	}
	else
	{
		SetSize(nSize + 1);
		SetAt(nSize - 1, sValue);
	}
}

inline void StringVector::InternalSetAt(int nIndex, ALString* sValue)
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		pData.pValues[nIndex] = sValue;
	else
		(pData.pValueBlocks[nIndex / nBlockSize])[nIndex % nBlockSize] = sValue;
}

inline ALString* StringVector::InternalGetAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		return pData.pValues[nIndex];
	else
		return (pData.pValueBlocks[nIndex / nBlockSize])[nIndex % nBlockSize];
}
