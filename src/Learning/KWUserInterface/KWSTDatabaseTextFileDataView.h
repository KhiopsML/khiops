// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KWSTDatabaseTextFile.h"

// ## Custom includes

#include "KWDatabaseFormatDetectorView.h"
#include "KWVersion.h"

// ##

////////////////////////////////////////////////////////////
// Classe KWSTDatabaseTextFileDataView
//    Data
// Editeur de KWSTDatabaseTextFile
class KWSTDatabaseTextFileDataView : public UIObjectView
{
public:
	// Constructeur
	KWSTDatabaseTextFileDataView();
	~KWSTDatabaseTextFileDataView();

	// Acces a l'objet edite
	KWSTDatabaseTextFile* GetKWSTDatabaseTextFile();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	// Acces au probleme d'apprentissage
	void SetObject(Object* object) override;
	KWSTDatabaseTextFile* GetSTDatabaseTextFile();

	////////////////////////////////////////////////////////////////////////
	// Fonctionnalite avancee

	// Ajout d'une action de visualisation des premieres lignes d'un fichier
	void AddShowFirstLinesAction();

	// Action de visualisation des premieres lignes d'un fichier
	void ShowFirstLines();

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// ##
};

// ## Custom inlines

// ##
