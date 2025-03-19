// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KIModelReinforcer.h"
#include "KIModelServiceView.h"

// ## Custom includes

#include "KIPredictorAttributeArrayView.h"

// ##

////////////////////////////////////////////////////////////
// Classe KIModelReinforcerView
//    Reinforce model
// Editeur de KIModelReinforcer
class KIModelReinforcerView : public KIModelServiceView
{
public:
	// Constructeur
	KIModelReinforcerView();
	~KIModelReinforcerView();

	// Acces a l'objet edite
	KIModelReinforcer* GetKIModelReinforcer();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	// Action de construction d'un dictionnaire de renforcement
	void BuildReinforcementClass();

	// Redefinition des methode de parametrage de l'objet edite
	void SetObject(Object* object) override;

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// Rafraichissement des specifications liees au predicteur courant selectionne
	void RefreshPredictorSpec(KIModelService* modelService) override;

	// Rafraichissement des listes d'aide
	void RefreshHelpLists();

	// ##
};

// ## Custom inlines

// ##
