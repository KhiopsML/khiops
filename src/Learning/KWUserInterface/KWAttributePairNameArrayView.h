// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KWAttributePairName.h"
#include "KWAttributePairNameView.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe KWAttributePairNameArrayView
//    Variable pair
// Editeur de tableau de KWAttributePairName
class KWAttributePairNameArrayView : public UIObjectArrayView
{
public:
	// Constructeur
	KWAttributePairNameArrayView();
	~KWAttributePairNameArrayView();

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

	// Actions utilisateurs
	void RemoveAllPairs();

	// Parametrage du nombre max d'attributs dans la liste
	// Par defaut: 0, signifie pas de limite
	void SetMaxAttributePairNumber(int nValue);
	int GetMaxAttributePairNumber() const;

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// Redefinition de l'insertion dans la liste pour controler le nombre max d'insertion
	void ActionInsertItemAfter() override;

	// Variables
	int nMaxAttributePairNumber;

	// ##
};

// ## Custom inlines

// ##
