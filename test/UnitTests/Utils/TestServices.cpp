// Copyright (c) 2023-2026 Orange. All rights reserved.
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
	boolean bDifferentLineNumber = false;
	const char sSys[] = "SYS";
	int nLineIndex;
	const int nMaxSize = 5000; // Permet de stocker un path tres long
	char lineRef[nMaxSize];
	char lineTest[nMaxSize];
	ALString sLineTest;
	ALString sLineRef;
	int nLineLength;
	const ALString sRootDir = GetRootDir();
	const ALString sTmpDir = FileService::GetTmpDir();
	int nPos;

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
	fileRef = p_fopen(sFileNameReference, "r");
	if (fileRef == NULL)
	{
		cout << "Unable to open ref file" << endl;
		return false;
	}

	fileTest = p_fopen(sFileNameTest, "r");
	if (not fileTest)
	{
		fclose(fileRef);
		cout << "Unable to open test file" << endl;
		return false;
	}

	// Comparaison ligne par ligne
	nLineIndex = 0;
	while (fgets(lineRef, sizeof(lineRef), fileRef) != NULL)
	{
		nLineIndex++;

		// Si il manque des lignes en test, il y a une erreur
		if (fgets(lineTest, sizeof(lineTest), fileTest) == NULL)
		{
			bSame = false;
			bDifferentLineNumber = true;
			cout << endl << "error not enough lines in test (Test: " << nLineIndex << " lines)" << endl;
			break;
		}

		// Nettoyage des fin de ligne avant de passer aux ALString
		nLineLength = (int)strlen(lineTest);
		if (nLineLength > 0 and lineTest[nLineLength - 1] == '\n')
			lineTest[nLineLength - 1] = '\0';
		nLineLength = (int)strlen(lineRef);
		if (nLineLength > 0 and lineRef[nLineLength - 1] == '\n')
			lineRef[nLineLength - 1] = '\0';
		sLineTest = ALString(lineTest);
		sLineRef = ALString(lineRef);

		// Remplacement du repertoire de travail par @ROOT_DIR@
		// du repertoire temporaire par @TMP_DIR@
		SearchAndReplace(sLineTest, sRootDir, "@ROOT_DIR@");
		SearchAndReplace(sLineTest, sTmpDir, "@TMP_DIR@");

		// On cherche la premiere occurence de 'SYS' dans la ligne de ref
		nPos = sLineRef.Find(sSys);

		// Est-ce que les lignes sont identiques ?
		bSame = strncmp(sLineRef, sLineTest, nPos) == 0;

		// Si les 2 lignes sont differentes (jusqu'au token SYS)
		if (not bSame)
		{
			// On remplace le separateur windows par le separateur unix pour etre
			// tolerant dans les chemins et on compare a nouveau les lignes
			SearchAndReplace(sLineTest, "\\", "/");
			bSame = strncmp(sLineRef, sLineTest, nPos) == 0;
			if (not bSame)
				break;
		}
	}

	// Si il y a trop de lignes en test, il y a une erreur
	if (fgets(lineTest, sizeof(lineTest), fileTest) != NULL)
	{
		bSame = false;
		bDifferentLineNumber = true;
		cout << endl << "error too many lines in test (Ref: " << nLineIndex << " lines)" << endl;
	}

	// Erreur si ligne differente, sans probleme de difference de nombre de lignes
	if (not bSame and not bDifferentLineNumber)
	{
		cout << endl << "error at line " << nLineIndex << endl;
		cout << "Ref:  " << sLineRef << endl;
		cout << "Test: " << sLineTest << endl;
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

	// Parametrage de l'arret pour la memoire
	// Le framework de gestion des tests unitaires n'ayant pas de main(), cela semble etre
	// l'endroit pertinent pour la detection des fuites memoire
	// MemSetAllocIndexExit(3391);

	// On passe en mode batch pour avoir des parametres par defaut, sans interaction utilisateur
	SetAcquireBatchMode(true);

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

	// Restitution du mode standard
	SetAcquireBatchMode(false);
	EXPECT_TRUE(bOk);
	return true;
}
