// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "PRChild.h"

PRChild::PRChild()
{
	nAge = 0;

	// ## Custom constructor

	// ##
}

PRChild::~PRChild()
{
	// ## Custom destructor

	// ##
}

void PRChild::CopyFrom(const PRChild* aSource)
{
	require(aSource != NULL);

	sFirstName = aSource->sFirstName;
	nAge = aSource->nAge;

	// ## Custom copyfrom

	// ##
}

PRChild* PRChild::Clone() const
{
	PRChild* aClone;

	aClone = new PRChild;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void PRChild::Write(ostream& ost) const
{
	ost << "Prenom\t" << GetFirstName() << "\n";
	ost << "Age\t" << GetAge() << "\n";
}

const ALString PRChild::GetClassLabel() const
{
	return "Enfant";
}

// ## Method implementation

const ALString PRChild::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

int PRChildCompareAge(const void* elem1, const void* elem2)
{
	PRChild* child1;
	PRChild* child2;

	// Acces aux objet a comparer
	child1 = cast(PRChild*, *(Object**)elem1);
	child2 = cast(PRChild*, *(Object**)elem2);

	// Comparaison
	return child1->GetAge() - child2->GetAge();
}

// ##
