// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWAttributePairName.h"

KWAttributePairName::KWAttributePairName() {}

KWAttributePairName::~KWAttributePairName() {}

void KWAttributePairName::CopyFrom(const KWAttributePairName* aSource)
{
	require(aSource != NULL);

	sFirstName = aSource->sFirstName;
	sSecondName = aSource->sSecondName;
}

KWAttributePairName* KWAttributePairName::Clone() const
{
	KWAttributePairName* aClone;

	aClone = new KWAttributePairName;
	aClone->CopyFrom(this);
	return aClone;
}

const ALString& KWAttributePairName::GetFirstSortedName() const
{
	if (sFirstName <= sSecondName)
		return sFirstName;
	else
		return sSecondName;
}

const ALString& KWAttributePairName::GetSecondSortedName() const
{
	if (sFirstName <= sSecondName)
		return sSecondName;
	else
		return sFirstName;
}

void KWAttributePairName::Write(ostream& ost) const
{
	ost << "First name\t" << GetFirstName() << "\n";
	ost << "Second name\t" << GetSecondName() << "\n";
}

const ALString KWAttributePairName::GetClassLabel() const
{
	return "Variable pair";
}

const ALString KWAttributePairName::GetObjectLabel() const
{
	ALString sLabel;

	sLabel = "(";
	sLabel += sFirstName;
	sLabel += ", ";
	sLabel += sSecondName;
	sLabel += ")";
	return sLabel;
}

int KWAttributePairNameCompare(const void* elem1, const void* elem2)
{
	KWAttributePairName* attributePairName1;
	KWAttributePairName* attributePairName2;
	int nDiff;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux attributs
	attributePairName1 = cast(KWAttributePairName*, *(Object**)elem1);
	attributePairName2 = cast(KWAttributePairName*, *(Object**)elem2);

	// Difference
	nDiff = attributePairName1->GetFirstSortedName().Compare(attributePairName2->GetFirstSortedName());
	if (nDiff == 0)
		nDiff = attributePairName1->GetSecondSortedName().Compare(attributePairName2->GetSecondSortedName());
	return nDiff;
}
