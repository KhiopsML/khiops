// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CharVector.h"

//////////////////////////////////////
// Implementation de la classe CharVector

int CharVectorCompareValue(const void* elem1, const void* elem2)
{
	return *(char*)elem1 - *(char*)elem2;
}

CharVector::~CharVector()
{
	MemVector::Delete(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

void CharVector::SetSize(int nValue)
{
	MemVector::SetSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);
}

void CharVector::Initialize()
{
	MemVector::Initialize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

void CharVector::Sort()
{
	MemVector::Sort(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, CharVectorCompareValue);
}

void CharVector::Shuffle()
{
	int i;
	int iSwap;
	char cSwappedValue;

	// Retour si pas assez d'elements
	if (GetSize() <= 1)
		return;

	// Boucle de swap d'elements du tableau
	for (i = 1; i < GetSize(); i++)
	{
		iSwap = RandomInt(i - 1);
		cSwappedValue = GetAt(iSwap);
		SetAt(iSwap, GetAt(i));
		SetAt(i, cSwappedValue);
	}
}

void CharVector::CopyFrom(const CharVector* cvSource)
{
	require(cvSource != NULL);
	MemVector::CopyFrom(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, cvSource->pData.hugeVector,
			    cvSource->nSize, cvSource->nAllocSize);
}

CharVector* CharVector::Clone() const
{
	CharVector* cvClone;

	cvClone = new CharVector;

	// Recopie
	cvClone->CopyFrom(this);
	return cvClone;
}

void CharVector::ImportBuffer(int nIndex, int nBufferSize, const char* cBuffer)
{
	MemVector::ImportBuffer(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nIndex, nBufferSize,
				cBuffer);
	ensure(ContainsCharsAt(nIndex, cBuffer, nBufferSize));
}

void CharVector::ImportSubBuffer(int nIndex, int nBufferSize, const CharVector* cBuffer, int nBufferStart)
{
	int nSizeToCopy;
	int nLocalCopy;
	int nSourcePos;
	int nDestPos;
	void* pSource;
	void* pDest;

	require(0 <= nIndex and nIndex < GetSize());
	require(nIndex + nBufferSize <= GetSize());
	require(cBuffer != NULL);
	require(cBuffer != this or nIndex == 0);
	require(0 <= nBufferStart and nBufferStart < cBuffer->GetSize());
	require(nBufferStart + nBufferSize <= cBuffer->GetSize());

	nSourcePos = nBufferStart;
	nDestPos = nIndex;
	nSizeToCopy = nBufferSize;

	while (nSizeToCopy > 0)
	{
		// On ne copie pas plus que ce que peut contenir le bloc de destination
		nLocalCopy = min(nBlockSize - (nDestPos % nBlockSize), nSizeToCopy);

		// ... et pas plus que ce que contient le bloc source
		nLocalCopy = min(nLocalCopy, nBlockSize - (nSourcePos % nBlockSize));

		// Positionnement du pointeur vers les donnees sources
		if (cBuffer->nAllocSize <= nBlockSize)
			pSource = &cBuffer->pData.hugeVector.pValues[nSourcePos];
		else
			pSource =
			    &cBuffer->pData.hugeVector.pValueBlocks[nSourcePos / nBlockSize][nSourcePos % nBlockSize];

		// Positionnement du pointeur vers les donnees destination
		if (nAllocSize <= nBlockSize)
			pDest = &pData.hugeVector.pValues[nDestPos];
		else
			pDest = &pData.hugeVector.pValueBlocks[nDestPos / nBlockSize][nDestPos % nBlockSize];

		// Copie de source vers dest
		memcpy(pDest, pSource, (size_t)nLocalCopy);

		nSizeToCopy -= nLocalCopy;
		nSourcePos += nLocalCopy;
		nDestPos += nLocalCopy;
	}

	ensure(cBuffer == this or ContainsSubCharVectorAt(nIndex, cBuffer, nBufferStart, nBufferSize));
}

void CharVector::ExportBuffer(int nIndex, int nBufferSize, char* cBuffer) const
{
	MemVector::ExportBuffer(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nIndex, nBufferSize,
				cBuffer);
	ensure(ContainsCharsAt(nIndex, cBuffer, nBufferSize));
}

boolean CharVector::SetLargeSize(int nValue)
{
	return MemVector::SetLargeSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);
}

void CharVector::Write(ostream& ost) const
{
	const int nMaxSize = 10;

	ost << GetClassLabel() + " (size=" << GetSize() << ")\n";
	for (int i = 0; i < GetSize() and i < nMaxSize; i++)
		ost << "\t" << GetAt(i) << "\n";
	if (GetSize() > nMaxSize)
		ost << "\t"
		    << "..."
		    << "\n";
}

longint CharVector::GetUsedMemory() const
{
	return sizeof(CharVector) + nAllocSize * sizeof(char);
}

const ALString CharVector::GetClassLabel() const
{
	return "Char vector";
}

void CharVector::Test()
{
	CharVector* cvTest;
	CharVector* cvClone;
	int nSize;
	int i;
	char c;

	// Creation
	SetRandomSeed(1);
	nSize = AcquireRangedInt("Taille du vecteur", 0, 10000, 1000);
	cvTest = new CharVector;
	cvTest->SetSize(nSize);
	cout << *cvTest << endl;

	// Remplissage avec des 1
	cout << "Remplissage avec des 1" << endl;
	for (i = 0; i < cvTest->GetSize(); i++)
		cvTest->SetAt(i, '1');
	cout << *cvTest << endl;

	// Duplication
	cout << "Duplication" << endl;
	cvClone = cvTest->Clone();
	cout << *cvClone << endl;

	// Retaillage
	nSize = AcquireRangedInt("Nouvelle taille du vecteur, avec conservation des valeurs", 0, 10000, 2000);
	cvTest->SetSize(nSize);
	cout << *cvTest << endl;

	// Retaillage avec reinitialisation a '0'
	nSize = AcquireRangedInt("Nouvelle taille du vecteur", 0, 10000, 1000);
	cvTest->SetSize(nSize);
	cvTest->Initialize();
	cout << *cvTest << endl;

	// Retaillage par ajouts successifs
	nSize = AcquireRangedInt("Nombre d'agrandissements", 0, 1000000, 9000);
	for (i = 0; i < nSize; i++)
		cvTest->Add(0);
	cout << *cvTest << endl;

	// Remplissage aleatoire
	cout << "Remplissage aleatoire" << endl;
	for (i = 0; i < cvTest->GetSize(); i++)
	{
		c = (char)(int('0') + RandomInt('z' - '0'));
		cvTest->SetAt(i, c);
	}
	cout << *cvTest << endl;

	// Tri
	cout << "Tri" << endl;
	cvTest->Sort();
	cout << *cvTest << endl;

	// Perturbation aleatoire
	cout << "Perturbation aleatoire" << endl;
	cvTest->Shuffle();
	cout << *cvTest << endl;

	// Liberations
	delete cvClone;
	delete cvTest;
}

void CharVector::Test2()
{
	boolean bOk = true;
	int nIndexSource;
	int nIndexDest;
	int nSource;
	int nDest;
	CharVector cv;
	char* cBuffer;

	// Test de TestCopyAt et TestCopySubCharVectorAt en faisant varier differentes combinaison de multi / mono blocs
	// Et en prenant 0 ou 4 comme index du charVector de destination et 0 ou 10 comme index du CharVector source
	for (nDest = 0; nDest <= 1; nDest++)
	{
		nIndexDest = nDest * 4;
		for (nSource = 0; nSource <= 1; nSource++)
		{
			nIndexSource = nSource * 10;

			///////////////////////
			// Tests de TestCopyAt

			// Multi-bloc : copie de 1 Ko
			bOk = CharVector::TestCopyAt(1 * lMB, 1 * lMB, nIndexDest, nIndexSource, 1 * lKB) and bOk;
			ensure(bOk);

			// Idem a l'index nBlockSize  (au debut du 2eme bloc)
			bOk =
			    CharVector::TestCopyAt(1 * lMB, 1 * lMB, nBlockSize + nIndexDest, nIndexSource, 1 * lKB) and
			    bOk;
			ensure(bOk);

			// Multi-bloc : copie de 100 Ko
			bOk = CharVector::TestCopyAt(1 * lMB, 1 * lMB, nIndexDest, nIndexSource, 100 * lKB) and bOk;
			ensure(bOk);

			// Multi-bloc : copie de 300 Ko
			bOk = CharVector::TestCopyAt(1 * lMB, 1 * lMB, nIndexDest, nIndexSource, 300 * lKB) and bOk;
			ensure(bOk);

			// Idem a l'index nBlockSize  (au debut du 2eme bloc)
			bOk = CharVector::TestCopyAt(1 * lMB, 1 * lMB, nBlockSize + nIndexDest, nIndexSource,
						     100 * lKB) and
			      bOk;
			ensure(bOk);

			// Mono-bloc
			bOk = CharVector::TestCopyAt(10 * lKB, 10 * lKB, nIndexDest, nIndexSource, 5 * lKB) and bOk;
			ensure(bOk);

			////////////////////////////////////
			// Tests de TestCopySubCharVectorAt

			// Multi-blocs source et destination, copie inferieur a un bloc
			bOk = TestCopySubCharVectorAt(1 * lMB, 1 * lMB, nIndexDest, nIndexSource, 1 * lKB) and bOk;
			ensure(bOk);

			// Multi-blocs source et destination, copie de 100 Ko
			bOk = TestCopySubCharVectorAt(1 * lMB, 1 * lMB, nIndexDest, nIndexSource, 100 * lKB) and bOk;
			ensure(bOk);

			// Multi-blocs source et destination, copie de 300 Ko
			bOk = TestCopySubCharVectorAt(1 * lMB, 1 * lMB, nIndexDest, nIndexSource, 300 * lKB) and bOk;
			ensure(bOk);

			// Mono-bloc source et destination
			bOk = TestCopySubCharVectorAt(20 * lKB, 20 * lKB, nIndexDest, nIndexSource, 1 * lKB) and bOk;
			ensure(bOk);

			// Mono-bloc source, multi-blocs destination copie de 1 KB
			bOk = TestCopySubCharVectorAt(1 * lMB, 20 * lKB, nIndexDest, nIndexSource, 1 * lKB) and bOk;
			ensure(bOk);

			// Multi-bloc source, mono-bloc destination
			bOk = TestCopySubCharVectorAt(20 * lKB, 1 * lMB, nIndexDest, nIndexSource, 1 * lKB) and bOk;
			ensure(bOk);
		}
	}

	// Test de ExportChars

	// CharVector mono-bloc
	cv.SetSize(10 * lKB);
	cv.InitWithRandomChars();
	cBuffer = new char[1 * lKB];

	// Copie de 1 KB a partir de l'index 0
	cv.ExportBuffer(0, 1 * lKB, cBuffer);
	bOk = cv.ContainsCharsAt(0, cBuffer, 1 * lKB) and bOk;
	ensure(bOk);

	// Copie de 1 KB a partir de l'index 10
	cv.ExportBuffer(10, 1 * lKB, cBuffer);
	bOk = cv.ContainsCharsAt(10, cBuffer, 1 * lKB) and bOk;
	ensure(bOk);

	delete[] cBuffer;

	// CharVector multi-bloc
	cv.SetSize(1 * lMB);
	cv.InitWithRandomChars();
	cBuffer = new char[64 * lKB];

	// Copie de 64 KB a partir de l'index 0
	cv.ExportBuffer(0, 64 * lKB, cBuffer);
	bOk = cv.ContainsCharsAt(0, cBuffer, 64 * lKB) and bOk;
	ensure(bOk);

	// Copie de 64 KB a partir de l'index 10
	cv.ExportBuffer(10, 64 * lKB, cBuffer);
	bOk = cv.ContainsCharsAt(10, cBuffer, 64 * lKB) and bOk;
	ensure(bOk);

	// Copie de 64 KB a partir de l'index 100 lKB
	cv.ExportBuffer(100 * lKB, 64 * lKB, cBuffer);
	bOk = cv.ContainsCharsAt(100 * lKB, cBuffer, 64 * lKB) and bOk;
	ensure(bOk);

	delete[] cBuffer;

	if (bOk)
		cout << "Test is OK  :-)" << endl;
	else
		cout << "Test is KO" << endl;
}
boolean CharVector::TestCopyAt(int nSizeDest, int nSizeSource, int nIndexDest, int nIndexSource, int nCopySize)
{
	CharVector cvDest;
	char* pSource;
	const char cDest = '1';
	const char cSource = '2';
	int nCharSourceNumber;
	int nCharDestNumber;
	int nErrorNumber;
	int i;
	boolean bOk;

	assert(nIndexDest + nCopySize < nSizeDest);
	assert(nIndexSource + nCopySize <= nSizeSource);

	cvDest.SetSize(nSizeDest);

	// Initialisation de tableau source avec des '2'
	pSource = new char[nSizeSource];
	for (i = 0; i < nSizeSource; i++)
	{
		pSource[i] = cSource;
	}

	// Initialisation du CharVector destination avec des '1'
	cvDest.InitWithChar(cDest);

	// Copie de la source vers la destination
	cvDest.ImportBuffer(nIndexDest, nCopySize, &pSource[nIndexSource]);

	// Comptage des caracteres
	nCharSourceNumber = 0;
	nCharDestNumber = 0;
	nErrorNumber = 0;
	for (i = 0; i < cvDest.GetSize(); i++)
	{
		switch (cvDest.GetAt(i))
		{
		case cSource:
			nCharSourceNumber++;
			break;
		case cDest:
			nCharDestNumber++;
			break;
		default:
			nErrorNumber++;
		}
	}

	// Test si les resultats sont OK
	bOk = nCharSourceNumber == nCopySize;
	ensure(bOk);
	bOk = nCharDestNumber == nSizeDest - nCharSourceNumber;
	ensure(bOk);
	delete[] pSource;
	return bOk;
}

boolean CharVector::TestCopySubCharVectorAt(int nSizeDest, int nSizeSource, int nIndexDest, int nIndexSource,
					    int nCopySize)
{
	CharVector cvSource;
	CharVector cvDest;
	const char cSource = '1';
	const char cDest = '2';
	int nCharSourceNumber;
	int nCharDestNumber;
	int nErrorNumber;
	int i;
	boolean bOk;

	assert(nIndexSource + nCopySize <= nSizeSource);
	assert(nIndexDest + nCopySize <= nSizeDest);

	// Initialisation des 2 CharVector avec le caractere '1' pour la source et '2' pour la destination
	cvDest.SetSize(nSizeDest);
	cvSource.SetSize(nSizeSource);
	cvDest.InitWithChar(cDest);
	cvSource.InitWithChar(cSource);

	// Copie de la source vers la destionation
	cvDest.ImportSubBuffer(nIndexDest, nCopySize, &cvSource, nIndexSource);

	// Comptage des caracteres dans le CharVector de destination
	nCharSourceNumber = 0;
	nCharDestNumber = 0;
	nErrorNumber = 0;
	for (i = 0; i < cvDest.GetSize(); i++)
	{
		switch (cvDest.GetAt(i))
		{
		case cSource:
			nCharSourceNumber++;
			break;
		case cDest:
			nCharDestNumber++;
			break;
		default:
			nErrorNumber++;
		}
	}

	// Verification des resultats obtenus
	bOk = nErrorNumber == 0;
	ensure(bOk);
	bOk = nCharSourceNumber == nCopySize and bOk;
	ensure(bOk);
	bOk = (nCharDestNumber == nSizeDest - nCharSourceNumber) and bOk;
	ensure(bOk);
	return bOk;
}
boolean CharVector::ContainsCharsAt(int nIndex, const char* sSourceChars, int nSourceLength) const
{
	boolean bOk;
	int i;

	require(0 <= nIndex and nIndex < GetSize());
	require(sSourceChars != NULL);

	// Verification de base
	bOk = (nIndex + nSourceLength <= GetSize());

	// Verification detaillee du contenu
	if (bOk)
	{
		for (i = 0; i < nSourceLength; i++)
			bOk = bOk and GetAt(nIndex + i) == sSourceChars[i];
	}
	return bOk;
}

boolean CharVector::ContainsSubCharVectorAt(int nIndex, const CharVector* cvSource, int nSourceStart,
					    int nSourceLength) const
{
	boolean bOk;
	int i;

	require(0 <= nIndex and nIndex < GetSize());
	require(cvSource != NULL);
	require(0 <= nSourceStart and nSourceStart < cvSource->GetSize());
	require(nSourceStart + nSourceLength <= cvSource->GetSize());

	// Verification de base
	bOk = (nIndex + nSourceLength <= GetSize());

	// Verification detaillee du contenu
	if (bOk)
	{
		for (i = 0; i < nSourceLength; i++)
			bOk = bOk and GetAt(nIndex + i) == cvSource->GetAt(nSourceStart + i);
	}
	return bOk;
}

void CharVector::InitWithChar(char c)
{
	int i;
	for (i = 0; i < GetSize(); i++)
	{
		SetAt(i, c);
	}
}

void CharVector::InitWithRandomChars()
{
	int i;
	for (i = 0; i < GetSize(); i++)
	{
		SetAt(i, (char)('a' + IthRandomInt(i, 25)));
	}
}