// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// Wed Jun 27 17:02:15 2007
// File generated  with GenereTable
// Insert your specific code inside "//## " sections

#include "CMModelingSpec.h"

CMModelingSpec::CMModelingSpec()
{
	bCMClassifier = true;

	// ## Custom constructor

	// ##
}

CMModelingSpec::~CMModelingSpec()
{

	// ## Custom destructor

	// ##
}

CMModelingSpec* CMModelingSpec::Clone() const
{
	CMModelingSpec* aClone;

	aClone = new CMModelingSpec;

	// aClone->bCMClassifier = bCMClassifier;
	// aClone->bForwardClassifier = bForwardClassifier;

	// ## Custom clone

	// ##
	return aClone;
}

void CMModelingSpec::Write(ostream& ost) const
{
	// ost << "CM classifier\t" << BooleanToString(IsForwardClassifier()) << "\n";
}

const ALString CMModelingSpec::GetClassLabel() const
{
	return "Classifieur logistique";
}

// ## Method implementation

const ALString CMModelingSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

CMMajorityClassifier* CMModelingSpec::GetClassifieurMajoritaire()
{
	return &classifieurMajoritaire;
}

// ##
