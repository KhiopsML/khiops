// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

/*
 * #%L
 * Software Name: Khiops Interpretation
 * Version : 9.0
 * %%
 * Copyright (C) 2018 Orange
 * %%
 * Authors: Marc Boull�, Bruno Guerraz, Carine Hue, Felipe Olmos, Nicolas Voisine
 *
 * This software is the confidential and proprietary information of Orange.
 * You shall not disclose such confidential information and shall use it only
 * in accordance with the terms of the license agreement you entered into
 * with Orange.
 * #L%
 */

#pragma once

#include "UserInterface.h"

#include "KIInterpretationSpec.h"
#include "KIWhyParameterView.h"
#include "KIHowParameterView.h"

#include "KWVersion.h"

////////////////////////////////////////////////////////////
// Classe KIInterpretationSpecView

// Editeur de KIInterpretationSpec
class KIInterpretationSpecView : public UIObjectView
{
public:
	// Constructeur
	KIInterpretationSpecView();
	~KIInterpretationSpecView();

	// Acces a l'objet edite
	KIInterpretationSpec* GetKIInterpretationSpec();

	////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	void SetObject(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	KIWhyParameterView* whyParameterView;
	KIHowParameterView* howParameterView;
};

// ## Custom inlines

// ##
