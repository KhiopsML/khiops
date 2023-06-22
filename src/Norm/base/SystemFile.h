// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "ALString.h"
#include "SystemFileDriver.h"
#include "MemoryStatsManager.h"

///////////////////////////////////////////////////////////////////////////
// Classe SystemFile
// Classe d'acces a un systeme de fichiers. C'est un wrapper leger vers un driver de fichier (classe SystemFileDriver)
// Les instances de cette classe sont crees par la classe SystemFileDriverCreator
class SystemFile : public Object
{
public:
	// Constructeur
	SystemFile();
	~SystemFile();

	/////////////////////////////////////////////////////////
	// Methodes d'acces aux fichiers

	// Ouverture
	boolean OpenInputFile(const ALString& sFilePathName);
	boolean OpenOutputFile(const ALString& sFilePathName);
	boolean OpenOutputFileForAppend(const ALString& sFilePathName);

	// Fermeture
	boolean CloseInputFile(const ALString& sFilePathName);
	boolean CloseOutputFile(const ALString& sFilePathName);

	// Renvoie le nombre d'octets lus, 0 si erreur ou fin de fichier
	longint Read(void* pBuffer, size_t size, size_t count);

	// Positionnement dans un fichier ouvert en lecture
	boolean SeekPositionInFile(longint lPosition);

	// Renvoie le nombre d'octets ecrits, 0 si il y a eu une erreur
	longint Write(const void* pBuffer, size_t size, size_t count);

	// Flush les donnees, renvoie true si OK
	boolean Flush();

	// Reserve un espace sur le disque pour le fichier ouvert
	// pour eviter de fragmenter le disque lors de l'ecriture de ce fichier
	// Renvoie true en cas de succes
	boolean ReserveExtraSize(longint lSize);

	// Renvoie le dernier message d'erreur
	ALString GetLastErrorMessage();

	// Service generaux de getsion des fichiers et repertoires
	static longint GetFileSize(const ALString& sFilePathName);
	static boolean Exist(const ALString& sFilePathName);
	static boolean RemoveFile(const ALString& sFilePathName);
	static boolean MakeDirectory(const ALString& sPathName);
	static boolean RemoveDirectory(const ALString& sPathName);
	static boolean MakeDirectories(const ALString& sPathName);
	static boolean CreateEmptyFile(const ALString& sPathName);
	static longint GetDiskFreeSpace(const ALString& sPathName);

	// Copie du systeme de fichier courant vers le systeme de fichier standard et inversement
	static boolean CopyFileFromLocal(const ALString& sSourceFilePathName, const ALString& sDestFilePathName);
	static boolean CopyFileToLocal(const ALString& sSourceFilePathName, const ALString& sDestFilePathName);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Recherche du driver pour une fonction de type read-write
	// On rend le driver si tout est ok
	// S'il n'y a pas de driver ou si le driver est read-only, on renvoie NULL avec un message d'erreur
	static SystemFileDriver* LookupWriteDriver(const ALString& sFilePathName, const ALString& sFunctionLabel);

	// Gestion du fichier, principalement son driver et son handle
	SystemFileDriver* fileDriver;
	void* fileHandle;
	boolean bIsOpenForRead;
	boolean bIsOpenForWrite;

	// Reserve physique disponible
	longint lReservedExtraSize;

	// Reserve physique demandee, non encore reservee
	longint lRequestedExtraSize;
};

inline longint SystemFile::Read(void* pBuffer, size_t size, size_t count)
{
	longint lRes;
	ALString sTmp;
	require(fileDriver != NULL);
	require(fileHandle != NULL);
	require(bIsOpenForRead);
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] fread Begin");
	lRes = fileDriver->Fread(pBuffer, size, count, fileHandle);
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] fread End");
	return lRes;
}

inline boolean SystemFile::SeekPositionInFile(longint lPosition)
{
	boolean bRes;
	ALString sTmp;

	require(fileDriver != NULL);
	require(fileHandle != NULL);
	require(bIsOpenForRead);
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] fseek Begin");

	bRes = fileDriver->SeekPositionInFile(lPosition, fileHandle);
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] fseek End");

	return bRes;
}

inline longint SystemFile::Write(const void* pBuffer, size_t size, size_t count)
{
	longint lRes;
	ALString sTmp;

	require(fileDriver != NULL);
	require(fileHandle != NULL);
	require(bIsOpenForWrite);

	// Mise a jour des informations sur la reserve
	// Ce n'est pas la peine de remettre la reserve a zero si elle est negative,
	// car toute nouvelle reserve ecrasera necessairement l'etat actuel
	lReservedExtraSize -= count * size;
	lRequestedExtraSize -= count * size;
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] fwrite Begin");

	lRes = fileDriver->Fwrite(pBuffer, size, count, fileHandle);
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] fwrite End");

	return lRes;
}

inline boolean SystemFile::Flush()
{
	boolean bRes;
	ALString sTmp;

	require(fileDriver != NULL);
	require(fileHandle != NULL);
	require(bIsOpenForWrite);
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] flush Begin");
	bRes = fileDriver->Flush(fileHandle);
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] flush End");
	return bRes;
}

inline ALString SystemFile::GetLastErrorMessage()
{
	require(fileDriver != NULL);
	require(fileHandle != NULL);
	return fileDriver->GetLastErrorMessage();
}