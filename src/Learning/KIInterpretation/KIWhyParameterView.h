// Copyright (c) 2023 Orange. All rights reserved.
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

#ifndef ISWHYPARAMETERVIEW_H
#define ISWHYPARAMETERVIEW_H

#include "UserInterface.h"
#include "KIInterpretationSpec.h"

/////////////////////////////////////////////////////////////////////
/// Classe ISParameterView : Vue sur le PARAMETRAGE specifique du calcul
/// des CONTRIBUTIONS d'un interpreteur de scores
class KIWhyParameterView : public UIObjectView
{
public:
	/// Constructeur
	KIWhyParameterView();
	~KIWhyParameterView();

	/// Constructeur generique
	KIWhyParameterView* Create() const;

	////////////////////////////////////////////////////////
	// Methodes a reimplementer dans les sous-classes

	/// Mise a jour du classifieur specifique par les valeurs de l'interface
	void EventUpdate(Object* object);

	/// Mise a jour des valeurs de l'interface par le classifier specifique
	void EventRefresh(Object* object);
};
#endif // ISWHYVIEWPARAMETER_H