// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCAnalysisSpec.h"

CCAnalysisSpec::CCAnalysisSpec()
{
	// CH IV Begin
	bGenericCoclustering = false;
	// CH IV End
}

CCAnalysisSpec::~CCAnalysisSpec() {}

CCCoclusteringSpec* CCAnalysisSpec::GetCoclusteringSpec()
{
	return &coclusteringSpec;
}

// CH IV Begin
CCInstancesVariablesCoclusteringSpec* CCAnalysisSpec::GetInstancesVariablesCoclusteringSpec()
{
	return &instancesVariablesCoclusteringSpec;
}

void CCAnalysisSpec::SetGenericCoclustering(boolean bValue)
{
	bGenericCoclustering = bValue;
}

boolean CCAnalysisSpec::GetGenericCoclustering() const
{
	return bGenericCoclustering;
}

void CCAnalysisSpec::SetCoclusteringType(const ALString& sValue)
{
	require(sValue == "Variables coclustering" or sValue == "Instances x Variables coclustering");
	bGenericCoclustering = (sValue == "Instances x Variables coclustering");
}

const ALString CCAnalysisSpec::GetCoclusteringType() const
{
	if (bGenericCoclustering)
		return "Instances x Variables coclustering";
	else
		return "Variables coclustering";
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
