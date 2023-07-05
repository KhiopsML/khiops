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
// Classe UITestSubObject
//    Test sub-object
class UITestSubObject : public Object
{
public:
	// Constructeur
	UITestSubObject();
	~UITestSubObject();

	// Copie et duplication
	void CopyFrom(const UITestSubObject* aSource);
	UITestSubObject* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Info
	const ALString& GetInfo() const;
	void SetInfo(const ALString& sValue);

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
	ALString sInfo;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& UITestSubObject::GetInfo() const
{
	return sInfo;
}

inline void UITestSubObject::SetInfo(const ALString& sValue)
{
	sInfo = sValue;
}

// ## Custom inlines

// ##