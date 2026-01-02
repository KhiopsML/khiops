// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "OutputBufferedFile.h"
#include "TaskProgression.h"

OutputBufferedFile::OutputBufferedFile()
{
	bIsPhysicalOpen = false;
	bNextOpenOnAppend = false;
	lTotalPhysicalWriteCalls = 0;
	lTotalPhysicalWriteBytes = 0;
}

OutputBufferedFile::~OutputBufferedFile()
{
	if (IsPhysycalOpen())
	{
		PhysicalClose();
	}
	delete fileHandle;
}

void OutputBufferedFile::CopyFrom(const OutputBufferedFile* bufferedFile)
{
	require(not IsOpened());

	BufferedFile::CopyFrom(bufferedFile);
	nCurrentBufferSize = 0;
	bNextOpenOnAppend = false;
}

boolean OutputBufferedFile::Open()
{
	boolean bOk;

	require(not IsOpened());
	require(GetFileName() != "");

	// Initialisation de la taille du buffer
	bOk = AllocateBuffer();
	bNextOpenOnAppend = false;
	lTotalPhysicalWriteCalls = 0;
	lTotalPhysicalWriteBytes = 0;

	// Ouverture du fichier
	if (bOk)
	{
		fileHandle = new SystemFile;

		// Ouverture du fichier
		if (not GetOpenOnDemandMode())
		{
			bOk = PhysicalOpen();
			if (not bOk)
			{
				delete fileHandle;
				fileHandle = NULL;
			}
		}
	}

	bIsOpened = bOk;
	bIsError = not bOk;
	nCurrentBufferSize = 0;
	return IsOpened();
}

boolean OutputBufferedFile::OpenForAppend()
{
	boolean bOk;

	require(not IsOpened());
	require(GetFileName() != "");

	bNextOpenOnAppend = true;

	// Initialisation de la taille du buffer
	bOk = AllocateBuffer();
	lTotalPhysicalWriteCalls = 0;
	lTotalPhysicalWriteBytes = 0;

	// Ouverture du fichier
	if (bOk)
	{
		// Ouverture du fichier
		fileHandle = new SystemFile;
		if (not GetOpenOnDemandMode())
		{
			bOk = PhysicalOpen();
			if (not bOk)
			{
				delete fileHandle;
				fileHandle = NULL;
			}
		}
	}
	bIsOpened = bOk;
	bIsError = not bOk;
	nCurrentBufferSize = 0;
	return IsOpened();
}

boolean OutputBufferedFile::Close()
{
	boolean bOk = true;

	require(IsOpened());
	assert(fileHandle != NULL);

	// Ecriture du contenu du buffer
	if (not bIsError)
		WriteToFile(nCurrentBufferSize);
	bOk = not bIsError;

	// Fermeture du fichier
	if (IsPhysycalOpen())
	{
		bOk = PhysicalClose() and bOk;
	}

	assert(not IsPhysycalOpen());

	// Nettoyage
	delete fileHandle;
	fileHandle = NULL;
	bIsOpened = false;
	bIsError = false;
	nCurrentBufferSize = 0;
	bNextOpenOnAppend = false;
	ResetBuffer();

	ensure(fileHandle == NULL);
	return bOk;
}

longint OutputBufferedFile::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = BufferedFile::GetUsedMemory();
	lUsedMemory += sizeof(OutputBufferedFile) - sizeof(BufferedFile);
	return lUsedMemory;
}

void OutputBufferedFile::ReserveExtraSize(longint lSize)
{
	assert(bIsOpened);
	require(lSize >= 0);

	// Dans le cas OpenOnDemand, on ne peut pas reserve d'espace sur le disque
	// car le fichier est ouvert en mode append, on ajoutera a la taille reservee
	if (GetOpenOnDemandMode())
		return;

	// On reserve de la taille que si necessaire par rapport a la taille de buffer courante geree par rapport au
	// flush et que l'on s'apprete a consommer plus de un buffer
	if (GetCurrentBufferSize() + lSize > 2 * GetBufferSize())
		fileHandle->ReserveExtraSize(GetCurrentBufferSize() + lSize);
}

longint OutputBufferedFile::GetTotalPhysicalWriteCalls() const
{
	return lTotalPhysicalWriteCalls;
}

longint OutputBufferedFile::GetTotalPhysicalWriteBytes() const
{
	return lTotalPhysicalWriteBytes;
}

void OutputBufferedFile::TestWriteFile(const ALString& sInputFileName, const ALString& sOutputFileName,
				       boolean bOpenOnDemand)
{
	longint lFileSize;
	int nBufferSize;
	FileCache buffer;
	SystemFile* fileHandle;
	boolean bOk;
	OutputBufferedFile outputFile;
	Timer timer;
	int i;
	OutputBufferedFile errorSender;
	boolean bSameFile;

	// References : taille du fichier d'entree
	lFileSize = PLRemoteFileService::GetFileSize(sInputFileName);
	cout << "Input file name\t" << sInputFileName << "\tSize\t" << lFileSize << endl
	     << "Output file name\t" << sOutputFileName << endl
	     << endl;

	// Chargement du fichier d'entree en memoire
	buffer.SetSize((int)lFileSize);

	// Creation du driver
	fileHandle = new SystemFile;
	bOk = fileHandle->OpenInputFile(sInputFileName);
	if (bOk)
	{
		buffer.ReadFromFile(fileHandle, 0, buffer.GetSize(), &errorSender);
		fileHandle->CloseInputFile(sInputFileName);
	}
	delete fileHandle;
	// Entete des resultats
	cout << "Buffer size\tidentical\ttime" << endl;

	// Boucle sur des buffers de taille decroissante
	nBufferSize = 128 * lMB;
	bOk = true;
	while (bOk and nBufferSize > 32 * lKB)
	{
		// Suppression du fichier
		FileService::RemoveFile(sOutputFileName);
		outputFile.SetFileName(sOutputFileName);
		outputFile.SetBufferSize(nBufferSize + 10);
		outputFile.SetOpenOnDemandMode(bOpenOnDemand);

		// Attente necessaire pour eviter une erreur lors de l'ouverture du fichier
		// apparemment le fichier n'a pas le temps d'etre ferme correctement avant son ouverture et on a une
		// erreur "acces denied"
		SystemSleep(0.01);

		// Ecriture du fichier resultat
		bOk = outputFile.Open();
		if (not bOk)
			cout << "Error while opening file for write" << endl;
		else
		{
			timer.Reset();
			timer.Start();

			// Ecriture du fichier
			for (i = 0; i < buffer.GetSize(); i++)
			{
				outputFile.Write(buffer.GetAt(i));
				if (outputFile.IsError())
					cout << "Error while writing output "
					     << FileService::GetLastSystemIOErrorMessage() << endl;
			}

			// Fermeture du fichier
			bOk = outputFile.Close();
			timer.Stop();
			if (not bOk)
				cout << "Error while closing output file" << endl;
			// Resultat sinon
			else
			{
				bSameFile = PLRemoteFileService::FileCompare(sInputFileName, sOutputFileName);

				// Ecriture des reusltats
				cout << LongintToHumanReadableString(nBufferSize) << "\t" << BooleanToString(bSameFile)
				     << "\t" << timer.GetElapsedTime() << endl;
				ensure(bSameFile);
			}

			// Diminution de la taille du buffer
			nBufferSize /= 2;
		}
	}
}

const CharVector* OutputBufferedFile::GetCache() const
{
	return &fcCache.cvBuffer;
}

boolean OutputBufferedFile::Write(const char* sValue, int nCharNumber)
{
	int i;
	int nBeginChar;
	int nEndChar;

	require(IsOpened());
	require(sValue != NULL);
	require(nCharNumber >= 0);

	// Si chaine vide, on ne fait rien
	if (nCharNumber == 0)
		return not bIsError;

	// Reserve de la taille a ecrire pour eviter de fragmenter le disque
	// uniquement si taille plus grande que celle geree via un buffer dans le flush
	if (nCharNumber > nBufferSize)
		ReserveExtraSize(nCharNumber);

	// Si on n'a pas assez de place dans le buffer
	if (nCharNumber >= GetAvailableSpace())
	{
		// On rempli le premier buffer au maximum
		nEndChar = GetAvailableSpace();
		for (i = 0; i < nEndChar; i++)
		{
			fcCache.SetAt(nCurrentBufferSize, sValue[i]);
			nCurrentBufferSize++;
		}
		nBeginChar = nEndChar;

		// On vide le buffer
		FlushCache();

		// Ecriture des caractere restants par buffer
		while (not IsError() and nBeginChar < nCharNumber)
		{
			// Cas d'un buffer entier a ecrire
			nEndChar = nBeginChar + nBufferSize;
			if (nEndChar <= nCharNumber)
			{
				for (i = nBeginChar; i < nEndChar; i++)
				{
					fcCache.SetAt(nCurrentBufferSize, sValue[i]);
					nCurrentBufferSize++;
				}
				nBeginChar = nEndChar;

				// On vide le buffer
				FlushCache();
			}
			// Cas d'une partie de buffer
			else
			{
				for (i = nBeginChar; i < nCharNumber; i++)
				{
					fcCache.SetAt(nCurrentBufferSize, sValue[i]);
					nCurrentBufferSize++;
				}
				nBeginChar = nCharNumber;
			}
		}
	}
	// Si on a assez de place on remplit le buffer
	else
	{
		for (i = 0; i < nCharNumber; i++)
		{
			fcCache.SetAt(nCurrentBufferSize, sValue[i]);
			nCurrentBufferSize++;
		}
	}

	if (GetOpenOnDemandMode())
	{
		// Fermeture du fichier
		// Le contenu restant dans le buffer sera ecrit lors du prochain flush ou lors du Close final
		if (IsPhysycalOpen())
			PhysicalClose();
	}
	return not bIsError;
}

boolean OutputBufferedFile::WriteSubPart(const CharVector* cvValue, int nBeginOffset, int nLength)
{
	// Adaptation du code de  OutputBufferedFile::Write(const char* sValue, int nCharNumber)
	int nBeginChar;
	int nEndChar;
	int nLastChar;

	require(IsOpened());
	require(cvValue != NULL);
	require(cvValue->GetSize() >= 0);
	require(0 <= nBeginOffset and nBeginOffset <= cvValue->GetSize());
	require(nLength >= 0);
	require(nBeginOffset + nLength <= cvValue->GetSize());

	// Caracteres de debut et fin a traiter
	nBeginChar = nBeginOffset;
	nLastChar = nBeginOffset + nLength;

	// Reserve de la taille a ecrire pour eviter de fragmenter le disque
	// seulement dans le cas ou l'ecriture est faite en plusieurs passes
	// car la reserve est effectuee automatiquement dans le flush via writeToFile
	if (nLength > nBufferSize)
		ReserveExtraSize(nLength);

	// Si on n'a pas assez de place dans le buffer
	if (nLength >= GetAvailableSpace())
	{
		// On remplit le premier buffer au maximum
		while (nCurrentBufferSize < nBufferSize)
		{
			fcCache.SetAt(nCurrentBufferSize, cvValue->GetAt(nBeginChar));
			nCurrentBufferSize++;
			nBeginChar++;
		}

		// On vide le buffer
		FlushCache();

		// Ecriture des caracteres restants par buffer
		while (not IsError() and nBeginChar < nLastChar)
		{
			// Cas d'un buffer entier a ecrire
			nEndChar = nBeginChar + nBufferSize;
			assert(nCurrentBufferSize == 0);
			if (nEndChar <= nLastChar)
			{
				while (nBeginChar < nEndChar)
				{
					fcCache.SetAt(nCurrentBufferSize, cvValue->GetAt(nBeginChar));
					nCurrentBufferSize++;
					nBeginChar++;
				}
				assert(nBeginChar == nEndChar);

				// On vide le buffer
				FlushCache();
			}
			// Cas d'une partie de buffer
			else
			{
				while (nBeginChar < nLastChar)
				{
					fcCache.SetAt(nCurrentBufferSize, cvValue->GetAt(nBeginChar));
					nCurrentBufferSize++;
					nBeginChar++;
				}
				assert(nBeginChar == nLastChar);
			}
		}
	}
	// Si on a assez de place, on remplit le buffer
	else
	{
		while (nBeginChar < nLastChar)
		{
			fcCache.SetAt(nCurrentBufferSize, cvValue->GetAt(nBeginChar));
			nCurrentBufferSize++;
			nBeginChar++;
		}
	}

	if (GetOpenOnDemandMode())
	{
		// Fermeture du fichier
		// Le contenu restant dans le buffer sera ecrit lors du prochain flush ou lors du Close final
		if (IsPhysycalOpen())
			PhysicalClose();
	}

	return not bIsError;
}

boolean OutputBufferedFile::WriteToFile(int nSizeToWrite)
{
	boolean bOk = true;
	int nHugeBufferSize;
	int nHugeWriteSize;
	char* sBuffer;
	int nLocalWrite;
	longint lWrittenNumber;
	int nSizeWritten;

	assert(fileHandle != NULL);

	if (nSizeToWrite > 0)
	{
		if (not bIsPhysicalOpen)
		{
			assert(GetOpenOnDemandMode());
			bOk = PhysicalOpen();
		}
		if (bOk)
		{
			if (nSizeToWrite >= nDefaultBufferSize)
				nHugeBufferSize = max(GetPreferredBufferSize(), (int)nDefaultBufferSize);
			else
				nHugeBufferSize = GetPreferredBufferSize();

			sBuffer = GetHugeBuffer(nHugeBufferSize);

			// Acces a la taille effective du buffer, potentiellement plus grande que ce qui a ete demande
			// s'il a ete redimmensionne par ailleurs
			nHugeBufferSize = GetHugeBufferSize();
			assert(nHugeBufferSize >= GetPreferredBufferSize());

			// Calcul de la taille a ecrire en multiple de GetPreferredBufferSize
			// On suppose que chaque technologie est potentiellement plus efficace avec des multitples de sa
			// GetPreferredBufferSize
			nHugeWriteSize = (nHugeBufferSize / GetPreferredBufferSize()) * GetPreferredBufferSize();
			assert(nHugeWriteSize > 0);

			// Reserve la taille qui va etre ecrite pour eviter la fragmentation du disque
			fileHandle->ReserveExtraSize(nSizeToWrite);

			// Boucle d'ecriture
			nSizeWritten = 0;
			while (bOk and nSizeToWrite > 0)
			{
				nLocalWrite = min(nHugeWriteSize, nSizeToWrite);
				fcCache.cvBuffer.ExportBuffer(nSizeWritten, nLocalWrite, sBuffer);
				nSizeWritten += nLocalWrite;
				lWrittenNumber = fileHandle->Write(sBuffer, sizeof(char), nLocalWrite);
				assert(lWrittenNumber == 0 or lWrittenNumber == nLocalWrite);
				lTotalPhysicalWriteCalls++;
				lTotalPhysicalWriteBytes += nLocalWrite;
				nSizeToWrite -= nLocalWrite;
				bOk = lWrittenNumber != 0;

				// reduction de la taille du buffer
				nCurrentBufferSize -= nLocalWrite;
			}

			if (not bOk)
				AddError("Unable to write file (" + fileHandle->GetLastErrorMessage() + ")");
			else
			{
				if (nCurrentBufferSize > 0)
				{ // On decale ce qui reste au debut du cache : ca sera ecrit lors du prochain Flush
					MoveLastSegmentsToHead(&fcCache.cvBuffer,
							       nSizeWritten / InternalGetBlockSize());
				}
			}
		}
	}
	bIsError = not bOk;

	return not bIsError;
}

boolean OutputBufferedFile::FlushCache()
{
	boolean bOk = true;
	int nSizeToWrite;

	require(IsOpened());
	require(nBufferSize >= 0);
	require(GetFileName() != "");

	// Arret si traitement inutile ou si erreur lors de l'ouverture
	if (nCurrentBufferSize == 0)
		return not bIsError;
	if (bIsError)
		return false;

	if (bOk)
	{
		// Ecriture d'un multiple de  GetPreferredBufferSize() sauf dans la cas particulier ou
		// le buffer est plus petit que GetPreferredBufferSize
		if (nBufferSize > GetPreferredBufferSize())
			nSizeToWrite = (nCurrentBufferSize / GetPreferredBufferSize()) * GetPreferredBufferSize();
		else
			nSizeToWrite = nCurrentBufferSize;

		// Ecriture dans le fichier par tranche de GetPreferredBufferSize
		WriteToFile(nSizeToWrite);
	}
	else
		bIsError = true;

	// Dans le mode OpenOnDemand on ne ferme pas a dessein,
	// Pour eviter les ouvertures fermetures successives, on fermera
	// 'a la main' le fichier
	return not bIsError;
}

boolean OutputBufferedFile::PhysicalOpen()
{
	boolean bOk;

	assert(not bIsPhysicalOpen);

	// Ouverture du fichier
	if (not bNextOpenOnAppend)
	{
		bOk = fileHandle->OpenOutputFile(GetFileName());
		bNextOpenOnAppend = true;
	}
	else
		bOk = fileHandle->OpenOutputFileForAppend(GetFileName());

	if (bOk)
	{
		bIsPhysicalOpen = true;
		bIsError = false;
	}
	else
	{
		bIsError = true;
		AddError("Unable to open output file (" + fileHandle->GetLastErrorMessage() + ")");
	}
	return bOk;
}

boolean OutputBufferedFile::PhysicalClose()
{
	boolean bOk;

	assert(bIsPhysicalOpen);

	// Fermeture du fichier
	bOk = fileHandle->CloseOutputFile(sFileName);
	if (not bOk)
	{
		bIsError = true;
		AddError("Unable to close file (" + fileHandle->GetLastErrorMessage() + ")");
	}
	bIsPhysicalOpen = false;
	return bOk;
}
