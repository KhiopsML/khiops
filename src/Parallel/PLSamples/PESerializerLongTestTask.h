// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "PLParallelTask.h"

/////////////////////////////////////////////////////////////////////////////
// Classe PESerializerLongTestTask
// Cette classe permet de tester la serialisation d'une chaine pour toutes les
// tailles de 0 a n
//
class PESerializerLongTestTask : public PLParallelTask
{
public:
	// Constructeur
	PESerializerLongTestTask();
	~PESerializerLongTestTask();

	// Lance le test : serialize une string de taille croissante dans chaque
	// masterPrepareTaskInput La taille de la string serialisee va de 0 a
	// nStringLength
	boolean TestSerializer(int nStringLength);

	// Methode de test
	// A lancer en simule et en parallele
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

	// Parametres
	int nMaxStringSize;

	// Variable du maitre
	ALString sStringToSerialize;

	// Variables partagees
	PLShared_Int input_nStringLength;
	PLShared_String input_sString;
};
