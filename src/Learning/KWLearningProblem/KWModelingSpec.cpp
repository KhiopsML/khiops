// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWModelingSpec.h"

KWModelingSpec::KWModelingSpec()
{
	const SNBPredictorSelectiveNaiveBayes refPredictorSelectiveNaiveBayes;

	// Valeurs par defaut standard
	bBaselinePredictor = false;
	nUnivariatePredictorNumber = 0;

	// Choix du predicteur SNB
	predictorSelectiveNaiveBayes =
	    KWPredictor::ClonePredictor(refPredictorSelectiveNaiveBayes.GetName(), KWType::Symbol);

#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10
		DEPRECATEDSourceSubObjets = NULL;
	}
#endif // DEPRECATED_V10
}

KWModelingSpec::~KWModelingSpec()
{
	delete predictorSelectiveNaiveBayes;
}

void KWModelingSpec::CopyFrom(const KWModelingSpec* aSource)
{
	require(aSource != NULL);

	bBaselinePredictor = aSource->bBaselinePredictor;
	nUnivariatePredictorNumber = aSource->nUnivariatePredictorNumber;
}

KWModelingSpec* KWModelingSpec::Clone() const
{
	KWModelingSpec* aClone;

	aClone = new KWModelingSpec;
	aClone->CopyFrom(this);
	return aClone;
}

boolean KWModelingSpec::GetBaselinePredictor() const
{
	return bBaselinePredictor;
}

void KWModelingSpec::SetBaselinePredictor(boolean bValue)
{
	bBaselinePredictor = bValue;
}

int KWModelingSpec::GetUnivariatePredictorNumber() const
{
	return nUnivariatePredictorNumber;
}

void KWModelingSpec::SetUnivariatePredictorNumber(int nValue)
{
	nUnivariatePredictorNumber = nValue;
}

const ALString KWModelingSpec::GetClassLabel() const
{
	return "Predictors";
}

const ALString KWModelingSpec::GetObjectLabel() const
{
	ALString sLabel;

	return sLabel;
}

KWPredictor* KWModelingSpec::GetPredictorSelectiveNaiveBayes()
{
#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10
		if (DEPRECATEDSourceSubObjets != NULL)
			return DEPRECATEDSourceSubObjets->predictorSelectiveNaiveBayes;
	}
#endif // DEPRECATED_V10
	return predictorSelectiveNaiveBayes;
}

KWPredictorDataGrid* KWModelingSpec::GetPredictorDataGrid()
{
#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10
		if (DEPRECATEDSourceSubObjets != NULL)
			return &DEPRECATEDSourceSubObjets->predictorDataGrid;
	}
#endif // DEPRECATED_V10
	return &predictorDataGrid;
}

KWAttributeConstructionSpec* KWModelingSpec::GetAttributeConstructionSpec()
{
#ifdef DEPRECATED_V10
	{
		// DEPRECATED V10
		if (DEPRECATEDSourceSubObjets != NULL)
			return &DEPRECATEDSourceSubObjets->attributeConstructionSpec;
	}
#endif // DEPRECATED_V10
	return &attributeConstructionSpec;
}

#ifdef DEPRECATED_V10
void KWModelingSpec::DEPRECATEDSetSourceSubObjets(KWModelingSpec* source)
{
	DEPRECATEDSourceSubObjets = source;
}

boolean KWModelingSpec::DEPRECATEDIsUpdated(const KWModelingSpec* source) const
{
	boolean bIsUpdated = false;
	require(source != NULL);

	bIsUpdated = bIsUpdated or source->GetBaselinePredictor() != GetBaselinePredictor();
	bIsUpdated = bIsUpdated or source->GetUnivariatePredictorNumber() != GetUnivariatePredictorNumber();
	return bIsUpdated;
}

void KWModelingSpec::DEPRECATEDCopyFrom(const KWModelingSpec* source)
{
	require(source != NULL);

	CopyFrom(source);
}
#endif // DEPRECATED_V10