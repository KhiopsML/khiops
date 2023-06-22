// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2021-04-06 18:11:58
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "KWAnalysisSpec.h"

KWAnalysisSpec::KWAnalysisSpec()
{
	// ## Custom constructor

	// Creation explicite des sous-objets, ce qui permet de creer des sous-objets specifiques dans des sous-classes
	modelingSpec = new KWModelingSpec;
	recoderSpec = new KWRecoderSpec;
	preprocessingSpec = new KWPreprocessingSpec;

	// ##
}

KWAnalysisSpec::~KWAnalysisSpec()
{
	// ## Custom destructor

	delete modelingSpec;
	delete recoderSpec;
	delete preprocessingSpec;

	// ##
}

void KWAnalysisSpec::CopyFrom(const KWAnalysisSpec* aSource)
{
	require(aSource != NULL);

	sTargetAttributeName = aSource->sTargetAttributeName;
	sMainTargetModality = aSource->sMainTargetModality;

	// ## Custom copyfrom

	// ##
}

KWAnalysisSpec* KWAnalysisSpec::Clone() const
{
	KWAnalysisSpec* aClone;

	aClone = new KWAnalysisSpec;
	aClone->CopyFrom(this);

	// ## Custom clone

	// Pas de clone des sous-objets
	assert(false);

	// ##
	return aClone;
}

void KWAnalysisSpec::Write(ostream& ost) const
{
	ost << "Target variable\t" << GetTargetAttributeName() << "\n";
	ost << "Main target value\t" << GetMainTargetModality() << "\n";
}

const ALString KWAnalysisSpec::GetClassLabel() const
{
	return "Parameters";
}

// ## Method implementation

const ALString KWAnalysisSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

KWModelingSpec* KWAnalysisSpec::GetModelingSpec()
{
	return modelingSpec;
}

KWRecoderSpec* KWAnalysisSpec::GetRecoderSpec()
{
	return recoderSpec;
}

KWPreprocessingSpec* KWAnalysisSpec::GetPreprocessingSpec()
{
	return preprocessingSpec;
}

// ##
