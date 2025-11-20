// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorNaiveBayes.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Classe KWPredictorNaiveBayes

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
					    ObjectArray* oaDataPreparationAttributes)
{
	ObjectArray oaSelectedDataPreparationAttributes;
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;

	// Selection des attributs informatifs
	for (nAttribute = 0; nAttribute < oaDataPreparationAttributes->GetSize(); nAttribute++)
	{
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*, oaDataPreparationAttributes->GetAt(nAttribute));
		if (dataPreparationAttribute->IsInformativeOnTarget())
			oaSelectedDataPreparationAttributes.Add(dataPreparationAttribute);
	}

	// Creation des attributs du predicteur
	CreatePredictorAttributesInClass(dataPreparationClass, &oaSelectedDataPreparationAttributes, NULL);
}

void KWPredictorNaiveBayes::CreatePredictorAttributesInClass(KWDataPreparationClass* dataPreparationClass,
							     ObjectArray* oaSelectedDataPreparationAttributes,
							     ContinuousVector* cvSelectedAttributeWeights)
{
	ObjectArray oaSelectedAttributeStats;
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;

	require(dataPreparationClass != NULL);
	require(dataPreparationClass->CheckDataPreparation());
	require(oaSelectedDataPreparationAttributes != NULL);
	require(cvSelectedAttributeWeights == NULL or
		cvSelectedAttributeWeights->GetSize() == oaSelectedDataPreparationAttributes->GetSize());
	require(GetTargetDescriptiveStats()->GetValueNumber() > 0);
	require(GetPredictorReport() != NULL);
	require(GetTrainedPredictor() != NULL);

	// Extraction de la partie preparation de chaque attribut selectionne et recollection recursive ensuite
	oaSelectedAttributeStats.SetSize(oaSelectedDataPreparationAttributes->GetSize());
	for (nAttribute = 0; nAttribute < oaSelectedDataPreparationAttributes->GetSize(); nAttribute++)
	{
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*, oaSelectedDataPreparationAttributes->GetAt(nAttribute));
		oaSelectedAttributeStats.SetAt(nAttribute, dataPreparationAttribute->GetPreparedStats());
	}
	CollectSelectedPreparationStats(&oaSelectedAttributeStats);

	// Parametrage de la classe du predicteur entraine
	GetTrainedPredictor()->SetPredictorClass(dataPreparationClass->GetDataPreparationClass(),
						 GetTargetAttributeType(), GetObjectLabel());

	// Construction des attributs de prediction selon le type de predicteur
	if (GetTargetAttributeType() == KWType::Symbol)
		CreateClassifierAttributesInClass(dataPreparationClass, oaSelectedDataPreparationAttributes,
						  cvSelectedAttributeWeights);
	else if (GetTargetAttributeType() == KWType::Continuous)
		CreateRegressorAttributesInClass(dataPreparationClass, oaSelectedDataPreparationAttributes,
						 cvSelectedAttributeWeights);

	// Memorisation du nombre d'attribut utilises par le predicteur
	GetPredictorReport()->SetUsedAttributeNumber(oaSelectedDataPreparationAttributes->GetSize());

	// Nettoyage des objets liees a la preparation :
	//   La memoire de la classe dataPreparationClass->GetDataPreparationClass() n'est plus responsabilite de dataPreparationClass
	//   Elle appartient maintenant a GetTrainedPredictor()
	dataPreparationClass->RemoveDataPreparation();

	ensure(GetTrainedPredictor()->Check());
}

void KWPredictorNaiveBayes::ExtractDataPreparationStats(const ObjectArray* oaDataPreparationAttributes,
							ObjectArray* oaDataPreparationAttributeStats)
{
	KWDataPreparationAttribute* dataPreparationAttribute;
	int i;

	require(oaDataPreparationAttributes != NULL);
	require(oaDataPreparationAttributeStats != NULL);

	// Extraction de la partie preparation de chaque attribut prepare
	oaDataPreparationAttributeStats->SetSize(oaDataPreparationAttributes->GetSize());
	for (i = 0; i < oaDataPreparationAttributes->GetSize(); i++)
	{
		dataPreparationAttribute = cast(KWDataPreparationAttribute*, oaDataPreparationAttributes->GetAt(i));
		oaDataPreparationAttributeStats->SetAt(i, dataPreparationAttribute->GetPreparedStats());
	}
}

void KWPredictorNaiveBayes::CreateClassifierAttributesInClass(KWDataPreparationClass* dataPreparationClass,
							      ObjectArray* oaUsedDataPreparationAttributes,
							      ContinuousVector* cvAttributeWeights)
{
	KWAttribute* classifierAttribute;

	require(dataPreparationClass != NULL);
	require(dataPreparationClass->Check());
	require(oaUsedDataPreparationAttributes != NULL);
	require(cvAttributeWeights == NULL or
		cvAttributeWeights->GetSize() == oaUsedDataPreparationAttributes->GetSize());
	require(GetTargetAttributeType() == KWType::Symbol);
	require(GetTrainedClassifier() != NULL);

	// Memorisation de la reference a l'attribut cible
	GetTrainedClassifier()->SetTargetAttribute(
	    GetTrainedClassifier()->GetPredictorClass()->LookupAttribute(GetTargetAttributeName()));

	// Ajout a la classe du predicteur l'attribut des valeurs cibles et ses frequences
	GetTrainedClassifier()->SetTargetValuesAttribute(
	    dataPreparationClass->GetDataPreparationTargetAttribute()->GetPreparedAttribute());

	// Ajout de l'attribut classifieur et des attributs de prediction
	classifierAttribute =
	    AddClassifierAttribute(dataPreparationClass, oaUsedDataPreparationAttributes, cvAttributeWeights);
	AddClassifierPredictionAttributes(classifierAttribute);

	// Completion automatique de types
	GetTrainedClassifier()->GetPredictorClass()->CompleteTypeInfo();
}

KWAttribute* KWPredictorNaiveBayes::AddClassifierAttribute(KWDataPreparationClass* dataPreparationClass,
							   ObjectArray* oaUsedDataPreparationAttributes,
							   ContinuousVector* cvAttributeWeights)
{
	KWClass* classifierClass;
	KWDRContinuousVector* weightRule;
	KWDerivationRuleOperand* operand;
	KWDRNBClassifier* classifierRule;
	KWAttribute* classifierAttribute;

	require(dataPreparationClass != NULL);
	require(dataPreparationClass->GetDataPreparationClass() != NULL);
	require(GetTrainedPredictor()->GetPredictorClass() == dataPreparationClass->GetDataPreparationClass());
	require(oaUsedDataPreparationAttributes != NULL);
	require(cvAttributeWeights == NULL or
		cvAttributeWeights->GetSize() >= oaUsedDataPreparationAttributes->GetSize());

	// Acces a la classe du classifieur (pour la lisibilite)
	classifierClass = dataPreparationClass->GetDataPreparationClass();

	// Creation d'une regle de ponderation des grilles
	weightRule = NULL;
	if (cvAttributeWeights != NULL)
	{
		weightRule = new KWDRContinuousVector;
		weightRule->SetValueNumber(oaUsedDataPreparationAttributes->GetSize());
	}

	// Creation d'une regle de regression destinee a accueilir les grilles, avec ou sans les poids
	if (weightRule == NULL)
		classifierRule = new KWDRNBClassifier;
	else
	{
		classifierRule = new KWDRSNBClassifier;

		// Parametrage apres nettoyage de la regle issue du constructeur
		assert(classifierRule->GetFirstOperand()->GetOrigin() == KWDerivationRuleOperand::OriginRule);
		assert(classifierRule->GetFirstOperand()->GetDerivationRule() != NULL);
		delete classifierRule->GetFirstOperand()->GetDerivationRule();
		classifierRule->GetFirstOperand()->SetDerivationRule(weightRule);
	}
	classifierRule->DeleteAllVariableOperands();

	// Ajout a la regle du classifieur des operands DataGrid, DataGridStats, DataGridBlock et DataGridStatsBlock
	AddPredictorDataGridStatsAndBlockOperands(classifierRule, weightRule, dataPreparationClass,
						  oaUsedDataPreparationAttributes, cvAttributeWeights);

	// Ajout d'un dernier operande pour les valeurs cible
	operand = new KWDerivationRuleOperand;
	operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	operand->SetType(GetTrainedClassifier()->GetTargetValuesAttribute()->GetType());
	operand->SetAttributeName(GetTrainedClassifier()->GetTargetValuesAttribute()->GetName());
	operand->SetStructureName(GetTrainedClassifier()->GetTargetValuesAttribute()->GetStructureName());
	classifierRule->AddOperand(operand);

	// Creation d'un attribut de classification
	classifierAttribute =
	    GetTrainedClassifier()->CreatePredictionAttribute(GetPrefix() + GetTargetAttributeName(), classifierRule);

	return classifierAttribute;
}

void KWPredictorNaiveBayes::AddClassifierPredictionAttributes(KWAttribute* classifierAttribute)
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

	require(classifierAttribute != NULL);
	require(GetTrainedClassifier() != NULL);
	require(GetTrainedClassifier()->GetPredictorClass()->LookupAttribute(classifierAttribute->GetName()) ==
		classifierAttribute);
	require(GetTrainedClassifier()->GetTargetValuesAttribute() != NULL);
	require(GetTrainedClassifier()->GetPredictorClass()->LookupAttribute(
		    GetTrainedClassifier()->GetTargetValuesAttribute()->GetName()) ==
		GetTrainedClassifier()->GetTargetValuesAttribute());
	require(GetTargetValueStats()->GetAttributeNumber() == 1);
	require(GetTargetValueStats()->GetAttributeAt(0)->GetAttributeType() == KWType::Symbol);
	require(GetTargetValueStats()->GetAttributeAt(0)->ArePartsSingletons());

	// Creation d'une regle de prediction de la valeur cible
	predictionRule = new KWDRTargetValue;
	predictionRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	predictionRule->GetFirstOperand()->SetAttributeName(classifierAttribute->GetName());

	// Ajout de l'attribut dans la classe
	predictionAttribute =
	    GetTrainedClassifier()->CreatePredictionAttribute("Predicted" + GetTargetAttributeName(), predictionRule);
	GetTrainedClassifier()->SetPredictionAttribute(predictionAttribute);

	// Creation d'une regle de prediction du score
	scoreRule = new KWDRTargetProb;
	scoreRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	scoreRule->GetFirstOperand()->SetAttributeName(classifierAttribute->GetName());

	// Ajout de l'attribut dans la classe
	scoreAttribute =
	    GetTrainedClassifier()->CreatePredictionAttribute("Score" + GetTargetAttributeName(), scoreRule);
	GetTrainedClassifier()->SetScoreAttribute(scoreAttribute);

	// Memorisation du nombre d'attribut de prediction des probabilites cibles
	targetValues = cast(const KWDGSAttributeSymbolValues*, GetTargetValueStats()->GetAttributeAt(0));
	GetTrainedClassifier()->SetTargetValueNumber(targetValues->GetValueNumber());

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
		targetProbAttribute = GetTrainedClassifier()->CreatePredictionAttribute(
		    "Prob" + GetTargetAttributeName() + targetValues->GetValueAt(nTarget), targetProbRule);
		GetTrainedClassifier()->SetProbAttributeAt(nTarget, targetValues->GetValueAt(nTarget),
							   targetProbAttribute);
	}

	// Optimisation des offsets de prediction, et ajout d'un attribut si necessaire
	bAddBiasedPredictionAttribute =
	    classifierPostOptimizer.PostOptimize(this, GetTrainedClassifier(), &cvScoreOffsets);
	if (bAddBiasedPredictionAttribute)
	{
		// Creation d'une regle de prediction biasee de la valeur cible
		biasedPredictionRule = new KWDRBiasedTargetValue;
		biasedPredictionRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		biasedPredictionRule->GetFirstOperand()->SetAttributeName(classifierAttribute->GetName());
		assert(biasedPredictionRule->GetSecondOperand()->GetOrigin() == KWDerivationRuleOperand::OriginRule);
		assert(biasedPredictionRule->GetSecondOperand()->GetDerivationRule() != NULL);

		// Parametrage de la regle de memorisation les offsets (accessible depuis le constructeur)
		offsetRule = cast(KWDRContinuousVector*, biasedPredictionRule->GetSecondOperand()->GetDerivationRule());
		offsetRule->SetValueNumber(cvScoreOffsets.GetSize());
		for (nTarget = 0; nTarget < cvScoreOffsets.GetSize(); nTarget++)
			offsetRule->SetValueAt(nTarget, cvScoreOffsets.GetAt(nTarget));

		// Ajout de l'attribut dans la classe
		biasedPredictionAttribute = GetTrainedClassifier()->CreatePredictionAttribute(
		    "BiasedPredicted" + GetTargetAttributeName(), biasedPredictionRule);
		GetTrainedClassifier()->SetPredictionAttribute(biasedPredictionAttribute);
	}
}

void KWPredictorNaiveBayes::CreateRegressorAttributesInClass(KWDataPreparationClass* dataPreparationClass,
							     ObjectArray* oaSelectedDataPreparationAttributes,
							     ContinuousVector* cvSelectedAttributeWeights)
{
	ObjectArray oaSelectedAttributeStats;
	KWAttribute* targetAttribute;
	KWAttribute* rankRegressorAttribute;
	KWAttribute* regressorAttribute;
	KWAttribute* targetValuesAttribute;

	require(dataPreparationClass != NULL);
	require(oaSelectedDataPreparationAttributes != NULL);
	require(cvSelectedAttributeWeights == NULL or
		cvSelectedAttributeWeights->GetSize() >= oaSelectedDataPreparationAttributes->GetSize());
	require(GetTargetAttributeType() == KWType::Continuous);
	require(GetTrainedRegressor() != NULL);
	require(GetTrainedRegressor()->GetPredictorClass() != NULL);

	// Memorisation de la reference a l'attribut cible
	targetAttribute = GetTrainedRegressor()->GetPredictorClass()->LookupAttribute(GetTargetAttributeName());
	GetTrainedRegressor()->SetTargetAttribute(targetAttribute);

	// Recherche de l'attribut memorisant les valeurs cibles
	targetValuesAttribute = dataPreparationClass->GetDataPreparationTargetAttribute()->GetPreparedAttribute();
	GetTrainedRegressor()->SetTargetValuesAttribute(targetValuesAttribute);

	// Ajout de l'attribut regresseur de rang
	rankRegressorAttribute = AddRankRegressorAttribute(dataPreparationClass, oaSelectedDataPreparationAttributes,
							   cvSelectedAttributeWeights);

	// Ajout des attribut de prediction pour la regression de rang
	AddRankRegressorPredictionAttributes(rankRegressorAttribute);

	// Ajout de l'attribut regresseur de valeur
	regressorAttribute =
	    AddRegressorAttribute(rankRegressorAttribute, targetValuesAttribute, cvSelectedAttributeWeights);

	// Ajout des attribut de prediction pour la regression de valeur
	AddRegressorPredictionAttributes(GetTrainedRegressor(), regressorAttribute, targetValuesAttribute);

	// Completion automatique des informations de la classe (nom de classe par regle...)
	GetTrainedRegressor()->GetPredictorClass()->CompleteTypeInfo();
}

KWAttribute* KWPredictorNaiveBayes::AddRankRegressorAttribute(KWDataPreparationClass* dataPreparationClass,
							      ObjectArray* oaUsedDataPreparationAttributes,
							      ContinuousVector* cvAttributeWeights)
{
	KWClass* rankRegressorClass;
	KWDRContinuousVector* weightRule;
	int nAttribute;
	KWDRNBRankRegressor* rankRegressorRule;
	KWDataGridStats emptyDataGridStats;
	KWDRDataGrid* emptyDataGridRule;
	KWDRDataGridStats* dataGridStatsRule;
	KWDerivationRuleOperand* operand;
	KWAttribute* rankRegressorAttribute;
	KWDGSAttributeDiscretization* emptyDataGridAttribute;

	require(oaUsedDataPreparationAttributes != NULL);
	require(cvAttributeWeights == NULL or
		cvAttributeWeights->GetSize() >= oaUsedDataPreparationAttributes->GetSize());
	require(GetTrainedRegressor() != NULL);

	// Acces a la classe du regresseur (pour la lisibilite)
	rankRegressorClass = dataPreparationClass->GetDataPreparationClass();

	// Creation d'une regle de ponderation des grilles
	weightRule = NULL;
	if (cvAttributeWeights != NULL)
	{
		weightRule = new KWDRContinuousVector;
		weightRule->SetValueNumber(oaUsedDataPreparationAttributes->GetSize());
		for (nAttribute = 0; nAttribute < oaUsedDataPreparationAttributes->GetSize(); nAttribute++)
			weightRule->SetValueAt(nAttribute, cvAttributeWeights->GetAt(nAttribute));
	}

	// Creation d'une regle de regression destinee a accueilir les grilles, avec ou sans les poids
	if (weightRule == NULL)
		rankRegressorRule = new KWDRNBRankRegressor;
	else
	{
		rankRegressorRule = new KWDRSNBRankRegressor;

		// Parametrage apres nettoyage de la regle issue du constructeur
		assert(rankRegressorRule->GetFirstOperand()->GetOrigin() == KWDerivationRuleOperand::OriginRule);
		assert(rankRegressorRule->GetFirstOperand()->GetDerivationRule() != NULL);
		delete rankRegressorRule->GetFirstOperand()->GetDerivationRule();
		rankRegressorRule->GetFirstOperand()->SetDerivationRule(weightRule);
	}
	rankRegressorRule->DeleteAllVariableOperands();

	// S'il y a des attributs predictifs:
	//   Ajout a la regle du regresseur des operands DataGrid, DataGridStats, DataGridBlock et DataGridStatsBlock
	if (oaUsedDataPreparationAttributes->GetSize() > 0)
	{
		AddPredictorDataGridStatsAndBlockOperands(rankRegressorRule, weightRule, dataPreparationClass,
							  oaUsedDataPreparationAttributes, cvAttributeWeights);
		rankRegressorClass->CompleteTypeInfo();
	}
	// S'il n'y a aucun attribut predictif:
	//   Le regresseur de rang a besoin neanmoins de l'effectif total, qui venait avec les grilles en
	//   entree; on genere une grille a une cellule pour indiquer cet effectif
	else
	{
		// Creation d'une grille a une cellule
		emptyDataGridAttribute = new KWDGSAttributeDiscretization;
		emptyDataGridAttribute->SetAttributeName("Empty");
		emptyDataGridStats.AddAttribute(emptyDataGridAttribute);
		emptyDataGridStats.CreateAllCells();
		emptyDataGridStats.SetUnivariateCellFrequencyAt(0, GetInstanceNumber());
		assert(emptyDataGridStats.Check());

		// Creation d'une regle de type grille
		emptyDataGridRule = new KWDRDataGrid;
		emptyDataGridRule->ImportDataGridStats(&emptyDataGridStats, false);

		// Creation d'une regle DataGridStats
		dataGridStatsRule = new KWDRDataGridStats;
		dataGridStatsRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginRule);
		dataGridStatsRule->GetFirstOperand()->SetDerivationRule(emptyDataGridRule);
		dataGridStatsRule->DeleteAllVariableOperands();

		// Ajout d'un operande DataGridStats au predicteur
		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		operand->SetDerivationRule(dataGridStatsRule);
		operand->SetType(dataGridStatsRule->GetType());
		operand->SetStructureName(dataGridStatsRule->GetStructureName());
		rankRegressorRule->AddOperand(operand);
	}

	// Creation d'un attribut de regression
	rankRegressorAttribute = GetTrainedRegressor()->CreatePredictionAttribute(
	    GetPrefix() + "R" + GetTargetAttributeName(), rankRegressorRule);
	return rankRegressorAttribute;
}

void KWPredictorNaiveBayes::AddRankRegressorPredictionAttributes(KWAttribute* rankRegressorAttribute)
{
	const int nPartileNumber = 5;
	KWDRTargetRankMean* meanRankRule;
	KWAttribute* meanRankAttribute;
	KWDRTargetRankStandardDeviation* standardDeviationRankRule;
	KWAttribute* standardDeviationRankAttribute;
	KWPredictionAttributeSpec* attributeSpec;
	int nPartile;
	Continuous cNormalizedRank;
	ALString sPartileName;
	KWDRTargetRankCumulativeProbAt* cumulativeProbRankRule;
	KWAttribute* cumulativeProbRankAttribute;
	KWDRValueRank* valueRankRule;
	KWAttribute* valueRankAttribute;
	KWDRTargetRankDensityAt* densityRankRule;
	KWAttribute* densityRankAttribute;
	ALString sTmp;

	require(GetTrainedRegressor() != NULL);
	require(rankRegressorAttribute != NULL);
	require(rankRegressorAttribute ==
		GetTrainedRegressor()->GetPredictorClass()->LookupAttribute(rankRegressorAttribute->GetName()));
	require(GetTrainedRegressor()->GetTargetValuesAttribute() != NULL);
	require(GetTrainedRegressor()->GetTargetValuesAttribute() ==
		GetTrainedRegressor()->GetPredictorClass()->LookupAttribute(
		    GetTrainedRegressor()->GetTargetValuesAttribute()->GetName()));

	// Creation et ajout de l'attribute pour la regle KWDRTargetMeanRank
	meanRankRule = new KWDRTargetRankMean;
	meanRankRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	meanRankRule->GetFirstOperand()->SetAttributeName(rankRegressorAttribute->GetName());
	meanRankAttribute =
	    GetTrainedRegressor()->CreatePredictionAttribute("MR" + GetTargetAttributeName(), meanRankRule);
	GetTrainedRegressor()->SetMeanRankAttribute(meanRankAttribute);

	// Creation et ajout de l'attribut pour la regle StandardDeviationRank
	standardDeviationRankRule = new KWDRTargetRankStandardDeviation;
	standardDeviationRankRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	standardDeviationRankRule->GetFirstOperand()->SetAttributeName(rankRegressorAttribute->GetName());
	standardDeviationRankAttribute = GetTrainedRegressor()->CreatePredictionAttribute(
	    "SDR" + GetTargetAttributeName(), standardDeviationRankRule);

	// Creation d'une specification d'attribut pour ajouter StandardDeviationRank au predicteur
	attributeSpec = new KWPredictionAttributeSpec;
	attributeSpec->SetLabel("StandardDeviationRank");
	attributeSpec->SetType(KWType::Continuous);
	attributeSpec->SetMandatory(false);
	attributeSpec->SetEvaluation(false);
	attributeSpec->SetAttribute(standardDeviationRankAttribute);
	GetTrainedRegressor()->AddPredictionAttributeSpec(attributeSpec);

	// Creation d'attributs pour la fonction de repartition
	for (nPartile = 1; nPartile <= nPartileNumber; nPartile++)
	{
		cNormalizedRank = Continuous(nPartile) / nPartileNumber;
		sPartileName = IntToString(nPartile);

		// Creation et ajout de l'attribut pour la regle CumulativeProbRank
		cumulativeProbRankRule = new KWDRTargetRankCumulativeProbAt;
		cumulativeProbRankRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		cumulativeProbRankRule->GetFirstOperand()->SetAttributeName(rankRegressorAttribute->GetName());
		cumulativeProbRankRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginConstant);
		cumulativeProbRankRule->GetSecondOperand()->SetContinuousConstant(cNormalizedRank);
		cumulativeProbRankAttribute = GetTrainedRegressor()->CreatePredictionAttribute(
		    "CPR" + sPartileName + GetTargetAttributeName(), cumulativeProbRankRule);

		// Creation d'une specification d'attribut pour ajouter cumulativeProbRank au predicteur
		attributeSpec = new KWPredictionAttributeSpec;
		attributeSpec->SetLabel(sTmp + "CumulativeProbRank" + IntToString(nPartile));
		attributeSpec->SetType(KWType::Continuous);
		attributeSpec->SetMandatory(false);
		attributeSpec->SetEvaluation(false);
		attributeSpec->SetAttribute(cumulativeProbRankAttribute);
		GetTrainedRegressor()->AddPredictionAttributeSpec(attributeSpec);

		// Libelle sur l'attribut en complement de la meta-donnees geree par l'attributeSpec
		cumulativeProbRankAttribute->SetLabel(sTmp + "Partile " + IntToString(nPartile) + " on " +
						      IntToString(nPartileNumber));
	}

	// Creation et ajout l'attribut pour la regle ValueRank
	valueRankRule = new KWDRValueRank;
	valueRankRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	valueRankRule->GetFirstOperand()->SetAttributeName(
	    GetTrainedRegressor()->GetTargetValuesAttribute()->GetName());
	valueRankRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	valueRankRule->GetSecondOperand()->SetAttributeName(GetTargetAttributeName());
	valueRankAttribute =
	    GetTrainedRegressor()->CreatePredictionAttribute("Rank" + GetTargetAttributeName(), valueRankRule);
	GetTrainedRegressor()->SetTargetAttributeRank(valueRankAttribute);

	// Creation et ajout l'attribut pour la regle DensityRank
	densityRankRule = new KWDRTargetRankDensityAt;
	densityRankRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	densityRankRule->GetFirstOperand()->SetAttributeName(rankRegressorAttribute->GetName());
	densityRankRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	densityRankRule->GetSecondOperand()->SetAttributeName(valueRankAttribute->GetName());
	densityRankAttribute =
	    GetTrainedRegressor()->CreatePredictionAttribute("DR" + GetTargetAttributeName(), densityRankRule);
	GetTrainedRegressor()->SetDensityRankAttribute(densityRankAttribute);
}

KWAttribute* KWPredictorNaiveBayes::AddRegressorAttribute(KWAttribute* rankRegressorAttribute,
							  KWAttribute* targetValuesAttribute,
							  ContinuousVector* cvAttributeWeights)
{
	KWDRNBRegressor* regressorRule;
	KWAttribute* regressorAttribute;

	require(GetTrainedRegressor() != NULL);
	require(rankRegressorAttribute != NULL);
	require(rankRegressorAttribute ==
		GetTrainedRegressor()->GetPredictorClass()->LookupAttribute(rankRegressorAttribute->GetName()));
	require(targetValuesAttribute != NULL);
	require(GetTrainedRegressor()->GetPredictorClass()->LookupAttribute(targetValuesAttribute->GetName()) ==
		targetValuesAttribute);

	// Creation et ajout de l'attribut pour la regle du regresseur
	if (cvAttributeWeights == NULL)
		regressorRule = new KWDRNBRegressor;
	else
		regressorRule = new KWDRSNBRegressor;
	regressorRule->GetFirstOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	regressorRule->GetFirstOperand()->SetAttributeName(rankRegressorAttribute->GetName());
	regressorRule->GetSecondOperand()->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
	regressorRule->GetSecondOperand()->SetAttributeName(targetValuesAttribute->GetName());
	regressorAttribute =
	    GetTrainedRegressor()->CreatePredictionAttribute(GetPrefix() + GetTargetAttributeName(), regressorRule);

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

void KWPredictorNaiveBayes::AddPredictorDataGridStatsAndBlockOperands(KWDerivationRule* predictorRule,
								      KWDRContinuousVector* weightRule,
								      KWDataPreparationClass* dataPreparationClass,
								      ObjectArray* oaUsedDataPreparationAttributes,
								      ContinuousVector* cvAttributeWeights)
{
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWDRDataGridStats* dataGridStatsRule;
	NumericKeyDictionary nkdBlockDataPreparationAttributeWeights;
	ContinuousObject* coBlockDataPreparationAttributeWeight;
	ObjectArray oaDataPreparationAttributesByBlock;
	ObjectArray* oaDataGridStatsBlockDataPreparationAttributes;
	ObjectDictionary odDataPreparationAttributesByBlock;
	ALString sAttributeBlockName;
	POSITION position;
	Object* oElement;
	KWDerivationRuleOperand* operand;
	KWAttribute* dataGridBlockAttribute;
	KWAttributeBlock* nativeAttributeBlock;
	KWDRDataGridStatsBlock* dataGridStatsBlockRule;
	int nCurrentWeightIndex;

	require(cvAttributeWeights != NULL or weightRule == NULL);
	require(cvAttributeWeights == NULL or weightRule != NULL);

	// Ajout d'un attribut de type grille de donnees par attribut prepare
	nCurrentWeightIndex = 0;
	for (nAttribute = 0; nAttribute < oaUsedDataPreparationAttributes->GetSize(); nAttribute++)
	{
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*, oaUsedDataPreparationAttributes->GetAt(nAttribute));

		// Si l'attribut est dans un bloc on reporte son aggregation a la fin de la methode
		if (not GetSNBForceDenseMode() and dataPreparationAttribute->IsNativeAttributeInBlock())
		{
			assert(dataPreparationAttribute->GetNativeAttributeNumber() == 1);
			sAttributeBlockName =
			    dataPreparationAttribute->GetNativeAttribute()->GetAttributeBlock()->GetName();
			oaDataGridStatsBlockDataPreparationAttributes =
			    cast(ObjectArray*, odDataPreparationAttributesByBlock.Lookup(sAttributeBlockName));

			// Ajout a dictionnaire des blocs si l'attribute du bloc n'y est pas encore
			if (oaDataGridStatsBlockDataPreparationAttributes == NULL)
			{
				oaDataGridStatsBlockDataPreparationAttributes = new ObjectArray();
				odDataPreparationAttributesByBlock.SetAt(sAttributeBlockName,
									 oaDataGridStatsBlockDataPreparationAttributes);
			}

			// Memorisation du dataPreparationAttribute et son poids si necessaire
			oaDataGridStatsBlockDataPreparationAttributes->Add(dataPreparationAttribute);
			if (weightRule != NULL)
			{
				coBlockDataPreparationAttributeWeight = new ContinuousObject;
				coBlockDataPreparationAttributeWeight->SetContinuous(
				    cvAttributeWeights->GetAt(nAttribute));
				nkdBlockDataPreparationAttributeWeights.SetAt(dataPreparationAttribute,
									      coBlockDataPreparationAttributeWeight);
			}
		}
		else
		{
			// Creation d'une regle DataGridStats
			dataGridStatsRule = dataPreparationAttribute->CreatePreparedStatsRule();

			// Ajout du poids de l'attribut
			if (weightRule != NULL)
				weightRule->SetValueAt(nCurrentWeightIndex, cvAttributeWeights->GetAt(nAttribute));

			// Ajout d'un operande DataGridStats au predicteur
			operand = new KWDerivationRuleOperand;
			operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
			operand->SetDerivationRule(dataGridStatsRule);
			operand->SetType(dataGridStatsRule->GetType());
			operand->SetStructureName(dataGridStatsRule->GetStructureName());
			predictorRule->AddOperand(operand);
			nCurrentWeightIndex++;
		}
	}

	// Ajout des regles DataGridStatsBlock pour les attributs provenant des blocs
	position = odDataPreparationAttributesByBlock.GetStartPosition();
	while (position != NULL)
	{
		odDataPreparationAttributesByBlock.GetNextAssoc(position, sAttributeBlockName, oElement);
		nativeAttributeBlock =
		    dataPreparationClass->GetDataPreparationClass()->LookupAttributeBlock(sAttributeBlockName);
		oaDataGridStatsBlockDataPreparationAttributes = cast(ObjectArray*, oElement);

		dataGridBlockAttribute =
		    dataPreparationClass->AddDataGridBlock(oaDataGridStatsBlockDataPreparationAttributes, "PB");

		// Ajout de la regle DataGridStatsBlock
		dataGridStatsBlockRule = new KWDRDataGridStatsBlock;
		dataGridStatsBlockRule->DeleteAllOperands();
		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		operand->SetAttributeName(dataGridBlockAttribute->GetName());
		dataGridStatsBlockRule->AddOperand(operand);
		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginAttribute);
		operand->SetAttributeBlockName(sAttributeBlockName);
		dataGridStatsBlockRule->AddOperand(operand);

		// Ajout de la regle DataGridBlockStats comme operande du classifieur
		operand = new KWDerivationRuleOperand;
		operand->SetOrigin(KWDerivationRuleOperand::OriginRule);
		operand->SetDerivationRule(dataGridStatsBlockRule);
		predictorRule->AddOperand(operand);

		// Ajout des poids des attributs du bloc
		if (weightRule != NULL)
		{
			for (nAttribute = 0; nAttribute < oaDataGridStatsBlockDataPreparationAttributes->GetSize();
			     nAttribute++)
			{
				dataPreparationAttribute =
				    cast(KWDataPreparationAttribute*,
					 oaDataGridStatsBlockDataPreparationAttributes->GetAt(nAttribute));
				coBlockDataPreparationAttributeWeight = cast(
				    ContinuousObject*,
				    nkdBlockDataPreparationAttributeWeights.Lookup((NUMERIC)dataPreparationAttribute));
				assert(coBlockDataPreparationAttributeWeight != NULL);
				weightRule->SetValueAt(nCurrentWeightIndex,
						       coBlockDataPreparationAttributeWeight->GetContinuous());
				nCurrentWeightIndex++;
			}
		}
		// Nettogaye du tableau d'attributs selectionnes
		oaDataGridStatsBlockDataPreparationAttributes->RemoveAll();
	}

	// Nettoyage du dictionnaire des atttributs de preparation par bloc
	odDataPreparationAttributesByBlock.DeleteAll();
	nkdBlockDataPreparationAttributeWeights.DeleteAll();

	ensure(weightRule == NULL or nCurrentWeightIndex == oaUsedDataPreparationAttributes->GetSize());
}
