// Copyright (c) 2024 Orange. All rights reserved.
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

KWPredictorSelectionReport* SNBPredictorSelectiveNaiveBayes::GetPredictorSelectionReport()
{
	return cast(KWPredictorSelectionReport*, GetPredictorReport());
}

boolean SNBPredictorSelectiveNaiveBayes::InternalTrain()
{
	boolean bOk;
	SNBPredictorSelectiveNaiveBayesTrainingTask trainingTask;

	// Delegation de l'entrainement a la tache d'apprentissage
	trainingTask.InternalTrain(this);
	bOk = trainingTask.IsTrainingSuccessful();

	return bOk;
}

void SNBPredictorSelectiveNaiveBayes::CreatePredictorReport()
{
	require(bIsTraining);
	require(predictorReport == NULL);

	predictorReport = new KWPredictorSelectionReport;
	predictorReport->SetLearningSpec(GetLearningSpec());
	predictorReport->SetPredictorName(GetName());
}

void SNBPredictorSelectiveNaiveBayes::CreatePredictorAttributesInClass(KWDataPreparationClass* dataPreparationClass,
								       ObjectArray* oaUsedDataPreparationAttributes,
								       ContinuousVector* cvAttributeWeights)
{
	KWPredictorNaiveBayes::CreatePredictorAttributesInClass(dataPreparationClass, oaUsedDataPreparationAttributes,
								cvAttributeWeights);
	FillPredictorAttributeMetaData();
}

int SNBPredictorSelectiveNaiveBayes::ComputeTrainingAttributeNumber() const
{
	int nTrainingAttributeNumber;

	require(GetClassStats() != NULL);
	require(GetClassStats()->Check());
	require(trainParameters.GetMaxEvaluatedAttributeNumber() >= 0);

	if (trainParameters.GetMaxEvaluatedAttributeNumber() > 0)
	{
		nTrainingAttributeNumber = min(GetClassStats()->GetTotalInformativeAttributeNumber(),
					       trainParameters.GetMaxEvaluatedAttributeNumber());
	}
	else
		nTrainingAttributeNumber = GetClassStats()->GetTotalInformativeAttributeNumber();

	return nTrainingAttributeNumber;
}

int SNBPredictorSelectiveNaiveBayes::ComputeTrainingSparseAttributeNumber()
{
	ObjectArray oaDataPreparationStats;
	int nTrainingSparseAttributeNumber;
	int nAttribute;
	KWDataPreparationStats* dataPreparationStats;
	KWAttribute* attribute;

	// Obtention des statistiques des attributs triees par level
	oaDataPreparationStats.CopyFrom(GetClassStats()->GetAllPreparedStats());
	oaDataPreparationStats.SetCompareFunction(KWLearningReportCompareSortValue);
	oaDataPreparationStats.Sort();

	// Calcul du nombre total d'attributs sparse
	nTrainingSparseAttributeNumber = 0;
	for (nAttribute = 0; nAttribute < oaDataPreparationStats.GetSize(); nAttribute++)
	{
		dataPreparationStats = cast(KWDataPreparationStats*, oaDataPreparationStats.GetAt(nAttribute));

		// Arret si l'attribut n'est pas informatif: Tout le reste ne le sera pas non plus
		if (not dataPreparationStats->IsInformative())
			break;

		// Arret si on depase le nombre d'attributs a utiliser dans l'entrainement
		if (nAttribute >= ComputeTrainingAttributeNumber())
			break;

		// On ignore l'attribut s'il est un paire
		if (dataPreparationStats->GetAttributeNumber() > 1)
			continue;

		// Verification que l'attribut est sparse
		// - Attribut Sparse => Attribut natif => Il est dans la classe d'apprentissage
		// - Attribut Sparse => Block non nulle
		attribute = GetClass()->LookupAttribute(dataPreparationStats->GetAttributeNameAt(0));
		if (attribute != NULL and attribute->GetAttributeBlock() != NULL)
			nTrainingSparseAttributeNumber++;
	}

	return nTrainingSparseAttributeNumber;
}

longint SNBPredictorSelectiveNaiveBayes::ComputeTrainingAttributesSparseMissingValueNumber() const
{
	IntVector* ivTrainingSparseMissingValueNumberPerAttribute;
	longint lTrainingAttributeSparseMissingValueNumber;
	int nAttribute;

	// Sum over all the sparse missing values for each attribute
	ivTrainingSparseMissingValueNumberPerAttribute = ComputeTrainingSparseMissingValueNumberPerAttribute();
	lTrainingAttributeSparseMissingValueNumber = 0;
	for (nAttribute = 0; nAttribute < ivTrainingSparseMissingValueNumberPerAttribute->GetSize(); nAttribute++)
		lTrainingAttributeSparseMissingValueNumber +=
		    longint(ivTrainingSparseMissingValueNumberPerAttribute->GetAt(nAttribute));

	ensure(lTrainingAttributeSparseMissingValueNumber >= 0);
	return lTrainingAttributeSparseMissingValueNumber;
}

IntVector* SNBPredictorSelectiveNaiveBayes::ComputeTrainingSparseMissingValueNumberPerAttribute() const
{
	ObjectArray oaDataPreparationStats;
	int nTrainingAttributeNumber;
	IntVector* ivTrainingSparseMissingValueNumberPerAttribute;
	int nAttribute;
	KWDataPreparationStats* dataPreparationStats;
	KWAttributeStats* attributeStats;

	require(GetClassStats() != NULL);
	require(GetClassStats()->Check());

	// Obtention des statistiques des attributs triees par level
	oaDataPreparationStats.CopyFrom(GetClassStats()->GetAllPreparedStats());
	oaDataPreparationStats.SetCompareFunction(KWLearningReportCompareSortValue);
	oaDataPreparationStats.Sort();

	// Initialisation a zero du vecteur en sortie
	nTrainingAttributeNumber = ComputeTrainingAttributeNumber();
	ivTrainingSparseMissingValueNumberPerAttribute = new IntVector;
	ivTrainingSparseMissingValueNumberPerAttribute->SetSize(nTrainingAttributeNumber);
	ivTrainingSparseMissingValueNumberPerAttribute->Initialize();

	// Calcul du nombre total des valeurs missing
	for (nAttribute = 0; nAttribute < oaDataPreparationStats.GetSize(); nAttribute++)
	{
		// Arret si on depase le nombre d'attributs a utiliser dans l'entrainement
		if (nAttribute >= nTrainingAttributeNumber)
			break;

		// On ignore l'attribut s'il est une paire
		dataPreparationStats = cast(KWDataPreparationStats*, oaDataPreparationStats.GetAt(nAttribute));
		if (dataPreparationStats->GetAttributeNumber() > 1)
			continue;

		// On re-cast en attribut stats pour recuperer le nombre de valeurs manquantes sparse
		attributeStats = cast(KWAttributeStats*, dataPreparationStats);
		ivTrainingSparseMissingValueNumberPerAttribute->SetAt(
		    nAttribute, attributeStats->GetDescriptiveStats()->GetSparseMissingValueNumber());
	}

	ensure(ivTrainingSparseMissingValueNumberPerAttribute->GetSize() == nTrainingAttributeNumber);

	return ivTrainingSparseMissingValueNumberPerAttribute;
}

double SNBPredictorSelectiveNaiveBayes::ComputeSparseMemoryFactor()
{
	KWDataTableSliceSet* sliceSet;
	longint lDenseContinuousAttributeMemory;
	int nAttribute;
	KWDataPreparationStats* dataPreparationStats;
	KWAttributeStats* attributeStats;
	longint lSeparatorMemoryPerInstance;
	LongintVector lvChunkSparseMemory;
	DoubleVector dvChunkSparseMemoryPerInstance;
	int nSlice;
	KWDataTableSlice* slice;
	int nChunk;
	longint lTotalSparseMemory;
	double dChunkDensityFactor;

	require(GetClassStats() != NULL);
	require(GetClassStats()->GetDataTableSliceSet() != NULL);
	require(GetClassStats()->GetDataTableSliceSet()->Check());

	// Acces au slice set
	sliceSet = GetClassStats()->GetDataTableSliceSet();

	// Estimation de la memoire totale des valeurs numeriques presents
	//
	//   #valeurs_numeriques_presents * sizeof(char)
	//
	// Elle est une sous-estimation parce que on suppose que chaque valeur utilise un byte
	lDenseContinuousAttributeMemory = 0l;
	for (nAttribute = 0; nAttribute < GetClassStats()->GetAllPreparedStats()->GetSize(); nAttribute++)
	{
		// On ignore l'attribut s'il est une paire
		dataPreparationStats =
		    cast(KWDataPreparationStats*, GetClassStats()->GetAllPreparedStats()->GetAt(nAttribute));
		if (dataPreparationStats->GetAttributeNumber() > 1)
			continue;

		// On ignore l'attribut s'il n'est pas numerique
		attributeStats = cast(KWAttributeStats*, dataPreparationStats);
		if (attributeStats->GetAttributeType() != KWType::Continuous)
			continue;

		// On ignore l'attribut s'il est sparse
		if (GetClass()->LookupAttribute(attributeStats->GetAttributeName())->GetAttributeBlock() != NULL)
			continue;

		// Ajout de la memoire des valeurs presents de l'attribut courant au total
		lDenseContinuousAttributeMemory +=
		    (longint(GetInstanceNumber()) - attributeStats->GetDescriptiveStats()->GetMissingValueNumber()) *
		    sizeof(char);
	}

	// Calcul de la memoire des separateurs des bloc sparses per chunk
	//
	// (#blocs + #attributs_denses + 1) * sizeof(char)
	//                              ^^^
	//                          fin de ligne
	lSeparatorMemoryPerInstance =
	    longint(sliceSet->GetTotalDenseAttributeNumber() + sliceSet->GetTotalAttributeBlockNumber() + 1) *
	    sizeof(char);

	// Calcul de la memoire par instance des valeurs sparse dans chaque chunk:
	// 1) Initialisation des vecteurs de tailles de chaque chunk et de densites par instance de chaque chunk
	lvChunkSparseMemory.SetSize(sliceSet->GetChunkNumber());
	lvChunkSparseMemory.Initialize();
	dvChunkSparseMemoryPerInstance.SetSize(sliceSet->GetChunkNumber());
	dvChunkSparseMemoryPerInstance.Initialize();
	for (nSlice = 0; nSlice < sliceSet->GetSliceNumber(); nSlice++)
	{
		slice = cast(KWDataTableSlice*, GetClassStats()->GetDataTableSliceSet()->GetSliceAt(nSlice));
		slice->GetDenseSymbolAttributeDiskSizes();
		for (nChunk = 0; nChunk < sliceSet->GetChunkNumber(); nChunk++)
		{
			lvChunkSparseMemory.UpgradeAt(nChunk, slice->GetDataFileSizes()->GetAt(nChunk));
			dvChunkSparseMemoryPerInstance.UpgradeAt(
			    nChunk, double(slice->GetDataFileSizes()->GetAt(nChunk)) /
					sliceSet->GetChunkInstanceNumbers()->GetAt(nChunk));
		}
	}

	// 2)  Ajustement des vecteurs et calcul de la taille totale des valeurs sparses
	lTotalSparseMemory = 0;
	for (nChunk = 0; nChunk < sliceSet->GetChunkNumber(); nChunk++)
	{
		// Enlevement des densites de la partie des valeurs denses et des separateurs
		dvChunkSparseMemoryPerInstance.UpgradeAt(nChunk, -lDenseContinuousAttributeMemory /
								     double(GetInstanceNumber()));
		dvChunkSparseMemoryPerInstance.UpgradeAt(nChunk, -sliceSet->GetTotalDenseSymbolAttributeDiskSize() /
								     double(GetInstanceNumber()));
		dvChunkSparseMemoryPerInstance.UpgradeAt(nChunk, double(-lSeparatorMemoryPerInstance));
		dvChunkSparseMemoryPerInstance.SetAt(nChunk, max(dvChunkSparseMemoryPerInstance.GetAt(nChunk), 0.0));

		// Enlevement des tailles de la partie des valeurs denses et des separateurs
		lvChunkSparseMemory.UpgradeAt(
		    nChunk, -longint(lDenseContinuousAttributeMemory *
				     double(sliceSet->GetChunkInstanceNumbers()->GetAt(nChunk)) / GetInstanceNumber()));
		lvChunkSparseMemory.UpgradeAt(
		    nChunk, -longint(sliceSet->GetTotalDenseSymbolAttributeDiskSize() *
				     double(sliceSet->GetChunkInstanceNumbers()->GetAt(nChunk)) / GetInstanceNumber()));
		lvChunkSparseMemory.UpgradeAt(nChunk, -lSeparatorMemoryPerInstance *
							  sliceSet->GetChunkInstanceNumbers()->GetAt(nChunk));
		lvChunkSparseMemory.SetAt(nChunk, max(lvChunkSparseMemory.GetAt(nChunk), 0ll));

		// Mise-a-jour de la memoire total des valeurs sparse
		lTotalSparseMemory += lvChunkSparseMemory.GetAt(nChunk);
	}

	// Calcul du facteur:
	//
	//   max_chunk(memoire des donnees sparse par instance dans un chunk) / (memoire des donnees sparse par instance)
	//
	// Cappe a 1 pour au cas ou il y a eu des erreurs d'arrondi
	// Cas normal
	dvChunkSparseMemoryPerInstance.Sort();
	if (lTotalSparseMemory > 0)
	{
		dChunkDensityFactor =
		    double(dvChunkSparseMemoryPerInstance.GetAt(dvChunkSparseMemoryPerInstance.GetSize() - 1)) /
		    (double(lTotalSparseMemory) / sliceSet->GetTotalInstanceNumber());
		dChunkDensityFactor = max(dChunkDensityFactor, 1.0);
	}
	// Cas sans valeurs sparse: On retourne le facteur par defaut
	else
		dChunkDensityFactor = 1.0;

	return dChunkDensityFactor;
}

void SNBPredictorSelectiveNaiveBayes::InternalTrainFinalizeWithEmptyPredictor()
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

void SNBPredictorSelectiveNaiveBayes::InternalTrainFinalizeWithUnivariatePredictor()
{
	KWDataPreparationClass dataPreparationClass;
	ObjectArray oaSelectedDataPreparationAttributes;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWSelectedAttributeReport* attributeReport;
	ContinuousVector cvSingleAttributeWeight;

	require(GetClassStats() != NULL);
	require(ComputeTrainingAttributeNumber() == 1);

	// Initialisation de la KWDataPreparationClass
	dataPreparationClass.SetLearningSpec(GetLearningSpec());
	dataPreparationClass.ComputeDataPreparationFromClassStats(GetClassStats());

	// Acces au seul attribut a mettre dans le modele (le plus informatif)
	oaSelectedDataPreparationAttributes.CopyFrom(dataPreparationClass.GetDataPreparationAttributes());
	oaSelectedDataPreparationAttributes.SetCompareFunction(KWDataPreparationAttributeCompareSortValue);
	oaSelectedDataPreparationAttributes.Sort();
	oaSelectedDataPreparationAttributes.SetSize(1);
	dataPreparationAttribute = cast(KWDataPreparationAttribute*, oaSelectedDataPreparationAttributes.GetAt(0));

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
	cvSingleAttributeWeight.SetSize(1);
	cvSingleAttributeWeight.SetAt(0, 1.0);

	// Creation de la classe du predicteur
	CreatePredictorAttributesInClass(&dataPreparationClass, &oaSelectedDataPreparationAttributes,
					 &cvSingleAttributeWeight);
}

void SNBPredictorSelectiveNaiveBayes::FillPredictorAttributeMetaData()
{
	const ALString sWeightMetaDataKey = "Weight";
	const ALString sImportanceMetaDataKey = "Importance";
	KWClass* predictorClass;
	ObjectDictionary oaAttributes;
	KWAttribute* attribute;
	int nAttribute;
	KWSelectedAttributeReport* attributeReport;

	require(GetTrainedPredictor() != NULL);
	require(GetTrainedPredictor()->GetPredictorClass() != NULL);

	// Acces a la classe du predicteur
	predictorClass = GetTrainedPredictor()->GetPredictorClass();

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
