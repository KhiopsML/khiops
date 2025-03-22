// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCAnalysisSpec.h"

CCAnalysisSpec::CCAnalysisSpec()
{
	bVarPartCoclustering = false;
}

CCAnalysisSpec::~CCAnalysisSpec() {}

void CCAnalysisSpec::SetVarPartCoclustering(boolean bValue)
{
	bVarPartCoclustering = bValue;
}

CCCoclusteringSpec* CCAnalysisSpec::GetCoclusteringSpec()
{
	return &coclusteringSpec;
}

boolean CCAnalysisSpec::GetVarPartCoclustering() const
{
	return bVarPartCoclustering;
}

const ALString CCAnalysisSpec::GetCoclusteringLabelFromType(boolean bIsVarPartCoclustering)
{
	if (bIsVarPartCoclustering)
		return "Instances x Variables coclustering";
	else
		return "Variables coclustering";
}

boolean CCAnalysisSpec::GetCoclusteringTypeFromLabel(const ALString& sLabel)
{
	require(sLabel == GetCoclusteringLabelFromType(true) or sLabel == GetCoclusteringLabelFromType(false));
	return sLabel == GetCoclusteringLabelFromType(true);
}

KWDataGridOptimizerParameters* CCAnalysisSpec::GetOptimizationParameters()
{
	return &optimizationParameters;
}

const ALString CCAnalysisSpec::GetClassLabel() const
{
	return "Parameters";
}

const ALString CCAnalysisSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}
