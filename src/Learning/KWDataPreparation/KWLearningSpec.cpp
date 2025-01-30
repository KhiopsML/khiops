// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWLearningSpec.h"

// Les include sont utiles sont dans l'implementation pour resoudre un probleme de dependance cyclique
#include "KWFrequencyVector.h"
#include "KWDiscretizerMODL.h"
#include "KWDataGrid.h"
#include "KWDescriptiveStats.h"
#include "KWDataGridCosts.h"

KWLearningService::KWLearningService()
{
	learningSpec = NULL;
}

KWLearningService::~KWLearningService() {}

boolean KWLearningService::Check() const
{
	boolean bOk = true;

	if (learningSpec == NULL)
	{
		bOk = false;
		AddError("Missing learning specifications");
	}
	else if (not learningSpec->Check())
	{
		bOk = false;
		AddError("Incorrect learning specification");
	}
	return bOk;
}

const ALString KWLearningService::GetClassLabel() const
{
	return "Learning service";
}

const ALString KWLearningService::GetObjectLabel() const
{
	if (learningSpec == NULL)
		return "???";
	else
		return learningSpec->GetObjectLabel();
}

//////////////////////////////////////////////////

boolean KWLearningSpec::bTextConstructionUsedByTrees = false;
int KWLearningSpec::nMaxModalityNumber = 1000000;

KWLearningSpec::KWLearningSpec()
{
	kwcClass = NULL;
	database = NULL;
	nTargetAttributeType = KWType::Unknown;
	bIsTargetStatsComputed = false;
	nInstanceNumber = 0;
	targetDescriptiveStats = NULL;
	targetValueStats = NULL;
	nMainTargetModalityIndex = -1;
	dNullConstructionCost = 0;
	dNullPreparationCost = 0;
	dNullDataCost = 0;
	nInitialAttributeNumber = -1;
	bMultiTableConstruction = false;
	bTextConstruction = false;
	bTrees = false;
	bAttributePairs = false;
	nConstructionFamilyNumber = 0;
	bCheckTargetAttribute = true;
}

KWLearningSpec::~KWLearningSpec()
{
	if (targetDescriptiveStats != NULL)
		delete targetDescriptiveStats;
	if (targetValueStats != NULL)
		delete targetValueStats;
}

void KWLearningSpec::SetShortDescription(const ALString& sValue)
{
	sShortDescription = sValue;
}

const ALString& KWLearningSpec::GetShortDescription() const
{
	return sShortDescription;
}

void KWLearningSpec::SetClass(KWClass* kwcValue)
{
	KWClass* initialClass;
	KWAttribute* targetAttribute;

	require(kwcValue != NULL);

	// Memorisation de la classe
	initialClass = kwcClass;
	kwcClass = kwcValue;

	// Synchronisation du type de l'attribut cible
	nTargetAttributeType = KWType::None;
	targetAttribute = kwcClass->LookupAttribute(sTargetAttributeName);
	if (targetAttribute != NULL)
		nTargetAttributeType = targetAttribute->GetType();

	// Reinitialisation des statistiques descriptives
	if (initialClass == NULL or kwcClass == NULL or initialClass->GetName() != kwcClass->GetName())
		ResetTargetStats();
}

void KWLearningSpec::SetDatabase(KWDatabase* databaseObjects)
{
	database = databaseObjects;

	// Reinitialisation des statistiques descriptives
	ResetTargetStats();
}

void KWLearningSpec::SetTargetAttributeName(const ALString& sValue)
{
	KWAttribute* targetAttribute;

	// Memorisation du nom
	sTargetAttributeName = sValue;

	// Synchronisation du type de l'attribut cible
	nTargetAttributeType = KWType::Unknown;
	if (sTargetAttributeName == "")
		nTargetAttributeType = KWType::None;
	else if (kwcClass != NULL)
	{
		targetAttribute = kwcClass->LookupAttribute(sTargetAttributeName);
		if (targetAttribute != NULL)
			nTargetAttributeType = targetAttribute->GetType();
	}

	// Reinitialisation des statistiques descriptives
	ResetTargetStats();
}

boolean KWLearningSpec::ComputeTargetStats(const KWTupleTable* tupleTable)
{
	require(Check());
	require(tupleTable != NULL);

	// Initialisation
	ResetTargetStats();
	bIsTargetStatsComputed = true;

	// Memorisation du nombre d'instances
	nInstanceNumber = tupleTable->GetTotalFrequency();

	// Pas de calcul de la cas non supervise
	if (GetTargetAttributeName() == "")
	{
		assert(targetDescriptiveStats == NULL);
		assert(targetValueStats == NULL);
	}
	// Cas supervise
	else
	{
		assert(targetDescriptiveStats != NULL);
		assert(targetValueStats != NULL);
		assert(GetTargetAttributeType() == KWType::Continuous or GetTargetAttributeType() == KWType::Symbol);

		// Calcul des stats descriptives de l'attribut cible
		bIsTargetStatsComputed = targetDescriptiveStats->ComputeStats(tupleTable);

		// Nettoyage initial des statistiques par valeur
		targetValueStats->DeleteAll();

		// Calcul des statistiques par valeur
		if (bIsTargetStatsComputed)
		{
			// Cas continu
			if (GetTargetAttributeType() == KWType::Continuous)
			{
				bIsTargetStatsComputed =
				    ComputeContinuousValueStats(GetTargetAttributeName(), tupleTable, targetValueStats);
			}
			// Cas symbol
			else if (GetTargetAttributeType() == KWType::Symbol)
			{
				bIsTargetStatsComputed = ComputeSymbolValueStats(GetTargetAttributeName(), tupleTable,
										 false, targetValueStats);

				// Index de la valeur cible principale
				if (bIsTargetStatsComputed)
				{
					// Calcul de l'index de la valeur cible principale
					nMainTargetModalityIndex = ComputeMainTargetModalityIndex();

					// Parametrage de la modalite cible principale dans la grille de valeurs cibles
					targetValueStats->SetMainTargetModalityIndex(nMainTargetModalityIndex);
				}
			}
		}
	}

	// Calcul des cout de model null
	if (bIsTargetStatsComputed)
	{
		ComputeNullCost();
	}

	// Reinitialisation des resultats si interruption utilisateur
	if (TaskProgression::IsInterruptionRequested())
		ResetTargetStats();

	// Quelques verifications
	ensure(targetDescriptiveStats == NULL or targetValueStats != NULL);
	ensure(targetDescriptiveStats != NULL or targetValueStats == NULL);
	ensure(targetDescriptiveStats == NULL or not bIsTargetStatsComputed or
	       targetDescriptiveStats->GetValueNumber() == targetValueStats->ComputeTotalGridSize());
	ensure(targetValueStats == NULL or not bIsTargetStatsComputed or
	       targetValueStats->ComputeGridFrequency() == nInstanceNumber);
	return bIsTargetStatsComputed;
}

boolean KWLearningSpec::IsTargetGrouped() const
{
	require(preprocessingSpec.CheckForTargetType(GetTargetAttributeType()));

	return GetTargetAttributeType() == KWType::Symbol and preprocessingSpec.GetTargetGrouped();
}

boolean KWLearningSpec::ComputeContinuousValueStats(const ALString& sAttributeName, const KWTupleTable* tupleTable,
						    KWDataGridStats* valueStats)
{
	boolean bOk = true;
	KWDGSAttributeContinuousValues* attributeContinuousValues;
	KWTupleTable univariateTupleTable;
	const KWTuple* univariateTuple;
	int nTuple;

	require(Check());
	require((not GetCheckTargetAttribute() and sAttributeName == GetTargetAttributeName()) or
		GetClass()->LookupAttribute(sAttributeName) != NULL);
	require((not GetCheckTargetAttribute() and sAttributeName == GetTargetAttributeName()) or
		GetClass()->LookupAttribute(sAttributeName)->GetType() == KWType::Continuous);
	require(tupleTable != NULL);
	require(tupleTable->GetAttributeNameAt(0) == sAttributeName);
	require(valueStats != NULL);

	// Extraction d'une table de tuples dedies a l'attribut
	tupleTable->BuildUnivariateTupleTable(sAttributeName, &univariateTupleTable);

	// Initialisation d'une partition en valeurs
	attributeContinuousValues = new KWDGSAttributeContinuousValues;
	attributeContinuousValues->SetAttributeName(sAttributeName);
	attributeContinuousValues->SetPartNumber(tupleTable->GetSize());

	attributeContinuousValues->SetInitialValueNumber(tupleTable->GetTotalFrequency());
	attributeContinuousValues->SetGranularizedValueNumber(tupleTable->GetTotalFrequency());

	// Initialisation des valeurs
	for (nTuple = 0; nTuple < univariateTupleTable.GetSize(); nTuple++)
	{
		univariateTuple = univariateTupleTable.GetAt(nTuple);
		attributeContinuousValues->SetValueAt(nTuple, univariateTuple->GetContinuousAt(0));
	}

	// Initialisation de la grille de statistiques univariees
	valueStats->DeleteAll();
	valueStats->AddAttribute(attributeContinuousValues);
	valueStats->CreateAllCells();

	// Initialisation des effectifs par valeur
	for (nTuple = 0; nTuple < univariateTupleTable.GetSize(); nTuple++)
	{
		univariateTuple = univariateTupleTable.GetAt(nTuple);
		valueStats->SetUnivariateCellFrequencyAt(nTuple, univariateTuple->GetFrequency());
	}
	assert(not bOk or valueStats->ComputeGridFrequency() == tupleTable->GetTotalFrequency());
	return bOk;
}

boolean KWLearningSpec::ComputeSymbolValueStats(const ALString& sAttributeName, const KWTupleTable* tupleTable,
						boolean bSortByDecreasingFrequencies, KWDataGridStats* valueStats)
{
	boolean bOk = true;
	KWDGSAttributeSymbolValues* attributeSymbolValues;
	KWTupleTable univariateTupleTable;
	const KWTuple* univariateTuple;
	Symbol sValue;
	Symbol sRef;
	int nValueNumber;
	int nValueIndex;
	int nStarValueFrequency;

	require(Check());
	require((not GetCheckTargetAttribute() and sAttributeName == GetTargetAttributeName()) or
		GetClass()->LookupAttribute(sAttributeName) != NULL);
	require((not GetCheckTargetAttribute() and sAttributeName == GetTargetAttributeName()) or
		GetClass()->LookupAttribute(sAttributeName)->GetType() == KWType::Symbol);
	require(tupleTable != NULL);
	require(tupleTable->GetAttributeNameAt(0) == sAttributeName);
	require(valueStats != NULL);

	// Extraction d'une table de tuples dedies a l'attribut
	tupleTable->BuildUnivariateTupleTable(sAttributeName, &univariateTupleTable);

	// Tri des valeurs par valeur croissante
	nValueNumber = univariateTupleTable.GetSize();
	if (bSortByDecreasingFrequencies)
		univariateTupleTable.SortByDecreasingFrequencies();
	else
		univariateTupleTable.SortByValues();

	// Filtrage si necessaire des valeurs singletons pour creer
	// une valeur speciale au dela de la premiere valeur singleton
	nStarValueFrequency = 0;
	if (bSortByDecreasingFrequencies)
	{
		for (nValueIndex = 0; nValueIndex < nValueNumber; nValueIndex++)
		{
			univariateTuple = univariateTupleTable.GetAt(nValueIndex);

			// Si effectif 1, on garde la modalite et on fusionne les suivantes
			if (univariateTuple->GetFrequency() == 1)
			{
				// Calcul de l'effectif de la modalite speciale
				nStarValueFrequency = nValueNumber - 1 - nValueIndex;

				// On ne garde que les permieres modalites
				nValueNumber = nValueIndex + 1;

				// Fin du traitement
				break;
			}
		}
	}

	// Initialisation d'une partition en valeurs
	attributeSymbolValues = new KWDGSAttributeSymbolValues;
	attributeSymbolValues->SetAttributeName(sAttributeName);
	if (nStarValueFrequency > 0)
		attributeSymbolValues->SetPartNumber(nValueNumber + 1);
	else
		attributeSymbolValues->SetPartNumber(nValueNumber);

	attributeSymbolValues->SetInitialValueNumber(nValueNumber);
	attributeSymbolValues->SetGranularizedValueNumber(nValueNumber);

	for (nValueIndex = 0; nValueIndex < nValueNumber; nValueIndex++)
	{
		univariateTuple = univariateTupleTable.GetAt(nValueIndex);
		attributeSymbolValues->SetValueAt(nValueIndex, univariateTuple->GetSymbolAt(0));
	}
	if (nStarValueFrequency > 0)
		attributeSymbolValues->SetValueAt(nValueNumber, Symbol::GetStarValue());

	// Initialisation de la grille de statistiques univariees
	valueStats->DeleteAll();
	valueStats->AddAttribute(attributeSymbolValues);
	valueStats->CreateAllCells();

	// Initialisation des effectifs
	for (nValueIndex = 0; nValueIndex < nValueNumber; nValueIndex++)
	{
		univariateTuple = univariateTupleTable.GetAt(nValueIndex);
		valueStats->SetUnivariateCellFrequencyAt(nValueIndex, univariateTuple->GetFrequency());
	}
	if (nStarValueFrequency > 0)
		valueStats->SetUnivariateCellFrequencyAt(nValueNumber, nStarValueFrequency);
	assert(not bOk or tupleTable->GetTotalFrequency() == valueStats->ComputeGridFrequency());
	return bOk;
}

KWLearningSpec* KWLearningSpec::Clone() const
{
	KWLearningSpec* kwlsClone;

	kwlsClone = new KWLearningSpec;
	kwlsClone->CopyFrom(this);
	return kwlsClone;
}

void KWLearningSpec::CopyFrom(const KWLearningSpec* kwlsSource)
{
	require(kwlsSource != NULL);

	// Recopie du contenu de l'objet source
	sShortDescription = kwlsSource->sShortDescription;
	kwcClass = kwlsSource->kwcClass;
	database = kwlsSource->database;
	sTargetAttributeName = kwlsSource->sTargetAttributeName;
	nTargetAttributeType = kwlsSource->nTargetAttributeType;
	sMainTargetModality = kwlsSource->sMainTargetModality;
	preprocessingSpec.CopyFrom(&kwlsSource->preprocessingSpec);

	// Recopie des donnees calculees
	nInstanceNumber = kwlsSource->nInstanceNumber;
	if (targetDescriptiveStats != NULL)
		delete targetDescriptiveStats;
	targetDescriptiveStats = NULL;
	if (kwlsSource->targetDescriptiveStats != NULL)
	{
		targetDescriptiveStats = kwlsSource->targetDescriptiveStats->Clone();
		targetDescriptiveStats->SetLearningSpec(this);
	}
	if (targetValueStats != NULL)
		delete targetValueStats;
	targetValueStats = NULL;
	if (kwlsSource->targetValueStats != NULL)
		targetValueStats = kwlsSource->targetValueStats->Clone();
	nMainTargetModalityIndex = kwlsSource->nMainTargetModalityIndex;
	dNullConstructionCost = kwlsSource->dNullConstructionCost;
	dNullPreparationCost = kwlsSource->dNullPreparationCost;
	dNullDataCost = kwlsSource->dNullDataCost;
	nInitialAttributeNumber = kwlsSource->nInitialAttributeNumber;
	bMultiTableConstruction = kwlsSource->bMultiTableConstruction;
	bTextConstruction = kwlsSource->bTextConstruction;
	bTrees = kwlsSource->bTrees;
	bAttributePairs = kwlsSource->bAttributePairs;
	bIsTargetStatsComputed = kwlsSource->bIsTargetStatsComputed;
	bCheckTargetAttribute = kwlsSource->bCheckTargetAttribute;
}

void KWLearningSpec::CopyTargetStatsFrom(const KWLearningSpec* kwlsSource)
{
	require(kwlsSource != NULL);

	// Recopie des donnees calculees
	nInstanceNumber = kwlsSource->nInstanceNumber;
	if (targetDescriptiveStats != NULL)
		delete targetDescriptiveStats;
	targetDescriptiveStats = NULL;
	if (kwlsSource->targetDescriptiveStats != NULL)
	{
		targetDescriptiveStats = kwlsSource->targetDescriptiveStats->Clone();
		targetDescriptiveStats->SetLearningSpec(this);
	}
	if (targetValueStats != NULL)
		delete targetValueStats;
	targetValueStats = NULL;
	if (kwlsSource->targetValueStats != NULL)
		targetValueStats = kwlsSource->targetValueStats->Clone();
	nMainTargetModalityIndex = kwlsSource->nMainTargetModalityIndex;
	bIsTargetStatsComputed = kwlsSource->bIsTargetStatsComputed;
	bCheckTargetAttribute = kwlsSource->bCheckTargetAttribute;
}

KWPreprocessingSpec* KWLearningService::GetPreprocessingSpec()
{
	require(Check());
	return learningSpec->GetPreprocessingSpec();
}

boolean KWLearningSpec::Check() const
{
	boolean bOk = true;

	// Classe
	if (kwcClass == NULL)
	{
		bOk = false;
		AddError("Missing dictionary");
	}
	else if (not kwcClass->Check())
	{
		bOk = false;
		AddError("Incorrect dictionary");
	}

	// Base de donnees
	if (database == NULL)
	{
		bOk = false;
		AddError("Missing database");
	}
	else if (kwcClass != NULL and kwcClass->GetName() != database->GetClassName())
	{
		bOk = false;
		AddError("Database (" + database->GetClassName() + ") inconsistent with dictionary (" +
			 kwcClass->GetName() + ")");
	}
	else if (not database->Check())
		bOk = false;

	// Attribut cible, si la verification est demandee
	if (bCheckTargetAttribute and sTargetAttributeName != "")
	{
		if (kwcClass != NULL)
		{
			KWAttribute* attribute;

			// Recherche de l'attribut et verifications
			attribute = kwcClass->LookupAttribute(sTargetAttributeName);
			if (attribute == NULL)
			{
				bOk = false;
				AddError("Target variable " + sTargetAttributeName + " unknown in dictionary " +
					 kwcClass->GetName());
			}
			else if (not KWType::IsSimple(attribute->GetType()))
			{
				bOk = false;
				AddError("Target variable " + sTargetAttributeName + " of type " +
					 KWType::ToString(attribute->GetType()));
			}
			else if (not attribute->GetUsed())
			{
				bOk = false;
				AddError("Target variable " + sTargetAttributeName + " unused in dictionary " +
					 kwcClass->GetName());
			}
		}
	}

	// Verification des algorithmes de pretraitement selon le type de la cible
	if (not preprocessingSpec.CheckForTargetType(GetTargetAttributeType()))
	{
		bOk = false;
		AddError("Incorrect specification for preprocessing algorithms");
	}

	// Verification de la synchronisation du type de l'attribut cible
	ensure(not bOk or (sTargetAttributeName == "" and nTargetAttributeType == KWType::None) or
	       (not bCheckTargetAttribute or
		kwcClass->LookupAttribute(sTargetAttributeName)->GetType() == nTargetAttributeType));
	ensure(nTargetAttributeType == KWType::Unknown or
	       (nTargetAttributeType == KWType::None and targetDescriptiveStats == NULL) or
	       (nTargetAttributeType == KWType::Continuous and
		cast(KWDescriptiveContinuousStats*, targetDescriptiveStats) != NULL) or
	       (nTargetAttributeType == KWType::Symbol and
		cast(KWDescriptiveSymbolStats*, targetDescriptiveStats) != NULL));
	ensure(nTargetAttributeType == KWType::Unknown or
	       (nTargetAttributeType == KWType::None and targetValueStats == NULL) or targetValueStats != NULL);
	ensure(nTargetAttributeType == KWType::Unknown or targetDescriptiveStats == NULL or
	       (targetDescriptiveStats->GetLearningSpec() == this and
		targetDescriptiveStats->GetAttributeName() == sTargetAttributeName));
	return bOk;
}

void KWLearningSpec::SetCheckTargetAttribute(boolean bValue)
{
	bCheckTargetAttribute = bValue;
}

boolean KWLearningSpec::GetCheckTargetAttribute() const
{
	return bCheckTargetAttribute;
}

const ALString KWLearningSpec::GetClassLabel() const
{
	return "Learning specification";
}

const ALString KWLearningSpec::GetObjectLabel() const
{
	ALString sLabel;

	// Nom de la classe
	if (GetClass() == NULL)
		sLabel = "???";
	else
		sLabel = GetClass()->GetName();

	// Nom de l'attribut
	sLabel += " ";
	if (GetTargetAttributeName() != "")
		sLabel += GetTargetAttributeName();

	return sLabel;
}

void KWLearningSpec::ResetTargetStats()
{
	// Initialisation du nombre d'instances
	nInstanceNumber = 0;

	// Synchronisation avec les statistique descriptives de l'attribut cible
	if (targetDescriptiveStats != NULL)
		delete targetDescriptiveStats;
	targetDescriptiveStats = NULL;
	if (nTargetAttributeType == KWType::Continuous)
		targetDescriptiveStats = new KWDescriptiveContinuousStats;
	else if (nTargetAttributeType == KWType::Symbol)
		targetDescriptiveStats = new KWDescriptiveSymbolStats;
	if (targetDescriptiveStats != NULL)
	{
		targetDescriptiveStats->SetLearningSpec(this);
		targetDescriptiveStats->SetAttributeName(sTargetAttributeName);
	}

	// Synchronisation avec les statistiques par valeurs de l'attribut cible
	if (targetValueStats != NULL)
		delete targetValueStats;
	targetValueStats = NULL;
	if (nTargetAttributeType == KWType::Continuous or nTargetAttributeType == KWType::Symbol)
		targetValueStats = new KWDataGridStats;

	// Index de la modalite cible principale
	nMainTargetModalityIndex = -1;

	// Couts du model null
	dNullConstructionCost = 0;
	dNullPreparationCost = 0;
	dNullDataCost = 0;

	// Mise a false du flag de calcul des stats
	bIsTargetStatsComputed = false;
}

void KWLearningSpec::ComputeNullCost()
{
	KWDiscretizerMODL discretizerMODL;
	KWDiscretizerMODLFamily* discretizerMODLFamily;
	KWDataGrid nullDataGrid;
	KWDGPart* nullPart;
	ObjectArray oaParts;
	KWDGCell* nullCell;
	KWDataGridRegressionCosts nullRegressionCost;
	KWFrequencyTable nullFrequencyTable;
	KWFrequencyTable* kwftNullPreparedTable;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	IntVector* ivFrequencyVector;
	int i;
	int nValueNumber;

	require(GetTargetAttributeName() == "" or GetTargetValueStats() != NULL);

	// Cas Continuous
	if (GetTargetAttributeType() == KWType::Continuous)
	{
		// Creation d'une grille de deux attributs numerique et une seule cellule
		nullDataGrid.Initialize(2, 0);

		// Creation des attributs, en gerant l'effet de bord d'une base vide
		nullDataGrid.GetAttributeAt(0)->SetAttributeName("Null");
		nullDataGrid.GetAttributeAt(0)->SetAttributeType(KWType::Continuous);
		nullDataGrid.GetAttributeAt(0)->SetInitialValueNumber(max(1, GetInstanceNumber()));
		nullDataGrid.GetAttributeAt(0)->SetGranularizedValueNumber(max(1, GetInstanceNumber()));
		nullDataGrid.GetAttributeAt(1)->SetAttributeName(GetTargetAttributeName());
		nullDataGrid.GetAttributeAt(1)->SetAttributeType(KWType::Continuous);
		nullDataGrid.GetAttributeAt(1)->SetInitialValueNumber(max(1, GetInstanceNumber()));
		nullDataGrid.GetAttributeAt(1)->SetGranularizedValueNumber(max(1, GetInstanceNumber()));
		nullDataGrid.GetAttributeAt(1)->SetAttributeTargetFunction(true);

		// Creation des intervalles
		nullPart = nullDataGrid.GetAttributeAt(0)->AddPart();
		nullPart->GetInterval()->SetLowerBound(KWDGInterval::GetMinLowerBound());
		nullPart->GetInterval()->SetUpperBound(KWDGInterval::GetMaxUpperBound());
		nullPart = nullDataGrid.GetAttributeAt(1)->AddPart();
		nullPart->GetInterval()->SetLowerBound(KWDGInterval::GetMinLowerBound());
		nullPart->GetInterval()->SetUpperBound(KWDGInterval::GetMaxUpperBound());

		// Creation de la cellule
		nullDataGrid.SetCellUpdateMode(true);
		nullDataGrid.BuildIndexingStructure();
		oaParts.Add(nullDataGrid.GetAttributeAt(0)->GetHeadPart());
		oaParts.Add(nullDataGrid.GetAttributeAt(1)->GetHeadPart());
		nullCell = nullDataGrid.AddCell(&oaParts);
		nullCell->SetCellFrequency(GetInstanceNumber());
		nullDataGrid.SetCellUpdateMode(false);
		nullDataGrid.DeleteIndexingStructure();
		assert(nullDataGrid.Check());

		// Initialisation de la structure de couts
		// Dans le cas de la regression, on met les cout MODL en dur
		nullRegressionCost.InitializeDefaultCosts(&nullDataGrid);

		// Memorisation des couts MODL
		dNullConstructionCost = nullRegressionCost.ComputeDataGridTotalConstructionCost(&nullDataGrid);
		dNullPreparationCost = nullRegressionCost.ComputeDataGridTotalPreparationCost(&nullDataGrid);
		dNullDataCost = nullRegressionCost.ComputeDataGridTotalDataCost(&nullDataGrid);
	}
	// Cas Symbol
	else if (GetTargetAttributeType() == KWType::Symbol)
	{
		// Recherche du discretiseur MODL, qui peut avoir ete redefini (cas des arbres de decision par exemple)
		if (GetPreprocessingSpec()
			->GetDiscretizerSpec()
			->GetDiscretizer(GetTargetAttributeType())
			->IsMODLFamily())
			discretizerMODLFamily = cast(
			    KWDiscretizerMODLFamily*,
			    GetPreprocessingSpec()->GetDiscretizerSpec()->GetDiscretizer(GetTargetAttributeType()));
		// Sinon, on prend le discretiseur MODL standard
		else
			discretizerMODLFamily = &discretizerMODL;

		// Creation d'une table de contingence cible avec une seule ligne et une colonne par valeur
		nValueNumber = GetTargetValueStats()->GetAttributeAt(0)->GetPartNumber();
		nullFrequencyTable.Initialize(1);

		// Acces au vecteur de la ligne et parametrage de sa taille (sense etre en representation dense)
		kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, nullFrequencyTable.GetFrequencyVectorAt(0));
		ivFrequencyVector = kwdfvFrequencyVector->GetFrequencyVector();

		ivFrequencyVector->SetSize(nValueNumber);

		// Alimentation de cette ligne par les frequences globales des valeurs cibles
		assert(GetTargetDescriptiveStats()->GetValueNumber() == GetTargetValueStats()->ComputeTargetGridSize());
		for (i = 0; i < nValueNumber; i++)
		{
			ivFrequencyVector->SetAt(i, GetTargetValueStats()->GetUnivariateCellFrequencyAt(i));
		}

		nullFrequencyTable.SetInitialValueNumber(nullFrequencyTable.GetTotalFrequency());
		nullFrequencyTable.SetGranularizedValueNumber(nullFrequencyTable.GetInitialValueNumber());

		// Utilisation du discretiseur specifie dans les pretraitements
		discretizerMODLFamily->Discretize(&nullFrequencyTable, kwftNullPreparedTable);

		// Memorisation des couts MODL
		dNullConstructionCost =
		    discretizerMODLFamily->ComputeDiscretizationConstructionCost(&nullFrequencyTable);
		dNullPreparationCost = discretizerMODLFamily->ComputeDiscretizationPreparationCost(&nullFrequencyTable);
		dNullDataCost = discretizerMODLFamily->ComputeDiscretizationDataCost(&nullFrequencyTable);

		// Nettoyage
		delete kwftNullPreparedTable;
	}
	// Cas non supervise
	else
	{
		// Cout pour le choix du modele null
		dNullConstructionCost = log(2.0);

		// Ces couts dependent du nombre et du type de variables (numerique ou categoriel)
		dNullPreparationCost = 0;
		dNullDataCost = 0;
	}
}

int KWLearningSpec::ComputeMainTargetModalityIndex() const
{
	int nIndex;
	const KWDGSAttributeSymbolValues* symbolValues;

	nIndex = -1;
	if (GetTargetAttributeType() == KWType::Symbol and targetValueStats != NULL and
	    targetValueStats->GetAttributeNumber() > 0 and not sMainTargetModality.IsEmpty())
	{
		assert(targetValueStats->GetAttributeNumber() == 1);
		symbolValues = cast(const KWDGSAttributeSymbolValues*, targetValueStats->GetAttributeAt(0));
		nIndex = symbolValues->ComputeSymbolPartIndex(sMainTargetModality);
	}
	return nIndex;
}

//////////////////////////////////////////////////////////////////////////////
// Classe PLShared_LearningSpec

PLShared_LearningSpec::PLShared_LearningSpec() {}

PLShared_LearningSpec::~PLShared_LearningSpec() {}

void PLShared_LearningSpec::SetLearningSpec(KWLearningSpec* learningSpec)
{
	require(learningSpec != NULL);
	SetObject(learningSpec);
}

KWLearningSpec* PLShared_LearningSpec::GetLearningSpec() const
{
	return cast(KWLearningSpec*, GetObject());
}

void PLShared_LearningSpec::FinalizeSpecification(KWClass* kwcValue, KWDatabase* databaseObjects)
{
	KWLearningSpec* learningSpec;

	require(kwcValue != NULL);
	require(databaseObjects != NULL);
	require(kwcValue->GetName() == databaseObjects->GetClassName());

	learningSpec = GetLearningSpec();
	learningSpec->kwcClass = kwcValue;
	learningSpec->database = databaseObjects;
	ensure(learningSpec->Check());
}

void PLShared_LearningSpec::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	KWLearningSpec* learningSpec;
	PLShared_Symbol sharedSymbol;
	PLShared_PreprocessingSpec sharedPreprocessingSpec;
	PLShared_DescriptiveContinuousStats sharedDescriptiveContinuousStats;
	PLShared_DescriptiveSymbolStats sharedDescriptiveSymbolStats;
	PLShared_DataGridStats sharedDataGridStats;

	require(serializer->IsOpenForWrite());

	learningSpec = cast(KWLearningSpec*, o);

	// Serialisation des donnees de base
	serializer->PutString(learningSpec->sShortDescription);
	serializer->PutString(learningSpec->sTargetAttributeName);
	serializer->PutInt(learningSpec->nTargetAttributeType);
	sharedSymbol = learningSpec->sMainTargetModality;
	sharedSymbol.Serialize(serializer);
	serializer->PutBoolean(learningSpec->bIsTargetStatsComputed);
	serializer->PutInt(learningSpec->nInstanceNumber);
	serializer->PutDouble(learningSpec->dNullConstructionCost);
	serializer->PutDouble(learningSpec->dNullPreparationCost);
	serializer->PutDouble(learningSpec->dNullDataCost);
	serializer->PutInt(learningSpec->nInitialAttributeNumber);
	serializer->PutBoolean(learningSpec->bMultiTableConstruction);
	serializer->PutBoolean(learningSpec->bTextConstruction);
	serializer->PutBoolean(learningSpec->bTrees);
	serializer->PutBoolean(learningSpec->bAttributePairs);
	serializer->PutInt(learningSpec->nConstructionFamilyNumber);
	serializer->PutInt(learningSpec->nMainTargetModalityIndex);
	serializer->PutInt(learningSpec->nMaxModalityNumber);
	serializer->PutBoolean(learningSpec->bCheckTargetAttribute);

	// Serialisation des specification de preprocessing
	sharedPreprocessingSpec.SerializeObject(serializer, &(learningSpec->preprocessingSpec));

	// Serialisation des statistiques sur l'attribut cible
	AddNull(serializer, learningSpec->targetDescriptiveStats);
	if (learningSpec->targetDescriptiveStats != NULL)
	{
		// Serialisation selon le type d'attribut
		if (learningSpec->nTargetAttributeType == KWType::Continuous)
			sharedDescriptiveContinuousStats.SerializeObject(serializer,
									 learningSpec->targetDescriptiveStats);
		else if (learningSpec->nTargetAttributeType == KWType::Symbol)
			sharedDescriptiveSymbolStats.SerializeObject(serializer, learningSpec->targetDescriptiveStats);
	}
	AddNull(serializer, learningSpec->targetValueStats);
	if (learningSpec->targetValueStats != NULL)
		sharedDataGridStats.SerializeObject(serializer, learningSpec->targetValueStats);
}

void PLShared_LearningSpec::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	KWLearningSpec* learningSpec;
	PLShared_Symbol sharedSymbol;
	PLShared_PreprocessingSpec sharedPreprocessingSpec;
	PLShared_DescriptiveContinuousStats sharedDescriptiveContinuousStats;
	PLShared_DescriptiveSymbolStats sharedDescriptiveSymbolStats;
	PLShared_DataGridStats sharedDataGridStats;

	require(serializer->IsOpenForRead());

	learningSpec = cast(KWLearningSpec*, o);

	// Deserialisation des donnees de base
	learningSpec->sShortDescription = serializer->GetString();
	learningSpec->sTargetAttributeName = serializer->GetString();
	learningSpec->nTargetAttributeType = serializer->GetInt();
	sharedSymbol.Deserialize(serializer);
	learningSpec->sMainTargetModality = sharedSymbol;
	learningSpec->bIsTargetStatsComputed = serializer->GetBoolean();
	learningSpec->nInstanceNumber = serializer->GetInt();
	learningSpec->dNullConstructionCost = serializer->GetDouble();
	learningSpec->dNullPreparationCost = serializer->GetDouble();
	learningSpec->dNullDataCost = serializer->GetDouble();
	learningSpec->nInitialAttributeNumber = serializer->GetInt();
	learningSpec->bMultiTableConstruction = serializer->GetBoolean();
	learningSpec->bTextConstruction = serializer->GetBoolean();
	learningSpec->bTrees = serializer->GetBoolean();
	learningSpec->bAttributePairs = serializer->GetBoolean();
	learningSpec->nConstructionFamilyNumber = serializer->GetInt();
	learningSpec->nMainTargetModalityIndex = serializer->GetInt();
	learningSpec->nMaxModalityNumber = serializer->GetInt();
	learningSpec->bCheckTargetAttribute = serializer->GetBoolean();

	// Deserialisation des specification de preprocessing
	sharedPreprocessingSpec.DeserializeObject(serializer, &(learningSpec->preprocessingSpec));

	// Deserialisation des statistiques sur l'attribut cible
	assert(learningSpec->targetDescriptiveStats == NULL);
	assert(learningSpec->targetValueStats == NULL);
	if (not GetNull(serializer))
	{
		// Deserialisation selon le type d'attribut
		if (learningSpec->nTargetAttributeType == KWType::Continuous)
		{
			learningSpec->targetDescriptiveStats = new KWDescriptiveContinuousStats;
			sharedDescriptiveContinuousStats.DeserializeObject(serializer,
									   learningSpec->targetDescriptiveStats);
		}
		else if (learningSpec->nTargetAttributeType == KWType::Symbol)
		{
			learningSpec->targetDescriptiveStats = new KWDescriptiveSymbolStats;
			sharedDescriptiveSymbolStats.DeserializeObject(serializer,
								       learningSpec->targetDescriptiveStats);
		}
		learningSpec->targetDescriptiveStats->SetLearningSpec(learningSpec);
	}
	if (not GetNull(serializer))
	{
		learningSpec->targetValueStats = new KWDataGridStats;
		sharedDataGridStats.DeserializeObject(serializer, learningSpec->targetValueStats);
	}
}

Object* PLShared_LearningSpec::Create() const
{
	return new KWLearningSpec;
}
