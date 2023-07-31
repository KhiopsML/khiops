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
			// et du repertoire temporaire par @TMP_DIR@
			SearchAndReplace(lineTest, sRootDir, "@ROOT_DIR@");
			SearchAndReplace(lineTest, sTmpDir, "@TMP_DIR@");

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