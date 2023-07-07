// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "CCVarPartCoclusteringSpec.h"

CCVarPartCoclusteringSpec::CCVarPartCoclusteringSpec()
{
	// ## Custom constructor

	// ##
}

CCVarPartCoclusteringSpec::~CCVarPartCoclusteringSpec()
{
	// ## Custom destructor

	oaAttributes.DeleteAll();
	oaAttributesAxes.DeleteAll();

	// ##
}

void CCVarPartCoclusteringSpec::CopyFrom(const CCVarPartCoclusteringSpec* aSource)
{
	require(aSource != NULL);

	sIdentifierAttribute = aSource->sIdentifierAttribute;

	// ## Custom copyfrom

	// ##
}

CCVarPartCoclusteringSpec* CCVarPartCoclusteringSpec::Clone() const
{
	CCVarPartCoclusteringSpec* aClone;

	aClone = new CCVarPartCoclusteringSpec;
	aClone->CopyFrom(this);

	// ## Custom clone

	// Pas de clone des sous-objets
	assert(false);

	// ##
	return aClone;
}

void CCVarPartCoclusteringSpec::Write(ostream& ost) const
{
	ost << "Identifier variable\t" << GetIdentifierAttribute() << "\n";
}

const ALString CCVarPartCoclusteringSpec::GetClassLabel() const
{
	return "Instances Variables coclustering parameters";
}

// ## Method implementation

const ALString CCVarPartCoclusteringSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

ObjectArray* CCVarPartCoclusteringSpec::GetAttributes()
{
	return &oaAttributes;
}

int CCVarPartCoclusteringSpec::GetMaxCoclusteringAttributeNumber()
{
	return 50;
}

// CH AB
ObjectArray* CCVarPartCoclusteringSpec::GetAttributesAndAxes()
{
	return &oaAttributesAxes;
}

int CCVarPartCoclusteringSpec::GetMaxCoclusteringAxisNumber()
{
	return 10;
}
// Fin CH AB

// CH AB DDD
KWDataGridOptimizerParameters* CCVarPartCoclusteringSpec::GetOptimizationParameters()
{
	return &optimizationParameters;
}

// CH IV Refactoring: nettoyer lignes suivantes?
// void CCVarPartCoclusteringSpec::SetOptimizationParameters(KWDataGridOptimizerParameters* parameters)
//{
//	optimizationParameters = parameters;
//}

// ##