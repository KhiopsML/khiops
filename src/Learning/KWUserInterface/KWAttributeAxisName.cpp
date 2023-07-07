// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWAttributeAxisName.h"

KWAttributeAxisName::KWAttributeAxisName()
{
	// ## Custom constructor

	// ##
}

KWAttributeAxisName::~KWAttributeAxisName()
{
	// ## Custom destructor

	// ##
}

void KWAttributeAxisName::CopyFrom(const KWAttributeAxisName* aSource)
{
	require(aSource != NULL);

	sAttributeName = aSource->sAttributeName;
	sOwnerAttributeName = aSource->sOwnerAttributeName;

	// ## Custom copyfrom

	// ##
}

KWAttributeAxisName* KWAttributeAxisName::Clone() const
{
	KWAttributeAxisName* aClone;

	aClone = new KWAttributeAxisName;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void KWAttributeAxisName::Write(ostream& ost) const
{
	ost << "Attribute name\t" << GetAttributeName() << "\n";
	ost << "Owner attribute name\t" << GetOwnerAttributeName() << "\n";
}

const ALString KWAttributeAxisName::GetClassLabel() const
{
	return "Variable axis";
}

// ## Method implementation

const ALString KWAttributeAxisName::GetObjectLabel() const
{
	ALString sLabel;

	sLabel = GetAttributeName() + GetOwnerAttributeName();
	return sLabel;
}

// ##
