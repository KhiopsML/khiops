// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "FileBuffer.h"

RequestIOFunction FileBuffer::fRequestIOFunction = NULL;
ReleaseIOFunction FileBuffer::fReleaseIOFunction = NULL;

FileBuffer::FileBuffer()
{
	// Les fonctions sont toutes les deux nulles ou toutes les deux affectees
	require((fRequestIOFunction == NULL and fReleaseIOFunction == NULL) or
		(fRequestIOFunction != NULL and fReleaseIOFunction != NULL));
}

FileBuffer::~FileBuffer() {}

boolean FileBuffer::WriteToFile(SystemFile* fileHandle, int nNumber, const Object* errorSender) const
{
	boolean bOk = true;
	int i;
	int nYetToWrite;
	int nWriteSize;
	longint lWrittenNumber;

	require(fileHandle != NULL);
	require(0 <= nNumber and nNumber <= cvBuffer.nSize);

	// Ajout de stats memoire
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(GetClassLabel() + " WriteToFile Begin");

	// Demande d'acces au disque
	if (GetRequestIOFunction() != NULL)
		fRequestIOFunction(1);

	// Reserve la taille qui va etre ecrite pour eviter la fragmentation du disque
	fileHandle->ReserveExtraSize(nNumber);

	// Cas mono-block du CharVector
	if (cvBuffer.nAllocSize <= cvBuffer.nBlockSize)
	{
		lWrittenNumber =
		    fileHandle->Write(cvBuffer.pData.hugeVector.pValues, cvBuffer.nElementSize, (size_t)nNumber);
		bOk = lWrittenNumber == nNumber;
	}
	// Cas multi-block du CharVector
	else
	{
		i = 0;
		nYetToWrite = nNumber;
		assert(cvBuffer.nElementSize == 1);
		while (bOk and nYetToWrite > 0)
		{
			// Calcul de ce qu'il faut ecrire
			if (nYetToWrite > cvBuffer.nBlockSize)
				nWriteSize = cvBuffer.nBlockSize;
			else
				nWriteSize = nYetToWrite;

			// Ecriture
			lWrittenNumber = fileHandle->Write(cvBuffer.pData.hugeVector.pValueBlocks[i],
							   cvBuffer.nElementSize, (size_t)nWriteSize);
			bOk = lWrittenNumber == nWriteSize;
			nYetToWrite -= nWriteSize;
			assert(nYetToWrite >= 0);
			i++;
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

int FileBuffer::ReadFromFile(SystemFile* fileHandle, longint lPos, int nNumber, const Object* errorSender)
{
	boolean bOk = true;
	longint lTotalReadNumber;
	longint lReadNumber;
	int nYetToRead;
	int nReadSize;
	int i;
	boolean bEof;

	require(fileHandle != NULL);
	require(nNumber <= cvBuffer.nSize);

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
		if (cvBuffer.nAllocSize <= cvBuffer.nBlockSize)
		{
			lTotalReadNumber =
			    fileHandle->Read(cvBuffer.pData.hugeVector.pValues, cvBuffer.nElementSize, (size_t)nNumber);
			bOk = lTotalReadNumber != -1;
		}
		// Cas multi-block du CharVector
		else
		{
			nYetToRead = nNumber;
			i = 0;
			bEof = false;
			while (bOk and not bEof and nYetToRead > 0)
			{
				// Calcul de ce qu'il faut lire
				if (nYetToRead > cvBuffer.nBlockSize)
					nReadSize = cvBuffer.nBlockSize;
				else
					nReadSize = nYetToRead;

				// Lecture
				lReadNumber = fileHandle->Read(cvBuffer.pData.hugeVector.pValueBlocks[i],
							       cvBuffer.nElementSize, (size_t)nReadSize);
				bOk = lReadNumber != -1;
				if (bOk)
					bEof = (lReadNumber != nReadSize);
				lTotalReadNumber += lReadNumber;
				assert(lTotalReadNumber <= nNumber);
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

int FileBuffer::ReadLine(SystemFile* fileHandle, boolean& bEol, boolean& bEof, const Object* errorSender)
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
	if (cvBuffer.nAllocSize <= cvBuffer.nBlockSize)
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
			if (nYetToRead > cvBuffer.nBlockSize)
				nReadSize = cvBuffer.nBlockSize;
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

int FileBuffer::ComputeLineNumber(int nSize) const
{
	int nbLines = 0;
	int i;
	int j;
	char* pValues;
	int nBlockNumber;
	int nMicroBlockNumber;
	int nIndice;

	require(nSize <= cvBuffer.nSize);

	// Cas mono-block
	if (cvBuffer.nAllocSize <= cvBuffer.nBlockSize)
	{
		pValues = cvBuffer.pData.hugeVector.pValues;
		assert(nSize <= cvBuffer.nBlockSize);
		nMicroBlockNumber = nSize / 16;
		nIndice = 0;
		for (i = 0; i < nMicroBlockNumber; i++)
		{
			if (pValues[nIndice + 0] == '\n')
				nbLines++;
			if (pValues[nIndice + 1] == '\n')
				nbLines++;
			if (pValues[nIndice + 2] == '\n')
				nbLines++;
			if (pValues[nIndice + 3] == '\n')
				nbLines++;
			if (pValues[nIndice + 4] == '\n')
				nbLines++;
			if (pValues[nIndice + 5] == '\n')
				nbLines++;
			if (pValues[nIndice + 6] == '\n')
				nbLines++;
			if (pValues[nIndice + 7] == '\n')
				nbLines++;
			if (pValues[nIndice + 8] == '\n')
				nbLines++;
			if (pValues[nIndice + 9] == '\n')
				nbLines++;
			if (pValues[nIndice + 10] == '\n')
				nbLines++;
			if (pValues[nIndice + 11] == '\n')
				nbLines++;
			if (pValues[nIndice + 12] == '\n')
				nbLines++;
			if (pValues[nIndice + 13] == '\n')
				nbLines++;
			if (pValues[nIndice + 14] == '\n')
				nbLines++;
			if (pValues[nIndice + 15] == '\n')
				nbLines++;
			nIndice += 16;
		}

		// Le dernier bout du  bloc: le reste de la division par 16
		for (i = nIndice; i < nSize; i++)
		{
			if (pValues[i] == '\n')
				nbLines++;
		}
	}
	// Cas multi-block
	else
	{
		i = 0;
		j = 0;

		// Inspire de  CharVector::SetAt
		// pour optimiser on compte par bloc de 16

		// On compte dans tous les blocs complets
		nBlockNumber = nSize / cvBuffer.nBlockSize;
		nMicroBlockNumber = cvBuffer.nBlockSize / 16;
		while (i < nBlockNumber)
		{
			pValues = cvBuffer.pData.hugeVector.pValueBlocks[i];

			// C'est un multiple de 16
			assert(cvBuffer.nBlockSize % 16 == 0);
			nIndice = 0;
			for (j = 0; j < nMicroBlockNumber; j++)
			{
				if (pValues[nIndice + 0] == '\n')
					nbLines++;
				if (pValues[nIndice + 1] == '\n')
					nbLines++;
				if (pValues[nIndice + 2] == '\n')
					nbLines++;
				if (pValues[nIndice + 3] == '\n')
					nbLines++;
				if (pValues[nIndice + 4] == '\n')
					nbLines++;
				if (pValues[nIndice + 5] == '\n')
					nbLines++;
				if (pValues[nIndice + 6] == '\n')
					nbLines++;
				if (pValues[nIndice + 7] == '\n')
					nbLines++;
				if (pValues[nIndice + 8] == '\n')
					nbLines++;
				if (pValues[nIndice + 9] == '\n')
					nbLines++;
				if (pValues[nIndice + 10] == '\n')
					nbLines++;
				if (pValues[nIndice + 11] == '\n')
					nbLines++;
				if (pValues[nIndice + 12] == '\n')
					nbLines++;
				if (pValues[nIndice + 13] == '\n')
					nbLines++;
				if (pValues[nIndice + 14] == '\n')
					nbLines++;
				if (pValues[nIndice + 15] == '\n')
					nbLines++;
				nIndice += 16;
			}
			i++;
		}

		// S'il reste un dernier bloc
		if (nSize > nBlockNumber * cvBuffer.nBlockSize)
		{
			// Acces a ce dernier block
			pValues = cvBuffer.pData.hugeVector.pValueBlocks[nSize / cvBuffer.nBlockSize];
			nMicroBlockNumber = (nSize % cvBuffer.nBlockSize) / 16;

			nIndice = 0;
			for (i = 0; i < nMicroBlockNumber; i++)
			{
				if (pValues[nIndice + 0] == '\n')
					nbLines++;
				if (pValues[nIndice + 1] == '\n')
					nbLines++;
				if (pValues[nIndice + 2] == '\n')
					nbLines++;
				if (pValues[nIndice + 3] == '\n')
					nbLines++;
				if (pValues[nIndice + 4] == '\n')
					nbLines++;
				if (pValues[nIndice + 5] == '\n')
					nbLines++;
				if (pValues[nIndice + 6] == '\n')
					nbLines++;
				if (pValues[nIndice + 7] == '\n')
					nbLines++;
				if (pValues[nIndice + 8] == '\n')
					nbLines++;
				if (pValues[nIndice + 9] == '\n')
					nbLines++;
				if (pValues[nIndice + 10] == '\n')
					nbLines++;
				if (pValues[nIndice + 11] == '\n')
					nbLines++;
				if (pValues[nIndice + 12] == '\n')
					nbLines++;
				if (pValues[nIndice + 13] == '\n')
					nbLines++;
				if (pValues[nIndice + 14] == '\n')
					nbLines++;
				if (pValues[nIndice + 15] == '\n')
					nbLines++;
				nIndice += 16;
			}

			// Le dernier bout du dernier bloc: le reste de la division par 16
			for (i = nIndice; i < nSize % cvBuffer.nBlockSize; i++)
			{
				if (pValues[i] == '\n')
					nbLines++;
			}
		}
	}
	return nbLines;
}

longint FileBuffer::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(FileBuffer);
	lUsedMemory += cvBuffer.GetUsedMemory();
	return lUsedMemory;
}

const ALString FileBuffer::GetClassLabel() const
{
	return "File buffer";
}

int FileBuffer::TestCountLines(const ALString& sFileName, boolean bSilentMode)
{
	FileBuffer fbTest;
	longint lFileSize;
	SystemFile* fileHandle;
	int nReadSize;
	int nLineNumber;
	Timer timer;
	FileBuffer errorSender;
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
		nLineNumber = fbTest.ComputeLineNumber(nReadSize);
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

int FileBuffer::FindEol(char* sBuffer, int nLength)
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