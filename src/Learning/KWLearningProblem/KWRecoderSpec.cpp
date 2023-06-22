// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2021-04-06 18:11:58
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "KWRecoderSpec.h"

KWRecoderSpec::KWRecoderSpec()
{
	bRecoder = false;

	// ## Custom constructor

	// ##
}

KWRecoderSpec::~KWRecoderSpec()
{
	// ## Custom destructor

	// ##
}

void KWRecoderSpec::CopyFrom(const KWRecoderSpec* aSource)
{
	require(aSource != NULL);

	bRecoder = aSource->bRecoder;

	// ## Custom copyfrom

	// ##
}

KWRecoderSpec* KWRecoderSpec::Clone() const
{
	KWRecoderSpec* aClone;

	aClone = new KWRecoderSpec;
	aClone->CopyFrom(this);

	// ## Custom clone

	aClone->recodingSpec.CopyFrom(&recodingSpec);

	// ##
	return aClone;
}

void KWRecoderSpec::Write(ostream& ost) const
{
	ost << "Build recoder\t" << BooleanToString(GetRecoder()) << "\n";
}

const ALString KWRecoderSpec::GetClassLabel() const
{
	return "Recoders";
}

// ## Method implementation

const ALString KWRecoderSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

KWRecodingSpec* KWRecoderSpec::GetRecodingSpec()
{
	return &recodingSpec;
}

// ##
