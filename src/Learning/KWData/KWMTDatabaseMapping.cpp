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

void KWMTDatabaseMapping::Write(ostream& ost) const
{
	// Methode ancetre
	KWDataPath::Write(ost);

	// Specialisation
	ost << "Data table file\t" << GetDataTableName() << "\n";
}

void KWMTDatabaseMapping::WriteHeaderLineReport(ostream& ost) const
{
	KWDataPath::WriteHeaderLineReport(ost);
	ost << "\tData table file";
}

void KWMTDatabaseMapping::WriteLineReport(ostream& ost) const
{
	KWDataPath::WriteLineReport(ost);
	ost << "\t" << GetDataTableName();
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
	if (mappedDataTableDriver != NULL)
		lUsedMemory += mappedDataTableDriver->GetUsedMemory();
	return lUsedMemory;
}

////////////////////////////////////////////////////////////
// Classe KWMTDatabaseMappingManager

KWMTDatabaseMappingManager::KWMTDatabaseMappingManager()
{
	// Specifialisation des objet data path crees par la classe
	delete dataPathCreator;
	dataPathCreator = new KWMTDatabaseMapping;
}

KWMTDatabaseMappingManager::~KWMTDatabaseMappingManager() {}

KWDataPathManager* KWMTDatabaseMappingManager::Create() const
{
	return new KWMTDatabaseMappingManager;
}

boolean KWMTDatabaseMappingManager::CheckPartially(boolean bWriteOnly) const
{
	boolean bOk;
	ObjectDictionary odDataTableNames;
	KWMTDatabaseMapping* mapping;
	int nMapping;
	ALString sAttributeName;
	KWMTDatabaseMapping parentMapping;
	const KWMTDatabaseMapping* checkMapping;

	// Methode ancetre
	bOk = Check();

	// Specialisation en verifiant la validite des specification des tables
	if (bOk)
	{
		// Verification de la table de mapping
		for (nMapping = 0; nMapping < GetMappingNumber(); nMapping++)
		{
			mapping = GetMappingAt(nMapping);
			assert(mapping->Check());

			// Verification de l'unicite des noms de tables utilises
			if (mapping->GetDataTableName() != "")
			{
				if (odDataTableNames.Lookup(mapping->GetDataTableName()) != NULL)
				{
					bOk = false;
					AddError("Data path " + mapping->GetObjectLabel() + " : Data table " +
						 mapping->GetDataTableName() + " already used");
				}
				odDataTableNames.SetAt(mapping->GetDataTableName(), mapping);
			}

			// En mode ecriture, les tables externes ne doivent pas etre renseignees
			if (bWriteOnly and mapping->GetDataTableName() != "" and mapping->GetExternalTable())
			{
				bOk = false;
				AddError("Data path " + mapping->GetObjectLabel() + " : External data table " +
					 mapping->GetDataTableName() + " should not be specified for output database");
			}

			// En mode ecriture, si une table secondaire non externe est renseignee, sa table parente doit l'etre egalement
			// En theorie, on pourrait developper du code pour autoriser ce type de specification, mais le rapport cout/benefice
			// est tres peu favorable pour cas cas d'usage marginal
			if (bWriteOnly and mapping->GetDataTableName() != "" and not mapping->GetExternalTable() and
			    mapping->GetAttributeNames()->GetSize() > 0)
			{
				// Recherche du mapping parent
				parentMapping.CopyFrom(mapping);
				parentMapping.GetAttributeNames()->SetSize(mapping->GetAttributeNames()->GetSize() - 1);
				checkMapping = LookupMapping(parentMapping.GetDataPath());

				// Verification dans le cas valide que ce mapping a egalement un nom de table specifie
				if (checkMapping != NULL and checkMapping->GetDataTableName() == "")
				{
					bOk = false;
					AddError("Data path " + mapping->GetObjectLabel() + " : data table " +
						 mapping->GetDataTableName() +
						 " cannot be specified without a data table being specified for its "
						 "owner table (data path " +
						 parentMapping.GetDataPath() + ")");
				}
			}
		}
	}
	return bOk;
}

const ALString KWMTDatabaseMappingManager::GetClassLabel() const
{
	return "Database";
}

const ALString KWMTDatabaseMappingManager::GetObjectLabel() const
{
	if (mainDataPath == NULL)
		return "";
	else
		return cast(KWMTDatabaseMapping*, mainDataPath)->GetDataTableName();
}
