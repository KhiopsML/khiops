// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PEProgressionTask.h"

PEProgressionTask::PEProgressionTask()
{
	nMethodToTest = 0;
	bBoostMode = false;
	DeclareTaskInput(&input_nIterationNumber);
}
PEProgressionTask::~PEProgressionTask() {}

void PEProgressionTask::SetMethodToTest(int nMethod)
{
	assert(nMethod >= 0 and nMethod < END);

	nMethodToTest = nMethod;
}
int PEProgressionTask::GetMethodToTest() const
{
	return nMethodToTest;
}

void PEProgressionTask::SetBoosteMode(boolean bValue)
{
	bBoostMode = bValue;
}

boolean PEProgressionTask::GetBoostMode() const
{
	return bBoostMode;
}

boolean PEProgressionTask::Test(int nMethod, boolean bValue)
{
	PEProgressionTask task;
	boolean bOk;

	assert(UIObject::GetUIMode() == UIObject::Graphic);

	cout << endl << "-- Test interruption requested in ";
	switch (nMethod)
	{
	case MASTER_INITIALIZE:
		cout << "MasterInitialize";
		break;
	case MASTER_PREPARE_INPUT:
		cout << "MasterPrepareInput";
		break;
	case MASTER_AGGREGATE:
		cout << "MasterAggregate";
		break;
	case MASTER_FINALIZE:
		cout << "MasterFinalize";
		break;
	default:
		cout << "UNKNOWN method";
	}

	if (bValue)
		cout << " with boost mode";
	cout << endl;
	TaskProgression::Start();
	task.SetBoosteMode(bValue);
	task.SetMethodToTest(nMethod);
	bOk = task.Run();
	cout << "Task done and returned " << BooleanToString(bOk) << endl;
	TaskProgression::Stop();
	return not bOk;
}

boolean PEProgressionTask::Test()
{
	int nMethod;
	int bOk;
	bOk = true;
	for (nMethod = MASTER_INITIALIZE; nMethod < END; nMethod++)
	{
		bOk = Test(nMethod, false) and bOk;
		ensure(bOk);
		bOk = Test(nMethod, true) and bOk;
		ensure(bOk);
	}
	return bOk;
}

const ALString PEProgressionTask::GetTaskName() const
{
	return "Progression Test Task";
}

PLParallelTask* PEProgressionTask::Create() const
{
	return new PEProgressionTask;
}

boolean PEProgressionTask::MasterInitialize()
{
	assert(UIObject::GetUIMode() == UIObject::Graphic);

	SystemSleep(0.1);
	nIterationNumber = 0;

	if (bBoostMode)
		SetBoostMode(true);

	if (nMethodToTest == MASTER_INITIALIZE)
		TaskProgression::ForceInterruptionRequested();

	if (TaskProgression::IsInterruptionRequested())
		cout << "Interruption requested detected in MasterInitialize" << endl;
	return true;
}

boolean PEProgressionTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{

	SystemSleep(0.1);
	input_nIterationNumber = nIterationNumber;
	if (nMethodToTest == MASTER_PREPARE_INPUT)
		TaskProgression::ForceInterruptionRequested();

	if (TaskProgression::IsInterruptionRequested())
		cout << "Interruption requested detected in MasterPrepareTaskInput" << endl;
	if (nIterationNumber == 10)
		bIsTaskFinished = true;
	else
		bIsTaskFinished = false;
	dTaskPercent = 1.0 * nIterationNumber / 10;
	nIterationNumber++;
	return true;
}

boolean PEProgressionTask::MasterAggregateResults()
{
	SystemSleep(0.1);
	if (nMethodToTest == MASTER_AGGREGATE)
		TaskProgression::ForceInterruptionRequested();
	if (TaskProgression::IsInterruptionRequested())
		cout << "Interruption requested detected in MasterAggregateResults" << endl;
	return true;
}

boolean PEProgressionTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	if (nMethodToTest == MASTER_FINALIZE)
		TaskProgression::ForceInterruptionRequested();
	if (TaskProgression::IsInterruptionRequested())
		cout << "Interruption requested detected in MasterFinalize" << endl;
	return true;
}

boolean PEProgressionTask::SlaveInitialize()
{
	SystemSleep(0.1);
	if (TaskProgression::IsInterruptionRequested())
		cout << "Interruption requested detected in SlaveInitialize" << endl;
	return true;
}

boolean PEProgressionTask::SlaveProcess()
{
	int i;
	boolean bIsInterruptionRequested = false;

	for (i = 0; i < 10; i++)
	{
		SystemSleep(0.01);
		bIsInterruptionRequested = TaskProgression::IsInterruptionRequested();
		if (bIsInterruptionRequested)
			break;
	}

	if (TaskProgression::IsInterruptionRequested())
		cout << "Interruption requested detected in SlaveProcess" << endl;
	return true;
}

boolean PEProgressionTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	if (TaskProgression::IsInterruptionRequested())
		cout << "Interruption requested detected in SlaveFinalize" << endl;
	return true;
}
