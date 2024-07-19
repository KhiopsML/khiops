// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KIInterpretationDictionary.h"
#include "KIInterpretationSpec.h"
#include "KIDRPredictor.h"

#include "KWTrainedPredictor.h"
#include "KWDRNBPredictor.h"

KIInterpretationDictionary::KIInterpretationDictionary(KIInterpretationSpec* spec)
{
	interpretationSpec = spec;
	kwcInputClassifier = NULL;
	kwcdInterpretationDomain = NULL;
	kwcInterpretationRootClass = NULL;
}

KIInterpretationDictionary::~KIInterpretationDictionary()
{
	CleanImport();
}

void KIInterpretationDictionary::CleanImport()
{
	sTargetValues.SetSize(0);

	// Nettoyage du tableau de noms de variables partitionnees
	oaPartitionedPredictiveAttributeNames.DeleteAll();
	oaPartitionedPredictiveAttributeNames.SetSize(0);

	// Nettoyage du tableau de noms de variables natives
	oaNativePredictiveAttributeNames.DeleteAll();
	oaNativePredictiveAttributeNames.SetSize(0);

	if (kwcdInterpretationDomain != NULL)
	{
		kwcdInterpretationDomain->DeleteAllClasses();
		delete kwcdInterpretationDomain;
		kwcdInterpretationDomain = NULL;
		kwcInterpretationRootClass = NULL;
	}
	kwcInputClassifier = NULL;
}

boolean KIInterpretationDictionary::CreateInterpretationDomain(const KWClass* inputClassifier)
{
	boolean bOk = true;

	if (inputClassifier == NULL)
		return false; // pas encore de dico d'input choisi par l'utilisateur

	if (kwcdInterpretationDomain != NULL)
	{
		kwcdInterpretationDomain->DeleteAllClasses();
		delete kwcdInterpretationDomain;
	}

	// cloner le domaine d'origine, afin de gerer les dictionnaires d'entree de type multi-table
	kwcdInterpretationDomain = inputClassifier->GetDomain()->CloneFromClass(inputClassifier);
	kwcdInterpretationDomain->SetName("InterpretationDomain");
	kwcInterpretationRootClass = kwcdInterpretationDomain->LookupClass(inputClassifier->GetName());
	assert(kwcInterpretationRootClass != NULL);
	kwcdInterpretationDomain->RenameClass(kwcInterpretationRootClass,
					      ALString("Interpretation_") + kwcInterpretationRootClass->GetName());

	return bOk;
}

boolean KIInterpretationDictionary::UpdateInterpretationAttributes()
{
	boolean bOk = true;
	ALString sValue;
	KWTrainedClassifier* trainedClassifier = new KWTrainedClassifier;

	bOk = trainedClassifier->ImportPredictorClass(kwcInterpretationRootClass);

	if (bOk)
	{
		ObjectArray* interpretationAttributes = new ObjectArray;

		// recherche des attributs necessaires dans le modele
		KWAttribute* classifierAttribute = NULL;
		KWAttribute* predictionAttribute = NULL;

		KWAttribute* attribute = kwcInterpretationRootClass->GetHeadAttribute();

		while (attribute != NULL)
		{
			if (attribute->GetStructureName() == "Classifier")
				classifierAttribute = attribute;
			else if (attribute->GetConstMetaData()->IsKeyPresent("Prediction"))
				predictionAttribute = attribute;
			else if (attribute->GetConstMetaData()->IsKeyPresent(INTERPRETATION_ATTRIBUTE_META_TAG))
				interpretationAttributes->Add(attribute);

			kwcInterpretationRootClass->GetNextAttribute(attribute);
		}

		// supprimer les eventuels attributs d'interpretation qui preexisteraient dans le dico, afin de gerer correctement les mises a jour des parametres via IHM (on recree systematiquement tous les attributs d'interpretation)
		for (int i = 0; i < interpretationAttributes->GetSize(); i++)
		{
			attribute = cast(KWAttribute*, interpretationAttributes->GetAt(i));
			kwcInterpretationRootClass->DeleteAttribute(attribute->GetName());
		}
		kwcdInterpretationDomain->Compile();

		delete interpretationAttributes;

		PrepareInterpretationClass();

		assert(classifierAttribute != NULL);
		assert(predictionAttribute != NULL);

		// creation des attributs de contribution
		if (interpretationSpec->GetWhyAttributesNumber() > 0)
		{
			if (interpretationSpec->GetWhyClass() == KIInterpretationSpec::ALL_CLASSES_LABEL)
			{
				sValue = ALString(GetWhyTypeShortLabel(interpretationSpec->GetWhyType()).GetValue()) +
					 ALString("_") + KIInterpretationSpec::ALL_CLASSES_LABEL;
				kwcInterpretationRootClass->GetMetaData()->SetStringValueAt("ContributionMethod",
											    sValue);

				// Parcours de toutes les classes
				for (int nIndex = 0; nIndex < sTargetValues.GetSize(); nIndex++)
				{
					// Extraction de la valeur de la classe cible
					const ALString sTargetValue = sTargetValues.GetAt(nIndex).GetValue();
					bOk = CreateContributionAttributesForClass(
					    sTargetValue, kwcInterpretationRootClass, classifierAttribute,
					    predictionAttribute, trainedClassifier);

					if (not bOk)
						break;
				}
			}
			else
			{
				sValue = ALString(GetWhyTypeShortLabel(interpretationSpec->GetWhyType()).GetValue()) +
					 ALString("_") + interpretationSpec->GetWhyClass();
				kwcInterpretationRootClass->GetMetaData()->SetStringValueAt("ContributionMethod",
											    sValue);
				bOk = CreateContributionAttributesForClass(
				    interpretationSpec->GetWhyClass(), kwcInterpretationRootClass, classifierAttribute,
				    predictionAttribute, trainedClassifier);
				sValue = ALString(GetWhyTypeShortLabel(interpretationSpec->GetWhyType()).GetValue()) +
					 ALString("_") + interpretationSpec->GetWhyClass();
				kwcInterpretationRootClass->GetMetaData()->SetStringValueAt("ContributionMethod",
											    sValue);
			}
		}

		// creation des attributs de renforcement
		if (bOk)
		{
			if (interpretationSpec->GetHowAttributesNumber() > 0 and
			    interpretationSpec->GetHowClass() != NO_VALUE_LABEL)
			{
				bOk = CreateReinforcementAttributesForClass(
				    interpretationSpec->GetHowClass(), kwcInterpretationRootClass, classifierAttribute,
				    predictionAttribute, trainedClassifier);
				kwcInterpretationRootClass->GetMetaData()->SetStringValueAt(
				    "ReinforcementClass", interpretationSpec->GetHowClass());
			}
		}
		if (bOk)
		{
			// ajout d'un metadata permettant de reconnaitre qu'il s'agit d'un dico d'interpretation
			kwcInterpretationRootClass->GetMetaData()->SetNoValueAt("InterpreterDictionary");
			kwcdInterpretationDomain->Compile();
		}
	}

	delete trainedClassifier;

	return bOk;
}

boolean KIInterpretationDictionary::CreateContributionAttributesForClass(const ALString sTargetClass,
									 KWClass* kwcInterpretation,
									 const KWAttribute* classifierAttribute,
									 const KWAttribute* predictionAttribute,
									 const KWTrainedClassifier* trainedClassifier)
{
	KWAttribute* scoreInterpretationAttribute =
	    CreateScoreContributionAttribute(sTargetClass, kwcInterpretation, classifierAttribute, predictionAttribute);
	scoreInterpretationAttribute->CompleteTypeInfo(kwcInterpretation);
	kwcInterpretation->InsertAttribute(scoreInterpretationAttribute);

	KWAttribute* contributionClassAttribute =
	    CreateContributionClassAttribute(scoreInterpretationAttribute, kwcInterpretation, sTargetClass);
	contributionClassAttribute->CompleteTypeInfo(kwcInterpretation);
	kwcInterpretation->InsertAttribute(contributionClassAttribute);

	KWAttribute* classPriorAttribute =
	    CreateClassPriorAttribute(sTargetClass, kwcInterpretation, trainedClassifier->GetTargetValuesAttribute(),
				      predictionAttribute, contributionClassAttribute);
	classPriorAttribute->CompleteTypeInfo(kwcInterpretation);
	kwcInterpretation->InsertAttribute(classPriorAttribute);

	// Parcours des variables explicatives a concurrence du nombre max de variables recherchees dans le pourquoi

	for (int nAttributeIndex = 0; nAttributeIndex < interpretationSpec->GetWhyAttributesNumber(); nAttributeIndex++)
	{
		if (not interpretationSpec->IsExpertMode())
		{
			KWAttribute* variableImportanceNameAttribute = CreateContributionNameAtAttribute(
			    scoreInterpretationAttribute, kwcInterpretation, sTargetClass, nAttributeIndex + 1);
			variableImportanceNameAttribute->CompleteTypeInfo(kwcInterpretation);
			kwcInterpretation->InsertAttribute(variableImportanceNameAttribute);

			KWAttribute* variableImportancePartitionAttribute = CreateContributionPartitionAtAttribute(
			    scoreInterpretationAttribute, kwcInterpretation, sTargetClass, nAttributeIndex + 1);
			variableImportancePartitionAttribute->CompleteTypeInfo(kwcInterpretation);
			kwcInterpretation->InsertAttribute(variableImportancePartitionAttribute);
		}

		KWAttribute* variableImportanceValueAttribute = CreateContributionValueAtAttribute(
		    scoreInterpretationAttribute, kwcInterpretation, sTargetClass, nAttributeIndex + 1);
		variableImportanceValueAttribute->CompleteTypeInfo(kwcInterpretation);
		kwcInterpretation->InsertAttribute(variableImportanceValueAttribute);
	}

	if (not kwcInterpretation->Check())
		return false;

	return true;
}

KWAttribute* KIInterpretationDictionary::CreateScoreContributionAttribute(ALString sTargetClass,
									  KWClass* kwcInterpretation,
									  const KWAttribute* classifierAttribute,
									  const KWAttribute* predictionAttribute)
{
	// creation d'une regle de derivation
	KWDerivationRule* rule = new KIDRClassifierContribution;
	rule->SetClassName(kwcInterpretation->GetName());

	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(classifierAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetSecondOperand()->SetAttributeName(predictionAttribute->GetName());
	rule->GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetOperandAt(2)->SetSymbolConstant(sTargetClass.GetBuffer(sTargetClass.GetLength()));
	rule->GetOperandAt(3)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetOperandAt(3)->SetSymbolConstant(GetWhyTypeShortLabel(interpretationSpec->GetWhyType()));
	rule->GetOperandAt(4)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetOperandAt(4)->SetSymbolConstant(interpretationSpec->GetSortWhyResults() ? "sorted" : "unsorted");

	assert(rule->Check());

	// creation de l'attribut, et affectation de la regle de derivation
	KWAttribute* contributionClassAttribute = new KWAttribute;
	contributionClassAttribute->SetName(
	    ((KWClass*)kwcInterpretation)
		->BuildAttributeName(ALString(GetWhyTypeShortLabel(interpretationSpec->GetWhyType()).GetValue()) +
				     ALString("_") + sTargetClass));
	contributionClassAttribute->SetType(KWType::Continuous);
	contributionClassAttribute->GetMetaData()->SetNoValueAt(INTERPRETATION_ATTRIBUTE_META_TAG);

	contributionClassAttribute->SetDerivationRule(rule);

	return contributionClassAttribute;
}

KWAttribute* KIInterpretationDictionary::CreateContributionValueAtAttribute(
    const KWAttribute* scoreInterpretationAttribute, KWClass* kwcInterpretation, ALString sTargetClass, int nIndex)
{
	// creation d'un attribut de type
	// Numerical	`NormalizedOddsRatio1_Predicted class`	 = ContributionValueAt(`NormalizedOddsRatio_Predicted class`, 1)	; <ClassifierInterpretationAttribute>	<ContributionValue1="SepalLength">

	// creation d'une regle de derivation
	KWDerivationRule* rule = new KIDRContributionValueAt;
	rule->SetClassName(kwcInterpretation->GetName());

	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nIndex);

	assert(rule->Check());

	// creation de l'attribut, et affectation de la regle de derivation
	KWAttribute* contributionClassAttribute = new KWAttribute;
	contributionClassAttribute->SetName(
	    ((KWClass*)kwcInterpretation)
		->BuildAttributeName(GetWhyTypeShortLabel(interpretationSpec->GetWhyType()) +
				     ALString(IntToString(nIndex)) + "_" + sTargetClass));
	contributionClassAttribute->SetType(KWType::Continuous);

	contributionClassAttribute->SetDerivationRule(rule);
	contributionClassAttribute->GetMetaData()->SetNoValueAt(INTERPRETATION_ATTRIBUTE_META_TAG);

	if (interpretationSpec->GetWhyAttributesNumber() == oaNativePredictiveAttributeNames.GetSize() and
	    interpretationSpec->GetSortWhyResults() == false)
	{
		StringObject* soNativeAttributeName =
		    cast(StringObject*, oaNativePredictiveAttributeNames.GetAt(nIndex - 1));
		contributionClassAttribute->GetMetaData()->SetStringValueAt(
		    "ContributionValue" + ALString(IntToString(nIndex)), soNativeAttributeName->GetString());
	}
	return contributionClassAttribute;
}

KWAttribute* KIInterpretationDictionary::CreateContributionNameAtAttribute(
    const KWAttribute* scoreInterpretationAttribute, KWClass* kwcInterpretation, ALString sTargetClass, int nIndex)
{
	// creation d'un attribut de type
	// 		Categorical	`ContributionVar1_Predicted class`	 = ContributionNameAt(`NormalizedOddsRatio_Predicted class`, 1)	; <ClassifierInterpretationAttribute>	<ContributionVar1="SepalLength">

	// creation d'une regle de derivation
	KWDerivationRule* rule = new KIDRContributionNameAt;
	rule->SetClassName(kwcInterpretation->GetName());

	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nIndex);

	assert(rule->Check());

	// creation de l'attribut, et affectation de la regle de derivation
	KWAttribute* attribute = new KWAttribute;
	attribute->SetName(
	    ((KWClass*)kwcInterpretation)
		->BuildAttributeName("ContributionVar" + ALString(IntToString(nIndex)) + "_" + sTargetClass));
	attribute->SetType(KWType::Symbol);
	attribute->GetMetaData()->SetNoValueAt(INTERPRETATION_ATTRIBUTE_META_TAG);

	if (interpretationSpec->GetWhyAttributesNumber() == oaNativePredictiveAttributeNames.GetSize() and
	    interpretationSpec->GetSortWhyResults() == false)
	{
		StringObject* soNativeAttributeName =
		    cast(StringObject*, oaNativePredictiveAttributeNames.GetAt(nIndex - 1));
		attribute->GetMetaData()->SetStringValueAt("ContributionVar" + ALString(IntToString(nIndex)),
							   soNativeAttributeName->GetString());
	}
	attribute->SetDerivationRule(rule);

	return attribute;
}

KWAttribute* KIInterpretationDictionary::CreateContributionPartitionAtAttribute(
    const KWAttribute* scoreInterpretationAttribute, KWClass* kwcInterpretation, ALString sTargetClass, int nIndex)
{
	// creation d'un attribut de type
	// 	Categorical	`ContributionPartition1_Predicted class`	 = ContributionPartitionAt(`NormalizedOddsRatio_Predicted class`, 1)	; <ClassifierInterpretationAttribute>	<ContributionPart1="SepalLength">

	// creation d'une regle de derivation
	KWDerivationRule* rule = new KIDRContributionPartitionAt;
	rule->SetClassName(kwcInterpretation->GetName());

	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nIndex);

	assert(rule->Check());

	// creation de l'attribut, et affectation de la regle de derivation
	KWAttribute* attribute = new KWAttribute;
	attribute->SetName(
	    ((KWClass*)kwcInterpretation)
		->BuildAttributeName("ContributionPartition" + ALString(IntToString(nIndex)) + "_" + sTargetClass));
	attribute->SetType(KWType::Symbol);
	attribute->GetMetaData()->SetNoValueAt(INTERPRETATION_ATTRIBUTE_META_TAG);

	if (interpretationSpec->GetWhyAttributesNumber() == oaNativePredictiveAttributeNames.GetSize() and
	    interpretationSpec->GetSortWhyResults() == false)
	{
		StringObject* soNativeAttributeName =
		    cast(StringObject*, oaNativePredictiveAttributeNames.GetAt(nIndex - 1));
		attribute->GetMetaData()->SetStringValueAt("ContributionPart" + ALString(IntToString(nIndex)),
							   soNativeAttributeName->GetString());
	}

	attribute->SetDerivationRule(rule);

	return attribute;
}

KWAttribute*
KIInterpretationDictionary::CreateContributionClassAttribute(const KWAttribute* scoreInterpretationAttribute,
							     KWClass* kwcInterpretation, ALString sTargetClass)
{
	// creation d'une regle de derivation
	KWDerivationRule* rule = new KIDRContributionClass;
	rule->SetClassName(kwcInterpretation->GetName());

	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());

	assert(rule->Check());

	// creation de l'attribut, et affectation de la regle de derivation
	KWAttribute* attribute = new KWAttribute;
	attribute->SetName(
	    ((KWClass*)kwcInterpretation)->BuildAttributeName("ContributionClass (" + sTargetClass + ")"));
	attribute->SetType(KWType::Symbol);
	attribute->GetMetaData()->SetNoValueAt(INTERPRETATION_ATTRIBUTE_META_TAG);
	attribute->SetDerivationRule(rule);

	return attribute;
}

KWAttribute* KIInterpretationDictionary::CreateClassPriorAttribute(ALString sTargetClass, KWClass* kwcInterpretation,
								   const KWAttribute* targetValuesAttribute,
								   const KWAttribute* predictionAttribute,
								   const KWAttribute* contributionClassAttribute)
{
	// creation d'une regle de derivation
	KWDerivationRule* rule = new KIDRContributionPriorClass;
	rule->SetClassName(kwcInterpretation->GetName());

	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(targetValuesAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetSymbolConstant(sTargetClass.GetBuffer(sTargetClass.GetLength()));
	rule->GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetOperandAt(2)->SetAttributeName(predictionAttribute->GetName());
	rule->GetOperandAt(3)->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetOperandAt(3)->SetAttributeName(contributionClassAttribute->GetName());

	assert(rule->Check());

	// creation de l'attribut, et affectation de la regle de derivation
	KWAttribute* priorAttribute = new KWAttribute;
	priorAttribute->SetName(((KWClass*)kwcInterpretation)->BuildAttributeName("PriorClass (" + sTargetClass + ")"));
	priorAttribute->SetType(KWType::Continuous);
	priorAttribute->GetMetaData()->SetNoValueAt(INTERPRETATION_ATTRIBUTE_META_TAG);
	priorAttribute->SetDerivationRule(rule);

	return priorAttribute;
}

boolean KIInterpretationDictionary::CreateReinforcementAttributesForClass(const ALString sTargetClass,
									  KWClass* kwcInterpretation,
									  const KWAttribute* classifierAttribute,
									  const KWAttribute* predictionAttribute,
									  const KWTrainedClassifier* trainedClassifier)
{
	// determiner le nombre max de variables levier qu'on souhaite utiliser
	int iReinforcementAttributesMaxNumber = ComputeReinforcementAttributesMaxNumber();

	if (iReinforcementAttributesMaxNumber == 0)
		return true;

	KWAttribute* scoreInterpretationAttribute = CreateScoreReinforcementAttribute(
	    sTargetClass, kwcInterpretation, classifierAttribute, predictionAttribute);
	scoreInterpretationAttribute->CompleteTypeInfo(kwcInterpretation);
	kwcInterpretation->InsertAttribute(scoreInterpretationAttribute);

	KWAttribute* initialScoreValueAttribute =
	    CreateReinforcementInitialScoreAttribute(scoreInterpretationAttribute, kwcInterpretation, sTargetClass);
	initialScoreValueAttribute->CompleteTypeInfo(kwcInterpretation);
	kwcInterpretation->InsertAttribute(initialScoreValueAttribute);

	// Parcours des variables explicatives a concurrence du nombre max de variables leviers, precedemment calcule
	for (int nAttributeIndex = 0; nAttributeIndex < iReinforcementAttributesMaxNumber; nAttributeIndex++)
	{
		KWAttribute* variableImportanceNameAttribute = CreateReinforcementNameAtAttribute(
		    scoreInterpretationAttribute, kwcInterpretation, sTargetClass, nAttributeIndex + 1);
		variableImportanceNameAttribute->CompleteTypeInfo(kwcInterpretation);
		kwcInterpretation->InsertAttribute(variableImportanceNameAttribute);

		KWAttribute* variableImportancePartitionAttribute = CreateReinforcementPartitionAtAttribute(
		    scoreInterpretationAttribute, kwcInterpretation, sTargetClass, nAttributeIndex + 1);
		variableImportancePartitionAttribute->CompleteTypeInfo(kwcInterpretation);
		kwcInterpretation->InsertAttribute(variableImportancePartitionAttribute);

		KWAttribute* finalScoreValueAttribute = CreateReinforcementFinalScoreAtAttribute(
		    scoreInterpretationAttribute, kwcInterpretation, sTargetClass, nAttributeIndex + 1);
		finalScoreValueAttribute->CompleteTypeInfo(kwcInterpretation);
		kwcInterpretation->InsertAttribute(finalScoreValueAttribute);

		KWAttribute* classChangeAttribute = CreateReinforcementClassChangeAtAttribute(
		    scoreInterpretationAttribute, kwcInterpretation, sTargetClass, nAttributeIndex + 1);
		classChangeAttribute->CompleteTypeInfo(kwcInterpretation);
		kwcInterpretation->InsertAttribute(classChangeAttribute);
	}

	if (not kwcInterpretation->Check())
		return false;

	return true;
}

KWAttribute* KIInterpretationDictionary::CreateScoreReinforcementAttribute(ALString sTargetClass,
									   KWClass* kwcInterpretation,
									   const KWAttribute* classifierAttribute,
									   const KWAttribute* predictionAttribute)
{
	// creation d'une regle de derivation
	KWDerivationRule* rule = new KIDRClassifierReinforcement;
	rule->SetClassName(kwcInterpretation->GetName());

	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(classifierAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetSecondOperand()->SetAttributeName(predictionAttribute->GetName());
	rule->GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetOperandAt(2)->SetSymbolConstant(sTargetClass.GetBuffer(sTargetClass.GetLength()));

	assert(rule->Check());

	// creation de l'attribut, et affectation de la regle de derivation
	KWAttribute* attribute = new KWAttribute;
	attribute->SetName(((KWClass*)kwcInterpretation)->BuildAttributeName("Reinforcement_" + sTargetClass));
	attribute->SetType(KWType::Continuous);
	attribute->GetMetaData()->SetNoValueAt(INTERPRETATION_ATTRIBUTE_META_TAG);

	attribute->SetDerivationRule(rule);

	return attribute;
}

KWAttribute*
KIInterpretationDictionary::CreateReinforcementInitialScoreAttribute(const KWAttribute* scoreInterpretationAttribute,
								     KWClass* kwcInterpretation, ALString sTargetClass)
{
	// creation d'une regle de derivation
	KWDerivationRule* rule = new KIDRReinforcementInitialScore;
	rule->SetClassName(kwcInterpretation->GetName());

	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());

	assert(rule->Check());

	// creation de l'attribut, et affectation de la regle de derivation
	KWAttribute* contributionClassAttribute = new KWAttribute;
	contributionClassAttribute->SetName(
	    ((KWClass*)kwcInterpretation)->BuildAttributeName("ReinforcementInitialScore_" + sTargetClass));
	contributionClassAttribute->SetType(KWType::Continuous);

	contributionClassAttribute->SetDerivationRule(rule);
	contributionClassAttribute->GetMetaData()->SetNoValueAt(INTERPRETATION_ATTRIBUTE_META_TAG);
	return contributionClassAttribute;
}

KWAttribute* KIInterpretationDictionary::CreateReinforcementFinalScoreAtAttribute(
    const KWAttribute* scoreInterpretationAttribute, KWClass* kwcInterpretation, ALString sTargetClass, int nIndex)
{
	// creation d'une regle de derivation
	KWDerivationRule* rule = new KIDRReinforcementFinalScoreAt;
	rule->SetClassName(kwcInterpretation->GetName());

	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nIndex);

	assert(rule->Check());

	// creation de l'attribut, et affectation de la regle de derivation
	KWAttribute* attribute = new KWAttribute;
	attribute->SetName(
	    ((KWClass*)kwcInterpretation)
		->BuildAttributeName("ReinforcementFinalScore" + ALString(IntToString(nIndex)) + "_" + sTargetClass));
	attribute->SetType(KWType::Continuous);

	attribute->SetDerivationRule(rule);
	attribute->GetMetaData()->SetNoValueAt(INTERPRETATION_ATTRIBUTE_META_TAG);

	if (interpretationSpec->GetHowAttributesNumber() == oaNativePredictiveAttributeNames.GetSize() and
	    interpretationSpec->GetSortWhyResults() == false)
	{
		StringObject* soNativeAttributeName =
		    cast(StringObject*, oaNativePredictiveAttributeNames.GetAt(nIndex - 1));
		attribute->GetMetaData()->SetStringValueAt("ReinforcementFinalScore" + ALString(IntToString(nIndex)),
							   soNativeAttributeName->GetString());
	}

	return attribute;
}
KWAttribute* KIInterpretationDictionary::CreateReinforcementNameAtAttribute(
    const KWAttribute* scoreInterpretationAttribute, KWClass* kwcInterpretation, ALString sTargetClass, int nIndex)
{
	// creation d'un attribut de type  Categorical 	Var_1 = InterpretationVariableImportanceName(Reinforcement, 1)

	// creation d'une regle de derivation
	KWDerivationRule* rule = new KIDRReinforcementNameAt;
	rule->SetClassName(kwcInterpretation->GetName());

	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nIndex);

	assert(rule->Check());

	// creation de l'attribut, et affectation de la regle de derivation
	KWAttribute* attribute = new KWAttribute;
	attribute->SetName(
	    ((KWClass*)kwcInterpretation)
		->BuildAttributeName("ReinforcementVar" + ALString(IntToString(nIndex)) + "_" + sTargetClass));
	attribute->SetType(KWType::Symbol);
	attribute->GetMetaData()->SetNoValueAt(INTERPRETATION_ATTRIBUTE_META_TAG);

	if (interpretationSpec->GetHowAttributesNumber() == oaNativePredictiveAttributeNames.GetSize() and
	    interpretationSpec->GetSortWhyResults() == false)
	{
		StringObject* soNativeAttributeName =
		    cast(StringObject*, oaNativePredictiveAttributeNames.GetAt(nIndex - 1));
		attribute->GetMetaData()->SetStringValueAt("ReinforcementValue" + ALString(IntToString(nIndex)),
							   soNativeAttributeName->GetString());
	}

	attribute->SetDerivationRule(rule);

	return attribute;
}

KWAttribute* KIInterpretationDictionary::CreateReinforcementPartitionAtAttribute(
    const KWAttribute* scoreInterpretationAttribute, KWClass* kwcInterpretation, ALString sTargetClass, int nIndex)
{
	// creation d'une regle de derivation
	KWDerivationRule* rule = new KIDRReinforcementPartitionAt;
	rule->SetClassName(kwcInterpretation->GetName());

	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nIndex);

	assert(rule->Check());

	// creation de l'attribut, et affectation de la regle de derivation
	KWAttribute* attribute = new KWAttribute;
	attribute->SetName(
	    ((KWClass*)kwcInterpretation)
		->BuildAttributeName("ReinforcementPartition" + ALString(IntToString(nIndex)) + "_" + sTargetClass));
	attribute->SetType(KWType::Symbol);
	attribute->GetMetaData()->SetNoValueAt(INTERPRETATION_ATTRIBUTE_META_TAG);

	if (interpretationSpec->GetHowAttributesNumber() == oaNativePredictiveAttributeNames.GetSize() and
	    interpretationSpec->GetSortWhyResults() == false)
	{
		StringObject* soNativeAttributeName =
		    cast(StringObject*, oaNativePredictiveAttributeNames.GetAt(nIndex - 1));
		attribute->GetMetaData()->SetStringValueAt("ReinforcementPart" + ALString(IntToString(nIndex)),
							   soNativeAttributeName->GetString());
	}
	attribute->SetDerivationRule(rule);

	return attribute;
}

KWAttribute* KIInterpretationDictionary::CreateReinforcementClassChangeAtAttribute(
    const KWAttribute* scoreInterpretationAttribute, KWClass* kwcInterpretation, ALString sTargetClass, int nIndex)
{
	// creation d'une regle de derivation
	KWDerivationRule* rule = new KIDRReinforcementClassChangeTagAt;
	rule->SetClassName(kwcInterpretation->GetName());

	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nIndex);

	assert(rule->Check());

	// creation de l'attribut, et affectation de la regle de derivation
	KWAttribute* attribute = new KWAttribute;
	attribute->SetName(((KWClass*)kwcInterpretation)
			       ->BuildAttributeName("ReinforcementClassChangeTag" + ALString(IntToString(nIndex)) +
						    "_" + sTargetClass));
	attribute->SetType(KWType::Continuous);

	attribute->SetDerivationRule(rule);
	attribute->GetMetaData()->SetNoValueAt(INTERPRETATION_ATTRIBUTE_META_TAG);

	if (interpretationSpec->GetHowAttributesNumber() == oaNativePredictiveAttributeNames.GetSize() and
	    interpretationSpec->GetSortWhyResults() == false)
	{
		StringObject* soNativeAttributeName =
		    cast(StringObject*, oaNativePredictiveAttributeNames.GetAt(nIndex - 1));
		attribute->GetMetaData()->SetStringValueAt(
		    "ReinforcementClassChangeTag" + ALString(IntToString(nIndex)), soNativeAttributeName->GetString());
	}
	return attribute;
}

int KIInterpretationDictionary::ComputeReinforcementAttributesMaxNumber()
{
	/* le nombre max est la valeur minimum entre :
		- le nombre d'attributs pour le renforcement, parametre via IHM
		- le nombre d'attributs selectionnes comme pouvant etre utilises comme variables leviers, , parametre via IHM
	*/

	assert(interpretationSpec != NULL);
	assert(kwcInterpretationRootClass != NULL);

	int result = interpretationSpec->GetHowAttributesNumber();

	int nbSelected = 0;

	KWAttribute* attribute = kwcInterpretationRootClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		if (attribute->GetConstMetaData()->GetStringValueAt(
			KIInterpretationDictionary::LEVER_ATTRIBUTE_META_TAG) == "true")
			nbSelected++;

		kwcInterpretationRootClass->GetNextAttribute(attribute);
	}

	if (nbSelected < result)
		result = nbSelected;

	return result;
}

const SymbolVector& KIInterpretationDictionary::GetTargetValues() const
{
	return sTargetValues;
}

ObjectArray* KIInterpretationDictionary::GetPredictiveAttributeNamesArray()
{
	return &oaPartitionedPredictiveAttributeNames;
}

boolean KIInterpretationDictionary::TestGroupTargetValues(KWClass* inputClassifier)
{

	boolean bOk = true;
	ALString sValue;
	//KWTrainedClassifier* trainedClassifier = new KWTrainedClassifier;

	//bOk = trainedClassifier->ImportPredictorClass(inputClassifier);

	if (bOk)
	{
		//ObjectArray* interpretationAttributes = new ObjectArray;
		KWDerivationRule* derivationrule;
		KWDerivationRuleOperand* operand;
		// recherche des attributs necessaires dans le modele
		//KWAttribute* classifierAttribute = NULL;
		//KWAttribute* predictionAttribute = NULL;

		KWAttribute* attribute = inputClassifier->GetHeadAttribute();

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

boolean KIInterpretationDictionary::ImportClassifier(KWClass* inputClassifier)
{
	KWTrainedClassifier* trainedClassifier = NULL;
	KWAttribute* predictionAttribute;
	KWAttribute* attribute;
	KWAttribute* nativeAttribute;
	KWDRNBClassifier* classifierRule;
	KWDerivationRuleOperand* operand;
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
	int nOperandIndex;
	int nFirstOperandIndex = 0;
	int nAttributeIndex;

	// reinitialisation d'une precedente importation
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
		bIsClassifier = trainedClassifier->ImportPredictorClass((KWClass*)inputClassifier);

		if (trainedClassifier->GetMetaDataPredictorLabel(inputClassifier) == "Data Grid")
			bIsClassifier = false; // pas gere (pour l'instant ?)

		if (!TestGroupTargetValues(inputClassifier))
			bIsClassifier = false; // pas gere (pour l'instant ?)

		// Cas ou le dictionnaire est bien celui d'un classifieur pouvant etre interprete
		if (bIsClassifier)
		{
			// Initalisation des indicateurs de presence d'un NB ou d'un SNB
			nAttributeNaiveBayes = 0;
			nAttributeSelectiveNaiveBayes = 0;

			for (int i = 0; i < trainedClassifier->GetTargetValueNumber(); i++)
				sTargetValues.Add(trainedClassifier->GetTargetValueAt(i));

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

				if (nAttributeNaiveBayes == 1)
					nFirstOperandIndex = 0;
				else if (nAttributeSelectiveNaiveBayes == 1)
					nFirstOperandIndex = 1;

				// Parcours des operandes pour identifier les noms des attributs explicatifs et des attributs natifs associes
				// La derniere operande n'est pas parcouru car reserve a l'attribut des valeurs cibles
				for (nOperandIndex = nFirstOperandIndex;
				     nOperandIndex < classifierRule->GetOperandNumber() - 1; nOperandIndex++)
				{
					operand = classifierRule->GetOperandAt(nOperandIndex);

					// Extraction du nom de la variable explicative
					sAttributeName =
					    operand->GetDerivationRule()->GetFirstOperand()->GetAttributeName();

					// Creation d'un StringObject pour memoriser ce nom de variable
					StringObject* soAttributeName = new StringObject;
					soAttributeName->SetString(sAttributeName);

					// Memorisation du nom de la variable pour la synchronisation
					// avec le tableau oaPartitionIntervals
					oaPartitionedPredictiveAttributeNames.Add(soAttributeName);

					// Extraction du nom de la variable native
					sAttributeName =
					    operand->GetDerivationRule()->GetSecondOperand()->GetAttributeName();

					// Creation d'un StringObject pour memoriser ce nom de variable
					soAttributeName = new StringObject;
					soAttributeName->SetString(sAttributeName);

					// Memorisation du nom de la variable pour la synchronisation
					// avec le tableau oaPartitionIntervals
					oaNativePredictiveAttributeNames.Add(soAttributeName);
				}

				if (oaNativePredictiveAttributeNames.GetSize() == 0)
					bOk = false;

				// Parcours des attributs explicatifs recenses
				for (nAttributeIndex = 0;
				     nAttributeIndex < oaPartitionedPredictiveAttributeNames.GetSize();
				     nAttributeIndex++)
				{
					// Extraction de l'attribut explicatif courant
					attribute = inputClassifier->LookupAttribute(
					    cast(StringObject*,
						 oaPartitionedPredictiveAttributeNames.GetAt(nAttributeIndex))
						->GetString());

					// Cas ou l'attribut partitionne n'est pas present dans le dictionnaire
					if (attribute == NULL)
						bOk = false;

					// Extraction de l'attribut natif
					nativeAttribute = inputClassifier->LookupAttribute(
					    cast(StringObject*, oaNativePredictiveAttributeNames.GetAt(nAttributeIndex))
						->GetString());

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

		//if (interpretationSpec->GetHowAttributesNumber() > GetPredictiveAttributeNamesArray()->GetSize())
		interpretationSpec->SetHowAttributesNumber(
		    GetPredictiveAttributeNamesArray()
			->GetSize()); // ne pas ecraser un precedent choix fait via l'IHM, lors d'une nouvelle importation

		//if (interpretationSpec->GetWhyAttributesNumber() > GetPredictiveAttributeNamesArray()->GetSize())
		interpretationSpec->SetWhyAttributesNumber(GetPredictiveAttributeNamesArray()->GetSize());

		assert(trainedClassifier->GetTargetAttribute() != NULL);

		// l'import a reussi, on peut donc creer un domaine specifique d'interpretation, et le(s) dico(s) d'interpretation qu'il doit contenir
		bOk = CreateInterpretationDomain(inputClassifier);
		if (bOk)
			PrepareInterpretationClass();
	}

	if (trainedClassifier != NULL)
		delete trainedClassifier;

	bIsClassifierNaiveBayes = bIsClassifierNaiveBayes and bOk;

	return bIsClassifierNaiveBayes;
}

void KIInterpretationDictionary::PrepareInterpretationClass()
{
	KWAttribute* nativeAttribute;
	ALString sNativeVariableName;
	int nAttributeIndex;

	if (oaPartitionedPredictiveAttributeNames.GetSize() == 0)
		return;

	assert(kwcInterpretationRootClass != NULL);
	assert(kwcdInterpretationDomain != NULL);

	// tagguer les attributs explicatifs contribuant au predicteur, et pouvant etre utilisees eventuellement comme leviers
	for (nAttributeIndex = 0; nAttributeIndex < oaPartitionedPredictiveAttributeNames.GetSize(); nAttributeIndex++)
	{
		sNativeVariableName =
		    cast(StringObject*, oaNativePredictiveAttributeNames.GetAt(nAttributeIndex))->GetString();

		// Extraction de l'attribut natif
		nativeAttribute = kwcInterpretationRootClass->LookupAttribute(sNativeVariableName);
		assert(nativeAttribute != NULL);
		if (not nativeAttribute->GetConstMetaData()->IsKeyPresent(LEVER_ATTRIBUTE_META_TAG))
		{
			nativeAttribute->GetMetaData()->SetStringValueAt(
			    LEVER_ATTRIBUTE_META_TAG,
			    "true"); // par defaut : toutes les variables natives pourront etre utilisees comme levier
		}
	}

	// synchroniser les proprietes Used et Loaded du dico de transfert a partir de la selection faite dans le classifieur d'entree
	// on parcourt cette fois-ci tous les attributs
	KWAttribute* attribute = kwcInterpretationRootClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		KWAttribute* inputAttribute = GetInputClassifier()->LookupAttribute(attribute->GetName());

		if (inputAttribute != NULL)
		{
			attribute->SetUsed(inputAttribute->GetUsed());
			attribute->SetLoaded(attribute->GetUsed());
		}
		kwcInterpretationRootClass->GetNextAttribute(attribute);
	}
	kwcdInterpretationDomain->Compile();
}

Symbol KIInterpretationDictionary::GetWhyTypeShortLabel(const ALString asWhyTypeLongLabel)
{
	if (asWhyTypeLongLabel == "Normalized odds ratio")
		return NORMALIZED_ODDS_RATIO_LABEL;
	else if (asWhyTypeLongLabel == "Weight of evidence")
		return WEIGHT_EVIDENCE_LABEL;
	else if (asWhyTypeLongLabel == "Log minimum of variable probabilities difference")
		return LOG_MIN_PROBA_DIFF_LABEL;
	else if (asWhyTypeLongLabel == "Information difference")
		return INFO_DIFF_LABEL;
	else if (asWhyTypeLongLabel == "Difference of probabilities")
		return DIFF_PROBA_LABEL;
	else if (asWhyTypeLongLabel == "Minimum of variable probabilities difference")
		return MIN_PROBA_DIFF_LABEL;
	else if (asWhyTypeLongLabel == "Modality probability")
		return MODALITY_PROBA_LABEL;
	else if (asWhyTypeLongLabel == "Log modality probability")
		return LOG_MODALITY_PROBA_LABEL;
	else if (asWhyTypeLongLabel == "Bayes distance")
		return BAYES_DISTANCE_LABEL;
	else if (asWhyTypeLongLabel == "Bayes distance without prior")
		return BAYES_DISTANCE_WITHOUT_PRIOR_LABEL;
	else if (asWhyTypeLongLabel == "Kullback")
		return KULLBACK_LABEL;
	else if (asWhyTypeLongLabel == "Shapley")
		return SHAPLEY_LABEL;
	else
		return "Undefined";
}

const ALString KIInterpretationDictionary::LEVER_ATTRIBUTE_META_TAG = "LeverAttribute";
const ALString KIInterpretationDictionary::INTERPRETATION_ATTRIBUTE_META_TAG = "ClassifierInterpretationAttribute";
const ALString KIInterpretationDictionary::NO_VALUE_LABEL = "No value";
