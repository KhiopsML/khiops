// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorSelectiveNaiveBayes.h"

KWPredictorSelectiveNaiveBayes::KWPredictorSelectiveNaiveBayes()
{
	predictorSelectionScoreManager = NULL;
	weightManager = NULL;
	dEpsilon = 1e-6;
}

KWPredictorSelectiveNaiveBayes::~KWPredictorSelectiveNaiveBayes()
{
	assert(weightManager == NULL);
	assert(predictorSelectionScoreManager == NULL);
}

boolean KWPredictorSelectiveNaiveBayes::IsTargetTypeManaged(int nType) const
{
	return nType == KWType::Symbol or nType == KWType::Continuous;
}

KWPredictor* KWPredictorSelectiveNaiveBayes::Create() const
{
	return new KWPredictorSelectiveNaiveBayes;
}

void KWPredictorSelectiveNaiveBayes::CopyFrom(const KWPredictor* kwpSource)
{
	KWPredictorSelectiveNaiveBayes* kwpsnbSource;

	require(kwpSource != NULL);

	// Appel de la methode ancetre
	KWPredictor::CopyFrom(kwpSource);

	// Recopie des parametres de selection de variable
	kwpsnbSource = cast(KWPredictorSelectiveNaiveBayes*, kwpSource);
	selectionParameters.CopyFrom(&kwpsnbSource->selectionParameters);
}

const ALString KWPredictorSelectiveNaiveBayes::GetName() const
{
	return "Selective Naive Bayes";
}

const ALString KWPredictorSelectiveNaiveBayes::GetPrefix() const
{
	return "SNB";
}

KWSelectionParameters* KWPredictorSelectiveNaiveBayes::GetSelectionParameters()
{
	return &selectionParameters;
}

KWPredictorSelectionReport* KWPredictorSelectiveNaiveBayes::GetPredictorSelectionReport()
{
	return cast(KWPredictorSelectionReport*, GetPredictorReport());
}

void KWPredictorSelectiveNaiveBayes::CreatePredictorReport()
{
	require(bIsTraining);
	require(predictorReport == NULL);

	predictorReport = new KWPredictorSelectionReport;
	predictorReport->SetLearningSpec(GetLearningSpec());
	predictorReport->SetPredictorName(GetName());
}

boolean KWPredictorSelectiveNaiveBayes::InternalTrain()
{
	boolean bTrace = false;
	boolean bOk;
	KWDataPreparationClass* dataPreparationClass;
	KWDataPreparationBase dataPreparationBase;
	double dCurrentPredictorCost;
	NumericKeyDictionary* nkdSelectedAttributes;
	ObjectArray oaSelectedDataPreparationAttributes;
	Timer timerTrain;
	ALString sTmp;

	require(Check());
	require(GetClassStats() != NULL);
	require(GetClassStats()->IsStatsComputed());
	require(GetClassStats()->GetDataTableSliceSet()->Check());
	require(weightManager == NULL);
	require(predictorSelectionScoreManager == NULL);

	// Nettoyage des resultats de selection
	GetPredictorSelectionReport()->GetSelectedAttributes()->DeleteAll();

	// Initialisation des donnees de travail locales a la methode
	nkdSelectedAttributes = NULL;

	// Apprentissage si au moins une valeur cible
	bOk = false;
	if (GetLearningSpec()->IsTargetStatsComputed() and GetTargetDescriptiveStats()->GetValueNumber() > 0)
	{
		bOk = true;

		// Temps initial
		if (bTrace)
			cout << "SNB: Begin" << endl;
		timerTrain.Start();

		///////////////////////////////////////////////////////////////////////////////
		// Preparation des donnees
		// Certaines methodes ont des besoins potentiellement important en ressources
		// memoire et peuvent echouer. Des message d'erreurs sont alors emis.

		// Libelle de tache: preparation
		TaskProgression::DisplayLabel("Preparation");

		// Calcul des specifications de preparation
		dataPreparationBase.SetLearningSpec(GetLearningSpec());
		dataPreparationClass = dataPreparationBase.GetDataPreparationClass();
		dataPreparationClass->SetLearningSpec(GetLearningSpec());
		dataPreparationClass->ComputeDataPreparationFromClassStats(GetClassStats());

		// Calcul des attributs a utiliser dans la base de preparation des donnees
		ComputeUsableAttributes(&dataPreparationBase);

		// Creation du gestionnaire des scores d'evaluation pour les algorithmes de selection
		if (GetTargetAttributeType() == KWType::Symbol)
		{
			if (IsTargetGrouped())
				predictorSelectionScoreManager = new KWGeneralizedClassifierSelectionScore;
			else
				predictorSelectionScoreManager = new KWClassifierSelectionScore;
		}
		else
			predictorSelectionScoreManager = new KWRegressorSelectionScore;
		predictorSelectionScoreManager->SetLearningSpec(GetLearningSpec());
		predictorSelectionScoreManager->SetDataPreparationBase(&dataPreparationBase);

		// OBSOLETE
		//  Transmission du parametrage sur la prise en compte des cout de construction/preparation
		//  Necessaire pour definir le cout dans ComputeSelectionModelAttributeCost(NewPrior)
		predictorSelectionScoreManager->SetPriorWeight(GetSelectionParameters()->GetPriorWeight());
		predictorSelectionScoreManager->SetConstructionCost(GetSelectionParameters()->GetConstructionCost());
		predictorSelectionScoreManager->SetPreparationCost(GetSelectionParameters()->GetPreparationCost());

		// Creation des variables de travail du gestionnaire de score
		if (bTrace)
			cout << "SNB: CreateWorkingData" << endl;
		predictorSelectionScoreManager->CreateWorkingData();

		// Calcul de la base de preparation, s'il n'y a pas eu de probleme d'allocation memoire
		// pour les variables de travail
		if (bTrace)
			cout << "SNB: ComputePreparedData" << endl;
		if (predictorSelectionScoreManager->IsWorkingDataCreated())
			dataPreparationBase.ComputePreparedData();

		// Apprentissage du predicteur baseline si pas d'attribut predictif
		if (dataPreparationBase.GetDataPreparationUsedAttributes()->GetSize() == 0)
		{
			// Emission d'un warning
			assert(oaSelectedDataPreparationAttributes.GetSize() == 0);
			AddWarning(sTmp + "No informative input variable available");
			InternalTrainNB(dataPreparationClass, &oaSelectedDataPreparationAttributes);
		}
		// Nettoyage si pas de classifieur apprenable
		else if (not dataPreparationBase.IsPreparedDataComputed())
		{
			// Nettoyage
			dataPreparationClass->DeleteDataPreparation();

			// Emission d'un warning
			AddWarning(sTmp + "Unable to train the " + KWType::GetPredictorLabel(GetTargetAttributeType()));
			bOk = false;
		}
		// On continue l'apprentissage effectif si au moins une classe cible et au moins un attribut a traiter
		else
		{
			///////////////////////////////////////////////////////////////////////
			// Initialisation des structures d'evaluation

			// Initialisation du gestionnaire de poids des attributs si pas d'interruption
			assert(weightManager == NULL);
			if (not StopTraining(&dataPreparationBase))
			{
				if (bTrace)
					cout << "SNB: CreateSNBWeightManager" << endl;
				weightManager = new KWPredictorSNBWeightManager;
				weightManager->SetDataPreparationBase(&dataPreparationBase);
				if (GetSelectionParameters()->GetSelectionCriterion() == "CMA")
					weightManager->SetWeightingMethod(
					    KWPredictorSNBWeightManager::PredictorCompressionRate);
				else if (GetSelectionParameters()->GetSelectionCriterion() == "MA")
					weightManager->SetWeightingMethod(KWPredictorSNBWeightManager::PredictorProb);
				else
					weightManager->SetWeightingMethod(KWPredictorSNBWeightManager::None);
				weightManager->Reset();
				weightManager->SetTraceLevel(GetSelectionParameters()->GetTraceLevel());
				weightManager->SetTraceSelectedAttributes(
				    GetSelectionParameters()->GetTraceSelectedAttributes());
				assert(weightManager->Check());
			}

			//////////////////////////////////////////////////////////////////////////////
			// Apprentissage

			// Apprentissage si pas d'interruption
			dCurrentPredictorCost = 0;
			if (not StopTraining(&dataPreparationBase))
			{
				// Libelle de tache: apprentissage
				if (bTrace)
					cout << "SNB: Train" << endl;
				TaskProgression::DisplayLabel("Train");

				// Initialisation du gestionnaire de scores d'evaluation (aucun attribut selectionne)
				predictorSelectionScoreManager->InitializeWorkingData();

				// Calcul du cout du predicteur initial
				dCurrentPredictorCost = predictorSelectionScoreManager->ComputeSelectionTotalCost();
				assert(CheckSelectionScore(dCurrentPredictorCost));

				// On en deduit le epsilon permettant de se comparer de facon relative a ce cout par
				// defaut La comparaison en "absolu" pose des problemes numeriques
				dEpsilon = (1 + fabs(dCurrentPredictorCost)) * 1e-2 / (1 + GetInstanceNumber());

				// Enregistrement de l'evaluation initiale
				RecordEvaluatedAttribute(KWPredictorEvaluatedAttribute::Initial, NULL,
							 dCurrentPredictorCost);

				// Optimisation de la selection des attributs: on obtient la selection MAP
				if (bTrace)
					cout << "SNB: OptimizeAttributeSelection" << endl;
				nkdSelectedAttributes = OptimizeAttributeSelection(dCurrentPredictorCost);
			}

			//////////////////////////////////////////////////////////////////////////////
			// Enregistrement des resultats

			// Enregistrement si pas d'interruption
			if (not StopTraining(&dataPreparationBase))
			{
				// Libelle de tache: enregistrement du modele
				TaskProgression::DisplayLabel("Register model");

				// Enregistrement de l'evaluation finale
				RecordEvaluatedAttribute(KWPredictorEvaluatedAttribute::Final, NULL,
							 dCurrentPredictorCost);

				// Calcul des poids des attributs
				if (bTrace)
					cout << "SNB: ComputeAttributeWeigths" << endl;
				check(weightManager);
				weightManager->ComputeAttributeWeigths();

				// Memorisation des attributs selectionnes dans le rapport de selection en sortie
				if (bTrace)
					cout << "SNB: CollectSelectedAttributes" << endl;
				CollectSelectedAttributes(weightManager->GetAttributeWeights(), nkdSelectedAttributes);

				// Filtrage des attributs selectionnes
				if (bTrace)
					cout << "SNB: FilterSelectedAttributes" << endl;
				FilterSelectedAttributes(weightManager->GetAttributeWeights(), nkdSelectedAttributes);

				// Construction d'un predicteur bayesien naif a partir de tous les attributs
				if (bTrace)
					cout << "SNB: InternalTrainWNB" << endl;
				if (weightManager->GetAttributeWeights() != NULL)
					InternalTrainWNB(dataPreparationClass,
							 dataPreparationClass->GetDataPreparationAttributes(),
							 weightManager->GetAttributeWeights());
				// S'il n'y a pas de poids (correspondant a un moyennage de modeles), on utilise les
				// attributs de la meilleure selection (MAP)
				else
				{
					nkdSelectedAttributes->ExportObjectArray(&oaSelectedDataPreparationAttributes);
					InternalTrainWNB(dataPreparationClass, &oaSelectedDataPreparationAttributes,
							 NULL);
				}

				// Ajout de meta-donnees (Level, Weight, MAP) aux attributs du classifieur appris
				FillPredictorAttributeMetaData(GetTrainedPredictor()->GetPredictorClass());
			}

			// Message sur le temps de calcul
			timerTrain.Stop();
			if (not StopTraining(&dataPreparationBase))
				AddSimpleMessage(sTmp +
						 "SNB train time: " + SecondsToString(timerTrain.GetElapsedTime()));

			else
				AddSimpleMessage(sTmp + "Train of Selective Naive Bayes predictor interrupted after " +
						 SecondsToString(timerTrain.GetElapsedTime()));
		}

		// Test si apprentissage interrompu
		bOk = bOk and not StopTraining(&dataPreparationBase);
		if (bTrace)
			cout << "SNB: End" << endl;
	}

	// Nettoyage des donnees de travail locales a la methode
	if (nkdSelectedAttributes != NULL)
		delete nkdSelectedAttributes;

	// Nettoyage des donnees de travail globales a la classe
	if (weightManager != NULL)
		delete weightManager;
	if (predictorSelectionScoreManager != NULL)
		delete predictorSelectionScoreManager;
	predictorSelectionScoreManager = NULL;
	weightManager = NULL;

	return bOk;
}

void KWPredictorSelectiveNaiveBayes::ComputeUsableAttributes(KWDataPreparationBase* dataPreparationBase)
{
	ObjectArray oaDataPreparationUsableAttributes;
	KWDataPreparationAttribute* dataPreparationAttribute;
	int nAttribute;

	require(dataPreparationBase != NULL);
	require(dataPreparationBase->Check());
	require(dataPreparationBase->GetDataPreparationUsedAttributes()->GetSize() == 0);

	// Specification des attributs a utiliser
	for (nAttribute = 0;
	     nAttribute < dataPreparationBase->GetDataPreparationClass()->GetDataPreparationAttributes()->GetSize();
	     nAttribute++)
	{
		dataPreparationAttribute = cast(
		    KWDataPreparationAttribute*,
		    dataPreparationBase->GetDataPreparationClass()->GetDataPreparationAttributes()->GetAt(nAttribute));

		// Ajout si attribut utile (table de contingence a plus de une ligne et SortValue (Level, DeltaLevel...)
		// strictement positivf)
		if (dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats() != NULL and
		    dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats()->ComputeSourceGridSize() >
			1 and
		    dataPreparationAttribute->GetPreparedStats()->GetSortValue() > 0)
		{
			oaDataPreparationUsableAttributes.Add(dataPreparationAttribute);
		}
	}

	// Tri des attributs
	oaDataPreparationUsableAttributes.SetCompareFunction(KWDataPreparationAttributeCompareSortValue);
	oaDataPreparationUsableAttributes.Sort();

	// Limitation au nombre max d'attributs
	if (GetTrainParameters()->GetMaxEvaluatedAttributeNumber() > 0 and
	    GetTrainParameters()->GetMaxEvaluatedAttributeNumber() < oaDataPreparationUsableAttributes.GetSize())
		oaDataPreparationUsableAttributes.SetSize(GetTrainParameters()->GetMaxEvaluatedAttributeNumber());

	// Memorisation des attributs a utiliser
	dataPreparationBase->SetDataPreparationUsedAttributes(&oaDataPreparationUsableAttributes);
}

void KWPredictorSelectiveNaiveBayes::FillPredictorAttributeMetaData(KWClass* kwcClass)
{
	const ALString sWeightMetaDataKey = "Weight";
	const ALString sImportanceMetaDataKey = "Importance";
	ObjectDictionary oaAttributes;
	KWAttribute* attribute;
	int nAttribute;
	KWSelectedAttributeReport* selectedAttribute;

	require(kwcClass != NULL);

	// Nettoyage initial des meta-donnees
	kwcClass->RemoveAllAttributesMetaDataKey(sWeightMetaDataKey);
	kwcClass->RemoveAllAttributesMetaDataKey(sImportanceMetaDataKey);

	// On range les attribut du classifieurs dans un dictionnaire, pour avoir un acces rapide
	attribute = kwcClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		oaAttributes.SetAt(attribute->GetName(), attribute);
		kwcClass->GetNextAttribute(attribute);
	}

	// Parcours des attributs selectionnes pour rechercher les informations d'apprentissage
	for (nAttribute = 0; nAttribute < GetPredictorSelectionReport()->GetSelectedAttributes()->GetSize();
	     nAttribute++)
	{
		selectedAttribute = cast(KWSelectedAttributeReport*,
					 GetPredictorSelectionReport()->GetSelectedAttributes()->GetAt(nAttribute));

		// Mise a jour de la version native de l'attribut
		attribute = kwcClass->LookupAttribute(selectedAttribute->GetNativeAttributeName());
		if (attribute != NULL)
		{
			attribute->GetMetaData()->SetDoubleValueAt(sWeightMetaDataKey, selectedAttribute->GetWeight());
			attribute->GetMetaData()->SetDoubleValueAt(sImportanceMetaDataKey,
								   selectedAttribute->GetImportance());
		}
	}
}

boolean KWPredictorSelectiveNaiveBayes::InitializeAllWeightedAttributes()
{
	int nAttribute;
	KWDataPreparationBase* dataPreparationBase;
	KWDataPreparationAttribute* dataPreparationAttribute;
	Continuous cWeight;

	require(CheckTrainWorkingData());
	require(weightManager->GetAttributeWeights() != NULL);
	require(weightManager->GetAttributeWeights()->GetSize() ==
		predictorSelectionScoreManager->GetDataPreparationBase()
		    ->GetDataPreparationClass()
		    ->GetDataPreparationAttributes()
		    ->GetSize());

	// Initialisation d'une evaluation avec les probabilites conditionnelles a priori des classes cibles
	predictorSelectionScoreManager->InitializeWorkingData();

	// Suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Computation of variable weights");

	// Parcours des attributs de la base de preparation
	// On exploite les attribut dans l'ordre de la dataPreparationBase, pour avoir un acces
	// compatible avec l'ordre des chunks (ce qui est vital lorsque les chunks sont sur disque)
	dataPreparationBase = predictorSelectionScoreManager->GetDataPreparationBase();
	for (nAttribute = 0; nAttribute < dataPreparationBase->GetDataPreparationUsedAttributes()->GetSize();
	     nAttribute++)
	{
		dataPreparationAttribute =
		    cast(KWDataPreparationAttribute*,
			 dataPreparationBase->GetDataPreparationUsedAttributes()->GetAt(nAttribute));

		// Taux d'avancement
		TaskProgression::DisplayProgression(
		    (int)(100 * (nAttribute * 1.0) /
			  dataPreparationBase->GetDataPreparationUsedAttributes()->GetSize()));
		if (StopTraining(dataPreparationBase))
			break;

		// Prise en compte de l'attribut si son poids est non nul
		cWeight = weightManager->GetAttributeWeights()->GetAt(dataPreparationAttribute->GetIndex());
		if (cWeight > 0)
			predictorSelectionScoreManager->AddWeightedAttribute(dataPreparationAttribute, cWeight);
	}

	// Fin de tache
	TaskProgression::EndTask();

	return true;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWPredictorSNBWeightManager

KWPredictorSNBWeightManager::KWPredictorSNBWeightManager()
{
	dataPreparationBase = NULL;
	cvAttributeWeights = NULL;
	nWeightingMethod = None;
	nTraceLevel = 0;
	bTraceSelectedAttributes = false;
}

KWPredictorSNBWeightManager::~KWPredictorSNBWeightManager()
{
	olEvaluatedAttributes.DeleteAll();
	if (cvAttributeWeights != NULL)
		delete cvAttributeWeights;
}

void KWPredictorSNBWeightManager::SetDataPreparationBase(KWDataPreparationBase* kwdpbBase)
{
	dataPreparationBase = kwdpbBase;
}

KWDataPreparationBase* KWPredictorSNBWeightManager::GetDataPreparationBase()
{
	return dataPreparationBase;
}

ContinuousVector* KWPredictorSNBWeightManager::GetAttributeWeights()
{
	return cvAttributeWeights;
}

void KWPredictorSNBWeightManager::SetWeightingMethod(int nValue)
{
	require(nValue == None or nValue == PredictorCompressionRate or nValue == PredictorProb);

	nWeightingMethod = nValue;

	// Creation/destruction du vecteur de poids selon la methode
	if (nWeightingMethod == None)
	{
		if (cvAttributeWeights != NULL)
			delete cvAttributeWeights;
		cvAttributeWeights = NULL;
	}
	else
	{
		if (cvAttributeWeights == NULL)
			cvAttributeWeights = new ContinuousVector;
	}
}

int KWPredictorSNBWeightManager::GetWeightingMethod() const
{
	return nWeightingMethod;
}

const ALString KWPredictorSNBWeightManager::GetWeightingMethodLabel() const
{
	if (nWeightingMethod == PredictorCompressionRate)
		return "PredictorCompressionRate";
	else if (nWeightingMethod == PredictorProb)
		return "PredictorProb";
	else
		return "None";
}

void KWPredictorSNBWeightManager::SetTraceLevel(int nValue)
{
	require(0 <= nValue and nValue <= 3);
	nTraceLevel = nValue;
}

int KWPredictorSNBWeightManager::GetTraceLevel() const
{
	return nTraceLevel;
}

void KWPredictorSNBWeightManager::SetTraceSelectedAttributes(boolean bValue)
{
	bTraceSelectedAttributes = bValue;
}

boolean KWPredictorSNBWeightManager::GetTraceSelectedAttributes() const
{
	return bTraceSelectedAttributes;
}

void KWPredictorSNBWeightManager::Reset()
{
	require(dataPreparationBase != NULL);

	// Supression des evaluations enregistres
	olEvaluatedAttributes.DeleteAll();

	// Retaillage eventuel du vecteur de poids
	if (GetWeightingMethod() != None)
	{
		check(cvAttributeWeights);
		cvAttributeWeights->SetSize(
		    dataPreparationBase->GetDataPreparationClass()->GetDataPreparationAttributes()->GetSize());
		cvAttributeWeights->Initialize();
	}

	ensure(Check());
}

void KWPredictorSNBWeightManager::UpdateWeightEvaluation(int nEvaluationType,
							 NumericKeyDictionary* nkdSelectedAttributes,
							 KWDataPreparationAttribute* evaluatedDataPreparationAttribute,
							 double dNewPredictorModelCost, double dNewPredictorDataCost)
{
	int nAttribute;
	int nSelectedAttributeNumber;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWPredictorEvaluatedAttribute* evaluatedAttribute;
	boolean bTrace;

	require(Check());
	require(KWPredictorEvaluatedAttribute::Initial <= nEvaluationType and
		nEvaluationType <= KWPredictorEvaluatedAttribute::Final);
	require(nkdSelectedAttributes != NULL);
	require(evaluatedDataPreparationAttribute != NULL or
		nEvaluationType == KWPredictorEvaluatedAttribute::Initial or
		nEvaluationType == KWPredictorEvaluatedAttribute::LocalOptimum or
		nEvaluationType == KWPredictorEvaluatedAttribute::GlobalOptimum or
		nEvaluationType == KWPredictorEvaluatedAttribute::ForcedRemoveAll or
		nEvaluationType == KWPredictorEvaluatedAttribute::ForcedEvaluation or
		nEvaluationType == KWPredictorEvaluatedAttribute::Final);
	require(dNewPredictorModelCost >= 0);
	require(dNewPredictorDataCost >= 0);

	// Creation/initialisation d'une evaluation d'attribut
	evaluatedAttribute = new KWPredictorEvaluatedAttribute;
	evaluatedAttribute->SetDataPreparationAttribute(evaluatedDataPreparationAttribute);
	evaluatedAttribute->SetType(nEvaluationType);
	evaluatedAttribute->SetModelCost(dNewPredictorModelCost);
	evaluatedAttribute->SetDataCost(dNewPredictorDataCost);

	// Trace
	if (GetTraceLevel() > 0)
	{
		// Ligne de titre pour la premiere trace
		if (nEvaluationType == KWPredictorEvaluatedAttribute::Initial)
		{
			// Affichage des nom des attributs
			if (bTraceSelectedAttributes)
			{
				for (nAttribute = 0;
				     nAttribute < dataPreparationBase->GetDataPreparationUsedAttributes()->GetSize();
				     nAttribute++)
				{
					dataPreparationAttribute = cast(
					    KWDataPreparationAttribute*,
					    dataPreparationBase->GetDataPreparationUsedAttributes()->GetAt(nAttribute));
					cout << "Attribute " << nAttribute + 1 << "\t";
				}
			}

			// Fin de l'entete
			cout << "Nb Var.\t";
			evaluatedAttribute->WriteLabel(cout);
		}

		// En trace niveau 1, seul les optimaux locaux et globaux sont affiches
		if (GetTraceLevel() == 1)
		{
			bTrace = nEvaluationType == KWPredictorEvaluatedAttribute::Initial or
				 nEvaluationType == KWPredictorEvaluatedAttribute::LocalOptimum or
				 nEvaluationType == KWPredictorEvaluatedAttribute::GlobalOptimum or
				 nEvaluationType == KWPredictorEvaluatedAttribute::Final;
		}
		// En trace niveau 2, seuls les changements importants sont affiches
		else if (GetTraceLevel() == 2)
			bTrace = evaluatedAttribute->IsAcceptationType();
		// En trace niveau 3, tout est affiche
		else
			bTrace = true;

		// Affichage
		if (bTrace)
		{
			// Affichage de la presence des attributs
			if (bTraceSelectedAttributes)
			{
				nSelectedAttributeNumber = 0;
				for (nAttribute = 0;
				     nAttribute < dataPreparationBase->GetDataPreparationUsedAttributes()->GetSize();
				     nAttribute++)
				{
					dataPreparationAttribute = cast(
					    KWDataPreparationAttribute*,
					    dataPreparationBase->GetDataPreparationUsedAttributes()->GetAt(nAttribute));
					if (nkdSelectedAttributes->Lookup(dataPreparationAttribute) != NULL)
					{
						cout << dataPreparationAttribute->GetPreparedAttribute()->GetName()
						     << "\t";
						nSelectedAttributeNumber++;
					}
				}
				for (nAttribute = nSelectedAttributeNumber;
				     nAttribute < dataPreparationBase->GetDataPreparationUsedAttributes()->GetSize();
				     nAttribute++)
					cout << "\t";
			}

			// Suite de la trace
			cout << nkdSelectedAttributes->GetCount() << "\t";
			evaluatedAttribute->Write(cout);
			cout << flush;
		}
	}

	// Memorisation de l'evaluation si le poids est gere
	if (nWeightingMethod != None)
		olEvaluatedAttributes.AddTail(evaluatedAttribute);
	// Sinon, destruction
	else
		delete evaluatedAttribute;
}

void KWPredictorSNBWeightManager::ComputeAttributeWeigths()
{
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWPredictorEvaluatedAttribute* evaluatedAttribute;
	NumericKeyDictionary nkdSelectedAttributes;
	double dInitialEvaluation;
	double dFinalEvaluation;
	Continuous cWeightThreshold;
	Continuous cWeight;
	Continuous cTotalWeight;
	double dCompressionRate;
	POSITION headPosition;
	POSITION secondPosition;
	POSITION currentPosition;

	require(Check());
	require(olEvaluatedAttributes.GetCount() >= 2);

	// Calcul uniquement si le poids est gere
	if (nWeightingMethod != None)
	{
		// Recherche de l'evaluation initiale
		headPosition = olEvaluatedAttributes.GetHeadPosition();
		evaluatedAttribute = cast(KWPredictorEvaluatedAttribute*, olEvaluatedAttributes.GetAt(headPosition));
		assert(evaluatedAttribute->GetType() == KWPredictorEvaluatedAttribute::Initial);
		dInitialEvaluation = evaluatedAttribute->GetTotalCost();

		// Modification de l'evaluation initiale si presence d'attributs obligatoires
		// Dans ce cas, on prend comme evaluation initiale la selection comportant tous
		// les attributs obligatoires
		secondPosition = headPosition;
		olEvaluatedAttributes.GetNext(secondPosition);
		if (cast(KWPredictorEvaluatedAttribute*, olEvaluatedAttributes.GetAt(secondPosition))->GetType() ==
		    KWPredictorEvaluatedAttribute::MandatoryAdd)
		{
			currentPosition = secondPosition;
			while (currentPosition != NULL)
			{
				evaluatedAttribute = cast(KWPredictorEvaluatedAttribute*,
							  olEvaluatedAttributes.GetNext(currentPosition));
				if (evaluatedAttribute->GetType() == KWPredictorEvaluatedAttribute::MandatoryAdd)
					dInitialEvaluation = evaluatedAttribute->GetTotalCost();
				else
					break;
			}
		}

		// Recherche de l'evaluation finale
		evaluatedAttribute = cast(KWPredictorEvaluatedAttribute*, olEvaluatedAttributes.GetTail());
		assert(evaluatedAttribute->GetType() == KWPredictorEvaluatedAttribute::Final);
		dFinalEvaluation = evaluatedAttribute->GetTotalCost();

		// Calcul du seuil de prise en compte des poids des classifieurs et des poids terminaux
		cWeightThreshold = (Continuous)(1.0 / (dataPreparationBase->GetInstanceNumber() + 1.0));

		// Parcours des evaluations d'attribut pour le calcul des poids
		cTotalWeight = 0;
		currentPosition = headPosition;
		while (currentPosition != NULL)
		{
			evaluatedAttribute =
			    cast(KWPredictorEvaluatedAttribute*, olEvaluatedAttributes.GetNext(currentPosition));
			dataPreparationAttribute = evaluatedAttribute->GetDataPreparationAttribute();

			// Calcul du poids du modele selon la methode
			if (nWeightingMethod == PredictorCompressionRate)
			{
				dCompressionRate =
				    (dInitialEvaluation - evaluatedAttribute->GetTotalCost()) / dInitialEvaluation;
				cWeight = (Continuous)dCompressionRate;

				// On ignore les modeles dont le cout est superieur au cout du modele par defaut
				if (cWeight < 0)
					cWeight = 0;
			}
			else
				cWeight = (Continuous)exp(dFinalEvaluation - evaluatedAttribute->GetTotalCost());

			// Evaluation par ajout d'attribut
			if (evaluatedAttribute->GetType() == KWPredictorEvaluatedAttribute::Add)
			{
				cTotalWeight += cWeight;

				// On incremente le poids de tous les attributs de la selection en cours
				UpdateAttributeWeigths(&nkdSelectedAttributes, cWeight);

				// Plus celui de l'attribut ajoute
				cvAttributeWeights->UpgradeAt(dataPreparationAttribute->GetIndex(), cWeight);
			}
			// Evaluation par supression d'attribut
			else if (evaluatedAttribute->GetType() == KWPredictorEvaluatedAttribute::Remove)
			{
				cTotalWeight += cWeight;

				// On incremente le poids de tous les attributs de la selection en cours
				UpdateAttributeWeigths(&nkdSelectedAttributes, cWeight);

				// Sauf celui de l'attribut supprime
				cvAttributeWeights->UpgradeAt(dataPreparationAttribute->GetIndex(), -cWeight);
			}
			// Evaluation forcee
			else if (evaluatedAttribute->GetType() == KWPredictorEvaluatedAttribute::ForcedEvaluation)
			{
				cTotalWeight += cWeight;

				// On incremente le poids de tous les attributs de la selection en cours
				UpdateAttributeWeigths(&nkdSelectedAttributes, cWeight);
			}

			// Ajout d'un attribut dans la selection
			if (evaluatedAttribute->GetType() == KWPredictorEvaluatedAttribute::MandatoryAdd or
			    evaluatedAttribute->GetType() == KWPredictorEvaluatedAttribute::UnevaluatedAdd or
			    evaluatedAttribute->GetType() == KWPredictorEvaluatedAttribute::BestAdd)
				nkdSelectedAttributes.SetAt((NUMERIC)dataPreparationAttribute,
							    dataPreparationAttribute);
			// Suppression d'un attribut de la selection
			else if (evaluatedAttribute->GetType() == KWPredictorEvaluatedAttribute::UnevaluatedRemove or
				 evaluatedAttribute->GetType() == KWPredictorEvaluatedAttribute::BestRemove)
				nkdSelectedAttributes.RemoveKey((NUMERIC)dataPreparationAttribute);
			// Suppression de tous les attributs de la selection
			else if (evaluatedAttribute->GetType() == KWPredictorEvaluatedAttribute::ForcedRemoveAll)
				nkdSelectedAttributes.RemoveAll();
		}

		// Normalisation des poids
		if (cTotalWeight > 0)
		{
			for (nAttribute = 0; nAttribute < cvAttributeWeights->GetSize(); nAttribute++)
			{
				cWeight = cvAttributeWeights->GetAt(nAttribute) / cTotalWeight;
				if (cWeight < cWeightThreshold)
					cvAttributeWeights->SetAt(nAttribute, 0);
				else
					cvAttributeWeights->SetAt(nAttribute, cWeight);
			}
		}
	}
}

boolean KWPredictorSNBWeightManager::Check() const
{
	boolean bOk = true;
	if (dataPreparationBase == NULL)
		bOk = false;
	if (bOk and nWeightingMethod != None)
		bOk = cvAttributeWeights != NULL and
		      cvAttributeWeights->GetSize() ==
			  dataPreparationBase->GetDataPreparationClass()->GetDataPreparationAttributes()->GetSize();
	return bOk;
}

void KWPredictorSNBWeightManager::WriteAttributeWeights(ostream& ost) const
{
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;

	require(Check());

	ost << "Variable\tWeight\n";
	for (nAttribute = 0;
	     nAttribute < dataPreparationBase->GetDataPreparationClass()->GetDataPreparationAttributes()->GetSize();
	     nAttribute++)
	{
		dataPreparationAttribute = cast(
		    KWDataPreparationAttribute*,
		    dataPreparationBase->GetDataPreparationClass()->GetDataPreparationAttributes()->GetAt(nAttribute));

		// Affichage du poids si la methode le prend en compte
		if (nWeightingMethod != None)
			ost << dataPreparationAttribute->GetPreparedAttribute()->GetName() << "\t"
			    << cvAttributeWeights->GetAt(nAttribute) << "\n";
		else
			ost << "\t0\n";
	}
	ost << flush;
}

void KWPredictorSNBWeightManager::UpdateAttributeWeigths(NumericKeyDictionary* nkdSelectedAttributes,
							 Continuous cDeltaWeight)
{
	POSITION position;
	Object* oElement;
	NUMERIC key;
	KWDataPreparationAttribute* dataPreparationAttribute;

	require(Check());
	require(nkdSelectedAttributes != NULL);

	// Parcours des attributs selectionnes pour mettre a jour leur poids
	position = nkdSelectedAttributes->GetStartPosition();
	while (position != NULL)
	{
		// Acces a un attribut
		nkdSelectedAttributes->GetNextAssoc(position, key, oElement);
		dataPreparationAttribute = cast(KWDataPreparationAttribute*, oElement);

		// Modification de son poids
		cvAttributeWeights->UpgradeAt(dataPreparationAttribute->GetIndex(), cDeltaWeight);
	}
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWPredictorEvaluatedAttribute

KWPredictorEvaluatedAttribute::KWPredictorEvaluatedAttribute()
{
	dataPreparationAttribute = NULL;
	nType = Initial;
	dModelCost = 0;
	dDataCost = 0;
}

KWPredictorEvaluatedAttribute::~KWPredictorEvaluatedAttribute() {}

void KWPredictorEvaluatedAttribute::SetDataPreparationAttribute(KWDataPreparationAttribute* kwdpaAttribute)
{
	dataPreparationAttribute = kwdpaAttribute;
}

KWDataPreparationAttribute* KWPredictorEvaluatedAttribute::GetDataPreparationAttribute()
{
	return dataPreparationAttribute;
}

void KWPredictorEvaluatedAttribute::SetType(int nValue)
{
	require(Initial <= nValue and nValue <= Final);
	nType = nValue;
}

int KWPredictorEvaluatedAttribute::GetType()
{
	return nType;
}

ALString KWPredictorEvaluatedAttribute::GetTypeLabel() const
{
	if (nType == Initial)
		return "Initial";
	else if (nType == Add)
		return "Add";
	else if (nType == MandatoryAdd)
		return "MandatoryAdd";
	else if (nType == BestAdd)
		return "BestAdd";
	else if (nType == Remove)
		return "Remove";
	else if (nType == BestRemove)
		return "BestRemove";
	else if (nType == LocalOptimum)
		return "LocalOptimum";
	else if (nType == GlobalOptimum)
		return "GlobalOptimum";
	else if (nType == ForcedRemoveAll)
		return "ForcedRemoveAll";
	else if (nType == UnevaluatedAdd)
		return "UnevaluatedAdd";
	else if (nType == UnevaluatedRemove)
		return "UnevaluatedRemove";
	else if (nType == ForcedEvaluation)
		return "ForcedEvaluation";
	else if (nType == Final)
		return "Final";
	else
		return "";
}

boolean KWPredictorEvaluatedAttribute::IsAcceptationType() const
{
	return nType == Initial or nType == MandatoryAdd or nType == BestAdd or nType == BestRemove or
	       nType == ForcedEvaluation or nType == LocalOptimum or nType == GlobalOptimum or nType == Final;
}

void KWPredictorEvaluatedAttribute::SetModelCost(double dValue)
{
	dModelCost = dValue;
}

double KWPredictorEvaluatedAttribute::GetModelCost() const
{
	return dModelCost;
}

void KWPredictorEvaluatedAttribute::SetDataCost(double dValue)
{
	dDataCost = dValue;
}

double KWPredictorEvaluatedAttribute::GetDataCost() const
{
	return dDataCost;
}

double KWPredictorEvaluatedAttribute::GetTotalCost() const
{
	return dModelCost + dDataCost;
}

void KWPredictorEvaluatedAttribute::WriteLabel(ostream& ost) const
{
	ost << "Variable\tType\tModel\tData\tTotal\n";
}

void KWPredictorEvaluatedAttribute::Write(ostream& ost) const
{
	if (dataPreparationAttribute != NULL)
		ost << dataPreparationAttribute->GetPreparedAttribute()->GetName();
	ost << "\t";
	ost << GetTypeLabel() << "\t";
	ost << GetModelCost() << "\t";
	ost << GetDataCost() << "\t";
	ost << GetTotalCost() << "\n";
}
