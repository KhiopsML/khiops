// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWMTDatabaseMapping.h"

KWMTDatabaseMapping::KWMTDatabaseMapping()
{
	mappedDataTableDriver = NULL;
	nMappedAttributeType = KWType::Unknown;
	kwoLastReadObject = NULL;
}

KWMTDatabaseMapping::~KWMTDatabaseMapping()
{
	assert(mappedDataTableDriver == NULL);
}

void KWMTDatabaseMapping::CopyFrom(const KWDataPath* aSource)
{
	require(aSource != NULL);

	// Methode ancetre
	KWDataPath::CopyFrom(aSource);

	// Specialisation
	// Pas de recopie des attributs de gestion, qui ne peuvent etre partages entre deux mapping
	// Seules les spec sont recopiees
	sDataTableName = cast(const KWMTDatabaseMapping*, aSource)->sDataTableName;
}

KWDataPath* KWMTDatabaseMapping::Create() const
{
	return new KWMTDatabaseMapping;
}

int KWMTDatabaseMapping::Compare(const KWDataPath* aSource) const
{
	int nCompare;

	// Methode ancetre
	nCompare = KWDataPath::Compare(aSource);

	// Specialisation
	if (nCompare == 0)
		nCompare = GetDataTableName().Compare(cast(const KWMTDatabaseMapping*, aSource)->GetDataTableName());
	return nCompare;
}

////////////////////////////////////////////////////////////

void KWMTDatabaseMapping::Write(ostream& ost) const
{
	// Methode ancetre
	KWDataPath::Write(ost);

	// Specialisation
	ost << "Data table file\t" << GetDataTableName() << "\n";
}

const ALString KWMTDatabaseMapping::GetClassLabel() const
{
	return "Multi-table mapping";
}

longint KWMTDatabaseMapping::GetUsedMemory() const
{
	longint lUsedMemory;

	// Methode ancetre
	lUsedMemory = KWDataPath::GetUsedMemory();
	lUsedMemory += sizeof(KWMTDatabaseMapping) - sizeof(KWDataPath);

	// Specialisation
	lUsedMemory += sDataTableName.GetLength();
	lUsedMemory += oaComponentTableMappings.GetUsedMemory();
	if (mappedDataTableDriver != NULL)
		lUsedMemory += mappedDataTableDriver->GetUsedMemory();
	return lUsedMemory;
}

void KWMTDatabaseMapping::CollectFullHierarchyComponentTableMappings(ObjectArray* oaResults)
{
	int i;
	KWMTDatabaseMapping* mapping;

	require(oaResults != NULL);

	// Ajout du mapping
	oaResults->Add(this);

	// Ajout des mapping de la composition
	for (i = 0; i < oaComponentTableMappings.GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, oaComponentTableMappings.GetAt(i));
		mapping->CollectFullHierarchyComponentTableMappings(oaResults);
	}
}
