// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "TestServices.h"
#include "Timer.h"
#include "Standard.h"
#include "Ermgt.h"
#include "ALString.h"
#include "Object.h"
#include "SortedList.h"
#include "SystemResource.h"
#include "CharVector.h"
#include "InputBufferedFile.h"
#include "OutputBufferedFile.h"
#include "Regexp.h"
#include "MemoryTest.h"
#include "TextService.h"
namespace
{

KHIOPS_TEST(base, SystemResource, TestSystemResource);
KHIOPS_TEST(base, Mem, TestMemory);
KHIOPS_TEST(base, Random, TestRandom);
KHIOPS_TEST(base, Error, Error::Test);
KHIOPS_TEST(base, FileService, FileService::Test);
KHIOPS_TEST(base, ALString, ALString::Test);
KHIOPS_TEST(base, Regex, Regex::Test);
KHIOPS_TEST(base, Timer, Timer::Test);
KHIOPS_TEST(base, ObjectArray, ObjectArray::Test);
KHIOPS_TEST(base, ObjectList, ObjectList::Test);
KHIOPS_TEST(base, SortedList, SortedList::Test);
KHIOPS_TEST(base, ObjectDictionary, ObjectDictionary::Test);
KHIOPS_TEST(base, NumericKeyDictionary, NumericKeyDictionary::Test);
KHIOPS_TEST(base, LongintDictionary, LongintDictionary::Test);
KHIOPS_TEST(base, LongintNumericKeyDictionary, LongintNumericKeyDictionary::Test);
KHIOPS_TEST(base, DoubleVector, DoubleVector::Test);
KHIOPS_TEST(base, IntVector, IntVector::Test);
KHIOPS_TEST(base, LongintVector, LongintVector::Test);
KHIOPS_TEST(base, CharVector, CharVector::Test);
KHIOPS_TEST(base, StringVector, StringVector::Test);
KHIOPS_TEST(base, TextService, TextService::Test);

TEST(long, InputBufferedFile)
{
	EXPECT_TRUE(InputBufferedFile::Test(0));
}

// TODO a partir de quelle duree c'est un long test ?
// TODO modifier le parsing du token SYS pour qu'il soit pris en compte en milieu de ligne

} // namespace
