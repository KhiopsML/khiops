// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "BaseTest.h"

///////////////////////////////////////////////////////
// Classe TestBaseComponents

TestBaseComponents::TestBaseComponents()
{
	SetIdentifier("TestBaseComponents");
	SetLabel("Test des composants de base");

	// Declaration des actions
	AddAction("SystemResourceTest", "Test System Resource", (ActionMethod)&TestBaseComponents::SystemResourceTest);
	AddAction("MemTest", "Test de la gestion de la memoire", (ActionMethod)&TestBaseComponents::MemTest);
	AddAction("RandomTest", "Test du generateur de nombre aleatoire",
		  (ActionMethod)&TestBaseComponents::RandomTest);
	AddAction("ErrorTest", "Test de la gestion d'erreur", (ActionMethod)&TestBaseComponents::ErrorTest);
	AddAction("FileServiceTest", "Test des utilitaires de gestion des fichiers",
		  (ActionMethod)&TestBaseComponents::FileServiceTest);
	AddAction("ALStringTest", "Test de manipulation des chaines de caracteres",
		  (ActionMethod)&TestBaseComponents::ALStringTest);
	AddAction("ALRegexTest", "Test des regex", (ActionMethod)&TestBaseComponents::ALRegexTest);
	AddAction("TimerTest", "Test de timer", (ActionMethod)&TestBaseComponents::TimerTest);
	AddAction("ObjectArrayTest", "Test des tableaux d'objects", (ActionMethod)&TestBaseComponents::ObjectArrayTest);
	AddAction("ObjectListTest", "Test des listes d'objects", (ActionMethod)&TestBaseComponents::ObjectListTest);
	AddAction("SortedListTest", "Test des listes triees d'objects",
		  (ActionMethod)&TestBaseComponents::SortedListTest);
	AddAction("ObjectDictionaryTest", "Test des dictionaires d'objects",
		  (ActionMethod)&TestBaseComponents::ObjectDictionaryTest);
	AddAction("NumericKeyDictionaryTest", "Test des dictionaires d'objects a cle numerique",
		  (ActionMethod)&TestBaseComponents::NumericKeyDictionaryTest);
	AddAction("DoubleVectorTest", "Test des vecteurs de doubles",
		  (ActionMethod)&TestBaseComponents::DoubleVectorTest);
	AddAction("IntVectorTest", "Test des vecteurs d'entiers", (ActionMethod)&TestBaseComponents::IntVectorTest);
	AddAction("LongintVectorTest", "Test des vecteurs d'entiers longs",
		  (ActionMethod)&TestBaseComponents::LongintVectorTest);
	AddAction("CharVectorTest", "Test des vecteurs de caracteres",
		  (ActionMethod)&TestBaseComponents::CharVectorTest);
	AddAction("StringVectorTest", "Test des vecteurs de chaines de caracteres",
		  (ActionMethod)&TestBaseComponents::StringVectorTest);
	AddAction("UserInterfaceTest", "Test de l'interface utilisateur",
		  (ActionMethod)&TestBaseComponents::UserInterfaceTest);
	AddAction("UserInterfaceTestView", "Test UI avec des View",
		  (ActionMethod)&TestBaseComponents::UserInterfaceTestView);
	AddAction("UserInterfaceTestFileChooser", "Test UI avec action de menu file chooser",
		  (ActionMethod)&TestBaseComponents::UserInterfaceTestFileChooser);
	AddAction("FileReaderCardTest", "Test UI avec fiche de lecture de fichier",
		  (ActionMethod)&TestBaseComponents::FileReaderCardTest);
}

TestBaseComponents::~TestBaseComponents()
{
	oaUITestObjects.DeleteAll();
}

void TestBaseComponents::SystemResourceTest()
{
	// Ressources systeme
	cout << "SYS MAC Address\t" << GetMACAddress() << endl;
	cout << "SYS machine GUID " << GetMachineGUID() << endl;
	cout << "SYS RESOURCE\tProcessor number\t" << SystemGetProcessorNumber() << "\n";
	cout << "SYS RESOURCE\tDisk Free Space on current directory\t"
	     << LongintToHumanReadableString(DiskGetFreeSpace(".")) << "\n";
	cout << "SYS RESOURCE\tCurrent timestamp\t" << CurrentTimestamp() << "\n";
	cout << "SYS RESOURCE\tCurrent precise timestamp\t" << CurrentPreciseTimestamp() << "\n";
	cout << "Sleep(0.1)\n";
	SystemSleep(0.1);
	cout << "SYS RESOURCE\tCurrent precise timestamp\t" << CurrentPreciseTimestamp() << "\n";
}

void TestBaseComponents::MemTest()
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
		AddMessage("Create VM");

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
		cout << "Creation of segments\t" << nIter << "\t" << nAllocNumber << "\t" << timer.GetElapsedTime()
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
		cout << "Deletion of segments\t" << nIter << "\t" << nAllocNumber << "\t" << timer.GetElapsedTime()
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
		cout << "Creation of segments\t" << nIter << "\t" << nAllocNumber << "\t" << timer.GetElapsedTime()
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
		cout << "Bloc deletion of segments\t" << nIter << "\t" << nAllocNumber << "\t" << timer.GetElapsedTime()
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
		cout << "Creation of segments\t" << nIter << "\t" << nAllocNumber << "\t" << timer.GetElapsedTime()
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
		cout << "Reverse deletion of segments\t" << nIter << "\t" << nAllocNumber << "\t"
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
		cout << "Creation of segments\t" << nIter << "\t" << nAllocNumber << "\t" << timer.GetElapsedTime()
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
		cout << "Random deletion of segments\t" << nIter << "\t" << nAllocNumber << "\t"
		     << timer.GetElapsedTime() << "\t" << LongintToHumanReadableString(MemGetHeapMemory()) << "\t"
		     << LongintToHumanReadableString(MemGetCurrentProcessVirtualMemory()) << endl;
	}
}

//  Test du generateur aleatoire
void TestBaseComponents::RandomTest()
{
	int nStartClock;
	int nStopClock;
	int nNumber = 100000000;
	int i;
	longint lRandomMean;
	double dMeanRandom;

	SetRandomSeed(1);

	// Random int standard
	nStartClock = clock();
	lRandomMean = 0;
	for (i = 0; i < nNumber; i++)
	{
		lRandomMean += RandomInt(1000000000);
	}
	dMeanRandom = lRandomMean * 1.0 / nNumber;
	nStopClock = clock();
	cout << "Standard random int mean\t" << dMeanRandom << "\n";
	cout << "SYS TIME\tStandard random int\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";

	// Random double standard
	nStartClock = clock();
	dMeanRandom = 0;
	for (i = 0; i < nNumber; i++)
	{
		dMeanRandom += RandomDouble();
	}
	dMeanRandom = dMeanRandom / nNumber;
	nStopClock = clock();
	cout << "Standard random double mean\t" << dMeanRandom << "\n";
	cout << "SYS TIME\tStandard random double\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";

	// Ith random int
	nStartClock = clock();
	lRandomMean = 0;
	for (i = 0; i < nNumber; i++)
	{
		lRandomMean += IthRandomInt(i, 1000000000);
	}
	dMeanRandom = lRandomMean * 1.0 / nNumber;
	nStopClock = clock();
	cout << "Standard ith random int mean\t" << dMeanRandom << "\n";
	cout << "SYS TIME\tStandard ith random int\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";

	// Ith random longint
	nStartClock = clock();
	lRandomMean = 0;
	for (i = 0; i < nNumber; i++)
	{
		lRandomMean += abs(IthRandomLongint(i)) % 1000000000;
	}
	lRandomMean /= nNumber;
	nStopClock = clock();
	cout << "Standard ith random longint mean\t" << dMeanRandom << "\n";
	cout << "SYS TIME\tStandard ith random longint\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC
	     << "\n\n";

	// Ith random double
	nStartClock = clock();
	dMeanRandom = 0;
	for (i = 0; i < nNumber; i++)
	{
		dMeanRandom += IthRandomDouble(i);
	}
	dMeanRandom /= nNumber;
	nStopClock = clock();
	cout << "Standard ith random double mean\t" << dMeanRandom << "\n";
	cout << "SYS TIME\tStandard ith random double\t" << (nStopClock - nStartClock) * 1.0 / CLOCKS_PER_SEC << "\n\n";
}

void TestBaseComponents::ErrorTest()
{
	Error::Test();
}

void TestBaseComponents::FileServiceTest()
{
	FileService::Test();
}

void TestBaseComponents::ALStringTest()
{
	ALString::Test();
}

void TestBaseComponents::ALRegexTest()
{
	Regex::Test();
}

void TestBaseComponents::TimerTest()
{
	Timer::Test();
}

void TestBaseComponents::ObjectArrayTest()
{
	ObjectArray::Test();
}

void TestBaseComponents::ObjectListTest()
{
	ObjectList::Test();
}

void TestBaseComponents::SortedListTest()
{
	SortedList::Test();
}

void TestBaseComponents::ObjectDictionaryTest()
{
	ObjectDictionary::Test();
}

void TestBaseComponents::NumericKeyDictionaryTest()
{
	NumericKeyDictionary::Test();
}

void TestBaseComponents::DoubleVectorTest()
{
	DoubleVector::Test();
}

void TestBaseComponents::IntVectorTest()
{
	IntVector::Test();
}

void TestBaseComponents::LongintVectorTest()
{
	LongintVector::Test();
}

void TestBaseComponents::CharVectorTest()
{
	CharVector::Test();
}

void TestBaseComponents::StringVectorTest()
{
	StringVector::Test();
}

void TestBaseComponents::UserInterfaceTest()
{
	UITest test;
	UIConfirmationCard confirmationCard;
	boolean bContinue;

	// On reste dans l'interface tant que l'on ne confirme pas la sortie
	UIObject::SetUIMode(UIObject::Graphic);
	bContinue = true;
	while (bContinue)
	{
		test.Open();
		bContinue = not confirmationCard.OpenAndConfirm();
	}
	UIObject::SetUIMode(UIObject::Textual);
}

void TestBaseComponents::UserInterfaceTestView()
{
	UITestClassSpecArrayView testArrayView;

	UIObject::SetUIMode(UIObject::Graphic);
	testArrayView.SetObjectArray(&oaUITestObjects);
	testArrayView.SetLineNumber(7);
	testArrayView.Open();
	UIObject::SetUIMode(UIObject::Textual);
}

void TestBaseComponents::UserInterfaceTestFileChooser()
{
	boolean bReOpen = false;
	UITestObject testObject;
	UITestObjectView testObjectView;

	// Parametrage de a fiche d'interface
	UIObject::SetUIMode(UIObject::Graphic);
	testObjectView.SetObject(&testObject);
	testObjectView.Check();

	// Ouverture en testant la synchornisation avec Java
	cout << "TestObjectView Start" << endl;
	cout << "Input object\n" << testObject << *testObject.GetSubObject() << *testObject.GetActionSubObject();
	testObjectView.Open();
	cout << "Output object\n" << testObject << *testObject.GetSubObject() << *testObject.GetActionSubObject();
	cout << "TestObjectView Stop" << endl;

	// Re-ouverture en testant la synchronisation avec Java
	if (bReOpen)
	{
		cout << "=================================================" << endl;
		cout << "TestObjectView Start2" << endl;
		testObjectView.Open();
		cout << "TestObjectView Stop2" << endl;
	}
	UIObject::SetUIMode(UIObject::Textual);
}

void TestBaseComponents::FileReaderCardTest()
{
	FileReaderCard fileReaderCard;

	UIObject::SetUIMode(UIObject::Graphic);
	fileReaderCard.Open();
	UIObject::SetUIMode(UIObject::Textual);
}

void TestBaseComponents::InteractiveTest()
{
	TestBaseComponents tests;

	UIObject::SetUIMode(UIObject::Textual);
	UIObject::SetTextualInteractiveModeAllowed(true);
	Error::SetDisplayErrorFunction(Error::GetDefaultDisplayErrorFunction());
	tests.Open();
}

void TestBaseComponents::BatchTest()
{
	TestBaseComponents tests;
	const ALString sHeader = "=========================================================\n";
	UIAction* action;
	int i;

	// On passe en mode textuel
	UIObject::SetUIMode(UIObject::Textual);

	// On passe en mode batch pour avoir des parametres par defaut, sans interaction utilisateur
	SetAcquireBatchMode(true);

	// Lancement de tous les tests
	for (i = 0; i < tests.GetActionNumber(); i++)
	{
		action = tests.GetActionAtIndex(i);

		// On saute les action predefinies
		if (action->GetIdentifier() == "Exit" or action->GetIdentifier() == "Refresh")
			continue;

		// Arret si premiere action avec interface
		if (action->GetIdentifier().Find("View") >= 0 or action->GetIdentifier().Find("UserInterface") >= 0)
			break;

		// Execution de l'action
		cout << sHeader << action->GetLabel() << endl;
		MemoryStatsManager::AddLog("Start " + action->GetLabel());
		(tests.*action->GetActionMethod())();
		MemoryStatsManager::AddLog("Stop " + action->GetLabel());
	}

	// Restitution des modes standard
	UIObject::SetUIMode(UIObject::Graphic);
	SetAcquireBatchMode(false);
}