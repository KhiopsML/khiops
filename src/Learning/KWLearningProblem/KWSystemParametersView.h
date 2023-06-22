// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWLearningSpec.h"
#include "KWVersion.h"

////////////////////////////////////////////////////////////
// Classe KWSystemParametersView
//    System parameters
// Editeur des parametres systemes suivants (variables statiques):
//    KWLearningSpec::MaxModalityNumber
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

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Calcul du nombre de processus utilise a partir de ce que l'utilisateur indique dans l'IHM
	// Si nRequestedCoreNumber == 1 , c'est du sequentiel, on n'utilise qu'un seul processus
	// Sinon on utilise un processus de plus que ce qui est demande sauf si il n'y a pas assez de
	// processus MPI lances
	int ComputeCoreNumber(int nRequestedCoreNumber) const;

	// Pendant de la methode ComputeCoreNumber, renvoie le nombre de coeurs affiche a l'IHM a partir
	// du nombre de processus utilises
	int ComputeRequestedCoreNumber(int nCoreNumber) const;
};
