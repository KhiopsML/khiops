// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KMDRRegisterAllRules.h"
#include "KMDRClassifier.h"
#include "KMDRLocalModelChooser.h"

void KMDRRegisterAllRules()
{
	// regles de derivation Enneade :
	KWDerivationRule::RegisterDerivationRule(new KMDRClassifier);
	KWDerivationRule::RegisterDerivationRule(new KMDRLocalModelChooser);
}
