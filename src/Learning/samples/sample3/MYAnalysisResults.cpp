// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "MYAnalysisResults.h"

MYAnalysisResults::MYAnalysisResults()
{
	bComputeBasicSecondaryStats = false;

	// ## Custom constructor

	bComputeBasicSecondaryStats = true;

	// ##
}

MYAnalysisResults::~MYAnalysisResults()
{
	// ## Custom destructor

	// ##
}

void MYAnalysisResults::CopyFrom(const MYAnalysisResults* aSource)
{
	require(aSource != NULL);

	KWAnalysisResults::CopyFrom(aSource);

	bComputeBasicSecondaryStats = aSource->bComputeBasicSecondaryStats;

	// ## Custom copyfrom

	// ##
}

MYAnalysisResults* MYAnalysisResults::Clone() const
{
	MYAnalysisResults* aClone;

	aClone = new MYAnalysisResults;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void MYAnalysisResults::Write(ostream& ost) const
{
	KWAnalysisResults::Write(ost);
	ost << "Compute basic secondary stats\t" << BooleanToString(GetComputeBasicSecondaryStats()) << "\n";
}

const ALString MYAnalysisResults::GetClassLabel() const
{
	return "Analysis results";
}

// ## Method implementation

const ALString MYAnalysisResults::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

// ##