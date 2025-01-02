// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "CCVarPartCoclusteringSpec.h"

CCVarPartCoclusteringSpec::CCVarPartCoclusteringSpec()
{
	// ## Custom constructor

	// ##
}

CCVarPartCoclusteringSpec::~CCVarPartCoclusteringSpec()
{
	// ## Custom destructor

	// ##
}

void CCVarPartCoclusteringSpec::CopyFrom(const CCVarPartCoclusteringSpec* aSource)
{
	require(aSource != NULL);

	sIdentifierAttributeName = aSource->sIdentifierAttributeName;

	// ## Custom copyfrom

	// ##
}

CCVarPartCoclusteringSpec* CCVarPartCoclusteringSpec::Clone() const
{
	CCVarPartCoclusteringSpec* aClone;

	aClone = new CCVarPartCoclusteringSpec;
	aClone->CopyFrom(this);

	// ## Custom clone

	// Pas de clone des sous-objets
	assert(false);

	// ##
	return aClone;
}

void CCVarPartCoclusteringSpec::Write(ostream& ost) const
{
	ost << "Identifier variable\t" << GetIdentifierAttributeName() << "\n";
}

const ALString CCVarPartCoclusteringSpec::GetClassLabel() const
{
	return "Instances Variables coclustering parameters";
}

// ## Method implementation

const ALString CCVarPartCoclusteringSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

// ##
