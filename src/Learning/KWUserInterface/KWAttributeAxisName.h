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
// Classe KWAttributeAxisName
//    Variable axis
class KWAttributeAxisName : public Object
{
public:
	// Constructeur
	KWAttributeAxisName();
	~KWAttributeAxisName();

	// Copie et duplication
	void CopyFrom(const KWAttributeAxisName* aSource);
	KWAttributeAxisName* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Attribute name
	const ALString& GetAttributeName() const;
	void SetAttributeName(const ALString& sValue);

	// Owner attribute name
	const ALString& GetOwnerAttributeName() const;
	void SetOwnerAttributeName(const ALString& sValue);

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
	ALString sAttributeName;
	ALString sOwnerAttributeName;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& KWAttributeAxisName::GetAttributeName() const
{
	return sAttributeName;
}

inline void KWAttributeAxisName::SetAttributeName(const ALString& sValue)
{
	sAttributeName = sValue;
}

inline const ALString& KWAttributeAxisName::GetOwnerAttributeName() const
{
	return sOwnerAttributeName;
}

inline void KWAttributeAxisName::SetOwnerAttributeName(const ALString& sValue)
{
	sOwnerAttributeName = sValue;
}

// ## Custom inlines

// ##
