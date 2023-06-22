// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DiversTests.h"

void ShowKeyInfo()
{
	time_t lt;

	lt = time(NULL);
	cout << "=========================================================================" << endl;
	cout << ctime(&lt) << endl;
	cout << "OS: " << p_getenv("OS") << endl;
	cout << "PROCESSOR_IDENTIFIER: " << p_getenv("PROCESSOR_IDENTIFIER") << endl;
	cout << "PROCESSOR_REVISION: " << p_getenv("PROCESSOR_REVISION") << endl;
	cout << "MAC Address: " << GetMACAddress() << endl;
	cout << "Computer name: " << LMLicenseService::GetComputerName() << endl;
	cout << "Machine ID: " << LMLicenseService::GetMachineID() << endl;
}

void TestMemAdvanced()
{
	ObjectArray oaTest;
	int i;
	ALString sTest =
	    "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
	StringObject soTest;
	int nMaxAlloc = 10000;

	soTest.SetString(sTest);

	// Affichage des stats sur la heap
	cout << "MemGetHeapMemory1: " << MemGetHeapMemory() << endl;
	MemPrintHeapStats(stdout);
	for (i = 0; i < nMaxAlloc; i++)
		oaTest.Add(soTest.Clone());
	cout << "MemGetHeapMemory2: " << MemGetHeapMemory() << endl;
	MemPrintHeapStats(stdout);
	for (i = 0; i < nMaxAlloc / 10; i++)
	{
		delete oaTest.GetAt(i * 10);
		oaTest.SetAt(i * 10, NULL);
	}
	cout << "MemGetHeapMemory3: " << MemGetHeapMemory() << endl;
	MemPrintHeapStats(stdout);
	oaTest.DeleteAll();
	cout << "MemGetHeapMemory4: " << MemGetHeapMemory() << endl;
	MemPrintHeapStats(stdout);
}

void TestBigFile()
{
	CharVector cvTest;
	InputBufferedFile inputFile1;
	OutputBufferedFile outputFile1;
	InputBufferedFile inputFile2;
	OutputBufferedFile outputFile2;
	OutputBufferedFile outputFile;
	char cIn;
	Timer timer;
	boolean bTestCreate = true;
	boolean bTestIO = true;
	int i;
	char* sField;
	int nFieldError;
	longint lBeginPos;
	int nBufferNumber;
	boolean bEol;

	if (bTestCreate)
	{
		cvTest.SetSize(1024);
		for (i = 0; i < cvTest.GetSize(); i++)
		{
			if (i % 1024 == 0)
				cvTest.SetAt(i, '\n');
			else if (i % 8 == 0)
				cvTest.SetAt(i, '\t');
			else
				cvTest.SetAt(i, '.');
		}

		// Creation du fichier
		timer.Start();
		outputFile1.SetFileName("c:/temp/BigFileIn1.txt");
		outputFile1.Open();
		outputFile2.SetFileName("c:/temp/BigFileIn2.txt");
		outputFile2.Open();
		for (i = 0; i < 1048576; i++)
		{
			outputFile1.Write(&cvTest);
			outputFile2.Write(&cvTest);
		}
		outputFile1.Close();
		outputFile2.Close();
		timer.Stop();
		cout << "Creation time: " << timer.GetElapsedTime() << endl;
	}

	if (bTestIO)
	{
		// Lecture/ecriture du fichier
		timer.Start();
		inputFile1.SetFileName("c:/temp/BigFileIn1.txt");
		inputFile2.SetFileName("c:/temp/BigFileIn2.txt");
		outputFile.SetFileName("c:/temp/BigFileOut.txt");
		inputFile1.Open();
		inputFile2.Open();
		outputFile.Open();
		nBufferNumber = 0;

		//
		lBeginPos = 0;
		while (not inputFile1.IsLastBuffer())
		{
			// Lecture d'un buffer
			inputFile1.Fill(lBeginPos);
			lBeginPos += inputFile1.GetBufferSize();
			nBufferNumber++;

			// Analyse du buffer
			while (not inputFile1.IsBufferEnd())
			{
				bEol = inputFile1.GetNextField(sField, nFieldError);
				outputFile.Write(sField);
				if (bEol)
					outputFile.Write('\n');
				else
					outputFile.Write('\t');
			}
		}

		//
		lBeginPos = 0;
		while (not inputFile2.IsLastBuffer())
		{
			// Lecture d'un buffer
			inputFile2.Fill(lBeginPos);
			lBeginPos += inputFile2.GetBufferSize();
			nBufferNumber++;

			// Analyse du buffer
			while (not inputFile2.IsBufferEnd())
			{
				bEol = inputFile2.GetNextField(sField, nFieldError);
				outputFile.Write(sField);
				if (bEol)
					outputFile.Write('\n');
				else
					outputFile.Write('\t');
			}
		}

		//
		inputFile1.Close();
		inputFile2.Close();
		outputFile.Close();
		timer.Stop();
		cout << "Buffers: " << nBufferNumber << endl;
		cout << "Read/write time: " << timer.GetElapsedTime() << endl;
	}
	cin >> cIn;
}

///////////////////////////////////////////////////////
// Classe avec ses propre methode d'allocation
class MemoryTest
{
public:
	// Constructeur
	MemoryTest()
	{
		cout << "MemoryTest()\n";
	};
	~MemoryTest()
	{
		cout << "~MemoryTest()\n";
	};

	// Allocateur
	void* operator new(size_t sz)
	{
		cout << "new MemoryTest " << sz << " bytes\n";
		return ::operator new(sz);
	}

	void operator delete(void* p)
	{
		cout << "delete MemoryTest\n";
		::operator delete(p);
	}

protected:
	int nTest;
};

class SubMemoryTest : public MemoryTest
{
public:
	// Constructeur
	SubMemoryTest()
	{
		cout << "SubMemoryTest()\n";
	};
	~SubMemoryTest()
	{
		cout << "~SubMemoryTest()\n";
	};

protected:
	ALString sTest;
};

void StudyMemoryManagement()
{
	MemoryTest memTest1;
	MemoryTest* memTest2;
	SubMemoryTest* subMemTest;

	cout << "MemoryTest: " << sizeof(MemoryTest) << endl;
	memTest2 = new MemoryTest;
	delete memTest2;
	subMemTest = new SubMemoryTest;
	delete subMemTest;
}
