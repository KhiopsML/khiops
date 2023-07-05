// Copyright (c) 2023 Orange. All rights reserved.
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
// Classe PRChild
//    Enfant
class PRChild : public Object
{
public:
	// Constructeur
	PRChild();
	~PRChild();

	// Copie et duplication
	void CopyFrom(const PRChild* aSource);
	PRChild* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Prenom
	const ALString& GetFirstName() const;
	void SetFirstName(const ALString& sValue);

	// Age
	int GetAge() const;
	void SetAge(int nValue);
	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// ##

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attributs de la classe
	ALString sFirstName;
	int nAge;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& PRChild::GetFirstName() const
{
	return sFirstName;
}

inline void PRChild::SetFirstName(const ALString& sValue)
{
	sFirstName = sValue;
}

inline int PRChild::GetAge() const
{
	return nAge;
}

inline void PRChild::SetAge(int nValue)
{
	nAge = nValue;
}

// ## Custom inlines

// Fonction de comparaison des enfants par age
int PRChildCompareAge(const void* elem1, const void* elem2);

// ##