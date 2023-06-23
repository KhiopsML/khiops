// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "InputBufferedFile.h"
#include "OutputBufferedFile.h"

class MemoryInputBufferedFile;
class MemoryOutputBufferedFile;

///////////////////////////////////////////////////////////////////////////
// Specialisation de InputBufferedFile pour lire les donnees en memoire
class MemoryInputBufferedFile : public InputBufferedFile
{
public:
	// Constructeur
	MemoryInputBufferedFile();
	~MemoryInputBufferedFile();

	// Remplissage du buffer
	// Si depassement du buffer, renvoie false sans remplir le buffer
	boolean FillBuffer(const CharVector* sInputRecord);

	// Remplissage du buffer avec un enregistrement
	// On ajoute si necessaire une fin de ligne en fin d'enregistrement
	boolean FillBufferWithRecord(const char* sInputRecord);

	// Reinitialisation du buffer
	void ResetBuffer();

	// Reimplementation de methodes virtuelles
	longint GetFileSize() const override;
	boolean Open() override;
	boolean IsOpened() const override;
	boolean Close() override;
};

///////////////////////////////////////////////////////////////////////////
// Specialisation de OutputBufferedFile pour exporter le contenu
// du buffer interne vers une chaine de caractere en memoire
class MemoryOutputBufferedFile : public OutputBufferedFile
{
public:
	// Constructeur
	MemoryOutputBufferedFile();
	~MemoryOutputBufferedFile();

	// Remplissage d'un enregistrement avec le contenu integral du buffer
	// Renvoie false en cas de depassement de capacite (avec un '\0' en fin), true sinon
	boolean FillRecordWithBuffer(char* sOutputRecord, int nOutputMaxLength);

	// Reinitialisation du buffer
	void ResetBuffer();

	// Reimplementation de methodes virtuelles
	boolean Open() override;
	boolean IsOpened() const override;
	boolean Close() override;
};
