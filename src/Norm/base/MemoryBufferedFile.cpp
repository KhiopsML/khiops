// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MemoryBufferedFile.h"

///////////////////////////////////////////////////////////////////////////
// Classe MemoryInputBufferedFile

MemoryInputBufferedFile::MemoryInputBufferedFile() {}

MemoryInputBufferedFile::~MemoryInputBufferedFile() {}

void MemoryInputBufferedFile::ResetBuffer()
{
	// Initialisations
	nPositionInCache = 0;
	nCurrentBufferSize = 0;
	nCacheSize = 0;
	nCurrentLineIndex = 1;
	bLastFieldReachEol = false;
	nLastBolPositionInCache = 0;
	nBufferLineNumber = 0;
}

boolean MemoryInputBufferedFile::FillBuffer(const CharVector* cvInputRecord)
{
	boolean bOk;
	int nInitialCurrentBufferSize;
	int nInitialBufferLineNumber;
	char c;
	int i;

	require(cvInputRecord != NULL);

	// Retaillage si necessaire du buffer
	nInitialCurrentBufferSize = nCurrentBufferSize;
	nInitialBufferLineNumber = nBufferLineNumber;
	bOk = AllocateBuffer();

	// Recopie de la chaine de caracteres dans le buffer
	if (bOk)
	{
		// Recopie du record, dans la limite de la taille du buffer
		for (i = 0; i < cvInputRecord->GetSize(); i++)
		{
			c = cvInputRecord->GetAt(i);

			// Memorisation du caractere sinon
			fcCache.SetAt(nCurrentBufferSize, c);
			nCurrentBufferSize++;
			if (c == '\n')
				nBufferLineNumber++;

			if (nCurrentBufferSize == nBufferSize)
				break;
		}

		// Memorisation de la taille du cache a la taille du buffer courant
		nCacheSize = nCurrentBufferSize;
	}
	ensure(nCurrentBufferSize >= nInitialCurrentBufferSize);
	ensure(not bOk or nCurrentBufferSize >= nInitialCurrentBufferSize + cvInputRecord->GetSize());
	return bOk;
}

boolean MemoryInputBufferedFile::FillBufferWithRecord(const char* sInputRecord)
{
	boolean bOk;
	int nInitialCurrentBufferSize;
	int nInitialBufferLineNumber;
	char c;
	int i;

	require(sInputRecord != NULL);

	// Retaillage si necessaire du buffer
	nInitialCurrentBufferSize = nCurrentBufferSize;
	nInitialBufferLineNumber = nBufferLineNumber;
	bOk = AllocateBuffer();

	// Recopie de la chaine de caracteres dans le buffer
	if (bOk)
	{
		// Recopie du record, dans la limite de la taille du buffer
		i = 0;
		bOk = false;
		while (nCurrentBufferSize < nBufferSize)
		{
			c = sInputRecord[i];

			// Arret si fin de chaine
			if (c == '\0')
			{
				// Ajout si necessaire d'un fin de ligne pour marquer la fin de l'enregistrement
				if (i == 0 or sInputRecord[i - 1] != '\n')
				{
					fcCache.SetAt(nCurrentBufferSize, '\n');
					nCurrentBufferSize++;
					nBufferLineNumber++;
				}
				bOk = true;
				break;
			}
			// Memorisation du caractere sinon
			else
			{
				fcCache.SetAt(nCurrentBufferSize, c);
				nCurrentBufferSize++;
				if (c == '\n')
					nBufferLineNumber++;
				i++;
			}
		}

		// Memorisation de la taille du cache a la taille du buffer courant
		if (bOk)
			nCacheSize = nCurrentBufferSize;

		// On annule la recopie du record en cas d'espace insuffisant
		if (not bOk)
		{
			nCurrentBufferSize = nInitialCurrentBufferSize;
			nBufferLineNumber = nInitialBufferLineNumber;
		}
	}
	ensure(nCurrentBufferSize >= nInitialCurrentBufferSize);
	ensure(not bOk or nCurrentBufferSize >= nInitialCurrentBufferSize + (int)strlen(sInputRecord));
	return bOk;
}

longint MemoryInputBufferedFile::GetFileSize() const
{
	// Taille virtuellement infinie pour un stream
	return LLONG_MAX;
}

boolean MemoryInputBufferedFile::Open()
{
	require(not bIsOpened);
	assert(nBufferSize > 0);

	bIsOpened = AllocateBuffer();
	if (bIsOpened)
		nPositionInCache = 0;
	return bIsOpened;
}

boolean MemoryInputBufferedFile::IsOpened() const
{
	return bIsOpened;
}

boolean MemoryInputBufferedFile::Close()
{
	bIsOpened = false;
	return true;
}

///////////////////////////////////////////////////////////////////////////
// Classe MemoryOutputBufferedFile

MemoryOutputBufferedFile::MemoryOutputBufferedFile() {}

MemoryOutputBufferedFile::~MemoryOutputBufferedFile() {}

void MemoryOutputBufferedFile::ResetBuffer()
{
	nCurrentBufferSize = 0;
}

boolean MemoryOutputBufferedFile::FillRecordWithBuffer(char* sOutputRecord, int nOutputMaxLength)
{
	int i;
	char c;
	boolean bOk;

	// TODO question : pourquoi ne pas utiliser la methode OutputBufferedFile::Write(const CharVector* cvValue)
	require(sOutputRecord != NULL);
	require(nOutputMaxLength > 0);

	// Remplissage du record a partir du buffer
	bOk = false;
	for (i = 0; i < nCurrentBufferSize; i++)
	{
		// Erreur si pas assez de place
		if (i >= nOutputMaxLength)
		{
			// On met le resultat en sortie a vide
			sOutputRecord[0] = '\0';
			break;
		}

		// Memorisation du caractere courant
		c = fcCache.GetAt(i);
		sOutputRecord[i] = c;

		// Ok si fin de chaine
		if (c == '\0')
		{
			bOk = true;
			break;
		}

		// Ok si fin de ligne: on nettoie la fin de ligne
		if (c == '\n')
		{
			sOutputRecord[i] = '\0';
			if (i > 0 and sOutputRecord[i - 1] == '\r')
				sOutputRecord[i - 1] = '\0';
			bOk = true;
			break;
		}
	}
	// Protection de la fin du record
	if (nCurrentBufferSize < nOutputMaxLength)
		sOutputRecord[nCurrentBufferSize] = '\0';
	else
		sOutputRecord[nOutputMaxLength - 1] = '\0';
	return bOk;
}

boolean MemoryOutputBufferedFile::Open()
{
	require(not bIsOpened);
	assert(nBufferSize > 0);
	bIsOpened = AllocateBuffer();
	return bIsOpened;
}

boolean MemoryOutputBufferedFile::IsOpened() const
{
	return bIsOpened;
}

boolean MemoryOutputBufferedFile::Close()
{
	bIsOpened = false;
	return true;
}

boolean MemoryOutputBufferedFile::FlushCache()
{
	require(IsOpened());
	bIsError = true;
	return false;
}
