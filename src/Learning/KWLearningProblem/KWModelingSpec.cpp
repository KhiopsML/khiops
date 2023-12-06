// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "KWModelingSpec.h"

KWModelingSpec::KWModelingSpec()
{
	bDataPreparationOnly = false;
	bInterpretableNames = false;
	bBaselinePredictor = false;
	nUnivariatePredictorNumber = 0;
	bSelectiveNaiveBayesPredictor = false;
	bNaiveBayesPredictor = false;
	bDataGridPredictor = false;

	// ## Custom constructor

	// Par defaut, on construit des nom de variables interpretables
	bInterpretableNames = true;

	// Par defaut, on a le predicteur de base (regression uniquement) et le SNB,
	// que l'on peut desactiver soit directement, soit par DataPreparationOnly
	bBaselinePredictor = true;
	bSelectiveNaiveBayesPredictor = true;

	// ##
}

KWModelingSpec::~KWModelingSpec()
{
	// ## Custom destructor

	// ##
}

void KWModelingSpec::CopyFrom(const KWModelingSpec* aSource)
{
	require(aSource != NULL);

	bDataPreparationOnly = aSource->bDataPreparationOnly;
	bInterpretableNames = aSource->bInterpretableNames;
	bBaselinePredictor = aSource->bBaselinePredictor;
	nUnivariatePredictorNumber = aSource->nUnivariatePredictorNumber;
	bSelectiveNaiveBayesPredictor = aSource->bSelectiveNaiveBayesPredictor;
	bNaiveBayesPredictor = aSource->bNaiveBayesPredictor;
	bDataGridPredictor = aSource->bDataGridPredictor;

	// ## Custom copyfrom

	// ##
}

KWModelingSpec* KWModelingSpec::Clone() const
{
	KWModelingSpec* aClone;

	aClone = new KWModelingSpec;
	aClone->CopyFrom(this);

	// ## Custom clone

	// ##
	return aClone;
}

void KWModelingSpec::Write(ostream& ost) const
{
	ost << "Do data preparation only\t" << BooleanToString(GetDataPreparationOnly()) << "\n";
	ost << "Build interpretable names\t" << BooleanToString(GetInterpretableNames()) << "\n";
	ost << "Baseline predictor\t" << BooleanToString(GetBaselinePredictor()) << "\n";
	ost << "Number of univariate predictors\t" << GetUnivariatePredictorNumber() << "\n";
	ost << "Selective Naive Bayes predictor\t" << BooleanToString(GetSelectiveNaiveBayesPredictor()) << "\n";
	ost << "Naive Bayes predictor\t" << BooleanToString(GetNaiveBayesPredictor()) << "\n";
	ost << "Data Grid predictor\t" << BooleanToString(GetDataGridPredictor()) << "\n";
}

const ALString KWModelingSpec::GetClassLabel() const
{
	return "Predictors";
}

// ## Method implementation

const ALString KWModelingSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

KWPredictor* KWModelingSpec::GetPredictorSelectiveNaiveBayes()
{
	return &predictorSelectiveNaiveBayes;
}

KWPredictorDataGrid* KWModelingSpec::GetPredictorDataGrid()
{
	return &predictorDataGrid;
}

KWAttributeConstructionSpec* KWModelingSpec::GetAttributeConstructionSpec()
{
	return &attributeConstructionSpec;
}

// ##
