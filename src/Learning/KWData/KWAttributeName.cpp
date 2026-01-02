// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWAttributeName.h"

KWAttributeName::KWAttributeName() {}

KWAttributeName::~KWAttributeName() {}

void KWAttributeName::CopyFrom(const KWAttributeName* aSource)
{
	require(aSource != NULL);

	sName = aSource->sName;
}

KWAttributeName* KWAttributeName::Clone() const
{
	KWAttributeName* aClone;

	aClone = new KWAttributeName;
	aClone->CopyFrom(this);
	return aClone;
}

void KWAttributeName::Write(ostream& ost) const
{
	ost << "Name\t" << GetName() << "\n";
}

const ALString KWAttributeName::GetClassLabel() const
{
	return "Variable";
}

const ALString KWAttributeName::GetObjectLabel() const
{
	return sName;
}
