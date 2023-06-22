// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SparseStudy.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Classe SparseStudy

SparseStudy::SparseStudy()
{
	nIndex = 0;
	lIndex = 0;
}

SparseStudy::~SparseStudy() {}

void SparseStudy::Write(ostream& ost) const
{
	ost << "(" << nIndex << ", " << lIndex << ", " << index << ")";
}

void SparseStudy::Test()
{
	SparseStudy sparseStudy;
	int iIndex;
	longint lIndex;
	LoadIndex index;
	Timer timer;
	int nMax = 1000000000;
	int nMaxAccess = 100000000;
	debug(nMaxAccess /= 10);
	int i;
	double dValue;

	// Tests de base
	cout << "sizeof(LoadIndex) " << sizeof(LoadIndex) << endl;
	cout << "sizeof(SparseStudy) " << sizeof(SparseStudy) << endl;
	index.Reset();
	cout << "Full(-1) " << index << endl;
	index.SetDenseIndex(-1);
	index.SetSparseIndex(-1);
	cout << "(-1, -1) " << index << endl;
	index.SetDenseIndex(0);
	index.SetSparseIndex(-1);
	cout << "(0, -1) " << index << endl;
	index.SetDenseIndex(-1);
	index.SetSparseIndex(0);
	cout << "(-1, 0) " << index << endl;

	// Affectation avec des entiers long
	lIndex = index;
	cout << "To longint: " << lIndex << endl;
	index.Reset();
	index = lIndex;
	cout << "From longint: " << index << endl;
	index.Reset();
	cout << "Full(-1) " << index << endl;
	lIndex = index;
	cout << "To longint: " << lIndex << endl;
	index.Reset();
	index = lIndex;
	cout << "From longint: " << index << endl;

	// Performance sur des entiers
	cout << "Performance sur des entiers: " << flush;
	iIndex = 128;
	timer.Reset();
	timer.Start();
	for (i = 0; i < nMax; i++)
		sparseStudy.nIndex = iIndex;
	timer.Stop();
	cout << sparseStudy << " " << timer.GetElapsedTime() << endl;

	// Performance sur des entiers longs
	cout << "Performance sur des entiers longs: " << flush;
	lIndex = 1024;
	lIndex *= lIndex;
	lIndex *= lIndex;
	timer.Reset();
	timer.Start();
	for (i = 0; i < nMax; i++)
		sparseStudy.lIndex = lIndex;
	timer.Stop();
	cout << sparseStudy << " " << timer.GetElapsedTime() << endl;

	// Performance sur des LoadIndex
	cout << "Performance sur des LoadIndex: " << flush;
	index.SetDenseIndex(256);
	index.SetSparseIndex(1024);
	timer.Reset();
	timer.Start();
	for (i = 0; i < nMax; i++)
		sparseStudy.index = index;
	timer.Stop();
	cout << sparseStudy << " " << timer.GetElapsedTime() << endl;

	// Initialisation d'un vecteur de valeurs
	sparseStudy.dvValues.SetSize(10000);
	for (i = 0; i < sparseStudy.dvValues.GetSize(); i++)
		sparseStudy.dvValues.SetAt(i, i);

	// Performance sur l'acces dense
	cout << "Performance sur l'acces dense: " << flush;
	sparseStudy.index.SetDenseIndex(256);
	sparseStudy.index.SetSparseIndex(-1);
	dValue = 0;
	timer.Reset();
	timer.Start();
	for (i = 0; i < nMaxAccess; i++)
		dValue += sparseStudy.GetDenseValue();
	timer.Stop();
	cout << dValue << " " << timer.GetElapsedTime() << endl;

	// Performance sur l'acces sparse
	cout << "Performance sur l'acces sparse: " << flush;
	sparseStudy.index.SetDenseIndex(256);
	sparseStudy.index.SetSparseIndex(1024);
	dValue = 0;
	timer.Reset();
	timer.Start();
	for (i = 0; i < nMaxAccess; i++)
		dValue += sparseStudy.GetSparseValue();
	timer.Stop();
	cout << dValue << " " << timer.GetElapsedTime() << endl;

	// Performance sur l'acces generique dense
	cout << "Performance sur l'acces generique dense: " << flush;
	sparseStudy.index.SetDenseIndex(256);
	sparseStudy.index.SetSparseIndex(-1);
	dValue = 0;
	timer.Reset();
	timer.Start();
	for (i = 0; i < nMaxAccess; i++)
		dValue += sparseStudy.GetValue();
	timer.Stop();
	cout << dValue << " " << timer.GetElapsedTime() << endl;

	// Performance sur l'acces generique sparse
	cout << "Performance sur l'acces generique sparse: " << flush;
	sparseStudy.index.SetDenseIndex(256);
	sparseStudy.index.SetSparseIndex(1024);
	dValue = 0;
	timer.Reset();
	timer.Start();
	for (i = 0; i < nMaxAccess; i++)
		dValue += sparseStudy.GetValue();
	timer.Stop();
	cout << dValue << " " << timer.GetElapsedTime() << endl;
}

/////////////////////////////////////////////////////////////////////////
void BasicVectorAccessTest()
{
	int i;
	Timer timer;
	longint lTotal;
	// int nSize;

	// for (nSize = 1; nSize <= 20; nSize++)
	{
		//
		timer.Reset();
		timer.Start();
		lTotal = 0;
		for (i = 0; i < 1000000000; i++)
		{
			lTotal += i;
		}
		timer.Stop();
		cout << "1\t" << timer.GetElapsedTime() << "\t" << lTotal << endl;
		//
		timer.Reset();
		timer.Start();
		lTotal = 0;
		for (i = 0; i < 1000000000; i++)
		{
			lTotal += i / 2;
		}
		timer.Stop();
		cout << "2\t" << timer.GetElapsedTime() << "\t" << lTotal << endl;
		//
		timer.Reset();
		timer.Start();
		lTotal = 0;
		for (i = 0; i < 1000000000; i++)
		{
			lTotal += i / 3;
		}
		timer.Stop();
		cout << "3\t" << timer.GetElapsedTime() << "\t" << lTotal << endl;
		//
		timer.Reset();
		timer.Start();
		lTotal = 0;
		for (i = 0; i < 1000000000; i++)
		{
			lTotal += i / 4;
		}
		timer.Stop();
		cout << "4\t" << timer.GetElapsedTime() << "\t" << lTotal << endl;
		//
		timer.Reset();
		timer.Start();
		lTotal = 0;
		for (i = 0; i < 1000000000; i++)
		{
			lTotal += i / 8;
		}
		timer.Stop();
		cout << "8\t" << timer.GetElapsedTime() << "\t" << lTotal << endl;
		//
		timer.Reset();
		timer.Start();
		lTotal = 0;
		for (i = 0; i < 1000000000; i++)
		{
			lTotal += i / 9;
		}
		timer.Stop();
		cout << "9\t" << timer.GetElapsedTime() << "\t" << lTotal << endl;
		//
		timer.Reset();
		timer.Start();
		lTotal = 0;
		for (i = 0; i < 1000000000; i++)
		{
			lTotal += i / 12;
		}
		timer.Stop();
		cout << "12\t" << timer.GetElapsedTime() << "\t" << lTotal << endl;
		//
		timer.Reset();
		timer.Start();
		lTotal = 0;
		for (i = 0; i < 1000000000; i++)
		{
			lTotal += i / 16;
		}
		timer.Stop();
		cout << "16\t" << timer.GetElapsedTime() << "\t" << lTotal << endl;
		//
		timer.Reset();
		timer.Start();
		lTotal = 0;
		for (i = 0; i < 1000000000; i++)
		{
			lTotal += i / 19;
		}
		timer.Stop();
		cout << "19\t" << timer.GetElapsedTime() << "\t" << lTotal << endl;
	}
}

#include <chrono>
using namespace std::chrono;

void DoubleVectorAccessTest()
{
	const int nSize = 1000;
	double dvVector[nSize];
	const int nStepNumber = 1000000;
	int nStep;
	int i;
	double dTotal;
	char* dvVectorAdress;
	std::chrono::time_point<std::chrono::steady_clock> start;
	std::chrono::time_point<std::chrono::steady_clock> stop;
	double dDuration;

	// Initialisation
	for (i = 0; i < nSize; i++)
		dvVector[i] = i;

	// Lecture
	start = high_resolution_clock::now();
	dTotal = 0;
	for (nStep = 0; nStep < nStepNumber; nStep++)
	{
		for (i = 0; i < nSize; i++)
		{
			dTotal += dvVector[i];
		}
	}
	stop = high_resolution_clock::now();
	dDuration = duration_cast<chrono::nanoseconds>(stop - start).count() / 1000000000.0;
	cout << "Total: " << dTotal << endl;
	cout << "Duration: " << dDuration << endl;

	// Lecture
	dvVectorAdress = (char*)(void*)&dvVector;
	start = high_resolution_clock::now();
	dTotal = 0;
	for (nStep = 0; nStep < nStepNumber; nStep++)
	{
		for (i = 0; i < nSize; i++)
		{
			dTotal += *((double*)&dvVectorAdress[i * 8]);
		}
	}
	stop = high_resolution_clock::now();
	dDuration = duration_cast<chrono::nanoseconds>(stop - start).count() / 1000000000.0;
	cout << "Total: " << dTotal << endl;
	cout << "Duration: " << dDuration << endl;
}

/////////////////////////////////////////////////////////////////////
// Classe KWContinuousValueBlock: gestion d'un bloc de valeurs sparse
KWContinuousValueBlockTest* KWContinuousValueBlockTest::NewValueBlock(int nSize)
{
	KWContinuousValueBlockTest* newValueBlock;
	int nMemorySize;
	void* pValueBlockMemory;
	int nSegmentNumber;
	KWValueIndexPairTest* valueIndexPairs;
	int i;

	require(nSize >= 0);

	// Cas mono-segment: le bloc entier peut tenir dans un segement memoire
	if (nSize <= nSegmentSize)
	{
		// Calcul de la taille a allouer
		nMemorySize = sizeof(int) + nSize * sizeof(KWValueIndexPairTest);

		// Creation des donnees d'un bloc
		pValueBlockMemory = NewMemoryBlock(nMemorySize);

		// Initialisation avec des zero
		memset(pValueBlockMemory, 0, nMemorySize * sizeof(char));

		// On utilise le "placement new" pour appeler un constructeur avec de la memoire preallouee (attention,
		// C++ avance)
		newValueBlock = new (pValueBlockMemory) KWContinuousValueBlockTest;
		assert(newValueBlock->GetValueNumber() == 0);

		// Parametrage du nombre de valeurs
		newValueBlock->nValueNumber = nSize;

		// On verifie par assertion que le packing des classe utilise est correct
		assert(sizeof(KWValueIndexPairTest) == sizeof(KWValue) + sizeof(int));
		assert(&(newValueBlock->cStartBlock) - (char*)newValueBlock == sizeof(int));
	}
	// Cas multi-segment: il faut plusieurs segment pour stocker le bloc
	//  On passe par un tableau de segments
	else
	{
		// Calcul du nombre de segment
		nSegmentNumber = (nSize - 1) / nSegmentSize + 1;

		// Calcul de la taille a allouer
		nMemorySize = sizeof(int) + nSegmentNumber * sizeof(KWValueIndexPairTest*);

		// Creation des donnees du bloc, avec un tableau de pointeurs vers des blocs
		pValueBlockMemory = NewMemoryBlock(nMemorySize);

		// On utilise le "placement new" pour appeler un constructeur avec de la memoire preallouee (attention,
		// C++ avance)
		newValueBlock = new (pValueBlockMemory) KWContinuousValueBlockTest;

		// Parametrage du nombre de valeurs
		newValueBlock->nValueNumber = nSize;
		assert(newValueBlock->GetValueNumber() == nSize);

		// Creation et initialisation des segments
		for (i = nSegmentNumber - 2; i >= 0; i--)
		{
			valueIndexPairs =
			    (KWValueIndexPairTest*)NewMemoryBlock(nSegmentSize * sizeof(KWValueIndexPairTest));
			((KWValueIndexPairTest**)&(newValueBlock->cStartBlock))[i] = valueIndexPairs;
			memset(valueIndexPairs, 0, nSegmentSize * sizeof(KWValueIndexPairTest));
		}
		if (nSize % nSegmentSize > 0)
		{
			valueIndexPairs = (KWValueIndexPairTest*)NewMemoryBlock((nSize % nSegmentSize) *
										sizeof(KWValueIndexPairTest));
			((KWValueIndexPairTest**)&(newValueBlock->cStartBlock))[nSegmentNumber - 1] = valueIndexPairs;
			memset(valueIndexPairs, 0, (nSize % nSegmentSize) * sizeof(KWValueIndexPairTest));
		}
		else
		{
			valueIndexPairs =
			    (KWValueIndexPairTest*)NewMemoryBlock(nSegmentSize * sizeof(KWValueIndexPairTest));
			((KWValueIndexPairTest**)&(newValueBlock->cStartBlock))[nSegmentNumber - 1] = valueIndexPairs;
			memset(valueIndexPairs, 0, nSegmentSize * sizeof(KWValueIndexPairTest));
		}
	}
	return newValueBlock;
}

KWContinuousValueBlockTest::~KWContinuousValueBlockTest()
{
	int i;
	int nSegmentNumber;
	KWValueIndexPairTest* valueIndexPairs;

	// Desallocation des segments de paire (valeur, index) dans le cas de tableaux de grande taille
	if (nValueNumber > nSegmentSize)
	{
		nSegmentNumber = (nValueNumber - 1) / nSegmentSize + 1;
		for (i = nSegmentNumber - 1; i >= 0; i--)
		{
			valueIndexPairs = ((KWValueIndexPairTest**)&(cStartBlock))[i];
			DeleteMemoryBlock(valueIndexPairs);
		}
	}
}

longint KWContinuousValueBlockTest::GetUsedMemory() const
{
	return sizeof(int) + nValueNumber * sizeof(KWValueIndexPairTest);
}

void KWContinuousValueBlockTest::TestPerformance()
{
	boolean bFullTest = false;
	KWContinuousValueBlockTest* valueBlock;
	IntVector ivSizes;
	int nTest;
	int nTotalIter;
	int nMaxSize = 100000;
	int nSize;
	int nTimes;
	int i;
	int nIndex;
	double dExpectedTotal;
	double dTotal;
	Timer timer;
	longint lEmptySize;
	longint lSize;
	ALString sTmp;

	// Nombre total d'iteration par test (plus petit en debug)
	nTotalIter = nMaxSize * 100;
	debug(nTotalIter = nMaxSize);
	cout << "Segment size\t" << nSegmentSize << endl;
	cout << "Total iter\t" << nTotalIter << endl;

	// Initialisation de la famille de test
	// Cas de test de toutes les tailles
	if (bFullTest)
	{
		nSize = 1;
		while (nSize <= nMaxSize)
		{
			ivSizes.Add(nSize);
			nSize += 1;
		}
	}
	// Cas de test massifs
	else
	{
		nSize = 1;
		while (nSize <= nMaxSize)
		{
			ivSizes.Add(nSize);
			nSize *= 10;
		}
	}

	// Parcours de tous les test
	for (nTest = 0; nTest < ivSizes.GetSize(); nTest++)
	{
		nSize = ivSizes.GetAt(nTest);

		// En test complet, on ne fait qu'une iteration par taille
		if (bFullTest)
			nTimes = 1;
		// Sinon, on ajuste selon le nombre total d'iterations
		else
			nTimes = nTotalIter / nSize;

		// Affichage de l'entete des resultats
		if (nTest == 0)
			cout << "Size\tEmpty size\tItem size\tCreation time\tRead time\tIndex check\n";

		// Statistique preliminaires
		valueBlock = NewValueBlock(0);
		lEmptySize = valueBlock->GetUsedMemory();
		cout << nSize << "\t";
		cout << lEmptySize << "\t";

		// Creation de cles dans le bloc
		timer.Reset();
		timer.Start();
		for (i = 0; i < nTimes; i++)
		{
			delete valueBlock;
			valueBlock = NewValueBlock(nSize);
		}
		timer.Stop();
		lSize = valueBlock->GetUsedMemory();
		cout << int((lSize - lEmptySize) * 10 / nSize) / 10.0 << "\t";
		cout << (timer.GetElapsedTime() / (nTimes * nSize)) * 1000000000 << "\t";

		// Initialisation du dernier bloc en cours
		for (nIndex = 0; nIndex < nSize; nIndex++)
		{
			valueBlock->SetAttributeSparseIndexAt(nIndex, nIndex + 1);
			valueBlock->SetValueAt(nIndex, nIndex + 1);
		}

		// Creation de cles dans le bloc
		timer.Reset();
		timer.Start();
		dExpectedTotal = nTimes * (valueBlock->GetValueNumber() + 1.0) * valueBlock->GetValueNumber();
		dTotal = 0;
		for (i = 0; i < nTimes; i++)
		{
			for (nIndex = 0; nIndex < valueBlock->GetValueNumber(); nIndex++)
			{
				dTotal += valueBlock->GetAttributeSparseIndexAt(nIndex);
				dTotal += valueBlock->GetValueAt(nIndex);
			}
		}
		timer.Stop();
		cout << (timer.GetElapsedTime() / (nTimes * nSize)) * 1000000000 << "\t";
		cout << dTotal - dExpectedTotal << endl;
		delete valueBlock;
	}
}