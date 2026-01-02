// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "UITestActionSubObject.h"

UITestActionSubObject::UITestActionSubObject()
{
	bDirectFileChooser = false;

	// ## Custom constructor

	bDirectFileChooser = true;

	// ##
}

UITestActionSubObject::~UITestActionSubObject()
{
	// ## Custom destructor

	// ##
}

void UITestActionSubObject::CopyFrom(const UITestActionSubObject* aSource)
{
	require(aSource != NULL);

	sFilePath = aSource->sFilePath;
	bDirectFileChooser = aSource->bDirectFileChooser;

	// ## Custom copyfrom

	// ##
}

UITestActionSubObject* UITestActionSubObject::Clone() const
{
	UITestActionSubObject* aClone;

	aClone = new UITestActionSubObject;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void UITestActionSubObject::Write(ostream& ost) const
{
	ost << "File\t" << GetFilePath() << "\n";
	ost << "Direct file chooser\t" << BooleanToString(GetDirectFileChooser()) << "\n";
}

const ALString UITestActionSubObject::GetClassLabel() const
{
	return "Test sub-object with action";
}

// ## Method implementation

const ALString UITestActionSubObject::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

// ##
