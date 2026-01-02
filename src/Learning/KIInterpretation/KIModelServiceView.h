// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KIModelService.h"

// ## Custom includes

#include "KWClassDomain.h"

// ##

////////////////////////////////////////////////////////////
// Classe KIModelServiceView
//    Interpretation service
// Editeur de KIModelService
class KIModelServiceView : public UIObjectView
{
public:
	// Constructeur
	KIModelServiceView();
	~KIModelServiceView();

	// Acces a l'objet edite
	KIModelService* GetKIModelService();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	// Nom de dictionnaire par defaut, qui sera utilise si possible pour le choix du predicteur
	// a l'ouverture de la boite de dialogue
	void SetDefaultClassName(const ALString& sValue);
	const ALString& GetDefaultClassName() const;

	// Base de donnees en entree, utilise potentiellement pour le choix du repertoire en sortie
	void SetTrainDatabase(const KWDatabase* database);
	const KWDatabase* GetTrainDatabase() const;

	// Fichier du dictionnaire en entree, utilise potentiellement pour le choix du repertoire en sortie
	void SetClassFileName(const ALString& sValue);
	const ALString& GetClassFileName() const;

	// Choix d'un nom de fichier dictionnaire avec son repertoire
	// Une boite de dialogue est presentee a l'utilisateur, avec un repertoire de sortie par defaut
	// On retourne un nom de fichier non vide si un choix valide a etet effectue
	ALString ChooseDictionaryFileName(const ALString& sDictionaryPrefix);

	// Reimplementation de la methode Open, pour prealimenter la liste des predicteurs utilisables
	void Open() override;

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// Rafraichissement des specifications liees au predicteur courant selectionne
	virtual void RefreshPredictorSpec(KIModelService* modelService);

	// Parametres de la classe
	ALString sDefaultClassName;
	const KWDatabase* trainDatabase;
	ALString sClassFileName;

	// ##
};

// ## Custom inlines

// ##
