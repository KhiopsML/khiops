// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once
////////////////////////////////////////////////////////////
// Thu Jul 15 10:55:39 2004
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe UITestClassSpec
//    Test UI (class spec)
class UITestClassSpec : public Object
{
public:
	// Constructeur
	UITestClassSpec();
	~UITestClassSpec();

	// Duplication
	UITestClassSpec* Clone() const;

	////////////////////////////////////////////////////////
	// Acces aux attributs

	// Nom
	const ALString& GetClassName() const;
	void SetClassName(const ALString& sValue);

	// Attributs
	int GetAttributeNumber() const;
	void SetAttributeNumber(int nValue);

	// Modaux
	int GetSymbolAttributeNumber() const;
	void SetSymbolAttributeNumber(int nValue);

	// Continus
	int GetContinuousAttributeNumber() const;
	void SetContinuousAttributeNumber(int nValue);

	// Calcules
	int GetDerivedAttributeNumber() const;
	void SetDerivedAttributeNumber(int nValue);

	////////////////////////////////////////////////////////
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
	ALString sClassName;
	int nAttributeNumber;
	int nSymbolAttributeNumber;
	int nContinuousAttributeNumber;
	int nDerivedAttributeNumber;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& UITestClassSpec::GetClassName() const
{
	return sClassName;
}

inline void UITestClassSpec::SetClassName(const ALString& sValue)
{
	sClassName = sValue;
}

inline int UITestClassSpec::GetAttributeNumber() const
{
	return nAttributeNumber;
}

inline void UITestClassSpec::SetAttributeNumber(int nValue)
{
	nAttributeNumber = nValue;
}

inline int UITestClassSpec::GetSymbolAttributeNumber() const
{
	return nSymbolAttributeNumber;
}

inline void UITestClassSpec::SetSymbolAttributeNumber(int nValue)
{
	nSymbolAttributeNumber = nValue;
}

inline int UITestClassSpec::GetContinuousAttributeNumber() const
{
	return nContinuousAttributeNumber;
}

inline void UITestClassSpec::SetContinuousAttributeNumber(int nValue)
{
	nContinuousAttributeNumber = nValue;
}

inline int UITestClassSpec::GetDerivedAttributeNumber() const
{
	return nDerivedAttributeNumber;
}

inline void UITestClassSpec::SetDerivedAttributeNumber(int nValue)
{
	nDerivedAttributeNumber = nValue;
}

// ## Custom inlines

// ##
