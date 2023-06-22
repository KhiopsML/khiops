// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLDatabaseTextFile.h"

///////////////////////////////////////////////////////////////////
// Classe PLDatabaseTextFile

PLDatabaseTextFile::PLDatabaseTextFile()
{
	plstDatabaseTextFile = NULL;
	plmtDatabaseTextFile = NULL;
}

PLDatabaseTextFile::~PLDatabaseTextFile()
{
	if (plstDatabaseTextFile != NULL)
		delete plstDatabaseTextFile;
	if (plmtDatabaseTextFile != NULL)
		delete plmtDatabaseTextFile;
}

void PLDatabaseTextFile::Initialize(boolean bIsMultiTableTechnology)
{
	KWMTDatabaseTextFile refMTDatabaseTextFile;
	KWSTDatabaseTextFile refSTDatabaseTextFile;

	// Nettoyage prealable
	Reset();

	// Creation et initialisation selon le type de base
	if (bIsMultiTableTechnology)
		plmtDatabaseTextFile = new PLMTDatabaseTextFile;
	else
		plstDatabaseTextFile = new PLSTDatabaseTextFile;
	ensure(IsInitialized());
}

void PLDatabaseTextFile::InitializeFrom(const KWDatabase* database)
{
	KWClass* kwcDatabaseClass;

	require(database != NULL);

	// Initialisation des bases de travail pour un transfer mono-table si c'est possible
	// (une seule table et pas de cle; en presence de cle, il faut en effet detecter les doublons potentiels)
	// En effet, dans ce cas, il n'y a pas besoin de pre-indexer la table d'entree pour la parallelisation
	kwcDatabaseClass = KWClassDomain::GetCurrentDomain()->LookupClass(database->GetClassName());
	if (database->GetTableNumber() == 1 and kwcDatabaseClass->GetKeyAttributeNumber() == 0)
		STInitializeFrom(database);
	// Sinon, initialisation selon le type de base
	else
		DefaultInitializeFrom(database);
	ensure(IsInitialized());
}

boolean PLDatabaseTextFile::IsInitialized() const
{
	assert(plstDatabaseTextFile == NULL or plmtDatabaseTextFile == NULL);
	return (plstDatabaseTextFile != NULL or plmtDatabaseTextFile != NULL);
}

void PLDatabaseTextFile::Reset()
{
	if (plstDatabaseTextFile != NULL)
		delete plstDatabaseTextFile;
	if (plmtDatabaseTextFile != NULL)
		delete plmtDatabaseTextFile;
	plstDatabaseTextFile = NULL;
	plmtDatabaseTextFile = NULL;
}

boolean PLDatabaseTextFile::IsMultiTableTechnology() const
{
	require(IsInitialized());
	return (plmtDatabaseTextFile != NULL);
}

PLSTDatabaseTextFile* PLDatabaseTextFile::GetSTDatabase()
{
	require(not IsMultiTableTechnology());
	return plstDatabaseTextFile;
}

PLMTDatabaseTextFile* PLDatabaseTextFile::GetMTDatabase()
{
	require(IsMultiTableTechnology());
	return plmtDatabaseTextFile;
}

KWDatabase* PLDatabaseTextFile::GetDatabase()
{
	require(IsInitialized());
	if (IsMultiTableTechnology())
		return plmtDatabaseTextFile;
	else
		return plstDatabaseTextFile;
}

void PLDatabaseTextFile::DisplayAllTableMessages(const ALString& sDatabaseLabel, const ALString& sRecordLabel,
						 longint lMainRecordNumber, longint lMainObjectNumber,
						 LongintVector* lvSecondaryRecordNumbers)
{
	KWDatabase* database;
	PLMTDatabaseTextFile* mtDatabase;
	KWMTDatabaseMapping* mapping;
	int i;
	ALString sMainLabel;
	ALString sTmp;

	require(IsInitialized());
	require(lMainRecordNumber >= 0);
	require(-1 <= lMainObjectNumber and lMainObjectNumber <= lMainRecordNumber);
	require(lvSecondaryRecordNumbers == NULL or lvSecondaryRecordNumbers->GetSize() == 1 or
		IsMultiTableTechnology());

	database = GetDatabase();
	if (database->GetVerboseMode() and not database->GetSilentMode())
	{
		// Message principal
		sMainLabel = sDatabaseLabel + " " + database->GetDatabaseName();
		sMainLabel += ": " + sRecordLabel + ": " + LongintToReadableString(lMainRecordNumber);
		if (lMainObjectNumber >= 0 and lMainObjectNumber < lMainRecordNumber)
			sMainLabel += sTmp + "\tSelected records: " + LongintToReadableString(lMainObjectNumber);
		AddSimpleMessage(sMainLabel);

		// Parcours des tables pour collecter les messages physique
		if (IsMultiTableTechnology() and lvSecondaryRecordNumbers != NULL)
		{
			mtDatabase = GetMTDatabase();
			Global::ActivateErrorFlowControl();
			for (i = 1; i < mtDatabase->GetTableNumber(); i++)
			{
				mapping = cast(KWMTDatabaseMapping*, mtDatabase->GetMultiTableMappings()->GetAt(i));

				// Test si mapping correspond a une table source interne
				if (not mtDatabase->IsReferencedClassMapping(mapping) and
				    mapping->GetDataTableName() != "")
				{
					// Test si nombre positif (-1 signifie que la base n'a pas ete ouverte)
					if (lvSecondaryRecordNumbers->GetAt(i) >= 0)
						AddSimpleMessage(
						    sTmp + "  Table " + mapping->GetDataTableName() + " " +
						    "Records: " +
						    LongintToReadableString(lvSecondaryRecordNumbers->GetAt(i)));
				}
			}
			Global::DesactivateErrorFlowControl();
		}
	}
}

const ALString PLDatabaseTextFile::GetClassLabel() const
{
	if (plmtDatabaseTextFile != NULL)
		return plmtDatabaseTextFile->GetClassLabel();
	else if (plstDatabaseTextFile != NULL)
		return plstDatabaseTextFile->GetClassLabel();
	else
		return "";
}

const ALString PLDatabaseTextFile::GetObjectLabel() const
{
	if (plmtDatabaseTextFile != NULL)
		return plmtDatabaseTextFile->GetObjectLabel();
	else if (plstDatabaseTextFile != NULL)
		return plstDatabaseTextFile->GetObjectLabel();
	else
		return "";
}

boolean PLDatabaseTextFile::ComputeOpenInformation(boolean bRead, boolean bIncludingClassMemory,
						   PLDatabaseTextFile* outputDatabaseTextFile)
{
	require(IsInitialized());
	if (IsMultiTableTechnology())
	{
		if (outputDatabaseTextFile == NULL)
			return plmtDatabaseTextFile->ComputeOpenInformation(bRead, bIncludingClassMemory, NULL);
		else
			return plmtDatabaseTextFile->ComputeOpenInformation(bRead, bIncludingClassMemory,
									    outputDatabaseTextFile->GetMTDatabase());
	}
	else
	{
		if (outputDatabaseTextFile == NULL)
			return plstDatabaseTextFile->ComputeOpenInformation(bRead, bIncludingClassMemory, NULL);
		else
			return plstDatabaseTextFile->ComputeOpenInformation(bRead, bIncludingClassMemory,
									    outputDatabaseTextFile->GetSTDatabase());
	}
}

boolean PLDatabaseTextFile::IsOpenInformationComputed() const
{
	require(IsInitialized());
	if (IsMultiTableTechnology())
		return plmtDatabaseTextFile->IsOpenInformationComputed();
	else
		return plstDatabaseTextFile->IsOpenInformationComputed();
}

void PLDatabaseTextFile::CleanOpenInformation()
{
	require(IsInitialized());
	if (IsMultiTableTechnology())
		return plmtDatabaseTextFile->CleanOpenInformation();
	else
		return plstDatabaseTextFile->CleanOpenInformation();
}

longint PLDatabaseTextFile::GetTotalFileSize() const
{
	require(IsInitialized());
	if (IsMultiTableTechnology())
		return plmtDatabaseTextFile->GetTotalFileSize();
	else
		return plstDatabaseTextFile->GetTotalFileSize();
}

longint PLDatabaseTextFile::GetTotalUsedFileSize() const
{
	require(IsInitialized());
	if (IsMultiTableTechnology())
		return plmtDatabaseTextFile->GetTotalUsedFileSize();
	else
		return plstDatabaseTextFile->GetTotalUsedFileSize();
}

longint PLDatabaseTextFile::GetEmptyOpenNecessaryMemory() const
{
	require(IsInitialized());
	if (IsMultiTableTechnology())
		return plmtDatabaseTextFile->GetEmptyOpenNecessaryMemory();
	else
		return plstDatabaseTextFile->GetEmptyOpenNecessaryMemory();
}

longint PLDatabaseTextFile::GetMinOpenNecessaryMemory() const
{
	require(IsInitialized());
	if (IsMultiTableTechnology())
		return plmtDatabaseTextFile->GetMinOpenNecessaryMemory();
	else
		return plstDatabaseTextFile->GetMinOpenNecessaryMemory();
}

longint PLDatabaseTextFile::GetMaxOpenNecessaryMemory() const
{
	require(IsInitialized());
	if (IsMultiTableTechnology())
		return plmtDatabaseTextFile->GetMaxOpenNecessaryMemory();
	else
		return plstDatabaseTextFile->GetMaxOpenNecessaryMemory();
}

longint PLDatabaseTextFile::GetOutputNecessaryDiskSpace() const
{
	require(IsInitialized());
	if (IsMultiTableTechnology())
		return plmtDatabaseTextFile->GetOutputNecessaryDiskSpace();
	else
		return plstDatabaseTextFile->GetOutputNecessaryDiskSpace();
}

int PLDatabaseTextFile::GetMaxSlaveProcessNumber() const
{
	require(IsInitialized());
	if (IsMultiTableTechnology())
		return plmtDatabaseTextFile->GetMaxSlaveProcessNumber();
	else
		return plstDatabaseTextFile->GetMaxSlaveProcessNumber();
}

int PLDatabaseTextFile::ComputeOpenBufferSize(boolean bRead, longint lOpenGrantedMemory) const
{
	require(IsInitialized());
	if (IsMultiTableTechnology())
		return plmtDatabaseTextFile->ComputeOpenBufferSize(bRead, lOpenGrantedMemory);
	else
		return plstDatabaseTextFile->ComputeOpenBufferSize(bRead, lOpenGrantedMemory);
}

void PLDatabaseTextFile::SetBufferSize(int nBufferSize)
{
	require(IsInitialized());
	if (IsMultiTableTechnology())
		return plmtDatabaseTextFile->SetBufferSize(nBufferSize);
	else
		return plstDatabaseTextFile->SetBufferSize(nBufferSize);
}

int PLDatabaseTextFile::GetBufferSize() const
{
	require(IsInitialized());
	if (IsMultiTableTechnology())
		return plmtDatabaseTextFile->GetBufferSize();
	else
		return plstDatabaseTextFile->GetBufferSize();
}

void PLDatabaseTextFile::PhysicalDeleteDatabase()
{
	require(IsInitialized());
	if (IsMultiTableTechnology())
		return plmtDatabaseTextFile->PhysicalDeleteDatabase();
	else
		return plstDatabaseTextFile->PhysicalDeleteDatabase();
}

void PLDatabaseTextFile::DefaultInitializeFrom(const KWDatabase* database)
{
	KWMTDatabaseTextFile refMTDatabaseTextFile;
	KWSTDatabaseTextFile refSTDatabaseTextFile;

	require(database != NULL);
	require(database->GetTechnologyName() == refMTDatabaseTextFile.GetTechnologyName() or
		database->GetTechnologyName() == refSTDatabaseTextFile.GetTechnologyName());

	// Initialisation selon le type de base
	Initialize(database->IsMultiTableTechnology());

	// Recopie selon le type de base
	if (database->IsMultiTableTechnology())
		plmtDatabaseTextFile->CopyFrom(database);
	else
		plstDatabaseTextFile->CopyFrom(database);
	ensure(IsInitialized());
}

void PLDatabaseTextFile::STInitializeFrom(const KWDatabase* database)
{
	KWMTDatabaseTextFile refMTDatabaseTextFile;
	KWSTDatabaseTextFile refSTDatabaseTextFile;

	require(database != NULL);
	require(database->GetTechnologyName() == refMTDatabaseTextFile.GetTechnologyName() or
		database->GetTechnologyName() == refSTDatabaseTextFile.GetTechnologyName());

	// Initialisation en mode mono-table
	Initialize(false);

	// Recopie des specifications generiques de la base en entree
	plstDatabaseTextFile->KWDatabase::CopyFrom(database);

	// Parametrage specifique de la base selon le type de base
	if (database->IsMultiTableTechnology())
	{
		plstDatabaseTextFile->SetHeaderLineUsed(cast(KWMTDatabaseTextFile*, database)->GetHeaderLineUsed());
		plstDatabaseTextFile->SetFieldSeparator(cast(KWMTDatabaseTextFile*, database)->GetFieldSeparator());
	}
	else
	{
		plstDatabaseTextFile->SetHeaderLineUsed(cast(KWSTDatabaseTextFile*, database)->GetHeaderLineUsed());
		plstDatabaseTextFile->SetFieldSeparator(cast(KWSTDatabaseTextFile*, database)->GetFieldSeparator());
	}
	ensure(IsInitialized());
}

///////////////////////////////////////////////////
// Classe PLShared_DatabaseTextFile

PLShared_DatabaseTextFile::PLShared_DatabaseTextFile() {}

PLShared_DatabaseTextFile::~PLShared_DatabaseTextFile() {}

void PLShared_DatabaseTextFile::SetPLDatabase(PLDatabaseTextFile* database)
{
	require(database != NULL);
	SetObject(database);
}

PLDatabaseTextFile* PLShared_DatabaseTextFile::GetPLDatabase()
{
	return cast(PLDatabaseTextFile*, GetObject());
}

KWDatabase* PLShared_DatabaseTextFile::GetDatabase()
{
	return GetPLDatabase()->GetDatabase();
}

PLSTDatabaseTextFile* PLShared_DatabaseTextFile::GetSTDatabase()
{
	return cast(PLDatabaseTextFile*, GetObject())->GetSTDatabase();
}

PLMTDatabaseTextFile* PLShared_DatabaseTextFile::GetMTDatabase()
{
	return cast(PLDatabaseTextFile*, GetObject())->GetMTDatabase();
}

void PLShared_DatabaseTextFile::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	PLDatabaseTextFile* database;
	boolean bIsMultiTableTechnology;
	PLShared_STDatabaseTextFile shared_stDatabase;
	PLShared_MTDatabaseTextFile shared_mtDatabase;

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());

	database = cast(PLDatabaseTextFile*, o);

	// Ecriture du type de base
	bIsMultiTableTechnology = database->IsMultiTableTechnology();
	serializer->PutBoolean(bIsMultiTableTechnology);

	// Serialisation selon le type de base
	if (bIsMultiTableTechnology)
		shared_mtDatabase.SerializeObject(serializer, database->GetMTDatabase());
	else
		shared_stDatabase.SerializeObject(serializer, database->GetSTDatabase());
}

void PLShared_DatabaseTextFile::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	PLDatabaseTextFile* database;
	boolean bIsMultiTableTechnology;
	PLShared_STDatabaseTextFile shared_stDatabase;
	PLShared_MTDatabaseTextFile shared_mtDatabase;

	require(serializer != NULL);
	require(serializer->IsOpenForRead());

	database = cast(PLDatabaseTextFile*, o);

	// Recherche du type de base
	bIsMultiTableTechnology = serializer->GetBoolean();

	// Deserialisation selon le type de base
	database->Initialize(bIsMultiTableTechnology);
	if (bIsMultiTableTechnology)
		shared_mtDatabase.DeserializeObject(serializer, database->GetMTDatabase());
	else
		shared_stDatabase.DeserializeObject(serializer, database->GetSTDatabase());
}

Object* PLShared_DatabaseTextFile::Create() const
{
	return new PLDatabaseTextFile;
}