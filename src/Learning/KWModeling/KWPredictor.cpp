// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictor.h"

KWPredictor::KWPredictor()
{
	classStats = NULL;
	predictorReport = NULL;
	trainedPredictor = NULL;
	bIsTraining = false;
}

KWPredictor::~KWPredictor()
{
	if (predictorReport != NULL)
	{
		delete predictorReport;
	}
	if (trainedPredictor != NULL)
	{
		delete trainedPredictor;
	}
}

const ALString KWPredictor::GetSuffix() const
{
	return "";
}

KWTrainParameters* KWPredictor::GetTrainParameters()
{
	return &trainParameters;
}

KWPredictor* KWPredictor::Clone() const
{
	KWPredictor* kwpClone;

	kwpClone = Create();
	kwpClone->CopyFrom(this);
	return kwpClone;
}

void KWPredictor::CopyFrom(const KWPredictor* kwpSource)
{
	require(kwpSource != NULL);
	SetLearningSpec(kwpSource->GetLearningSpec());
	trainParameters.CopyFrom(&kwpSource->trainParameters);
}

void KWPredictor::SetClassStats(KWClassStats* stats)
{
	require(stats == NULL or GetLearningSpec() == stats->GetLearningSpec());
	classStats = stats;
}

KWClassStats* KWPredictor::GetClassStats() const
{
	ensure(classStats == NULL or GetLearningSpec() == classStats->GetLearningSpec());
	return classStats;
}

void KWPredictor::Train()
{
	boolean bTrainOk;
	debug(boolean bCurrentSilentMode);

	// Trace memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " " + GetObjectLabel() + " Train Begin");

	// L'indicateur d'apprentissage passe a true
	bIsTraining = true;

	// Nettoyage initial des resultats d'apprentissage
	DeleteTrainedResults();

	// On apprend si les specification sont valides, sinon on envoie (via Check()) des messages d'erreur
	if (Check())
	{
		// Creation des resultats d'apprentissage
		CreatePredictorReport();
		CreateTrainedPredictor();

		// Apprentissage
		bTrainOk = InternalTrain();
		if (not bTrainOk)
			DeleteTrainedResults();
	}
	// A l'issue de l'apprentissage, soit rien n'a ete appris, soit le predicteur appris est valide
	// et sa classe n'appartient pas a un domaine de classe
	// On force l'affichage eventuel de messages d'erreur en mode debug
	debug(bCurrentSilentMode = Global::GetSilentMode());
	debug(Global::SetSilentMode(false));
	ensure((predictorReport == NULL and trainedPredictor == NULL) or
	       (IsTrained() and trainedPredictor->GetPredictorClass()->GetDomain() != NULL));
	debug(Global::SetSilentMode(bCurrentSilentMode));

	// Preparation de la classe de prediction pour le deploiement
	if (trainedPredictor != NULL)
		trainedPredictor->PrepareDeploymentClass(true, false);

	// L'indicateur d'apprentissage passe a false
	bIsTraining = false;

	// Trace memoire
	MemoryStatsManager::AddLog(GetClassLabel() + " " + GetObjectLabel() + " Train End");
}

boolean KWPredictor::IsTraining() const
{
	return bIsTraining;
}

boolean KWPredictor::IsTrained() const
{
	ensure((predictorReport == NULL and trainedPredictor == NULL) or
	       (predictorReport != NULL and trainedPredictor != NULL));
	ensure((predictorReport == NULL and trainedPredictor == NULL) or
	       trainedPredictor->GetTargetType() == GetTargetAttributeType());
	ensure((predictorReport == NULL and trainedPredictor == NULL) or trainedPredictor->Check());
	return trainedPredictor != NULL;
}

void KWPredictor::RemoveTrainedResults()
{
	// Dereferencement des resultats d'apprentissage
	nkdSelectedDataPreparationStats.RemoveAll();
	nkdRecursivelySelectedDataPreparationStats.RemoveAll();
	predictorReport = NULL;
	trainedPredictor = NULL;
	ensure(not IsTrained());
}

void KWPredictor::DeleteTrainedResults()
{
	// Destruction des resultats d'apprentissage
	nkdSelectedDataPreparationStats.RemoveAll();
	nkdRecursivelySelectedDataPreparationStats.RemoveAll();
	if (predictorReport != NULL)
		delete predictorReport;
	predictorReport = NULL;
	if (trainedPredictor != NULL)
		delete trainedPredictor;
	trainedPredictor = NULL;

	ensure(not IsTrained());
}

const NumericKeyDictionary* KWPredictor::GetSelectedDataPreparationStats() const
{
	require(IsTrained());
	return &nkdSelectedDataPreparationStats;
}

const NumericKeyDictionary* KWPredictor::GetRecursivelySelectedDataPreparationStats() const
{
	require(IsTrained());
	return &nkdRecursivelySelectedDataPreparationStats;
}

KWPredictorReport* KWPredictor::GetPredictorReport()
{
	require((IsTraining() and predictorReport != NULL) or IsTrained());
	return predictorReport;
}

KWTrainedPredictor* KWPredictor::GetTrainedPredictor()
{
	require((IsTraining() and trainedPredictor != NULL) or IsTrained());
	return trainedPredictor;
}

KWTrainedClassifier* KWPredictor::GetTrainedClassifier()
{
	require((IsTraining() and trainedPredictor != NULL) or IsTrained());
	require(GetTargetAttributeType() == KWType::Symbol);
	return cast(KWTrainedClassifier*, trainedPredictor);
}

KWTrainedRegressor* KWPredictor::GetTrainedRegressor()
{
	require((IsTraining() and trainedPredictor != NULL) or IsTrained());
	require(GetTargetAttributeType() == KWType::Continuous);
	return cast(KWTrainedRegressor*, trainedPredictor);
}

KWTrainedClusterer* KWPredictor::GetTrainedClusterer()
{
	require((IsTraining() and trainedPredictor != NULL) or IsTrained());
	require(GetTargetAttributeType() == KWType::None);
	return cast(KWTrainedClusterer*, trainedPredictor);
}

KWPredictorEvaluation* KWPredictor::Evaluate(KWDatabase* database)
{
	KWPredictorEvaluation* predictorEvaluation;

	require(IsTrained());
	require(database != NULL);

	// Creation des resultats d'evaluation selon le type de predicteur
	if (GetTargetAttributeType() == KWType::Symbol)
		predictorEvaluation = new KWClassifierEvaluation;
	else if (GetTargetAttributeType() == KWType::Continuous)
		predictorEvaluation = new KWRegressorEvaluation;
	else
	{
		assert(GetTargetAttributeType() == KWType::None);
		predictorEvaluation = new KWPredictorEvaluation;
	}

	// Evaluation
	predictorEvaluation->Evaluate(this, database);
	return predictorEvaluation;
}

boolean KWPredictor::Check() const
{
	boolean bOk = true;

	// Validation des specifications
	bOk = KWLearningService::Check();

	// Compatibilite entre le type de predicteur et le type d'attribut cible des spec
	// On passe par GetLearningSpec() pour eviter la recursion (GetTargetAttributeType() a Check() en require)
	if (bOk and not IsTargetTypeManaged(GetLearningSpec()->GetTargetAttributeType()))
	{
		AddError(GetName() + " predictor cannot be used as a " +
			 KWType::GetPredictorLabel(GetLearningSpec()->GetTargetAttributeType()) + " for database " +
			 GetDatabase()->GetDatabaseName());
		bOk = false;
	}

	// Verification de la compatibilite avec les statistiques sur les attributs
	assert(classStats == NULL or GetLearningSpec() == classStats->GetLearningSpec());

	// Test de validite du predicteur
	if (bOk)
		bOk = trainParameters.Check();

	return bOk;
}

const ALString KWPredictor::GetClassLabel() const
{
	ALString sClassLabel;

	if (GetLearningSpec() == NULL)
		sClassLabel = "Predictor";
	else
		sClassLabel = KWType::GetPredictorLabel(GetLearningSpec()->GetTargetAttributeType());
	return sClassLabel;
}

const ALString KWPredictor::GetObjectLabel() const
{
	return GetName();
}

const ALString KWPredictor::GetIdentifier() const
{
	ALString sIdentifier;

	if (GetPrefix() != "")
		sIdentifier = GetPrefix() + "_";
	sIdentifier += GetClass()->GetName();
	if (GetSuffix() != "")
		sIdentifier += "_" + GetSuffix();
	return sIdentifier;
}

void KWPredictor::CreatePredictorReport()
{
	require(bIsTraining);
	require(predictorReport == NULL);

	predictorReport = new KWPredictorReport;
	predictorReport->SetLearningSpec(GetLearningSpec());
	predictorReport->SetPredictorName(GetObjectLabel());
}

void KWPredictor::CreateTrainedPredictor()
{
	require(bIsTraining);
	require(trainedPredictor == NULL);

	// Creation du predicteur
	if (GetTargetAttributeType() == KWType::Symbol)
		trainedPredictor = new KWTrainedClassifier;
	else if (GetTargetAttributeType() == KWType::Continuous)
		trainedPredictor = new KWTrainedRegressor;
	else if (GetTargetAttributeType() == KWType::None)
		trainedPredictor = new KWTrainedClusterer;
	check(trainedPredictor);

	// Memorisation de son nom
	trainedPredictor->SetName(GetName());
}

boolean KWPredictor::InternalTrain()
{
	require(IsTraining());
	return false;
}

void KWPredictor::CollectSelectedPreparationStats(ObjectArray* oaUsedDataPreparationStats)
{
	const boolean bDisplay = false;
	KWDataPreparationStats* dataPreparationStats;
	KWAttributeStats* attributeStats;
	int i;
	int nAttribute;
	ALString sAttributeName;
	KWClass* kwcPreparedClass;
	KWAttribute* attribute;
	NumericKeyDictionary nkdAllUsedAttributes;
	ObjectArray oaAllUsedAttributes;

	require(nkdSelectedDataPreparationStats.GetCount() == 0);
	require(nkdRecursivelySelectedDataPreparationStats.GetCount() == 0);
	require(oaUsedDataPreparationStats != NULL);
	require(GetClassStats() != NULL);
	require(GetClass() != NULL);
	require(GetClass() == GetClassStats()->GetClass());

	// Acces au dictionnaire de la classe preparee
	kwcPreparedClass = GetClass();
	if (bDisplay)
		cout << GetName() << endl;

	// Parcours des preparations d'attributs utilisees
	for (i = 0; i < oaUsedDataPreparationStats->GetSize(); i++)
	{
		dataPreparationStats = cast(KWDataPreparationStats*, oaUsedDataPreparationStats->GetAt(i));
		check(dataPreparationStats);

		// Memorisation dans le dictionnaire des preparations de donnees selectionnes directement ou non
		nkdSelectedDataPreparationStats.SetAt(dataPreparationStats, dataPreparationStats);
		nkdRecursivelySelectedDataPreparationStats.SetAt(dataPreparationStats, dataPreparationStats);
		if (bDisplay)
			cout << "\t" << dataPreparationStats->GetSortName() << endl;

		// Analyse de la preparation pour en extraire les attributs
		for (nAttribute = 0; nAttribute < dataPreparationStats->GetAttributeNumber(); nAttribute++)
		{
			sAttributeName = dataPreparationStats->GetAttributeNameAt(nAttribute);

			// Recherche de l'attribut prepare correspondant
			attributeStats = GetClassStats()->LookupAttributeStats(sAttributeName);

			// Pris en compte si non nul, comme dans le cas d'une paire ou d'une grille
			if (attributeStats != NULL)
			{
				// Memorisation dans le dictionnaire des preparations d'attributs selectionnes
				// recursivement
				nkdRecursivelySelectedDataPreparationStats.SetAt(attributeStats, attributeStats);
				if (bDisplay)
				{
					if (nkdSelectedDataPreparationStats.Lookup(attributeStats) == NULL)
						cout << "\t\t" << attributeStats->GetSortName() << endl;
				}
			}

			// Analyse des attributs utilise recursivement dans une forumule de calcul, comme par exemple
			// pour les arbres
			attribute = kwcPreparedClass->LookupAttribute(sAttributeName);
			check(attribute);
			if (attribute->GetDerivationRule() != NULL)
				attribute->GetDerivationRule()->BuildAllUsedAttributes(attribute,
										       &nkdAllUsedAttributes);
		}
	}

	// Prise en compte des preparation de tous les attributs utilises recursivement
	nkdAllUsedAttributes.ExportObjectArray(&oaAllUsedAttributes);
	for (i = 0; i < oaAllUsedAttributes.GetSize(); i++)
	{
		attribute = cast(KWAttribute*, oaAllUsedAttributes.GetAt(i));

		// On ne prend comte que les attribut de la classe prepares, et non ceux des sous-tables
		if (attribute->GetParentClass() == kwcPreparedClass)
		{
			attributeStats = GetClassStats()->LookupAttributeStats(attribute->GetName());

			// Pris en compte si non nul
			if (attributeStats != NULL)
			{
				// Memorisation dans le dictionnaire des preparations d'attributs selectionnes
				// recursivement
				nkdRecursivelySelectedDataPreparationStats.SetAt(attributeStats, attributeStats);
				if (bDisplay)
				{
					if (nkdSelectedDataPreparationStats.Lookup(attributeStats) == NULL)
						cout << "\t\t" << attributeStats->GetSortName() << endl;
				}
			}
		}
	}
}

int KWPredictorCompareName(const void* first, const void* second)
{
	KWPredictor* aFirst;
	KWPredictor* aSecond;
	int nResult;

	aFirst = cast(KWPredictor*, *(Object**)first);
	aSecond = cast(KWPredictor*, *(Object**)second);
	nResult = aFirst->GetName().Compare(aSecond->GetName());
	return nResult;
}

///////////////////////////////////////////////////////////////////////////

void KWPredictor::RegisterPredictor(KWPredictor* predictor)
{
	require(predictor != NULL);
	require(predictor->GetName() != "");
	require(odPredictors == NULL or odPredictors->Lookup(predictor->GetName()) == NULL);

	// Creation si necessaire du dictionnaire de Predictors
	if (odPredictors == NULL)
		odPredictors = new ObjectDictionary;

	// Memorisation du Predictor
	odPredictors->SetAt(predictor->GetName(), predictor);
}

KWPredictor* KWPredictor::LookupPredictor(const ALString& sName, int nTargetType)
{
	KWPredictor* predictor;

	require(nTargetType == KWType::Symbol or nTargetType == KWType::Continuous or nTargetType == KWType::None);

	// Creation si necessaire du dictionnaire de Predictors
	if (odPredictors == NULL)
		odPredictors = new ObjectDictionary;

	// Recherche du predicteur du bon type
	predictor = cast(KWPredictor*, odPredictors->Lookup(sName));
	if (predictor != NULL and predictor->IsTargetTypeManaged(nTargetType))
		return predictor;
	else
		return NULL;
}

KWPredictor* KWPredictor::ClonePredictor(const ALString& sName, int nTargetType)
{
	KWPredictor* referencePredictor;

	require(nTargetType == KWType::Symbol or nTargetType == KWType::Continuous or nTargetType == KWType::None);

	// Creation si necessaire du dictionnaire de Predictors
	if (odPredictors == NULL)
		odPredictors = new ObjectDictionary;

	// Recherche d'un Predictor de meme nom
	referencePredictor = cast(KWPredictor*, odPredictors->Lookup(sName));

	// Retour de son Clone si possible
	if (referencePredictor != NULL and referencePredictor->IsTargetTypeManaged(nTargetType))
		return referencePredictor->Clone();
	else
		return NULL;
}

void KWPredictor::ExportAllPredictors(int nTargetType, ObjectArray* oaPredictors)
{
	ObjectArray oaAllPredictors;
	int i;
	KWPredictor* predictor;

	require(nTargetType == KWType::Symbol or nTargetType == KWType::Continuous or nTargetType == KWType::None);
	require(oaPredictors != NULL);

	// Creation si necessaire du dictionnaire de predicteurs
	if (odPredictors == NULL)
		odPredictors = new ObjectDictionary;

	// Recherche des predicteurs du bon type
	odPredictors->ExportObjectArray(&oaAllPredictors);
	oaPredictors->RemoveAll();
	for (i = 0; i < oaAllPredictors.GetSize(); i++)
	{
		predictor = cast(KWPredictor*, oaAllPredictors.GetAt(i));
		if (predictor->IsTargetTypeManaged(nTargetType))
			oaPredictors->Add(predictor);
	}

	// Tri des predicteurs avant de retourner le tableau
	oaPredictors->SetCompareFunction(KWPredictorCompareName);
	oaPredictors->Sort();
}

void KWPredictor::DeleteAllPredictors()
{
	if (odPredictors != NULL)
	{
		odPredictors->DeleteAll();
		delete odPredictors;
		odPredictors = NULL;
	}
	ensure(odPredictors == NULL);
}

ObjectDictionary* KWPredictor::odPredictors = NULL;
