// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "DTForestParameter.h"
#include "DTDecisionTreeParameterView.h"

/////////////////////////////////////////////////////////////////////
/// Classe DTPredictorParameterView : Vue sur le PARAMETRAGE specifique d'un
/// classifieur de type arbre
class DTForestParameterView : public UIObjectView // public DTDecisionTreeParameterView
{
public:
	/// Constructeur
	DTForestParameterView();
	~DTForestParameterView();

	/// Constructeur generique
	DTForestParameterView* Create() const;

	////////////////////////////////////////////////////////
	// Methodes a reimplementer dans les sous-classes

	/// Mise a jour du classifieur specifique par les valeurs de l'interface
	void EventUpdate(Object* object);

	/// Mise a jour des valeurs de l'interface par le classifier specifique
	void EventRefresh(Object* object);

protected:
};