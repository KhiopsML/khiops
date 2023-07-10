// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWAttributeSpec.h"

KWAttributeSpec::KWAttributeSpec()
{
	bUsed = false;
	bDerived = false;

	// ## Custom constructor

	// ##
}

KWAttributeSpec::~KWAttributeSpec()
{
	// ## Custom destructor

	// ##
}

void KWAttributeSpec::CopyFrom(const KWAttributeSpec* aSource)
{
	require(aSource != NULL);

	bUsed = aSource->bUsed;
	sType = aSource->sType;
	sName = aSource->sName;
	bDerived = aSource->bDerived;
	sMetaData = aSource->sMetaData;
	sLabel = aSource->sLabel;

	// ## Custom copyfrom

	// ##
}

KWAttributeSpec* KWAttributeSpec::Clone() const
{
	KWAttributeSpec* aClone;

	aClone = new KWAttributeSpec;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void KWAttributeSpec::Write(ostream& ost) const
{
	ost << "Used\t" << BooleanToString(GetUsed()) << "\n";
	ost << "Type\t" << GetType() << "\n";
	ost << "Name\t" << GetName() << "\n";
	ost << "Derived\t" << BooleanToString(GetDerived()) << "\n";
	ost << "Meta-data\t" << GetMetaData() << "\n";
	ost << "Label\t" << GetLabel() << "\n";
}

const ALString KWAttributeSpec::GetClassLabel() const
{
	return "Variable";
}

// ## Method implementation

const ALString KWAttributeSpec::GetObjectLabel() const
{
	ALString sObjectLabel;

	return sObjectLabel;
}

// ##
