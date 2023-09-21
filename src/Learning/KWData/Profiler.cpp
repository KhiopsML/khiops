// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Profiler.h"

Profiler::Profiler()
{
	bIsTrace = false;
	bIsStarted = false;
	fJsonTraceFile = NULL;
}

Profiler::~Profiler()
{
	assert(not bIsStarted);
	assert(fJsonTraceFile == NULL);
	odMethodTimers.DeleteAll();
	svMethodNames.SetSize(0);
}

void Profiler::Start(const ALString& sFileName)
{
	require(sFileName != "");
	require(not bIsStarted);
	assert(fJsonTraceFile == NULL);

	// Memorisation du nom du fichier
	bIsStarted = true;
	sProfilingStatsFileName = sFileName;

	// Ouverture du fichier de trace
	if (bIsTrace)
	{
		fJsonTraceFile = new JSONFile;
		fJsonTraceFile->SetFileName(sFileName + ".json");
		fJsonTraceFile->OpenForWrite();
	}
}

const ALString& Profiler::GetFileName() const
{
	return sProfilingStatsFileName;
}

void Profiler::Stop()
{
	fstream fstProfilingStats;
	ALString sMethodName;
	Timer* methodTimer;
	int i;
	boolean bOk;

	require(bIsStarted);
	require(odMethodTimers.GetCount() == svMethodNames.GetSize());

	// Fermeture du fichier de trace
	if (bIsTrace)
	{
		assert(fJsonTraceFile != NULL);
		fJsonTraceFile->Close();
		delete fJsonTraceFile;
		fJsonTraceFile = NULL;
	}

	// Ecriture du fichier de stats
	bOk = FileService::OpenOutputFile(sProfilingStatsFileName, fstProfilingStats);
	if (bOk)
	{
		fstProfilingStats << "Method\tCall\tMean time\tTotal time\n";
		for (i = 0; i < svMethodNames.GetSize(); i++)
		{
			sMethodName = svMethodNames.GetAt(i);
			methodTimer = cast(Timer*, odMethodTimers.Lookup(sMethodName));
			assert(methodTimer != NULL);
			assert(not methodTimer->IsStarted());

			// Affichage des stats
			fstProfilingStats << sMethodName << "\t";
			fstProfilingStats << methodTimer->GetStartNumber() << "\t";
			fstProfilingStats << methodTimer->GetMeanElapsedTime() << "\t";
			fstProfilingStats << methodTimer->GetElapsedTime() << "\n";
		}

		FileService::CloseOutputFile(sProfilingStatsFileName, fstProfilingStats);
	}

	// Nettoyage
	sProfilingStatsFileName = "";
	odMethodTimers.DeleteAll();
	svMethodNames.SetSize(0);
	dvMethodLastStopElapsedTimes.SetSize(0);
	bIsStarted = false;
}

boolean Profiler::IsStarted()
{
	return bIsStarted;
}

void Profiler::BeginMethod(const ALString& sMethodName)
{
	Timer* methodTimer;
	ALString sJsonKey;

	require(sMethodName != "");

	// Methode active uniquement si le profiling est demarre
	if (bIsStarted)
	{
		// Recherche ou creation du timer correspondant a la methode dans le dictionnaire
		methodTimer = cast(Timer*, odMethodTimers.Lookup(sMethodName));
		if (methodTimer == NULL)
		{
			methodTimer = new Timer;
			odMethodTimers.SetAt(sMethodName, methodTimer);
			svMethodNames.Add(sMethodName);
			dvMethodLastStopElapsedTimes.Add(0);
		}

		// Mise a jour des stats du timer
		methodTimer->Start();

		// Ecriture de la trace
		if (bIsTrace)
		{
			// On suffixe le nom de la methode par le nombre d'appel, d'une part a titre informatif,
			// d'autre part pour genere un fichier json valide en evitant les cles identiques
			// potentiellement dans les meme sections
			sJsonKey = sMethodName;
			sJsonKey += "[";
			sJsonKey += IntToString(methodTimer->GetStartNumber());
			sJsonKey += "]";
			fJsonTraceFile->BeginKeyObject(sJsonKey);
		}
	}
}

void Profiler::EndMethod(const ALString& sMethodName)
{
	Timer* methodTimer;
	int i = 0;
	int nMethodIndex;
	double dMethodLastElapsedTime;

	require(sMethodName != "");

	// Methode active uniquement si le profiling est demarre
	if (bIsStarted)
	{
		// Recherche du timer correspondant a la methode dans le dictionnaire
		methodTimer = cast(Timer*, odMethodTimers.Lookup(sMethodName));
		assert(methodTimer != NULL);

		// Mise a jour des stats du timer
		methodTimer->Stop();

		// Ecriture de la trace
		if (bIsTrace)
		{
			// Recherche de l'index de la methode
			// Une recherche sequentielle est ici suffisante
			nMethodIndex = -1;
			for (i = 0; i < svMethodNames.GetSize(); i++)
			{
				if (svMethodNames.GetAt(i) == sMethodName)
				{
					nMethodIndex = i;
					break;
				}
			}
			assert(nMethodIndex >= 0);

			// Calcul du temps passe dans le methode depuis son dernier start
			dMethodLastElapsedTime =
			    methodTimer->GetElapsedTime() - dvMethodLastStopElapsedTimes.GetAt(nMethodIndex);
			dvMethodLastStopElapsedTimes.SetAt(nMethodIndex, methodTimer->GetElapsedTime());
			assert(dMethodLastElapsedTime >= 0);

			// Fermeture de l'objet, en memorisant le temps passe dans la methode
			fJsonTraceFile->WriteKeyString("time", DoubleToString(dMethodLastElapsedTime));
			fJsonTraceFile->EndObject();
		}
	}
}

void Profiler::WriteKeyString(const ALString& sKey, const ALString& sValue)
{
	require(sKey != "");
	require(sValue != "");

	// Methode active uniquement si le profiling est demarre
	if (bIsStarted)
	{
		// Ecriture de la trace
		if (bIsTrace)
			fJsonTraceFile->WriteKeyString(sKey, sValue);
	}
}

void Profiler::SetTrace(boolean bTrace)
{
	require(not bIsStarted);
	bIsTrace = bTrace;
}

boolean Profiler::GetTrace() const
{
	return bIsTrace;
}
