// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated  with GenereTable

#include "CCInstancesVariablesCoclusteringSpec.h"

CCInstancesVariablesCoclusteringSpec::CCInstancesVariablesCoclusteringSpec() {}

CCInstancesVariablesCoclusteringSpec::~CCInstancesVariablesCoclusteringSpec()
{
	// ## Custom destructor

	oaAttributes.DeleteAll();
	oaAttributesAxes.DeleteAll();

	// ##
}

void CCInstancesVariablesCoclusteringSpec::CopyFrom(const CCInstancesVariablesCoclusteringSpec* aSource)
{
	require(aSource != NULL);

	sIdentifierAttribute = aSource->sIdentifierAttribute;
	// ## Custom copyfrom

	// ##
}

CCInstancesVariablesCoclusteringSpec* CCInstancesVariablesCoclusteringSpec::Clone() const
{
	CCInstancesVariablesCoclusteringSpec* aClone;

	aClone = new CCInstancesVariablesCoclusteringSpec;
	aClone->CopyFrom(this);

	// ## Custom clone

	// Pas de clone des sous-objets
	assert(false);

	// ##
	return aClone;
}

void CCInstancesVariablesCoclusteringSpec::Write(ostream& ost) const
{
	ost << "Identifier variable\t" << GetIdentifierAttribute() << "\n";
}

const ALString CCInstancesVariablesCoclusteringSpec::GetClassLabel() const
{
	return "Instances Variables Coclustering parameters";
}

// ## Method implementation

const ALString CCInstancesVariablesCoclusteringSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

ObjectArray* CCInstancesVariablesCoclusteringSpec::GetAttributes()
{
	return &oaAttributes;
}

int CCInstancesVariablesCoclusteringSpec::GetMaxCoclusteringAttributeNumber()
{
	return 50;
}

// CH AB
ObjectArray* CCInstancesVariablesCoclusteringSpec::GetAttributesAndAxes()
{
	return &oaAttributesAxes;
}

int CCInstancesVariablesCoclusteringSpec::GetMaxCoclusteringAxisNumber()
{
	return 10;
}
// Fin CH AB

// CH AB DDD
KWDataGridOptimizerParameters* CCInstancesVariablesCoclusteringSpec::GetOptimizationParameters()
{
	return &optimizationParameters;
}

// CH IV Refactoring: nettoyer lignes suivantes?
// void CCInstancesVariablesCoclusteringSpec::SetOptimizationParameters(KWDataGridOptimizerParameters* parameters)
//{
//	optimizationParameters = parameters;
//}
// ##
