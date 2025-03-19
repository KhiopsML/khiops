// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KIModelService.h"

// ## Custom includes

#include "KWClassDomain.h"

// ##

////////////////////////////////////////////////////////////
// Classe KIModelServiceView
//    Interpretation service
// Editeur de KIModelService
class KIModelServiceView : public UIObjectView
{
public:
	// Constructeur
	KIModelServiceView();
	~KIModelServiceView();

	// Acces a l'objet edite
	KIModelService* GetKIModelService();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	// Nom de dictionnaire par defaut, qui sera utilise si possible pour le choix du predicteur
	// a l'ouverture de la boite de dialogue
	void SetDefaultClassName(const ALString& sValue);
	const ALString& GetDefaultClassName() const;

	// Reimplementation de la methode Open, pour prealimenter la liste des predicteurs utilisables
	void Open() override;

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// Rafraichissement des specifications liees au predicteur courant selectionne
	virtual void RefreshPredictorSpec(KIModelService* modelService);

	// Nom de dictionnaire par defaut
	ALString sDefaultClassName;

	// ##
};

// ## Custom inlines

// ##
