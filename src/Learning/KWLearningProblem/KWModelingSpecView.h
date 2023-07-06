// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWModelingSpec.h"
#include "KWAttributeConstructionSpecView.h"
#include "KWModelingAdvancedSpecView.h"
#include "KWModelingExpertSpecView.h"

////////////////////////////////////////////////////////////
// Classe KWModelingSpecView
//    Predictors
// Editeur de KWModelingSpec
class KWModelingSpecView : public UIObjectView
{
public:
	// Constructeur
	KWModelingSpecView();
	~KWModelingSpecView();

	// Acces a l'objet edite
	KWModelingSpec* GetKWModelingSpec();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// Parametrage de l'objet edite
	void SetObject(Object* object) override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
};
