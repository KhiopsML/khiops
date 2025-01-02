// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLParallelTask.h"

///////////////////////////////////////////////////////////////////////
// Classe PEProtocolTestTask
// Classe de test qui permet de tester le protocole MPI maitre/esclaves
// ainsi que le comportement en cas d'echec des methodes du maitre et des
// esclaves (comme SlaveInitialize()) Cette classe est tres legere, aucun
// traitement n'est realise.
class PEProtocolTestTask : public PLParallelTask
{
public:
	// Constructeur
	PEProtocolTestTask();
	~PEProtocolTestTask();

	// Met tous les parametre dans l'etat initial
	void Init();

	// Nombre d'iterations du programme normal
	// par defaut 1000
	void SetIterationNumber(int nNumber);
	int GetIterationNumber() const;

	// Est-ce que la methode SlaveInitialize() a un probleme (false par defaut)
	void SetSlaveInitializeIssue(boolean bIssue);
	boolean GetSlaveInitializeIssue() const;

	// Iteration a laquelle la methode SlaveProcess renvoie false
	// Par defaut a l'infini : ne renvoie jamais false
	// 0 = infini
	void SetSlaveProcessIssue(int nIterationNumber);
	int GetSlaveProcessIssue() const;

	// Est-ce que la methode SlaveFinalize() a un probleme (false par defaut)
	void SetSlaveFinalizeIssue(boolean bIssue);
	boolean GetSlaveFinalizeIssue() const;

	// Est-ce que la methode MasterInitialize() a un probleme (false par defaut)
	void SetMasterInitializeIssue(boolean bIssue);
	boolean GetMasterInitializeIssue() const;

	// Est-ce que la methode MasterPrepareTaskInput() a un probleme (false par
	// defaut) Actif seulement dans le cas d'une erreur fatale
	void SetMasterPrepareTaskInputIssue(int nIterationNumberIssue);
	int GetMasterPrepareTaskInputIssue() const;

	// Iteration a laquelle la methode MasterAggregateResults renvoie false
	// Par defaut a l'infini : ne renvoie jamais false
	// 0 = infini
	void SetMasterAggregateIssue(int nIterationNumber);
	int GetMasterAggregateIssue() const;

	// Est-ce que la methode MasterFinalize() a un probleme (false par defaut)
	void SetMasterFinalizeIssue(boolean bIssue);
	boolean GetMasterFinalizeIssue() const;

	// Interruption utilisateur au bout de nSeconds secondes
	// 0 secondes = pas d'interruption
	// Doit etre utilise en mode graphic sinon il n'y a pas d'interruption utilisateur
	void SetInterruptionByUser(int nSeconds);
	boolean GetInterruptionByUser() const;

	// Les erreurs sont traitees comme des erreur Fatal
	// AddFatalError est appele au lieu de renvoyer false
	void SetIssueAsFatalError(boolean bIsFatal);
	boolean GetIssueAsFatalError() const;

	// Lance la tache, renvoie true si toutes les methodes des maitres et esclaves
	// ont renvoye true
	boolean Run();

	// Test du framework
	static boolean Test();

	// Test des erreurs fatales, nTestIndex de 0 a 7
	static void TestFatalError(int nTestIndex);

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

	PLShared_Boolean shared_bSlaveInitializeIssue;
	PLShared_Boolean shared_bSlaveFinalizeIssue;
	PLShared_Int shared_nSlaveProcessIssue;
	PLShared_Boolean shared_bFatalError;

	//////////////////////////////////////////////////////
	// Input de la tache parallelisee
	// Envoyes par le master aux esclaves avant chaque tache

	PLShared_Boolean input_bSlaveProcessFatalError;

	//////////////////////////////////////////////////////
	// Resultats de la tache executee par un esclave
	// Envoyes par les esclaves au Master a l'issue de chaque tache

	//////////////////////////////////////////////////////
	// Variables du Master
	boolean bMasterInitializeIssue;
	int nMasterPrepareTaskInputIssue;
	boolean bMasterFinalizeIssue;
	int nMasterAggregateIssue;
	int nIterationNumber;
	int nTimeBeforeInterruption;
};
