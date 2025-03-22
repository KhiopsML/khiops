// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SystemResource.h"
#include "Standard.h"

int main(int argc, char** argv)
{
	if (argc != 1)
	{
		cout << "This program is used by Khiops. Displays the number of physical cores that are available."
		     << endl;
		return EXIT_FAILURE;
	}

	// Display proc number on stdout
	cout << SystemGetProcessorNumber() << endl;
	return EXIT_SUCCESS;
}
