// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KWClassManagement.h"

// ## Custom includes

#include "KWVersion.h"
#include "KWClassSpecArrayView.h"
#include "KWAttributeStats.h"
#include "KWClassBuilderView.h"
#include "LMLicenseManager.h"

// ##

////////////////////////////////////////////////////////////
// Classe KWClassManagementView
//    Dictionary management
// Editeur de KWClassManagement
class KWClassManagementView : public UIObjectView
{
public:
	// Constructeur
	KWClassManagementView();
	~KWClassManagementView();

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

	// ## Custom declarations

	// Actions de menu
	void OpenFile();
	void CloseFile();
	void SaveFile();
	void SaveFileUnder();
	void ExportAsJSON();
	void BuildClassDef();
	void ReloadFile();
	void Quit();

	// Enregistrement des classes en proposant un nom de fichier
	void SaveFileUnderName(const ALString& sFileName);

	// Export des classes au format JSON en proposant un nom de fichier
	void ExportAsJSONFileUnderName(const ALString& sJSONFileName);

	// Parametrage de l'objet edite
	void SetObject(Object* object) override;

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// Acces a l'objet sous son bon type
	KWClassManagement* GetClassManagement();

	// Vue sur le constructeur de dictionnaire, pour en memoriser le parametrage de la base
	// d'une utilisation a l'autre
	KWClassBuilderView classBuilderView;

	// ##
};

// ## Custom inlines

// ##