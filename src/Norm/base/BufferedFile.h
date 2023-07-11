// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "FileCache.h"
#include "FileService.h"
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

	// Ouverture a la demande, methode a utiliser lorsque le nombre de fichiers a lire/ecrire simultanement depasse
	// le nombre maximum autorise par le systeme (GetMaxOpenedFileNumber). Dans ce mode, l'ouverture et la fermeture
	// sont effectuees automatiquement lors des acces disque et non lors de l'appel au methodes Open() et Close().
	// La methode IsOpened() renverra true meme si le fichier n'est pas physiquement ouvert.
	// Par defaut : false
	void SetOpenOnDemandMode(boolean bValue);
	boolean GetOpenOnDemandMode();

	// Test si erreur grave
	// Positionne suite a une erreur grave de lecture ou ecriture, reinitialise uniquement a la fermeture du fichier
	boolean IsError() const;

	// Taille du buffer (par defaut 8 MB)
	virtual void SetBufferSize(int nValue);
	int GetBufferSize() const;

	// Taille du contenu du buffer
	int GetCurrentBufferSize() const;

	// Taille preferee du buffer de fichier (cf. PLRemoteFileService)
	// Taiile reactualise des que le nom de fichier est specifie (0 si non specifie)
	// Cette taille est utilisee si possible pour les lectures/ecritures physiques dans le fichier
	int GetPreferredBufferSize() const;

	// Adapte la taille allouee a la taille du buffer
	void PruneBuffer();

	// Nettoyage du buffer pour gagner de la memoire, si le buffer n'est pas ouvert
	void CleanBuffer();

	// Mode silencieux, pour inhiber tout affichage de message (defaut: false)
	void SetSilentMode(boolean bValue);
	boolean GetSilentMode() const;

	// Redefinition des methodes de gestion des erreurs pour tenir compte du mode d'affichage
	void AddSimpleMessage(const ALString& sLabel) const override;
	void AddMessage(const ALString& sLabel) const override;
	void AddWarning(const ALString& sLabel) const override;
	void AddError(const ALString& sLabel) const override;

	// Memoire utilisee
	longint GetUsedMemory() const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation

	// Taille par defaut recommendee
	// Taille minimale pour gerer les fichiers de tres grande taille
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

	// Deplace tous les segments a partir de nSegmentIndex vers le debut
	// Les segments qui etaient au debut sont copies a la fin pour que toutes les adresses soient valides
	void MoveLastSegmentsToHead(CharVector* cv, int nSegmentIndex);

	///////////////////////////////////////////////////////////////////////////////
	// Declaration des variables d'instance par taille decroissante pour optimiser
	// la taille memoire de l'objet (cf. alignement des variables pour l'OS)

	// Nom du fichier
	ALString sFileName;

	// Buffer de lecture / ecriture
	FileCache fcCache;

	// Fichier systeme (soit ANSI, soit HDFS)
	SystemFile* fileHandle;

	// Taille  du buffer
	int nBufferSize;

	// Taille du contenu du buffer
	int nCurrentBufferSize;

	// Taille preferee du buffer
	int nPreferredBufferSize;

	// fichier ouvert
	boolean bIsOpened;

	// Mode verbeux
	boolean bSilentMode;

	// Mode d'ouverture a la demande : l'ouverture et la fermeture sont
	// effectues lors des acces disque
	boolean bOpenOnDemandMode;

	// Indicateur d'erreur
	boolean bIsError;

	// Utilisation d'une ligne d'en-tete
	boolean bHeaderLineUsed;

	// Separateur de champ
	char cFieldSeparator;

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

inline void BufferedFile::SetOpenOnDemandMode(boolean bValue)
{
	bOpenOnDemandMode = bValue;
}

inline boolean BufferedFile::GetOpenOnDemandMode()
{
	return bOpenOnDemandMode;
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
	return fcCache.InternalGetAllocSize();
}

inline int BufferedFile::InternalGetBlockSize()
{
	return FileCache::InternalGetBlockSize();
}

inline int BufferedFile::InternalGetElementSize()
{
	return FileCache::InternalGetElementSize();
}

inline char* BufferedFile::InternalGetMonoBlockBuffer() const
{
	return fcCache.InternalGetMonoBlockBuffer();
}

inline char* BufferedFile::InternalGetMultiBlockBuffer(int i) const
{
	return fcCache.InternalGetMultiBlockBuffer(i);
}

inline int BufferedFile::InternalGetBufferSize() const
{
	return fcCache.InternalGetBufferSize();
}

inline int BufferedFile::GetCurrentBufferSize() const
{
	return nCurrentBufferSize;
}

inline int BufferedFile::GetPreferredBufferSize() const
{
	ensure(nPreferredBufferSize > 0 or GetFileName() == "");
	return nPreferredBufferSize;
}

inline void BufferedFile::SetSilentMode(boolean bValue)
{
	bSilentMode = bValue;
}

inline boolean BufferedFile::GetSilentMode() const
{
	return bSilentMode;
}
