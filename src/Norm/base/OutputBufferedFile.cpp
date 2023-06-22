// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "OutputBufferedFile.h"
#include "TaskProgression.h"

OutputBufferedFile::OutputBufferedFile() {}

OutputBufferedFile::~OutputBufferedFile()
{
	if (IsOpened())
		fileHandle->CloseOutputFile(sFileName);
}

void OutputBufferedFile::CopyFrom(const OutputBufferedFile* bufferedFile)
{
	BufferedFile::CopyFrom(bufferedFile);
	nCurrentBufferSize = 0;
}

boolean OutputBufferedFile::Open()
{
	boolean bOk;

	require(not IsOpened());
	require(GetFileName() != "");

	// Initialisation de la taille du buffer
	bOk = AllocateBuffer();

	// Ouverture du fichier
	if (bOk)
	{
		if (GetFileDriverCreator()->IsLocal(sFileName))
		{
			fileHandle = new SystemFile;

			// Ouverture du fichier
			bOk = fileHandle->OpenOutputFile(GetFileName());
			if (not bOk and fileHandle != NULL)
			{
				delete fileHandle;
				fileHandle = NULL;
			}
		}
		else
		{
			bOk = false;
			assert(false);
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

	// TODO require que le fichier est local
	require(not IsOpened());
	require(GetFileName() != "");

	// Initialisation de la taille du buffer
	bOk = AllocateBuffer();

	// Ouverture du fichier
	if (bOk)
	{
		if (GetFileDriverCreator()->IsLocal(sFileName))
		{
			// Ouverture du fichier
			fileHandle = new SystemFile;
			bOk = fileHandle->OpenOutputFileForAppend(GetFileName());
			if (not bOk)
			{
				delete fileHandle;
				fileHandle = NULL;
			}
		}
		else
		{
			bOk = false;
			assert(false);
		}
	}
	bIsOpened = bOk;
	bIsError = not bOk;
	nCurrentBufferSize = 0;
	return IsOpened();
}

boolean OutputBufferedFile::Close()
{
	boolean bOk;

	require(IsOpened());
	assert(fileHandle != NULL);

	if (GetFileDriverCreator()->IsLocal(sFileName))
	{
		// Ecriture du contenu du buffer
		Flush();
		bOk = not bIsError;

		// Fermeture du fichier
		bOk = fileHandle->CloseOutputFile(sFileName) and bOk;
		delete fileHandle;
		fileHandle = NULL;
	}
	else
	{
		bOk = false;
		assert(false);
	}
	ensure(fileHandle == NULL);
	bIsOpened = false;
	bIsError = false;
	nCurrentBufferSize = 0;
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

	// Acces au driver uniquement dans le cas local
	if (fileHandle != NULL)
	{
		assert(GetFileDriverCreator()->IsLocal(sFileName));

		// On reserve de la taille que si necessaire par rapport a la taille de buffer courante geree par
		// rapport au flush et que l'on s'apprete a consommer plus de un buffer
		if (GetCurrentBufferSize() + lSize > 2 * GetBufferSize())
			fileHandle->ReserveExtraSize(GetCurrentBufferSize() + lSize);
	}
}

void OutputBufferedFile::TestWriteFile(const ALString& sInputFileName, const ALString& sOutputFileName)
{
	longint lFileSize;
	longint lOutputFileSize;
	int nBufferSize;
	FileBuffer buffer;
	SystemFile* fileHandle;
	boolean bOk;
	OutputBufferedFile outputFile;
	Timer timer;
	int i;
	OutputBufferedFile errorSender;

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
	cout << "Buffer size\tsize diff\ttime" << endl;

	// Boucle sur des buffers de taille decroissante
	nBufferSize = 128 * lMB;
	bOk = true;
	while (bOk and nBufferSize > 32)
	{
		// Suppression du fichier
		FileService::RemoveFile(sOutputFileName);
		outputFile.SetFileName(sOutputFileName);
		outputFile.SetBufferSize(nBufferSize);

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
					cout << "Error while writing output" << endl;
			}

			// Fermeture du fichier
			bOk = outputFile.Close();
			timer.Stop();
			if (not bOk)
				cout << "Error while closing output file" << endl;
			// Resultat sinon
			else
			{
				// Taille du nouveau fichier
				lOutputFileSize = PLRemoteFileService::GetFileSize(sOutputFileName);

				// Ecriture des reusltats
				cout << nBufferSize << "\t" << lOutputFileSize - lFileSize << "\t"
				     << timer.GetElapsedTime() << endl;
			}

			// Diminution de la taille du buffer
			nBufferSize /= 2;
		}
	}
}

void OutputBufferedFile::Write(const char* sValue, int nCharNumber)
{
	int i;
	int nBeginChar;
	int nEndChar;

	require(IsOpened());
	require(sValue != NULL);
	require(nCharNumber >= 0);

	// Si chaine vide, on ne fait rien
	if (nCharNumber == 0)
		return;

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
			fbBuffer.SetAt(nCurrentBufferSize, sValue[i]);
			nCurrentBufferSize++;
		}
		nBeginChar = nEndChar;

		// On vide le buffer
		Flush();
		if (IsError())
			return;

		// Ecriture des caractere restants par buffer
		while (nBeginChar < nCharNumber)
		{
			// Cas d'un buffer entier a ecrire
			nEndChar = nBeginChar + nBufferSize;
			if (nEndChar <= nCharNumber)
			{
				for (i = nBeginChar; i < nEndChar; i++)
				{
					fbBuffer.SetAt(nCurrentBufferSize, sValue[i]);
					nCurrentBufferSize++;
				}
				nBeginChar = nEndChar;

				// On vide le buffer
				Flush();
				if (IsError())
					return;
			}
			// Cas d'une partie de buffer
			else
			{
				for (i = nBeginChar; i < nCharNumber; i++)
				{
					fbBuffer.SetAt(nCurrentBufferSize, sValue[i]);
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
			fbBuffer.SetAt(nCurrentBufferSize, sValue[i]);
			nCurrentBufferSize++;
		}
	}
}

void OutputBufferedFile::WriteSubPart(const CharVector* cvValue, int nBeginOffset, int nLength)
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
			fbBuffer.SetAt(nCurrentBufferSize, cvValue->GetAt(nBeginChar));
			nCurrentBufferSize++;
			nBeginChar++;
		}

		// On vide le buffer
		Flush();
		if (IsError())
			return;

		// Ecriture des caracteres restants par buffer
		while (nBeginChar < nLastChar)
		{
			// Cas d'un buffer entier a ecrire
			nEndChar = nBeginChar + nBufferSize;
			assert(nCurrentBufferSize == 0);
			if (nEndChar <= nLastChar)
			{
				while (nBeginChar < nEndChar)
				{
					fbBuffer.SetAt(nCurrentBufferSize, cvValue->GetAt(nBeginChar));
					nCurrentBufferSize++;
					nBeginChar++;
				}
				assert(nBeginChar == nEndChar);

				// On vide le buffer
				Flush();
				if (IsError())
					return;
			}
			// Cas d'une partie de buffer
			else
			{
				while (nBeginChar < nLastChar)
				{
					fbBuffer.SetAt(nCurrentBufferSize, cvValue->GetAt(nBeginChar));
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
			fbBuffer.SetAt(nCurrentBufferSize, cvValue->GetAt(nBeginChar));
			nCurrentBufferSize++;
			nBeginChar++;
		}
	}
}

void OutputBufferedFile::Flush()
{
	boolean bOk;

	require(IsOpened());
	require(nBufferSize >= 0);
	require(GetFileName() != "");

	// Arret si traitement inutile
	if (nCurrentBufferSize == 0)
		return;

	// Ecriture soit en local
	if (GetFileDriverCreator()->IsLocal(sFileName))
	{
		bIsError = bIsError or fileHandle == NULL;
		if (bIsError)
			return;

		// Ecriture
		bOk = fbBuffer.WriteToFile(fileHandle, GetCurrentBufferSize(), this);
		bIsError = not bOk;
	}
	else
	{
		assert(fileDriver != NULL);

		// Ecriture du contenu du buffer
		bOk = fileDriver->Flush(this);
		bIsError = not bOk;
	}

	// On vide le buffer
	nCurrentBufferSize = 0;
}