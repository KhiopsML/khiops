// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KNIStream.h"

int KNIStream::nAllStreamsMemoryLimit = 0;

KNIStream::KNIStream()
{
	streamClass = NULL;
	lStreamUsedMemory = 0;
	nStreamMemoryLimit = KNI_DefaultMaxStreamMemory;
}

KNIStream::~KNIStream() {}

longint KNIStream::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KNIStream);
	if (streamClass != NULL)
	{
		lUsedMemory += streamClass->GetDomain()->GetUsedMemory();
		lUsedMemory += inputStream.GetUsedMemory() - sizeof(KWMTDatabaseStream);
		lUsedMemory += outputStream.GetUsedMemory() - sizeof(KWMTDatabaseStream);
	}
	return lUsedMemory;
}

void KNIStream::SetStreamUsedMemory(longint lValue)
{
	require(lValue > 0);
	lStreamUsedMemory = lValue;
}

longint KNIStream::GetStreamUsedMemory() const
{
	return lStreamUsedMemory;
}

longint KNIStream::GetStreamAvailableMemory() const
{
	return (longint)ceil(nStreamMemoryLimit * lMB / (1 + MemGetAllocatorOverhead()));
}

void KNIStream::SetStreamMemoryLimit(int nValue)
{
	require(nValue > 0);
	require(not GetInputStream()->IsOpenedForRead());
	require(not GetOutputStream()->IsOpenedForWrite());
	nStreamMemoryLimit = nValue;
}

int KNIStream::GetStreamMemoryLimit() const
{
	return nStreamMemoryLimit;
}

void KNIStream::SetAllStreamsMemoryLimit(int nValue)
{
	require(nValue >= 0);
	nAllStreamsMemoryLimit = nValue;
}

int KNIStream::GetAllStreamsMemoryLimit()
{
	return nAllStreamsMemoryLimit;
}
