// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PEProtocolTestTask.h"
#include "PLMPITaskDriver.h"
#include "PLMPISystemFileDriverRemote.h"
#include "../Utils/TestServices.h"
#include "../Utils/ParallelTest.h"

#include "gtest/gtest.h"
namespace
{

class PEProtocolTest : public ::ParallelTest
{
protected:
	PEProtocolTestTask* testingTask;

	void SetUp() override
	{
		testingTask = new PEProtocolTestTask;
		ParallelTest::SetUp();
	}

	void TearDown() override
	{
		ParallelTest::TearDown();
		delete testingTask;
	}
};

TEST_P(PEProtocolTest, nominalBehavior)
{
	if (GetProcessId() == 0)
		EXPECT_TRUE(testingTask->Run());
}

TEST_P(PEProtocolTest, MasterInitializeIssue)
{
	if (GetProcessId() == 0)
	{
		testingTask->SetMasterInitializeIssue(true);
		EXPECT_FALSE(testingTask->Run());
	}
}

TEST_P(PEProtocolTest, MasterFinalizeIssue)
{
	if (GetProcessId() == 0)
	{
		testingTask->SetMasterFinalizeIssue(true);
		EXPECT_FALSE(testingTask->Run());
	}
}

TEST_P(PEProtocolTest, SlaveInitializeIssue)
{
	if (GetProcessId() == 0)
	{
		testingTask->SetSlaveInitializeIssue(true);
		EXPECT_FALSE(testingTask->Run());
	}
}

TEST_P(PEProtocolTest, SlaveFinalizeIssue)
{
	if (GetProcessId() == 0)
	{
		testingTask->SetSlaveFinalizeIssue(true);
		EXPECT_FALSE(testingTask->Run());
	}
}

TEST_P(PEProtocolTest, MasterAggregateIssue)
{
	if (GetProcessId() == 0)
	{
		testingTask->SetMasterAggregateIssue(500);
		EXPECT_FALSE(testingTask->Run());
	}
}

TEST_P(PEProtocolTest, SlaveProcessIssue)
{
	if (GetProcessId() == 0)
	{
		testingTask->SetSlaveProcessIssue(500);
		EXPECT_FALSE(testingTask->Run());
	}
}

TEST_P(PEProtocolTest, userInterruption)
{
	boolean bOk;
	if (GetProcessId() == 0)
	{
		bOk = UIObject::SetUIMode(UIObject::Graphic);
		if (bOk)
		{
			// Test d'un arret utilisateur
			testingTask->SetInterruptionByUser(1);
			testingTask->SetIterationNumber(100000000);
			EXPECT_FALSE(testingTask->Run());
			EXPECT_TRUE(testingTask->IsTaskInterruptedByUser());
		}
	}
}

INSTANTIATE_TEST_SUITE_P(SimulatedOrSerial, PEProtocolTest, testing::Values(RunType::MPI, RunType::SIMULATED));

} // namespace
