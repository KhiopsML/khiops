// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "CCCoclusteringSpec.h"

CCCoclusteringSpec::CCCoclusteringSpec()
{
	// ## Custom constructor

	// ##
}

CCCoclusteringSpec::~CCCoclusteringSpec()
{
	// ## Custom destructor

	oaAttributeNames.DeleteAll();

	// ##
}

void CCCoclusteringSpec::CopyFrom(const CCCoclusteringSpec* aSource)
{
	require(aSource != NULL);

	sFrequencyAttributeName = aSource->sFrequencyAttributeName;

	// ## Custom copyfrom

	// ##
}

CCCoclusteringSpec* CCCoclusteringSpec::Clone() const
{
	CCCoclusteringSpec* aClone;

	aClone = new CCCoclusteringSpec;
	aClone->CopyFrom(this);

	// ## Custom clone

	// Pas de clone des sous-objets
	assert(false);

	// ##
	return aClone;
}

void CCCoclusteringSpec::Write(ostream& ost) const
{
	ost << "Frequency variable\t" << GetFrequencyAttributeName() << "\n";
}

const ALString CCCoclusteringSpec::GetClassLabel() const
{
	return "Coclustering parameters";
}

// ## Method implementation

const ALString CCCoclusteringSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

ObjectArray* CCCoclusteringSpec::GetAttributeNames()
{
	return &oaAttributeNames;
}

int CCCoclusteringSpec::GetMaxCoclusteringAttributeNumber()
{
	return 10;
}

// ##
