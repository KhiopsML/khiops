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
// Classe KWClassSpec
//    Dictionary
class KWClassSpec : public Object
{
public:
	// Constructeur
	KWClassSpec();
	~KWClassSpec();

	// Copie et duplication
	void CopyFrom(const KWClassSpec* aSource);
	KWClassSpec* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Name
	const ALString& GetClassName() const;
	void SetClassName(const ALString& sValue);

	// Root
	boolean GetRoot() const;
	void SetRoot(boolean bValue);

	// Key
	const ALString& GetKey() const;
	void SetKey(const ALString& sValue);

	// Variables
	int GetAttributeNumber() const;
	void SetAttributeNumber(int nValue);

	// Categorical
	int GetSymbolAttributeNumber() const;
	void SetSymbolAttributeNumber(int nValue);

	// Numerical
	int GetContinuousAttributeNumber() const;
	void SetContinuousAttributeNumber(int nValue);

	// Derived
	int GetDerivedAttributeNumber() const;
	void SetDerivedAttributeNumber(int nValue);

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
	ALString sClassName;
	boolean bRoot;
	ALString sKey;
	int nAttributeNumber;
	int nSymbolAttributeNumber;
	int nContinuousAttributeNumber;
	int nDerivedAttributeNumber;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& KWClassSpec::GetClassName() const
{
	return sClassName;
}

inline void KWClassSpec::SetClassName(const ALString& sValue)
{
	sClassName = sValue;
}

inline boolean KWClassSpec::GetRoot() const
{
	return bRoot;
}

inline void KWClassSpec::SetRoot(boolean bValue)
{
	bRoot = bValue;
}

inline const ALString& KWClassSpec::GetKey() const
{
	return sKey;
}

inline void KWClassSpec::SetKey(const ALString& sValue)
{
	sKey = sValue;
}

inline int KWClassSpec::GetAttributeNumber() const
{
	return nAttributeNumber;
}

inline void KWClassSpec::SetAttributeNumber(int nValue)
{
	nAttributeNumber = nValue;
}

inline int KWClassSpec::GetSymbolAttributeNumber() const
{
	return nSymbolAttributeNumber;
}

inline void KWClassSpec::SetSymbolAttributeNumber(int nValue)
{
	nSymbolAttributeNumber = nValue;
}

inline int KWClassSpec::GetContinuousAttributeNumber() const
{
	return nContinuousAttributeNumber;
}

inline void KWClassSpec::SetContinuousAttributeNumber(int nValue)
{
	nContinuousAttributeNumber = nValue;
}

inline int KWClassSpec::GetDerivedAttributeNumber() const
{
	return nDerivedAttributeNumber;
}

inline void KWClassSpec::SetDerivedAttributeNumber(int nValue)
{
	nDerivedAttributeNumber = nValue;
}

// ## Custom inlines

// ##