// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "HugeBuffer.h"

///////////////////////////////////////////////////////////////////////////////
// Buffer unique de grande taille pour
//      - la gestion des champs dans la methode InputBufferedFile::GetNextField
//      - la copie de fichiers dans la methode SystemFileDriver::CopyFileFromLocal
// Pour eviter les problemes avec les DLL:
//  . on utilise pour la partie privee des declaration en static, dans un unnamed namespace,
//    pour forcer un acces local uniquement depuis ce fichier,
//  . le buffer est lui-meme une variable statique de cette methode statique

namespace
{
static const unsigned int nBufferMinSize = 1048576;
static boolean bHugeBufferDeletionRegistered = false;
static unsigned int nHugeBufferSize = 0;

// Declaration du buffer
static char* sHugeBuffer = NULL;

// Creation du buffer
static char* NewHugeBuffer(unsigned int nHugeSize)
{
	assert(sHugeBuffer == NULL);
	require(nHugeSize > 0);

	// Creation et initialisation du buffer
	return SystemObject::NewCharArray(nHugeBufferSize);
}
} // namespace

/////////////////////////////////////////////////////////////////////////
// Methode publiques

void DeleteHugeBuffer()
{
	if (sHugeBuffer != NULL)
	{
		SystemObject::DeleteCharArray(sHugeBuffer);
		sHugeBuffer = NULL;
		nHugeBufferSize = 0;
	}
}

int GetHugeBufferSize()
{
	ensure(nHugeBufferSize == 0 or sHugeBuffer != NULL);
	return nHugeBufferSize;
}

char* GetHugeBuffer(unsigned int nHugeSize)
{
	require(nHugeSize <= 128 * lMB);

	if (sHugeBuffer == NULL or nHugeSize > nHugeBufferSize)
	{
		// Enregistrement de sa destruction en fin de programme
		// seulement une seule fois, lors de la premiere allocation
		if (not bHugeBufferDeletionRegistered)
		{
			atexit(DeleteHugeBuffer);
			bHugeBufferDeletionRegistered = true;
		}

		// Destruction du buffer si il existe
		if (sHugeBuffer != NULL)
			DeleteHugeBuffer();

		// On borne par une taille minimum (aucune raison d'avoir un petit buffer)
		nHugeBufferSize = max(nHugeSize, nBufferMinSize);

		// Allocation du buffer
		sHugeBuffer = NewHugeBuffer(nHugeBufferSize);

		// Initialisation
		memset(sHugeBuffer, '\0', nHugeBufferSize * sizeof(char));
	}

	return sHugeBuffer;
}

char* GetHugeBufferAdress()
{
	return sHugeBuffer;
}
