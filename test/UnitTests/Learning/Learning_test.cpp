// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "Standard.h"
#include "KWClass.h"
#include "KWClassDomain.h"
#include "KWProbabilityTable.h"
#include "KWQuantileBuilder.h"

#include "TestServices.h"

namespace
{
// Tests de learning, regroupes par librairie

// Librairie KWData
KHIOPS_TEST(KWData, KWClass, KWClass::Test);
KHIOPS_TEST(KWData, KWClassDomain, KWClassDomain::Test);

// Librairie KWDataPreparation
KHIOPS_TEST(KWDataPreparation, KWQuantileIntervalBuilder, KWQuantileIntervalBuilder::Test);
KHIOPS_TEST(KWDataPreparation, KWProbabilityTable, KWProbabilityTable::Test);

} // namespace
