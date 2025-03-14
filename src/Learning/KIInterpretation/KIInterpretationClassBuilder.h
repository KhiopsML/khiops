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
// Classe KIInterpretationClassBuilder
// Creation d'une classe (KWClass) dediee a l'interpretation
// ou au renforcement d'un predicteur
class KIInterpretationClassBuilder : public Object
{
public:
	// Constructeur
	KIInterpretationClassBuilder(KIInterpretationSpec* spec);
	~KIInterpretationClassBuilder();

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
	///// Implementation
protected:
	// Creation du domaine propre a l'interpretation, ainsi que du dictionnaire
	// d'intepretation ou de renforcement issus du classifieur d'entree
	boolean CreateInterpretationDomain(const KWClass* inputClassifier);

	////////////////////////////////////////////////////////////////////////////
	// Creation des attributs de contribution du dictionnaite d'interpretation

	// Creation des attributs de contribution du dictionnaire d'interpretation
	void CreateContributionAttributesForClass(KWClass* kwcInterpretation, const ALString& sTargetClass,
						  const KWAttribute* classifierAttribute,
						  const KWAttribute* predictionAttribute, const KWTrainedClassifier*);

	// Creation de l'attribut gerant le renforcement
	KWAttribute* CreateScoreContributionAttribute(KWClass* kwcInterpretation, const ALString& sTargetClass,
						      const KWAttribute* classifierAttribute,
						      const KWAttribute* predictionAttribute);

	// Creation de l'attribut de valeur d'importance pour une valeur de classe et un index d'attribut d'importance
	KWAttribute* CreateContributionValueAtAttribute(KWClass* kwcInterpretation, const ALString& sTargetClass,
							const KWAttribute* scoreInterpretationAttribute, int nIndex);

	// Creation de l'attribut du nom de la variable d'importance pour une valeur de classe et un index d'attribut d'importance
	KWAttribute* CreateContributionNameAtAttribute(KWClass* kwcInterpretation, const ALString& sTargetClass,
						       const KWAttribute* scoreInterpretationAttribute, int nIndex);

	// Creation de l'attribut de la partie de la variable d'importance pour une valeur de classe et un index d'attribut d'importance
	KWAttribute* CreateContributionPartAtAttribute(KWClass* kwcInterpretation, const ALString& sTargetClass,
						       const KWAttribute* scoreInterpretationAttribute, int nIndex);

	////////////////////////////////////////////////////////////////////////////
	// Creation des attributs de renforcement du dictionnaite d'interpretation

	// Creation de l'ensemble des attributs de renforcement
	void CreateReinforcementAttributesForClass(KWClass* kwcInterpretation, const ALString& sTargetClass,
						   const KWAttribute* classifierAttribute,
						   const KWAttribute* predictionAttribute, const KWTrainedClassifier*);

	// Creation de l'attribut gerant le renforcement
	KWAttribute* CreateScoreReinforcementAttribute(KWClass* kwcInterpretation, const ALString& sTargetClass,
						       const KWAttribute* classifierAttribute,
						       const KWAttribute* predictionAttribute);

	// Creation de l'attribut pour le score initial
	KWAttribute* CreateReinforcementInitialScoreAttribute(KWClass* kwcInterpretation, const ALString& sTargetClass,
							      const KWAttribute* scoreInterpretationAttribute);

	// Creation de l'attribut pour le score final, pour un index d'attribut de renfortcement
	KWAttribute* CreateReinforcementFinalScoreAtAttribute(KWClass* kwcInterpretation, const ALString& sTargetClass,
							      const KWAttribute* scoreInterpretationAttribute,
							      int nIndex);

	// Creation de l'attribut pour le nom de la variable, pour un index d'attribut de renfortcement
	KWAttribute* CreateReinforcementNameAtAttribute(KWClass* kwcInterpretation, const ALString& sTargetClass,
							const KWAttribute* scoreInterpretationAttribute, int nIndex);

	// Creation de l'attribut pour la partie de variable, pour un index d'attribut de renfortcement
	KWAttribute* CreateReinforcementPartAtAttribute(KWClass* kwcInterpretation, const ALString& sTargetClass,
							const KWAttribute* scoreInterpretationAttribute, int nIndex);

	// Creation de l'attribut pour le changement de classe, pour un index d'attribut de renfortcement
	KWAttribute* CreateReinforcementClassChangeAtAttribute(KWClass* kwcInterpretation, const ALString& sTargetClass,
							       const KWAttribute* scoreInterpretationAttribute,
							       int nIndex);

	// Calcul du nombre max d'attributs de renfocrement
	// Ce nombre max est la valeur minimum entre:
	// - le nombre d'attributs pour le renforcement, parametre via IHM
	// - le nombre d'attributs selectionnes comme pouvant etre utilises comme variables leviers, , parametre via IHM
	int ComputeReinforcementAttributesMaxNumber();

	// Nettoyage
	void CleanImport();

	// Libelle du type d'interpretation
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

	// Vecteur des valeurs cibles
	SymbolVector svTargetValues;

	// Noms des variables partitionnees
	StringVector svPartitionedPredictiveAttributeNames;

	// Noms des variables natives
	StringVector svNativePredictiveAttributeNames;
};

inline KWClassDomain* KIInterpretationClassBuilder::GetInterpretationDomain() const
{
	return kwcdInterpretationDomain;
}

inline KWClass* KIInterpretationClassBuilder::GetInputClassifier()
{
	return kwcInputClassifier;
}

inline KWClass* KIInterpretationClassBuilder::GetInterpretationMainClass() const
{
	return kwcInterpretationMainClass;
}
