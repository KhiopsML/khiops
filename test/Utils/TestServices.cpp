// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "TestServices.h"

// Retourne le chemin du repertoire qui contient le projet Khiops
inline ALString GetRootDir()
{
	ALString sFileDir;
	ALString sRootDir;
	int nSep = 0;
	int nPos;

	sFileDir = FileService::GetPathName(__FILE__);
	for (nPos = sFileDir.GetLength(); nPos > 0; nPos--)
	{
		if (sFileDir.GetAt(nPos - 1) == FileService::GetFileSeparator())
			nSep++;
		if (nSep == 3)
			break;
	}
	sRootDir = sFileDir.Left(nPos - 1);

	return sRootDir;
}

void SearchAndReplace(char* sInputString, const ALString& sSearchString, const ALString& sReplaceString)
{
	int nReplacePos;
	ALString sBeginString;
	ALString sEndString;
	ALString sOutputString;

	// Remplacement iteratif des pattern trouves a partir de la chaine pretraitee precedente
	nReplacePos = 0;
	sBeginString = "";
	sEndString = sInputString;
	sOutputString = sBeginString;
	while (nReplacePos >= 0 and sEndString.GetLength() > 0)
	{
		nReplacePos = sEndString.Find(sSearchString);

		// Si non trouve, on garde la fin de la chaine en cours de traitement
		if (nReplacePos == -1)
			sOutputString += sEndString;
		// Sinon, en prend le debut puis la valeur de remplacement
		else
		{
			sBeginString = sEndString.Left(nReplacePos);
			sEndString = sEndString.Right(sEndString.GetLength() - sSearchString.GetLength() - nReplacePos);
			sOutputString += sBeginString;
			sOutputString += sReplaceString;
		}
	}
	char* test = sOutputString.GetBuffer(sOutputString.GetLength());
	sInputString[0] = '\0';
	memcpy(sInputString, test, sOutputString.GetLength());
	sInputString[sOutputString.GetLength()] = '\0';
}

boolean FileCompareForTest(const ALString& sFileNameReference, const ALString& sFileNameTest)
{
	FILE* fileRef;
	FILE* fileTest;
	boolean bOk1;
	boolean bOk2;
	boolean bSame = true;
	const char sys[] = "SYS";
	int nLineIndex;
	const int sizeMax = 512;
	char lineRef[sizeMax];
	char lineTest[sizeMax];
	const ALString sRootDir = GetRootDir();
	const ALString sTmpDir = FileService::GetTmpDir();

	// Initialisations
	bOk1 = false;
	bOk2 = false;
	fileRef = NULL;
	fileTest = NULL;

	// Retour si un fichier n'existe pas
	if (not FileService::FileExists(sFileNameReference))
	{
		cout << "File " << sFileNameReference << " is missing" << endl;
		return false;
	}

	if (not FileService::FileExists(sFileNameTest))
	{
		cout << "File " << sFileNameTest << " is missing" << endl;
		return false;
	}

	// Ouverture des fichiers
	bOk1 = FileService::OpenInputBinaryFile(sFileNameReference, fileRef);
	if (bOk1)
	{
		bOk2 = FileService::OpenInputBinaryFile(sFileNameTest, fileTest);
		if (not bOk2)
		{
			fclose(fileRef);
			return false;
		}
	}

	// Comparaison ligne par ligne
	if (bOk1 and bOk2)
	{
		nLineIndex = 0;
		while (fgets(lineRef, sizeof(lineRef), fileRef) != NULL)
		{
			nLineIndex++;
			if (fgets(lineTest, sizeof(lineTest), fileTest) == NULL)
			{
				bSame = false;
				break;
			}

			// Remplacement du repêrtoire de travail par @ROOT_DIR@
			// du repertoire temporaire par @TMP_DIR@
			// et du separateur windows par le separateur unix
			SearchAndReplace(lineTest, sRootDir, "@ROOT_DIR@");
			SearchAndReplace(lineTest, sTmpDir, "@TMP_DIR@");
			SearchAndReplace(lineTest, "\\", "/");

			// Si les 2 lignes sont differentes et qu'elles ne commencent pas toutes
			// les 2 par SYS, les fichiers sont differents
			if (not(memcmp(lineRef, sys, strlen(sys) - 1) == 0 and
				memcmp(lineTest, sys, strlen(sys) - 1) == 0) and
			    (strcmp(lineRef, lineTest) != 0))
			{
				bSame = false;
				break;
			}
		}

		if (not bSame)
			cout << endl << "error at line " << nLineIndex << endl << "=> " << lineTest << endl;
	}

	// Fermeture des fichiers
	if (fileRef != NULL)
		fclose(fileRef);
	if (fileTest != NULL)
		fclose(fileTest);
	return bSame;
}

inline int p_dup(int fd)
{
#ifdef _WIN32
	return _dup(fd);
#else
	return dup(fd);
#endif
}

inline int p_dup2(int fd, int fd2)
{
#ifdef _WIN32
	return _dup2(fd, fd2);
#else
	return dup2(fd, fd2);
#endif
}

inline int p_fileno(FILE* __stream)
{
#ifdef _WIN32
	return _fileno(__stream);
#else
	return fileno(__stream);
#endif
}

boolean TestAndCompareResults(const char* sTestPath, const char* test_suite, const char* test_name,
			      void (*method_to_test)())
{
	ALString sTestFileName;
	boolean bOk;
	ALString sTmp;
	ALString sFileName;
	FILE* stream;
	int fdInit;
	int fd;

	// Nommmage du ficher de sortie d'apres test suit et test name de googleTest
	sFileName = sTmp + test_suite + "_" + test_name + ".txt";

	// Creation du repertoire results si necessaire
	if (not FileService::DirExists(sTmp + sTestPath + "results"))
		FileService::CreateNewDirectory(sTmp + sTestPath + "results");
	sTestFileName = sTmp + sTestPath + "results" + FileService::GetFileSeparator() + sFileName;

	stream = p_fopen(sTestFileName, "w+");
	EXPECT_TRUE(stream != NULL);

	// Redirection de cout vers le stream dedie au batch
	fdInit = p_dup(STDOUT_FILENO);
	p_dup2(p_fileno(stream), STDOUT_FILENO);

	// Lancement de la methode de test de la classe
	(*method_to_test)();

	// On restitue cout dans son etat initial
	fflush(stdout);
	fclose(stream);
	p_dup2(fdInit, STDOUT_FILENO);

#ifdef _WIN32
	_close(fdInit);
#else
	close(fdInit);
#endif

	// comparaison du fichier issu de la sortie standrd avec le fichier de reference
	bOk = FileCompareForTest(sTmp + sTestPath + "results.ref" + FileService::GetFileSeparator() + sFileName,
				 sTestFileName);

	EXPECT_TRUE(bOk);
	return true;
}
