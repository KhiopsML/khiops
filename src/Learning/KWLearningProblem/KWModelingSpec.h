// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

////////////////////////////////////////////////////////////
// File generated with Genere tool
// Insert your specific code inside "//## " sections

#include "Object.h"

// ## Custom includes

#include "KWVersion.h"
#include "SNBPredictorSelectiveNaiveBayes.h"
#include "KWAttributeConstructionSpec.h"
#include "KWPredictorDataGrid.h"

// ##

////////////////////////////////////////////////////////////
// Classe KWModelingSpec
//    Predictors
class KWModelingSpec : public Object
{
public:
	// Constructeur
	KWModelingSpec();
	~KWModelingSpec();

	// Copie et duplication
	void CopyFrom(const KWModelingSpec* aSource);
	KWModelingSpec* Clone() const;

	///////////////////////////////////////////////////////////
	// Acces aux attributs

	// Do data preparation only
	boolean GetDataPreparationOnly() const;
	void SetDataPreparationOnly(boolean bValue);

	// Build interpretable names
	boolean GetInterpretableNames() const;
	void SetInterpretableNames(boolean bValue);

	// Baseline predictor
	boolean GetBaselinePredictor() const;
	void SetBaselinePredictor(boolean bValue);

	// Number of univariate predictors
	int GetUnivariatePredictorNumber() const;
	void SetUnivariatePredictorNumber(int nValue);

	// Selective Naive Bayes predictor
	boolean GetSelectiveNaiveBayesPredictor() const;
	void SetSelectiveNaiveBayesPredictor(boolean bValue);

	// Naive Bayes predictor
	boolean GetNaiveBayesPredictor() const;
	void SetNaiveBayesPredictor(boolean bValue);

	// Data Grid predictor
	boolean GetDataGridPredictor() const;
	void SetDataGridPredictor(boolean bValue);

	///////////////////////////////////////////////////////////
	// Divers

	// Ecriture
	void Write(ostream& ost) const override;

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

	// ## Custom declarations

	///////////////////////////////////////////////////////////
	// Parametrage avance

	// Parametrage d'un predicteur Bayesien selectif
	KWPredictor* GetPredictorSelectiveNaiveBayes();

	// Parametrage d'un predicteur Data Grid
	KWPredictorDataGrid* GetPredictorDataGrid();

	// Parametrage de la construction d'attributs
	KWAttributeConstructionSpec* GetAttributeConstructionSpec();

	// ##

	////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Attributs de la classe
	boolean bDataPreparationOnly;
	boolean bInterpretableNames;
	boolean bBaselinePredictor;
	int nUnivariatePredictorNumber;
	boolean bSelectiveNaiveBayesPredictor;
	boolean bNaiveBayesPredictor;
	boolean bDataGridPredictor;

	// ## Custom implementation

	// Parametrage des classifieurs
	SNBPredictorSelectiveNaiveBayes predictorSelectiveNaiveBayes;
	KWPredictorDataGrid predictorDataGrid;

	// Parametrage de la construction d'attributs
	KWAttributeConstructionSpec attributeConstructionSpec;

	// ##
};

////////////////////////////////////////////////////////////
// Implementations inline

inline boolean KWModelingSpec::GetDataPreparationOnly() const
{
	return bDataPreparationOnly;
}

inline void KWModelingSpec::SetDataPreparationOnly(boolean bValue)
{
	bDataPreparationOnly = bValue;
}

inline boolean KWModelingSpec::GetInterpretableNames() const
{
	return bInterpretableNames;
}

inline void KWModelingSpec::SetInterpretableNames(boolean bValue)
{
	bInterpretableNames = bValue;
}

inline boolean KWModelingSpec::GetBaselinePredictor() const
{
	return bBaselinePredictor;
}

inline void KWModelingSpec::SetBaselinePredictor(boolean bValue)
{
	bBaselinePredictor = bValue;
}

inline int KWModelingSpec::GetUnivariatePredictorNumber() const
{
	return nUnivariatePredictorNumber;
}

inline void KWModelingSpec::SetUnivariatePredictorNumber(int nValue)
{
	nUnivariatePredictorNumber = nValue;
}

inline boolean KWModelingSpec::GetSelectiveNaiveBayesPredictor() const
{
	return bSelectiveNaiveBayesPredictor;
}

inline void KWModelingSpec::SetSelectiveNaiveBayesPredictor(boolean bValue)
{
	bSelectiveNaiveBayesPredictor = bValue;
}

inline boolean KWModelingSpec::GetNaiveBayesPredictor() const
{
	return bNaiveBayesPredictor;
}

inline void KWModelingSpec::SetNaiveBayesPredictor(boolean bValue)
{
	bNaiveBayesPredictor = bValue;
}

inline boolean KWModelingSpec::GetDataGridPredictor() const
{
	return bDataGridPredictor;
}

inline void KWModelingSpec::SetDataGridPredictor(boolean bValue)
{
	bDataGridPredictor = bValue;
}

// ## Custom inlines

// ##
