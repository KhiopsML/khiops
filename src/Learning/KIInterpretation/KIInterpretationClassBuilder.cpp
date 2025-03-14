// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KIInterpretationClassBuilder.h"

KIInterpretationClassBuilder::KIInterpretationClassBuilder(KIInterpretationSpec* spec)
{
	interpretationSpec = spec;
	kwcInputClassifier = NULL;
	kwcdInterpretationDomain = NULL;
	kwcInterpretationMainClass = NULL;
}

KIInterpretationClassBuilder::~KIInterpretationClassBuilder()
{
	CleanImport();
}

void KIInterpretationClassBuilder::CleanImport()
{
	svTargetValues.SetSize(0);

	// Nettoyage du tableau de noms de variables partitionnees
	svPartitionedPredictiveAttributeNames.SetSize(0);

	// Nettoyage du tableau de noms de variables natives
	svNativePredictiveAttributeNames.SetSize(0);
	if (kwcdInterpretationDomain != NULL)
	{
		kwcdInterpretationDomain->DeleteAllClasses();
		delete kwcdInterpretationDomain;
		kwcdInterpretationDomain = NULL;
		kwcInterpretationMainClass = NULL;
	}
	kwcInputClassifier = NULL;
}

boolean KIInterpretationClassBuilder::CreateInterpretationDomain(const KWClass* inputClassifier)
{
	boolean bOk = true;

	// Il doit y avoir un dico en entree
	if (inputClassifier == NULL)
		return false;

	if (kwcdInterpretationDomain != NULL)
	{
		kwcdInterpretationDomain->DeleteAllClasses();
		delete kwcdInterpretationDomain;
	}

	// Clone du domaine d'origine, afin de gerer les dictionnaires d'entree de type multi-table
	kwcdInterpretationDomain = inputClassifier->GetDomain()->CloneFromClass(inputClassifier);
	kwcdInterpretationDomain->SetName("InterpretationDomain");
	kwcInterpretationMainClass = kwcdInterpretationDomain->LookupClass(inputClassifier->GetName());
	assert(kwcInterpretationMainClass != NULL);
	kwcdInterpretationDomain->RenameClass(kwcInterpretationMainClass,
					      "Interpretation_" + kwcInterpretationMainClass->GetName());
	return bOk;
}

boolean KIInterpretationClassBuilder::UpdateInterpretationAttributes()
{
	boolean bOk = true;
	ALString sValue;
	int nIndex;
	KWTrainedClassifier trainedClassifier;
	ObjectArray oaInterpretationAttributes;
	KWAttribute* classifierAttribute;
	KWAttribute* predictionAttribute;
	KWAttribute* attribute;
	ALString sTargetValue;
	int i;

	// Import de l'interpreteur
	bOk = trainedClassifier.ImportPredictorClass(kwcInterpretationMainClass);
	if (bOk)
	{
		// Recherche des attributs necessaires dans le modele
		classifierAttribute = NULL;
		predictionAttribute = NULL;

		// Nettoyage des meta-data existantes
		attribute = kwcInterpretationMainClass->GetHeadAttribute();
		while (attribute != NULL)
		{
			if (attribute->GetStructureName() == "Classifier")
				classifierAttribute = attribute;
			else if (attribute->GetConstMetaData()->IsKeyPresent("Prediction"))
				predictionAttribute = attribute;
			else if (attribute->GetConstMetaData()->IsKeyPresent(INTERPRETATION_ATTRIBUTE_META_TAG))
				oaInterpretationAttributes.Add(attribute);
			else if (attribute->GetConstMetaData()->IsKeyPresent("ClassifierInterpretationVariable"))
				oaInterpretationAttributes.Add(attribute);
			else if (attribute->GetConstMetaData()->IsKeyPresent("ContributionVariable"))
				oaInterpretationAttributes.Add(attribute);
			else if (attribute->GetConstMetaData()->IsKeyPresent("ContributionVariableRank"))
				oaInterpretationAttributes.Add(attribute);
			else if (attribute->GetConstMetaData()->IsKeyPresent("ContributionPartRank"))
				oaInterpretationAttributes.Add(attribute);
			else if (attribute->GetConstMetaData()->IsKeyPresent("ContributionValueRank"))
				oaInterpretationAttributes.Add(attribute);
			else if (attribute->GetConstMetaData()->IsKeyPresent("ReinforcementFinalScoreRank"))
				oaInterpretationAttributes.Add(attribute);
			else if (attribute->GetConstMetaData()->IsKeyPresent("ReinforcementVariableRank"))
				oaInterpretationAttributes.Add(attribute);
			else if (attribute->GetConstMetaData()->IsKeyPresent("ReinforcementPartRank"))
				oaInterpretationAttributes.Add(attribute);
			else if (attribute->GetConstMetaData()->IsKeyPresent("ReinforcementClassChangeTagRank"))
				oaInterpretationAttributes.Add(attribute);
			else if (attribute->GetConstMetaData()->IsKeyPresent("ReinforcementInitialScore"))
				oaInterpretationAttributes.Add(attribute);
			kwcInterpretationMainClass->GetNextAttribute(attribute);
		}

		// Supression des eventuels attributs d'interpretation qui preexisteraient dans le dico,
		// afin de gerer correctement les mises a jour des parametres via IHM
		// (on recree systematiquement tous les attributs d'interpretation)
		for (i = 0; i < oaInterpretationAttributes.GetSize(); i++)
		{
			attribute = cast(KWAttribute*, oaInterpretationAttributes.GetAt(i));
			kwcInterpretationMainClass->DeleteAttribute(attribute->GetName());
		}

		// Compilation
		kwcdInterpretationDomain->Compile();

		// Preparation de la classe de l'interpreteur
		PrepareInterpretationClass();
		assert(classifierAttribute != NULL);
		assert(predictionAttribute != NULL);

		// On met toutes les variables en unused
		kwcInterpretationMainClass->SetAllAttributesUsed(false);

		// On met les attributs de cle en Used
		for (nIndex = 0; nIndex < kwcInterpretationMainClass->GetKeyAttributeNumber(); nIndex++)
		{
			attribute = kwcInterpretationMainClass->GetKeyAttributeAt(nIndex);
			check(attribute);
			attribute->SetUsed(true);
			attribute->SetLoaded(true);
		}

		// Creation des attributs de contribution
		if (interpretationSpec->GetWhyAttributesNumber() > 0)
		{
			if (interpretationSpec->GetWhyClass() == KIInterpretationSpec::ALL_CLASSES_LABEL)
			{
				sValue = GetWhyTypeShortLabel(interpretationSpec->GetWhyType());
				kwcInterpretationMainClass->GetMetaData()->SetStringValueAt("ContributionMethod",
											    sValue);

				if (interpretationSpec->GetMaxAttributesNumber() ==
				    interpretationSpec->GetWhyAttributesNumber())
				{
					kwcInterpretationMainClass->GetMetaData()->SetNoValueAt(
					    KIInterpretationSpec::ALL_CLASSES_LABEL);
				}
				else
				{
					kwcInterpretationMainClass->GetMetaData()->SetDoubleValueAt(
					    "VariableNumber", interpretationSpec->GetWhyAttributesNumber());
				}
				// Parcours de toutes les classes
				for (nIndex = 0; nIndex < svTargetValues.GetSize(); nIndex++)
				{
					// Extraction de la valeur de la classe cible
					sTargetValue = svTargetValues.GetAt(nIndex).GetValue();
					CreateContributionAttributesForClass(kwcInterpretationMainClass, sTargetValue,
									     classifierAttribute, predictionAttribute,
									     &trainedClassifier);
				}
			}
			else
			{
				sValue = GetWhyTypeShortLabel(interpretationSpec->GetWhyType());
				kwcInterpretationMainClass->GetMetaData()->SetStringValueAt("ContributionMethod",
											    sValue);
				CreateContributionAttributesForClass(
				    kwcInterpretationMainClass, interpretationSpec->GetWhyClass(), classifierAttribute,
				    predictionAttribute, &trainedClassifier);
			}
		}

		// Creation des attributs de renforcement
		if (bOk)
		{
			if (interpretationSpec->GetHowAttributesNumber() > 0 and
			    interpretationSpec->GetHowClass() != NO_VALUE_LABEL)
			{
				CreateReinforcementAttributesForClass(
				    kwcInterpretationMainClass, interpretationSpec->GetHowClass(), classifierAttribute,
				    predictionAttribute, &trainedClassifier);
				kwcInterpretationMainClass->GetMetaData()->SetStringValueAt(
				    "ReinforcementClass", interpretationSpec->GetHowClass());
			}
		}
		if (bOk)
		{
			// Ajout d'un metadata permettant de reconnaitre qu'il s'agit d'un dico d'interpretation
			kwcInterpretationMainClass->GetMetaData()->SetNoValueAt("InterpreterDictionary");
			kwcdInterpretationDomain->Compile();
		}
	}
	return bOk;
}

void KIInterpretationClassBuilder::CreateContributionAttributesForClass(KWClass* kwcInterpretation,
									const ALString& sTargetClass,
									const KWAttribute* classifierAttribute,
									const KWAttribute* predictionAttribute,
									const KWTrainedClassifier* trainedClassifier)
{
	KWAttribute* scoreInterpretationAttribute;
	KWAttribute* contributionVariableNameAttribute;
	KWAttribute* contributionVariablePartAttribute;
	KWAttribute* contributionVariableValueAttribute;
	int nAttributeIndex;

	// Creation de l'attribut gerant les contribution
	scoreInterpretationAttribute =
	    CreateScoreContributionAttribute(kwcInterpretation, sTargetClass, classifierAttribute, predictionAttribute);
	scoreInterpretationAttribute->CompleteTypeInfo(kwcInterpretation);
	kwcInterpretation->InsertAttribute(scoreInterpretationAttribute);
	scoreInterpretationAttribute->SetUsed(false);

	// Parcours des variables explicatives a concurrence du nombre max de variables recherchees dans le pourquoi
	for (nAttributeIndex = 0; nAttributeIndex < interpretationSpec->GetWhyAttributesNumber(); nAttributeIndex++)
	{
		if (not interpretationSpec->IsExpertMode())
		{
			// Nom de la variable de contribution
			contributionVariableNameAttribute = CreateContributionNameAtAttribute(
			    kwcInterpretation, sTargetClass, scoreInterpretationAttribute, nAttributeIndex + 1);
			contributionVariableNameAttribute->CompleteTypeInfo(kwcInterpretation);
			kwcInterpretation->InsertAttribute(contributionVariableNameAttribute);

			// Partie de la variable de contribution
			contributionVariablePartAttribute = CreateContributionPartAtAttribute(
			    kwcInterpretation, sTargetClass, scoreInterpretationAttribute, nAttributeIndex + 1);
			contributionVariablePartAttribute->CompleteTypeInfo(kwcInterpretation);
			kwcInterpretation->InsertAttribute(contributionVariablePartAttribute);
		}

		// Valeur de contribution
		contributionVariableValueAttribute = CreateContributionValueAtAttribute(
		    kwcInterpretation, sTargetClass, scoreInterpretationAttribute, nAttributeIndex + 1);
		contributionVariableValueAttribute->CompleteTypeInfo(kwcInterpretation);
		kwcInterpretation->InsertAttribute(contributionVariableValueAttribute);
	}
	assert(kwcInterpretation->Check());
}

KWAttribute* KIInterpretationClassBuilder::CreateScoreContributionAttribute(KWClass* kwcInterpretation,
									    const ALString& sTargetClass,
									    const KWAttribute* classifierAttribute,
									    const KWAttribute* predictionAttribute)
{
	KWAttribute* contributionClassAttribute;
	KWDerivationRule* rule;
	ALString sValue;

	// Creation de la regle de derivation
	rule = new KIDRClassifierContribution;
	rule->SetClassName(kwcInterpretation->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(classifierAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetSecondOperand()->SetAttributeName(predictionAttribute->GetName());
	rule->GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetOperandAt(2)->SetSymbolConstant((Symbol)sTargetClass);
	rule->GetOperandAt(3)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetOperandAt(3)->SetSymbolConstant(Symbol(GetWhyTypeShortLabel(interpretationSpec->GetWhyType())));
	rule->GetOperandAt(4)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetOperandAt(4)->SetSymbolConstant(interpretationSpec->GetSortWhyResults() ? "sorted" : "unsorted");
	assert(rule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	contributionClassAttribute = new KWAttribute;
	sValue = GetWhyTypeShortLabel(interpretationSpec->GetWhyType());
	contributionClassAttribute->SetName(kwcInterpretation->BuildAttributeName(sValue + "_" + sTargetClass));
	contributionClassAttribute->SetType(KWType::Continuous);
	contributionClassAttribute->GetMetaData()->SetNoValueAt(INTERPRETATION_ATTRIBUTE_META_TAG);
	contributionClassAttribute->SetDerivationRule(rule);
	return contributionClassAttribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateContributionValueAtAttribute(
    KWClass* kwcInterpretation, const ALString& sTargetClass, const KWAttribute* scoreInterpretationAttribute,
    int nIndex)
{
	KWDerivationRule* rule;
	KWAttribute* contributionClassAttribute;
	ALString soNativeAttributeName;
	ALString sValue;
	ALString sTmp;

	// Creation de la regle de derivation
	rule = new KIDRContributionValueAt;
	rule->SetClassName(kwcInterpretation->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nIndex);
	assert(rule->Check());

	// Attribut natif de la regle
	soNativeAttributeName = svNativePredictiveAttributeNames.GetAt(nIndex - 1);

	// Creation de l'attribut, et affectation de la regle de derivation
	contributionClassAttribute = new KWAttribute;
	sValue = GetWhyTypeShortLabel(interpretationSpec->GetWhyType());
	contributionClassAttribute->SetName(
	    kwcInterpretation->BuildAttributeName(sValue + "_" + sTargetClass + "_" + soNativeAttributeName));
	contributionClassAttribute->SetType(KWType::Continuous);
	contributionClassAttribute->SetDerivationRule(rule);
	if (interpretationSpec->GetWhyAttributesNumber() == svNativePredictiveAttributeNames.GetSize() and
	    interpretationSpec->GetSortWhyResults() == false)
	{
		contributionClassAttribute->GetMetaData()->SetStringValueAt("ContributionVariable",
									    soNativeAttributeName);
		contributionClassAttribute->GetMetaData()->SetStringValueAt("Target", sTargetClass);
	}
	else
	{
		contributionClassAttribute->GetMetaData()->SetDoubleValueAt("ContributionValueRank", nIndex);
		contributionClassAttribute->GetMetaData()->SetStringValueAt("Target", sTargetClass);
		contributionClassAttribute->SetName(kwcInterpretation->BuildAttributeName(
		    sTmp + "ContributionValue" + IntToString(nIndex) + "_" + sTargetClass));
	}
	return contributionClassAttribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateContributionNameAtAttribute(
    KWClass* kwcInterpretation, const ALString& sTargetClass, const KWAttribute* scoreInterpretationAttribute,
    int nIndex)
{
	KWDerivationRule* rule;
	KWAttribute* attribute;
	ALString sTmp;

	// Creation de la regle de derivation
	rule = new KIDRContributionNameAt;
	rule->SetClassName(kwcInterpretation->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nIndex);
	assert(rule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	attribute = new KWAttribute;
	attribute->SetName(kwcInterpretation->BuildAttributeName(sTmp + "ContributionVariable" + IntToString(nIndex) +
								 "_" + sTargetClass));
	attribute->SetType(KWType::Symbol);

	attribute->GetMetaData()->SetDoubleValueAt("ContributionVariableRank", nIndex);
	attribute->GetMetaData()->SetStringValueAt("Target", sTargetClass);
	if (interpretationSpec->GetWhyAttributesNumber() == svNativePredictiveAttributeNames.GetSize() and
	    interpretationSpec->GetSortWhyResults() == false)
	{
		attribute->GetMetaData()->SetDoubleValueAt("ContributionVariableRank", nIndex);
		attribute->GetMetaData()->SetStringValueAt("Target", sTargetClass);
	}
	attribute->SetDerivationRule(rule);

	return attribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateContributionPartAtAttribute(
    KWClass* kwcInterpretation, const ALString& sTargetClass, const KWAttribute* scoreInterpretationAttribute,
    int nIndex)
{
	KWDerivationRule* rule;
	KWAttribute* attribute;
	ALString sTmp;

	// Creation de la regle de derivation
	rule = new KIDRContributionPartitionAt;
	rule->SetClassName(kwcInterpretation->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nIndex);
	assert(rule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	attribute = new KWAttribute;
	attribute->SetName(kwcInterpretation->BuildAttributeName(sTmp + "ContributionPart" + IntToString(nIndex) + "_" +
								 sTargetClass));
	attribute->SetType(KWType::Symbol);
	attribute->GetMetaData()->SetDoubleValueAt("ContributionPartRank", nIndex);
	attribute->GetMetaData()->SetStringValueAt("Target", sTargetClass);
	if (interpretationSpec->GetWhyAttributesNumber() == svNativePredictiveAttributeNames.GetSize() and
	    interpretationSpec->GetSortWhyResults() == false)
	{
		attribute->GetMetaData()->SetDoubleValueAt("ContributionPartRank", nIndex);
		attribute->GetMetaData()->SetStringValueAt("Target", sTargetClass);
	}
	attribute->SetDerivationRule(rule);
	return attribute;
}

void KIInterpretationClassBuilder::CreateReinforcementAttributesForClass(KWClass* kwcInterpretation,
									 const ALString& sTargetClass,
									 const KWAttribute* classifierAttribute,
									 const KWAttribute* predictionAttribute,
									 const KWTrainedClassifier* trainedClassifier)
{
	int nAttributeIndex;
	int nReinforcementAttributesMaxNumber;
	KWAttribute* scoreInterpretationAttribute;
	KWAttribute* initialScoreValueAttribute;
	KWAttribute* reinforcemenVariableNameAttribute;
	KWAttribute* reinforcemenVariablePartAttribute;
	KWAttribute* finalScoreValueAttribute;
	KWAttribute* classChangeAttribute;

	require(ComputeReinforcementAttributesMaxNumber() > 0);

	// Nombre max de variables levier qu'on souhaite utiliser
	nReinforcementAttributesMaxNumber = ComputeReinforcementAttributesMaxNumber();

	// Attribut pour le renforcement
	scoreInterpretationAttribute = CreateScoreReinforcementAttribute(kwcInterpretation, sTargetClass,
									 classifierAttribute, predictionAttribute);
	scoreInterpretationAttribute->CompleteTypeInfo(kwcInterpretation);
	kwcInterpretation->InsertAttribute(scoreInterpretationAttribute);

	// Attribut pour le score initial
	initialScoreValueAttribute =
	    CreateReinforcementInitialScoreAttribute(kwcInterpretation, sTargetClass, scoreInterpretationAttribute);
	initialScoreValueAttribute->CompleteTypeInfo(kwcInterpretation);
	kwcInterpretation->InsertAttribute(initialScoreValueAttribute);

	// Parcours des variables explicatives a concurrence du nombre max de variables leviers, precedemment calcule
	for (nAttributeIndex = 0; nAttributeIndex < nReinforcementAttributesMaxNumber; nAttributeIndex++)
	{
		// Attribut pourt la variable de renforcement
		reinforcemenVariableNameAttribute = CreateReinforcementNameAtAttribute(
		    kwcInterpretation, sTargetClass, scoreInterpretationAttribute, nAttributeIndex + 1);
		reinforcemenVariableNameAttribute->CompleteTypeInfo(kwcInterpretation);
		kwcInterpretation->InsertAttribute(reinforcemenVariableNameAttribute);

		// Attribut pour la partie de variable de renforcement
		reinforcemenVariablePartAttribute = CreateReinforcementPartAtAttribute(
		    kwcInterpretation, sTargetClass, scoreInterpretationAttribute, nAttributeIndex + 1);
		reinforcemenVariablePartAttribute->CompleteTypeInfo(kwcInterpretation);
		kwcInterpretation->InsertAttribute(reinforcemenVariablePartAttribute);

		// Attribut pour le score final apres renforcement
		finalScoreValueAttribute = CreateReinforcementFinalScoreAtAttribute(
		    kwcInterpretation, sTargetClass, scoreInterpretationAttribute, nAttributeIndex + 1);
		finalScoreValueAttribute->CompleteTypeInfo(kwcInterpretation);
		kwcInterpretation->InsertAttribute(finalScoreValueAttribute);

		// Attribut pour l'indicateur de changement de classe
		classChangeAttribute = CreateReinforcementClassChangeAtAttribute(
		    kwcInterpretation, sTargetClass, scoreInterpretationAttribute, nAttributeIndex + 1);
		classChangeAttribute->CompleteTypeInfo(kwcInterpretation);
		kwcInterpretation->InsertAttribute(classChangeAttribute);
	}
	ensure(kwcInterpretation->Check());
}

KWAttribute* KIInterpretationClassBuilder::CreateScoreReinforcementAttribute(KWClass* kwcInterpretation,
									     const ALString& sTargetClass,
									     const KWAttribute* classifierAttribute,
									     const KWAttribute* predictionAttribute)
{
	KWDerivationRule* rule;
	KWAttribute* attribute;

	// Creation de la regle de derivation
	rule = new KIDRClassifierReinforcement;
	rule->SetClassName(kwcInterpretation->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(classifierAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetSecondOperand()->SetAttributeName(predictionAttribute->GetName());
	rule->GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetOperandAt(2)->SetSymbolConstant((Symbol)sTargetClass);
	assert(rule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	attribute = new KWAttribute;
	attribute->SetName(kwcInterpretation->BuildAttributeName("Reinforcement_" + sTargetClass));
	attribute->SetType(KWType::Continuous);
	attribute->GetMetaData()->SetNoValueAt(INTERPRETATION_ATTRIBUTE_META_TAG);
	attribute->SetUsed(false);
	attribute->SetDerivationRule(rule);
	return attribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateReinforcementInitialScoreAttribute(
    KWClass* kwcInterpretation, const ALString& sTargetClass, const KWAttribute* scoreInterpretationAttribute)
{
	KWDerivationRule* rule;
	KWAttribute* attribute;

	// Creation de la regle de derivation
	rule = new KIDRReinforcementInitialScore;
	rule->SetClassName(kwcInterpretation->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());

	assert(rule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	attribute = new KWAttribute;
	attribute->SetName(kwcInterpretation->BuildAttributeName("ReinforcementInitialScore_" + sTargetClass));
	attribute->SetType(KWType::Continuous);
	attribute->SetDerivationRule(rule);
	attribute->GetMetaData()->SetNoValueAt("ReinforcementInitialScore");
	attribute->GetMetaData()->SetStringValueAt("Target", sTargetClass);
	return attribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateReinforcementFinalScoreAtAttribute(
    KWClass* kwcInterpretation, const ALString& sTargetClass, const KWAttribute* scoreInterpretationAttribute,
    int nIndex)
{

	KWDerivationRule* rule;
	KWAttribute* attribute;

	// Creation de la regle de derivation
	rule = new KIDRReinforcementFinalScoreAt;
	rule->SetClassName(kwcInterpretation->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nIndex);
	assert(rule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	attribute = new KWAttribute;
	attribute->SetName(kwcInterpretation->BuildAttributeName("ReinforcementFinalScore" +
								 ALString(IntToString(nIndex)) + "_" + sTargetClass));
	attribute->SetType(KWType::Continuous);
	attribute->SetDerivationRule(rule);
	attribute->GetMetaData()->SetDoubleValueAt("ReinforcementFinalScoreRank", nIndex);
	attribute->GetMetaData()->SetStringValueAt("Target", sTargetClass);
	return attribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateReinforcementNameAtAttribute(
    KWClass* kwcInterpretation, const ALString& sTargetClass, const KWAttribute* scoreInterpretationAttribute,
    int nIndex)
{
	KWDerivationRule* rule;
	KWAttribute* attribute;

	// Creation de la regle de derivation
	rule = new KIDRReinforcementNameAt;
	rule->SetClassName(kwcInterpretation->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nIndex);
	assert(rule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	attribute = new KWAttribute;
	attribute->SetName(kwcInterpretation->BuildAttributeName("ReinforcementVariable_" +
								 ALString(IntToString(nIndex)) + "_" + sTargetClass));
	attribute->SetType(KWType::Symbol);
	attribute->GetMetaData()->SetDoubleValueAt("ReinforcementVariableRank", nIndex);
	attribute->GetMetaData()->SetStringValueAt("Target", sTargetClass);
	attribute->SetDerivationRule(rule);
	return attribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateReinforcementPartAtAttribute(
    KWClass* kwcInterpretation, const ALString& sTargetClass, const KWAttribute* scoreInterpretationAttribute,
    int nIndex)
{
	KWDerivationRule* rule;
	KWAttribute* attribute;

	// Creation de la regle de derivation
	rule = new KIDRReinforcementPartitionAt;
	rule->SetClassName(kwcInterpretation->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nIndex);
	assert(rule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	attribute = new KWAttribute;
	attribute->SetName(kwcInterpretation->BuildAttributeName("ReinforcementPart" + ALString(IntToString(nIndex)) +
								 "_" + sTargetClass));
	attribute->SetType(KWType::Symbol);
	attribute->GetMetaData()->SetDoubleValueAt("ReinforcementPartRank", nIndex);
	attribute->GetMetaData()->SetStringValueAt("Target", sTargetClass);
	attribute->SetDerivationRule(rule);
	return attribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateReinforcementClassChangeAtAttribute(
    KWClass* kwcInterpretation, const ALString& sTargetClass, const KWAttribute* scoreInterpretationAttribute,
    int nIndex)
{
	KWDerivationRule* rule;
	KWAttribute* attribute;

	// Creation de la regle de derivation
	rule = new KIDRReinforcementClassChangeTagAt;
	rule->SetClassName(kwcInterpretation->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nIndex);
	assert(rule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	attribute = new KWAttribute;
	attribute->SetName(kwcInterpretation->BuildAttributeName("ReinforcementClassChangeTag" +
								 ALString(IntToString(nIndex)) + "_" + sTargetClass));
	attribute->SetType(KWType::Continuous);
	attribute->SetDerivationRule(rule);
	attribute->GetMetaData()->SetDoubleValueAt("ReinforcementClassChangeTagRank", nIndex);
	attribute->GetMetaData()->SetStringValueAt("Target", sTargetClass);
	return attribute;
}

int KIInterpretationClassBuilder::ComputeReinforcementAttributesMaxNumber()
{
	KWAttribute* attribute;
	int result;
	int nbSelected;

	require(interpretationSpec != NULL);
	require(kwcInterpretationMainClass != NULL);

	result = interpretationSpec->GetHowAttributesNumber();
	nbSelected = 0;

	attribute = kwcInterpretationMainClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		if (attribute->GetConstMetaData()->GetStringValueAt(
			KIInterpretationClassBuilder::LEVER_ATTRIBUTE_META_TAG) == "true")
			nbSelected++;

		kwcInterpretationMainClass->GetNextAttribute(attribute);
	}

	if (nbSelected < result)
		result = nbSelected;

	return result;
}

const SymbolVector* KIInterpretationClassBuilder::GetTargetValues() const
{
	return &svTargetValues;
}

StringVector* KIInterpretationClassBuilder::GetPredictiveAttributeNamesArray()
{
	return &svPartitionedPredictiveAttributeNames;
}

boolean KIInterpretationClassBuilder::TestGroupTargetValues(KWClass* inputClassifier)
{
	boolean bOk = true;
	ALString sValue;
	KWDerivationRule* derivationrule;
	KWDerivationRuleOperand* operand;
	KWAttribute* attribute;

	if (bOk)
	{
		attribute = inputClassifier->GetHeadAttribute();

		while (attribute != NULL and bOk)
		{
			if (attribute->GetStructureName() == "DataGrid" and
			    attribute->GetConstMetaData()->IsKeyPresent("Level"))
			{

				if (bOk)
				{
					derivationrule = attribute->GetDerivationRule();
					operand = derivationrule->GetSecondOperand();
					if (operand->GetDerivationRule()->GetName() == "ValueGroups")
						bOk = false;
				}
			}
			inputClassifier->GetNextAttribute(attribute);
		}
	}
	return bOk;
}

boolean KIInterpretationClassBuilder::ImportClassifier(KWClass* inputClassifier)
{
	KWTrainedClassifier* trainedClassifier = NULL;
	KWTrainedRegressor* trainedRegressor = NULL;
	KWAttribute* predictionAttribute;
	KWAttribute* attribute;
	KWAttribute* nativeAttribute;
	KWDRNBClassifier* classifierRule;
	KWDRNBClassifier referenceNBRule;
	KWDRSNBClassifier referenceSNBRule;
	boolean bIsClassifier;
	boolean bIsClassifierNaiveBayes;
	boolean bOk;
	ObjectArray oaClasses;
	ALString sAttributeName;
	ALString sAttributePredictorName;
	int nAttributeNaiveBayes;
	int nAttributeSelectiveNaiveBayes;
	int nAttributeIndex;
	const KWDRDataGridStats refDataGridStatsRule;
	const KWDRDataGridStatsBlock refDataGridStatsBlockRule;
	int i;

	// Reinitialisation d'une precedente importation
	CleanImport();

	bIsClassifier = false;
	bIsClassifierNaiveBayes = false;
	bOk = true;

	if (inputClassifier == NULL or not inputClassifier->Check() or
	    not inputClassifier->GetConstMetaData()->IsKeyPresent("PredictorType") or
	    inputClassifier->GetConstMetaData()->IsKeyPresent("InterpreterDictionary"))
		bOk = false;

	if (bOk)
	{
		// Test d'importation de ce dictionnaire dans un classifieur
		trainedClassifier = new KWTrainedClassifier;
		bIsClassifier = trainedClassifier->ImportPredictorClass(inputClassifier);

		// On ne gere pas les grilles
		if (bIsClassifier and trainedClassifier->GetMetaDataPredictorLabel(inputClassifier) == "Data Grid")
			bIsClassifier = false; // pas gere (pour l'instant ?)

		// On ne gere pas actuellement les classifieur avec groupement de la cible
		if (bIsClassifier and not TestGroupTargetValues(inputClassifier))
		{
			Global::AddWarning("Interpret model", "",
					   "Interpretation dictionary not yet implemented "
					   "for classifiers with grouped target values");
			bIsClassifier = false;
		}

		// Message d'erreur specifique pour le cas des regresseurs, non geres actuellement
		if (not bIsClassifier)
		{
			trainedRegressor = new KWTrainedRegressor;
			if (trainedRegressor->ImportPredictorClass(inputClassifier))
			{
				Global::AddWarning("Interpret model", "",
						   "Interpretation dictionary not yet implemented for regressors");
			}
			delete trainedRegressor;
		}

		// Cas ou le dictionnaire est bien celui d'un classifieur pouvant etre interprete
		if (bIsClassifier)
		{
			// Initalisation des indicateurs de presence d'un NB ou d'un SNB
			nAttributeNaiveBayes = 0;
			nAttributeSelectiveNaiveBayes = 0;

			for (i = 0; i < trainedClassifier->GetTargetValueNumber(); i++)
				svTargetValues.Add(trainedClassifier->GetTargetValueAt(i));

			// Extraction de l'attribut de prediction
			predictionAttribute = trainedClassifier->GetPredictionAttribute();

			if (predictionAttribute->GetDerivationRule() == NULL)
				bIsClassifier = false;
			else if (predictionAttribute->GetDerivationRule()->GetOperandNumber() == 0)
				bIsClassifier = false;

			// Cas ou l'attribut de prediction n'est pas un attribut de type Structure
			if (bIsClassifier and
			    (predictionAttribute->GetDerivationRule()->GetFirstOperand()->GetType() !=
				 KWType::Structure ||
			     predictionAttribute->GetDerivationRule()->GetFirstOperand()->GetOrigin() !=
				 KWDerivationRuleOperand::OriginAttribute))
				bIsClassifier = false;

			if (bIsClassifier)
			{
				// Extraction de l'attribute Structure qui decrit le classifier
				// Cet attribut doit avoir une regle de derivation dont le nom est celui
				// d'une regle NB ou SNB
				attribute = inputClassifier->LookupAttribute(
				    predictionAttribute->GetDerivationRule()->GetFirstOperand()->GetAttributeName());

				assert(attribute != NULL);

				// Cas d'un attribut avec RDD
				if (attribute->GetDerivationRule() != NULL)
				{
					if (attribute->GetDerivationRule()->GetName() == referenceNBRule.GetName())
					{
						nAttributeNaiveBayes++;
						sAttributePredictorName = attribute->GetName();
					}
					if (attribute->GetDerivationRule()->GetName() == referenceSNBRule.GetName())
					{
						nAttributeSelectiveNaiveBayes++;
						sAttributePredictorName = attribute->GetName();
					}
				}
			}

			// Cas d'un predicteur Bayesien naif NB ou SNB
			if (nAttributeNaiveBayes + nAttributeSelectiveNaiveBayes == 1)
			{
				bIsClassifierNaiveBayes = true;

				// Extraction de la regle de derivation de l'attribut classifieur
				classifierRule = cast(
				    KWDRNBClassifier*,
				    inputClassifier->LookupAttribute(sAttributePredictorName)->GetDerivationRule());

				// Initilisation des listes d'attributs native et calcule du NB et SNB
				classifierRule->ExportAttributeNames(&svPartitionedPredictiveAttributeNames,
								     &svNativePredictiveAttributeNames);

				if (svNativePredictiveAttributeNames.GetSize() == 0)
					bOk = false;

				// Parcours des attributs explicatifs recenses
				for (nAttributeIndex = 0;
				     nAttributeIndex < svPartitionedPredictiveAttributeNames.GetSize();
				     nAttributeIndex++)
				{
					// Extraction de l'attribut explicatif courant
					attribute = inputClassifier->LookupAttribute(
					    svPartitionedPredictiveAttributeNames.GetAt(nAttributeIndex));

					// Cas ou l'attribut partitionne n'est pas present dans le dictionnaire
					if (attribute == NULL)
						bOk = false;

					// Extraction de l'attribut natif
					nativeAttribute = inputClassifier->LookupAttribute(
					    svNativePredictiveAttributeNames.GetAt(nAttributeIndex));

					// Cas ou l'attribut natif n'est pas present dans le dictionnaire
					if (nativeAttribute == NULL)
						bOk = false;
				}
			}
		}
	}

	bIsClassifierNaiveBayes = bIsClassifierNaiveBayes and bOk;
	kwcInputClassifier = (bIsClassifierNaiveBayes ? inputClassifier : NULL);
	if (kwcInputClassifier)
	{
		interpretationSpec->SetMaxAttributesNumber(GetPredictiveAttributeNamesArray()->GetSize());

		// Ne pas ecraser un precedent choix fait via l'IHM, lors d'une nouvelle importation
		interpretationSpec->SetHowAttributesNumber(GetPredictiveAttributeNamesArray()->GetSize());
		interpretationSpec->SetWhyAttributesNumber(GetPredictiveAttributeNamesArray()->GetSize());
		assert(trainedClassifier->GetTargetAttribute() != NULL);

		// L'import a reussi, on peut donc creer un domaine specifique d'interpretation, et le(s) dico(s) d'interpretation qu'il doit contenir
		bOk = CreateInterpretationDomain(inputClassifier);
		if (bOk)
			PrepareInterpretationClass();
	}
	if (trainedClassifier != NULL)
		delete trainedClassifier;
	bIsClassifierNaiveBayes = bIsClassifierNaiveBayes and bOk;
	return bIsClassifierNaiveBayes;
}

void KIInterpretationClassBuilder::PrepareInterpretationClass()
{
	KWAttribute* nativeAttribute;
	KWAttribute* attribute;
	KWAttribute* inputAttribute;
	ALString sNativeVariableName;
	int nAttributeIndex;

	require(kwcInterpretationMainClass != NULL);
	require(kwcdInterpretationDomain != NULL);

	if (svPartitionedPredictiveAttributeNames.GetSize() == 0)
		return;

	// Taggage des attributs explicatifs contribuant au predicteur, et pouvant etre utilisees eventuellement comme leviers
	for (nAttributeIndex = 0; nAttributeIndex < svPartitionedPredictiveAttributeNames.GetSize(); nAttributeIndex++)
	{
		sNativeVariableName = svNativePredictiveAttributeNames.GetAt(nAttributeIndex);

		// Extraction de l'attribut natif
		nativeAttribute = kwcInterpretationMainClass->LookupAttribute(sNativeVariableName);
		assert(nativeAttribute != NULL);
		if (not nativeAttribute->GetConstMetaData()->IsKeyPresent(LEVER_ATTRIBUTE_META_TAG))
		{
			nativeAttribute->GetMetaData()->SetStringValueAt(
			    LEVER_ATTRIBUTE_META_TAG,
			    "true"); // par defaut : toutes les variables natives pourront etre utilisees comme levier
		}
	}

	// Synchronisation des proprietes Used et Loaded du dico de transfert a partir de la selection faite dans le classifieur d'entree
	// on parcourt cette fois-ci tous les attributs
	attribute = kwcInterpretationMainClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		inputAttribute = GetInputClassifier()->LookupAttribute(attribute->GetName());

		if (inputAttribute != NULL)
		{
			attribute->SetUsed(inputAttribute->GetUsed());
			attribute->SetLoaded(attribute->GetUsed());
		}
		kwcInterpretationMainClass->GetNextAttribute(attribute);
	}
	kwcdInterpretationDomain->Compile();
}

const ALString& KIInterpretationClassBuilder::GetWhyTypeShortLabel(const ALString& asWhyTypeLongLabel)
{
	if (asWhyTypeLongLabel == "Shapley")
		return SHAPLEY_LABEL;
	else
		return UNDEFINED_LABEL;
}

const ALString KIInterpretationClassBuilder::LEVER_ATTRIBUTE_META_TAG = "LeverVariable";
const ALString KIInterpretationClassBuilder::INTERPRETATION_ATTRIBUTE_META_TAG = "ClassifierInterpretationVariable";
const ALString KIInterpretationClassBuilder::NO_VALUE_LABEL = "";
