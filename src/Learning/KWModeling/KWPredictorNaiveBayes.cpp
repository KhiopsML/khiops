// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorNaiveBayes.h"

KWPredictorNaiveBayes::KWPredictorNaiveBayes() {}

KWPredictorNaiveBayes::~KWPredictorNaiveBayes() {}

boolean KWPredictorNaiveBayes::IsTargetTypeManaged(int nType) const
{
	return nType == KWType::Symbol or nType == KWType::Continuous;
}

KWPredictor* KWPredictorNaiveBayes::Create() const
{
	return new KWPredictorNaiveBayes;
}

const ALString KWPredictorNaiveBayes::GetName() const
{
	return "Naive Bayes";
}

const ALString KWPredictorNaiveBayes::GetPrefix() const
{
	return "NB";
}

boolean KWPredictorNaiveBayes::InternalTrain()
{
	boolean bOk;
	KWDataPreparationClass dataPreparationClass;

	require(Check());
	require(GetClassStats() != NULL);
	require(GetClassStats()->IsStatsComputed());
	require(GetTargetAttributeType() == KWType::Symbol or GetTargetAttributeType() == KWType::Continuous);
	require(IsTraining());

	// Apprentissage si au moins une valeur cible
	bOk = false;
	if (GetTargetDescriptiveStats()->GetValueNumber() > 0)
	{
		// Parametrage de la preparation de donnees
		dataPreparationClass.SetLearningSpec(GetLearningSpec());

		// Generation de la classe de preparation des donnees
		dataPreparationClass.ComputeDataPreparationFromClassStats(GetClassStats());

		// Construction d'un predicteur bayesien naif a partir de tous les attributs
		InternalTrainNB(&dataPreparationClass, dataPreparationClass.GetDataPreparationAttributes());
		bOk = true;
	}
	return bOk;
}

void KWPredictorNaiveBayes::InternalTrainNB(KWDataPreparationClass* dataPreparationClass,
					    ObjectArray* oaDataPreparationUsedAttributes)
{
	InternalTrainWNB(dataPreparationClass, oaDataPreparationUsedAttributes, NULL);
}

void KWPredictorNaiveBayes::InternalTrainWNB(KWDataPreparationClass* dataPreparationClass,
					     ObjectArray* oaDataPreparationUsedAttributes,
					     ContinuousVector* cvDataPreparationWeights)
{
	ObjectArray oaFilteredDataPreparationAttributes;
	ContinuousVector cvWorkingWeights;
	ContinuousVector* cvFilterDataPreparationWeights;
	KWDataPreparationAttribute* dataPreparationAttribute;
	int nAttribute;

	require(dataPreparationClass != NULL);
	require(dataPreparationClass->CheckDataPreparation());
	require(oaDataPreparationUsedAttributes != NULL);
	require(cvDataPreparationWeights == NULL or
		cvDataPreparationWeights->GetSize() == dataPreparationClass->GetDataPreparationAttributes()->GetSize());
	require(GetTargetDescriptiveStats()->GetValueNumber() > 0);
	require(GetPredictorReport() != NULL);
	require(GetTrainedPredictor() != NULL);
	require(GetTrainedPredictor()->GetPredictorClass() == NULL);

	// Indirection sur un vecteur de poids filtre, selon le vecteur de poids en entree
	if (cvDataPreparationWeights == NULL)
		cvFilterDataPreparationWeights = NULL;
	else
		cvFilterDataPreparationWeights = &cvWorkingWeights;

	// Filtrage des attributs et poids effectivement utilises
	for (nAttribute = 0; nAttribute < oaDataPreparationUsedAttributes->GetSize(); nAttribute++)
	{
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*, oaDataPreparationUsedAttributes->GetAt(nAttribute));

		// Test si attribut utile: son SortValue (Level, DeltaLevel...) doit etre strictement positif
		if (dataPreparationAttribute->GetPreparedStats()->GetSortValue() > 0 and
		    (cvDataPreparationWeights == NULL or cvDataPreparationWeights->GetAt(nAttribute) != 0))
		{
			oaFilteredDataPreparationAttributes.Add(dataPreparationAttribute);
			if (cvFilterDataPreparationWeights != NULL)
				cvFilterDataPreparationWeights->Add(cvDataPreparationWeights->GetAt(nAttribute));
		}
	}

	// Initialisation de la classe du predicteur
	GetTrainedPredictor()->SetPredictorClass(dataPreparationClass->GetDataPreparationClass(),
						 GetTargetAttributeType(), GetObjectLabel());

	// Construction des attributs de prediction selon le type de predicteur
	if (GetTargetAttributeType() == KWType::Symbol)
	{
		InternalTrainWNBClassifier(GetTrainedClassifier(), dataPreparationClass,
					   &oaFilteredDataPreparationAttributes, cvFilterDataPreparationWeights);
	}
	else if (GetTargetAttributeType() == KWType::Continuous)
	{
		InternalTrainWNBRegressor(GetTrainedRegressor(), dataPreparationClass,
					  &oaFilteredDataPreparationAttributes, cvFilterDataPreparationWeights);
	}

	// Memorisation du nombre d'attribut utilises par le classifier
	GetPredictorReport()->SetUsedAttributeNumber(oaFilteredDataPreparationAttributes.GetSize());

	// Nettoyage de la preparation, la classe de preparation etant maintenant reference par le predicteur
	dataPreparationClass->RemoveDataPreparation();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// WNBClassifier

void KWPredictorNaiveBayes::InternalTrainWNBClassifier(KWTrainedClassifier* trainedClassifier,
						       KWDataPreparationClass* dataPreparationClass,
						       ObjectArray* oaDataPreparationUsedAttributes,
						       ContinuousVector* cvDataPreparationWeights)
{
	KWAttribute* classifierAttribute;
	KWAttribute* targetValuesAttribute;

	require(trainedClassifier != NULL);
	require(trainedClassifier->GetPredictorClass() != NULL);
	require(dataPreparationClass != NULL);
	require(oaDataPreparationUsedAttributes != NULL);
	require(cvDataPreparationWeights == NULL or
		cvDataPreparationWeights->GetSize() >= oaDataPreparationUsedAttributes->GetSize());

	// Memorisation de la reference a l'attribut cible
	trainedClassifier->SetTargetAttribute(
	    trainedClassifier->GetPredictorClass()->LookupAttribute(GetTargetAttributeName()));

	// Recherche de l'attribut memorisant les valeurs cibles
	targetValuesAttribute = dataPreparationClass->GetDataPreparationTargetAttribute()->GetPreparedAttribute();
	trainedClassifier->SetTargetValuesAttribute(targetValuesAttribute);

	// Ajout de l'attribut classifieur
	classifierAttribute =
	    AddClassifierAttribute(trainedClassifier, oaDataPreparationUsedAttributes, cvDataPreparationWeights);

	// Ajout des attributs de prediction pour la classification
	AddClassifierPredictionAttributes(trainedClassifier, classifierAttribute);

	// Completion automatique des informations de la classe (nom de classe par regle...)
	trainedClassifier->GetPredictorClass()->CompleteTypeInfo();
}

KWAttribute* KWPredictorNaiveBayes::AddClassifierAttribute(KWTrainedClassifier* trainedClassifier,
							   ObjectArray* oaDataPreparationUsedAttributes,
							   ContinuousVector* cvDataPreparationWeights)
{
	boolean bTrace = false;
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWDRContinuousVector* weightRule;
	KWDerivationRuleOperand* operand;
	KWDRDataGridStats* dgsRule;
	KWDRNBClassifier* classifierRule;
	KWAttribute* classifierAttribute;

	require(trainedClassifier != NULL);
	require(oaDataPreparationUsedAttributes != NULL);
	require(cvDataPreparationWeights == NULL or
		cvDataPreparationWeights->GetSize() >= oaDataPreparationUsedAttributes->GetSize());

	// Creation d'une regle de ponderation des grilles
	weightRule = NULL;
	if (cvDataPreparationWeights != NULL)
	{
		weightRule = new KWDRContinuousVector;
		weightRule->SetValueNumber(oaDataPreparationUsedAttributes->GetSize());
		for (nAttribute = 0; nAttribute < oaDataPreparationUsedAttributes->GetSize(); nAttribute++)
			weightRule->SetValueAt(nAttribute, cvDataPreparationWeights->GetAt(nAttribute));
	}

	// Creation d'une regle de regression destinee a accueilir les grilles, avec ou sans les poids
	if (weightRule == NULL)
		classifierRule = new KWDRNBClassifier;
	else
	{
		classifierRule = new KWDRSNBClassifier;
		classifierRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
		classifierRule->GetFirstOperand()->SetDerivationRule(weightRule);
	}
	classifierRule->DeleteAllVariableOperands();

	// Ajout d'un attribut de type grille de donnees par attribut prepare
	for (nAttribute = 0; nAttribute < oaDataPreparationUsedAttributes->GetSize(); nAttribute++)
	{
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*, oaDataPreparationUsedAttributes->GetAt(nAttribute));

		// Affichage des statistiques de preparation
		if (bTrace)
		{
			cout << dataPreparationAttribute->GetObjectLabel() << endl;
			cout << *dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats() << endl;
		}

		// Creation d'une regle DataGridStats
		dgsRule = dataPreparationAttribute->CreatePreparedStatsRule();

		// Ajout d'un operande DataGridStats au predicteur
		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		operand->SetDerivationRule(dgsRule);
		operand->SetType(dgsRule->GetType());
		operand->SetStructureName(dgsRule->GetStructureName());
		classifierRule->AddOperand(operand);
	}

	// Ajout d'un dernier operande pour les valeurs cible
	operand = new KWDerivationRuleOperand;
	operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	operand->SetType(trainedClassifier->GetTargetValuesAttribute()->GetType());
	operand->SetAttributeName(trainedClassifier->GetTargetValuesAttribute()->GetName());
	operand->SetStructureName(trainedClassifier->GetTargetValuesAttribute()->GetStructureName());
	classifierRule->AddOperand(operand);

	// Creation d'un attribut de classification
	classifierAttribute =
	    trainedClassifier->CreatePredictionAttribute(GetPrefix() + GetTargetAttributeName(), classifierRule);
	return classifierAttribute;
}

void KWPredictorNaiveBayes::AddClassifierPredictionAttributes(KWTrainedClassifier* trainedClassifier,
							      KWAttribute* classifierAttribute)
{
	KWDRTargetValue* predictionRule;
	KWDRTargetProb* scoreRule;
	KWDRTargetProbAt* targetProbRule;
	KWAttribute* predictionAttribute;
	KWAttribute* scoreAttribute;
	KWAttribute* targetProbAttribute;
	const KWDGSAttributeSymbolValues* targetValues;
	int nTarget;
	boolean bAddBiasedPredictionAttribute;
	KWClassifierPostOptimizer classifierPostOptimizer;
	ContinuousVector cvScoreOffsets;
	KWDRContinuousVector* offsetRule;
	KWDRBiasedTargetValue* biasedPredictionRule;
	KWAttribute* biasedPredictionAttribute;

	require(trainedClassifier != NULL);
	require(classifierAttribute != NULL);
	require(trainedClassifier->GetPredictorClass()->LookupAttribute(classifierAttribute->GetName()) ==
		classifierAttribute);
	require(trainedClassifier->GetTargetValuesAttribute() != NULL);
	require(trainedClassifier->GetPredictorClass()->LookupAttribute(
		    trainedClassifier->GetTargetValuesAttribute()->GetName()) ==
		trainedClassifier->GetTargetValuesAttribute());

	// Creation d'une regle de prediction de la valeur cible
	predictionRule = new KWDRTargetValue;
	predictionRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	predictionRule->GetFirstOperand()->SetAttributeName(classifierAttribute->GetName());

	// Ajout de l'attribut dans la classe
	predictionAttribute =
	    trainedClassifier->CreatePredictionAttribute("Predicted" + GetTargetAttributeName(), predictionRule);
	trainedClassifier->SetPredictionAttribute(predictionAttribute);

	// Creation d'une regle de prediction du score
	scoreRule = new KWDRTargetProb;
	scoreRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	scoreRule->GetFirstOperand()->SetAttributeName(classifierAttribute->GetName());

	// Ajout de l'attribut dans la classe
	scoreAttribute = trainedClassifier->CreatePredictionAttribute("Score" + GetTargetAttributeName(), scoreRule);
	trainedClassifier->SetScoreAttribute(scoreAttribute);

	// Memorisation du nombre d'attribut de prediction des probabilites cibles
	assert(GetTargetValueStats()->GetAttributeNumber() == 1);
	assert(GetTargetValueStats()->GetAttributeAt(0)->GetAttributeType() == KWType::Symbol);
	assert(GetTargetValueStats()->GetAttributeAt(0)->ArePartsSingletons());
	targetValues = cast(const KWDGSAttributeSymbolValues*, GetTargetValueStats()->GetAttributeAt(0));
	trainedClassifier->SetTargetValueNumber(targetValues->GetValueNumber());

	// Creation d'attributs pour les probabilites conditionnelles par valeur cible
	for (nTarget = 0; nTarget < targetValues->GetValueNumber(); nTarget++)
	{
		// Creation d'une regle de prediction de la probabilite cible pour une valeur donnees
		targetProbRule = new KWDRTargetProbAt;
		targetProbRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		targetProbRule->GetFirstOperand()->SetAttributeName(classifierAttribute->GetName());
		targetProbRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		targetProbRule->GetSecondOperand()->SetSymbolConstant(targetValues->GetValueAt(nTarget));

		// Ajout de l'attribut dans la classe
		targetProbAttribute = trainedClassifier->CreatePredictionAttribute(
		    "Prob" + GetTargetAttributeName() + targetValues->GetValueAt(nTarget), targetProbRule);
		trainedClassifier->SetProbAttributeAt(nTarget, targetValues->GetValueAt(nTarget), targetProbAttribute);
	}

	// Optimisation des offsets de prediction, et ajout d'un attribut si necessaire
	bAddBiasedPredictionAttribute =
	    classifierPostOptimizer.PostOptimize(this, GetTrainedClassifier(), &cvScoreOffsets);
	if (bAddBiasedPredictionAttribute)
	{
		// Creation d'une regle pour memoriser les offsets
		offsetRule = new KWDRContinuousVector;
		offsetRule->SetValueNumber(cvScoreOffsets.GetSize());
		for (nTarget = 0; nTarget < cvScoreOffsets.GetSize(); nTarget++)
			offsetRule->SetValueAt(nTarget, cvScoreOffsets.GetAt(nTarget));

		// Creation d'une regle de prediction biasee de la valeur cible
		biasedPredictionRule = new KWDRBiasedTargetValue;
		biasedPredictionRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		biasedPredictionRule->GetFirstOperand()->SetAttributeName(classifierAttribute->GetName());
		biasedPredictionRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
		biasedPredictionRule->GetSecondOperand()->SetDerivationRule(offsetRule);

		// Ajout de l'attribut dans la classe
		biasedPredictionAttribute = trainedClassifier->CreatePredictionAttribute(
		    "BiasedPredicted" + GetTargetAttributeName(), biasedPredictionRule);
		trainedClassifier->SetPredictionAttribute(biasedPredictionAttribute);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// WNBRegressor

void KWPredictorNaiveBayes::InternalTrainWNBRegressor(KWTrainedRegressor* trainedRegressor,
						      KWDataPreparationClass* dataPreparationClass,
						      ObjectArray* oaDataPreparationUsedAttributes,
						      ContinuousVector* cvDataPreparationWeights)
{
	KWAttribute* rankRegressorAttribute;
	KWAttribute* regressorAttribute;
	KWAttribute* targetValuesAttribute;

	require(trainedRegressor != NULL);
	require(trainedRegressor->GetPredictorClass() != NULL);
	require(dataPreparationClass != NULL);
	require(oaDataPreparationUsedAttributes != NULL);
	require(cvDataPreparationWeights == NULL or
		cvDataPreparationWeights->GetSize() >= oaDataPreparationUsedAttributes->GetSize());

	// Memorisation de la reference a l'attribut cible
	trainedRegressor->SetTargetAttribute(
	    trainedRegressor->GetPredictorClass()->LookupAttribute(GetTargetAttributeName()));

	// Recherche de l'attribut memorisant les valeurs cibles
	targetValuesAttribute = dataPreparationClass->GetDataPreparationTargetAttribute()->GetPreparedAttribute();
	trainedRegressor->SetTargetValuesAttribute(targetValuesAttribute);

	// Ajout de l'attribut regresseur de rang
	rankRegressorAttribute =
	    AddRankRegressorAttribute(trainedRegressor, oaDataPreparationUsedAttributes, cvDataPreparationWeights);

	// Ajout des attribut de prediction pour la regression de rang
	AddRankRegressorPredictionAttributes(trainedRegressor, rankRegressorAttribute);

	// Ajout de l'attribut regresseur de valeur
	regressorAttribute = AddRegressorAttribute(trainedRegressor, rankRegressorAttribute, targetValuesAttribute,
						   cvDataPreparationWeights);

	// Ajout des attribut de prediction pour la regression de valeur
	AddRegressorPredictionAttributes(trainedRegressor, regressorAttribute, targetValuesAttribute);

	// Completion automatique des informations de la classe (nom de classe par regle...)
	trainedRegressor->GetPredictorClass()->CompleteTypeInfo();
}

KWAttribute* KWPredictorNaiveBayes::AddRankRegressorAttribute(KWTrainedRegressor* trainedRegressor,
							      ObjectArray* oaDataPreparationUsedAttributes,
							      ContinuousVector* cvDataPreparationWeights)
{
	boolean bTrace = false;
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWDRContinuousVector* weightRule;
	KWDerivationRuleOperand* operand;
	KWDRDataGridStats* dgsRule;
	KWDRNBRankRegressor* rankRegressorRule;
	KWAttribute* rankRegressorAttribute;
	KWDRDataGrid* dgEmptyRule;
	KWDataGridStats emptyDataGridStats;
	KWDGSAttributeDiscretization* emptyDataGridAttribute;

	require(trainedRegressor != NULL);
	require(oaDataPreparationUsedAttributes != NULL);
	require(cvDataPreparationWeights == NULL or
		cvDataPreparationWeights->GetSize() >= oaDataPreparationUsedAttributes->GetSize());

	// Creation d'une regle de ponderation des grilles
	weightRule = NULL;
	if (cvDataPreparationWeights != NULL)
	{
		weightRule = new KWDRContinuousVector;
		weightRule->SetValueNumber(oaDataPreparationUsedAttributes->GetSize());
		for (nAttribute = 0; nAttribute < oaDataPreparationUsedAttributes->GetSize(); nAttribute++)
			weightRule->SetValueAt(nAttribute, cvDataPreparationWeights->GetAt(nAttribute));
	}

	// Creation d'une regle de regression destinee a accueilir les grilles, avec ou sans les poids
	if (weightRule == NULL)
		rankRegressorRule = new KWDRNBRankRegressor;
	else
	{
		rankRegressorRule = new KWDRSNBRankRegressor;
		rankRegressorRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
		rankRegressorRule->GetFirstOperand()->SetDerivationRule(weightRule);
	}
	rankRegressorRule->DeleteAllVariableOperands();

	// Ajout d'un attribut de type grille de donnees par attribut prepare
	for (nAttribute = 0; nAttribute < oaDataPreparationUsedAttributes->GetSize(); nAttribute++)
	{
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*, oaDataPreparationUsedAttributes->GetAt(nAttribute));

		// Affichage des statistiques de preparation
		if (bTrace)
		{
			cout << dataPreparationAttribute->GetObjectLabel() << endl;
			cout << *dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats() << endl;
		}

		// Creation d'une regle DataGridStats
		dgsRule = dataPreparationAttribute->CreatePreparedStatsRule();

		// Ajout d'un operande DataGridStats au predicteur
		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		operand->SetDerivationRule(dgsRule);
		operand->SetType(dgsRule->GetType());
		operand->SetStructureName(dgsRule->GetStructureName());
		rankRegressorRule->AddOperand(operand);
	}

	// Cas particulier ou il n'y a aucun attribut predictif
	// Le regresseur de rang a besoin neanmoins de l'effectif total, qui venait avec les grilles en entree
	// On genere une grille a une cellule pour indiquer cet effectif
	if (oaDataPreparationUsedAttributes->GetSize() == 0)
	{
		// Creation d'une grille a une cellule
		emptyDataGridAttribute = new KWDGSAttributeDiscretization;
		emptyDataGridAttribute->SetAttributeName("Empty");
		emptyDataGridStats.AddAttribute(emptyDataGridAttribute);
		emptyDataGridStats.CreateAllCells();
		emptyDataGridStats.SetUnivariateCellFrequencyAt(0, GetInstanceNumber());
		assert(emptyDataGridStats.Check());

		// Creation d'une regle de type grille
		dgEmptyRule = new KWDRDataGrid;
		dgEmptyRule->ImportDataGridStats(&emptyDataGridStats, false);

		// Creation d'une regle DataGridStats
		dgsRule = new KWDRDataGridStats;
		dgsRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
		dgsRule->GetFirstOperand()->SetDerivationRule(dgEmptyRule);
		dgsRule->DeleteAllVariableOperands();

		// Ajout d'un operande DataGridStats au predicteur
		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		operand->SetDerivationRule(dgsRule);
		operand->SetType(dgsRule->GetType());
		operand->SetStructureName(dgsRule->GetStructureName());
		rankRegressorRule->AddOperand(operand);
	}

	// Creation d'un attribut de regression
	rankRegressorAttribute = trainedRegressor->CreatePredictionAttribute(
	    GetPrefix() + "R" + GetTargetAttributeName(), rankRegressorRule);
	return rankRegressorAttribute;
}

void KWPredictorNaiveBayes::AddRankRegressorPredictionAttributes(KWTrainedRegressor* trainedRegressor,
								 KWAttribute* rankRegressorAttribute)
{
	KWDRTargetRankMean* meanRankRule;
	KWAttribute* meanRankAttribute;
	KWDRTargetRankStandardDeviation* standardDeviationRankRule;
	KWAttribute* standardDeviationRankAttribute;
	KWDRTargetRankCumulativeProbAt* cumulativeProbRankRule;
	KWAttribute* cumulativeProbRankAttribute;
	const int nPartileNumber = 5;
	int nPartile;
	Continuous cNormalizedRank;
	ALString sPartileName;
	KWDRTargetRankDensityAt* densityRankRule;
	KWAttribute* densityRankAttribute;
	KWDRValueRank* valueRankRule;
	KWAttribute* valueRankAttribute;
	KWAttribute* targetValuesAttribute;
	KWPredictionAttributeSpec* attributeSpec;
	ALString sTmp;

	require(trainedRegressor != NULL);
	require(rankRegressorAttribute != NULL);
	require(trainedRegressor->GetPredictorClass()->LookupAttribute(rankRegressorAttribute->GetName()) ==
		rankRegressorAttribute);
	require(trainedRegressor->GetTargetValuesAttribute() != NULL);
	require(trainedRegressor->GetPredictorClass()->LookupAttribute(
		    trainedRegressor->GetTargetValuesAttribute()->GetName()) ==
		trainedRegressor->GetTargetValuesAttribute());

	// Creation d'une regle de calcul de rang moyen
	meanRankRule = new KWDRTargetRankMean;
	meanRankRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	meanRankRule->GetFirstOperand()->SetAttributeName(rankRegressorAttribute->GetName());

	// Ajout de l'attribut dans la classe
	meanRankAttribute = trainedRegressor->CreatePredictionAttribute("MR" + GetTargetAttributeName(), meanRankRule);
	trainedRegressor->SetMeanRankAttribute(meanRankAttribute);

	// Creation d'une regle StandardDeviationRank
	standardDeviationRankRule = new KWDRTargetRankStandardDeviation;
	standardDeviationRankRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	standardDeviationRankRule->GetFirstOperand()->SetAttributeName(rankRegressorAttribute->GetName());

	// Ajout de l'attribut dans la classe
	standardDeviationRankAttribute =
	    trainedRegressor->CreatePredictionAttribute("SDR" + GetTargetAttributeName(), standardDeviationRankRule);

	// Creation d'une specification d'attribut pour ajouter StandardDeviationRank au predicteur
	attributeSpec = new KWPredictionAttributeSpec;
	attributeSpec->SetLabel("StandardDeviationRank");
	attributeSpec->SetType(KWType::Continuous);
	attributeSpec->SetMandatory(false);
	attributeSpec->SetEvaluation(false);
	attributeSpec->SetAttribute(standardDeviationRankAttribute);
	trainedRegressor->AddPredictionAttributeSpec(attributeSpec);

	// Creation d'attributs pour la fonction de repartition
	for (nPartile = 1; nPartile <= nPartileNumber; nPartile++)
	{
		cNormalizedRank = Continuous(nPartile) / nPartileNumber;
		sPartileName = IntToString(nPartile);

		// Creation d'une regle CumulativeProbRank
		cumulativeProbRankRule = new KWDRTargetRankCumulativeProbAt;
		cumulativeProbRankRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		cumulativeProbRankRule->GetFirstOperand()->SetAttributeName(rankRegressorAttribute->GetName());
		cumulativeProbRankRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		cumulativeProbRankRule->GetSecondOperand()->SetContinuousConstant(cNormalizedRank);

		// Ajout de l'attribut dans la classe
		cumulativeProbRankAttribute = trainedRegressor->CreatePredictionAttribute(
		    "CPR" + sPartileName + GetTargetAttributeName(), cumulativeProbRankRule);

		// Creation d'une specification d'attribut pour ajouter cumulativeProbRank au predicteur
		attributeSpec = new KWPredictionAttributeSpec;
		attributeSpec->SetLabel(sTmp + "CumulativeProbRank" + IntToString(nPartile));
		attributeSpec->SetType(KWType::Continuous);
		attributeSpec->SetMandatory(false);
		attributeSpec->SetEvaluation(false);
		attributeSpec->SetAttribute(cumulativeProbRankAttribute);
		trainedRegressor->AddPredictionAttributeSpec(attributeSpec);

		// Libelle sur l'attribut en complement de la meta-donnees geree par l'attributeSpec
		cumulativeProbRankAttribute->SetLabel(sTmp + "Partile " + IntToString(nPartile) + " on " +
						      IntToString(nPartileNumber));
	}

	// Acces a l'attribut sur les valeurs cibles
	targetValuesAttribute = trainedRegressor->GetTargetValuesAttribute();
	check(targetValuesAttribute);

	// Creation de la regle de derivation ValueRank
	valueRankRule = new KWDRValueRank;
	valueRankRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	valueRankRule->GetFirstOperand()->SetAttributeName(targetValuesAttribute->GetName());
	valueRankRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	valueRankRule->GetSecondOperand()->SetAttributeName(GetTargetAttributeName());

	// Creation de l'attribut ValueRank
	valueRankAttribute =
	    trainedRegressor->CreatePredictionAttribute("Rank" + GetTargetAttributeName(), valueRankRule);
	trainedRegressor->SetTargetAttributeRank(valueRankAttribute);

	// Creation d'une regle DensityRank
	densityRankRule = new KWDRTargetRankDensityAt;
	densityRankRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	densityRankRule->GetFirstOperand()->SetAttributeName(rankRegressorAttribute->GetName());
	densityRankRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	densityRankRule->GetSecondOperand()->SetAttributeName(valueRankAttribute->GetName());

	// Ajout de l'attribut dans la classe
	densityRankAttribute =
	    trainedRegressor->CreatePredictionAttribute("DR" + GetTargetAttributeName(), densityRankRule);
	trainedRegressor->SetDensityRankAttribute(densityRankAttribute);
}

KWAttribute* KWPredictorNaiveBayes::AddRegressorAttribute(KWTrainedRegressor* trainedRegressor,
							  KWAttribute* rankRegressorAttribute,
							  KWAttribute* targetValuesAttribute,
							  ContinuousVector* cvDataPreparationWeights)
{
	KWDRNBRegressor* regressorRule;
	KWAttribute* regressorAttribute;

	require(trainedRegressor != NULL);
	require(rankRegressorAttribute != NULL);
	require(trainedRegressor->GetPredictorClass()->LookupAttribute(rankRegressorAttribute->GetName()) ==
		rankRegressorAttribute);
	require(targetValuesAttribute != NULL);
	require(trainedRegressor->GetPredictorClass()->LookupAttribute(targetValuesAttribute->GetName()) ==
		targetValuesAttribute);

	// Creation d'une regle de regression
	if (cvDataPreparationWeights == NULL)
		regressorRule = new KWDRNBRegressor;
	else
		regressorRule = new KWDRSNBRegressor;
	regressorRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	regressorRule->GetFirstOperand()->SetAttributeName(rankRegressorAttribute->GetName());
	regressorRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	regressorRule->GetSecondOperand()->SetAttributeName(targetValuesAttribute->GetName());

	// Ajout de l'attribut dans la classe
	regressorAttribute =
	    trainedRegressor->CreatePredictionAttribute(GetPrefix() + GetTargetAttributeName(), regressorRule);
	return regressorAttribute;
}

void KWPredictorNaiveBayes::AddRegressorPredictionAttributes(KWTrainedRegressor* trainedRegressor,
							     KWAttribute* regressorAttribute,
							     KWAttribute* targetValuesAttribute)
{
	KWDRTargetMean* meanRule;
	KWAttribute* meanAttribute;
	KWDRTargetStandardDeviation* standardDeviationRule;
	KWAttribute* standardDeviationAttribute;
	KWDRTargetDensityAt* densityRule;
	KWAttribute* densityAttribute;
	KWPredictionAttributeSpec* attributeSpec;

	require(trainedRegressor != NULL);
	require(regressorAttribute != NULL);
	require(trainedRegressor->GetPredictorClass()->LookupAttribute(regressorAttribute->GetName()) ==
		regressorAttribute);
	require(targetValuesAttribute != NULL);
	require(trainedRegressor->GetPredictorClass()->LookupAttribute(targetValuesAttribute->GetName()) ==
		targetValuesAttribute);

	// Creation d'une regle de calcul de valeur moyenne
	meanRule = new KWDRTargetMean;
	meanRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	meanRule->GetFirstOperand()->SetAttributeName(regressorAttribute->GetName());

	// Ajout de l'attribut dans la classe
	meanAttribute = trainedRegressor->CreatePredictionAttribute("M" + GetTargetAttributeName(), meanRule);
	trainedRegressor->SetMeanAttribute(meanAttribute);

	// Creation d'une regle StandardDeviation
	standardDeviationRule = new KWDRTargetStandardDeviation;
	standardDeviationRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	standardDeviationRule->GetFirstOperand()->SetAttributeName(regressorAttribute->GetName());

	// Ajout de l'attribut dans la classe
	standardDeviationAttribute =
	    trainedRegressor->CreatePredictionAttribute("SD" + GetTargetAttributeName(), standardDeviationRule);

	// Creation d'une specification d'attribut pour ajouter StandardDeviation au predicteur
	attributeSpec = new KWPredictionAttributeSpec;
	attributeSpec->SetLabel("StandardDeviation");
	attributeSpec->SetType(KWType::Continuous);
	attributeSpec->SetMandatory(false);
	attributeSpec->SetEvaluation(false);
	attributeSpec->SetAttribute(standardDeviationAttribute);
	trainedRegressor->AddPredictionAttributeSpec(attributeSpec);

	// Creation d'une regle Density
	densityRule = new KWDRTargetDensityAt;
	densityRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	densityRule->GetFirstOperand()->SetAttributeName(regressorAttribute->GetName());
	densityRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	densityRule->GetSecondOperand()->SetAttributeName(trainedRegressor->GetTargetAttribute()->GetName());

	// Ajout de l'attribut dans la classe
	densityAttribute = trainedRegressor->CreatePredictionAttribute("D" + GetTargetAttributeName(), densityRule);
	trainedRegressor->SetDensityAttribute(densityAttribute);
}