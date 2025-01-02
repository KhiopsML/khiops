// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UITestSubObject.h"

UITestSubObject::UITestSubObject()
{
	// ## Custom constructor

	// ##
}

UITestSubObject::~UITestSubObject()
{
	// ## Custom destructor

	// ##
}

void UITestSubObject::CopyFrom(const UITestSubObject* aSource)
{
	require(aSource != NULL);

	sInfo = aSource->sInfo;

	// ## Custom copyfrom

	// ##
}

UITestSubObject* UITestSubObject::Clone() const
{
	UITestSubObject* aClone;

	aClone = new UITestSubObject;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void UITestSubObject::Write(ostream& ost) const
{
	ost << "Info\t" << GetInfo() << "\n";
}

const ALString UITestSubObject::GetClassLabel() const
{
	return "Test sub-object";
}

// ## Method implementation

const ALString UITestSubObject::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

// ##
