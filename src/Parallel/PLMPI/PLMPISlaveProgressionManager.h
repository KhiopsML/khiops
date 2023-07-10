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
	PLMPISlaveProgressionManager();
	~PLMPISlaveProgressionManager();

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

	// Etat de l'esclave (VOID,PROCESS,FINALIZE)
	void SetSlaveState(State nState);

	// Acces direct l'interruption sans passer par MPI
	void SetInterruptionRequested(boolean bInterruption);
	boolean GetInterruptionRequested() const;

	// Acces au tableau qui recense si le nombre d'erreur max est atteint
	// cet tableau appartient a PLMPISlave, il est mis a jour pendant la requete d'interruption
	void SetMaxErrorReached(IntVector* ivMaxErrors);
	IntVector* GetMaxErrorReached() const;

	PLMPITracer* GetTracerMPI();

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
private:
	int nOldProgression; // Derniere progression enregistree (utilisee pour ne pas ecrire tout le temps dans la
			     // fentere)
	MPI_Request sendRequest;
	boolean bIsInterruptionRequested;

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
	State nSlaveState;

	IntVector* ivMaxErrorReached;
};

inline PLMPITracer* PLMPISlaveProgressionManager::GetTracerMPI()
{
	return cast(PLMPITracer*, PLParallelTask::GetDriver()->GetTracerMPI());
}
