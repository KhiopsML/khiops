// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "BufferedFile.h"
#include "FileService.h"

BufferedFileDriverCreator BufferedFile::fileDriverCreator;

BufferedFile::BufferedFile()
{
	nBufferSize = nDefaultBufferSize;
	sFileName = "";
	bHeaderLineUsed = true;
	cFieldSeparator = '\t';
	fileHandle = NULL;
	bIsOpened = false;
	bIsError = false;
	fileDriver = NULL;
	nCurrentBufferSize = 0;
}

BufferedFile::~BufferedFile(void)
{
	ensure(fileHandle == NULL);
	ensure(fileDriver == NULL);
}

void BufferedFile::CopyFrom(const BufferedFile* bufferedFile)
{
	require(not IsOpened());

	sFileName = bufferedFile->GetFileName();
	nBufferSize = bufferedFile->GetBufferSize();
	bHeaderLineUsed = bufferedFile->GetHeaderLineUsed();
	cFieldSeparator = bufferedFile->GetFieldSeparator();
	fileHandle = NULL;
	fileDriver = NULL;
	bIsOpened = false;
}

void BufferedFile::SetFileName(const ALString& sValue)
{
	require(not IsOpened());

	sFileName = sValue;
}

const ALString& BufferedFile::GetFileName() const
{
	return sFileName;
}

void BufferedFile::SetHeaderLineUsed(boolean bValue)
{
	bHeaderLineUsed = bValue;
}

boolean BufferedFile::GetHeaderLineUsed() const
{
	return bHeaderLineUsed;
}

longint BufferedFile::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(BufferedFile);
	lUsedMemory += sFileName.GetLength();
	lUsedMemory += fbBuffer.GetUsedMemory();
	if (fileHandle != NULL)
		lUsedMemory += sizeof(SystemFile);
	if (fileDriver != NULL)
		lUsedMemory += sizeof(BufferedFileDriver);
	return lUsedMemory;
}

const ALString BufferedFile::GetClassLabel() const
{
	return "File";
}

const ALString BufferedFile::GetObjectLabel() const
{
	return sFileName;
}

boolean BufferedFile::AllocateBuffer()
{
	int nAllocatedBufferSize;

	// On cherche la taille a allouer
	nAllocatedBufferSize = GetAllocatedBufferSize();

	// Retaillage si necessaire du buffer
	assert(nAllocatedBufferSize >= 0);
	if (fbBuffer.GetSize() < nAllocatedBufferSize)
	{
		// Allocation d'un buffer de grande taille, avec protection contre les probleme memoire
		fbBuffer.SetLargeSize(nAllocatedBufferSize);

		// Message d'erreur si necessaire
		if (fbBuffer.GetSize() != nAllocatedBufferSize)
		{
			ALString sTmp;
			AddError(sTmp + "Unable to allocate file buffer with " + IntToString(nAllocatedBufferSize) +
				 " bytes");
			bIsError = true;
		}
	}
	return not bIsError;
}

void BufferedFile::PruneBuffer()
{
	int nAllocatedBufferSize;

	nAllocatedBufferSize = GetAllocatedBufferSize();
	assert(nAllocatedBufferSize >= 0);

	if (fbBuffer.GetSize() == nAllocatedBufferSize)
		return;
	if (fbBuffer.GetSize() < nAllocatedBufferSize)
		AllocateBuffer();
	else
		fbBuffer.SetSize(nAllocatedBufferSize);
}

void BufferedFile::CleanBuffer()
{
	require(not IsOpened());
	fbBuffer.SetSize(0);
	nCurrentBufferSize = 0;
}

void BufferedFile::ResetBuffer()
{
	fbBuffer.SetSize(0);
	nCurrentBufferSize = 0;
}

const CharVector* BufferedFile::GetBuffer() const
{
	return &fbBuffer.cvBuffer;
}
