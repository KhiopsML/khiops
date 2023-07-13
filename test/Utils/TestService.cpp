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

void SearchAndReplace(char* sString, const ALString& sSearch, const ALString& sReplace)
{
	// TODO
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
			SearchAndReplace(lineTest, sRootDir, "@ROOT_DIR@");

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
			cout << endl << "error at line " << nLineIndex << endl;
	}

	// Fermeture des fichiers
	if (fileRef != NULL)
		fclose(fileRef);
	if (fileTest != NULL)
		fclose(fileTest);
	return bSame;
}

inline void TestDup()
{
	int fdInit = dup(STDOUT_FILENO);
	int fd = open("file", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	dup(fd);
	close(fd);
	dup2(fd, STDOUT_FILENO); // make stdout go to file

	dup2(fdInit, 1);
	close(fdInit);
}