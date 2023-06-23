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
	boolean Write(const ALString& sValue);
	boolean Write(const CharVector* cvValue);
	boolean Write(const char* sValue);
	boolean Write(const char* sValue, int nCharNumber);
	boolean Write(char c);
	boolean WriteEOL();

	// Ecriture d'une sous-partie d'un buffer, a partir d'un point de depart
	boolean WriteSubPart(const CharVector* cvValue, int nBeginOffset, int nLength);

	// Ecriture dans le buffer du contenu d'un champ entier
	// Tout champ contenant un separateur de champ ou un double-quote est entoure de doubles-quotes
	// Dans ce cas, les double-quotes sont doubles
	boolean WriteField(const char* sValue);

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Methode technique, reserve un espace sur le dique pour le fichier ouvert ce qui permet
	// d'eviter de la fragmentation lors de l'ecriture de ce fichier
	void ReserveExtraSize(longint lSize);

	////////////////////////////////////////////////////////////////////////
	// Statistiques sur les ecritures physiques
	// Ces statistiques sont reinitialisees apres chaque ouverture du fichier
	// et disponibles en permanence, y compris apres la fermeture du fichier

	// Nombre total de ecritures physiques
	longint GetTotalPhysicalWriteCalls() const;

	// Nombre total d'octets lus
	longint GetTotalPhysicalWriteBytes() const;

	///////////////////////////////////////////////////////////////////////////////////////////
	// Test de la classe

	// Methode de test
	// Lecture du fichier d'entree et ecriture du fichier de sortie en utilisant plusieurs tailles de buffer
	// Les fichiers sont compares pour chaque taille de buffer
	// En entree STD et HDFS, en sortie STD et HDFS
	static void TestWriteFile(const ALString& sInputFileURI, const ALString& sOutputFileURI, boolean bOpenOnDemand);

	// Methode avancee pour avoir acces au cache en read-only
	const CharVector* GetCache() const;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Ecriture du contenu du buffer dans le fichier
	// Peut provoquer une erreur
	// Lorsque la taille du cache est plus grande que GetPreferredBufferSize,
	// seule un multiple de GetPreferredBufferSize est ecrit, le reste a ecrire est recopier
	// au debut du cache
	virtual boolean FlushCache();

	// Ecriture des premiers caracteres du cache par tranche de GetPreferredBufferSize
	// Ouvre le fichier si necessaire (mode OpenOnDemand) mais ne le ferme pas
	// Met a jour bIsError en cas d'erreur
	boolean WriteToFile(int nSizeToWrite);

	// Espace non rempli dans le buffer
	int GetAvailableSpace();

	// Ouverture et fermeture physiques du fihcier
	boolean IsPhysycalOpen() const;
	boolean PhysicalOpen();
	boolean PhysicalClose();

	// Est-ce que le fichier est physiquement ouvert ?
	boolean bIsPhysicalOpen;

	// Utilise dans le mode OpenOnDemand : est-ce que la prochaien ouverture doit etre en append ?
	// Lors des ouvertures successives on utilse le Open en mode append.C'est lors de la premiere
	// ouverture que c'est delicat car il faut ouvrir en append seulement si c'est la methode
	// OpenForAppend qui a ete utilisee.
	// On commence avec bNextOpenOnAppend=false, il passe a true dans le PhysycalOpen et dans le OpenForAppend
	boolean bNextOpenOnAppend;

	///////////////////////////////////////////
	// Statistiques sur les ecritures physiques

	// Nombre total de ecritures physiques
	longint lTotalPhysicalWriteCalls;

	// Nombre total d'octets lus
	longint lTotalPhysicalWriteBytes;
};

///////////////////////
/// Methodes en inline

inline boolean OutputBufferedFile::Write(char c)
{
	require(IsOpened());
	require(c != '\0');

	// Si le vecteur est plein, on le vide
	if (GetAvailableSpace() == 0)
	{
		FlushCache();
		if (GetOpenOnDemandMode())
			PhysicalClose();
	}
	if (not IsError())
	{
		fcCache.SetAt(nCurrentBufferSize, c);
		nCurrentBufferSize++;
	}
	return not bIsError;
}

inline boolean OutputBufferedFile::WriteEOL()
{
#ifndef __UNIX__
	Write('\r');
#endif // __UNIX__
	Write('\n');
	return not bIsError;
}

inline boolean OutputBufferedFile::Write(const char* sValue)
{
	Write(sValue, (int)strlen(sValue));
	return not bIsError;
}

inline boolean OutputBufferedFile::Write(const ALString& sValue)
{
	Write(sValue, sValue.GetLength());
	return not bIsError;
}

inline boolean OutputBufferedFile::Write(const CharVector* cvValue)
{
	WriteSubPart(cvValue, 0, cvValue->GetSize());
	return not bIsError;
}

inline boolean OutputBufferedFile::WriteField(const char* sValue)
{
	int nLength;
	boolean bWithDoubleQuote;
	char c;

	require(sValue != NULL);
	require(sValue != GetHugeBufferAdress());

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

			// Doublement si necessaire du double-quote
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
	return not bIsError;
}

inline int OutputBufferedFile::GetAvailableSpace()
{
	return nBufferSize - nCurrentBufferSize;
}

inline boolean OutputBufferedFile::IsPhysycalOpen() const
{
	return bIsPhysicalOpen;
}
