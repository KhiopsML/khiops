// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWMTDatabaseTextFile.h"

KWMTDatabaseTextFile::KWMTDatabaseTextFile()
{
	// Parametrage du driver de table en remplacant celui de la classe ancetre
	assert(dataTableDriverCreator != NULL);
	delete dataTableDriverCreator;
	dataTableDriverCreator = new KWDataTableDriverTextFile;
}

KWMTDatabaseTextFile::~KWMTDatabaseTextFile() {}

KWDatabase* KWMTDatabaseTextFile::Create() const
{
	return new KWMTDatabaseTextFile;
}

void KWMTDatabaseTextFile::AddPrefixToUsedFiles(const ALString& sPrefix)
{
	int nMapping;
	KWMTDatabaseMapping* mapping;
	ALString sDataTableName;
	ALString sFilePrefix;
	ALString sNewDatabaseName;

	// Renommage si nom existant
	if (GetDatabaseName() != "")
	{
		// Personnalisation des noms de table du mapping (qui comprennent la table racine)
		for (nMapping = 0; nMapping < GetMultiTableMappings()->GetSize(); nMapping++)
		{
			mapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(nMapping));

			// Nom par defaut de la base de donnee cible
			sDataTableName = mapping->GetDataTableName();
			if (sDataTableName != "")
			{
				// Extraction du prefixe du fichier
				sFilePrefix = FileService::GetFilePrefix(sDataTableName);

				// On construit le nom complet du fichier
				sDataTableName =
				    FileService::SetFilePrefix(mapping->GetDataTableName(), sPrefix + sFilePrefix);

				// Parametrage avec le nouveau nom
				mapping->SetDataTableName(sDataTableName);
			}
		}
	}
}

void KWMTDatabaseTextFile::AddSuffixToUsedFiles(const ALString& sSuffix)
{
	int nMapping;
	KWMTDatabaseMapping* mapping;
	ALString sDataTableName;
	ALString sFilePrefix;
	ALString sNewDatabaseName;

	// Renommage si nom existant
	if (GetDatabaseName() != "")
	{
		// Personnalisation des noms de table du mapping (qui comprennent la table racine)
		for (nMapping = 0; nMapping < GetMultiTableMappings()->GetSize(); nMapping++)
		{
			mapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(nMapping));

			// Nom par defaut de la base de donnee cible
			sDataTableName = mapping->GetDataTableName();
			if (sDataTableName != "")
			{
				// Extraction du prefixe du fichier
				sFilePrefix = FileService::GetFilePrefix(sDataTableName);

				// On construit le nom complet du fichier
				sDataTableName =
				    FileService::SetFilePrefix(mapping->GetDataTableName(), sFilePrefix + sSuffix);

				// Parametrage avec le nouveau nom
				mapping->SetDataTableName(sDataTableName);
			}
		}
	}
}

void KWMTDatabaseTextFile::AddPathToUsedFiles(const ALString& sPathName)
{
	int nMapping;
	KWMTDatabaseMapping* mapping;
	ALString sDataTableName;

	// Renommage si nom existant
	if (GetDatabaseName() != "" and sPathName != "")
	{
		// Personnalisation des noms de table du mapping (qui comprennent la table racine)
		for (nMapping = 0; nMapping < GetMultiTableMappings()->GetSize(); nMapping++)
		{
			mapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(nMapping));

			// Nom par defaut de la base de donnee cible
			sDataTableName = mapping->GetDataTableName();
			if (sDataTableName != "" and not FileService::IsPathInFilePath(sDataTableName))
			{
				// On construit le nom complet du fichier
				sDataTableName = FileService::BuildFilePathName(sPathName, sDataTableName);

				// Parametrage avec le nouveau nom
				mapping->SetDataTableName(sDataTableName);
			}
		}
	}
}

void KWMTDatabaseTextFile::ExportUsedFileSpecs(ObjectArray* oaUsedFileSpecs) const
{
	FileSpec* fsDataTable;
	int nMapping;
	KWMTDatabaseMapping* mapping;

	// Appel de la methode ancetre
	KWMTDatabase::ExportUsedFileSpecs(oaUsedFileSpecs);

	// Personnalisation des noms de table du mapping (hors table racine, deja traitee)
	for (nMapping = 1; nMapping < oaMultiTableMappings.GetSize(); nMapping++)
	{
		mapping = cast(KWMTDatabaseMapping*, oaMultiTableMappings.GetAt(nMapping));

		// Ajout d'une spec de fichier utilise par table de mapping
		fsDataTable = new FileSpec;
		oaUsedFileSpecs->Add(fsDataTable);

		// Parametrage du nom du mapping
		fsDataTable->SetLabel("data table for mapping " + mapping->GetDataPath());
		fsDataTable->SetFilePathName(mapping->GetDataTableName());
	}
}

void KWMTDatabaseTextFile::ExportUsedWriteFileSpecs(ObjectArray* oaUsedFileSpecs) const
{
	FileSpec* fsDataTable;
	int nMapping;
	KWMTDatabaseMapping* mapping;

	// Appel de la methode ancetre
	KWMTDatabase::ExportUsedFileSpecs(oaUsedFileSpecs);

	// Personnalisation des noms de table du mapping (hors table racine, deja traitee)
	for (nMapping = 1; nMapping < oaMultiTableMappings.GetSize(); nMapping++)
	{
		mapping = cast(KWMTDatabaseMapping*, oaMultiTableMappings.GetAt(nMapping));

		// Export uniquement hors classes referencees
		if (not IsReferencedClassMapping(mapping))
		{
			// Ajout d'une spec de fichier utilise par table de mapping
			fsDataTable = new FileSpec;
			oaUsedFileSpecs->Add(fsDataTable);

			// Parametrage du nom du mapping
			fsDataTable->SetLabel("data table for mapping " + mapping->GetDataPath());
			fsDataTable->SetFilePathName(mapping->GetDataTableName());
		}
	}
}

ALString KWMTDatabaseTextFile::GetTechnologyName() const
{
	return "Multiple table text file";
}

boolean KWMTDatabaseTextFile::IsMultiTableTechnology() const
{
	return true;
}

void KWMTDatabaseTextFile::SetHeaderLineUsed(boolean bValue)
{
	cast(KWDataTableDriverTextFile*, dataTableDriverCreator)->SetHeaderLineUsed(bValue);
}

boolean KWMTDatabaseTextFile::GetHeaderLineUsed() const
{
	return cast(KWDataTableDriverTextFile*, dataTableDriverCreator)->GetHeaderLineUsed();
}

void KWMTDatabaseTextFile::SetFieldSeparator(char cValue)
{
	cast(KWDataTableDriverTextFile*, dataTableDriverCreator)->SetFieldSeparator(cValue);
}

char KWMTDatabaseTextFile::GetFieldSeparator() const
{
	return cast(KWDataTableDriverTextFile*, dataTableDriverCreator)->GetFieldSeparator();
}
