// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PELullabyTask.h"

PELullabyTask::PELullabyTask() {}

PELullabyTask::~PELullabyTask() {}

boolean PELullabyTask::Run()
{
	boolean bOk;
	nMyTaskIndex = 0;
	TaskProgression::SetDisplayedLevelNumber(1);

	// Lancement de la tache, avec gestion de la barre d'avancement
	TaskProgression::SetTitle(GetTaskLabel());
	TaskProgression::Start();
	bOk = PLParallelTask::Run();
	TaskProgression::Stop();
	return bOk;
}

boolean PELullabyTask::Test()
{
	PELullabyTask taskTest;
	return taskTest.Run();
}

const ALString PELullabyTask::GetTaskName() const
{
	return "Resting Slave Task ";
}

PLParallelTask* PELullabyTask::Create() const
{
	return new PELullabyTask;
}

boolean PELullabyTask::MasterInitialize()
{
	// Chaque pas contient  2 slaveProcess de chaque esclave
	// (il en faut au moins 1 pour qu'on voie les esclaves s'endormir 1 par 1)
	nStepSize = 2 * GetProcessNumber();

	return true;
}

boolean PELullabyTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	ALString sTmp;
	// En utilisant if (GetTaskIndex()==100) comme condition, on aurait une boucle inifinie car, le taskIndex
	// n'est pas incremente lorsqu'on met un esclave en sommeil, il faut donc utiliser notre propre iterateur
	if (nMyTaskIndex == nStepSize and GetRestingSlaveNumber() < GetProcessNumber() - 1)
	{
		SetSlaveAtRest();
		nMyTaskIndex = 0;
		AddMessage(sTmp + "Set slave to rest at task index " + IntToString(GetTaskIndex()));
	}
	else
		nMyTaskIndex++;

	if (GetTaskIndex() > (GetProcessNumber() + 1) * nStepSize)
		bIsTaskFinished = true;

	return true;
}

boolean PELullabyTask::MasterAggregateResults()
{
	ALString sTmp;
	if (nMyTaskIndex == nStepSize + 1)
	{
		AddMessage(sTmp + "Set all slave at work at task index " + IntToString(GetTaskIndex()));
		SetAllSlavesAtWork();
	}
	return true;
}

boolean PELullabyTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	return true;
}

boolean PELullabyTask::SlaveInitialize()
{
	return true;
}

boolean PELullabyTask::SlaveProcess()
{
	// ALString sTmp;
	// AddMessage(sTmp + "Slave " + IntToString(GetProcessId()) + " taskIndex " + IntToString(GetTaskIndex()));
	return true;
}

boolean PELullabyTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{

	return true;
}
