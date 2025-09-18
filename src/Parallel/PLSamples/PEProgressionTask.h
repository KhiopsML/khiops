// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLParallelTask.h"

///////////////////////////////////////////////////////////////////////
// Classe PEProgressionTask
// Classe qui permet de tester l'arret utilisateur
class PEProgressionTask : public PLParallelTask
{
public:
	// Constructeur
	PEProgressionTask();
	~PEProgressionTask();

	// 1: MasterInitialize 2:MasterPrepareTaskInput 3:MasterAggregate 4: MasterFinalize
	void SetMethodToTest(int nmethod);
	int GetMethodToTest() const;

	void SetBoostMode(boolean bValue);
	boolean GetBoostMode() const;

	// Test l'arret utilisateur dans le methode choisie avec ou sans le mode boost
	// nMethod peut prendre les valeurs  1: MasterInitialize 2:MasterPrepareTaskInput 3:MasterAggregate 4:
	// MasterFinalize renvoie true si la tache renvoie false (ca doit etre le cas quand l'interruption est detectee)
	static boolean Test(int nMethod, boolean bBoostMode);

	// Test l'arret utilisateur dans toutes les methodes du master, avec et sans le mode boost
	static boolean Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles de tache
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean MasterInitialize() override;
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;
	boolean MasterAggregateResults() override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveInitialize() override;
	boolean SlaveProcess() override;
	boolean SlaveFinalize(boolean bProcessEndedCorrectly) override;

	enum METHOD_TO_TEST
	{
		NONE,
		MASTER_INITIALIZE,
		MASTER_PREPARE_INPUT,
		MASTER_AGGREGATE,
		MASTER_FINALIZE,
		END
	};

	PLShared_Int input_nIterationNumber;
	int nIterationNumber;
	int nMethodToTest;
	boolean bBoostMode;
};
