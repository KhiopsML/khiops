// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SystemFileDriver.h"
#include "InputBufferedFile.h"
#include "HugeBuffer.h"

SystemFileDriver::SystemFileDriver()
{
	nSchemeCharNumber = -1;
}

SystemFileDriver::~SystemFileDriver() {}

longint SystemFileDriver::GetSystemPreferredBufferSize() const
{
	return 0;
}

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
			if (sDirectory != "" and not DirExists(sDirectory))
			{
				bOk = MakeDirectory(sDirectory);
				if (not bOk)
					return false;
			}
		}
		bOk = DirExists(sPathName);
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
	char* cBuffer;
	int nRead = -1;
	int nWrite;
	ALString sTmp;
	int nBufferSize;

	require(IsManaged(sDestFilePathName));

	// On caste en int dans le min car sinon avec gcc on a "undefined reference to
	// SysteFileDriver::nMaxBufferSizeForCopying
	nBufferSize =
	    min(PLRemoteFileService::GetPreferredBufferSize(sDestFilePathName), (int)nMaxBufferSizeForCopying);
	cBuffer = GetHugeBuffer(nBufferSize);
	errno = 0;

	bOk = FileService::OpenInputBinaryFile(sSourceFilePathName, fSource);
	if (bOk)
	{
		fDest = Open(sDestFilePathName, 'w');
		bOk = fDest != NULL;
		if (bOk)
		{
			nRead = nBufferSize;
			while (nRead == nBufferSize)
			{
				nRead = (int)std::fread(cBuffer, sizeof(char), nBufferSize, fSource);
				if (ferror(fSource) == 0)
				{
					nWrite = (int)Fwrite(cBuffer, sizeof(char), nRead, fDest);
					if (nWrite == 0)
					{
						Global::AddError("File", sDestFilePathName,
								 sTmp + "Unable to write file (" +
								     GetLastErrorMessage() + ")");
						bOk = false;
						break;
					}
				}
				else
				{
					bOk = false;
					Global::AddError("File", sSourceFilePathName,
							 sTmp + "Unable to read file (" + GetLastErrorMessage() + ")");
					break;
				}
			}
			if (not Close(fDest))
			{
				bOk = false;
				Global::AddError("File", sDestFilePathName,
						 sTmp + "Unable to close file (" + GetLastErrorMessage() + ")");
			}
		}
		else
		{
			Global::AddError("File", sDestFilePathName,
					 sTmp + "Unable to open output file (" + GetLastErrorMessage() + ")");
		}
		bOk = FileService::CloseInputBinaryFile(sSourceFilePathName, fSource) and bOk;
	}
	return bOk;
}

boolean SystemFileDriver::CopyFileToLocal(const char* sSourceFilePathName, const char* sDestFilePathName)
{
	void* fSource;
	FILE* fDest;
	boolean bOk;
	char* cBuffer;
	int nRead;
	int nWrite;
	ALString sTmp;
	int nBufferSize;

	require(IsManaged(sSourceFilePathName));

	errno = 0;

	// On caste en int dans le min car sinon avec gcc on a "undefined reference to
	// SysteFileDriver::nMaxBufferSizeForCopying
	nBufferSize =
	    min(PLRemoteFileService::GetPreferredBufferSize(sSourceFilePathName), (int)nMaxBufferSizeForCopying);
	cBuffer = GetHugeBuffer(nBufferSize);

	bOk = FileService::OpenOutputBinaryFile(sDestFilePathName, fDest);
	if (bOk)
	{
		fSource = Open(sSourceFilePathName, 'r');
		bOk = fSource != NULL;
		if (bOk)
		{
			nRead = nBufferSize;
			while (nRead == nBufferSize)
			{
				nRead = (int)Fread(cBuffer, sizeof(char), nBufferSize, fSource);
				if (nRead != -1)
				{
					nWrite = (int)std::fwrite(cBuffer, sizeof(char), nRead, fDest);
					if (nWrite != nRead and ferror(fDest) != 0)
					{
						Global::AddError("File", sDestFilePathName,
								 sTmp + "Unable to write file (" +
								     GetLastErrorMessage() + ")");
						bOk = false;
						break;
					}
				}
				else
				{
					Global::AddError("File", sSourceFilePathName,
							 sTmp + "Unable to read file (" + GetLastErrorMessage() + ")");
					bOk = false;
					break;
				}
			}
			bOk = Close(fSource) and bOk;
		}
		else
		{
			Global::AddError("File", sDestFilePathName,
					 sTmp + "Unable to open output file (" + GetLastErrorMessage() + ")");
		}

		bOk = FileService::CloseOutputBinaryFile(sDestFilePathName, fDest) and bOk;
	}
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
