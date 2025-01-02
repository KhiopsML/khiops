// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLParallelTask.h"
#include "PLSharedObject.h"
#include "TaskProgression.h"

//////////////////////////////////////////////////////////////////////////
// Classe d'exemple a vocation didactique
// Estimation du nombre pi en parallele
// L'utilisateur choisit le nombre d'iteration total
// Le master reparti le nombre d'iterations sur tous les esclaves
//
// Le programme sequentiel est :
//
// static double SerialPi()
// {
//	 double sum = 0.0;
//	 double step = 1.0 / total_iter;
//   double x;
//	 for (int i = 0; i < total_iter; i++)
//	 {
//	 	x = (i + 0.5) * step;
//	 	sum = sum + 4.0 / (1.0 + x * x);
//	 }
//	 return step * sum;
// }
class PEPiTask : public PLParallelTask
{
public:
	// Constructeur
	PEPiTask();
	~PEPiTask();

	// Nombre d'iteration total
	void SetTotalIteration(int nMax);

	// Lance le calcul de Pi, avec gestion de la barre d'avancement
	void ComputePi();

	// Resultats : Pi
	double GetComputedPi() const;

	// Lance le programme avec nSlavesNumber esclaves sur nIterationNumber
	// iterations
	static void Test(int nSlavesNumber, int nIterationNumber);

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

	///////////////////////////////////////////////////////////
	// Parametres partages par le maitre et les esclaves
	// tout au long du programme

	// Nombre de pas
	PLShared_Double shared_dSlaveStep;

	//////////////////////////////////////////////////////
	// Input de la tache parallelisee
	// Envoyes par le master aux esclaves avant chaque tache

	// Nombre d'iteration a effectuer pour une tache
	PLShared_Int input_nIterationNumber;

	// Debut de l'iteration pour une tache
	PLShared_Int input_nIterationStart;

	//////////////////////////////////////////////////////
	// Resultats de la tache executee par un esclave
	// Envoyes par les esclaves au Master a l'issue de chaque tache

	// Calcul local de Pi
	PLShared_Double output_dLocalPi;

	//////////////////////////////////////////////////////
	// Variables du Master

	// Nombre d'iterations total demande par l'utilisateur
	int nTotalIteration;

	// Nombre d'iterations par esclave
	int nIterationPerTask;

	// Index courant de l'iteration
	int nIteration;

	// Resultat du programme
	double dPi;
};
