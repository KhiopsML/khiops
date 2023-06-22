// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SystemFile.h"
#include "SystemFileDriverCreator.h"

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

	require(fileDriver == NULL);
	require(not bIsOpenForRead and not bIsOpenForWrite);
	assert(lRequestedExtraSize == 0);
	assert(lReservedExtraSize == 0);

	// Recherche du driver
	fileDriver = SystemFileDriverCreator::LookupDriver(sFilePathName, this);
	if (fileDriver == NULL)
		return false;

	// Test si nom de fichier renseigne
	bOk = (strcmp(sFilePathName, "") != 0);
	if (not bOk)
		Global::AddError("File", sFilePathName, "Unable to open file (missing file name)");
	// Tentative d'ouverture du fichier
	else
	{
		p_SetMachineLocale();
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] Open Begin");
		fileHandle = fileDriver->Open(sFilePathName, 'r');
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] Open End");
		p_SetApplicationLocale();
		bOk = fileHandle != NULL;
		if (not bOk)
			Global::AddError("File", sFilePathName,
					 sTmp + "Unable to open file (" + fileDriver->GetLastErrorMessage() + ")");
		else
			bIsOpenForRead = true;
	}

	// Nettoyage en cas d'erreur
	if (not bOk)
	{
		assert(not bIsOpenForRead);
		fileDriver = NULL;
		fileHandle = NULL;
	}
	return bOk;
}

boolean SystemFile::OpenOutputFile(const ALString& sFilePathName)
{
	boolean bOk;
	ALString sTmp;

	require(fileDriver == NULL);
	require(fileHandle == NULL);
	require(not bIsOpenForRead and not bIsOpenForWrite);
	assert(lRequestedExtraSize == 0);
	assert(lReservedExtraSize == 0);

	// Recherche du driver en mode ecriture
	fileDriver = LookupWriteDriver(sFilePathName, "open output file");
	if (fileDriver == NULL)
		return false;

	// Le nom du fichier doit etre present
	bOk = sFilePathName != "";
	if (not bOk)
		Global::AddError("File", sFilePathName, "Unable to open output file (missing file name)");
	// Tentative d'ouverture du fichier
	else
	{
		p_SetMachineLocale();
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] Open Begin");

		fileHandle = fileDriver->Open(sFilePathName, 'w');
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] Open End");

		p_SetApplicationLocale();
		bOk = fileHandle != NULL;
		if (not bOk)
			Global::AddError("File", sFilePathName,
					 sTmp + "Unable to open output file (" + fileDriver->GetLastErrorMessage() +
					     ")");
		else
			bIsOpenForWrite = true;
	}

	// Nettoyage en cas d'erreur
	if (not bOk)
	{
		assert(not bIsOpenForWrite);
		fileDriver = NULL;
		fileHandle = NULL;
	}
	return bOk;
}

boolean SystemFile::OpenOutputFileForAppend(const ALString& sFilePathName)
{
	boolean bOk;
	ALString sTmp;

	require(fileDriver == NULL);
	require(fileHandle == NULL);
	require(not bIsOpenForRead and not bIsOpenForWrite);
	assert(lRequestedExtraSize == 0);
	assert(lReservedExtraSize == 0);

	// Recherche du driver en mode ecriture
	fileDriver = LookupWriteDriver(sFilePathName, "open output file for append");
	if (fileDriver == NULL)
		return false;

	// Le nom du fichier doit etre present
	bOk = sFilePathName != "";
	if (not bOk)
		Global::AddError("File", sFilePathName, "Unable to open output file for append (missing file name)");
	// Tentative d'ouverture du fichier
	else
	{
		p_SetMachineLocale();
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] Open Begin");

		fileHandle = fileDriver->Open(sFilePathName, 'a');
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] Open End");

		p_SetApplicationLocale();
		bOk = fileHandle != NULL;
		if (not bOk)
			Global::AddError("File", sFilePathName,
					 sTmp + "Unable to open output file for append (" +
					     fileDriver->GetLastErrorMessage() + ")");
		else
			bIsOpenForWrite = true;
	}

	// Nettoyage en cas d'erreur
	if (not bOk)
	{
		assert(not bIsOpenForWrite);
		fileDriver = NULL;
		fileHandle = NULL;
	}
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

	// Gestionde la reserve
	if (lReservedExtraSize > 0)
	{
		// Message d'erreur uniquement s'il n'y a pas eu d'erreur avant
		// En release, on n'emet pas jamais de message d'erreur, car ce cas peut se produire
		// legitimement en cas d'interruption de tache
		debug(if (bOk) Global::AddError("File", sFilePathName,
						"Physical error when closing file (remaining reserved size)"));
	}

	// Flush du contenu du rapport juste avant la fermeture pour detecter une erreur en ecriture
	bOk = fileDriver->flush(fileHandle);
	if (not bOk)
		Global::AddError("File", sFilePathName,
				 sTmp + "Physical error when writing data to file (" +
				     fileDriver->GetLastErrorMessage() + ")");

	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + fileDriver->GetDriverName() + "] close write Begin");
	fileDriver->Close(fileHandle);
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

longint SystemFile::GetFileSize(const ALString& sFilePathName)
{
	longint lFileSize = 0;
	SystemFileDriver* driver;
	ALString sTmp;
	driver = SystemFileDriverCreator::LookupDriver(sFilePathName, NULL);
	if (driver != NULL)
	{
		p_SetMachineLocale();
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(sTmp + "driver [" + driver->GetDriverName() + "] GetFileSize Begin");

		lFileSize = driver->GetFileSize(sFilePathName);
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(sTmp + "driver [" + driver->GetDriverName() + "] GetFileSize End");

		p_SetApplicationLocale();
	}
	return lFileSize;
}

boolean SystemFile::Exist(const ALString& sFilePathName)
{
	boolean bOk = false;
	SystemFileDriver* driver;
	ALString sTmp;
	driver = SystemFileDriverCreator::LookupDriver(sFilePathName, NULL);
	if (driver != NULL)
	{
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(sTmp + "driver [" + driver->GetDriverName() + "] Exist Begin");
		p_SetMachineLocale();
		// TODO le setMachineLocale est fait ici est mais il est aussi fait dans FileService
		bOk = driver->Exist(sFilePathName);
		p_SetApplicationLocale();
		if (FileService::LogIOStats())
			MemoryStatsManager::AddLog(sTmp + "driver [" + driver->GetDriverName() + "] Exist End");
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
	if (not bOk)
		Global::AddError("file", "", sTmp + "Unable to copy file to " + driver->GetScheme());
	p_SetApplicationLocale();
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
	if (not bOk)
		Global::AddError("file", "", sTmp + "Unable to copy file from " + driver->GetScheme());
	p_SetApplicationLocale();
	if (FileService::LogIOStats())
		MemoryStatsManager::AddLog(sTmp + "driver [" + driver->GetDriverName() + "] CopyFileToLocal End");
	return bOk;
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
					 sTmp + driver->GetScheme() + " is a read-only scheme and does not support '" +
					     sFunctionLabel + "' function");
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
	// On choisit un seul de 1 MB, car c'est appromativement le cas ou le temps d'acces (~10 ms) est
	// du meme ordre que le temps d'ecriture (pour 100 MB/s)
	if (lReservedExtraSize <= 0 and lRequestedExtraSize >= lMB)
	{
		lReservedExtraSize = lRequestedExtraSize;
		bOk = fileDriver->ReserveExtraSize(lReservedExtraSize, fileHandle);
	}
	return bOk;
}