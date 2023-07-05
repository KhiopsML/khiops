// Copyright (c) 2023 Orange. All rights reserved.
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
	const ALString& GetFrequencyAttribute() const;
	void SetFrequencyAttribute(const ALString& sValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// Tableau des attributes (KWAttributeName), parametres d'un coclustering
	ObjectArray* GetAttributes();

	// Nombre max d'attributs pour le coclustering
	static int GetMaxCoclusteringAttributeNumber();

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	ALString sFrequencyAttribute;

	// ## Custom implementation

	// Tableau des variables
	ObjectArray oaAttributes;

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& CCCoclusteringSpec::GetFrequencyAttribute() const
{
	return sFrequencyAttribute;
}

inline void CCCoclusteringSpec::SetFrequencyAttribute(const ALString& sValue)
{
	sFrequencyAttribute = sValue;
}

// ## Custom inlines

// ##