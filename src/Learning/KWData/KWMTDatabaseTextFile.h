// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWMTDatabase.h"
#include "KWDataTableDriverTextFile.h"

//////////////////////////////////////////////////////////////////////
// Base multi-table gere dans un ensemble de fichiers textes
class KWMTDatabaseTextFile : public KWMTDatabase
{
public:
	// Constructeur
	KWMTDatabaseTextFile();
	~KWMTDatabaseTextFile();

	// Creation dynamique
	KWDatabase* Create() const override;

	// Ajout d'un prefix ou d'un suffixe aux fichiers utilises pour gerer la base
	void AddPrefixToUsedFiles(const ALString& sPrefix) override;

	// Ajout d'un d'un suffixe aux fichiers utilises pour gerer la base (ici, fin du nom du fichier, avant le
	// suffxie du fichier)
	void AddSuffixToUsedFiles(const ALString& sSuffix) override;

	// Ajout d'un path aux fichiers utilises pour gerer la base (qui n'en ont pas deja un)
	void AddPathToUsedFiles(const ALString& sPathName) override;

	// Export des specs de fichier utilises pour gerer la base (cf KWDatabase)
	void ExportUsedFileSpecs(ObjectArray* oaUsedFileSpecs) const override;
	void ExportUsedWriteFileSpecs(ObjectArray* oaUsedFileSpecs) const override;

	// Nom de la technologie de base de donnees
	// Ici: table multiple en mode fichier texte
	ALString GetTechnologyName() const override;

	// Indique que l'on est en technologie multi-tables
	boolean IsMultiTableTechnology() const override;

	///////////////////////////////////////////////////////////
	// Parametrage de la structure du fichier

	// Utilisation d'une ligne d'entete: par defaut true
	void SetHeaderLineUsed(boolean bValue);
	boolean GetHeaderLineUsed() const;

	// Separateur de champs utilise (par defaut: '\t')
	void SetFieldSeparator(char cValue);
	char GetFieldSeparator() const;
};