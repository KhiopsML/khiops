// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLSerializer.h"

#include "../Utils/TestServices.h"

namespace
{
TEST(PLSerializer, int)
{
	int nIn = 6;
	int nOut;
	PLSerializer serializer;
	serializer.OpenForWrite(NULL);
	serializer.PutInt(nIn);
	serializer.Close();

	serializer.OpenForRead(NULL);
	nOut = serializer.GetInt();
	serializer.Close();

	ASSERT_EQ(nIn, nOut);
}

KHIOPS_TEST(PLSerializer, full, PLSerializer::Test());

} // namespace