// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"

////////////////////////////////////////////////////////////
// Classe KWAttributePairName
//    Variable pair
class KWAttributePairName : public Object
{
public:
	// Constructeur
	KWAttributePairName();
	~KWAttributePairName();

	// Copie et duplication
	void CopyFrom(const KWAttributePairName* aSource);
	KWAttributePairName* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Nom du premeier atttribut
	void SetFirstName(const ALString& sValue);
	const ALString& GetFirstName() const;

	// Nom du second attribut
	const ALString& GetSecondName() const;
	void SetSecondName(const ALString& sValue);

	// Nom des attributs tries par ordre alphabetique, pour avoir une facon unique d'identifier la paire
	const ALString& GetFirstSortedName() const;
	const ALString& GetSecondSortedName() const;

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attributs de la classe
	ALString sFirstName;
	ALString sSecondName;
};

// Methode de comparaison base sur le nom de l'attribut
int KWAttributePairNameCompare(const void* elem1, const void* elem2);

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& KWAttributePairName::GetFirstName() const
{
	return sFirstName;
}

inline void KWAttributePairName::SetFirstName(const ALString& sValue)
{
	sFirstName = sValue;
}

inline const ALString& KWAttributePairName::GetSecondName() const
{
	return sSecondName;
}

inline void KWAttributePairName::SetSecondName(const ALString& sValue)
{
	sSecondName = sValue;
}
