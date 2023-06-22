// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// 2021-04-25 11:10:57
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KWDataGridOptimizerParameters.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe KWDataGridOptimizerParametersView
//    Data Grid optimization
// Editeur de KWDataGridOptimizerParameters
class KWDataGridOptimizerParametersView : public UIObjectView
{
public:
	// Constructeur
	KWDataGridOptimizerParametersView();
	~KWDataGridOptimizerParametersView();

	// Acces a l'objet edite
	KWDataGridOptimizerParameters* GetKWDataGridOptimizerParameters();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	// ##
	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// ## Custom implementation

	// ##
};

// ## Custom inlines

// ##
