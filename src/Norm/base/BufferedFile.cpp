// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "BufferedFile.h"
#include "FileService.h"
#include "PLRemoteFileService.h"

BufferedFile::BufferedFile()
{
	nBufferSize = nDefaultBufferSize;
	sFileName = "";
	bHeaderLineUsed = true;
	cFieldSeparator = '\t';
	fileHandle = NULL;
	bIsOpened = false;
	bIsError = false;
	nCurrentBufferSize = 0;
	nPreferredBufferSize = 0;
	bOpenOnDemandMode = false;
	bSilentMode = false;
}

BufferedFile::~BufferedFile(void)
{
	ensure(fileHandle == NULL);
}

void BufferedFile::CopyFrom(const BufferedFile* bufferedFile)
{
	require(not IsOpened());

	sFileName = bufferedFile->GetFileName();
	nBufferSize = bufferedFile->GetBufferSize();
	nCurrentBufferSize = 0;
	nPreferredBufferSize = bufferedFile->GetPreferredBufferSize();
	bHeaderLineUsed = bufferedFile->GetHeaderLineUsed();
	cFieldSeparator = bufferedFile->GetFieldSeparator();
	fileHandle = NULL;
	bIsOpened = false;
	bSilentMode = bufferedFile->bSilentMode;
}

void BufferedFile::SetFileName(const ALString& sValue)
{
	require(not IsOpened());

	sFileName = sValue;

	// Parametrage du taille preferee du buffer en fonction du nom du fichier
	if (sValue == "")
		nPreferredBufferSize = 0;
	else
		nPreferredBufferSize = PLRemoteFileService::GetPreferredBufferSize(sFileName);
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

void BufferedFile::AddSimpleMessage(const ALString& sLabel) const
{
	if (not bSilentMode)
		Object::AddSimpleMessage(sLabel);
}

void BufferedFile::AddMessage(const ALString& sLabel) const
{
	if (not bSilentMode)
		Object::AddMessage(sLabel);
}

void BufferedFile::AddWarning(const ALString& sLabel) const
{
	if (not bSilentMode)
		Object::AddWarning(sLabel);
}

void BufferedFile::AddError(const ALString& sLabel) const
{
	if (not bSilentMode)
		Object::AddError(sLabel);
}

longint BufferedFile::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(BufferedFile);
	lUsedMemory += sFileName.GetLength();
	lUsedMemory += fcCache.GetUsedMemory();
	if (fileHandle != NULL)
		lUsedMemory += sizeof(SystemFile);
	return lUsedMemory;
}

const ALString BufferedFile::GetClassLabel() const
{
	return "File";
}

const ALString BufferedFile::GetObjectLabel() const
{
	return FileService::GetURIUserLabel(sFileName);
}

boolean BufferedFile::AllocateBuffer()
{
	int nAllocatedBufferSize;

	// On cherche la taille a allouer
	nAllocatedBufferSize = GetAllocatedBufferSize();

	// Retaillage si necessaire du buffer
	assert(nAllocatedBufferSize >= 0);
	if (fcCache.GetSize() < nAllocatedBufferSize)
	{
		// Allocation d'un buffer de grande taille, avec protection contre les probleme memoire
		fcCache.SetLargeSize(nAllocatedBufferSize);

		// Message d'erreur si necessaire
		if (fcCache.GetSize() != nAllocatedBufferSize)
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

	if (fcCache.GetSize() == nAllocatedBufferSize)
		return;
	if (fcCache.GetSize() < nAllocatedBufferSize)
		AllocateBuffer();
	else
		fcCache.SetSize(nAllocatedBufferSize);
}

void BufferedFile::CleanBuffer()
{
	require(not IsOpened());
	fcCache.SetSize(0);
	nCurrentBufferSize = 0;
}

void BufferedFile::ResetBuffer()
{
	fcCache.SetSize(0);
	nCurrentBufferSize = 0;
}

void BufferedFile::MoveLastSegmentsToHead(CharVector* cv, int nSegmentIndex)
{
	int nBlockNumber;
	int i;
	char* pTemp;

	require(0 <= nSegmentIndex and nSegmentIndex < ((cv->nSize - 1) / cv->nBlockSize) + 1);

	// Cas particulier de deplacement du premier segment
	if (nSegmentIndex == 0)
		return;

	// Cas general: on a au moins plusieurs blocks
	assert(cv->nAllocSize > cv->nBlockSize);
	nBlockNumber = ((cv->nSize - 1) / cv->nBlockSize) + 1;
	assert(nBlockNumber > 1);
	assert(nSegmentIndex < nBlockNumber);
	for (i = nSegmentIndex; i < nBlockNumber; i++)
	{
		pTemp = cv->pData.hugeVector.pValueBlocks[i - nSegmentIndex];
		cv->pData.hugeVector.pValueBlocks[i - nSegmentIndex] = cv->pData.hugeVector.pValueBlocks[i];
		cv->pData.hugeVector.pValueBlocks[i] = pTemp;
	}
}
