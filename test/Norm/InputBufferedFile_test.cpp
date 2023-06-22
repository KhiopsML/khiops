// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "InputBufferedFile.h"

#include "gtest/gtest.h"
namespace
{

TEST(InputBufferedFile, ReadWriteStd)
{
	boolean bOk;
	bOk = InputBufferedFile::Test(0);
	EXPECT_TRUE(bOk);
}

// TEST(InputBufferedFileRead, std_CacheSize)
// {
// 	EXPECT_TRUE(InputBufferedFile::TestReadWrite("cache size", 1 * lMB, 0));
// }
// TEST(InputBufferedFileRead, std_CacheSize_plus_1)
// {
// 	EXPECT_TRUE(InputBufferedFile::TestReadWrite("cache size + 1", 1 * lMB - 1, 0));
// }
// TEST(InputBufferedFileRead, std_CacheSize_minus_1)
// {
// 	EXPECT_TRUE(InputBufferedFile::TestReadWrite("cache size -1", 1 * lMB + 1, 0));
// }
// TEST(InputBufferedFileRead, std_8MB)
// {
// 	EXPECT_TRUE(InputBufferedFile::TestReadWrite("8_MB", 8 * lMB, 0));
// }
// TEST(InputBufferedFileRead, std_8MB_minus_1)
// {
// 	EXPECT_TRUE(InputBufferedFile::TestReadWrite("8_MB - 1", 8 * lMB - 1, 0));
// }
// TEST(InputBufferedFileRead, std_8MB_plus_1)
// {
// 	EXPECT_TRUE(InputBufferedFile::TestReadWrite("8_MB + 1", 8 * lMB + 1, 0));
// }

} // namespace