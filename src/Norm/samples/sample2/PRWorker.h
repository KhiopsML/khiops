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
// Classe PRWorker
//    Employe
class PRWorker : public Object
{
public:
	// Constructeur
	PRWorker();
	~PRWorker();

	// Copie et duplication
	void CopyFrom(const PRWorker* aSource);
	PRWorker* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Prenom
	const ALString& GetFirstName() const;
	void SetFirstName(const ALString& sValue);

	// Nom
	const ALString& GetFamilyName() const;
	void SetFamilyName(const ALString& sValue);
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
	ALString sFamilyName;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& PRWorker::GetFirstName() const
{
	return sFirstName;
}

inline void PRWorker::SetFirstName(const ALString& sValue)
{
	sFirstName = sValue;
}

inline const ALString& PRWorker::GetFamilyName() const
{
	return sFamilyName;
}

inline void PRWorker::SetFamilyName(const ALString& sValue)
{
	sFamilyName = sValue;
}

// ## Custom inlines

// ##
