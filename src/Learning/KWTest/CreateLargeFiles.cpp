// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CreateLargeFiles.h"

boolean MovePositionInBinaryFile(FILE* fFile, longint lOffset)
{
	boolean bOk;

	// Pour les fichiers de plus de 4 Go, il existe une API speciale (stat64...)
#ifdef _WIN32
	_fseeki64(fFile, lOffset, SEEK_CUR);
#elif defined(__APPLE__)
	fseeko(fFile, lOffset, SEEK_CUR);
#else
	fseeko64(fFile, lOffset, SEEK_CUR);
#endif
	bOk = (ferror(fFile) == 0);
	return bOk;
}

longint TellPositionInBinaryFile(FILE* fFile)
{
	longint lCurrentPosition;

	// Pour les fichiers de plus de 4 Go, il existe une API speciale (stat64...)
#ifdef _WIN32
	lCurrentPosition = _ftelli64(fFile);
#elif defined(__APPLE__)
	lCurrentPosition = ftello(fFile);
#else
	lCurrentPosition = ftello64(fFile);
#endif
	return lCurrentPosition;
}

// Ecriture d'un grande nombre d'octets 64 kb en mode standard ou rapide
void WriteBuffers(FILE* fOutput, longint lByteSize, char cValue, boolean bFast)
{
	const int nBufferLength = MemSegmentByteSize;
	char sBuffer[nBufferLength];
	int i;
	longint lWrittenSize;
	Timer timer;

	require(fOutput != NULL);
	require(lByteSize >= 0);

	// Initialisation du contenu du buffer
	for (i = 0; i < nBufferLength; i++)
		sBuffer[i] = cValue;
	sBuffer[nBufferLength - 1] = '\n';

	// En mode rapide, en reserve d'abord la taille necessaire dans le fichier,
	// En ecrivant un octet a la fin
	if (bFast)
	{
		// Ecriture d'un octet en fin de fichier
		MovePositionInBinaryFile(fOutput, lByteSize - 1);
		fwrite("\0", 1, sizeof(char), fOutput);

		// On se redeplace a la posiiton courante initiale
		MovePositionInBinaryFile(fOutput, -lByteSize);
	}

	// Ecriture dans le fichier a partir de sa position courante
	lWrittenSize = 0;
	while (lWrittenSize < lByteSize)
	{
		if (lWrittenSize + nBufferLength < lByteSize)
		{
			fwrite(sBuffer, sizeof(char), nBufferLength, fOutput);
			lWrittenSize += nBufferLength;
		}
		else
		{
			fwrite(sBuffer, sizeof(char), int(lByteSize - lWrittenSize), fOutput);
			lWrittenSize = lByteSize;
		}
	}
}

// Etude de creation de fichier selon different mode
void StudyCreateLargeFiles(int argc, char** argv)
{
	ALString sRootDir;
	longint lFileSize;
	StringVector svOpenModes;
	int nIter;
	boolean bFast;
	int nMode;
	ALString sMode;
	ALString sPathName;
	FILE* fFile;
	Timer timer;

	// Test du nombre d'arguments
	if (argc != 3)
		cout << "Study create large files <RootDir> <FileSize>\n";
	else
	{
		// Acces aux operandes
		sRootDir = argv[1];
		lFileSize = StringToLongint(argv[2]);

		// Creation du repertoire racine
		FileService::MakeDirectories(sRootDir);

		// Liste des modes testes (wb et w+b ne marchent pas, car un fichier est recree e chaque fois)
		svOpenModes.Add("r+b");
		svOpenModes.Add("a+b");
		svOpenModes.Add("ab");

		// Creation des fichiers selon tous les modes
		cout << "FileSize\tFast\tMode\tSize\tTime" << endl;
		for (nIter = 0; nIter < 2; nIter++)
		{
			bFast = nIter % 2 == 1;
			for (nMode = 0; nMode < svOpenModes.GetSize(); nMode++)
			{
				sMode = svOpenModes.GetAt(nMode);
				sPathName = sRootDir + "/StudyLargeFile" + LongintToString(lFileSize / lKB) + "KB" +
					    BooleanToString(bFast) + sMode + ".txt";
				timer.Reset();
				timer.Start();

				// Ouverture du fichier un premiere fois en mode standrd, pour ecrire la premiere moitie
				// des octets
				fFile = p_fopen(sPathName, "wb");
				WriteBuffers(fFile, lFileSize / 2, 'a', bFast);
				fclose(fFile);

				// Ouverture du fichier un deuxieme selon le mode specifie, pour ecrire la seconde
				// moitie des octets
				fFile = p_fopen(sPathName, sMode);
				if (sMode.GetAt(0) != 'a')
					FileService::SeekPositionInBinaryFile(fFile, lFileSize / 2);
				WriteBuffers(fFile, lFileSize - lFileSize / 2, 'b', bFast);
				fclose(fFile);

				// Destruction du fichier
				timer.Stop();
				cout << lFileSize << "\t" << BooleanToString(bFast) << "\t" << sMode << "\t"
				     << FileService::GetFileSize(sPathName) << "\t" << timer.GetElapsedTime() << endl;
				FileService::RemoveFile(sPathName);
			}
		}
	}
}

// Creation d'un fichier de grande taille, en mode standard ou rapide
void CreateLargeFile(const ALString& sPathName, longint lFileSize, boolean bFast)
{
	FILE* fFile;
	longint lPosition;
	const int nBufferLength = MemSegmentByteSize;
	char sBuffer[nBufferLength];
	int i;
	Timer timer;

	// Initialisation du contenu du buffer
	for (i = 0; i < nBufferLength; i++)
		sBuffer[i] = ' ';
	sBuffer[nBufferLength - 1] = '\n';

	// Creation d'un fichier d'une taille donnee
	timer.Reset();
	timer.Start();
	if (bFast)
	{
		fFile = p_fopen(sPathName, "w+b");
		if (fFile)
		{
			// Now go to the intended end of the file
			// (subtract 1 since we're writing a single character).
			FileService::SeekPositionInBinaryFile(fFile, lFileSize - 1);

			// Write at least one byte to extend the file (if necessary).
			fwrite("", 1, sizeof(char), fFile);
			fclose(fFile);
		}
	}
	else
	{
		fFile = p_fopen(sPathName, "wb");
		if (fFile)
			fclose(fFile);
	}
	cout << "\t" << timer.GetElapsedTime();

	// Fill
	if (bFast)
	{
		fFile = p_fopen(sPathName, "w+b");
		FileService::SeekPositionInBinaryFile(fFile, 0);
	}
	else
		fFile = p_fopen(sPathName, "wb");
	if (fFile)
	{
		lPosition = 0;
		FileService::SeekPositionInBinaryFile(fFile, lPosition);
		while (lPosition < lFileSize)
		{
			if (lPosition + nBufferLength < lFileSize)
			{
				fwrite(sBuffer, sizeof(char), nBufferLength, fFile);
				lPosition += nBufferLength;
			}
			else
			{
				fwrite(sBuffer, sizeof(char), int(lFileSize - lPosition), fFile);
				lPosition = lFileSize;
			}
		}
		fclose(fFile);
	}
	timer.Stop();
	cout << "\t" << timer.GetElapsedTime();
}

// Copie d'un fichier de grande taille vers deux fichiers puyis concatenation, en mode standard ou rapide
// La copie est effectue de maniere entrelacee, de facon a morceler les fichiers
void CopyFileTwiceThenConcatenate(const ALString& sPathName, boolean bFast)
{
	boolean bRemoveFiles = true;
	longint lFileSize;
	longint lPosition;
	const int nBufferLength = MemSegmentByteSize;
	char sBuffer[nBufferLength];
	int nByteReadNumber;
	FILE* fFile;
	FILE* fCopy1;
	FILE* fCopy2;
	FILE* fConcat;
	ALString sCopy1PathName;
	ALString sCopy2PathName;
	ALString sConcatPathName;
	Timer timer;
	ALString sTmp;

	bRemoveFiles = false;

	// Taille du fichier d'entree
	lFileSize = FileService::GetFileSize(sPathName);

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Copie deux fois

	// Nom des fichiers copies
	sCopy1PathName = FileService::SetFileSuffix(sPathName, sTmp + BooleanToString(bFast) + ".Copy1.txt");
	sCopy2PathName = FileService::SetFileSuffix(sPathName, sTmp + BooleanToString(bFast) + ".Copy2.txt");

	// Initialisation du timer
	timer.Reset();
	timer.Start();

	// Ouverture des fichiers
	fFile = p_fopen(sPathName, "rb");
	if (bFast)
	{
		// Create file with given size
		fCopy1 = p_fopen(sCopy1PathName, "w+b");
		FileService::SeekPositionInBinaryFile(fCopy1, lFileSize - 1);
		fwrite("", 1, sizeof(char), fCopy1);
		FileService::SeekPositionInBinaryFile(fCopy1, 0);

		// Create file with given size
		fCopy2 = p_fopen(sCopy2PathName, "w+b");
		FileService::SeekPositionInBinaryFile(fCopy2, lFileSize - 1);
		fwrite("", 1, sizeof(char), fCopy2);
		FileService::SeekPositionInBinaryFile(fCopy2, 0);
	}
	else
	{
		// Create file
		fCopy1 = p_fopen(sCopy1PathName, "wb");
		fCopy2 = p_fopen(sCopy2PathName, "wb");
	}

	// Copy with inter-leaving files
	lPosition = 0;
	while (lPosition < lFileSize)
	{
		if (lPosition + nBufferLength < lFileSize)
		{
			nByteReadNumber = (int)fread(sBuffer, sizeof(char), nBufferLength, fFile);
			fwrite(sBuffer, sizeof(char), nBufferLength, fCopy1);
			fwrite(sBuffer, sizeof(char), nBufferLength, fCopy2);
			lPosition += nBufferLength;
		}
		else
		{
			nByteReadNumber = (int)fread(sBuffer, sizeof(char), int(lFileSize - lPosition), fFile);
			fwrite(sBuffer, sizeof(char), int(lFileSize - lPosition), fCopy1);
			fwrite(sBuffer, sizeof(char), int(lFileSize - lPosition), fCopy2);
			lPosition = lFileSize;
		}
	}

	// Fermeture des fichiers
	fclose(fFile);
	fclose(fCopy1);
	fclose(fCopy2);
	timer.Stop();
	cout << "\t" << timer.GetElapsedTime();

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Concatenation

	// Nom du fichier concatene
	sConcatPathName = FileService::SetFileSuffix(sPathName, sTmp + BooleanToString(bFast) + ".Concat.txt");

	// Initialisation du timer
	timer.Reset();
	timer.Start();

	// Ouverture du fichier en sortie
	if (bFast)
	{
		// Create file with given size
		fConcat = p_fopen(sConcatPathName, "w+b");
		FileService::SeekPositionInBinaryFile(fConcat, 2 * lFileSize - 1);
		fwrite("", 1, sizeof(char), fConcat);
		FileService::SeekPositionInBinaryFile(fConcat, 0);
	}
	else
	{
		// Create file
		fConcat = p_fopen(sConcatPathName, "wb");
	}

	// Copie du premier fichier
	fCopy1 = p_fopen(sCopy1PathName, "rb");
	lPosition = 0;
	while (lPosition < lFileSize)
	{
		if (lPosition + nBufferLength < lFileSize)
		{
			nByteReadNumber = (int)fread(sBuffer, sizeof(char), nBufferLength, fCopy1);
			fwrite(sBuffer, sizeof(char), nBufferLength, fConcat);
			lPosition += nBufferLength;
		}
		else
		{
			nByteReadNumber = (int)fread(sBuffer, sizeof(char), int(lFileSize - lPosition), fCopy1);
			fwrite(sBuffer, sizeof(char), int(lFileSize - lPosition), fConcat);
			lPosition = lFileSize;
		}
	}
	fclose(fCopy1);

	// Copie du second fichier
	fCopy2 = p_fopen(sCopy2PathName, "rb");
	lPosition = 0;
	while (lPosition < lFileSize)
	{
		if (lPosition + nBufferLength < lFileSize)
		{
			nByteReadNumber = (int)fread(sBuffer, sizeof(char), nBufferLength, fCopy2);
			fwrite(sBuffer, sizeof(char), nBufferLength, fConcat);
			lPosition += nBufferLength;
		}
		else
		{
			nByteReadNumber = (int)fread(sBuffer, sizeof(char), int(lFileSize - lPosition), fCopy2);
			fwrite(sBuffer, sizeof(char), int(lFileSize - lPosition), fConcat);
			lPosition = lFileSize;
		}
	}
	fclose(fCopy2);

	// Fermeture des fichiers
	fclose(fConcat);
	timer.Stop();
	cout << "\t" << timer.GetElapsedTime();

	// Nettoyage
	if (bRemoveFiles)
	{
		FileService::RemoveFile(sPathName);
		FileService::RemoveFile(sCopy1PathName);
		FileService::RemoveFile(sCopy2PathName);
		FileService::RemoveFile(sConcatPathName);
	}
}

// Methode principale de pilotage des test
void TestCreateLargeFiles(int argc, char** argv)
{
	ALString sRootDir;
	longint lMaxFileSize;
	int nIterNumber;
	int nIter;
	boolean bFast;
	longint lFileSize;
	ALString sPathName;

	// Test du nombre d'arguments
	if (argc != 4)
		cout << "Test create large files <RootDir> <MaxSize> <IterNumber>\n";
	else
	{
		// Acces aux operandes
		sRootDir = argv[1];
		lMaxFileSize = StringToLongint(argv[2]);
		nIterNumber = StringToInt(argv[3]);

		// Boucle de test
		cout << "Iter\tFile size\tFast\tCreate\tFill\tCopy twice\tConcat" << endl;
		FileService::MakeDirectories(sRootDir);
		for (nIter = 0; nIter < 2 * nIterNumber; nIter++)
		{
			bFast = (nIter % 2 == 1);
			lFileSize = lKB;
			while (lFileSize < lMaxFileSize)
			{
				cout << nIter / 2 << "\t" << lFileSize << "\t" << BooleanToString(bFast);
				sPathName = sRootDir + "/LargeFile" + LongintToString(lFileSize / lKB) + "KB" +
					    BooleanToString(bFast) + ".txt";
				CreateLargeFile(sPathName, lFileSize, bFast);
				CopyFileTwiceThenConcatenate(sPathName, bFast);
				cout << endl;
				if (lFileSize < lGB)
					lFileSize *= 32;
				else
					lFileSize *= 2;
			}
		}
	}
}
