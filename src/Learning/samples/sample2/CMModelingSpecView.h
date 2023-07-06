// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "CMModelingSpec.h"
#include "KWModelingSpecView.h"

////////////////////////////////////////////////////////////
/// Classe CMModelingSpecView :
//    Modeling parameters
// Editeur de CMModelingSpec
class CMModelingSpecView : public KWModelingSpecView
{
public:
	/// Constructeur
	CMModelingSpecView();
	~CMModelingSpecView();

	////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	/// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object);

	/// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object);

	/// Libelles utilisateur
	const ALString GetClassLabel() const;

	/// Parametrage de l'objet edite
	void SetObject(Object* object);

	////////////////////////////////////////////////////////
	//// Implementation
protected:
};