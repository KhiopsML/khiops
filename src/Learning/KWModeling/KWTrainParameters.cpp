// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWTrainParameters.h"

KWTrainParameters::KWTrainParameters()
{
	nMaxEvaluatedAttributeNumber = 0;

	// ## Custom constructor

	sClassifierCriterion = "None";

	// ##
}

KWTrainParameters::~KWTrainParameters()
{
	// ## Custom destructor

	// ##
}

void KWTrainParameters::CopyFrom(const KWTrainParameters* aSource)
{
	require(aSource != NULL);

	nMaxEvaluatedAttributeNumber = aSource->nMaxEvaluatedAttributeNumber;
	sClassifierCriterion = aSource->sClassifierCriterion;

	// ## Custom copyfrom

	// ##
}

KWTrainParameters* KWTrainParameters::Clone() const
{
	KWTrainParameters* aClone;

	aClone = new KWTrainParameters;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void KWTrainParameters::Write(ostream& ost) const
{
	ost << "Max number of evaluated variables\t" << GetMaxEvaluatedAttributeNumber() << "\n";
	ost << "Classification criterion\t" << GetClassifierCriterion() << "\n";
}

const ALString KWTrainParameters::GetClassLabel() const
{
	return "Train parameters";
}

// ## Method implementation

const ALString KWTrainParameters::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

boolean KWTrainParameters::CheckMaxEvaluatedAttributeNumber(int nValue) const
{
	boolean bOk;
	bOk = nValue >= 0;
	if (not bOk)
		AddError("Max evaluated variable number must positive");
	return bOk;
}

boolean KWTrainParameters::CheckClassifierCriterion(const ALString& sValue) const
{
	boolean bOk;
	bOk = sValue == "None" or sValue == "Accuracy" or sValue == "BalancedAccuracy";
	if (not bOk)
		AddError("Incorrect classifier criterion (" + sValue + ")");
	return bOk;
}

boolean KWTrainParameters::Check() const
{
	return CheckMaxEvaluatedAttributeNumber(GetMaxEvaluatedAttributeNumber()) and
	       CheckClassifierCriterion(GetClassifierCriterion());
}

// ##
