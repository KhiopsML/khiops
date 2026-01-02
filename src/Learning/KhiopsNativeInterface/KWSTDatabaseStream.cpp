// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWSTDatabaseStream.h"

KWSTDatabaseStream::KWSTDatabaseStream()
{
	// Parametrage du driver de table en remplacant celui de la classe ancetre
	assert(dataTableDriverCreator != NULL);
	delete dataTableDriverCreator;
	dataTableDriverCreator = new KWDataTableDriverStream;
}

KWSTDatabaseStream::~KWSTDatabaseStream() {}

KWDatabase* KWSTDatabaseStream::Create() const
{
	return new KWSTDatabaseStream;
}

ALString KWSTDatabaseStream::GetTechnologyName() const
{
	return "Single table stream";
}

void KWSTDatabaseStream::SetHeaderLine(const ALString& sValue)
{
	cast(KWDataTableDriverStream*, dataTableDriverCreator)->SetHeaderLine(sValue);
}

const ALString& KWSTDatabaseStream::GetHeaderLine() const
{
	return cast(KWDataTableDriverStream*, dataTableDriverCreator)->GetHeaderLine();
}

void KWSTDatabaseStream::SetFieldSeparator(char cValue)
{
	cast(KWDataTableDriverStream*, dataTableDriverCreator)->SetFieldSeparator(cValue);
}

char KWSTDatabaseStream::GetFieldSeparator() const
{
	return cast(KWDataTableDriverStream*, dataTableDriverCreator)->GetFieldSeparator();
}

KWObject* KWSTDatabaseStream::ReadFromBuffer(const char* sBuffer)
{
	KWObject* kwoObject;

	require(sBuffer != NULL);

	// Alimentation du buffer en entree
	cast(KWDataTableDriverStream*, dataTableDriverCreator)->FillBufferWithRecord(sBuffer);

	// Lecture de l'objet
	kwoObject = Read();

	// Reinitialisation du buffer en entree
	cast(KWDataTableDriverStream*, dataTableDriverCreator)->ResetInputBuffer();
	return kwoObject;
}

boolean KWSTDatabaseStream::WriteToBuffer(KWObject* kwoObject, char* sBuffer, int nMaxBufferLength)
{
	boolean bOk;

	// Vidage du buffer en sortie
	cast(KWDataTableDriverStream*, dataTableDriverCreator)->ResetOutputBuffer();

	// Ecriture de l'objet dans le buffer du stream
	Write(kwoObject);

	// On recupere le resultat
	bOk = cast(KWDataTableDriverStream*, dataTableDriverCreator)->FillRecordWithBuffer(sBuffer, nMaxBufferLength);
	return bOk;
}

void KWSTDatabaseStream::SetMaxBufferSize(int nValue)
{
	require(nValue > 0);
	cast(KWDataTableDriverStream*, dataTableDriverCreator)->SetBufferSize(nValue);
}

int KWSTDatabaseStream::GetMaxBufferSize() const
{
	return cast(KWDataTableDriverStream*, dataTableDriverCreator)->GetBufferSize();
}
