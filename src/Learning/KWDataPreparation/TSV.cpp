// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "TSV.h"

ALString TSV::Export(const ALString& sValue)
{
	int nLength;
	ALString sResult;
	char c;
	int i;

	// Ajout de doubles quotes autour de la valeur si elle contient des double-quotes ou des tabulations
	if (sValue.Find('"') >= 0 or sValue.Find('\t') >= 0)
	{
		sResult = '"';
		nLength = sValue.GetLength();
		for (i = 0; i < nLength; i++)
		{
			c = sValue.GetAt(i);
			if (c == '"')
				sResult += '"';
			sResult += c;
		}
		sResult += '"';
	}
	else
		sResult = sValue;
	return sResult;
}
