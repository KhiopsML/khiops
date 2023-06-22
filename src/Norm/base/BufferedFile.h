// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "FileBuffer.h"
#include "FileService.h"
#include "BufferedFileDriver.h"
#include "SystemFileDriver.h"

/////////////////////////////////////////////////////////////////////////////////
// Fichier avec buffer potentiellement de grande taille
// Classe virtuelle ancetre des classes de fichiers en lecture ou ecriture
// Il s('agit de fichiers textes, avec gestion de ligne d'entete et de champs sur chaque ligne
class BufferedFile : public Object
{
public:
	// Constructeur
	BufferedFile();
	~BufferedFile();

	// Copie des specifications (hors fichier et buffer)
	void CopyFrom(const BufferedFile* bufferedFile);

	// Nom du fichier
	virtual void SetFileName(const ALString& sValue);
	const ALString& GetFileName() const;

	// Utilisation d'une ligne d'entete: par defaut true
	// Non utilise directement: memorise pour ceux qui en ont besoin
	void SetHeaderLineUsed(boolean bValue);
	boolean GetHeaderLineUsed() const;

	// Separateur de champs utilise (par defaut: '\t')
	// Utilise pour les methodes de type GetNextField
	void SetFieldSeparator(char cValue);
	char GetFieldSeparator() const;

	// Methode d'ouverture/fermeture
	virtual boolean Open() = 0;
	virtual boolean Close() = 0;
	virtual boolean IsOpened() const;

	// Test si erreur grave
	// Positionne suite a une erreur grave de lecture ou ecriture, reinitialise uniquement a la fermeture du fichier
	boolean IsError() const;

	// Taille du buffer (par defaut 8 MB)
	virtual void SetBufferSize(int nValue);
	int GetBufferSize() const;

	// Taille du contenu du buffer
	int GetCurrentBufferSize() const;

	// Adapte la taille allouee a la taille du buffer
	void PruneBuffer();

	// Nettoyage du buffer pour gagner de la memoire, si le buffer n'est pas ouvert
	void CleanBuffer();

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Methode avancee pour avoir acces au buffer en read-only
	const CharVector* GetBuffer() const;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation

	// Acces au generateur de driver fichiers
	// Le createur de driver renverra selon le cas un driver local ou remote
	static BufferedFileDriverCreator* GetFileDriverCreator();

	static const int nDefaultBufferSize = 8 * lMB;

protected:
	// Allocation du buffer selon la taille demandee, avec retaillage uniquement si necessaire (et uniquement en
	// agrandissement) en preservant le contenu precedent Renvoie true si OK Renvoie false sinon, avec
	// positionnnement du IsError, reinitialisation du buffer a vide et emisssion d'un message d'erreur
	boolean AllocateBuffer();

	// Taille a allouer au buffer (par defaut, redirection vers GetBufferSize)
	// Permet de differencier la specification de la taille alloue (Set/GetBufferSize)
	// de la taille effectivement allouee
	virtual int GetAllocatedBufferSize() const;
	virtual void SetAllocatedBufferSize(int nValue);

	// Reinitialisation du buffer a vide
	void ResetBuffer();

	///////////////////////////////////////////////////////////////////////////////
	// Declaration des variables d'instance par taille decroissante pour optimiser
	// la taille memoire de l'objet (cf. alignement des variables pour l'OS)

	// Nom du fichier
	ALString sFileName;

	// Buffer de lecture / ecriture
	FileBuffer fbBuffer;

	// Fichier systeme (soit ANSI, soit HDFS)
	SystemFile* fileHandle;

	// Driver pour les fichiers: local ou remote
	BufferedFileDriver* fileDriver;

	// Taille  du buffer
	int nBufferSize;

	// Taille du contenu du buffer
	int nCurrentBufferSize;

	// fichier ouvert
	boolean bIsOpened;

	// Indicateur d'erreur
	boolean bIsError;

	// Utilisation d'une ligne d'en-tete
	boolean bHeaderLineUsed;

	// Separateur de champ
	char cFieldSeparator;

	// Createur des driver de fichiers
	static BufferedFileDriverCreator fileDriverCreator;

	// Methodes privees tres techniques
	// Mise a disposition des attributs protected aux classes friends
	int InternalGetAllocSize() const;
	static int InternalGetBlockSize();
	static int InternalGetElementSize();
	char* InternalGetMonoBlockBuffer() const;
	char* InternalGetMultiBlockBuffer(int i) const;
	int InternalGetBufferSize() const;

	friend class PLRemoteFileService;
};

// Methode en inline

inline void BufferedFile::SetFieldSeparator(char cValue)
{
	cFieldSeparator = cValue;
}

inline char BufferedFile::GetFieldSeparator() const
{
	return cFieldSeparator;
}

inline boolean BufferedFile::IsOpened() const
{
	return bIsOpened;
}

inline int BufferedFile::GetBufferSize() const
{
	return nBufferSize;
}

inline void BufferedFile::SetBufferSize(int nValue)
{
	require(nValue >= 0);
	nBufferSize = nValue;
}

inline boolean BufferedFile::IsError() const
{
	return bIsError;
}

inline int BufferedFile::GetAllocatedBufferSize() const
{
	return GetBufferSize();
}

inline void BufferedFile::SetAllocatedBufferSize(int nValue)
{
	SetBufferSize(nValue);
}

inline int BufferedFile::InternalGetAllocSize() const
{
	return fbBuffer.InternalGetAllocSize();
}

inline int BufferedFile::InternalGetBlockSize()
{
	return FileBuffer::InternalGetBlockSize();
}

inline int BufferedFile::InternalGetElementSize()
{
	return FileBuffer::InternalGetElementSize();
}

inline char* BufferedFile::InternalGetMonoBlockBuffer() const
{
	return fbBuffer.InternalGetMonoBlockBuffer();
}

inline char* BufferedFile::InternalGetMultiBlockBuffer(int i) const
{
	return fbBuffer.InternalGetMultiBlockBuffer(i);
}

inline int BufferedFile::InternalGetBufferSize() const
{
	return fbBuffer.InternalGetBufferSize();
}

inline BufferedFileDriverCreator* BufferedFile::GetFileDriverCreator()
{
	return &fileDriverCreator;
}

inline int BufferedFile::GetCurrentBufferSize() const
{
	return nCurrentBufferSize;
}
