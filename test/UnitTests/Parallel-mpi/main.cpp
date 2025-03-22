// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "gtest/gtest.h"
#include "MpiEnvironment.h"

int main(int argc, char** argv)
{
	// Filter out Google Test arguments
	::testing::InitGoogleTest(&argc, argv);

	::testing::AddGlobalTestEnvironment(new MpiEnvironment);

	int result = RUN_ALL_TESTS();
	return result;
}
