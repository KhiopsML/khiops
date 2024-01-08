// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PEHelloWorldTask.h"

PEHelloWorldTask::PEHelloWorldTask()
{
	nIteration = 0;
}

PEHelloWorldTask::~PEHelloWorldTask() {}

void PEHelloWorldTask::Hello()
{
	Run();
}

void PEHelloWorldTask::Test()
{
	PEHelloWorldTask test;
	test.Hello();
}

const ALString PEHelloWorldTask::GetTaskName() const
{
	return "Hello task";
}

PLParallelTask* PEHelloWorldTask::Create() const
{
	return new PEHelloWorldTask;
}

boolean PEHelloWorldTask::MasterInitialize()
{
	return true;
}

boolean PEHelloWorldTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	const int nTotalTaskNumber = 100;

	// Calcul de la fraction du travail total pour la prochaine tache
	dTaskPercent = 1.0 / nTotalTaskNumber;

	// Les esclaves affichent Hello World 100 fois en cummule
	// apres le programme s'arrete
	if (GetTaskIndex() > nTotalTaskNumber)
		bIsTaskFinished = true;

	return true;
}

boolean PEHelloWorldTask::MasterAggregateResults()
{
	return true;
}

boolean PEHelloWorldTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	return true;
}

boolean PEHelloWorldTask::SlaveInitialize()
{
	nIteration = 0;
	return true;
}

boolean PEHelloWorldTask::SlaveProcess()
{
	cout << "Hello World from slave " << GetProcessId() << " iteration number " << nIteration << " task id "
	     << GetTaskIndex() << endl;
	nIteration++;
	return true;
}

boolean PEHelloWorldTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	return true;
}
