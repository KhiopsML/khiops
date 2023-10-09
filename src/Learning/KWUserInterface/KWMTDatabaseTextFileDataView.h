// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KWMTDatabaseTextFile.h"

// ## Custom includes

#include "KWMTDatabaseMappingArrayView.h"
#include "KWDatabaseFormatDetectorView.h"

// ##

////////////////////////////////////////////////////////////
// Classe KWMTDatabaseTextFileDataView
//    Data
// Editeur de KWMTDatabaseTextFile
class KWMTDatabaseTextFileDataView : public UIObjectView
{
public:
	// Constructeur
	KWMTDatabaseTextFileDataView();
	~KWMTDatabaseTextFileDataView();

	// Acces a l'objet edite
	KWMTDatabaseTextFile* GetKWMTDatabaseTextFile();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	// Passage en mode ecriture seulement, lecture sinon
	// En mode ecriture seulement, seul le mapping de la classe principale et
	// de sa composition sont specifies
	void ToWriteOnlyMode();

	// Acces au probleme d'apprentissage
	void SetObject(Object* object) override;
	KWMTDatabaseTextFile* GetMTDatabaseTextFile();

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// Mise a jour des mapping multi-tables editables, selon le mode Read ou Write
	void ResfreshMultiTableMapping();

	// Mode ecriture seulement
	boolean bModeWriteOnly;

	// Tableau des mappings
	ObjectArray oaMultiTableMapping;

	// ##
};

// ## Custom inlines

// ##
