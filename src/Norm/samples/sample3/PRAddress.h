// Copyright (c) 2023-2026 Orange. All rights reserved.
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
// Classe PRAddress
//    Adresse
class PRAddress : public Object
{
public:
	// Constructeur
	PRAddress();
	~PRAddress();

	// Copie et duplication
	void CopyFrom(const PRAddress* aSource);
	PRAddress* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Code postal
	const ALString& GetZipCode() const;
	void SetZipCode(const ALString& sValue);

	// Ville
	const ALString& GetCity() const;
	void SetCity(const ALString& sValue);

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
	///// Implementation
protected:
	// Attributs de la classe
	ALString sZipCode;
	ALString sCity;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& PRAddress::GetZipCode() const
{
	return sZipCode;
}

inline void PRAddress::SetZipCode(const ALString& sValue)
{
	sZipCode = sValue;
}

inline const ALString& PRAddress::GetCity() const
{
	return sCity;
}

inline void PRAddress::SetCity(const ALString& sValue)
{
	sCity = sValue;
}

// ## Custom inlines

// ##
