// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KIModelReinforcer.h"

KIModelReinforcer::KIModelReinforcer()
{
	// ## Custom constructor

	// ##
}

KIModelReinforcer::~KIModelReinforcer()
{
	// ## Custom destructor

	// ##
}

void KIModelReinforcer::CopyFrom(const KIModelReinforcer* aSource)
{
	require(aSource != NULL);

	KIModelService::CopyFrom(aSource);

	sReinforcedTargetValue = aSource->sReinforcedTargetValue;

	// ## Custom copyfrom

	// ##
}

KIModelReinforcer* KIModelReinforcer::Clone() const
{
	KIModelReinforcer* aClone;

	aClone = new KIModelReinforcer;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void KIModelReinforcer::Write(ostream& ost) const
{
	KIModelService::Write(ost);
	ost << "Target value to reinforce\t" << GetReinforcedTargetValue() << "\n";
}

const ALString KIModelReinforcer::GetClassLabel() const
{
	return "Reinforce model";
}

// ## Method implementation

const ALString KIModelReinforcer::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

// ##
