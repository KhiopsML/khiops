// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "KWVersion.h"
#include "KWPredictorSelectiveNaiveBayes.h"
#include "SNBPredictorSelectiveNaiveBayes.h"
#include "KWAttributeConstructionSpec.h"
#include "KWPredictorDataGrid.h"

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

	// Baseline predictor
	boolean GetBaselinePredictor() const;
	void SetBaselinePredictor(boolean bValue);

	// Number of univariate predictors
	int GetUnivariatePredictorNumber() const;
	void SetUnivariatePredictorNumber(int nValue);

	// Parametrage d'un predicteur Bayesien selectif
	KWPredictor* GetPredictorSelectiveNaiveBayes();

	// Parametrage d'un predicteur Data Grid
	KWPredictorDataGrid* GetPredictorDataGrid();

	// Parametrage de la construction d'attributs
	KWAttributeConstructionSpec* GetAttributeConstructionSpec();

	///////////////////////////////////////////////////////////
	// Divers

	// Libelles utilisateur
	const ALString GetClassLabel() const override;
	const ALString GetObjectLabel() const override;

#ifdef DEPRECATED_V10
	// DEPRECATED V10: champ obsolete, conserve de facon cachee en V10 pour compatibilite ascendante des scenarios
	void DEPRECATEDSetSourceSubObjets(KWModelingSpec* source);

	// DEPRECATED V10: test si un champ a ete modifie par rapport a une version de reference
	virtual boolean DEPRECATEDIsUpdated(const KWModelingSpec* source) const;
	virtual void DEPRECATEDCopyFrom(const KWModelingSpec* source);
#endif // DEPRECATED_V10

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Attributs de la classe
	boolean bBaselinePredictor;
	int nUnivariatePredictorNumber;

	// Parametrage des classifieurs
	KWPredictor* predictorSelectiveNaiveBayes;
	KWPredictorDataGrid predictorDataGrid;

	// Parametrage de la construction d'attributs
	KWAttributeConstructionSpec attributeConstructionSpec;

#ifdef DEPRECATED_V10
	// DEPRECATED V10: memorisation de l'objet edite source, pour que les onglets obsolete editent les nouveaux
	// sous-objets
	KWModelingSpec* DEPRECATEDSourceSubObjets;
#endif // DEPRECATED_V10
};
