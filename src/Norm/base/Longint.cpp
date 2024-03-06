// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Longint.h"
#include "Standard.h"

const char* const LongintToHumanReadableString(longint lValue)
{
	const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
	int i;
	char* sBuffer = StandardGetBuffer();
	longint lTmp;

	require(lValue >= 0);

	// Recherche de l'exposant
	i = 0;
	lTmp = lValue;
	while (lTmp >= 1024)
	{
		lTmp /= 1024;
		i++;
	}

	// Affichage exact si des bytes plus petit que 1 KB
	if (i == 0)
		snprintf(sBuffer, BUFFER_LENGTH, "%lld %s", lValue, units[i]);
	// Sinon, affichage avec 1 digit de precision
	else
		snprintf(sBuffer, BUFFER_LENGTH, "%.1f %s", lValue / pow(1024.0, i), units[i]);
	return sBuffer;
}

const char* const LongintToReadableString(longint lValue)
{
	longint lAbsoluteValue;
	longint lTmp;
	longint lTmp2;
	longint lScale;
	char* sBuffer = StandardGetBuffer();
	int nPos;

	// Valeur absolue du nombre
	lAbsoluteValue = abs(lValue);

	// Cas des nombre inferieur a 10000
	if (lAbsoluteValue < 10000)
		snprintf(sBuffer, BUFFER_LENGTH, "%lld", lValue);
	// Sinon, on insere des separateurs de millier
	else
	{
		// Recherche de l'exposant
		lTmp = lAbsoluteValue;
		lTmp2 = 0;
		lScale = 1;
		while (lTmp >= 1000)
		{
			lTmp2 = lTmp2 + lScale * (lTmp % 1000);
			lTmp /= 1000;
			lScale *= 1000;
		}

		// Ecriture
		nPos = 0;
		if (lValue < 0)
			nPos += snprintf(&sBuffer[nPos], BUFFER_LENGTH - nPos, "-");
		nPos += snprintf(&sBuffer[nPos], BUFFER_LENGTH - nPos, "%d", (int)lTmp);
		while (lScale > 1)
		{
			lScale /= 1000;
			lTmp = lTmp2 / lScale;
			lTmp2 = lTmp2 % lScale;
			nPos += snprintf(&sBuffer[nPos], BUFFER_LENGTH - nPos, ",%03d", (int)lTmp);
		}
	}
	return sBuffer;
}
