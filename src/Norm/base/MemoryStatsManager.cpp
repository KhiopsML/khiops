// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MemoryStatsManager.h"

#include "PLRemoteFileService.h"
#include "FileService.h"
#include "Ermgt.h"

boolean MemoryStatsManager::bIsOpened = false;
ALString MemoryStatsManager::sStatsLogFileName;
ALString MemoryStatsManager::sLocalFileName;
longint MemoryStatsManager::lAutomaticLogFrequency = 0;
int MemoryStatsManager::nCollectedStats = NoStats;
Timer MemoryStatsManager::timer;
int MemoryStatsManager::nStatsFieldNumber = 0;
int MemoryStatsManager::nStatsFieldIndex = 0;
FILE* MemoryStatsManager::fMemoryStats = NULL;

// Handler d'ajout d'un log sans libelle pour les traces memoires
void MemoryStatsHandlerLogWriter()
{
	MemoryStatsManager::AddLog("");
}

inline void MemoryStatsManager::WriteString(const char* sValue)
{
	if (nStatsFieldIndex > 0)
		fprintf(fMemoryStats, "\t");
	fprintf(fMemoryStats, "%s", sValue);
	nStatsFieldIndex++;
}

inline void MemoryStatsManager::WriteLongint(longint lValue)
{
	if (nStatsFieldIndex > 0)
		fprintf(fMemoryStats, "\t");
	fprintf(fMemoryStats, "%lld", lValue);
	nStatsFieldIndex++;
}

inline void MemoryStatsManager::WriteTime(double dValue)
{
	longint lTotalMicroSeconds;
	int nSeconds;
	int nMicroSeconds;

	require(dValue >= 0);

	if (nStatsFieldIndex > 0)
		fprintf(fMemoryStats, "\t");

	// Extraction des secondes et Micro-secondes
	lTotalMicroSeconds = longint(floor(dValue * 1000000));
	nSeconds = int(lTotalMicroSeconds / 1000000);
	nMicroSeconds = lTotalMicroSeconds % 1000000;
	assert(0 <= nMicroSeconds and nMicroSeconds < 1000000);

	// Ecriture des secondes et Micro-secondes
	fprintf(fMemoryStats, "%d.%06d", nSeconds, nMicroSeconds);
	nStatsFieldIndex++;
}

boolean MemoryStatsManager::OpenLogFile(const char* sFileName, longint lLogFrequency, int nStatsToCollect)
{
	boolean bOk = true;
	ALString sActualFileName;
	int nFieldId;

	require(sFileName != NULL);
	require(lLogFrequency >= 0);
	require(NoStats < nStatsToCollect and nStatsToCollect <= AllStats);
	require(not IsOpened());

	// Creation si necessaire des repertoires intermediaires
	PLRemoteFileService::MakeDirectories(FileService::GetPathName(sFileName));

	// Calcul d'un nom de fichier en fonction du procesId
	sActualFileName = sFileName;
	if (GetProcessId() != 0)
		sActualFileName = FileService::SetFilePrefix(
		    sActualFileName, FileService::GetFilePrefix(sActualFileName) + "_" + IntToString(GetProcessId()));

	// Preparation de la copie sur HDFS si necessaire
	bOk = PLRemoteFileService::BuildOutputWorkingFile(sActualFileName, sLocalFileName);

	// Ouverture du fichier en ecriture
	if (bOk)
		bOk = FileService::OpenOutputBinaryFile(sLocalFileName, fMemoryStats);

	// Initialisation si OK
	if (bOk)
	{
		// Memorisation des caracteristiques d'ouverture
		bIsOpened = true;
		sStatsLogFileName = sActualFileName;
		lAutomaticLogFrequency = lLogFrequency;
		nCollectedStats = nStatsToCollect;

		// On libere la memoire de la variable locale
		sActualFileName = "";

		// Temps de depart
		timer.Reset();
		timer.Start();

		// Calcul du nombre de champs de stats a prendre en compte
		nStatsFieldNumber = 0;
		nFieldId = 1;
		while (nFieldId < AllStats)
		{
			if (nStatsToCollect & nFieldId)
				nStatsFieldNumber++;
			nFieldId *= 2;
		}

		// Ajoute une ligne d'entete
		nStatsFieldIndex = 0;
		if (nCollectedStats & LogTime)
			WriteString("Time");
		if (nCollectedStats & HeapMemory)
			WriteString("Heap mem");
		if (nCollectedStats & MaxHeapRequestedMemory)
			WriteString("Max heap mem");
		if (nCollectedStats & TotalHeapRequestedMemory)
			WriteString("Total heap mem");
		if (nCollectedStats & AllocStats)
		{
			if (nCollectedStats & AllocNumber)
				WriteString("Alloc");
			if (nCollectedStats & MaxAllocNumber)
				WriteString("Max alloc");
			if (nCollectedStats & TotalAllocNumber)
				WriteString("Total alloc");
			if (nCollectedStats & TotalFreeNumber)
				WriteString("Total free");
			if (nCollectedStats & GrantedSize)
				WriteString("Granted");
			if (nCollectedStats & MaxGrantedSize)
				WriteString("Max granted");
			if (nCollectedStats & TotalRequestedSize)
				WriteString("Total requested size");
			if (nCollectedStats & TotalGrantedSize)
				WriteString("Total granted size");
			if (nCollectedStats & TotalFreeSize)
				WriteString("Total free size");
		}
		if (nCollectedStats & LogLabel)
			WriteString("Label");
		fprintf(fMemoryStats, "\n");

		// Parametrage du handler si necessaire
		if (lAutomaticLogFrequency > 0 or (nCollectedStats & AllocStats))
		{
			// Parametrage du handler si frequence strictement positive
			if (lAutomaticLogFrequency > 0)
				MemSetStatsHandler(MemoryStatsHandlerLogWriter, lAutomaticLogFrequency);
			// Si pas de rafraichissement, mais besoin de collecter les stats,
			// on utilise un frequence max sans handler
			else
				MemSetStatsHandler(NULL, LLONG_MAX);
		}

		// Ajout d'un premier log
		AddLog("Start memory log");
	}
	return bOk;
}

boolean MemoryStatsManager::OpenLogFileFromEnvVars(boolean bVerbose)
{
	boolean bOk = true;
	ALString sKhiopsMemStatsLogFileName;
	ALString sKhiopsMemStatsLogFrequency;
	ALString sKhiopsMemStatsLogToCollect;
	longint lLogFrequency;
	int nStatsToCollect;

	// Recherche des valeurs des variables d'environnement
	sKhiopsMemStatsLogFileName = p_getenv("KhiopsMemStatsLogFileName");
	sKhiopsMemStatsLogFrequency = p_getenv("KhiopsMemStatsLogFrequency");
	sKhiopsMemStatsLogToCollect = p_getenv("KhiopsMemStatsLogToCollect");

	// Test si variables d'environnement presentes
	bOk = bOk and sKhiopsMemStatsLogFileName != "";
	bOk = bOk and sKhiopsMemStatsLogFrequency != "";
	bOk = bOk and sKhiopsMemStatsLogToCollect != "";

	// Test si elles sont valides
	lLogFrequency = 0;
	nStatsToCollect = 0;
	if (bOk)
	{
		lLogFrequency = StringToLongint(sKhiopsMemStatsLogFrequency);
		bOk = bOk and sKhiopsMemStatsLogFrequency == LongintToString(lLogFrequency);
		bOk = bOk and lLogFrequency >= 0;
	}
	if (bOk)
	{
		nStatsToCollect = StringToInt(sKhiopsMemStatsLogToCollect);
		bOk = bOk and sKhiopsMemStatsLogToCollect == IntToString(nStatsToCollect);
		bOk = bOk and NoStats < nStatsToCollect and nStatsToCollect <= AllStats;
	}

	// Ouverture du fichier si OK
	if (bOk)
	{
		// Message en mode verbeux
		if (bVerbose)
		{
			cout << "MemoryStatsManager activated\n";
			cout << "\tKhiopsMemStatsLogFileName=" << sKhiopsMemStatsLogFileName << "\n";
			cout << "\tKhiopsMemStatsLogFrequency=" << sKhiopsMemStatsLogFrequency << "\n";
			cout << "\tKhiopsMemStatsLogToCollect=" << sKhiopsMemStatsLogToCollect << "\n";
		}

		// Ouverture du fichier
		bOk = OpenLogFile(sKhiopsMemStatsLogFileName, lLogFrequency, nStatsToCollect);
	}
	return bOk;
}

boolean MemoryStatsManager::CloseLogFile()
{
	boolean bOk = true;

	// On ferme le fichier uniquement si le fichier de log est ouvert
	if (IsOpened())
	{
		// Ajout d'un dernier log
		AddLog("Stop memory log");

		// Supression du handler
		if (lAutomaticLogFrequency > 0 or (nCollectedStats & AllocStats))
			MemSetStatsHandler(NULL, 0);

		// Fermeture du fichier
		bOk = FileService::CloseOutputBinaryFile(sLocalFileName, fMemoryStats);

		// Copie vers HDFS si necessaire
		// TODO BUG cette methode est appelee apres SystemFileDriverCreator::UnregisterDrivers();
		// donc on aura toujour bOk==false
		if (bOk)
			bOk = PLRemoteFileService::CleanOutputWorkingFile(sStatsLogFileName, sLocalFileName);

		// Reinitialisation
		bIsOpened = false;
		sStatsLogFileName = "";
		lAutomaticLogFrequency = 0;
		nCollectedStats = NoStats;
		timer.Stop();
		timer.Reset();
		nStatsFieldNumber = 0;
		nStatsFieldIndex = 0;
	}
	return bOk;
}

const ALString& MemoryStatsManager::GetLogFileName()
{
	return sStatsLogFileName;
}

longint MemoryStatsManager::GetAutomaticLogFrequency()
{
	return lAutomaticLogFrequency;
}

int MemoryStatsManager::GetCollectedStats()
{
	return nCollectedStats;
}

void MemoryStatsManager::AddLog(const char* sLabel)
{
	require(sLabel != NULL);

	// On ajoute un log uniquement si le fichier de log est ouvert
	if (IsOpened())
	{
		nStatsFieldIndex = 0;
		if (nCollectedStats & LogTime)
			WriteTime(timer.GetElapsedTime());
		if (nCollectedStats & HeapMemory)
			WriteLongint(MemGetHeapMemory());
		if (nCollectedStats & MaxHeapRequestedMemory)
			WriteLongint(MemGetMaxHeapRequestedMemory());
		if (nCollectedStats & TotalHeapRequestedMemory)
			WriteLongint(MemGetTotalHeapRequestedMemory());
		if (nCollectedStats & AllocStats)
		{
			if (nCollectedStats & AllocNumber)
				WriteLongint(MemGetAllocNumber());
			if (nCollectedStats & MaxAllocNumber)
				WriteLongint(MemGetMaxAllocNumber());
			if (nCollectedStats & TotalAllocNumber)
				WriteLongint(MemGetTotalAllocNumber());
			if (nCollectedStats & TotalFreeNumber)
				WriteLongint(MemGetTotalFreeNumber());
			if (nCollectedStats & GrantedSize)
				WriteLongint(MemGetGrantedSize());
			if (nCollectedStats & MaxGrantedSize)
				WriteLongint(MemGetMaxGrantedSize());
			if (nCollectedStats & TotalRequestedSize)
				WriteLongint(MemGetTotalRequestedSize());
			if (nCollectedStats & TotalGrantedSize)
				WriteLongint(MemGetTotalGrantedSize());
			if (nCollectedStats & TotalFreeSize)
				WriteLongint(MemGetTotalFreeSize());
		}
		if (nCollectedStats & LogLabel)
			WriteString(sLabel);
		fprintf(fMemoryStats, "\n");
	}
}
