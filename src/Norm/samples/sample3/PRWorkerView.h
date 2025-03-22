// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "PRWorker.h"

// ## Custom includes

#include "PRChildArrayView.h"
#include "PRAddressView.h"

// ##

////////////////////////////////////////////////////////////
// Classe PRWorkerView
//    Employe
// Editeur de PRWorker
class PRWorkerView : public UIObjectView
{
public:
	// Constructeur
	PRWorkerView();
	~PRWorkerView();

	// Acces a l'objet edite
	PRWorker* GetPRWorker();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	// Action d'ecriture d'un rapport
	void ActionWriteReport();

	// Acction d'acces a l'adresse professionnelle
	void ActionInspectProfessionnalAddress();

	// Acces a l'objet edite par la vue
	void SetObject(Object* object) override;
	PRWorker* GetWorker();

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// ##
};

// ## Custom inlines

// ##
