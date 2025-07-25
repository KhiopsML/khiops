// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWMTDatabaseStream.h"

KWMTDatabaseStream::KWMTDatabaseStream()
{
	bSecondaryRecordError = false;

	// Parametrage du driver de table en remplacant celui de la classe ancetre
	assert(dataTableDriverCreator != NULL);
	delete dataTableDriverCreator;
	dataTableDriverCreator = new KWDataTableDriverStream;
}

KWMTDatabaseStream::~KWMTDatabaseStream()
{
	odHeaderLines.DeleteAll();
}

KWDatabase* KWMTDatabaseStream::Create() const
{
	return new KWMTDatabaseStream;
}

ALString KWMTDatabaseStream::GetTechnologyName() const
{
	return "Multiple table stream";
}

void KWMTDatabaseStream::SetHeaderLineAt(const ALString& sDataPath, const ALString& sHeaderLine)
{
	StringObject* soHeaderLine;

	require(LookupMultiTableMapping(sDataPath) != NULL);
	require(not IsReferencedClassMapping(LookupMultiTableMapping(sDataPath)));

	soHeaderLine = cast(StringObject*, odHeaderLines.Lookup(sDataPath));
	if (soHeaderLine == NULL)
	{
		soHeaderLine = new StringObject;
		odHeaderLines.SetAt(sDataPath, soHeaderLine);
	}
	soHeaderLine->SetString(sHeaderLine);
}

const ALString KWMTDatabaseStream::GetHeaderLineAt(const ALString& sDataPath) const
{
	StringObject* soHeaderLine;

	require(LookupMultiTableMapping(sDataPath) != NULL);
	require(not IsReferencedClassMapping(LookupMultiTableMapping(sDataPath)));

	soHeaderLine = cast(StringObject*, odHeaderLines.Lookup(sDataPath));
	if (soHeaderLine == NULL)
		return "";
	else
		return soHeaderLine->GetString();
}

void KWMTDatabaseStream::SetFieldSeparator(char cValue)
{
	cast(KWDataTableDriverStream*, dataTableDriverCreator)->SetFieldSeparator(cValue);
}

char KWMTDatabaseStream::GetFieldSeparator() const
{
	return cast(KWDataTableDriverStream*, dataTableDriverCreator)->GetFieldSeparator();
}

boolean KWMTDatabaseStream::SetSecondaryRecordAt(KWMTDatabaseMapping* mapping, const char* sRecord)
{
	KWDataTableDriverStream* dataTableDriver;

	require(mapping != NULL);
	require(LookupMultiTableMapping(mapping->GetDataPath()) == mapping);
	require(not IsReferencedClassMapping(mapping));
	require(sRecord != NULL);

	// Alimentation du buffer en entree pour la table secondaire
	dataTableDriver = cast(KWDataTableDriverStream*, mapping->GetDataTableDriver());

	// Attention, le driver peut etre a NULL si la table secondaire n'est utilisee pour aucun calcul
	if (dataTableDriver != NULL)
		return dataTableDriver->FillBufferWithRecord(sRecord);
	else
		return true;
}

void KWMTDatabaseStream::SetSecondaryRecordError(boolean bValue)
{
	bSecondaryRecordError = bValue;
}

boolean KWMTDatabaseStream::GetSecondaryRecordError() const
{
	return bSecondaryRecordError;
}

void KWMTDatabaseStream::SetMappingMaxBufferSize(KWMTDatabaseMapping* mapping, int nSize)
{
	KWDataTableDriverStream* dataTableDriver;

	require(mapping != NULL);
	require(LookupMultiTableMapping(mapping->GetDataPath()) == mapping);
	require(not IsReferencedClassMapping(mapping));

	dataTableDriver = cast(KWDataTableDriverStream*, mapping->GetDataTableDriver());

	// Attention, le driver peut etre a NULL si la table secondaire n'est utilisee pour aucun calcul
	if (dataTableDriver != NULL)
		dataTableDriver->SetBufferSize(nSize);
}

int KWMTDatabaseStream::GetMappingMaxBufferSize(KWMTDatabaseMapping* mapping) const
{
	KWDataTableDriverStream* dataTableDriver;

	require(mapping != NULL);
	require(LookupMultiTableMapping(mapping->GetDataPath()) == mapping);
	require(not IsReferencedClassMapping(mapping));

	dataTableDriver = cast(KWDataTableDriverStream*, mapping->GetDataTableDriver());
	// Attention, le driver peut etre a NULL si la table secondaire n'est utilisee pour aucun calcul
	if (dataTableDriver != NULL)
		return dataTableDriver->GetBufferSize();
	else
		return 0;
}

KWObject* KWMTDatabaseStream::ReadFromBuffer(const char* sBuffer)
{
	KWObject* kwoObject;
	KWMTDatabaseMapping* mapping;
	int i;

	require(sBuffer != NULL);
	require(GetMainMapping()->GetDataTableDriver() != NULL);

	// On repositionne le flag d'erreur a false, pour permettre des lectures sucessives
	bIsError = false;
	bSecondaryRecordError = false;

	// Alimentation du buffer en entree
	cast(KWDataTableDriverStream*, GetMainMapping()->GetDataTableDriver())->FillBufferWithRecord(sBuffer);

	// Lecture de l'objet
	kwoObject = Read();

	// Reinitialisation de tous les mappings des tables secondaires
	for (i = 0; i < GetMultiTableMappings()->GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(i));
		if (not IsReferencedClassMapping(mapping) and mapping->GetDataTableDriver() != NULL)
		{
			// Reinitialisation du buffer
			cast(KWDataTableDriverStream*, mapping->GetDataTableDriver())->ResetInputBuffer();

			// Reinitialisation de la bufferisation des objets
			mapping->CleanLastReadKey();
			if (mapping->GetLastReadObject() != NULL)
			{
				delete mapping->GetLastReadObject();
				mapping->SetLastReadObject(NULL);
			}
		}
	}
	return kwoObject;
}

boolean KWMTDatabaseStream::WriteToBuffer(KWObject* kwoObject, char* sBuffer, int nMaxBufferLength)
{
	boolean bOk;

	require(GetMainMapping()->GetDataTableDriver() != NULL);

	// On repositionne le flag d'erreur a false, pour permettre des ecritures sucessives
	bIsError = false;

	// Vidage du buffer en sortie
	cast(KWDataTableDriverStream*, GetMainMapping()->GetDataTableDriver())->ResetOutputBuffer();

	// Ecriture de l'objet dans le buffer du stream
	Write(kwoObject);

	// On recupere le resultat
	bOk = not IsError();
	if (bOk)
		bOk = cast(KWDataTableDriverStream*, GetMainMapping()->GetDataTableDriver())
			  ->FillRecordWithBuffer(sBuffer, nMaxBufferLength);
	return bOk;
}

void KWMTDatabaseStream::SetMaxBufferSize(int nValue)
{
	require(nValue > 0);
	cast(KWDataTableDriverStream*, dataTableDriverCreator)->SetBufferSize(nValue);
}

int KWMTDatabaseStream::GetMaxBufferSize() const
{
	return cast(KWDataTableDriverStream*, dataTableDriverCreator)->GetBufferSize();
}

longint KWMTDatabaseStream::GetUsedMemory() const
{
	longint lUsedMemory;
	KWMTDatabaseMapping* mapping;
	int i;
	StringObject* soHeaderLine;

	// Classe ancetre
	lUsedMemory = KWMTDatabase::GetUsedMemory();

	// Specialisation
	lUsedMemory += sizeof(KWMTDatabaseStream) - sizeof(KWMTDatabaseStream);
	lUsedMemory += odHeaderLines.GetUsedMemory();

	// Taille occupee par les lignes d'entete
	for (i = 0; i < mappingManager.GetMappingNumber(); i++)
	{
		mapping = mappingManager.GetMappingAt(i);
		if (not IsReferencedClassMapping(mapping))
		{
			soHeaderLine = cast(StringObject*, odHeaderLines.Lookup(mapping->GetDataPath()));
			if (soHeaderLine != NULL)
			{
				lUsedMemory += mapping->GetDataPath().GetLength();
				lUsedMemory += soHeaderLine->GetUsedMemory();
			}
		}
	}
	return lUsedMemory;
}

KWDataTableDriver* KWMTDatabaseStream::CreateDataTableDriver(KWMTDatabaseMapping* mapping) const
{
	KWDataTableDriverTextFile dataTableDriverTextFileCreator;
	KWDataTableDriverStream* dataTableDriver;

	require(mapping != NULL);

	// Cas d'une table reference, geree par des fichiers
	if (IsReferencedClassMapping(mapping))
	{
		dataTableDriverTextFileCreator.SetFieldSeparator(GetFieldSeparator());
		return dataTableDriverTextFileCreator.Clone();
	}
	// Cas d'une table interne, geree en memoire
	else
	{
		dataTableDriver = cast(KWDataTableDriverStream*, dataTableDriverCreator->Clone());
		dataTableDriver->SetHeaderLine(GetHeaderLineAt(mapping->GetDataPath()));
		return dataTableDriver;
	}
}
