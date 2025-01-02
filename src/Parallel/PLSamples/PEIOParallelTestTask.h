// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLParallelTask.h"
#include "TaskProgression.h"
#include "UserInterface.h"

/////////////////////////////////////////////////////////////////////////////
// Classe PEIOParallelTestTask
// Classe de test de la classe PLParallelTask en utilisant les
// InputBufferdFile paralleles. Cette classe permet de compter le nombre total
// de champs, lignes et octets de plusieurs fichiers tabules. Chaque fichier est
// attribue a un esclave. L'algorithme n'est pas optimal : la duree du programme
// est egale a la duree du comptage sur le plus gros fichier en sequentiel
class PEIOParallelTestTask : public PLParallelTask
{
public:
	// Constructeur
	PEIOParallelTestTask();
	~PEIOParallelTestTask();

	// Ajout d'un fichier dont on doit compter les lignes
	void AddFileName(const ALString& sName);

	// Lance le calcul des stats sur les fichiers
	void ComputeFilesStats();

	// Resultats sur l'ensemble des fichiers
	longint GetTotalFieldNumber() const;
	longint GetTotalLineNumber() const;

	// Methode de test, avec en parametre le nombre d'esclaves et la liste des
	// fichiers
	static void TestCountTotalLines(int nSlaveNumber, const StringVector* svFilesName);

	// Methode de test
	static void Test();

	//////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Reimplementation des methodes virtuelles de tache
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean ComputeResourceRequirements() override;
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

	//////////////////////////////////////////////////////
	// Input de la tache parallelisee
	// Envoyes par le master aux esclaves avant chaque tache
	PLShared_String input_sFileToProcess;

	//////////////////////////////////////////////////////
	// Resultats de la tache executee par un esclave
	// Envoyes par les esclaves au Maitre a l'issue de chaque tache

	// Nombre de champs et lignes lus lors de chaque traitement
	PLShared_Longint output_lFieldNumber;
	PLShared_Longint output_lLineNumber;

	//////////////////////////////////////////////////////
	// Variables du Master

	// Index du fichier courant
	int nFileIndex;

	// Fichiers d'entree
	StringVector svFileName;

	// Resultats du programme
	longint lTotalFieldNumber;
	longint lTotalLineNumber;

	// Taille des fichiers
	longint lTotalFileSizes;
};
