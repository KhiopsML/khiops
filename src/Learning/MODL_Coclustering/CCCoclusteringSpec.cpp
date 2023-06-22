// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2021-02-05 18:19:44
// File generated  with GenereTable
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

	oaAttributes.DeleteAll();

	// ##
}

void CCCoclusteringSpec::CopyFrom(const CCCoclusteringSpec* aSource)
{
	require(aSource != NULL);

	sFrequencyAttribute = aSource->sFrequencyAttribute;

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
	ost << "Frequency variable\t" << GetFrequencyAttribute() << "\n";
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

ObjectArray* CCCoclusteringSpec::GetAttributes()
{
	return &oaAttributes;
}

int CCCoclusteringSpec::GetMaxCoclusteringAttributeNumber()
{
	return 10;
}

// ##
