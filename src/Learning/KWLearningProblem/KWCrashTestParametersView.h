// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "InputBufferedFile.h"
#include "PLParallelTask.h"
#include "KWDatabaseMemoryGuard.h"
#include "KWVersion.h"

////////////////////////////////////////////////////////////
// Classe KWCrashTestParametersView
//    Crash test parameters
// Editeur des parametres permet de simuler des crash
class KWCrashTestParametersView : public UIObjectView
{
public:
	// Constructeur
	KWCrashTestParametersView();
	~KWCrashTestParametersView();

	////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Reinitialisation des parametres a leur valeur par defaut
	void ResetParameters();

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Initialisation des champs relatifs au crash test des taches
	void InitializeTaskCrashTestFields();
};
