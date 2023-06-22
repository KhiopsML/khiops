// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "Object.h"
#include "FileService.h"
#include "SystemFileDriverCreator.h"
#include "SystemFileDriver.h"

class InputBufferedFile;
class OutputBufferedFile;
class BufferedFile;
class BufferedFileDriver;
class BufferedFileDriverCreator;

///////////////////////////////////////////////////////////////////////////
// Classe BufferedFileDriver
// Driver pour les acces fichiers. Classe virtuelle pure a implementer pour
// gerer des systemes de fichiers alternatifs (file, hdfs..)
class BufferedFileDriver : public Object
{
public:
	virtual BufferedFileDriver* Create() const = 0;

	///////////////////////////////////////////////////////////////
	// Methode pour InputBufferedFile: cf doc de InputBufferedFile

	// Verification de l'existance du fichier et mise a jour de la taille
	virtual boolean OpenForRead(InputBufferedFile*) = 0;
	virtual boolean OpenForWrite(OutputBufferedFile*) = 0;
	virtual boolean OpenForWriteAppend(OutputBufferedFile*) = 0;

	virtual boolean Close() = 0;

	virtual boolean Flush(OutputBufferedFile*) = 0;

	// Alimentation d'un buffer en se callant sur les lignes entieres
	// En cas d'erreur l'attribut bIsError de InputBufferedFile est mis a true
	virtual void Fill(InputBufferedFile*, longint lBeginPos, boolean& bSkippedLine, longint& lRealBeginPos) = 0;
	virtual longint FindEOL(InputBufferedFile*, longint lBeginPos, boolean& bEolFound) = 0;

	// Alimentation du buffer simple
	virtual void Read(InputBufferedFile*, longint lBeginPos) = 0;

	// Renvoie false si la ligne est trop grande
	// met a jour le buffer en cas d'erreur
	virtual boolean FillWithHeaderLine(InputBufferedFile*) = 0;

	// Test d'existence d'un fichier (ou d'un directory)
	virtual boolean Exist(const ALString& sFileURI) = 0;

	// Taille d'un fichiers en bytes
	// Renvoie 0 si probleme d'acces au fichier
	virtual longint GetFileSize(const ALString& sFileURI) = 0;

	// Supprime le fichier
	virtual boolean RemoveFile(const ALString& sFileURI) = 0;

	// Cree un fichier vide avec ecrasement si necessaire
	// Pas d'emission de message d'erreurs
	virtual boolean CreateEmptyFile(const ALString& sFilePathName) = 0;

	// Creation d'un repertoire
	// Indique en sortie si le repertoire existe, sans message d'erreur
	virtual boolean MakeDirectory(const ALString& sPathName) = 0;

	// Creation d'un repertoire et si necessaire de tous les repertoire intermediaires
	virtual boolean MakeDirectories(const ALString& sPathName) = 0;

	//  Copie le fichier distant source (REMOTE) vers la destination locale (STD ou HDFS)
	virtual boolean CopyFile(const ALString& sSourceURI, const ALString& sDestURI) = 0;

	// Recherche de la memoire disque disponible sur un repertoire
	// (se base sur le repertoire courant "." si le repertoire en parametre est "")
	// Renvoie 0 si erreur ou pas de place disponible
	virtual longint GetDiskFreeSpace(const ALString& sPathName) = 0;
};

///////////////////////////////////////////////////////////////////////////
// Classe BufferedFileDriverCreator
// Classe qui cree des drivers de fichiers (HDFS ou distants)
// TODO etudier cette classe, a priori on n'a plus besoinde HDFS ici,
// il n'y a donc plus qu'un seul type de fichiers a gerer : les fichiers distants, est-ce qu'on a besoin d'un
// DriverCreator du coup ?
class BufferedFileDriverCreator : public Object
{
public:
	// Creation d'un driver adapte a l'URI passee en parametre
	// La detection de la technologie est automatique et basse sur les schema de l'URI
	BufferedFileDriver* CreateBufferedFileDriver(const ALString& sURIFilePathName, const Object* errorSender);

	// Renvoie true si le fichier est de type STD ou
	// ou si c'est un fichier distant avec un hostname local (sauf si  bRemoteIsNeverLocal)
	boolean IsLocal(const ALString& sURIFilePathName) const;

	// Acces aux drivers, ils appartiennent a l'appelant
	static void SetDriverRemote(BufferedFileDriver*);
	static BufferedFileDriver* GetDriverRemote();

protected:
	// Lorsque ce boolean est a true, les fichiers REMOTE dont le host est localhost ne sont pas consideres comme
	// "local" Cf. methode IsLocal()
	const boolean bRemoteIsNeverLocal = true;

	// Drivers pour les fichiers distants et HDFS
	// La liste des types d'URI gerees correspond a celle definies par FileService::IsURITypeManaged
	// On la gere "en dur" plutot que de facon generique, car leur nombre ne va pas augmenter
	static BufferedFileDriver* driverRemote;
};

inline BufferedFileDriver* BufferedFileDriverCreator::CreateBufferedFileDriver(const ALString& sURIFilePathName,
									       const Object* errorSender)
{
	ALString sScheme;
	BufferedFileDriver* driver;
	ALString sMessage;

	driver = NULL;

	// Verification que l'URI a une forme attendue
	if (not FileService::IsURIWellFormed(sURIFilePathName))
	{
		sMessage = "the file URI is not well-formed (" + sURIFilePathName + ")";
		if (errorSender != NULL)
			errorSender->AddError(sMessage);
		else
			Global::AddError("", "", sMessage);
		return NULL;
	}

	// Choix du driver
	sScheme = FileService::GetURIScheme(sURIFilePathName);
	if (sScheme == "")
		assert(false);
	else if (sScheme == FileService::sRemoteScheme)
	{
		assert(driverRemote != NULL);
		driver = driverRemote;
	}

	// Si aucun driver n'est trouve c'est que le schema de l'URI n'est pas connu
	if (driver == NULL)
	{
		sMessage = "there is no driver avalaible for the URI '" + sScheme + "://'";
		if (errorSender != NULL)
			errorSender->AddError(sMessage);
		else
			Global::AddError("", "", sMessage);
		return NULL;
	}

	return driver->Create();
}