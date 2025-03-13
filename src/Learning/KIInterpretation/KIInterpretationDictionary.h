// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class ISAnalysisSpec;
class ISModelingSpec;
class ISLearningProblem;
class KIInterpretationSpec;
class ISAnalysisResults;
class KWTrainedClassifier;

#include "KWClass.h"
#include "KWDRNBPredictor.h"
#include "KIInterpretationSpec.h"
#include "KIDRPredictor.h"
#include "KWTrainedPredictor.h"

///////////////////////////////////////////////////////////////
// Classe KIInterpretationDictionary
// Gestion d'une classe (KWClass) dediee a l'interpretation d'un predicteur

class KIInterpretationDictionary : public Object
{
public:
	// Constructeur
	KIInterpretationDictionary(KIInterpretationSpec* spec);
	~KIInterpretationDictionary();

	// Domaine contenant la classe d'interpretation
	KWClassDomain* GetInterpretationDomain() const;

	// Classe d'interpretation principale
	KWClass* GetInterpretationMainClass() const;

	// Acces au classifieur d'input valide.
	// Retourne NULL si aucun classifieur valide n'a ete specifie.
	KWClass* GetInputClassifier();

	// Test de compatibilite du dictionnaire a interpreter
	boolean ImportClassifier(KWClass* inputClassifier);

	// Test de compatibilite du dictionnaire a ne pas utiliser l'option group target Values
	boolean TestGroupTargetValues(KWClass* inputClassifier);

	// Acces a la liste des valeurs cible
	const SymbolVector* GetTargetValues() const;

	// Acces au tableau des noms variables predictives
	StringVector* GetPredictiveAttributeNamesArray();

	// Creation des meta-tags dans le dictionnaire, pour reperer les variables levier potentielles
	void PrepareInterpretationClass();

	// Creation ou mise a jour des attributs necessaires a l'interpretation (contribution ou reenforcement),
	// dans le dico d'interpretation
	boolean UpdateInterpretationAttributes();

	// Types de methodes d'interpretation
	const ALString SHAPLEY_LABEL = "Shapley";
	const ALString UNDEFINED_LABEL = "Undefined";

	// Definition des type de meta-data permettant de reperer les variables dediees
	// a l'interpretation dans les dictionnaires
	static const ALString LEVER_ATTRIBUTE_META_TAG;
	static const ALString INTERPRETATION_ATTRIBUTE_META_TAG;
	static const ALString NO_VALUE_LABEL;

	//////////////////////////////////////////////////
	//// Implementation
protected:
	// Creation du domaine propre a l'interpretation, ainsi que le(s) dictionnaire(s)
	// d'intepretation issus du classifieur d'entree
	boolean CreateInterpretationDomain(const KWClass* inputClassifier);

	// Creation des attributs de contribution du dico d'interpretation :
	boolean CreateContributionAttributesForClass(ALString sTargetClass, KWClass* kwcInterpretation,
						     const KWAttribute* classifierAttribute,
						     const KWAttribute* predictionAttribute,
						     const KWTrainedClassifier*);

	// ???
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

	// Creation des attributs de reenforcement du dico d'interpretation :
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

	const ALString& GetWhyTypeShortLabel(const ALString& asWhyTypeLongLabel);

	////////////////////////////
	// Variables membres

	// Domaine temporaire servant a la generation du dico de transfert
	KWClassDomain* kwcdInterpretationDomain;

	// Dictionnaire de transfert (dico principal, dans le cas d'un classifieur multi table)
	KWClass* kwcInterpretationMainClass;

	// Classifieur d'entree
	KWClass* kwcInputClassifier;

	// Specification d'interpretation
	KIInterpretationSpec* interpretationSpec;

	SymbolVector svTargetValues;

	// Noms des variables partitionnees
	StringVector svPartitionedPredictiveAttributeNames;

	// Noms des variables natives
	StringVector svNativePredictiveAttributeNames;
};

inline KWClassDomain* KIInterpretationDictionary::GetInterpretationDomain() const
{
	return kwcdInterpretationDomain;
}

inline KWClass* KIInterpretationDictionary::GetInputClassifier()
{
	return kwcInputClassifier;
}

inline KWClass* KIInterpretationDictionary::GetInterpretationMainClass() const
{
	return kwcInterpretationMainClass;
}
