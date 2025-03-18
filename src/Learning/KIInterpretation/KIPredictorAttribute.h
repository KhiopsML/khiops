// Copyright (c) 2023-2025 Orange. All rights reserved.
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
// Classe KIPredictorAttribute
//    Predictor variable
class KIPredictorAttribute : public Object
{
public:
	// Constructeur
	KIPredictorAttribute();
	~KIPredictorAttribute();

	// Copie et duplication
	void CopyFrom(const KIPredictorAttribute* aSource);
	KIPredictorAttribute* Clone() const;

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

	// Importance
	double GetImportance() const;
	void SetImportance(double dValue);

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
	double dImportance;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline boolean KIPredictorAttribute::GetUsed() const
{
	return bUsed;
}

inline void KIPredictorAttribute::SetUsed(boolean bValue)
{
	bUsed = bValue;
}

inline const ALString& KIPredictorAttribute::GetType() const
{
	return sType;
}

inline void KIPredictorAttribute::SetType(const ALString& sValue)
{
	sType = sValue;
}

inline const ALString& KIPredictorAttribute::GetName() const
{
	return sName;
}

inline void KIPredictorAttribute::SetName(const ALString& sValue)
{
	sName = sValue;
}

inline double KIPredictorAttribute::GetImportance() const
{
	return dImportance;
}

inline void KIPredictorAttribute::SetImportance(double dValue)
{
	dImportance = dValue;
}

// ## Custom inlines

// ##
