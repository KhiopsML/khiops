// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLParallelTask.h"

///////////////////////////////////////////////////////////////////////
// Classe PELullabyTask
// Classe qui permet de tester la methode SetSlaveAtRest()
// Le test est constitue de "pas", un pas contient plusieurs slaveProcess (ou iteration).
// Tous les pas ont le meme nombre de SlaveProcess.
// A chaque pas un esclave est mis en sommeil, jusqu'a ce qu'il n'y est plus qu'un esclave qui travaille
// A la fin de ce pas, tous les esclaves sont remis au travail pour un dernier pas.
// La taille des pas est calculee automatiquement, il faut au minimum un SlaveProcess de chaque esclave
// pour voir tous les esclaves s'endormir 1 par 1. La taille choisie est de 2 * (nombre d'esclaves), pour etre
// sur de voir tout le monde.
class PELullabyTask : public PLParallelTask
{
public:
	// Constructeur
	PELullabyTask();
	~PELullabyTask();

	// Lance la tache, renvoie true si toutes les methodes des maitres et esclaves
	// ont renvoye true
	boolean Run();

	// Test du framework
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

	//////////////////////////////////////////////////////
	// Variables du Master

	int nStepSize;

	// On n'utilise pas GetTaskIndex car celui-ci n'est pas itere lorsqu'on met l'esclave en sommeil
	int nMyTaskIndex;
};
