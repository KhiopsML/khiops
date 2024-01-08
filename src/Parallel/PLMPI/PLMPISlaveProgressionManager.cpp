// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLMPISlave.h"

PLMPISlaveProgressionManager::PLMPISlaveProgressionManager(void)
{
	int nCommunicatorSize;

	MPI_Comm_size(*PLMPITaskDriver::GetTaskComm(), &nCommunicatorSize);
	dPeriod = (nCommunicatorSize - 1.0) / (nMessagesFrequency * 1.0);
	tMessages.Start();

	bIsInterruptionRequested = false;
	nOldProgression = 0;
	sendRequest = MPI_REQUEST_NULL;
	ivMaxErrorReached = NULL;
}

PLMPISlaveProgressionManager::~PLMPISlaveProgressionManager(void)
{
	if (sendRequest != MPI_REQUEST_NULL)
		MPI_Request_free(&sendRequest);

	// Reception des messages qu'on n'aurait pas encore reçu
	IsInterruptionRequested();

	ivMaxErrorReached = NULL;
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
	if (bIsInterruptionRequested)
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
		serializer.PutInt(PLSlaveState::StateToInt(nSlaveState));
		serializer.Close();

		// Envoi du serializer : on sait que sa taille fait 6 (c'est juste un int)
		serializer.ExportBufferMonoBlock(sBuffer, 12);
		MPI_Isend(sBuffer, 12, MPI_CHAR, 0, SLAVE_PROGRESSION, MPI_COMM_WORLD, &sendRequest);
	}
}

boolean PLMPISlaveProgressionManager::IsInterruptionResponsive() const
{
	return true;
}

void PLMPISlaveProgressionManager::SetSlaveState(State nState)
{
	nSlaveState = nState;
}

void PLMPISlaveProgressionManager::SetInterruptionRequested(boolean bInterruption)
{
	bIsInterruptionRequested = bInterruption;
}

boolean PLMPISlaveProgressionManager::GetInterruptionRequested() const
{
	return bIsInterruptionRequested;
}

void PLMPISlaveProgressionManager::SetMaxErrorReached(IntVector* ivMaxErrors)
{
	assert(ivMaxErrors != NULL and ivMaxErrors->GetSize() == 3);
	ivMaxErrorReached = ivMaxErrors;
}
IntVector* PLMPISlaveProgressionManager::GetMaxErrorReached() const
{
	return ivMaxErrorReached;
}

void PLMPISlaveProgressionManager::Start() {}

boolean PLMPISlaveProgressionManager::IsInterruptionRequested()
{
	int nMessage;
	MPI_Status status;
	PLMPIMsgContext context;
	PLSerializer serializer;

	if (not bIsInterruptionRequested)
	{
		MPI_Iprobe(0, INTERRUPTION_REQUESTED, MPI_COMM_WORLD, &nMessage, &status);
		if (nMessage)
		{

			context.Recv(MPI_COMM_WORLD, 0, INTERRUPTION_REQUESTED);
			serializer.OpenForRead(&context);
			serializer.Close();

			if (GetTracerMPI()->GetActiveMode())
				GetTracerMPI()->AddRecv(0, INTERRUPTION_REQUESTED);
			bIsInterruptionRequested = true;
		}

		MPI_Iprobe(0, MAX_ERROR_FLOW, MPI_COMM_WORLD, &nMessage, &status);
		if (nMessage)
		{
			context.Recv(MPI_COMM_WORLD, 0, MAX_ERROR_FLOW);
			serializer.OpenForRead(&context);
			serializer.GetIntVector(ivMaxErrorReached);
			serializer.Close();
			if (GetTracerMPI()->GetActiveMode())
				GetTracerMPI()->AddRecv(0, MAX_ERROR_FLOW);
		}
	}
	return bIsInterruptionRequested;
}

void PLMPISlaveProgressionManager::Stop() {}

void PLMPISlaveProgressionManager::SetLevelNumber(int nValue) {}

void PLMPISlaveProgressionManager::SetCurrentLevel(int nValue) {}

void PLMPISlaveProgressionManager::SetTitle(const ALString& sValue) {}

void PLMPISlaveProgressionManager::SetMainLabel(const ALString& sValue) {}

void PLMPISlaveProgressionManager::SetLabel(const ALString& sValue) {}
