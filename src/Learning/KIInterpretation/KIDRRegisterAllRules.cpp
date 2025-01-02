// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KIDRRegisterAllRules.h"

void KIDRRegisterAllRules()
{
	KWDerivationRule::RegisterDerivationRule(new KIDRClassifierContribution);
	KWDerivationRule::RegisterDerivationRule(new KIDRContributionNameAt);
	KWDerivationRule::RegisterDerivationRule(new KIDRContributionValueAt);
	KWDerivationRule::RegisterDerivationRule(new KIDRContributionPartitionAt);
	KWDerivationRule::RegisterDerivationRule(new KIDRContributionPriorClass);
	KWDerivationRule::RegisterDerivationRule(new KIDRContributionClass);
	KWDerivationRule::RegisterDerivationRule(new KIDRClassifierReinforcement);
	KWDerivationRule::RegisterDerivationRule(new KIDRReinforcementNameAt);
	KWDerivationRule::RegisterDerivationRule(new KIDRReinforcementInitialScore);
	KWDerivationRule::RegisterDerivationRule(new KIDRReinforcementFinalScoreAt);
	KWDerivationRule::RegisterDerivationRule(new KIDRReinforcementPartitionAt);
	KWDerivationRule::RegisterDerivationRule(new KIDRReinforcementClassChangeTagAt);
}
