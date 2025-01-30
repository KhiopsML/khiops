// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
#include "PLParallelTask.h"

/////////////////////////////////////////////////////////////////////////////
// Classe PESerializerTestTask
// Cette classe permet de tester le serializer (classe PLSerializer) en mode
// standard et parallele Elle test l'envoi de types simples et la grosse
// volumetrie (variables de plus d'un bloc memoire).
//
class PESerializerTestTask : public PLParallelTask
{
public:
	// Constructeur
	PESerializerTestTask();
	~PESerializerTestTask();

	// Lance le test
	boolean TestSerializer(int nLargeCharVectorSize, int nLargeStringSize);

	boolean TestSerializerLong();

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
	int nLargeCharVectorSize;
	int nLargeStringSize;

	// Variable du maitre
	ALString sSimpleStringToSerialize;
	ALString sSimpleCharVectorToSerialize;

	// Variables partagees
	PLShared_String input_sString;
	PLShared_String input_sEmptyString;
	PLShared_String input_sLargeString;
	PLShared_CharVector input_cvCharVector;
	PLShared_CharVector input_cvLargeCharVector;
	PLShared_CharVector input_cvEmptyCharVector;
	PLShared_IntVector input_ivIntVector;
	PLShared_Char input_char;

	PLShared_Int shared_nLargeCharVectorSize;
	PLShared_Int shared_nLargeStringSize;
};
