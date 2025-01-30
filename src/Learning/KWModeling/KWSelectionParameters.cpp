// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWSelectionParameters.h"

KWSelectionParameters::KWSelectionParameters()
{
	nMaxSelectedAttributeNumber = 0;
	nOptimizationLevel = 0;
	dPriorWeight = 0;
	dPriorExponent = 0;
	bConstructionCost = false;
	bPreparationCost = false;
	nTraceLevel = 0;
	bTraceSelectedAttributes = false;

	// ## Custom constructor

	sSelectionCriterion = "CMA";
	sOptimizationAlgorithm = "MS_FFWBW";

	// Par defaut, on prend en compte les prior dans le SNB
	// Le poids de prior a 0.25 releve d'une etude empirique
	dPriorWeight = 0.25;
	bConstructionCost = true;
	bPreparationCost = true;
	dPriorExponent = 0.95;
	// ##
}

KWSelectionParameters::~KWSelectionParameters()
{
	// ## Custom destructor

	// ##
}

void KWSelectionParameters::CopyFrom(const KWSelectionParameters* aSource)
{
	require(aSource != NULL);

	nMaxSelectedAttributeNumber = aSource->nMaxSelectedAttributeNumber;
	sSelectionCriterion = aSource->sSelectionCriterion;
	sOptimizationAlgorithm = aSource->sOptimizationAlgorithm;
	nOptimizationLevel = aSource->nOptimizationLevel;
	dPriorWeight = aSource->dPriorWeight;
	dPriorExponent = aSource->dPriorExponent;
	bConstructionCost = aSource->bConstructionCost;
	bPreparationCost = aSource->bPreparationCost;
	nTraceLevel = aSource->nTraceLevel;
	bTraceSelectedAttributes = aSource->bTraceSelectedAttributes;

	// ## Custom copyfrom

	// ##
}

KWSelectionParameters* KWSelectionParameters::Clone() const
{
	KWSelectionParameters* aClone;

	aClone = new KWSelectionParameters;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void KWSelectionParameters::Write(ostream& ost) const
{
	ost << "Max number of selected variables\t" << GetMaxSelectedAttributeNumber() << "\n";
	ost << "Selection criterion\t" << GetSelectionCriterion() << "\n";
	ost << "Optimization algorithm\t" << GetOptimizationAlgorithm() << "\n";
	ost << "Optimization level\t" << GetOptimizationLevel() << "\n";
	ost << "Prior weight (expert)\t" << GetPriorWeight() << "\n";
	ost << "Prior exponent (expert)\t" << GetPriorExponent() << "\n";
	ost << "Construction cost (expert)\t" << BooleanToString(GetConstructionCost()) << "\n";
	ost << "Preparation cost (expert)\t" << BooleanToString(GetPreparationCost()) << "\n";
	ost << "Trace level\t" << GetTraceLevel() << "\n";
	ost << "Trace selected variables\t" << BooleanToString(GetTraceSelectedAttributes()) << "\n";
}

const ALString KWSelectionParameters::GetClassLabel() const
{
	return "Selection parameters";
}

// ## Method implementation

const ALString KWSelectionParameters::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

boolean KWSelectionParameters::CheckMaxSelectedAttributeNumber(int nValue) const
{
	boolean bOk;
	bOk = nValue >= 0;
	if (not bOk)
		AddError("Max selected variable number must positive");
	return bOk;
}

boolean KWSelectionParameters::CheckSelectionCriterion(const ALString& sValue) const
{
	boolean bOk;

	bOk = sValue == "CMA" or sValue == "MA" or sValue == "MAP";
	if (not bOk)
		AddError("Incorrect selection criterion (" + sValue + ")");
	return bOk;
}

boolean KWSelectionParameters::CheckOptimizationAlgorithm(const ALString& sValue) const
{
	boolean bOk;

	bOk = sValue == "MS_FFWBW" or sValue == "OPT" or sValue == "FW" or sValue == "FWBW" or sValue == "FFW" or
	      sValue == "FFWBW";
	if (not bOk)
		AddError("Incorrect optimization algorithm (" + sValue + ")");
	return bOk;
}

boolean KWSelectionParameters::CheckOptimizationLevel(int nValue) const
{
	boolean bOk;
	bOk = nValue >= 0;
	if (not bOk)
		AddError("Optimization level must positive");
	return bOk;
}

boolean KWSelectionParameters::CheckTraceLevel(int nValue) const
{
	boolean bOk;
	bOk = 0 <= nValue and nValue <= 3;
	if (not bOk)
		AddError("Trace level must be between 0 and 3");
	return bOk;
}

boolean KWSelectionParameters::Check() const
{
	return CheckMaxSelectedAttributeNumber(GetMaxSelectedAttributeNumber()) and
	       CheckSelectionCriterion(GetSelectionCriterion()) and
	       CheckOptimizationAlgorithm(GetOptimizationAlgorithm()) and
	       CheckOptimizationLevel(GetOptimizationLevel()) and CheckTraceLevel(GetTraceLevel());
}

// ##
