// Copyright (c) 2023-2025 Orange. All rights reserved.
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
	bVerbose = false;
	bHeaderLineUsed = true;
}

PLFileConcatenater::~PLFileConcatenater() {}

void PLFileConcatenater::SetFileName(const ALString& sValue)
{
	sOutputFileName = sValue;
}

ALString PLFileConcatenater::GetFileName() const
{
	return sOutputFileName;
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

void PLFileConcatenater::SetHeaderLineUsed(boolean bUsed)
{
	bHeaderLineUsed = bUsed;
}

boolean PLFileConcatenater::GetHeaderLineUsed() const
{
	return bHeaderLineUsed;
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

void PLFileConcatenater::SetVerbose(boolean bValue)
{
	bVerbose = bValue;
}

boolean PLFileConcatenater::GetVerbose() const
{
	return bVerbose;
}

const ALString PLFileConcatenater::GetClassLabel() const
{
	return "file concatenater";
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

boolean PLFileConcatenater::Concatenate(const StringVector* svChunkURIs, const Object* errorSender) const
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
	SystemFile* fileHandle;
	longint lInputBufferSize;
	int nOutputBufferSize;
	int nInputPreferredSize;
	int nOutputPreferredSize;
	longint lRemainingMemory;
	longint lOutputFileSize;
	longint lChunkSize;
	ALString sField;

	require(sOutputFileName != "");
	require(svChunkURIs != NULL);
	require(not bDisplayProgression or dProgressionEnd > dProgressionBegin);
	require(not bDisplayProgression or dProgressionEnd <= 1);

	// Initialisations
	bOk = true;
	nChunkIndex = 0;
	fileHandle = NULL;
	dTaskPercent = dProgressionEnd - dProgressionBegin;

	// Lancement des serveurs de fichiers si on n'est pas dans une tache
	if (GetProcessId() == 0)
	{
		PLParallelTask::GetDriver()->StartFileServers();
	}

	// Memoire disponible sur la machine
	lRemainingMemory = RMResourceManager::GetRemainingAvailableMemory();
	if (svChunkURIs->GetSize() > 0)
		nInputPreferredSize = PLRemoteFileService::GetPreferredBufferSize(svChunkURIs->GetAt(0));
	else
		nInputPreferredSize = SystemFile::nMinPreferredBufferSize;
	nOutputPreferredSize = PLRemoteFileService::GetPreferredBufferSize(sOutputFileName);

	if (lRemainingMemory >= (longint)nInputPreferredSize + nOutputPreferredSize)
	{
		nOutputBufferSize = nOutputPreferredSize;
		lInputBufferSize = lRemainingMemory - nOutputBufferSize;

		// Ajustement de la taille a un multiple de preferred size
		if (lInputBufferSize > nInputPreferredSize)
			lInputBufferSize = (lInputBufferSize / nInputPreferredSize) * nInputPreferredSize;

		// On n'utilise pas plus de 8 preferred size
		if (lInputBufferSize > 8 * (longint)nOutputPreferredSize)
			lInputBufferSize = 8 * (longint)nOutputPreferredSize;
	}
	else
	{
		// On fait au mieux avec les ressources disponibles
		lInputBufferSize = lRemainingMemory / 2;
		nOutputBufferSize = (int)(lRemainingMemory - lInputBufferSize);
	}

	// Ajustement a des tailles raisonables
	nOutputBufferSize = InputBufferedFile::FitBufferSize(nOutputBufferSize);
	lInputBufferSize = InputBufferedFile::FitBufferSize(lInputBufferSize);

	if (bVerbose)
	{
		cout << "Available memory on the host " << LongintToHumanReadableString(lRemainingMemory) << endl;
		cout << "Dispatched for input " << LongintToHumanReadableString(lInputBufferSize) << " and output "
		     << LongintToHumanReadableString(nOutputBufferSize) << endl;
	}

	outputBuffer.SetBufferSize(nOutputBufferSize);
	outputBuffer.SetFileName(sOutputFileName);
	outputBuffer.SetFieldSeparator(cSep);
	bOk = outputBuffer.Open();

	if (bOk)
	{
		// Calcul de la taille du fichier de sortie pour reserver la taille,
		//  en commencant par la taille du header
		lOutputFileSize = 0;
		if (bHeaderLineUsed and svHeaderLine.GetSize() > 0)
		{
			for (i = 0; i < svHeaderLine.GetSize(); i++)
			{
				sField = svHeaderLine.GetAt(i);
				lOutputFileSize += sField.GetLength();
			}
			lOutputFileSize += max(0, svHeaderLine.GetSize() - 1); // Les separateurs
			lOutputFileSize += FileService::GetEOL().GetLength();  // la fin de ligne
		}

		// Calcul de la taille du fichier de sortie
		for (nChunkIndex = 0; nChunkIndex < svChunkURIs->GetSize(); nChunkIndex++)
		{
			sChunkURI = svChunkURIs->GetAt(nChunkIndex);
			lChunkSize = PLRemoteFileService::GetFileSize(sChunkURI);
			lOutputFileSize += lChunkSize;
			if (lChunkSize == 0 and not PLRemoteFileService::FileExists(sChunkURI))
			{
				bOk = false;
				if (errorSender != NULL)
					errorSender->AddError("Missing chunk file : " + sChunkURI);
				break;
			}
			if (TaskProgression::IsInterruptionRequested())
			{
				bOk = false;
				break;
			}
		}

		// Concatenation
		nChunkIndex = 0;
		if (bOk)
		{
			// Reservation de la taille du fichier de sortie
			outputBuffer.ReserveExtraSize(lOutputFileSize);

			// Ecriture du header en passant par la methode WriteField de OutputBufferFile qui gere les separateurs dans les champs.
			if (bHeaderLineUsed)
			{
				for (i = 0; i < svHeaderLine.GetSize(); i++)
				{
					if (i > 0)
					{
						outputBuffer.Write(cSep);
					}
					outputBuffer.WriteField(svHeaderLine.GetAt(i));
				}
				outputBuffer.WriteEOL();
			}

			// Parcours de tous les chunks
			for (nChunkIndex = 0; nChunkIndex < svChunkURIs->GetSize(); nChunkIndex++)
			{
				sChunkURI = svChunkURIs->GetAt(nChunkIndex);

				// Interruption utilisateur ?
				if (TaskProgression::IsInterruptionRequested())
				{
					bOk = false;
					break;
				}

				// Concatenation d'un nouveau chunk
				inputFile.SetFileName(sChunkURI);

				// Ouverture du chunk en lecture (en ignorant la gestion des BOM, car on a des fichiers internes)
				inputFile.SetUTF8BomManagement(false);
				bOk = inputFile.Open();
				if (bOk)
				{
					inputFile.SetBufferSize((int)min(lInputBufferSize, inputFile.GetFileSize()));

					// Lecture du chunk par blocs et ecriture dans le fichier de sortie
					lPosition = 0;
					while (lPosition < inputFile.GetFileSize())
					{
						// Lecture efficace d'un buffer
						bOk = inputFile.FillBytes(lPosition);
						if (bOk)
							outputBuffer.WriteSubPart(inputFile.GetCache(),
										  inputFile.GetBufferStartInCache(),
										  inputFile.GetCurrentBufferSize());
						else
							break;
						lPosition += inputFile.GetCurrentBufferSize();
					}

					// Fermeture du chunk
					bOk = inputFile.Close() and bOk;

					// Suppression du chunk
					PLRemoteFileService::RemoveFile(sChunkURI);

					// Affichage de la progression en prenant en compte la progression intiale dProgressionLevel
					// et la portion de progression represente par la concatenation dTaskPercent
					if (bDisplayProgression)
						TaskProgression::DisplayProgression((int)ceil(
						    100 * (dProgressionBegin +
							   dTaskPercent * (nChunkIndex + 1) / svChunkURIs->GetSize())));
				}
				if (not bOk)
					break;
			}
		}
		if (bDisplayProgression)
			TaskProgression::DisplayProgression((int)(100 * dProgressionEnd));

		// Fermeture du fichier de sortie
		bOk = outputBuffer.Close() and bOk;
	}

	// Nettoyage du resultat si echec ou interruption
	if (not bOk and PLRemoteFileService::FileExists(sOutputFileName))
		PLRemoteFileService::RemoveFile(sOutputFileName);

	// Suppression des chunks en cas d'echec
	if (not bOk)
	{
		// On a commence le traitement du chunk nChunkIndex, on n'est pas sur qu'il ait ete detruit
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
