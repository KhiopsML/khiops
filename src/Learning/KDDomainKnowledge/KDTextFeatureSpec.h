// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe KDTextFeatureSpec
//    Text feature parameters
class KDTextFeatureSpec : public Object
{
public:
	// Constructeur
	KDTextFeatureSpec();
	~KDTextFeatureSpec();

	// Copie et duplication
	void CopyFrom(const KDTextFeatureSpec* aSource);
	KDTextFeatureSpec* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Text features
	const ALString& GetTextFeatures() const;
	void SetTextFeatures(const ALString& sValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// Verification de l'intefrite avec message d'erreur
	boolean Check() const override;

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	ALString sTextFeatures;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& KDTextFeatureSpec::GetTextFeatures() const
{
	return sTextFeatures;
}

inline void KDTextFeatureSpec::SetTextFeatures(const ALString& sValue)
{
	sTextFeatures = sValue;
}

// ## Custom inlines

// ##
