// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWRecodingSpec.h"

KWRecodingSpec::KWRecodingSpec()
{
	bFilterAttributes = false;
	nMaxFilteredAttributeNumber = 0;
	bKeepInitialSymbolAttributes = false;
	bKeepInitialContinuousAttributes = false;
	bRecodeProbabilisticDistance = false;

	// ## Custom constructor

	bFilterAttributes = true;
	nMaxFilteredAttributeNumber = 0;
	bKeepInitialSymbolAttributes = false;
	bKeepInitialContinuousAttributes = false;
	sRecodeSymbolAttributes = "part Id";
	sRecodeContinuousAttributes = "part Id";
	sRecodeBivariateAttributes = "part Id";

	// ##
}

KWRecodingSpec::~KWRecodingSpec()
{
	// ## Custom destructor

	// ##
}

void KWRecodingSpec::CopyFrom(const KWRecodingSpec* aSource)
{
	require(aSource != NULL);

	bFilterAttributes = aSource->bFilterAttributes;
	nMaxFilteredAttributeNumber = aSource->nMaxFilteredAttributeNumber;
	bKeepInitialSymbolAttributes = aSource->bKeepInitialSymbolAttributes;
	bKeepInitialContinuousAttributes = aSource->bKeepInitialContinuousAttributes;
	sRecodeSymbolAttributes = aSource->sRecodeSymbolAttributes;
	sRecodeContinuousAttributes = aSource->sRecodeContinuousAttributes;
	sRecodeBivariateAttributes = aSource->sRecodeBivariateAttributes;
	bRecodeProbabilisticDistance = aSource->bRecodeProbabilisticDistance;

	// ## Custom copyfrom

	// ##
}

KWRecodingSpec* KWRecodingSpec::Clone() const
{
	KWRecodingSpec* aClone;

	aClone = new KWRecodingSpec;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void KWRecodingSpec::Write(ostream& ost) const
{
	ost << "Keep informative variables only\t" << BooleanToString(GetFilterAttributes()) << "\n";
	ost << "Max number of filtered variables\t" << GetMaxFilteredAttributeNumber() << "\n";
	ost << "Keep initial categorical variables\t" << BooleanToString(GetKeepInitialSymbolAttributes()) << "\n";
	ost << "Keep initial numerical variables\t" << BooleanToString(GetKeepInitialContinuousAttributes()) << "\n";
	ost << "Categorical recoding method\t" << GetRecodeSymbolAttributes() << "\n";
	ost << "Numerical recoding method\t" << GetRecodeContinuousAttributes() << "\n";
	ost << "Pairs recoding method\t" << GetRecodeBivariateAttributes() << "\n";
	ost << "Recode using prob distance (expert)\t" << BooleanToString(GetRecodeProbabilisticDistance()) << "\n";
}

const ALString KWRecodingSpec::GetClassLabel() const
{
	return "Recoding parameters";
}

// ## Method implementation

const ALString KWRecodingSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

boolean KWRecodingSpec::CheckMaxFilteredAttributeNumber(int nValue) const
{
	boolean bOk;
	bOk = nValue >= 0;
	if (not bOk)
		AddError("Max filtered variable number must positive");
	return bOk;
}

boolean KWRecodingSpec::CheckSymbolRecodingMethod(const ALString& sValue) const
{
	boolean bOk;

	bOk = sValue == "part Id" or sValue == "part label" or sValue == "0-1 binarization" or
	      sValue == "conditional info" or sValue == "none";
	if (not bOk)
		AddError("Incorrect categorical recoding method (" + sValue + ")");
	return bOk;
}

boolean KWRecodingSpec::CheckContinuousRecodingMethod(const ALString& sValue) const
{
	boolean bOk;

	bOk = sValue == "part Id" or sValue == "part label" or sValue == "0-1 binarization" or
	      sValue == "conditional info" or sValue == "center-reduction" or sValue == "0-1 normalization" or
	      sValue == "rank normalization" or sValue == "none";
	if (not bOk)
		AddError("Incorrect numerical recoding method (" + sValue + ")");
	return bOk;
}

boolean KWRecodingSpec::CheckBivariateRecodingMethod(const ALString& sValue) const
{
	boolean bOk;

	bOk = sValue == "part Id" or sValue == "part label" or sValue == "0-1 binarization" or
	      sValue == "conditional info" or sValue == "none";
	if (not bOk)
		AddError("Incorrect bivariate recoding method (" + sValue + ")");
	return bOk;
}

boolean KWRecodingSpec::Check() const
{
	return CheckMaxFilteredAttributeNumber(GetMaxFilteredAttributeNumber()) and
	       CheckSymbolRecodingMethod(GetRecodeSymbolAttributes()) and
	       CheckContinuousRecodingMethod(GetRecodeContinuousAttributes()) and
	       CheckBivariateRecodingMethod(GetRecodeBivariateAttributes());
}

// ##
