// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWSTDatabase.h"

KWSTDatabase::KWSTDatabase()
{
	dataTableDriverCreator = new KWDataTableDriver;
}

KWSTDatabase::~KWSTDatabase()
{
	delete dataTableDriverCreator;
}

KWDatabase* KWSTDatabase::Create() const
{
	return new KWSTDatabase;
}

boolean KWSTDatabase::CheckFormat() const
{
	boolean bOk;
	KWDataTableDriver* dataTableDriverCheck;

	// Test du format du driver de table, en passant par une variable temporaire dont on peut parametrer le nom
	dataTableDriverCheck = dataTableDriverCreator->Clone();
	dataTableDriverCheck->SetDataTableName(GetDatabaseName());
	bOk = dataTableDriverCheck->CheckFormat();
	delete dataTableDriverCheck;

	// Test pour la base ancetre
	bOk = bOk and KWDatabase::CheckFormat();
	return bOk;
}

void KWSTDatabase::CopyFrom(const KWDatabase* kwdSource)
{
	const KWSTDatabase* kwstdSource = cast(KWSTDatabase*, kwdSource);

	require(kwstdSource->dataTableDriverCreator->IsClosed());

	// Copie standard
	KWDatabase::CopyFrom(kwdSource);

	// Copie des parametres du driver
	dataTableDriverCreator->CopyFrom(kwstdSource->dataTableDriverCreator);
}

int KWSTDatabase::Compare(const KWDatabase* kwdSource) const
{
	int nCompare;
	const KWSTDatabase* kwstdSource = cast(KWSTDatabase*, kwdSource);

	// Comparaison de base
	nCompare = KWDatabase::Compare(kwdSource);

	// Comparaison specifique
	if (nCompare == 0)
		nCompare = dataTableDriverCreator->Compare(kwstdSource->dataTableDriverCreator);
	return nCompare;
}

void KWSTDatabase::SetVerboseMode(boolean bValue)
{
	// Appel de la methode ancetre
	KWDatabase::SetVerboseMode(bValue);

	// Parametrage du driver
	dataTableDriverCreator->SetVerboseMode(bValue);
}

void KWSTDatabase::SetSilentMode(boolean bValue)
{
	// Appel de la methode ancetre
	KWDatabase::SetSilentMode(bValue);

	// Parametrage du driver
	dataTableDriverCreator->SetSilentMode(bValue);
}

KWDataTableDriver* KWSTDatabase::GetDataTableDriver()
{
	require(not IsOpenedForRead() and not IsOpenedForWrite());
	return dataTableDriverCreator;
}

longint KWSTDatabase::GetUsedMemory() const
{
	longint lUsedMemory;

	// Methode ancetre
	lUsedMemory = KWDatabase::GetUsedMemory();

	// Specialisation
	lUsedMemory += sizeof(KWDataTableDriver*);
	if (dataTableDriverCreator != NULL)
		lUsedMemory += dataTableDriverCreator->GetUsedMemory();
	return lUsedMemory;
}

longint KWSTDatabase::ComputeOpenNecessaryMemory(boolean bRead, boolean bIncludingClassMemory)
{
	longint lNecessaryMemory;

	require(not IsOpenedForRead() and not IsOpenedForWrite());
	require(kwcClass != NULL);

	// Appel de la methode ancetre
	lNecessaryMemory = KWDatabase::ComputeOpenNecessaryMemory(bRead, bIncludingClassMemory);

	// On complete par la taille demandee par le driver, en parametrant temporairement le nom de la base
	assert(dataTableDriverCreator->GetDataTableName() == "");
	dataTableDriverCreator->SetDataTableName(GetDatabaseName());
	lNecessaryMemory += dataTableDriverCreator->ComputeOpenNecessaryMemory(bRead);
	dataTableDriverCreator->SetDataTableName("");
	return lNecessaryMemory;
}

KWDataTableDriver* KWSTDatabase::CreateDataTableDriver() const
{
	return dataTableDriverCreator->Clone();
}

boolean KWSTDatabase::BuildDatabaseClass(KWClass* kwcDatabaseClass)
{
	boolean bOk;

	dataTableDriverCreator->SetDataTableName(GetDatabaseName());
	bOk = dataTableDriverCreator->BuildDataTableClass(kwcDatabaseClass);
	dataTableDriverCreator->SetDataTableName("");
	return bOk;
}

boolean KWSTDatabase::IsTypeInitializationManaged() const
{
	return dataTableDriverCreator->IsTypeInitializationManaged();
}

boolean KWSTDatabase::PhysicalOpenForRead()
{
	boolean bOk;

	// Parametrage
	dataTableDriverCreator->SetDataTableName(GetDatabaseName());
	dataTableDriverCreator->SetClass(kwcPhysicalClass);

	// Ouverture physique
	bOk = dataTableDriverCreator->OpenForRead(kwcClass);
	return bOk;
}

boolean KWSTDatabase::PhysicalOpenForWrite()
{
	boolean bOk;

	// Parametrage
	dataTableDriverCreator->SetDataTableName(GetDatabaseName());
	dataTableDriverCreator->SetClass(kwcClass);

	// Ouverture physique
	bOk = dataTableDriverCreator->OpenForWrite();
	return bOk;
}

boolean KWSTDatabase::IsPhysicalEnd() const
{
	return dataTableDriverCreator->IsEnd();
}

KWObject* KWSTDatabase::PhysicalRead()
{
	KWObject* kwoObject;

	// Lecture avec le driver
	kwoObject = dataTableDriverCreator->Read();

	// Positionnement du flag d'erreur
	bIsError = bIsError or dataTableDriverCreator->IsError();

	// Incrementation du compteur d'objet utilise au niveau physique
	if (not bIsError)
		dataTableDriverCreator->SetUsedRecordNumber(dataTableDriverCreator->GetUsedRecordNumber() + 1);
	return kwoObject;
}

void KWSTDatabase::PhysicalSkip()
{
	dataTableDriverCreator->Skip();

	// Positionnement du flag d'erreur
	bIsError = bIsError or dataTableDriverCreator->IsError();
}

void KWSTDatabase::PhysicalWrite(const KWObject* kwoObject)
{
	dataTableDriverCreator->Write(kwoObject);

	// Positionnement du flag d'erreur
	bIsError = bIsError or dataTableDriverCreator->IsError();

	// Incrementation du compteur d'objet utilise au niveau physique
	if (not bIsError)
		dataTableDriverCreator->SetUsedRecordNumber(dataTableDriverCreator->GetUsedRecordNumber() + 1);
}

boolean KWSTDatabase::PhysicalClose()
{
	boolean bOk = true;

	if (dataTableDriverCreator->IsOpenedForRead() or dataTableDriverCreator->IsOpenedForWrite())
		bOk = dataTableDriverCreator->Close();
	dataTableDriverCreator->SetDataTableName("");
	dataTableDriverCreator->SetClass(NULL);
	return bOk;
}

void KWSTDatabase::PhysicalDeleteDatabase()
{
	KWDataTableDriver* mappedDataTableDriver;

	// Creation d'un driver associe au mapping, pour pouvoir detruire la table correspondante
	mappedDataTableDriver = CreateDataTableDriver();
	mappedDataTableDriver->SetDataTableName(GetDatabaseName());

	// Destruction de la base si possible
	mappedDataTableDriver->DeleteDataTable();

	// Nettoyage
	delete mappedDataTableDriver;
}

longint KWSTDatabase::GetPhysicalEstimatedObjectNumber()
{
	longint lNumber;

	require(dataTableDriverCreator->GetDataTableName() == "");
	require(dataTableDriverCreator->GetClass() == NULL);

	// Parametrage du driver pour estimer le nombre d'objets
	dataTableDriverCreator->SetDataTableName(GetDatabaseName());
	dataTableDriverCreator->SetClass(KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName()));
	lNumber = dataTableDriverCreator->GetEstimatedObjectNumber();
	dataTableDriverCreator->SetDataTableName("");
	dataTableDriverCreator->SetClass(NULL);

	ensure(dataTableDriverCreator->GetDataTableName() == "");
	ensure(dataTableDriverCreator->GetClass() == NULL);
	return lNumber;
}

double KWSTDatabase::GetPhysicalReadPercentage()
{
	return dataTableDriverCreator->GetReadPercentage();
}

longint KWSTDatabase::GetPhysicalRecordIndex() const
{
	return dataTableDriverCreator->GetRecordIndex();
}
