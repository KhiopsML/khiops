// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "TableServices.h"

int GetTokenSeparatorCount(const char* sTokens, char cSeparator)
{
	int nResult = 0;
	const char* sString;

	require(sTokens != NULL);
	require(cSeparator != '\0');
	require(cSeparator != ' ');

	sString = sTokens;
	while (sString[0] != '\0')
	{
		if (sString[0] == cSeparator)
			nResult++;
		sString++;
	}
	return nResult;
}

char* NextToken(char*& sCurrentToken, char cSeparator)
{
	char* sNextToken;
	char* sEndCurrentToken;

	require(cSeparator != '\0');
	require(cSeparator != ' ');

	// Supression des blancs au debut
	while (sCurrentToken[0] == ' ')
		sCurrentToken++;

	// Recherche du premier separateur dans la chaine
	sNextToken = sCurrentToken;
	while (sNextToken[0] != '\0' and sNextToken[0] != cSeparator)
		sNextToken++;

	// Suppression des blancs a la fin
	sEndCurrentToken = sNextToken;
	while (sEndCurrentToken > sCurrentToken)
	{
		sEndCurrentToken--;
		if (sEndCurrentToken[0] == ' ')
			sEndCurrentToken[0] = '\0';
		else
			break;
	}

	// Cas trouve
	if (sNextToken[0] == cSeparator)
	{
		sNextToken[0] = '\0';
		sNextToken++;
	}

	//

	return sNextToken;
}

boolean AreTokensEmpty(const char* sTokens, char cSeparator)
{
	const char* sBuffer;

	require(sTokens != NULL);
	require(cSeparator != '\0');
	require(cSeparator != ' ');

	sBuffer = sTokens;
	while (sBuffer[0] != '\0')
	{
		if (sBuffer[0] != ' ' and sBuffer[0] != cSeparator and sBuffer[0] != '\n')
			return false;
		sBuffer++;
	}
	return true;
}

char* PreprocessReal(char* sToken)
{
	char* sString;

	require(sToken != NULL);

	sString = sToken;
	while (sString[0] != '\0')
	{
		if (sString[0] == ',')
			sString[0] = '.';
		sString++;
	}
	return sToken;
}
