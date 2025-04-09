// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KIModelService.h"

KIModelService::KIModelService()
{
	nPredictorAttributeNumber = 0;

	// ## Custom constructor

	// ##
}

KIModelService::~KIModelService()
{
	// ## Custom destructor

	// ##
}

void KIModelService::CopyFrom(const KIModelService* aSource)
{
	require(aSource != NULL);

	sPredictorClassName = aSource->sPredictorClassName;
	nPredictorAttributeNumber = aSource->nPredictorAttributeNumber;

	// ## Custom copyfrom

	// ##
}

KIModelService* KIModelService::Clone() const
{
	KIModelService* aClone;

	aClone = new KIModelService;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void KIModelService::Write(ostream& ost) const
{
	ost << "Predictor dictionary\t" << GetPredictorClassName() << "\n";
	ost << "Number of predictor variables\t" << GetPredictorAttributeNumber() << "\n";
}

const ALString KIModelService::GetClassLabel() const
{
	return "Interpretation service";
}

// ## Method implementation

const ALString KIModelService::GetObjectLabel() const
{
	return sPredictorClassName;
}

boolean KIModelService::Check() const
{
	boolean bOk = true;

	// Test si un predicteur est disponible
	if (GetPredictorClassName() == "")
	{
		AddError("Missing predictor dictionary");
		bOk = false;
	}
	else if (KWClassDomain::GetCurrentDomain()->LookupClass(GetPredictorClassName()) == NULL)
	{
		AddError("Dictionary " + GetPredictorClassName() + " does not exist");
		bOk = false;
	}
	else if (not classBuilder.IsPredictorImported())
	{
		AddError("Dictionary " + GetPredictorClassName() + " is not a valid predictor");
		bOk = false;
	}

	// Erreur si le predicteur ne contient aucun attribut
	if (bOk and GetPredictorAttributeNumber() == 0)
	{
		AddError("No prediction variable in predictor " + GetPredictorClassName());
		bOk = false;
	}

	ensure(not bOk or GetPredictorClassName() == classBuilder.GetPredictorClass()->GetName());
	ensure(not bOk or classBuilder.GetPredictorClass()->IsCompiled());
	return bOk;
}

KIInterpretationClassBuilder* KIModelService::GetClassBuilder()
{
	return &classBuilder;
}

// ##
