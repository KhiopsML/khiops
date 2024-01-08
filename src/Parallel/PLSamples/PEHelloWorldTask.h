// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLParallelTask.h"

///////////////////////////////////////////////////////////////////////
// Classe PEHelloWorldTask
// Exemple simplissime de programme parallele
// Chaque esclave affiche son rang et l'iteration (le nombre de fois ou il est
// passe dans la methode SlaveProcess).
class PEHelloWorldTask : public PLParallelTask
{
public:
	// Constructeur
	PEHelloWorldTask();
	~PEHelloWorldTask();

	// Methode principale
	void Hello();

	// Methode de test
	static void Test();

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

	// Nombre d'iteration par esclave
	int nIteration;
};
