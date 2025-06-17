// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDataTableDriverTextFile.h"
#include "KWObject.h"

/////////////////////////////////////////////////////
// Table de donnees au format stream
//
// Un stream de donnees texte possede une structure d'enregistrement
// analogue a un fichier de donnees texte:
//		- la ligne d'entete precise lors de la creation contient les libelles des champs
//		- un stream par instance, a alimenter avant toute lecture
//		- un caractere (',', ' ', TAB...) comme separateur de champs
//
// On herite de KWDataTableDriverTextFile pour recuperer l'essentiel de ses traitements,
// tout en desactivant ses fonctionnalites specifiques fichiers par reimplementation
// a vide de ses methodes virtuelles
class KWDataTableDriverStream : public KWDataTableDriverTextFile
{
public:
	// Constructeur
	KWDataTableDriverStream();
	~KWDataTableDriverStream();

	// Creation dynamique
	KWDataTableDriver* Create() const override;

	// Recopie des attributs de definition
	void CopyFrom(const KWDataTableDriver* kwdtdSource) override;

	///////////////////////////////////////////////////////////
	// Parametrage de la structure du stream
	// Separateur de champs utilise (cf. classe ancetre)

	// Ligne d'entete
	void SetHeaderLine(const ALString& sValue);
	const ALString& GetHeaderLine() const;

	///////////////////////////////////////////////////////
	// Enregistrement courant du stream

	// Vidage du buffer de entree, pour preparer la lecture d'un nouvel enregistrement en debut de buffer
	// Sequence d'utilisation typique: ResetInputBuffer, FillRecordWithRecord, Read
	void ResetInputBuffer();

	// Remplissage du buffer avec un enregistrement ajoute en fin de position courante du buffer
	// Retaillage potentiel du buffer su le BufferSize a ete agrandi
	// Si depassement du buffer, renvoie false sans remplir le buffer
	boolean FillBufferWithRecord(const char* sInputRecord);

	// Vidage du buffer de sortie, pour preparer l'ecriture d'un nouvel enregistrement en debut de buffer
	// Sequence d'utilisation typique: ResetOutputBuffer, Write, FillRecordWithBuffer
	void ResetOutputBuffer();

	// Remplissage d'un enregistrement avec le buffer (sans retour a la ligne)
	// Renvoie false en cas de depassement de capacite (avec un '\0' en fin), true sinon
	boolean FillRecordWithBuffer(char* sOutputRecord, int nOutputMaxLength);

	///////////////////////////////////////////////////////////
	// Reimplementation de methodes virtuelles de KWDataTableDriver
	boolean OpenForRead(const KWClass* kwcLogicalClass) override;
	boolean IsEnd() const override;
	longint GetEstimatedObjectNumber() override;
	longint ComputeNecessaryMemoryForFullExternalRead(const KWClass* kwcLogicalClass) override;
	longint ComputeNecessaryDiskSpaceForFullWrite(const KWClass* kwcLogicalClass, longint lInputFileSize) override;
	double GetReadPercentage() const override;
	longint GetUsedMemory() const override;

	/////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////
	// Reimplementation de methodes virtuelles de KWDataTableDriverTextFile
	// principalement pour desactiver le lien aux fichiers
	boolean UpdateInputBuffer() override;
	boolean FillInputBufferWithFullLines(longint lBeginPos, longint lMaxEndPos) override;
	boolean CheckInputBuffer() override;
	boolean OpenInputDatabaseFile() override;
	boolean OpenOutputDatabaseFile() override;
	boolean ReadHeaderLineFields(StringVector* svFirstLineFields) override;

	// Ligne d'en-tete
	ALString sHeaderLine;
};

///////////////////////////////////////////////////////////////////////////
// Specialisation pour les stream de InputBufferedFile
class StreamInputBufferedFile : public InputBufferedFile
{
public:
	// Constructeur
	StreamInputBufferedFile();
	~StreamInputBufferedFile();

	// Remplissage du buffer avec un enregistrement
	// Si depassement du buffer, renvoie false sans remplir le buffer
	boolean FillBufferWithRecord(const char* sInputRecord);

	// Reinitialisation du buffer
	void ResetBuffer();

	// Reimplementation de methodes virtuelles
	longint GetFileSize() const;
	boolean Open();
	boolean IsOpened() const;
	boolean Close();
};

///////////////////////////////////////////////////////////////////////////
// Specialisation pour les stream de OutputBufferedFile
class StreamOutputBufferedFile : public OutputBufferedFile
{
public:
	// Constructeur
	StreamOutputBufferedFile();
	~StreamOutputBufferedFile();

	// Remplissage d'un enregistrement avec le buffer
	// Renvoie false en cas de depassement de capacite (avec un '\0' en fin), true sinon
	boolean FillRecordWithBuffer(char* sOutputRecord, int nOutputMaxLength);

	// Reinitialisation du buffer
	void ResetBuffer();

	// Reimplementation de methodes virtuelles
	boolean Open();
	boolean IsOpened() const;
	boolean Close();
};
