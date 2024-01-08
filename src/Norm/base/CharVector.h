// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "MemVector.h"

//////////////////////////////////////////////////////////
// Classe CharVector
// Vecteur de caracteres, de taille quelconque.
// Les acces aux vecteurs sont controles par assertions.
class CharVector : public Object
{
public:
	CharVector(void);
	~CharVector(void);

	// Taille
	// Lors d'un retaillage, la partie commune entre l'ancien et le nouveau
	// vecteur est preservee, la partie supplementaire est initialisee a '\0'
	void SetSize(int nValue);
	int GetSize() const;

	// (Re)initialisation a '\0'
	void Initialize();

	// Acces aux elements du vecteur
	void SetAt(int nIndex, char cValue);
	char GetAt(int nIndex) const;

	// Modification par addition d'une valeur a un element
	void UpgradeAt(int nIndex, char cDeltaValue);

	// Ajout d'un element en fin et retaillage du vecteur
	void Add(char cValue);

	// Tri des valeur par ordre croissant
	void Sort();

	// Perturbation aleatoire de l'ordre des valeurs
	void Shuffle();

	// Copie a partir d'un vecteur source
	// (retaillage si necessaire)
	void CopyFrom(const CharVector* cvSource);

	// Duplication
	CharVector* Clone() const;

	// Copie d'une chaine de caracteres C a partir d'une position
	void ImportBuffer(int nIndex, int nBufferSize, const char* cBuffer);

	// Copie d'une sous partie de vecteur de caracteres a partir d'une position
	// L'utilisation avec cvSource==this est permise uniquement dans le cas particulier du debut de buffer
	// (nIndex==0)
	void ImportSubBuffer(int nIndex, int nBufferSize, const CharVector* cBuffer, int nBufferStart);

	// Export vers un tableau de chars a partir d'un index donne, d'une quantite donnee.
	// Le tableau de chars doit etre alloue
	void ExportBuffer(int nIndex, int nBufferSize, char* cBuffer) const;

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

	// Test des methodes CopyCharsAt, CopySubCharVectorAt et ExportChars
	static void Test2();

	// Initialisation du tableau avec le caractere passe en parametre
	void InitWithChar(char c);
	void InitWithRandomChars();

	///////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Test de la methode CopyCharsAt.
	// Creation d'un CharVector de taille nSizeDest qui ne contient que des '1'
	// Creation d'un char* de taille nSizeSource qui ne contient que des '2'
	// Copie de nCopySize octets du deuxieme tableau dans le premier a l'index nIndexDest, a partir de nIndexSource
	// Renvoie true si le CharVector contient le bon nombre de '1' et de '2'
	static boolean TestCopyAt(int nSizeDest, int nSizeSource, int nIndexDest, int nIndexSource, int nCopySize);

	// Test de la methode CopySubCharVectorAt en suivant le meme principe que TestCopyAt
	static boolean TestCopySubCharVectorAt(int nSizeDest, int nSizeSource, int nIndexDest, int nIndexSource,
					       int nCopySize);

	// Methodes utiles pour des assertions
	boolean ContainsCharsAt(int nIndex, const char* sSourceChars, int nSourceLength) const;
	boolean ContainsSubCharVectorAt(int nIndex, const CharVector* cvSource, int nSourceStart,
					int nSourceLength) const;

	friend class PLFileConcatenater;

	// Constantes specifiques a la classe (permet une compilation optimisee pour les methodes SetAt et GetAt)
	static const int nElementSize = (int)sizeof(char);
	static const int nBlockSize = (int)((MemSegmentByteSize) / nElementSize);

	// Taille effective et taille allouee
	int nSize;
	int nAllocSize;

	// Donnees: soit directement un block de valeurs, soit un tableau de blocks
	// selon la taille allouee, soit un vecteur de grande taille generique pour beneficier
	// des methodes de MemVector
	union
	{
		char* pValues;
		char** pValueBlocks;
		MemHugeVector hugeVector;
	} pData;

	// Classes friend ayant besoin d'un acces direct aux blocks
	friend class FileCache;
	friend class BufferedFile;
	friend class PLSerializer;

	// Methodes privees tres techniques
	// Mise a disposition des attributs protected aux classes friends
	int InternalGetAllocSize() const;
	static int InternalGetBlockSize();
	static int InternalGetElementSize();
	char* InternalGetMonoBlockBuffer() const;
	char* InternalGetMultiBlockBuffer(int i) const;
};

/////////////////////////////////////////////////////////////////
// Implementations en inline

inline CharVector::CharVector()
{
	MemVector::Create(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

inline int CharVector::GetSize() const
{
	return nSize;
}

inline void CharVector::SetAt(int nIndex, char cValue)
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		pData.pValues[nIndex] = cValue;
	else
		(pData.pValueBlocks[nIndex / nBlockSize])[nIndex % nBlockSize] = cValue;
}

inline char CharVector::GetAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		return pData.pValues[nIndex];
	else
		return (pData.pValueBlocks[nIndex / nBlockSize])[nIndex % nBlockSize];
}

inline void CharVector::UpgradeAt(int nIndex, char cDeltaValue)
{
	require(0 <= nIndex and nIndex < GetSize());
	if (nAllocSize <= nBlockSize)
		pData.pValues[nIndex] += cDeltaValue;
	else
		pData.pValueBlocks[nIndex / nBlockSize][nIndex % nBlockSize] += cDeltaValue;
}

inline void CharVector::Add(char cValue)
{
	if (nSize < nAllocSize)
		nSize++;
	else
		SetSize(nSize + 1);
	SetAt(nSize - 1, cValue);
}

inline int CharVector::InternalGetAllocSize() const
{
	return nAllocSize;
}

inline int CharVector::InternalGetBlockSize()
{
	return nBlockSize;
}

inline int CharVector::InternalGetElementSize()
{
	return nElementSize;
}

inline char* CharVector::InternalGetMonoBlockBuffer() const
{
	assert(nAllocSize <= nBlockSize);
	return pData.hugeVector.pValues;
}
inline char* CharVector::InternalGetMultiBlockBuffer(int i) const
{
	assert(nAllocSize > nBlockSize);
	return pData.hugeVector.pValueBlocks[i];
}
