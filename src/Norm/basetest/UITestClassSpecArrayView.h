// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
////////////////////////////////////////////////////////////
// Thu Jul 15 10:55:39 2004
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "UITestClassSpec.h"
#include "UITestClassSpecView.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe UITestClassSpecArrayView
//    Test UI (class spec)
// Editeur de tableau de UITestClassSpec
class UITestClassSpecArrayView : public UIObjectArrayView
{
public:
	// Constructeur
	UITestClassSpecArrayView();
	~UITestClassSpecArrayView();

	////////////////////////////////////////////////////////
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

	// Action de test des modification intensives du contenu de la liste
	void TestExtensiveListChanges();

	// Action de test visant a provoquer un crash Java
	void TestJavaCrash();

	// ##

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// ## Custom implementation

	// ##
};

// ## Custom inlines

// ##