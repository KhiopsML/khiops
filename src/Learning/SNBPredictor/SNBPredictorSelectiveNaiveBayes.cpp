// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SNBPredictorSelectiveNaiveBayes.h"

SNBPredictorSelectiveNaiveBayes::SNBPredictorSelectiveNaiveBayes() {}

SNBPredictorSelectiveNaiveBayes::~SNBPredictorSelectiveNaiveBayes() {}

boolean SNBPredictorSelectiveNaiveBayes::IsTargetTypeManaged(int nType) const
{
	return nType == KWType::Symbol or nType == KWType::Continuous;
}

KWPredictor* SNBPredictorSelectiveNaiveBayes::Create() const
{
	return new SNBPredictorSelectiveNaiveBayes;
}

void SNBPredictorSelectiveNaiveBayes::CopyFrom(const KWPredictor* sourcePredictor)
{
	SNBPredictorSelectiveNaiveBayes* snbPredictor;

	require(sourcePredictor != NULL);

	// Appel de la methode ancetre
	KWPredictorNaiveBayes::CopyFrom(sourcePredictor);

	// Recopie des parametres de selection de variable
	snbPredictor = cast(SNBPredictorSelectiveNaiveBayes*, sourcePredictor);
	selectionParameters.CopyFrom(&snbPredictor->selectionParameters);
}

const ALString SNBPredictorSelectiveNaiveBayes::GetName() const
{
	return "Selective Naive Bayes";
}

const ALString SNBPredictorSelectiveNaiveBayes::GetPrefix() const
{
	return "SNB";
}

KWSelectionParameters* SNBPredictorSelectiveNaiveBayes::GetSelectionParameters()
{
	return &selectionParameters;
}

int SNBPredictorSelectiveNaiveBayes::GetTrainingAttributeNumber()
{
	int nTotalInformativeAttributeNumber;
	int nTrainingAttributeNumber;

	require(GetClassStats() != NULL);
	require(GetClassStats()->Check());
	require(GetTrainParameters()->GetMaxEvaluatedAttributeNumber() >= 0);

	nTotalInformativeAttributeNumber = GetClassStats()->GetTotalInformativeAttributeNumber();

	if (GetTrainParameters()->GetMaxEvaluatedAttributeNumber() > 0)
		nTrainingAttributeNumber =
		    min(nTotalInformativeAttributeNumber, GetTrainParameters()->GetMaxEvaluatedAttributeNumber());
	else
		nTrainingAttributeNumber = nTotalInformativeAttributeNumber;

	return nTrainingAttributeNumber;
}

KWPredictorSelectionReport* SNBPredictorSelectiveNaiveBayes::GetPredictorSelectionReport()
{
	return cast(KWPredictorSelectionReport*, GetPredictorReport());
}

void SNBPredictorSelectiveNaiveBayes::CreatePredictorReport()
{
	require(bIsTraining);
	require(predictorReport == NULL);

	predictorReport = new KWPredictorSelectionReport;
	predictorReport->SetLearningSpec(GetLearningSpec());
	predictorReport->SetPredictorName(GetName());
}

boolean SNBPredictorSelectiveNaiveBayes::InternalTrain()
{
	boolean bOk;
	SNBPredictorSNBTrainingTask* trainingTask;

	// Delegation de l'entrainement a la tache d'apprentissage
	trainingTask = new SNBPredictorSNBDirectTrainingTask;
	trainingTask->InternalTrain(this);
	bOk = trainingTask->IsTrainingSuccessful();

	// Nettoyage
	delete trainingTask;

	return bOk;
}

void SNBPredictorSelectiveNaiveBayes::InternalTrainEmptyPredictor()
{
	ObjectArray oaEmptyAttributes;
	KWDataPreparationClass dataPreparationClass;

	// Initialisation de la KWDataPreparationClass
	dataPreparationClass.SetLearningSpec(GetLearningSpec());
	dataPreparationClass.ComputeDataPreparationFromClassStats(GetClassStats());

	// Entrainement d'un predicteur vide avec warning utilisateur
	AddWarning("No informative input variable available");
	InternalTrainNB(&dataPreparationClass, &oaEmptyAttributes);
}

void SNBPredictorSelectiveNaiveBayes::InternalTrainUnivariatePredictor()
{
	KWDataPreparationClass dataPreparationClass;
	ObjectArray oaDataPreparationAttributes;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWSelectedAttributeReport* attributeReport;
	ContinuousVector cvSingleAttributeWeight;

	require(GetClassStats() != NULL);
	require(GetTrainingAttributeNumber() == 1);

	// Initialisation de la KWDataPreparationClass
	dataPreparationClass.SetLearningSpec(GetLearningSpec());
	dataPreparationClass.ComputeDataPreparationFromClassStats(GetClassStats());

	// Acces au seul attribut a mettre dans le modele (le plus informatif)
	oaDataPreparationAttributes.CopyFrom(dataPreparationClass.GetDataPreparationAttributes());
	oaDataPreparationAttributes.SetCompareFunction(KWDataPreparationAttributeCompareSortValue);
	oaDataPreparationAttributes.Sort();
	dataPreparationAttribute = cast(KWDataPreparationAttribute*, oaDataPreparationAttributes.GetAt(0));

	// (Re)creation du rapport de selection
	GetPredictorSelectionReport()->GetSelectedAttributes()->DeleteAll();
	attributeReport = new KWSelectedAttributeReport;
	attributeReport->SetNativeAttributeName(dataPreparationAttribute->ComputeNativeAttributeName());
	attributeReport->SetPreparedAttributeName(dataPreparationAttribute->GetPreparedAttribute()->GetName());
	attributeReport->SetUnivariateEvaluation(dataPreparationAttribute->GetPreparedStats()->GetLevel());
	attributeReport->SetWeight(1.0);
	GetPredictorSelectionReport()->GetSelectedAttributes()->Add(attributeReport);
	GetPredictorSelectionReport()->SetUsedAttributeNumber(1);

	// Preparation des poids: le seul attribut selectionne a un poids de 1.0, le reste zero
	cvSingleAttributeWeight.SetSize(dataPreparationClass.GetDataPreparationAttributes()->GetSize());
	cvSingleAttributeWeight.Initialize();
	cvSingleAttributeWeight.SetAt(dataPreparationAttribute->GetIndex(), 1.0);

	// Creation de la classe du predicteur
	InternalTrainWNB(&dataPreparationClass, dataPreparationClass.GetDataPreparationAttributes(),
			 &cvSingleAttributeWeight);
	FillPredictorAttributeMetaData(GetTrainedPredictor()->GetPredictorClass());
}

// TODO: Eliminer une fois le MAP elimine de Khiops
void SNBPredictorSelectiveNaiveBayes::InternalTrainMAP(KWDataPreparationClass* dataPreparationClass,
						       SNBHardAttributeSelection* mapAttributeSelection)
{
	ObjectArray* oaMapAttributes;
	ObjectArray oaMapDataPreparationAttributes;
	int nAttribute;
	SNBDataTableBinarySliceSetAttribute* attribute;
	KWDataPreparationAttribute* dataPreparationAttribute;

	require(mapAttributeSelection != NULL);
	require(mapAttributeSelection->GetAttributeNumber());

	// Obtention des attributes selectionnes dans l'ordre original de la classe
	oaMapAttributes = mapAttributeSelection->CollectAttributes();
	oaMapAttributes->SetCompareFunction(SNBDataTableBinarySliceSetAttributeCompareDataPreparationClassIndex);
	oaMapAttributes->Sort();

	// Obtention des KWDataPreparationAttribute's selectionnes et apprentissage avec
	oaMapDataPreparationAttributes.SetSize(oaMapAttributes->GetSize());
	for (nAttribute = 0; nAttribute < oaMapAttributes->GetSize(); nAttribute++)
	{
		attribute = cast(SNBDataTableBinarySliceSetAttribute*, oaMapAttributes->GetAt(nAttribute));
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*, dataPreparationClass->GetDataPreparationAttributes()->GetAt(
							  attribute->GetDataPreparationClassIndex()));
		oaMapDataPreparationAttributes.SetAt(nAttribute, dataPreparationAttribute);
	}
	InternalTrainNB(dataPreparationClass, &oaMapDataPreparationAttributes);

	// Nettoyage
	delete oaMapAttributes;
}

void SNBPredictorSelectiveNaiveBayes::FillPredictorAttributeMetaData(KWClass* predictorClass)
{
	const ALString sWeightMetaDataKey = "Weight";
	const ALString sImportanceMetaDataKey = "Importance";
	ObjectDictionary oaAttributes;
	KWAttribute* attribute;
	int nAttribute;
	KWSelectedAttributeReport* attributeReport;

	require(predictorClass != NULL);

	// Nettoyage initial des meta-donnees
	predictorClass->RemoveAllAttributesMetaDataKey(sWeightMetaDataKey);
	predictorClass->RemoveAllAttributesMetaDataKey(sImportanceMetaDataKey);

	// On range les attribut du classifieurs dans un dictionnaire, pour avoir un acces rapide
	attribute = predictorClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		oaAttributes.SetAt(attribute->GetName(), attribute);
		predictorClass->GetNextAttribute(attribute);
	}

	// Parcours des attributs selectionnes pour rechercher les informations d'apprentissage
	for (nAttribute = 0; nAttribute < GetPredictorSelectionReport()->GetSelectedAttributes()->GetSize();
	     nAttribute++)
	{
		attributeReport = cast(KWSelectedAttributeReport*,
				       GetPredictorSelectionReport()->GetSelectedAttributes()->GetAt(nAttribute));

		// Mise a jour de la version native de l'attribut
		attribute = predictorClass->LookupAttribute(attributeReport->GetNativeAttributeName());
		if (attribute != NULL)
		{
			attribute->GetMetaData()->SetDoubleValueAt(sWeightMetaDataKey, attributeReport->GetWeight());
			attribute->GetMetaData()->SetDoubleValueAt(sImportanceMetaDataKey,
								   attributeReport->GetImportance());
		}
	}
}
