// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "UITestActionSubObject.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe UITestActionSubObjectView
//    Test sub-object with action
// Editeur de UITestActionSubObject
class UITestActionSubObjectView : public UIObjectView
{
public:
	// Constructeur
	UITestActionSubObjectView();
	~UITestActionSubObjectView();

	// Acces a l'objet edite
	UITestActionSubObject* GetUITestActionSubObject();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	// Actions
	void ChooseFile();
	void AskQuestion();
	void OK();

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// ##
};

// ## Custom inlines

// ##
