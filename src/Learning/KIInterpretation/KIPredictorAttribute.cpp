// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KIPredictorAttribute.h"

KIPredictorAttribute::KIPredictorAttribute()
{
	bUsed = false;
	dImportance = 0;

	// ## Custom constructor

	// ##
}

KIPredictorAttribute::~KIPredictorAttribute()
{
	// ## Custom destructor

	// ##
}

void KIPredictorAttribute::CopyFrom(const KIPredictorAttribute* aSource)
{
	require(aSource != NULL);

	bUsed = aSource->bUsed;
	sType = aSource->sType;
	sName = aSource->sName;
	dImportance = aSource->dImportance;

	// ## Custom copyfrom

	// ##
}

KIPredictorAttribute* KIPredictorAttribute::Clone() const
{
	KIPredictorAttribute* aClone;

	aClone = new KIPredictorAttribute;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void KIPredictorAttribute::Write(ostream& ost) const
{
	ost << "Used\t" << BooleanToString(GetUsed()) << "\n";
	ost << "Type\t" << GetType() << "\n";
	ost << "Name\t" << GetName() << "\n";
	ost << "Importance\t" << GetImportance() << "\n";
}

const ALString KIPredictorAttribute::GetClassLabel() const
{
	return "Predictor variable";
}

// ## Method implementation

int KIPredictorAttributeCompareImportance(const void* elem1, const void* elem2)
{
	KIPredictorAttribute* predictorAttribute1;
	KIPredictorAttribute* predictorAttribute2;
	int nDiff;

	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux attributs
	predictorAttribute1 = cast(KIPredictorAttribute*, *(Object**)elem1);
	predictorAttribute2 = cast(KIPredictorAttribute*, *(Object**)elem2);

	// Difference d'importance
	nDiff = -KWContinuous::CompareIndicatorValue(predictorAttribute1->GetImportance(),
						     predictorAttribute2->GetImportance());

	// Comparaison sur le nom en cas d'egalite
	if (nDiff == 0)
		nDiff = predictorAttribute1->GetName().Compare(predictorAttribute2->GetName());
	return nDiff;
}

const ALString KIPredictorAttribute::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

// ##
