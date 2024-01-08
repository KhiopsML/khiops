// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#ifdef _MSC_VER
// To disable fopen warnings (Visual C++ deprecated method)
#define _CRT_SECURE_NO_WARNINGS
#endif // _MSC_VER

#include "SystemDivers.h"
// #include "Windows.h"
// #include <shellapi.h>
#include <stdlib.h>
#include <stdio.h>

int OpenPdfFile(char* sPdfFilePathName)
{
	/*
	HINSTANCE nRet;
	nRet = ShellExecuteA(GetDesktopWindow(),// NULL
	"Open",
	sPdfFilePathName,
	NULL,
	NULL,
	SW_SHOWNORMAL);
	return ((int)nRet>32);
	*/
	char sCommand[1000];
	sprintf(sCommand, "start %s", sPdfFilePathName);
	system(sCommand);
	return 0;
}
