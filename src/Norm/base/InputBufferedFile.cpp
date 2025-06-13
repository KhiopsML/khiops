// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "InputBufferedFile.h"
#include "SystemFileDriver.h"
#include "HugeBuffer.h"

const unsigned char InputBufferedFile::cUTF8Bom[nUTF8BomSize] = {0xEF, 0xBB, 0xBF};
int InputBufferedFile::nMaxLineLength = 8 * lMB;

InputBufferedFile::InputBufferedFile()
{
	Reset();
	bUTF8BomManagement = true;
	bCacheOn = true;
	lTotalPhysicalReadCalls = 0;
	lTotalPhysicalReadBytes = 0;
}

void InputBufferedFile::CopyFrom(const InputBufferedFile* bufferedFile)
{
	require(not IsOpened());

	// Copie Standard
	BufferedFile::CopyFrom(bufferedFile);

	// Reinitialisation des attributs supplementaires
	Reset();
	bUTF8BomManagement = bufferedFile->bUTF8BomManagement;
	bCacheOn = bufferedFile->bCacheOn;
}

InputBufferedFile::~InputBufferedFile(void)
{
	assert(not bIsOpened);
}

boolean InputBufferedFile::Open()
{
	boolean bOk;

	require(GetFileName() != "");
	require(not IsOpened());
	require(fileHandle == NULL);

	// Reinitialisation des donnees de travail
	Reset();
	lTotalPhysicalReadCalls = 0;
	lTotalPhysicalReadBytes = 0;
	bOk = true;
	fileHandle = new SystemFile;

	// Si c'est un fichier remote sur le localhost, on extrait le path pour le traiter en  ficher local
	if (PLRemoteFileService::RemoteIsLocal(sFileName))
		sLocalFileName = FileService::GetURIFilePathName(sFileName);
	else
		sLocalFileName = sFileName;

	// Ouverture du fichier
	if (not GetOpenOnDemandMode())
	{
		bOk = fileHandle->OpenInputFile(sLocalFileName);
		if (not bOk)
			AddError("Unable to open file (" + fileHandle->GetLastErrorMessage() + ")");
	}

	// On calcul la taille du fichier
	if (bOk)
	{
		lFileSize = fileHandle->GetFileSize(sLocalFileName);
		if (lFileSize == -1)
		{
			bOk = false;
			AddError("Unable to open get file size (" + fileHandle->GetLastErrorMessage() + ")");
		}
	}

	if (not bOk and fileHandle != NULL)
	{
		delete fileHandle;
		fileHandle = NULL;
	}

	// Fin des initialisations
	bIsOpened = bOk;
	bIsError = not bOk;
	if (bOk)
		lCacheStartInFile = 0;
	return bOk;
}

boolean InputBufferedFile::Close()
{
	boolean bOk = true;

	require(IsOpened());

	if (not GetOpenOnDemandMode())
	{
		bOk = fileHandle->CloseInputFile(sLocalFileName);
		if (not bOk)
			AddError("Unable to close file (" + fileHandle->GetLastErrorMessage() + ")");
	}
	delete fileHandle;
	fileHandle = NULL;

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

boolean InputBufferedFile::SearchNextLine(longint& lBeginPos, longint& lNextLinePos)
{
	return SearchNextLineUntil(lBeginPos, GetFileSize(), lNextLinePos);
}

boolean InputBufferedFile::FillOneLine(longint& lBeginPos, boolean& bLineTooLong)
{
	boolean bOk = true;
	longint lSearchPos;
	longint lMaxEndPos;
	longint lNextLinePos;
	int nCacheSearchPos;
	int nCacheMaxEndPos;
	int nSizeToFill;

	require(0 <= lBeginPos and lBeginPos <= GetFileSize());
	require(not(lBeginPos >= lCacheStartInFile + 1 and lBeginPos <= lCacheStartInFile + nCacheSize) or
		fcCache.GetAt(int(lBeginPos - lCacheStartInFile - 1)) == '\n' or IsFirstPositionInFile());

	// Initialisation
	nBufferStartInCache = 0;
	nPositionInCache = 0;
	nLastBolPositionInCache = nPositionInCache;
	nCurrentBufferSize = 0;
	nBufferLineNumber = 0;
	nCurrentLineIndex = 1;
	bLastFieldReachEol = false;
	lNextLinePos = -1;
	bLineTooLong = false;
	nUTF8BomSkippedCharNumber = 0;

	// Calcul de la position max pour la recherche de la ligne
	lMaxEndPos = min(lBeginPos + GetMaxLineLength(), GetFileSize());

	// Arret immediat si erreur
	if (bIsError)
		bOk = false;
	// Cas general
	else
	{
		// Recherche jusqu'a la position max
		lSearchPos = lBeginPos;
		nSizeToFill = 0;
		while (bOk and lSearchPos < lMaxEndPos)
		{
			// Cas ou la position recherchee est dans le cache
			if (lSearchPos >= lCacheStartInFile and lSearchPos < lCacheStartInFile + nCacheSize)
			{
				// Position de depart de la recherche dans le cache
				nCacheSearchPos = int(lSearchPos - lCacheStartInFile);

				// Position max de recherche dans le cache
				nCacheMaxEndPos = nCacheSize;
				if (lMaxEndPos < lCacheStartInFile + nCacheMaxEndPos)
					nCacheMaxEndPos = int(lMaxEndPos - lCacheStartInFile);

				// Recherche d'une fin de ligne dans le cache
				while (nCacheSearchPos < nCacheMaxEndPos)
				{
					if (fcCache.GetAt(nCacheSearchPos) == '\n')
					{
						lNextLinePos = lCacheStartInFile + nCacheSearchPos + 1;
						break;
					}
					nCacheSearchPos++;
				}

				// Arret de la recherche si position trouvee
				if (lNextLinePos != -1)
					break;

				// Mise a jour de la position de recherche pour la suite
				lSearchPos = lCacheStartInFile + nCacheSearchPos;

				// Preparation de la taille minimum a remplir
				assert(lSearchPos - lBeginPos <= GetMaxLineLength());
				nSizeToFill = int(lSearchPos - lBeginPos);
			}

			// Nouvelle lecture dans le fichier si necessaire
			assert(lNextLinePos == -1);
			if (lSearchPos < lMaxEndPos)
			{
				assert(lSearchPos >= lCacheStartInFile + nCacheSize);
				assert(lCacheStartInFile + nCacheSize - lBeginPos < GetMaxLineLength());
				assert(nSizeToFill < GetMaxLineLength());

				// Calcul de la taille a lire
				nSizeToFill += min(GetBufferSize(), GetPreferredBufferSize());
				nSizeToFill = min(nSizeToFill, GetMaxLineLength());
				if (lBeginPos + nSizeToFill > GetFileSize())
					nSizeToFill = int(GetFileSize() - lBeginPos);

				// Remplissage du buffer a partir du debut, avec une taille plus grande
				bOk = InternalFillBytes(lBeginPos, nSizeToFill);
				assert(bOk or bIsError);
			}
			// Sinon, arret de la recherche
			else
				break;
		}
	}

	// Finalisation
	if (bOk)
	{
		// On positionne le debut du buffer sur le debut de la ligne
		assert(nPositionInCache == nBufferStartInCache);
		assert(lBeginPos >= lCacheStartInFile and lBeginPos <= lCacheStartInFile + nCacheSize);
		nBufferStartInCache = int(lBeginPos - lCacheStartInFile);
		nPositionInCache = nBufferStartInCache;
		nLastBolPositionInCache = nPositionInCache;

		// Cas particulier de la fin du fichier atteint sans '\n' en dernier caractere:
		// il s'agit quand meme d'une fin de ligne
		if (lNextLinePos == -1 and lMaxEndPos == GetFileSize() and
		    lCacheStartInFile + nCacheSize == GetFileSize())
			lNextLinePos = GetFileSize();

		// Ajustement de la taille du buffer sur la ligne trouvee
		if (lNextLinePos != -1)
		{
			assert(lNextLinePos >= lCacheStartInFile and lNextLinePos <= lCacheStartInFile + nCacheSize);
			assert(lNextLinePos <= GetFileSize());
			assert(lBeginPos <= lNextLinePos);
			assert(lBeginPos < lNextLinePos or lBeginPos == GetFileSize());
			assert(lNextLinePos - lBeginPos <= GetMaxLineLength());
			assert(not bLineTooLong);
			nCurrentBufferSize = int(lNextLinePos - lBeginPos);
		}
		// Sinon, on a pas trouve la ligne
		else
		{
			bLineTooLong = true;
			nCurrentBufferSize = 0;
		}
	}
	else
		nCurrentBufferSize = 0;

	// Verification en sortie
	ensure(bOk == not bIsError);
	ensure(not bOk or nBufferStartInCache == int(lBeginPos - lCacheStartInFile));
	ensure(not bOk or not bLineTooLong or nCurrentBufferSize == 0);
	ensure(not bOk or bLineTooLong or nBufferStartInCache + nCurrentBufferSize <= nCacheSize);
	ensure(not bOk or bLineTooLong or (nCurrentBufferSize == 0 and lBeginPos == GetFileSize()) or
	       (nCurrentBufferSize > 0 and nCurrentBufferSize <= GetMaxLineLength() and
		(lCacheStartInFile + nBufferStartInCache + nCurrentBufferSize == GetFileSize() or
		 fcCache.GetAt(nBufferStartInCache + nCurrentBufferSize - 1) == '\n')));
	return bOk;
}

boolean InputBufferedFile::FillInnerLines(longint& lBeginPos)
{
	return FillInnerLinesUntil(lBeginPos, GetFileSize());
}

boolean InputBufferedFile::FillOuterLines(longint& lBeginPos, boolean& bLineTooLong)
{
	return FillOuterLinesUntil(lBeginPos, GetFileSize(), bLineTooLong);
}

boolean InputBufferedFile::SearchNextLineUntil(longint& lBeginPos, longint lMaxEndPos, longint& lNextLinePos)
{
	boolean bOk = true;
	longint lSearchPos;
	int nCacheSearchPos;
	int nCacheMaxEndPos;
	int nSizeToFill;

	require(0 <= lBeginPos and lBeginPos <= GetFileSize());
	require(lBeginPos <= lMaxEndPos and lMaxEndPos <= GetFileSize());
	require(GetBufferSize() > 0 or lBeginPos == lMaxEndPos);

	// Initialisation
	nBufferStartInCache = 0;
	nPositionInCache = 0;
	nLastBolPositionInCache = nPositionInCache;
	nCurrentBufferSize = 0;
	bLastFieldReachEol = false;
	nBufferLineNumber = 0;
	nCurrentLineIndex = 1;

	// Arret immediat si erreur
	if (bIsError)
	{
		bOk = false;
		lNextLinePos = -1;
	}
	// Cas d'un zone de recherche vide
	else if (lBeginPos == lMaxEndPos)
	{
		lNextLinePos = -1;

		// On rempit une zone vide pour se positionner en lBeginPos
		InternalFillBytes(lBeginPos, 0);
	}
	// Cas general
	else
	{
		// Recherche jusqu'a la position max
		lNextLinePos = -1;
		lSearchPos = lBeginPos;
		while (bOk and lSearchPos < lMaxEndPos)
		{
			// Cas ou la position recherchee est dans le cache
			if (lSearchPos >= lCacheStartInFile and lSearchPos < lCacheStartInFile + nCacheSize)
			{
				// Position de depart de la recherche dans le cache
				nCacheSearchPos = int(lSearchPos - lCacheStartInFile);

				// Position max de recherche dans le cache
				nCacheMaxEndPos = nCacheSize;
				if (lMaxEndPos < lCacheStartInFile + nCacheMaxEndPos)
					nCacheMaxEndPos = int(lMaxEndPos - lCacheStartInFile);

				// Recherche d'une fin de ligne dans le cache
				while (nCacheSearchPos < nCacheMaxEndPos)
				{
					if (fcCache.GetAt(nCacheSearchPos) == '\n')
					{
						lNextLinePos = lCacheStartInFile + nCacheSearchPos + 1;
						break;
					}
					nCacheSearchPos++;
				}

				// Arret de la recherche si position trouvee
				if (lNextLinePos != -1)
					break;

				// Mise a jour de la position de recherche pour la suite
				lSearchPos = lCacheStartInFile + nCacheSearchPos;
			}

			// Nouvelle lecture dans le fichier si necessaire
			assert(lNextLinePos == -1);
			if (lSearchPos < lMaxEndPos)
			{
				assert(lSearchPos < lCacheStartInFile or lSearchPos >= lCacheStartInFile + nCacheSize);

				// Calcul de la taille a lire
				nSizeToFill = min(GetBufferSize(), GetPreferredBufferSize());
				if (lMaxEndPos - lSearchPos < nSizeToFill)
					nSizeToFill = int(lMaxEndPos - lSearchPos);

				// Remplissage du buffer
				bOk = InternalFillBytes(lSearchPos, nSizeToFill);
				assert(bOk or bIsError);
			}
			// Sinon, arret de la recherche
			else
				break;
		}
	}

	// Cas particulier de la fin du fichier atteint sans '\n' en dernier caractere:
	// il s'agit quand meme d'une fin de ligne
	if (bOk and lNextLinePos == -1 and lMaxEndPos == GetFileSize() and
	    lCacheStartInFile + nCacheSize == GetFileSize())
		lNextLinePos = GetFileSize();

	// Finalisation en mettant le buffer courant a une taille vide
	nCurrentBufferSize = 0;
	if (bOk and lNextLinePos != -1)
	{
		// Modification de la position dans le cache
		nBufferStartInCache = int(lNextLinePos - lCacheStartInFile);
		nPositionInCache = nBufferStartInCache;
		nLastBolPositionInCache = nPositionInCache;
	}

	// Verification en sortie
	ensure(bOk == not bIsError);
	ensure(nCurrentBufferSize == 0);
	ensure(not bOk or lNextLinePos == -1 or
	       (lNextLinePos >= lCacheStartInFile + 1 and lNextLinePos <= lCacheStartInFile + nCacheSize));
	ensure(not bOk or lNextLinePos != -1 or (lCacheStartInFile + nCacheSize >= lMaxEndPos));
	ensure(not bOk or lNextLinePos == -1 or (0 <= nBufferStartInCache and nBufferStartInCache <= nCacheSize));
	ensure(not bOk or lNextLinePos == -1 or lNextLinePos == GetBufferStartInFile());
	ensure(not bOk or lNextLinePos == -1 or lNextLinePos == GetPositionInFile());
	ensure(not bOk or lNextLinePos == -1 or
	       (lCacheStartInFile + nBufferStartInCache + nCurrentBufferSize == GetFileSize()) or
	       (nBufferStartInCache > 0 and fcCache.GetAt(nBufferStartInCache - 1) == '\n'));
	return bOk;
}

boolean InputBufferedFile::FillInnerLinesUntil(longint& lBeginPos, longint lMaxEndPos)
{
	boolean bOk;
	int nSizeToFill;

	require(0 <= lBeginPos and lBeginPos <= GetFileSize());
	require(lBeginPos <= lMaxEndPos and lMaxEndPos <= GetFileSize());
	require(GetBufferSize() > 0 or lBeginPos == lMaxEndPos);
	require(not(lBeginPos >= lCacheStartInFile + 1 and lBeginPos <= lCacheStartInFile + nCacheSize) or
		fcCache.GetAt(int(lBeginPos - lCacheStartInFile - 1)) == '\n' or IsFirstPositionInFile());

	// Calcul de la taille a lire
	nSizeToFill = GetBufferSize();
	if (lMaxEndPos < lBeginPos + nSizeToFill)
		nSizeToFill = int(lMaxEndPos - lBeginPos);

	// Remplissage d'un buffer
	bOk = InternalFillBytes(lBeginPos, nSizeToFill);

	// Si on arrive pas a la fin du fichier, on retaille le buffer pour n'avoir que des lignes entieres
	if (bOk and GetBufferStartInFile() + nCurrentBufferSize < GetFileSize())
	{
		// Sinon on recherche en arriere a partir de la fin du buffer
		while (nCurrentBufferSize > 0)
		{
			if (fcCache.GetAt(nBufferStartInCache + nCurrentBufferSize - 1) == '\n')
				break;
			nCurrentBufferSize--;
		}
	}

	// Verification en sortie
	ensure(bOk == not bIsError);
	ensure(not bOk or lBeginPos == GetBufferStartInFile());
	ensure(not bOk or lBeginPos == GetPositionInFile());
	ensure(not bOk or nCurrentBufferSize == 0 or lBeginPos + nCurrentBufferSize == GetFileSize() or
	       fcCache.GetAt(nBufferStartInCache + nCurrentBufferSize - 1) == '\n');
	ensure(not bOk or nPositionInCache == nBufferStartInCache);
	ensure(not bOk or nLastBolPositionInCache == nPositionInCache);
	ensure(not bOk or lBeginPos + nCurrentBufferSize <= lMaxEndPos or
	       (IsFirstPositionInFile() and (lBeginPos + nCurrentBufferSize <= lMaxEndPos + GetPositionInFile())));
	return bOk;
}

boolean InputBufferedFile::FillOuterLinesUntil(longint& lBeginPos, longint lMaxEndPos, boolean& bLineTooLong)
{
	boolean bOk;
	longint lNextLinePos;
	longint lNewBeginPos;

	require(0 <= lBeginPos and lBeginPos <= GetFileSize());
	require(lBeginPos <= lMaxEndPos and lMaxEndPos <= GetFileSize());
	require(GetBufferSize() > 0 or lBeginPos == lMaxEndPos);

	// Remplissage du buffer avec des lignes entieres
	bLineTooLong = false;
	bOk = FillInnerLinesUntil(lBeginPos, lMaxEndPos);

	// Si buffer vide, tentative de lecture d'une seule ligne potentiellement de grande taille
	if (bOk and GetCurrentBufferSize() == 0 and lBeginPos < lMaxEndPos)
	{
		bOk = FillOneLine(lBeginPos, bLineTooLong);

		// Si non trouvee et que l'on a pas atteint la fin du chunk, on cherche un nouveau debut de ligne
		if (bOk and GetCurrentBufferSize() == 0)
		{
			assert(bLineTooLong);

			// Recherche a partir de la taille max d'une ligne au dela de la position courante si on est
			// loin de la position de fin
			lNextLinePos = -1;
			if (lBeginPos + GetMaxLineLength() < lMaxEndPos)
			{
				lNewBeginPos = lBeginPos + GetMaxLineLength();
				bOk = SearchNextLineUntil(lNewBeginPos, lMaxEndPos, lNextLinePos);
			}

			// Si non trouve, on force la position du buffer a lMaxEndPos en remplissant une zone vide
			if (bOk and lNextLinePos == -1)
				InternalFillBytes(lMaxEndPos, 0);
			assert(GetCurrentBufferSize() == 0);
			assert(not bOk or GetPositionInFile() >= lMaxEndPos or
			       (GetPositionInFile() - lBeginPos) > GetMaxLineLength());
		}
		assert(not bOk or bLineTooLong or
		       GetPositionInFile() + GetCurrentBufferSize() >= lBeginPos + GetBufferSize() or
		       GetPositionInFile() + GetCurrentBufferSize() >= lMaxEndPos);
	}
	return bOk;
}

boolean InputBufferedFile::FillBytes(longint& lBeginPos)
{
	int nSizeToFill;
	nSizeToFill = nBufferSize;
	if (lBeginPos + nBufferSize > GetFileSize())
		nSizeToFill = int(GetFileSize() - lBeginPos);
	return InternalFillBytes(lBeginPos, nSizeToFill);
}

const ALString InputBufferedFile::GetFieldErrorLabel(int nFieldError)
{
	ALString sTmp;

	require(FieldNoError < nFieldError and nFieldError <= FieldTooLong);

	switch (nFieldError)
	{
	case FieldMissingBeginDoubleQuote:
		return "missing double quote at the start of the field";
	case FieldMissingMiddleDoubleQuote:
		return "double quote in the middle of the field should be paired";
	case FieldMissingEndDoubleQuote:
		return "missing double quote at the end of the field";
	case FieldTooLong:
		return sTmp + "field too long, truncated to " + IntToString(nMaxFieldSize) + " characters";
	default:
		assert(false);
		return "";
	}
}

const ALString InputBufferedFile::GetLineTooLongErrorLabel()
{
	ALString sTmp;

	return sTmp + "line too long (beyond " + IntToString(GetMaxLineLength()) + " characters)";
}

boolean InputBufferedFile::GetNextField(char*& sField, int& nFieldLength, int& nFieldError, boolean& bLineTooLong)
{
	boolean bUnconditionalLoop = true; // Pour eviter un warning dans la boucle
	char c;
	boolean bEndOfLine;
	int i;
	int iStart;

	// Acces au buffer
	sField = GetHugeBuffer(nMaxFieldSize + 1);
	assert(sField != NULL);

	// Si le dernier champ lu etait sur une fin de ligne,
	// nous sommes sur un debut de ligne...
	if (bLastFieldReachEol)
	{
		nCurrentLineIndex++;
		bLastFieldReachEol = false;
	}

	// Lecture des caracteres du token
	i = 0;
	nFieldError = FieldNoError;
	bEndOfLine = true;
	if (not IsBufferEnd())
	{
		// Lecture du premier caractere: traitement special si double quote
		c = GetNextChar();

		// Traitement special si le champ commence par double quote
		// (sauf si separateur double quote)
		if (c == '"' and cFieldSeparator != '"')
		{
			bEndOfLine = GetNextDoubleQuoteField(sField, i, nFieldError);
		}
		// Traitement standard si le champ ne commence pas par double quote
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

			// Erreur si double quote a la fin du champ
			if (GetPositionInCache() > 1 and GetPrevChar() == '"')
				nFieldError = FieldMissingBeginDoubleQuote;
			// Test additionnel dans le cas d'une fin de ligne, pour gestion des fins de ligne windows "\r\n
			else if (c != cFieldSeparator and GetPositionInCache() > 2 and GetPrevChar() == '\r' and
				 GetPrevPrevChar() == '"')
				nFieldError = FieldMissingBeginDoubleQuote;
		}
	}

	// Test de depassement de longueur
	if (nFieldError != FieldNoError and i >= (int)nMaxFieldSize)
		nFieldError = FieldTooLong;

	// Supression des blancs en fin (TrimRight)
	// Attention: on utilise iswspace et non isspace, systematiquement dans tous les sources
	// Une tentative de passage a isspace a entraine une degradation des performances de presque 25%
	// dans un traitement complet impliquant des lectures de fichier
	i--;
	while (i >= 0)
	{
		c = sField[i];
		if (not iswspace(c))
			break;
		i--;
	}
	i++;

	// Fin de champ
	sField[i] = '\0';

	// Supression des blancs en debut (TrimLeft)
	iStart = 0;
	while ((c = sField[iStart]) != '\0' and iswspace(c))
		iStart++;
	if (iStart > 0)
	{
		// Mise a jour de la nouvelle longueur du champ
		i -= iStart;

		// Copie de la fin du champ (y compris le '\0')
		memmove(sField, &sField[iStart], i + 1);
	}
	assert(sField[i] == '\0');

	// Memorisation de la longueur du champ
	nFieldLength = i;
	assert((int)strlen(sField) == nFieldLength);

	// Gestion d'une fin de ligne
	if (bEndOfLine)
	{
		// L'erreur de type ligne trop longue ecrase les autres
		bLineTooLong = IsLineTooLong();
		nLastBolPositionInCache = nPositionInCache;
	}
	else
		bLineTooLong = false;

	// Retour du fin de ligne
	return bEndOfLine;
}

boolean InputBufferedFile::SkipField(int& nFieldError, boolean& bLineTooLong)
{
	boolean bUnconditionalLoop = true; // Pour eviter un warning dans la boucle
	char c;
	boolean bEndOfLine;
	int nStartPositionInCache;

	// Si le dernier champ lu etait sur une fin de ligne,
	// nous sommes sur un debut de ligne...
	if (bLastFieldReachEol)
	{
		nCurrentLineIndex++;
		bLastFieldReachEol = false;
	}

	// Lecture des caracteres du token
	nStartPositionInCache = GetPositionInCache();
	nFieldError = FieldNoError;
	bEndOfLine = true;
	if (not IsBufferEnd())
	{
		// Lecture du premier caractere: traitement special si double quote
		c = GetNextChar();

		// Traitement special si le champ commence par double quote
		// (sauf si separateur double quote)
		if (c == '"' and cFieldSeparator != '"')
		{
			bEndOfLine = SkipDoubleQuoteField(nFieldError);
		}
		// Traitement standard si le champ ne commence pas par double quote
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

				// Caractere suivant
				if (IsBufferEnd())
					break;
				c = GetNextChar();
			}

			// Erreur si double quote a la fin du champ
			if (GetPositionInCache() > 1 and GetPrevChar() == '"')
				nFieldError = FieldMissingBeginDoubleQuote;
			// Test additionnel dans le cas d'une fin de ligne, pour gestion des fins de ligne windows "\r\n
			else if (c != cFieldSeparator and GetPositionInCache() > 2 and GetPrevChar() == '\r' and
				 GetPrevPrevChar() == '"')
				nFieldError = FieldMissingBeginDoubleQuote;
		}
	}

	// Test de depassement de longueur
	if (nFieldError != FieldNoError and GetPositionInCache() - nStartPositionInCache >= (int)nMaxFieldSize)
		nFieldError = FieldTooLong;

	// Gestion d'une fin de ligne
	if (bEndOfLine)
	{
		// L'erreur de type ligne trop longue ecrase les autres
		bLineTooLong = IsLineTooLong();
		nLastBolPositionInCache = nPositionInCache;
	}
	else
		bLineTooLong = false;

	// Retour du fin de ligne
	return bEndOfLine;
}

void InputBufferedFile::GetNextLine(CharVector* cvLine, boolean& bLineTooLong)
{
	char c;

	require(cvLine != NULL);

	// Incrementation du numero de ligne si on avait atteint la fin de ligne par un GetNextField
	if (not IsBufferEnd() and bLastFieldReachEol)
	{
		nCurrentLineIndex++;
		nLastBolPositionInCache = nPositionInCache;
		bLastFieldReachEol = false;
	}

	// Lecture caractere a caractere
	cvLine->SetSize(0);
	while (not IsBufferEnd())
	{
		c = GetNextChar();
		cvLine->Add(c);

		// Gestion de la fin de ligne
		if (c == '\n')
		{
			nCurrentLineIndex++;
			break;
		}
	}
	bLineTooLong = IsLineTooLong();
	nLastBolPositionInCache = nPositionInCache;
}

void InputBufferedFile::ExtractSubBuffer(int nBeginPos, int nEndPos, CharVector* cvSubBuffer) const
{
	require(nBeginPos >= 0 and nBeginPos < nEndPos);
	require(nEndPos <= fcCache.GetSize());
	require(cvSubBuffer != NULL);

	// Retaillage du resultat
	cvSubBuffer->SetSize(nEndPos - nBeginPos);

	// Recopie de la sous-partie du buffer
	cvSubBuffer->ImportSubBuffer(0, nEndPos - nBeginPos, &fcCache.cvBuffer, nBufferStartInCache + nBeginPos);
}

void InputBufferedFile::SetUTF8BomManagement(boolean bValue)
{
	require(not IsOpened());
	bUTF8BomManagement = bValue;
}

boolean InputBufferedFile::GetUTF8BomManagement() const
{
	return bUTF8BomManagement;
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

	// On est cense avoir lu un double quote en debut de champ
	sField[0] = '"';
	i = 1;

	// Le champ doit se terminer par un double quote
	// On accepte les double quotes (doubles) a l'interieur du champ, et meme le separateur de champ
	// En cas d'erreur (pas de double quote final ou double quote non double au milieu), on
	// parse "normalement" jusqu'au prochain separateur et on rend le champ entier tel quel
	// (y compris le premier double quote)
	nFieldError = FieldMissingEndDoubleQuote;
	bEndOfLine = true;

	// Si on ne passe pas dans la boucle, on sera en erreur (double quote suivi de fin de fichier)
	if (not IsBufferEnd())
	{
		// Lecture du premier caractere: traitement special si double quote
		c = GetNextChar();

		// Traitement special si le champ commence par double quote
		while (bUnconditionalLoop)
		{
			// Test si double quote
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

				// OK si fin de champ apres double quote
				if (c == cFieldSeparator)
				{
					nFieldError = FieldNoError;
					bEndOfLine = false;
					break;
				}

				// OK si fin de ligne apres double quote
				if (c == '\n')
				{
					nFieldError = FieldNoError;
					bLastFieldReachEol = true;
					break;
				}

				// OK si nouveau double quote (il a ete correctement double)
				if (c == '"')
				{
					// On memorise ici les deux double quote
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
				// KO si on trouve un double quote isole au milieu du champ
				//  On continue quand-meme a parser pour recuperer l'erreur
				//  On memorise neanmoins de double quote isole
				else
				{
					assert(c != '"');
					nFieldError = FieldMissingMiddleDoubleQuote;

					// On memorise le caractere double quote isole
					if (i < (int)nMaxFieldSize)
					{
						sField[i] = '"';
						i++;
					}

					// Si carriage return suivi de fin de fichier ou de ligne, c'est OK
					if (c == '\r')
					{
						// On annule temporairement la prise du double quote
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

						// OK si fin de champ apres double quote
						if (c == cFieldSeparator)
						{
							nFieldError = FieldNoError;
							bEndOfLine = false;
							break;
						}

						// OK si fin de ligne apres double quote
						if (c == '\n')
						{
							nFieldError = FieldNoError;
							bLastFieldReachEol = true;
							break;
						}

						// On memorise le caractere carriage return, apres avoir restitue le
						// double quote
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

		// On doit supprimer le premier double quote, ainsi que toutes les paires de double quote
		// Le dernier double quote n'a pas ete memorise
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
	// La gestion des lignes trop longues est effectuee dans la methode appelante
	return bEndOfLine;
}

boolean InputBufferedFile::SkipDoubleQuoteField(int& nFieldError)
{
	boolean bUnconditionalLoop = true; // Pour eviter un warning dans la boucle
	char c;
	boolean bEndOfLine;

	require(nFieldError == FieldNoError);

	// Le champ doit se terminer par un double quote
	// On accepte les double quotes (doubles) a l'interieur du champ, et meme le separateur de champ
	// En cas d'erreur (pas de double quote final ou double quote non double au milieu), on
	// parse "normalement" jusqu'au prochain separateur et on rend le champ entier tel quel
	// (y compris le premier double quote)
	nFieldError = FieldMissingEndDoubleQuote;
	bEndOfLine = true;

	// Si on ne passe pas dans la boucle, on sera en erreur (double quote suivi de fin de fichier)
	if (not IsBufferEnd())
	{
		// Lecture du premier caractere: traitement special si double quote
		c = GetNextChar();

		// Traitement special si le champ commence par double quote
		while (bUnconditionalLoop)
		{
			// Test si double quote
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

				// OK si fin de champ apres double quote
				if (c == cFieldSeparator)
				{
					nFieldError = FieldNoError;
					bEndOfLine = false;
					break;
				}

				// OK si fin de ligne apres double quote
				if (c == '\n')
				{
					nFieldError = FieldNoError;
					bLastFieldReachEol = true;
					break;
				}

				// OK si nouveau double quote (il a ete correctement double)
				if (c == '"')
				{
					// Preparation du caractere suivant
					if (not IsBufferEnd())
						c = GetNextChar();
					continue;
				}
				// KO si on trouve un double quote isole au milieu du champ
				//  On continue quand-meme a parser pour recuperer l'erreur
				//  On memorise neanmoins de double quote isole
				else
				{
					assert(c != '"');
					nFieldError = FieldMissingMiddleDoubleQuote;

					// Si carriage return suivi de fin de fichier ou de ligne, c'est OK
					if (c == '\r')
					{
						// OK si fin du buffer
						if (IsBufferEnd())
						{
							nFieldError = FieldNoError;
							break;
						}

						// Lecture caractere suivant, en skippant le carriage return
						assert(not IsBufferEnd());
						c = GetNextChar();

						// OK si fin de champ apres double quote
						if (c == cFieldSeparator)
						{
							nFieldError = FieldNoError;
							bEndOfLine = false;
							break;
						}

						// OK si fin de ligne apres double quote
						if (c == '\n')
						{
							nFieldError = FieldNoError;
							bLastFieldReachEol = true;
							break;
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

			// Caractere suivant
			if (IsBufferEnd())
				break;
			c = GetNextChar();
		}
	}

	// La gestion des lignes trop longues est effectuee dans la methode appelante
	return bEndOfLine;
}

boolean InputBufferedFile::CheckEncoding() const
{
	boolean bOk = true;
	boolean bIgnoreUTF8Bom = true;
	const unsigned char cUTF16BigEndianBom[2] = {0xFE, 0xFF};
	const unsigned char cUTF16LittleEndianBom[2] = {0xFF, 0xFE};
	const unsigned char cUTF32BigEndianBom[4] = {0x00, 0x00, 0xFE, 0xFF};
	const unsigned char cUTF32LittleEndianBom[4] = {0xFF, 0xFE, 0x00, 0x00};
	const ALString sConversionMessage = "; try convert the file to ANSI or UTF-8 (without BOM) format";
	int i;
	char c;
	int nCarriageReturnNumber;
	ALString sMacMessage;
	ALString sTmp;

	require(GetPositionInFile() == 0 or GetPositionInFile() == nUTF8BomSize);

	// Test de la presence du BOM, saute si on a saute a saute le BOM UTF8
	if (GetPositionInFile() == 0)
	{
		// Test si encodage UTF8 avec BOM
		if (not bIgnoreUTF8Bom and nCurrentBufferSize >= 3 and
		    (unsigned char) fcCache.GetAt(0) == cUTF8Bom[0] and
		    (unsigned char) fcCache.GetAt(1) == cUTF8Bom[1] and (unsigned char) fcCache.GetAt(2) == cUTF8Bom[2])
		{
			bOk = false;
			AddError(sTmp + "Encoding type cannot be handled (UTF-8 with BOM)" + sConversionMessage);
		}
		// Test si encodage UTF32 big endian
		else if (nCurrentBufferSize >= 4 and (unsigned char) fcCache.GetAt(0) == cUTF32BigEndianBom[0] and
			 (unsigned char) fcCache.GetAt(1) == cUTF32BigEndianBom[1] and
			 (unsigned char) fcCache.GetAt(2) == cUTF32BigEndianBom[2] and
			 (unsigned char) fcCache.GetAt(3) == cUTF32BigEndianBom[3])
		{
			bOk = false;
			AddError(sTmp + "Encoding type cannot be handled (UTF-32 big endian)" + sConversionMessage);
		}
		// Test si encodage UTF32 little endian
		else if (nCurrentBufferSize >= 4 and (unsigned char) fcCache.GetAt(0) == cUTF32LittleEndianBom[0] and
			 (unsigned char) fcCache.GetAt(1) == cUTF32LittleEndianBom[1] and
			 (unsigned char) fcCache.GetAt(2) == cUTF32LittleEndianBom[2] and
			 (unsigned char) fcCache.GetAt(3) == cUTF32LittleEndianBom[3])
		{
			bOk = false;
			AddError(sTmp + "Encoding type cannot be handled (UTF-32 little endian)" + sConversionMessage);
		}
		// Test si encodage UTF16 big endian
		else if (nCurrentBufferSize >= 2 and (unsigned char) fcCache.GetAt(0) == cUTF16BigEndianBom[0] and
			 (unsigned char) fcCache.GetAt(1) == cUTF16BigEndianBom[1])
		{
			bOk = false;
			AddError(sTmp + "Encoding type cannot be handled (UTF-16 big endian)" + sConversionMessage);
		}
		// Test si encodage UTF16 little endian
		else if (nCurrentBufferSize >= 2 and (unsigned char) fcCache.GetAt(0) == cUTF16LittleEndianBom[0] and
			 (unsigned char) fcCache.GetAt(1) == cUTF16LittleEndianBom[1])
		{
			bOk = false;
			AddError(sTmp + "Encoding type cannot be handled (UTF-16 little endian)" + sConversionMessage);
		}
	}

	// Recherche de caracteres NULL
	if (bOk)
	{
		for (i = 0; i < nCurrentBufferSize; i++)
		{
			c = fcCache.GetAt(i);
			if (c == '\0')
			{
				bOk = false;
				AddError(sTmp + "Encoding type cannot be handled (null char detected at offset " +
					 IntToString(i + 1) + ")" + sConversionMessage);
				break;
			}
			if (i > 1000)
				break;
		}
	}

	// Recherche d'un eventuel format Mac anterieur a Mac OS 10 si une seule ligne dans le buffer
	if (bOk and GetBufferLineNumber() == 1)
	{
		nCarriageReturnNumber = 0;
		for (i = 0; i < nCurrentBufferSize; i++)
		{
			c = fcCache.GetAt(i);
			if (c == '\r')
				nCarriageReturnNumber++;
			if (nCarriageReturnNumber > 1)
			{
				bOk = false;

				// Message d'erreur
				sMacMessage = "Only one line detected in";
				if (nCurrentBufferSize == GetFileSize())
					sMacMessage += sTmp + " all file (" +
						       LongintToReadableString(nCurrentBufferSize) + " characters)";
				else
					sMacMessage += sTmp + " the first " +
						       LongintToReadableString(nCurrentBufferSize) + " characters read";
				sMacMessage += ", with several CR (Carriage Return) characters used";
				sMacMessage +=
				    "; it could be a Mac OS Classic format (now deprecated, since Mac OS X in 2001)";
				sMacMessage += "; try to convert the file to recent Mac, linux or windows format";
				AddError(sMacMessage);
				break;
			}
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
					      boolean bSilent, boolean bVerbose, StringVector* svFirstLineFields)
{
	boolean bOk;
	InputBufferedFile inputFile;
	boolean bLineTooLong;
	boolean bEndOfLine;
	longint lBeginPos;
	char* sField;
	int nField;
	int nFieldLength;
	int nFieldError;
	boolean bCloseOk;
	ALString sTmp;

	require(svFirstLineFields != NULL);

	// Initialisation du vecteur
	svFirstLineFields->SetSize(0);

	// Extraction de la premiere ligne du fichier d'entree en utilisant un fichier bufferise
	inputFile.SetFileName(sInputFileName);
	inputFile.SetFieldSeparator(cInputFieldSeparator);
	inputFile.SetSilentMode(bSilent);

	// Ouverture du fichier
	bOk = inputFile.Open();
	if (bOk)
	{
		// Taille de buffer reduite si fichier de petite taille
		// (en veillant a ne pas avoir un buffer de taille nulle)
		if (inputFile.GetFileSize() < GetMaxLineLength())
			inputFile.SetBufferSize((int)inputFile.GetFileSize());
		else
			inputFile.SetBufferSize(GetMaxLineLength());

		// Lecture de la premiere ligne
		lBeginPos = 0;
		bOk = inputFile.FillOneLine(lBeginPos, bLineTooLong);
		if (not bOk)
		{
			inputFile.AddError("Unable to read first line");
		}
		else if (bLineTooLong)
		{
			inputFile.AddError("First line, " + GetLineTooLongErrorLabel());
			bOk = false;
		}
	}

	// Test si caracteres speciaux
	// Erreur si du caractere NULL
	if (bOk)
		bOk = inputFile.CheckEncoding();

	// Recherche des champs de la premiere ligne
	if (bOk)
	{
		Global::ActivateErrorFlowControl();
		bEndOfLine = false;
		nField = 0;
		while (not bEndOfLine)
		{
			bEndOfLine = inputFile.GetNextField(sField, nFieldLength, nFieldError, bLineTooLong);
			nField++;
			assert(not bLineTooLong);

			// Warning sur le nom du champ
			if (nFieldError != inputFile.FieldNoError and not bSilent and bVerbose)
			{
				inputFile.AddWarning(sTmp + "First line field " + IntToString(nField) + " <" +
						     InputBufferedFile::GetDisplayValue(sField) +
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
		bOk = bCloseOk and bOk;
	}

	// Nettoyage si neessaire
	if (not bOk)
		svFirstLineFields->SetSize(0);
	return bOk;
}

ALString InputBufferedFile::GetDisplayValue(const ALString& sValue)
{
	// On prend une longueur max de 25, pour permettre d'avoir une valeur de timestamp avec time zone complete
	// dans son format usuel "YYYY-MM-DD HH:MM:SSzzzzzz"
	const int nMaxSize = 25;

	if (sValue.GetLength() <= nMaxSize + 3)
		return sValue;
	else
		return sValue.Left(nMaxSize) + "...";
}

void InputBufferedFile::SetCacheOn(boolean bValue)
{
	bCacheOn = bValue;
}

boolean InputBufferedFile::GetCacheOn() const
{
	return bCacheOn;
}

const CharVector* InputBufferedFile::GetCache() const
{
	return &fcCache.cvBuffer;
}

longint InputBufferedFile::GetTotalPhysicalReadCalls() const
{
	return lTotalPhysicalReadCalls;
}

longint InputBufferedFile::GetTotalPhysicalReadBytes() const
{
	return lTotalPhysicalReadBytes;
}

boolean InputBufferedFile::Test(int nFileType)
{
	boolean bOk = true;

	bOk = bOk and TestReadWrite("cache size", 1 * lMB, nFileType);
	ensure(bOk);

	bOk = bOk and TestReadWrite("cache size + 1", 1 * lMB - 1, nFileType);
	ensure(bOk);

	bOk = bOk and TestReadWrite("cache size -1", 1 * lMB + 1, nFileType);
	ensure(bOk);

	bOk = bOk and TestReadWrite("8_MB", 8 * lMB, nFileType);
	ensure(bOk);

	bOk = bOk and TestReadWrite("8_MB - 1", 8 * lMB - 1, nFileType);
	ensure(bOk);

	bOk = bOk and TestReadWrite("8_MB + 1", 8 * lMB + 1, nFileType);
	ensure(bOk);

	if (not bOk)
		cout << "Error in test" << endl;
	return bOk;
}

boolean InputBufferedFile::TestCopy(const ALString& sSrcFileName, int nFileType, int nOverhad)
{
	InputBufferedFile ibFile;
	OutputBufferedFile obFile;
	longint lFilePos;
	boolean bOk;
	ALString sTmp;
	ALString sDestFileName;
	boolean bSameFile;
	int nChunkSize;
	CharVector cvTempBuffer;
	ALString sRemoteFileName;

	switch (nFileType)
	{
	case 0:
		sRemoteFileName = sSrcFileName;
		break;
	case 1:
		sRemoteFileName = sTmp + FileService::sRemoteScheme + "://" + GetLocalHostName() + sSrcFileName;
		break;
	case 2:
		sRemoteFileName = sSrcFileName;
		break;
	default:
		assert(false);
	}

	cout << endl
	     << "Copying " + sRemoteFileName + " (size " +
		    LongintToHumanReadableString(PLRemoteFileService::GetFileSize(sRemoteFileName)) + ")"
	     << endl;

	cout << endl;
	cout << "Buffer size\tidenticals" << endl;

	bOk = true;

	// Boucle sur la taille du buffer
	nChunkSize = 128 * lMB;
	// nChunkSize = 512 * lKB;
	while (bOk and nChunkSize > 16 * lKB)
	{
		// Construction du nom du nouveau fichier
		sDestFileName = FileService::BuildFilePathName(
		    FileService::GetPathName(FileService::GetURIFilePathName(sRemoteFileName)),
		    FileService::BuildFileName(FileService::GetFilePrefix(sRemoteFileName) + "_" +
						   LongintToString(nChunkSize),
					       FileService::GetFileSuffix(sRemoteFileName)));
		obFile.SetFileName(sDestFileName);
		bOk = obFile.Open();
		if (bOk)
		{
			ibFile.SetFileName(sSrcFileName);
			bOk = ibFile.Open();
		}
		if (bOk)
		{
			ibFile.SetBufferSize(nChunkSize + nOverhad);

			// Lecture et ecriture
			lFilePos = 0;
			while (lFilePos < ibFile.GetFileSize())
			{
				bOk = ibFile.FillBytes(lFilePos);
				if (not bOk)
					break;
				assert(ibFile.fcCache.cvBuffer.GetSize() > 0);

				// Ecriture du contenu du buffer
				obFile.WriteSubPart(&ibFile.fcCache.cvBuffer, ibFile.GetBufferStartInCache(),
						    ibFile.GetCurrentBufferSize());
				lFilePos += ibFile.GetCurrentBufferSize();
			}
			ibFile.Close();
			obFile.Close();
		}
		ensure(bOk);
		if (bOk)
		{
			bSameFile = PLRemoteFileService::FileCompare(sRemoteFileName, sDestFileName);
		}
		else
			bSameFile = false;

		cout << LongintToHumanReadableString(nChunkSize) << "\t" << BooleanToString(bSameFile) << endl;

		if (bSameFile)
			PLRemoteFileService::RemoveFile(sDestFileName);
		else
			cout << "ERROR File " << sDestFileName << " is not identical to source" << endl;
		ensure(bSameFile);
		nChunkSize /= 2;
	}
	return bOk;
}

longint InputBufferedFile::TestCountUsingSearchNextLine(int nInputChunkSize)
{
	boolean bOk;
	longint lFilePos;
	longint lMaxEndPos;
	longint lNextLinePos;
	longint lLineNumber;

	require(GetFileName() != "");
	require(not IsOpened());
	require(nInputChunkSize > 0);

	// Parcours du fichier
	lLineNumber = 0;
	bOk = Open();
	if (bOk)
	{
		lFilePos = 0;
		lMaxEndPos = 0;

		// Parcours du fichier par chunk
		while (bOk and lFilePos < GetFileSize())
		{
			lMaxEndPos += nInputChunkSize;
			lMaxEndPos = min(lMaxEndPos, GetFileSize());

			// Parcours des lignes d'un chunk
			while (lFilePos < lMaxEndPos)
			{
				bOk = SearchNextLineUntil(lFilePos, lMaxEndPos, lNextLinePos);
				if (not bOk)
					break;

				// Prise en compte de la ligne si trouvee
				if (lNextLinePos != -1)
				{
					lFilePos = lNextLinePos;
					lLineNumber++;
				}
				// Si non trouvee, on se place en fin de chunk
				else
					lFilePos = lMaxEndPos;
			}
		}

		// Fermeture
		Close();
	}

	// On retourne -1 en cas d'erreur
	if (not bOk)
		lLineNumber = -1;
	return lLineNumber;
}

longint InputBufferedFile::TestCountUsingFillOneLine(int nInputChunkSize, longint& lLongLineNumber)
{
	boolean bOk;
	longint lFilePos;
	longint lMaxEndPos;
	boolean bLineTooLong;
	longint lNextLinePos;
	longint lLineNumber;

	require(GetFileName() != "");
	require(not IsOpened());
	require(nInputChunkSize > 0);

	// Parcours du fichier
	lLineNumber = 0;
	lLongLineNumber = 0;
	bOk = Open();
	if (bOk)
	{
		lFilePos = 0;
		lMaxEndPos = 0;

		// Parcours du fichier par chunk
		while (bOk and lFilePos < GetFileSize())
		{
			lMaxEndPos += nInputChunkSize;
			lMaxEndPos = min(lMaxEndPos, GetFileSize());

			// Parcours des lignes d'un chunk
			while (lFilePos < lMaxEndPos)
			{
				bOk = FillOneLine(lFilePos, bLineTooLong);
				if (not bOk)
					break;

				// Prise en compte de la ligne si trouvee
				if (not bLineTooLong)
				{
					lFilePos += GetCurrentBufferSize();
					lLineNumber++;
					assert(GetBufferLineNumber() == 1);
				}
				// Si non trouvee, on cherche un nouveau debut de ligne
				else
				{
					assert(GetBufferLineNumber() == 0);
					bOk = SearchNextLine(lFilePos, lNextLinePos);
					if (not bOk)
						break;

					// Prise en compte de la ligne si trouvee
					if (lNextLinePos != -1)
					{
						assert(lNextLinePos - lFilePos > GetMaxLineLength());
						lFilePos = lNextLinePos;
						lLineNumber++;
						lLongLineNumber++;
					}
					// Si non trouvee, on se place en fin de fichier
					else
						lFilePos = GetFileSize();
					assert(GetBufferLineNumber() == 0);
				}
			}
		}

		// Fermeture
		Close();
	}

	// On retourne -1 en cas d'erreur
	if (not bOk)
	{
		lLineNumber = -1;
		lLongLineNumber = -1;
	}
	return lLineNumber;
}

longint InputBufferedFile::TestCountUsingFillInnerLines(int nInputChunkSize, longint& lLongLineNumber)
{
	boolean bOk;
	longint lBeginChunk;
	longint lEndChunk;
	longint lFilePos;
	longint lNewFilePos;
	longint lMaxEndPos;
	boolean bLineTooLong;
	longint lNextLinePos;
	longint lLineNumber;
	longint lLineNumberUsingCurrentBuffer;

	require(GetFileName() != "");
	require(not IsOpened());
	require(nInputChunkSize > 0);

	// Parcours du fichier
	lLineNumber = 0;
	lLongLineNumber = 0;
	lLineNumberUsingCurrentBuffer = 0;
	bOk = Open();
	if (bOk)
	{
		lBeginChunk = 0;
		lEndChunk = nInputChunkSize;

		// Parcours du fichier par chunk
		while (bOk and lBeginChunk < GetFileSize())
		{
			// On commence l'analyse au debut du chunk
			lFilePos = lBeginChunk;

			// On la termine sur la derniere ligne commencant dans le chunk, donc dans le '\n' se trouve
			// potentiellement sur le debut de chunk suivant
			lMaxEndPos = min(lEndChunk + 1, GetFileSize());

			// On commence par rechercher un debut de ligne, sauf si on est en debut de fichier
			if (lFilePos > 0)
			{
				bOk = SearchNextLineUntil(lFilePos, lMaxEndPos, lNextLinePos);
				if (not bOk)
					break;

				// On se positionne sur le debut de la ligne suivante si elle est trouvee
				if (lNextLinePos != -1)
					lFilePos = lNextLinePos;
				// Si non trouvee, on se place en fin de chunk
				else
					lFilePos = lMaxEndPos;
			}

			// Analyse du chunk si on a y trouve un debut de ligne
			if (bOk and lFilePos < lMaxEndPos)
			{
				// Parcours du chunk par paquets de lignes
				while (bOk and lFilePos < lMaxEndPos)
				{
					// Remplissage du buffer avec des ligne
					bOk = FillInnerLinesUntil(lFilePos, lMaxEndPos);
					if (not bOk)
						break;

					// Comptage des lignes du buffer si on a un buffer non vide
					if (GetCurrentBufferSize() > 0)
					{
						while (not IsBufferEnd())
						{
							SkipLine(bLineTooLong);
							lLineNumber++;
							if (bLineTooLong)
								lLongLineNumber++;
						}
						lFilePos += GetCurrentBufferSize();
						lLineNumberUsingCurrentBuffer += GetBufferLineNumber();
						assert(lLineNumber == lLineNumberUsingCurrentBuffer);
					}
					// Sinon, tentative de lecture d'une seule ligne potentiellement de grande
					// taille
					else
					{
						lLineNumber++;
						lLineNumberUsingCurrentBuffer++;
						bOk = FillOneLine(lFilePos, bLineTooLong);
						if (not bOk)
							break;

						// Ajout d'une ligne trop longue
						if (bLineTooLong)
							lLongLineNumber++;

						// Prise en compte de la taille de la ligne si trouvee
						if (not bLineTooLong)
							lFilePos += GetCurrentBufferSize();
						// Si non trouvee et que l'on a pas atteint la fin du chunk, on cherche
						// un nouveau debut de ligne
						else if (lFilePos + GetMaxLineLength() < lMaxEndPos)
						{
							// Recherche a partir de la taille max d'une ligne au dela de la
							// position courante
							lNewFilePos = lFilePos + GetMaxLineLength();
							bOk =
							    SearchNextLineUntil(lNewFilePos, lMaxEndPos, lNextLinePos);
							if (not bOk)
								break;

							// Deplacement vers le debut de la ligne suivante si trouvee
							if (lNextLinePos != -1)
							{
								assert(lNextLinePos - lFilePos > GetMaxLineLength());
								lFilePos = lNextLinePos;
							}
							// Si non trouvee, on se place en fin de chunk
							else
								lFilePos = lMaxEndPos;
						}
						// Sinon, on se positionne en fin de chunk pour arreter l'analyse
						else
							lFilePos = lMaxEndPos;
					}
				}
			}

			// Passage au chunk suivant
			lBeginChunk += nInputChunkSize;
			lEndChunk += nInputChunkSize;
		}

		// Fermeture
		Close();
	}

	// On retourne -1 en cas d'erreur
	if (not bOk)
	{
		lLineNumber = -1;
		lLongLineNumber = -1;
	}
	return lLineNumber;
}

longint InputBufferedFile::TestCountUsingFillOuterLines(int nInputChunkSize, longint& lLongLineNumber)
{
	boolean bOk;
	longint lBeginChunk;
	longint lEndChunk;
	longint lFilePos;
	longint lMaxEndPos;
	boolean bLineTooLong;
	longint lNextLinePos;
	longint lLineNumber;
	longint lLineNumberUsingCurrentBuffer;

	require(GetFileName() != "");
	require(not IsOpened());
	require(nInputChunkSize > 0);

	// Parcours du fichier
	lLineNumber = 0;
	lLongLineNumber = 0;
	lLineNumberUsingCurrentBuffer = 0;
	bOk = Open();
	if (bOk)
	{
		lBeginChunk = 0;
		lEndChunk = nInputChunkSize;

		// Parcours du fichier par chunk
		while (bOk and lBeginChunk < GetFileSize())
		{
			// On commence l'analyse au debut du chunk
			lFilePos = lBeginChunk;

			// On la termine sur la derniere ligne commencant dans le chunk, donc dans le '\n' se trouve
			// potentiellement sur le debut de chunk suivant
			lMaxEndPos = min(lEndChunk + 1, GetFileSize());

			// On commence par rechercher un debut de ligne, sauf si on est en debut de fichier
			if (lFilePos > 0)
			{
				bOk = SearchNextLineUntil(lFilePos, lMaxEndPos, lNextLinePos);
				if (not bOk)
					break;

				// On se positionne sur le debut de la ligne suivante si elle est trouvee
				if (lNextLinePos != -1)
					lFilePos = lNextLinePos;
				// Si non trouvee, on se place en fin de chunk
				else
					lFilePos = lMaxEndPos;
			}

			// Analyse du chunk si on a y trouve un debut de ligne
			if (bOk and lFilePos < lMaxEndPos)
			{
				// Parcours du chunk par paquets de lignes
				while (bOk and lFilePos < lMaxEndPos)
				{
					// Remplissage du buffer avec des ligne
					bOk = FillOuterLinesUntil(lFilePos, lMaxEndPos, bLineTooLong);
					if (not bOk)
						break;

					// Comptage des lignes du buffer si pas de ligne trop longue
					if (not bLineTooLong)
					{
						while (not IsBufferEnd())
						{
							SkipLine(bLineTooLong);
							lLineNumber++;
							if (bLineTooLong)
								lLongLineNumber++;
						}
						lFilePos += GetCurrentBufferSize();
						lLineNumberUsingCurrentBuffer += GetBufferLineNumber();
						assert(lLineNumber == lLineNumberUsingCurrentBuffer);
					}
					// Sinon, on se positionne sur la fin de la ligne suivante ou la a derniere
					// position
					else
					{
						assert(bLineTooLong);
						assert(GetCurrentBufferSize() == 0);
						assert(GetPositionInFile() - lFilePos > GetMaxLineLength() or
						       GetPositionInFile() == lMaxEndPos);
						lLineNumber++;
						lLongLineNumber++;
						lLineNumberUsingCurrentBuffer++;
						lFilePos = GetPositionInFile();
					}
				}
			}

			// Passage au chunk suivant
			lBeginChunk += nInputChunkSize;
			lEndChunk += nInputChunkSize;
		}

		// Fermeture
		Close();
	}

	// On retourne -1 en cas d'erreur
	if (not bOk)
	{
		lLineNumber = -1;
		lLongLineNumber = -1;
	}
	return lLineNumber;
}

longint InputBufferedFile::TestCountUsingFillOuterLinesBackward(int nInputChunkSize, longint& lLongLineNumber)
{
	boolean bOk;
	longint lBeginChunk;
	longint lEndChunk;
	longint lFilePos;
	longint lMaxEndPos;
	boolean bLineTooLong;
	longint lNextLinePos;
	longint lLineNumber;
	longint lLineNumberUsingCurrentBuffer;

	require(GetFileName() != "");
	require(not IsOpened());
	require(nInputChunkSize > 0);

	// Parcours du fichier
	lLineNumber = 0;
	lLongLineNumber = 0;
	lLineNumberUsingCurrentBuffer = 0;
	bOk = Open();
	if (bOk)
	{
		lEndChunk = GetFileSize();
		lBeginChunk = (GetFileSize() / nInputChunkSize) * nInputChunkSize;
		if (lBeginChunk == lEndChunk)
			lBeginChunk -= nInputChunkSize;

		// Parcours du fichier par chunk
		while (bOk and lEndChunk > 0)
		{
			// On commence l'analyse au debut du chunk
			lFilePos = lBeginChunk;

			// On la termine sur la derniere ligne commencant dans le chunk, donc dans le '\n' se trouve
			// potentiellement sur le debut de chunk suivant
			lMaxEndPos = min(lEndChunk + 1, GetFileSize());

			// On commence par rechercher un debut de ligne, sauf si on est en debut de fichier
			if (lFilePos > 0)
			{
				bOk = SearchNextLineUntil(lFilePos, lMaxEndPos, lNextLinePos);
				if (not bOk)
					break;

				// On se positionne sur le debut de la ligne suivante si elle est trouvee
				if (lNextLinePos != -1)
					lFilePos = lNextLinePos;
				// Si non trouvee, on se place en fin de chunk
				else
					lFilePos = lMaxEndPos;
			}

			// Analyse du chunk si on a y trouve un debut de ligne
			if (bOk and lFilePos < lMaxEndPos)
			{
				// Parcours du chunk par paquets de lignes
				while (bOk and lFilePos < lMaxEndPos)
				{
					// Remplissage du buffer avec des ligne
					bOk = FillOuterLinesUntil(lFilePos, lMaxEndPos, bLineTooLong);
					if (not bOk)
						break;

					// Comptage des lignes du buffer si on pas de ligne trop longue
					if (not bLineTooLong)
					{
						while (not IsBufferEnd())
						{
							SkipLine(bLineTooLong);
							lLineNumber++;
							if (bLineTooLong)
								lLongLineNumber++;
						}
						lFilePos += GetCurrentBufferSize();
						lLineNumberUsingCurrentBuffer += GetBufferLineNumber();
						assert(lLineNumber == lLineNumberUsingCurrentBuffer);
					}
					// Sinon, on se positionne sur la fin de la ligne suivante ou la a derniere
					// position
					else
					{
						assert(bLineTooLong);
						assert(GetCurrentBufferSize() == 0);
						assert(GetPositionInFile() - lFilePos > GetMaxLineLength() or
						       GetPositionInFile() == lMaxEndPos);
						lLineNumber++;
						lLongLineNumber++;
						lLineNumberUsingCurrentBuffer++;
						lFilePos = GetPositionInFile();
					}
				}
			}

			// Passage au chunk precedent
			lEndChunk = lBeginChunk;
			lBeginChunk -= nInputChunkSize;
		}

		// Fermeture
		Close();
	}

	// On retourne -1 en cas d'erreur
	if (not bOk)
	{
		lLineNumber = -1;
		lLongLineNumber = -1;
	}
	return lLineNumber;
}

boolean InputBufferedFile::TestCountExtensive()
{
	boolean bOk = true;
	boolean bDisplay = true;
	InputBufferedFile ibFile;
	StringVector svFileNames;
	ALString sFileName;
	int nFile;
	IntVector ivInputChunkSize;
	IntVector ivInputBufferSize;
	IntVector ivInputCacheOn;
	IntVector ivInputPreferredBufferSize;
	IntVector ivInputMaxLineLength;
	StringVector svMethods;
	int nInputChunkSize;
	int nInputBufferSize;
	boolean bInputCacheOn;
	int nInputPreferredBufferSize;
	int nInputMaxLineLength;
	ALString sMethod;
	int i1;
	int i2;
	int i3;
	int i4;
	int i5;
	int i6;
	longint lFileSize;
	boolean bRefBOM;
	boolean bBOMTolerance;
	longint lRefLineNumber;
	longint lLineNumber;
	longint lRefLongLineNumber;
	longint lLongLineNumber;
	longint lBeginPos;
	Timer timer;
	boolean bOneRun;

	// Choix des fichiers a analyser
	svFileNames.Add("D:\\temp\\Datasets\\Iris\\IrisUtf8Bom.txt");
	svFileNames.Add("D:\\temp\\Datasets\\Iris\\Iris.txt");
	svFileNames.Add("D:\\temp\\Datasets\\Adult\\Adult.txt");
	svFileNames.Add("D:\\temp\\Datasets\\NewYorkTimes\\NewYorkTimes.txt");

	// Affichage de l'entete
	if (bDisplay)
		cout << "File\tMax line length\tChunk\tBuffer\tCache\tPreferred size\tMethod\tLines\tLong lines\tRead "
			"call\tRead bytes\tTime\n";

	// Boucle principale d'analyse par fichier
	for (nFile = 0; nFile < svFileNames.GetSize(); nFile++)
	{
		sFileName = svFileNames.GetAt(nFile);
		lFileSize = FileService::GetFileSize(sFileName);

		// Recherche de la presence d'un BOM
		bRefBOM = false;
		ibFile.SetFileName(sFileName);
		bOk = ibFile.Open();
		if (bOk)
		{
			lBeginPos = 0;
			bOk = ibFile.FillBytes(lBeginPos);
			if (bOk)
				bRefBOM = (ibFile.GetPositionInFile() > 0);
			ibFile.Close();
		}
		ibFile.SetFileName("");

		// Reinitialisation des parametres, qui peuvcent dependre de la taille des fichier a traites
		ivInputChunkSize.SetSize(0);
		ivInputBufferSize.SetSize(0);
		ivInputCacheOn.SetSize(0);
		ivInputPreferredBufferSize.SetSize(0);
		ivInputMaxLineLength.SetSize(0);
		svMethods.SetSize(0);

		// Parametrage des longeur maximale de lignes
		if (lFileSize < 10000)
			ivInputMaxLineLength.Add(10);
		if (lFileSize < 10000000)
			ivInputMaxLineLength.Add(100);
		ivInputMaxLineLength.Add(10000);
		ivInputMaxLineLength.Add(100000);
		ivInputMaxLineLength.Add(1000000);
		ivInputMaxLineLength.Add(int(8 * lMB));
		for (i1 = 0; i1 < ivInputMaxLineLength.GetSize(); i1++)
		{
			nInputMaxLineLength = ivInputMaxLineLength.GetAt(i1);
			if (nInputMaxLineLength > lFileSize)
			{
				ivInputMaxLineLength.SetSize(i1 + 1);
				break;
			}
		}

		// Parametrage de tailles de chunk
		nInputChunkSize = 1;
		while (nInputChunkSize < 1000000000)
		{
			if (lFileSize / 10000 < nInputChunkSize and nInputChunkSize <= lFileSize)
				ivInputChunkSize.Add(nInputChunkSize);
			nInputChunkSize *= 10;
		}

		// Parametrage de tailles de buffer
		nInputBufferSize = 1;
		while (nInputBufferSize < 100000000)
		{
			if (lFileSize / 10000 < nInputBufferSize and nInputBufferSize <= lFileSize)
				ivInputBufferSize.Add(nInputBufferSize);
			nInputBufferSize *= 3;
		}

		// Parametrage de l'utilisation du cache
		ivInputCacheOn.Add(0);
		ivInputCacheOn.Add(1);

		// Parametrage de tailles preferres de buffer pour les operations elementaires de lecture/ecriture
		ivInputPreferredBufferSize.Add(0); // Pour avoir la valeur par defaut
		if (lFileSize < 10000000)
			ivInputPreferredBufferSize.Add(1000);
		ivInputPreferredBufferSize.Add(int(64 * lKB));
		if (lFileSize > 100 * lMB)
			ivInputPreferredBufferSize.Add(int(64 * lMB));

		// Parametrage des methodes a tester
		svMethods.Add("SearchNextLine");
		svMethods.Add("FillOneLine");
		svMethods.Add("FillInnerLines");
		svMethods.Add("FillOuterLines");
		svMethods.Add("FillOuterLinesBackward");

		// Analyse pour toutes les combinaisons de parametres
		lRefLineNumber = -2;
		for (i1 = 0; i1 < ivInputMaxLineLength.GetSize(); i1++)
		{
			nInputMaxLineLength = ivInputMaxLineLength.GetAt(i1);

			// On repart sur une nouvelle reference pour les lignes trop longues
			lRefLongLineNumber = -2;

			for (i2 = 0; i2 < ivInputChunkSize.GetSize(); i2++)
			{
				nInputChunkSize = ivInputChunkSize.GetAt(i2);
				for (i3 = 0; i3 < ivInputBufferSize.GetSize(); i3++)
				{
					nInputBufferSize = ivInputBufferSize.GetAt(i3);
					for (i4 = 0; i4 < ivInputCacheOn.GetSize(); i4++)
					{
						bInputCacheOn = ivInputCacheOn.GetAt(i4);
						for (i5 = 0; i5 < ivInputPreferredBufferSize.GetSize(); i5++)
						{
							nInputPreferredBufferSize =
							    ivInputPreferredBufferSize.GetAt(i5);
							for (i6 = 0; i6 < svMethods.GetSize(); i6++)
							{
								sMethod = svMethods.GetAt(i6);

								// Execution uniquement sur un seul cas
								bOneRun = false;
								if (bOneRun)
								{
									if (FileService::GetFileName(sFileName) !=
										"IrisUtf8Bom.txt" or
									    i1 != 0 or i2 != 0 or i3 != 0 or i4 != 0 or
									    i5 != 0 or i6 != 2)
										continue;
								}

								// Initialisation du fichier
								ibFile.SetFileName(sFileName);
								ibFile.SetBufferSize(nInputBufferSize);
								ibFile.SetCacheOn(bInputCacheOn);
								if (nInputPreferredBufferSize != 0)
									ibFile.nPreferredBufferSize =
									    nInputPreferredBufferSize;
								SetMaxLineLength(nInputMaxLineLength);

								// Affichage des parametres
								if (bDisplay)
								{
									cout << FileService::GetFileName(sFileName)
									     << "\t";
									cout << nInputMaxLineLength << "\t";
									cout << nInputChunkSize << "\t";
									cout << nInputBufferSize << "\t";
									cout << BooleanToString(bInputCacheOn) << "\t"
									     << flush;
									cout << ibFile.nPreferredBufferSize << "\t";
									cout << sMethod << "\t" << flush;
								}

								// Appel de la methode
								timer.Reset();
								timer.Start();
								lLineNumber = -2;
								lLongLineNumber = -2;
								if (sMethod == "SearchNextLine")
									lLineNumber =
									    ibFile.TestCountUsingSearchNextLine(
										nInputChunkSize);
								else if (sMethod == "FillOneLine")
									lLineNumber = ibFile.TestCountUsingFillOneLine(
									    nInputChunkSize, lLongLineNumber);
								else if (sMethod == "FillInnerLines")
									lLineNumber =
									    ibFile.TestCountUsingFillInnerLines(
										nInputChunkSize, lLongLineNumber);
								else if (sMethod == "FillOuterLines")
									lLineNumber =
									    ibFile.TestCountUsingFillOuterLines(
										nInputChunkSize, lLongLineNumber);
								else if (sMethod == "FillOuterLinesBackward")
									lLineNumber =
									    ibFile.TestCountUsingFillOuterLinesBackward(
										nInputChunkSize, lLongLineNumber);
								assert(lLineNumber >= -1);
								timer.Stop();

								// On restitue la longueur de ligne maximal par defaut
								SetMaxLineLength(int(8 * lMB));

								// Memorisation du resultat de reference la premiere
								// fois
								if (lRefLineNumber == -2)
									lRefLineNumber = lLineNumber;
								if (lRefLongLineNumber == -2 and lLongLineNumber != -2)
									lRefLongLineNumber = lLongLineNumber;
								bOk = bOk and lLineNumber == lRefLineNumber;
								bOk = bOk and lLongLineNumber == lRefLongLineNumber;

								// Affichage des resultats
								if (bDisplay)
								{
									bBOMTolerance =
									    bRefBOM and
									    (nInputBufferSize < nUTF8BomSize or
									     nInputChunkSize < nUTF8BomSize);
									cout << lLineNumber << "\t";
									cout << lLongLineNumber << "\t";
									cout << ibFile.GetTotalPhysicalReadCalls()
									     << "\t";
									cout << ibFile.GetTotalPhysicalReadBytes()
									     << "\t";
									cout << timer.GetElapsedTime() << "\t";
									if (abs(lLineNumber - lRefLineNumber) >
									    (bBOMTolerance ? 1 : 0))
										cout << "Error : bad line number\t";
									if (lRefLongLineNumber != -2 and
									    lLongLineNumber != -2 and
									    abs(lLongLineNumber - lRefLongLineNumber) >
										(bBOMTolerance ? 1 : 0))
										cout
										    << "Error : bad long line number\t";
									cout << endl;
								}
							}
						}
					}
				}
			}
		}
	}
	return bOk;
}

boolean InputBufferedFile::TestReadWrite(const ALString& sLabel, int nFileSize, int nFileType)
{
	ALString sFullFileName;
	boolean bOk = false;
	InputBufferedFile errorSender;
	SystemFile* fileHandle;
	FileCache fileCache;
	ALString sFileName;
	ALString sTmpDir;

	// Acces au repertoire temporaire
	if (nFileType == 2)
		sTmpDir = "hdfs:///tmp";
	else
		sTmpDir = FileService::GetTmpDir();
	if (sTmpDir.IsEmpty())
	{
		cout << "Temporary directory not found" << endl;
		return false;
	}

	// Remplissage du buffer avec une chaine aleatoire
	SetRandomSeed(0);
	fileCache.SetSize(nFileSize);
	fileCache.cvBuffer.InitWithRandomChars();

	// Creation du nom du fichier a partir de sa taille
	sFileName = "";
	sFileName += IntToString(nFileSize);
	sFileName += ".txt";

	// Nom complet du fichier
	sFullFileName = FileService::BuildFilePathName(sTmpDir, sFileName);
	cout << endl << "Generating \'" << sLabel << "\'" << endl;

	// Ecriture du fichier
	fileHandle = new SystemFile;
	bOk = fileHandle->OpenOutputFile(sFullFileName);
	if (bOk)
	{
		bOk = fileCache.WriteToFile(fileHandle, fileCache.GetSize(), &errorSender);
		if (not bOk)
			cout << "ERROR while writing " << sFullFileName << endl;
		fileHandle->CloseOutputFile(sFullFileName);
	}
	else
		cout << "ERROR while opening " << sFullFileName << " for write" << endl;

	if (bOk)
	{
		// Test en faisant varier l'overhead
		bOk = TestCopy(sFullFileName, nFileType, 19) and bOk;
		bOk = TestCopy(sFullFileName, nFileType, 7) and bOk;
	}

	// Nettoyage
	delete fileHandle;
	PLRemoteFileService::RemoveFile(sFullFileName);
	return bOk;
}

void InputBufferedFile::Reset()
{
	ResetBuffer();
	sLocalFileName = "";
	nBufferStartInCache = 0;
	nPositionInCache = 0;
	nLastBolPositionInCache = 0;
	nCurrentBufferSize = 0;
	nBufferLineNumber = 0;
	nCurrentLineIndex = 1;
	lCacheStartInFile = -1;
	bLastFieldReachEol = false;
	lFileSize = -1;
	nAllocatedBufferSize = 0;
	nCacheSize = 0;
	nUTF8BomSkippedCharNumber = 0;
}

boolean InputBufferedFile::InternalFillBytes(longint& lBeginPos, int nSizeToFill)
{
	boolean bOk;
	longint lInternalBeginPos;
	int nInternalSizeToFill;
	boolean bUTF8BomDetected;

	require(nSizeToFill >= 0);
	require(0 <= lBeginPos and lBeginPos + nSizeToFill <= lFileSize);

	// Par defaut, on ne saute pas de caractere de BOM
	nUTF8BomSkippedCharNumber = 0;

	// Cas particulier de taille vide, qui ne sert qu'a reinitialiser les variables d'etat correctement
	if (nSizeToFill == 0)
		return InternalRawFillBytes(lBeginPos, nSizeToFill);
	// Cas ou on ne gere pas le NOM UTF8
	else if (not GetUTF8BomManagement())
		return InternalRawFillBytes(lBeginPos, nSizeToFill);
	// Cas general
	else
	{
		// Parametrage de la lecture pour permettre si necessaire la detection du BOM
		lInternalBeginPos = lBeginPos;
		nInternalSizeToFill = nSizeToFill;
		if (lFileSize >= nUTF8BomSize)
		{
			if (lInternalBeginPos < nUTF8BomSize)
			{
				lInternalBeginPos = 0;
				nInternalSizeToFill += int(lBeginPos);
			}
			if (lInternalBeginPos == 0 and nInternalSizeToFill < nUTF8BomSize)
				nInternalSizeToFill = nUTF8BomSize;
		}

		// Lecture
		bOk = InternalRawFillBytes(lInternalBeginPos, nInternalSizeToFill);

		// Detection du BOM utf8
		if (bOk and lInternalBeginPos == 0 and nInternalSizeToFill >= nUTF8BomSize)
		{
			bUTF8BomDetected = DetectUTF8Bom();

			// On saute le BOM utf8 s'il est detecte, et on ajuste la taille du buffer
			if (bUTF8BomDetected)
			{
				assert(nBufferStartInCache == 0);
				assert(nPositionInCache == 0);
				nBufferStartInCache = nUTF8BomSize;
				nPositionInCache = nUTF8BomSize;
				nLastBolPositionInCache = nUTF8BomSize;
				nCurrentBufferSize = nInternalSizeToFill - nUTF8BomSize;

				// On memorise le nombre de caracteres de BOM sautes
				assert(lBeginPos < nUTF8BomSize);
				nUTF8BomSkippedCharNumber = nUTF8BomSize - int(lBeginPos);

				// Modification de la position de depart
				lBeginPos = nUTF8BomSize;
			}
			// Sinon, on se replace a l'endroit de la lecture initiale, avec la taille du buffer demandee
			else
			{
				nBufferStartInCache += int(lBeginPos);
				nPositionInCache += int(lBeginPos);
				nLastBolPositionInCache = nPositionInCache;
				nCurrentBufferSize = nSizeToFill;
			}
			ensure(not bOk or GetPositionInFile() == lBeginPos);
			ensure(not bOk or nBufferStartInCache + nCurrentBufferSize <= nCacheSize);
			ensure(not bOk or nLastBolPositionInCache == nPositionInCache);
		}
		return bOk;
	}
}

boolean InputBufferedFile::InternalRawFillBytes(longint lBeginPos, int nSizeToFill)
{
	boolean bOk = true;
	boolean bVerbose = false;
	int nYetToRead;
	int nRemainingInCache; // Quantite de donnee qui est deja contenue dans le cache (en segments entiers)
	int nDataFromCache;    // Quantite de donnee qui est deja contenue dans le cache
	int nSizeToRemove;

	require(nSizeToFill >= 0);
	require(0 <= lBeginPos and lBeginPos + nSizeToFill <= lFileSize);

	// Initialisation
	nBufferStartInCache = 0;
	nPositionInCache = 0;
	nLastBolPositionInCache = nPositionInCache;
	nCurrentBufferSize = 0;
	bLastFieldReachEol = false;
	nBufferLineNumber = 0;
	nCurrentLineIndex = 1;
	nDataFromCache = 0;
	nRemainingInCache = 0;

	if (bVerbose)
	{
		cout << "InternalFillBytes at " << LongintToReadableString(lBeginPos) << " size "
		     << LongintToReadableString(nSizeToFill) << " file size " << LongintToReadableString(lFileSize)
		     << endl;
		cout << "cache size " << LongintToReadableString(nCacheSize) << " lCacheStartInFile "
		     << LongintToReadableString(lCacheStartInFile) << " preferred size "
		     << LongintToReadableString(GetPreferredBufferSize()) << endl;
	}

	// Arret immediat si erreur
	if (bIsError)
		bOk = false;
	// Effet de bord de buffer de taille nulle
	else if (nSizeToFill == 0)
	{
		// Cas ou la position de depart est dans le cache
		if (lBeginPos >= lCacheStartInFile and lBeginPos < lCacheStartInFile + nCacheSize)
		{
			nBufferStartInCache = int(lBeginPos - lCacheStartInFile);
			nPositionInCache = nBufferStartInCache;
			nLastBolPositionInCache = nPositionInCache;
		}
		// Sinon, on passe a un cache vide
		else
		{
			lCacheStartInFile = lBeginPos;
			nCacheSize = 0;
		}
		bOk = true;
	}
	// Le debut du buffer est  dans le cache
	else if (lBeginPos >= lCacheStartInFile and lBeginPos < lCacheStartInFile + nCacheSize)
	{
		// Le buffer est entierement dans le cache
		nYetToRead = (int)min((longint)nSizeToFill, lFileSize - lBeginPos);
		if (lBeginPos + nYetToRead <= lCacheStartInFile + nCacheSize)
		{
			nBufferStartInCache = int(lBeginPos - lCacheStartInFile);
			nPositionInCache = nBufferStartInCache;
			nLastBolPositionInCache = nPositionInCache;
			if (bVerbose)
				cout << "\tBuffer is contained in cache at "
				     << LongintToReadableString(nBufferStartInCache) << endl;
		}
		// Le debut du buffer est dans le cache, mais pas la fin
		else
		{
			// Decalage du contenu du cache, qui est rempli a partir de lCacheStartInFile
			// la quantite a supprimer est donc lBeginPos - lCacheStartInFile
			nSizeToRemove = (int)(lBeginPos - lCacheStartInFile);
			nDataFromCache = nCacheSize - nSizeToRemove;
			assert(nDataFromCache >= 0);

			// On veut avoir des blocs entiers dans le cache, on arrondi au bloc inferieur
			nSizeToRemove = (nSizeToRemove / InternalGetBlockSize()) * InternalGetBlockSize();
			nRemainingInCache = nCacheSize - nSizeToRemove;
			assert(nSizeToRemove >= 0);
			assert(nRemainingInCache >= 0);

			// Decalage de segments entiers au debut du cache
			MoveLastSegmentsToHead(&fcCache.cvBuffer, nSizeToRemove / InternalGetBlockSize());

			if (bVerbose)
			{
				cout << "\tThe beginning of the buffer is contained in the cache" << endl;
				cout << "\tdata size already in cache in cache "
				     << LongintToReadableString(nDataFromCache) << endl;
				cout << "\tmoving chars from pos "
				     << LongintToReadableString(lBeginPos - lCacheStartInFile) << endl;
			}

			// Arrondi de nSizeToFill - nDataFromCache au nPreferredBufferSize superieur
			if (bCacheOn)
				nYetToRead =
				    (int)ceil((nSizeToFill - nDataFromCache) * 1.0 / GetPreferredBufferSize()) *
				    GetPreferredBufferSize();
			else
				nYetToRead = nSizeToFill - nDataFromCache;
			assert(nYetToRead >= 0);

			// La copie du fichier s'effectue a partir de la fin de l'ancien buffer
			bOk = FillCache(lCacheStartInFile + nCacheSize, nYetToRead, nRemainingInCache);

			// Le debut du cache ne correspond pas au debut du buffer car on a copie des blocs entiers
			nBufferStartInCache = (lBeginPos - lCacheStartInFile) % InternalGetBlockSize();
			nPositionInCache = nBufferStartInCache;
			nLastBolPositionInCache = nPositionInCache;

			// Le debut du cache dans le fichier ne correspond pas a lBeginPos car lBeginPos n'est pas sur
			// le debut du bloc
			lCacheStartInFile = lBeginPos - nBufferStartInCache;
		}
	}
	else
	{
		// Arrondi de nSizeToFill au nPreferredBufferSize superieur
		if (bCacheOn)
			nYetToRead = (int)ceil(nSizeToFill * 1.0 / GetPreferredBufferSize()) * GetPreferredBufferSize();
		else
			nYetToRead = nSizeToFill;
		assert(nYetToRead >= 0);

		// Le buffer n'est pas dans le cache, on le remplit
		bOk = FillCache(lBeginPos, nYetToRead, 0);
		assert(nBufferStartInCache == 0);
		assert(nPositionInCache == 0);

		// Le debut du cache dans le fichier correspond a lBeginPos
		lCacheStartInFile = lBeginPos;
	}

	// Finalisation
	if (not bOk)
	{
		nCurrentBufferSize = 0;
		bIsError = true;
	}
	else
	{
		nCurrentBufferSize = (int)min((longint)nSizeToFill, lFileSize - lBeginPos);
	}

	if (bVerbose)
		cout << "\tbuffer size " << LongintToReadableString(nCurrentBufferSize) << endl;

	if (bVerbose)
		cout << "\tFillBytes end: " << bIsError << endl;
	ensure(bOk == not bIsError);
	ensure(not bOk or GetPositionInFile() == lBeginPos);
	ensure(not bOk or nBufferStartInCache + nCurrentBufferSize <= nCacheSize);
	ensure(not bOk or nLastBolPositionInCache == nPositionInCache);
	return not bIsError;
}

boolean InputBufferedFile::FillCache(longint lFilePos, int nSizeToCopy, int nPosToCopy)
{
	int nLocalRead;
	boolean bOk = true;
	boolean bVerbose = false;
	char* sBuffer;
	int nSizeToRead;
	int nHugeBufferSize;
	int nHugeReadSize;

	require(0 <= lFilePos and lFilePos <= lFileSize);
	assert(nSizeToCopy > 0);

	// Cas particuliers ou la copie n'est pas necessaire
	if (lFilePos >= lFileSize or nSizeToCopy == 0)
		return true;

	// On ne copie pas plus que la taille du fichier
	nSizeToCopy = (int)min((longint)nSizeToCopy, lFileSize - lFilePos);

	if (GetOpenOnDemandMode())
		bOk = fileHandle->OpenInputFile(sLocalFileName);

	// Lecture dans le buffer, avec positionnement de l'indicateur fin de fichier
	if (bOk)
	{
		// Ajout de stats memoire
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(GetClassLabel() + " BasicFill Begin");

		// Reallocation du buffer selon la taille demandee
		nCacheSize = nSizeToCopy + nPosToCopy;
		nAllocatedBufferSize = nCacheSize;
		bOk = AllocateBuffer();

		// Lecture
		if (bOk)
		{
			if (bVerbose)
			{
				cout << "\tRead " << LongintToReadableString(nSizeToCopy) << " starting from "
				     << LongintToReadableString(lFilePos) << " to pos "
				     << LongintToReadableString(nPosToCopy) << endl;
			}

			// Demande d'un buffer d'au moins GetPreferredBufferSize, voire nDefaultBufferSize,
			// selon la taille a lire, pour minimiser le nombre d'allocation de HugeBufer, independamment
			// du GetPreferredBufferSize qui peut varier selon la technologie
			if (nSizeToCopy >= nDefaultBufferSize)
				// On caste en int car sinon avec gcc on a "undefined reference to
				// BufferedFile::nDefaultBufferSize "
				nHugeBufferSize = max(GetPreferredBufferSize(), (int)nDefaultBufferSize);
			else
				nHugeBufferSize = GetPreferredBufferSize();
			sBuffer = GetHugeBuffer(nHugeBufferSize);

			// Acces a la taille effective du buffer, potentiellement plus grande que ce qui a etet demande
			// s'il a ete redimmensionne par ailleurs
			nHugeBufferSize = GetHugeBufferSize();
			assert(nHugeBufferSize >= GetPreferredBufferSize());

			// Calcul de la taille a lire en multiple de GetPreferredBufferSize
			// On suppose que chaque technologie est potentiellement plus efficace avec des multitples de sa
			// GetPreferredBufferSize
			nHugeReadSize = (nHugeBufferSize / GetPreferredBufferSize()) * GetPreferredBufferSize();
			assert(nHugeReadSize > 0);

			// Boucle de lecture
			bOk = fileHandle->SeekPositionInFile(lFilePos);
			if (not bOk)
				AddError("Problem with seek in file (" + fileHandle->GetLastErrorMessage() + ")");
			while (bOk and nSizeToCopy > 0)
			{
				nSizeToRead = min(nSizeToCopy, nHugeReadSize);
				nLocalRead =
				    (int)fileHandle->Read(sBuffer, InternalGetElementSize(), (size_t)nSizeToRead);
				lTotalPhysicalReadCalls++;
				lTotalPhysicalReadBytes += nSizeToRead;
				bOk = nLocalRead != 0;
				if (bOk)
				{
					fcCache.cvBuffer.ImportBuffer(nPosToCopy, nLocalRead, sBuffer);
					nSizeToCopy -= nLocalRead;
					nPosToCopy += nLocalRead;
				}
				else
				{
					AddError("Unable to read file (" + fileHandle->GetLastErrorMessage() + ")");
				}
			}

			// Ajout de stats memoire
			if (FileService::LogIOStats())
				MemoryStatsManager::AddLog(GetClassLabel() + " BasicFill End");
		}
		if (GetOpenOnDemandMode())
		{
			bOk = fileHandle->CloseInputFile(sLocalFileName);
			if (not bOk)
				AddError("Unable to read file (" + fileHandle->GetLastErrorMessage() + ")");
		}
	}
	return bOk;
}

boolean InputBufferedFile::DetectUTF8Bom() const
{
	require(GetPositionInFile() == 0);

	// Test si encodage UTF8 avec BOM
	return (nCurrentBufferSize >= 3 and (unsigned char) fcCache.GetAt(0) == cUTF8Bom[0] and
		(unsigned char) fcCache.GetAt(1) == cUTF8Bom[1] and (unsigned char) fcCache.GetAt(2) == cUTF8Bom[2]);
}

void InputBufferedFile::WriteEolPos(const ALString& sFileName)
{
	FILE* file;
	longint lFileSize;
	longint lPos;
	char c;

	lFileSize = FileService::GetFileSize(sFileName);

	// Ouverture des fichiers
	FileService::OpenInputBinaryFile(sFileName, file);

	lPos = 0;
	while (lPos < lFileSize)
	{
		c = (char)fgetc(file);
		if (c == '\n')
			cout << lPos << endl;
		lPos++;
	}

	// Fermeture des fichiers
	if (file != NULL)
		fclose(file);
}
