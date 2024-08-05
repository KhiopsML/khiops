// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PELullabyTask.h"
#include "PLMPITaskDriver.h"
#include "../Utils/TestServices.h"
#include "../Utils/ParallelTest.h"

#include "gtest/gtest.h"
namespace
{

class PELullabyTest : public ::ParallelTest
{
protected:
	PELullabyTask* testingTask;

	void SetUp() override
	{
		testingTask = new PELullabyTask;
		ParallelTest::SetUp();
	}

	void TearDown() override
	{
		ParallelTest::TearDown();
		delete testingTask;
	}
};

TEST_P(PELullabyTest, nominalBehavior)
{
	EXPECT_TRUE(testingTask->Run());
}

// TEST_P(PELullabyTest, callTwice)
// {
// 	if (GetProcessId() == 0)
// 	{
// 		EXPECT_TRUE(testingTask->Run());
// 		EXPECT_TRUE(testingTask->Run());
// 	}
// }

INSTANTIATE_TEST_SUITE_P(SimulatedOrSerial, PELullabyTest, testing::Values(RunType::MPI, RunType::SIMULATED));

} // namespace
