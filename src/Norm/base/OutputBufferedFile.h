// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "BufferedFile.h"
#include "PLRemoteFileService.h"
#include "Timer.h"

// Classe qui permet d'ecrire dans un fichier.
// Les donnees sont bufferisees avant ecriture pour minimiser l'acces au fichier.
// l'ecriture est effectivement realisee apres un Flush()
class OutputBufferedFile : public BufferedFile
{
public:
	// Constructeur
	OutputBufferedFile();
	~OutputBufferedFile();

	// Copie des specifications: principalement: nom du fichier, header line, separator (hors fichier et buffer)
	void CopyFrom(const OutputBufferedFile* bufferedFile);

	// Ouverture du fichier
	boolean Open() override;
	virtual boolean OpenForAppend();

	// fermeture du fichier (precede d'un flush)
	boolean Close() override;

	// Ecriture dans le buffer et ecriture dans le fichier si depassement de la taille du buffer
	void Write(const ALString& sValue);
	void Write(const CharVector* cvValue);
	void Write(const char* sValue);
	void Write(const char* sValue, int nCharNumber);
	void Write(char c);
	void WriteEOL();

	// Ecriture d'une sous-partie d'un buffer, a partir d'un point de depart
	void WriteSubPart(const CharVector* cvValue, int nBeginOffset, int nLength);

	// Ecriture dans le buffer du contenu d'un champ entier
	// Tout champ contenant un separateur de champ ou un double-quote est entoure de doubles-quotes
	// Dans ce cas, les double-quotes sont doubles
	void WriteField(const char* sValue);

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Methode technique, reserve un espace sur le dique pour le fichier ouvert ce qui permet
	// d'eviter de la fragmentation lors de l'ecriture de ce fichier
	void ReserveExtraSize(longint lSize);

	// Methode de test
	// Lecture du fichier d'entree et ecriture du fichier de sortie en utilisant plusieurs tailles de buffer
	// En entree STD et HDFS, en sortie STD et HDFS
	static void TestWriteFile(const ALString& sInputFileURI, const ALString& sOutputFileURI);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Ecriture du contenu du buffer dans le fichier
	// Peut provoquer une erreur
	virtual void Flush();

	// Espace non rempli dans le buffer
	int GetAvailableSpace();
};

///////////////////////
/// Methodes en inline

inline int OutputBufferedFile::GetAvailableSpace()
{
	return nBufferSize - nCurrentBufferSize;
}

inline void OutputBufferedFile::Write(char c)
{
	require(IsOpened());
	require(c != '\0');

	// Si le vecteur est plein, on le vide
	if (GetAvailableSpace() == 0)
	{
		Flush();
		if (IsError())
			return;
	}
	fbBuffer.SetAt(nCurrentBufferSize, c);
	nCurrentBufferSize++;
}

inline void OutputBufferedFile::WriteEOL()
{
#ifndef __UNIX__
	Write('\r');
#endif // __UNIX__
	Write('\n');
}

inline void OutputBufferedFile::Write(const char* sValue)
{
	Write(sValue, (int)strlen(sValue));
}

inline void OutputBufferedFile::Write(const ALString& sValue)
{
	Write(sValue, sValue.GetLength());
}

inline void OutputBufferedFile::Write(const CharVector* cvValue)
{
	WriteSubPart(cvValue, 0, cvValue->GetSize());
}

inline void OutputBufferedFile::WriteField(const char* sValue)
{
	int nLength;
	boolean bWithDoubleQuote;
	char c;

	// Recherche de la longueur du champ, et de la necessite de mettre des doubles-quotes
	// Seulement si contient des separateur, ou si premier caractere est un double-quote
	nLength = 0;
	c = sValue[0];
	bWithDoubleQuote = (c == '"');
	while (c != '\0')
	{
		if (c == cFieldSeparator)
			bWithDoubleQuote = true;
		nLength++;
		c = sValue[nLength];
	}
	assert(nLength == (int)strlen(sValue));

	// Ecriture avec double-quotes
	if (bWithDoubleQuote)
	{
		Write('"');
		nLength = 0;
		c = sValue[0];
		while (c != '\0')
		{
			Write(c);
			if (c == '"')
				Write('"');
			nLength++;
			c = sValue[nLength];
		}
		Write('"');
	}
	// Ecriture sans double-quotes
	else
		Write(sValue, nLength);
}
