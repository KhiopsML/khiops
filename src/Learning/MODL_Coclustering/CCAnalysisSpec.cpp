// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCAnalysisSpec.h"

CCAnalysisSpec::CCAnalysisSpec()
{
	// CH IV Begin
	bVarPartCoclustering = false;
	// CH IV End
}

CCAnalysisSpec::~CCAnalysisSpec() {}

CCCoclusteringSpec* CCAnalysisSpec::GetCoclusteringSpec()
{
	return &coclusteringSpec;
}

// CH IV Begin
CCVarPartCoclusteringSpec* CCAnalysisSpec::GetVarPartCoclusteringSpec()
{
	return &varPartCoclusteringSpec;
}

void CCAnalysisSpec::SetVarPartCoclustering(boolean bValue)
{
	bVarPartCoclustering = bValue;
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
// CH IV End

const ALString CCAnalysisSpec::GetClassLabel() const
{
	return "Parameters";
}

const ALString CCAnalysisSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}
