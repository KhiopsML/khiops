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

		// Fermreture
		fstTaskProgression.close();
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
			sLastCompletedTaskMessage = CurrentTimestamp();
			sLastCompletedTaskMessage += "\tCompleted task\t";
			sLastCompletedTaskMessage += sLastTaskMainLabel;
			if (sLastTaskLabel != "")
				sLastCompletedTaskMessage += "\t" + sLastTaskLabel;
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
			// On fabrique un message de progression, sauf si'il n'y a pas de libelle principal de tache
			// (probablement mode silencieux)
			sLastProgressionMessage = "";
			if (sLastTaskMainLabel != "")
			{
				sLastProgressionMessage = CurrentTimestamp();
				sLastProgressionMessage += "\tCurrent task\t";
				sLastProgressionMessage += sLastTaskMainLabel;
				if (sLastTaskLabel != "")
					sLastProgressionMessage += "\t" + sLastTaskLabel;
				sLastProgressionMessage += "\n";
				for (n = 0; n <= nCurrentLevel and n < nLevelNumber; n++)
				{
					sLastProgressionMessage += "\t";
					sLastProgressionMessage += IntToString(n + 1);
					sLastProgressionMessage += ".\t";
					sLastProgressionMessage += IntToString(ivProgressions.GetAt(n));
					sLastProgressionMessage += "%\t";
					sLastProgressionMessage += svMainLabels.GetAt(n);
					sLastProgressionMessage += "\n";
				}
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
			// Indicateur de depassement de la capacite de memorisation des message
			if (olLastCompletedTaskMessages.GetCount() > nMaxCompletedTaskMessageNumber)
				fstProgressionMessages << "...\n";

			// Affichage des ancien messages
			position = olLastCompletedTaskMessages.GetTailPosition();
			n = 0;
			while (position != NULL)
			{
				soMessage = cast(StringObject*, olLastCompletedTaskMessages.GetPrev(position));
				if (n < nMaxCompletedTaskMessageNumber)
					fstProgressionMessages << soMessage->GetString();
				n++;
			}

			// Affichage du message courant
			fstProgressionMessages << sLastProgressionMessage;

			// Fermeture du fichier
			fstProgressionMessages.close();
		}
	}
}