// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KNIStream.h"

int KNIStream::nAllStreamsMemoryLimit = 0;

KNIStream::KNIStream()
{
	streamClass = NULL;
	lStreamOpeningUsedMemory = 0;
	lStreamRecodingUsedMemory = 0;
	nStreamMemoryLimit = KNI_DefaultMaxStreamMemory;

	// Le format de sortie dans KNI est le format tabulaire
	outputStream.SetDenseOutputFormat(true);
}

KNIStream::~KNIStream() {}

longint KNIStream::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KNIStream);
	if (streamClass != NULL)
	{
		lUsedMemory += inputStream.GetUsedMemory() - sizeof(KWMTDatabaseStream);
		lUsedMemory += outputStream.GetUsedMemory() - sizeof(KWMTDatabaseStream);
	}
	return lUsedMemory;
}

void KNIStream::SetStreamOpeningUsedMemory(longint lValue)
{
	require(lValue >= 0);
	lStreamOpeningUsedMemory = lValue;
}

longint KNIStream::GetStreamOpeningUsedMemory() const
{
	return lStreamOpeningUsedMemory;
}

void KNIStream::SetStreamRecodingUsedMemory(longint lValue)
{
	lStreamRecodingUsedMemory = lValue;
}

longint KNIStream::GetStreamRecodingUsedMemory() const
{
	return lStreamRecodingUsedMemory;
}

longint KNIStream::GetStreamRecodingAvailableMemory() const
{
	longint lAvailableMemory;

	lAvailableMemory = GetStreamActualMemoryLimit() - GetStreamOpeningUsedMemory();
	ensure(lAvailableMemory >= GetStreamMinimumRecodingMemoryLimit());
	return lAvailableMemory;
}

longint KNIStream::GetStreamRecodingAvailableBufferMemory() const
{
	return GetStreamRecodingAvailableMemory() / 2;
}

longint KNIStream::GetStreamRecodingAvailableComputationMemory() const
{
	return GetStreamRecodingAvailableMemory() - GetStreamRecodingAvailableMemory() / 2;
}

longint KNIStream::GetStreamMinimumRecodingMemoryLimit() const
{
	return 2 * InputBufferedFile::GetMaxLineLength();
}

longint KNIStream::GetStreamActualMemoryLimit() const
{
	return nStreamMemoryLimit * lMB;
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
