// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWAttributeSubsetStats.h"

///////////////////////////////////////////////////////
// Classe KWAttributeSubsetStats

KWAttributeSubsetStats::KWAttributeSubsetStats()
{
	classStats = NULL;

	// L'indicateur est mis a vrai des que l'attribut cible est ajoute
	// dans la liste des attributs
	bIsTargetAttributePartitioned = false;

	// Par defaut, il n'y a aucune contrainte sur le nombre max de cellules par grille
	nMaxCellNumberConstraint = 0;
}

KWAttributeSubsetStats::~KWAttributeSubsetStats()
{
	// Attention: il faut explicitement appeler cette methode dans le destructeur,
	// bien qu'elle soit appelee dans le constructeur ancetre
	CleanDataPreparationResults();
}

void KWAttributeSubsetStats::SetAttributeNumber(int nValue)
{
	require(nValue >= 0);
	svAttributeNames.SetSize(nValue);
}

int KWAttributeSubsetStats::GetAttributeNumber() const
{
	return svAttributeNames.GetSize();
}

void KWAttributeSubsetStats::SetAttributeNameAt(int nIndex, const ALString& sAttributeName)
{
	require(0 <= nIndex and nIndex < svAttributeNames.GetSize());
	require(sAttributeName != "" and GetAttributeNameAt(nIndex) == "");
	require(sAttributeName != GetTargetAttributeName());
	svAttributeNames.SetAt(nIndex, sAttributeName);
}

const ALString& KWAttributeSubsetStats::GetAttributeNameAt(int nIndex) const
{
	require(0 <= nIndex and nIndex < svAttributeNames.GetSize());
	return svAttributeNames.GetAt(nIndex);
}

void KWAttributeSubsetStats::SetTargetAttributePartitioned(boolean bValue)
{
	bIsTargetAttributePartitioned = bValue;
}

boolean KWAttributeSubsetStats::GetTargetAttributePartitioned() const
{
	return bIsTargetAttributePartitioned;
}

void KWAttributeSubsetStats::SetClassStats(KWClassStats* stats)
{
	require(stats == NULL or GetLearningSpec() == stats->GetLearningSpec());
	classStats = stats;
}

KWClassStats* KWAttributeSubsetStats::GetClassStats() const
{
	ensure(classStats == NULL or GetLearningSpec() == classStats->GetLearningSpec());
	return classStats;
}

boolean KWAttributeSubsetStats::CheckSpecifications() const
{
	boolean bOk = true;
	int i;
	ALString sAttributeName;
	KWAttribute* attribute;
	boolean bTargetAttributeUsed;

	require(Check());

	// Verification de la validite des attributs
	bTargetAttributeUsed = false;
	for (i = 0; i < GetAttributeNumber(); i++)
	{
		sAttributeName = GetAttributeNameAt(i);

		// Memorisation de l'usage de l'attribut cible
		if (sAttributeName == GetTargetAttributeName())
			bTargetAttributeUsed = true;

		// Pas de verification supplementaire sur l'attribut cible (deja traite par le LearningSpec)
		if (bTargetAttributeUsed)
			continue;

		// Recherche de l'attribut
		attribute = GetClass()->LookupAttribute(sAttributeName);

		// Test d'existence de l'attribut
		if (attribute == NULL)
		{
			bOk = false;
			AddError("Variable " + sAttributeName + " does not exist");
		}

		// Test d'utilisation de l'attribut
		if (attribute != NULL and not attribute->GetUsed())
		{
			bOk = false;
			AddError("Variable " + sAttributeName + " is not used");
		}

		// Test du type de l'attribut
		if (attribute != NULL and not KWType::IsSimple(attribute->GetType()))
		{
			bOk = false;
			AddError("Variable " + sAttributeName + " is not of numerical or categorical type");
		}

		// Test de chargement en memoire de l'attribut
		if (attribute != NULL and not attribute->GetLoaded())
		{
			bOk = false;
			AddError("Variable " + sAttributeName + " is not loaded");
		}

		// Arret si erreurs
		if (not bOk)
			break;
	}

	// L'attribut cible ne doit pas faire partie des attributs a traiter
	if (bOk)
	{
		if (bTargetAttributeUsed)
		{
			bOk = false;
			AddError("Target variable " + GetTargetAttributeName() + " is used in the subset");
		}
	}

	// Test de coherence pour le partitionnement de l'attribut cible
	assert(not(GetTargetAttributeName() == "" and GetTargetAttributePartitioned()));
	assert(not(GetTargetAttributeType() == KWType::Continuous and not GetTargetAttributePartitioned()));
	assert(not(GetAttributeNumber() > 1 and GetTargetAttributePartitioned()));
	return bOk;
}

boolean KWAttributeSubsetStats::ComputeStats(const KWTupleTable* tupleTable)
{
	KWDiscretizerSpec currentDiscretizerSpec;
	KWGrouperSpec currentGrouperSpec;
	KWDiscretizerSpec modlRefDiscretizerSpec;
	KWGrouperSpec modlRefGrouperSpec;
	KWDataGrid* dataGrid;
	KWDataGrid* optimizedDataGrid;
	KWDataGridCosts* dataGridCosts;
	KWDataGridOptimizer dataGridOptimizer;
	double dGridCost;

	require(Check());
	require(CheckSpecifications());

	// Les methodes de pretraitement supervise sont obligatoirement MODL : on memorise le parameterage avant de la
	// passer en MODL defaut L'analyse bivarie exploite en effet des pre-granularisation basees sur les
	// pretraitements MODL
	currentDiscretizerSpec.CopyFrom(learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec());
	currentGrouperSpec.CopyFrom(learningSpec->GetPreprocessingSpec()->GetGrouperSpec());
	learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetSupervisedMethodName(
	    modlRefDiscretizerSpec.GetSupervisedMethodName());
	learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->SetSupervisedMethodName(
	    modlRefDiscretizerSpec.GetUnsupervisedMethodName());
	learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetSupervisedMethodName(
	    modlRefGrouperSpec.GetSupervisedMethodName());
	learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->SetSupervisedMethodName(
	    modlRefGrouperSpec.GetUnsupervisedMethodName());

	// Nettoyage des donnees de preparation
	CleanDataPreparationResults();
	bIsStatsComputed = true;

	// Recopie du parametrage d'optimisation des grilles
	dataGridOptimizer.GetParameters()->CopyFrom(GetPreprocessingSpec()->GetDataGridOptimizerParameters());
	dataGridOptimizer.SetClassStats(GetClassStats());

	// Creation du DataGrid
	dataGrid = CreateDataGrid(tupleTable);

	// Arret si grille non creee, par exemple en raison d'une interruption utilisateur
	if (dataGrid == NULL)
		bIsStatsComputed = false;
	// On continue
	else
	{
		// Creation et initialisation de la structure de couts
		dataGridCosts = CreateDataGridCost();
		dataGridCosts->InitializeDefaultCosts(dataGrid);

		// Parametrage des couts de l'optimiseur de grille
		dataGridOptimizer.SetDataGridCosts(dataGridCosts);

		// Optimisation de la grille
		optimizedDataGrid = new KWDataGrid;
		dGridCost = dataGridOptimizer.OptimizeDataGrid(dataGrid, optimizedDataGrid);

		// Simplification eventuelle de la grille pour tenir compte de la contrainte du MaxPartNumber
		if (not TaskProgression::IsInterruptionRequested() and
		    GetPreprocessingSpec()->GetMaxPartNumber() > 0 and
		    optimizedDataGrid->ComputeMaxPartNumber() > GetPreprocessingSpec()->GetMaxPartNumber())
			dGridCost = dataGridOptimizer.SimplifyDataGrid(optimizedDataGrid);

		// Memorisation des couts MODL
		SetConstructionCost(dataGridCosts->ComputeDataGridTotalConstructionCost(optimizedDataGrid));
		SetPreparationCost(dataGridCosts->ComputeDataGridTotalPreparationCost(optimizedDataGrid));
		SetDataCost(dataGridCosts->ComputeDataGridTotalDataCost(optimizedDataGrid));

		// Evaluation de la grille par son taux de compression
		// On passe par les couts de grille, qui gerent un modele nul par ensemble d'attributs,
		// ce qui est indispensable notamment dans le cas non supervise
		assert(0 <= dGridCost and dGridCost <= dataGridCosts->GetTotalDefaultCost());
		dPreparedLevel = dataGridCosts->ComputeDataGridCompressionCoefficient(optimizedDataGrid);

		// On tronque a 0 si level trop petit
		// Avec 1e-10, on peut encore detecter des informations de niches dans des bases
		// de tres grande tailes (mots dans un corpus de texte), tout en restant loin
		// du bruit numerique (1e-15)
		if (dPreparedLevel < 1e-10)
			dPreparedLevel = 0;

		// Calcul des donnes de preparation si necessaire
		// Dans le cas supervise, on garde quand meme la grille, qui permet le recodage
		// de tous els attributs, y compris non informatifs
		if (dPreparedLevel > 0 or GetTargetAttributeType() != KWType::None)
			ComputeDataPreparationResults(optimizedDataGrid);

		// Nettoyage
		delete dataGrid;
		delete dataGridCosts;
		delete optimizedDataGrid;

		// Reinitialisation des resultats si interruption utilisateur
		if (TaskProgression::IsInterruptionRequested())
		{
			CleanDataPreparationResults();
			bIsStatsComputed = false;
		}
	}

	// Restauration des methodes de pretraitement supervise initiales
	learningSpec->GetPreprocessingSpec()->GetDiscretizerSpec()->CopyFrom(&currentDiscretizerSpec);
	learningSpec->GetPreprocessingSpec()->GetGrouperSpec()->CopyFrom(&currentGrouperSpec);
	return bIsStatsComputed;
}

const ALString KWAttributeSubsetStats::GetSortName() const
{
	ALString sSortName;
	int i;

	// Calcul d'un nom par concatenation des noms des attributs
	for (i = 0; i < GetAttributeNumber(); i++)
	{
		if (i > 0)
			sSortName += "`";
		sSortName += GetAttributeNameAt(i);
	}
	return sSortName;
}

double KWAttributeSubsetStats::GetSortValue() const
{
	require(IsStatsComputed());

	return GetLevel();
}

longint KWAttributeSubsetStats::GetUsedMemory() const
{
	longint lUsedMemory;

	lUsedMemory = KWDataPreparationStats::GetUsedMemory();
	lUsedMemory += sizeof(KWAttributeSubsetStats) - sizeof(KWDataPreparationStats);
	lUsedMemory += svAttributeNames.GetUsedMemory();
	return lUsedMemory;
}

void KWAttributeSubsetStats::WriteReport(ostream& ost)
{
	require(Check());
	require(IsStatsComputed());

	require(IsStatsComputed());

	// Evaluation
	ost << "Data Grid\n";

	// Affichage des details de la grille
	GetPreparedDataGridStats()->WritePartial(ost, true, GetTargetAttributeName() == "");
}

void KWAttributeSubsetStats::WriteHeaderLineReport(ostream& ost)
{
	require(IsStatsComputed());

	// Identification des attributs
	ost << "Names\t";

	// Evaluation
	ost << "Level\t";

	// Statistiques sur la grille
	ost << "Variables\t";
	ost << "Cells\t";
	ost << "Grid size";

	// Couts MODL
	if (GetWriteCosts())
		ost << "\tConstr. cost\tPrep. cost\tData cost";
}

void KWAttributeSubsetStats::WriteLineReport(ostream& ost)
{
	int i;

	require(IsStatsComputed());

	// Identification des attributs
	if (GetPreparedDataGridStats() != NULL)
	{
		for (i = 0; i < GetPreparedDataGridStats()->GetAttributeNumber(); i++)
		{
			if (i > 0)
				ost << "&";
			ost << TSV::Export(GetPreparedDataGridStats()->GetAttributeAt(i)->GetAttributeName());
		}
	}
	ost << "\t";

	// Evaluation
	ost << GetLevel() << "\t";

	// Statistiques sur la grille
	if (GetPreparedDataGridStats() == NULL)
		ost << "0\t0\t0";
	else
	{
		// Affichage des statistiques
		ost << GetPreparedDataGridStats()->ComputeInformativeAttributeNumber() << "\t";
		ost << GetPreparedDataGridStats()->ComputeCellNumber() << "\t";
		ost << GetPreparedDataGridStats()->ComputeGridSize();
	}

	// Couts MODL
	if (GetWriteCosts())
	{
		ost << "\t" << GetConstructionCost() << "\t" << GetPreparationCost() << "\t" << GetDataCost();
	}
}

void KWAttributeSubsetStats::WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary)
{
	ContinuousVector cvAttributeDomainLowerBounds;
	ContinuousVector cvAttributeDomainUpperBounds;
	KWDescriptiveContinuousStats* descriptiveContinuousStats;
	const KWDGSAttributePartition* dgsAttributePartition;
	KWAttributeStats* attributeStats;
	int i;
	ALString sTmp;

	require(IsStatsComputed());

	//////////////////////////////////////////////////////////////////////
	// Ecriture des champs de base
	if (bSummary)
	{
		// Identification des attributs
		// Nom de l'attribut
		fJSON->WriteKeyString("rank", GetIdentifier());
		fJSON->BeginKeyList("names");
		for (i = 0; i < GetAttributeNumber(); i++)
			fJSON->WriteString(GetAttributeNameAt(i));
		fJSON->EndList();

		// Evaluation de la paire
		fJSON->WriteKeyDouble("level", GetLevel());

		// Statistiques sur la grille
		if (GetPreparedDataGridStats() == NULL)
		{
			fJSON->WriteKeyInt("variables", 0);
			fJSON->WriteKeyInt("cells", 1);
			fJSON->WriteKeyInt("gridSize", 1);
		}
		else
		{
			// Statistiques sur la grille
			fJSON->WriteKeyInt("variables",
					   GetPreparedDataGridStats()->ComputeInformativeAttributeNumber());
			fJSON->WriteKeyInt("vells", GetPreparedDataGridStats()->ComputeCellNumber());
			fJSON->WriteKeyInt("gridSize", GetPreparedDataGridStats()->ComputeGridSize());
		}

		// Couts MODL
		if (GetWriteCosts())
		{
			fJSON->WriteKeyDouble("constructionCost", GetConstructionCost());
			fJSON->WriteKeyDouble("preparationCost", GetPreparationCost());
			fJSON->WriteKeyDouble("dataCost", GetDataCost());
		}
	}
	//////////////////////////////////////////////////////////////////////
	// Ecriture des champs detailles
	else
	{
		// Grille de preparation
		if (preparedDataGridStats != NULL)
		{
			// Collecte des valeurs min et max des attributs
			cvAttributeDomainLowerBounds.SetSize(preparedDataGridStats->GetAttributeNumber());
			cvAttributeDomainUpperBounds.SetSize(preparedDataGridStats->GetAttributeNumber());
			for (i = 0; i < preparedDataGridStats->GetAttributeNumber(); i++)
			{
				dgsAttributePartition = preparedDataGridStats->GetAttributeAt(i);
				if (dgsAttributePartition->GetAttributeType() == KWType::Continuous)
				{
					attributeStats = GetClassStats()->LookupAttributeStats(
					    dgsAttributePartition->GetAttributeName());
					check(attributeStats);
					descriptiveContinuousStats =
					    cast(KWDescriptiveContinuousStats*, attributeStats->GetDescriptiveStats());
					cvAttributeDomainLowerBounds.SetAt(i, descriptiveContinuousStats->GetMin());
					cvAttributeDomainUpperBounds.SetAt(i, descriptiveContinuousStats->GetMax());
				}
			}

			// Parametrage de la grille
			preparedDataGridStats->SetJSONAttributeDomainLowerBounds(&cvAttributeDomainLowerBounds);
			preparedDataGridStats->SetJSONAttributeDomainUpperBounds(&cvAttributeDomainUpperBounds);

			// Ecriture du rapport JSON
			preparedDataGridStats->WriteJSONKeyReport(fJSON, "dataGrid");

			// Nettoyage du parametrage
			preparedDataGridStats->SetJSONAttributeDomainLowerBounds(NULL);
			preparedDataGridStats->SetJSONAttributeDomainUpperBounds(NULL);
		}
	}
}

KWDataGridCosts* KWAttributeSubsetStats::CreateDataGridCost() const
{
	KWDataGridCosts* dataGridCosts;

	require(Check());
	require(CheckSpecifications());

	// Creation et initialisation de la structure de couts
	dataGridCosts = NULL;
	if (GetTargetAttributeName() != "" and GetInstanceNumber() > 0 and
	    GetTargetDescriptiveStats()->GetValueNumber() >= 2)
	{
		// Cas de la classification
		if (GetTargetAttributeType() == KWType::Symbol)
		{
			// Cas ou avec groupage de la cible, et ou l'attribut cible fait partie des attributs a traiter
			if (IsTargetGrouped() and bIsTargetAttributePartitioned)
				dataGridCosts = new KWDataGridGeneralizedClassificationCosts;
			// Cas sans groupage de la cible
			else
				dataGridCosts = new KWDataGridClassificationCosts;
		}
		// Cas de la regression
		else if (GetTargetAttributeType() == KWType::Continuous)
		{
			assert(bIsTargetAttributePartitioned);
			dataGridCosts = new KWDataGridRegressionCosts;
		}
	}
	// Cas non supervise
	else
		dataGridCosts = new KWDataGridClusteringCosts;

	check(dataGridCosts);
	return dataGridCosts;
}

KWDataGrid* KWAttributeSubsetStats::CreateDataGrid(const KWTupleTable* tupleTable)
{
	KWDataGridManager dataGridManager;
	const KWDGSAttributeSymbolValues* targetValues;
	KWDataGrid* dataGrid;
	int nTargetValueNumber;
	int nTargetValue;
	int nDataGridAttributeNumber;
	int nAttribute;
	KWAttribute* attribute;
	int nTupleAttributeIndex;
	KWDGAttribute* dgAttribute;
	boolean bCellCreationOk;

	require(Check());
	require(CheckSpecifications());
	require(tupleTable != NULL);
	require(tupleTable->GetSize() > 0);
	require(GetInstanceNumber() == tupleTable->GetTotalFrequency());

	// Cas de la classification supervisee sans groupement de valeurs
	nTargetValueNumber = 0;
	if (GetTargetAttributeName() != "" and not bIsTargetAttributePartitioned)
		nTargetValueNumber = GetTargetDescriptiveStats()->GetValueNumber();

	// Calcul du nombre d'attributs de la grille, en tenant compte
	// de l'attribut cible, eventuellement partitionne
	nDataGridAttributeNumber = GetAttributeNumber();
	if (GetTargetAttributeName() != "" and bIsTargetAttributePartitioned)
		nDataGridAttributeNumber++;

	// Creation du DataGrid initial
	dataGrid = new KWDataGrid;
	dataGrid->Initialize(nDataGridAttributeNumber, nTargetValueNumber);

	// Initialisation des valeurs cibles
	if (nTargetValueNumber > 0)
	{
		assert(GetTargetAttributeType() == KWType::Symbol);
		targetValues = cast(const KWDGSAttributeSymbolValues*, GetTargetValueStats()->GetAttributeAt(0));
		for (nTargetValue = 0; nTargetValue < dataGrid->GetTargetValueNumber(); nTargetValue++)
		{
			dataGrid->SetTargetValueAt(nTargetValue, targetValues->GetValueAt(nTargetValue));
		}
	}

	// Debut de suivi de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Initialize data grid");

	// Initialisation des attributs et de leurs parties
	for (nAttribute = 0; nAttribute < dataGrid->GetAttributeNumber(); nAttribute++)
	{
		// Recherche de l'index de l'attribut dans la table de tuples
		if (nAttribute < GetAttributeNumber())
			nTupleAttributeIndex = tupleTable->LookupAttributeIndex(GetAttributeNameAt(nAttribute));
		// Cas de l'attribut cible partitionne, pris en compte dans la grille
		else
			nTupleAttributeIndex = tupleTable->LookupAttributeIndex(GetTargetAttributeName());
		assert(nTupleAttributeIndex >= 0);

		// Recherche de l'attribut du DataGrid correspondant
		dgAttribute = dataGrid->GetAttributeAt(nAttribute);

		// Parametrage de l'attribut du DataGrid
		dgAttribute->SetAttributeName(tupleTable->GetAttributeNameAt(nTupleAttributeIndex));
		dgAttribute->SetAttributeType(tupleTable->GetAttributeTypeAt(nTupleAttributeIndex));
		dgAttribute->SetAttributeTargetFunction(dgAttribute->GetAttributeName() == GetTargetAttributeName());

		// Recuperation du cout de selection/construction de l'attribut hors attribut cible
		if (dgAttribute->GetAttributeName() != GetTargetAttributeName())
		{
			attribute = GetClass()->LookupAttribute(dgAttribute->GetAttributeName());
			check(attribute);
			dgAttribute->SetCost(attribute->GetCost());
		}

		// Creation des parties de l'attribut selon son type
		TaskProgression::DisplayLabel("Initialize " + dgAttribute->GetAttributeName() + " parts");
		if (dgAttribute->GetAttributeType() == KWType::Continuous)
			CreateAttributeIntervals(tupleTable, dgAttribute);
		else
			CreateAttributeValueSets(tupleTable, dgAttribute);

		// Test interruption utilisateur
		if (TaskProgression::IsInterruptionRequested())
			break;
	}

	// On force le calcul des statistiques sur les attributs informatifs
	if (not TaskProgression::IsInterruptionRequested())
	{
		dataGrid->SetCellUpdateMode(true);
		dataGrid->SetCellUpdateMode(false);
	}

	// Supression des attributs non informatifs
	// TODO MB cela fausse le nombre d'attributs initiaux
	// Cela pose de gros probleme pour gerer les grilles sans attributs
	// dataGrid->DeleteNonInformativeAttributes();

	// Creation des cellules
	bCellCreationOk = true;
	if (not TaskProgression::IsInterruptionRequested())
	{
		TaskProgression::DisplayLabel("Initialize cells");
		bCellCreationOk = CreateDataGridCells(tupleTable, dataGrid);
	}

	// Fin de suivi de tache
	TaskProgression::EndTask();

	// Destruction de la grille si interruption utilisateur
	if (TaskProgression::IsInterruptionRequested() or not bCellCreationOk)
	{
		delete dataGrid;
		dataGrid = NULL;
	}

	ensure(dataGrid == NULL or dataGrid->Check());
	return dataGrid;
}

void KWAttributeSubsetStats::HandleOptimizationStep(const KWDataGrid* optimizedDataGrid,
						    const KWDataGrid* initialGranularizedDataGrid,
						    boolean bIsLastSaving) const
{
}

boolean KWAttributeSubsetStats::GetPregranularizedNumericalAttributes()
{
	return bPregranularizedNumericalAttributes;
}

void KWAttributeSubsetStats::SetPregranularizedNumericalAttributes(boolean bValue)
{
	bPregranularizedNumericalAttributes = bValue;
}

boolean KWAttributeSubsetStats::CreateAttributeIntervals(const KWTupleTable* tupleTable, KWDGAttribute* dgAttribute)
{
	int nTuple;
	const KWTuple* tuple;
	KWTupleTable attributeTupleTable;
	Continuous cSourceValue;
	Continuous cSourceRef;
	KWDGPart* part;
	Continuous cIntervalBound;
	double dProgression;
	int nMaxValueNumber;
	int nMinValueNumber = 500;

	require(Check());
	require(CheckSpecifications());
	require(tupleTable != NULL);
	require(tupleTable->GetSize() > 0);
	require(tupleTable->LookupAttributeIndex(dgAttribute->GetAttributeName()) != -1);
	require(tupleTable->GetAttributeTypeAt(tupleTable->LookupAttributeIndex(dgAttribute->GetAttributeName())) ==
		KWType::Continuous);
	require(dgAttribute != NULL);
	require(dgAttribute->GetPartNumber() == 0);

	// Construction d'une table de tuples univariee dediee a l'attribut
	tupleTable->BuildUnivariateTupleTable(dgAttribute->GetAttributeName(), &attributeTupleTable);
	if (TaskProgression::IsInterruptionRequested())
		return false;

	// Pre-granularisation des attributs numeriques cible (regression) et des attributs numeriques explicatifs en
	// analyse non supervisee (co-clustering) Cette pre-granularisation permet :
	// - en regression, de limiter le nombre de valeurs cible au seuil sqrt(N log N), superieur a l'ecart type
	// theorique de la precision de prediction en sqrt(N)
	// - en coclustering

	// Calcul du nombre maximal de valeurs tolere en fonction du nombre d'instances
	if (attributeTupleTable.GetTotalFrequency() < nMinValueNumber)
		nMaxValueNumber = attributeTupleTable.GetTotalFrequency();
	else
		nMaxValueNumber =
		    nMinValueNumber +
		    (int)ceil(sqrt((attributeTupleTable.GetTotalFrequency() - nMinValueNumber) *
				   log((attributeTupleTable.GetTotalFrequency() - nMinValueNumber)) / log(2.0)));

	// Cas de la pre-granularisation de l'attribut
	if (GetPregranularizedNumericalAttributes() and attributeTupleTable.GetSize() > nMaxValueNumber and
	    (dgAttribute->GetAttributeTargetFunction() or GetTargetAttributeName() == ""))
		CreateAttributePreGranularizedIntervals(&attributeTupleTable, dgAttribute, nMaxValueNumber);
	// Sinon
	else
	{
		// Creation d'une premiere partie, avec sa borne inf (contenant la valeur manquante)
		part = dgAttribute->AddPart();
		part->GetInterval()->SetLowerBound(KWDGInterval::GetMinLowerBound());
		part->GetInterval()->SetUpperBound(KWDGInterval::GetMaxUpperBound());

		// Creation des parties de l'attribut pour chaque tuple
		cSourceRef = KWDGInterval::GetMinLowerBound();

		for (nTuple = 0; nTuple < attributeTupleTable.GetSize(); nTuple++)
		{
			tuple = attributeTupleTable.GetAt(nTuple);

			// Progression
			if (TaskProgression::IsRefreshNecessary(nTuple))
			{
				// Cas d'un attribut de grille, qui n'est pas un attribut interne d'un attribut VarPart
				if (not dgAttribute->IsInnerAttribute())
				{
					// Avancement: au prorata de la base pour l'attribut en cours, en reservant 50
					// pour la creation des cellules
					dProgression = dgAttribute->GetAttributeIndex() * 50.0 /
						       dgAttribute->GetDataGrid()->GetAttributeNumber();
					dProgression += (nTuple * 50.0 / attributeTupleTable.GetSize()) /
							dgAttribute->GetDataGrid()->GetAttributeNumber();
					TaskProgression::DisplayProgression((int)dProgression);
					if (TaskProgression::IsInterruptionRequested())
						return false;
				}
			}

			// Valeur du tuple
			cSourceValue = tuple->GetContinuousAt(0);

			// Memorisation de la valeur de reference initiale pour le premier tuple
			if (nTuple == 0)
				cSourceRef = cSourceValue;
			// Creation d'un nouvel intervalle sinon
			else
			{
				assert(cSourceValue > cSourceRef);

				// Calcul de la borne sup de l'intervalle courant, comme moyenne de la valeur
				// des deux objets de part et d'autre de l'intervalle
				cIntervalBound = KWContinuous::GetHumanReadableLowerMeanValue(cSourceRef, cSourceValue);

				// Memorisation de la borne sup de l'intervalle en court
				part->GetInterval()->SetUpperBound(cIntervalBound);

				// Creation d'une nouvelle partie avec sa borne inf
				part = dgAttribute->AddPart();
				part->GetInterval()->SetLowerBound(cIntervalBound);
				part->GetInterval()->SetUpperBound(KWDGInterval::GetMaxUpperBound());

				// Nouvelle valeur de reference
				cSourceRef = cSourceValue;
			}
		}
		// Parametrage du nombre total de valeurs (= nombre d'instances)
		dgAttribute->SetInitialValueNumber(attributeTupleTable.GetTotalFrequency());
		dgAttribute->SetGranularizedValueNumber(attributeTupleTable.GetTotalFrequency());
		assert(dgAttribute->GetPartNumber() == attributeTupleTable.GetSize() or
		       GetPregranularizedNumericalAttributes());
		assert(dgAttribute->GetInitialValueNumber() + 1 >= dgAttribute->GetPartNumber());
		ensure(dgAttribute->Check());
	}

	return true;
}

boolean KWAttributeSubsetStats::CreateAttributeValueSets(const KWTupleTable* tupleTable, KWDGAttribute* dgAttribute)
{
	int nTuple;
	const KWTuple* tuple;
	KWTupleTable attributeTupleTable;
	KWDGPart* part;
	KWDGValue* value;
	double dProgression;

	require(Check());
	require(CheckSpecifications());
	require(tupleTable != NULL);
	require(tupleTable->GetSize() > 0);
	require(tupleTable->LookupAttributeIndex(dgAttribute->GetAttributeName()) != -1);
	require(tupleTable->GetAttributeTypeAt(tupleTable->LookupAttributeIndex(dgAttribute->GetAttributeName())) ==
		KWType::Symbol);
	require(dgAttribute != NULL);
	require(dgAttribute->GetPartNumber() == 0);

	// Construction d'une table de tuples univariee dediee a l'attribut
	tupleTable->BuildUnivariateTupleTable(dgAttribute->GetAttributeName(), &attributeTupleTable);
	if (TaskProgression::IsInterruptionRequested())
		return false;

	// Tri des tuple par effectif decroissant, puis valeurs croissantes
	attributeTupleTable.SortByDecreasingFrequencies();

	// Creation des parties mono-valeurs de l'attribut pour chaque tuple
	// La modalite speciale est affectee a la valeur la moins frequente arrivee en premier
	for (nTuple = 0; nTuple < attributeTupleTable.GetSize(); nTuple++)
	{
		tuple = attributeTupleTable.GetAt(nTuple);

		// Verifications de coherence
		assert(nTuple == 0 or attributeTupleTable.GetAt(nTuple - 1)->GetFrequency() > tuple->GetFrequency() or
		       attributeTupleTable.GetAt(nTuple - 1)->GetSymbolAt(0).CompareValue(tuple->GetSymbolAt(0)) < 0);

		// Progression
		if (TaskProgression::IsRefreshNecessary(nTuple))
		{
			// Cas d'un attribut de grille, qui n'est pas un attribut interne d'un attribut VarPart
			if (not dgAttribute->IsInnerAttribute())
			{
				// Avancement: au prorata de la base pour l'attribut en cours, en reservant 50 pour la
				// creation des cellules
				dProgression = dgAttribute->GetAttributeIndex() * 50.0 /
					       dgAttribute->GetDataGrid()->GetAttributeNumber();
				dProgression += (nTuple * 50.0 / attributeTupleTable.GetSize()) /
						dgAttribute->GetDataGrid()->GetAttributeNumber();
				TaskProgression::DisplayProgression((int)dProgression);
				if (TaskProgression::IsInterruptionRequested())
					return false;
			}
		}

		// Creation d'une nouvelle partie mono-valeur
		part = dgAttribute->AddPart();
		value = part->GetSymbolValueSet()->AddSymbolValue(tuple->GetSymbolAt(0));
		value->SetValueFrequency(tuple->GetFrequency());
	}
	assert(attributeTupleTable.GetSize() == dgAttribute->GetPartNumber());
	assert(dgAttribute->GetPartNumber() > 0);

	// Initialisation de la partie par defaut, contenant la modalite speciale
	// Compte-tenu du tri prealable des tuples, il s'agit de la derniere partie de l'attribut
	dgAttribute->GetTailPart()->GetSymbolValueSet()->AddSymbolValue(Symbol::GetStarValue());

	// Parametrage du nombre total de valeurs
	// Pour un attribut categoriel, l'InitialValueNumber ne contient plus la StarValue
	dgAttribute->SetInitialValueNumber(dgAttribute->GetPartNumber());
	// On ne prend pas en compte la StarValue dans Vg
	dgAttribute->SetGranularizedValueNumber(dgAttribute->GetPartNumber());
	ensure(dgAttribute->Check());
	return true;
}

boolean KWAttributeSubsetStats::CreateAttributePreGranularizedIntervals(const KWTupleTable* attributeTupleTable,
									KWDGAttribute* dgAttribute, int nMaxValueNumber)
{
	int nTuple;
	const KWTuple* tuple;
	KWQuantileIntervalBuilder quantileIntervalBuilder;
	IntVector ivFrequencies;
	int nActualPartileNumber;
	int nPartileIndex;
	Continuous cSourceValue;
	Continuous cSourceRef;
	KWDGPart* part;
	Continuous cIntervalBound;

	// Extraction des effectifs
	for (nTuple = 0; nTuple < attributeTupleTable->GetSize(); nTuple++)
	{
		tuple = attributeTupleTable->GetAt(nTuple);
		ivFrequencies.Add(tuple->GetFrequency());
	}

	// Initialisation du quantileBuilder
	quantileIntervalBuilder.InitializeFrequencies(&ivFrequencies);

	// Granularisation avec un nombre de partiles egal a la valeur seuil nMaxValueNumber
	quantileIntervalBuilder.ComputeQuantiles(nMaxValueNumber);

	// Initialisation du nombre effectif de partiles (peut etre inferieur au nombre theorique du fait de doublons)
	nActualPartileNumber = quantileIntervalBuilder.GetIntervalNumber();

	// Creation d'une premiere partie, avec sa borne inf (contenant la valeur manquante)
	part = dgAttribute->AddPart();
	part->GetInterval()->SetLowerBound(KWDGInterval::GetMinLowerBound());
	part->GetInterval()->SetUpperBound(KWDGInterval::GetMaxUpperBound());

	// Creation des parties de l'attribut pour chaque tuple
	cSourceRef = KWDGInterval::GetMinLowerBound();

	// Creation des partiles
	for (nPartileIndex = 0; nPartileIndex < nActualPartileNumber; nPartileIndex++)
	{
		// Extraction de la valeur sup du partile courant
		tuple = attributeTupleTable->GetAt(quantileIntervalBuilder.GetIntervalLastValueIndexAt(nPartileIndex));
		cSourceRef = tuple->GetContinuousAt(0);

		// Cas ou il reste un partile a traiter
		if (nPartileIndex + 1 < nActualPartileNumber)
		{
			// Extraction de la valeur inf du partile suivant
			cSourceValue =
			    attributeTupleTable
				->GetAt(quantileIntervalBuilder.GetIntervalFirstValueIndexAt(nPartileIndex + 1))
				->GetContinuousAt(0);

			assert(cSourceValue > cSourceRef);

			// Calcul de la borne sup de l'intervalle courant, comme moyenne de la valeur
			// des deux objets de part et d'autre de l'intervalle
			cIntervalBound = KWContinuous::GetHumanReadableLowerMeanValue(cSourceRef, cSourceValue);

			// Memorisation de la borne sup de l'intervalle en court
			part->GetInterval()->SetUpperBound(cIntervalBound);

			// Creation d'une nouvelle partie avec sa borne inf
			part = dgAttribute->AddPart();
			part->GetInterval()->SetLowerBound(cIntervalBound);
			part->GetInterval()->SetUpperBound(KWDGInterval::GetMaxUpperBound());
		}
	}

	// Parametrage du nombre total de valeurs (= nombre d'instances) et verifications
	dgAttribute->SetInitialValueNumber(attributeTupleTable->GetTotalFrequency());
	dgAttribute->SetGranularizedValueNumber(attributeTupleTable->GetTotalFrequency());
	assert(dgAttribute->GetInitialValueNumber() + 1 >= dgAttribute->GetPartNumber());
	ensure(dgAttribute->Check());

	return true;
}

boolean KWAttributeSubsetStats::CreateDataGridCells(const KWTupleTable* tupleTable, KWDataGrid* dataGrid)
{
	boolean bOk = true;
	boolean bDisplayInstanceCreation = false;
	ObjectArray oaParts;
	int nTuple;
	const KWTuple* tuple;
	Continuous cValue;
	Symbol sValue;
	Symbol sTargetValue;
	int nTargetIndex;
	KWDGPart* part;
	IntVector ivAttributeIndexes;
	int nTargetAttributeIndex;
	int nAttribute;
	KWDGAttribute* dgAttribute;
	KWDGCell* cell;
	int nCellFrequency;
	ALString sTmp;

	require(Check());
	require(CheckSpecifications());
	require(tupleTable != NULL);
	require(tupleTable->GetSize() > 0);
	require(dataGrid != NULL);
	require(dataGrid->GetAttributeNumber() <= tupleTable->GetAttributeNumber());
	require(dataGrid->GetCellNumber() == 0);
	require(dataGrid->Check());
	require(not dataGrid->IsVarPartDataGrid());

	// Passage en mode update
	Global::ActivateErrorFlowControl();
	dataGrid->SetCellUpdateMode(true);
	dataGrid->BuildIndexingStructure();

	// Recherche des index des attributs dans la table de tuples contenant les donnees
	ivAttributeIndexes.SetSize(dataGrid->GetAttributeNumber());
	nTargetIndex = 0;
	for (nAttribute = 0; nAttribute < dataGrid->GetAttributeNumber(); nAttribute++)
	{
		dgAttribute = dataGrid->GetAttributeAt(nAttribute);

		// Memorisation de son index
		ivAttributeIndexes.SetAt(nAttribute, tupleTable->LookupAttributeIndex(dgAttribute->GetAttributeName()));
		assert(ivAttributeIndexes.GetAt(nAttribute) != -1);
	}

	// Recherche de l'index de l'attribut cible
	nTargetAttributeIndex = -1;
	if (dataGrid->GetTargetValueNumber() > 0)
		nTargetAttributeIndex = tupleTable->LookupAttributeIndex(GetTargetAttributeName());
	assert(dataGrid->GetTargetValueNumber() == 0 or nTargetAttributeIndex >= 0);

	// Ajout d'instances dans le DataGrid
	oaParts.SetSize(dataGrid->GetAttributeNumber());
	for (nTuple = 0; nTuple < tupleTable->GetSize(); nTuple++)
	{
		tuple = tupleTable->GetAt(nTuple);

		// Progression
		if (TaskProgression::IsRefreshNecessary(nTuple))
		{
			TaskProgression::DisplayProgression((int)(50 + nTuple * 50.0 / tupleTable->GetSize()));
			if (TaskProgression::IsInterruptionRequested())
				break;
		}

		// Recherche de l'effectif de la cellule
		nCellFrequency = tuple->GetFrequency();

		// Recherche des parties pour les valeurs de l'objet courant
		for (nAttribute = 0; nAttribute < dataGrid->GetAttributeNumber(); nAttribute++)
		{
			dgAttribute = dataGrid->GetAttributeAt(nAttribute);

			// Recherche de la partie associee a la valeur selon son type
			if (dgAttribute->GetAttributeType() == KWType::Continuous)
			{
				cValue = tuple->GetContinuousAt(ivAttributeIndexes.GetAt(nAttribute));
				part = dgAttribute->LookupContinuousPart(cValue);
				oaParts.SetAt(nAttribute, part);
				if (bDisplayInstanceCreation)
					cout << cValue << "\t";
			}
			else
			{
				sValue = tuple->GetSymbolAt(ivAttributeIndexes.GetAt(nAttribute));
				part = dgAttribute->LookupSymbolPart(sValue);
				oaParts.SetAt(nAttribute, part);
				if (bDisplayInstanceCreation)
					cout << sValue << "\t";
			}
		}

		// Recherche de l'index de la classe cible
		if (dataGrid->GetTargetValueNumber() > 0)
		{
			sTargetValue = tuple->GetSymbolAt(nTargetAttributeIndex);
			nTargetIndex = GetTargetValueStats()->GetAttributeAt(0)->ComputeSymbolPartIndex(sTargetValue);
			if (bDisplayInstanceCreation)
				cout << dataGrid->GetTargetValueAt(nTargetIndex) << "\t";
		}

		// Recherche de la cellule
		cell = dataGrid->LookupCell(&oaParts);

		// Creation si necessaire, en tenant compte si demande d'une contrainet sur le nombre max de cellules
		if (cell == NULL)
		{
			// On cree une cellule si l'on ne depasse pas le nombre max
			if (nMaxCellNumberConstraint == 0 or dataGrid->GetCellNumber() < nMaxCellNumberConstraint)
				cell = dataGrid->AddCell(&oaParts);
			// Sinon, on arrete la creation (sauf en mode memoire "risque")
			else
			{
				// Message d'erreur si limite atteinte
				if (dataGrid->GetCellNumber() == nMaxCellNumberConstraint)
				{
					AddError(sTmp +
						 "Not enough memory to create initial data grid, too many data grid "
						 "cells have been created (" +
						 IntToString(nMaxCellNumberConstraint) + ") and " +
						 IntToString((int)ceil((tupleTable->GetSize() - nTuple) * 100.0 /
								       tupleTable->GetSize())) +
						 "% of the database remains to analyse");
					AddMessage(RMResourceManager::BuildMemoryLimitMessage());
				}

				// Creation en mode risque
				if (RMResourceConstraints::GetIgnoreMemoryLimit())
				{
					// Message d'avertissement la premiere fois
					if (dataGrid->GetCellNumber() == nMaxCellNumberConstraint)
						RMResourceManager::DisplayIgnoreMemoryLimitMessage();

					// Creation de la cellule
					cell = dataGrid->AddCell(&oaParts);
				}
				// Sinon, on arrete
				else
				{
					bOk = false;
					break;
				}
			}
		}

		// Mise a jour des effectifs de la classe cible
		if (dataGrid->GetTargetValueNumber() > 0)
			cell->UpgradeTargetFrequencyAt(nTargetIndex, nCellFrequency);
		// ou de la cellule directement si pas d'attribut cible
		else
			cell->SetCellFrequency(cell->GetCellFrequency() + nCellFrequency);

		// Affichage de la cellule
		if (bDisplayInstanceCreation)
			cout << *cell;
	}

	// Fin du mode update
	dataGrid->SetCellUpdateMode(false);
	dataGrid->DeleteIndexingStructure();
	Global::DesactivateErrorFlowControl();
	return bOk;
}

void KWAttributeSubsetStats::ComputeDataPreparationResults(const KWDataGrid* dataGrid)
{
	// Creation de la grille de preparation
	require(preparedDataGridStats == NULL);
	preparedDataGridStats = new KWDataGridStats;
	dataGrid->ExportDataGridStats(preparedDataGridStats);
	preparedDataGridStats->SetMainTargetModalityIndex(GetMainTargetModalityIndex());
}

boolean KWAttributeSubsetStats::bPregranularizedNumericalAttributes = true;

///////////////////////////////////////////////////////
// Classe KWAttributePairStats

KWAttributePairStats::KWAttributePairStats()
{
	SetAttributeNumber(2);
}

KWAttributePairStats::~KWAttributePairStats() {}

void KWAttributePairStats::SetAttributeName1(const ALString& sAttributeName)
{
	require(GetAttributeNumber() == 2);
	SetAttributeNameAt(0, sAttributeName);
}

const ALString& KWAttributePairStats::GetAttributeName1() const
{
	require(GetAttributeNumber() == 2);
	return GetAttributeNameAt(0);
}

void KWAttributePairStats::SetAttributeName2(const ALString& sAttributeName)
{
	require(GetAttributeNumber() == 2);
	SetAttributeNameAt(1, sAttributeName);
}

const ALString& KWAttributePairStats::GetAttributeName2() const
{
	require(GetAttributeNumber() == 2);
	return GetAttributeNameAt(1);
}

boolean KWAttributePairStats::ComputeStats(const KWTupleTable* tupleTable)
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWAttributeSubsetStats::ComputeStats(tupleTable);

	// Specialisation de la methode dans le cas non supervise,
	// pour "nettoyer" les grilles non informatives
	// Il s'agit d'un "patch" pour corriger, dans les rapports uniquement,
	// le probleme du prior multinomial pour les variables categorielle qui
	// pourrait etre plus parcimonieux avec un prior hierarchique
	// (cf. etude sur le prior multinomial hierarchique)
	// On peut ainsi avoir une grille non supervisee bivariee qui fait intervenir
	// un seul attribut categoriel ayant deux partie, donc un apparence de correlation
	if (bOk and GetTargetAttributeName() == "")
	{
		if (preparedDataGridStats != NULL and preparedDataGridStats->ComputeInformativeAttributeNumber() < 2)
		{
			// Nettoyage de la grille
			delete preparedDataGridStats;
			preparedDataGridStats = NULL;

			// Mise a 0 du level
			// Remarque: on ne corrige pas le cout de preparation du rapport,
			// ce qui n'est pas tres coherent, mais n'est pas tres grave
			dPreparedLevel = 0;
		}
	}
	return bOk;
}

KWDataGridCosts* KWAttributePairStats::CreateDataGridCost() const
{
	KWDataGridCosts* dataGridCosts;
	KWDataGridClassificationCosts dataGridClassificationCost;

	// Appele de la methode ancetre
	dataGridCosts = KWAttributeSubsetStats::CreateDataGridCost();

	// Specialisation dans la cas de la classification supervisee
	// pour penaliser les pares de variables selon le nombre de famille de constructions de variables en cours
	if (dataGridCosts != NULL and dataGridCosts->GetClassLabel() == dataGridClassificationCost.GetClassLabel())
	{
		// Specification du cout de selection de la famille de modeles
		// C'est le seul cas ou les paires de variables sont une famille de construction, au meme titre
		// que la les arbres
		// Dans les autres cas, la grille prend en compte le BasicSelectionCost dans
		// le cout de construction de la variable
		dataGridCosts->SetModelFamilySelectionCost(GetLearningSpec()->GetSelectionCost());
	}
	check(dataGridCosts);
	return dataGridCosts;
}

double KWAttributePairStats::GetDeltaLevel() const
{
	double dDeltaLevel;
	KWAttributeStats* attributeStats;
	int i;

	require(GetAttributeNumber() == 2);
	require(IsStatsComputed());

	dDeltaLevel = GetLevel();
	if (GetTargetAttributeName() != "")
	{
		for (i = 0; i < GetAttributeNumber(); i++)
		{
			// Recherche des statistiques univariees
			attributeStats = GetClassStats()->LookupAttributeStats(GetAttributeNameAt(i));

			// Affichage du niveau
			if (attributeStats != NULL)
				dDeltaLevel -= attributeStats->GetSortValue();
		}
	}
	return dDeltaLevel;
}

double KWAttributePairStats::GetSortValue() const
{
	require(IsStatsComputed());
	return GetDeltaLevel();
}

void KWAttributePairStats::WriteReport(ostream& ost)
{
	require(GetAttributeNumber() == 2);
	require(IsStatsComputed());

	// Entete
	ost << "Rank\t" << GetIdentifier() << "\n";

	// Affichage des details de la grille
	GetPreparedDataGridStats()->WritePartial(ost, true, GetTargetAttributeName() == "");
}

boolean KWAttributePairStats::IsReported() const
{
	// Si on a une vrai structure bivariee, on affiche le rapport detaille
	if (GetLevel() > 0 and preparedDataGridStats != NULL and
	    preparedDataGridStats->ComputeInformativeAttributeNumber() == 2)
		return true;
	// Sinon, on l'affiche dans le cas ou la paire est plus informative que chaque attribut individuel
	// (cela peut arriver, cas les algorithmes pour les paires sont plus pousses qu'en univarie)
	else if (GetDeltaLevel() > 0 and preparedDataGridStats != NULL)
		return true;
	// Sinon: pas d'affichage detaille
	else
		return false;
}

void KWAttributePairStats::WriteHeaderLineReport(ostream& ost)
{
	int i;

	require(GetAttributeNumber() == 2);
	require(IsStatsComputed());

	// Identification des attributs
	ost << "Rank";
	for (i = 0; i < GetAttributeNumber(); i++)
	{
		ost << "\t"
		    << "Name " << i + 1;
	}

	// Evaluation de l'apport relatif de la paire
	if (GetTargetAttributeName() != "")
		ost << "\tDeltaLevel";

	// Evaluation de la paire
	ost << "\tLevel";

	// Rappel du niveau individuel de chaque variable dans le cas supervise
	if (GetTargetAttributeName() != "")
	{
		for (i = 0; i < GetAttributeNumber(); i++)
			ost << "\tLevel " << i + 1;
	}

	// Nombre de variables informatives
	ost << "\tVariables";

	// Nombre de parties des variables
	for (i = 0; i < GetAttributeNumber(); i++)
		ost << "\tParts " << i + 1;

	// Nombre de cellules
	ost << "\tCells";

	// Couts MODL
	if (GetWriteCosts())
		ost << "\tConstr. cost\tPrep. cost\tData cost";
}

void KWAttributePairStats::WriteLineReport(ostream& ost)
{
	int i;
	const KWDGSAttributePartition* dgsAttributePartition;
	KWAttributeStats* attributeStats;

	require(GetAttributeNumber() == 2);
	require(IsStatsComputed());
	require(GetClassStats() != NULL);

	// Identification des attributs
	ost << GetIdentifier();
	for (i = 0; i < GetAttributeNumber(); i++)
	{
		ost << "\t" << TSV::Export(GetAttributeNameAt(i));
	}

	// Evaluation de l'apport relatif de la paire
	if (GetTargetAttributeName() != "")
		ost << "\t" << GetDeltaLevel();

	// Evaluation de la paire
	ost << "\t" << GetLevel();

	// Rappel du niveau individuel de chaque variable dans le cas supervise
	if (GetTargetAttributeName() != "")
	{
		for (i = 0; i < GetAttributeNumber(); i++)
		{
			// Recherche des statistiques univariees
			attributeStats = NULL;
			attributeStats = GetClassStats()->LookupAttributeStats(GetAttributeNameAt(i));

			// Affichage du niveau
			if (attributeStats == NULL)
				ost << "\t0";
			else
				ost << "\t" << attributeStats->GetLevel();
		}
	}

	// Statistiques sur la grille
	if (GetPreparedDataGridStats() == NULL)
	{
		ost << "\t0";
		for (i = 0; i < GetAttributeNumber(); i++)
			ost << "\t1";
		ost << "\t1";
	}
	else
	{
		// Nombre de variables informatives
		ost << "\t" << GetPreparedDataGridStats()->ComputeInformativeAttributeNumber();

		// Nombre de parties des variables
		for (i = 0; i < GetAttributeNumber(); i++)
		{
			dgsAttributePartition = GetPreparedDataGridStats()->SearchAttribute(GetAttributeNameAt(i));
			if (dgsAttributePartition == NULL)
				ost << "\t" << 1;
			else
				ost << "\t" << dgsAttributePartition->GetPartNumber();
		}

		// Nombre de cellules de la grille
		ost << "\t" << GetPreparedDataGridStats()->ComputeCellNumber();
	}

	// Couts MODL
	if (GetWriteCosts())
	{
		ost << "\t" << GetConstructionCost() << "\t" << GetPreparationCost() << "\t" << GetDataCost();
	}
}

void KWAttributePairStats::WriteJSONArrayFields(JSONFile* fJSON, boolean bSummary)
{
	ContinuousVector cvAttributeMinValues;
	ContinuousVector cvAttributeMaxValues;
	KWDescriptiveContinuousStats* descriptiveContinuousStats;
	const KWDGSAttributePartition* dgsAttributePartition;
	KWAttributeStats* attributeStats;
	int i;
	ALString sTmp;

	require(IsStatsComputed());

	//////////////////////////////////////////////////////////////////////
	// Ecriture des champs de base
	if (bSummary)
	{
		// Identification des attributs
		// Nom de l'attribut
		fJSON->WriteKeyString("rank", GetIdentifier());
		for (i = 0; i < GetAttributeNumber(); i++)
		{
			fJSON->WriteKeyString(sTmp + "name" + IntToString(i + 1), GetAttributeNameAt(i));
		}

		// Evaluation de l'apport relatif de la paire
		if (GetTargetAttributeName() != "")
			fJSON->WriteKeyDouble("deltaLevel", GetDeltaLevel());

		// Evaluation de la paire
		fJSON->WriteKeyDouble("level", GetLevel());

		// Rappel du niveau individuel de chaque variable dans le cas supervise
		if (GetTargetAttributeName() != "")
		{
			for (i = 0; i < GetAttributeNumber(); i++)
			{
				// Recherche des statistiques univariees
				attributeStats = GetClassStats()->LookupAttributeStats(GetAttributeNameAt(i));

				// Affichage du niveau
				if (attributeStats == NULL)
					fJSON->WriteKeyDouble(sTmp + "level" + IntToString(i + 1), 0);
				else
					fJSON->WriteKeyDouble(sTmp + "level" + IntToString(i + 1),
							      attributeStats->GetLevel());
			}
		}

		// Statistiques sur la grille
		if (GetPreparedDataGridStats() == NULL)
		{
			fJSON->WriteKeyInt("variables", 0);
			for (i = 0; i < GetAttributeNumber(); i++)
				fJSON->WriteKeyInt(sTmp + "parts" + IntToString(i + 1), 1);
			fJSON->WriteKeyInt("cells", 1);
		}
		else
		{
			// Nombre de variables informatives
			fJSON->WriteKeyInt("variables",
					   GetPreparedDataGridStats()->ComputeInformativeAttributeNumber());

			// Nombre de parties des variables
			for (i = 0; i < GetAttributeNumber(); i++)
			{
				dgsAttributePartition =
				    GetPreparedDataGridStats()->SearchAttribute(GetAttributeNameAt(i));
				if (dgsAttributePartition == NULL)
					fJSON->WriteKeyInt(sTmp + "parts" + IntToString(i + 1), 1);
				else
					fJSON->WriteKeyInt(sTmp + "parts" + IntToString(i + 1),
							   dgsAttributePartition->GetPartNumber());
			}

			// Nombre de cellules de la grille
			fJSON->WriteKeyInt("cells", GetPreparedDataGridStats()->ComputeCellNumber());
		}

		// Couts MODL
		if (GetWriteCosts())
		{
			fJSON->WriteKeyDouble("constructionCost", GetConstructionCost());
			fJSON->WriteKeyDouble("preparationCost", GetPreparationCost());
			fJSON->WriteKeyDouble("dataCost", GetDataCost());
		}
	}
	//////////////////////////////////////////////////////////////////////
	// Ecriture des champs detailles
	else
	{
		// Grille de preparation
		if (preparedDataGridStats != NULL)
		{
			// Collecte des valeurs min et max des attributs
			cvAttributeMinValues.SetSize(preparedDataGridStats->GetAttributeNumber());
			cvAttributeMaxValues.SetSize(preparedDataGridStats->GetAttributeNumber());
			for (i = 0; i < preparedDataGridStats->GetAttributeNumber(); i++)
			{
				dgsAttributePartition = preparedDataGridStats->GetAttributeAt(i);
				if (dgsAttributePartition->GetAttributeType() == KWType::Continuous)
				{
					attributeStats = GetClassStats()->LookupAttributeStats(
					    dgsAttributePartition->GetAttributeName());
					check(attributeStats);
					descriptiveContinuousStats =
					    cast(KWDescriptiveContinuousStats*, attributeStats->GetDescriptiveStats());
					cvAttributeMinValues.SetAt(i, descriptiveContinuousStats->GetMin());
					cvAttributeMaxValues.SetAt(i, descriptiveContinuousStats->GetMax());
				}
			}

			// Parametrage de la grille
			preparedDataGridStats->SetJSONAttributeDomainLowerBounds(&cvAttributeMinValues);
			preparedDataGridStats->SetJSONAttributeDomainUpperBounds(&cvAttributeMaxValues);

			// Ecriture du rapport JSON
			preparedDataGridStats->WriteJSONKeyReport(fJSON, "dataGrid");

			// Nettoyage du parametrage
			preparedDataGridStats->SetJSONAttributeDomainLowerBounds(NULL);
			preparedDataGridStats->SetJSONAttributeDomainUpperBounds(NULL);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// Classe PLShared_AttributeSubsetStats

PLShared_AttributeSubsetStats::PLShared_AttributeSubsetStats() {}

PLShared_AttributeSubsetStats::~PLShared_AttributeSubsetStats() {}

void PLShared_AttributeSubsetStats::SetAttributeSubset(KWAttributeSubsetStats* attributeSubsetStats)
{
	require(attributeSubsetStats != NULL);
	SetObject(attributeSubsetStats);
}

KWAttributeSubsetStats* PLShared_AttributeSubsetStats::GetAttributeSubset()
{
	return cast(KWAttributeSubsetStats*, GetObject());
}

void PLShared_AttributeSubsetStats::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWAttributeSubsetStats* attributeSubsetStats;

	require(serializer->IsOpenForWrite());

	// Appel de la methode ancetre
	PLShared_DataPreparationStats::SerializeObject(serializer, o);

	// Serialisation des attributs specifiques
	attributeSubsetStats = cast(KWAttributeSubsetStats*, o);

	// Serialisation des attributs (sauf classStats et oaDataGridStats)
	serializer->PutStringVector(&attributeSubsetStats->svAttributeNames);
	serializer->PutBoolean(attributeSubsetStats->GetTargetAttributePartitioned());
	serializer->PutInt(attributeSubsetStats->nMaxCellNumberConstraint);
}

void PLShared_AttributeSubsetStats::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWAttributeSubsetStats* attributeSubsetStats;

	require(serializer->IsOpenForRead());

	// Appel de la methode ancetre
	PLShared_DataPreparationStats::DeserializeObject(serializer, o);

	// Deserialisation des attributs specifiques
	attributeSubsetStats = cast(KWAttributeSubsetStats*, o);
	serializer->GetStringVector(&attributeSubsetStats->svAttributeNames);
	attributeSubsetStats->SetTargetAttributePartitioned(serializer->GetBoolean());
	attributeSubsetStats->nMaxCellNumberConstraint = serializer->GetInt();
}

Object* PLShared_AttributeSubsetStats::Create() const
{
	return new KWAttributeSubsetStats;
}

//////////////////////////////////////////////////////////////////////////////
// Classe PLShared_AttributePairStats

PLShared_AttributePairStats::PLShared_AttributePairStats() {}

PLShared_AttributePairStats::~PLShared_AttributePairStats() {}

void PLShared_AttributePairStats::SetAttributePair(KWAttributePairStats* attributeSubsetStats)
{
	require(attributeSubsetStats != NULL);
	SetObject(attributeSubsetStats);
}

KWAttributePairStats* PLShared_AttributePairStats::GetAttributePair()
{
	return cast(KWAttributePairStats*, GetObject());
}

void PLShared_AttributePairStats::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	// Appel de la methode ancetre
	PLShared_AttributeSubsetStats::SerializeObject(serializer, o);
}

void PLShared_AttributePairStats::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	// Appel de la methode ancetre
	PLShared_AttributeSubsetStats::DeserializeObject(serializer, o);
}

Object* PLShared_AttributePairStats::Create() const
{
	return new KWAttributePairStats;
}
