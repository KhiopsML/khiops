// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDDomainConstraint.h"

KDDomainConstraint::KDDomainConstraint() {}

KDDomainConstraint::~KDDomainConstraint() {}

KDEntitySet* KDDomainConstraint::GetCoveredRules()
{
	return &coveredRules;
}

KDEntitySet* KDDomainConstraint::GetConstrainedClasses()
{
	return &constrainedClasses;
}

boolean KDDomainConstraint::Check() const
{
	return coveredRules.Check() and constrainedClasses.Check();
}

void KDDomainConstraint::Write(ostream& ost) const
{
	ost << coveredRules << " in " << constrainedClasses << "\n";
}

const ALString KDDomainConstraint::GetClassLabel() const
{
	return "Domain constraint";
}
