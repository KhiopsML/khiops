// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "DTGlobalTag.h"
#include "UserInterface.h"
#include "DTDecisionTreeParameter.h"

/////////////////////////////////////////////////////////////////////
// Classe DTDecisionTreeParameterView : Vue sur le PARAMETRAGE specifique d'un
// classifieur de type arbre
class DTDecisionTreeParameterView : public UIObjectView
{
public:
	// Constructeur
	DTDecisionTreeParameterView();
	~DTDecisionTreeParameterView();

	// Constructeur generique
	DTDecisionTreeParameterView* Create() const;

	////////////////////////////////////////////////////////
	// Methodes a reimplementer dans les sous-classes

	// Mise a jour du classifieur specifique par les valeurs de l'interface
	void EventUpdate(Object* object);

	// Mise a jour des valeurs de l'interface par le classifier specifique
	void EventRefresh(Object* object);

	// nom des champs
};
