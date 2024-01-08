// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KWBenchmarkSpec.h"

// ## Custom includes

#include "KWBenchmarkClassSpecView.h"
#include "KWDatabaseView.h"

// ##

////////////////////////////////////////////////////////////
// Classe KWBenchmarkSpecView
//    Benchmark
// Editeur de KWBenchmarkSpec
class KWBenchmarkSpecView : public UIObjectView
{
public:
	// Constructeur
	KWBenchmarkSpecView();
	~KWBenchmarkSpecView();

	// Acces a l'objet edite
	KWBenchmarkSpec* GetKWBenchmarkSpec();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	// Acces au probleme d'apprentissage
	void SetObject(Object* object) override;
	Object* GetObject() override;
	KWBenchmarkSpec* GetBenchmarkSpec();

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// Gestion des domaines de classes necessaire a l'edition des spec de benchmark
	KWClassDomain* currentClassDomain;
	KWClassDomain* temporaryClassDomain;

	// ##
};

// ## Custom inlines

// ##
