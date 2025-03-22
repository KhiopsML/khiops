// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "SystemFileDriver.h"

#include "FileService.h"
#include "MemoryStatsManager.h"
#include "Utils.h"

///////////////////////////////////////////////////////////////////////////
// Classe SystemFileDriverLibrary
// Classe qui s'instancie a partir d'une bibliotheque dynamique
// chaque instance est un driver de fichier (hdfs, s3...)
class SystemFileDriverLibrary : public SystemFileDriver
{
public:
	// Constructeur
	SystemFileDriverLibrary();
	~SystemFileDriverLibrary();

	// Reimplementation des methodes virtuelles
	const char* GetDriverName() const override;
	const char* GetScheme() const override;
	const char* GetVersion() const override;
	boolean IsReadOnly() const override;
	boolean Connect() override;
	boolean Disconnect() override;
	boolean IsConnected() const override;
	longint GetSystemPreferredBufferSize() const override;
	boolean FileExists(const char* sFilePathName) const override;
	boolean DirExists(const char* sFilePathName) const override;
	longint GetFileSize(const char* sFilePathName) const override;
	void* Open(const char* sFilePathName, char cMode) override;
	boolean Close(void* stream) override;
	longint Fread(void* ptr, size_t size, size_t count, void* stream) override;
	boolean SeekPositionInFile(longint lPosition, void* stream) override;
	const char* GetLastErrorMessage() const override;
	longint Fwrite(const void* ptr, size_t size, size_t count, void* stream) override;
	boolean Flush(void* stream) override;
	boolean RemoveFile(const char* sFilePathName) const override;
	boolean MakeDirectory(const char* sPathName) const override;
	boolean RemoveDirectory(const char* sFilePathName) const override;
	longint GetDiskFreeSpace(const char* sPathName) const override;
	boolean CopyFileFromLocal(const char* sSourceFilePathName, const char* sDestFilePathName) override;
	boolean CopyFileToLocal(const char* sSourceFilePathName, const char* sDestFilePathName) override;

	// De/chargement de la bibliotheque dynamique
	boolean LoadLibrary(const ALString& sLibraryFilePathName);
	void UnloadLibrary();
	boolean IsLibraryLoaded() const;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Initialisation a NULL des pointeurs de fonctions
	void Clean();

	// Binding des fonctions vers la bibliotheque
	// Si la methode n'est pas obligatoire, il n'y a pas d'erreur si elle est absente de la librairie
	// En cas d'erreur renvoie NULL et bIsError est mis a true
	void* BindToLibrary(const ALString& sMethodName, boolean bMandatory);

	// Variable de gestion du chargement de la librairie
	boolean bIsError;
	ALString sLibraryName;
	void* handleLibrary;

	// Pointeurs vers les fonctions de la dll
	const char* (*ptr_driver_getDriverName)();
	const char* (*ptr_driver_getVersion)();
	const char* (*ptr_driver_getScheme)();
	int (*ptr_driver_isReadOnly)();
	int (*ptr_driver_connect)();
	int (*ptr_driver_disconnect)();
	int (*ptr_driver_isConnected)();
	long long int (*ptr_driver_getSystemPreferredBufferSize)();
	int (*ptr_driver_fileExists)(const char* filename);
	int (*ptr_driver_dirExists)(const char* filename);
	long long int (*ptr_driver_getFileSize)(const char* filename);
	void* (*ptr_driver_fopen)(const char* filename, char mode);
	int (*ptr_driver_fclose)(void* stream);
	long long int (*ptr_driver_fread)(void* ptr, size_t size, size_t count, void* stream);
	int (*ptr_driver_fseek)(void* stream, long long int offset, int whence);
	const char* (*ptr_driver_getlasterror)();
	long long int (*ptr_driver_fwrite)(const void* ptr, size_t size, size_t count, void* stream);
	int (*ptr_driver_fflush)(void* stream);
	int (*ptr_driver_remove)(const char* filename);
	int (*ptr_driver_mkdir)(const char* filename);
	int (*ptr_driver_rmdir)(const char* filename);
	long long int (*ptr_driver_diskFreeSpace)(const char* filename);
	int (*ptr_driver_copyToLocal)(const char* sourcefilename, const char* destfilename);
	int (*ptr_driver_copyFromLocal)(const char* sourcefilename, const char* destfilename);
};
