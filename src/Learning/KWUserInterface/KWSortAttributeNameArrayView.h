// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWAttributeNameArrayView.h"
#include "KWClassDomain.h"

////////////////////////////////////////////////////////////
// Classe KWSortAttributeNameArrayView
class KWSortAttributeNameArrayView : public KWAttributeNameArrayView
{
public:
	// Constructeur
	KWSortAttributeNameArrayView();
	~KWSortAttributeNameArrayView();

	// Parametrage de la classe
	void SetClassName(const ALString& sValue);
	const ALString& GetClassName() const;

	// Action de position des variables de cle par rapport a la classe
	// Aucune variable de cle si la classe n'existe pas
	void ActionSelectDefaultKeyAttributes();

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	ALString sClassName;
};
