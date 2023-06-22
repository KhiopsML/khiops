// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLFileConcatenater.h"
#include "SystemFile.h"
#include "SystemFileDriverCreator.h"

PLFileConcatenater::PLFileConcatenater()
{
	dProgressionBegin = 0;
	dProgressionEnd = 1;
	cSep = '\t';
	bDisplayProgression = false;
	nBufferSize = BufferedFile::nDefaultBufferSize;
}

PLFileConcatenater::~PLFileConcatenater() {}

void PLFileConcatenater::SetFileName(const ALString& sValue)
{
	sFileName = sValue;
}

ALString PLFileConcatenater::GetFileName() const
{
	return sFileName;
}

StringVector* PLFileConcatenater::GetHeaderLine()
{
	return &svHeaderLine;
}

void PLFileConcatenater::SetFieldSeparator(char cValue)
{
	cSep = cValue;
}

char PLFileConcatenater::GetFieldSeparator() const
{
	return cSep;
}

void PLFileConcatenater::SetBufferSize(int nValue)
{
	require(nValue > 0);
	nBufferSize = nValue;
}

int PLFileConcatenater::GetBufferSize() const
{
	return nBufferSize;
}

void PLFileConcatenater::SetDisplayProgression(boolean bDisplay)
{
	bDisplayProgression = bDisplay;
}

boolean PLFileConcatenater::GetDisplayProgression() const
{
	return bDisplayProgression;
}

void PLFileConcatenater::SetProgressionBegin(double dProgression)
{
	require(0 <= dProgression and dProgression <= 1);
	dProgressionBegin = dProgression;
}

double PLFileConcatenater::GetProgressionBegin() const
{
	return dProgressionBegin;
}

void PLFileConcatenater::SetProgressionEnd(double dProgression)
{
	require(0 <= dProgression and dProgression <= 1);
	dProgressionEnd = dProgression;
}

double PLFileConcatenater::GetProgressionEnd() const
{
	return dProgressionEnd;
}

void PLFileConcatenater::RemoveChunks(const StringVector* svChunkURIs) const
{
	int i;
	ALString sChunkURI;
	double dTaskPercent;

	require(dProgressionEnd > dProgressionBegin);
	require(dProgressionEnd <= 1);
	require(svChunkURIs != NULL);

	if (GetProcessId() == 0)
	{
		// Lancement des serveurs de fichiers
		PLParallelTask::GetDriver()->StartFileServers();
	}

	dTaskPercent = dProgressionEnd - dProgressionBegin;

	// Parcours de tous les fichiers
	for (i = 0; i < svChunkURIs->GetSize(); i++)
	{
		sChunkURI = svChunkURIs->GetAt(i);
		if (sChunkURI != "")
			PLRemoteFileService::RemoveFile(sChunkURI);

		// Progression
		if (bDisplayProgression)
			TaskProgression::DisplayProgression(
			    (int)ceil(100 * (dProgressionBegin + dTaskPercent * i / svChunkURIs->GetSize())));
	}
	if (bDisplayProgression)
		TaskProgression::DisplayProgression((int)ceil(100 * dProgressionEnd));

	// Arret des serveurs de fichiers
	if (GetProcessId() == 0)
		PLParallelTask::GetDriver()->StopFileServers();
}

boolean PLFileConcatenater::Concatenate(const StringVector* svChunkURIs, const Object* errorSender,
					boolean bRemoveChunks) const
{
	int nChunkIndex;
	ALString sChunkURI;
	OutputBufferedFile outputBuffer;
	double dTaskPercent;
	InputBufferedFile inputFile;
	longint lPosition;
	ALString sTmp;
	boolean bOk;
	int i;
	int nChunkSize;
	SystemFile* fileHandle;

	require(sFileName != "");
	require(svChunkURIs != NULL);
	require(not bDisplayProgression or dProgressionEnd > dProgressionBegin);
	require(not bDisplayProgression or dProgressionEnd <= 1);

	fileHandle = NULL;
	bOk = true;
	dTaskPercent = dProgressionEnd - dProgressionBegin;

	// Lancement des serveurs de fichiers si on n'est pas dans une tache
	if (GetProcessId() == 0)
	{
		PLParallelTask::GetDriver()->StartFileServers();
	}

	// Ecriture du Header en passant par la methode WriteField  de OutputBufferFile qui gere les separateur dans les
	// champs
	if (svHeaderLine.GetSize() > 0)
	{
		outputBuffer.SetBufferSize(64 * lKB);
		outputBuffer.SetFileName(sFileName);
		outputBuffer.SetFieldSeparator(cSep);
		bOk = outputBuffer.Open();
		if (bOk)
		{
			for (i = 0; i < svHeaderLine.GetSize(); i++)
			{
				if (i > 0)
					outputBuffer.Write(cSep);
				outputBuffer.WriteField(svHeaderLine.GetAt(i));
			}
			outputBuffer.WriteEOL();
			bOk = outputBuffer.Close();
		}
	}

	// Ouverture du fichier de sortie (en mode append si le header a deja ete ecrit)
	if (bOk)
	{
		fileHandle = new SystemFile;
		if (svHeaderLine.GetSize() > 0)
			bOk = fileHandle->OpenOutputFileForAppend(sFileName) and bOk;
		else
			bOk = fileHandle->OpenOutputFile(sFileName) and bOk;
	}
	nChunkIndex = 0;
	if (bOk)
	{
		// TODO reserver la taille du fichier de sortie (moins ce qui est deja ecrit pdans le header)
		// Parcours de tous les chunks
		for (nChunkIndex = 0; nChunkIndex < svChunkURIs->GetSize(); nChunkIndex++)
		{
			sChunkURI = svChunkURIs->GetAt(nChunkIndex);

			// Interruption ?
			if (not bOk or TaskProgression::IsInterruptionRequested())
				break;

			// Concatenation d'un nouveau chunk
			if (PLRemoteFileService::Exist(sChunkURI))
			{
				inputFile.SetFileName(sChunkURI);

				// Ouverture du chunk en lecture
				bOk = inputFile.Open();
				if (bOk)
				{
					if (nBufferSize > inputFile.GetFileSize())
						nChunkSize = (int)inputFile.GetFileSize();
					else
						nChunkSize = nBufferSize;
					inputFile.SetBufferSize(inputFile.FitBufferSize(nChunkSize));

					// Lecture du chunk par blocs et ecriture dans le fichier de sortie
					lPosition = 0;
					while (lPosition < inputFile.GetFileSize())
					{
						// Lecture efficace d'un buffer
						bOk = inputFile.BasicFill(lPosition);
						if (bOk)
							bOk = inputFile.fbBuffer.WriteToFile(
							    fileHandle, inputFile.GetCurrentBufferSize(), errorSender);
						else
							break;
						lPosition += inputFile.GetBufferSize();
					}

					// Fermeture du chunk
					bOk = inputFile.Close() and bOk;

					// Suppression du chunk
					if (bRemoveChunks)
						PLRemoteFileService::RemoveFile(sChunkURI);

					// Affichage de la progression en prenant en compte la progression intiale
					// dProgressionLevel et la portion de progression represente par la
					// concatenation dTaskPercent
					if (bDisplayProgression)
						TaskProgression::DisplayProgression((int)ceil(
						    100 * (dProgressionBegin +
							   dTaskPercent * (nChunkIndex + 1) / svChunkURIs->GetSize())));
				}
			}
			else
			{
				if (errorSender != NULL)
					errorSender->AddError("Missing chunk file : " + sChunkURI);
				bOk = false;
				break;
			}
		}
		if (bDisplayProgression)
			TaskProgression::DisplayProgression((int)(100 * dProgressionEnd));

		// Fermeture du fichier de sortie
		bOk = fileHandle->CloseOutputFile(sFileName) and bOk;
	}

	// Nettoyage du resultat si echec ou interruption
	if (not bOk and FileService::Exist(sFileName))
		FileService::RemoveFile(sFileName);

	// Suppression des chunks en cas d'echec
	if (not bOk and bRemoveChunks)
	{
		// On a commence le traitement au chunk nChunkIndex, on n'est pas sur qu'il ait ete detruit
		for (i = nChunkIndex; i < svChunkURIs->GetSize(); i++)
		{
			PLRemoteFileService::RemoveFile(svChunkURIs->GetAt(i));
		}
	}

	if (fileHandle != NULL)
		delete fileHandle;

	// Arret des serveurs de fichiers
	if (GetProcessId() == 0)
	{
		PLParallelTask::GetDriver()->StopFileServers();
	}

	if (PLParallelTask::GetVerbose() and not PLParallelTask::IsRunning())
	{
		AddMessage("Statistics of concatenation :");
		PLParallelTask::GetDriver()->GetIOReadingStats()->SetDescription("IO Reading");
		PLParallelTask::GetDriver()->GetIORemoteReadingStats()->SetDescription("Remote IO Reading");
		AddMessage(PLParallelTask::GetDriver()->GetIOReadingStats()->WriteString());
		AddMessage(PLParallelTask::GetDriver()->GetIORemoteReadingStats()->WriteString());
		AddMessage(sTmp + "Remote access " +
			   IntToString(PLParallelTask::GetDriver()->GetIORemoteReadingStats()->GetValueNumber()) +
			   " / " + IntToString(PLParallelTask::GetDriver()->GetIOReadingStats()->GetValueNumber()));
		PLParallelTask::GetDriver()->GetIOReadingStats()->Reset();
		PLParallelTask::GetDriver()->GetIORemoteReadingStats()->Reset();
	}

	return bOk;
}
