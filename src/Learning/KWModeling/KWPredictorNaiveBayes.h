// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWPredictor.h"
#include "KWDRPreprocessing.h"
#include "KWDRMath.h"
#include "KWDRCompare.h"
#include "KWDRLogical.h"
#include "KWDRNBPredictor.h"
#include "KWDataGridStats.h"
#include "KWClassifierPostOptimizer.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Predicteur bayesien naif
// Le parametrage d'une instance KWClassStats est obligatoire
class KWPredictorNaiveBayes : public KWPredictor
{
public:
	// Constructeur
	KWPredictorNaiveBayes();
	~KWPredictorNaiveBayes();

	// Type de predicteur disponible: classification et regression
	boolean IsTargetTypeManaged(int nType) const override;

	// Constructeur generique
	KWPredictor* Create() const override;

	// Nom du predicteur
	const ALString GetName() const override;

	// Prefixe du predicteur
	const ALString GetPrefix() const override;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Redefinition de la methode d'apprentissage
	boolean InternalTrain() override;

	// Creation de la regle d'un predicteur bayesien naif dans la classe de preparation (dataPreparationClass) a partir de:
	// - Les attributs prepares a utiliser (oaUsedDataPreparationAttributes)
	// - Les poids des attributs (cvAttributeWeights)
	//
	// Cette creation filtre les attributs inutiles (partition source singleton ou poids nul)
	// Cela permet de specifier facilement des variantes de predicteur Bayesien dans des sous classes
	virtual void CreatePredictorAttributesInClass(KWDataPreparationClass* dataPreparationClass,
						      ObjectArray* oaUsedDataPreparationAttributes,
						      ContinuousVector* cvAttributeWeights);
	void InternalTrainNB(KWDataPreparationClass* dataPreparationClass,
			     ObjectArray* oaUsedDataPreparationAttributes);

	// Extraction d'un tableau de preparation d'attribut (KWDataPreparationStats)
	// a partir d'un tableau d'attribut prepares (KWDataPreparationAttribute)
	// Memoire: le tableau en sortie appartient a l'appelant, son contenu correspond au preparation references par
	// cahque attribut prepare
	void ExtractDataPreparationStats(const ObjectArray* oaDataPreparationAttributes,
					 ObjectArray* oaDataPreparationAttributeStats);

	////////////////////////////////////////////////////////////////////////////////////
	// Apprentissage dans le cas d'un classifieur

	// Pilotage de l'apprentissage dans le cas d'un classifieur
	void CreateClassifierAttributesInClass(KWDataPreparationClass* dataPreparationClass,
					       ObjectArray* oaUsedDataPreparationAttributes,
					       ContinuousVector* cvAttributeWeights);

	// Ajout de l'attribut classifieur
	KWAttribute* AddClassifierAttribute(KWDataPreparationClass* dataPreparationClass,
					    ObjectArray* oaUsedDataPreparationAttributes,
					    ContinuousVector* cvAttributeWeights);

	// Ajout des attribut de prediction pour la classification
	// Ajout egalement de l'attribut de prediction biaise en faveur du critere de classification
	// (cf. methode GetClassifierCriterion des TrainParameters, via la classe KWClassifierPostOptimizer)
	void AddClassifierPredictionAttributes(KWAttribute* classifierAttribute);

	////////////////////////////////////////////////////////////////////////////////////
	// Apprentissage dans le cas d'un regresseur

	// Pilotage de l'apprentissage dans le cas d'un regresseur
	void CreateRegressorAttributesInClass(KWDataPreparationClass* dataPreparationClass,
					      ObjectArray* oaUsedDataPreparationAttributes,
					      ContinuousVector* cvAttributeWeights);

	// Ajout de l'attribut regresseur de rang
	KWAttribute* AddRankRegressorAttribute(KWDataPreparationClass* dataPreparationClass,
					       ObjectArray* oaUsedDataPreparationAttributes,
					       ContinuousVector* cvAttributeWeights);

	// Ajout des attribut de prediction pour la regression de rang
	void AddRankRegressorPredictionAttributes(KWAttribute* rankRegressorAttribute);

	// Ajout de l'attribut regresseur de valeur
	KWAttribute* AddRegressorAttribute(KWAttribute* rankRegressorAttribute, KWAttribute* targetValuesAttribute,
					   ContinuousVector* cvAttributeWeights);

	// Ajout des attribut de prediction pour la regression de valeur
	void AddRegressorPredictionAttributes(KWTrainedRegressor* trainedRegressor, KWAttribute* regressorAttribute,
					      KWAttribute* targetValuesAttribute);

	void AddPredictorDataGridStatsAndBlockOperands(KWDerivationRule* predictorRule,
						       KWDRContinuousVector* weightRule,
						       KWDataPreparationClass* dataPreparationClass,
						       ObjectArray* oaUsedDataPreparationAttributes,
						       ContinuousVector* cvAttributeWeights);
};
