// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"
#include "KWModelingSpec.h"

// ## Custom includes

// ##

////////////////////////////////////////////////////////////
// Classe MYModelingSpec
//    Modeling parameters
class MYModelingSpec : public KWModelingSpec
{
public:
	// Constructeur
	MYModelingSpec();
	~MYModelingSpec();

	// Copie et duplication
	void CopyFrom(const MYModelingSpec* aSource);
	MYModelingSpec* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Selective Naive Bayes predictor
	boolean GetSelectiveNaiveBayesPredictor() const;
	void SetSelectiveNaiveBayesPredictor(boolean bValue);

	// Naive Bayes predictor
	boolean GetNaiveBayesPredictor() const;
	void SetNaiveBayesPredictor(boolean bValue);

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
	boolean bSelectiveNaiveBayesPredictor;
	boolean bNaiveBayesPredictor;

	// ## Custom implementation

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline boolean MYModelingSpec::GetSelectiveNaiveBayesPredictor() const
{
	return bSelectiveNaiveBayesPredictor;
}

inline void MYModelingSpec::SetSelectiveNaiveBayesPredictor(boolean bValue)
{
	bSelectiveNaiveBayesPredictor = bValue;
}

inline boolean MYModelingSpec::GetNaiveBayesPredictor() const
{
	return bNaiveBayesPredictor;
}

inline void MYModelingSpec::SetNaiveBayesPredictor(boolean bValue)
{
	bNaiveBayesPredictor = bValue;
}

// ## Custom inlines

// ##
