// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// 2021-04-25 11:10:57
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KWAttributeName.h"
#include "KWAttributeNameView.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe KWAttributeNameArrayView
//    Variable
// Editeur de tableau de KWAttributeName
class KWAttributeNameArrayView : public UIObjectArrayView
{
public:
	// Constructeur
	KWAttributeNameArrayView();
	~KWAttributeNameArrayView();

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

	// Parametrage du nombre max d'attributs dans la liste
	// Par defaut: 0, signifie pas de limite
	void SetMaxAttributeNumber(int nValue);
	int GetMaxAttributeNumber() const;

	// Parametrage de l'adjectif qualificatif pour designer les attributs gerees par la liste
	// Pardefaut: rien
	void SetAttributeLabel(const ALString& sValue);
	const ALString& GetAttributeLabel();

	// ##

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// ## Custom implementation

	// Redefinition de l'insertion dans la liste pour controler le nombre max d'insertion
	void ActionInsertItemAfter() override;

	// Variables
	int nMaxAttributeNumber;
	ALString sAttributeLabel;

	// ##
};

// ## Custom inlines

// ##
