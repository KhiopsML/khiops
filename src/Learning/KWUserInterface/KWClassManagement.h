// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

#include "KWClassSpec.h"
#include "KWClassDomain.h"
#include "PLRemoteFileService.h"

// ##

////////////////////////////////////////////////////////////
// Classe KWClassManagement
//    Dictionary management
class KWClassManagement : public Object
{
public:
	// Constructeur
	KWClassManagement();
	~KWClassManagement();

	// Copie et duplication
	void CopyFrom(const KWClassManagement* aSource);
	KWClassManagement* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Analysis dictionary
	const ALString& GetClassName() const;
	void SetClassName(const ALString& sValue);

	// Dictionary file
	const ALString& GetClassFileName() const;
	void SetClassFileName(const ALString& sValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// Acces aux specs des classes
	ObjectArray* GetClassSpecs();

	//////////////////////////////////////////////////////////
	// Gestion des classes et synchronisation avec les specs

	// Lecture des classes
	// On reste dans le meme etat si echec de la lecture
	// Sinon, on a de nouveaux dictionnaire, et on en choisit un par defaut
	// si necessaire
	boolean ReadClasses();

	// Ecriture des classes courantes
	boolean WriteClasses();

	// Export des classes courantes au format JSON
	boolean ExportJSONClasses(const ALString& sJSONFileName);

	// Supression de toutes les classes
	void DropClasses();

	// Synchronisation entre les dictionnaires et leurs specs
	void RefreshClassSpecs();

	// Recherche d'une classe par defaut parmi les classes existante
	//  Rien si aucune classe disponible
	//  La classe si elle est unique
	//  La classe racine s'il n'y en a qu'une
	const ALString SearchDefaultClassName() const;

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	ALString sClassName;
	ALString sClassFileName;

	// ## Custom implementation

	// Specs des classes
	ObjectArray oaClassSpecs;

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& KWClassManagement::GetClassName() const
{
	return sClassName;
}

inline void KWClassManagement::SetClassName(const ALString& sValue)
{
	sClassName = sValue;
}

inline const ALString& KWClassManagement::GetClassFileName() const
{
	return sClassFileName;
}

inline void KWClassManagement::SetClassFileName(const ALString& sValue)
{
	sClassFileName = sValue;
}

// ## Custom inlines

// ##
