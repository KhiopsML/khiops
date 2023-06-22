// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDRAll.h"

void KWDRRegisterAllRules()
{
	KWDRRegisterReferenceRule();
	KWDRRegisterRandomRule();
	KWDRRegisterStandardRules();
	KWDRRegisterMathRules();
	KWDRRegisterStringRules();
	KWDRRegisterCompareRules();
	KWDRRegisterLogicalRules();
	KWDRRegisterDateTimeRules();
	KWDRRegisterVectorRules();
	KWDRRegisterHashMapRules();
	KWDRRegisterMultiTableRules();
	KWDerivationRule::RegisterDerivationRule(new KWDRTokenCounts);
	KWDerivationRule::RegisterDerivationRule(new KWDRTokenize);
	KWDerivationRule::RegisterDerivationRule(new KWDRCharNGramCounts);
}