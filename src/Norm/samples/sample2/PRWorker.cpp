// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "PRWorker.h"

PRWorker::PRWorker()
{
	// ## Custom constructor

	// ##
}

PRWorker::~PRWorker()
{
	// ## Custom destructor

	// ##
}

void PRWorker::CopyFrom(const PRWorker* aSource)
{
	require(aSource != NULL);

	sFirstName = aSource->sFirstName;
	sFamilyName = aSource->sFamilyName;

	// ## Custom copyfrom

	// ##
}

PRWorker* PRWorker::Clone() const
{
	PRWorker* aClone;

	aClone = new PRWorker;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void PRWorker::Write(ostream& ost) const
{
	ost << "Prenom\t" << GetFirstName() << "\n";
	ost << "Nom\t" << GetFamilyName() << "\n";
}

const ALString PRWorker::GetClassLabel() const
{
	return "Employe";
}

// ## Method implementation

const ALString PRWorker::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

// ##