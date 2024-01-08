// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UserInterface.h"

#include "KWClassSpec.h"

// ## Custom includes

#include "KWClassDomain.h"
#include "KWAttributeSpecArrayView.h"
#include "KWAttributeSpec.h"

// La classe KWClassSpecView permet une edition "legere" d'un dictionnaire
// existant, en modifiant certaines caracteristiques des attributs (Used, Type...)
// Le dictionnaire en sortie est mis a jour et compile s'il est valide

// ##

////////////////////////////////////////////////////////////
// Classe KWClassSpecView
//    Dictionary
// Editeur de KWClassSpec
class KWClassSpecView : public UIObjectView
{
public:
	// Constructeur
	KWClassSpecView();
	~KWClassSpecView();

	// Acces a l'objet edite
	KWClassSpec* GetKWClassSpec();

	///////////////////////////////////////////////////////////
	// Redefinition des methodes a reimplementer obligatoirement

	// Mise a jour de l'objet par les valeurs de l'interface
	void EventUpdate(Object* object) override;

	// Mise a jour des valeurs de l'interface par l'objet
	void EventRefresh(Object* object) override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;

	// ## Custom declarations

	// Action de selection ou deselection de tous les attributs
	void SelectAll();
	void UnselectAll();

	// Acces a l'editeur liste des attributs de la classe
	// Permet de parametrer les champs visibles et editables
	// (par defaut: tous les champs (definis dans KWAttributeSpec)
	// sont visibles et non editables
	// Memoire: la vue retournee appartient a l'appele
	KWAttributeSpecArrayView* GetAttributeSpecArrayView();

	// Redefinition des methode de parametrage de l'objet edite
	void SetObject(Object* object) override;
	Object* GetObject() override;

	// ##
	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// ## Custom implementation

	// Editeur de la liste des specifications d'attributs
	KWAttributeSpecArrayView* attributeSpecArrayView;

	// Tableau des specifications des attributs
	ObjectArray oaAttributeSpecs;

	// ##
};

// ## Custom inlines

// ##
