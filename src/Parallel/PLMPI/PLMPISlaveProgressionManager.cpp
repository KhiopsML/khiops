// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLMPISlave.h"

PLMPISlaveProgressionManager::PLMPISlaveProgressionManager(void)
{
	int nCommunicatorSize;

	bRMA_ON = true;
	MPI_Comm_size(*PLMPITaskDriver::GetTaskComm(), &nCommunicatorSize);
	dPeriod = (nCommunicatorSize - 1.0) / (nMessagesFrequency * 1.0);
	tMessages.Start();

	// Creation de la fenetre RMA pour controler l'arret
	MPI_Alloc_mem(sizeof(int), MPI_INFO_NULL, &bInterruptionRequested);
	MPI_Win_create(&bInterruptionRequested, sizeof(int), sizeof(int), MPI_INFO_NULL,
		       *PLMPITaskDriver::GetTaskComm(), &winForInterruption);

	bInterruptionRequested = false;
	nOldProgression = 0;
	sendRequest = MPI_REQUEST_NULL;
	bInterruptionDiscovered = false;
}

PLMPISlaveProgressionManager::~PLMPISlaveProgressionManager(void)
{
	int flag;
	void* adressWinInterruption;

	if (sendRequest != MPI_REQUEST_NULL)
		MPI_Request_free(&sendRequest);

	MPI_Win_get_attr(winForInterruption, MPI_WIN_BASE, &adressWinInterruption, &flag);
	MPI_Win_free(&winForInterruption);
}

void PLMPISlaveProgressionManager::SetProgression(int nValue)
{
	int nMessageIsSent = 0;
	MPI_Status status;
	PLMPIMsgContext context;
	PLSerializer serializer;

	// Si la valeur est nulle on n'envoie rien car
	// ca ne sert a rien car le maitre est deja au courant
	// et quand on fait TaskProgression::EndTask(); Norm fait automatiquement SetProgression(0) et il ne faut pas
	if (nValue == 0)
		return;
	if (bInterruptionRequested)
		return;
	if (nValue == nOldProgression)
		return;

	nOldProgression = nValue;

	if (tMessages.GetElapsedTime() < dPeriod)
		return;
	tMessages.Reset();
	tMessages.Start();

	// On n'envoie que si le precedent a ete recu
	MPI_Test(&sendRequest, &nMessageIsSent, &status);

	if (nMessageIsSent)
	{
		serializer.OpenForWrite(NULL);
		serializer.PutInt(nValue);
		serializer.PutInt(nSlaveState);
		serializer.Close();

		// Envoi du serializer : on sait que sa taille fait 6 (c'est juste un int)
		serializer.ExportBufferMonoBlock(sBuffer, 12);
		MPI_Isend(sBuffer, 12, MPI_CHAR, 0, SLAVE_PROGRESSION, MPI_COMM_WORLD, &sendRequest);
	}
}

void PLMPISlaveProgressionManager::SetSlaveState(PLSlaveState::State nState)
{
	nSlaveState = nState;
}

void PLMPISlaveProgressionManager::Start() {}

void PLMPISlaveProgressionManager::Stop() {}

void PLMPISlaveProgressionManager::SetLevelNumber(int nValue) {}

void PLMPISlaveProgressionManager::SetCurrentLevel(int nValue) {}

void PLMPISlaveProgressionManager::SetTitle(const ALString& sValue) {}

void PLMPISlaveProgressionManager::SetMainLabel(const ALString& sValue) {}

void PLMPISlaveProgressionManager::SetLabel(const ALString& sValue) {}