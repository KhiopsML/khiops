// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "FileCache.h"

RequestIOFunction FileCache::fRequestIOFunction = NULL;
ReleaseIOFunction FileCache::fReleaseIOFunction = NULL;

FileCache::FileCache()
{
	// Les fonctions sont toutes les deux nulles ou toutes les deux affectees
	require((fRequestIOFunction == NULL and fReleaseIOFunction == NULL) or
		(fRequestIOFunction != NULL and fReleaseIOFunction != NULL));
}

FileCache::~FileCache() {}

int FileCache::ReadLine(SystemFile* fileHandle, boolean& bEol, boolean& bEof, const Object* errorSender)
{
	boolean bOk = true;
	int nLineLength;
	int i;
	int nReadSize;
	int nReadInOneBlock;
	int nYetToRead;
	int nEolPos;

	check(fileHandle);
	bEof = false;
	// Cas mono-block du CharVector
	nLineLength = 0;
	if (cvBuffer.nAllocSize <= CharVector::nBlockSize)
	{
		// Lecture d'un buffer
		nLineLength = (int)fileHandle->Read(cvBuffer.pData.hugeVector.pValues, cvBuffer.nElementSize,
						    (size_t)cvBuffer.nSize);
		bOk = nLineLength != -1;

		// Identification de la fin de ligne et de la fin de fichier
		if (bOk)
		{
			nEolPos = FindEol(cvBuffer.pData.hugeVector.pValues, nLineLength);
			bEol = nEolPos != -1;
			bEof = nLineLength != cvBuffer.nSize;

			// Nombre de caractere lus dans la ligne, si fin de ligne avant fin de buffer
			if (nEolPos != -1)
				nLineLength = nEolPos;
		}
	}
	// Cas multi-block du CharVector
	else
	{
		nYetToRead = cvBuffer.nSize;
		nLineLength = 0;
		i = 0;
		bEof = false;
		while (bOk and not bEof and nYetToRead > 0)
		{
			// Calcul de ce qu'il faut lire
			if (nYetToRead > CharVector::nBlockSize)
				nReadSize = CharVector::nBlockSize;
			else
				nReadSize = nYetToRead;

			// Lecture
			nReadInOneBlock = (int)fileHandle->Read(cvBuffer.pData.hugeVector.pValueBlocks[i],
								cvBuffer.nElementSize, (size_t)nReadSize);
			nYetToRead -= nReadSize;
			bOk = nReadInOneBlock != -1;
			if (not bOk)
				break;

			// Identification de la fin de ligne et de la fin de fichier
			nEolPos = FindEol(cvBuffer.pData.hugeVector.pValueBlocks[i], nReadInOneBlock);
			bEol = (nEolPos != -1);
			bEof = nReadInOneBlock != nReadSize;
			assert(nReadInOneBlock == nReadSize or bEol or bEof);

			// Mise a jour de la longueur de ligne
			if (nEolPos != -1)
				nLineLength += nEolPos;
			else
				nLineLength += nReadInOneBlock;

			// Arret si fin de ligne ou de fichier
			if (bEol or bEof)
				break;

			// Incrementation du block
			i++;
		}
	}

	// Affichage de l'erreur eventuelle
	if (not bOk and errorSender != NULL)
		errorSender->AddError(fileHandle->GetLastErrorMessage());

	// Retour
	if (bOk)
		return nLineLength;
	else
	{
		assert(bEol == false);
		assert(bEof == false);
		return -1;
	}
}

int FileCache::ComputeLineNumber(int nBeginPos, int nEndPos) const
{
	int nLineNumber;
	int i;
	char* pValues;
	int nFirstBlockIndex;
	int nLastBlockIndex;
	int nFirstBlockBeginPos;

	require(0 <= nBeginPos and nBeginPos <= nEndPos);
	require(nEndPos <= cvBuffer.nSize);

	// Cas mono-block
	if (cvBuffer.nAllocSize <= CharVector::nBlockSize)
	{
		pValues = cvBuffer.pData.hugeVector.pValues;
		assert(nEndPos <= CharVector::nBlockSize);

		nLineNumber = ComputeBlockLineNumber(pValues, nBeginPos, nEndPos);
	}
	// Cas multi-bloc
	else
	{
		// On recherche les index extremites des bloc complets
		nFirstBlockIndex = nBeginPos / CharVector::nBlockSize;
		nLastBlockIndex = nEndPos / CharVector::nBlockSize;

		// Cas d'un seul bloc
		if (nFirstBlockIndex == nLastBlockIndex)
		{
			pValues = cvBuffer.pData.hugeVector.pValueBlocks[nFirstBlockIndex];
			nLineNumber = ComputeBlockLineNumber(pValues, nBeginPos % CharVector::nBlockSize,
							     nEndPos % CharVector::nBlockSize);
		}
		// Cas ou plusieurs blocs sont utilises
		else
		{
			// Prise en compte du premier bloc
			nFirstBlockBeginPos = nBeginPos % CharVector::nBlockSize;
			nLineNumber = 0;
			if (nFirstBlockBeginPos > 0)
			{
				pValues = cvBuffer.pData.hugeVector.pValueBlocks[nFirstBlockIndex];
				nLineNumber =
				    ComputeBlockLineNumber(pValues, nFirstBlockBeginPos, CharVector::nBlockSize);
				nFirstBlockIndex++;
				assert(nBeginPos < nFirstBlockIndex * CharVector::nBlockSize);
				assert(nFirstBlockIndex * CharVector::nBlockSize - nBeginPos < CharVector::nBlockSize);
			}

			// Prise en compte des block entiers
			for (i = nFirstBlockIndex; i < nLastBlockIndex; i++)
			{
				pValues = cvBuffer.pData.hugeVector.pValueBlocks[i];
				nLineNumber += ComputeBlockLineNumber(pValues, 0, CharVector::nBlockSize);
			}

			// S'il reste un dernier bloc
			if (nEndPos > nLastBlockIndex * CharVector::nBlockSize)
			{
				pValues = cvBuffer.pData.hugeVector.pValueBlocks[nLastBlockIndex];
				nLineNumber += ComputeBlockLineNumber(pValues, 0, nEndPos % CharVector::nBlockSize);
			}
		}
	}
	return nLineNumber;
}

longint FileCache::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(FileCache);
	lUsedMemory += cvBuffer.GetUsedMemory();
	return lUsedMemory;
}

const ALString FileCache::GetClassLabel() const
{
	return "File buffer";
}

int FileCache::TestCountLines(const ALString& sFileName, boolean bSilentMode)
{
	FileCache fbTest;
	longint lFileSize;
	SystemFile* fileHandle;
	int nReadSize;
	int nLineNumber;
	Timer timer;
	FileCache errorSender;
	boolean bOk = false;

	nLineNumber = 0;

	// Acces au driver
	fileHandle = new SystemFile;

	// Acces a la taille du fichier, et arret si fichier trop gros
	lFileSize = SystemFile::GetFileSize(sFileName);
	cout << sFileName << " size " << lFileSize << endl;
	if (lFileSize > INT_MAX / 2)
	{
		Global::AddError("File", sFileName, "File too large to be loaded in one single buffer");
		delete fileHandle;
		return 0;
	}

	// Test de lecture et de comptage des lignes d'un fichier
	timer.Start();
	fbTest.cvBuffer.SetSize((int)lFileSize);
	bOk = fileHandle->OpenInputFile(sFileName);
	nLineNumber = 0;
	nReadSize = 0;

	if (bOk)
	{
		nReadSize = fbTest.ReadFromFile(fileHandle, 0, fbTest.GetSize(), &errorSender);
		nLineNumber = fbTest.ComputeLineNumber(0, nReadSize);
		fileHandle->CloseInputFile(sFileName);
	}

	// Cas d'une seule ligne (pas de eol)
	if (nReadSize > 0 and nLineNumber == 0)
	{
		nLineNumber = 1;
	}
	timer.Stop();
	if (not bSilentMode)
		cout << sFileName << " lines " << nLineNumber << "\t" << SecondsToString(timer.GetElapsedTime())
		     << endl;
	delete fileHandle;

	return nLineNumber;
}

inline int FileCache::ComputeBlockLineNumber(const char* pValues, int nBeginPos, int nEndPos) const
{
	int nLineNumber;
	int i;
	int nFirstMicroBlockIndex;
	int nLastMicroBlockIndex;
	int nFirstMicroBlockEnd;
	int nIndice;

	require(pValues != NULL);
	require(0 <= nBeginPos and nBeginPos <= nEndPos);
	require(nEndPos <= CharVector::nBlockSize);

	// Calcul des index des bloc de debut et de fin
	nFirstMicroBlockIndex = nBeginPos / 16;
	nLastMicroBlockIndex = nEndPos / 16;

	// Fin du premier block non entier
	nFirstMicroBlockEnd = nFirstMicroBlockIndex * 16;
	if (nBeginPos > nFirstMicroBlockEnd)
	{
		nFirstMicroBlockIndex++;
		nFirstMicroBlockEnd += 16;
	}
	assert(nEndPos - nBeginPos == (nFirstMicroBlockEnd - nBeginPos) +
					  (nLastMicroBlockIndex - nFirstMicroBlockIndex) * 16 +
					  (nEndPos - nLastMicroBlockIndex * 16));

	// Prise en compte du premier bloc, potentiellement non complet
	nLineNumber = 0;
	for (i = nBeginPos; i < nFirstMicroBlockEnd; i++)
	{
		if (pValues[i] == '\n')
			nLineNumber++;
	}

	// Calcul des nombre de lignes dans le blocs entiers
	nIndice = nFirstMicroBlockEnd;
	for (i = nFirstMicroBlockIndex; i < nLastMicroBlockIndex; i++)
	{
		if (pValues[nIndice + 0] == '\n')
			nLineNumber++;
		if (pValues[nIndice + 1] == '\n')
			nLineNumber++;
		if (pValues[nIndice + 2] == '\n')
			nLineNumber++;
		if (pValues[nIndice + 3] == '\n')
			nLineNumber++;
		if (pValues[nIndice + 4] == '\n')
			nLineNumber++;
		if (pValues[nIndice + 5] == '\n')
			nLineNumber++;
		if (pValues[nIndice + 6] == '\n')
			nLineNumber++;
		if (pValues[nIndice + 7] == '\n')
			nLineNumber++;
		if (pValues[nIndice + 8] == '\n')
			nLineNumber++;
		if (pValues[nIndice + 9] == '\n')
			nLineNumber++;
		if (pValues[nIndice + 10] == '\n')
			nLineNumber++;
		if (pValues[nIndice + 11] == '\n')
			nLineNumber++;
		if (pValues[nIndice + 12] == '\n')
			nLineNumber++;
		if (pValues[nIndice + 13] == '\n')
			nLineNumber++;
		if (pValues[nIndice + 14] == '\n')
			nLineNumber++;
		if (pValues[nIndice + 15] == '\n')
			nLineNumber++;
		nIndice += 16;
	}

	// Le dernier bout du  bloc: le reste de la division par 16
	for (i = nIndice; i < nEndPos; i++)
	{
		if (pValues[i] == '\n')
			nLineNumber++;
	}
	return nLineNumber;
}

int FileCache::FindEol(char* sBuffer, int nLength)
{
	int i;

	require(sBuffer != NULL);
	require(nLength > 0);

	for (i = 0; i < nLength; i++)
	{
		assert(sBuffer[i] != '\0');
		if (sBuffer[i] == '\n')
			return i;
	}
	return -1;
}

boolean FileCache::WriteToFile(SystemFile* fileHandle, int nSizeToWrite, const Object* errorSender) const
{
	boolean bOk = true;
	int nYetToWrite;
	longint lWrittenNumber;
	char* sBuffer;
	int nSourcePos;
	int nSizeToExport;
	int nHugeBufferSize;

	require(fileHandle != NULL);
	require(0 <= nSizeToWrite and nSizeToWrite <= cvBuffer.nSize);

	// Ajout de stats memoire
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(GetClassLabel() + " WriteToFile Begin");

	// Demande d'acces au disque
	if (GetRequestIOFunction() != NULL)
		fRequestIOFunction(1);

	// Reserve la taille qui va etre ecrite pour eviter la fragmentation du disque
	fileHandle->ReserveExtraSize(nSizeToWrite);

	// Cas mono-block du CharVector
	if (cvBuffer.nAllocSize <= CharVector::nBlockSize)
	{
		lWrittenNumber =
		    fileHandle->Write(cvBuffer.pData.hugeVector.pValues, cvBuffer.nElementSize, (size_t)nSizeToWrite);
		bOk = lWrittenNumber == nSizeToWrite;
	}
	// Cas multi-block du CharVector
	else
	{
		nHugeBufferSize = fileHandle->GetPreferredBufferSize();
		sBuffer = GetHugeBuffer(nHugeBufferSize);
		nYetToWrite = nSizeToWrite;
		nSourcePos = 0;
		assert(cvBuffer.nElementSize == 1);
		while (bOk and nYetToWrite > 0)
		{
			nSizeToExport = min(nHugeBufferSize, nYetToWrite);
			cvBuffer.ExportBuffer(nSourcePos, nSizeToExport, sBuffer);
			lWrittenNumber = fileHandle->Write(sBuffer, cvBuffer.nElementSize, (size_t)nSizeToExport);
			bOk = lWrittenNumber == nSizeToExport;
			nYetToWrite -= nSizeToExport;
			nSourcePos += nSizeToExport;
		}
	}

	// Affichage de l'erreur eventuelle
	if (not bOk and errorSender != NULL)
		errorSender->AddError(fileHandle->GetLastErrorMessage());

	// Relachement du disque
	if (GetReleaseIOFunction() != NULL)
		fReleaseIOFunction(1);

	// Ajout de stats memoire
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(GetClassLabel() + " WriteToFile End");

	return bOk;
}

int FileCache::ReadFromFile(SystemFile* fileHandle, longint lPos, int nSizeToRead, const Object* errorSender)
{
	boolean bOk = true;
	longint lTotalReadNumber;
	longint lReadNumber;
	int nYetToRead;
	int nReadSize;
	int i;
	boolean bEof;

	require(fileHandle != NULL);
	require(nSizeToRead <= cvBuffer.nSize);

	// Ajout de stats memoire
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(GetClassLabel() + " ReadFromFile Begin");

	// Demande d'acces au disque
	if (GetRequestIOFunction() != NULL)
		fRequestIOFunction(0);

	// On positionne la tete de lecture
	bOk = fileHandle->SeekPositionInFile(lPos);
	lTotalReadNumber = -1;
	if (bOk)
	{
		// Cas mono-block du CharVector
		lTotalReadNumber = 0;
		if (cvBuffer.nAllocSize <= CharVector::nBlockSize)
		{
			lTotalReadNumber = fileHandle->Read(cvBuffer.pData.hugeVector.pValues, cvBuffer.nElementSize,
							    (size_t)nSizeToRead);
			bOk = lTotalReadNumber != -1;
		}
		// Cas multi-block du CharVector
		else
		{
			nYetToRead = nSizeToRead;
			i = 0;
			bEof = false;
			while (bOk and not bEof and nYetToRead > 0)
			{
				// Calcul de ce qu'il faut lire
				if (nYetToRead > CharVector::nBlockSize)
					nReadSize = CharVector::nBlockSize;
				else
					nReadSize = nYetToRead;

				// Lecture
				lReadNumber = fileHandle->Read(cvBuffer.pData.hugeVector.pValueBlocks[i],
							       cvBuffer.nElementSize, (size_t)nReadSize);
				bOk = lReadNumber != -1;
				if (bOk)
					bEof = (lReadNumber != nReadSize);
				lTotalReadNumber += lReadNumber;
				assert(lTotalReadNumber <= nSizeToRead);
				assert(not bOk or lReadNumber == nReadSize or bEof);
				nYetToRead -= nReadSize;
				assert(nYetToRead >= 0);
				i++;
			}
		}
	}

	// Affichage de l'erreur eventuelle
	if (not bOk and errorSender != NULL)
		errorSender->AddError(fileHandle->GetLastErrorMessage());

	// Demande d'acces au disque
	if (GetReleaseIOFunction() != NULL)
		fReleaseIOFunction(0);

	// Ajout de stats memoire
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(GetClassLabel() + " ReadFromFile End");

	// Retour
	if (bOk)
		return (int)lTotalReadNumber;
	else
		return 0;
}
