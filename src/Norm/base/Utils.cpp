// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Utils.h"

int GetMajorVersion(const ALString& sFullVersion)
{
	ALString sMajorVersion;
	int i;

	require(sFullVersion != "");
	require(isdigit(sFullVersion[0]));

	// Recherche du point de depart
	i = 0;
	while (i < sFullVersion.GetLength())
	{
		if (isdigit(sFullVersion.GetAt(i)))
			sMajorVersion += sFullVersion.GetAt(i);
		else
			break;
		i++;
	}
	assert(1 <= sMajorVersion.GetLength() and sMajorVersion.GetLength() <= 3);
	return StringToInt(sMajorVersion);
}

int GetMinorVersion(const ALString& sFullVersion)
{
	ALString sMinorVersion;
	int i;

	require(sFullVersion != "");
	require(isdigit(sFullVersion[0]));

	// Recherche du separateur .
	i = sFullVersion.Find('.');
	assert(i > 0);
	i++;
	while (i < sFullVersion.GetLength())
	{
		if (isdigit(sFullVersion.GetAt(i)))
			sMinorVersion += sFullVersion.GetAt(i);
		else
			break;
		i++;
	}
	assert(sMinorVersion.GetLength() <= 1);
	if (sMinorVersion == "")
		return 0;
	else
		return StringToInt(sMinorVersion);
}
