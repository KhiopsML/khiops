// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWClassManagement.h"
#include "KWClassManagementActionView.h"

#include "KWVersion.h"
#include "KWClassSpecArrayView.h"
#include "KWAttributeStats.h"
#include "KWClassBuilderView.h"
#include "KWResultFilePathBuilder.h"

////////////////////////////////////////////////////////////
// Classe KWClassManagementDialogView
//    Dictionary management
// Editeur de KWClassManagement
class KWClassManagementDialogView : public KWClassManagementActionView
{
public:
	// Constructeur
	KWClassManagementDialogView();
	~KWClassManagementDialogView();

	// Acces a l'objet edite
	KWClassManagement* GetKWClassManagement();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Nouvelles actions specifiques
	void BuildClassDef();
	void EditFile();

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Vue sur le constructeur de dictionnaire, pour en memoriser le parametrage de la base
	// d'une utilisation a l'autre
	KWClassBuilderView classBuilderView;
};
