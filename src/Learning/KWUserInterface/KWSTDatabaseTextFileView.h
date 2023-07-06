// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWSTDatabaseTextFile.h"
#include "KWDatabaseView.h"
#include "KWSTDatabaseTextFileDataView.h"

////////////////////////////////////////////////////////////
// Classe KWSTDatabaseTextFileView
// Editeur de KWSTDatabaseTextFile
class KWSTDatabaseTextFileView : public KWDatabaseView
{
public:
	// Constructeur
	KWSTDatabaseTextFileView();
	~KWSTDatabaseTextFileView();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// Creation dynamique
	KWDatabaseView* Create() const override;

	// Nom d'une technologie de base de donnees editee
	ALString GetTechnologyName() const override;

	// Acces a la base
	KWSTDatabaseTextFile* GetSTDatabaseTextFile();

	////////////////////////////////////////////////////////////////////////
	// Fonctionnalite avancee

	// Ajout d'une action de visualisation des premieres lignes d'un fichier
	void AddShowFirstLinesAction();

	// Action de visualisation des premieres lignes d'un fichier
	void ShowFirstLines();

	////////////////////////////////////////////////////////
	///// Implementation
protected:
};
