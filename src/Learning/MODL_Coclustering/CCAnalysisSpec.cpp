// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CCAnalysisSpec.h"

CCAnalysisSpec::CCAnalysisSpec() {}

CCAnalysisSpec::~CCAnalysisSpec() {}

CCCoclusteringSpec* CCAnalysisSpec::GetCoclusteringSpec()
{
	return &coclusteringSpec;
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
