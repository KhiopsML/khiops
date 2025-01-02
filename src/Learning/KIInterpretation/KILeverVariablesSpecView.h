// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

/*
 * #%L
 * Software Name: Khiops Interpretation
 * Version : 9.0
 * %%
 * Copyright (C) 2019 Orange
 * This software is the confidential and proprietary information of Orange.
 * You shall not disclose such confidential information and shall use it only
 * in accordance with the terms of the license agreement you entered into
 * with Orange.
 * #L%
 */

#pragma once

#include "UserInterface.h"
#include "KWClassManagementView.h"

/////////////////////////////////////////////////////////////////////
/// Classe KILeverVariablesSpecView : Vue sur la classe des variables leviers
class KILeverVariablesSpecView : public KWClassSpecView
{
public:
	/// Constructeur
	KILeverVariablesSpecView();
	~KILeverVariablesSpecView();

	/// Constructeur generique
	KILeverVariablesSpecView* Create() const;

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Redefinition des methode de parametrage de l'objet edite
	void SetObject(Object* object) override;

protected:
};
