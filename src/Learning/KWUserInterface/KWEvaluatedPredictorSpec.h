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
// Classe KWEvaluatedPredictorSpec
//    Evaluated predictor
class KWEvaluatedPredictorSpec : public Object
{
public:
	// Constructeur
	KWEvaluatedPredictorSpec();
	~KWEvaluatedPredictorSpec();

	// Copie et duplication
	void CopyFrom(const KWEvaluatedPredictorSpec* aSource);
	KWEvaluatedPredictorSpec* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Evaluated
	boolean GetEvaluated() const;
	void SetEvaluated(boolean bValue);

	// Predictor
	const ALString& GetPredictorType() const;
	void SetPredictorType(const ALString& sValue);

	// Name
	const ALString& GetPredictorName() const;
	void SetPredictorName(const ALString& sValue);

	// Dictionary
	const ALString& GetClassName() const;
	void SetClassName(const ALString& sValue);

	// Target variable
	const ALString& GetTargetAttributeName() const;
	void SetTargetAttributeName(const ALString& sValue);

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
	boolean bEvaluated;
	ALString sPredictorType;
	ALString sPredictorName;
	ALString sClassName;
	ALString sTargetAttributeName;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline boolean KWEvaluatedPredictorSpec::GetEvaluated() const
{
	return bEvaluated;
}

inline void KWEvaluatedPredictorSpec::SetEvaluated(boolean bValue)
{
	bEvaluated = bValue;
}

inline const ALString& KWEvaluatedPredictorSpec::GetPredictorType() const
{
	return sPredictorType;
}

inline void KWEvaluatedPredictorSpec::SetPredictorType(const ALString& sValue)
{
	sPredictorType = sValue;
}

inline const ALString& KWEvaluatedPredictorSpec::GetPredictorName() const
{
	return sPredictorName;
}

inline void KWEvaluatedPredictorSpec::SetPredictorName(const ALString& sValue)
{
	sPredictorName = sValue;
}

inline const ALString& KWEvaluatedPredictorSpec::GetClassName() const
{
	return sClassName;
}

inline void KWEvaluatedPredictorSpec::SetClassName(const ALString& sValue)
{
	sClassName = sValue;
}

inline const ALString& KWEvaluatedPredictorSpec::GetTargetAttributeName() const
{
	return sTargetAttributeName;
}

inline void KWEvaluatedPredictorSpec::SetTargetAttributeName(const ALString& sValue)
{
	sTargetAttributeName = sValue;
}

// ## Custom inlines

// ##
