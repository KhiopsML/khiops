// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "CCPostProcessedAttribute.h"

CCPostProcessedAttribute::CCPostProcessedAttribute()
{
	nPartNumber = 0;
	nMaxPartNumber = 0;

	// ## Custom constructor

	// ##
}

CCPostProcessedAttribute::~CCPostProcessedAttribute()
{
	// ## Custom destructor

	// ##
}

void CCPostProcessedAttribute::CopyFrom(const CCPostProcessedAttribute* aSource)
{
	require(aSource != NULL);

	sType = aSource->sType;
	sName = aSource->sName;
	nPartNumber = aSource->nPartNumber;
	nMaxPartNumber = aSource->nMaxPartNumber;

	// ## Custom copyfrom

	// ##
}

CCPostProcessedAttribute* CCPostProcessedAttribute::Clone() const
{
	CCPostProcessedAttribute* aClone;

	aClone = new CCPostProcessedAttribute;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void CCPostProcessedAttribute::Write(ostream& ost) const
{
	ost << "Type\t" << GetType() << "\n";
	ost << "Name\t" << GetName() << "\n";
	ost << "Part number\t" << GetPartNumber() << "\n";
	ost << "Max part number\t" << GetMaxPartNumber() << "\n";
}

const ALString CCPostProcessedAttribute::GetClassLabel() const
{
	return "Coclustering variable";
}

// ## Method implementation

const ALString CCPostProcessedAttribute::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

// ##
