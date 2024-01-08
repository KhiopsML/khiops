// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PEPiTask.h"

PEPiTask::PEPiTask() : PLParallelTask()
{
	nTotalIteration = 0;
	nIterationPerTask = 0;
	nIteration = 0;
	dPi = 0;

	////////////////////////////////////////////////
	// declaration des variables partagees entre le maitre et les esclave

	// Les parametres du programme, affectes une fois
	// pour toutes au debut du programme
	DeclareSharedParameter(&shared_dSlaveStep);

	// Les parametres en entree du traitement des esclaves
	// Ici : le nombre d'iteration et le point depart
	DeclareTaskInput(&input_nIterationNumber);
	DeclareTaskInput(&input_nIterationStart);

	// Les resultats du traitement renvoyes par l'esclave
	// Ici : Approximation partielle de pi par un esclave
	DeclareTaskOutput(&output_dLocalPi);
}

PEPiTask::~PEPiTask() {}

void PEPiTask::SetTotalIteration(int nValue)
{
	require(nValue >= 0);

	// Le parametre utilisateur
	nTotalIteration = nValue;
}

void PEPiTask::ComputePi()
{
	// Preparation d'une barre de progression de la tache, avec titre dependant du
	// mode d'execution
	if (not PLParallelTask::GetDriver()->IsParallelModeAvailable() or
	    RMResourceConstraints::GetMaxCoreNumberOnCluster() == 1)
	{
		TaskProgression::SetTitle("Pi estimator (Sequential mode)");

		// Pour cette tache en particulier, en sequentiel, on ne lance qu'un seul esclave
		TaskProgression::SetDisplayedLevelNumber(3);
	}
	else
	{
		TaskProgression::SetTitle("Pi estimator (parallel mode)");
		TaskProgression::SetDisplayedLevelNumber(3);
	}

	// Lancement de la tache, avec gestion de la barre d'avancement
	TaskProgression::Start();
	Run();
	TaskProgression::Stop();
}

double PEPiTask::GetComputedPi() const
{
	require(IsJobDone());
	return dPi;
}

void PEPiTask::Test(int nSlavesNumber, int nIterationNumber)
{
	PEPiTask piTask;
	ALString sTmp;

	require(nSlavesNumber >= 0);
	require(nIterationNumber >= 0);

	// Parametrage de la tache
	piTask.SetTotalIteration(nIterationNumber);

	// Calcul de pi
	piTask.ComputePi();

	// Affichage des resultats, avec le maximum de precision
	piTask.AddSimpleMessage(sTmp + "Pi = " + DoubleToString(piTask.GetComputedPi()));
	piTask.AddSimpleMessage(sTmp + "Processing time : " + SecondsToString(piTask.GetJobElapsedTime()));
}

const ALString PEPiTask::GetTaskName() const
{
	return "Pi parallel calculator";
}

PLParallelTask* PEPiTask::Create() const
{
	return new PEPiTask;
}

/////////////////////////////////////////////////////////////////////
// Methode du Master

boolean PEPiTask::MasterInitialize()
{
	Global::SetMaxErrorFlowNumber(10);

	// Initialisation des variables
	dPi = 0;
	nIteration = 0;

	// Calcul du pas commun a tous les esclaves
	// Au sortir de cette methode les esclaves auront acces a cette variable
	shared_dSlaveStep = 1.0 / nTotalIteration;

	// Calcul du nombre d'iteration par esclave
	nIterationPerTask = (int)ceil(nTotalIteration * 1.0 / GetProcessNumber());
	return true;
}

boolean PEPiTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	if (nIteration >= nTotalIteration)
	{
		// Fin du travail
		dTaskPercent = 1;
		bIsTaskFinished = true;
	}
	else
	{
		// On borne le nombre d'iteration pour ne pas depasser nIterationMax
		if (nIteration + nIterationPerTask > nTotalIteration)
		{
			nIterationPerTask = nTotalIteration - nIteration;
		}

		// Mise a jour des input qui seront envoyes aux esclave
		// avant le traitement
		input_nIterationNumber = nIterationPerTask;
		input_nIterationStart = nIteration;

		nIteration += nIterationPerTask;

		// Pour afficher la progression, on indique la taille de la tache allouee a
		// l'esclave : le ratio entre la tache locale et la tache globale
		dTaskPercent = 1.0 * nIterationPerTask / nTotalIteration;
	}
	return true;
}

boolean PEPiTask::MasterAggregateResults()
{
	// Aggregation des resultats
	dPi += output_dLocalPi;

	return true;
}

boolean PEPiTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	// Nothing to do
	return true;
}

/////////////////////////////////////////////////////////////////////
// Methode du slave

boolean PEPiTask::SlaveInitialize()
{
	// Nothing to do
	return true;
}

boolean PEPiTask::SlaveProcess()
{
	int nIterBegin;
	int nIterMax;
	int nIterCurrent;
	int nTaskSize;
	double dX;
	double dSum = 0;
	double dProgression;
	ALString sTmp;

	// Recuperation des input envoyes par le master
	nIterBegin = input_nIterationStart;
	nIterMax = input_nIterationStart + input_nIterationNumber;

	// Nombre d'iterations a realiser (pour le calcul de la progression)
	nTaskSize = nIterMax - nIterBegin;

	// Calcul pour les iterations locales a la tache
	for (nIterCurrent = nIterBegin; nIterCurrent < nIterMax; nIterCurrent++)
	{
		dX = (nIterCurrent + 0.5) * shared_dSlaveStep;
		dSum += 4.0 / (1.0 + dX * dX);

		// Suivi de la tache toutes les 100 nIterations
		if (nIterCurrent % 1000 == 0)
		{
			dProgression = (nIterCurrent - nIterBegin) * 1.0 / nTaskSize;
			TaskProgression::DisplayProgression((int)floor(dProgression * 100));
			if (TaskProgression::IsInterruptionRequested())
				break;
		}
	}
	// Mise a jour de l'output pour envoi au master
	output_dLocalPi = dSum * shared_dSlaveStep;
	return true;
}

boolean PEPiTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	SystemSleep(GetProcessId());
	// Nothing to do
	return true;
}
