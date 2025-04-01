// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KIInterpretationClassBuilder.h"

// Inclusion des header dans le source et non dans le hedaer, pour eviter les cycles d'inclusion
#include "KIModelInterpreter.h"
#include "KIModelReinforcer.h"

KIInterpretationClassBuilder::KIInterpretationClassBuilder()
{
	kwcPredictorClass = NULL;
}

KIInterpretationClassBuilder::~KIInterpretationClassBuilder()
{
	Clean();
}

boolean KIInterpretationClassBuilder::ImportPredictor(KWClass* kwcInputPredictor)
{
	const boolean bTrace = false;
	const KWDRNBClassifier referenceNBRule;
	const KWDRSNBClassifier referenceSNBRule;
	boolean bOk;
	KWTrainedClassifier trainedClassifier;
	KWTrainedRegressor trainedRegressor;
	KWAttribute* predictionAttribute;
	KWAttribute* attribute;
	KWDRNBClassifier* classifierRule;
	boolean bIsClassifier;
	ObjectArray oaClasses;
	ALString sAttributeName;
	int i;

	require(kwcInputPredictor != NULL);
	require(kwcInputPredictor->Check());
	require(KWType::IsSimple(KWTrainedPredictor::GetMetaDataPredictorType(kwcInputPredictor)));

	// Nettoyage prealable
	Clean();

	// Initialisations
	bIsClassifier = false;
	bOk = true;

	// Test d'importation de ce dictionnaire dans un classifieur
	bIsClassifier = trainedClassifier.ImportPredictorClass(kwcInputPredictor);

	// On ne gere pas les grilles
	if (bIsClassifier and trainedClassifier.GetMetaDataPredictorLabel(kwcInputPredictor) == "Data Grid")
	{
		Global::AddWarning("Dictionary", kwcInputPredictor->GetName(),
				   "Interpretation services not available for data grid predictors");
		bIsClassifier = false;
	}

	// On exclut le cas d'un dictionnaire d'interpretation
	if (bIsClassifier and kwcInputPredictor->GetConstMetaData()->IsKeyPresent(GetIntepreterMetaDataKey()))
	{
		Global::AddWarning("Dictionary", kwcInputPredictor->GetName(),
				   "Already used as an interpretation dictionary");
		bIsClassifier = false;
	}

	// On exclut le cas d'un dictionnaire de renforcement
	if (bIsClassifier and kwcInputPredictor->GetConstMetaData()->IsKeyPresent(GetReinforcerMetaDataKey()))
	{
		Global::AddWarning("Dictionary", kwcInputPredictor->GetName(),
				   "Already used as a reinforcement dictionary");
		bIsClassifier = false;
	}

	// On ne gere pas actuellement les classifieur avec groupement de la cible
	if (bIsClassifier and IsClassifierClassUsingTargetValueGrouping(kwcInputPredictor))
	{
		Global::AddWarning("Dictionary", kwcInputPredictor->GetName(),
				   "Interpretation services not yet implemented "
				   "for classifiers with grouped target values");
		bIsClassifier = false;
	}

	// On ne gere pas actuellement les classifieur avec pretraitement bivaries
	if (bIsClassifier and IsClassifierClassUsingBivariatePreprocessing(kwcInputPredictor))
	{
		Global::AddWarning("Dictionary", kwcInputPredictor->GetName(),
				   "Interpretation services not yet implemented "
				   "for classifiers with variable pairs");
		bIsClassifier = false;
	}

	// Message d'erreur specifique pour le cas des regresseurs, non geres actuellement
	if (not bIsClassifier)
	{
		if (trainedRegressor.ImportPredictorClass(kwcInputPredictor))
		{
			Global::AddWarning("Dictionary", kwcInputPredictor->GetName(),
					   "Interpretation services not yet implemented for regressors");
		}
		trainedRegressor.DeletePredictor();
	}

	// Cas ou le dictionnaire est bien celui d'un classifieur pouvant etre interprete
	if (bIsClassifier)
	{
		// Collecte des valeurs cibles
		for (i = 0; i < trainedClassifier.GetTargetValueNumber(); i++)
			svTargetValues.Add(trainedClassifier.GetTargetValueAt(i));

		// Erreur s'il ny a pas au moins deux classe
		if (svTargetValues.GetSize() <= 1)
			bIsClassifier = false;

		// Extraction de l'attribut de prediction
		predictionAttribute = trainedClassifier.GetPredictionAttribute();
		if (predictionAttribute->GetDerivationRule() == NULL)
			bIsClassifier = false;
		else if (predictionAttribute->GetDerivationRule()->GetOperandNumber() == 0)
			bIsClassifier = false;
		sPredictionAttributeName = predictionAttribute->GetName();

		// L'attribut de prediction doit avoir en premier operande un attribut de type Structure
		if (bIsClassifier and
		    (predictionAttribute->GetDerivationRule()->GetFirstOperand()->GetType() != KWType::Structure or
		     predictionAttribute->GetDerivationRule()->GetFirstOperand()->GetOrigin() !=
			 KWDerivationRuleOperand::OriginAttribute))
			bIsClassifier = false;

		// Recherche de l'attribut decrivant le predicteur
		if (bIsClassifier)
		{
			// Extraction de l'attribut Structure qui decrit le classifier
			// Cet attribut doit avoir une regle de derivation dont le nom est celui
			// d'une regle NB ou SNB
			attribute = kwcInputPredictor->LookupAttribute(
			    predictionAttribute->GetDerivationRule()->GetFirstOperand()->GetAttributeName());
			assert(attribute != NULL);
			sPredictorRuleAttributeName = attribute->GetName();

			// Le predicteur est specifie via une regle de derivation
			if (attribute->GetDerivationRule() == NULL)
				bIsClassifier = false;
			// Il doit etre soit le Naive Bayes, soit le Selective Naive Bayes
			else
			{
				if (attribute->GetDerivationRule()->GetName() != referenceNBRule.GetName() and
				    attribute->GetDerivationRule()->GetName() != referenceSNBRule.GetName())
				{
					Global::AddWarning(
					    "Dictionary", kwcInputPredictor->GetName(),
					    "Interpretation services are available only for naive bayes predictors");
					bIsClassifier = false;
				}
			}
		}

		// Cas d'un classifier valide, donc heritant du Naive Bayes
		if (bIsClassifier)
		{
			assert(sPredictorRuleAttributeName != "");

			// Extraction de la regle de derivation de l'attribut classifieur
			classifierRule =
			    cast(KWDRNBClassifier*,
				 kwcInputPredictor->LookupAttribute(sPredictorRuleAttributeName)->GetDerivationRule());

			// Initialisation des listes d'attributs du classifier
			classifierRule->ExportAttributeNames(&svPredictorAttributeNames,
							     &svPredictorPartitionedAttributeNames);
			assert(svPredictorAttributeNames.GetSize() == svPredictorPartitionedAttributeNames.GetSize());

			// Le classifier doit avoir au moins un attribut
			if (svPredictorAttributeNames.GetSize() == 0)
			{
				Global::AddWarning("Dictionary", kwcInputPredictor->GetName(),
						   "Interpretation services not available for predictors without "
						   "predictor variables");
				bIsClassifier = false;
			}

			// Parcours des attributs explicatifs recenses, pour des tests d'integrite avances
			// Ces precautions permettent de se premunir au mieux contre des incoherence de
			// dictionnaires de classifieurs modifies a la main
			for (i = 0; i < svPredictorAttributeNames.GetSize(); i++)
			{
				// Verification de l'attribut du predicteur natif
				attribute = kwcInputPredictor->LookupAttribute(svPredictorAttributeNames.GetAt(i));
				if (attribute == NULL)
					bIsClassifier = false;

				// Verification de l'attribut partitionne du predicteur natif
				attribute =
				    kwcInputPredictor->LookupAttribute(svPredictorPartitionedAttributeNames.GetAt(i));
				if (attribute == NULL)
					bIsClassifier = false;
			}
		}
	}

	// Nettoyage
	trainedClassifier.DeletePredictor();

	// Memorisation du predicteur si ok
	bOk = bIsClassifier;
	if (bOk)
	{
		kwcPredictorClass = kwcInputPredictor;
		assert(kwcPredictorClass != NULL);
		assert(svTargetValues.GetSize() >= 2);
		assert(svPredictorAttributeNames.GetSize() >= 1);
		assert(svPredictorPartitionedAttributeNames.GetSize() >= svPredictorAttributeNames.GetSize());
	}
	// Nettoyage sinon
	else
		Clean();

	// Trace
	if (bTrace)
	{
		cout << "ImportPredictor " << kwcInputPredictor->GetName() << ": "
		     << BooleanToString(IsPredictorImported()) << "\n";

		// Caracteristiques d'un predicteur importe
		if (IsPredictorImported())
		{
			// Nom des attributs du predicteur et de prediction
			cout << "\tPredictor rule variable name: " << sPredictorRuleAttributeName << "\n";
			cout << "\tPrediction variable name: " << sPredictionAttributeName << "\n";

			// Valeurs cibles
			cout << "\tTarget values: " << svTargetValues.GetSize() << "\n";
			for (i = 0; i < svTargetValues.GetSize(); i++)
				cout << "\t\t" << svTargetValues.GetAt(i) << "\n";

			// Variables du predicteur
			cout << "\tPredictor variables: " << svPredictorAttributeNames.GetSize() << "\n";
			for (i = 0; i < svPredictorAttributeNames.GetSize(); i++)
				cout << "\t\t" << svPredictorAttributeNames.GetAt(i) << "\t"
				     << svPredictorPartitionedAttributeNames.GetAt(i) << "\n";
		}
	}
	return bOk;
}

boolean KIInterpretationClassBuilder::IsPredictorImported() const
{
	return kwcPredictorClass != NULL;
}

void KIInterpretationClassBuilder::Clean()
{
	// Nettoyage des resultats d'import
	kwcPredictorClass = NULL;
	sPredictorRuleAttributeName = "";
	sPredictionAttributeName = "";
	svTargetValues.SetSize(0);
	svPredictorAttributeNames.SetSize(0);
	svPredictorPartitionedAttributeNames.SetSize(0);
}

KWClass* KIInterpretationClassBuilder::BuildInterpretationServiceClass(const ALString& sServiceLabel,
								       const StringVector* svServiceMetaDataKeys) const
{
	KWClassDomain* kwcdInterpretationServiceDomain;
	KWClass* kwcInterpretationServiceClass;
	KWAttribute* attribute;
	SymbolVector svInterpretedTargetValues;
	Symbol sTargetValue;
	int nIndex;
	StringVector svInterpretationMetaDataKeys;

	require(IsPredictorImported());
	require(GetPredictorAttributeNumber() > 0);
	require(sServiceLabel != "");
	require(svServiceMetaDataKeys != NULL);

	// Clone du domaine d'origine a partir du predicteur
	kwcdInterpretationServiceDomain = GetPredictorClass()->GetDomain()->CloneFromClass(GetPredictorClass());
	kwcdInterpretationServiceDomain->SetName(sServiceLabel + "Domain");
	kwcInterpretationServiceClass = kwcdInterpretationServiceDomain->LookupClass(GetPredictorClass()->GetName());
	kwcdInterpretationServiceDomain->RenameClass(kwcInterpretationServiceClass,
						     sServiceLabel + "_" + kwcInterpretationServiceClass->GetName());

	// On met toutes les variables en unused
	kwcInterpretationServiceClass->SetAllAttributesUsed(false);

	// On met les attributs de la cle en used
	for (nIndex = 0; nIndex < kwcInterpretationServiceClass->GetKeyAttributeNumber(); nIndex++)
	{
		attribute = kwcInterpretationServiceClass->GetKeyAttributeAt(nIndex);
		check(attribute);
		attribute->SetUsed(true);
		attribute->SetLoaded(true);
	}

	// Nettoyage des meta-data liees au service, interpretation potentiellement existants
	for (nIndex = 0; nIndex < svServiceMetaDataKeys->GetSize(); nIndex++)
		kwcInterpretationServiceClass->GetMetaData()->RemoveKey(svServiceMetaDataKeys->GetAt(nIndex));
	attribute = kwcInterpretationServiceClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		for (nIndex = 0; nIndex < svServiceMetaDataKeys->GetSize(); nIndex++)
			attribute->GetMetaData()->RemoveKey(svServiceMetaDataKeys->GetAt(nIndex));
		kwcInterpretationServiceClass->GetNextAttribute(attribute);
	}
	return kwcInterpretationServiceClass;
}

boolean KIInterpretationClassBuilder::IsClassifierClassUsingTargetValueGrouping(KWClass* kwcClassifier) const
{
	boolean bIsTargetValueGrouping;
	const KWDRDataGrid referenceDataGridRule;
	const KWDRValueGroups referenceValueGroupsRule;
	KWDRDataGrid* dataGridRule;
	KWDerivationRuleOperand* operand;
	KWAttribute* attribute;

	require(kwcClassifier != NULL);

	// Parcours des attributs pour detecter l'utilisation d'une regle de groupement des valeurs
	bIsTargetValueGrouping = false;
	attribute = kwcClassifier->GetHeadAttribute();
	while (attribute != NULL)
	{
		if (attribute->GetStructureName() == referenceDataGridRule.GetStructureName() and
		    attribute->GetConstMetaData()->IsKeyPresent(KWDataPreparationAttribute::GetLevelMetaDataKey()))
		{
			if (attribute->GetDerivationRule()->GetName() == referenceDataGridRule.GetName())
			{
				dataGridRule = cast(KWDRDataGrid*, attribute->GetDerivationRule());

				// On ne regarde que les grilles a deux dimensions lies au traitement univarie
				// pour ne pas regarder la deuxieme variable du traitement bivarie qui est necessairement
				// groupee dans le cas categoriel
				if (dataGridRule->GetAttributeNumber() == 2)
				{
					operand = dataGridRule->GetSecondOperand();
					if (operand->GetDerivationRule()->GetName() ==
					    referenceValueGroupsRule.GetName())
						bIsTargetValueGrouping = true;
					if (bIsTargetValueGrouping)
						break;
				}
			}
		}
		kwcClassifier->GetNextAttribute(attribute);
	}
	return bIsTargetValueGrouping;
}

boolean KIInterpretationClassBuilder::IsClassifierClassUsingBivariatePreprocessing(KWClass* kwcClassifier) const
{
	boolean bIsBivariatePreprocessing;
	const KWDRDataGrid referenceDataGridRule;
	const KWDRValueGroups referenceValueGroupsRule;
	KWDRDataGrid* dataGridRule;
	KWAttribute* attribute;

	require(kwcClassifier != NULL);

	// Parcours des attributs pour detecter l'utilisation d'une regle de groupement des valeurs
	bIsBivariatePreprocessing = false;
	attribute = kwcClassifier->GetHeadAttribute();
	while (attribute != NULL)
	{
		if (attribute->GetStructureName() == referenceDataGridRule.GetStructureName() and
		    attribute->GetConstMetaData()->IsKeyPresent(KWDataPreparationAttribute::GetLevelMetaDataKey()))
		{
			if (attribute->GetDerivationRule()->GetName() == referenceDataGridRule.GetName())
			{
				dataGridRule = cast(KWDRDataGrid*, attribute->GetDerivationRule());

				// Un pretraitement bivarie repose sur une grille a trois dimensions, la derniere gerant l'attribut cible
				if (dataGridRule->GetAttributeNumber() > 2)
				{
					bIsBivariatePreprocessing = true;
					break;
				}
			}
		}
		kwcClassifier->GetNextAttribute(attribute);
	}
	return bIsBivariatePreprocessing;
}

void KIInterpretationClassBuilder::CreateContributionAttributesForClass(
    KWClass* kwcInterpretation, const ALString& sTargetClass, const KWAttribute* predictorRuleAttribute,
    const KWAttribute* predictionAttribute, boolean bIsGlobalRanking, int nContributionAttributeNumber) const
{
	KWAttribute* scoreInterpretationAttribute;
	KWAttribute* contributionVariableNameAttribute;
	KWAttribute* contributionVariablePartAttribute;
	KWAttribute* contributionVariableValueAttribute;
	int nAttributeIndex;

	require(kwcInterpretation != NULL);
	require(predictorRuleAttribute != NULL);
	require(predictionAttribute != NULL);
	require(kwcInterpretation->LookupAttribute(predictorRuleAttribute->GetName()) == predictorRuleAttribute);
	require(kwcInterpretation->LookupAttribute(predictionAttribute->GetName()) == predictionAttribute);
	require(0 < nContributionAttributeNumber and nContributionAttributeNumber <= GetPredictorAttributeNumber());

	// Creation de l'attribut gerant les contribution
	scoreInterpretationAttribute = CreateScoreContributionAttribute(
	    kwcInterpretation, sTargetClass, predictorRuleAttribute, predictionAttribute, bIsGlobalRanking);
	scoreInterpretationAttribute->CompleteTypeInfo(kwcInterpretation);
	kwcInterpretation->InsertAttribute(scoreInterpretationAttribute);
	scoreInterpretationAttribute->SetUsed(false);

	// Parcours des variables explicatives a concurrence du nombre max de variables recherchees dans le pourquoi
	for (nAttributeIndex = 0; nAttributeIndex < nContributionAttributeNumber; nAttributeIndex++)
	{
		// Cas d'un ranking global des valeurs de Shapley
		if (bIsGlobalRanking)
		{
			// Valeur de contribution
			contributionVariableValueAttribute = CreateContributionValueAtAttribute(
			    kwcInterpretation, sTargetClass, scoreInterpretationAttribute, bIsGlobalRanking,
			    nAttributeIndex + 1);
			contributionVariableValueAttribute->CompleteTypeInfo(kwcInterpretation);
			kwcInterpretation->InsertAttribute(contributionVariableValueAttribute);
		}
		// Cas d'un ranking individuel des valeurs de Shapley
		else
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

			// Valeur de contribution
			contributionVariableValueAttribute = CreateContributionValueAtAttribute(
			    kwcInterpretation, sTargetClass, scoreInterpretationAttribute, bIsGlobalRanking,
			    nAttributeIndex + 1);
			contributionVariableValueAttribute->CompleteTypeInfo(kwcInterpretation);
			kwcInterpretation->InsertAttribute(contributionVariableValueAttribute);
		}
	}
	assert(kwcInterpretation->Check());
}

KWAttribute* KIInterpretationClassBuilder::CreateScoreContributionAttribute(KWClass* kwcInterpretation,
									    const ALString& sTargetClass,
									    const KWAttribute* predictorRuleAttribute,
									    const KWAttribute* predictionAttribute,
									    boolean bIsGlobalRanking) const
{
	KWAttribute* contributionClassAttribute;
	KWDerivationRule* rule;
	ALString sValue;

	// Creation de la regle de derivation
	rule = new KIDRClassifierContribution;
	rule->SetClassName(kwcInterpretation->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(predictorRuleAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetSecondOperand()->SetAttributeName(predictionAttribute->GetName());
	rule->GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetOperandAt(2)->SetSymbolConstant((Symbol)sTargetClass);
	rule->GetOperandAt(3)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetOperandAt(3)->SetSymbolConstant(Symbol(GetShapleyLabel()));
	rule->GetOperandAt(4)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetOperandAt(4)->SetSymbolConstant(bIsGlobalRanking ? "unsorted" : "sorted");
	assert(rule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	contributionClassAttribute = new KWAttribute;
	contributionClassAttribute->SetName(kwcInterpretation->BuildAttributeName("Contribution_" + sTargetClass));
	contributionClassAttribute->SetType(KWType::Continuous);
	contributionClassAttribute->SetDerivationRule(rule);
	return contributionClassAttribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateContributionValueAtAttribute(
    KWClass* kwcInterpretation, const ALString& sTargetClass, const KWAttribute* scoreInterpretationAttribute,
    boolean bIsGlobalRanking, int nAttributeRank) const
{
	KWDerivationRule* rule;
	KWAttribute* contributionClassAttribute;
	ALString sPredictorAttributeName;

	require(1 <= nAttributeRank);

	// Creation de la regle de derivation
	rule = new KIDRContributionValueAt;
	rule->SetClassName(kwcInterpretation->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nAttributeRank);
	assert(rule->Check());

	// Attribut natif de la regle
	sPredictorAttributeName = svPredictorAttributeNames.GetAt(nAttributeRank - 1);

	// Creation de l'attribut, et affectation de la regle de derivation
	contributionClassAttribute = new KWAttribute;
	contributionClassAttribute->SetDerivationRule(rule);
	/*DDD
	if (interpretationSpec->GetWhyAttributesNumber() == svPredictorAttributeNames.GetSize() and
	    interpretationSpec->GetSortWhyResults() == false)
	{
	*/
	if (bIsGlobalRanking)
	{
		contributionClassAttribute->SetName(kwcInterpretation->BuildAttributeName(
		    GetShapleyLabel() + "_" + sTargetClass + "_" + sPredictorAttributeName));
		contributionClassAttribute->SetType(KWType::Continuous);
		contributionClassAttribute->GetMetaData()->SetStringValueAt(GetContributionAttributeMetaDataKey(),
									    sPredictorAttributeName);
		contributionClassAttribute->GetMetaData()->SetStringValueAt(GetTargetMetaDataKey(), sTargetClass);
	}
	else
	{
		contributionClassAttribute->SetName(kwcInterpretation->BuildAttributeName(
		    GetShapleyLabel() + "Value_" + sTargetClass + "_" + IntToString(nAttributeRank)));
		contributionClassAttribute->GetMetaData()->SetDoubleValueAt(GetContributionValueRankMetaDataKey(),
									    nAttributeRank);
		contributionClassAttribute->GetMetaData()->SetStringValueAt(GetTargetMetaDataKey(), sTargetClass);
	}
	return contributionClassAttribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateContributionNameAtAttribute(
    KWClass* kwcInterpretation, const ALString& sTargetClass, const KWAttribute* scoreInterpretationAttribute,
    int nAttributeRank) const
{
	KWDerivationRule* rule;
	KWAttribute* attribute;

	require(1 <= nAttributeRank);

	// Creation de la regle de derivation
	rule = new KIDRContributionNameAt;
	rule->SetClassName(kwcInterpretation->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nAttributeRank);
	assert(rule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	attribute = new KWAttribute;
	attribute->SetName(kwcInterpretation->BuildAttributeName(GetShapleyLabel() + "Variable_" + sTargetClass + "_" +
								 IntToString(nAttributeRank)));
	attribute->SetType(KWType::Symbol);

	attribute->GetMetaData()->SetDoubleValueAt(GetContributionAttributeRankMetaDataKey(), nAttributeRank);
	attribute->GetMetaData()->SetStringValueAt(GetTargetMetaDataKey(), sTargetClass);
	/*DDD
	if (interpretationSpec->GetWhyAttributesNumber() == svPredictorAttributeNames.GetSize() and
	    interpretationSpec->GetSortWhyResults() == false)
	{
	*/
	attribute->GetMetaData()->SetDoubleValueAt(GetContributionAttributeRankMetaDataKey(), nAttributeRank);
	attribute->GetMetaData()->SetStringValueAt(GetTargetMetaDataKey(), sTargetClass);
	//DDD}
	attribute->SetDerivationRule(rule);

	return attribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateContributionPartAtAttribute(
    KWClass* kwcInterpretation, const ALString& sTargetClass, const KWAttribute* scoreInterpretationAttribute,
    int nAttributeRank) const
{
	KWDerivationRule* rule;
	KWAttribute* attribute;

	require(1 <= nAttributeRank);

	// Creation de la regle de derivation
	rule = new KIDRContributionPartitionAt;
	rule->SetClassName(kwcInterpretation->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nAttributeRank);
	assert(rule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	attribute = new KWAttribute;
	attribute->SetName(kwcInterpretation->BuildAttributeName(GetShapleyLabel() + "Part_" + sTargetClass + "_" +
								 IntToString(nAttributeRank)));
	attribute->SetType(KWType::Symbol);
	attribute->GetMetaData()->SetDoubleValueAt(GetContributionPartRankMetaDataKey(), nAttributeRank);
	attribute->GetMetaData()->SetStringValueAt(GetTargetMetaDataKey(), sTargetClass);
	/*DDD
	if (interpretationSpec->GetWhyAttributesNumber() == svPredictorAttributeNames.GetSize() and
	    interpretationSpec->GetSortWhyResults() == false)
	{
	*/
	attribute->GetMetaData()->SetDoubleValueAt(GetContributionPartRankMetaDataKey(), nAttributeRank);
	attribute->GetMetaData()->SetStringValueAt(GetTargetMetaDataKey(), sTargetClass);
	//DDD}
	attribute->SetDerivationRule(rule);
	return attribute;
}

void KIInterpretationClassBuilder::CreateReinforcementAttributesForClass(
    KWClass* kwcReinforcement, const ALString& sTargetClass, const KWAttribute* predictorRuleAttribute,
    const KWAttribute* predictionAttribute, const StringVector* svReinforcementAttributeNames) const
{
	int nAttributeIndex;
	int nReinforcementAttributesMaxNumber;
	KWAttribute* scoreInterpretationAttribute;
	KWAttribute* initialScoreValueAttribute;
	KWAttribute* reinforcemenVariableNameAttribute;
	KWAttribute* reinforcemenVariablePartAttribute;
	KWAttribute* finalScoreValueAttribute;
	KWAttribute* classChangeAttribute;

	require(kwcReinforcement != NULL);
	require(predictorRuleAttribute != NULL);
	require(predictionAttribute != NULL);
	require(kwcReinforcement->LookupAttribute(predictorRuleAttribute->GetName()) == predictorRuleAttribute);
	require(kwcReinforcement->LookupAttribute(predictionAttribute->GetName()) == predictionAttribute);
	require(svReinforcementAttributeNames != NULL);
	require(svReinforcementAttributeNames->GetSize() > 0);

	// Nombre max de variables levier qu'on souhaite utiliser
	nReinforcementAttributesMaxNumber = svReinforcementAttributeNames->GetSize();

	// Attribut pour le renforcement
	//DDD ON DEVRAIT PARAMETRER PAR svReinforcementAttributeNames
	scoreInterpretationAttribute = CreateScoreReinforcementAttribute(kwcReinforcement, sTargetClass,
									 predictorRuleAttribute, predictionAttribute);
	scoreInterpretationAttribute->CompleteTypeInfo(kwcReinforcement);
	kwcReinforcement->InsertAttribute(scoreInterpretationAttribute);

	// Attribut pour le score initial
	initialScoreValueAttribute =
	    CreateReinforcementInitialScoreAttribute(kwcReinforcement, sTargetClass, scoreInterpretationAttribute);
	initialScoreValueAttribute->CompleteTypeInfo(kwcReinforcement);
	kwcReinforcement->InsertAttribute(initialScoreValueAttribute);

	// Parcours des variables explicatives a concurrence du nombre max de variables leviers, precedemment calcule
	for (nAttributeIndex = 0; nAttributeIndex < svReinforcementAttributeNames->GetSize(); nAttributeIndex++)
	{
		// Attribut pourt la variable de renforcement
		reinforcemenVariableNameAttribute = CreateReinforcementNameAtAttribute(
		    kwcReinforcement, sTargetClass, scoreInterpretationAttribute, nAttributeIndex + 1);
		reinforcemenVariableNameAttribute->CompleteTypeInfo(kwcReinforcement);
		kwcReinforcement->InsertAttribute(reinforcemenVariableNameAttribute);

		// Attribut pour la partie de variable de renforcement
		reinforcemenVariablePartAttribute = CreateReinforcementPartAtAttribute(
		    kwcReinforcement, sTargetClass, scoreInterpretationAttribute, nAttributeIndex + 1);
		reinforcemenVariablePartAttribute->CompleteTypeInfo(kwcReinforcement);
		kwcReinforcement->InsertAttribute(reinforcemenVariablePartAttribute);

		// Attribut pour le score final apres renforcement
		finalScoreValueAttribute = CreateReinforcementFinalScoreAtAttribute(
		    kwcReinforcement, sTargetClass, scoreInterpretationAttribute, nAttributeIndex + 1);
		finalScoreValueAttribute->CompleteTypeInfo(kwcReinforcement);
		kwcReinforcement->InsertAttribute(finalScoreValueAttribute);

		// Attribut pour l'indicateur de changement de classe
		classChangeAttribute = CreateReinforcementClassChangeAtAttribute(
		    kwcReinforcement, sTargetClass, scoreInterpretationAttribute, nAttributeIndex + 1);
		classChangeAttribute->CompleteTypeInfo(kwcReinforcement);
		kwcReinforcement->InsertAttribute(classChangeAttribute);
	}
	ensure(kwcReinforcement->Check());
}

KWAttribute*
KIInterpretationClassBuilder::CreateScoreReinforcementAttribute(KWClass* kwcReinforcement, const ALString& sTargetClass,
								const KWAttribute* predictorRuleAttribute,
								const KWAttribute* predictionAttribute) const
{
	KWDerivationRule* rule;
	KWAttribute* attribute;

	// Creation de la regle de derivation
	rule = new KIDRClassifierReinforcement;
	rule->SetClassName(kwcReinforcement->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(predictorRuleAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetSecondOperand()->SetAttributeName(predictionAttribute->GetName());
	rule->GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetOperandAt(2)->SetSymbolConstant((Symbol)sTargetClass);
	assert(rule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	attribute = new KWAttribute;
	attribute->SetName(kwcReinforcement->BuildAttributeName("Reinforcement_" + sTargetClass));
	attribute->SetType(KWType::Continuous);
	attribute->SetUsed(false);
	attribute->SetDerivationRule(rule);
	return attribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateReinforcementInitialScoreAttribute(
    KWClass* kwcReinforcement, const ALString& sTargetClass, const KWAttribute* scoreInterpretationAttribute) const
{
	KWDerivationRule* rule;
	KWAttribute* attribute;

	// Creation de la regle de derivation
	rule = new KIDRReinforcementInitialScore;
	rule->SetClassName(kwcReinforcement->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());

	assert(rule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	attribute = new KWAttribute;
	attribute->SetName(kwcReinforcement->BuildAttributeName("ReinforcementInitialScore_" + sTargetClass));
	attribute->SetType(KWType::Continuous);
	attribute->SetDerivationRule(rule);
	attribute->GetMetaData()->SetNoValueAt(GetReinforcementInitialScoreMetaDataKey());
	attribute->GetMetaData()->SetStringValueAt(GetTargetMetaDataKey(), sTargetClass);
	return attribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateReinforcementFinalScoreAtAttribute(
    KWClass* kwcReinforcement, const ALString& sTargetClass, const KWAttribute* scoreInterpretationAttribute,
    int nIndex) const
{

	KWDerivationRule* rule;
	KWAttribute* attribute;

	// Creation de la regle de derivation
	rule = new KIDRReinforcementFinalScoreAt;
	rule->SetClassName(kwcReinforcement->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nIndex);
	assert(rule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	attribute = new KWAttribute;
	attribute->SetName(kwcReinforcement->BuildAttributeName("ReinforcementFinalScore_" + sTargetClass + "_" +
								ALString(IntToString(nIndex))));
	attribute->SetType(KWType::Continuous);
	attribute->SetDerivationRule(rule);
	attribute->GetMetaData()->SetDoubleValueAt(GetReinforcementFinalScoreRankMetaDataKey(), nIndex);
	attribute->GetMetaData()->SetStringValueAt(GetTargetMetaDataKey(), sTargetClass);
	return attribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateReinforcementNameAtAttribute(
    KWClass* kwcReinforcement, const ALString& sTargetClass, const KWAttribute* scoreInterpretationAttribute,
    int nIndex) const
{
	KWDerivationRule* rule;
	KWAttribute* attribute;

	// Creation de la regle de derivation
	rule = new KIDRReinforcementNameAt;
	rule->SetClassName(kwcReinforcement->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nIndex);
	assert(rule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	attribute = new KWAttribute;
	attribute->SetName(kwcReinforcement->BuildAttributeName("ReinforcementVariable_" + sTargetClass + "_" +
								ALString(IntToString(nIndex))));
	attribute->SetType(KWType::Symbol);
	attribute->GetMetaData()->SetDoubleValueAt(GetReinforcementAttributeRankMetaDataKey(), nIndex);
	attribute->GetMetaData()->SetStringValueAt(GetTargetMetaDataKey(), sTargetClass);
	attribute->SetDerivationRule(rule);
	return attribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateReinforcementPartAtAttribute(
    KWClass* kwcReinforcement, const ALString& sTargetClass, const KWAttribute* scoreInterpretationAttribute,
    int nIndex) const
{
	KWDerivationRule* rule;
	KWAttribute* attribute;

	// Creation de la regle de derivation
	rule = new KIDRReinforcementPartitionAt;
	rule->SetClassName(kwcReinforcement->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nIndex);
	assert(rule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	attribute = new KWAttribute;
	attribute->SetName(kwcReinforcement->BuildAttributeName("ReinforcementPart_" + sTargetClass + "_" +
								ALString(IntToString(nIndex))));
	attribute->SetType(KWType::Symbol);
	attribute->GetMetaData()->SetDoubleValueAt(GetReinforcementPartRankMetaDataKey(), nIndex);
	attribute->GetMetaData()->SetStringValueAt(GetTargetMetaDataKey(), sTargetClass);
	attribute->SetDerivationRule(rule);
	return attribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateReinforcementClassChangeAtAttribute(
    KWClass* kwcReinforcement, const ALString& sTargetClass, const KWAttribute* scoreInterpretationAttribute,
    int nIndex) const
{
	KWDerivationRule* rule;
	KWAttribute* attribute;

	// Creation de la regle de derivation
	rule = new KIDRReinforcementClassChangeTagAt;
	rule->SetClassName(kwcReinforcement->GetName());
	rule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	rule->GetFirstOperand()->SetAttributeName(scoreInterpretationAttribute->GetName());
	rule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	rule->GetSecondOperand()->SetContinuousConstant(nIndex);
	assert(rule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	attribute = new KWAttribute;
	attribute->SetName(kwcReinforcement->BuildAttributeName("ReinforcementClassChangeTag_" + sTargetClass + "_" +
								ALString(IntToString(nIndex))));
	attribute->SetType(KWType::Continuous);
	attribute->SetDerivationRule(rule);
	attribute->GetMetaData()->SetDoubleValueAt(GetReinforcementClassChangeTagRankMetaDataKey(), nIndex);
	attribute->GetMetaData()->SetStringValueAt(GetTargetMetaDataKey(), sTargetClass);
	return attribute;
}

const SymbolVector* KIInterpretationClassBuilder::GetTargetValues() const
{
	return &svTargetValues;
}

int KIInterpretationClassBuilder::GetPredictorAttributeNumber() const
{
	return svPredictorAttributeNames.GetSize();
}

const StringVector* KIInterpretationClassBuilder::GetPredictorAttributeNames() const
{
	return &svPredictorAttributeNames;
}

const StringVector* KIInterpretationClassBuilder::GetPredictorPartitionedAttributeNames() const
{
	return &svPredictorPartitionedAttributeNames;
}

KWClass* KIInterpretationClassBuilder::BuildInterpretationClass(const KIModelInterpreter* modelInterpreterSpec) const
{
	KWClass* kwcInterpretationClass;
	KWAttribute* predictorRuleAttribute;
	KWAttribute* predictionAttribute;
	SymbolVector svInterpretedTargetValues;
	int nContributionAttributeNumber;
	StringVector svInterpretationMetaDataKeys;
	int i;

	require(IsPredictorImported());
	require(GetPredictorAttributeNumber() > 0);
	require(modelInterpreterSpec != NULL);
	require(modelInterpreterSpec->GetContributionAttributeNumber() > 0);
	require(modelInterpreterSpec->GetShapleyValueRanking() == "Global" or
		modelInterpreterSpec->GetShapleyValueRanking() == "Individual");

	// Collecte des meta-data dediees a l'interpretation
	svInterpretationMetaDataKeys.Add(GetIntepreterMetaDataKey());
	svInterpretationMetaDataKeys.Add(GetShapleyValueRankingMetaDataKey());
	svInterpretationMetaDataKeys.Add(GetContributionAttributeMetaDataKey());
	svInterpretationMetaDataKeys.Add(GetContributionAttributeRankMetaDataKey());
	svInterpretationMetaDataKeys.Add(GetContributionPartRankMetaDataKey());
	svInterpretationMetaDataKeys.Add(GetContributionValueRankMetaDataKey());

	// Construction d'un dictionnaire d'interpretation
	kwcInterpretationClass = BuildInterpretationServiceClass("Interpretation", &svInterpretationMetaDataKeys);

	// Recherche des attributs du predicteur et de prediction
	predictorRuleAttribute = kwcInterpretationClass->LookupAttribute(sPredictorRuleAttributeName);
	predictionAttribute = kwcInterpretationClass->LookupAttribute(sPredictionAttributeName);
	assert(predictorRuleAttribute != NULL);
	assert(predictionAttribute != NULL);

	// Meta-data sur le dictionnaire d'interpretation
	kwcInterpretationClass->GetMetaData()->SetNoValueAt(GetIntepreterMetaDataKey());
	kwcInterpretationClass->GetMetaData()->SetStringValueAt(GetShapleyValueRankingMetaDataKey(),
								modelInterpreterSpec->GetShapleyValueRanking());

	// Nombre d'attribut du predicteur a interpreter
	nContributionAttributeNumber =
	    min(GetPredictorAttributeNumber(), modelInterpreterSpec->GetContributionAttributeNumber());
	assert(nContributionAttributeNumber > 0);

	// On interprete toutes les classes, selon la specification actuelle de l'interpreteur
	svInterpretedTargetValues.CopyFrom(GetTargetValues());
	for (i = 0; i < svInterpretedTargetValues.GetSize(); i++)
	{
		// Creation des attributs d'interpretation pour la valeur cible
		CreateContributionAttributesForClass(
		    kwcInterpretationClass, svInterpretedTargetValues.GetAt(i).GetValue(), predictorRuleAttribute,
		    predictionAttribute, modelInterpreterSpec->GetShapleyValueRanking() == "Global",
		    nContributionAttributeNumber);
	}
	return kwcInterpretationClass;
}

KWClass* KIInterpretationClassBuilder::BuildReinforcementClass(const KIModelReinforcer* modelReinforcerSpec) const
{
	KWClass* kwcReinforcementClass;
	KWAttribute* predictorRuleAttribute;
	KWAttribute* predictionAttribute;
	KWAttribute* leverAttribute;
	SymbolVector svReinforcedTargetValues;
	StringVector svReinforcementMetaDataKeys;
	StringVector svReinforcementAttributeNames;
	KIPredictorAttribute* predictorAttribute;
	int i;

	require(IsPredictorImported());
	require(GetPredictorAttributeNumber() > 0);
	require(modelReinforcerSpec != NULL);
	require(modelReinforcerSpec->ComputeSelectedLeverAttributeNumber() > 0);

	// Collecte des meta-data dediees au renforcement
	svReinforcementMetaDataKeys.Add(GetReinforcerMetaDataKey());
	svReinforcementMetaDataKeys.Add(GetReinforcementClassMetaDataKey());
	svReinforcementMetaDataKeys.Add(GetLeverAttributeMetaDataKey());
	svReinforcementMetaDataKeys.Add(GetReinforcementInitialScoreMetaDataKey());
	svReinforcementMetaDataKeys.Add(GetReinforcementAttributeRankMetaDataKey());
	svReinforcementMetaDataKeys.Add(GetReinforcementPartRankMetaDataKey());
	svReinforcementMetaDataKeys.Add(GetReinforcementFinalScoreRankMetaDataKey());
	svReinforcementMetaDataKeys.Add(GetReinforcementClassChangeTagRankMetaDataKey());

	// Construction d'un dictionnaire de renforcement
	kwcReinforcementClass = BuildInterpretationServiceClass("Reinforcement", &svReinforcementMetaDataKeys);

	// Recherche des attributs du predicteur et de prediction
	predictorRuleAttribute = kwcReinforcementClass->LookupAttribute(sPredictorRuleAttributeName);
	predictionAttribute = kwcReinforcementClass->LookupAttribute(sPredictionAttributeName);
	assert(predictorRuleAttribute != NULL);
	assert(predictionAttribute != NULL);

	// Meta-data sur le dictionnaire d'interpretation
	kwcReinforcementClass->GetMetaData()->SetNoValueAt(GetReinforcerMetaDataKey());
	kwcReinforcementClass->GetMetaData()->SetStringValueAt(GetReinforcementClassMetaDataKey(),
							       modelReinforcerSpec->GetReinforcedTargetValue());

	// Collecte des attributs a renforcer
	for (i = 0; i < modelReinforcerSpec->GeConstLeverAttributes()->GetSize(); i++)
	{
		predictorAttribute =
		    cast(KIPredictorAttribute*, modelReinforcerSpec->GeConstLeverAttributes()->GetAt(i));

		// Gestion des variables levier
		if (predictorAttribute->GetUsed())
		{
			// Memorisation du nom
			svReinforcementAttributeNames.Add(predictorAttribute->GetName());

			// Parametrage de la meta-donne de la variable levier
			leverAttribute = kwcReinforcementClass->LookupAttribute(predictorAttribute->GetName());
			assert(leverAttribute != NULL);
			leverAttribute->GetMetaData()->SetNoValueAt(GetLeverAttributeMetaDataKey());
		}
	}
	assert(svReinforcementAttributeNames.GetSize() > 0);

	// On interprete uniquement la classe a renforcer, selon la specification actuelle du renforcer
	svReinforcedTargetValues.Add((Symbol)modelReinforcerSpec->GetReinforcedTargetValue());
	for (i = 0; i < svReinforcedTargetValues.GetSize(); i++)
	{
		// Creation des attributs d'interpretation pour la valeur cible
		CreateReinforcementAttributesForClass(
		    kwcReinforcementClass, svReinforcedTargetValues.GetAt(i).GetValue(), predictorRuleAttribute,
		    predictionAttribute, &svReinforcementAttributeNames);
	}
	return kwcReinforcementClass;
}

const ALString& KIInterpretationClassBuilder::GetIntepreterMetaDataKey()
{
	static const ALString sMetaDataKey = "InterpreterDictionary";
	return sMetaDataKey;
}

const ALString& KIInterpretationClassBuilder::GetShapleyValueRankingMetaDataKey()
{
	static const ALString sMetaDataKey = "ShapleyValueRanking";
	return sMetaDataKey;
}

const ALString& KIInterpretationClassBuilder::GetContributionAttributeMetaDataKey()
{
	static const ALString sMetaDataKey = "ContributionVariable";
	return sMetaDataKey;
}

const ALString& KIInterpretationClassBuilder::GetContributionAttributeRankMetaDataKey()
{
	static const ALString sMetaDataKey = "ContributionVariableRank";
	return sMetaDataKey;
}

const ALString& KIInterpretationClassBuilder::GetContributionPartRankMetaDataKey()
{
	static const ALString sMetaDataKey = "ContributionPartRank";
	return sMetaDataKey;
}

const ALString& KIInterpretationClassBuilder::GetContributionValueRankMetaDataKey()
{
	static const ALString sMetaDataKey = "ContributionValueRank";
	return sMetaDataKey;
}

const ALString& KIInterpretationClassBuilder::GetReinforcerMetaDataKey()
{
	static const ALString sMetaDataKey = "ReinforcerDictionary";
	return sMetaDataKey;
}

const ALString& KIInterpretationClassBuilder::GetReinforcementClassMetaDataKey()
{
	static const ALString sMetaDataKey = "ReinforcementClass";
	return sMetaDataKey;
}

const ALString& KIInterpretationClassBuilder::GetLeverAttributeMetaDataKey()
{
	static const ALString sMetaDataKey = "LeverVariable";
	return sMetaDataKey;
}

const ALString& KIInterpretationClassBuilder::GetReinforcementInitialScoreMetaDataKey()
{
	static const ALString sMetaDataKey = "ReinforcementInitialScore";
	return sMetaDataKey;
}

const ALString& KIInterpretationClassBuilder::GetReinforcementAttributeRankMetaDataKey()
{
	static const ALString sMetaDataKey = "ReinforcementVariableRank";
	return sMetaDataKey;
}

const ALString& KIInterpretationClassBuilder::GetReinforcementPartRankMetaDataKey()
{
	static const ALString sMetaDataKey = "ReinforcementPartRank";
	return sMetaDataKey;
}

const ALString& KIInterpretationClassBuilder::GetReinforcementFinalScoreRankMetaDataKey()
{
	static const ALString sMetaDataKey = "ReinforcementFinalScoreRank";
	return sMetaDataKey;
}

const ALString& KIInterpretationClassBuilder::GetReinforcementClassChangeTagRankMetaDataKey()
{
	static const ALString sMetaDataKey = "ReinforcementClassChangeTagRank";
	return sMetaDataKey;
}

const ALString& KIInterpretationClassBuilder::GetTargetMetaDataKey()
{
	static const ALString sMetaDataKey = "Target";
	return sMetaDataKey;
}

const ALString& KIInterpretationClassBuilder::GetShapleyLabel()
{
	static const ALString sMetaDataKey = "Shapley";
	return sMetaDataKey;
}
