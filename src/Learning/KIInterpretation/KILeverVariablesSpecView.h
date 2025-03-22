// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWClassSpecView.h"
#include "KIInterpretationDictionary.h"

/////////////////////////////////////////////////////////////////////
// Classe KILeverVariablesSpecView : Vue sur la classe des variables leviers
class KILeverVariablesSpecView : public KWClassSpecView
{
public:
	// Constructeur
	KILeverVariablesSpecView();
	~KILeverVariablesSpecView();

	// Constructeur generique
	KILeverVariablesSpecView* Create() const;

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Redefinition des methode de parametrage de l'objet edite
	void SetObject(Object* object) override;

protected:
};
