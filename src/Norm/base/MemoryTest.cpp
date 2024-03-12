// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MemoryTest.h"
#include "Timer.h"
#include "CharVector.h"
#include "Vector.h"

void TestMemory()
{
	boolean bKeepHandler;
	boolean bHugeAlloc;
	const int nSegmentSize = MemSegmentByteSize * 1;
	int nAllocSize;
	int nAllocNumber;
	int nAlloc;
	int nIterNumber;
	int nIter;
	int i;
	int j;
	int nCorrectAllocNumber;
	ObjectArray oaAllocatedObjects;
	IntVector* ivVector;
	CharVector* cvVector;
	boolean bDrawDots;
	boolean bUse;
	boolean bFree;
	Timer timer;
	int nBenchmarkMemoryPercentage;
	double dBenchmarkMemory;

	// Affichage des statistiques sur la memoire utilisable
	cout << "SYS MEMORY\tMemoire physique theorique adressable\t"
	     << LongintToHumanReadableString(MemGetAdressablePhysicalMemory()) << endl;
	cout << "SYS MEMORY\tMemoire physique totale\t" << LongintToHumanReadableString(MemGetAvailablePhysicalMemory())
	     << endl;
	cout << "SYS MEMORY\tMemoire physique disponible\t" << LongintToHumanReadableString(MemGetFreePhysicalMemory())
	     << endl;
	cout << "SYS MEMORY\tMemoire virtuelle utilisee\t"
	     << LongintToHumanReadableString(MemGetCurrentProcessVirtualMemory()) << endl;
	cout << "SYS MEMORY\tMemoire heap utilisee\t" << LongintToHumanReadableString(MemGetHeapMemory()) << endl;

	// Changement du handler
	bKeepHandler = AcquireBoolean("Keep default alloc error handler", true);
	if (not bKeepHandler)
		MemSetAllocErrorHandler(NULL);

	// Test d'erreur
	bHugeAlloc = AcquireBoolean("Huge alloc (may cause a fatal error)", false);
	if (bHugeAlloc)
	{
		cout << "Create VM" << endl;

		// Boucle infini de creation memoire
		while (bHugeAlloc == true)
			SystemObject::NewIntArray(RandomInt(10) + 1);
	}

	// Test d'allocations
	cout << "Test of alloc of large vectors\n";
	nAllocSize = AcquireRangedInt("Size of vectors", 1, 100000000, 1000);
	nAllocNumber = AcquireRangedInt("Alloc number", 0, 10000000, 1000);
	bDrawDots = AcquireBoolean("Print progression dots", false);
	timer.Reset();
	timer.Start();
	nCorrectAllocNumber = 0;
	oaAllocatedObjects.SetSize(nAllocNumber);
	for (nAlloc = 0; nAlloc < nAllocNumber; nAlloc++)
	{
		ivVector = new IntVector;
		ivVector->SetLargeSize(nAllocSize);
		oaAllocatedObjects.SetAt(nAlloc, ivVector);
		if (ivVector->GetSize() == nAllocSize)
		{
			nCorrectAllocNumber++;
			if (bDrawDots)
				cout << "." << flush;
		}
		else
		{
			oaAllocatedObjects.SetSize(nAlloc + 1);
			cout << "!" << ivVector->GetSize() << "!" << flush;
			break;
		}
	}
	timer.Stop();
	if (bDrawDots)
		cout << endl;
	cout << "Allocated vectors:\t" << nCorrectAllocNumber << endl;
	cout << "SYS TIME\tAllocation\t" << timer.GetElapsedTime() << "\n\n";

	// Utilisation des vecteurs en acces sequentiel
	bUse = AcquireBoolean("Sequential use of allocated vectors", true);
	if (bUse)
	{
		timer.Reset();
		timer.Start();
		for (nAlloc = 0; nAlloc < oaAllocatedObjects.GetSize(); nAlloc++)
		{
			ivVector = cast(IntVector*, oaAllocatedObjects.GetAt(nAlloc));
			for (i = 0; i < ivVector->GetSize(); i++)
				ivVector->SetAt(i, 1);
			if (bDrawDots)
				cout << "." << flush;
		}
		timer.Stop();
		if (bDrawDots)
			cout << endl;
		cout << "SYS TIME\tSequential use of vectors\t" << timer.GetElapsedTime() << "\n\n";
	}

	// Utilisation des vecteurs en acces aleatoire
	bUse = AcquireBoolean("Random use of allocated vectors", true);
	if (bUse)
	{
		timer.Reset();
		timer.Start();
		for (nAlloc = 0; nAlloc < oaAllocatedObjects.GetSize(); nAlloc++)
		{
			ivVector =
			    cast(IntVector*, oaAllocatedObjects.GetAt(RandomInt(oaAllocatedObjects.GetSize() - 1)));
			for (i = 0; i < ivVector->GetSize(); i++)
				ivVector->SetAt(RandomInt(ivVector->GetSize() - 1), 1);
			if (bDrawDots)
				cout << "." << flush;
		}
		timer.Stop();
		if (bDrawDots)
			cout << endl;
		cout << "SYS TIME\tRandom use of vectors\t" << timer.GetElapsedTime() << "\n\n";
	}

	// Desallocation
	bFree = AcquireBoolean("Free memory", true);
	if (bFree)
	{
		timer.Reset();
		timer.Start();
		oaAllocatedObjects.DeleteAll();
		timer.Stop();
		cout << "SYS TIME\tFree memory\t" << timer.GetElapsedTime() << "\n\n";
	}

	// Benchmark massif d'allocation
	// Test d'allocations
	cout << "Extensive memory benchmark\n";
	nBenchmarkMemoryPercentage = AcquireRangedInt("Percentage of available memory to exploit", 0, 100, 10);
	if (nBenchmarkMemoryPercentage > 0)
	{
		// Calcul de la memoire utilisee pour le benchmark
		dBenchmarkMemory = (MemGetAvailablePhysicalMemory() * 1.0 * nBenchmarkMemoryPercentage / 100.0);
		cout << "SYS MEMORY\tMemoire utilisee pour le benchmark\t" << (int)(dBenchmarkMemory / 1048576) << " Mo"
		     << endl;

		// Commentaire
		cout << "\tAllocation de grands nombres de vecteurs d'entiers" << endl;
		cout << "\t  Size: taille des vecteur d'entier" << endl;
		cout << "\t  Nb: nombre d'allocations demandees" << endl;
		cout << "\t  NbAlloc: nombre d'allocations effectuees" << endl;
		cout << "\t  Memory: taille totale allouee en Mo" << endl;
		cout << "\t  T new: temps d'allocation" << endl;
		cout << "\t  T delete: temps de liberation" << endl;

		// Entete
		cout << "SYS MEMORY\tSize\tNb\tNbAlloc\tMemory\tT new\tT delete" << endl;

		// Initialisation des parametres de benchmark
		nAllocSize = 100;
		while (nAllocSize <= 1000000)
		{
			nAllocNumber = (int)(dBenchmarkMemory / (nAllocSize * sizeof(int)));
			cout << "SYS MEMORY\t" << flush;
			cout << nAllocSize << "\t" << flush;
			cout << nAllocNumber << "\t" << flush;

			// Boucle d'allocation
			timer.Reset();
			timer.Start();
			nCorrectAllocNumber = 0;
			oaAllocatedObjects.SetSize(nAllocNumber);
			for (nAlloc = 0; nAlloc < nAllocNumber; nAlloc++)
			{
				ivVector = new IntVector;
				ivVector->SetLargeSize(nAllocSize);
				oaAllocatedObjects.SetAt(nAlloc, ivVector);
				if (ivVector->GetSize() == nAllocSize)
				{
					nCorrectAllocNumber++;
				}
				else
				{
					oaAllocatedObjects.SetSize(nAlloc + 1);
					cout << "!" << ivVector->GetSize() << "!" << flush;
					break;
				}
			}
			timer.Stop();
			cout << nCorrectAllocNumber << "\t" << flush;
			cout << (int)(nCorrectAllocNumber * 1.0 * nAllocSize * sizeof(int) / (1024 * 1024)) << "\t"
			     << flush;
			cout << timer.GetElapsedTime() << "\t" << flush;

			// Boucle de desallocation
			timer.Reset();
			timer.Start();
			oaAllocatedObjects.DeleteAll();
			timer.Stop();
			cout << timer.GetElapsedTime() << "\n" << flush;

			// Passage a la taille suivante
			nAllocSize *= 2;
		}
	}

	// Test d'allocation de segments
	cout << "Massive segment allocation\n";
	nIterNumber = AcquireRangedInt("Iteration number", 0, 10, 1);
	nAllocNumber = AcquireRangedInt("Alloc number", 0, 10000000, 10000);
	oaAllocatedObjects.SetSize(nAllocNumber);
	for (nIter = 0; nIter < nIterNumber; nIter++)
	{
		timer.Reset();
		timer.Start();
		for (i = 0; i < nAllocNumber; i++)
		{
			cvVector = new CharVector;
			cvVector->SetSize(nSegmentSize);
			oaAllocatedObjects.SetAt(i, cvVector);
		}
		timer.Stop();
		cout << "SYS Creation of segments\t" << nIter << "\t" << nAllocNumber << "\t" << timer.GetElapsedTime()
		     << "\t" << LongintToHumanReadableString(MemGetHeapMemory()) << "\t"
		     << LongintToHumanReadableString(MemGetCurrentProcessVirtualMemory()) << endl;
		timer.Reset();
		timer.Start();
		for (i = 0; i < nAllocNumber; i++)
		{
			cvVector = cast(CharVector*, oaAllocatedObjects.GetAt(i));
			delete cvVector;
		}
		timer.Stop();
		cout << "SYS Deletion of segments\t" << nIter << "\t" << nAllocNumber << "\t" << timer.GetElapsedTime()
		     << "\t" << LongintToHumanReadableString(MemGetHeapMemory()) << "\t"
		     << LongintToHumanReadableString(MemGetCurrentProcessVirtualMemory()) << endl;
		timer.Reset();
		timer.Start();
		for (i = 0; i < nAllocNumber; i++)
		{
			cvVector = new CharVector;
			cvVector->SetSize(nSegmentSize);
			oaAllocatedObjects.SetAt(i, cvVector);
		}
		timer.Stop();
		cout << "SYS Creation of segments\t" << nIter << "\t" << nAllocNumber << "\t" << timer.GetElapsedTime()
		     << "\t" << LongintToHumanReadableString(MemGetHeapMemory()) << "\t"
		     << LongintToHumanReadableString(MemGetCurrentProcessVirtualMemory()) << endl;
		timer.Reset();
		timer.Start();
		for (j = 0; j < 10; j++)
		{
			i = j;
			while (i < nAllocNumber)
			{
				cvVector = cast(CharVector*, oaAllocatedObjects.GetAt(i));
				delete cvVector;
				i += 10;
			}
		}
		timer.Stop();
		cout << "SYS Bloc deletion of segments\t" << nIter << "\t" << nAllocNumber << "\t"
		     << timer.GetElapsedTime() << "\t" << LongintToHumanReadableString(MemGetHeapMemory()) << "\t"
		     << LongintToHumanReadableString(MemGetCurrentProcessVirtualMemory()) << endl;
		timer.Reset();
		timer.Start();
		for (i = 0; i < nAllocNumber; i++)
		{
			cvVector = new CharVector;
			cvVector->SetSize(nSegmentSize);
			oaAllocatedObjects.SetAt(i, cvVector);
		}
		timer.Stop();
		cout << "SYS Creation of segments\t" << nIter << "\t" << nAllocNumber << "\t" << timer.GetElapsedTime()
		     << "\t" << LongintToHumanReadableString(MemGetHeapMemory()) << "\t"
		     << LongintToHumanReadableString(MemGetCurrentProcessVirtualMemory()) << endl;
		timer.Reset();
		timer.Start();
		for (i = nAllocNumber - 1; i >= 0; i--)
		{
			cvVector = cast(CharVector*, oaAllocatedObjects.GetAt(i));
			delete cvVector;
		}
		timer.Stop();
		cout << "SYS Reverse deletion of segments\t" << nIter << "\t" << nAllocNumber << "\t"
		     << timer.GetElapsedTime() << "\t" << LongintToHumanReadableString(MemGetHeapMemory()) << "\t"
		     << LongintToHumanReadableString(MemGetCurrentProcessVirtualMemory()) << endl;
		timer.Reset();
		timer.Start();
		for (i = 0; i < nAllocNumber; i++)
		{
			cvVector = new CharVector;
			cvVector->SetSize(nSegmentSize);
			oaAllocatedObjects.SetAt(i, cvVector);
		}
		timer.Stop();
		cout << "SYS Creation of segments\t" << nIter << "\t" << nAllocNumber << "\t" << timer.GetElapsedTime()
		     << "\t" << LongintToHumanReadableString(MemGetHeapMemory()) << "\t"
		     << LongintToHumanReadableString(MemGetCurrentProcessVirtualMemory()) << endl;
		oaAllocatedObjects.Shuffle();
		timer.Reset();
		timer.Start();
		for (i = 0; i < nAllocNumber; i++)
		{
			cvVector = cast(CharVector*, oaAllocatedObjects.GetAt(i));
			delete cvVector;
		}
		timer.Stop();
		cout << "SYS Random deletion of segments\t" << nIter << "\t" << nAllocNumber << "\t"
		     << timer.GetElapsedTime() << "\t" << LongintToHumanReadableString(MemGetHeapMemory()) << "\t"
		     << LongintToHumanReadableString(MemGetCurrentProcessVirtualMemory()) << endl;
	}
}
