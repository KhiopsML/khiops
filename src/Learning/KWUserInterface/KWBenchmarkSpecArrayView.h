// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KWBenchmarkSpec.h"
#include "KWBenchmarkSpecView.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe KWBenchmarkSpecArrayView
//    Benchmark
// Editeur de tableau de KWBenchmarkSpec
class KWBenchmarkSpecArrayView : public UIObjectArrayView
{
public:
	// Constructeur
	KWBenchmarkSpecArrayView();
	~KWBenchmarkSpecArrayView();

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
	///// Implementation
protected:
	// ## Custom implementation

	// ##
};

// ## Custom inlines

// ##
