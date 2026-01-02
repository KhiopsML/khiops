// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWLearningSpec.h"
#include "RMResourceSystem.h"
#include "KWResultFilePathBuilder.h"
#include "KWVersion.h"

////////////////////////////////////////////////////////////
// Classe KWSystemParametersView
//    System parameters
// Editeur des parametres systemes suivants (variables statiques):
//    RMResourceConstraints::OptimizationTime
//    RMResourceConstraints::MemoryLimit
//    RMResourceConstraints::nMaxCoreNumber
//    FileService::UserTmpDir
//    RMResourceConstraints::SimultaneousDiskAccessNumber
//    PLParallelTask::ParallelLogFileName
//    PLParallelTask::ParallelSimulated
class KWSystemParametersView : public UIObjectView
{
public:
	// Constructeur
	KWSystemParametersView();
	~KWSystemParametersView();

	////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
};
