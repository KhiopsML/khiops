// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SNBIndexVector.h"

/////////////////////////////////////////////
// Implementation de la classe SNBIndexVector

inline void SNBIndexVector::SetValueIndexSize(int nValue)
{
	require(0 <= nValue);
	require(GetSize() == 0);

	nValueIndexSize = nValue;
	if (nValue == 0)
		nBitNumberPerValueIndex = 0;
	else
		nBitNumberPerValueIndex = int(ceil(log(nValue) / log(2)));
}

inline int SNBIndexVector::GetValueIndexSize() const
{
	return nValueIndexSize;
}

void SNBIndexVector::SetSize(int nValue)
{
	int nStorageSize;

	require(GetSize() == 0 or nValue == 0);
	require(nValue >= 0);
	require(GetValueIndexSize() > 0 or nValue == 0);

	// Calcul de la taille necessaire au stockage de tous les index
	nStorageSize =
	    (longint(nValue) * GetBitNumberPerValueIndex() + nBitNumberPerStorageUnit - 1) / nBitNumberPerStorageUnit;
	assert(longint(nStorageSize) * nBitNumberPerStorageUnit >= longint(nValue) * GetBitNumberPerValueIndex());
	assert(longint(nStorageSize - 1) * nBitNumberPerStorageUnit < longint(nValue) * GetBitNumberPerValueIndex());

	// Dimensionnement du vecteur de bytes
	nSize = nValue;
	lvStorage.SetSize(nStorageSize);
}

void SNBIndexVector::Initialize()
{
	lvStorage.Initialize();
}

void SNBIndexVector::CopyFrom(const SNBIndexVector* snbivSource)
{
	require(snbivSource != NULL);

	nSize = snbivSource->nSize;
	nValueIndexSize = snbivSource->nValueIndexSize;
	nBitNumberPerValueIndex = snbivSource->nBitNumberPerValueIndex;
	lvStorage.CopyFrom(&snbivSource->lvStorage);
}

SNBIndexVector* SNBIndexVector::Clone() const
{
	SNBIndexVector* snbsnbivClone;

	snbsnbivClone = new SNBIndexVector;

	// Recopie
	snbsnbivClone->CopyFrom(this);
	return snbsnbivClone;
}

boolean SNBIndexVector::SetLargeSize(int nValue)
{
	boolean bOk;
	int nStorageSize;

	require(GetSize() == 0 or nValue == 0);
	require(nValue >= 0);
	require(GetValueIndexSize() > 0 or nValue == 0);

	// Calcul de la taille necessaire au stockage de tous les index
	nStorageSize =
	    (longint(nValue) * GetBitNumberPerValueIndex() + nBitNumberPerStorageUnit - 1) / nBitNumberPerStorageUnit;
	assert(longint(nStorageSize) * nBitNumberPerStorageUnit >= longint(nValue) * GetBitNumberPerValueIndex());
	assert(longint(nStorageSize - 1) * nBitNumberPerStorageUnit < longint(nValue) * GetBitNumberPerValueIndex());

	// Dimensionnement du vecteur de bytes
	nSize = nValue;
	bOk = lvStorage.SetLargeSize(nStorageSize);
	if (not bOk)
		nSize = 0;
	return bOk;
}

void SNBIndexVector::Write(ostream& ost) const
{
	const int nMaxSize = 10;

	ost << GetClassLabel() + " (size=" << GetSize() << "value index size=" << GetValueIndexSize() << ")\n";
	for (int i = 0; i < GetSize() and i < nMaxSize; i++)
		ost << "\t" << GetAt(i) << "\n";
	if (GetSize() > nMaxSize)
		ost << "\t"
		    << "..."
		    << "\n";
}

longint SNBIndexVector::GetUsedMemory() const
{
	return sizeof(SNBIndexVector) + lvStorage.GetUsedMemory() - sizeof(LongintVector);
}

int SNBIndexVector::GetBitNumberPerValueIndex() const
{
	return nBitNumberPerValueIndex;
}

const ALString SNBIndexVector::GetClassLabel() const
{
	return "Index vector";
}

void SNBIndexVector::Test()
{
	SNBIndexVector* snbivTest;
	SNBIndexVector* snbivClone;
	int nSize;
	int nMaxSize;
	int nValueIndexSize;
	int nRandomIndex;
	int nBenchmarkNumber;
	int nBenchmark;
	int nPreviousSize;
	int i;
	longint lTotalRead;
	longint lTotalWritten;
	int nStartClock;
	int nStopClock;
	longint lTotalSize;
	boolean bOk;

	// Creation
	SetRandomSeed(1);
	nSize = AcquireRangedInt("Taille du vecteur", 0, 1000000, 1000);
	nValueIndexSize = AcquireRangedInt("Taille des index de valeur", 2, 1000000, 7);
	snbivTest = new SNBIndexVector;
	snbivTest->SetValueIndexSize(nValueIndexSize);
	snbivTest->SetSize(nSize);
	cout << *snbivTest << endl;

	// Remplissage avec des index maximaux
	cout << "Remplissage" << endl;
	for (i = 0; i < snbivTest->GetSize(); i++)
		snbivTest->SetAt(i, nValueIndexSize - 1);
	cout << *snbivTest << endl;

	// Duplication
	cout << "Duplication" << endl;
	snbivClone = snbivTest->Clone();
	cout << *snbivClone << endl;

	// Remplissage sequentiel
	cout << "Remplissage sequentiel" << endl;
	for (i = 0; i < snbivTest->GetSize(); i++)
		snbivTest->SetAt(i, i % nValueIndexSize);
	cout << *snbivTest << endl;

	// Benchmarks intensifs
	nMaxSize = AcquireRangedInt("Taille max des vecteurs pour les benchmarks intensifs", 1000, 100000000, 1000000);
	nBenchmarkNumber = AcquireRangedInt("Nombre de benchmarks intensifs", 0, 100000, 100);
	snbivTest->SetSize(0);
	nSize = 0;
	nPreviousSize = 0;
	lTotalSize = 0;
	nStartClock = clock();
	bOk = true;
	for (nBenchmark = 0; nBenchmark < nBenchmarkNumber; nBenchmark++)
	{
		nPreviousSize = nSize;

		// Tirage d'une taille aleatoire
		nSize = RandomInt(nMaxSize);

		// Tirage d'une taille d'index aleatoire entre 2 et 100 ou entre 10^9 et 2 * 10^9
		if (RandomDouble() <= 0.5)
			nValueIndexSize = 2 + RandomInt(98);
		else
			nValueIndexSize = 1000000000 + RandomInt(1000000000);

		// Retaillage apres remise a la taille nulle
		snbivTest->SetSize(0);
		snbivTest->SetValueIndexSize(nValueIndexSize), snbivTest->SetSize(nSize);
		lTotalSize += snbivTest->GetSize();

		// Remplissage avec les index: on sait que le total fait nSize*nRandomIndex
		lTotalWritten = 0;
		for (i = 0; i < snbivTest->GetSize(); i++)
		{
			nRandomIndex = RandomInt(nValueIndexSize - 1);
			snbivTest->SetAt(i, nRandomIndex);
			lTotalWritten += nRandomIndex;
		}

		// Test de recopie
		snbivClone->CopyFrom(snbivTest);
		lTotalSize += snbivClone->GetSize();

		// Verification du total
		lTotalRead = 0;
		for (i = 0; i < snbivClone->GetSize(); i++)
			lTotalRead += snbivClone->GetAt(i);
		bOk = bOk and lTotalRead == lTotalWritten;
		assert(bOk);

		// Point d'avancement
		cout << "." << flush;
	}
	nStopClock = clock();
	cout << "\nTotal size allocated\t" << lTotalSize << endl;
	if (not bOk)
		cout << "\tAn error occured in benchmark (see debug mode)" << endl;
	cout << "SYS TIME\tBenchmark\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";

	// Allocation de tres grande taille
	nMaxSize = AcquireRangedInt("Taille max des vecteurs pour les allocations de tres grande taille", 1000,
				    1000000000, 1000000);
	nSize = 1000;
	cout << "SYS MEMORY\t";
	while (nSize < nMaxSize)
	{
		snbivTest->SetLargeSize(0);
		bOk = snbivTest->SetLargeSize(nSize);
		cout << " " << nSize << flush;
		if (not bOk)
		{
			cout << " (" << snbivTest->GetSize() << ")";
			break;
		}
		nSize *= 2;
	}
	cout << endl;

	// Liberations
	delete snbivClone;
	delete snbivTest;
}
