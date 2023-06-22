// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWAttributePairsSpecFileView;

#include "UserInterface.h"
#include "KWAttributePairsSpec.h"

////////////////////////////////////////////////////////////
// Classe KWAttributePairsSpecFileView
//    Actions d'import/export du parametrage des paires de variables
class KWAttributePairsSpecFileView : public UIObjectView
{
public:
	// Constructeur
	KWAttributePairsSpecFileView();
	~KWAttributePairsSpecFileView();

	////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Actions utilisateurs
	void ImportPairs();
	void ExportPairs();

	// Acces a la specification des paires
	KWAttributePairsSpec* GetAttributePairsSpec();
};
