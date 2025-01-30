// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Standard.h"
#include "Object.h"
#include "Ermgt.h"
#include "ALString.h"
#include "Vector.h"
#include "Timer.h"

class TaskProgressionManager;
class FileTaskProgressionManager;

/////////////////////////////////////////////////////////////////////////
// Classe TaskProgressionManager
// Definition d'une classe ancetre des managers.
// Toutes les methodes, similaires aux methodes de pilotage de la classe
// TaskProgression, sont virtuelles et doivent etre reimplementees
// dans les sous-classes.
class TaskProgressionManager : public Object
{
public:
	// Methodes similaires a celles de TaskProgression
	virtual void Start() = 0;
	virtual boolean IsInterruptionRequested() = 0;
	virtual void Stop() = 0;
	virtual void SetLevelNumber(int nValue) = 0;
	virtual void SetCurrentLevel(int nValue) = 0;
	virtual void SetTitle(const ALString& sValue) = 0;
	virtual void SetMainLabel(const ALString& sValue) = 0;
	virtual void SetLabel(const ALString& sValue) = 0;
	virtual void SetProgression(int nValue) = 0;

	// Doit renvoyer true ou false sans aucun calcul pour indiquer si on doit
	// temporiser ou non le test des interruptions (false pour temporiser).
	virtual boolean IsInterruptionResponsive() const = 0;
};

/////////////////////////////////////////////////////////////////////////
// Classe FileTaskProgressionManager
// Implementation d'une interface utilisateur de suivi de progression des
// tache, avec sortie dans un fichier.
// Le fichier est ouvert/ferme a chaque message pour permettre une consultation
// externe permettant de suivre une execution en mode batch (sans interface utilisateur)
// Seuls les derniers messages d'avancement sont disponibles
class FileTaskProgressionManager : public TaskProgressionManager
{
public:
	// Constructeur
	FileTaskProgressionManager();
	~FileTaskProgressionManager();

	// Nom du fichier de sortie des message d'avancement
	void SetTaskProgressionFileName(const ALString& sFileName);
	const ALString& GetTaskProgressionFileName() const;

	// Redefinition des methodes virtuelles de TaskProgressionManager
	void Start() override;
	boolean IsInterruptionRequested() override;
	void Stop() override;
	void SetLevelNumber(int nValue) override;
	void SetCurrentLevel(int nValue) override;
	void SetTitle(const ALString& sValue) override;
	void SetMainLabel(const ALString& sValue) override;
	void SetLabel(const ALString& sValue) override;
	void SetProgression(int nValue) override;
	boolean IsInterruptionResponsive() const override;

	/////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Ajout d'un message de progression a memoriser
	void AddCompletedTaskMessage(const ALString& sMessage);

	// Ecriture des messages de progression
	void WriteProgressionMessages();

	// Nom du fichier memorisant les messages de progressions
	ALString sTaskProgressionFileName;

	// Memorisation des informations apportees par les methodes virtuelles reimplementees
	boolean bStarted;
	int nLevelNumber;
	int nCurrentLevel;
	StringVector svMainLabels;
	IntVector ivProgressions;
	int nTaskIndex;

	// Bufferisation des derniers messages
	Timer tLastProgressionMessage;
	ObjectList olLastCompletedTaskMessages;
	ALString sLastTaskMainLabel;
	ALString sLastTaskLabel;
	ALString sLastProgressionMessage;
	static int nMaxCompletedTaskMessageNumber;
	static double dMinInterMessageSeconds;
	ALString sStartTimeStamp;

	// Sortie dans la console
	boolean bPrintProgressionInConsole;
	const ALString sLinePrefix = "Khiops.progression";
};
