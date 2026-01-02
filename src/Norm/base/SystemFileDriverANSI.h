// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "SystemFileDriver.h"
#include "Standard.h"
#include "FileService.h"

///////////////////////////////////////////////////////////////////////////
// Classe SystemFileDriverANSI
// Classe d'acces aux fichiers standards
class SystemFileDriverANSI : public SystemFileDriver
{
public:
	// Constructeur
	SystemFileDriverANSI();
	~SystemFileDriverANSI();

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
	boolean ReserveExtraSize(longint lSize, void* stream) override;
};
