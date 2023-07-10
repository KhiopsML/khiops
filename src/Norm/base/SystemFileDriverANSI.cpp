// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SystemFileDriverANSI.h"

SystemFileDriverANSI::SystemFileDriverANSI() {}

SystemFileDriverANSI::~SystemFileDriverANSI() {}

const char* SystemFileDriverANSI::GetDriverName() const
{
	return "ANSI driver";
}

const char* SystemFileDriverANSI::GetScheme() const
{
	return "";
}

const char* SystemFileDriverANSI::GetVersion() const
{
	return "1.0.0";
}

boolean SystemFileDriverANSI::IsReadOnly() const
{
	return false;
}

boolean SystemFileDriverANSI::Connect()
{
	return true;
}

boolean SystemFileDriverANSI::Disconnect()
{
	return true;
}

boolean SystemFileDriverANSI::IsConnected() const
{
	return true;
}

longint SystemFileDriverANSI::GetSystemPreferredBufferSize() const
{
	return 1 * lMB;
}

boolean SystemFileDriverANSI::FileExists(const char* sFilePathName) const
{
	return FileService::FileExists(FileService::GetURIFilePathName(sFilePathName));
}

boolean SystemFileDriverANSI::DirExists(const char* sFilePathName) const
{
	return FileService::DirExists(FileService::GetURIFilePathName(sFilePathName));
}

longint SystemFileDriverANSI::GetFileSize(const char* sFilePathName) const
{
	return FileService::GetFileSize(FileService::GetURIFilePathName(sFilePathName));
}

void* SystemFileDriverANSI::Open(const char* sFilePathName, char cMode)
{
	boolean bOk;
	ALString sTmp;
	void* handle;

	require(cMode == 'r' or cMode == 'w' or cMode == 'a');

	// Ouverture en lecture
	handle = NULL;
	if (cMode == 'r')
	{
		handle = fopen(sFilePathName, "rb");
	}
	// Ouverture en ecriture
	else if (cMode == 'w')
	{
		handle = fopen(sFilePathName, "wb");
	}
	// Ouverture en append
	else if (cMode == 'a')
	{
		// On ouvre le fichier en mode "r+b" (et non "ab") pour pouvoir utiliser des Seek par la suite,
		// et exploiter la methode ReserveExtraSize sans interaction avec le mode append
		handle = fopen(sFilePathName, "r+b");
		if (handle != NULL)
		{
			// Pour se mettre en append, on se deplace vers la fin de fichier
			bOk = FileService::SystemSeekPositionInBinaryFile((FILE*)handle, 0, SEEK_END);

			// Fermeture du fichier si erreur
			if (not bOk)
			{
				// TODO virer le message
				Global::AddError("File", sFilePathName,
						 sTmp + "Unable to call fseek to end of file (" +
						     GetLastErrorMessage() + ")");

				// Fermeture directe du fichier, pour eviter un message d'erreur additionnel
				fclose((FILE*)handle);
				handle = NULL;
			}
		}
	}
	return handle;
}

boolean SystemFileDriverANSI::Close(void* stream)
{
	return fclose((FILE*)stream) == 0;
}

longint SystemFileDriverANSI::Fread(void* ptr, size_t size, size_t count, void* stream)
{
	longint lRes;

	// Lecture dans le fichier
	lRes = std::fread(ptr, size, count, (FILE*)stream);
	if (lRes != longint(count) and ferror((FILE*)stream))
		lRes = -1;
	return lRes;
}

boolean SystemFileDriverANSI::SeekPositionInFile(longint lPosition, void* stream)
{
	return FileService::SeekPositionInBinaryFile((FILE*)stream, lPosition);
}

const char* SystemFileDriverANSI::GetLastErrorMessage() const
{
	return strerror(errno);
}

longint SystemFileDriverANSI::Fwrite(const void* ptr, size_t size, size_t count, void* stream)
{
	longint lWrite;

	// Ecriture dans le fichier
	lWrite = std::fwrite(ptr, size, count, (FILE*)stream);
	if (lWrite != longint(count) and ferror((FILE*)stream))
		lWrite = 0;
	return lWrite;
}

boolean SystemFileDriverANSI::Flush(void* stream)
{
	int nRet;
	nRet = std::fflush((FILE*)stream);
	return nRet == 0;
}

boolean SystemFileDriverANSI::RemoveFile(const char* sFilePathName) const
{
	return FileService::RemoveFile(FileService::GetURIFilePathName(sFilePathName));
}

boolean SystemFileDriverANSI::MakeDirectory(const char* sPathName) const
{
	return FileService::MakeDirectory(FileService::GetURIFilePathName(sPathName));
}

boolean SystemFileDriverANSI::RemoveDirectory(const char* sFilePathName) const
{
	return FileService::RemoveDirectory(FileService::GetURIFilePathName(sFilePathName));
}

longint SystemFileDriverANSI::GetDiskFreeSpace(const char* sPathName) const
{
	return FileService::GetDiskFreeSpace(FileService::GetURIFilePathName(sPathName));
}

boolean SystemFileDriverANSI::ReserveExtraSize(longint lSize, void* stream)
{
	require(lSize >= 0);
	return FileService::ReserveExtraSize((FILE*)stream, lSize);
}
