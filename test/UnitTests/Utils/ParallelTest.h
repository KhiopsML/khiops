// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "gtest/gtest.h"

// Fixture a utiliser pour realiser les tests en sequentiel, en parallele et en simule.
// Les tests qui utilisent cette fixture vont etre lance en sequentiel puis en simule (sauf si il y a plus d'un
// processus, auquel cas le test est skippe) Pour lancer les test il faut definir les tests avec parametres
// (Value-parameterized tests) et les instancier INSTANTIATE_TEST_SUITE_P(nom_du_test, nom_de_la_classe ,
// testing::Values(RunType::MPI, RunType::SIMULATED));

namespace
{
using ::testing::TestWithParam;
using ::testing::Values;

enum class RunType
{
	MPI = 0,
	SIMULATED = 1
};

void operator<<(ostream& o, const RunType& r)
{
	if (r == RunType::MPI)
		o << "MPI";
	else
		o << "simulated";
}

class ParallelTest : public testing::TestWithParam<RunType>
{

protected:
	void SetUp() override
	{
		if (GetParam() == RunType::SIMULATED)
		{
			if (RMResourceManager::GetLogicalProcessNumber() > 1)
			{
				GTEST_SKIP();
			}
			else
			{
				PLParallelTask::SetParallelSimulated(true);
				PLParallelTask::SetSimulatedSlaveNumber(8);
			}
		}
	}
};
} // namespace
