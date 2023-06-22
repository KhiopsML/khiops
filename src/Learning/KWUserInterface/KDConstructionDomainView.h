// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KDConstructionDomain.h"

// ## Custom includes

#include "KDConstructionRuleArrayView.h"

// ##

////////////////////////////////////////////////////////////
// Classe KDConstructionDomainView
//    Variable construction parameters
// Editeur de KDConstructionDomain
class KDConstructionDomainView : public UIObjectView
{
public:
	// Constructeur
	KDConstructionDomainView();
	~KDConstructionDomainView();

	// Acces a l'objet edite
	KDConstructionDomain* GetKDConstructionDomain();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	// Selection par defaut des regles de construction
	void DefaultSelection();

	// Action de selection ou deselection de toutes les regles de construction
	void SelectAll();
	void UnselectAll();

	// Acces au probleme d'apprentissage
	void SetObject(Object* object) override;
	KDConstructionDomain* GetConstructionDomain();

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// ##
};

// ## Custom inlines

// ##
