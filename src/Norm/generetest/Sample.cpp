// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Sample.h"

Sample::Sample()
{
	nSecond = 0;
	nInteger = 0;
	nTransient1 = 0;
	nTransient2 = 0;
	dDouble = 0;
	cChar = ' ';
	bBoolean = false;
	nTransient3 = 0;
	nDerived1 = 0;

	// ## Custom constructor

	nSecond = 42;

	// ##
}

Sample::~Sample()
{
	// ## Custom destructor

	// ##
}

void Sample::CopyFrom(const Sample* aSource)
{
	require(aSource != NULL);

	sFirst = aSource->sFirst;
	nSecond = aSource->nSecond;
	sString = aSource->sString;
	nInteger = aSource->nInteger;
	nTransient1 = aSource->nTransient1;
	nTransient2 = aSource->nTransient2;
	dDouble = aSource->dDouble;
	cChar = aSource->cChar;
	bBoolean = aSource->bBoolean;
	nTransient3 = aSource->nTransient3;
	nDerived1 = aSource->nDerived1;
	sDerived2 = aSource->sDerived2;

	// ## Custom copyfrom

	// ##
}

Sample* Sample::Clone() const
{
	Sample* aClone;

	aClone = new Sample;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void Sample::Write(ostream& ost) const
{
	ost << "Key first field\t" << GetFirst() << "\n";
	ost << "Key second field\t" << GetSecond() << "\n";
	ost << "String\t" << GetString() << "\n";
	ost << "Integer\t" << GetInteger() << "\n";
	ost << "Transient 1\t" << GetTransient1() << "\n";
	ost << "Double\t" << GetDouble() << "\n";
	ost << "Boolean\t" << BooleanToString(GetBoolean()) << "\n";
	ost << "Derived 1\t" << GetDerived1() << "\n";
	ost << "Derived 2\t" << GetDerived2() << "\n";
}

const ALString Sample::GetClassLabel() const
{
	return "Sample";
}

// ## Method implementation

const ALString Sample::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

// ##
