// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SystemFile.h"
#include "SystemFileDriverCreator.h"

boolean SystemFile::bAlwaysErrorOnOpen = false;
boolean SystemFile::bAlwaysErrorOnRead = false;
boolean SystemFile::bAlwaysErrorOnFlush = false;

SystemFile::SystemFile()
{
	bIsOpenForRead = false;
	bIsOpenForWrite = false;
	lRequestedExtraSize = 0;
	lReservedExtraSize = 0;
	fileHandle = NULL;
	fileDriver = NULL;
}

SystemFile::~SystemFile()
{
	ensure(not bIsOpenForRead);
	ensure(not bIsOpenForWrite);
}

boolean SystemFile::OpenInputFile(const ALString& sFilePathName)
{
	boolean bOk;
	ALString sTmp;

	require(fileHandle == NULL);
	require(not bIsOpenForRead and not bIsOpenForWrite);
	assert(lRequestedExtraSize == 0);
	assert(lReservedExtraSize == 0);

	// Recherche du driver
	fileDriver = SystemFileDriverCreator::LookupDriver(sFilePathName, this);
	if (fileDriver == NULL)
		return false;

	// Mode de test : toujours en echec
	if (bAlwaysErrorOnOpen)
	{
		errno = ECANCELED;
		return false;
	}

	// Ouverture du fichier
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] Open Begin");
	p_SetMachineLocale();
	fileHandle = fileDriver->Open(sFilePathName, 'r');
	p_SetApplicationLocale();
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] Open End");

	// Gestion de l'erreur
	bOk = fileHandle != NULL;
	if (bOk)
		bIsOpenForRead = true;
	return bOk;
}

boolean SystemFile::OpenOutputFile(const ALString& sFilePathName)
{
	boolean bOk;
	ALString sTmp;

	require(fileHandle == NULL);
	require(not bIsOpenForRead and not bIsOpenForWrite);
	assert(lRequestedExtraSize == 0);
	assert(lReservedExtraSize == 0);

	// Recherche du driver en mode ecriture
	fileDriver = LookupWriteDriver(sFilePathName, "open output file");
	if (fileDriver == NULL)
		return false;

	// Mode de test : toujours en echec
	if (bAlwaysErrorOnOpen)
	{
		errno = ECANCELED;
		return false;
	}

	// Ouverture du fichier
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] Open Begin");
	p_SetMachineLocale();
	fileHandle = fileDriver->Open(sFilePathName, 'w');
	p_SetApplicationLocale();
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] Open End");

	// Gestion de l'erreur
	bOk = fileHandle != NULL;
	if (bOk)
		bIsOpenForWrite = true;
	return bOk;
}

boolean SystemFile::OpenOutputFileForAppend(const ALString& sFilePathName)
{
	boolean bOk;
	ALString sTmp;

	require(fileHandle == NULL);
	require(not bIsOpenForRead and not bIsOpenForWrite);
	assert(lRequestedExtraSize == 0);
	assert(lReservedExtraSize == 0);

	// Recherche du driver en mode ecriture
	fileDriver = LookupWriteDriver(sFilePathName, "open output file for append");
	if (fileDriver == NULL)
		return false;

	// Mode de test : toujours en echec
	if (bAlwaysErrorOnOpen)
	{
		errno = ECANCELED;
		return false;
	}

	// Ouverture du fichier
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] Open Begin");
	p_SetMachineLocale();
	fileHandle = fileDriver->Open(sFilePathName, 'a');
	p_SetApplicationLocale();
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] Open End");

	// Gestion de l'erreur
	bOk = fileHandle != NULL;
	if (bOk)
		bIsOpenForWrite = true;
	return bOk;
}

boolean SystemFile::CloseInputFile(const ALString& sFilePathName)
{
	boolean bOk;
	ALString sTmp;

	require(fileDriver != NULL);
	require(fileHandle != NULL);
	require(bIsOpenForRead);

	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] close read Begin");
	bOk = fileDriver->Close(fileHandle);
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] close read End");

	// Nettoyage
	fileDriver = NULL;
	fileHandle = NULL;
	bIsOpenForRead = false;
	return bOk;
}

boolean SystemFile::CloseOutputFile(const ALString& sFilePathName)
{
	boolean bOk = true;
	ALString sTmp;
	require(fileDriver != NULL);
	require(not fileDriver->IsReadOnly());
	require(fileHandle != NULL);
	require(bIsOpenForWrite);

	// Gestion de la reserve
	// Assertion uniquement s'il n'y a pas eu d'erreur avant
	// En principe, on ne peut pas fermer un fichier sans avoir ecrit dans toute la place reservee
	// Attention, la place demandee (requested) ne peut etre negative, alors que la place effectivement
	// reservee (reserved) peut l'etre (car elle elle est decrementee, meme si elle n'a pas ete reservee)
	assert(lRequestedExtraSize == 0 or errno != 0);
	assert(lReservedExtraSize <= 0 or errno != 0);

	// Flush du contenu du rapport juste avant la fermeture pour detecter une erreur en ecriture
	bOk = fileDriver->Flush(fileHandle);

	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] close write Begin");
	bOk = fileDriver->Close(fileHandle) and bOk;
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] close write End");

	// Nettoyage
	fileDriver = NULL;
	lReservedExtraSize = 0;
	lRequestedExtraSize = 0;
	fileHandle = NULL;
	bIsOpenForWrite = false;
	return bOk;
}

longint SystemFile::Read(void* pBuffer, size_t size, size_t count)
{
	longint lRes;
	ALString sTmp;
	require(fileDriver != NULL);
	require(fileHandle != NULL);
	require(bIsOpenForRead);

	// Mode de test : toujours en echec
	if (bAlwaysErrorOnRead)
	{
		errno = ECANCELED;
		return 0;
	}

	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] fread Begin");
	lRes = fileDriver->Fread(pBuffer, size, count, fileHandle);
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] fread End");

	// Renvoie 0 en cas d'erreur
	if (lRes == -1)
		lRes = 0;
	return lRes;
}

boolean SystemFile::SeekPositionInFile(longint lPosition)
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

longint SystemFile::Write(const void* pBuffer, size_t size, size_t count)
{
	longint lRes;
	ALString sTmp;

	require(fileDriver != NULL);
	require(fileHandle != NULL);
	require(bIsOpenForWrite);

	// Mode de test : toujours en echec
	if (bAlwaysErrorOnFlush)
	{
		errno = ECANCELED;
		return 0;
	}

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

boolean SystemFile::Flush()
{
	boolean bRes;
	ALString sTmp;

	require(fileDriver != NULL);
	require(fileHandle != NULL);
	require(bIsOpenForWrite);

	// Mode de test : toujours en echec
	if (bAlwaysErrorOnFlush)
	{
		errno = ECANCELED;
		return false;
	}

	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] flush Begin");
	bRes = fileDriver->Flush(fileHandle);
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] flush End");
	return bRes;
}

ALString SystemFile::GetLastErrorMessage()
{
	// Le driver peut etre null dans le cas ou on ne peut pas ouvrir le fichier
	if (fileDriver == NULL)
	{
		return "file driver is missing";
	}
	return fileDriver->GetLastErrorMessage();
}

longint SystemFile::GetFileSize(const ALString& sFilePathName)
{
	longint lFileSize = 0;
	SystemFileDriver* driver;
	ALString sTmp;

	driver = SystemFileDriverCreator::LookupDriver(sFilePathName, NULL);
	if (driver != NULL)
	{
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(sTmp + "driver [" + driver->GetDriverName() + "] GetFileSize Begin");

		p_SetMachineLocale();
		lFileSize = driver->GetFileSize(sFilePathName);
		p_SetApplicationLocale();

		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(sTmp + "driver [" + driver->GetDriverName() + "] GetFileSize End");
	}
	return lFileSize;
}

boolean SystemFile::FileExists(const ALString& sFilePathName)
{
	boolean bOk = false;
	SystemFileDriver* driver;
	ALString sTmp;

	driver = SystemFileDriverCreator::LookupDriver(sFilePathName, NULL);
	if (driver != NULL)
	{
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(sTmp + "driver [" + driver->GetDriverName() + "] FileExists Begin");
		p_SetMachineLocale();
		bOk = driver->FileExists(sFilePathName);
		p_SetApplicationLocale();
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(sTmp + "driver [" + driver->GetDriverName() + "] FileExists End");
	}
	return bOk;
}

boolean SystemFile::DirExists(const ALString& sFilePathName)
{
	boolean bOk = false;
	SystemFileDriver* driver;
	ALString sTmp;

	driver = SystemFileDriverCreator::LookupDriver(sFilePathName, NULL);
	if (driver != NULL)
	{
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(sTmp + "driver [" + driver->GetDriverName() + "] DirExists Begin");
		p_SetMachineLocale();
		bOk = driver->DirExists(sFilePathName);
		p_SetApplicationLocale();
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(sTmp + "driver [" + driver->GetDriverName() + "] DirExists End");
	}
	return bOk;
}

boolean SystemFile::RemoveFile(const ALString& sFilePathName)
{
	boolean bOk = false;
	SystemFileDriver* driver;
	ALString sTmp;

	// Recherche du driver en mode ecriture
	driver = LookupWriteDriver(sFilePathName, "remove file");
	if (driver == NULL)
		return false;

	// Suppression du fichier
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + driver->GetDriverName() + "] remove Begin");
	p_SetMachineLocale();
	bOk = driver->RemoveFile(sFilePathName);
	p_SetApplicationLocale();
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + driver->GetDriverName() + "] remove End");
	return bOk;
}

boolean SystemFile::MakeDirectory(const ALString& sPathName)
{
	boolean bOk = false;
	SystemFileDriver* driver;

	// Recherche du driver en mode ecriture
	driver = LookupWriteDriver(sPathName, "make directory");
	if (driver == NULL)
		return false;

	// Creation du repertoire
	p_SetMachineLocale();
	bOk = driver->MakeDirectory(sPathName);
	p_SetApplicationLocale();
	return bOk;
}

boolean SystemFile::RemoveDirectory(const ALString& sPathName)
{
	boolean bOk = false;
	SystemFileDriver* driver;

	// Recherche du driver en mode ecriture
	driver = LookupWriteDriver(sPathName, "remove directory");
	if (driver == NULL)
		return false;

	// Creation du repertoire
	p_SetMachineLocale();
	bOk = driver->RemoveDirectory(sPathName);
	p_SetApplicationLocale();
	return bOk;
}

boolean SystemFile::MakeDirectories(const ALString& sPathName)
{
	boolean bOk = false;
	SystemFileDriver* driver;

	// Recherche du driver en mode ecriture
	driver = LookupWriteDriver(sPathName, "make directory");
	if (driver == NULL)
		return false;

	// Creation des repertoires
	p_SetMachineLocale();
	bOk = driver->MakeDirectories(sPathName);
	p_SetApplicationLocale();
	return bOk;
}

boolean SystemFile::CreateEmptyFile(const ALString& sPathName)
{
	boolean bOk = false;
	SystemFileDriver* driver;

	// Recherche du driver en mode ecriture
	driver = LookupWriteDriver(sPathName, "open output file");
	if (driver == NULL)
		return false;

	// Creation du fichier
	p_SetMachineLocale();
	bOk = driver->CreateEmptyFile(sPathName);
	p_SetApplicationLocale();
	return bOk;
}

longint SystemFile::GetDiskFreeSpace(const ALString& sPathName)
{
	longint lFileSize = 0;
	SystemFileDriver* driver;

	// Recherche du driver en mode ecriture
	driver = LookupWriteDriver(sPathName, "get disk free space");
	if (driver == NULL)
		return false;

	// Recherche de l'espace libre
	p_SetMachineLocale();
	lFileSize = driver->GetDiskFreeSpace(sPathName);
	p_SetApplicationLocale();
	return lFileSize;
}

boolean SystemFile::CopyFileFromLocal(const ALString& sSourceFilePathName, const ALString& sDestFilePathName)
{
	boolean bOk = false;
	SystemFileDriver* driver;
	ALString sTmp;

	// Recherche du driver en mode ecriture
	driver = LookupWriteDriver(sDestFilePathName, "copy file from local");
	if (driver == NULL)
		return false;

	// Copie
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + driver->GetDriverName() + "] CopyFileFromLocal Begin");

	p_SetMachineLocale();
	bOk = driver->CopyFileFromLocal(sSourceFilePathName, sDestFilePathName);
	p_SetApplicationLocale();

	if (not bOk)
		Global::AddError("File", sSourceFilePathName, sTmp + "Unable to copy file to " + driver->GetScheme());
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + driver->GetDriverName() + "] CopyFileFromLocal End");

	return bOk;
}

boolean SystemFile::CopyFileToLocal(const ALString& sSourceFilePathName, const ALString& sDestFilePathName)
{
	boolean bOk = false;
	SystemFileDriver* driver;
	ALString sTmp;

	// Recherche du driver en mode ecriture
	driver = LookupWriteDriver(sSourceFilePathName, "copy file to local");
	if (driver == NULL)
		return false;

	// Copie
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + driver->GetDriverName() + "] CopyFileToLocal Begin");

	p_SetMachineLocale();
	bOk = driver->CopyFileToLocal(sSourceFilePathName, sDestFilePathName);
	p_SetApplicationLocale();

	if (not bOk)
		Global::AddError("File", sSourceFilePathName,
				 sTmp + "Unable to copy " + sSourceFilePathName + " from " + driver->GetScheme());
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + driver->GetDriverName() + "] CopyFileToLocal End");
	return bOk;
}

void SystemFile::SetAlwaysErrorOnOpen(boolean bValue)
{
	bAlwaysErrorOnOpen = bValue;
}

boolean SystemFile::GetAlwaysErrorOnOpen()
{
	return bAlwaysErrorOnOpen;
}

void SystemFile::SetAlwaysErrorOnRead(boolean bValue)
{
	bAlwaysErrorOnRead = bValue;
}
boolean SystemFile::GetAlwaysErrorOnRead()
{
	return bAlwaysErrorOnRead;
}

void SystemFile::SetAlwaysErrorOnFlush(boolean bValue)
{
	bAlwaysErrorOnFlush = bValue;
}

boolean SystemFile::GetAlwaysErrorOnFlush()
{
	return bAlwaysErrorOnFlush;
}

SystemFileDriver* SystemFile::LookupWriteDriver(const ALString& sFilePathName, const ALString& sFunctionLabel)
{
	SystemFileDriver* driver;
	ALString sTmp;

	// Recherche du driver
	driver = SystemFileDriverCreator::LookupDriver(sFilePathName, NULL);

	// On test s'il est read-only
	if (driver != NULL)
	{
		if (driver->IsReadOnly())
		{
			Global::AddError("File", sFilePathName,
					 sTmp + driver->GetScheme() +
					     " is a read-only URI scheme and does not support '" + sFunctionLabel +
					     "' function");
			driver = NULL;
		}
	}
	return driver;
}

boolean SystemFile::ReserveExtraSize(longint lSize)
{
	require(fileDriver != NULL);
	require(fileHandle != NULL);
	require(bIsOpenForWrite);

	boolean bOk = true;
	require(lSize >= 0);

	// Mise a jour de la reserve demandee
	lRequestedExtraSize = max(lRequestedExtraSize, lSize);

	// Si la reserve est epuisee et qu'une nouvelle reserve est specifie et suffisante, on l'applique
	// (la reserve, decrementee dans chaque fwrite, peut etre negative)
	// On choisit un seuil de 1 MB, car c'est appromativement le cas ou le temps d'acces (~10 ms) est
	// du meme ordre que le temps d'ecriture (pour 100 MB/s)
	if (lReservedExtraSize <= 0 and lRequestedExtraSize >= lMB)
	{
		lReservedExtraSize = lRequestedExtraSize;
		bOk = fileDriver->ReserveExtraSize(lReservedExtraSize, fileHandle);
	}
	return bOk;
}

int SystemFile::GetPreferredBufferSize() const
{
	longint lPreferredSize;

	assert(bIsOpenForRead or bIsOpenForWrite);
	assert(fileDriver != NULL);

	// On borne par les min et max
	lPreferredSize = fileDriver->GetSystemPreferredBufferSize();
	lPreferredSize = min(lPreferredSize, (longint)SystemFile::nMaxPreferredBufferSize);
	lPreferredSize = max(lPreferredSize, (longint)SystemFile::nMinPreferredBufferSize);
	return (int)lPreferredSize;
}
