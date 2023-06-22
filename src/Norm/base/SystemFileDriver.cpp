// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SystemFileDriver.h"
#include "InputBufferedFile.h"

SystemFileDriver::SystemFileDriver()
{
	nSchemeCharNumber = -1;
}

SystemFileDriver::~SystemFileDriver() {}

boolean SystemFileDriver::CreateEmptyFile(const char* sPathName)
{
	boolean bOk = false;
	void* fileHandler;

	assert(IsConnected());
	assert(IsManaged(sPathName));

	fileHandler = Open(sPathName, 'w');
	if (fileHandler != NULL)
		bOk = Close(fileHandler);
	return bOk;
}

boolean SystemFileDriver::MakeDirectories(const char* pathName) const
{
	boolean bOk = true;
	int nEnd;
	ALString sDirectory;
	ALString sPathName;

	require(IsManaged(pathName));

	// On ne cree pas les repertoires que si le chemin est non vide
	if (strcmp(pathName, "") != 0)
	{
		// Initialisation de la position apres le 'scheme://'
		GetSchemeCharNumber();
		nEnd = nSchemeCharNumber + 3;

		// Cas d'un debut de fichier en 'scheme:///'
		sPathName = pathName;
		if (sPathName.GetLength() > nEnd and sPathName.GetAt(nEnd) == '/')
			nEnd++;

		// Parcours des repertoire intermediaires
		while (nEnd < sPathName.GetLength())
		{
			// Extraction du directory intermediaire suivant
			while (nEnd < sPathName.GetLength() and not FileService::IsFileSeparator(sPathName.GetAt(nEnd)))
				nEnd++;
			sDirectory = sPathName.Left(nEnd);
			nEnd++;

			// Creation du directory
			if (sDirectory != "" and not Exist(sDirectory))
			{
				bOk = MakeDirectory(sDirectory);
				if (not bOk)
					return false;
			}
		}
		bOk = Exist(sPathName);
	}
	return bOk;
}

boolean SystemFileDriver::ReserveExtraSize(longint lSize, void* stream)
{
	return true;
}

boolean SystemFileDriver::CopyFileFromLocal(const char* sSourceFilePathName, const char* sDestFilePathName)
{
	FILE* fSource;
	void* fDest;
	boolean bOk;
	const int nBufferSize = 1 * lMB;
	char* cBuffer;
	int nRead = -1;

	require(IsManaged(sDestFilePathName));

	// TODO utiliser le meme buffer que celui de InputBufferedFile
	cBuffer = NewCharArray(nBufferSize);

	bOk = FileService::OpenInputBinaryFile(sSourceFilePathName, fSource);
	if (bOk)
	{
		fDest = this->Open(sDestFilePathName, 'w');
		bOk = fDest != NULL;
		if (bOk)
		{
			nRead = nBufferSize;
			while (nRead == nBufferSize)
			{
				nRead = (int)std::fread(cBuffer, sizeof(char), nBufferSize, fSource);
				if (nRead != -1)
					this->fwrite(cBuffer, sizeof(char), nRead, fDest);
			}
			bOk = this->Close(fDest);
		}
		FileService::CloseInputBinaryFile(sSourceFilePathName, fSource);
	}
	if (nRead == -1)
		bOk = false;
	DeleteCharArray(cBuffer);
	return bOk;
}

boolean SystemFileDriver::CopyFileToLocal(const char* sSourceFilePathName, const char* sDestFilePathName)
{
	void* fSource;
	FILE* fDest;
	boolean bOk;
	const int nBufferSize = 1 * lMB;
	char* cBuffer;
	int nRead;

	require(IsManaged(sSourceFilePathName));

	// TODO utiliser le meme buffer que celui de InputBufferedFile
	cBuffer = NewCharArray(nBufferSize);

	bOk = FileService::OpenOutputBinaryFile(sDestFilePathName, fDest);
	if (bOk)
	{
		fSource = this->Open(sSourceFilePathName, 'r');
		bOk = fSource != NULL;
		if (bOk)
		{
			nRead = nBufferSize;
			while (nRead == nBufferSize)
			{
				nRead = (int)this->fread(cBuffer, sizeof(char), nBufferSize, fSource);
				if (nRead != -1)
					std::fwrite(cBuffer, sizeof(char), nRead, fDest);
			}
			this->Close(fSource);
		}
		bOk = FileService::CloseOutputBinaryFile(sDestFilePathName, fDest) and bOk;
	}
	DeleteCharArray(cBuffer);
	return bOk;
}

boolean SystemFileDriver::IsManaged(const char* sFileName) const
{
	require(sFileName != NULL);
	return FileService::GetURIScheme(sFileName) == GetScheme() and FileService::IsURIWellFormed(sFileName);
}

const char* SystemFileDriver::GetURIFilePathName(const char* sFileName) const
{
	require(IsManaged(sFileName));
	return FileService::GetURIFilePathName(sFileName);
}
