// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#ifdef _MSC_VER
// To disable fopen warnings (Visual C++ deprecated method)
#define _CRT_SECURE_NO_WARNINGS
#endif // _MSC_VER

#include "KNITest.h"
#include "TestServices.h"
#include "../../src/Learning/KNITransfer/KNIRecodeFile.cpp"

#define MAXITER 1000
#define MAXBUFFERSIZE 1000

// Test side effetcts with Iris dataset
void TestSideEffect(const char* sDictionaryFileName, const char* sDictionaryName, const char* sInputFileName)
{
	int nRetCode;
	int hStream;
	const int nMaxIter = 1000;
	int nIter;
	int nIterClose;
	int ivStreamHandles[MAXITER];
	char sHeaderLine[MAXBUFFERSIZE];
	char sOutputRecord[MAXBUFFERSIZE];

	assert(sDictionaryFileName != NULL);
	assert(sDictionaryName != NULL);
	assert(sInputFileName != NULL);

	//////////////////////////////////////////////////////////
	// Test standard

	// Message de debut
	printf("Begin test KNI\n");

	printf("SYS KNI version: %d\n", KNIGetVersion());
	printf("SYS KNI full version: %s\n", KNIGetFullVersion());

	// Open stream
	strcpy(sHeaderLine, "SepalLength	SepalWidth	PetalLength	PetalWidth	Class");
	hStream = KNIOpenStream(sDictionaryFileName, sDictionaryName, sHeaderLine, '\t');
	printf("Open stream %s %s: %d\n", sDictionaryFileName, sDictionaryName, hStream);

	// Recode string records (correct or wrong records)
	if (hStream >= 0)
	{
		// Correct record
		nRetCode = KNIRecodeStreamRecord(hStream, "5.1	3.5	1.4	0.2	Iris-setosa", sOutputRecord,
						 MAXBUFFERSIZE);
		printf("Recode stream record %s: %d\n", sOutputRecord, nRetCode);

		// Correct record
		nRetCode = KNIRecodeStreamRecord(hStream, "5.9	3.0	5.1	1.8	Iris-virginica", sOutputRecord,
						 MAXBUFFERSIZE);
		printf("Recode stream record %s: %d\n", sOutputRecord, nRetCode);

		// Invalid record
		nRetCode = KNIRecodeStreamRecord(hStream, "5.1	3.5	1.4	0.2", sOutputRecord, MAXBUFFERSIZE);
		printf("Recode invalid stream record %s: %d\n", sOutputRecord, nRetCode);

		// Empty record
		nRetCode = KNIRecodeStreamRecord(hStream, "", sOutputRecord, MAXBUFFERSIZE);
		printf("Recode empty stream record %s: %d\n", sOutputRecord, nRetCode);

		// NULL record
		nRetCode = KNIRecodeStreamRecord(hStream, NULL, sOutputRecord, MAXBUFFERSIZE);
		printf("Recode NULL stream record %s: %d\n", sOutputRecord, nRetCode);

		// Output record too short
		nRetCode =
		    KNIRecodeStreamRecord(hStream, "5.9	3.0	5.1	1.8	Iris-virginica", sOutputRecord, 10);
		printf("Recode stream record without enough output length %s: %d\n", sOutputRecord, nRetCode);

		// NULL output record
		nRetCode =
		    KNIRecodeStreamRecord(hStream, "5.9	3.0	5.1	1.8	Iris-virginica", NULL, MAXBUFFERSIZE);
		printf("Recode stream record with NULL output record %s: %d\n", sOutputRecord, nRetCode);

		// Output record with negative size
		nRetCode =
		    KNIRecodeStreamRecord(hStream, "5.9	3.0	5.1	1.8	Iris-virginica", sOutputRecord, -1);
		printf("Recode stream record with negative output size %s: %d\n", sOutputRecord, nRetCode);
	}

	// Close stream
	if (hStream >= 0)
	{
		nRetCode = KNICloseStream(hStream);
	}

	//////////////////////////////////////////////////////////
	// Test d'erreurs

	printf("\nWrong parameter tests\n");

	// Fichier de dictionaire NULL
	hStream = KNIOpenStream(NULL, sDictionaryName, "", '\t');
	printf("Open stream with NULL dictionary file %s %s: %d\n", "NULL", sDictionaryName, hStream);

	// Mauvais fichier de dictionaire
	hStream = KNIOpenStream("WrongFile", sDictionaryName, "", '\t');
	printf("Open stream with missing dictionary file %s %s: %d\n", "WrongFile", sDictionaryName, hStream);

	// Fichier de dictionaire invalide (entraine deux pointeurs non desalloues)
	hStream = KNIOpenStream(sInputFileName, sDictionaryName, "", '\t');
	printf("Open stream with invalid dictionary file %s %s: %d\n", sInputFileName, sDictionaryName, hStream);

	// Dictionnaire NULL
	hStream = KNIOpenStream(sDictionaryFileName, NULL, "", '\t');
	printf("Open stream with NULL dictionary %s %s: %d\n", sDictionaryFileName, "NULL", hStream);

	// Mauvais dictionnaire
	hStream = KNIOpenStream(sDictionaryFileName, "WrongDic", "", '\t');
	printf("Open stream with missing dictionary %s %s: %d\n", sDictionaryFileName, "WrongDic", hStream);

	// Header line NULL
	hStream = KNIOpenStream(sDictionaryFileName, sDictionaryName, NULL, '\t');
	printf("Open stream with NULL header line %s %s: %d\n", sDictionaryFileName, sDictionaryName, hStream);

	// Bad header line
	hStream = KNIOpenStream(sDictionaryFileName, sDictionaryName, "v1	v2", ';');
	printf("Open stream with bad header line %s %s: %d\n", sDictionaryFileName, sDictionaryName, hStream);

	// Separateur eol
	hStream = KNIOpenStream(sDictionaryFileName, sDictionaryName, "v1	v2", '\n');
	printf("Open stream with eol separator %s %s: %d\n", sDictionaryFileName, sDictionaryName, hStream);

	// Recodage avec mauvais stream
	sOutputRecord[0] = '\0';
	nRetCode =
	    KNIRecodeStreamRecord(0, "5.1	3.5	1.4	0.2	Iris-setosa", sOutputRecord, MAXBUFFERSIZE);
	printf("Recode with wrong stream %s: %d\n", sOutputRecord, nRetCode);

	// Fermeture d'un mauvais stream
	nRetCode = KNICloseStream(-1);
	printf("Close wrong stream (-1): %d\n", nRetCode);
	nRetCode = KNICloseStream(0);
	printf("Close wrong stream (0): %d\n", nRetCode);

	//////////////////////////////////////////////////////////
	// Test en volumetrie

	// Initialisation du tableau de streams
	printf("\nMuliple test: up to %d simultaneous streams\n", nMaxIter);
	for (nIter = 0; nIter < nMaxIter; nIter++)
		ivStreamHandles[nIter] = -1;

	// Boucle d'ouverture de nombreux stream
	printf("Muliple open stream\n");
	for (nIter = 0; nIter < nMaxIter; nIter++)
	{
		// Create stream
		hStream = KNIOpenStream(sDictionaryFileName, sDictionaryName, sHeaderLine, '\t');

		// Memorisation si OK
		if (hStream >= 0)
			ivStreamHandles[nIter] = hStream;
		// Arret sinon
		else
		{
			printf("SYS\t=> stream index %d\n",
			       nIter); // L'index du stream qui ne peut pas etre ouvert depend de la RAM disponible
			ASSERT_GT(nIter, 2);                // On doit pouvoir ouvrir au moins 3 streams
			printf("\t=> Error %d\n", hStream); // Le code retour n'est pas systeme dependant

			break;
		}
	}

	// Boucle de fermeture des streams correctement ouverts
	printf("Muliple close stream\n");
	for (nIterClose = 0; nIterClose < nIter; nIterClose++)
	{
		// Open stream
		hStream = ivStreamHandles[nIterClose];
		nRetCode = KNICloseStream(hStream);

		// Arret si erreur
		if (nRetCode != KNI_OK)
		{
			printf("Error while closing stream %d: Error %d\n", nIterClose, nRetCode);
			break;
		}
	}
}

void TestIris()
{
	ALString sTestPath;
	ALString sDictionaryPath;
	ALString sDataPath;
	ALString sOutputPath;
	ALString sRefFilePath;
	int nLineNumber;
	boolean bOk;

	sTestPath = FileService::GetPathName(__FILE__);
	sDictionaryPath = FileService::BuildFilePathName(sTestPath, "ModelingIris.kdic");
	sDataPath = FileService::BuildFilePathName(sTestPath, "../LearningTest/datasets/Iris/Iris.txt");
	sOutputPath = sTestPath + "results" + FileService::GetFileSeparator() + "R_Iris.txt";
	sRefFilePath = sTestPath + "results.ref" + FileService::GetFileSeparator() + "R_Iris.txt";

	// Test des effets de bord
	TestSideEffect(sDictionaryPath, "SNB_Iris", sDataPath);

	// Test de deploiement
	nLineNumber = KNIRecodeFile(sDictionaryPath, "SNB_Iris", sDataPath, sOutputPath, "");
	ASSERT_EQ(nLineNumber, 150);

	// Comparaison
	bOk = FileCompareForTest(sRefFilePath, sOutputPath);
	ASSERT_TRUE(bOk);

	// Message de fin
	printf("\nEnd test KNI");
}

void TestAdult()
{
	ALString sTestPath;
	ALString sDictionaryPath;
	ALString sDataPath;
	ALString sOutputPath;
	ALString sRefFilePath;
	int nLineNumber;
	FILE* fRef;
	FILE* fWithoutHeader;
	char ch;
	boolean bOk;

	sTestPath = FileService::GetPathName(__FILE__);
	sDictionaryPath = FileService::BuildFilePathName(sTestPath, "../LearningTest/datasets/Adult/Adult.kdic");
	sDataPath = FileService::BuildFilePathName(sTestPath, "../LearningTest/datasets/Adult/Adult.txt");
	sOutputPath = sTestPath + "results" + FileService::GetFileSeparator() + "R_Adult.txt";

	// Test de deploiement
	nLineNumber = KNIRecodeFile(sDictionaryPath, "Adult", sDataPath, sOutputPath, "");
	ASSERT_EQ(nLineNumber, 48842);

	// Verification du fichier deploye
	// Le fichier d'entree contient un header, le fichier deploye n'en contient pas
	// On recopie le fichier d'entree sans le header pour effectuer un ecomparaison

	// Saut du header
	FileService::OpenInputBinaryFile(sDataPath, fRef);
	ch = getc(fRef);
	while (ch != '\n' and ch != EOF)
	{
		ch = getc(fRef);
	}

	// Copie du fichier tel quel
	sRefFilePath = sTestPath + "results" + FileService::GetFileSeparator() + "ref_Adult.txt";
	FileService::OpenOutputBinaryFile(sRefFilePath, fWithoutHeader);
	ch = getc(fRef);
	while (ch != EOF)
	{
		putc(ch, fWithoutHeader);
		ch = getc(fRef);
	}
	FileService::CloseInputBinaryFile(sDataPath, fRef);
	FileService::CloseOutputBinaryFile(sRefFilePath, fWithoutHeader);

	// Comparaison
	bOk = FileCompareForTest(sRefFilePath, sOutputPath);
	ASSERT_TRUE(bOk);

	// Message de fin
	printf("\nEnd test Adult");
}

namespace
{
KHIOPS_TEST(KNI, Iris, ::TestIris);
KHIOPS_TEST(KNI, Adult, ::TestAdult);

} // namespace
