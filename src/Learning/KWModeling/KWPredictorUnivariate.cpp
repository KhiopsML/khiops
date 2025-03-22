// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorUnivariate.h"

////////////////////////////////////////////////////////////////////////////////
// Classe KWPredictorUnivariate

KWPredictorUnivariate::KWPredictorUnivariate()
{
	nUnivariateRank = 0;
	bBestUnivariate = true;
}

KWPredictorUnivariate::~KWPredictorUnivariate() {}

KWPredictor* KWPredictorUnivariate::Create() const
{
	return new KWPredictorUnivariate;
}

const ALString KWPredictorUnivariate::GetName() const
{
	return "Univariate";
}

const ALString KWPredictorUnivariate::GetPrefix() const
{
	ALString sPrefix;
	sPrefix = "BU";
	if (nUnivariateRank > 0)
		sPrefix += IntToString(nUnivariateRank);
	return sPrefix;
}

const ALString KWPredictorUnivariate::GetSuffix() const
{
	if (nUnivariateRank == 0)
		return GetAttributeName();
	else
		return "";
}

void KWPredictorUnivariate::SetUnivariateRank(int nValue)
{
	require(nValue >= 0);
	nUnivariateRank = nValue;
}

int KWPredictorUnivariate::GetUnivariateRank() const
{
	return nUnivariateRank;
}

void KWPredictorUnivariate::CopyFrom(const KWPredictor* kwpSource)
{
	const KWPredictorUnivariate* kwpuSource = cast(const KWPredictorUnivariate*, kwpSource);

	// Appel de la methode ancetre
	KWPredictor::CopyFrom(kwpSource);

	// Recopie des nouveau attributs de specification
	SetBestUnivariate(kwpuSource->GetBestUnivariate());
	SetAttributeName(kwpuSource->GetAttributeName());
}

void KWPredictorUnivariate::SetBestUnivariate(boolean bValue)
{
	bBestUnivariate = bValue;
}

boolean KWPredictorUnivariate::GetBestUnivariate() const
{
	return bBestUnivariate;
}

void KWPredictorUnivariate::SetAttributeName(const ALString& sValue)
{
	sAttributeName = sValue;
}

const ALString& KWPredictorUnivariate::GetAttributeName() const
{
	return sAttributeName;
}

KWPredictorEvaluation* KWPredictorUnivariate::Evaluate(KWDatabase* database)
{
	KWPredictorEvaluation* predictorEvaluation;

	require(IsTrained());
	require(database != NULL);

	// Creation des resultats d'evaluation selon le type de predicteur
	if (GetTargetAttributeType() == KWType::Symbol)
	{
		predictorEvaluation = new KWClassifierUnivariateEvaluation;
	}
	else if (GetTargetAttributeType() == KWType::Continuous)
	{
		predictorEvaluation = new KWRegressorUnivariateEvaluation;
	}
	else
	{
		assert(GetTargetAttributeType() == KWType::None);
		predictorEvaluation = new KWPredictorEvaluation;
	}

	// Evaluation
	predictorEvaluation->Evaluate(this, database);
	return predictorEvaluation;
}

const ALString& KWPredictorUnivariate::GetSourceAttributeName() const
{
	require(IsTrained());
	ensure(trainedPredictor->GetPredictorClass()->LookupAttribute(sSourceAttributeName) != NULL);
	return sSourceAttributeName;
}

const ALString& KWPredictorUnivariate::GetUnivariateAttributeName() const
{
	require(IsTrained());
	ensure(trainedPredictor->GetPredictorClass()->LookupAttribute(sUnivariateAttributeName) != NULL);
	return sUnivariateAttributeName;
}

const KWDataGridStats* KWPredictorUnivariate::GetTrainDataGridStats() const
{
	require(Check());
	require(IsTrained());
	require(GetUnivariateAttributeName() != "");

	return &trainDataGridStats;
}

const ALString KWPredictorUnivariate::GetObjectLabel() const
{
	return GetName() + " " + GetAttributeName();
}

boolean KWPredictorUnivariate::InternalTrain()
{
	const ALString sSourceVariableMetaDataKey = "SourceVariable";
	KWDataPreparationClass dataPreparationClass;
	ObjectArray oaTheDataPreparationAttribute;
	int nAttribute;
	KWDataPreparationAttribute* dataPreparationAttribute;
	double dCost;
	double dBestCost;
	int nBestAttribute;
	KWAttribute* predictorSourceAttribute;
	KWPredictionAttributeSpec* predictionAttributeSpec;

	require(Check());
	require(GetClassStats() != NULL);
	require(GetBestUnivariate() or (GetClass()->LookupAttribute(GetAttributeName()) != NULL and
					GetClass()->LookupAttribute(GetAttributeName())->GetUsed()));

	// Calcul des statistiques si necessaire
	if (not GetClassStats()->IsStatsComputed())
		GetClassStats()->ComputeStats();
	assert(not GetClassStats()->IsStatsComputed() or GetTargetValueStats() != NULL);
	assert(not GetClassStats()->IsStatsComputed() or GetTargetDescriptiveStats() != NULL);

	// Apprentissage si au moins une valeur cible
	if (GetLearningSpec()->IsTargetStatsComputed() and GetTargetDescriptiveStats()->GetValueNumber() > 0)
	{
		// Parametrage de la preparation de donnees
		dataPreparationClass.SetLearningSpec(GetLearningSpec());

		// Generation de la classe de preparation des donnees
		dataPreparationClass.ComputeDataPreparationFromClassStats(GetClassStats());

		// Recherche de l'attribut a utiliser pour un predicteur univarie
		// Cas du meilleurs possible
		nBestAttribute = -1;
		if (GetBestUnivariate())
		{
			// Recherche du meilleur attribut
			dBestCost = -DBL_MAX;
			for (nAttribute = 0;
			     nAttribute < dataPreparationClass.GetDataPreparationAttributes()->GetSize(); nAttribute++)
			{
				dataPreparationAttribute =
				    cast(KWDataPreparationAttribute*,
					 dataPreparationClass.GetDataPreparationAttributes()->GetAt(nAttribute));

				// Test si l'attribut univarie pretraite
				if (dataPreparationAttribute->GetNativeAttributeNumber() == 1)
				{
					dCost = dataPreparationAttribute->GetPreparedStats()->GetLevel();

					// Test si amelioration
					if (dCost > dBestCost)
					{
						dBestCost = dCost;
						nBestAttribute = nAttribute;
					}
				}
			}
		}
		// Sinon, recherche de l'attribut correspondant au nom specifie
		else
		{
			// Recherche de l'attribut discretise ou groupe base sur l'attribut source
			nBestAttribute = -1;
			for (nAttribute = 0;
			     nAttribute < dataPreparationClass.GetDataPreparationAttributes()->GetSize(); nAttribute++)
			{
				dataPreparationAttribute =
				    cast(KWDataPreparationAttribute*,
					 dataPreparationClass.GetDataPreparationAttributes()->GetAt(nAttribute));

				// Test si l'attribut correspond
				if (dataPreparationAttribute->GetNativeAttributeNumber() == 1 and
				    dataPreparationAttribute->GetNativeAttribute()->GetName() == GetAttributeName())
				{
					nBestAttribute = nAttribute;
					break;
				}
			}
		}

		// Si echec: nettoyage
		sSourceAttributeName = "";
		sUnivariateAttributeName = "";
		trainDataGridStats.DeleteAll();
		if (nBestAttribute == -1)
		{
			// Nettoyage
			dataPreparationClass.DeleteDataPreparation();
			return false;
		}
		// Sinon, construction du predicteur univarie
		else
		{
			// Initialisation des donnees correspondant a l'attribut choisi
			dataPreparationAttribute =
			    cast(KWDataPreparationAttribute*,
				 dataPreparationClass.GetDataPreparationAttributes()->GetAt(nBestAttribute));
			oaTheDataPreparationAttribute.Add(dataPreparationAttribute);

			// Memorisation des caracteristiques d'apprentissage
			sSourceAttributeName = dataPreparationAttribute->GetNativeAttribute()->GetName();
			sUnivariateAttributeName = dataPreparationAttribute->GetPreparedAttribute()->GetName();

			// Memorisation des statistiques sur l'attribut
			trainDataGridStats.CopyFrom(
			    dataPreparationAttribute->GetPreparedStats()->GetPreparedDataGridStats());

			// Construction d'un predicteur bayesien naif a partir de l'attribut selectionne
			// Appele en dernier, car l'objet dataPreparationClass est nettoye suite a cet a l'appel de
			// InternalTrainNB
			InternalTrainNB(&dataPreparationClass, &oaTheDataPreparationAttribute);

			// Nettoyage initial des meta-donnees des attributs
			GetTrainedPredictor()->GetPredictorClass()->RemoveAllAttributesMetaDataKey(
			    sSourceVariableMetaDataKey);

			// Ajout de l'attribut source dans les specifications d'apprentissage, pour permettre
			// l'evaluation de la grille de preparation de l'attribut sur une base de test
			predictorSourceAttribute =
			    GetTrainedPredictor()->GetPredictorClass()->LookupAttribute(sSourceAttributeName);
			predictionAttributeSpec = new KWPredictionAttributeSpec;
			predictionAttributeSpec->SetLabel(sSourceVariableMetaDataKey);
			predictionAttributeSpec->SetType(predictorSourceAttribute->GetType());
			predictionAttributeSpec->SetMandatory(false);
			predictionAttributeSpec->SetEvaluation(true);
			predictionAttributeSpec->SetAttribute(predictorSourceAttribute);
			GetTrainedPredictor()->AddPredictionAttributeSpec(predictionAttributeSpec);
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
// Classe KWEvaluatedDataGridStats

KWEvaluatedDataGridStats::KWEvaluatedDataGridStats()
{
	predictorDataGridStats = NULL;
}

KWEvaluatedDataGridStats::~KWEvaluatedDataGridStats() {}

void KWEvaluatedDataGridStats::SetPredictorDataGridStats(const KWDataGridStats* dataGridStats)
{
	predictorDataGridStats = dataGridStats;
}

const KWDataGridStats* KWEvaluatedDataGridStats::GetPredictorDataGridStats() const
{
	return predictorDataGridStats;
}

void KWEvaluatedDataGridStats::InitializeEvaluation(const KWPredictor* predictor)
{
	KWClassStats* classStats;
	KWAttributeStats* attributeStats;
	KWDescriptiveContinuousStats* descriptiveContinuousStats;
	const KWDGSAttributePartition* attributePartition;
	int i;

	require(predictor != NULL);
	require(predictorDataGridStats != NULL);

	// Parametrage des bornes des attributs numeriques de la grilles
	cvJSONEvaluatedAttributeDomainLowerBounds.SetSize(predictorDataGridStats->GetAttributeNumber());
	cvJSONEvaluatedAttributeDomainUpperBounds.SetSize(predictorDataGridStats->GetAttributeNumber());

	// On les met a missing par defaut
	for (i = 0; i < cvJSONEvaluatedAttributeDomainLowerBounds.GetSize(); i++)
	{
		cvJSONEvaluatedAttributeDomainLowerBounds.SetAt(i, KWContinuous::GetMissingValue());
		cvJSONEvaluatedAttributeDomainUpperBounds.SetAt(i, KWContinuous::GetMissingValue());
	}

	// On les parametre correctement si possible
	classStats = predictor->GetClassStats();
	if (classStats != NULL)
	{
		for (i = 0; i < predictorDataGridStats->GetAttributeNumber(); i++)
		{
			attributePartition = predictorDataGridStats->GetAttributeAt(i);

			// Traitement des attributs numeriques
			if (attributePartition->GetAttributeType() == KWType::Continuous)
			{
				descriptiveContinuousStats = NULL;

				// Cas d'un attribut source
				attributeStats =
				    classStats->LookupAttributeStats(attributePartition->GetAttributeName());
				if (attributeStats != NULL)
					descriptiveContinuousStats =
					    cast(KWDescriptiveContinuousStats*, attributeStats->GetDescriptiveStats());
				// Cas de l'attribut cible
				else if (classStats->GetLearningSpec()->IsTargetStatsComputed() and
					 attributePartition->GetAttributeName() == classStats->GetTargetAttributeName())
					descriptiveContinuousStats = cast(KWDescriptiveContinuousStats*,
									  classStats->GetTargetDescriptiveStats());

				// Parametrage des bornes
				if (descriptiveContinuousStats != NULL)
				{
					cvJSONEvaluatedAttributeDomainLowerBounds.SetAt(
					    i, descriptiveContinuousStats->GetMin());
					cvJSONEvaluatedAttributeDomainUpperBounds.SetAt(
					    i, descriptiveContinuousStats->GetMax());
				}
			}
		}
	}
}

void KWEvaluatedDataGridStats::WriteJSONFields(JSONFile* fJSON)
{
	// Parametrage de la grille
	SetJSONAttributeDomainLowerBounds(&cvJSONEvaluatedAttributeDomainLowerBounds);
	SetJSONAttributeDomainUpperBounds(&cvJSONEvaluatedAttributeDomainUpperBounds);

	// Ecriture du rapport JSON en appelant la methode ancetre,
	// qui est correctement parametree le temps de l'ecriture
	KWDataGridStats::WriteJSONFields(fJSON);

	// Nettoyage du parametrage
	SetJSONAttributeDomainLowerBounds(NULL);
	SetJSONAttributeDomainUpperBounds(NULL);
}

void KWEvaluatedDataGridStats::SortSourceCells(ObjectArray* oaSourceCells, int nTargetAttributeIndex) const
{
	ObjectArray oaPredictorSourceCells;
	int i;
	KWDGSSourceCell* predictorCell;
	KWDGSSourceCell* currentCell;
	NumericKeyDictionary nkdCurrentCells;

	require(oaSourceCells != NULL);
	require(0 <= nTargetAttributeIndex and nTargetAttributeIndex < GetTargetAttributeNumber());

	// En presence d'une grille en apprentissage, on tri les cellules de la la grille d'apprentissage,
	// puis les cellule de la grille d'evaluation selon le meme ordre qu'en apprentissage
	if (predictorDataGridStats != NULL)
	{
		// Acces au cellule en apprentissage
		predictorDataGridStats->ExportSourceCellsAt(&oaPredictorSourceCells,
							    GetFirstTargetAttributeIndex() + nTargetAttributeIndex);

		// On continue si on a les memes nombre de cellules
		if (oaPredictorSourceCells.GetSize() == oaSourceCells->GetSize())
		{
			// Memorisation de l'association entre les deux ordonnancements, qui doivent etre les memes
			for (i = 0; i < oaPredictorSourceCells.GetSize(); i++)
			{
				predictorCell = cast(KWDGSSourceCell*, oaPredictorSourceCells.GetAt(i));
				currentCell = cast(KWDGSSourceCell*, oaSourceCells->GetAt(i));
				assert(KWDGSCellCompareIndexes(&predictorCell, &currentCell) == 0);
				nkdCurrentCells.SetAt(predictorCell, currentCell);
			}

			// Tri de ces cellules selon leur interet dans le predicteur
			oaPredictorSourceCells.SetCompareFunction(KWDGSSourceCellCompareDecreasingInterest);
			oaPredictorSourceCells.Sort();

			// On tri alors les cellules courantes selon le meme ordre
			for (i = 0; i < oaPredictorSourceCells.GetSize(); i++)
			{
				predictorCell = cast(KWDGSSourceCell*, oaPredictorSourceCells.GetAt(i));
				currentCell = cast(KWDGSSourceCell*, nkdCurrentCells.Lookup(predictorCell));
				oaSourceCells->SetAt(i, currentCell);
			}
		}

		// Nettoyage
		oaPredictorSourceCells.DeleteAll();
	}
	// Sinon, comportement standard
	else
		KWDataGridStats::SortSourceCells(oaSourceCells, nTargetAttributeIndex);
}

////////////////////////////////////////////////////////////////////////////////
// Classe KWClassifierUnivariateEvaluation

KWClassifierUnivariateEvaluation::KWClassifierUnivariateEvaluation() {}

KWClassifierUnivariateEvaluation::~KWClassifierUnivariateEvaluation() {}

void KWClassifierUnivariateEvaluation::InitializeCriteria()
{
	// Appel a la methode ancetre
	KWClassifierEvaluation::InitializeCriteria();

	// Initialisation de la grille
	dgsEvaluationResults.DeleteAll();
}

void KWClassifierUnivariateEvaluation::Evaluate(KWPredictor* predictor, KWDatabase* database)
{
	KWPredictorUnivariate* univariatePredictor;

	require(predictor != NULL);

	// Parametrage de la grille resultat d'evaluation
	univariatePredictor = cast(KWPredictorUnivariate*, predictor);
	dgsEvaluationResults.SetPredictorDataGridStats(univariatePredictor->GetTrainDataGridStats());
	dgsEvaluationResults.InitializeEvaluation(univariatePredictor);

	// Appel a la methode ancetre
	KWClassifierEvaluation::Evaluate(predictor, database);
}

KWDataGridStats* KWClassifierUnivariateEvaluation::GetEvaluatedDataGridStats()
{
	return &dgsEvaluationResults;
}

void KWClassifierUnivariateEvaluation::WriteReport(ostream& ost)
{
	// Appel de la methode ancetre
	KWClassifierEvaluation::WriteReport(ost);

	// Affichage de la grille d'evaluation
	ost << "\n";
	GetEvaluatedDataGridStats()->WriteCellArrayLineReport(ost);
}

void KWClassifierUnivariateEvaluation::WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary)
{
	// Appel de la methode ancetre
	KWClassifierEvaluation::WriteJSONArrayFields(fJSON, bSummary);

	// Ecriture de la grille
	if (not bSummary)
		GetEvaluatedDataGridStats()->WriteJSONKeyReport(fJSON, "dataGrid");
}

boolean KWClassifierUnivariateEvaluation::IsJSONReported(boolean bSummary) const
{
	return true;
}

KWPredictorEvaluationTask* KWClassifierUnivariateEvaluation::CreatePredictorEvaluationTask()
{
	return new KWClassifierUnivariateEvaluationTask;
}

////////////////////////////////////////////////////////////////////////////////
// Classe KWRegressorUnivariateEvaluation

KWRegressorUnivariateEvaluation::KWRegressorUnivariateEvaluation() {}

KWRegressorUnivariateEvaluation::~KWRegressorUnivariateEvaluation() {}

void KWRegressorUnivariateEvaluation::InitializeCriteria()
{
	// Appel a la methode ancetre
	KWRegressorEvaluation::InitializeCriteria();

	// Initialisation de la grille
	dgsEvaluationResults.DeleteAll();
}

void KWRegressorUnivariateEvaluation::Evaluate(KWPredictor* predictor, KWDatabase* database)
{
	KWPredictorUnivariate* univariatePredictor;

	require(predictor != NULL);

	// Parametrage de la grille resultat d'evaluation
	univariatePredictor = cast(KWPredictorUnivariate*, predictor);
	dgsEvaluationResults.SetPredictorDataGridStats(univariatePredictor->GetTrainDataGridStats());
	dgsEvaluationResults.InitializeEvaluation(univariatePredictor);

	// Appel a la methode ancetre
	KWRegressorEvaluation::Evaluate(predictor, database);
}

KWDataGridStats* KWRegressorUnivariateEvaluation::GetEvaluatedDataGridStats()
{
	return &dgsEvaluationResults;
}

void KWRegressorUnivariateEvaluation::WriteReport(ostream& ost)
{
	// Appel de la methode ancetre
	KWRegressorEvaluation::WriteReport(ost);

	// Affichage de la grille d'evaluation
	ost << "\n";
	GetEvaluatedDataGridStats()->WriteCellArrayLineReport(ost);
}

void KWRegressorUnivariateEvaluation::WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary)
{
	// Appel de la methode ancetre
	KWRegressorEvaluation::WriteJSONArrayFields(fJSON, bSummary);

	// Ecriture de la grille
	if (not bSummary)
		GetEvaluatedDataGridStats()->WriteJSONKeyReport(fJSON, "dataGrid");
}

boolean KWRegressorUnivariateEvaluation::IsJSONReported(boolean bSummary) const
{
	return true;
}

KWPredictorEvaluationTask* KWRegressorUnivariateEvaluation::CreatePredictorEvaluationTask()
{
	return new KWRegressorUnivariateEvaluationTask;
}

////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridEvaluation
//
KWDataGridEvaluation::KWDataGridEvaluation()
{
	predictorDataGridStats = NULL;
	evaluatedDataGridStats = NULL;
	evaluatedDataGrid = NULL;
	livDataGridAttributes = NULL;
}

KWDataGridEvaluation::~KWDataGridEvaluation()
{
	// Les variables de travail sont en principe nulles au moment de la destruction
	assert(evaluatedDataGrid == NULL);
	assert(oaExplicativeParts.GetSize() == 0);
	assert(oaTargetParts.GetSize() == 0);

	// Seul le resultat d'evaluation peut etre non nul au moment de la destruction
	if (evaluatedDataGridStats != NULL)
		delete evaluatedDataGridStats;
}

void KWDataGridEvaluation::SetPredictorDataGridStats(const KWDataGridStats* dataGridStats)
{
	predictorDataGridStats = dataGridStats;
}

const KWDataGridStats* KWDataGridEvaluation::GetPredictorDataGridStats() const
{
	return predictorDataGridStats;
}

void KWDataGridEvaluation::SetDataGridAttributeLoadIndexes(const KWLoadIndexVector* attributeLoadIndexes)
{
	livDataGridAttributes = attributeLoadIndexes;
}

const KWLoadIndexVector* KWDataGridEvaluation::GetDataGridAttributeLoadIndexes() const
{
	return livDataGridAttributes;
}

void KWDataGridEvaluation::Initialize()
{
	KWDataGridStats unsupervisedDataGridStats;

	require(evaluatedDataGrid == NULL);
	require(predictorDataGridStats != NULL);

	// Nettoyage du resultat d'evaluation
	if (evaluatedDataGridStats != NULL)
		delete evaluatedDataGridStats;
	evaluatedDataGridStats = NULL;

	// On passe par une grille de stats non supervisee intermediaire, pour ne pas gerer les cas
	// speciaux de KWDataGrid avec un sans attribut cible dedie (qui empeche tout traitement generique)
	unsupervisedDataGridStats.CopyFrom(predictorDataGridStats);
	unsupervisedDataGridStats.SetSourceAttributeNumber(0);
	unsupervisedDataGridStats.SetMainTargetModalityIndex(-1);

	// Creation d'une grille algorithmique a partir de la grille de stats du predicteur
	// Ce type de grille permet de peupler une grille avec un nombre d'instances (en test)
	// non coherent avec la granularite du predicteur
	evaluatedDataGrid = new KWEvaluatedDataGrid;
	evaluatedDataGrid->ImportDataGridStats(&unsupervisedDataGridStats);
	assert(evaluatedDataGrid->GetAttributeNumber() == predictorDataGridStats->GetAttributeNumber());
	evaluatedDataGrid->GetAttributeAt(0)->ExportParts(&oaExplicativeParts);
	evaluatedDataGrid->GetAttributeAt(1)->ExportParts(&oaTargetParts);

	// On la vide de toutes ses instances, et on la prepare aux mises a jour
	evaluatedDataGrid->DeleteAllCells();
	evaluatedDataGrid->SetCellUpdateMode(true);
	evaluatedDataGrid->BuildIndexingStructure();
}

void KWDataGridEvaluation::AddInstance(const KWObject* kwoObject)
{
	boolean bDisplayInstanceCreation = false;
	ObjectArray oaParts;
	Continuous cValue;
	Symbol sValue;
	KWDGPart* part;
	int nAttribute;
	KWDGAttribute* dgAttribute;
	KWDGCell* cell;

	require(GetPredictorDataGridStats() != NULL);
	require(evaluatedDataGrid != NULL);
	require(evaluatedDataGridStats == NULL);
	require(evaluatedDataGrid->GetAttributeNumber() == predictorDataGridStats->GetAttributeNumber());
	require(evaluatedDataGrid->GetTargetValueNumber() == 0);
	require(kwoObject != NULL);

	// Recherche des parties pour les valeurs de l'objet courant
	oaParts.SetSize(evaluatedDataGrid->GetAttributeNumber());
	for (nAttribute = 0; nAttribute < evaluatedDataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = evaluatedDataGrid->GetAttributeAt(nAttribute);

		// Recherche de la partie associee a la valeur selon son type
		if (dgAttribute->GetAttributeType() == KWType::Continuous)
		{
			cValue = kwoObject->GetContinuousValueAt(livDataGridAttributes->GetAt(nAttribute));
			part = dgAttribute->LookupContinuousPart(cValue);
			oaParts.SetAt(nAttribute, part);
			if (bDisplayInstanceCreation)
				cout << cValue << "\t";
		}
		else
		{
			sValue = kwoObject->GetSymbolValueAt(livDataGridAttributes->GetAt(nAttribute));
			part = dgAttribute->LookupSymbolPart(sValue);
			oaParts.SetAt(nAttribute, part);
			if (bDisplayInstanceCreation)
				cout << sValue << "\t";
		}
	}

	// Recherche de la cellule
	cell = evaluatedDataGrid->LookupCell(&oaParts);

	// Creation si necessaire
	if (cell == NULL)
		cell = evaluatedDataGrid->AddCell(&oaParts);

	// Mise a jour des effectifs de la cellule
	cell->SetCellFrequency(cell->GetCellFrequency() + 1);

	// Affichage de la cellule
	if (bDisplayInstanceCreation)
		cout << *cell;
}

void KWDataGridEvaluation::AddEvaluatedDataGridStats(KWDataGridStats* addedEvaluatedDataGrid)
{
	int nNumVariableParts;
	int nVariablePartIndex;
	int nNumTargetParts;
	int nTargetPartIndex;
	int nDeltaCellFrequency;
	KWDGCell* cellToUpdate;
	ObjectArray currentParts;

	require(addedEvaluatedDataGrid != NULL);
	require(addedEvaluatedDataGrid->GetAttributeNumber() == 2);
	require(addedEvaluatedDataGrid->GetTargetAttributeNumber() == 1);

	nNumVariableParts = addedEvaluatedDataGrid->GetAttributeAt(0)->GetPartNumber();
	nNumTargetParts = addedEvaluatedDataGrid->GetAttributeAt(1)->GetPartNumber();

	currentParts.SetSize(2);
	for (nVariablePartIndex = 0; nVariablePartIndex < nNumVariableParts; nVariablePartIndex++)
	{
		currentParts.SetAt(0, oaExplicativeParts.GetAt(nVariablePartIndex));
		for (nTargetPartIndex = 0; nTargetPartIndex < nNumTargetParts; nTargetPartIndex++)
		{
			nDeltaCellFrequency =
			    addedEvaluatedDataGrid->GetBivariateCellFrequencyAt(nVariablePartIndex, nTargetPartIndex);

			currentParts.SetAt(1, oaTargetParts.GetAt(nTargetPartIndex));
			cellToUpdate = evaluatedDataGrid->LookupCell(&currentParts);

			if (cellToUpdate == NULL)
				cellToUpdate = evaluatedDataGrid->AddCell(&currentParts);

			cellToUpdate->SetCellFrequency(cellToUpdate->GetCellFrequency() + nDeltaCellFrequency);
		}
	}
}

void KWDataGridEvaluation::Finalize()
{
	KWDataGridStats workingDataGridStats;
	int nCellNumber;
	int nCellIndex;
	IntVector ivPartIndexes;

	require(GetPredictorDataGridStats() != NULL);
	require(evaluatedDataGrid != NULL);
	require(evaluatedDataGridStats == NULL);
	require(evaluatedDataGrid->GetAttributeNumber() == predictorDataGridStats->GetAttributeNumber());

	// Fin du mode update de la grille algorithmique
	evaluatedDataGrid->SetCellUpdateMode(false);
	evaluatedDataGrid->DeleteIndexingStructure();

	// Memorisation du resultat dans une grille de stats intermediaires, pour recuperer les effectifs
	evaluatedDataGrid->ExportDataGridStats(&workingDataGridStats);

	// On duplique la grille initiale pour avoir exactement les memes definitions de partition
	// En effet, l'export de DataGrid ne gere que les partition en groupes de valeurs,
	// pas en valeurs individuelles
	// On cree une specialisation de la grille permettant la personnalisation des rapports
	evaluatedDataGridStats = new KWEvaluatedDataGridStats;
	evaluatedDataGridStats->CopyFrom(GetPredictorDataGridStats());
	evaluatedDataGridStats->SetPredictorDataGridStats(GetPredictorDataGridStats());

	// Remise a 0 des cellules
	evaluatedDataGridStats->DeleteAllCells();
	evaluatedDataGridStats->CreateAllCells();
	assert(evaluatedDataGridStats->GetAttributeNumber() == workingDataGridStats.GetAttributeNumber());
	assert(evaluatedDataGridStats->ComputeTotalGridSize() == workingDataGridStats.ComputeTotalGridSize());

	// Recopie des effectifs collectes
	nCellNumber = workingDataGridStats.ComputeTotalGridSize();
	ivPartIndexes.SetSize(workingDataGridStats.GetAttributeNumber());
	for (nCellIndex = 0; nCellIndex < nCellNumber; nCellIndex++)
	{
		workingDataGridStats.ComputePartIndexes(nCellIndex, &ivPartIndexes);
		assert(evaluatedDataGridStats->CheckPartIndexes(&ivPartIndexes));
		evaluatedDataGridStats->SetCellFrequencyAt(&ivPartIndexes,
							   workingDataGridStats.GetCellFrequencyAt(&ivPartIndexes));
	}

	// Nettoyage
	delete evaluatedDataGrid;
	evaluatedDataGrid = NULL;
	oaExplicativeParts.SetSize(0);
	oaTargetParts.SetSize(0);

	// Verifications que la grille de stats ayant les memes caracteristiques que la grille du predicteur
	// (la grille intermediaire etait non supervise, pour des raisons de traitement generique)
	ensure(evaluatedDataGridStats->ComputeTotalGridSize() == predictorDataGridStats->ComputeTotalGridSize());
	ensure(evaluatedDataGridStats->GetAttributeNumber() == predictorDataGridStats->GetAttributeNumber());
	ensure(evaluatedDataGridStats->GetSourceAttributeNumber() ==
	       predictorDataGridStats->GetSourceAttributeNumber());
	ensure(evaluatedDataGridStats->GetMainTargetModalityIndex() ==
	       predictorDataGridStats->GetMainTargetModalityIndex());
}

KWDataGridStats* KWDataGridEvaluation::GetEvaluatedDataGridStats()
{
	return evaluatedDataGridStats;
}

////////////////////////////////////////////////////////////////////////////////
// Classe KWEvalutedDataGrid

boolean KWEvaluatedDataGrid::Check() const
{
	boolean bOk = true;
	ObjectDictionary odAttributes;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGPart* part;
	KWDGCell* cell;
	int nAttributeCellNumber;
	ALString sTmp;
	int nTargetAttributeNumber;

	// Initialisation du nombre d'attributs indiques comme cible
	nTargetAttributeNumber = 0;

	// Verification de la granularite
	if (nGranularity < 0)
	{
		AddError(sTmp + "Granularity " + IntToString(nGranularity) + "  must be an integer greater than 0");
		bOk = false;
	}

	// Verification des attributs
	for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
	{
		attribute = cast(KWDGAttribute*, oaAttributes.GetAt(nAttribute));

		// Verification de l'attribut
		bOk = bOk and attribute->Check();
		if (not bOk)
			break;

		// On compte le nombre d'attributs tagges comme cible
		if (attribute->GetAttributeTargetFunction())
		{
			nTargetAttributeNumber++;
		}

		// Verification du lien entre l'attribut et la structure
		if (attribute->GetAttributeIndex() != nAttribute)
		{
			attribute->AddError(sTmp + "The variable index does not correspond to " +
					    "its rank in the parent structure (" + IntToString(nAttribute) + ")");
			bOk = false;
		}
		if (attribute->dataGrid != this)
		{
			attribute->AddError("Variable incorrectly linked to its parent structure");
			bOk = false;
		}

		// Rangement de l'attribut dans un dictionnaire, pour verifier son unicite
		if (odAttributes.Lookup(attribute->GetAttributeName()) != NULL)
		{
			attribute->AddError("Another variable already exists with the same name");
			bOk = false;
		}
		else
			odAttributes.SetAt(attribute->GetAttributeName(), attribute);

		// Calcul du nombre total de cellules references par l'attribut
		nAttributeCellNumber = 0;
		part = attribute->GetHeadPart();
		while (part != NULL)
		{
			nAttributeCellNumber += part->GetCellNumber();
			attribute->GetNextPart(part);
		}

		// Verification de la coherence avec le nombre total de cellules
		if (nAttributeCellNumber != nCellNumber)
		{
			attribute->AddError(
			    sTmp + "The number of cells in the variable (" + IntToString(nAttributeCellNumber) + ")" +
			    " is different with that of the data grid (" + IntToString(nCellNumber) + ")");
			bOk = false;
			break;
		}
	}

	// Verification qu'il n'y a pas plus d'un attribut cible
	if (nTargetAttributeNumber > 1)
	{
		AddError(sTmp + "There are " + IntToString(nTargetAttributeNumber) +
			 " target variables in the data grid");
		bOk = false;
	}

	// Verification que l'eventuel attribut cible est bien le dernier
	if (nTargetAttributeNumber == 1)
	{
		check(GetTargetAttribute());
		assert(GetAttributeAt(GetTargetAttribute()->GetAttributeIndex()) == GetTargetAttribute());
		if (GetTargetAttribute()->GetAttributeIndex() != GetAttributeNumber() - 1)
		{
			AddError("The target variable should be the last variable in the data grid");
			bOk = false;
		}
	}

	// Verification de l'absence d'attribut cible si necessaire
	if (nTargetAttributeNumber == 0)
	{
		if (GetTargetAttribute() != NULL)
		{
			AddError("A target variable is wrongly referenced");
			bOk = false;
		}
	}

	// Verification qu'il n'y a pas de valeurs cible en meme temps qu'un attribut cible
	if (nTargetAttributeNumber == 1 and GetTargetValueNumber() > 0)
	{
		AddError(sTmp + "There are both " + IntToString(GetTargetValueNumber()) +
			 " target values and one target variable (" + GetTargetAttribute()->GetAttributeName() +
			 ")"
			 " in the data grid");
		bOk = false;
	}

	// Verification de toutes les cellules
	if (bOk)
	{
		cell = headCell;
		while (cell != NULL)
		{
			// Validite des parties d'attribut de la cellule
			// (l'existence de la cellule est forcement verifiee)
			if (bOk)
			{
				bOk = CheckCellParts(&(cell->oaParts));
				if (not bOk)
					cell->AddError("Incorrect referenced parts");
			}

			// Verification du referencement dans les listes des parties par attribut
			if (bOk)
			{
				for (nAttribute = 0; nAttribute < GetAttributeNumber(); nAttribute++)
				{
					// Test d'existence de la cellule dans la liste de la partie
					part = cell->GetPartAt(nAttribute);
					bOk = bOk and part->CheckCell(cell);
					if (not bOk)
						break;
				}
			}
			if (not bOk)
				break;
			cell = cell->nextCell;
		}
	}

	return bOk;
}

////////////////////////////////////////////////////////////////////////////////
// Classe KWClassifierUnivariateEvaluationTask

KWClassifierUnivariateEvaluationTask::KWClassifierUnivariateEvaluationTask()
{
	classifierUnivariateEvaluation = NULL;

	// Declaration des variables partagees
	DeclareTaskOutput(&output_slaveDataGridEvaluation);
	DeclareSharedParameter(&shared_livDataGridAttributes);
	DeclareSharedParameter(&shared_classifierDataGridStats);
}

KWClassifierUnivariateEvaluationTask::~KWClassifierUnivariateEvaluationTask() {}

const ALString KWClassifierUnivariateEvaluationTask::GetTaskName() const
{
	return "Univariate classifier evaluation";
}

PLParallelTask* KWClassifierUnivariateEvaluationTask::Create() const
{
	return new KWClassifierUnivariateEvaluationTask;
}

boolean KWClassifierUnivariateEvaluationTask::ComputeResourceRequirements()
{
	boolean bOk;
	longint lMemory;
	KWDataGrid dgHelper;

	require(shared_classifierDataGridStats.GetDataGridStats() != NULL);

	// Appel a la methode ancetre
	bOk = KWClassifierEvaluationTask::ComputeResourceRequirements();

	// A partir de la grille du regresseur on estime les tailles des grilles de travail et de message
	dgHelper.ImportDataGridStats(shared_classifierDataGridStats.GetDataGridStats());
	lMemory = shared_classifierDataGridStats.GetDataGridStats()->GetUsedMemory();
	lMemory += dgHelper.GetUsedMemory();

	// On ajoute une de chacune au maitre es esclave
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->UpgradeMin(lMemory);
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->UpgradeMax(lMemory);
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMin(lMemory);
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMax(lMemory);
	return bOk;
}

boolean KWClassifierUnivariateEvaluationTask::MasterInitialize()
{
	boolean bOk;

	require(predictorEvaluation != NULL);
	require(classifierUnivariateEvaluation == NULL);

	// Appel a la methode ancetre
	bOk = KWClassifierEvaluationTask::MasterInitialize();

	// Vue de KWClassifierUnivariateEvaluation du rapport d'evaluation demandeur
	classifierUnivariateEvaluation = cast(KWClassifierUnivariateEvaluation*, predictorEvaluation);

	// Initialisation du service d'evaluation via DataGrid du maitre
	masterDataGridEvaluation.SetPredictorDataGridStats(shared_classifierDataGridStats.GetDataGridStats());
	masterDataGridEvaluation.SetDataGridAttributeLoadIndexes(
	    shared_livDataGridAttributes.GetConstLoadIndexVector());
	masterDataGridEvaluation.Initialize();
	return bOk;
}

boolean KWClassifierUnivariateEvaluationTask::MasterAggregateResults()
{
	boolean bOk;

	// Appel a la methode ancetre
	bOk = KWClassifierEvaluationTask::MasterAggregateResults();

	masterDataGridEvaluation.AddEvaluatedDataGridStats(output_slaveDataGridEvaluation.GetDataGridStats());
	return bOk;
}

boolean KWClassifierUnivariateEvaluationTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	// Appel a la methode ancetre
	bOk = KWClassifierEvaluationTask::MasterFinalize(bProcessEndedCorrectly);

	// Finalisation et trasfert de l'evaluation a l'objet du rapport
	masterDataGridEvaluation.Finalize();
	classifierUnivariateEvaluation->dgsEvaluationResults.CopyFrom(
	    masterDataGridEvaluation.GetEvaluatedDataGridStats());
	return bOk;
}

boolean KWClassifierUnivariateEvaluationTask::SlaveInitialize()
{
	boolean bOk;

	// Appel a la methode ancetre
	bOk = KWClassifierEvaluationTask::SlaveInitialize();

	slaveDataGridEvaluation.SetPredictorDataGridStats(shared_classifierDataGridStats.GetDataGridStats());
	slaveDataGridEvaluation.SetDataGridAttributeLoadIndexes(shared_livDataGridAttributes.GetConstLoadIndexVector());
	return bOk;
}

boolean KWClassifierUnivariateEvaluationTask::SlaveProcessExploitDatabase()
{
	boolean bOk;

	output_slaveDataGridEvaluation.GetDataGridStats()->DeleteAllCells();
	slaveDataGridEvaluation.Initialize();

	// Appel a la methode ancetre
	bOk = KWClassifierEvaluationTask::SlaveProcessExploitDatabase();

	slaveDataGridEvaluation.Finalize();
	output_slaveDataGridEvaluation.GetDataGridStats()->CopyFrom(
	    slaveDataGridEvaluation.GetEvaluatedDataGridStats());
	return bOk;
}

boolean KWClassifierUnivariateEvaluationTask::SlaveProcessExploitDatabaseObject(const KWObject* databaseObject)
{
	boolean bOk;

	bOk = KWClassifierEvaluationTask::SlaveProcessExploitDatabaseObject(databaseObject);

	slaveDataGridEvaluation.AddInstance(databaseObject);

	return bOk;
}

void KWClassifierUnivariateEvaluationTask::InitializePredictorSharedVariables(KWPredictor* predictor)
{
	int i;
	KWPredictorUnivariate* predictorUnivariate;
	const KWDataGridStats* predictorDataGridStats;
	KWClass* predictorClass;
	KWAttribute* predictorAttribute;

	// Appel a la methode ancetre
	KWClassifierEvaluationTask::InitializePredictorSharedVariables(predictor);

	// Vue de KWPredictorUnivariate du predicteur demandeur
	predictorUnivariate = cast(KWPredictorUnivariate*, predictor);
	predictorDataGridStats = predictorUnivariate->GetTrainDataGridStats();
	predictorClass = predictorUnivariate->GetTrainedPredictor()->GetPredictorClass();

	// Sauvegarde des indices de chargement des attributes du predicteur
	shared_livDataGridAttributes.SetSize(predictorDataGridStats->GetAttributeNumber());
	for (i = 0; i < predictorDataGridStats->GetAttributeNumber(); i++)
	{
		// Recherche de l'index de l'attribut dans le dictionnaire du predicteur
		predictorAttribute =
		    predictorClass->LookupAttribute(predictorDataGridStats->GetAttributeAt(i)->GetAttributeName());
		assert(predictorAttribute != NULL);
		assert(predictorAttribute->GetLoadIndex().IsValid());
		shared_livDataGridAttributes.SetAt(i, predictorAttribute->GetLoadIndex());
	}

	// Sauvegarde de la grille des donnees d'apprentissage du predicteur
	shared_classifierDataGridStats.SetDataGridStats(predictorUnivariate->GetTrainDataGridStats()->Clone());
}

void KWClassifierUnivariateEvaluationTask::CleanPredictorSharedVariables()
{
	shared_livDataGridAttributes.SetSize(0);
	shared_classifierDataGridStats.GetDataGridStats()->DeleteAll();
}

////////////////////////////////////////////////////////////////////////////////
// Classe KWRegressorUnivariateEvaluationTask

KWRegressorUnivariateEvaluationTask::KWRegressorUnivariateEvaluationTask()
{
	regressorUnivariateEvaluation = NULL;

	// Declaration des variables partagees
	DeclareTaskOutput(&output_slaveDataGridEvaluation);
	DeclareSharedParameter(&shared_livDataGridAttributes);
	DeclareSharedParameter(&shared_regressorDataGridStats);
}

KWRegressorUnivariateEvaluationTask::~KWRegressorUnivariateEvaluationTask() {}

const ALString KWRegressorUnivariateEvaluationTask::GetTaskName() const
{
	return "Univariate regressor evaluation";
}

PLParallelTask* KWRegressorUnivariateEvaluationTask::Create() const
{
	return new KWRegressorUnivariateEvaluationTask;
}

boolean KWRegressorUnivariateEvaluationTask::ComputeResourceRequirements()
{
	boolean bOk;
	longint lMemory;
	KWDataGrid dgHelper;

	require(shared_regressorDataGridStats.GetDataGridStats() != NULL);

	// Appel a la methode ancetre
	bOk = KWRegressorEvaluationTask::ComputeResourceRequirements();

	// A partir de la grille du regresseur on estime les tailles des grilles de travail et de message
	dgHelper.ImportDataGridStats(shared_regressorDataGridStats.GetDataGridStats());
	lMemory = 2 * shared_regressorDataGridStats.GetDataGridStats()->GetUsedMemory();
	lMemory += dgHelper.GetUsedMemory();

	// On ajoute une de chacune au maitre es esclave
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->UpgradeMin(lMemory);
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->UpgradeMax(lMemory);
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMin(lMemory);
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMax(lMemory);
	return bOk;
}

boolean KWRegressorUnivariateEvaluationTask::MasterInitialize()
{
	boolean bOk;

	require(predictorEvaluation != NULL);
	require(regressorUnivariateEvaluation == NULL);

	// Appel a la methode ancetre
	bOk = KWRegressorEvaluationTask::MasterInitialize();

	// Vue de KWRegressorUnivariateEvaluation du predicteur demandeur
	regressorUnivariateEvaluation = cast(KWRegressorUnivariateEvaluation*, predictorEvaluation);

	// Initialisation du service d'evaluation via DataGrid du maitre
	masterDataGridEvaluation.SetPredictorDataGridStats(shared_regressorDataGridStats.GetDataGridStats());
	masterDataGridEvaluation.SetDataGridAttributeLoadIndexes(
	    shared_livDataGridAttributes.GetConstLoadIndexVector());
	masterDataGridEvaluation.Initialize();
	return bOk;
}

boolean KWRegressorUnivariateEvaluationTask::MasterAggregateResults()
{
	boolean bOk;

	// Appel a la methode ancetre
	bOk = KWRegressorEvaluationTask::MasterAggregateResults();

	masterDataGridEvaluation.AddEvaluatedDataGridStats(output_slaveDataGridEvaluation.GetDataGridStats());
	return bOk;
}

boolean KWRegressorUnivariateEvaluationTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	// Appel a la methode ancetre
	bOk = KWRegressorEvaluationTask::MasterFinalize(bProcessEndedCorrectly);

	masterDataGridEvaluation.Finalize();
	regressorUnivariateEvaluation->dgsEvaluationResults.CopyFrom(
	    masterDataGridEvaluation.GetEvaluatedDataGridStats());
	return bOk;
}

boolean KWRegressorUnivariateEvaluationTask::SlaveInitialize()
{
	boolean bOk;

	// Appel a la methode ancetre
	bOk = KWRegressorEvaluationTask::SlaveInitialize();

	slaveDataGridEvaluation.SetPredictorDataGridStats(shared_regressorDataGridStats.GetDataGridStats());
	slaveDataGridEvaluation.SetDataGridAttributeLoadIndexes(shared_livDataGridAttributes.GetConstLoadIndexVector());
	return bOk;
}

boolean KWRegressorUnivariateEvaluationTask::SlaveProcessExploitDatabase()
{
	boolean bOk;

	output_slaveDataGridEvaluation.GetDataGridStats()->DeleteAllCells();
	slaveDataGridEvaluation.Initialize();

	// Appel a la methode ancetre
	bOk = KWRegressorEvaluationTask::SlaveProcessExploitDatabase();

	slaveDataGridEvaluation.Finalize();
	output_slaveDataGridEvaluation.GetDataGridStats()->CopyFrom(
	    slaveDataGridEvaluation.GetEvaluatedDataGridStats());
	return bOk;
}

boolean KWRegressorUnivariateEvaluationTask::SlaveProcessExploitDatabaseObject(const KWObject* databaseObject)
{
	boolean bOk;

	// Appel a la methode ancetre
	bOk = KWRegressorEvaluationTask::SlaveProcessExploitDatabaseObject(databaseObject);

	slaveDataGridEvaluation.AddInstance(databaseObject);
	return bOk;
}

void KWRegressorUnivariateEvaluationTask::InitializePredictorSharedVariables(KWPredictor* predictor)
{
	int i;
	KWPredictorUnivariate* predictorUnivariate;
	const KWDataGridStats* predictorDataGridStats;
	KWClass* predictorClass;
	KWAttribute* predictorAttribute;

	// Appel a la methode ancetre
	KWRegressorEvaluationTask::InitializePredictorSharedVariables(predictor);

	predictorUnivariate = cast(KWPredictorUnivariate*, predictor);
	predictorDataGridStats = predictorUnivariate->GetTrainDataGridStats();
	predictorClass = predictorUnivariate->GetTrainedPredictor()->GetPredictorClass();

	shared_livDataGridAttributes.SetSize(predictorDataGridStats->GetAttributeNumber());
	for (i = 0; i < predictorDataGridStats->GetAttributeNumber(); i++)
	{
		// Recherche de l'index de l'attribut dans le dictionnaire du predicteur
		predictorAttribute =
		    predictorClass->LookupAttribute(predictorDataGridStats->GetAttributeAt(i)->GetAttributeName());
		assert(predictorAttribute != NULL);
		assert(predictorAttribute->GetLoadIndex().IsValid());
		shared_livDataGridAttributes.SetAt(i, predictorAttribute->GetLoadIndex());
	}
	shared_regressorDataGridStats.SetDataGridStats(predictorUnivariate->GetTrainDataGridStats()->Clone());
}
