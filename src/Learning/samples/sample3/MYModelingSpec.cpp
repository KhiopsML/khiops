// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "MYModelingSpec.h"

MYModelingSpec::MYModelingSpec()
{
	bSelectiveNaiveBayesPredictor = false;
	bNaiveBayesPredictor = false;

	// ## Custom constructor

	// Valeurs par defaut standard
	bSelectiveNaiveBayesPredictor = true;
	bNaiveBayesPredictor = false;

	// ##
}

MYModelingSpec::~MYModelingSpec()
{
	// ## Custom destructor

	// ##
}

void MYModelingSpec::CopyFrom(const MYModelingSpec* aSource)
{
	require(aSource != NULL);

	KWModelingSpec::CopyFrom(aSource);

	bSelectiveNaiveBayesPredictor = aSource->bSelectiveNaiveBayesPredictor;
	bNaiveBayesPredictor = aSource->bNaiveBayesPredictor;

	// ## Custom copyfrom

	// ##
}

MYModelingSpec* MYModelingSpec::Clone() const
{
	MYModelingSpec* aClone;

	aClone = new MYModelingSpec;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void MYModelingSpec::Write(ostream& ost) const
{
	KWModelingSpec::Write(ost);
	ost << "Selective Naive Bayes predictor\t" << BooleanToString(GetSelectiveNaiveBayesPredictor()) << "\n";
	ost << "Naive Bayes predictor\t" << BooleanToString(GetNaiveBayesPredictor()) << "\n";
}

const ALString MYModelingSpec::GetClassLabel() const
{
	return "Modeling parameters";
}

// ## Method implementation

const ALString MYModelingSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

// ##
