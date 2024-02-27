// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

#include "KWAttributeName.h"
#include "KWVersion.h"

// ##

////////////////////////////////////////////////////////////
// Classe CCCoclusteringSpec
//    Coclustering parameters
class CCCoclusteringSpec : public Object
{
public:
	// Constructeur
	CCCoclusteringSpec();
	~CCCoclusteringSpec();

	// Copie et duplication
	void CopyFrom(const CCCoclusteringSpec* aSource);
	CCCoclusteringSpec* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Frequency variable
	const ALString& GetFrequencyAttributeName() const;
	void SetFrequencyAttributeName(const ALString& sValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	//////////////////////////////////////////////////////////////////////////////////////
	// Specification utilisateur du coclustering de variables
	//   - un tableau de nom d'attributs numeriques ou categoriel du dictionnaire
	//   - une variable d'effectif (optionnel)
	//
	// Les variables de la specification doivent etre en Used dans le dictionnaire

	// Tableau des attributes (KWAttributeName), parametres d'un coclustering de variables
	ObjectArray* GetAttributeNames();

	// Nombre max d'attributs pour le coclustering
	static int GetMaxCoclusteringAttributeNumber();

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	ALString sFrequencyAttributeName;

	// ## Custom implementation

	// Tableau des variables
	ObjectArray oaAttributeNames;

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& CCCoclusteringSpec::GetFrequencyAttributeName() const
{
	return sFrequencyAttributeName;
}

inline void CCCoclusteringSpec::SetFrequencyAttributeName(const ALString& sValue)
{
	sFrequencyAttributeName = sValue;
}

// ## Custom inlines

// ##
