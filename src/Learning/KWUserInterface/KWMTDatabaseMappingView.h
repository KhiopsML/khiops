// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KWMTDatabaseMapping.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe KWMTDatabaseMappingView
//    Multi-table mapping
// Editeur de KWMTDatabaseMapping
class KWMTDatabaseMappingView : public UIObjectView
{
public:
	// Constructeur
	KWMTDatabaseMappingView();
	~KWMTDatabaseMappingView();

	// Acces a l'objet edite
	KWMTDatabaseMapping* GetKWMTDatabaseMapping();

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
