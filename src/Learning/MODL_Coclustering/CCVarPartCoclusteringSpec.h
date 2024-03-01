// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

#include "KWVersion.h"

// ##

////////////////////////////////////////////////////////////
// Classe CCVarPartCoclusteringSpec
//    Instances Variables coclustering parameters
class CCVarPartCoclusteringSpec : public Object
{
public:
	// Constructeur
	CCVarPartCoclusteringSpec();
	~CCVarPartCoclusteringSpec();

	// Copie et duplication
	void CopyFrom(const CCVarPartCoclusteringSpec* aSource);
	CCVarPartCoclusteringSpec* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Identifier variable
	const ALString& GetIdentifierAttributeName() const;
	void SetIdentifierAttributeName(const ALString& sValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	//////////////////////////////////////////////////////////////////////////////////////
	// Specification utilisateur du coclustering instances x variables
	//   - une variable identifiant
	//      - si non renseigne, on en genere une correspondant au numero de ligne des instances
	//
	// Cela suffit d'un point de vue utilisateur
	// On creera une grille ayant:
	//   - un attribut identifiant (categoriel et present dans le dictionnaire)
	//   - un attribut de type partie de variable
	//       - autant de variable internes qu'il y a de variables numeriques ou categorielles
	//         disponible dans le dictionnaire en entree, en Used, et au moins une

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	ALString sIdentifierAttributeName;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& CCVarPartCoclusteringSpec::GetIdentifierAttributeName() const
{
	return sIdentifierAttributeName;
}

inline void CCVarPartCoclusteringSpec::SetIdentifierAttributeName(const ALString& sValue)
{
	sIdentifierAttributeName = sValue;
}

// ## Custom inlines

// ##
