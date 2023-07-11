// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "CCAnalysisSpec.h"

CCAnalysisSpec::CCAnalysisSpec()
{
	// ## Custom constructor

	// ##
}

CCAnalysisSpec::~CCAnalysisSpec()
{
	// ## Custom destructor

	// ##
}

void CCAnalysisSpec::CopyFrom(const CCAnalysisSpec* aSource)
{
	require(aSource != NULL);

	// ## Custom copyfrom

	// ##
}

CCAnalysisSpec* CCAnalysisSpec::Clone() const
{
	CCAnalysisSpec* aClone;

	aClone = new CCAnalysisSpec;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void CCAnalysisSpec::Write(ostream& ost) const {}

const ALString CCAnalysisSpec::GetClassLabel() const
{
	return "Parameters";
}

// ## Method implementation

const ALString CCAnalysisSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

CCCoclusteringSpec* CCAnalysisSpec::GetCoclusteringSpec()
{
	return &coclusteringSpec;
}

// ##
