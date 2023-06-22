// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "InputBufferedFile.h"
#include "SystemFileDriver.h"

int InputBufferedFile::nMaxLineLength = 8 * lMB;

///////////////////////////////////////////////////////////////////////////////
// Buffer unique de grande taille pour la gestion des champs dans la methode
// InputBufferedFile::GetNextField
// Pour eviter les problemes avec les DLL:
//  . on utilise un methode statique, dans un unnamed namespace,
//    pour forcer un acces local uniquement depuis ce fichier,
//  . le buffer est lui-meme une variable statique de cette methode statique

namespace
{
// Declaration du buffer
static char* sInputBufferedFileHugeBuffer = NULL;

// Destruction du buffer
static void InputBufferedFileHugeBufferDelete()
{
	if (sInputBufferedFileHugeBuffer != NULL)
	{
		SystemObject::DeleteCharArray(sInputBufferedFileHugeBuffer);
		sInputBufferedFileHugeBuffer = NULL;
	}
}

// Creation du buffer
static void InputBufferedFileHugeBufferNew()
{
	int i;
	int nSize;

	if (sInputBufferedFileHugeBuffer == NULL)
	{
		// Creation et initialisation du buffer
		// On n'utilise pas max car sur linux on a une reference indefinie vers
		// « InputBufferedFile::nHugeBufferSize »
		nSize = InputBufferedFile::nHugeBufferSize;
		if (nSize < InputBufferedFile::nMaxFieldSize + 1)
			nSize = InputBufferedFile::nMaxFieldSize + 1;
		sInputBufferedFileHugeBuffer = SystemObject::NewCharArray(nSize);
		for (i = 0; i <= InputBufferedFile::nMaxFieldSize; i++)
			sInputBufferedFileHugeBuffer[i] = '\0';

		// Enregistrement de sa destruction en fin de programme
		atexit(InputBufferedFileHugeBufferDelete);
	}
}

// Acces au buffer
static inline char* InputBufferedFileGetHugeBuffer()
{
	if (sInputBufferedFileHugeBuffer == NULL)
		InputBufferedFileHugeBufferNew();
	return sInputBufferedFileHugeBuffer;
}
} // namespace

InputBufferedFile::InputBufferedFile()
{
	Reset();
}

void InputBufferedFile::SetFileName(const ALString& sValue)
{
	BufferedFile::SetFileName(sValue);
}

void InputBufferedFile::CopyFrom(const InputBufferedFile* bufferedFile)
{
	// Copie Standard
	BufferedFile::CopyFrom(bufferedFile);

	// Reinitialisation des attributs supplementaires
	Reset();
}

InputBufferedFile::~InputBufferedFile(void)
{
	assert(not bIsOpened);
}

boolean InputBufferedFile::Open()
{
	boolean bOk;
	ALString sLocalFileName;

	require(GetFileName() != "");
	require(not IsOpened());
	require(fileHandle == NULL);

	// Reinitialisation des donnees de travail
	Reset();
	bOk = true;

	// Si le fichier est local
	if (GetFileDriverCreator()->IsLocal(sFileName))
	{
		fileHandle = new SystemFile;

		if (FileService::GetURIScheme(sFileName) == FileService::sRemoteScheme)
		{
			assert(FileService::GetURIHostName(sFileName) == GetLocalHostName());
			sLocalFileName = FileService::GetURIFilePathName(sFileName);
		}
		else
			sLocalFileName = sFileName;

		// Ouverture du fichier
		bOk = fileHandle->OpenInputFile(sLocalFileName);

		// On calcul la taille du fichier
		lFileSize = fileHandle->GetFileSize(sLocalFileName);

		if (not bOk and fileHandle != NULL)
		{
			delete fileHandle;
			fileHandle = NULL;
		}
	}
	else
	{
		// Creation du driver
		assert(fileDriver == NULL);
		fileDriver = GetFileDriverCreator()->CreateBufferedFileDriver(sFileName, this);
		if (fileDriver != NULL)
		{
			// L'acces a la taille du fichier est fait dans le Open
			bOk = fileDriver->OpenForRead(this);
		}
		else
		{
			bOk = false;
		}
	}

	// Fin des initialisation
	bIsOpened = bOk;
	bIsError = not bOk;
	if (bOk)
		lBufferBeginPos = 0;
	lLastEndBufferPos = LONG_MAX;
	return bOk;
}

boolean InputBufferedFile::Close()
{
	boolean bOk;

	require(IsOpened());

	// Fermeture physique du fichier en local seulement
	bOk = true;

	if (GetFileDriverCreator()->IsLocal(this->sFileName))
	{
		bOk = fileHandle->CloseInputFile(sFileName);
		delete fileHandle;
		fileHandle = NULL;
	}
	else
	{
		if (fileDriver != NULL)
		{
			bOk = fileDriver->Close();
			delete fileDriver;
			fileDriver = NULL;
		}
		else
			bOk = false;
	}

	bIsOpened = false;
	bIsError = false;
	ensure(fileHandle == NULL);

	// Reinitialisation des attributs supplementaires
	Reset();
	return bOk;
}

void InputBufferedFile::SetBufferSize(int nValue)
{
	BufferedFile::SetBufferSize(nValue);
	nAllocatedBufferSize = nValue;
}

boolean InputBufferedFile::Fill(longint lBeginPos)
{
	boolean bOk;
	require(nBufferSize <= GetMaxBufferSize());

	// Initialisation
	nPositionInBuffer = 0;
	nBufferLineNumber = 0;

	// Lecture dans le buffer, avec positionnement de l'indicateur fin de fichier
	if (GetFileDriverCreator()->IsLocal(this->sFileName))
	{
		// Ajout de stats memoire
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(GetClassLabel() + " Local Fill Begin");

		require(0 <= lBeginPos and lBeginPos <= lFileSize);
		bOk = FillLocal(lBeginPos);

		// Ajout de stats memoire
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(GetClassLabel() + " Local Fill End");
	}
	else
	{
		// Ajout de stats memoire
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(GetClassLabel() + " Remote Fill Begin");

		assert(fileDriver != NULL);
		require(nBufferSize <= GetMaxBufferSize());
		fileDriver->Fill(this, lBeginPos, bIsSkippedLine, lBufferBeginPos);
		bOk = not bIsError;
		if (bOk)
			nBufferLineNumber = fbBuffer.ComputeLineNumber(nCurrentBufferSize);

		// Ajout de stats memoire
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(GetClassLabel() + " Remote Fill End");
	}
	if (bOk)
	{
		// Si on est arrive a la fin du fichier sans avoir lu le separateur fin de fichier
		// on position la fin de fichier
		if (lBufferBeginPos + nCurrentBufferSize == lFileSize)
			bEof = true;
	}

	return bOk;
}

boolean InputBufferedFile::BasicFill(longint lBeginPos)
{
	int nReadSize;
	boolean bOk;

	// Initialisations
	bIsHeadOfFile = (lBeginPos == 0);
	lBufferBeginPos = lBeginPos;
	nPositionInBuffer = 0;
	nReadLineNumber = 0;
	bLastFieldReachEol = false;

	nAllocatedBufferSize = nBufferSize;
	bOk = AllocateBuffer();

	if (bOk)
	{
		// Lecture dans le buffer, avec positionnement de l'indicateur fin de fichier
		if (InputBufferedFile::GetFileDriverCreator()->IsLocal(this->sFileName))
		{
			// Ajout de stats memoire
			if (FileService::LogIOStats())
				MemoryStatsManager::AddLog(GetClassLabel() + " BasicFill Local Begin");

			require(0 <= lBeginPos and lBeginPos <= lFileSize);
			nReadSize = fbBuffer.ReadFromFile(fileHandle, lBeginPos, nBufferSize, this);

			// Ajout de stats memoire
			if (FileService::LogIOStats())
				MemoryStatsManager::AddLog(GetClassLabel() + " BasicFill Local End");

			bOk = nReadSize != 0;
			if (not bOk)
			{
				nCurrentBufferSize = 0;
				bIsError = true;
			}
			else
				nCurrentBufferSize = nReadSize;
		}
		else
		{
			assert(fileDriver != NULL);

			// Ajout de stats memoire
			if (FileService::LogIOStats())
				MemoryStatsManager::AddLog(GetClassLabel() + " BasicFill Remote Begin");

			// En cas d'erreur l'attribut bIsError de InputBufferedFile est mis a true
			fileDriver->Read(this, lBeginPos);

			// Ajout de stats memoire
			if (FileService::LogIOStats())
				MemoryStatsManager::AddLog(GetClassLabel() + " BasicFill Remote End");
		}
	}
	else
		bIsError = true;

	return not bIsError;
}

boolean InputBufferedFile::FillLocal(longint lBeginPos)
{
	boolean bEolFound;
	boolean bBolFound;    // Debut de ligne trouve
	int nBeginLinePos;    // Premier debut de ligne dans le buffer
	int nBufferSizeLimit; // Taille limite du buffer pour ne pas detecter des lignes plus grandes que
			      // GetMaxLineLength
	int nExtraPartPos; // Position de depart de la partie supplementaire du buffer : nChunkSize - nBeginLinePos - 1;
	int nEolPos;
	int nBufferIndex;
	int nLocalRead;
	int nSizeToRead;
	int nYetToRead;
	int nStartPos;
	longint lCumulatedRead;
	boolean bOk = true;
	boolean bVerbose = false;
	boolean bHaveToSearchEol;
	longint lNewBeginPos; // Permet d'optimiser le remplissage lorsque lBeginPos est superieur a lLastEndBufferPos
	bEof = false;
	char* sHugeBuffer;

	require(nBufferSize >= 0);
	require(nBufferSize <= GetMaxBufferSize());

	// Initialisation
	nBeginLinePos = 0;
	bIsSkippedLine = false;
	bBolFound = false;
	bEolFound = false;
	nEolPos = 0;
	nReadLineNumber = 0;
	nCurrentBufferSize = 0;
	lCumulatedRead = 0;
	sHugeBuffer = InputBufferedFileGetHugeBuffer();

	// Effet de bord
	if (lBeginPos >= lFileSize)
	{
		nCurrentBufferSize = 0;
		bEof = true;
		lNextFilePos = -1;
		return true;
	}

	// Optimisation : si lBeginPos est entre les lBeginPos+nChunkSize et lNextFilePos du dernier remplissage
	// On commence a lNextFilePos (car on est sur qu'il n'y a pas de debut de ligne)
	if (lBeginPos >= lLastEndBufferPos and lBeginPos < lNextFilePos)
	{
		lNewBeginPos = lNextFilePos;
	}
	else
	{
		lNewBeginPos = lBeginPos;
	}
	// Positionnement de la lecture sur lNewBeginPos et
	// Recherche du debut de la premiere ligne
	bOk = FindBol(lNewBeginPos, nBufferSize, lFileSize, fileHandle, nBeginLinePos, bBolFound);
	bIsError = not bOk;
	lBufferBeginPos = lNewBeginPos + nBeginLinePos;

	if (bVerbose)
	{
		if (bBolFound)
			cout << endl
			     << "Begin line at " << LongintToReadableString(lBufferBeginPos) << " (initial pos "
			     << LongintToReadableString(lNewBeginPos) << ", nBeginLinePos "
			     << LongintToReadableString(nBeginLinePos) << ")"
			     << " Buffer Size " << LongintToReadableString(nBufferSize) << " file size "
			     << LongintToReadableString(lFileSize) << endl;
		else
			cout << endl
			     << "Begin line not found "
			     << " (initial pos " << LongintToReadableString(lNewBeginPos) << ")"
			     << " Buffer Size " << LongintToReadableString(nBufferSize) << endl;
	}

	// Position a partir de laquelle il faut rechercher eol
	nExtraPartPos = nBufferSize - (int)(lNewBeginPos - lBeginPos + nBeginLinePos) - 1;
	assert(lNewBeginPos + nBeginLinePos + nExtraPartPos == lBeginPos + nBufferSize - 1);

	// Taille limite du buffer pour ne pas detecter des lignes plus grandes que GetMaxLineLength
	nBufferSizeLimit = nExtraPartPos + GetMaxLineLength();

	// Lecture bloc par bloc pour trouver le prochain eol
	nYetToRead = nBufferSizeLimit;
	bEolFound = false;
	nBufferIndex = 0;

	if (nYetToRead + lBufferBeginPos > lFileSize)
		nYetToRead = (int)(lFileSize - lBufferBeginPos);

	nAllocatedBufferSize = nBufferSize;
	bOk = AllocateBuffer();

	while (bBolFound and not bEolFound and nYetToRead > 0 and not bEof and bOk)
	{
		if (nYetToRead > nHugeBufferSize /*InternalGetBlockSize()*/)
			nSizeToRead = nHugeBufferSize /*InternalGetBlockSize()*/;
		else
			nSizeToRead = nYetToRead;

		nLocalRead = (int)fileHandle->Read(sHugeBuffer, InternalGetElementSize(), (size_t)nSizeToRead);
		// cout << "Read " << nSizeToRead << " => " << nLocalRead << endl;
		// cout << "Read " << LongintToHumanReadableString(nSizeToRead) << " => " <<
		// LongintToHumanReadableString(nLocalRead) << endl;
		nYetToRead -= nLocalRead;
		lCumulatedRead += nLocalRead;
		bOk = nLocalRead != -1;
		if (not bOk)
		{
			bIsError = true;
			AddError(fileHandle->GetLastErrorMessage());
		}
		else
		{
			// On agrandir le buffer
			if (nCurrentBufferSize + nLocalRead > nAllocatedBufferSize)
			{
				nAllocatedBufferSize = nCurrentBufferSize + nLocalRead;
				AllocateBuffer();
			}

			bEof = nLocalRead != nSizeToRead;
		}

		assert(not bIsError or nLocalRead == nSizeToRead);

		int nYetToFill = nLocalRead;
		int nSizeToFill = 0;
		int nTotalFilled = 0;
		while (nYetToFill > 0)
		{

			if (nYetToFill > InternalGetBlockSize())
				nSizeToFill = InternalGetBlockSize();
			else
				nSizeToFill = nYetToFill;

			// Lecture d'un bloc
			if (InternalGetAllocSize() <= InternalGetBlockSize())
			{

				memcpy(InternalGetMonoBlockBuffer(), &sHugeBuffer[nTotalFilled], (size_t)nSizeToFill);
			}
			else
			{
				memcpy(InternalGetMultiBlockBuffer(nBufferIndex / InternalGetBlockSize()),
				       &sHugeBuffer[nTotalFilled], (size_t)nSizeToFill);
			}
			nYetToFill -= nSizeToFill;
			nTotalFilled += nSizeToFill;

			// Recherche de eol dans le bloc qu'on vient d'ajouter, mais seulement dans la partie
			// supplementaire, apres nBufferSize - nBeginLinePos -1

			// Soit on est avant nExtraPartPos => on ne cherche pas eol,
			// soit on est a cheval sur nExtraPartPos => on ne cherche qu'a partir de nExtraPartPos,
			// soit on a depasse nExtraPartPos => on cherche dans tout le bloc
			bHaveToSearchEol = true;
			nStartPos = -1;
			if (nBufferIndex < nExtraPartPos)
			{
				if (nExtraPartPos < nBufferIndex + nSizeToFill)
					// A cheval : on va chercher eol a partir de nExtraPartPos
					nStartPos = nExtraPartPos;
				else
				{
					// Avant : on ne cherche pas eol
					bHaveToSearchEol = false;

					// Mais si fin de fichier, eol trouvee
					if (bEof)
					{
						bEolFound = true;
						bEof = true;
					}
				}
			}
			else
			{
				// On a depasse nExtraPartPos, on va chercher eol dans tout le bloc
				nStartPos = nBufferIndex;
			}

			// Recherche de eol a partir de nStartPos
			if (bHaveToSearchEol)
			{
				assert(nStartPos != -1);
				for (nEolPos = nStartPos; nEolPos < nBufferIndex + nSizeToFill; nEolPos++)
				{
					if (nEolPos == nBufferSizeLimit)
						break;

					if (fbBuffer.GetAt(nEolPos) == '\n')
					{
						bEolFound = true;
						break;
					}
				}

				if (bVerbose and bEolFound)
					cout << "Find eol at " << LongintToReadableString(nEolPos) << " => "
					     << LongintToReadableString(lNewBeginPos + nBeginLinePos + nEolPos) << endl;
				if (bEolFound)
					nCurrentBufferSize = nEolPos + 1;
				else
					nCurrentBufferSize += nSizeToFill;
			}
			else
				nCurrentBufferSize += nSizeToFill;

			nBufferIndex += nSizeToFill;
			if (bEolFound)
				break;
		}
	}

	// On n'a pas forcement lu la fin de fichier
	if (not bEof and lBufferBeginPos + nCurrentBufferSize == lFileSize)
	{
		bEof = true;
	}

	// Si fin de fichier, on a aussi EOL
	if (bEof)
	{
		lNextFilePos = -1;
		bEolFound = true;
	}
	else
	{
		// Si on a trouve EOL, la prochaine position de debut est nEolPos
		// Sinon c'est tout ce qu'on a parcouru (nEolPos, la meme chose): il est inutile de relire cette partie
		// du fichier
		lNextFilePos = lBufferBeginPos + nEolPos + 1;
	}

	if (bOk and not bEolFound and not bEof)
	{
		// Si on n'a pas trouve de eol et qu'on a trouve un debut de ligne
		// On indique qu'on saute cette ligne et on remplit le buffer au minimum
		if (bBolFound)
		{
			bIsSkippedLine = true;

			// Recherche de la derniere fin de ligne dans le buffer
			for (nBufferIndex = nExtraPartPos - 1; nBufferIndex > 0; nBufferIndex--)
			{

				if (fbBuffer.GetAt(nBufferIndex) == '\n')
					break;
			}
			nCurrentBufferSize = nBufferIndex;

			// On positionne la fin de ligne
			lNextFilePos = lBufferBeginPos + nBufferIndex + 1;
		}
		else
		{
			// On n'a trouve ni debut ni fin, on ne garde rien
			nCurrentBufferSize = 0;
		}
	}

	// En cas d'erreur le buffer est vide
	if (not bOk or not bBolFound)
		nCurrentBufferSize = 0;

	assert(nAllocatedBufferSize >= nCurrentBufferSize);

	// Modification de la taille du buffer
	nAllocatedBufferSize = nCurrentBufferSize;
	PruneBuffer();

	if (bVerbose)
	{
		if (bEof)
			cout << "Find EOF" << endl;
		if (bIsSkippedLine)
			cout << "line skipped" << endl;
	}
	lLastEndBufferPos = lBeginPos + nBufferSize;

	return bOk;
}

longint InputBufferedFile::FindEolPosition(longint lBeginPos, boolean& bEolFound)
{

	// TODO est-ce que cette methode est utilisee ???
	boolean bOk;
	bEolFound = false;
	char c;
	Timer tTimer;
	longint lEolPos;
	char* cBuffer;
	const int nSmallBufferSize = 1 * lMB;
	longint lCharNumber;
	longint lTotalRead;
	longint lRead;
	int i;

	require(IsOpened());
	require(lBeginPos <= GetFileSize());

	// Si le fichier est local
	if (GetFileDriverCreator()->IsLocal(this->sFileName))
	{

		// Demande d'acces au disque
		if (FileBuffer::GetRequestIOFunction() != NULL)
			FileBuffer::fRequestIOFunction(0);

		// On se positionne dans le fichier
		bOk = fileHandle->SeekPositionInFile(lBeginPos);
		bIsError = not bOk;
		if (bIsError)
			return 0;

		// On cherche la fin de ligne
		// Affichage d'un message toutes les 10s si la fin de ligne est tres longue a trouvee
		// dans une methode exterieure (bPostRecurringMessage = true)
		bEolFound = false;
		cBuffer = NewCharArray(nSmallBufferSize);
		lTotalRead = 0;
		lCharNumber = 0;
		while (lTotalRead < GetFileSize() and bOk and not bEolFound)
		{
			lRead = fileHandle->Read(cBuffer, sizeof(char), nSmallBufferSize);
			lTotalRead += lRead;
			bOk = lRead != -1;
			if (bOk)
			{
				i = 0;
				while (i < lRead)
				{
					c = cBuffer[i];
					if (c == '\n')
					{
						bEolFound = true;
						break;
					}

					// Test periodique du timer
					if (lCharNumber % 65536 == 0)
					{
						tTimer.Stop();
						if (tTimer.GetElapsedTime() > 10)
						{
							AddWarning("Seeking end of line...");
							tTimer.Reset();
						}
						tTimer.Start();
					}
					i++;
					lCharNumber++;
				}
			}

			bEof = lRead != nSmallBufferSize and bOk;
			assert((bEof and lTotalRead == GetFileSize()) or (not bEof and lTotalRead != GetFileSize()));
		}
		DeleteCharArray(cBuffer);
		// Relachement du disque
		if (FileBuffer::GetReleaseIOFunction() != NULL)
			FileBuffer::fReleaseIOFunction(0);

		lEolPos = lBeginPos + lCharNumber;
	}
	else
	{
		lEolPos = fileDriver->FindEOL(this, lBeginPos, bEolFound);
		bOk = true;
	}

	// En cas d'erreur on ne trouver rien
	if (not bOk)
	{
		lEolPos = 0;
		bIsError = true;
		bEolFound = false;
	}

	return lEolPos;
}

const ALString InputBufferedFile::GetFieldErrorLabel(int nFieldError)
{
	require(FieldNoError < nFieldError and nFieldError <= FieldTooLong);

	if (nFieldError == FieldTabReplaced)
		return "tabulation replaced by space char";
	else if (nFieldError == FieldCtrlZReplaced)
		return "Ctrl-Z (ascii 26) replaced by space char";
	else if (nFieldError == FieldMiddleDoubleQuote)
		return "double-quote in the middle of the field should be paired";
	else if (nFieldError == FieldMissingEndDoubleQuote)
		return "missing double-quote at the end of the field";
	else
	{
		assert(nFieldError == FieldTooLong);
		return "field too long";
	}
}

boolean InputBufferedFile::GetNextField(char*& sField, int& nFieldError)
{
	const char cCtrlZ = char(26);
	boolean bUnconditionalLoop = true; // Pour eviter un warning dans la boucle
	char c;
	boolean bEndOfLine;
	int i;
	int iStart;

	// Acces au buffer
	sField = InputBufferedFileGetHugeBuffer();
	assert(sField != NULL);

	// Si le dernier champ lu etait sur une fin de ligne,
	// nous sommes sur un debut de ligne...
	if (bLastFieldReachEol)
	{
		nReadLineNumber++;
		bLastFieldReachEol = false;
	}

	// Lecture des caracteres du token
	i = 0;
	nFieldError = FieldNoError;
	bEndOfLine = true;
	if (not IsBufferEnd())
	{
		// Lecture du premier caractere: traitement special si double-quote
		c = GetNextChar();

		// Traitement special si le champ commence par double-quote
		// (sauf si separateur double-quote)
		if (c == '"' and cFieldSeparator != '"')
		{
			bEndOfLine = GetNextDoubleQuoteField(sField, i, nFieldError);
		}
		// Traitement standard si le champ ne commence pas par double-quote
		else
		{
			// Analyse standard du champ
			while (bUnconditionalLoop)
			{
				// Arret si fin de ligne
				if (c == '\n')
				{
					bLastFieldReachEol = true;
					break;
				}

				// Arret si fin de champ
				if (c == cFieldSeparator)
				{
					bEndOfLine = false;
					break;
				}

				// Mise a jour du champ si pas de depassement de longueur
				if (i < (int)nMaxFieldSize)
				{
					sField[i] = c;
					i++;
				}

				// Caractere suivant
				if (IsBufferEnd())
					break;
				c = GetNextChar();
			}
		}
	}

	// Test de depassement de longueur
	if (i >= (int)nMaxFieldSize)
		nFieldError = FieldTooLong;

	// Supression des blancs en fin (TrimRight)
	i--;
	while (i >= 0)
	{
		c = sField[i];
		if (not iswspace(c) and c != cCtrlZ)
			break;
		i--;
	}
	i++;

	// Fin de champ
	sField[i] = '\0';

	// Supression des blancs en debut (TrimLeft)
	iStart = 0;
	while ((c = sField[iStart]) != '\0' and (iswspace(c) or c == cCtrlZ))
		iStart++;
	if (iStart > 0)
	{
		// Mise a jour de la nouvelle longueur du champ
		i -= iStart;

		// Copie de la fin du champ (y compris le '\0')
		memmove(sField, &sField[iStart], i + 1);
	}
	assert(sField[i] == '\0');

	// Remplacement des tabulations et Ctrl-Z par des blancs
	iStart = i - 1;
	while (iStart >= 0)
	{
		c = sField[iStart];
		// Remplacement de la tabulation, qui pose probleme dans les rapports de visualisation
		if (c == '\t')
		{
			if (nFieldError == FieldNoError)
				nFieldError = FieldTabReplaced;
			sField[iStart] = ' ';
		}
		// Remplacement du caractere Ctrl-Z, qui fait planter le parser lex et yacc des dictionnaires
		else if (c == cCtrlZ)
		{
			if (nFieldError == FieldNoError)
				nFieldError = FieldCtrlZReplaced;
			sField[iStart] = ' ';
		}
		iStart--;
	}

	// Retour du fin de ligne
	return bEndOfLine;
}

boolean InputBufferedFile::SkipField()
{
	boolean bUnconditionalLoop = true; // Pour eviter un warning dans la boucle
	char c;

	// Si le dernier champ lu etait sur une fin de ligne,
	// nous sommes sur un debut de ligne...
	if (bLastFieldReachEol)
	{
		nReadLineNumber++;
		bLastFieldReachEol = false;
	}

	// Lecture des caracteres du token
	if (not IsBufferEnd())
	{
		// Lecture du premier caractere: traitement special si double-quote
		c = GetNextChar();

		// Traitement special si le champ commence par double-quote
		// (sauf si separateur double-quote)
		if (c == '"' and cFieldSeparator != '"')
			return SkipDoubleQuoteField();
		// Traitement standard si le champ ne commence pas par double-quote
		else
		{
			// Analyse standard du champ
			while (bUnconditionalLoop)
			{
				// Arret si fin de ligne
				if (c == '\n')
				{
					bLastFieldReachEol = true;
					return true;
				}

				// Arret si fin de champ
				if (c == cFieldSeparator)
					return false;

				// Caractere suivant
				if (IsBufferEnd())
					break;
				c = GetNextChar();
			}
		}
	}

	// Retour de fin de ligne (fin de buffer)
	return true;
}

void InputBufferedFile::GetNextLine(CharVector* cvLine)
{
	char c;

	require(cvLine != NULL);

	// Lecture caractere a caractere
	cvLine->SetSize(0);
	while (not IsBufferEnd())
	{
		c = GetNextChar();
		cvLine->Add(c);

		// Gestion de la fin de ligne
		if (c == '\n')
		{
			nReadLineNumber++;
			return;
		}
	}
}

void InputBufferedFile::ExtractSubBuffer(int nBeginPos, int nEndPos, CharVector* cvSubBuffer) const
{
	int i;
	int j;

	require(nBeginPos >= 0 and nBeginPos < nEndPos);
	require(nEndPos <= fbBuffer.GetSize());
	require(cvSubBuffer != NULL);

	// Retaillage du resultat
	cvSubBuffer->SetSize(nEndPos - nBeginPos);

	// Recopie de la sous-partie du buffer
	j = 0;
	for (i = nBeginPos; i < nEndPos; i++)
	{
		cvSubBuffer->SetAt(j, fbBuffer.GetAt(i));
		j++;
	}
}

boolean InputBufferedFile::GetNextDoubleQuoteField(char* sField, int& i, int& nFieldError)
{
	boolean bUnconditionalLoop = true; // Pour eviter un warning dans la boucle
	char c;
	boolean bEndOfLine;
	int iNew;
	int iLast;

	require(sField != NULL);
	require(i == 0);
	require(nFieldError == FieldNoError);

	// On est cense avoir lu un double-quote en debut de champ
	sField[0] = '"';
	i = 1;

	// Le champ doit se terminer par un double-quote
	// On accepte les double-quotes (doubles) a l'interieur du champ, et meme le separateur de champ
	// En cas d'erreur (pas de double-quote final ou double-quote non double au milieu), on
	// parse "normalement" jusqu'au prochain separateur et on rend le champ entier tel quel
	// (y compris le premier double-quote)
	nFieldError = FieldMissingEndDoubleQuote;
	bEndOfLine = true;
	// Si on ne passe pas dans la boucle, on sera en erreur (double-quote suivi de fin de fichier)
	if (not IsBufferEnd())
	{
		// Lecture du premier caractere: traitement special si double-quote
		c = GetNextChar();

		// Traitement special si le champ commence par double-quote
		while (bUnconditionalLoop)
		{
			// Test si double-quote
			if (c == '"')
			{
				// OK si fin du buffer
				if (IsBufferEnd())
				{
					nFieldError = FieldNoError;
					break;
				}

				// Lecture caractere suivant, en skippant les carriage return
				assert(not IsBufferEnd());
				c = GetNextChar();

				// OK si fin de champ apres double-quote
				if (c == cFieldSeparator)
				{
					nFieldError = FieldNoError;
					bEndOfLine = false;
					break;
				}

				// OK si fin de ligne apres double-quote
				if (c == '\n')
				{
					nFieldError = FieldNoError;
					bLastFieldReachEol = true;
					break;
				}

				// OK si nouveau double-quote (il a ete correctement double)
				if (c == '"')
				{
					// On memorise ici les deux double-quote
					if (i < (int)nMaxFieldSize)
					{
						sField[i] = '"';
						i++;
					}
					if (i < (int)nMaxFieldSize)
					{
						sField[i] = '"';
						i++;
					}

					// Preparation du caractere suivant
					if (not IsBufferEnd())
						c = GetNextChar();
					continue;
				}
				// KO si on trouve un double-quote isole au milieu du champ
				//  On continue quand-meme a parser pour recuperer l'erreur
				//  On memorise neanmoins de double-quote isole
				else
				{
					assert(c != '"');
					nFieldError = FieldMiddleDoubleQuote;

					// On memorise le caractere double-quote isole
					if (i < (int)nMaxFieldSize)
					{
						sField[i] = '"';
						i++;
					}

					// Si carriage return suivi de fin de fichier ou de ligne, c'est OK
					if (c == '\r')
					{
						// On annule temporairement la prise du double-quote
						if (i < (int)nMaxFieldSize)
							i--;

						// OK si fin du buffer
						if (IsBufferEnd())
						{
							nFieldError = FieldNoError;
							break;
						}

						// Lecture caractere suivant, en skippant le carriage return
						assert(not IsBufferEnd());
						c = GetNextChar();

						// OK si fin de champ apres double-quote
						if (c == cFieldSeparator)
						{
							nFieldError = FieldNoError;
							bEndOfLine = false;
							break;
						}

						// OK si fin de ligne apres double-quote
						if (c == '\n')
						{
							nFieldError = FieldNoError;
							bLastFieldReachEol = true;
							break;
						}

						// On memorise le caractere carriage return, apres avoir restitue le
						// double-quote
						if (i < (int)nMaxFieldSize)
							i++;
						if (i < (int)nMaxFieldSize)
						{
							sField[i] = '\r';
							i++;
						}
					}

					// On recupere l'erreur en parsant jusqu'au prochain separateur de champ, de
					// ligne , ou fin de fichier
					while (bUnconditionalLoop)
					{
						// Arret si fin de ligne
						if (c == '\n')
						{
							bLastFieldReachEol = true;
							break;
						}

						// Arret si fin de champ
						if (c == cFieldSeparator)
						{
							bEndOfLine = false;
							break;
						}

						// Mise a jour du champ si pas depassement de longueur
						if (i < (int)nMaxFieldSize)
						{
							sField[i] = c;
							i++;
						}

						// Caractere suivant
						if (IsBufferEnd())
							break;
						c = GetNextChar();
					}

					// Arret
					break;
				}
			}

			// Arret en erreur si fin de ligne
			if (c == '\n')
			{
				assert(nFieldError != FieldNoError);
				bLastFieldReachEol = true;
				break;
			}

			// Mis a jour du champ si pas depassement de longueur
			if (i < (int)nMaxFieldSize)
			{
				sField[i] = c;
				i++;
			}

			// Caractere suivant
			if (IsBufferEnd())
				break;
			c = GetNextChar();
		}
	}

	// On extrait la partie utile du champ seulement s'il n'y a pas d'erreur
	if (nFieldError == FieldNoError)
	{
		assert(sField[0] == '"');

		// On doit supprimer le premier double-quote, ainsi que toutes les paires de double-quote
		// Le dernier double-quotes n'a pas ete memorise
		iLast = i;
		i = 0;
		iNew = 1;
		while (iNew < iLast)
		{
			sField[i] = sField[iNew];
			if (sField[iNew] == '"')
			{
				iNew++;
				assert(iNew >= (int)nMaxFieldSize or iNew < iLast);
				assert(iNew >= (int)nMaxFieldSize or sField[iNew] == '"');
			}
			i++;
			iNew++;
		}
	}
	return bEndOfLine;
}

boolean InputBufferedFile::SkipDoubleQuoteField()
{
	boolean bUnconditionalLoop = true; // Pour eviter un warning dans la boucle
	char c;

	if (not IsBufferEnd())
	{
		// Lecture du premier caractere: traitement special si double-quote
		c = GetNextChar();

		// Analyse speciale du champs
		while (bUnconditionalLoop)
		{
			// Test si double-quote
			if (c == '"')
			{
				// Arret si fin du buffer
				if (IsBufferEnd())
					return true;

				// Lecture caractere suivant
				assert(not IsBufferEnd());
				c = GetNextChar();

				// OK si fin de champ apres double-quote
				if (c == cFieldSeparator)
					return false;

				// OK si fin de ligne apres double-quote
				if (c == '\n')
				{
					bLastFieldReachEol = true;
					return true;
				}

				// OK si nouveau double-quote (il a ete correctement double)
				if (c == '"')
				{
					// Preparation du caractere suivant
					if (not IsBufferEnd())
						c = GetNextChar();
					continue;
				}
				// KO si on trouve un double-quote isole au milieu du champ
				//  On continue quand-meme a parser pour recuperer l'erreur
				//  On memorise neanmoins de double-quote isole
				else
				{
					// On recupere l'erreur en parsant jusqu'au prochain separateur de champ, de
					// ligne , ou fin de fichier
					while (bUnconditionalLoop)
					{
						// Arret si fin de ligne
						if (c == '\n')
						{
							bLastFieldReachEol = true;
							return true;
						}

						// Arret si fin de champ
						if (c == cFieldSeparator)
							return false;

						// Caractere suivant
						if (IsBufferEnd())
							break;
						c = GetNextChar();
					}

					// Arret
					return true;
				}
			}

			// Arret si fin de ligne
			if (c == '\n')
			{
				bLastFieldReachEol = true;
				return true;
			}

			// Caractere suivant
			if (IsBufferEnd())
				break;
			c = GetNextChar();
		}
	}

	// Retour de fin de ligne (fin de buffer)
	return true;
}

boolean InputBufferedFile::CheckEncoding(const Object* errorSender) const
{
	boolean bOk = true;
	const unsigned char cUTF8Bom[3] = {0xEF, 0xBB, 0xBF};
	const unsigned char cUTF16BigEndianBom[2] = {0xFE, 0xFF};
	const unsigned char cUTF16LittleEndianBom[2] = {0xFF, 0xFE};
	const unsigned char cUTF32BigEndianBom[4] = {0x00, 0x00, 0xFE, 0xFF};
	const unsigned char cUTF32LittleEndianBom[4] = {0xFF, 0xFE, 0x00, 0x00};
	const ALString sMessage = "; try convert the file to ANSI or UTF-8 (without BOM) format";
	int i;
	char c;
	ALString sTmp;

	// Test si encodage UTF8 avec BOM
	if (nCurrentBufferSize >= 3 and (unsigned char) fbBuffer.GetAt(0) == cUTF8Bom[0] and
	    (unsigned char) fbBuffer.GetAt(1) == cUTF8Bom[1] and (unsigned char) fbBuffer.GetAt(2) == cUTF8Bom[2])
	{
		bOk = false;
		if (errorSender != NULL)
			errorSender->AddError(sTmp + "Encoding type cannot be handled (UTF-8 with BOM)" + sMessage);
	}
	// Test si encodage UTF32 big endian
	else if (nCurrentBufferSize >= 4 and (unsigned char) fbBuffer.GetAt(0) == cUTF32BigEndianBom[0] and
		 (unsigned char) fbBuffer.GetAt(1) == cUTF32BigEndianBom[1] and
		 (unsigned char) fbBuffer.GetAt(2) == cUTF32BigEndianBom[2] and
		 (unsigned char) fbBuffer.GetAt(3) == cUTF32BigEndianBom[3])
	{
		bOk = false;
		if (errorSender != NULL)
			errorSender->AddError(sTmp + "Encoding type cannot be handled (UTF-16 big endian)" + sMessage);
	}
	// Test si encodage UTF32 little endian
	else if (nCurrentBufferSize >= 4 and (unsigned char) fbBuffer.GetAt(0) == cUTF32LittleEndianBom[0] and
		 (unsigned char) fbBuffer.GetAt(1) == cUTF32LittleEndianBom[1] and
		 (unsigned char) fbBuffer.GetAt(2) == cUTF32LittleEndianBom[2] and
		 (unsigned char) fbBuffer.GetAt(3) == cUTF32LittleEndianBom[3])
	{
		bOk = false;
		if (errorSender != NULL)
			errorSender->AddError(sTmp + "Encoding type cannot be handled (UTF-16 little endian)" +
					      sMessage);
	}
	// Test si encodage UTF16 big endian
	else if (nCurrentBufferSize >= 2 and (unsigned char) fbBuffer.GetAt(0) == cUTF16BigEndianBom[0] and
		 (unsigned char) fbBuffer.GetAt(1) == cUTF16BigEndianBom[1])
	{
		bOk = false;
		if (errorSender != NULL)
			errorSender->AddError(sTmp + "Encoding type cannot be handled (UTF-16 big endian)" + sMessage);
	}
	// Test si encodage UTF16 little endian
	else if (nCurrentBufferSize >= 2 and (unsigned char) fbBuffer.GetAt(0) == cUTF16LittleEndianBom[0] and
		 (unsigned char) fbBuffer.GetAt(1) == cUTF16LittleEndianBom[1])
	{
		bOk = false;
		if (errorSender != NULL)
			errorSender->AddError(sTmp + "Encoding type cannot be handled (UTF-16 little endian)" +
					      sMessage);
	}

	// Recherche de caracteres NULL
	if (bOk)
	{
		for (i = 0; i < nCurrentBufferSize; i++)
		{
			c = fbBuffer.GetAt(i);
			if (c == '\0')
			{
				bOk = false;
				if (errorSender != NULL)
					errorSender->AddError(
					    sTmp + "Encoding type cannot be handled (null char detected at offset " +
					    IntToString(i + 1) + ")" + sMessage);
				break;
			}
			if (i > 1000)
				break;
		}
	}
	return bOk;
}

longint InputBufferedFile::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = BufferedFile::GetUsedMemory();
	lUsedMemory += sizeof(InputBufferedFile) - sizeof(BufferedFile);
	return lUsedMemory;
}

boolean InputBufferedFile::GetFirstLineFields(const ALString& sInputFileName, char cInputFieldSeparator,
					      StringVector* svFirstLineFields, const Object* errorSender)
{
	boolean bOk;
	InputBufferedFile inputFile;
	ALString sTmp;
	boolean bEndOfLine;
	char* sField;
	int nField;
	int nFieldError;
	boolean bCloseOk;

	require(svFirstLineFields != NULL);

	// Initialisation du vecteur
	svFirstLineFields->SetSize(0);

	// Extraction de la premiere ligne du fichier d'entree en utilisant un fichier bufferise
	inputFile.SetFileName(sInputFileName);
	inputFile.SetFieldSeparator(cInputFieldSeparator);

	// Ouverture du fichier
	bOk = inputFile.Open();
	if (not bOk)
	{
		if (errorSender != NULL)
			errorSender->AddError("Error while opening input file");
		return false;
	}
	if (bOk)
	{
		// Taille de buffer reduite si fichier de petite taille
		// (en veillant a ne pas avoir un buffer de taille nulle)
		if (inputFile.GetFileSize() < BufferedFile::nDefaultBufferSize)
			inputFile.SetBufferSize((int)inputFile.GetFileSize());
		else
			inputFile.SetBufferSize(BufferedFile::nDefaultBufferSize);

		// Lecture de la premiere ligne
		bOk = inputFile.FillWithHeaderLine();
		if (not bOk)
		{
			if (errorSender != NULL)
				errorSender->AddError(sTmp + "First line too long");
		}
	}

	// Test si caracteres speciaux
	// Erreur si du caractere NULL
	if (bOk)
		bOk = inputFile.CheckEncoding(errorSender);

	// Recherche des champs de la premiere ligne
	if (bOk)
	{
		Global::ActivateErrorFlowControl();
		bEndOfLine = false;
		nField = 0;
		while (not bEndOfLine)
		{
			bEndOfLine = inputFile.GetNextField(sField, nFieldError);
			nField++;

			// Erreur sur le nom du champ
			if (nFieldError != inputFile.FieldNoError)
			{
				if (errorSender != NULL)
					errorSender->AddWarning(sTmp + "First line field " + IntToString(nField) +
								" <" + InputBufferedFile::GetDisplayValue(sField) +
								"> : " + inputFile.GetFieldErrorLabel(nFieldError));
			}
			svFirstLineFields->Add(sField);
		}
		Global::DesactivateErrorFlowControl();
	}

	// Fermeture du fichier d'entree
	if (inputFile.IsOpened())
	{
		bCloseOk = inputFile.Close();
		if (not bCloseOk)
		{
			if (errorSender != NULL)
				errorSender->AddError("Error while closing input file");
			bOk = false;
		}
	}

	// Nettoyage si neessaire
	if (not bOk)
		svFirstLineFields->SetSize(0);
	return bOk;
}

ALString InputBufferedFile::GetDisplayValue(const ALString& sValue)
{
	const int nMaxSize = 20;
	if (sValue.GetLength() <= nMaxSize + 3)
		return sValue;
	else
		return sValue.Left(nMaxSize) + "...";
}

boolean InputBufferedFile::TestCountLines(const ALString& sFileName, int nFileType)
{
	InputBufferedFile inputFile;
	char* sField;
	int nFieldError;
	longint lFileSize;
	int nChunkSize;
	longint lLineNumber;
	longint lLinesCountNumberWithSkip;
	longint lLinesCountWithNextField;
	boolean bEol;
	longint lBeginPos;
	int nBufferNumber;
	boolean bOk;
	ALString sTmp;
	ALString sRemoteFileName;

	// Nombre de lignes de reference
	lLineNumber = FileBuffer::TestCountLines(sFileName, true);

	switch (nFileType)
	{
	case 0:
		sRemoteFileName = sFileName;
		break;
	case 1:
		sRemoteFileName = sTmp + FileService::sRemoteScheme + "://" + GetLocalHostName() + sFileName;
		break;
	case 2:
		sRemoteFileName = sFileName;
		break;
	default:
		assert(false);
	}

	lFileSize = PLRemoteFileService::GetFileSize(sFileName);

	// Ligne d'entete sur les stats de lecture du fichier
	cout << "File name\t" << sRemoteFileName << "\tSize\t" << lFileSize << "\tLines\t" << lLineNumber << endl
	     << endl;
	cout << "Buffer Size"
	     << "\t"
	     << "Buffer Number"
	     << "\t"
	     << "Line number"
	     << "\t"
	     << "Computed line 1"
	     << "\t"
	     << "Computed line 2" << endl;

	// Boucle sur des buffers de taille decroissante
	nChunkSize = 128 * lMB;
	inputFile.SetFileName(sRemoteFileName);
	while (nChunkSize >= 64 * lKB)
	{
		// Initialisations
		nBufferNumber = 0;
		lLinesCountNumberWithSkip = 0;
		lLinesCountWithNextField = 0;

		//////////////////////////////////////
		// comptage des lignes et des champs en utilisant Fill

		inputFile.Open();
		lBeginPos = 0;
		if (inputFile.IsOpened())
		{

			while (lBeginPos < inputFile.GetFileSize())
			{
				// Lecture d'un buffer
				inputFile.SetBufferSize(nChunkSize);
				bOk = inputFile.Fill(lBeginPos);

				if (not bOk)
					Global::AddError("", "", FileService::GetLastSystemIOErrorMessage());

				lBeginPos += nChunkSize;
				nBufferNumber++;
				if (inputFile.GetBufferSkippedLine())
				{
					lLinesCountWithNextField++;
				}

				// Analyse du buffer
				while (bOk and not inputFile.IsBufferEnd())
				{
					bEol = false;
					while (not bEol)
					{
						bEol = inputFile.GetNextField(sField, nFieldError);
					}
					lLinesCountWithNextField++;
				}
			}

			// Fermeture
			inputFile.Close();
		}

		///////////////////////////////////////
		// Comptage des lignes avec skipline

		inputFile.SetFileName(sRemoteFileName);
		bOk = inputFile.Open();
		lLinesCountNumberWithSkip = 0;
		if (bOk)
		{
			lBeginPos = 0;
			while (lBeginPos < inputFile.GetFileSize())
			{
				// Lecture d'un buffer
				inputFile.SetBufferSize(nChunkSize);
				bOk = inputFile.Fill(lBeginPos);
				if (not bOk)
					Global::AddError("", "", FileService::GetLastSystemIOErrorMessage());
				lBeginPos += nChunkSize;
				if (inputFile.GetBufferSkippedLine())
				{
					lLinesCountNumberWithSkip++;
				}

				// Analyse du buffer
				while (bOk and not inputFile.IsBufferEnd())
				{
					inputFile.SkipLine();
					lLinesCountNumberWithSkip++;
				}
			}
			// Fermeture
			inputFile.Close();
		}

		// Affichage des stats d'analyse du fichier
		if (bOk)
			cout << LongintToHumanReadableString(nChunkSize) << "\t" << nBufferNumber << "\t" << lLineNumber
			     << "\t" << lLinesCountNumberWithSkip << "\t" << lLinesCountWithNextField << endl;
		else
			cout << "ERROR" << endl;

		if (lLineNumber != lLinesCountNumberWithSkip)
		{
			Global::AddError("", "", "Error whith SkipLine method");
			return false;
		}
		if (lLineNumber != lLinesCountWithNextField)
		{
			Global::AddError("", "", "Error whith GetNextField method");
			return false;
		}

		// Diminution de la taille du buffer
		nChunkSize /= 2;
	}
	return true;
}

boolean InputBufferedFile::Test(int nFileType)
{
	FileBuffer fb;
	ALString sTmpDir;
	int i;
	int nFieldsNumber;
	int nBufferSize;
	boolean bOk;

	assert(nFileType >= 0 and nFileType <= 2);

	// Positionnement de la graine aleatoire pour avoir un test reproductible
	SetRandomSeed(0);

	// Acces au repertorie temporaire
	if (nFileType == 2)
		sTmpDir = "hdfs:///tmp";
	else
		sTmpDir = FileService::GetTmpDir();
	if (sTmpDir.IsEmpty())
	{
		cout << "Temporary directory not found" << endl;
		return false;
	}
	cout << "SYS Temporary directory " << sTmpDir << endl;

	// Fichier vide
	bOk = TestWriteBuffer(sTmpDir, "empty_file.txt", "empty file", &fb, nFileType);
	if (not bOk)
	{
		cout << "Error in test" << endl;
		return false;
	}

	// Gros fichier avec une seule ligne et plusieurs champs - 8 MB
	nBufferSize = 8 * lMB;
	while (fb.GetSize() < nBufferSize)
	{
		if (fb.GetSize() > 0)
			fb.Add('\t');
		AddCharsInFileBuffer(&fb, 'f', RandomInt(8));
	}
	bOk = TestWriteBuffer(sTmpDir, "one_line_8MB_file.txt", "one line big file (8 MB)", &fb, nFileType);
	if (not bOk)
	{
		cout << "Error in test" << endl;
		return false;
	}

	// Gros fichier avec un seul champ et une seule ligne - 1 MB
	nBufferSize = lMB;
	AddCharsInFileBuffer(&fb, 'f', nBufferSize);
	bOk = TestWriteBuffer(sTmpDir, "one_line_one_field_1MB_file.txt", "one line one field file (1 MB)", &fb,
			      nFileType);
	if (not bOk)
	{
		cout << "Error in test" << endl;
		return false;
	}

	// Gros fichier avec un seul champ et une seule ligne - 8 MB
	nBufferSize = 8 * lMB;
	AddCharsInFileBuffer(&fb, 'f', nBufferSize);
	bOk = TestWriteBuffer(sTmpDir, "one_line_one_field_8MB_file.txt", "one line one field big file (8 MB)", &fb,
			      nFileType);
	if (not bOk)
	{
		cout << "Error in test" << endl;
		return false;
	}

	// Fichier avec un seul caractere: eol
	fb.Add('\n');
	bOk = TestWriteBuffer(sTmpDir, "one_char_eol_file.txt", "file whith only eol", &fb, nFileType);
	if (not bOk)
	{
		cout << "Error in test" << endl;
		return false;
	}

	// Fichier avec un seul caractere: tab
	fb.Add('\t');
	bOk = TestWriteBuffer(sTmpDir, "one_char_tab_file.txt", "file whith only tab", &fb, nFileType);
	if (not bOk)
	{
		cout << "Error in test" << endl;
		return false;
	}

	// Fichier de 1 MB avec 10 champs par ligne
	nBufferSize = lMB;
	nFieldsNumber = 10;
	while (fb.GetSize() < nBufferSize)
	{
		for (i = 0; i < nFieldsNumber; i++)
		{
			if (i > 0)
				fb.Add('\t');
			AddCharsInFileBuffer(&fb, 'f', RandomInt(8));
		}
		fb.Add('\n');
	}
	bOk = TestWriteBuffer(sTmpDir, "1MB_10fields_file.txt", "1 MB file whith 10 fields per line", &fb, nFileType);
	if (not bOk)
	{
		cout << "Error in test" << endl;
		return false;
	}

	// Fichier de 8 MB avec 10 champs par ligne
	nBufferSize = 8 * lMB;
	nFieldsNumber = 10;
	while (fb.GetSize() < nBufferSize)
	{
		for (i = 0; i < nFieldsNumber; i++)
		{
			if (i > 0)
				fb.Add('\t');
			AddCharsInFileBuffer(&fb, 'f', RandomInt(8));
		}
		fb.Add('\n');
	}
	bOk = TestWriteBuffer(sTmpDir, "8MB_10fields_file.txt", "8 MB file whith 10 fields per line", &fb, nFileType);
	if (not bOk)
	{
		cout << "Error in test" << endl;
		return false;
	}

	// Fichier avec 100 lignes de 128 o
	for (i = 0; i < 100; i++)
	{
		AddCharsInFileBuffer(&fb, 'f', 128);
		fb.Add('\n');
	}

	bOk = TestWriteBuffer(sTmpDir, "100lines_file.txt", "file whith 100 lines of 128 o", &fb, nFileType);
	if (not bOk)
	{
		cout << "Error in test" << endl;
		return false;
	}

	// Fichier avec 100 lignes de 128 B dont une ligne de 10 MB au debut au milieu et a la fin
	AddCharsInFileBuffer(&fb, 'f', 10 * lMB);
	fb.Add('\n');

	for (i = 1; i < 99; i++)
	{
		if (i == 50)
			AddCharsInFileBuffer(&fb, 'f', 10 * lMB);
		else
			AddCharsInFileBuffer(&fb, 'f', RandomInt(128));
		fb.Add('\n');
	}
	AddCharsInFileBuffer(&fb, 'f', 10 * lMB);
	fb.Add('\n');

	bOk = TestWriteBuffer(sTmpDir, "100lines_3bigLines_file.txt", "file whith 97 lines of 128 B and 3 big lines",
			      &fb, nFileType);
	if (not bOk)
	{
		cout << "Error in test" << endl;
		return false;
	}

	// Fichier avec 100 000 lignes de 128 B dont une ligne de 20 MB au debut au milieu et a la fin
	AddCharsInFileBuffer(&fb, 'f', 20 * lMB);
	fb.Add('\n');

	for (i = 1; i < 99999; i++)
	{
		if (i == 50)
			AddCharsInFileBuffer(&fb, 'f', 20 * lMB);
		else
			AddCharsInFileBuffer(&fb, 'f', RandomInt(128));
		fb.Add('\n');
	}
	AddCharsInFileBuffer(&fb, 'f', 20 * lMB);
	fb.Add('\n');

	bOk = TestWriteBuffer(sTmpDir, "99997lines_3hugeLines_file.txt",
			      "file whith 99997 lines of 128 B and 3 huge lines", &fb, nFileType);
	if (not bOk)
	{
		cout << "Error in test" << endl;
		return false;
	}

	// Fin des tests
	cout << "All tests done" << endl;
	return true;
}

boolean InputBufferedFile::TestCount(const ALString& sFileName, int nChunkSize)
{
	InputBufferedFile ibFile;
	longint lFilePos;
	boolean bOk;
	int nLineNumber;
	ALString sTmp;

	lFilePos = 0;
	nLineNumber = 0;
	// ibFile.SetBufferSize(32 * lKB);
	ibFile.SetFileName(sFileName);
	bOk = ibFile.Open();
	if (bOk)
	{
		while (lFilePos < ibFile.GetFileSize())
		{
			ibFile.SetBufferSize(nChunkSize);
			bOk = ibFile.Fill(lFilePos);
			nLineNumber += ibFile.GetBufferLineNumber();
			if (not bOk)
				break;
			if (ibFile.GetBufferSkippedLine())
			{
				ibFile.AddWarning(sTmp + "line " + LongintToReadableString(nLineNumber + 1) +
						  " is skipped");
				nLineNumber++;
			}
			lFilePos += nChunkSize;
		}

		ibFile.Close();
	}
	if (bOk)
		cout << "line number " << LongintToReadableString(nLineNumber) << endl;
	else
	{
		cout << "------------------- BUG ---------------------------" << endl;
	}

	return bOk;
}

boolean InputBufferedFile::FillWithHeaderLine()
{
	char c;
	boolean bLineTooLong;
	longint lCurrentFileSize;
	boolean bOk = true;
	int nSmallBufferSize;
	char* cBuffer;
	longint lTotalRead;
	longint lRead;
	boolean bEolFound;
	int i;

	require(IsOpened());

	if (GetFileDriverCreator()->IsLocal(this->sFileName))
	{
		// Re-initialisation de InputBuffer
		// On memorise le FileSize, qui sera reinitialise par le Reset()
		lCurrentFileSize = lFileSize;
		Reset();
		lFileSize = lCurrentFileSize;

		// Reset du charVector sous-jacent
		ResetBuffer();

		// Demande d'acces au disque
		if (FileBuffer::GetRequestIOFunction() != NULL)
			FileBuffer::fRequestIOFunction(0);

		// On se positionne au debut dans le fichier
		bIsError = not fileHandle->SeekPositionInFile(0);
		if (bIsError)
			return false;

		// Allocation d'un buffer de lecture
		nSmallBufferSize = MemSegmentByteSize;
		if (GetFileSize() < nSmallBufferSize)
			nSmallBufferSize = (int)GetFileSize();
		cBuffer = NewCharArray(nSmallBufferSize);

		// Boucle de recherche
		lTotalRead = 0;
		bEolFound = false;
		bLineTooLong = false;
		while (lTotalRead < GetFileSize() and bOk and not bEolFound)
		{
			// Reduction si necessaire du nombre de caracteres a lire
			if (nSmallBufferSize > GetFileSize() - lTotalRead)
				nSmallBufferSize = (int)(GetFileSize() - lTotalRead);

			// Lecture du buffer
			lRead = fileHandle->Read(cBuffer, sizeof(char), nSmallBufferSize);
			lTotalRead += lRead;
			bOk = lRead != 0;
			bEof = lTotalRead == GetFileSize();

			// Analyse du buffer
			if (bOk)
			{
				i = 0;
				while (i < lRead)
				{
					c = cBuffer[i];
					i++;

					// Ajout du caractere
					fbBuffer.Add(c);
					nCurrentBufferSize++;

					// Test si depassement de la longueur maximale des lignes
					if (nCurrentBufferSize > nMaxLineLength)
					{
						bLineTooLong = true;
						break;
					}

					// Arret si on a trouve la fin de ligne
					if (c == '\n')
					{
						bEolFound = true;
						break;
					}
				}
			}
		}
		bIsError = not bOk;

		// Desallocation du buffer de lecture
		DeleteCharArray(cBuffer);

		// Relachement de l'acces au disque
		if (FileBuffer::GetReleaseIOFunction() != NULL)
			FileBuffer::fReleaseIOFunction(0);
	}
	else
	{
		assert(fileDriver != NULL);
		bLineTooLong = not fileDriver->FillWithHeaderLine(this);
	}

	return not bLineTooLong;
}

boolean InputBufferedFile::TestWriteBuffer(const ALString& sTmpDir, const ALString& sFileName, const ALString& sLabel,
					   FileBuffer* fileBuffer, int nFileType)
{
	ALString sFullFileName;
	boolean bOk = false;
	InputBufferedFile errorSender;
	SystemFile* fileHandle;

	require(sFileName != "");
	require(fileBuffer != NULL);

	// Nom complet du fichier
	sFullFileName = FileService::BuildFilePathName(sTmpDir, sFileName);
	cout << endl << "Generating " << sLabel << " " << sFileName << endl;

	// Ecriture du fichier
	fileHandle = new SystemFile;
	bOk = fileHandle->OpenOutputFile(sFullFileName);
	if (bOk)
	{
		bOk = fileBuffer->WriteToFile(fileHandle, fileBuffer->GetSize(), &errorSender);
		if (not bOk)
			cout << "ERROR while writing " << sFullFileName << endl;
		fileHandle->CloseOutputFile(sFullFileName);
	}
	else
		cout << "ERROR while opening " << sFullFileName << " for write" << endl;

	delete fileHandle;
	fileBuffer->SetSize(0);
	if (bOk)
	{
		// Test sur le fichier
		cout << "Test count lines for " << sFileName << endl;
		bOk = TestCountLines(sFullFileName, nFileType);
	}

	PLRemoteFileService::RemoveFile(sFullFileName);
	return bOk;
}

void InputBufferedFile::AddCharsInFileBuffer(FileBuffer* fileBuffer, char c, int nNumber)
{
	int i;

	require(fileBuffer != NULL);
	require(nNumber >= 0);

	for (i = 0; i < nNumber; i++)
		fileBuffer->Add(c);
}

void InputBufferedFile::Reset()
{
	bEof = false;
	nPositionInBuffer = 0;
	nCurrentBufferSize = 0;
	bIsHeadOfFile = false;
	nReadLineNumber = 0;
	lBufferBeginPos = -1;
	bLastFieldReachEol = false;
	nBufferLineNumber = 0;
	lFileSize = -1;
	bIsSkippedLine = false;
	lNextFilePos = 0;
	nAllocatedBufferSize = 0;
}

boolean InputBufferedFile::FindBol(longint lBeginPos, int nChunkSize, longint lFileSize, SystemFile* fileHandle,
				   int& nBeginLinePos, boolean& bBolFound)
{
	char c;
	boolean bOk = true;
	boolean bEolFound;
	char* cBuffer;
	int nSmallBufferSize = 64 * lKB;
	longint lTotalRead;
	longint lRead;
	int i;

	bEolFound = false;
	nBeginLinePos = 0;

	if (lBeginPos == 0)
	{
		bBolFound = true;
		bOk = fileHandle->SeekPositionInFile(0);
	}
	else if (lBeginPos >= lFileSize)
	{
		bBolFound = false;
		bOk = true;
	}
	else
	{
		// On se positionne dans le fichier juste avant le debut du buffer
		// au cas ou le debut de ligne est au debut du buffer
		assert(lBeginPos >= 1);
		bOk = fileHandle->SeekPositionInFile(lBeginPos - 1);
		if (bOk)
		{
			// Parcours du fichier
			// On boucle jusqu'a nChunkSize + 1 car on commence a chercher a BeginPos - 1
			if (nSmallBufferSize > nChunkSize)
				nSmallBufferSize = nChunkSize;
			assert(InputBufferedFile::nHugeBufferSize >= nSmallBufferSize);
			cBuffer = InputBufferedFileGetHugeBuffer();
			lTotalRead = 0;

			lRead = nSmallBufferSize;
			while (lRead == nSmallBufferSize and bOk and not bEolFound and nBeginLinePos < nChunkSize + 1)
			{
				lRead = fileHandle->Read(cBuffer, sizeof(char), nSmallBufferSize);
				lTotalRead += lRead;
				bOk = lRead != -1;
				if (bOk)
				{
					i = 0;
					while (nBeginLinePos < nChunkSize + 1 and i < lRead)
					{
						c = cBuffer[i];
						if (c == '\n')
						{
							bEolFound = true;
							break;
						}
						i++;
						nBeginLinePos++;
					}
				}
			}

			// Si on a trouve une EOL et qu'elle n'est pas sur une EOF ou une fin de chunk
			// On positionne le debut de ligne BOL
			if (bEolFound and lBeginPos + nBeginLinePos < lFileSize and // EOF
			    nBeginLinePos <= nChunkSize)                            // Fin de chunk
			{
				bBolFound = true;
				fileHandle->SeekPositionInFile(lBeginPos + nBeginLinePos);
			}
		}
	}
	return bOk;
}
