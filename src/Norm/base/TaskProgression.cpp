// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "TaskProgression.h"

boolean TaskProgression::bIsStarted = false;
boolean TaskProgression::bSilentMode = false;
double TaskProgression::dMaxTaskTime = 0;
int TaskProgression::nDisplayedLevelNumber = 1;
int TaskProgression::nMaxDisplayedLevelNumber = 20;
int TaskProgression::nCurrentLevel = -1;
ALString TaskProgression::sTitle = "";
clock_t TaskProgression::tLastDisplayTime = 0;
StringVector TaskProgression::svLastDisplayedMainLabels;
IntVector TaskProgression::ivLastDisplayedProgressions;
StringVector TaskProgression::svLastDisplayedLabels;
StringVector TaskProgression::svLastMainLabels;
IntVector TaskProgression::ivLastProgressions;
StringVector TaskProgression::svLastLabels;
clock_t TaskProgression::tLastInterruptionTest = 0;
boolean TaskProgression::bIsInterruptionRequested = false;
longint TaskProgression::lInterruptionRequestNumber = 0;
longint TaskProgression::lInterruptionRequestIndex = 0;
clock_t TaskProgression::tStartRequested = 0;
const double TaskProgression::dMinElapsedTime = 0.3;
boolean TaskProgression::bIsManagerStarted = false;
TaskProgressionManager* TaskProgression::currentManager = NULL;
FileTaskProgressionManager* TaskProgression::currentFileManager = NULL;
FileTaskProgressionManager TaskProgression::fileManager;

void TaskProgression::Start()
{
	require(not bIsStarted);
	require(nCurrentLevel == -1);

	// Initialisation des affichages par niveau
	Initialize();
	tLastDisplayTime = 0;
	svLastMainLabels.Initialize();
	ivLastProgressions.Initialize();
	svLastLabels.Initialize();
	tLastInterruptionTest = 0;
	bIsInterruptionRequested = false;

	// Initialisation de la gestion du demarage differe
	if (currentManager != NULL or currentFileManager != NULL)
	{
		tStartRequested = clock();
		bIsManagerStarted = false;

		// Initialisation du manager
		if (currentManager != NULL)
		{
			// Parametrage initial du manager courant
			currentManager->SetTitle(sTitle);
			currentManager->SetLevelNumber(nDisplayedLevelNumber);
		}
		if (currentFileManager != NULL)
		{
			currentFileManager->SetTitle(sTitle);
			currentFileManager->SetLevelNumber(nDisplayedLevelNumber);
		}
	}
	bIsStarted = true;
}

boolean TaskProgression::IsStarted()
{
	return bIsStarted;
}

boolean TaskProgression::IsInterruptionRequested()
{
	clock_t tTime;
	double dElapsedTime;

	// Incrementation du nombre de demandes d'interruptions
	lInterruptionRequestNumber++;

	// Arret si le nombre correspond a l'index de demande d'interruption
	if (lInterruptionRequestNumber == lInterruptionRequestIndex)
		bIsInterruptionRequested = true;

	// Bufferisation du resultat: on renvoie true si la tache est interrompue
	if (bIsInterruptionRequested)
		return true;

	// Cas standard: test dans le cas d'un manager
	// On ignore les fileManager, qui ne gerent pas les interruptions
	if (bIsStarted and currentManager != NULL)
	{
		// Test de temps max de tache ecoule
		if (dMaxTaskTime > 0)
		{
			tTime = clock();
			dElapsedTime = ((double)(tTime - tStartRequested)) / CLOCKS_PER_SEC;
			if (dElapsedTime > dMaxTaskTime)
				return true;
		}

		// Test d'interruption utilisateur
		DifferedStart();
		if (bIsManagerStarted)
		{
			tTime = clock();
			dElapsedTime = ((double)(tTime - tLastInterruptionTest)) / CLOCKS_PER_SEC;
			if (tLastInterruptionTest == 0 or dElapsedTime > dMinElapsedTime)
			{
				tLastInterruptionTest = tTime;
				bIsInterruptionRequested = currentManager->IsInterruptionRequested();
				return bIsInterruptionRequested;
			}
			else
				return false;
		}
		else
			return false;
	}
	else
		return false;
}

void TaskProgression::Stop()
{
	require(bIsStarted);
	require(nCurrentLevel == -1);

	if (currentManager != NULL)
		currentManager->Stop();
	if (currentFileManager != NULL)
		currentFileManager->Stop();

	// Reinitialisation des variables d'etat
	tStartRequested = 0;
	bIsManagerStarted = false;
	bIsStarted = false;
	tLastInterruptionTest = 0;
	bIsInterruptionRequested = false;

	// Reinitialisation des libelles
	sTitle = "";
	svLastMainLabels.Initialize();
	ivLastProgressions.Initialize();
	svLastLabels.Initialize();

	// Reinitialisation des parametres generaux
	nDisplayedLevelNumber = 1;
	dMaxTaskTime = 0;
}

void TaskProgression::SetDisplayedLevelNumber(int nValue)
{
	require(not bIsStarted);
	require(1 <= nValue and nValue < nMaxDisplayedLevelNumber);

	nDisplayedLevelNumber = nValue;
}

int TaskProgression::GetDisplayedLevelNumber()
{
	return nDisplayedLevelNumber;
}

void TaskProgression::SetSilentMode(boolean bValue)
{
	require(not bIsStarted);

	bSilentMode = bValue;
}

boolean TaskProgression::GetSilentMode()
{
	return bSilentMode;
}

void TaskProgression::SetMaxTaskTime(double dValue)
{
	require(not bIsStarted);
	require(dValue >= 0);

	dMaxTaskTime = dValue;
}

double TaskProgression::GetMaxTaskTime()
{
	return dMaxTaskTime;
}

void TaskProgression::SetTitle(const ALString& sValue)
{
	sTitle = sValue;
	if ((currentManager != NULL or currentFileManager != NULL) and not bSilentMode)
	{
		DifferedStart();
		if (currentManager != NULL)
			currentManager->SetTitle(sValue);
		if (currentFileManager != NULL)
			currentFileManager->SetTitle(sValue);
	}
}

const ALString& TaskProgression::GetTitle()
{
	return sTitle;
}

void TaskProgression::BeginTask()
{
	require(nCurrentLevel >= -1);

	nCurrentLevel++;
	if (currentManager != NULL or currentFileManager != NULL)
	{
		DifferedStart();
		if (currentManager != NULL)
			currentManager->SetCurrentLevel(nCurrentLevel);
		if (currentFileManager != NULL)
			currentFileManager->SetCurrentLevel(nCurrentLevel);
	}
	CleanLabels();
}

void TaskProgression::EndTask()
{
	require(nCurrentLevel >= 0);

	// Pour une fin de tache de niveau 0, on met le LastDisplayTime a 0
	// pour forcer un reaffichage a vide de la barre d'avancement
	// Sinon, il peut de passer pas mal de temps avant que le debut de la tache suivante
	// efface ce qui reste de la tache en cours
	if (nCurrentLevel == 0)
		tLastDisplayTime = 0;
	CleanLabels();
	nCurrentLevel--;
	if (currentManager != NULL or currentFileManager != NULL)
	{
		DifferedStart();
		if (currentManager != NULL)
			currentManager->SetCurrentLevel(nCurrentLevel);
		if (currentFileManager != NULL)
			currentFileManager->SetCurrentLevel(nCurrentLevel);
	}
}

void TaskProgression::CleanLabels()
{
	clock_t tTime;
	double dElapsedTime;

	require(nCurrentLevel >= 0);

	if ((currentManager != NULL or currentFileManager != NULL) and not bSilentMode)
	{
		DifferedStart();
		if (currentManager != NULL)
		{
			// Memorisation uniquement si necessaire
			if (nCurrentLevel < nDisplayedLevelNumber)
			{
				Initialize();

				// Reinitialisation des libelles et de la progression
				svLastMainLabels.SetAt(nCurrentLevel, "");
				svLastLabels.SetAt(nCurrentLevel, "");
				ivLastProgressions.SetAt(nCurrentLevel, 0);
			}

			// Calcul de la date du dernier affichage
			tTime = clock();
			dElapsedTime = ((double)(tTime - tLastDisplayTime)) / CLOCKS_PER_SEC;

			// Affichage si delai important
			if (dElapsedTime > dMinElapsedTime)
			{
				DisplayFullLevel();
				tLastDisplayTime = tTime;
			}
		}
		if (currentFileManager != NULL)
		{
			currentFileManager->SetMainLabel("");
			currentFileManager->SetLabel("");
			currentFileManager->SetProgression(0);
		}
	}
}

void TaskProgression::DisplayMainLabel(const ALString& sValue)
{
	clock_t tTime;
	double dElapsedTime;

	require(nCurrentLevel >= 0);

	if ((currentManager != NULL or currentFileManager != NULL) and not bSilentMode)
	{
		DifferedStart();
		if (currentManager != NULL)
		{
			// Memorisation uniquement si necessaire
			if (nCurrentLevel < nDisplayedLevelNumber)
			{
				Initialize();

				// Memorisation de la derniere demande
				svLastMainLabels.SetAt(nCurrentLevel, sValue);
			}

			// Calcul de la date du dernier affichage
			tTime = clock();
			dElapsedTime = ((double)(tTime - tLastDisplayTime)) / CLOCKS_PER_SEC;

			// Affichage si delai important
			if (dElapsedTime > dMinElapsedTime)
			{
				DisplayFullLevel();
				tLastDisplayTime = tTime;
			}
		}
		if (currentFileManager != NULL)
			currentFileManager->SetMainLabel(sValue);
	}
}

void TaskProgression::DisplayLabel(const ALString& sValue)
{
	clock_t tTime;
	double dElapsedTime;

	require(nCurrentLevel >= 0);

	if ((currentManager != NULL or currentFileManager != NULL) and not bSilentMode)
	{
		DifferedStart();
		if (currentManager != NULL)
		{
			// Memorisation uniquement si necessaire
			if (nCurrentLevel < nDisplayedLevelNumber)
			{
				Initialize();

				// Memorisation de la derniere demande
				svLastLabels.SetAt(nCurrentLevel, sValue);
			}

			// Calcul de la date du dernier affichage
			tTime = clock();
			dElapsedTime = ((double)(tTime - tLastDisplayTime)) / CLOCKS_PER_SEC;

			// Affichage si delai important
			if (dElapsedTime > dMinElapsedTime)
			{
				DisplayFullLevel();
				tLastDisplayTime = tTime;
			}
		}
		if (currentFileManager != NULL)
			currentFileManager->SetLabel(sValue);
	}
}

void TaskProgression::DisplayProgression(int nValue)
{
	clock_t tTime;
	double dElapsedTime;

	require(nCurrentLevel >= 0);
	require(0 <= nValue and nValue <= 100);

	if ((currentManager != NULL or currentFileManager != NULL) and not bSilentMode)
	{
		DifferedStart();
		if (currentManager != NULL)
		{
			// Memorisation uniquement si necessaire
			if (nCurrentLevel < nDisplayedLevelNumber)
			{
				Initialize();

				// Memorisation de la derniere demande
				ivLastProgressions.SetAt(nCurrentLevel, nValue);
			}

			// Calcul de la date du dernier affichage
			tTime = clock();
			dElapsedTime = ((double)(tTime - tLastDisplayTime)) / CLOCKS_PER_SEC;

			// Affichage si delai important
			if (dElapsedTime > dMinElapsedTime)
			{
				DisplayFullLevel();
				tLastDisplayTime = tTime;
			}
		}
		if (currentFileManager != NULL)
			currentFileManager->SetProgression(nValue);
	}
}

void TaskProgression::SetManager(TaskProgressionManager* manager)
{
	require(not bIsStarted);

	currentManager = manager;
}

TaskProgressionManager* TaskProgression::GetManager()
{
	return currentManager;
}

void TaskProgression::SetTaskProgressionLogFileName(const ALString& sValue)
{
	require(not bIsStarted);

	fileManager.SetTaskProgressionFileName(sValue);
	if (sValue == "")
		currentFileManager = NULL;
	else
		currentFileManager = &fileManager;
}

const ALString& TaskProgression::GetTaskProgressionLogFileName()
{
	return fileManager.GetTaskProgressionFileName();
}

longint TaskProgression::GetInterruptionRequestNumber()
{
	return lInterruptionRequestNumber;
}

void TaskProgression::SetInterruptionRequestIndex(longint lIndex)
{
	require(lIndex >= 0);
	lInterruptionRequestIndex = lIndex;
}

longint TaskProgression::GetInterruptionRequestIndex()
{
	return lInterruptionRequestIndex;
}

void TaskProgression::SetExternalInterruptionRequestIndex()
{
	ALString sInterruptionRequestIndex;
	longint lIndex;

	// Recherche de la valeur de la variable d'environnement
	sInterruptionRequestIndex = p_getenv("INTERRUPTION_REQUEST_INDEX");

	// Parametrage de l'index de requete d'interruption
	lIndex = StringToLongint(sInterruptionRequestIndex);
	if (lIndex >= 0)
		SetInterruptionRequestIndex(lIndex);
}

void TaskProgression::DisplayFullLevel()
{
	int nDisplayLevel;

	require(nCurrentLevel >= 0);
	require(currentManager != NULL);
	require(not bSilentMode);

	// On affiche les libelles pour tous les niveaux affiches
	for (nDisplayLevel = 0; nDisplayLevel < GetDisplayedLevelNumber(); nDisplayLevel++)
	{
		// On se positionne au niveau ou on veut faire l'affichage
		currentManager->SetCurrentLevel(nDisplayLevel);

		// Affichage bufferise du libelle principal
		if (svLastDisplayedMainLabels.GetAt(nDisplayLevel) != svLastMainLabels.GetAt(nDisplayLevel))
		{
			currentManager->SetMainLabel(svLastMainLabels.GetAt(nDisplayLevel));
			svLastDisplayedMainLabels.SetAt(nDisplayLevel, svLastMainLabels.GetAt(nDisplayLevel));
		}

		// Affichage bufferise du libelle
		if (svLastDisplayedLabels.GetAt(nDisplayLevel) != svLastLabels.GetAt(nDisplayLevel))
		{
			currentManager->SetLabel(svLastLabels.GetAt(nDisplayLevel));
			svLastDisplayedLabels.SetAt(nDisplayLevel, svLastLabels.GetAt(nDisplayLevel));
		}

		// Affichage bufferise de la progression
		if (ivLastDisplayedProgressions.GetAt(nDisplayLevel) != ivLastProgressions.GetAt(nDisplayLevel))
		{
			currentManager->SetProgression(ivLastProgressions.GetAt(nDisplayLevel));
			ivLastDisplayedProgressions.SetAt(nDisplayLevel, ivLastProgressions.GetAt(nDisplayLevel));
		}
	}

	// On se repositionne au niveau courant
	currentManager->SetCurrentLevel(nCurrentLevel);
}

void TaskProgression::DifferedStart()
{
	clock_t tTime;
	double dElapsedTime;

	// Gestion du lancement differe si necessaire
	if (not bIsManagerStarted and not bSilentMode and bIsStarted)
	{
		// Lancement si duree ecoulee assez importante
		tTime = clock();
		dElapsedTime = ((double)(tTime - tStartRequested)) / CLOCKS_PER_SEC;
		if (dElapsedTime > dMinElapsedTime)
		{
			if (currentManager != NULL)
				currentManager->Start();
			if (currentFileManager != NULL)
				currentFileManager->Start();
			bIsManagerStarted = true;
		}
	}
}

void TaskProgression::Initialize()
{
	if (svLastDisplayedMainLabels.GetSize() == 0)
	{
		svLastDisplayedMainLabels.SetSize(nMaxDisplayedLevelNumber);
		ivLastDisplayedProgressions.SetSize(nMaxDisplayedLevelNumber);
		svLastDisplayedLabels.SetSize(nMaxDisplayedLevelNumber);
		svLastMainLabels.SetSize(nMaxDisplayedLevelNumber);
		ivLastProgressions.SetSize(nMaxDisplayedLevelNumber);
		svLastLabels.SetSize(nMaxDisplayedLevelNumber);
	}
}

void TaskProgression::Test()
{
	int nMaxOuterLoop = 1000;
	int nOuterLoop;
	int nInnerLoop;
	double dTotal;
	ALString sTmp;

	// Debut de la tache
	TaskProgression::SetTitle("Suivi de progression");
	TaskProgression::Start();
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Boucles de comptage");

	// Deroullement de la tache
	dTotal = 0;
	for (nOuterLoop = 0; nOuterLoop < nMaxOuterLoop; nOuterLoop++)
	{
		// Boucle de comptage interne
		for (nInnerLoop = 0; nInnerLoop < 10000000; nInnerLoop++)
		{
			dTotal++;

			// Test d'arret de temps en temps
			if (nInnerLoop % 10000000 == 0)
			{
				// Message de progression
				TaskProgression::DisplayLabel(DoubleToString(dTotal));
				TaskProgression::DisplayProgression(nOuterLoop * 100 / nMaxOuterLoop);

				// Test d'arret
				if (TaskProgression::IsInterruptionRequested())
					break;
			}
		}

		// Test d'arret
		if (TaskProgression::IsInterruptionRequested())
			break;
	}

	// Fin de la tache
	TaskProgression::EndTask();
	TaskProgression::Stop();

	// Message sur le taux de progression final
	Global::AddSimpleMessage(sTmp +
				 "Pourcentage effectue: " + IntToString(int(nOuterLoop * 100.0 / nMaxOuterLoop)) + "%");
}