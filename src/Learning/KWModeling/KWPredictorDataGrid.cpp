// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorDataGrid.h"

///////////////////////////////////////////////////////////////////////////
// Classe KWPredictorDataGrid

KWPredictorDataGrid::KWPredictorDataGrid() {}

KWPredictorDataGrid::~KWPredictorDataGrid() {}

boolean KWPredictorDataGrid::IsTargetTypeManaged(int nType) const
{
	return nType == KWType::Symbol or nType == KWType::None;
}

KWPredictor* KWPredictorDataGrid::Create() const
{
	return new KWPredictorDataGrid;
}

void KWPredictorDataGrid::CopyFrom(const KWPredictor* kwpSource)
{
	KWPredictorDataGrid* kwpdgSource;

	require(kwpSource != NULL);

	// Appel de la methode ancetre
	KWPredictor::CopyFrom(kwpSource);

	// Recopie des parametres d'apprentissage des modeles en grille
	kwpdgSource = cast(KWPredictorDataGrid*, kwpSource);
	dataGridOptimizerParameters.CopyFrom(&kwpdgSource->dataGridOptimizerParameters);
}

const ALString KWPredictorDataGrid::GetName() const
{
	return "Data Grid";
}

const ALString KWPredictorDataGrid::GetPrefix() const
{
	return "DG";
}

KWDataGridOptimizerParameters* KWPredictorDataGrid::GetDataGridOptimizerParameters()
{
	return &dataGridOptimizerParameters;
}

KWPredictorDataGridReport* KWPredictorDataGrid::GetPredictorDataGridReport()
{
	return cast(KWPredictorDataGridReport*, GetPredictorReport());
}

void KWPredictorDataGrid::CreatePredictorReport()
{
	require(bIsTraining);
	require(predictorReport == NULL);

	predictorReport = new KWPredictorDataGridReport;
	predictorReport->SetLearningSpec(GetLearningSpec());
	predictorReport->SetPredictorName(GetName());
}

int KWPredictorDataGrid::FillPredictorDataGridReport(ObjectArray* oaDataPreparationAttributes,
						     ContinuousVector* cvWeights)
{
	KWPredictorDataGridReport* dataGridReport;
	int i;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWSelectedDataGridReport* selectedDataGridReport;
	int nAttribute;
	ObjectDictionary odUsedAttributeNames;

	require(oaDataPreparationAttributes != NULL);
	require(cvWeights == NULL or cvWeights->GetSize() == oaDataPreparationAttributes->GetSize());

	// Initialisation du rapport
	dataGridReport = GetPredictorDataGridReport();
	dataGridReport->SetClassStats(GetClassStats());
	dataGridReport->GetSelectedDataGridReports()->DeleteAll();

	// Memorisation des grilles utilisees
	for (i = 0; i < oaDataPreparationAttributes->GetSize(); i++)
	{
		dataPreparationAttribute = cast(KWDataPreparationAttribute*, oaDataPreparationAttributes->GetAt(i));

		// Creation d'un rapport par grille
		selectedDataGridReport = new KWSelectedDataGridReport;
		dataGridReport->GetSelectedDataGridReports()->Add(selectedDataGridReport);
		selectedDataGridReport->SetPreparedDataGridStats(
		    dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats()->Clone());
		selectedDataGridReport->SetUnivariateEvaluation(
		    dataPreparationAttribute->GetPreparedStats()->GetSortValue());
		if (cvWeights != NULL)
			selectedDataGridReport->SetWeight(cvWeights->GetAt(i));

		// Memorisation des attributs utilises dans un dictionnaire
		for (nAttribute = 0;
		     nAttribute < dataPreparationAttribute->GetPreparedStats()->GetPreparedAttributeNumber();
		     nAttribute++)
		{
			odUsedAttributeNames.SetAt(
			    dataPreparationAttribute->GetPreparedStats()->GetAttributeNameAt(nAttribute), this);
		}
	}

	// On retourne le nombre total d'attribut utilises
	dataGridReport->SetUsedAttributeNumber(odUsedAttributeNames.GetCount());
	return dataGridReport->GetUsedAttributeNumber();
}

boolean KWPredictorDataGrid::InternalTrain()
{
	KWTupleTableLoader tupleTableLoader;
	KWTupleTable multivariateTupleTable;
	StringVector svAttributeNames;
	KWDataPreparationClass dataPreparationClass;
	KWAttributeSubsetStats* attributeSubsetStats;
	ObjectArray* oaTrainAttributeStats;
	KWDataPreparationStats* dataPreparationStats;
	int nAttribute;
	Continuous cWeight;
	Continuous cTotalWeight;
	KWDataPreparationAttribute* dataPreparationAttribute;
	ContinuousVector cvWeights;
	KWDataGridOptimizerParameters refDataGridOptimizerParameters;
	int nUsedAttributeNumber;
	boolean bOk;

	require(Check());
	require(GetClassStats() != NULL);

	// Calcul des statistiques si necessaire
	if (not GetClassStats()->IsStatsComputed())
		GetClassStats()->ComputeStats();
	assert(GetTargetAttributeName() == "" or not GetClassStats()->IsStatsComputed() or
	       GetTargetDescriptiveStats() != NULL);

	// Apprentissage en non supervise, si au moins une valeur cible en supervise
	if (GetTargetAttributeName() == "" or
	    (GetLearningSpec()->IsTargetStatsComputed() and GetTargetDescriptiveStats()->GetValueNumber() > 0))
	{
		/////////////////////////////////////////////////////////////
		// Apprentissage

		// Creation et initialisation d'un objet de stats pour l'ensemble des attributs
		attributeSubsetStats = new KWAttributeSubsetStats;
		attributeSubsetStats->SetLearningSpec(GetLearningSpec());
		if (GetTargetAttributeName() != "")
			attributeSubsetStats->SetClassStats(GetClassStats());

		// Selection des attributs participant a l'apprentissage
		// TODO MB cela fausse le nombre d'attributs initials
		oaTrainAttributeStats = SelectTrainAttributeStats();

		// Initialisation des attributs participant a l'apprentissage
		attributeSubsetStats->SetAttributeNumber(oaTrainAttributeStats->GetSize());
		for (nAttribute = 0; nAttribute < oaTrainAttributeStats->GetSize(); nAttribute++)
		{
			dataPreparationStats = cast(KWDataPreparationStats*, oaTrainAttributeStats->GetAt(nAttribute));
			attributeSubsetStats->SetAttributeNameAt(nAttribute, dataPreparationStats->GetSortName());
		}

		// Preparation des donnees a charger
		PrepareClassForLoad(oaTrainAttributeStats);

		// Chargement des donnees en memoire
		bOk = GetDatabase()->ReadAll();

		// Lecture des tuples necessaire pour l'apprentissage
		if (bOk)
		{
			// Parametrage du chargeur de tuples pour lire les attributs de la grille, y compris l'attribut
			// cible
			tupleTableLoader.SetInputClass(GetClass());
			tupleTableLoader.SetInputDatabaseObjects(GetDatabase()->GetObjects());
			svAttributeNames.SetSize(oaTrainAttributeStats->GetSize());
			for (nAttribute = 0; nAttribute < oaTrainAttributeStats->GetSize(); nAttribute++)
			{
				dataPreparationStats =
				    cast(KWDataPreparationStats*, oaTrainAttributeStats->GetAt(nAttribute));
				svAttributeNames.SetAt(nAttribute, dataPreparationStats->GetSortName());
			}
			if (GetTargetAttributeName() != "")
				svAttributeNames.Add(GetTargetAttributeName());

			// Lecture des tuples
			tupleTableLoader.LoadMultivariate(&svAttributeNames, &multivariateTupleTable);
		}

		// Vidage memoire de la base de donnees
		GetDatabase()->DeleteAll();
		InitClassForLoad();

		// Parametrage de l'optimisation des grilles
		refDataGridOptimizerParameters.CopyFrom(
		    GetLearningSpec()->GetPreprocessingSpec()->GetDataGridOptimizerParameters());
		GetLearningSpec()->GetPreprocessingSpec()->GetDataGridOptimizerParameters()->CopyFrom(
		    GetDataGridOptimizerParameters());

		// Calcul des stats avec le bon echantillon, si lecture OK
		if (bOk)
		{
			bOk = attributeSubsetStats->ComputeStats(&multivariateTupleTable);
			multivariateTupleTable.CleanAll();
		}

		// Restitution du parametrage d'optimisation
		GetLearningSpec()->GetPreprocessingSpec()->GetDataGridOptimizerParameters()->CopyFrom(
		    GetDataGridOptimizerParameters());

		/////////////////////////////////////////////////////////////
		// Memorisation des resultats d'apprentissage

		// Si Ok uniquement
		if (bOk)
		{
			// Parametrage de la preparation de donnees
			dataPreparationClass.SetLearningSpec(GetLearningSpec());

			// Generation de la classe de preparation des donnees
			dataPreparationClass.ComputeDataPreparationFromAttributeSubsetStats(GetClassStats(),
											    attributeSubsetStats);

			// Calcul des poids des attributs
			cTotalWeight = 0;
			for (nAttribute = 0;
			     nAttribute < dataPreparationClass.GetDataPreparationAttributes()->GetSize(); nAttribute++)
			{
				dataPreparationAttribute =
				    cast(KWDataPreparationAttribute*,
					 dataPreparationClass.GetDataPreparationAttributes()->GetAt(nAttribute));

				// Recherche du poids de l'attribut (poids a 1 par defaut)
				cWeight = (Continuous)dataPreparationAttribute->GetPreparedStats()->GetSortValue();
				assert(0 <= cWeight and cWeight <= 1);
				cTotalWeight += cWeight;
			}
			if (cTotalWeight < 0)
				cTotalWeight = 0;

			// Ajout de toutes les contributions au calcul du score
			cvWeights.SetSize(dataPreparationClass.GetDataPreparationAttributes()->GetSize());
			for (nAttribute = 0;
			     nAttribute < dataPreparationClass.GetDataPreparationAttributes()->GetSize(); nAttribute++)
			{
				dataPreparationAttribute =
				    cast(KWDataPreparationAttribute*,
					 dataPreparationClass.GetDataPreparationAttributes()->GetAt(nAttribute));

				// Recherche du poids de l'attribut (poids a 1 par defaut)
				cWeight = 1;
				if (cTotalWeight != 0)
					cWeight =
					    (Continuous)dataPreparationAttribute->GetPreparedStats()->GetSortValue() /
					    cTotalWeight;
				assert(0 <= cWeight and cWeight <= 1);
				cvWeights.SetAt(nAttribute, cWeight);
			}

			// Alimentation du rapport de prediction par les statistiques sur les grilles selectionnees
			// Appel avant l'apprentissage, pour disposer  d'un dataPreparationClass valide
			nUsedAttributeNumber =
			    FillPredictorDataGridReport(dataPreparationClass.GetDataPreparationAttributes(), NULL);

			// Apprentissage dans le cas supervise, en reutilisant le predicteur Bayesien naif
			if (GetTargetAttributeType() != KWType::None)
			{
				InternalTrainNB(&dataPreparationClass,
						dataPreparationClass.GetDataPreparationAttributes());
			}
			// Apprentissage dans le cas non supervise
			else
			{
				InternalTrainUnsupervisedDG(&dataPreparationClass,
							    dataPreparationClass.GetDataPreparationAttributes());

				// Nettoyage de la preparation, la classe de preparation etant maintenant reference par
				// le predicteur
				dataPreparationClass.RemoveDataPreparation();
			}

			// On rememorise le nombre d'attributs utilises par le predicteur
			// (il a ete mis a jour de facon erronee par les methode heritees du Naive Bayes predictor)
			GetPredictorReport()->SetUsedAttributeNumber(nUsedAttributeNumber);
		}

		// Nettoyage
		delete oaTrainAttributeStats;
		delete attributeSubsetStats;
		return bOk;
	}
	return false;
}

void KWPredictorDataGrid::InternalTrainUnsupervisedDG(KWDataPreparationClass* dataPreparationClass,
						      ObjectArray* oaDataPreparationUsedAttributes)
{
	boolean bTrace = false;
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	KWAttribute* clustererAttribute;
	KWPredictionAttributeSpec* predictionAttributeSpec;

	require(Check());
	require(GetClassStats() != NULL);
	require(GetClassStats()->IsStatsComputed());
	require(GetTargetAttributeType() == KWType::None);
	require(IsTraining());

	// Initialisation de la classe du predicteur
	GetTrainedPredictor()->SetPredictorClass(dataPreparationClass->GetDataPreparationClass(),
						 GetTargetAttributeType(), GetObjectLabel());

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

		// Creation d'un attribut de recodage, permettant d'identifier la cellule de la grille de preparation
		clustererAttribute = dataPreparationAttribute->AddPreparedIdAttribute();

		// Report du libelle de la grille de preparation
		clustererAttribute->SetLabel(dataPreparationAttribute->GetPreparedAttribute()->GetLabel());

		// Ajout dans les specification d'apprentissage de l'attribut de recodage
		predictionAttributeSpec = new KWPredictionAttributeSpec;
		predictionAttributeSpec->SetLabel("RecodingVariable");
		predictionAttributeSpec->SetType(clustererAttribute->GetType());
		predictionAttributeSpec->SetMandatory(true);
		predictionAttributeSpec->SetEvaluation(false);
		predictionAttributeSpec->SetAttribute(clustererAttribute);
		GetTrainedPredictor()->AddPredictionAttributeSpec(predictionAttributeSpec);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void KWPredictorDataGrid::PrepareClassForLoad(ObjectArray* oaSelectedAttributeStats)
{
	KWAttribute* attribute;
	KWDataPreparationStats* dataPreparationStats;
	int nAttribute;
	KWClass* learningClass;

	require(Check());
	require(oaSelectedAttributeStats != NULL);

	// Recherche de la classe a modifier
	// En utilisant temporairement cette variable, on s'affranchit des controle d'integrite declenches par le
	// GetClass()
	learningClass = GetClass();

	// Passage en Unload de tous les attributs
	learningClass->SetAllAttributesLoaded(false);

	// Passage en Loaded de l'attribut cible
	// On passe par GetLearningSpec() pour l'acces au target attribute afin d'eviter le Check() du LearningService
	// (la classe n'est plus compilee suite au passage au Unload des attributs)
	if (GetLearningSpec()->GetTargetAttributeName() != "")
	{
		attribute = learningClass->LookupAttribute(GetLearningSpec()->GetTargetAttributeName());
		check(attribute);
		attribute->SetLoaded(true);
	}

	// Passage en Load des attributs selectionnes
	for (nAttribute = 0; nAttribute < oaSelectedAttributeStats->GetSize(); nAttribute++)
	{
		dataPreparationStats = cast(KWDataPreparationStats*, oaSelectedAttributeStats->GetAt(nAttribute));

		// Passage en Load de l'attribut correspondant
		attribute = learningClass->LookupAttribute(dataPreparationStats->GetSortName());
		check(attribute);
		attribute->SetLoaded(true);
	}

	// Compilation de la classe
	learningClass->GetDomain()->Compile();
	ensure(Check());
}

void KWPredictorDataGrid::InitClassForLoad()
{
	KWClass* learningClass;

	require(Check());

	// Passage en Load de tous les attributs
	learningClass = GetClass();
	learningClass->SetAllAttributesLoaded(true);

	// Compilation de la classe
	learningClass->GetDomain()->Compile();
}

ObjectArray* KWPredictorDataGrid::SelectTrainAttributeStats()
{
	ObjectArray* oaSelectedAttributeStats;
	const int nMinValueNumber = 2;
	int nTargetModalityNumber;
	KWAttributeStats* attributeStats;
	int i;
	const longint lEmptyExeSize = 10000000;
	longint lEmptyObjectSize;
	longint lAvailableMemory;
	longint lUsedMemory;
	longint lAttributeRequiredMemory;
	int nUsedAttributes;
	ALString sTmp;

	require(Check());
	require(GetClassStats() != NULL);

	// Creation du tableau resultat
	oaSelectedAttributeStats = new ObjectArray;

	// Nombre de modalite cibles
	nTargetModalityNumber = 0;
	if (GetTargetAttributeName() != "")
		nTargetModalityNumber = GetTargetDescriptiveStats()->GetValueNumber();

	// Recopie des attributs sources utilisables
	for (i = 0; i < GetClassStats()->GetAttributeStats()->GetSize(); i++)
	{
		attributeStats = cast(KWAttributeStats*, GetClassStats()->GetAttributeStats()->GetAt(i));

		// Ajout de l'attribut si assez de valeurs differentes
		if (attributeStats->GetDescriptiveStats()->GetValueNumber() >= nMinValueNumber)
			oaSelectedAttributeStats->Add(attributeStats);
	}

	// Tri de ces attributs par importance decroissante
	// Le shuffle initial sert a ranger aleatoirement les ex-equaux
	oaSelectedAttributeStats->Shuffle();
	oaSelectedAttributeStats->SetCompareFunction(KWLearningReportCompareSortValue);
	oaSelectedAttributeStats->Sort();

	// Calcul de la memoire disponible
	lAvailableMemory = RMResourceManager::GetRemainingAvailableMemory();

	// Initialisation de la memoire necessaire pour l'apprentissage
	lEmptyObjectSize = sizeof(KWObject) + sizeof(KWObject*);
	lUsedMemory = lEmptyExeSize + GetClassStats()->GetInstanceNumber() * (lEmptyObjectSize + sizeof(KWValue));
	lUsedMemory +=
	    3 * GetClassStats()->GetInstanceNumber() * (sizeof(KWDGMCell) + nTargetModalityNumber * sizeof(KWValue));

	// On compte le nombre d'attributs utilisables pour l'apprentissage
	nUsedAttributes = 0;
	while (nUsedAttributes < oaSelectedAttributeStats->GetSize())
	{
		attributeStats = cast(KWAttributeStats*, oaSelectedAttributeStats->GetAt(nUsedAttributes));
		check(GetClass()->LookupAttribute(attributeStats->GetSortName()));

		// Estimation de la place memoire necessaire pour traiter l'attribut dans le cas continu
		if (attributeStats->GetAttributeType() == KWType::Continuous)
		{
			lAttributeRequiredMemory = GetClassStats()->GetInstanceNumber() * sizeof(KWValue);
			lAttributeRequiredMemory += 3 * attributeStats->GetDescriptiveStats()->GetValueNumber() *
						    (sizeof(KWDGMPart) + sizeof(KWDGInterval));
		}
		// et dans le cas symbolique
		else
		{
			lAttributeRequiredMemory = GetClassStats()->GetInstanceNumber() * sizeof(KWValue);
			lAttributeRequiredMemory += 3 * attributeStats->GetDescriptiveStats()->GetValueNumber() *
						    (sizeof(KWDGMPart) + sizeof(KWDGValueSet) + sizeof(KWDGValue));
		}

		// On prend en compte l'attribut si assez de memoire
		if (lUsedMemory + lAttributeRequiredMemory < lAvailableMemory)
			lUsedMemory += lAttributeRequiredMemory;
		else
		{
			// Arret avec warning utilisateur
			AddWarning(sTmp + "Train with a subset of " + IntToString(nUsedAttributes) +
				   " variables out of " + IntToString(oaSelectedAttributeStats->GetSize()) +
				   " (not enough memory)");
			break;
		}

		// Incrementation du nombre d'attributs selectionnes
		nUsedAttributes++;
	}

	// On tronque le tableau en fonction du nombre d'attributs utilises
	oaSelectedAttributeStats->SetSize(nUsedAttributes);
	return oaSelectedAttributeStats;
}

///////////////////////////////////////////////////////////////////////////////
// Classe KWPredictorDataGridReport

KWPredictorDataGridReport::KWPredictorDataGridReport()
{
	classStats = NULL;
}

KWPredictorDataGridReport::~KWPredictorDataGridReport()
{
	oaSelectedDataGridReports.DeleteAll();
}

void KWPredictorDataGridReport::SetClassStats(KWClassStats* stats)
{
	require(stats == NULL or GetLearningSpec() == stats->GetLearningSpec());
	classStats = stats;
}

KWClassStats* KWPredictorDataGridReport::GetClassStats() const
{
	ensure(classStats == NULL or GetLearningSpec() == classStats->GetLearningSpec());
	return classStats;
}

ObjectArray* KWPredictorDataGridReport::GetSelectedDataGridReports()
{
	return &oaSelectedDataGridReports;
}

void KWPredictorDataGridReport::WriteReport(ostream& ost)
{
	ObjectArray oaSortedReports;
	KWSelectedDataGridReport* dataGridReport;

	// Appel de la methode ancetre
	KWPredictorReport::WriteReport(ost);

	// Calcul des identifiants de grilles
	ComputeRankIdentifiers(&oaSelectedDataGridReports);

	// Resume des grilles selectionnees, s'il y en a plusieurs
	if (oaSelectedDataGridReports.GetSize() > 1)
		WriteArrayLineReport(ost, "Selected data grid statistics", &oaSelectedDataGridReports);

	// Detail des grilles selectionnees, s'il y en a plusieurs
	if (oaSelectedDataGridReports.GetSize() > 1)
		WriteArrayReport(ost, "Selected data grid detailed statistics", &oaSelectedDataGridReports);
	// Detail de la grille selectionnee, s'il y en a une seule
	else if (oaSelectedDataGridReports.GetSize() == 1)
	{
		dataGridReport = cast(KWSelectedDataGridReport*, oaSelectedDataGridReports.GetAt(0));

		// Rappel de l'evaluation univariee
		ost << "Level\t" << dataGridReport->GetUnivariateEvaluation() << "\n";

		// Affichage des details de la grille
		ost << "\n";
		dataGridReport->GetPreparedDataGridStats()->WritePartial(ost, true, GetTargetAttributeName() == "");
	}
}

void KWPredictorDataGridReport::WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary)
{
	KWSelectedDataGridReport* dataGridReport;
	KWDataGridStats* dataGridStats;
	ContinuousVector cvJSONAttributeMinValues;
	ContinuousVector cvJSONAttributeMaxValues;
	KWAttributeStats* attributeStats;
	KWDescriptiveContinuousStats* descriptiveContinuousStats;
	const KWDGSAttributePartition* attributePartition;
	int i;

	// Appel de la methode ancetre
	KWPredictorReport::WriteJSONArrayFields(fJSON, bSummary);

	// On affiche la grille, uniquement s'il y en a une seule
	// (a verifier: c'est peut-etre le seul cas possible actuellement)
	if (not bSummary and oaSelectedDataGridReports.GetSize() == 1)
	{
		dataGridReport = cast(KWSelectedDataGridReport*, oaSelectedDataGridReports.GetAt(0));
		dataGridStats = cast(KWDataGridStats*, dataGridReport->GetPreparedDataGridStats());

		// Rappel de l'evaluation univariee
		fJSON->WriteKeyDouble("level", dataGridReport->GetUnivariateEvaluation());

		// Parametrage des bornes des attributs numeriques de la grilles
		cvJSONAttributeMinValues.SetSize(dataGridStats->GetAttributeNumber());
		cvJSONAttributeMaxValues.SetSize(dataGridStats->GetAttributeNumber());

		// On les met a missing par defaut
		for (i = 0; i < cvJSONAttributeMinValues.GetSize(); i++)
		{
			cvJSONAttributeMinValues.SetAt(i, KWContinuous::GetMissingValue());
			cvJSONAttributeMaxValues.SetAt(i, KWContinuous::GetMissingValue());
		}

		// On les parametre correctement si possible
		if (classStats != NULL)
		{
			for (i = 0; i < dataGridStats->GetAttributeNumber(); i++)
			{
				attributePartition = dataGridStats->GetAttributeAt(i);

				// Traitement des attributs numeriques
				if (attributePartition->GetAttributeType() == KWType::Continuous)
				{
					descriptiveContinuousStats = NULL;

					// Cas d'un attribut source
					attributeStats =
					    classStats->LookupAttributeStats(attributePartition->GetAttributeName());
					if (attributeStats != NULL)
						descriptiveContinuousStats =
						    cast(KWDescriptiveContinuousStats*,
							 attributeStats->GetDescriptiveStats());
					// Cas de l'attribut cible
					else if (classStats->GetLearningSpec()->IsTargetStatsComputed() and
						 attributePartition->GetAttributeName() ==
						     classStats->GetTargetAttributeName())
						descriptiveContinuousStats =
						    cast(KWDescriptiveContinuousStats*,
							 classStats->GetTargetDescriptiveStats());

					// Parametrage des bornes
					if (descriptiveContinuousStats != NULL)
					{
						cvJSONAttributeMinValues.SetAt(i, descriptiveContinuousStats->GetMin());
						cvJSONAttributeMaxValues.SetAt(i, descriptiveContinuousStats->GetMax());
					}
				}
			}
		}

		// Parametrage de la grille
		dataGridStats->SetJSONAttributeMinValues(&cvJSONAttributeMinValues);
		dataGridStats->SetJSONAttributeMaxValues(&cvJSONAttributeMaxValues);

		// Affichage des details de la grille
		dataGridStats->WriteJSONKeyReport(fJSON, "dataGrid");

		// Nettoyage du parametrage
		dataGridStats->SetJSONAttributeMinValues(NULL);
		dataGridStats->SetJSONAttributeMaxValues(NULL);
	}
}

boolean KWPredictorDataGridReport::IsJSONReported(boolean bSummary) const
{
	boolean bIsReported;

	// Appel de la methode ancetre
	bIsReported = KWPredictorReport::IsJSONReported(bSummary);

	// Specialisation pour le rapport detaille
	if (not bSummary)
	{
		bIsReported = oaSelectedDataGridReports.GetSize() == 1;
	}
	return bIsReported;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWSelectedDataGridReport

KWSelectedDataGridReport::KWSelectedDataGridReport()
{
	dUnivariateEvaluation = 0;
	dWeight = 0;
	preparedDataGridStats = NULL;
}

KWSelectedDataGridReport::~KWSelectedDataGridReport()
{
	if (preparedDataGridStats != NULL)
		delete preparedDataGridStats;
}

void KWSelectedDataGridReport::SetPreparedDataGridStats(KWDataGridStats* dataGridStats)
{
	if (preparedDataGridStats != NULL)
		delete preparedDataGridStats;
	preparedDataGridStats = dataGridStats;
}

const KWDataGridStats* KWSelectedDataGridReport::GetPreparedDataGridStats() const
{
	return preparedDataGridStats;
}

void KWSelectedDataGridReport::SetUnivariateEvaluation(double dValue)
{
	dUnivariateEvaluation = dValue;
}

double KWSelectedDataGridReport::GetUnivariateEvaluation() const
{
	return dUnivariateEvaluation;
}

void KWSelectedDataGridReport::SetWeight(double dValue)
{
	dWeight = dValue;
}

double KWSelectedDataGridReport::GetWeight() const
{
	return dWeight;
}

void KWSelectedDataGridReport::WriteHeaderLineReport(ostream& ost)
{
	require(preparedDataGridStats != NULL);

	// Rang
	ost << "Rank\t";

	// Identification des attributs
	ost << "Names\t";

	// Evaluation univariee
	ost << "Level\t";

	// Poids
	ost << "Weight\t";

	// Statistiques sur la grille
	ost << "Variables\t";
	ost << "Cells\t";
	ost << "Grid size";
}

void KWSelectedDataGridReport::WriteLineReport(ostream& ost)
{
	require(preparedDataGridStats != NULL);

	// Rang
	ost << GetIdentifier() << "\t";

	// Nom de la grille
	ost << GetSortName();
	ost << "\t";

	// Evaluation univariee
	ost << dUnivariateEvaluation << "\t";

	// Poids
	ost << dWeight << "\t";

	// Affichage des statistiques, selon le cas supervise ou non
	ost << preparedDataGridStats->ComputeInformativeAttributeNumber() << "\t";
	ost << preparedDataGridStats->ComputeCellNumber() << "\t";
	ost << preparedDataGridStats->ComputeGridSize();
}

void KWSelectedDataGridReport::WriteReport(ostream& ost)
{
	require(preparedDataGridStats != NULL);

	// Rang et nom de la grille
	ost << "\n";
	ost << "Rank\t" << GetIdentifier() << "\n";
	ost << "Data grid\t";
	ost << GetSortName();
	ost << "\n";

	// Affichage des details de la grille
	preparedDataGridStats->WritePartial(ost, true, preparedDataGridStats->GetSourceAttributeNumber() == 0);
}

const ALString KWSelectedDataGridReport::GetSortName() const
{
	ALString sSortName;
	int i;
	int nAttributeNumber;

	require(preparedDataGridStats != NULL);

	// Calcul du nombre d'attribut, selon la nature supervisee ou non de la grille
	if (preparedDataGridStats->GetSourceAttributeNumber() == 0)
		nAttributeNumber = preparedDataGridStats->GetAttributeNumber();
	else
		nAttributeNumber = preparedDataGridStats->GetSourceAttributeNumber();

	// Affichage des attributs
	for (i = 0; i < nAttributeNumber; i++)
	{
		if (i > 0)
			sSortName += "&";
		sSortName += preparedDataGridStats->GetAttributeAt(i)->GetAttributeName();
	}
	return sSortName;
}

double KWSelectedDataGridReport::GetSortValue() const
{
	return GetWeight();
}
