// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "InputBufferedFile.h"
#include "HugeBuffer.h"

static const unsigned int nHugeBufferSize = 1048576;

///////////////////////////////////////////////////////////////////////////////
// Buffer unique de grande taille pour
//      - la gestion des champs dans la methode InputBufferedFile::GetNextField
//      - la copie de fichiers dans la methode SystemFileDriver::CopyFileFromLocal
// Pour eviter les problemes avec les DLL:
//  . on utilise un methode statique, dans un unnamed namespace,
//    pour forcer un acces local uniquement depuis ce fichier,
//  . le buffer est lui-meme une variable statique de cette methode statique

// Declaration du buffer
static char* sHugeBuffer = NULL;

// Destruction du buffer
static void HugeBufferDelete()
{
	if (sHugeBuffer != NULL)
	{
		SystemObject::DeleteCharArray(sHugeBuffer);
		sHugeBuffer = NULL;
	}
}

// Creation du buffer
static void HugeBufferNew()
{
	int i;
	int nSize;

	if (sHugeBuffer == NULL)
	{
		// Creation et initialisation du buffer
		// On n'utilise pas la methode max(,) car sinon sur linux on a une reference indefinie
		// vers InputBufferedFile::nMaxFieldSize
		nSize = nHugeBufferSize;
		if (nSize < InputBufferedFile::nMaxFieldSize + 1)
			nSize = InputBufferedFile::nMaxFieldSize + 1;
		if (nSize < SystemFileDriver::nBufferSizeForCopying)
			nSize = SystemFileDriver::nBufferSizeForCopying;
		sHugeBuffer = SystemObject::NewCharArray(nSize);
		for (i = 0; i <= InputBufferedFile::nMaxFieldSize; i++)
			sHugeBuffer[i] = '\0';

		// Enregistrement de sa destruction en fin de programme
		atexit(HugeBufferDelete);
	}
}

char* GetHugeBuffer()
{
	if (sHugeBuffer == NULL)
		HugeBufferNew();
	return sHugeBuffer;
}
