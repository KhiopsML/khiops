// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "StreamTest.h"

#ifdef _WIN32
#include <fcntl.h>
#endif // _WIN32

// Visual C++: supression des Warning sur les assignation au sein d'un expression conditionnelle (cf.
// TestStreamLineCount7 et TestStreamLineCount8)
#ifdef __MSC__
#pragma warning(disable : 4706) // disable C4706 warning
#endif                          // __MSC__

void StreamTest(int argc, char** argv)
{
	if (argc != 2)
		cout << "Il faut un unique nom de fichier en parametre" << endl;
	else
	{
		TestStreamLineCount8(argv[1]);
		TestStreamLineCount7(argv[1]);
		TestStreamLineCount6(argv[1]);
		TestStreamLineCount5(argv[1]);
		TestStreamLineCount4(argv[1]);
		TestStreamLineCount3(argv[1]);
		TestStreamLineCount2(argv[1]);
		TestStreamLineCount1(argv[1]);
	}
}

void TestStreamEndLine()
{
	boolean bOk;
	const int nBufferSize = 16384;
	char cBuffer[nBufferSize + 1];
	FILE* fpBinaryOut;
	int nLineNumber;
	clock_t tBegin;
	clock_t tEnd;
	double dTotalComputeTime;

	tBegin = clock();
	fpBinaryOut = p_fopen("test.txt", "wb");
	bOk = fpBinaryOut != NULL;
	nLineNumber = 0;
	if (bOk)
	{
		p_strcpy(cBuffer, "Bonjour\r\nCoucou\r\n");
		fwrite(cBuffer, sizeof(char), strlen(cBuffer), fpBinaryOut);
		fclose(fpBinaryOut);
	}
	tEnd = clock();
	dTotalComputeTime = (double)(tEnd - tBegin) / CLOCKS_PER_SEC;
}

// Implementation avec fichier texte
void TestStreamLineCount1(const ALString& sFileName)
{
	boolean bOk;
	fstream fst;
	int nLineNumber;
	clock_t tBegin;
	clock_t tEnd;
	double dTotalComputeTime;

	tBegin = clock();
	bOk = FileService::OpenInputFile(sFileName, fst);
	nLineNumber = 0;
	if (bOk)
	{
		while (not fst.eof())
		{
			if (fst.get() == '\n')
				nLineNumber++;
		}
		fst.close();
	}
	tEnd = clock();
	dTotalComputeTime = (double)(tEnd - tBegin) / CLOCKS_PER_SEC;

	// Affichage du resultat
	cout << "TextFile(RT1)\t" << sFileName << "\t" << nLineNumber << "\t" << SecondsToString(dTotalComputeTime)
	     << endl;
}

// Implementation avec fichier binaire
void TestStreamLineCount2(const ALString& sFileName)
{
	boolean bOk;
	fstream fst;
	int nLineNumber;
	clock_t tBegin;
	clock_t tEnd;
	double dTotalComputeTime;

	tBegin = clock();
	fst.open(sFileName, ios::in | ios::binary);
	bOk = fst.is_open();
	nLineNumber = 0;
	if (bOk)
	{
		while (not fst.eof())
		{
			if (fst.get() == '\n')
				nLineNumber++;
		}
		fst.close();
	}
	tEnd = clock();
	dTotalComputeTime = (double)(tEnd - tBegin) / CLOCKS_PER_SEC;

	// Affichage du resultat
	cout << "BinaryFile(RB2)\t" << sFileName << "\t" << nLineNumber << "\t" << SecondsToString(dTotalComputeTime)
	     << endl;
}

// Implementation avec fichier binaire et buffer de lecture
void TestStreamLineCount3(const ALString& sFileName)
{
	boolean bOk;
	const int nBufferSize = 16384;
	char sBuffer[nBufferSize + 1];
	char* p;
	int i;
	int nByteReadNumber;
	fstream fst;
	int nLineNumber;
	clock_t tBegin;
	clock_t tEnd;
	double dTotalComputeTime;

	tBegin = clock();
	fst.open(sFileName, ios::in | ios::binary);
	bOk = fst.is_open();
	nLineNumber = 0;
	if (bOk)
	{
		while (not fst.eof())
		{
			fst.read(sBuffer, nBufferSize);

			// Recherche du nombre de butes lus
			if (not fst.eof())
				nByteReadNumber = nBufferSize;
			else
			{
				nByteReadNumber = 0;
				p = sBuffer;
				while (*p != '\0')
				{
					p++;
					nByteReadNumber++;
				}
			}

			// Recherche des retours chariots
			p = sBuffer;
			i = 0;
			while (i < nByteReadNumber)
			{
				if (*p == '\n')
					nLineNumber++;
				p++;
				i++;
			}
		}
		fst.close();
	}
	tEnd = clock();
	dTotalComputeTime = (double)(tEnd - tBegin) / CLOCKS_PER_SEC;

	// Affichage du resultat
	cout << "BinaryFile(RB3)\t" << sFileName << "\t" << nLineNumber << "\t" << SecondsToString(dTotalComputeTime)
	     << endl;
}

// Implementation avec fichier binaire et buffer de fichier
void TestStreamLineCount4(const ALString& sFileName)
{
	boolean bOk;
	// const int nBufferSize = 16384;
	fstream fst;
	int nLineNumber;
	clock_t tBegin;
	clock_t tEnd;
	double dTotalComputeTime;

	tBegin = clock();
	// fst.rdbuf()->setbuf(sBuffer, nBufferSize);
	fst.open(sFileName, ios::in | ios::binary);
	bOk = fst.is_open();
	nLineNumber = 0;
	if (bOk)
	{
		while (not fst.eof())
		{
			if (fst.get() == '\n')
				nLineNumber++;
		}
		fst.close();
	}
	tEnd = clock();
	dTotalComputeTime = (double)(tEnd - tBegin) / CLOCKS_PER_SEC;

	// Affichage du resultat
	cout << "BinaryFile(FB4)\t" << sFileName << "\t" << nLineNumber << "\t" << SecondsToString(dTotalComputeTime)
	     << endl;
}

// Implementation avec fichier binaire et API C
void TestStreamLineCount5(const ALString& sFileName)
{
	boolean bOk;
	const int nBufferSize = 16384;
	char cBuffer[nBufferSize];
	char* p;
	int i;
	int nByteReadNumber;
	FILE* fpBinaryIn;
	int nLineNumber;
	clock_t tBegin;
	clock_t tEnd;
	double dTotalComputeTime;

	tBegin = clock();
	fpBinaryIn = p_fopen(sFileName, "rb");
	bOk = fpBinaryIn != NULL;
	nLineNumber = 0;
	if (bOk)
	{
		while ((nByteReadNumber = (int)fread(cBuffer, sizeof(char), sizeof(cBuffer), fpBinaryIn)) > 0)
		{
			// Recherche des retours chariots
			p = cBuffer;
			i = 0;
			while (i < nByteReadNumber)
			{
				if (*p == '\n')
					nLineNumber++;
				p++;
				i++;
			}
		}
		fclose(fpBinaryIn);
	}
	tEnd = clock();
	dTotalComputeTime = (double)(tEnd - tBegin) / CLOCKS_PER_SEC;

	// Affichage du resultat
	cout << "BinaryFile(RB5)\t" << sFileName << "\t" << nLineNumber << "\t" << SecondsToString(dTotalComputeTime)
	     << endl;
}

// Implementation avec fichier binaire et API C
void TestStreamLineCount6(const ALString& sFileName)
{
	boolean bOk;
	const int nBufferSize = 16384;
	char cBuffer[nBufferSize + 1];
	char* p;
	int nByteReadNumber;
	FILE* fpBinaryIn;
	int nLineNumber;
	clock_t tBegin;
	clock_t tEnd;
	double dTotalComputeTime;

	tBegin = clock();
	fpBinaryIn = p_fopen(sFileName, "rb");
	bOk = fpBinaryIn != NULL;
	nLineNumber = 0;
	if (bOk)
	{
		while ((nByteReadNumber = (int)fread(cBuffer, sizeof(char), nBufferSize, fpBinaryIn)) > 0)
		{
			// Recherche des retours chariots
			cBuffer[nByteReadNumber + 1] = '\0';
			p = cBuffer;
			while ((p = strchr(p, '\n')) != NULL)
			{
				nLineNumber++;
				p++;
			}
		}
		fclose(fpBinaryIn);
	}
	tEnd = clock();
	dTotalComputeTime = (double)(tEnd - tBegin) / CLOCKS_PER_SEC;

	// Affichage du resultat
	cout << "BinaryFile(RB6)\t" << sFileName << "\t" << nLineNumber << "\t" << SecondsToString(dTotalComputeTime)
	     << endl;
}

// Implementation avec fichier binaire et API C
void TestStreamLineCount7(const ALString& sFileName)
{
	boolean bOk;
	const int nBufferSize = 16384;
	char cBuffer[nBufferSize + 1];
	int nByteReadNumber;
	FILE* fpBinaryIn;
	int nLineNumber;
	clock_t tBegin;
	clock_t tEnd;
	double dTotalComputeTime;

	tBegin = clock();
	fpBinaryIn = p_fopen(sFileName, "rb");
	bOk = fpBinaryIn != NULL;
	nLineNumber = 0;
	if (bOk)
	{
		while ((nByteReadNumber = (int)fread(cBuffer, sizeof(char), nBufferSize, fpBinaryIn)) > 0)
		{
			char* p = cBuffer;

			while ((p = (char*)memchr(p, '\n', (cBuffer + nByteReadNumber) - p)))
			{
				++p;
				++nLineNumber;
			}
		}
		fclose(fpBinaryIn);
	}
	tEnd = clock();
	dTotalComputeTime = (double)(tEnd - tBegin) / CLOCKS_PER_SEC;

	// Affichage du resultat
	cout << "BinaryFile(RB7)\t" << sFileName << "\t" << nLineNumber << "\t" << SecondsToString(dTotalComputeTime)
	     << endl;
}

// Implementation avec fichier binaire et API C
void TestStreamLineCount8(const ALString& sFileName)
{
	// Methode specifique Windows pour etude technique (le portage Linux n'est pas necessaire)
#ifdef _WIN32
	boolean bOk;
	const int nBufferSize = 16384;
	char cBuffer[nBufferSize + 1];
	int nByteReadNumber;
	int nBinaryIn;
	int nLineNumber;
	clock_t tBegin;
	clock_t tEnd;
	double dTotalComputeTime;

	tBegin = clock();

	nBinaryIn = _open(sFileName, _O_BINARY | _O_RDONLY);
	bOk = nBinaryIn != -1;
	nLineNumber = 0;
	if (bOk)
	{
		while ((nByteReadNumber = _read(nBinaryIn, cBuffer, nBufferSize)) > 0)
		{
			char* p = cBuffer;

			while ((p = (char*)memchr(p, '\n', (cBuffer + nByteReadNumber) - p)))
			{
				++p;
				++nLineNumber;
			}
		}
		_close(nBinaryIn);
	}
	tEnd = clock();
	dTotalComputeTime = (double)(tEnd - tBegin) / CLOCKS_PER_SEC;

	// Affichage du resultat
	cout << "BinaryFile(RB8)\t" << sFileName << "\t" << nLineNumber << "\t" << SecondsToString(dTotalComputeTime)
	     << endl;
#endif // _WIN32
}
