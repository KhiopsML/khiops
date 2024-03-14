// Copyright (c) 2024 Orange. All rights reserved.
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

//////////////////////////////////////////////////////////////////////////////
// Predicteur bayesien naif
// Le parametrage des ClassStats est obligatoire
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

	/////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Redefinition de la methode d'apprentissage
	boolean InternalTrain() override;

	// Construction d'un predicteur bayesien naif a partir d'un tableau d'attributs prepares
	// La methode filtre les attributs inutiles (partition source singleton ou poids nul)
	// Cela permet de specifier facilement des variantes de predicteur Bayesien dans des sous classes
	// Parametres:
	//     dataPreparationClass: classe de preparation a completer avec les attributs du predicteur
	//     oaUsedDataPreparationAttributes: les attributs prepares a utiliser
	//	   cvAttributeWeights: poids des attributs (indexes par leur index dans la dataPreparationClass)
	void InternalTrainNB(KWDataPreparationClass* dataPreparationClass,
			     ObjectArray* oaUsedDataPreparationAttributes);
	virtual void InternalTrainFinishTrainedPredictor(KWDataPreparationClass* dataPreparationClass,
							 ObjectArray* oaUsedDataPreparationAttributes,
							 ContinuousVector* cvAttributeWeights);

	// Extraction d'un tableau de preparation d'attribut (KWDataPreparationStats)
	// a partir d'un tableau d'attribut prepares (KWDataPreparationAttribute)
	// Memoire: le tableau en sortie appartient a l'appelant, son contenu correspond au preparation references par
	// cahque attribut prepare
	void ExtractDataPreparationStats(const ObjectArray* oaDataPreparationAttributes,
					 ObjectArray* oaDataPreparationAttributeStats);

	////////////////////////////////////////////////////////////////////////////////////
	// Apprentissage dans le cas d'un classifieur

	// Pilotage de l'apprentissage dans le cas d'un classifieur
	void InternalTrainFinishTrainedClassifier(KWDataPreparationClass* dataPreparationClass,
						  ObjectArray* oaUsedDataPreparationAttributes,
						  ContinuousVector* cvAttributeWeights);

	// Ajout de l'attribut classifieur
	KWAttribute* AddClassifierAttribute(KWDataPreparationClass* dataPreparationClass,
					    ObjectArray* oaUsedDataPreparationAttributes,
					    ContinuousVector* cvAttributeWeights);
	// DDD
	KWAttribute* OLDAddClassifierAttribute(KWTrainedClassifier* trainedClassifier,
					       ObjectArray* oaUsedDataPreparationAttributes,
					       ContinuousVector* cvAttributeWeights);

	// Ajout des attribut de prediction pour la classification
	// Ajout egalement de l'attribut de prediction biaise en faveur du critere de classification
	// (cf. methode GetClassifierCriterion des TrainParameters, via la classe KWClassifierPostOptimizer)
	void AddClassifierPredictionAttributes(KWAttribute* classifierAttribute);

	////////////////////////////////////////////////////////////////////////////////////
	// Apprentissage dans le cas d'un regresseur

	// Pilotage de l'apprentissage dans le cas d'un regresseur
	void InternalTrainFinishTrainedRegressor(KWDataPreparationClass* dataPreparationClass,
						 ObjectArray* oaUsedDataPreparationAttributes,
						 ContinuousVector* cvAttributeWeights);

	// Ajout de l'attribut regresseur de rang
	KWAttribute* AddRankRegressorAttribute(KWDataPreparationClass* dataPreparationClass,
					       ObjectArray* oaUsedDataPreparationAttributes,
					       ContinuousVector* cvAttributeWeights);

	// DDD
	KWAttribute* OLDAddRankRegressorAttribute(KWTrainedRegressor* trainedRegressor,
						  ObjectArray* oaUsedDataPreparationAttributes,
						  ContinuousVector* cvAttributeWeights);

	// Ajout des attribut de prediction pour la regression de rang
	void AddRankRegressorPredictionAttributes(KWTrainedRegressor* trainedRegressor,
						  KWAttribute* rankRegressorAttribute);

	// Ajout de l'attribut regresseur de valeur
	KWAttribute* AddRegressorAttribute(KWTrainedRegressor* trainedRegressor, KWAttribute* rankRegressorAttribute,
					   KWAttribute* targetValuesAttribute, ContinuousVector* cvAttributeWeights);

	// Ajout des attribut de prediction pour la regression de valeur
	void AddRegressorPredictionAttributes(KWTrainedRegressor* trainedRegressor, KWAttribute* regressorAttribute,
					      KWAttribute* targetValuesAttribute);

	void AddPredictorDataGridStatsAndBlockOperands(KWDerivationRule* predictorRule,
						       KWDRContinuousVector* weightRule,
						       KWDataPreparationClass* dataPreparationClass,
						       ObjectArray* oaUsedDataPreparationAttributes,
						       ContinuousVector* cvAttributeWeights);
};
