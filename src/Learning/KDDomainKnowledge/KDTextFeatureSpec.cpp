// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KDTextFeatureSpec.h"

KDTextFeatureSpec::KDTextFeatureSpec()
{
	// ## Custom constructor

	sTextFeatures = "words";

	// ##
}

KDTextFeatureSpec::~KDTextFeatureSpec()
{
	// ## Custom destructor

	// ##
}

void KDTextFeatureSpec::CopyFrom(const KDTextFeatureSpec* aSource)
{
	require(aSource != NULL);

	sTextFeatures = aSource->sTextFeatures;

	// ## Custom copyfrom

	// ##
}

KDTextFeatureSpec* KDTextFeatureSpec::Clone() const
{
	KDTextFeatureSpec* aClone;

	aClone = new KDTextFeatureSpec;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void KDTextFeatureSpec::Write(ostream& ost) const
{
	ost << "Text features\t" << GetTextFeatures() << "\n";
}

const ALString KDTextFeatureSpec::GetClassLabel() const
{
	return "Text feature parameters";
}

// ## Method implementation

const ALString KDTextFeatureSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

boolean KDTextFeatureSpec::Check() const
{
	boolean bOk;

	bOk = GetTextFeatures() == "ngrams" or GetTextFeatures() == "words" or GetTextFeatures() == "tokens";
	if (not bOk)
		AddError("Invalid parameter " + GetTextFeatures() + " : must be ngrams, words or tokens");
	return bOk;
}

// ##
