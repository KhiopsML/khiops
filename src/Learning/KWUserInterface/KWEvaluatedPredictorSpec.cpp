// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2021-04-25 11:10:57
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "KWEvaluatedPredictorSpec.h"

KWEvaluatedPredictorSpec::KWEvaluatedPredictorSpec()
{
	bEvaluated = false;

	// ## Custom constructor

	// ##
}

KWEvaluatedPredictorSpec::~KWEvaluatedPredictorSpec()
{
	// ## Custom destructor

	// ##
}

void KWEvaluatedPredictorSpec::CopyFrom(const KWEvaluatedPredictorSpec* aSource)
{
	require(aSource != NULL);

	bEvaluated = aSource->bEvaluated;
	sPredictorType = aSource->sPredictorType;
	sPredictorName = aSource->sPredictorName;
	sClassName = aSource->sClassName;
	sTargetAttributeName = aSource->sTargetAttributeName;

	// ## Custom copyfrom

	// ##
}

KWEvaluatedPredictorSpec* KWEvaluatedPredictorSpec::Clone() const
{
	KWEvaluatedPredictorSpec* aClone;

	aClone = new KWEvaluatedPredictorSpec;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void KWEvaluatedPredictorSpec::Write(ostream& ost) const
{
	ost << "Evaluated\t" << BooleanToString(GetEvaluated()) << "\n";
	ost << "Predictor\t" << GetPredictorType() << "\n";
	ost << "Name\t" << GetPredictorName() << "\n";
	ost << "Dictionary\t" << GetClassName() << "\n";
	ost << "Target variable\t" << GetTargetAttributeName() << "\n";
}

const ALString KWEvaluatedPredictorSpec::GetClassLabel() const
{
	return "Evaluated predictor";
}

// ## Method implementation

const ALString KWEvaluatedPredictorSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

// ##
