// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataItem.h"

void KWDataItem::WriteJSONFields(JSONFile* fJSON) const {}

void KWDataItem::WriteJSONReport(JSONFile* fJSON) const
{
	fJSON->BeginObject();
	WriteJSONFields(fJSON);
	fJSON->EndObject();
}

void KWDataItem::WriteJSONKeyReport(JSONFile* fJSON, const ALString& sKey) const
{
	fJSON->BeginKeyObject(sKey);
	WriteJSONFields(fJSON);
	fJSON->EndObject();
}
