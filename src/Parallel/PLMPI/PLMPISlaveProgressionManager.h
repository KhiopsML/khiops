// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLMPITaskDriver.h"
#include "TaskProgressionManager.h"
#include "PLMPISlaveProgressionManager.h"
#include "PLSlaveState.h"

///////////////////////////////////////////////////////////////////////////////
// classe PLMPISlaveProgressionManager
// Cette classe encapsule les appels a MPI pour gerer l'avancement et l'arret utilisateur d'un esclave (classe PLSlave)
// L'implementation de cette classe doit etre coherente avec la gestion de l'avancement des esclaves dans la classe
// PLMaster (GetPLSlavesProgression par exemple)
//
class PLMPISlaveProgressionManager : public TaskProgressionManager
{
public:
	// Constructeur
	PLMPISlaveProgressionManager(void);
	~PLMPISlaveProgressionManager(void);

	void Start() override;
	boolean IsInterruptionRequested() override;
	void Stop() override;
	void SetLevelNumber(int nValue) override;
	void SetCurrentLevel(int nValue) override;
	void SetTitle(const ALString& sValue) override;
	void SetMainLabel(const ALString& sValue) override;
	void SetLabel(const ALString& sValue) override;
	void SetProgression(int nValue) override;

	// Etat de l'esclave (VOID,PROCESS,FINALIZE)
	void SetSlaveState(PLSlaveState::State nState);

	// Renvoie true si l'interruption utilisateur a deja ete detectee
	boolean IsInterruptionDiscovered() const;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
private:
	MPI_Win winForInterruption; // Fenetre RMA pour l'arret
	int nOldProgression; // Derniere progression enregistree (utilisee pour ne pas ecrire tout le temps dans la
			     // fentere)
	boolean bRMA_ON;
	MPI_Request sendRequest;
	boolean bInterruptionRequested;
	boolean bInterruptionDiscovered;

	// On n'envoie pas le message a chaque fois pour ne pas charger le maitre (en fonction du nombre d'esclaves)
	// nombre de message envoyes par secondes si il y a un esclave
	const int nMessagesFrequency = 5;

	// Temps minimal entre 2 messages (fonction de nMessagesFrequency et du nombre d'esclaves)
	// Si une notification de progression suit une autre et qu'elel sont trop rapprochees dans le temps.
	// La seconde n'est pas prise en compte
	double dPeriod;

	// Pour mesurer la duree entre dux notifictions de progression
	Timer tMessages;

	// Buffer pour envoi asynchrone, on envoie juste 2 int on sait que le serializer fait 12 chars
	// Il ne faut pas le toucher tant que l'envoi n'a pas ete recu
	char sBuffer[12];

	// Status de l'esclave qui envoie le message
	int nSlaveState;
};

inline boolean PLMPISlaveProgressionManager::IsInterruptionDiscovered() const
{
	return bInterruptionDiscovered;
}

inline boolean PLMPISlaveProgressionManager::IsInterruptionRequested()
{
	if (bInterruptionRequested)
	{
		bInterruptionDiscovered = true;
	}
	return bInterruptionRequested;
}