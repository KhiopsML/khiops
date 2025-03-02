// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "PEPi.h"

// ## Custom includes
#include "SystemResource.h"
// ##

////////////////////////////////////////////////////////////
// Classe PEPiView
//    Pi parallel computation
// Editeur de PEPi
class PEPiView : public UIObjectView
{
public:
	// Constructeur
	PEPiView();
	~PEPiView();

	// Acces a l'objet edite
	PEPi* GetPEPi();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	// Methode de test
	// Lancement d'une vue permettant de parametrer et declencher le calcul de Pi
	// en parallele
	static void Test(int argv, char** argc);

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation
	//  Action de calcul
	void ComputePi();

	// ##
};

// ## Custom inlines

// ##
