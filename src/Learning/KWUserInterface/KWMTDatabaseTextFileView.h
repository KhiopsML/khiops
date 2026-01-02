// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWMTDatabaseTextFile.h"
#include "KWDatabaseView.h"
#include "KWMTDatabaseTextFileDataView.h"

////////////////////////////////////////////////////////////
// Classe KWMTDatabaseTextFileView
// Editeur de KWMTDatabaseTextFile
class KWMTDatabaseTextFileView : public KWDatabaseView
{
public:
	// Constructeur
	KWMTDatabaseTextFileView();
	~KWMTDatabaseTextFileView();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// Indique si on est en mode multi-table
	boolean IsMultiTableTechnology() const override;

	// Parametrage du nombre de table editable, si le mode le permet
	void SetEditableTableNumber(int nValue) override;
	int GetEditableTableNumber() const override;

	// Parametrage du mode d'utilisation de la base: general ou ecriture seulement
	// En mode ecriture seulement, seul le mapping de la classe principale et
	// de sa composition sont specifies
	void ToWriteOnlyMode() override;

	// Creation dynamique
	KWDatabaseView* Create() const override;

	// Nom d'une technologie de base de donnees editee
	ALString GetTechnologyName() const override;

	// Acces a la base
	KWMTDatabaseTextFile* GetMTDatabaseTextFile();

	////////////////////////////////////////////////////////
	///// Implementation
protected:
};
