// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"

#include "CCLearningProblem.h"

////////////////////////////////////////////////////////////
// Classe CCLearningProblemToolView
class CCLearningProblemToolView : public UIObjectView
{
public:
	// Constructeur
	CCLearningProblemToolView();
	~CCLearningProblemToolView();

	////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Actions disponibles
	void SelectInputCoclustering();

	// Acces au probleme d'apprentissage
	CCLearningProblem* GetLearningProblem();

	/////////////////////////////////////////////////////////////
	///// Implementation
protected:
};