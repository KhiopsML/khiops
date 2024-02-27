// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#define UIDEV
#include "UserInterface.h"

UIData::UIData()
{
	bEditable = true;
}

const ALString UIData::GetClassLabel() const
{
	return "UI Data";
}

void UIData::SetEditable(boolean bValue)
{
	bEditable = bValue;
}

boolean UIData::GetEditable() const
{
	return bEditable;
}

boolean UIData::IsElement() const
{
	return false;
}

const ALString UIData::GetActualStyle() const
{
	return GetStyle();
}
