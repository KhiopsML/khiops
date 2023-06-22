// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PEProtocolTestTask.h"

PEProtocolTestTask::PEProtocolTestTask()
{
	////////////////////////////////////////////////
	// declaration des variables partagees entre le maitre et les esclave

	// Les parametres du programme, affectes une fois
	// pour toutes au debut du programme
	DeclareSharedParameter(&shared_bSlaveInitializeIssue);
	DeclareSharedParameter(&shared_bSlaveFinalizeIssue);
	DeclareSharedParameter(&shared_nSlaveProcessIssue);
	DeclareSharedParameter(&shared_bFatalError);

	DeclareTaskInput(&input_bSlaveProcessFatalError);

	// Initalisation des parametres
	Init();
}

PEProtocolTestTask::~PEProtocolTestTask() {}

void PEProtocolTestTask::Init()
{
	shared_bSlaveInitializeIssue = false;
	shared_bSlaveFinalizeIssue = false;
	bMasterInitializeIssue = false;
	nMasterPrepareTaskInputIssue = INT_MAX;
	bMasterFinalizeIssue = false;
	shared_bFatalError = false;
	shared_nSlaveProcessIssue = INT_MAX;
	nMasterAggregateIssue = INT_MAX;
	nIterationNumber = 1000;
	nTimeBeforeInterruption = 0;
}

void PEProtocolTestTask::SetIterationNumber(int nNumber)
{
	assert(nNumber > 0);
	nIterationNumber = nNumber;
}

int PEProtocolTestTask::GetIterationNumber() const
{
	return nIterationNumber;
}

void PEProtocolTestTask::SetSlaveInitializeIssue(boolean bIssue)
{
	if (bIssue)
		SetTaskUserLabel("PEProtocolTestTask SlaveInitialize Issue");
	shared_bSlaveInitializeIssue = bIssue;
}

boolean PEProtocolTestTask::GetSlaveInitializeIssue() const
{
	return shared_bSlaveInitializeIssue;
}

void PEProtocolTestTask::SetSlaveProcessIssue(int nSlaveProcessIssue)
{
	if (nSlaveProcessIssue == 0)
		shared_nSlaveProcessIssue = INT_MAX;
	else
	{
		SetTaskUserLabel("PEProtocolTestTask SlaveProcess Issue");
		shared_nSlaveProcessIssue = nSlaveProcessIssue;
	}
}

int PEProtocolTestTask::GetSlaveProcessIssue() const
{
	return shared_nSlaveProcessIssue;
}

void PEProtocolTestTask::SetSlaveFinalizeIssue(boolean bIssue)
{
	if (bIssue)
		SetTaskUserLabel("PEProtocolTestTask SlaveFinalize Issue");
	shared_bSlaveFinalizeIssue = bIssue;
}

boolean PEProtocolTestTask::GetSlaveFinalizeIssue() const
{
	return shared_bSlaveFinalizeIssue;
}

void PEProtocolTestTask::SetMasterInitializeIssue(boolean bIssue)
{
	if (bIssue)
		SetTaskUserLabel("PEProtocolTestTask MasterInitialize Issue");
	bMasterInitializeIssue = bIssue;
}

boolean PEProtocolTestTask::GetMasterInitializeIssue() const
{
	return bMasterInitializeIssue;
}

void PEProtocolTestTask::SetMasterPrepareTaskInputIssue(int nIterationNumberIssue)
{
	nMasterPrepareTaskInputIssue = nIterationNumberIssue;
}

int PEProtocolTestTask::GetMasterPrepareTaskInputIssue() const
{
	return nMasterPrepareTaskInputIssue;
}

void PEProtocolTestTask::SetMasterAggregateIssue(int nIterationNumberIssue)
{
	require(nIterationNumber > nIterationNumberIssue);

	if (nIterationNumber == 0)
		nMasterAggregateIssue = INT_MAX;
	else
	{
		SetTaskUserLabel("PEProtocolTestTask MasterAggregate Issue");
		nMasterAggregateIssue = nIterationNumberIssue;
	}
}

int PEProtocolTestTask::GetMasterAggregateIssue() const
{
	return nMasterAggregateIssue;
}

void PEProtocolTestTask::SetMasterFinalizeIssue(boolean bIssue)
{
	if (bIssue)
		SetTaskUserLabel("PEProtocolTestTask MasterFinalize Issue");
	bMasterFinalizeIssue = bIssue;
}

boolean PEProtocolTestTask::GetMasterFinalizeIssue() const
{
	return bMasterFinalizeIssue;
}

void PEProtocolTestTask::SetInterruptionByUser(int nSeconds)
{
	nTimeBeforeInterruption = nSeconds;

	// Arret utilisateur
	if (nTimeBeforeInterruption > 0)
	{
		SetTaskUserLabel("PEProtocolTestTask Interruption By User");
		TaskProgression::SetMaxTaskTime(nTimeBeforeInterruption);
	}
}

boolean PEProtocolTestTask::GetInterruptionByUser() const
{
	if (nTimeBeforeInterruption > 0)
		return true;
	return false;
}

void PEProtocolTestTask::SetIssueAsFatalError(boolean bIsFatal)
{
	shared_bFatalError = bIsFatal;
}

boolean PEProtocolTestTask::GetIssueAsFatalError() const
{
	return shared_bFatalError;
}

boolean PEProtocolTestTask::Run()
{
	boolean bOk;

	TaskProgression::SetDisplayedLevelNumber(1);

	// Lancement de la tache, avec gestion de la barre d'avancement
	TaskProgression::SetTitle(GetTaskLabel());
	TaskProgression::Start();
	bOk = PLParallelTask::Run();
	TaskProgression::Stop();
	return bOk;
}

boolean PEProtocolTestTask::Test()
{
	boolean bOk = true;
	PEProtocolTestTask testingTask;

	testingTask.AddMessage("Test nominal behavior");
	bOk = testingTask.Run();
	ensure(bOk);

	// Test d'un erreur lors de MasterInitialize
	testingTask.AddMessage("Test issue on MasterInitialize");
	testingTask.Init();
	testingTask.SetMasterInitializeIssue(true);
	bOk = bOk and not testingTask.Run();
	ensure(bOk);

	// Test d'un erreur lors de MasterFinalize
	testingTask.AddMessage("Test issue on MasterFinalize");
	testingTask.Init();
	testingTask.SetMasterFinalizeIssue(true);
	bOk = bOk and not testingTask.Run();
	ensure(bOk);

	// Test d'un erreur lors de SlaveInitialize
	testingTask.AddMessage("Test issue on SlaveInitialize");
	testingTask.Init();
	testingTask.SetSlaveInitializeIssue(true);
	bOk = bOk and not testingTask.Run();
	ensure(bOk);

	// Test d'un erreur lors de SlaveFinalize
	testingTask.AddMessage("Test issue on SlaveFinalize");
	testingTask.Init();
	testingTask.SetSlaveFinalizeIssue(true);
	bOk = bOk and not testingTask.Run();
	ensure(bOk);

	// Test d'un erreur lors de MasterAggregate a l'iteration 500
	testingTask.AddMessage("Test issue on MasterAggregate at 500");
	testingTask.Init();
	testingTask.SetMasterAggregateIssue(500);
	bOk = bOk and not testingTask.Run();
	ensure(bOk);

	// Test d'un erreur lors de SlaveProcess a l'iteration 500
	testingTask.AddMessage("Test issue on SlaveProcess at 500");
	testingTask.Init();
	testingTask.SetSlaveProcessIssue(500);
	bOk = bOk and not testingTask.Run();
	ensure(bOk);

	// Test d'un arret utilisateur
	testingTask.AddMessage("Test user interruption in 2 seconds");
	testingTask.Init();
	testingTask.SetInterruptionByUser(2);
	testingTask.SetIterationNumber(100000000);
	bOk = bOk and not testingTask.Run();
	bOk = bOk and testingTask.IsTaskInterruptedByUser();
	ensure(bOk);

	if (bOk)
		testingTask.AddMessage("Test is OK");
	else
		testingTask.AddMessage("Test failed");

	return bOk;
}

void PEProtocolTestTask::TestFatalError(int nTestIndex)
{
	PEProtocolTestTask testingTask;

	switch (nTestIndex)
	{
	case 0:
		testingTask.AddMessage("Test nominal behavior");
		testingTask.SetIssueAsFatalError(true);
		testingTask.Run();
		break;

	case 1:
		// Test d'un erreur lors de MasterInitialize
		testingTask.AddMessage("Test issue on MasterInitialize");
		testingTask.Init();
		testingTask.SetIssueAsFatalError(true);
		testingTask.SetMasterInitializeIssue(true);
		testingTask.Run();
		break;

	case 2:
		// Test d'un erreur lors de MasterFinalize
		testingTask.AddMessage("Test issue on MasterFinalize");
		testingTask.Init();
		testingTask.SetIssueAsFatalError(true);
		testingTask.SetMasterFinalizeIssue(true);
		testingTask.Run();
		break;

	case 3:
		// Test d'un erreur lors de SlaveInitialize
		testingTask.AddMessage("Test issue on SlaveInitialize");
		testingTask.Init();
		testingTask.SetIssueAsFatalError(true);
		testingTask.SetSlaveInitializeIssue(true);
		testingTask.Run();
		break;

	case 4:
		// Test d'un erreur lors de SlaveFinalize
		testingTask.AddMessage("Test issue on SlaveFinalize");
		testingTask.Init();
		testingTask.SetIssueAsFatalError(true);
		testingTask.SetSlaveFinalizeIssue(true);
		testingTask.Run();
		break;

	case 5:
		// Test d'un erreur lors de SlaveProcess a l'iteration 500
		testingTask.AddMessage("Test issue on SlaveProcess at 500");
		testingTask.Init();
		testingTask.SetIssueAsFatalError(true);
		testingTask.SetSlaveProcessIssue(500);
		testingTask.Run();
		break;

	case 6:
		// Test d'un erreur lors de MasterAggregate a l'iteration 500
		testingTask.AddMessage("Test issue on MasterPrepareTaskInput at 500");
		testingTask.Init();
		testingTask.SetIssueAsFatalError(true);
		testingTask.SetMasterPrepareTaskInputIssue(500);
		testingTask.Run();
		break;

	case 7:
		testingTask.AddMessage("Test issue on MasterAggregate at 500");
		testingTask.Init();
		testingTask.SetIssueAsFatalError(true);
		testingTask.SetMasterAggregateIssue(500);
		testingTask.Run();
		break;
	default:
		cout << "Wrong test index" << endl;
	}
}

const ALString PEProtocolTestTask::GetTaskName() const
{
	return "Protocol Test Task";
}

PLParallelTask* PEProtocolTestTask::Create() const
{
	return new PEProtocolTestTask;
}

boolean PEProtocolTestTask::MasterInitialize()
{
	// Erreur
	if (bMasterInitializeIssue)
	{
		if (shared_bFatalError)
			AddFatalError("MasterInitialize error");
		else
			return false;
	}
	return true;
}

boolean PEProtocolTestTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	if (GetTaskIndex() == nMasterPrepareTaskInputIssue)
		if (shared_bFatalError)
			AddFatalError("MasterPrepareTaskInput error");

	if (GetTaskIndex() == shared_nSlaveProcessIssue and shared_bFatalError)
	{
		input_bSlaveProcessFatalError = true;
	}
	else
	{
		input_bSlaveProcessFatalError = false;
	}

	// Fin de la tache
	if (GetTaskIndex() == nIterationNumber)
		bIsTaskFinished = true;

	// Calcul de la progression
	dTaskPercent = 1.0 / nIterationNumber;
	return true;
}

boolean PEProtocolTestTask::MasterAggregateResults()
{
	if (GetTaskIndex() == nMasterAggregateIssue)
	{
		if (shared_bFatalError)
			AddFatalError("MasterAggregate error");
		else
			return false;
	}
	return true;
}

boolean PEProtocolTestTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	if (bMasterFinalizeIssue)
	{
		if (shared_bFatalError)
			AddFatalError("MasterFinalize error");
		else
			return false;
	}
	return true;
}

boolean PEProtocolTestTask::SlaveInitialize()
{
	if (shared_bSlaveInitializeIssue and not shared_bFatalError)
		return false;
	if (shared_bSlaveInitializeIssue and shared_bFatalError and GetProcessId() == 1)
		AddFatalError("SlaveInitialize error");

	return true;
}

boolean PEProtocolTestTask::SlaveProcess()
{
	if (input_bSlaveProcessFatalError)
	{
		AddFatalError("SlaveProcess error");
	}

	if (GetTaskIndex() == shared_nSlaveProcessIssue)
		return false;

	// On fait semblant de travailler 0.1s
	SystemSleep(0.001);

	return true;
}

boolean PEProtocolTestTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	if (shared_bSlaveFinalizeIssue and not shared_bFatalError)
		return false;
	if (shared_bSlaveFinalizeIssue and shared_bFatalError and GetProcessId() == 1)
		AddFatalError("SlaveFinalize error");

	return true;
}