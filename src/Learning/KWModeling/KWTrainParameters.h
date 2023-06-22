// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// 2018-07-19 14:35:54
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe KWTrainParameters
//    Train parameters
class KWTrainParameters : public Object
{
public:
	// Constructeur
	KWTrainParameters();
	~KWTrainParameters();

	// Copie et duplication
	void CopyFrom(const KWTrainParameters* aSource);
	KWTrainParameters* Clone() const;

	////////////////////////////////////////////////////////
	// Acces aux attributs

	// Max number of evaluated variables
	int GetMaxEvaluatedAttributeNumber() const;
	void SetMaxEvaluatedAttributeNumber(int nValue);

	// Classification criterion
	const ALString& GetClassifierCriterion() const;
	void SetClassifierCriterion(const ALString& sValue);

	////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	//////////////////////////////////////////////////////////////////////////////
	// Description fine des parametres
	//
	// MaxEvaluatedAttributeNumber
	//  Seuls les premiers attributs (au sens de l'evaluation univariee) seront
	//  evalues pour apprendre le predicteur
	//  Par defaut: 0 (signifie pas de maximum)
	//
	// GetClassifierCriterion
	//  Par defaut: None
	//   Valeurs possibles: None, Accuracy, BalancedAccuracy

	// Verification des parametres
	boolean CheckMaxEvaluatedAttributeNumber(int nValue) const;
	boolean CheckClassifierCriterion(const ALString& sValue) const;

	// Verification globale
	boolean Check() const override;

	// ##

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attributs de la classe
	int nMaxEvaluatedAttributeNumber;
	ALString sClassifierCriterion;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline int KWTrainParameters::GetMaxEvaluatedAttributeNumber() const
{
	return nMaxEvaluatedAttributeNumber;
}

inline void KWTrainParameters::SetMaxEvaluatedAttributeNumber(int nValue)
{
	nMaxEvaluatedAttributeNumber = nValue;
}

inline const ALString& KWTrainParameters::GetClassifierCriterion() const
{
	return sClassifierCriterion;
}

inline void KWTrainParameters::SetClassifierCriterion(const ALString& sValue)
{
	sClassifierCriterion = sValue;
}

// ## Custom inlines

// ##
