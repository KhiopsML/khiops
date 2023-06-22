// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLTracer.h"

PLTracer::PLTracer()
{
	bSynchronous = true;
	bTimeDecoration = true;
	bActive = false;
	bShortDescription = false;
}

PLTracer::~PLTracer() {}

void PLTracer::CopyFrom(const PLTracer* source)
{
	bActive = source->bActive;
	bTimeDecoration = source->bTimeDecoration;
	bSynchronous = source->bSynchronous;
	sFileName = source->sFileName;
	bShortDescription = source->bShortDescription;
}

void PLTracer::SetActiveMode(boolean bValue)
{
	bActive = bValue;
}

void PLTracer::SetTimeDecorationMode(boolean bValue)
{
	bTimeDecoration = bValue;
}

void PLTracer::SetSynchronousMode(boolean bValue)
{
	bSynchronous = bValue;
}

void PLTracer::AddTrace(const ALString& sTrace)
{
	AddTraceAsString(sTrace);
}

void PLTracer::AddTrace(const ALString& sTrace, boolean ret)
{
	if (ret)
		AddTraceAsString(sTrace);
	else
		AddTraceAsString(sTrace + " -> KO <-");
}

void PLTracer::AddTraceWithDuration(const ALString& sTrace, double dElapsedTime)
{
	AddTraceAsString(sTrace + '\t' + SecondsToString(dElapsedTime));
}

void PLTracer::PrintTracesToFile() const
{
	int i;
	fstream fst;
	boolean bOk;
	ALString sDirectoryPath;

	require(not sFileName.IsEmpty());
	require(not bSynchronous);

	// Construction du repertoire si necessaire
	sDirectoryPath = FileService::GetPathName(sFileName);
	if (not FileService::IsDirectory(sDirectoryPath))
		FileService::MakeDirectories(sDirectoryPath);

	// Ouverture du fichier
	bOk = FileService::OpenOutputFile(sFileName, fst);

	// Si OK ecrtiture des traces
	if (bOk)
	{
		// Affichage de chaque trace
		for (i = 0; i < traces.GetSize(); i++)
		{
			fst << traces.GetAt(i) << "\n";
		}
		FileService::CloseOutputFile(sFileName, fst);
	}
	else
		AddError("Unable to open log file" + sFileName);
}

void PLTracer::SetFileName(const ALString& sLogFileName)
{
	require(not sLogFileName.IsEmpty());

	if (FileService::Exist(sLogFileName))
	{
		FileService::RemoveFile(sLogFileName);
	}
	sFileName = sLogFileName;
}

void PLTracer::PrintTraces() const
{
	int i;

	require(not bSynchronous);

	for (i = 0; i < traces.GetSize(); i++)
	{
		cout << traces.GetAt(i) << endl;
	}
}

void PLTracer::Clean()
{
	traces.SetSize(0);
}