// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Vector.h"

//////////////////////////////////////
// Implementation de la classe DoubleVector

int DoubleVectorCompareValue(const void* elem1, const void* elem2)
{
	double dValue1;
	double dValue2;

	// Acces aux valeurs
	dValue1 = *(double*)elem1;
	dValue2 = *(double*)elem2;

	// Comparaison
	if (dValue1 == dValue2)
		return 0;
	else
		return dValue1 > dValue2 ? 1 : -1;
}

DoubleVector::~DoubleVector()
{
	MemVector::Delete(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

void DoubleVector::SetSize(int nValue)
{
	MemVector::SetSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);
}

void DoubleVector::Initialize()
{
	MemVector::Initialize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

void DoubleVector::Sort()
{
	MemVector::Sort(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, DoubleVectorCompareValue);
}

void DoubleVector::Shuffle()
{
	int i;
	int iSwap;
	double dSwappedValue;

	// Retour si pas assez d'elements
	if (GetSize() <= 1)
		return;

	// Boucle de swap d'elements du tableau
	for (i = 1; i < GetSize(); i++)
	{
		iSwap = RandomInt(i);
		dSwappedValue = GetAt(iSwap);
		SetAt(iSwap, GetAt(i));
		SetAt(i, dSwappedValue);
	}
}

void DoubleVector::CopyFrom(const DoubleVector* dvSource)
{
	require(dvSource != NULL);
	MemVector::CopyFrom(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, dvSource->pData.hugeVector,
			    dvSource->nSize, dvSource->nAllocSize);
}

DoubleVector* DoubleVector::Clone() const
{
	DoubleVector* dvClone;

	dvClone = new DoubleVector;

	// Recopie
	dvClone->CopyFrom(this);
	return dvClone;
}

void DoubleVector::ImportBuffer(int nIndex, int nElementNumber, const char* cByteBuffer)
{
	MemVector::ImportBuffer(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nIndex, nElementNumber,
				cByteBuffer);
}

void DoubleVector::ExportBuffer(int nIndex, int nElementNumber, char* cByteBuffer) const
{
	MemVector::ExportBuffer(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nIndex, nElementNumber,
				cByteBuffer);
}

boolean DoubleVector::SetLargeSize(int nValue)
{
	return MemVector::SetLargeSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);
}

void DoubleVector::Write(ostream& ost) const
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

longint DoubleVector::GetUsedMemory() const
{
	return sizeof(DoubleVector) + nAllocSize * sizeof(double);
}

const ALString DoubleVector::GetClassLabel() const
{
	return "Double vector";
}

void DoubleVector::Test()
{
	DoubleVector* dvTest;
	DoubleVector* dvClone;
	int nSize;
	int i;

	// Creation
	SetRandomSeed(1);
	nSize = AcquireRangedInt("Taille du vecteur", 0, 10000, 1000);
	dvTest = new DoubleVector;
	dvTest->SetSize(nSize);
	cout << *dvTest << endl;

	// Remplissage avec des 1
	cout << "Remplissage avec des 1" << endl;
	for (i = 0; i < dvTest->GetSize(); i++)
		dvTest->SetAt(i, 1);
	cout << *dvTest << endl;

	// Duplication
	cout << "Duplication" << endl;
	dvClone = dvTest->Clone();
	cout << *dvClone << endl;

	// Retaillage
	nSize = AcquireRangedInt("Nouvelle taille du vecteur, avec conservation des valeurs", 0, 10000, 2000);
	dvTest->SetSize(nSize);
	cout << *dvTest << endl;

	// Retaillage avec reinitialisation a '0'
	nSize = AcquireRangedInt("Nouvelle taille du vecteur", 0, 10000, 1000);
	dvTest->SetSize(nSize);
	dvTest->Initialize();
	cout << *dvTest << endl;

	// Retaillage par ajouts successifs
	nSize = AcquireRangedInt("Nombre d'agrandissements", 0, 1000000, 9000);
	for (i = 0; i < nSize; i++)
		dvTest->Add(0);
	cout << *dvTest << endl;

	// Remplissage aleatoire
	cout << "Remplissage aleatoire" << endl;
	for (i = 0; i < dvTest->GetSize(); i++)
		dvTest->SetAt(i, RandomDouble());
	cout << *dvTest << endl;

	// Tri
	cout << "Tri" << endl;
	dvTest->Sort();
	cout << *dvTest << endl;

	// Perturbation aleatoire
	cout << "Perturbation aleatoire" << endl;
	dvTest->Shuffle();
	cout << *dvTest << endl;

	// Liberations
	delete dvClone;
	delete dvTest;
}

void DoubleVector::Test2()
{
	const longint lSizeMono = 100;
	const longint lSizeMulti = 8 * lKB + 1;

	cout << "Test mono bloc" << endl;
	TestImportExport(lSizeMono, 0, 0);
	cout << endl << "Test multi bloc" << endl;
	TestImportExport(lSizeMulti, 0, 0);
	cout << endl << "Test mono bloc, export from index 4" << endl;
	TestImportExport(lSizeMono, 4, 0);
	cout << endl << "Test multi bloc, export from index 4" << endl;
	TestImportExport(lSizeMulti, 4, 0);
	cout << endl << "Test mono bloc, export at index 4" << endl;
	TestImportExport(lSizeMono, 0, 4);
	cout << endl << "Test multi bloc, export at index 4" << endl;
	TestImportExport(lSizeMulti, 0, 4);
}

void DoubleVector::TestImportExport(int nVectorSize, int nIndexSource, int nIndexDest)
{
	DoubleVector dvSource;
	DoubleVector dvDest;
	char* cBuffer;
	int i;

	// Test mono bloc
	for (i = 0; i < nVectorSize + nIndexSource; i++)
	{
		dvSource.Add(1.0 * i);
	}
	cBuffer = NewCharArray(nVectorSize * sizeof(double));
	dvSource.ExportBuffer(nIndexSource, nVectorSize, cBuffer);

	dvDest.SetSize(nVectorSize + nIndexDest);
	dvDest.ImportBuffer(nIndexDest, nVectorSize, cBuffer);
	DeleteCharArray(cBuffer);

	for (i = 0; i < nVectorSize; i++)
	{
		cout << dvDest.GetAt(i) << " ";

		if (i > 100)
		{
			cout << "...";
			break;
		}
	}
	cout << endl;
}

/////////////////////////////////////////////
// Implementation de la classe IntVector

int IntVectorCompareValue(const void* elem1, const void* elem2)
{
	return *(int*)elem1 - *(int*)elem2;
}

IntVector::~IntVector()
{
	MemVector::Delete(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

void IntVector::SetSize(int nValue)
{
	MemVector::SetSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);
}

void IntVector::Initialize()
{
	MemVector::Initialize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

void IntVector::Sort()
{
	MemVector::Sort(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, IntVectorCompareValue);
}

void IntVector::Shuffle()
{
	int i;
	int iSwap;
	int nSwappedValue;

	// Retour si pas assez d'elements
	if (GetSize() <= 1)
		return;

	// Boucle de swap d'elements du tableau
	for (i = 1; i < GetSize(); i++)
	{
		iSwap = RandomInt(i);
		nSwappedValue = GetAt(iSwap);
		SetAt(iSwap, GetAt(i));
		SetAt(i, nSwappedValue);
	}
}

void IntVector::CopyFrom(const IntVector* ivSource)
{
	require(ivSource != NULL);
	MemVector::CopyFrom(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, ivSource->pData.hugeVector,
			    ivSource->nSize, ivSource->nAllocSize);
}

IntVector* IntVector::Clone() const
{
	IntVector* ivClone;

	ivClone = new IntVector;

	// Recopie
	ivClone->CopyFrom(this);
	return ivClone;
}

void IntVector::ImportBuffer(int nIndex, int nElementNumber, const char* cByteBuffer)
{
	MemVector::ImportBuffer(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nIndex, nElementNumber,
				cByteBuffer);
}

void IntVector::ExportBuffer(int nIndex, int nElementNumber, char* cByteBuffer) const
{
	MemVector::ExportBuffer(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nIndex, nElementNumber,
				cByteBuffer);
}

boolean IntVector::SetLargeSize(int nValue)
{
	return MemVector::SetLargeSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);
}

void IntVector::Write(ostream& ost) const
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

longint IntVector::GetUsedMemory() const
{
	return sizeof(IntVector) + nAllocSize * sizeof(int);
}

const ALString IntVector::GetClassLabel() const
{
	return "Int vector";
}

void IntVector::Test()
{
	IntVector* ivTest;
	IntVector* ivClone;
	int nSize;
	int nMaxSize;
	int nBenchmarkNumber;
	int nBenchmark;
	int nPreviousSize;
	int i;
	double dTotal;
	int nStartClock;
	int nStopClock;
	double dTotalSize;
	boolean bOk;
	boolean bBenchmarkSort;

	// Creation
	SetRandomSeed(1);
	nSize = AcquireRangedInt("Taille du vecteur", 0, 1000000, 1000);
	ivTest = new IntVector;
	ivTest->SetSize(nSize);
	cout << *ivTest << endl;

	// Remplissage avec des 1
	cout << "Remplissage avec des 1" << endl;
	for (i = 0; i < ivTest->GetSize(); i++)
		ivTest->SetAt(i, 1);
	cout << *ivTest << endl;

	// Duplication
	cout << "Duplication" << endl;
	ivClone = ivTest->Clone();
	cout << *ivClone << endl;

	// Retaillage
	nSize = AcquireRangedInt("Nouvelle taille du vecteur, avec conservation des valeurs", 0, 1000000, 2000);
	ivTest->SetSize(nSize);
	cout << *ivTest << endl;

	// Retaillage avec reinitialisation a '0'
	nSize = AcquireRangedInt("Nouvelle taille du vecteur", 0, 1000000, 1000);
	ivTest->SetSize(nSize);
	ivTest->Initialize();
	cout << *ivTest << endl;

	// Retaillage par ajouts successifs
	nSize = AcquireRangedInt("Nombre d'agrandissements", 0, 1000000, 9000);
	for (i = 0; i < nSize; i++)
		ivTest->Add(0);
	cout << *ivTest << endl;

	// Remplissage aleatoire
	cout << "Remplissage aleatoire" << endl;
	for (i = 0; i < ivTest->GetSize(); i++)
		ivTest->SetAt(i, RandomInt(ivTest->GetSize()));
	cout << *ivTest << endl;

	// Tri
	cout << "Tri" << endl;
	ivTest->Sort();
	cout << *ivTest << endl;

	// Perturbation aleatoire
	cout << "Perturbation aleatoire" << endl;
	ivTest->Shuffle();
	cout << *ivTest << endl;

	// Benchmarks intensifs
	nMaxSize = AcquireRangedInt("Taille max des vecteurs pour les benchmarks intensifs", 1000, 100000000, 1000000);
	nBenchmarkNumber = AcquireRangedInt("Nombre de benchmarks intensifs", 0, 100000, 100);
	bBenchmarkSort = AcquireBoolean("Sort benchmark", true);
	ivTest->SetSize(0);
	nSize = 0;
	nPreviousSize = 0;
	dTotalSize = 0;
	nStartClock = clock();
	bOk = true;
	for (nBenchmark = 0; nBenchmark < nBenchmarkNumber; nBenchmark++)
	{
		nPreviousSize = nSize;

		// Tirage d'une taille aleatoire, pour tester toutes les transitions entre petite taille et grande
		// taille
		if (nBenchmark % 4 == 0 or nBenchmark % 4 == 1)
			nSize = RandomInt(nBlockSize);
		else
			nSize = RandomInt(nMaxSize);

		// On force la transition a 0 dans quelques cas
		if (nBenchmark % 16 == 1 or nBenchmark % 16 == 3)
			nSize = 0;

		// Retaillage
		ivTest->SetSize(nSize);
		dTotalSize += ivTest->GetSize();

		// Verification du total avant retaillage
		dTotal = 0;
		for (i = 0; i < ivTest->GetSize(); i++)
			dTotal += ivTest->GetAt(i);
		bOk = bOk and
		      ((nSize <= nPreviousSize and fabs(dTotal - nSize * (nSize - 1.0) / 2.0) < 1e-5) or
		       (nSize > nPreviousSize and fabs(dTotal - nPreviousSize * (nPreviousSize - 1.0) / 2.0) < 1e-5));
		assert(bOk);

		// Remplissage avec les index: on sait que le total fait nSize*(nSize-1)/2
		for (i = 0; i < ivTest->GetSize(); i++)
			ivTest->SetAt(i, i);

		// Verification du total apres retaillage
		dTotal = 0;
		for (i = 0; i < ivTest->GetSize(); i++)
			dTotal += ivTest->GetAt(i);
		bOk = bOk and fabs(dTotal - nSize * (nSize - 1.0) / 2.0) < 1e-5;
		assert(bOk);

		// Perturbation aleatoire
		ivTest->Shuffle();

		// Verification du total apres la perturbation
		dTotal = 0;
		for (i = 0; i < ivTest->GetSize(); i++)
			dTotal += ivTest->GetAt(i);
		bOk = bOk and fabs(dTotal - nSize * (nSize - 1.0) / 2.0) < 1e-5;
		assert(bOk);

		// Tri
		if (bBenchmarkSort)
			ivTest->Sort();
		// On restitue les valeurs initiales si le tri n'a pas ete effectue
		else
		{
			for (i = 0; i < ivTest->GetSize(); i++)
				ivTest->SetAt(i, i);
		}

		// Verification du tri
		if (bBenchmarkSort)
		{
			for (i = 1; i < ivTest->GetSize(); i++)
				bOk = bOk and ivTest->GetAt(i) >= ivTest->GetAt(i - 1);
			assert(bOk);
		}

		// Verification du total apres le tri
		dTotal = 0;
		for (i = 0; i < ivTest->GetSize(); i++)
			dTotal += ivTest->GetAt(i);
		bOk = bOk and fabs(dTotal - nSize * (nSize - 1.0) / 2.0) < 1e-5;
		assert(bOk);

		// Test d'allocation de grande taille
		ivClone->SetLargeSize(ivTest->GetSize());
		if (ivClone->GetSize() != ivTest->GetSize())
			cout << "SYS MEM set large size " << ivTest->GetSize() << " -> " << ivClone->GetSize() << endl;
		dTotalSize += ivClone->GetSize();
		ivClone->SetSize(0);

		// Test de recopie
		ivClone->CopyFrom(ivTest);
		dTotalSize += ivClone->GetSize();

		// Verification du tri de la copie
		if (bBenchmarkSort)
		{
			for (i = 1; i < ivClone->GetSize(); i++)
				bOk = bOk and ivClone->GetAt(i) >= ivClone->GetAt(i - 1);
			assert(bOk);
		}

		// Verification du total apres le tri
		dTotal = 0;
		for (i = 0; i < ivClone->GetSize(); i++)
			dTotal += ivClone->GetAt(i);
		bOk = bOk and fabs(dTotal - nSize * (nSize - 1.0) / 2.0) < 1e-5;
		assert(bOk);

		// Point d'avancement
		cout << "." << flush;
	}
	nStopClock = clock();
	cout << "\nTotal size allocated\t" << dTotalSize << endl;
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
		bOk = ivTest->SetLargeSize(nSize);
		cout << " " << nSize << flush;
		if (not bOk)
		{
			cout << " (" << ivTest->GetSize() << ")";
			break;
		}
		nSize *= 2;
	}
	cout << endl;

	// Liberations
	delete ivClone;
	delete ivTest;
}

//////////////////////////////////////
// Implementation de la classe LongintVector

int LongintVectorCompareValue(const void* elem1, const void* elem2)
{
	longint lValue1;
	longint lValue2;

	// Acces aux valeurs
	lValue1 = *(longint*)elem1;
	lValue2 = *(longint*)elem2;

	// Comparaison
	if (lValue1 == lValue2)
		return 0;
	else
		return lValue1 > lValue2 ? 1 : -1;
}

LongintVector::~LongintVector()
{
	MemVector::Delete(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

void LongintVector::SetSize(int nValue)
{
	MemVector::SetSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);
}

void LongintVector::Initialize()
{
	MemVector::Initialize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

void LongintVector::Sort()
{
	MemVector::Sort(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, LongintVectorCompareValue);
}

void LongintVector::Shuffle()
{
	int i;
	int iSwap;
	longint dSwappedValue;

	// Retour si pas assez d'elements
	if (GetSize() <= 1)
		return;

	// Boucle de swap d'elements du tableau
	for (i = 1; i < GetSize(); i++)
	{
		iSwap = RandomInt(i);
		dSwappedValue = GetAt(iSwap);
		SetAt(iSwap, GetAt(i));
		SetAt(i, dSwappedValue);
	}
}

void LongintVector::CopyFrom(const LongintVector* dvSource)
{
	require(dvSource != NULL);
	MemVector::CopyFrom(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, dvSource->pData.hugeVector,
			    dvSource->nSize, dvSource->nAllocSize);
}

LongintVector* LongintVector::Clone() const
{
	LongintVector* dvClone;

	dvClone = new LongintVector;

	// Recopie
	dvClone->CopyFrom(this);
	return dvClone;
}

void LongintVector::ImportBuffer(int nIndex, int nElementNumber, const char* cByteBuffer)
{
	MemVector::ImportBuffer(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nIndex, nElementNumber,
				cByteBuffer);
}

void LongintVector::ExportBuffer(int nIndex, int nElementNumber, char* cByteBuffer) const
{
	MemVector::ExportBuffer(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nIndex, nElementNumber,
				cByteBuffer);
}

boolean LongintVector::SetLargeSize(int nValue)
{
	return MemVector::SetLargeSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);
}

void LongintVector::Write(ostream& ost) const
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

longint LongintVector::GetUsedMemory() const
{
	return sizeof(LongintVector) + nAllocSize * sizeof(longint);
}

const ALString LongintVector::GetClassLabel() const
{
	return "Longint vector";
}

void LongintVector::Test()
{
	LongintVector* lvTest;
	LongintVector* lvClone;
	longint lValue;
	int nSize;
	int i;

	// Creation
	SetRandomSeed(1);
	nSize = AcquireRangedInt("Taille du vecteur", 0, 10000, 1000);
	lvTest = new LongintVector;
	lvTest->SetSize(nSize);
	cout << *lvTest << endl;

	// Remplissage avec des 1
	cout << "Remplissage avec des 1" << endl;
	for (i = 0; i < lvTest->GetSize(); i++)
		lvTest->SetAt(i, 1);
	cout << *lvTest << endl;

	// Duplication
	cout << "Duplication" << endl;
	lvClone = lvTest->Clone();
	cout << *lvClone << endl;

	// Retaillage
	nSize = AcquireRangedInt("Nouvelle taille du vecteur, avec conservation des valeurs", 0, 10000, 2000);
	lvTest->SetSize(nSize);
	cout << *lvTest << endl;

	// Retaillage avec reinitialisation a '0'
	nSize = AcquireRangedInt("Nouvelle taille du vecteur", 0, 10000, 1000);
	lvTest->SetSize(nSize);
	lvTest->Initialize();
	cout << *lvTest << endl;

	// Retaillage par ajouts successifs
	nSize = AcquireRangedInt("Nombre d'agrandissements", 0, 1000000, 9000);
	for (i = 0; i < nSize; i++)
		lvTest->Add(0);
	cout << *lvTest << endl;

	// Remplissage aleatoire
	cout << "Remplissage aleatoire" << endl;
	for (i = 0; i < lvTest->GetSize(); i++)
	{
		lvTest->SetAt(i, RandomInt(lvTest->GetSize()));
	}
	cout << *lvTest << endl;

	// Tri
	cout << "Tri" << endl;
	lvTest->Sort();
	cout << *lvTest << endl;

	// Perturbation aleatoire
	cout << "Perturbation aleatoire" << endl;
	lvTest->Shuffle();
	cout << *lvTest << endl;

	// Affichage lisible
	cout << "Affichage en bytes de facon lisible" << endl;
	lValue = 1;
	while (lValue < 100 * lPB)
	{
		cout << "\t" << LongintToHumanReadableString(lValue) << "\t" << lValue << endl;
		lValue *= 3;
	}

	// Affichage lisible
	cout << "Affichage avec separateuir de milliers" << endl;
	lValue = 1;
	while (lValue < 100 * lPB)
	{
		cout << "\t" << LongintToReadableString(-lValue) << "\t" << -lValue << endl;
		cout << "\t" << LongintToReadableString(lValue) << "\t" << lValue << endl;
		lValue *= 3;
	}

	// Liberations
	delete lvClone;
	delete lvTest;
}

/////////////////////////////////////////////
// Implementation de la classe StringVector

int StringVectorCompareValue(const void* elem1, const void* elem2)
{
	ALString* sValue1;
	ALString* sValue2;

	// Comparaison des chaines de caracteres
	sValue1 = *(ALString**)elem1;
	sValue2 = *(ALString**)elem2;
	return sValue1->Compare(*sValue2);
}

StringVector::~StringVector()
{
	int i;

	// Destruction des chaines de caracteres du vecteur
	for (i = 0; i < GetSize(); i++)
		delete InternalGetAt(i);

	// Destruction du vecteur
	MemVector::Delete(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize);
}

void StringVector::SetSize(int nValue)
{
	int i;
	int nInitialSize;

	// Si diminution de taille, destruction des chaines de caracteres en trop
	nInitialSize = GetSize();
	for (i = nValue; i < nInitialSize; i++)
		delete InternalGetAt(i);

	// Retaillage du vecteur
	MemVector::SetSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);

	// Allocation des nouvelles chaines de caracteres
	for (i = nInitialSize; i < nValue; i++)
		InternalSetAt(i, new ALString);
}

void StringVector::Initialize()
{
	int i;

	// Reinitialisation a chaine vide
	for (i = 0; i < GetSize(); i++)
		SetAt(i, "");
}

void StringVector::Sort()
{
	MemVector::Sort(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, StringVectorCompareValue);
}

void StringVector::Shuffle()
{
	int i;
	int iSwap;
	ALString* sSwappedValue;

	// Retour si pas assez d'elements
	if (GetSize() <= 1)
		return;

	// Boucle de swap d'elements du tableau
	for (i = 1; i < GetSize(); i++)
	{
		iSwap = RandomInt(i);
		sSwappedValue = InternalGetAt(iSwap);
		InternalSetAt(iSwap, InternalGetAt(i));
		InternalSetAt(i, sSwappedValue);
	}
}

void StringVector::CopyFrom(const StringVector* svSource)
{
	int i;

	require(svSource != NULL);

	// Retaillage
	SetSize(svSource->GetSize());

	// Copie des valeurs
	for (i = 0; i < GetSize(); i++)
		SetAt(i, svSource->GetAt(i));
}

StringVector* StringVector::Clone() const
{
	StringVector* svClone;

	svClone = new StringVector;

	// Recopie
	svClone->CopyFrom(this);
	return svClone;
}

boolean StringVector::SetLargeSize(int nValue)
{
	boolean bOk;
	int i;
	int nInitialSize;

	// Si diminution de taille, destruction des chaines de caracteres en trop
	nInitialSize = GetSize();
	for (i = nValue; i < nInitialSize; i++)
		delete InternalGetAt(i);

	// Retaillage du vecteur
	bOk = MemVector::SetLargeSize(pData.hugeVector, nSize, nAllocSize, nBlockSize, nElementSize, nValue);

	// Allocation des nouvelles chaines de caracteres, dans la limite de la taille effectivement allouee
	for (i = nInitialSize; i < nSize; i++)
		InternalSetAt(i, new ALString);
	return bOk;
}

void StringVector::Write(ostream& ost) const
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

longint StringVector::GetUsedMemory() const
{
	longint lUsedMemory;
	int i;
	ALString* sString;

	lUsedMemory = sizeof(StringVector) + nAllocSize * sizeof(ALString*);
	for (i = 0; i < GetSize(); i++)
	{
		sString = InternalGetAt(i);
		check(sString);
		lUsedMemory += sString->GetUsedMemory();
	}
	return lUsedMemory;
}

const ALString StringVector::GetClassLabel() const
{
	return "String vector";
}

void StringVector::Test()
{
	StringVector* svTest;
	StringVector* svClone;
	int nSize;
	int nMaxSize;
	int nBenchmarkNumber;
	int nBenchmark;
	int i;
	int nStartClock;
	int nStopClock;
	double dTotalSize;
	boolean bOk;
	boolean bBenchmarkSort;
	ALString sTmp;
	ALString sValue;

	// Creation
	SetRandomSeed(1);
	nSize = AcquireRangedInt("Taille du vecteur", 0, 1000000, 1000);
	svTest = new StringVector;
	svTest->SetSize(nSize);
	cout << *svTest << endl;

	// Remplissage avec des 1
	cout << "Remplissage avec des s1" << endl;
	for (i = 0; i < svTest->GetSize(); i++)
		svTest->SetAt(i, "s1");
	cout << *svTest << endl;

	// Duplication
	cout << "Duplication" << endl;
	svClone = svTest->Clone();
	cout << *svClone << endl;

	// Retaillage
	nSize = AcquireRangedInt("Nouvelle taille du vecteur, avec conservation des valeurs", 0, 1000000, 2000);
	svTest->SetSize(nSize);
	cout << *svTest << endl;

	// Retaillage avec reinitialisation a vide
	nSize = AcquireRangedInt("Nouvelle taille du vecteur", 0, 1000000, 1000);
	svTest->SetSize(nSize);
	svTest->Initialize();
	cout << *svTest << endl;

	// Retaillage par ajouts successifs
	nSize = AcquireRangedInt("Nombre d'agrandissements", 0, 1000000, 9000);
	for (i = 0; i < nSize; i++)
		svTest->Add("");
	cout << *svTest << endl;

	// Remplissage aleatoire
	cout << "Remplissage aleatoire" << endl;
	for (i = 0; i < svTest->GetSize(); i++)
	{
		sValue = sTmp + "s" + IntToString(RandomInt(svTest->GetSize()));
		svTest->SetAt(i, sValue);
		assert(sValue == svTest->GetAt(i));
	}
	cout << *svTest << endl;

	// Tri
	cout << "Tri" << endl;
	svTest->Sort();
	cout << *svTest << endl;

	// Perturbation aleatoire
	cout << "Perturbation aleatoire" << endl;
	svTest->Shuffle();
	cout << *svTest << endl;

	// Benchmarks intensifs
	nMaxSize = AcquireRangedInt("Taille max des vecteurs pour les benchmarks intensifs", 1000, 100000000, 1000000);
	nBenchmarkNumber = AcquireRangedInt("Nombre de benchmarks intensifs", 0, 100000, 100);
	bBenchmarkSort = AcquireBoolean("Sort benchmark", true);
	svTest->SetSize(0);
	nSize = 0;
	dTotalSize = 0;
	nStartClock = clock();
	bOk = true;
	for (nBenchmark = 0; nBenchmark < nBenchmarkNumber; nBenchmark++)
	{
		// Tirage d'une taille aleatoire, pour tester toutes les transitions entre petite taille et grande
		// taille
		if (nBenchmark % 4 == 0 or nBenchmark % 4 == 1)
			nSize = RandomInt(nBlockSize);
		else
			nSize = RandomInt(nMaxSize);

		// On force la transition a 0 dans quelques cas
		if (nBenchmark % 16 == 1 or nBenchmark % 16 == 3)
			nSize = 0;

		// Retaillage
		svTest->SetSize(nSize);
		dTotalSize += svTest->GetSize();
		// Perturbation aleatoire
		svTest->Shuffle();

		// Tri
		if (bBenchmarkSort)
			svTest->Sort();
		// On restitue les valeurs initiales si le tri n'a pas ete effectue
		else
		{
			for (i = 0; i < svTest->GetSize(); i++)
				svTest->SetAt(i, sTmp + "s" + IntToString(i));
		}

		// Verification du tri
		if (bBenchmarkSort)
		{
			for (i = 1; i < svTest->GetSize(); i++)
				bOk = bOk and svTest->GetAt(i) >= svTest->GetAt(i - 1);
			assert(bOk);
		}

		// Test d'allocation de grande taille
		svClone->SetLargeSize(svTest->GetSize());
		if (svClone->GetSize() != svTest->GetSize())
			cout << "SYS MEM set large size " << svTest->GetSize() << " -> " << svClone->GetSize() << endl;
		dTotalSize += svClone->GetSize();
		svClone->SetSize(0);

		// Test de recopie
		svClone->CopyFrom(svTest);
		dTotalSize += svClone->GetSize();

		// Verification du tri de la copie
		if (bBenchmarkSort)
		{
			for (i = 1; i < svClone->GetSize(); i++)
				bOk = bOk and svClone->GetAt(i) >= svClone->GetAt(i - 1);
		}
		// Point d'avancement
		cout << "." << flush;
	}
	nStopClock = clock();
	cout << "\nTotal size allocated\t" << dTotalSize << endl;
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
		bOk = svTest->SetLargeSize(nSize);
		cout << " " << nSize << flush;
		if (not bOk)
		{
			cout << " (" << svTest->GetSize() << ")";
			break;
		}
		nSize *= 2;
	}
	cout << endl;

	// Allocation de chaines de tres grandes tailles, aleatoires
	nBenchmarkNumber =
	    AcquireRangedInt("Nombre total d'allocation de chaines de tres grande taille", 0, 1000000, 100000);
	svTest->SetSize(1000);
	nStartClock = clock();
	dTotalSize = 0;
	for (nBenchmark = 0; nBenchmark < nBenchmarkNumber; nBenchmark++)
	{
		nSize = 100000 + RandomInt(900000);
		dTotalSize += nSize;
		i = RandomInt(svTest->GetSize() - 1);
		sValue.GetBufferSetLength(nSize);
		svTest->SetAt(i, sValue);
		sValue.Empty();
	}
	nStopClock = clock();
	cout << "SYS MEMORY\tTotal size allocated\t" << dTotalSize << endl;
	cout << "SYS TIME\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << endl;

	// Liberations
	delete svClone;
	delete svTest;
}
