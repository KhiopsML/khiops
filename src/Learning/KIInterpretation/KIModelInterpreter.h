// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"
#include "KIModelService.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe KIModelInterpreter
//    Interpret model
class KIModelInterpreter : public KIModelService
{
public:
	// Constructeur
	KIModelInterpreter();
	~KIModelInterpreter();

	// Copie et duplication
	void CopyFrom(const KIModelInterpreter* aSource);
	KIModelInterpreter* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Shapley values
	const ALString& GetShapleyValues() const;
	void SetShapleyValues(const ALString& sValue);

	// Number of contribution variables
	int GetContributionAttributeNumber() const;
	void SetContributionAttributeNumber(int nValue);

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
	ALString sShapleyValues;
	int nContributionAttributeNumber;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& KIModelInterpreter::GetShapleyValues() const
{
	return sShapleyValues;
}

inline void KIModelInterpreter::SetShapleyValues(const ALString& sValue)
{
	sShapleyValues = sValue;
}

inline int KIModelInterpreter::GetContributionAttributeNumber() const
{
	return nContributionAttributeNumber;
}

inline void KIModelInterpreter::SetContributionAttributeNumber(int nValue)
{
	nContributionAttributeNumber = nValue;
}

// ## Custom inlines

// ##
