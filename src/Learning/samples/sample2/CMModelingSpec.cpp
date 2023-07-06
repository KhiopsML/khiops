// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "CMModelingSpec.h"

CMModelingSpec::CMModelingSpec()
{
	bTrainMajorityClassifier = true;
}

CMModelingSpec::~CMModelingSpec() {}

CMModelingSpec* CMModelingSpec::Clone() const
{
	CMModelingSpec* aClone;

	aClone = new CMModelingSpec;
	return aClone;
}

void CMModelingSpec::Write(ostream& ost) const
{
	ost << "Majority classifier\t" << BooleanToString(bTrainMajorityClassifier) << "\n";
}

const ALString CMModelingSpec::GetClassLabel() const
{
	return "Majoprity classifier";
}

const ALString CMModelingSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

CMMajorityClassifier* CMModelingSpec::GetMajorityClassifier()
{
	return &majorityClassifier;
}
