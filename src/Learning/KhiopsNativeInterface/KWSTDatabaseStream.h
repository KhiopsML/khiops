// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWSTDatabase.h"
#include "KWDataTableDriverStream.h"

//////////////////////////////////////////////////////////////////////
// Base simple-table dans gere dans un stream
class KWSTDatabaseStream : public KWSTDatabase
{
public:
	// Constructeur
	KWSTDatabaseStream();
	~KWSTDatabaseStream();

	// Creation dynamique
	KWDatabase* Create() const;

	// Nom de la technologie de base de donnees
	// Ici: table simple en mode fichier stream
	ALString GetTechnologyName() const;

	///////////////////////////////////////////////////////////
	// Parametrage de la structure du stream

	// Ligne d'entete
	void SetHeaderLine(const ALString& sValue);
	const ALString& GetHeaderLine() const;

	// Separateur de champs utilise (par defaut: '\t')
	void SetFieldSeparator(char cValue);
	char GetFieldSeparator() const;

	///////////////////////////////////////////////////////
	// Lecture/ecriture a partir d'un buffer

	// Lecture d'une instance depuis un buffer
	// Renvoie NULL si echec
	KWObject* ReadFromBuffer(const char* sBuffer);

	// Ecriture d'un instance dans un buffer
	// renvoie false si pas assez de place dans le buffer
	boolean WriteToBuffer(KWObject* kwoObject, char* sBuffer, int nMaxBufferLength);

	// Parametrage de la taille max des buffers
	void SetMaxBufferSize(int nValue);
	int GetMaxBufferSize() const;
};