// Copyright (c) 2023 Orange. All rights reserved.
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
	// En parallele :   on remplace le buffer courant par un autre buffer
	//					pour que le buffer courant ne soit pas modifier

	longint lObjectNumber;
	InputBufferedFile* currentInputBuffer;

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
	longint lOverallSize;

	require(not bWriteMode);
	require(inputBuffer != NULL);
	require(lBeginPosition <= lEndPosition);

	// Retaillage eventuel du buffer
	lOverallSize = lEndPosition - lBeginPosition;
	assert(lOverallSize >= 0);
	if (lOverallSize < inputBuffer->GetBufferSize())
		inputBuffer->SetBufferSize((int)lOverallSize);

	// Alimentation du buffer
	bOk = inputBuffer->Fill(lBeginPosition);
	ensure(inputBuffer->IsFileEnd() or inputBuffer->IsError() or inputBuffer->IsBufferEnd() or lOverallSize > 0);
	return bOk;
}

boolean PLDataTableDriverTextFile::UpdateInputBuffer()
{
	boolean bOk = true;
	longint lRemainingSize;

	require(lBeginPosition <= lEndPosition);
	require(lBeginPosition <= inputBuffer->GetPositionInFile());

	if (inputBuffer->IsBufferEnd() and not IsEnd())
	{
		assert(inputBuffer->GetPositionInFile() < lEndPosition);

		// Retaillage du buffer s'il ne reste pas beaucoup a lire
		lRemainingSize = lEndPosition - inputBuffer->GetPositionInFile();
		assert(lRemainingSize > 0);
		if (lRemainingSize < inputBuffer->GetBufferSize())
			inputBuffer->SetBufferSize((int)lRemainingSize);

		// Alimentation du buffer
		bOk = FillInputBufferWithFullLines();
	}
	return bOk;
}

double PLDataTableDriverTextFile::GetReadPercentage()
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

boolean PLDataTableDriverTextFile::ComputeDataItemLoadIndexes(const KWClass* kwcLogicalClass)
{
	// En parallele : Idem methode ancetre sans l'ouverture du fichier
	boolean bOk = true;
	KWClass kwcHeaderLineClass;
	KWClass* kwcUsedHeaderLineClass;

	// Reinitialisation du fichier de la base de donnees
	ResetDatabaseFile();

	// Creation d'une classe fictive basee sur l'analyse de la premiere ligne du fichier, dans le cas d'une ligne
	// d'entete pour obtenir le nombre de champs et leur eventuels nom et position (sans se soucier de leur type)
	kwcUsedHeaderLineClass = NULL;
	if (GetHeaderLineUsed())
	{
		kwcHeaderLineClass.SetName(kwcClass->GetDomain()->BuildClassName(GetClassName() + "HeaderLine"));
		kwcClass->GetDomain()->InsertClass(&kwcHeaderLineClass);
		bOk = BuildDataTableClass(&kwcHeaderLineClass);
		kwcClass->GetDomain()->RemoveClass(kwcHeaderLineClass.GetName());
		kwcUsedHeaderLineClass = &kwcHeaderLineClass;
	}

	// Calcul des index des champs de la classe physique
	if (bOk)
		bOk = KWDataTableDriverTextFile::ComputeDataItemLoadIndexes(kwcLogicalClass, kwcUsedHeaderLineClass);
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

const IntVector* PLDataTableDriverTextFile::GetConstRootKeyIndexes() const
{
	return &ivRootKeyIndexes;
}

IntVector* PLDataTableDriverTextFile::GetRootKeyIndexes()
{
	return &ivRootKeyIndexes;
}

void PLDataTableDriverTextFile::InitializeLastReadKeySize(int nValue)
{
	require(nValue >= 0);
	lastReadRootKey.SetSize(nValue);
	lastReadRootKey.Initialize();
}