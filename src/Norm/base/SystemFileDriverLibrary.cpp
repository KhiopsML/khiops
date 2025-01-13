// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SystemFileDriverLibrary.h"

SystemFileDriverLibrary::SystemFileDriverLibrary()
{
	Clean();
}

SystemFileDriverLibrary::~SystemFileDriverLibrary() {}

const char* SystemFileDriverLibrary::GetDriverName() const
{
	require(IsLibraryLoaded());
	return ptr_driver_getDriverName();
}

const char* SystemFileDriverLibrary::GetScheme() const
{
	require(IsLibraryLoaded());
	return ptr_driver_getScheme();
}

const char* SystemFileDriverLibrary::GetVersion() const
{
	require(IsLibraryLoaded());
	return ptr_driver_getVersion();
}

boolean SystemFileDriverLibrary::IsReadOnly() const
{
	require(IsLibraryLoaded());
	return (ptr_driver_isReadOnly() == 1);
}

boolean SystemFileDriverLibrary::Connect()
{
	boolean bOk = true;
	ALString sTmp;
	require(IsLibraryLoaded());
	if (not IsConnected())
	{
		if (MemoryStatsManager::IsOpened())
			MemoryStatsManager::AddLog(sTmp + "driver [" + GetDriverName() + "] Connect Begin");
		bOk = (ptr_driver_connect() == 1);
		if (MemoryStatsManager::IsOpened())
			MemoryStatsManager::AddLog(sTmp + "driver [" + GetDriverName() + "] Connect End");
	}
	return bOk;
}

boolean SystemFileDriverLibrary::Disconnect()
{
	boolean bOk = true;
	ALString sTmp;

	require(IsLibraryLoaded());

	if (IsConnected())
	{
		if (MemoryStatsManager::IsOpened())
			MemoryStatsManager::AddLog(sTmp + "driver [" + GetDriverName() + "] Disconnect Begin");
		bOk = (ptr_driver_disconnect() == 1);
		if (MemoryStatsManager::IsOpened())
			MemoryStatsManager::AddLog(sTmp + "driver [" + GetDriverName() + "] Disconnect End");
	}

	// Dechargement de la bibliotheque
	UnloadLibrary();
	return bOk;
}

boolean SystemFileDriverLibrary::IsConnected() const
{
	require(IsLibraryLoaded());
	return (ptr_driver_isConnected() == 1);
}

longint SystemFileDriverLibrary::GetSystemPreferredBufferSize() const
{
	longint lPrefferedSize;
	require(IsLibraryLoaded());
	if (ptr_driver_getSystemPreferredBufferSize == NULL)
		return 0;
	else
	{
		lPrefferedSize = ptr_driver_getSystemPreferredBufferSize();
		if (lPrefferedSize < 0)
			AddFatalError("The preferred size of the buffer must be positive");
		return lPrefferedSize;
	}
}

boolean SystemFileDriverLibrary::FileExists(const char* sFilePathName) const
{
	require(IsConnected());
	require(IsManaged(sFilePathName));
	return (ptr_driver_fileExists(sFilePathName) == 1);
}

boolean SystemFileDriverLibrary::DirExists(const char* sFilePathName) const
{
	require(IsConnected());
	require(IsManaged(sFilePathName));
	return (ptr_driver_dirExists(sFilePathName) == 1);
}

longint SystemFileDriverLibrary::GetFileSize(const char* sFilePathName) const
{
	require(IsConnected());
	require(IsManaged(sFilePathName));
	return ptr_driver_getFileSize(sFilePathName);
}

void* SystemFileDriverLibrary::Open(const char* sFilePathName, char cMode)
{
	require(IsConnected());
	require(cMode == 'r' or cMode == 'w' or cMode == 'a');
	require(cMode == 'r' or not IsReadOnly());
	return ptr_driver_fopen(sFilePathName, cMode);
}

boolean SystemFileDriverLibrary::Close(void* stream)
{
	boolean bOk;

	require(IsConnected());
	require(stream != NULL);

	bOk = ptr_driver_fclose(stream) == 0;
	stream = NULL;
	return bOk;
}

longint SystemFileDriverLibrary::Fread(void* ptr, size_t size, size_t count, void* stream)
{
	require(IsConnected());
	require(stream != NULL);
	return ptr_driver_fread(ptr, size, count, stream);
}

boolean SystemFileDriverLibrary::SeekPositionInFile(longint lPosition, void* stream)
{
	boolean bOk;

	require(IsConnected());
	require(stream != NULL);

	bOk = (ptr_driver_fseek(stream, lPosition, SEEK_SET) == 0);
	return bOk;
}

const char* SystemFileDriverLibrary::GetLastErrorMessage() const
{
	require(IsConnected());
	return ptr_driver_getlasterror();
}

longint SystemFileDriverLibrary::Fwrite(const void* ptr, size_t size, size_t count, void* stream)
{
	require(IsConnected());
	require(not IsReadOnly());
	require(stream != NULL);
	return ptr_driver_fwrite(ptr, size, count, stream);
}

boolean SystemFileDriverLibrary::Flush(void* stream)
{
	require(IsConnected());
	require(not IsReadOnly());
	require(stream != NULL);
	return ptr_driver_fflush(stream) != EOF;
}

boolean SystemFileDriverLibrary::RemoveFile(const char* sFilePathName) const
{
	require(IsConnected());
	require(not IsReadOnly());
	require(IsManaged(sFilePathName));
	return (ptr_driver_remove(sFilePathName) == 1);
}

boolean SystemFileDriverLibrary::MakeDirectory(const char* sURIPathName) const
{
	require(IsConnected());
	require(not IsReadOnly());
	require(IsManaged(sURIPathName));
	return (ptr_driver_mkdir(sURIPathName) == 1);
}

boolean SystemFileDriverLibrary::RemoveDirectory(const char* sFilePathName) const
{
	require(IsConnected());
	require(not IsReadOnly());
	require(IsManaged(sFilePathName));
	return (ptr_driver_rmdir(sFilePathName) == 1);
}

longint SystemFileDriverLibrary::GetDiskFreeSpace(const char* sPathName) const
{
	require(IsConnected());
	require(not IsReadOnly());
	require(IsManaged(sPathName));
	return ptr_driver_diskFreeSpace(sPathName);
}

boolean SystemFileDriverLibrary::CopyFileFromLocal(const char* sSourceFilePathName, const char* sDestFilePathName)
{
	require(IsConnected());
	require(not IsReadOnly());
	require(IsManaged(sDestFilePathName));

	// Appel de la methode ancetre si non disponible
	if (ptr_driver_copyFromLocal == NULL)
	{
		return SystemFileDriver::CopyFileFromLocal(sSourceFilePathName, sDestFilePathName);
	}
	// Appel de la methode de la librairie sinon
	else
		return ptr_driver_copyFromLocal(sSourceFilePathName, sDestFilePathName);
}

boolean SystemFileDriverLibrary::CopyFileToLocal(const char* sSourceFilePathName, const char* sDestFilePathName)
{
	require(IsConnected());
	require(not IsReadOnly());
	require(IsManaged(sSourceFilePathName));

	// Appel de la methode ancetre si non disponible
	if (ptr_driver_copyToLocal == NULL)
	{
		return SystemFileDriver::CopyFileToLocal(sSourceFilePathName, sDestFilePathName);
	}
	// Appel de la methode de la librairie sinon
	else
		return ptr_driver_copyToLocal(sSourceFilePathName, sDestFilePathName);
}

boolean SystemFileDriverLibrary::LoadLibrary(const ALString& sLibraryFilePathName)
{
	ALString sTmp;
	char sErrorMessage[SYSTEM_MESSAGE_LENGTH + 1];

	require(not IsLibraryLoaded());

	// On ne recharge pas la librairie si elle est deja chargee
	if (IsLibraryLoaded())
		return true;

	// Chargement de la librairie
	sLibraryName = FileService::GetFileName(sLibraryFilePathName);

	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog(sTmp + "driver [" + sLibraryName + "] LoadLibrary Begin");

	handleLibrary = LoadSharedLibrary(sLibraryFilePathName, sErrorMessage);
	if (handleLibrary == NULL)
	{
		AddError(sTmp + "Library loading error (" + sErrorMessage + ")");
		return false;
	}

	// Recherche des methodes read-only
	// Le flag d'erreur est mis a jour par BindToLibrary
	*(void**)(&ptr_driver_getDriverName) = BindToLibrary("driver_getDriverName", true);
	*(void**)(&ptr_driver_getVersion) = BindToLibrary("driver_getVersion", true);
	*(void**)(&ptr_driver_getScheme) = BindToLibrary("driver_getScheme", true);
	*(void**)(&ptr_driver_isReadOnly) = BindToLibrary("driver_isReadOnly", true);
	*(void**)(&ptr_driver_connect) = BindToLibrary("driver_connect", true);
	*(void**)(&ptr_driver_disconnect) = BindToLibrary("driver_disconnect", true);
	*(void**)(&ptr_driver_isConnected) = BindToLibrary("driver_isConnected", true);
	*(void**)(&ptr_driver_fileExists) = BindToLibrary("driver_fileExists", true);
	*(void**)(&ptr_driver_dirExists) = BindToLibrary("driver_dirExists", true);
	*(void**)(&ptr_driver_getFileSize) = BindToLibrary("driver_getFileSize", true);
	*(void**)(&ptr_driver_fopen) = BindToLibrary("driver_fopen", true);
	*(void**)(&ptr_driver_fclose) = BindToLibrary("driver_fclose", true);
	*(void**)(&ptr_driver_fseek) = BindToLibrary("driver_fseek", true);
	*(void**)(&ptr_driver_fread) = BindToLibrary("driver_fread", true);
	*(void**)(&ptr_driver_getlasterror) = BindToLibrary("driver_getlasterror", true);

	// Recherche des methodes read-write
	if (not bIsError and not IsReadOnly())
	{
		*(void**)(&ptr_driver_fwrite) = BindToLibrary("driver_fwrite", true);
		*(void**)(&ptr_driver_fflush) = BindToLibrary("driver_fflush", true);
		*(void**)(&ptr_driver_remove) = BindToLibrary("driver_remove", true);
		*(void**)(&ptr_driver_mkdir) = BindToLibrary("driver_mkdir", true);
		*(void**)(&ptr_driver_rmdir) = BindToLibrary("driver_rmdir", true);
		*(void**)(&ptr_driver_diskFreeSpace) = BindToLibrary("driver_diskFreeSpace", true);
	}

	// Methodes optionnelles
	*(void**)(&ptr_driver_copyToLocal) = BindToLibrary("driver_copyToLocal", false);
	*(void**)(&ptr_driver_copyFromLocal) = BindToLibrary("driver_copyFromLocal", false);
	*(void**)(&ptr_driver_getSystemPreferredBufferSize) =
	    BindToLibrary("driver_getSystemPreferredBufferSize", false);

	if (MemoryStatsManager::IsOpened())
		MemoryStatsManager::AddLog(sTmp + "driver [" + sLibraryName + "] LoadLibrary End");

	// Verification de la version
	if (*(void**)(&ptr_driver_getVersion) != NULL)
	{
		if (GetMajorVersion(GetVersion()) != 0)
		{
			bIsError = true;
			AddError(sTmp + "the driver major version must be 0 (current version is " + GetVersion() + ")");
		}
	}

	// Nettoyage complet si erreur
	if (bIsError)
	{
		UnloadLibrary();
		Clean();
		bIsError = true;
	}
	return not bIsError;
}

void SystemFileDriverLibrary::UnloadLibrary()
{
	if (IsLibraryLoaded())
	{
		FreeSharedLibrary(handleLibrary);
		Clean();
	}
}

boolean SystemFileDriverLibrary::IsLibraryLoaded() const
{
	return handleLibrary != NULL;
}

const ALString SystemFileDriverLibrary::GetClassLabel() const
{
	return "File driver";
}

const ALString SystemFileDriverLibrary::GetObjectLabel() const
{
	return sLibraryName;
}

void SystemFileDriverLibrary::Clean()
{
	bIsError = false;
	handleLibrary = NULL;
	*(void**)(&ptr_driver_getDriverName) = NULL;
	*(void**)(&ptr_driver_getVersion) = NULL;
	*(void**)(&ptr_driver_getScheme) = NULL;
	*(void**)(&ptr_driver_isReadOnly) = NULL;
	*(void**)(&ptr_driver_connect) = NULL;
	*(void**)(&ptr_driver_disconnect) = NULL;
	*(void**)(&ptr_driver_isConnected) = NULL;
	*(void**)(&ptr_driver_getSystemPreferredBufferSize) = NULL;
	*(void**)(&ptr_driver_fileExists) = NULL;
	*(void**)(&ptr_driver_dirExists) = NULL;
	*(void**)(&ptr_driver_getFileSize) = NULL;
	*(void**)(&ptr_driver_fopen) = NULL;
	*(void**)(&ptr_driver_fclose) = NULL;
	*(void**)(&ptr_driver_fseek) = NULL;
	*(void**)(&ptr_driver_fread) = NULL;
	*(void**)(&ptr_driver_fwrite) = NULL;
	*(void**)(&ptr_driver_fflush) = NULL;
	*(void**)(&ptr_driver_mkdir) = NULL;
	*(void**)(&ptr_driver_remove) = NULL;
	*(void**)(&ptr_driver_rmdir) = NULL;
	*(void**)(&ptr_driver_diskFreeSpace) = NULL;
	*(void**)(&ptr_driver_copyToLocal) = NULL;
	*(void**)(&ptr_driver_copyFromLocal) = NULL;
	*(void**)(&ptr_driver_getlasterror) = NULL;
}

void* SystemFileDriverLibrary::BindToLibrary(const ALString& sMethodName, boolean bMandatory)
{
	void* pFunction;
	require(handleLibrary != NULL);

	// On conditionne par bIsError pour ne pas multiplier les messages d'erreur
	pFunction = NULL;
	if (not bIsError)
	{
		pFunction = GetSharedLibraryFunction(handleLibrary, sMethodName);
		if (pFunction == NULL and bMandatory)
		{
			bIsError = true;
			AddError("Unable to load function " + sMethodName + " from library");
		}
	}
	return pFunction;
}
