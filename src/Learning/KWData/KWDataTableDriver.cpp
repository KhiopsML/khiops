// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataTableDriver.h"

KWDataTableDriver::KWDataTableDriver()
{
	kwcClass = NULL;
	bVerboseMode = true;
	bSilentMode = false;
	lRecordIndex = 0;
	lUsedRecordNumber = 0;
}

KWDataTableDriver::~KWDataTableDriver()
{
	assert(kwcClass == NULL);
}

KWDataTableDriver* KWDataTableDriver::Clone() const
{
	KWDataTableDriver* kwdtdClone;
	kwdtdClone = Create();
	kwdtdClone->CopyFrom(this);
	return kwdtdClone;
}

KWDataTableDriver* KWDataTableDriver::Create() const
{
	return new KWDataTableDriver;
}

void KWDataTableDriver::CopyFrom(const KWDataTableDriver* kwdtdSource)
{
	require(kwdtdSource != NULL);
	require(kwcClass == NULL);

	// Reinitialisation prealable de toutes les variables
	kwcClass = NULL;
	bVerboseMode = true;
	bSilentMode = false;
	lRecordIndex = 0;
	lUsedRecordNumber = 0;
	sDataTableName = "";

	// Recopie des parametres de specification de la base
	SetDataTableName(kwdtdSource->GetDataTableName());
	SetVerboseMode(kwdtdSource->GetVerboseMode());
	SetSilentMode(kwdtdSource->GetSilentMode());
}

int KWDataTableDriver::Compare(const KWDataTableDriver* kwdtdSource) const
{
	int nCompare;

	require(kwdtdSource != NULL);

	// Comparaison de la specification
	nCompare = GetDataTableName().Compare(kwdtdSource->GetDataTableName());
	return nCompare;
}

void KWDataTableDriver::AddSimpleMessage(const ALString& sLabel) const
{
	if (not bSilentMode and bVerboseMode)
		Object::AddSimpleMessage(sLabel);
}

void KWDataTableDriver::AddMessage(const ALString& sLabel) const
{
	if (not bSilentMode and bVerboseMode)
		Object::AddMessage(sLabel);
}

void KWDataTableDriver::AddWarning(const ALString& sLabel) const
{
	if (not bSilentMode and bVerboseMode)
		Object::AddWarning(sLabel);
}

void KWDataTableDriver::AddError(const ALString& sLabel) const
{
	if (not bSilentMode)
		Object::AddError(sLabel);
}

longint KWDataTableDriver::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = sizeof(KWDataTableDriver);
	lUsedMemory += sDataTableName.GetLength();
	return lUsedMemory;
}

const ALString KWDataTableDriver::GetClassLabel() const
{
	return "Data table";
}

const ALString KWDataTableDriver::GetObjectLabel() const
{
	ALString sLabel;

	sLabel = FileService::GetURIUserLabel(sDataTableName);
	if (lRecordIndex > 0 and not IsClosed() and not IsError())
	{
		sLabel += " : Record ";
		sLabel += LongintToString(lRecordIndex);
	}
	return sLabel;
}

boolean KWDataTableDriver::CheckFormat() const
{
	return true;
}

boolean KWDataTableDriver::Check() const
{
	boolean bOk = true;

	// Recherche du nom de la table
	if (GetDataTableName() == "")
	{
		bOk = false;
		AddWarning("Data table name not specified");
	}

	// Verification du format
	bOk = bOk and CheckFormat();
	return bOk;
}

//////////////////////////////////////////////////////////////////////////////////

boolean KWDataTableDriver::BuildDataTableClass(KWClass* kwcDataTableClass)
{
	require(kwcDataTableClass != NULL);
	return false;
}

boolean KWDataTableDriver::IsTypeInitializationManaged() const
{
	return false;
}

boolean KWDataTableDriver::OpenForRead(const KWClass* kwcLogicalClass)
{
	return true;
}

boolean KWDataTableDriver::OpenForWrite()
{
	return true;
}

boolean KWDataTableDriver::IsOpenedForRead() const
{
	return false;
}

boolean KWDataTableDriver::IsOpenedForWrite() const
{
	return false;
}

boolean KWDataTableDriver::IsEnd() const
{
	return true;
}

KWObject* KWDataTableDriver::Read()
{
	return NULL;
}

void KWDataTableDriver::Skip() {}

const KWObjectKey* KWDataTableDriver::GetLastReadRootKey() const
{
	return &lastReadRootKey;
}

void KWDataTableDriver::Write(const KWObject* kwoObject)
{
	require(kwoObject != NULL);
}

boolean KWDataTableDriver::Close()
{
	return true;
}

boolean KWDataTableDriver::IsClosed() const
{
	return true;
}

void KWDataTableDriver::DeleteDataTable() {}

longint KWDataTableDriver::GetEstimatedObjectNumber()
{
	return 0;
}

longint KWDataTableDriver::ComputeOpenNecessaryMemory(boolean bRead)
{
	return 0;
}

longint KWDataTableDriver::ComputeNecessaryMemoryForFullExternalRead(const KWClass* kwcLogicalClass)
{
	require(kwcLogicalClass != NULL);
	return 0;
}

longint KWDataTableDriver::ComputeNecessaryDiskSpaceForFullWrite(const KWClass* kwcLogicalClass, longint lInputFileSize)
{
	require(kwcLogicalClass != NULL);
	require(lInputFileSize >= 0);
	return 0;
}

double KWDataTableDriver::GetReadPercentage()
{
	return 0;
}
