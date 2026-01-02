// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KWRecodingSpec.h"

// ## Custom includes

#include "KWVersion.h"

// ##

////////////////////////////////////////////////////////////
// Classe KWRecodingSpecView
//    Recoding parameters
// Editeur de KWRecodingSpec
class KWRecodingSpecView : public UIObjectView
{
public:
	// Constructeur
	KWRecodingSpecView();
	~KWRecodingSpecView();

	// Acces a l'objet edite
	KWRecodingSpec* GetKWRecodingSpec();

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
	///// Implementation
protected:
	// ## Custom implementation

	// ##
};

// ## Custom inlines

// ##
