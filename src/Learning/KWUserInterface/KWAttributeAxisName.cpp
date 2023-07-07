// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2019-02-25 11:58:40
// File generated  with GenereTable
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
	sAxisName = aSource->sAxisName;

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
	ost << "AttributeName\t" << GetAttributeName() << "\n";
	ost << "AxisName\t" << GetAxisName() << "\n";
}

const ALString KWAttributeAxisName::GetClassLabel() const
{
	return "VariableAxis";
}

// ## Method implementation

const ALString KWAttributeAxisName::GetObjectLabel() const
{
	ALString sLabel;

	sLabel = GetAttributeName() + GetAxisName();
	return sLabel;
}

// ##
