// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataTableDriverStream.h"

KWDataTableDriverStream::KWDataTableDriverStream() {}

KWDataTableDriverStream::~KWDataTableDriverStream() {}

KWDataTableDriver* KWDataTableDriverStream::Create() const
{
	return new KWDataTableDriverStream;
}

void KWDataTableDriverStream::CopyFrom(const KWDataTableDriver* kwdtdSource)
{
	const KWDataTableDriverStream* kwdttfSource = cast(KWDataTableDriverStream*, kwdtdSource);

	// Copie standard
	KWDataTableDriverTextFile::CopyFrom(kwdtdSource);

	// Recopie des attribut supplementaires
	SetHeaderLine(kwdttfSource->GetHeaderLine());
}

void KWDataTableDriverStream::SetHeaderLine(const ALString& sValue)
{
	sHeaderLine = sValue;
}

const ALString& KWDataTableDriverStream::GetHeaderLine() const
{
	return sHeaderLine;
}

void KWDataTableDriverStream::ResetInputBuffer()
{
	StreamInputBufferedFile* streamInputBuffer;

	require(IsOpenedForRead());
	streamInputBuffer = cast(StreamInputBufferedFile*, inputBuffer);
	streamInputBuffer->ResetBuffer();

	// Remise a zero des index de records
	ResetDatabaseFile();
}

boolean KWDataTableDriverStream::FillBufferWithRecord(const char* sInputRecord)
{
	boolean bOk;
	StreamInputBufferedFile* streamInputBuffer;

	require(IsOpenedForRead());
	streamInputBuffer = cast(StreamInputBufferedFile*, inputBuffer);

	// Retaillage si necessaire du buffer du stream
	if (inputBuffer->GetBufferSize() < GetBufferSize())
		inputBuffer->SetBufferSize(GetBufferSize());

	// Recodage
	bOk = streamInputBuffer->FillBufferWithRecord(sInputRecord);
	return bOk;
}

void KWDataTableDriverStream::ResetOutputBuffer()
{
	StreamOutputBufferedFile* streamOutputBuffer;

	require(IsOpenedForWrite());
	streamOutputBuffer = cast(StreamOutputBufferedFile*, outputBuffer);
	streamOutputBuffer->ResetBuffer();

	// Remise a zero des index de records
	ResetDatabaseFile();
}

boolean KWDataTableDriverStream::FillRecordWithBuffer(char* sOutputRecord, int nOutputMaxLength)
{
	StreamOutputBufferedFile* streamOutputBuffer;

	require(IsOpenedForWrite());

	streamOutputBuffer = cast(StreamOutputBufferedFile*, outputBuffer);
	return streamOutputBuffer->FillRecordWithBuffer(sOutputRecord, nOutputMaxLength);
}

boolean KWDataTableDriverStream::OpenForRead(const KWClass* kwcLogicalClass)
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDataTableDriverTextFile::OpenForRead(kwcLogicalClass);

	// On reinitialise le buffer en entree, qui a etet utilise pour prepositionne la header-line
	if (bOk)
		ResetInputBuffer();
	return bOk;
}

boolean KWDataTableDriverStream::IsEnd() const
{
	return inputBuffer->IsBufferEnd();
}

longint KWDataTableDriverStream::GetEstimatedObjectNumber()
{
	return 0;
}

longint KWDataTableDriverStream::ComputeNecessaryMemoryForFullExternalRead(const KWClass* kwcLogicalClass)
{
	require(kwcLogicalClass != NULL);
	return 0;
}

longint KWDataTableDriverStream::ComputeNecessaryDiskSpaceForFullWrite(const KWClass* kwcLogicalClass,
								       longint lInputFileSize)
{
	require(kwcLogicalClass != NULL);
	require(lInputFileSize >= 0);
	return 0;
}

double KWDataTableDriverStream::GetReadPercentage()
{
	return 0;
}

longint KWDataTableDriverStream::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KWDataTableDriverTextFile::GetUsedMemory();
	lUsedMemory += sizeof(KWDataTableDriverStream) - sizeof(KWDataTableDriverTextFile);
	lUsedMemory += sHeaderLine.GetAllocLength();
	return lUsedMemory;
}

boolean KWDataTableDriverStream::UpdateInputBuffer()
{
	return true;
}

boolean KWDataTableDriverStream::FillInputBufferWithFullLines(longint lBeginPos, longint lMaxEndPos)
{
	require(0 <= lBeginPos);
	require(lBeginPos <= lMaxEndPos);
	return true;
}

boolean KWDataTableDriverStream::CheckInputBuffer()
{
	return true;
}

boolean KWDataTableDriverStream::OpenInputDatabaseFile()
{
	boolean bOk;

	require(inputBuffer == NULL);
	require(outputBuffer == NULL);

	// Initialisation des caracteristiques du fichier
	bWriteMode = false;

	// Initialisation du buffer
	inputBuffer = new StreamInputBufferedFile;
	inputBuffer->SetFieldSeparator(cFieldSeparator);
	inputBuffer->SetHeaderLineUsed(bHeaderLineUsed);
	inputBuffer->SetFileName(GetDataTableName());
	inputBuffer->SetBufferSize(nBufferSize);

	// Ouverture du fichier
	bOk = inputBuffer->Open();

	// On met la ligne de header dans le buffer
	if (bOk)
	{
		bOk = cast(StreamInputBufferedFile*, inputBuffer)->FillBufferWithRecord(sHeaderLine);
		if (not bOk)
			inputBuffer->Close();
	}

	// Nettoyage si erreur
	if (not bOk)
	{
		delete inputBuffer;
		inputBuffer = NULL;
	}
	return bOk;
}

boolean KWDataTableDriverStream::OpenOutputDatabaseFile()
{
	boolean bOk;

	require(inputBuffer == NULL);
	require(outputBuffer == NULL);

	// Initialisation des caracteristiques du fichier
	bWriteMode = false;

	// Initialisation du buffer
	outputBuffer = new StreamOutputBufferedFile;
	outputBuffer->SetFieldSeparator(cFieldSeparator);
	outputBuffer->SetHeaderLineUsed(bHeaderLineUsed);
	outputBuffer->SetFileName(GetDataTableName());
	outputBuffer->SetBufferSize(nBufferSize);

	// Ouverture du fichier
	bOk = outputBuffer->Open();
	if (bOk)
		bWriteMode = true;

	// Nettoyage si erreur
	if (not bOk)
	{
		delete outputBuffer;
		outputBuffer = NULL;
	}
	return bOk;
}

boolean KWDataTableDriverStream::ReadHeaderLineFields(StringVector* svFirstLineFields)
{
	boolean bOk;
	boolean bEndOfLine;
	boolean bLineTooLong;
	int nField;
	int nFieldLength;
	int nFieldError;
	char* sField;
	ALString sTmp;

	require(inputBuffer == NULL);
	require(svFirstLineFields != NULL);

	// Ouverture du buffer en lecture (la ligne de header est recopiee dans le buffer)
	bOk = OpenInputDatabaseFile();

	// Recherche des champs de la premiere ligne
	if (bOk)
	{
		assert(inputBuffer != NULL);
		bEndOfLine = false;
		nField = 0;
		while (not bEndOfLine)
		{
			bEndOfLine = inputBuffer->GetNextField(sField, nFieldLength, nFieldError, bLineTooLong);
			nField++;

			// Il ne doit pas y avoir de probleme de ligne trop longue dans KNI
			assert(not bLineTooLong);

			// Erreur sur le nom du champ
			if (nFieldError != inputBuffer->FieldNoError)
			{
				AddWarning(sTmp + "Header line field " + IntToString(nField) + " <" +
					   inputBuffer->GetDisplayValue(sField) +
					   "> : " + inputBuffer->GetFieldErrorLabel(nFieldError));
			}
			svFirstLineFields->Add(sField);
		}

		// Nettoyage
		inputBuffer->Close();
		delete inputBuffer;
		inputBuffer = NULL;
	}

	ensure(inputBuffer == NULL);
	return bOk;
}

///////////////////////////////////////////////////////////////////////////
// Classe StreamInputBufferedFile

StreamInputBufferedFile::StreamInputBufferedFile() {}

StreamInputBufferedFile::~StreamInputBufferedFile() {}

void StreamInputBufferedFile::ResetBuffer()
{
	// Initialisations
	nPositionInCache = 0;
	nCurrentBufferSize = 0;
	nCacheSize = 0;
	nCurrentLineIndex = 1;
	bLastFieldReachEol = false;
	nLastBolPositionInCache = 0;
	nBufferLineNumber = 0;
}

boolean StreamInputBufferedFile::FillBufferWithRecord(const char* sInputRecord)
{
	boolean bOk;
	int nInitialCurrentBufferSize;
	int nInitialBufferLineNumber;
	char c;
	int i;

	require(sInputRecord != NULL);

	// Retaillage si necessaire du buffer
	nInitialCurrentBufferSize = nCurrentBufferSize;
	nInitialBufferLineNumber = nBufferLineNumber;
	bOk = AllocateBuffer();

	// Recopie de la chaine de caracteres dans le buffer
	if (bOk)
	{
		// Recopie du record, dans la limite de la taille du buffer
		i = 0;
		bOk = false;
		while (nCurrentBufferSize < nBufferSize)
		{
			c = sInputRecord[i];

			// Arret si fin de chaine
			if (c == '\0')
			{
				// Ajout si necessaire d'un fin de ligne pour marquer la fin de l'enregistrement
				if (i == 0 or sInputRecord[i - 1] != '\n')
				{
					fcCache.SetAt(nCurrentBufferSize, '\n');
					nCurrentBufferSize++;
					nBufferLineNumber++;
				}
				bOk = true;
				break;
			}
			// Memorisation du caractere sinon
			else
			{
				fcCache.SetAt(nCurrentBufferSize, c);
				nCurrentBufferSize++;
				if (c == '\n')
					nBufferLineNumber++;
				i++;
			}
		}

		// Memorisation de la taille du cache a la taille du buffer courant
		if (bOk)
			nCacheSize = nCurrentBufferSize;

		// On annule la recopie du record en cas d'espace insuffisant
		if (not bOk)
		{
			nCurrentBufferSize = nInitialCurrentBufferSize;
			nBufferLineNumber = nInitialBufferLineNumber;
		}

		// Protection de la fin du buffer
		fcCache.SetAt(nBufferSize - 1, '\0');
	}
	ensure(nCurrentBufferSize >= nInitialCurrentBufferSize);
	ensure(not bOk or nCurrentBufferSize >= nInitialCurrentBufferSize + (int)strlen(sInputRecord));
	return bOk;
}

longint StreamInputBufferedFile::GetFileSize() const
{
	// Taille virtuellement infinie pour un stream
	return LLONG_MAX;
}

boolean StreamInputBufferedFile::Open()
{
	require(not bIsOpened);
	assert(nBufferSize > 0);

	bIsOpened = AllocateBuffer();
	if (bIsOpened)
		nPositionInCache = 0;
	return bIsOpened;
}

boolean StreamInputBufferedFile::IsOpened() const
{
	return bIsOpened;
}

boolean StreamInputBufferedFile::Close()
{
	bIsOpened = false;
	return true;
}

///////////////////////////////////////////////////////////////////////////
// Classe StreamOutputBufferedFile

StreamOutputBufferedFile::StreamOutputBufferedFile() {}

StreamOutputBufferedFile::~StreamOutputBufferedFile() {}

void StreamOutputBufferedFile::ResetBuffer()
{
	nCurrentBufferSize = 0;
}

boolean StreamOutputBufferedFile::FillRecordWithBuffer(char* sOutputRecord, int nOutputMaxLength)
{
	int i;
	char c;
	boolean bOk;

	require(sOutputRecord != NULL);
	require(nOutputMaxLength > 0);

	// Remplissage du record a partir du buffer
	bOk = false;
	for (i = 0; i < nCurrentBufferSize; i++)
	{
		// Erreur si pas assez de place
		if (i >= nOutputMaxLength)
		{
			// On met le resultat en sortie a vide
			sOutputRecord[0] = '\0';
			break;
		}

		// Memorisation du caractere courant
		c = fcCache.GetAt(i);
		sOutputRecord[i] = c;

		// Ok si fin de chaine
		if (c == '\0')
		{
			bOk = true;
			break;
		}

		// Ok si fin de ligne: on nettoie la fin de ligne
		if (c == '\n')
		{
			sOutputRecord[i] = '\0';
			if (i > 0 and sOutputRecord[i - 1] == '\r')
				sOutputRecord[i - 1] = '\0';
			bOk = true;
			break;
		}
	}
	// Protection de la fin du record
	if (nCurrentBufferSize < nOutputMaxLength)
		sOutputRecord[nCurrentBufferSize] = '\0';
	else
		sOutputRecord[nOutputMaxLength - 1] = '\0';
	return bOk;
}

boolean StreamOutputBufferedFile::Open()
{
	require(not bIsOpened);
	assert(nBufferSize > 0);
	bIsOpened = AllocateBuffer();
	return bIsOpened;
}

boolean StreamOutputBufferedFile::IsOpened() const
{
	return bIsOpened;
}

boolean StreamOutputBufferedFile::Close()
{
	bIsOpened = false;
	return true;
}
