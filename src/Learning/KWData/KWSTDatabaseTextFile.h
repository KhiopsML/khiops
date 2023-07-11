// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWSTDatabase.h"
#include "KWDataTableDriverTextFile.h"

//////////////////////////////////////////////////////////////////////
// Base simple-table geree dans un fichier texte
class KWSTDatabaseTextFile : public KWSTDatabase
{
public:
	// Constructeur
	KWSTDatabaseTextFile();
	~KWSTDatabaseTextFile();

	// Creation dynamique
	KWDatabase* Create() const override;

	// Nom de la technologie de base de donnees
	// Ici: table simple en mode fichier texte
	ALString GetTechnologyName() const override;

	///////////////////////////////////////////////////////////
	// Parametrage de la structure du fichier

	// Utilisation d'une ligne d'entete: par defaut true
	void SetHeaderLineUsed(boolean bValue);
	boolean GetHeaderLineUsed() const;

	// Separateur de champs utilise (par defaut: '\t')
	void SetFieldSeparator(char cValue);
	char GetFieldSeparator() const;

	// Test de transfert de fichier
	//   Si WriteFileName est absent: pas d'ecriture
	//   Si ClassFileName est absent: la classe est lue d'apres le fichier
	static void TestReadWrite(const ALString& sClassFileName, const ALString& sClassName,
				  const ALString& sReadFileName, const ALString& sWriteFileName);

	// Test general
	static void Test();
};
