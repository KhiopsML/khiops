// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "PRAddress.h"

PRAddress::PRAddress()
{
	// ## Custom constructor

	// ##
}

PRAddress::~PRAddress()
{
	// ## Custom destructor

	// ##
}

void PRAddress::CopyFrom(const PRAddress* aSource)
{
	require(aSource != NULL);

	sZipCode = aSource->sZipCode;
	sCity = aSource->sCity;

	// ## Custom copyfrom

	// ##
}

PRAddress* PRAddress::Clone() const
{
	PRAddress* aClone;

	aClone = new PRAddress;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void PRAddress::Write(ostream& ost) const
{
	ost << "Code postal\t" << GetZipCode() << "\n";
	ost << "Ville\t" << GetCity() << "\n";
}

const ALString PRAddress::GetClassLabel() const
{
	return "Adresse";
}

// ## Method implementation

const ALString PRAddress::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

// ##