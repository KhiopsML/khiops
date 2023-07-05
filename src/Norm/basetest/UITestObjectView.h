// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "UITestObject.h"

// ## Custom includes

#include "UITestSubObjectView.h"
#include "UITestActionSubObjectView.h"

// ##

////////////////////////////////////////////////////////////
// Classe UITestObjectView
//    Test object
// Editeur de UITestObject
class UITestObjectView : public UIObjectView
{
public:
	// Constructeur
	UITestObjectView();
	~UITestObjectView();

	// Acces a l'objet edite
	UITestObject* GetUITestObject();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	// Actions de la fiche
	void InspectSubObject();
	void StartBatchProcessing();

	// Parametrage de l'objet edite
	void SetObject(Object* object) override;

	// Acces a l'objet edite
	UITestObject* GetTestObject();

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// ##
};

// ## Custom inlines

// ##