// Copyright (c) 2024 Orange. All rights reserved.
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
	KWDRRegisterTextRules();
	KWDRRegisterTextListRules();
	KWDRRegisterCompareRules();
	KWDRRegisterLogicalRules();
	KWDRRegisterDateTimeRules();
	KWDRRegisterVectorRules();
	KWDRRegisterHashMapRules();
	KWDRRegisterMultiTableRules();
	KWDRRegisterTableCreationRules();
	KWDRRegisterTextualAnalysisRules();
}
