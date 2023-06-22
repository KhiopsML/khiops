// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "ISDRRegisterAllRules.h"
#include "ISDRPredictor.h"

void ISDRRegisterAllRules()
{
	KWDerivationRule::RegisterDerivationRule(new ISDRClassifierContribution);
	KWDerivationRule::RegisterDerivationRule(new ISDRContributionNameAt);
	KWDerivationRule::RegisterDerivationRule(new ISDRContributionValueAt);
	KWDerivationRule::RegisterDerivationRule(new ISDRContributionPartitionAt);
	KWDerivationRule::RegisterDerivationRule(new ISDRContributionPriorClass);
	KWDerivationRule::RegisterDerivationRule(new ISDRContributionClass);
	KWDerivationRule::RegisterDerivationRule(new ISDRClassifierReinforcement);
	KWDerivationRule::RegisterDerivationRule(new ISDRReinforcementNameAt);
	KWDerivationRule::RegisterDerivationRule(new ISDRReinforcementInitialScore);
	KWDerivationRule::RegisterDerivationRule(new ISDRReinforcementFinalScoreAt);
	KWDerivationRule::RegisterDerivationRule(new ISDRReinforcementPartitionAt);
	KWDerivationRule::RegisterDerivationRule(new ISDRReinforcementClassChangeTagAt);
}