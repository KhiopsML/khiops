// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWClassManagement.h"

#include "KWVersion.h"
#include "KWClassSpecArrayView.h"
#include "KWAttributeStats.h"
#include "KWClassBuilderView.h"
#include "KWResultFilePathBuilder.h"

////////////////////////////////////////////////////////////
// Classe KWClassManagementDialogView
//    Dictionary management
// Editeur de KWClassManagement
class KWClassManagementDialogView : public UIObjectView
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

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// Actions de menu
	void BuildClassDef();
	void ReloadFile();
	void EditFile();

	// Enregistrement des classes en proposant un nom de fichier
	void SaveFileUnderName(const ALString& sFileName);

	// Parametrage de l'objet edite
	void SetObject(Object* object) override;

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Acces a l'objet sous son bon type
	KWClassManagement* GetClassManagement();

	// Vue sur le constructeur de dictionnaire, pour en memoriser le parametrage de la base
	// d'une utilisation a l'autre
	KWClassBuilderView classBuilderView;
};
