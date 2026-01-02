// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLDataTableDriverTextFile.h"

PLDataTableDriverTextFile::PLDataTableDriverTextFile()
{
	lBeginPosition = 0;
	lEndPosition = 0;
	bIsOpened = false;
}

PLDataTableDriverTextFile::~PLDataTableDriverTextFile()
{
	// En parallele : Les buffers sont detruits avant l'appel a cette methode
	// on le met a NULL pour verifier les assertion du destructeur de la classe mere
	inputBuffer = NULL;
	outputBuffer = NULL;
}

void PLDataTableDriverTextFile::SetRecordIndex(longint lValue)
{
	require(lValue >= 0);
	lRecordIndex = lValue;
}

KWDataTableDriver* PLDataTableDriverTextFile::Create() const
{
	return new PLDataTableDriverTextFile;
}

boolean PLDataTableDriverTextFile::OpenForRead(const KWClass* kwcLogicalClass)
{
	// En parallele : Idem methode ancetre sans l'ouverture du fichier
	//					et sans calcul des index

	// Reinitialisation du fichier de la base de donnees
	ResetDatabaseFile();
	bIsOpened = true;
	bWriteMode = false;
	return true;
}

boolean PLDataTableDriverTextFile::OpenForWrite()
{
	// En parallele : le buffer est ouvert a l'exterieur de la classe : on ne fait rien
	bIsOpened = true;
	bWriteMode = true;
	return true;
}

boolean PLDataTableDriverTextFile::IsOpenedForRead() const
{
	return bIsOpened and not bWriteMode;
}

boolean PLDataTableDriverTextFile::IsOpenedForWrite() const
{
	return bIsOpened and bWriteMode;
}

longint PLDataTableDriverTextFile::GetEstimatedObjectNumber()
{
	longint lObjectNumber;
	InputBufferedFile* currentInputBuffer;

	// En parallele :   on remplace le buffer courant par un autre buffer
	//					pour que le buffer courant ne soit pas modifier

	// Sauvegarde du buffer courant avant l'appel a la fonction
	currentInputBuffer = inputBuffer;

	inputBuffer = NULL;
	lObjectNumber = KWDataTableDriverTextFile::GetEstimatedObjectNumber();

	// Restitution du buffer courant
	inputBuffer = currentInputBuffer;
	return lObjectNumber;
}

boolean PLDataTableDriverTextFile::BuildDataTableClass(KWClass* kwcDataTableClass)
{
	boolean bOk;
	InputBufferedFile* currentInputBuffer;

	// Sauvegarde du buffer courant avant l'appel a la fonction
	currentInputBuffer = inputBuffer;

	inputBuffer = NULL;
	bOk = KWDataTableDriverTextFile::BuildDataTableClass(kwcDataTableClass);

	// Restitution du buffer courant
	inputBuffer = currentInputBuffer;
	return bOk;
}

boolean PLDataTableDriverTextFile::Close()
{
	bIsOpened = false;
	lBeginPosition = 0;
	lEndPosition = 0;

	// En parallele : le buffer est ferme a l'exterieur de la classe : on ne fait rien
	return true;
}

longint PLDataTableDriverTextFile::GetEncodingErrorNumber() const
{
	// En parallele, la gestion des erreur d'encodage se fait dans les taches
	// On rend 0 si le buffer d'entree n'est pas exploitable
	if (inputBuffer == NULL or not inputBuffer->IsOpened())
		return 0;
	else
		return inputBuffer->GetEncodingErrorNumber();
}

boolean PLDataTableDriverTextFile::OpenInputDatabaseFile()
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDataTableDriverTextFile::OpenInputDatabaseFile();

	// Parametrage par defaut des points de depart pour la lecture et l'ecriture
	lBeginPosition = 0;
	lEndPosition = 0;
	if (bOk)
		lEndPosition = inputBuffer->GetBufferSize();
	return bOk;
}

boolean PLDataTableDriverTextFile::FillFirstInputBuffer()
{
	boolean bOk;

	require(not bWriteMode);
	require(inputBuffer != NULL);
	require(lBeginPosition <= lEndPosition);

	// Alimentation du buffer en forcant le debut du chunk parametre dans le driver
	bOk = FillInputBufferWithFullLines(lBeginPosition, lEndPosition);
	return bOk;
}

boolean PLDataTableDriverTextFile::UpdateInputBuffer()
{
	boolean bOk = true;

	require(not bWriteMode);
	require(inputBuffer != NULL);
	require(lBeginPosition <= lEndPosition);
	require(lBeginPosition <= inputBuffer->GetPositionInFile());

	if (inputBuffer->IsBufferEnd() and not IsEnd())
	{
		// On est ici strictement au milieu du chunk
		assert(lBeginPosition < inputBuffer->GetPositionInFile());
		assert(inputBuffer->GetPositionInFile() < lEndPosition);

		// Alimentation du buffer par rapport a la position en cours dans le fichier
		bOk = FillInputBufferWithFullLines(inputBuffer->GetPositionInFile(), lEndPosition);
	}
	return bOk;
}

double PLDataTableDriverTextFile::GetReadPercentage() const
{
	double dPercentage = 0;
	longint lPositionInFile;

	// Calcul du pourcentage d'avancement en se basant sur la position courante dans le fichier
	if (inputBuffer != NULL)
	{
		lPositionInFile = inputBuffer->GetPositionInFile();
		if (lPositionInFile <= lBeginPosition or lBeginPosition == lEndPosition)
			dPercentage = 0;
		else if (lPositionInFile >= lEndPosition)
			dPercentage = 1;
		else
			dPercentage = (lPositionInFile - lBeginPosition) * 1.0 / (lEndPosition - lBeginPosition);
	}
	return dPercentage;
}

void PLDataTableDriverTextFile::SetInputBuffer(InputBufferedFile* buffer)
{
	require(inputBuffer == NULL or outputBuffer == NULL);
	require(buffer == NULL or outputBuffer == NULL);

	inputBuffer = buffer;
}

InputBufferedFile* PLDataTableDriverTextFile::GetInputBuffer()
{
	return inputBuffer;
}

void PLDataTableDriverTextFile::SetOutputBuffer(OutputBufferedFile* buffer)
{
	require(inputBuffer == NULL or outputBuffer == NULL);
	require(inputBuffer == NULL or buffer == NULL);

	bWriteMode = false;
	outputBuffer = buffer;
	if (outputBuffer != NULL)
		bWriteMode = true;
}

OutputBufferedFile* PLDataTableDriverTextFile::GetOutputBuffer()
{
	return outputBuffer;
}

boolean PLDataTableDriverTextFile::ComputeDataItemLoadIndexes(const KWClass* kwcLogicalClass,
							      const KWClass* kwcHeaderLineClass)
{
	// En parallele : Idem methode ancetre sans l'ouverture du fichier
	boolean bOk = true;

	// Reinitialisation du fichier de la base de donnees
	ResetDatabaseFile();

	// Calcul des index des champs de la classe physique
	bOk = KWDataTableDriverTextFile::ComputeDataItemLoadIndexes(kwcLogicalClass, kwcHeaderLineClass);
	return bOk;
}

const KWLoadIndexVector* PLDataTableDriverTextFile::GetConstDataItemLoadIndexes() const
{
	return &livDataItemLoadIndexes;
}

KWLoadIndexVector* PLDataTableDriverTextFile::GetDataItemLoadIndexes()
{
	return &livDataItemLoadIndexes;
}

const IntVector* PLDataTableDriverTextFile::GetConstMainKeyIndexes() const
{
	return &ivMainKeyIndexes;
}

IntVector* PLDataTableDriverTextFile::GetMainKeyIndexes()
{
	return &ivMainKeyIndexes;
}

void PLDataTableDriverTextFile::InitializeLastReadKeySize(int nValue)
{
	require(nValue >= 0);
	lastReadMainKey.SetSize(nValue);
	lastReadMainKey.Initialize();
}
