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

boolean KIInterpretationClassBuilder::ImportPredictor(const KWClass* kwcInputPredictor)
{
	boolean bOk;
	const boolean bTrace = false;
	const KWDRNBClassifier referenceNBRule;
	const KWDRSNBClassifier referenceSNBRule;
	const KWDRDataGrid referenceDataGridRule;
	KWTrainedClassifier trainedClassifier;
	KWTrainedRegressor trainedRegressor;
	KWAttribute* predictionAttribute;
	KWAttribute* attribute;
	KWAttribute* dataGridAttribute;
	KWDRDataGrid* dataGridRule;
	KWDRNBClassifier* classifierRule;
	KWDRDataGridStats* dataGridStatsRule;
	boolean bIsClassifier;
	int nClassifierPairNumber;
	KWClass* kwcShadowClass;
	ALString sDefaultPairName;
	int i;
	ALString sTmp;

	require(kwcInputPredictor != NULL);
	require(kwcInputPredictor->IsCompiled());
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
		AddPredictorWarning(kwcInputPredictor,
				    "Interpretation services not available for data grid predictors");
		bIsClassifier = false;
	}

	// On exclut le cas d'un dictionnaire d'interpretation
	if (bIsClassifier and kwcInputPredictor->GetConstMetaData()->IsKeyPresent(GetIntepreterMetaDataKey()))
	{
		AddPredictorWarning(kwcInputPredictor, "Already used as an interpretation dictionary");
		bIsClassifier = false;
	}

	// On exclut le cas d'un dictionnaire de renforcement
	if (bIsClassifier and kwcInputPredictor->GetConstMetaData()->IsKeyPresent(GetReinforcerMetaDataKey()))
	{
		AddPredictorWarning(kwcInputPredictor, "Already used as a reinforcement dictionary");
		bIsClassifier = false;
	}

	// Message d'erreur specifique pour le cas des regresseurs, non geres actuellement
	if (not bIsClassifier)
	{
		if (trainedRegressor.ImportPredictorClass(kwcInputPredictor))
		{
			AddPredictorWarning(kwcInputPredictor, "Interpretation services not available for regressors");
		}
		trainedRegressor.DeletePredictor();
	}

	// Cas ou le dictionnaire est bien celui d'un classifieur pouvant etre interprete
	predictionAttribute = NULL;
	if (bIsClassifier)
	{
		// Collecte des valeurs cibles
		for (i = 0; i < trainedClassifier.GetTargetValueNumber(); i++)
			svTargetValues.Add(trainedClassifier.GetTargetValueAt(i));

		// Erreur s'il ny a pas au moins deux classe
		if (svTargetValues.GetSize() <= 1)
		{
			AddPredictorError(kwcInputPredictor, sTmp + "Invalid classifier with " +
								 IntToString(svTargetValues.GetSize()) +
								 " target value");
			bIsClassifier = false;
		}

		// Extraction de l'attribut de prediction
		predictionAttribute = trainedClassifier.GetPredictionAttribute();
		if (predictionAttribute->GetDerivationRule() == NULL)
		{
			AddPredictorError(kwcInputPredictor,
					  sTmp + "Invalid classifier with incorrect prediction variable " +
					      predictionAttribute->GetName() + " (missing derivation rule)");
			bIsClassifier = false;
		}
		else if (predictionAttribute->GetDerivationRule()->GetOperandNumber() == 0)
		{
			AddPredictorError(kwcInputPredictor,
					  sTmp + "Invalid classifier with incorrect prediction variable " +
					      predictionAttribute->GetName() +
					      " (use a derivation rule without any operand)");
			bIsClassifier = false;
		}
		sPredictionAttributeName = predictionAttribute->GetName();

		// L'attribut de prediction doit avoir en premier operande un attribut de type Structure
		if (bIsClassifier and
		    (predictionAttribute->GetDerivationRule()->GetFirstOperand()->GetType() != KWType::Structure or
		     predictionAttribute->GetDerivationRule()->GetFirstOperand()->GetOrigin() !=
			 KWDerivationRuleOperand::OriginAttribute))
		{
			AddPredictorError(kwcInputPredictor,
					  sTmp + "Invalid classifier with incorrect prediction variable " +
					      predictionAttribute->GetName() +
					      " (first operand should be a variable of type Structure(Classifier))");
			bIsClassifier = false;
		}
	}
	// Recherche de l'attribut decrivant le predicteur
	classifierRule = NULL;
	if (bIsClassifier)
	{
		// Extraction de l'attribut Structure qui decrit le classifier
		// Cet attribut doit avoir une regle de derivation dont le nom est celui
		// d'une regle NB ou SNB
		attribute = kwcInputPredictor->LookupAttribute(
		    predictionAttribute->GetDerivationRule()->GetFirstOperand()->GetAttributeName());

		// L'attribut doit exister dans la classe
		if (attribute == NULL)
		{
			AddPredictorError(
			    kwcInputPredictor,
			    sTmp + "Invalid classifier with missing classifier variable " +
				predictionAttribute->GetDerivationRule()->GetFirstOperand()->GetAttributeName());
			bIsClassifier = false;
		}
		// Le predicteur est specifie via une regle de derivation
		else if (attribute->GetDerivationRule() == NULL)
		{
			AddPredictorError(kwcInputPredictor,
					  sTmp + "Invalid classifier with incorrect classifier variable " +
					      attribute->GetName() + " (missing derivation rule)");

			bIsClassifier = false;
		}
		// Il doit etre soit le Naive Bayes, soit le Selective Naive Bayes
		else
		{
			// Test du type de classifieur
			if (attribute->GetDerivationRule()->GetName() != referenceNBRule.GetName() and
			    attribute->GetDerivationRule()->GetName() != referenceSNBRule.GetName())
			{
				Global::AddWarning(
				    "Dictionary", kwcInputPredictor->GetName(),
				    "Interpretation services are available only for naive bayes predictors");
				bIsClassifier = false;
			}
			// Memorisation du nom de l'attribut du classifieur et de sa regle si ok
			else
			{
				sPredictorRuleAttributeName = attribute->GetName();
				classifierRule = cast(KWDRNBClassifier*, attribute->GetDerivationRule());
			}
		}
	}

	// Cas d'un classifier valide, donc heritant du Naive Bayes
	// Verification des attributs de prediction et comptage des paires d'attribut utilises par le predicteur
	// Ces paires ont un status partticulier, car il n'y a pas d'attribut explicitement genere par paire,
	// comme dans le cas univarie standard, des agregats de type texte, multi-table ou arbres
	nClassifierPairNumber = 0;
	if (bIsClassifier)
	{
		assert(sPredictorRuleAttributeName != "");
		assert(classifierRule != NULL);

		// Initialisation des listes d'attributs du classifier
		classifierRule->ExportAttributeNames(kwcInputPredictor, &svPredictorAttributeNames,
						     &svPredictorDataGridAttributeNames,
						     &oaPredictorDenseAttributeDataGridStatsRules);

		// Le classifier doit avoir au moins un attribut
		if (svPredictorAttributeNames.GetSize() == 0)
		{
			AddPredictorWarning(kwcInputPredictor,
					    "Interpretation services not available for predictors without "
					    "predictor variables");
			bIsClassifier = false;
		}

		// Parcours des attributs explicatifs recenses, pour des tests d'integrite avances
		// Ces precautions permettent de se premunir au mieux contre des incoherence de
		// dictionnaires de classifieurs modifies a la main
		for (i = 0; i < svPredictorAttributeNames.GetSize(); i++)
		{
			// Verification de l'attribut de preparation du predicteur natif
			dataGridAttribute =
			    kwcInputPredictor->LookupAttribute(svPredictorDataGridAttributeNames.GetAt(i));
			if (dataGridAttribute == NULL)
			{
				AddPredictorError(kwcInputPredictor,
						  "Invalid classifier with missing preparation variable " +
						      svPredictorDataGridAttributeNames.GetAt(i));
				bIsClassifier = false;
				break;
			}

			// Verification de l'existence d'une regle de preparation
			assert(dataGridAttribute != NULL);
			dataGridRule = NULL;
			if (dataGridAttribute->GetDerivationRule() == NULL)
			{
				AddPredictorError(kwcInputPredictor,
						  sTmp + "Invalid classifier with incorrect peparation variable " +
						      dataGridAttribute->GetName() + " (missing derivation rule)");

				bIsClassifier = false;
				break;
			}
			// Test du type de regle
			else if (dataGridAttribute->GetDerivationRule()->GetName() != referenceDataGridRule.GetName())
			{
				AddPredictorError(kwcInputPredictor,
						  "Invalid classifier with incorrect peparation variable " +
						      dataGridAttribute->GetName() + " (should used a " +
						      referenceDataGridRule.GetName() + " derivation rule)");
				bIsClassifier = false;
				break;
			}
			// Memorisation de la regle si OK
			else
			{
				dataGridRule = cast(KWDRDataGrid*, dataGridAttribute->GetDerivationRule());
			}

			// Verification du nombre de dimension de la regle de preparation
			if (dataGridRule->GetAttributeNumber() < 2 or dataGridRule->GetAttributeNumber() > 3)
			{
				AddPredictorError(kwcInputPredictor,
						  "Invalid classifier with incorrect peparation variable " +
						      dataGridAttribute->GetName() +
						      " (wrong number prepared dimensions");
				bIsClassifier = false;
				break;
			}

			// Verification de l'attribut du predicteur natif, uniquement dans le cas univarie
			// Dans le cas des paires de variables, avec des grille a trois dimensions, il n'y a pas
			// d'attribut genere par paire de variables
			assert(dataGridRule->GetAttributeNumber() == 2 or svPredictorAttributeNames.GetAt(i) == "");
			if (svPredictorAttributeNames.GetAt(i) != "")
			{
				attribute = kwcInputPredictor->LookupAttribute(svPredictorAttributeNames.GetAt(i));
				if (attribute == NULL)
				{
					AddPredictorError(kwcInputPredictor,
							  "Invalid classifier with missing predictor variable " +
							      svPredictorAttributeNames.GetAt(i));
					bIsClassifier = false;
					break;
				}
			}

			// Comptage des paires
			if (dataGridRule->GetAttributeNumber() == 3)
			{
				assert(svPredictorAttributeNames.GetAt(i) == "");
				nClassifierPairNumber++;
			}
		}
	}

	// Creation si necessaire de nom d'attributs unqiues pour les paires d'attributs utilisee par le classifieur
	if (bIsClassifier and nClassifierPairNumber > 0)
	{
		// Construction d'une variante allegee du dictionnaire de classification, uniquement pour pouvoir generer
		// un nom de variable unique par paire d'attribt
		kwcShadowClass = BuildShadowClass(kwcInputPredictor);

		// Creation d'une variable par paire
		for (i = 0; i < svPredictorAttributeNames.GetSize(); i++)
		{
			// Les paires correspondent aux variables du predicteur, n'ayant pas de nom
			if (svPredictorAttributeNames.GetAt(i) == "")
			{
				// Recherche de la preparation bivariee
				dataGridStatsRule =
				    cast(KWDRDataGridStats*, oaPredictorDenseAttributeDataGridStatsRules.GetAt(i));
				assert(dataGridStatsRule != NULL);
				assert(dataGridStatsRule->GetOperandNumber() == 3);
				assert(dataGridStatsRule->GetOperandAt(0)->GetAttributeName() ==
				       svPredictorDataGridAttributeNames.GetAt(i));
				assert(dataGridStatsRule->GetOperandAt(1)->GetAttributeName() != "");
				assert(dataGridStatsRule->GetOperandAt(2)->GetAttributeName() != "");

				// On concatene les nom des variables, separes par un back-quote, comme pour le nom par defaut
				// en preparation multi-variee
				sDefaultPairName = dataGridStatsRule->GetOperandAt(1)->GetAttributeName() + '`' +
						   dataGridStatsRule->GetOperandAt(2)->GetAttributeName();

				// Utilisation du dictionnaire intermediaire pour creer un nom unique pour la paire
				attribute = new KWAttribute;
				attribute->SetName(kwcShadowClass->BuildAttributeName(sDefaultPairName));
				kwcShadowClass->InsertAttribute(attribute);

				// Memorisation du nom de l'attribut
				svPredictorAttributeNames.SetAt(i, attribute->GetName());
			}
		}

		// Nettoyage
		delete kwcShadowClass;
	}

	// Import des meta-data des variables du predictor
	if (bIsClassifier)
		ImportPredictorVariablesMetaData(kwcInputPredictor);

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
		assert(svPredictorDataGridAttributeNames.GetSize() == svPredictorAttributeNames.GetSize());
		assert(oaPredictorDenseAttributeDataGridStatsRules.GetSize() == svPredictorAttributeNames.GetSize());
		assert(dvPredictorAttributeLevels.GetSize() == svPredictorAttributeNames.GetSize());
		assert(dvPredictorAttributeWeights.GetSize() == svPredictorAttributeNames.GetSize());
		assert(dvPredictorAttributeImportances.GetSize() == svPredictorAttributeNames.GetSize());
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
			{
				cout << "\t\t" << svPredictorAttributeNames.GetAt(i) << "\t";
				cout << svPredictorDataGridAttributeNames.GetAt(i) << "\t";
				cout << dvPredictorAttributeLevels.GetAt(i) << "\t";
				cout << dvPredictorAttributeWeights.GetAt(i) << "\t";
				cout << dvPredictorAttributeImportances.GetAt(i) << "\n";
			}
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
	svPredictorDataGridAttributeNames.SetSize(0);
	oaPredictorDenseAttributeDataGridStatsRules.SetSize(0);
	dvPredictorAttributeLevels.SetSize(0);
	dvPredictorAttributeWeights.SetSize(0);
	dvPredictorAttributeImportances.SetSize(0);
}

KWClass* KIInterpretationClassBuilder::BuildShadowClass(const KWClass* kwcInputClass) const
{
	KWClass* kwcShadowClass;
	KWAttribute* attribute;
	KWAttribute* shadowAttribute;

	require(kwcInputClass != NULL);

	// Creation de la classe
	kwcShadowClass = new KWClass;
	kwcShadowClass->SetName("Shadow");

	// Reservation d'un nom d'attribut par attribut source
	attribute = kwcInputClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		// Creation d'un attribut par attribut source
		shadowAttribute = new KWAttribute;
		shadowAttribute->SetName(attribute->GetName());
		kwcShadowClass->InsertAttribute(shadowAttribute);

		// Creation d'un attribut par bloc d'attributs source
		if (attribute->IsInBlock() and attribute->IsFirstInBlock())
		{
			shadowAttribute = new KWAttribute;
			shadowAttribute->SetName(attribute->GetAttributeBlock()->GetName());
			kwcShadowClass->InsertAttribute(shadowAttribute);
		}

		// Attribut source suivant
		kwcInputClass->GetNextAttribute(attribute);
	}
	ensure(kwcShadowClass->GetAttributeNumber() >= kwcInputClass->GetAttributeNumber());
	return kwcShadowClass;
}

void KIInterpretationClassBuilder::ImportPredictorVariablesMetaData(const KWClass* kwcInputPredictor)
{
	KWAttribute* attribute;
	KWDRNBClassifier* classifierRule;
	double dWeight;
	double dLevel;
	double dImportance;
	int i;

	require(kwcInputPredictor != NULL);
	require(sPredictorRuleAttributeName != "");
	require(svPredictorAttributeNames.GetSize() > 0);
	require(svPredictorDataGridAttributeNames.GetSize() > 0);
	require(dvPredictorAttributeLevels.GetSize() == 0);
	require(dvPredictorAttributeWeights.GetSize() == 0);
	require(dvPredictorAttributeImportances.GetSize() == 0);

	// Initialisation des vecteurs de meta-data
	dvPredictorAttributeLevels.SetSize(svPredictorAttributeNames.GetSize());
	dvPredictorAttributeWeights.SetSize(svPredictorAttributeNames.GetSize());
	dvPredictorAttributeImportances.SetSize(svPredictorAttributeNames.GetSize());

	// Recherche de la regle de prediction
	attribute = kwcInputPredictor->LookupAttribute(sPredictorRuleAttributeName);
	classifierRule = cast(KWDRNBClassifier*, attribute->GetDerivationRule());
	assert(classifierRule->GetDataGridStatsNumber() == svPredictorAttributeNames.GetSize());

	// Collecte des meta-data
	for (i = 0; i < svPredictorAttributeNames.GetSize(); i++)
	{
		// On recupere le Weight du predicteur
		dWeight = classifierRule->GetDataGridWeightAt(i);

		// On recherche le Level dans l'attribut prepare
		attribute = kwcInputPredictor->LookupAttribute(svPredictorDataGridAttributeNames.GetAt(i));
		assert(attribute != NULL);
		dLevel =
		    attribute->GetConstMetaData()->GetDoubleValueAt(KWDataPreparationAttribute::GetLevelMetaDataKey());
		dLevel = max(dLevel, (double)0);
		dLevel = min(dLevel, (double)1);

		// On recherche l'importance dans l'attribut, qui peut etre non existant dans le cas d'une paire
		dImportance = 0;
		attribute = kwcInputPredictor->LookupAttribute(svPredictorAttributeNames.GetAt(i));
		if (attribute != NULL)
		{
			dImportance = attribute->GetConstMetaData()->GetDoubleValueAt(
			    SNBPredictorSelectiveNaiveBayes::GetImportanceMetaDataKey());
			dImportance = max(dImportance, (double)0);
			dImportance = min(dImportance, (double)1);
		}

		// Recalcul de l'importance si necessaire
		if (dImportance == 0)
			dImportance = sqrt(dLevel * dWeight);

		// Memorisation des meta-data
		dvPredictorAttributeWeights.SetAt(i, dWeight);
		dvPredictorAttributeLevels.SetAt(i, dLevel);
		dvPredictorAttributeImportances.SetAt(i, dImportance);
	}
	ensure(dvPredictorAttributeWeights.GetSize() == svPredictorAttributeNames.GetSize());
	ensure(dvPredictorAttributeLevels.GetSize() == svPredictorAttributeNames.GetSize());
	ensure(dvPredictorAttributeImportances.GetSize() == svPredictorAttributeNames.GetSize());
}

void KIInterpretationClassBuilder::AddPredictorError(const KWClass* kwcInputPredictor, const ALString& sLabel) const
{
	require(kwcInputPredictor != NULL);
	require(sLabel != "");
	Global::AddError("Dictionary", kwcInputPredictor->GetName(), sLabel);
}

void KIInterpretationClassBuilder::AddPredictorWarning(const KWClass* kwcInputPredictor, const ALString& sLabel) const
{
	require(kwcInputPredictor != NULL);
	require(sLabel != "");
	Global::AddWarning("Dictionary", kwcInputPredictor->GetName(), sLabel);
}

KWClass* KIInterpretationClassBuilder::BuildInterpretationServiceClass(const ALString& sServiceLabel,
								       const StringVector* svServiceMetaDataKeys) const
{
	KWClassDomain* kwcdInterpretationServiceDomain;
	KWClass* kwcInterpretationServiceClass;
	KWAttribute* attribute;
	SymbolVector svInterpretedTargetValues;
	Symbol sTargetValue;
	KWDRDataGridStats* dataGridStatsRule;
	int i;

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
	for (i = 0; i < kwcInterpretationServiceClass->GetKeyAttributeNumber(); i++)
	{
		attribute = kwcInterpretationServiceClass->GetKeyAttributeAt(i);
		check(attribute);
		attribute->SetUsed(true);
		attribute->SetLoaded(true);
	}

	// Nettoyage des meta-data liees au services d'interpretation potentiellement existants
	for (i = 0; i < svServiceMetaDataKeys->GetSize(); i++)
		kwcInterpretationServiceClass->GetMetaData()->RemoveKey(svServiceMetaDataKeys->GetAt(i));
	attribute = kwcInterpretationServiceClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		for (i = 0; i < svServiceMetaDataKeys->GetSize(); i++)
			attribute->GetMetaData()->RemoveKey(svServiceMetaDataKeys->GetAt(i));
		kwcInterpretationServiceClass->GetNextAttribute(attribute);
	}

	// Creation de variables pour chaque paire d'attribut du predicteur
	for (i = 0; i < oaPredictorDenseAttributeDataGridStatsRules.GetSize(); i++)
	{
		// Recherche de la preparation
		dataGridStatsRule = cast(KWDRDataGridStats*, oaPredictorDenseAttributeDataGridStatsRules.GetAt(i));

		// Creation d'un attribut de paire si necessaire
		if (dataGridStatsRule != NULL and dataGridStatsRule->GetOperandNumber() == 3)
		{
			CreatePairAttribute(kwcInterpretationServiceClass, svPredictorAttributeNames.GetAt(i),
					    dataGridStatsRule->GetOperandAt(1)->GetAttributeName(),
					    dataGridStatsRule->GetOperandAt(2)->GetAttributeName(),
					    svPredictorDataGridAttributeNames.GetAt(i),
					    dvPredictorAttributeLevels.GetAt(i), dvPredictorAttributeWeights.GetAt(i),
					    dvPredictorAttributeImportances.GetAt(i));
		}
	}
	return kwcInterpretationServiceClass;
}

KWAttribute* KIInterpretationClassBuilder::CreatePairAttribute(KWClass* kwcInterpretationServiceClass,
							       const ALString& sPairAttributeName,
							       const ALString& sPairName1, const ALString& sPairName2,
							       const ALString& sPairDataGridAttributeName,
							       double dLevel, double dWeight, double dImportance) const
{
	KWAttribute* pairAttribute;
	KWAttribute* attribute1;
	KWAttribute* attribute2;
	KWDRCellLabel* cellLabelRule;
	KWDerivationRuleOperand* operand;

	require(kwcInterpretationServiceClass != NULL);
	require(sPairAttributeName != "");
	require(sPairName1 != sPairName2);
	require(kwcInterpretationServiceClass->LookupAttribute(sPairAttributeName) == NULL);
	require(kwcInterpretationServiceClass->LookupAttribute(sPairName1) != NULL);
	require(kwcInterpretationServiceClass->LookupAttribute(sPairName2) != NULL);
	require(0 <= dLevel and dLevel <= 1);
	require(0 <= dWeight and dWeight <= 1);
	require(0 <= dImportance and dImportance <= 1);

	// Recherche des attributs existant dans la classe
	attribute1 = kwcInterpretationServiceClass->LookupAttribute(sPairName1);
	attribute2 = kwcInterpretationServiceClass->LookupAttribute(sPairName2);

	// Creation d'une regle de type libelle, pour des raison d'interpretabilites
	cellLabelRule = new KWDRCellLabel;

	// Initialisation du premier operande de la regle avec l'attribut de preparation grille
	cellLabelRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	cellLabelRule->GetFirstOperand()->SetAttributeName(sPairDataGridAttributeName);
	cellLabelRule->DeleteAllVariableOperands();

	// Ajout d'un operande pour le premier attribut de la paire
	operand = new KWDerivationRuleOperand;
	cellLabelRule->AddOperand(operand);
	operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	operand->SetType(attribute1->GetType());
	operand->SetAttributeName(sPairName1);

	// Ajout d'un operande pour le second attribut de la paire
	operand = new KWDerivationRuleOperand;
	cellLabelRule->AddOperand(operand);
	operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	operand->SetType(attribute2->GetType());
	operand->SetAttributeName(sPairName2);

	// Creation de l'attribut, en Unused
	pairAttribute = new KWAttribute;
	pairAttribute->SetName(sPairAttributeName);
	pairAttribute->SetUsed(false);
	pairAttribute->SetDerivationRule(cellLabelRule);
	pairAttribute->CompleteTypeInfo(kwcInterpretationServiceClass);
	kwcInterpretationServiceClass->InsertAttribute(pairAttribute);

	// Ajout des meta-data de Level, Weight et Importance
	pairAttribute->GetMetaData()->SetDoubleValueAt(KWDataPreparationAttribute::GetLevelMetaDataKey(), dLevel);
	pairAttribute->GetMetaData()->SetDoubleValueAt(SNBPredictorSelectiveNaiveBayes::GetWeightMetaDataKey(),
						       dWeight);
	pairAttribute->GetMetaData()->SetDoubleValueAt(SNBPredictorSelectiveNaiveBayes::GetImportanceMetaDataKey(),
						       dImportance);

	// Ajout des meta-data specifiques aux paires
	pairAttribute->GetMetaData()->SetNoValueAt(GetIsPairMetaDataKey());
	pairAttribute->GetMetaData()->SetStringValueAt(GetName1MetaDataKey(), sPairName1);
	pairAttribute->GetMetaData()->SetStringValueAt(GetName2MetaDataKey(), sPairName2);

	// Ajout d'un commentaire
	pairAttribute->SetLabel(
	    "Variable related to a pair used by the predictor, created for interpretation purposes");
	return pairAttribute;
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
	KWAttribute* attribute;
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

		// On passe les attributs de contribution du predicteur en used, car ceux-ci sont utilises usuellement
		// par les outils de type visualisation des valeurs de SHAP
		for (nAttribute = 0; nAttribute < nContributionAttributeNumber; nAttribute++)
		{
			predictorAttribute = cast(KIPredictorAttribute*, oaPredictorAttributes.GetAt(nAttribute));
			attribute = kwcInterpretationClass->LookupAttribute(predictorAttribute->GetName());
			attribute->SetUsed(true);
			attribute->SetLoaded(true);
		}

		// Parcours de classe cibles
		for (nTarget = 0; nTarget < svInterpretedTargetValues->GetSize(); nTarget++)
		{
			sTargetValue = svInterpretedTargetValues->GetAt(nTarget);

			// Parcours des attributs
			for (nAttribute = 0; nAttribute < nContributionAttributeNumber; nAttribute++)
			{
				predictorAttribute =
				    cast(KIPredictorAttribute*, oaPredictorAttributes.GetAt(nAttribute));

				// Creation de l'attribut d'interpretation
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
				// Creation des attributs d'interpretation pour les services par rang
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

const StringVector* KIInterpretationClassBuilder::GetPredictorDataGridAttributeNames() const
{
	require(IsPredictorImported());
	return &svPredictorDataGridAttributeNames;
}

void KIInterpretationClassBuilder::BuildPredictorAttributes(ObjectArray* oaPredictorAttributes) const
{
	KIPredictorAttribute* predictorAttribute;
	KWAttribute* attribute;
	int i;

	require(IsPredictorImported());
	require(oaPredictorAttributes != NULL);
	require(oaPredictorAttributes->GetSize() == 0);

	// Alimentation a partir des specifications disponibles dans le ClassBuilder
	for (i = 0; i < GetPredictorAttributeNumber(); i++)
	{
		// Ajout d'une variable au tableau
		predictorAttribute = new KIPredictorAttribute;
		oaPredictorAttributes->Add(predictorAttribute);

		// Nom de la variable
		predictorAttribute->SetName(GetPredictorAttributeNames()->GetAt(i));

		// Type de la variable
		attribute = GetPredictorClass()->LookupAttribute(GetPredictorAttributeNames()->GetAt(i));
		if (attribute != NULL)
			predictorAttribute->SetType(KWType::ToString(attribute->GetType()));
		// Dans le cas d'une paire, il n'y a pas d'attribut pour le predicteur
		// Un attribut dedie a l'interpretation sera cree, avec le type Symbol
		else
			predictorAttribute->SetType(KWType::ToString(KWType::Symbol));

		// Importance de la variable
		predictorAttribute->SetImportance(dvPredictorAttributeImportances.GetAt(i));
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
	svInterpretationMetaDataKeys.Add(GetIsPairMetaDataKey());
	svInterpretationMetaDataKeys.Add(GetName1MetaDataKey());
	svInterpretationMetaDataKeys.Add(GetName2MetaDataKey());

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
	svReinforcementMetaDataKeys.Add(GetIsPairMetaDataKey());
	svReinforcementMetaDataKeys.Add(GetName1MetaDataKey());
	svReinforcementMetaDataKeys.Add(GetName2MetaDataKey());

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

	// Creation des attributs de renforcement, uniquement pour la classe a renforcer
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

const ALString& KIInterpretationClassBuilder::GetIsPairMetaDataKey()
{
	static const ALString sMetaDataKey = "IsPair";
	return sMetaDataKey;
}

const ALString& KIInterpretationClassBuilder::GetName1MetaDataKey()
{
	static const ALString sMetaDataKey = "Name1";
	return sMetaDataKey;
}

const ALString& KIInterpretationClassBuilder::GetName2MetaDataKey()
{
	static const ALString sMetaDataKey = "Name2";
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
