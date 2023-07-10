// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Standard.h"
#include "ALString.h"
#include "Object.h"
#include "KWClass.h"

///////////////////////////////////////////////////////////////////////////////////////
// Classe KDEntity
// Designation d'une entite avec ou sans un nom de partie de l'entite
// Par exemple: dictionnaire avec ses variables, regle de derivation et ses operandes
class KDEntity : public Object
{
public:
	// Constructeur et destructeur
	KDEntity();
	~KDEntity();

	// Nom de l'entite
	void SetName(const ALString sValue);
	const ALString& GetName() const;

	// Nom de la partie de l'entite
	void SetPartName(const ALString sValue);
	const ALString& GetPartName() const;

	// Test si l'entite recouvre une paire (nom, partie)
	// L'entite testee ne doit pas etre vide
	// Pour correspondre, il doit y avoir soit egalite des noms, soit vide pour l'entite de reference
	boolean Matches(const ALString& sEntityName, const ALString& sEntityPartName) const;

	// Identifiant de l'entite, exploitant son non d'entite et nom de partie
	const ALString GetId() const;

	// Verification de l'integrite
	// Il ne doit pas y avoir de doublon dans le tableau des entites
	boolean Check() const override;

	// Affichage, ecriture dans un fichier
	// L'affichage d'un nom de partie ou d'entite se fait comme pour les nom de variable des dictionnaire
	// Un point est utilise comme separateur entre entite et partie
	// Un nom vide est affiche avec le caractere '*'
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	ALString sName;
	ALString sPartName;
};

///////////////////////////////////////////////////////////////////////////////////////
// Classe KDEntitySet
// Ensemble d'entite
class KDEntitySet : public Object
{
public:
	// Constructeur et destructeur
	KDEntitySet();
	~KDEntitySet();

	// Mode exclusion (defaut: false)
	// En mode normal, le tableau des entites represente les entites autorisees
	// En mode exclusion, le tableau des entites represente les entites interdites
	void SetExcludeMode(boolean bValue);
	boolean GetExcludeMode() const;

	// Tableau des entites autorisees (ou interdites en mode exclusion)
	// Memoire: le tableau et son contenu appartiennent a l'appele
	ObjectArray* GetEntities();

	// Test si l'ensemble d'entites recouvre une paire (nom, partie)
	// L'entite testee ne doit pas etre vide
	// Pour correspondre, il doit y avoir y avoir correspondance avec une des entites de l'ensemble
	// en mdoe normal, avec aucune en mode exclusion
	boolean Matches(const ALString& sEntityName, const ALString& sEntityPartName) const;

	// Verification de l'integrite
	// Il ne doit pas y avoir de doublon dans le tableau des entites
	boolean Check() const override;

	// Affichage, ecriture dans un fichier
	// L'affichage se fait sous la forme d'un ensemble d'entites entre caracteres '{' et '}'
	// Le mot exclusion est larque en prefixant par un caractere '^'
	void Write(ostream& ost) const override;

	// Libelles utilisateurs
	const ALString GetClassLabel() const override;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	boolean bExcludeMode;
	ObjectArray oaEntities;
};
