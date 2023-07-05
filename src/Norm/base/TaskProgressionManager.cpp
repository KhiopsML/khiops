// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "TaskProgressionManager.h"

/////////////////////////////////////////////////////////////////////////
// Classe FileTaskProgressionManager

int FileTaskProgressionManager::nMaxCompletedTaskMessageNumber = 20;
double FileTaskProgressionManager::dMinInterMessageSeconds = 5.0;

FileTaskProgressionManager::FileTaskProgressionManager()
{
	bStarted = false;
	nLevelNumber = 0;
	nCurrentLevel = -1;
	nTaskIndex = 1;
	bPrintProgressionInConsole = false;
}

FileTaskProgressionManager::~FileTaskProgressionManager()
{
	svMainLabels.SetSize(0);
	olLastCompletedTaskMessages.DeleteAll();
}

void FileTaskProgressionManager::SetTaskProgressionFileName(const ALString& sFileName)
{
	fstream fstTaskProgression;

	// Ouverture du fichier si necessaire pour reinitialiser le contenu du fichier
	sTaskProgressionFileName = sFileName;
	if (sTaskProgressionFileName != "")
	{
		// Creation si necessaire des repertoires intermediaires
		FileService::MakeDirectories(FileService::GetPathName(sTaskProgressionFileName));

		// Fermeture du fichier si celui-ci est deja ouvert
		// Ce cas peut arriver si on appelle plusieurs fois ParseParameters (notamment via MODL_dll)
		if (fstTaskProgression.is_open())
			fstTaskProgression.close();

		// Ouverture du fichier en ecriture
		// Ici, on ne passe pas par la classe FileService pour ne pas
		// entrainer une boucle entre FileService et Global
		p_SetMachineLocale();
		fstTaskProgression.open(sTaskProgressionFileName, ios::out);
		p_SetApplicationLocale();

		// Message d'erreur si probleme d'ouverture
		if (not fstTaskProgression.is_open())
		{
			Global::AddError("File", sTaskProgressionFileName, "Unable to open task progression file");
		}

		// Fermeture
		fstTaskProgression.close();

		// Sur Linux, si le fichier de progression est stdout ou stderr, l'affichage est dans la console
		// On ajoutera un prefixe a chaque ligne
#ifdef __linux_or_apple__
		if (sFileName == "/dev/stdout" or sFileName == "/dev/stderr")
			bPrintProgressionInConsole = true;
#endif
	}
}

const ALString& FileTaskProgressionManager::GetTaskProgressionFileName() const
{
	return sTaskProgressionFileName;
}

void FileTaskProgressionManager::Start()
{
	bStarted = true;
	tLastProgressionMessage.Reset();
}

boolean FileTaskProgressionManager::IsInterruptionRequested()
{
	return false;
}

void FileTaskProgressionManager::Stop()
{
	bStarted = false;
	tLastProgressionMessage.Reset();
}

void FileTaskProgressionManager::SetLevelNumber(int nValue)
{
	require(nValue >= 0);

	// Nettoyage
	svMainLabels.SetSize(0);
	ivProgressions.SetSize(0);

	// Initialisation a la bonne taille
	nLevelNumber = nValue;
	svMainLabels.SetSize(nLevelNumber);
	ivProgressions.SetSize(nLevelNumber);
}

void FileTaskProgressionManager::SetCurrentLevel(int nValue)
{
	ALString sLastCompletedTaskMessage;

	nCurrentLevel = nValue;

	// Detection de fin de tache
	if (nCurrentLevel == -1)
	{
		// Memorisation d'un message de fin de tache, sauf si elle n'a pas de libelle principal (mode silencieux
		// probablement)
		if (sLastTaskMainLabel != "")
		{
			// Ajout d'un prefix si affichage dans la console
			if (bPrintProgressionInConsole)
				sLastCompletedTaskMessage = sLinePrefix + "\t";
			sLastCompletedTaskMessage += CurrentTimestamp();
			sLastCompletedTaskMessage += "\tCompleted task\t";
			sLastCompletedTaskMessage += IntToString(nTaskIndex);
			sLastCompletedTaskMessage += "\t";
			sLastCompletedTaskMessage += sLastTaskMainLabel;
			sLastCompletedTaskMessage += "\t";
			sLastCompletedTaskMessage += sLastTaskLabel;

			// Ajout de tabulations pour en avoir le meme nombre sur chaque ligne
			sLastCompletedTaskMessage += "\t\t\t\t";
			sLastCompletedTaskMessage += "\n";
			AddCompletedTaskMessage(sLastCompletedTaskMessage);
		}

		// Nettoyage
		sLastProgressionMessage = "";
		sLastTaskMainLabel = "";
		sLastTaskLabel = "";

		// Affichage des messages de progression
		WriteProgressionMessages();

		// Reinitialisation du timer
		tLastProgressionMessage.Reset();

		nTaskIndex++;
	}
}

void FileTaskProgressionManager::SetTitle(const ALString& sValue) {}

void FileTaskProgressionManager::SetMainLabel(const ALString& sValue)
{
	//  Memorisation du libelle pour une tache principale
	if (nCurrentLevel == 0 and sValue != "")
		sLastTaskMainLabel = sValue;

	//  Memorisation du libelle
	if (0 <= nCurrentLevel and nCurrentLevel < nLevelNumber)
		svMainLabels.SetAt(nCurrentLevel, sValue);
}

void FileTaskProgressionManager::SetLabel(const ALString& sValue)
{
	//  Memorisation du libelle secondaire pour une tache principale
	if (nCurrentLevel == 0 and sValue != "")
		sLastTaskLabel = sValue;
}

void FileTaskProgressionManager::SetProgression(int nValue)
{
	int n;
	int i;

	//  Memorisation de la progression
	if (0 <= nCurrentLevel and nCurrentLevel < nLevelNumber)
		ivProgressions.SetAt(nCurrentLevel, nValue);

	// Message synthetique
	if (bStarted)
	{
		// Arret du timer
		if (tLastProgressionMessage.IsStarted())
			tLastProgressionMessage.Stop();

		// Message si assez de temps ecoule depuis le dernier message
		if (tLastProgressionMessage.GetElapsedTime() > dMinInterMessageSeconds)
		{
			// On fabrique un message de progression, sauf s'il n'y a pas de libelle principal de tache
			// (probablement mode silencieux)
			sLastProgressionMessage = "";
			if (sLastTaskMainLabel != "")
			{
				if (bPrintProgressionInConsole)
					sLastProgressionMessage = sLinePrefix + "\t";
				sLastProgressionMessage += CurrentTimestamp();
				sLastProgressionMessage += "\tCurrent task\t";
				sLastProgressionMessage += IntToString(nTaskIndex);
				sLastProgressionMessage += "\t";
				sLastProgressionMessage += sLastTaskMainLabel;
				sLastProgressionMessage += "\t";
				sLastProgressionMessage += sLastTaskLabel;

				// Affichage de l'avancement (on affiche 2 barres de progression au max)
				for (n = 0; n <= nCurrentLevel and n < nLevelNumber and n < 2; n++)
				{
					sLastProgressionMessage += "\t";
					sLastProgressionMessage += IntToString(ivProgressions.GetAt(n));
					sLastProgressionMessage += "%\t";
					sLastProgressionMessage += svMainLabels.GetAt(n);
				}

				// Ajout de tabulations pour qu'il y en ait toujours le meme nombre par ligne
				for (i = 2; i > n; i--)
				{
					sLastProgressionMessage += "\t\t";
				}
				sLastProgressionMessage += "\n";
			}

			// Affichage des messages de progression
			WriteProgressionMessages();

			// Reinitialisation du timer
			tLastProgressionMessage.Reset();
		}

		// Redemarage du timer
		tLastProgressionMessage.Start();
	}
}

boolean FileTaskProgressionManager::IsInterruptionResponsive() const
{
	return true;
}

void FileTaskProgressionManager::AddCompletedTaskMessage(const ALString& sMessage)
{
	StringObject* soMessage;

	// Reutilisation du dernier du message le plus ancien si liste trop grande
	if (olLastCompletedTaskMessages.GetCount() > nMaxCompletedTaskMessageNumber + 1)
		soMessage = cast(StringObject*, olLastCompletedTaskMessages.RemoveTail());
	// Sinon, creation d'un nouveau message
	else
		soMessage = new StringObject;

	// Initialisation et memorisation d'un nouveau message
	soMessage->SetString(sMessage);
	olLastCompletedTaskMessages.AddHead(soMessage);
}

void FileTaskProgressionManager::WriteProgressionMessages()
{
	fstream fstProgressionMessages;
	POSITION position;
	StringObject* soMessage;
	int n;

	// Traitement du message si fichier parametre
	if (sTaskProgressionFileName != "")
	{
		// Ouverture du fichier de messages de progressions
		// On ne passe pas par FileService pour ne pas generer trop de messages d'erreur
		p_SetMachineLocale();
		fstProgressionMessages.open(sTaskProgressionFileName, ios::out);
		p_SetApplicationLocale();

		// Si fichier ouvert
		if (fstProgressionMessages.is_open())
		{
			if (sStartTimeStamp == "")
				sStartTimeStamp = CurrentTimestamp();

			// Indentification du debut du bloc de progression
			fstProgressionMessages << "\n";
			if (bPrintProgressionInConsole)
				fstProgressionMessages << sLinePrefix << "\t";
			fstProgressionMessages << sStartTimeStamp;
			fstProgressionMessages << "\tprogression_start\t\t\t\t\t\t\t\n";

			// Affichage des anciens messages (les nMaxCompletedTaskMessageNumber plus recents)
			// On part de la fin pour afficher les plus ancien en premier, en se limitant au nombre max
			// specifie
			position = olLastCompletedTaskMessages.GetTailPosition();
			n = olLastCompletedTaskMessages.GetCount();
			while (position != NULL)
			{
				soMessage = cast(StringObject*, olLastCompletedTaskMessages.GetPrev(position));
				if (n < nMaxCompletedTaskMessageNumber)
					fstProgressionMessages << soMessage->GetString();
				n--;
			}

			// Affichage du message courant
			fstProgressionMessages << sLastProgressionMessage;
			if (bPrintProgressionInConsole)
				fstProgressionMessages << sLinePrefix << "\t";
			fstProgressionMessages << CurrentTimestamp();
			fstProgressionMessages << "\tprogression_stop\t\t\t\t\t\t\t\n";

			// Fermeture du fichier
			fstProgressionMessages.close();
		}
	}
}