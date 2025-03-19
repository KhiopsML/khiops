// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KIModelInterpreter.h"

KIModelInterpreter::KIModelInterpreter()
{
	nContributionAttributeNumber = 0;

	// ## Custom constructor

	nContributionAttributeNumber = nDefaultContributionAttributeNumber;

	// ##
}

KIModelInterpreter::~KIModelInterpreter()
{
	// ## Custom destructor

	// ##
}

void KIModelInterpreter::CopyFrom(const KIModelInterpreter* aSource)
{
	require(aSource != NULL);

	KIModelService::CopyFrom(aSource);

	sShapleyValueRanking = aSource->sShapleyValueRanking;
	nContributionAttributeNumber = aSource->nContributionAttributeNumber;

	// ## Custom copyfrom

	// ##
}

KIModelInterpreter* KIModelInterpreter::Clone() const
{
	KIModelInterpreter* aClone;

	aClone = new KIModelInterpreter;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void KIModelInterpreter::Write(ostream& ost) const
{
	KIModelService::Write(ost);
	ost << "Shapley values ranking\t" << GetShapleyValueRanking() << "\n";
	ost << "Number of contribution variables\t" << GetContributionAttributeNumber() << "\n";
}

const ALString KIModelInterpreter::GetClassLabel() const
{
	return "Interpret model";
}

// ## Method implementation

const ALString KIModelInterpreter::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

// ##
