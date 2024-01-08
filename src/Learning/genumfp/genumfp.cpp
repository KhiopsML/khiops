// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "genumfp.h"

int main(int argc, char** argv)
{
	boolean bResult;
	MHCommandLine commandLine;

	// MemSetAllocIndexExit(519);

	bResult = commandLine.ComputeHistogram(argc, argv);
	if (bResult)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}
