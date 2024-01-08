// Copyright (c) 2024 Orange. All rights reserved.
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
// Classe KWAttributeSpec
//    Variable
class KWAttributeSpec : public Object
{
public:
	// Constructeur
	KWAttributeSpec();
	~KWAttributeSpec();

	// Copie et duplication
	void CopyFrom(const KWAttributeSpec* aSource);
	KWAttributeSpec* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Used
	boolean GetUsed() const;
	void SetUsed(boolean bValue);

	// Type
	const ALString& GetType() const;
	void SetType(const ALString& sValue);

	// Name
	const ALString& GetName() const;
	void SetName(const ALString& sValue);

	// Derived
	boolean GetDerived() const;
	void SetDerived(boolean bValue);

	// Meta-data
	const ALString& GetMetaData() const;
	void SetMetaData(const ALString& sValue);

	// Label
	const ALString& GetLabel() const;
	void SetLabel(const ALString& sValue);

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
	boolean bUsed;
	ALString sType;
	ALString sName;
	boolean bDerived;
	ALString sMetaData;
	ALString sLabel;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline boolean KWAttributeSpec::GetUsed() const
{
	return bUsed;
}

inline void KWAttributeSpec::SetUsed(boolean bValue)
{
	bUsed = bValue;
}

inline const ALString& KWAttributeSpec::GetType() const
{
	return sType;
}

inline void KWAttributeSpec::SetType(const ALString& sValue)
{
	sType = sValue;
}

inline const ALString& KWAttributeSpec::GetName() const
{
	return sName;
}

inline void KWAttributeSpec::SetName(const ALString& sValue)
{
	sName = sValue;
}

inline boolean KWAttributeSpec::GetDerived() const
{
	return bDerived;
}

inline void KWAttributeSpec::SetDerived(boolean bValue)
{
	bDerived = bValue;
}

inline const ALString& KWAttributeSpec::GetMetaData() const
{
	return sMetaData;
}

inline void KWAttributeSpec::SetMetaData(const ALString& sValue)
{
	sMetaData = sValue;
}

inline const ALString& KWAttributeSpec::GetLabel() const
{
	return sLabel;
}

inline void KWAttributeSpec::SetLabel(const ALString& sValue)
{
	sLabel = sValue;
}

// ## Custom inlines

// ##
