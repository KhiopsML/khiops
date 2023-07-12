// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "TestServices.h"

boolean FileCompareForTest(const ALString& sFileName1, const ALString& sFileName2)
{
	FILE* file1;
	FILE* file2;
	boolean bOk1;
	boolean bOk2;
	boolean bSame = true;
	const char sys[] = "SYS";
	int nLineIndex;
	const int sizeMax = 512;
	char line1[sizeMax];
	char line2[sizeMax];

	// Initialisations
	bOk1 = false;
	bOk2 = false;
	file1 = NULL;
	file2 = NULL;

	// Retour si un fichier n'existe pas
	if (not FileService::FileExists(sFileName1))
	{
		cout << "File " << sFileName1 << " is missing" << endl;
		return false;
	}

	if (not FileService::FileExists(sFileName2))
	{
		cout << "File " << sFileName2 << " is missing" << endl;
		return false;
	}

	// Ouverture des fichiers
	bOk1 = FileService::OpenInputBinaryFile(sFileName1, file1);
	if (bOk1)
	{
		bOk2 = FileService::OpenInputBinaryFile(sFileName2, file2);
		if (not bOk2)
		{
			fclose(file1);
			return false;
		}
	}

	// Comparaison ligne par ligne
	if (bOk1 and bOk2)
	{
		nLineIndex = 0;
		while (fgets(line1, sizeof(line1), file1) != NULL)
		{
			nLineIndex++;
			if (fgets(line2, sizeof(line2), file2) == NULL)
			{
				bSame = false;
				break;
			}

			// Si les 2 lignes sont differentes et qu'elles ne commencent pas toutes
			// les 2 par SYS, les fichiers sont differents
			if (not(memcmp(line1, sys, strlen(sys) - 1) == 0 and
				memcmp(line2, sys, strlen(sys) - 1) == 0) and
			    (strcmp(line1, line2) != 0))
			{
				bSame = false;
				break;
			}
		}

		if (not bSame)
			cout << endl << "error at line " << nLineIndex << endl;
	}

	// Fermeture des fichiers
	if (file1 != NULL)
		fclose(file1);
	if (file2 != NULL)
		fclose(file2);
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