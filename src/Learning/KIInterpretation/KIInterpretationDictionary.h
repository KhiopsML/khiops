// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#ifndef PT_INTERPRETATION_DICTIONARY_H
#define PT_INTERPRETATION_DICTIONARY_H

#include "KWClass.h"

class ISAnalysisSpec;
class ISModelingSpec;
class ISLearningProblem;
class KIInterpretationSpec;
class ISAnalysisResults;
class KWTrainedClassifier;

class KIInterpretationDictionary : public Object
{
public:
	// Constructeurs
	KIInterpretationDictionary(KIInterpretationSpec* spec);
	~KIInterpretationDictionary();

	KWClassDomain* GetInterpretationDomain() const;

	KWClass* GetInterpretationRootClass() const;

	/// Acces au classifieur d'input valide. Retourne NULL si aucun classifieur valide n'a ete specifie.
	KWClass* GetInputClassifier();

	/// Test de compatibilite du dictionnaire a interpreter
	boolean ImportClassifier(KWClass* inputClassifier);

	/// Acces a la liste des valeurs cible
	const SymbolVector& GetTargetValues() const;

	/// Acces au tableau des noms variables predictives
	ObjectArray* GetPredictiveAttributeNamesArray();

	/// creer des meta-tags dans le dictionnaire, pour reperer les variables levier potentielles
	void PrepareInterpretationClass();

	/** creation ou mise a jour des attributs necessaires a l'interpretation (contribution ou reenforcement), dans
	 * le dico d'interpretation */
	boolean UpdateInterpretationAttributes();

	const Symbol SHAPLEY_LABEL = "Shapley";
	const Symbol NORMALIZED_ODDS_RATIO_LABEL = "NormalizedOddsRatio";
	const Symbol MIN_PROBA_DIFF_LABEL = "MinProbaDiff";
	const Symbol WEIGHT_EVIDENCE_LABEL = "WeightEvidence";
	const Symbol INFO_DIFF_LABEL = "InfoDiff";
	const Symbol DIFF_PROBA_LABEL = "DiffProba";
	const Symbol MODALITY_PROBA_LABEL = "ModalityProba";
	const Symbol BAYES_DISTANCE_LABEL = "BayesDistance";
	const Symbol KULLBACK_LABEL = "Kullback";
	const Symbol LOG_MODALITY_PROBA_LABEL = "LogModalityProba";
	const Symbol LOG_MIN_PROBA_DIFF_LABEL = "LogMinProbaDiff";
	const Symbol BAYES_DISTANCE_WITHOUT_PRIOR_LABEL = "BayesDistanceWithoutPrior";

	static const ALString LEVER_ATTRIBUTE_META_TAG;
	static const ALString INTERPRETATION_ATTRIBUTE_META_TAG;
	static const ALString NO_VALUE_LABEL;

protected:
	/** creation du domaine propre a l'interpretation, ainsi que le(s) dictionnaire(s) d'intepretation issus du
	 * classifieur d'entree */
	boolean CreateInterpretationDomain(const KWClass* inputClassifier);

	// creation des attributs de contribution du dico d'interpretation :
	boolean CreateContributionAttributesForClass(ALString sTargetClass, KWClass* kwcInterpretation,
						     const KWAttribute* classifierAttribute,
						     const KWAttribute* predictionAttribute,
						     const KWTrainedClassifier*);

	KWAttribute* CreateScoreContributionAttribute(ALString sTargetClass, KWClass* kwcInterpretation,
						      const KWAttribute* classifierAttribute,
						      const KWAttribute* predictionAttribute);

	KWAttribute* CreateContributionValueAtAttribute(const KWAttribute* scoreInterpretationAttribute,
							KWClass* kwcInterpretation, ALString sTargetClass, int nIndex);

	KWAttribute* CreateContributionNameAtAttribute(const KWAttribute* scoreInterpretationAttribute,
						       KWClass* kwcInterpretation, ALString sTargetClass, int nIndex);

	KWAttribute* CreateContributionPartitionAtAttribute(const KWAttribute* scoreInterpretationAttribute,
							    KWClass* kwcInterpretation, ALString sTargetClass,
							    int nIndex);

	KWAttribute* CreateClassPriorAttribute(ALString sTargetClass, KWClass* kwcInterpretation,
					       const KWAttribute* targetValuesAttribute,
					       const KWAttribute* predictionAttribute,
					       const KWAttribute* contributionClassAttribute);

	KWAttribute* CreateContributionClassAttribute(const KWAttribute* scoreInterpretationAttribute,
						      KWClass* kwcInterpretation, ALString sTargetClass);

	// creation des attributs de reenforcement du dico d'interpretation :
	boolean CreateReinforcementAttributesForClass(ALString sTargetClass, KWClass* kwcInterpretation,
						      const KWAttribute* classifierAttribute,
						      const KWAttribute* predictionAttribute,
						      const KWTrainedClassifier*);

	KWAttribute* CreateScoreReinforcementAttribute(ALString sTargetClass, KWClass* kwcInterpretation,
						       const KWAttribute* classifierAttribute,
						       const KWAttribute* predictionAttribute);

	KWAttribute* CreateReinforcementInitialScoreAttribute(const KWAttribute* scoreInterpretationAttribute,
							      KWClass* kwcInterpretation, ALString sTargetClass);

	KWAttribute* CreateReinforcementFinalScoreAtAttribute(const KWAttribute* scoreInterpretationAttribute,
							      KWClass* kwcInterpretation, ALString sTargetClass,
							      int nIndex);

	KWAttribute* CreateReinforcementNameAtAttribute(const KWAttribute* scoreInterpretationAttribute,
							KWClass* kwcInterpretation, ALString sTargetClass, int nIndex);

	KWAttribute* CreateReinforcementPartitionAtAttribute(const KWAttribute* scoreInterpretationAttribute,
							     KWClass* kwcInterpretation, ALString sTargetClass,
							     int nIndex);

	KWAttribute* CreateReinforcementClassChangeAtAttribute(const KWAttribute* scoreInterpretationAttribute,
							       KWClass* kwcInterpretation, ALString sTargetClass,
							       int nIndex);

	int ComputeReinforcementAttributesMaxNumber();

	void CleanImport();

	Symbol GetWhyTypeShortLabel(const ALString asWhyTypeLongLabel);

	// variables membres

	/** domaine temporaire servant a la generation du dico de transfert */
	KWClassDomain* kwcdInterpretationDomain;

	/** dictionnaire de transfert (dico maitre, dans le cas d'un classifieur multi table) */
	KWClass* kwcInterpretationRootClass;

	/** classifieur d'entree */
	KWClass* kwcInputClassifier;

	KIInterpretationSpec* interpretationSpec;

	SymbolVector sTargetValues;

	/* tableau de noms de variables partitionnees */
	ObjectArray oaPartitionedPredictiveAttributeNames;

	/* tableau de noms de variables natives */
	ObjectArray oaNativePredictiveAttributeNames;
};

inline KWClassDomain* KIInterpretationDictionary::GetInterpretationDomain() const
{
	return kwcdInterpretationDomain;
}

inline KWClass* KIInterpretationDictionary::GetInputClassifier()
{
	return kwcInputClassifier;
}

inline KWClass* KIInterpretationDictionary::GetInterpretationRootClass() const
{
	return kwcInterpretationRootClass;
}

#endif