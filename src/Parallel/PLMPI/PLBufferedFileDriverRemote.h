// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLMPITaskDriver.h"
#include "BufferedFileDriver.h"
#include "PLErrorWithIndex.h"

/////////////////////////////////////////////////////////////////////////////
// Classe PLBufferedFileDriverRemote
// Drivers des acces distants dans InputBufferedFile et PLRemoteFileService
class PLBufferedFileDriverRemote : public BufferedFileDriver
{
public:
	// Constructeur
	PLBufferedFileDriverRemote();
	~PLBufferedFileDriverRemote();
	PLBufferedFileDriverRemote* Create() const override;

	// Implementation des methodes virtuelles pures
	void Fill(InputBufferedFile* buffer, longint lBeginPos, boolean& bSkippedLine, longint& lRealBeginPos) override;
	void Read(InputBufferedFile* buffer, longint lBeginPos) override;
	boolean Flush(OutputBufferedFile*) override;
	boolean OpenForRead(InputBufferedFile*) override;
	boolean OpenForWrite(OutputBufferedFile*) override;
	boolean OpenForWriteAppend(OutputBufferedFile*) override;
	boolean Close() override;
	longint FindEOL(InputBufferedFile*, longint lBeginPos, boolean& bEolFound) override;
	boolean FillWithHeaderLine(InputBufferedFile*) override;
	boolean Exist(const ALString& sFileURI) override;
	longint GetFileSize(const ALString&) override;
	boolean RemoveFile(const ALString& sFileURI) override;
	boolean CreateEmptyFile(const ALString& sFilePathName) override;
	boolean CopyFile(const ALString& sSourceURI, const ALString& sDestURI) override;
	boolean MakeDirectory(const ALString& sPathName) override;
	boolean MakeDirectories(const ALString& sPathName) override;
	longint GetDiskFreeSpace(const ALString& sPathName) override;

protected:
	Timer tRemoteAccess;

	PLMPITracer* GetTracerMPI() const;
};

inline PLMPITracer* PLBufferedFileDriverRemote::GetTracerMPI() const
{
	return cast(PLMPITracer*, PLMPITaskDriver::GetDriver()->GetTracerMPI());
}

inline boolean PLBufferedFileDriverRemote::Flush(OutputBufferedFile*)
{
	assert(false);
	return false;
}