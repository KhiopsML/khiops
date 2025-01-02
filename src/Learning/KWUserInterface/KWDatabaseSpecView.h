// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWDatabase.h"
#include "KWDatabaseSamplingView.h"
#include "KWDatabaseSelectionView.h"

////////////////////////////////////////////////////////////
// Classe KWDatabaseSpecView
//
// Editeur generique de KWDatabase en trois sous-onglets d'identifiants
//   - Data: specification du ou des fichiers et de leur format, a parametrer
//   - Sampling: specification d'echantillonnage
//   - Selection: specification de selection
class KWDatabaseSpecView : public UIObjectView
{
public:
	// Constructeur
	KWDatabaseSpecView();
	~KWDatabaseSpecView();

	// Parametrage de la fiche de specification de Data
	// Cette fiche depend de la technologie de base utilises (mono-table, multi-table...)
	// et doit etre specifiee avant utilisation de la classre
	// Memoire: appartient a l'appele
	void SetDataView(UIObjectView* formatView);
	UIObjectView* GetDataView();

	// Passage en mode de lecture basique
	// En mode lecture basique, seule la fiche de saisie Data de la base est visible (avec le detecteur de format),
	// les fiches de Sampling et Selection devenant invisibles
	void ToBasicReadMode();

	// Passage en mode ecriture seulement, lecture sinon
	// En mode ecriture seulement, seule la fiche de saisie Data de la base est visible (sans le detecteur de
	// format), les fiches de Sampling et Selection devenant invisibles
	void ToWriteOnlyMode();

	// Acces a l'objet edite
	KWDatabase* GetKWDatabase();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// Controle d'integrite
	boolean Check() const override;

	// Parametrage de la base et de ses vues
	void SetObject(Object* object) override;
};
