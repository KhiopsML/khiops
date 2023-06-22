// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// 2021-01-21 16:48:22
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "Sample.h"
#include "SampleView.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe SampleArrayView
//    Sample
// Editeur de tableau de Sample
class SampleArrayView : public UIObjectArrayView
{
public:
	// Constructeur
	SampleArrayView();
	~SampleArrayView();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Creation d'un objet (du bon type), suite a une demande d'insertion utilisateur
	Object* EventNew() override;

	// Mise a jour de l'objet correspondant a l'index courant par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet correspondant a l'index courant
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
