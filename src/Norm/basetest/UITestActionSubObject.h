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
// Classe UITestActionSubObject
//    Test sub-object with action
class UITestActionSubObject : public Object
{
public:
	// Constructeur
	UITestActionSubObject();
	~UITestActionSubObject();

	// Copie et duplication
	void CopyFrom(const UITestActionSubObject* aSource);
	UITestActionSubObject* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// File
	const ALString& GetFilePath() const;
	void SetFilePath(const ALString& sValue);

	// Direct file chooser
	boolean GetDirectFileChooser() const;
	void SetDirectFileChooser(boolean bValue);

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
	ALString sFilePath;
	boolean bDirectFileChooser;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& UITestActionSubObject::GetFilePath() const
{
	return sFilePath;
}

inline void UITestActionSubObject::SetFilePath(const ALString& sValue)
{
	sFilePath = sValue;
}

inline boolean UITestActionSubObject::GetDirectFileChooser() const
{
	return bDirectFileChooser;
}

inline void UITestActionSubObject::SetDirectFileChooser(boolean bValue)
{
	bDirectFileChooser = bValue;
}

// ## Custom inlines

// ##
