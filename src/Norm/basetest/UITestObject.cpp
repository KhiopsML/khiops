// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// 2021-01-31 18:14:10
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "UITestObject.h"

UITestObject::UITestObject()
{
	nDuration = 0;

	// ## Custom constructor

	nDuration = 2;

	// ##
}

UITestObject::~UITestObject()
{
	// ## Custom destructor

	// ##
}

void UITestObject::CopyFrom(const UITestObject* aSource)
{
	require(aSource != NULL);

	nDuration = aSource->nDuration;
	sResult = aSource->sResult;

	// ## Custom copyfrom

	// ##
}

UITestObject* UITestObject::Clone() const
{
	UITestObject* aClone;

	aClone = new UITestObject;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void UITestObject::Write(ostream& ost) const
{
	ost << "Duration\t" << GetDuration() << "\n";
	ost << "Result\t" << GetResult() << "\n";
}

const ALString UITestObject::GetClassLabel() const
{
	return "Test object";
}

// ## Method implementation

const ALString UITestObject::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

UITestSubObject* UITestObject::GetSubObject()
{
	return &testSubObject;
}

UITestActionSubObject* UITestObject::GetActionSubObject()
{
	return &testActionSubObject;
}

// ##