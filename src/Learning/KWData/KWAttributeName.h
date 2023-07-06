// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"

////////////////////////////////////////////////////////////
// Classe KWAttributeName
//    Permet un parametrage simple d'autres classes
class KWAttributeName : public Object
{
public:
	// Constructeur
	KWAttributeName();
	~KWAttributeName();

	// Copie et duplication
	void CopyFrom(const KWAttributeName* aSource);
	KWAttributeName* Clone() const;

	// Name
	const ALString& GetName() const;
	void SetName(const ALString& sValue);

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attributs de la classe
	ALString sName;
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& KWAttributeName::GetName() const
{
	return sName;
}

inline void KWAttributeName::SetName(const ALString& sValue)
{
	sName = sValue;
}