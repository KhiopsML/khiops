// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

/*
 * #%L
 * Software Name: Khiops Interpretation
 * Version : 9.0
 * %%
 * Copyright (C) 2019 Orange
 * This software is the confidential and proprietary information of Orange.
 * You shall not disclose such confidential information and shall use it only
 * in accordance with the terms of the license agreement you entered into
 * with Orange.
 * #L%
 */

#include "KIInterpretationSpec.h"

KIInterpretationSpec::KIInterpretationSpec()
{
	interpretationDictionary = new KIInterpretationDictionary(this);
	leverClassSpec = new KWClassSpec;

	SetDefaultParameters();
}

KIInterpretationSpec::~KIInterpretationSpec()
{
	delete interpretationDictionary;
	delete leverClassSpec;
}

void KIInterpretationSpec::SetDefaultParameters()
{
	nVariableMaxNumber = 0;
	nWhyAttributesNumber = 1;
	nHowAttributesNumber = 0;
	sWhyType = "Normalized odds ratio";
	sWhyClass = PREDICTED_CLASS_LABEL;
	bSortWhyResults = true;
	bExpertMode = false;
	sHowClass = KIInterpretationDictionary::NO_VALUE_LABEL;
}

KWClassSpec* KIInterpretationSpec::GetLeverClassSpec() const
{
	return leverClassSpec;
}

int KIInterpretationSpec::GetHowAttributesNumber() const
{
	return nHowAttributesNumber;
}

void KIInterpretationSpec::SetHowAttributesNumber(int nNumber)
{
	nHowAttributesNumber = nNumber;
}

int KIInterpretationSpec::GetWhyAttributesNumber() const
{
	return nWhyAttributesNumber;
}

void KIInterpretationSpec::SetWhyAttributesNumber(int nNumber)
{
	nWhyAttributesNumber = nNumber;
}

int KIInterpretationSpec::GetMaxAttributesNumber() const
{
	return nVariableMaxNumber;
}

void KIInterpretationSpec::SetMaxAttributesNumber(int nNumber)
{
	nVariableMaxNumber = nNumber;
}

boolean KIInterpretationSpec::IsExpertMode() const
{
	return bExpertMode;
}

void KIInterpretationSpec::SetExpertMode(boolean bValue)
{
	bExpertMode = bValue;
}

boolean KIInterpretationSpec::GetSortWhyResults() const
{
	return bSortWhyResults;
}

void KIInterpretationSpec::SetSortWhyResults(boolean bValue)
{
	bSortWhyResults = bValue;
}

ALString KIInterpretationSpec::GetHowClass() const
{
	return sHowClass;
}

void KIInterpretationSpec::SetHowClass(ALString sValue)
{
	sHowClass = sValue;
}

ALString KIInterpretationSpec::GetWhyClass() const
{
	return sWhyClass;
}

void KIInterpretationSpec::SetWhyClass(ALString sValue)
{
	sWhyClass = sValue;
}

ALString KIInterpretationSpec::GetWhyType() const
{
	return sWhyType;
}

void KIInterpretationSpec::SetWhyType(ALString sValue)
{
	sWhyType = sValue;
}

KIInterpretationSpec* KIInterpretationSpec::Clone() const
{
	KIInterpretationSpec* spec = new KIInterpretationSpec;
	spec->bExpertMode = bExpertMode;
	spec->bSortWhyResults = bSortWhyResults;
	spec->nHowAttributesNumber = nHowAttributesNumber;
	spec->nWhyAttributesNumber = nWhyAttributesNumber;
	spec->sHowClass = sHowClass;
	spec->sWhyClass = sWhyClass;
	spec->sWhyType = sWhyType;

	assert(leverClassSpec != NULL);
	spec->leverClassSpec = leverClassSpec->Clone();

	return spec;
}

void KIInterpretationSpec::CopyFrom(KIInterpretationSpec* aSource)
{
	if (aSource != NULL)
	{
		bExpertMode = aSource->bExpertMode;
		bSortWhyResults = aSource->bSortWhyResults;
		nHowAttributesNumber = aSource->nHowAttributesNumber;
		nWhyAttributesNumber = aSource->nWhyAttributesNumber;
		sHowClass = aSource->sHowClass;
		sWhyClass = aSource->sWhyClass;
		sWhyType = aSource->sWhyType;

		assert(leverClassSpec != NULL);
		leverClassSpec->CopyFrom(aSource->leverClassSpec);
	}
}

void KIInterpretationSpec::WriteReport(ostream& ost)
{
	// Parametres de l'interpretation
	ost << "\n"
	    << "Interpretation parameters"
	    << "\n";
	ost << "Why attribute number "
	    << "\t" << nWhyAttributesNumber;
	ost << "Why class "
	    << "\t" << sWhyClass;
	ost << "Why method "
	    << "\t" << sWhyType;
	ost << "Sort Why Results "
	    << "\t" << bSortWhyResults;
	ost << "How attribute number "
	    << "\t" << nHowAttributesNumber;
	ost << "How class value "
	    << "\t" << sHowClass;

	ost << "\n\n";
}

const char* KIInterpretationSpec::PREDICTED_CLASS_LABEL = "Predicted class";
const char* KIInterpretationSpec::CLASS_OF_HIGHEST_GAIN_LABEL = "Class of highest gain";
const char* KIInterpretationSpec::ALL_CLASSES_LABEL = "All classes";