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
// Classe KIModelService
//    Interpretation service
class KIModelService : public Object
{
public:
	// Constructeur
	KIModelService();
	~KIModelService();

	// Copie et duplication
	void CopyFrom(const KIModelService* aSource);
	KIModelService* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Predictor dictionary
	const ALString& GetPredictorClassName() const;
	void SetPredictorClassName(const ALString& sValue);

	// Number of predictor variables
	int GetPredictorAttributeNumber() const;
	void SetPredictorAttributeNumber(int nValue);

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
	ALString sPredictorClassName;
	int nPredictorAttributeNumber;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline const ALString& KIModelService::GetPredictorClassName() const
{
	return sPredictorClassName;
}

inline void KIModelService::SetPredictorClassName(const ALString& sValue)
{
	sPredictorClassName = sValue;
}

inline int KIModelService::GetPredictorAttributeNumber() const
{
	return nPredictorAttributeNumber;
}

inline void KIModelService::SetPredictorAttributeNumber(int nValue)
{
	nPredictorAttributeNumber = nValue;
}

// ## Custom inlines

// ##
