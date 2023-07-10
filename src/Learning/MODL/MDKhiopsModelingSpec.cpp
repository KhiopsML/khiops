// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "MDKhiopsModelingSpec.h"

MDKhiopsModelingSpec::MDKhiopsModelingSpec()
{
	bSelectiveNaiveBayesPredictor = false;
	bMAPNaiveBayesPredictor = false;
	bNaiveBayesPredictor = false;
	bDataGridPredictor = false;

	// ## Custom constructor

	// Valeurs par defaut standard
	bSelectiveNaiveBayesPredictor = true;
	bMAPNaiveBayesPredictor = false;
	bNaiveBayesPredictor = false;
	bDataGridPredictor = false;

	// ##
}

MDKhiopsModelingSpec::~MDKhiopsModelingSpec()
{
	// ## Custom destructor

	// ##
}

void MDKhiopsModelingSpec::CopyFrom(const MDKhiopsModelingSpec* aSource)
{
	require(aSource != NULL);

	KWModelingSpec::CopyFrom(aSource);

	bSelectiveNaiveBayesPredictor = aSource->bSelectiveNaiveBayesPredictor;
	bMAPNaiveBayesPredictor = aSource->bMAPNaiveBayesPredictor;
	bNaiveBayesPredictor = aSource->bNaiveBayesPredictor;
	bDataGridPredictor = aSource->bDataGridPredictor;

	// ## Custom copyfrom

	// ##
}

MDKhiopsModelingSpec* MDKhiopsModelingSpec::Clone() const
{
	MDKhiopsModelingSpec* aClone;

	aClone = new MDKhiopsModelingSpec;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void MDKhiopsModelingSpec::Write(ostream& ost) const
{
	KWModelingSpec::Write(ost);
	ost << "Selective Naive Bayes predictor\t" << BooleanToString(GetSelectiveNaiveBayesPredictor()) << "\n";
	ost << "MAP Naive Bayes predictor\t" << BooleanToString(GetMAPNaiveBayesPredictor()) << "\n";
	ost << "Naive Bayes predictor\t" << BooleanToString(GetNaiveBayesPredictor()) << "\n";
	ost << "Data Grid predictor\t" << BooleanToString(GetDataGridPredictor()) << "\n";
}

const ALString MDKhiopsModelingSpec::GetClassLabel() const
{
	return "Modeling parameters";
}

// ## Method implementation

const ALString MDKhiopsModelingSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

#ifdef DEPRECATED_V10
boolean MDKhiopsModelingSpec::DEPRECATEDIsUpdated(const KWModelingSpec* source) const
{
	boolean bIsUpdated = false;
	MDKhiopsModelingSpec* mdSource;

	require(source != NULL);

	mdSource = cast(MDKhiopsModelingSpec*, source);
	bIsUpdated = bIsUpdated or KWModelingSpec::DEPRECATEDIsUpdated(source);
	bIsUpdated = bIsUpdated or mdSource->GetSelectiveNaiveBayesPredictor() != GetSelectiveNaiveBayesPredictor();
	bIsUpdated = bIsUpdated or mdSource->GetMAPNaiveBayesPredictor() != GetMAPNaiveBayesPredictor();
	bIsUpdated = bIsUpdated or mdSource->GetNaiveBayesPredictor() != GetNaiveBayesPredictor();
	bIsUpdated = bIsUpdated or mdSource->GetDataGridPredictor() != GetDataGridPredictor();
	return bIsUpdated;
}

void MDKhiopsModelingSpec::DEPRECATEDCopyFrom(const KWModelingSpec* source)
{
	require(source != NULL);

	KWModelingSpec::DEPRECATEDCopyFrom(source);
	CopyFrom(cast(MDKhiopsModelingSpec*, source));
}
#endif // DEPRECATED_V10

// ##
