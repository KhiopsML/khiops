// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

#include "UITestSubObject.h"
#include "UITestActionSubObject.h"

// ##

////////////////////////////////////////////////////////////
// Classe UITestObject
//    Test object
class UITestObject : public Object
{
public:
	// Constructeur
	UITestObject();
	~UITestObject();

	// Copie et duplication
	void CopyFrom(const UITestObject* aSource);
	UITestObject* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Duration
	int GetDuration() const;
	void SetDuration(int nValue);

	// Result
	const ALString& GetResult() const;
	void SetResult(const ALString& sValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	// Acces aux sous-objetx
	UITestSubObject* GetSubObject();
	UITestActionSubObject* GetActionSubObject();

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	int nDuration;
	ALString sResult;

	// ## Custom implementation

	UITestSubObject testSubObject;
	UITestActionSubObject testActionSubObject;

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline int UITestObject::GetDuration() const
{
	return nDuration;
}

inline void UITestObject::SetDuration(int nValue)
{
	nDuration = nValue;
}

inline const ALString& UITestObject::GetResult() const
{
	return sResult;
}

inline void UITestObject::SetResult(const ALString& sValue)
{
	sResult = sValue;
}

// ## Custom inlines

// ##
