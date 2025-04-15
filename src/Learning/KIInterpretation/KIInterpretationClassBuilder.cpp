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
			classifierRule->ExportAttributeNames(kwcInputPredictor, &svPredictorAttributeNames,
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

void KIInterpretationClassBuilder::CreateInterpretationAttributes(KWClass* kwcInterpretationClass,
								  const KWAttribute* predictorRuleAttribute,
								  const SymbolVector* svInterpretedTargetValues,
								  boolean bIsGlobalRanking,
								  int nContributionAttributeNumber) const
{
	KWAttribute* interpreterAttribute;
	ObjectArray oaPredictorAttributes;
	KIPredictorAttribute* predictorAttribute;
	int nTarget;
	int nAttribute;
	Symbol sTargetValue;

	require(kwcInterpretationClass != NULL);
	require(predictorRuleAttribute != NULL);
	require(svInterpretedTargetValues != NULL);
	require(svInterpretedTargetValues->GetSize() > 0);
	require(0 < nContributionAttributeNumber and nContributionAttributeNumber <= GetPredictorAttributeNumber());

	// Creation de l'attribut gerant les interpretations
	interpreterAttribute = CreateInterpreterAttribute(kwcInterpretationClass, predictorRuleAttribute);
	interpreterAttribute->SetUsed(false);

	// Creation des attributs de contribution dans le cas d'un ranking global
	if (bIsGlobalRanking)
	{
		// Creation de la liste des attributs du predicteur, tries par importance decroissante
		BuildPredictorAttributes(&oaPredictorAttributes);

		// Parcours de classe cibles
		for (nTarget = 0; nTarget < svInterpretedTargetValues->GetSize(); nTarget++)
		{
			sTargetValue = svInterpretedTargetValues->GetAt(nTarget);

			// Parcours des attributs
			for (nAttribute = 0; nAttribute < nContributionAttributeNumber; nAttribute++)
			{
				predictorAttribute =
				    cast(KIPredictorAttribute*, oaPredictorAttributes.GetAt(nAttribute));
				CreateContributionAttribute(
				    kwcInterpretationClass, interpreterAttribute, new KIDRContributionAt, sTargetValue,
				    predictorAttribute->GetName(), GetContributionAttributeMetaDataKey());
			}
		}

		// Nettoyage
		oaPredictorAttributes.DeleteAll();
	}
	// Creation des attributs de contribution dans le cas d'un ranking individuel
	else
	{
		// Parcours de classe cibles
		for (nTarget = 0; nTarget < svInterpretedTargetValues->GetSize(); nTarget++)
		{
			sTargetValue = svInterpretedTargetValues->GetAt(nTarget);

			// Parcours des attributs
			for (nAttribute = 0; nAttribute < nContributionAttributeNumber; nAttribute++)
			{
				// Creation des attributs pour les services par rang
				CreateRankedContributionAttribute(
				    kwcInterpretationClass, interpreterAttribute, new KIDRContributionAttributeAt,
				    "Variable", sTargetValue, nAttribute, GetContributionAttributeRankMetaDataKey());
				CreateRankedContributionAttribute(kwcInterpretationClass, interpreterAttribute,
								  new KIDRContributionPartAt, "Part", sTargetValue,
								  nAttribute, GetContributionPartRankMetaDataKey());
				CreateRankedContributionAttribute(kwcInterpretationClass, interpreterAttribute,
								  new KIDRContributionValueAt, "Value", sTargetValue,
								  nAttribute, GetContributionValueRankMetaDataKey());
			}
		}
	}
}

KWAttribute* KIInterpretationClassBuilder::CreateInterpreterAttribute(KWClass* kwcInterpretationClass,
								      const KWAttribute* predictorRuleAttribute) const
{
	KWAttribute* interpreterAttribute;
	KIDRClassifierInterpreter* interpreterRule;
	ALString sValue;

	require(kwcInterpretationClass != NULL);
	require(predictorRuleAttribute != NULL);

	// Creation de la regle de derivation
	interpreterRule = new KIDRClassifierInterpreter;
	interpreterRule->SetClassName(kwcInterpretationClass->GetName());
	interpreterRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	interpreterRule->GetFirstOperand()->SetAttributeName(predictorRuleAttribute->GetName());
	assert(interpreterRule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	interpreterAttribute = new KWAttribute;
	interpreterAttribute->SetName(
	    kwcInterpretationClass->BuildAttributeName("Interpreter_" + predictorRuleAttribute->GetName()));
	interpreterAttribute->SetDerivationRule(interpreterRule);
	interpreterAttribute->SetType(interpreterRule->GetType());
	interpreterAttribute->SetStructureName(interpreterRule->GetStructureName());
	kwcInterpretationClass->InsertAttribute(interpreterAttribute);
	return interpreterAttribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateContributionAttribute(
    KWClass* kwcInterpretationClass, const KWAttribute* interpreterAttribute, KWDerivationRule* kwdrContributionRule,
    Symbol sTargetValue, const ALString& sAttributeName, const ALString& sAttributeMetaDataKey) const
{
	KWAttribute* contributionAttribute;

	require(kwcInterpretationClass != NULL);
	require(interpreterAttribute != NULL);
	require(kwdrContributionRule != NULL);
	require(sAttributeName != "");
	require(sAttributeMetaDataKey != "");

	// Parametrage de la regle
	kwdrContributionRule->SetClassName(kwcInterpretationClass->GetName());
	kwdrContributionRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	kwdrContributionRule->GetFirstOperand()->SetAttributeName(interpreterAttribute->GetName());
	kwdrContributionRule->GetOperandAt(1)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	kwdrContributionRule->GetOperandAt(1)->SetSymbolConstant(sTargetValue);
	kwdrContributionRule->GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	kwdrContributionRule->GetOperandAt(2)->SetSymbolConstant((Symbol)sAttributeName);
	assert(kwdrContributionRule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	contributionAttribute = new KWAttribute;
	contributionAttribute->SetDerivationRule(kwdrContributionRule);
	contributionAttribute->SetType(kwdrContributionRule->GetType());
	contributionAttribute->SetName(
	    kwcInterpretationClass->BuildAttributeName(GetShapleyLabel() + "_" + sTargetValue + "_" + sAttributeName));
	contributionAttribute->SetType(KWType::Continuous);
	contributionAttribute->GetMetaData()->SetStringValueAt(sAttributeMetaDataKey, sAttributeName);
	contributionAttribute->GetMetaData()->SetStringValueAt(GetTargetMetaDataKey(), sTargetValue.GetValue());
	kwcInterpretationClass->InsertAttribute(contributionAttribute);
	return contributionAttribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateRankedContributionAttribute(
    KWClass* kwcInterpretationClass, const KWAttribute* interpreterAttribute,
    KWDerivationRule* kwdrRankedContributionRule, const ALString& sBaseName, Symbol sTargetValue, int nRank,
    const ALString& sRankMetaDataKey) const
{
	KWAttribute* contributionAttribute;

	require(kwcInterpretationClass != NULL);
	require(interpreterAttribute != NULL);
	require(kwdrRankedContributionRule != NULL);
	require(sBaseName != "");
	require(nRank >= 0);
	require(sRankMetaDataKey != "");

	// Parametrage de la regle
	kwdrRankedContributionRule->SetClassName(kwcInterpretationClass->GetName());
	kwdrRankedContributionRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	kwdrRankedContributionRule->GetFirstOperand()->SetAttributeName(interpreterAttribute->GetName());
	kwdrRankedContributionRule->GetOperandAt(1)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	kwdrRankedContributionRule->GetOperandAt(1)->SetSymbolConstant(sTargetValue);
	kwdrRankedContributionRule->GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	kwdrRankedContributionRule->GetOperandAt(2)->SetContinuousConstant(nRank + 1);
	assert(kwdrRankedContributionRule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	contributionAttribute = new KWAttribute;
	contributionAttribute->SetDerivationRule(kwdrRankedContributionRule);
	contributionAttribute->SetType(kwdrRankedContributionRule->GetType());
	contributionAttribute->SetName(kwcInterpretationClass->BuildAttributeName(
	    GetShapleyLabel() + sBaseName + "_" + sTargetValue + "_" + IntToString(nRank + 1)));
	contributionAttribute->GetMetaData()->SetDoubleValueAt(sRankMetaDataKey, nRank + 1);
	contributionAttribute->GetMetaData()->SetStringValueAt(GetTargetMetaDataKey(), sTargetValue.GetValue());
	kwcInterpretationClass->InsertAttribute(contributionAttribute);
	return contributionAttribute;
}

void KIInterpretationClassBuilder::CreateReinforcementAttributes(KWClass* kwcReinforcementClass,
								 const KWAttribute* predictorRuleAttribute,
								 const SymbolVector* svReinforcedTargetValues,
								 const StringVector* svReinforcementAttributes) const
{
	KWAttribute* reinforcerAttribute;
	int nTarget;
	int nAttribute;
	Symbol sTargetValue;

	require(kwcReinforcementClass != NULL);
	require(predictorRuleAttribute != NULL);
	require(svReinforcedTargetValues != NULL);
	require(svReinforcedTargetValues->GetSize() > 0);
	require(svReinforcementAttributes != NULL);
	require(svReinforcementAttributes->GetSize() > 0);

	// Creation de l'attribut gerant les interpretations
	reinforcerAttribute =
	    CreateReinforcerAttribute(kwcReinforcementClass, predictorRuleAttribute, svReinforcementAttributes);
	reinforcerAttribute->SetUsed(false);

	// Creation des attributs de renforcement dans le cas d'un ranking individuel
	// Parcours de classe cibles
	for (nTarget = 0; nTarget < svReinforcedTargetValues->GetSize(); nTarget++)
	{
		sTargetValue = svReinforcedTargetValues->GetAt(nTarget);

		// Creation de l'attribut du score initial, qui ne depend ps de l'attribut de renforcement
		CreateRankedReinforcementAttribute(kwcReinforcementClass, reinforcerAttribute,
						   new KIDRReinforcementInitialScoreAt, "InitialScore", sTargetValue,
						   -1, GetReinforcementInitialScoreMetaDataKey());

		// Parcours selon le nombre d'attribut de renforcement
		for (nAttribute = 0; nAttribute < svReinforcementAttributes->GetSize(); nAttribute++)
		{
			// Creation des attributs pour les services par rang
			CreateRankedReinforcementAttribute(kwcReinforcementClass, reinforcerAttribute,
							   new KIDRReinforcementAttributeAt, "Variable", sTargetValue,
							   nAttribute, GetReinforcementAttributeRankMetaDataKey());
			CreateRankedReinforcementAttribute(kwcReinforcementClass, reinforcerAttribute,
							   new KIDRReinforcementPartAt, "Part", sTargetValue,
							   nAttribute, GetReinforcementPartRankMetaDataKey());
			CreateRankedReinforcementAttribute(
			    kwcReinforcementClass, reinforcerAttribute, new KIDRReinforcementFinalScoreAt, "FinalScore",
			    sTargetValue, nAttribute, GetReinforcementFinalScoreRankMetaDataKey());
			CreateRankedReinforcementAttribute(kwcReinforcementClass, reinforcerAttribute,
							   new KIDRReinforcementClassChangeTagAt, "ClassChangeTag",
							   sTargetValue, nAttribute,
							   GetReinforcementClassChangeTagRankMetaDataKey());
		}
	}
}

KWAttribute*
KIInterpretationClassBuilder::CreateReinforcerAttribute(KWClass* kwcReinforcementClass,
							const KWAttribute* predictorRuleAttribute,
							const StringVector* svReinforcementAttributes) const
{
	KWAttribute* reinforcerAttribute;
	KIDRClassifierReinforcer* reinforcerRule;
	KWDRSymbolVector* reinforcementAttributesRule;
	int nAttribute;
	ALString sValue;

	require(kwcReinforcementClass != NULL);
	require(predictorRuleAttribute != NULL);
	require(svReinforcementAttributes != NULL);
	require(svReinforcementAttributes->GetSize() > 0);

	// Creation d'une regle de derivation pour les noms des attributs de renforcement
	reinforcementAttributesRule = new KWDRSymbolVector;
	reinforcementAttributesRule->SetValueNumber(svReinforcementAttributes->GetSize());
	for (nAttribute = 0; nAttribute < svReinforcementAttributes->GetSize(); nAttribute++)
		reinforcementAttributesRule->SetValueAt(nAttribute,
							(Symbol)svReinforcementAttributes->GetAt(nAttribute));

	// Creation de la regle de derivation
	reinforcerRule = new KIDRClassifierReinforcer;
	reinforcerRule->SetClassName(kwcReinforcementClass->GetName());
	reinforcerRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	reinforcerRule->GetFirstOperand()->SetAttributeName(predictorRuleAttribute->GetName());
	reinforcerRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
	reinforcerRule->GetSecondOperand()->SetDerivationRule(reinforcementAttributesRule);
	assert(reinforcerRule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	reinforcerAttribute = new KWAttribute;
	reinforcerAttribute->SetName(
	    kwcReinforcementClass->BuildAttributeName("Reinforcer_" + predictorRuleAttribute->GetName()));
	reinforcerAttribute->SetDerivationRule(reinforcerRule);
	reinforcerAttribute->SetType(reinforcerRule->GetType());
	reinforcerAttribute->SetStructureName(reinforcerRule->GetStructureName());
	kwcReinforcementClass->InsertAttribute(reinforcerAttribute);
	return reinforcerAttribute;
}

KWAttribute* KIInterpretationClassBuilder::CreateRankedReinforcementAttribute(
    KWClass* kwcReinforcementClass, const KWAttribute* reinforcerAttribute,
    KWDerivationRule* kwdrRankedReinforcementRule, const ALString& sBaseName, Symbol sTargetValue, int nRank,
    const ALString& sRankMetaDataKey) const
{
	KWAttribute* reinforcementAttribute;
	ALString sAttributeName;

	require(kwcReinforcementClass != NULL);
	require(reinforcerAttribute != NULL);
	require(kwdrRankedReinforcementRule != NULL);
	require(sBaseName != "");
	require(nRank >= -1);
	require(sRankMetaDataKey != "");

	// Parametrage de la regle
	kwdrRankedReinforcementRule->SetClassName(kwcReinforcementClass->GetName());
	kwdrRankedReinforcementRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	kwdrRankedReinforcementRule->GetFirstOperand()->SetAttributeName(reinforcerAttribute->GetName());
	kwdrRankedReinforcementRule->GetOperandAt(1)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
	kwdrRankedReinforcementRule->GetOperandAt(1)->SetSymbolConstant(sTargetValue);
	if (nRank >= 0)
	{
		kwdrRankedReinforcementRule->GetOperandAt(2)->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		kwdrRankedReinforcementRule->GetOperandAt(2)->SetContinuousConstant(nRank + 1);
	}
	assert(kwdrRankedReinforcementRule->Check());

	// Creation de l'attribut, et affectation de la regle de derivation
	reinforcementAttribute = new KWAttribute;
	reinforcementAttribute->SetDerivationRule(kwdrRankedReinforcementRule);
	reinforcementAttribute->SetType(kwdrRankedReinforcementRule->GetType());
	sAttributeName = GetReinforcementLabel() + sBaseName + "_" + sTargetValue;
	if (nRank >= 0)
		sAttributeName = sAttributeName + "_" + IntToString(nRank + 1);
	reinforcementAttribute->SetName(kwcReinforcementClass->BuildAttributeName(sAttributeName));
	if (nRank >= 0)
		reinforcementAttribute->GetMetaData()->SetDoubleValueAt(sRankMetaDataKey, nRank + 1);
	reinforcementAttribute->GetMetaData()->SetStringValueAt(GetTargetMetaDataKey(), sTargetValue.GetValue());
	kwcReinforcementClass->InsertAttribute(reinforcementAttribute);
	return reinforcementAttribute;
}

const SymbolVector* KIInterpretationClassBuilder::GetTargetValues() const
{
	require(IsPredictorImported());
	return &svTargetValues;
}

int KIInterpretationClassBuilder::GetPredictorAttributeNumber() const
{
	require(IsPredictorImported());
	return svPredictorAttributeNames.GetSize();
}

const StringVector* KIInterpretationClassBuilder::GetPredictorAttributeNames() const
{
	require(IsPredictorImported());
	return &svPredictorAttributeNames;
}

const StringVector* KIInterpretationClassBuilder::GetPredictorPartitionedAttributeNames() const
{
	require(IsPredictorImported());
	return &svPredictorPartitionedAttributeNames;
}

void KIInterpretationClassBuilder::BuildPredictorAttributes(ObjectArray* oaPredictorAttributes) const
{
	KIPredictorAttribute* predictorAttribute;
	KWAttribute* attribute;
	double dLevel;
	double dWeight;
	double dImportance;
	int i;

	require(IsPredictorImported());
	require(oaPredictorAttributes != NULL);
	require(oaPredictorAttributes->GetSize() == 0);

	// Alimentation a partir des specification disponible dans le ClassBuilder
	for (i = 0; i < GetPredictorAttributeNumber(); i++)
	{
		// Ajout d'une variable au tableau
		predictorAttribute = new KIPredictorAttribute;
		oaPredictorAttributes->Add(predictorAttribute);

		// Specification de la variable
		attribute = GetPredictorClass()->LookupAttribute(GetPredictorAttributeNames()->GetAt(i));
		assert(attribute != NULL);
		predictorAttribute->SetType(KWType::ToString(attribute->GetType()));
		predictorAttribute->SetName(attribute->GetName());

		// Recherche de l'importance via les meta-data, en se protegeant contre les meta-data erronnees
		if (attribute->GetConstMetaData()->IsKeyPresent(
			SNBPredictorSelectiveNaiveBayes::GetImportanceMetaDataKey()))
		{
			dImportance = attribute->GetConstMetaData()->GetDoubleValueAt(
			    SNBPredictorSelectiveNaiveBayes::GetImportanceMetaDataKey());
			dImportance = max(dImportance, (double)0);
			dImportance = min(dImportance, (double)1);
		}
		// Recherche a partir du Level et du Weight si Importance non trouve
		else
		{
			dLevel = attribute->GetConstMetaData()->GetDoubleValueAt(
			    KWDataPreparationAttribute::GetLevelMetaDataKey());
			dLevel = max(dLevel, (double)0);
			dLevel = min(dLevel, (double)1);
			dWeight = attribute->GetConstMetaData()->GetDoubleValueAt(
			    SNBPredictorSelectiveNaiveBayes::GetWeightMetaDataKey());
			dLevel = max(dLevel, (double)0);
			dLevel = min(dLevel, (double)1);
			dImportance = sqrt(dLevel * dWeight);
		}
		predictorAttribute->SetImportance(KWContinuous::DoubleToContinuous(dImportance));
	}

	// Tri par importance decroissante
	oaPredictorAttributes->SetCompareFunction(KIPredictorAttributeCompareImportance);
	oaPredictorAttributes->Sort();
}

KWClass* KIInterpretationClassBuilder::BuildInterpretationClass(const KIModelInterpreter* modelInterpreterSpec) const
{
	KWClass* kwcInterpretationClass;
	KWAttribute* predictorRuleAttribute;
	KWAttribute* predictionAttribute;
	SymbolVector svInterpretedTargetValues;
	int nContributionAttributeNumber;
	StringVector svInterpretationMetaDataKeys;

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

	// Creation des attributs d'interpretation
	CreateInterpretationAttributes(kwcInterpretationClass, predictorRuleAttribute, &svInterpretedTargetValues,
				       modelInterpreterSpec->GetShapleyValueRanking() == "Global",
				       nContributionAttributeNumber);
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
	for (i = 0; i < modelReinforcerSpec->GetConstLeverAttributes()->GetSize(); i++)
	{
		predictorAttribute =
		    cast(KIPredictorAttribute*, modelReinforcerSpec->GetConstLeverAttributes()->GetAt(i));

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

	// Creation des attributs de renforcement
	svReinforcedTargetValues.Add((Symbol)modelReinforcerSpec->GetReinforcedTargetValue());
	CreateReinforcementAttributes(kwcReinforcementClass, predictorRuleAttribute, &svReinforcedTargetValues,
				      &svReinforcementAttributeNames);
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

const ALString& KIInterpretationClassBuilder::GetReinforcementLabel()
{
	static const ALString sMetaDataKey = "Reinforcement";
	return sMetaDataKey;
}
