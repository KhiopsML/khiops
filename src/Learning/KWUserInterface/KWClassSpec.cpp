// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWClassSpec.h"

KWClassSpec::KWClassSpec()
{
	bRoot = false;
	nAttributeNumber = 0;
	nSymbolAttributeNumber = 0;
	nContinuousAttributeNumber = 0;
	nDerivedAttributeNumber = 0;

	// ## Custom constructor

	// ##
}

KWClassSpec::~KWClassSpec()
{
	// ## Custom destructor

	// ##
}

void KWClassSpec::CopyFrom(const KWClassSpec* aSource)
{
	require(aSource != NULL);

	sClassName = aSource->sClassName;
	bRoot = aSource->bRoot;
	sKey = aSource->sKey;
	nAttributeNumber = aSource->nAttributeNumber;
	nSymbolAttributeNumber = aSource->nSymbolAttributeNumber;
	nContinuousAttributeNumber = aSource->nContinuousAttributeNumber;
	nDerivedAttributeNumber = aSource->nDerivedAttributeNumber;

	// ## Custom copyfrom

	// ##
}

KWClassSpec* KWClassSpec::Clone() const
{
	KWClassSpec* aClone;

	aClone = new KWClassSpec;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void KWClassSpec::Write(ostream& ost) const
{
	ost << "Name\t" << GetClassName() << "\n";
	ost << "Root\t" << BooleanToString(GetRoot()) << "\n";
	ost << "Key\t" << GetKey() << "\n";
	ost << "Variables\t" << GetAttributeNumber() << "\n";
	ost << "Categorical\t" << GetSymbolAttributeNumber() << "\n";
	ost << "Numerical\t" << GetContinuousAttributeNumber() << "\n";
	ost << "Derived\t" << GetDerivedAttributeNumber() << "\n";
}

const ALString KWClassSpec::GetClassLabel() const
{
	return "Dictionary";
}

// ## Method implementation

const ALString KWClassSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

// ##