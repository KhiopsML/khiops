// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "UserInterface.h"
#include "KWClassManagement.h"
#include "KWClassSpecArrayView.h"
#include "KWClassManagementDialogView.h"
#include "KWVersion.h"

////////////////////////////////////////////////////////////
// Classe KWClassManagementActionView
//    Dictionary management
// Editeur de KWClassManagement pour la partie action de menu
class KWClassManagementActionView : public UIObjectView
{
public:
	// Constructeur
	KWClassManagementActionView();
	~KWClassManagementActionView();

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

	// Actions de menu
	void OpenFile();
	void CloseFile();
	void SaveFile();
	void SaveFileUnder();
	void ExportAsJSON();
	void ManageClasses();
	void Quit();

	// Enregistrement des classes en proposant un nom de fichier
	void SaveFileUnderName(const ALString& sFileName);

	// Export des classes au format JSON en proposant un nom de fichier
	void ExportAsJSONFileUnderName(const ALString& sJSONFileName);

	////////////////////////////////////////////////////////////////////////
	// Fonctionnalite avancee
	// Quand la vue de ClassManagement charge un nouveau fichier dictionnaire,
	// elle choisit un nouveau dictionnaire d'analyse par defaut et doit
	// mettre a jour ce dictionnaire dans base de train et test

	// Parametrage de la base d'apprentissage
	// Memoire: appartient a l'appelant
	void SetTrainDatabase(KWDatabase* trainDatabaseSettings);
	KWDatabase* GetTrainDatabase();

	// Parametrage de la base de test
	// Memoire: appartient a l'appelant
	void SetTestDatabase(KWDatabase* testDatabaseSettings);
	KWDatabase* GetTestDatabase();

	// Parametrage de l'objet edite
	void SetObject(Object* object) override;

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Acces a l'objet sous son bon type
	KWClassManagement* GetClassManagement();

	// Verification de la coherence du nom du dictionnaire avec celui des bases
	boolean CheckDictionaryName();

	// Synchronisation du nom de dictionnaire avec les bases de train et test
	void CopyDictionaryNameToDatabases();

	// Bases d'apprentissage et de test
	KWDatabase* trainDatabase;
	KWDatabase* testDatabase;
};