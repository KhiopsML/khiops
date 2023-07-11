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
// Classe CCPostProcessedAttribute
//    Coclustering variable
class CCPostProcessedAttribute : public Object
{
public:
	// Constructeur
	CCPostProcessedAttribute();
	~CCPostProcessedAttribute();

	// Copie et duplication
	void CopyFrom(const CCPostProcessedAttribute* aSource);
	CCPostProcessedAttribute* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Type
	const ALString& GetType() const;
	void SetType(const ALString& sValue);

	// Name
	const ALString& GetName() const;
	void SetName(const ALString& sValue);

	// Part number
	int GetPartNumber() const;
	void SetPartNumber(int nValue);

	// Max part number
	int GetMaxPartNumber() const;
	void SetMaxPartNumber(int nValue);

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
	ALString sType;
	ALString sName;
	int nPartNumber;
	int nMaxPartNumber;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& CCPostProcessedAttribute::GetType() const
{
	return sType;
}

inline void CCPostProcessedAttribute::SetType(const ALString& sValue)
{
	sType = sValue;
}

inline const ALString& CCPostProcessedAttribute::GetName() const
{
	return sName;
}

inline void CCPostProcessedAttribute::SetName(const ALString& sValue)
{
	sName = sValue;
}

inline int CCPostProcessedAttribute::GetPartNumber() const
{
	return nPartNumber;
}

inline void CCPostProcessedAttribute::SetPartNumber(int nValue)
{
	nPartNumber = nValue;
}

inline int CCPostProcessedAttribute::GetMaxPartNumber() const
{
	return nMaxPartNumber;
}

inline void CCPostProcessedAttribute::SetMaxPartNumber(int nValue)
{
	nMaxPartNumber = nValue;
}

// ## Custom inlines

// ##
