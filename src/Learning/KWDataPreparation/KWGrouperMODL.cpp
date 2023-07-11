// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWGrouperMODL.h"

boolean KWGrouperMODLFamily::IsMODLFamily() const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////
// Classe KWGrouperMODL

double KWGrouperMODL::dEpsilon = 1e-6;

KWGrouperMODL::KWGrouperMODL()
{
	groupingCosts = new KWMODLGroupingCosts;
}

KWGrouperMODL::~KWGrouperMODL()
{
	delete groupingCosts;
}

const ALString KWGrouperMODL::GetName() const
{
	return "MODL";
}

KWGrouper* KWGrouperMODL::Create() const
{
	return new KWGrouperMODL;
}

void KWGrouperMODL::SetGroupingCosts(KWUnivariatePartitionCosts* kwupcCosts)
{
	require(kwupcCosts != NULL);
	require(groupingCosts != NULL);
	delete groupingCosts;
	groupingCosts = kwupcCosts;
}

KWUnivariatePartitionCosts* KWGrouperMODL::GetGroupingCosts() const
{
	ensure(groupingCosts != NULL);
	return groupingCosts;
}
double KWGrouperMODL::ComputeGroupingCost(KWFrequencyTable* kwftGroupedTable, int nInitialModalityNumber) const
{
	double dCost;

	require(kwftGroupedTable != NULL);
	require(nInitialModalityNumber >= 1);

	// Cas particuliers: cout nul
	if (kwftGroupedTable->GetTotalFrequency() == 0 or kwftGroupedTable->GetFrequencyVectorSize() <= 1)
		return 0;

	// Initialisation des donnees de travail
	InitializeWorkingData(kwftGroupedTable, nInitialModalityNumber);

	// Cout global du modele
	dCost = ComputePartitionGlobalCost(kwftGroupedTable);

	// Nettoyage des donnees de travail
	CleanWorkingData();

	return dCost;
}

double KWGrouperMODL::ComputeGroupingModelCost(KWFrequencyTable* kwftGroupedTable, int nInitialModalityNumber) const
{
	double dCost;

	require(kwftGroupedTable != NULL);
	require(nInitialModalityNumber >= 1);

	// Cas particuliers: cout nul
	if (kwftGroupedTable->GetTotalFrequency() == 0 or kwftGroupedTable->GetFrequencyVectorSize() <= 1)
		return 0;

	// Initialisation des donnees de travail
	InitializeWorkingData(kwftGroupedTable, nInitialModalityNumber);

	// Cout global du modele
	dCost = groupingCosts->ComputePartitionGlobalModelCost(kwftGroupedTable);

	// Nettoyage des donnees de travail
	CleanWorkingData();

	return dCost;
}

double KWGrouperMODL::ComputeGroupingConstructionCost(KWFrequencyTable* kwftGroupedTable,
						      int nInitialModalityNumber) const
{
	double dCost;

	require(kwftGroupedTable != NULL);
	require(nInitialModalityNumber >= 1);

	// Cas particuliers: cout nul
	if (kwftGroupedTable->GetTotalFrequency() == 0 or kwftGroupedTable->GetFrequencyVectorSize() <= 1)
		return 0;

	// Initialisation des donnees de travail
	InitializeWorkingData(kwftGroupedTable, nInitialModalityNumber);

	// Cout global du modele
	dCost = groupingCosts->ComputePartitionGlobalConstructionCost(kwftGroupedTable);

	// Nettoyage des donnees de travail
	CleanWorkingData();

	return dCost;
}

double KWGrouperMODL::ComputeGroupingPreparationCost(KWFrequencyTable* kwftGroupedTable,
						     int nInitialModalityNumber) const
{
	double dCost;

	require(kwftGroupedTable != NULL);
	require(nInitialModalityNumber >= 1);

	// Cas particuliers: cout nul
	if (kwftGroupedTable->GetTotalFrequency() == 0 or kwftGroupedTable->GetFrequencyVectorSize() <= 1)
		return 0;

	// Initialisation des donnees de travail
	InitializeWorkingData(kwftGroupedTable, nInitialModalityNumber);

	// Cout global du modele
	dCost = groupingCosts->ComputePartitionGlobalPreparationCost(kwftGroupedTable);

	// Nettoyage des donnees de travail
	CleanWorkingData();

	return dCost;
}

double KWGrouperMODL::ComputeGroupingDataCost(KWFrequencyTable* kwftGroupedTable, int nInitialModalityNumber) const
{
	double dCost;

	require(kwftGroupedTable != NULL);
	require(nInitialModalityNumber >= 1);

	// Cas particuliers: cout nul
	if (kwftGroupedTable->GetTotalFrequency() == 0 or kwftGroupedTable->GetFrequencyVectorSize() <= 1)
		return 0;

	// Initialisation des donnees de travail
	InitializeWorkingData(kwftGroupedTable, nInitialModalityNumber);

	// Cout global du modele
	dCost = groupingCosts->ComputePartitionGlobalDataCost(kwftGroupedTable);

	// Nettoyage des donnees de travail
	CleanWorkingData();

	return dCost;
}

boolean KWGrouperMODL::Check() const
{
	boolean bOk;
	bOk = dParam == 0;
	if (not bOk)
		AddError("The main parameter of the algorithm must be 0");
	return bOk;
}

/////////////////////////////////////////////////////////////////////////

void KWGrouperMODL::GranularizeFrequencyTable(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget,
					      int nGranularity, KWQuantileGroupBuilder* quantileBuilder) const
{
	int nInstanceNumber;
	int nPartileIndex;
	int nTargetIndex;
	int nTargetValueNumber;
	int nSourceValueNumber;
	int nSourceIndex;
	int nPartileNumber;
	int nActualPartileNumber;
	boolean bDisplayResults = false;
	KWDenseFrequencyVector* kwdfvPartileFrequencyVector;
	KWDenseFrequencyVector* kwdfvSourceFrequencyVector;
	IntVector* ivPartileFrequencies;
	IntVector* ivSourceFrequencies;

	require(0 <= nGranularity);

	// Nombre d'instances de la table initiale
	nInstanceNumber = kwftSource->GetTotalFrequency();

	// Calcul du nombre de partiles de la table granularisee
	nPartileNumber = (int)pow(2, nGranularity);

	// Cas ou la granularisation n'est pas appliquee : non prise en compte de la granularite ou granularite maximale
	if (nGranularity == 0 or nPartileNumber >= nInstanceNumber)
	{
		kwftTarget = kwftSource->Clone();
		kwftTarget->SetGranularity(nGranularity);
		kwftTarget->SetGarbageModalityNumber(0);
	}

	// Sinon on granularise
	else
	{
		require(quantileBuilder->IsFrequencyInitialized());

		nTargetValueNumber = kwftSource->GetFrequencyVectorSize();
		nSourceValueNumber = kwftSource->GetFrequencyVectorNumber();

		if (bDisplayResults)
		{
			cout << " Debut Granularize  avec QuantileGroupBuilder = " << nGranularity
			     << "\t Nbre de partiles = " << nPartileNumber << "\t nInstanceNumber " << nInstanceNumber
			     << endl;
			cout << "Nbre valeurs explicatives table initiale " << kwftSource->GetFrequencyVectorNumber()
			     << endl;
		}

		// Calcul des quantiles
		quantileBuilder->ComputeQuantiles(nPartileNumber);

		// Initialisation du nombre de partiles effectif (peut etre inferieur au nombre theorique du fait de
		// doublons)
		nActualPartileNumber = quantileBuilder->GetGroupNumber();

		// Cas d'un nombre de partiles egal au nombre de valeurs initiales (granularisation maximale et absence
		// de singletons)
		if (nActualPartileNumber == nSourceValueNumber)
		{
			kwftTarget = kwftSource->Clone();
			kwftTarget->SetGranularity(nGranularity);
			kwftTarget->SetGarbageModalityNumber(0);
		}
		else
		{
			// Creation de la table d'effectif cible
			kwftTarget = new KWFrequencyTable;
			kwftTarget->SetFrequencyVectorCreator(GetFrequencyVectorCreator()->Clone());
			kwftTarget->Initialize(nActualPartileNumber);

			// Initialisation du nombre de valeurs initiales et apres granularisation
			kwftTarget->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
			kwftTarget->SetGranularizedValueNumber(nActualPartileNumber);

			// Initialisation granularite et poubelle
			kwftTarget->SetGranularity(nGranularity);
			kwftTarget->SetGarbageModalityNumber(0);

			// Parcours des partiles
			for (nPartileIndex = 0; nPartileIndex < nActualPartileNumber; nPartileIndex++)
			{
				// Acces au vecteur du partile (sense etre en representation dense)
				kwdfvPartileFrequencyVector =
				    cast(KWDenseFrequencyVector*, kwftTarget->GetFrequencyVectorAt(nPartileIndex));
				ivPartileFrequencies = kwdfvPartileFrequencyVector->GetFrequencyVector();
				ivPartileFrequencies->SetSize(nTargetValueNumber);

				// Parcours des valeurs sources
				for (nSourceIndex = quantileBuilder->GetGroupFirstValueIndexAt(nPartileIndex);
				     nSourceIndex <= quantileBuilder->GetGroupLastValueIndexAt(nPartileIndex);
				     nSourceIndex++)
				{
					// Acces au vecteur source (sense etre en representation dense)
					kwdfvSourceFrequencyVector = cast(
					    KWDenseFrequencyVector*, kwftSource->GetFrequencyVectorAt(nSourceIndex));
					ivSourceFrequencies = kwdfvSourceFrequencyVector->GetFrequencyVector();

					// Ajout des frequences du vecteur source courant
					for (nTargetIndex = 0; nTargetIndex < ivPartileFrequencies->GetSize();
					     nTargetIndex++)
					{
						ivPartileFrequencies->UpgradeAt(
						    nTargetIndex, ivSourceFrequencies->GetAt(nTargetIndex));
					}
				}
			}
		}
	}

	// Initialisation du nombre de modalites par vecteur
	for (nSourceIndex = 0; nSourceIndex < kwftTarget->GetFrequencyVectorNumber(); nSourceIndex++)
	{
		kwftTarget->GetFrequencyVectorAt(nSourceIndex)->SetModalityNumber(1);
	}
	ensure(kwftSource->GetTotalFrequency() == kwftTarget->GetTotalFrequency());
}

void KWGrouperMODL::TestGranularizeFrequencyTable()
{
	int nSourceNumber;
	int nSource;
	int nTargetNumber;
	int nTableFrequency;
	int nI;
	int nRandom;
	int nGranularity;
	KWFrequencyTable kwftTest;
	KWFrequencyTable* kwftTestTarget;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	IntVector* ivFrequencyVector;
	KWGrouperMODL grouperMODL;
	KWQuantileGroupBuilder quantileBuilder;
	IntVector ivInputFrequencies;

	// Test de validite
	cout << "Initialisation d'une matrice de correlation" << endl;
	// nSourceNumber = AcquireRangedInt("Nombre de valeurs pour la loi source", 2, 100, 10);
	// nTargetNumber = AcquireRangedInt("Nombre de valeurs pour la loi cible", 2, 20, 5);
	nSourceNumber = 100;
	nTargetNumber = 2;
	nTableFrequency = 1000;
	kwftTest.Initialize(nSourceNumber);

	// Initialisation de la taille des vecteurs de la table
	for (nSource = 0; nSource < nSourceNumber; nSource++)
	{
		// Acces au vecteur de la ligne et parametrage de sa taille (sense etre en representation dense)
		kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, kwftTest.GetFrequencyVectorAt(nSource));
		ivFrequencyVector = kwdfvFrequencyVector->GetFrequencyVector();

		ivFrequencyVector->SetSize(nTargetNumber);
	}

	kwftTest.SetInitialValueNumber(nTableFrequency);
	kwftTest.SetGranularizedValueNumber(nTableFrequency);

	// Remplissage aleatoire
	cout << "Remplissage aleatoire" << endl;
	// nTableFrequency = AcquireRangedInt("Effectif total", 10, 1000000, 1000);
	for (nI = 0; nI < nTableFrequency; nI++)
	{
		nRandom = RandomInt(nSourceNumber * nTargetNumber - 1);
		cast(KWDenseFrequencyVector*, kwftTest.GetFrequencyVectorAt(nRandom / nTargetNumber))
		    ->GetFrequencyVector()
		    ->UpgradeAt(nRandom - nTargetNumber * (nRandom / nTargetNumber), 1);
	}
	cout << "Table initiale " << kwftTest << endl;

	// Initialisation du vecteur d'effectifs de la table
	for (nSource = 0; nSource < kwftTest.GetFrequencyVectorNumber(); nSource++)
		ivInputFrequencies.Add(kwftTest.GetFrequencyVectorAt(nSource)->ComputeTotalFrequency());

	// Initialisation du quantileBuilder
	quantileBuilder.InitializeFrequencies(&ivInputFrequencies);

	kwftTestTarget = NULL;

	// Parcours des granularites
	for (nGranularity = 1; nGranularity <= (int)ceil(log(kwftTest.GetTotalFrequency() * 1.0) / log(2.0));
	     nGranularity++)
	{
		kwftTest.SortTableBySourceFrequency(false, NULL, NULL);
		grouperMODL.GranularizeFrequencyTable(&kwftTest, kwftTestTarget, nGranularity, &quantileBuilder);
		cout << "Table apres granularisation avec la granularite" << nGranularity << " avec "
		     << pow(2, nGranularity) << " partiles " << endl;
		cout << *kwftTestTarget << endl;
		delete kwftTestTarget;
	}
}

void KWGrouperMODL::PostOptimizeGranularity(KWFrequencyTable* kwftTarget, IntVector* ivGroups,
					    KWQuantileGroupBuilder* quantileBuilder, int nLastExploredGranularity) const
{
	int nCurrentGranularity;
	int nBestGranularity;
	int nGranularityPartileNumber;
	int nCatchAllFirstModalityIndex;
	int nCatchAllFirstModalityGroupIndex;
	int nCatchAllModalityIndex;
	int nCatchAllModalityGroupIndex;
	boolean bIncompatibleGranularity;
	boolean bLastExploredGranularity;

	require(kwftTarget->GetGranularity() > nLastExploredGranularity + 1);

	// Initialisation
	bLastExploredGranularity = false;
	bIncompatibleGranularity = false;
	nBestGranularity = kwftTarget->GetGranularity();

	// Boucle descendante sur les granularites jusqu'a rencontrer l'avant derniere granularite exploree ou une
	// granularite incompatible avec la partition
	nCurrentGranularity = kwftTarget->GetGranularity() - 1;
	while (not bLastExploredGranularity and not bIncompatibleGranularity)
	{
		nGranularityPartileNumber = (int)pow(2.0, nCurrentGranularity);

		// Cas ou le nombre de partiles theorique de cette granularite est superieur ou egal a la taille de la
		// partition
		if (nGranularityPartileNumber >= kwftTarget->GetFrequencyVectorNumber())
		{
			nGranularityPartileNumber = quantileBuilder->ComputeQuantiles(nGranularityPartileNumber);

			if (nGranularityPartileNumber >= kwftTarget->GetFrequencyVectorNumber())
			{
				// On doit verifier que, pour cette granularite, toutes les modalites du fourre-tout
				// sont dans le meme groupe

				// Extraction de l'index de la 1ere modalite du fourre-tout. Le fourre-tout est le
				// dernier groupe du quantileBuilder
				nCatchAllFirstModalityIndex =
				    quantileBuilder->GetGroupFirstValueIndexAt(nGranularityPartileNumber - 1);

				// Index du groupe de cette modalite
				nCatchAllFirstModalityGroupIndex = ivGroups->GetAt(nCatchAllFirstModalityIndex);

				// Parcours des autres modalites du fourre-tout pour verifier qu'elles sont dans le meme
				// groupe que la premiere modalite du fourre-tout
				nCatchAllModalityIndex = nCatchAllFirstModalityIndex + 1;
				while (not bIncompatibleGranularity and
				       nCatchAllModalityIndex <=
					   quantileBuilder->GetGroupLastValueIndexAt(nGranularityPartileNumber - 1))
				{
					nCatchAllModalityGroupIndex = ivGroups->GetAt(nCatchAllModalityIndex);
					if (nCatchAllFirstModalityGroupIndex != nCatchAllModalityGroupIndex)
						bIncompatibleGranularity = true;
					nCatchAllModalityIndex++;
				}
			}
			else
				bIncompatibleGranularity = true;
		}
		else
			bIncompatibleGranularity = true;

		// Cas d'une granularite compatible : memorisation de la granularite
		if (not bIncompatibleGranularity)
			nBestGranularity = nCurrentGranularity;

		// Prochaine granularite
		nCurrentGranularity--;
		bLastExploredGranularity = (nCurrentGranularity == nLastExploredGranularity);
	}

	// Memorisation de la meilleure granularite
	kwftTarget->SetGranularity(nBestGranularity);
}

void KWGrouperMODL::GroupPreprocessedTable(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget,
					   IntVector*& ivGroups) const
{
	boolean bDisplayResults = false;
	boolean bDisplayFinalResults = false;
	KWQuantileGroupBuilder quantileBuilder;
	KWFrequencyTable* kwftGranularizedTable;
	KWFrequencyTable* kwftOptimizedGranularizedTable;
	IntVector* ivGranularizedGroups;
	IntVector ivInputFrequencies;
	double dCost;
	double dBestCost;
	int nGranularityIndex;
	int nGranularityMax;
	int nSourceIndex;
	boolean bIsLastGranularity;
	boolean bIsGranularitySelected;
	int nCurrentPartileNumber;
	int nPreviousPartileNumber;
	int nMaxPartileNumber;
	double dRequiredIncreasingCoefficient = 1.5;
	boolean bSingleton;
	int nCurrentExploredGranularity;
	int nLastExploredGranularity;

	require(kwftSource != NULL);
	require(kwftSource->GetFrequencyVectorSize() > 1);

	// Initialisations
	dBestCost = DBL_MAX;
	kwftGranularizedTable = NULL;
	nCurrentPartileNumber = 0;
	nPreviousPartileNumber = 0;
	bIsLastGranularity = false;

	// Granularite maximale : dernier G tel que 2^G soit < N
	// Cela garantit qu'avec la granularite maximale, l'effectif minimal d'une partie sera 2 et que
	// les singletons seront groupes dans le fourre-tout
	nGranularityMax = (int)floor(log(kwftSource->GetTotalFrequency() * 1.0) / log(2.0));
	if (pow(2.0, nGranularityMax) == kwftSource->GetTotalFrequency() and nGranularityMax > 1)
		nGranularityMax--;

	// Affichage
	if (bDisplayResults)
		cout << " N \t" << kwftSource->GetTotalFrequency() << "\t Gmax \t" << nGranularityMax << endl;

	// Initialisation
	nGranularityIndex = 1;
	nMaxPartileNumber = 0;
	bSingleton = false;
	// Initialisation du vecteur d'effectifs de la table
	for (nSourceIndex = 0; nSourceIndex < kwftSource->GetFrequencyVectorNumber(); nSourceIndex++)
	{
		ivInputFrequencies.Add(kwftSource->GetFrequencyVectorAt(nSourceIndex)->ComputeTotalFrequency());
		if (ivInputFrequencies.GetAt(ivInputFrequencies.GetSize() - 1) > 1)
			nMaxPartileNumber++;
		else
			bSingleton = true;
	}
	// Cas de la presence d'un groupe fourre-tout dans la partition a la granularite maximale
	if (bSingleton)
		nMaxPartileNumber++;

	// Initialisation du quantileBuilder utilise pour chaque granularisation
	quantileBuilder.InitializeFrequencies(&ivInputFrequencies);

	// Initialisation
	nCurrentExploredGranularity = -1;
	nLastExploredGranularity = -1;

	// Parcours des granularites
	while (nGranularityIndex <= nGranularityMax and not bIsLastGranularity)
	{
		// Arret si interruption utilisateur
		if (TaskProgression::IsInterruptionRequested())
		{
			// Initialisation de la table finale a la table du modele nul
			if (kwftTarget != NULL)
				delete kwftTarget;
			kwftTarget = new KWFrequencyTable;
			kwftTarget->ComputeNullTable(kwftSource);

			if (ivGroups != NULL)
				delete ivGroups;
			ivGroups = new IntVector;
			ivGroups->SetSize(kwftSource->GetFrequencyVectorNumber());

			break;
		}
		if (bDisplayResults)
			cout << "Granularite " << nGranularityIndex << endl;

		// Transformation de la table selon la granularite
		GranularizeFrequencyTable(kwftSource, kwftGranularizedTable, nGranularityIndex, &quantileBuilder);

		// Arret du parcours si derniere granularite ou pas en mode granularite
		nCurrentPartileNumber = kwftGranularizedTable->GetFrequencyVectorNumber();

		// Test s'il s'agit de la derniere granularite a traiter :
		// - si le nombre de partiles est maximale
		// - si la granularite est maximale
		// - si l'effectif minimum avant le fourre tout est inferieur a l'effectif minimum demande par
		// l'utilisateur
		bIsLastGranularity = (nCurrentPartileNumber == nMaxPartileNumber) or
				     (nGranularityIndex == nGranularityMax) or
				     ((int)ceil(kwftSource->GetTotalFrequency() * 1.0 / pow(2, nGranularityIndex)) <
				      GetMinGroupFrequency());

		// Test d'une granularite eligible
		// Le nombre de partiles pour la granularite courante est il :
		// - superieur d'au moins un facteur dRequiredIncreasingCoefficient au nombre de partiles pour la
		// derniere granularite traitee ET
		// - inferieur d'au moins un facteur dRequiredIncreasingCoefficient au nombre de partiles max calcule
		// prealablement
		bIsGranularitySelected =
		    (nCurrentPartileNumber >= nPreviousPartileNumber * dRequiredIncreasingCoefficient) and
		    (nCurrentPartileNumber * dRequiredIncreasingCoefficient <= nMaxPartileNumber);

		// Cas du traitement de la granularite courante
		if (bIsGranularitySelected or bIsLastGranularity)
		{
			// Memorisation de la derniere granularite exploree
			nLastExploredGranularity = nCurrentExploredGranularity;
			nCurrentExploredGranularity = nGranularityIndex;

			if (bDisplayResults)
			{
				// cout << " Table avant groupage " << *kwftSource;
				cout << "Poubelle potentielle "
				     << (nCurrentPartileNumber >
					 KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage())
				     << endl;
			}

			// Groupage de la table d'effectifs source
			GroupFrequencyTable(kwftGranularizedTable, kwftOptimizedGranularizedTable,
					    ivGranularizedGroups);
			delete kwftGranularizedTable;
			kwftGranularizedTable = NULL;

			if (bDisplayResults)
			{
				// cout << " Table groupee apres GroupFrequencyTable " << endl;
				// cout << *kwftOptimizedGranularizedTable;
			}

			dCost = ComputeGroupingCost(kwftOptimizedGranularizedTable, nCurrentPartileNumber);

			// Cas de l'amelioration du cout
			if (dCost < dBestCost)
			{
				// Memorisation du cout optimal
				dBestCost = dCost;

				// Destruction de l'optimum precedent
				if (kwftTarget != NULL)
					delete kwftTarget;
				// Memorisation du nouvel optimum
				kwftTarget = new KWFrequencyTable;
				kwftTarget->CopyFrom(kwftOptimizedGranularizedTable);

				if (ivGroups != NULL)
					delete ivGroups;
				ivGroups = new IntVector;
				ivGroups->CopyFrom(ivGranularizedGroups);
			}

			if (bDisplayResults)
			{
				cout << "Granularite\tPrep. cost\tConstr. cost\tData cost\tTotal cost\tGroups" << endl;
				cout << nGranularityIndex << "\t"
				     << ComputeGroupingPreparationCost(kwftOptimizedGranularizedTable,
								       nCurrentPartileNumber)
				     << "\t"
				     << ComputeGroupingConstructionCost(kwftOptimizedGranularizedTable,
									nCurrentPartileNumber)
				     << "\t"
				     << ComputeGroupingDataCost(kwftOptimizedGranularizedTable, nCurrentPartileNumber)
				     << "\t"
				     << ComputeGroupingCost(kwftOptimizedGranularizedTable, nCurrentPartileNumber)
				     << "\t" << kwftOptimizedGranularizedTable->GetFrequencyVectorNumber() << endl;
				cout << " Table groupee " << endl;
				// cout << *kwftOptimizedGranularizedTable << endl;
			}

			// Memorisation de la taille de la table granularisee qui vient d'etre traitee
			nPreviousPartileNumber = nCurrentPartileNumber;

			// Nettoyage
			delete ivGranularizedGroups;
			delete kwftOptimizedGranularizedTable;
			ivGranularizedGroups = NULL;
			kwftOptimizedGranularizedTable = NULL;
		}
		// Sinon : la table granularisee est trop proche de la precedente et on ne la traite pas pour gagner du
		// temps
		else
		{
			delete kwftGranularizedTable;
			kwftGranularizedTable = NULL;
		}

		nGranularityIndex++;
	}

	// Post-optimisation de la granularite : on attribue a la partition optimale la plus petite granularite pour
	// laquelle cette partition est definie
	if (nLastExploredGranularity != -1 and kwftTarget->GetGranularity() > nLastExploredGranularity + 1)
		PostOptimizeGranularity(kwftTarget, ivGroups, &quantileBuilder, nLastExploredGranularity);

	if (bDisplayFinalResults)
	{
		cout << "Meilleure granularite groupage = " << kwftTarget->GetGranularity() << " sur "
		     << nGranularityMax << endl;
		cout << "Nombre de modalites dans le groupe poubelle " << kwftTarget->GetGarbageModalityNumber()
		     << endl;
		cout << "Meilleure table groupe a la fin de GroupPreprocessedTable" << *kwftTarget;
	}
	assert(kwftTarget != NULL);
	assert(ivGroups != NULL);
}

void KWGrouperMODL::GroupFrequencyTable(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget,
					IntVector*& ivGroups) const
{
	require(kwftSource != NULL);
	require(kwftSource->GetFrequencyVectorSize() > 0);

	// Initialisation des donnees de travail
	InitializeWorkingData(kwftSource, kwftSource->GetFrequencyVectorNumber());

	// Cas particulier avec moins de 3 modalites a grouper
	// Pas d'introduction d'un eventuel groupe poubelle dans ce cas
	if (kwftSource->GetFrequencyVectorNumber() <= 3)
		SmallSourceNumberGroup(kwftSource, kwftTarget, ivGroups);
	// Cas particulier avec deux classes cibles
	else if (kwftSource->GetFrequencyVectorSize() == 2)
		TwoClassesGroup(kwftSource, kwftTarget, ivGroups);
	// Cas general
	else
	{
		// Cas sans recherche de groupe poubelle :
		// - nombre initial de modalites inferieur au seuil de rentabilite d'un groupe poubelle
		// - mode sans poubelle
		// - nombre final de groupes demande <=2 ce qui exclut les partitions avec groupe poubelle qui doivent
		// contenir une partition informative (2 groupes) + le groupe poubelle
		if (kwftSource->GetFrequencyVectorNumber() <
			KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage() or
		    GetMaxGroupNumber() == 2 or GetMaxGroupNumber() == 1)
			MultipleClassesGroup(kwftSource, kwftTarget, ivGroups);
		// Sinon : recherche d'une partition avec ou sans groupe poubelle
		else
			MultipleClassesGroupWithGarbageSearch(kwftSource, kwftTarget, ivGroups);
	}

	// Nettoyage des donnees de travail
	CleanWorkingData();
}

/////////////////////////////////////////////////////////////////////////

void KWGrouperMODL::SmallSourceNumberGroup(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget,
					   IntVector*& ivGroups) const
{
	double dCostOneGroup;
	KWFrequencyVector* workingFrequencyVector;
	int nSource;
	// Si on est en mode poubelle et dans cette methode, c'est que le nombre initial de modalites est <=3 donc V_G <
	// V_min Pas assez de modalites pour rechercher un groupe poubelle
	int nGarbageModalityNumber = 0;

	require(kwftSource != NULL);
	require(kwftSource->GetTotalFrequency() > 0);
	require(kwftSource->GetFrequencyVectorNumber() >= 1);
	require(kwftSource->GetFrequencyVectorNumber() <= 3);
	require(kwftSource->GetFrequencyVectorSize() > 1);
	require(kwftSource->GetFrequencyVectorNumber() <= GetInitialValueNumber());

	// Creation du vecteur d'effectif de travail
	workingFrequencyVector = GetFrequencyVectorCreator()->Create();
	InitializeFrequencyVector(workingFrequencyVector);

	// Evaluation du cout avec un seul groupe
	for (nSource = 0; nSource < kwftSource->GetFrequencyVectorNumber(); nSource++)
	{
		if (nSource == 0)
			workingFrequencyVector->CopyFrom(kwftSource->GetFrequencyVectorAt(nSource));
		else
			AddFrequencyVector(workingFrequencyVector, kwftSource->GetFrequencyVectorAt(nSource));
	}
	dCostOneGroup = ComputePartitionCost(1, nGarbageModalityNumber) + ComputeGroupCost(workingFrequencyVector);

	// Cas particulier ou il n'y a qu'une valeur source
	if (kwftSource->GetFrequencyVectorNumber() <= 1)
	{
		kwftTarget = kwftSource->Clone();
		ivGroups = new IntVector;
		ivGroups->SetSize(kwftSource->GetFrequencyVectorNumber());
	}
	// Cas avec deux valeurs sources
	else if (kwftSource->GetFrequencyVectorNumber() == 2)
	{
		double dCostGroup0;
		double dCostGroup1;
		double dCostTwoGroups;

		// Evaluation de la solution en deux groupes
		dCostGroup0 = ComputeGroupCost(kwftSource->GetFrequencyVectorAt(0));
		dCostGroup1 = ComputeGroupCost(kwftSource->GetFrequencyVectorAt(1));
		dCostTwoGroups = ComputePartitionCost(2, nGarbageModalityNumber) + dCostGroup0 + dCostGroup1;

		// Cas ou la meilleure solution a un seul groupe
		if (dCostOneGroup < dCostTwoGroups + dEpsilon or GetMaxGroupNumber() == 1)
		{
			kwftTarget = new KWFrequencyTable;
			kwftTarget->SetFrequencyVectorCreator(kwftSource->GetFrequencyVectorCreator()->Clone());
			kwftTarget->Initialize(1);
			kwftTarget->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
			kwftTarget->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
			// Parametrage granularite et poubelle
			kwftTarget->SetGranularity(kwftSource->GetGranularity());
			kwftTarget->SetGarbageModalityNumber(kwftSource->GetGarbageModalityNumber());
			InitializeFrequencyVector(kwftTarget->GetFrequencyVectorAt(0));
			MergeFrequencyVectors(kwftTarget->GetFrequencyVectorAt(0), kwftSource->GetFrequencyVectorAt(0),
					      kwftSource->GetFrequencyVectorAt(1));
			ivGroups = new IntVector;
			ivGroups->SetSize(2);
			ivGroups->SetAt(0, 0);
			ivGroups->SetAt(1, 0);
		}
		// Sinon, la meilleure solution a deux groupes
		else
		{
			kwftTarget = kwftSource->Clone();
			ivGroups = new IntVector;
			ivGroups->SetSize(2);
			ivGroups->SetAt(0, 0);
			ivGroups->SetAt(1, 1);
		}
	}
	// Cas avec trois valeurs sources
	else
	{
		double dCostTwoGroups;
		int nBestTwoGroupsIndex;
		double dCostThreeGroups;
		double dCostGroup0;
		double dCostGroup1;
		double dCostGroup2;
		double dCost;
		double dBestCost;

		assert(kwftSource->GetFrequencyVectorNumber() == 3);

		// Evaluation de la solution en trois groupes
		dCostGroup0 = ComputeGroupCost(kwftSource->GetFrequencyVectorAt(0));
		dCostGroup1 = ComputeGroupCost(kwftSource->GetFrequencyVectorAt(1));
		dCostGroup2 = ComputeGroupCost(kwftSource->GetFrequencyVectorAt(2));
		dCostThreeGroups =
		    ComputePartitionCost(3, nGarbageModalityNumber) + dCostGroup0 + dCostGroup1 + dCostGroup2;

		// Evaluation de la solution en deux groupes
		dBestCost = DBL_MAX;
		nBestTwoGroupsIndex = -1;
		// Goupage (0) + (1,2)
		MergeFrequencyVectors(workingFrequencyVector, kwftSource->GetFrequencyVectorAt(1),
				      kwftSource->GetFrequencyVectorAt(2));
		dCost = ComputeGroupCost(workingFrequencyVector) + dCostGroup0;
		if (dCost < dBestCost)
		{
			dBestCost = dCost;
			nBestTwoGroupsIndex = 0;
		}
		// Goupage (1) + (0,2)
		MergeFrequencyVectors(workingFrequencyVector, kwftSource->GetFrequencyVectorAt(0),
				      kwftSource->GetFrequencyVectorAt(2));
		dCost = ComputeGroupCost(workingFrequencyVector) + dCostGroup1;
		if (dCost < dBestCost)
		{
			dBestCost = dCost;
			nBestTwoGroupsIndex = 1;
		}
		// Goupage (2) + (0,1)
		MergeFrequencyVectors(workingFrequencyVector, kwftSource->GetFrequencyVectorAt(0),
				      kwftSource->GetFrequencyVectorAt(1));
		dCost = ComputeGroupCost(workingFrequencyVector) + dCostGroup2;
		if (dCost < dBestCost)
		{
			dBestCost = dCost;
			nBestTwoGroupsIndex = 2;
		}
		dCostTwoGroups = ComputePartitionCost(2, nGarbageModalityNumber) + dBestCost;

		// Cas ou la meilleure solution a un seul groupe
		if ((dCostOneGroup < dCostTwoGroups + dEpsilon and dCostOneGroup < dCostThreeGroups + dEpsilon) or
		    GetMaxGroupNumber() == 1)
		{
			kwftTarget = new KWFrequencyTable;
			kwftTarget->SetFrequencyVectorCreator(kwftSource->GetFrequencyVectorCreator()->Clone());
			kwftTarget->Initialize(1);
			// Parametrage granularite et poubelle
			kwftTarget->SetGranularity(kwftSource->GetGranularity());
			kwftTarget->SetGarbageModalityNumber(kwftSource->GetGarbageModalityNumber());
			kwftTarget->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
			kwftTarget->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
			InitializeFrequencyVector(kwftTarget->GetFrequencyVectorAt(0));
			MergeFrequencyVectors(kwftTarget->GetFrequencyVectorAt(0), kwftSource->GetFrequencyVectorAt(0),
					      kwftSource->GetFrequencyVectorAt(1));
			AddFrequencyVector(kwftTarget->GetFrequencyVectorAt(0), kwftSource->GetFrequencyVectorAt(2));
			ivGroups = new IntVector;
			ivGroups->SetSize(3);
			ivGroups->SetAt(0, 0);
			ivGroups->SetAt(1, 0);
			ivGroups->SetAt(2, 0);
		}
		// Cas ou la meilleure solution a deux groupes
		else if (dCostTwoGroups < dCostThreeGroups + dEpsilon or GetMaxGroupNumber() == 2)
		{
			kwftTarget = new KWFrequencyTable;
			kwftTarget->SetFrequencyVectorCreator(kwftSource->GetFrequencyVectorCreator()->Clone());
			kwftTarget->Initialize(2);
			kwftTarget->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
			kwftTarget->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
			// Parametrage granularite et poubelle
			kwftTarget->SetGranularity(kwftSource->GetGranularity());
			kwftTarget->SetGarbageModalityNumber(kwftSource->GetGarbageModalityNumber());
			kwftTarget->GetFrequencyVectorAt(0)->CopyFrom(
			    kwftSource->GetFrequencyVectorAt(nBestTwoGroupsIndex));
			InitializeFrequencyVector(kwftTarget->GetFrequencyVectorAt(1));
			MergeFrequencyVectors(kwftTarget->GetFrequencyVectorAt(1),
					      kwftSource->GetFrequencyVectorAt((nBestTwoGroupsIndex + 1) % 3),
					      kwftSource->GetFrequencyVectorAt((nBestTwoGroupsIndex + 2) % 3));
			ivGroups = new IntVector;
			ivGroups->SetSize(3);
			ivGroups->SetAt(0, 1);
			ivGroups->SetAt(1, 1);
			ivGroups->SetAt(2, 1);
			ivGroups->SetAt(nBestTwoGroupsIndex, 0);
		}
		// Sinon, la meilleure solution a trois groupes
		else
		{
			kwftTarget = kwftSource->Clone();
			ivGroups = new IntVector;
			ivGroups->SetSize(3);
			ivGroups->SetAt(0, 0);
			ivGroups->SetAt(1, 1);
			ivGroups->SetAt(2, 2);
		}
	}

	// Nettoyage
	delete workingFrequencyVector;

	ensure(kwftTarget->GetFrequencyVectorNumber() <= kwftSource->GetFrequencyVectorNumber());
	ensure(kwftTarget->GetFrequencyVectorSize() == kwftSource->GetFrequencyVectorSize());
	ensure(kwftTarget->GetTotalFrequency() == kwftSource->GetTotalFrequency());
	ensure(ivGroups->GetSize() == kwftSource->GetFrequencyVectorNumber());
}

/////////////////////////////////////////////////////////////////////////

void KWGrouperMODL::TwoClassesGroup(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget,
				    IntVector*& ivGroups) const
{
	KWGrouperMODLTwoClasses grouperMODLTwoClasses;
	KWFrequencyTable* kwftPreprocessedSource;
	IntVector* ivSourceToPreprocessedIndexes;
	IntVector* ivPreprocessedToGroups;
	int nSource;
	boolean bDisplayResults = false;
	boolean bMergePureSourceValues = true;

	require(kwftSource != NULL);
	require(kwftSource->GetFrequencyVectorSize() == 2);

	// Mode sans recherche de poubelle
	if (bMergePureSourceValues)
	{
		// Preprocessing pour fusionner les lignes pures
		MergePureSourceValues(kwftSource, kwftPreprocessedSource, ivSourceToPreprocessedIndexes);
	}

	// Sinon (mode recherche poubelle interne) : on ne veut pas de pretraitement = fusion des modalites pures car le
	// critere minimal avec poubelle
	//  ne conduit pas obligatoirement a ce que les modalites pures soient dans le meme groupe. On peut avoir une
	//  modalite pure dans le groupe poubelle et une modalite pure dans un autre groupe
	else
	{
		kwftPreprocessedSource = kwftSource->Clone();
		ivSourceToPreprocessedIndexes = new IntVector;
		ivSourceToPreprocessedIndexes->SetSize(kwftSource->GetFrequencyVectorNumber());
		for (nSource = 0; nSource < ivSourceToPreprocessedIndexes->GetSize(); nSource++)
			ivSourceToPreprocessedIndexes->SetAt(nSource, nSource);
	}

	// Parametrage du grouper
	grouperMODLTwoClasses.SetMaxIntervalNumber(GetMaxGroupNumber());
	grouperMODLTwoClasses.GetDiscretizationCosts()->CopyFrom(GetGroupingCosts());

	// Groupage de la table preprocessee
	grouperMODLTwoClasses.Group(kwftPreprocessedSource, groupingCosts->GetValueNumber(), kwftTarget,
				    ivPreprocessedToGroups);
	if (bDisplayResults)
	{
		cout << " Cout de la table groupee par grouperMODLTwoClasses " << ComputePartitionGlobalCost(kwftTarget)
		     << "\t Taille partition " << kwftTarget->GetFrequencyVectorNumber() << endl;
		kwftTarget->Write(cout);
	}

	// Affectation des index de groupes pour la table initiale
	ivGroups = ivSourceToPreprocessedIndexes;
	for (nSource = 0; nSource < kwftSource->GetFrequencyVectorNumber(); nSource++)
		ivGroups->SetAt(nSource, ivPreprocessedToGroups->GetAt(ivSourceToPreprocessedIndexes->GetAt(nSource)));

	assert(fabs(ComputePartitionGlobalCost(kwftTarget) -
		    grouperMODLTwoClasses.ComputePartitionGlobalCost(kwftTarget)) < dEpsilon);

	// Nettoyage
	if (kwftPreprocessedSource != NULL)
		delete kwftPreprocessedSource;
	delete ivPreprocessedToGroups;
}

/////////////////////////////////////////////////////////////////////////

void KWGrouperMODL::MultipleClassesGroup(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget,
					 IntVector*& ivGroups) const
{
	boolean bPrintResults = false;
	boolean bPrintGroupNumbers = false;
	boolean bPreprocessPureGroups = true;
	boolean bBuildReliableSubGroups = true;
	boolean bBuildFewGroups = true;
	boolean bPostOptimization = true;
	boolean bPreprocessing;
	boolean bFewGroups;
	KWFrequencyTable* kwftPreprocessedSource;
	ObjectArray* oaInitialGroups;
	ObjectArray* oaInitialGroupMerges;
	ObjectArray* oaNewGroups;
	IntVector* ivSourceToPreprocessedIndexes;
	IntVector* ivPreprocessedToSubGroupsIndexes;
	IntVector* ivFewerInitialIndexes;
	IntVector* ivOptimizedIndexes;
	IntVector* ivPreprocessedToOptimizedIndexes;
	int i;
	int nMinFrequency;
	int nTotalFrequency;

	require(kwftSource != NULL);
	require(kwftSource->GetFrequencyVectorSize() > 1);
	require(kwftSource->GetFrequencyVectorNumber() < KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage() or
		GetMaxGroupNumber() == 2 or GetMaxGroupNumber() == 1);

	// Affichage de la table initiale
	if (bPrintResults)
		cout << "Initial table\n" << *kwftSource << endl;

	// Affichage des statistiques initiales et du nombre de groupes
	if (bPrintGroupNumbers)
		cout << kwftSource->GetTotalFrequency() << "\t" << kwftSource->GetFrequencyVectorNumber() << "\t";

	//////////////////////////////////////////////////////////////////////////
	// Preprocessing 1: on transforme la table de contingence source en une
	// table preprocesses contenant des "sous-groupes" initiaux inseccables,
	// qui seront attribues en blocs a un groupe
	// Cela permet de reduire le nombre de valeurs initial pour les algorithmes
	// d'optimisation et de post-optimisation utilises ensuite
	//    Input: kwftSource
	//    Output: kwftPreprocessedSource (obligatoire)
	//            ivSourceToPreprocessedIndexes (facultatif)

	// Preprocessing: fusion les groupes purs ayant meme classe cible
	bPreprocessing = false;
	ivSourceToPreprocessedIndexes = NULL;
	kwftPreprocessedSource = NULL;
	if (bPreprocessPureGroups)
	{
		bPreprocessing = true;
		MergePureSourceValues(kwftSource, kwftPreprocessedSource, ivSourceToPreprocessedIndexes);
	}
	// Sinon, la table initiale est consideree comme la table preprocessee
	else
		kwftPreprocessedSource = kwftSource;
	assert(kwftPreprocessedSource != NULL);
	assert(bPreprocessPureGroups == (ivSourceToPreprocessedIndexes != NULL));

	// Affichage du nombre de groupes
	if (bPrintGroupNumbers)
		cout << "Nombre de groupes de la table preprocessee "
		     << kwftPreprocessedSource->GetFrequencyVectorNumber() << "\t";

	//////////////////////////////////////////////////////////////////////////
	// Preprocessing 2: on identifie les sous-groupes stables par l'ensemble
	// des groupages bi-classe afin de reduire encore le nombre de groupes
	// initiaux en nuisant le moins possible a la qualite du groupage final
	//    Input: kwftPreprocessedSource
	//    Output: oaInitialGroups (obligatoire)
	//            ivPreprocessedToSubGroupsIndexes (facultatif)

	// Test si on a encore un nombre trop important de valeurs a grouper
	ivPreprocessedToSubGroupsIndexes = NULL;
	oaInitialGroups = NULL;
	if (bBuildReliableSubGroups)
	{
		// Construction des sous-groupes stables en partant des sous-groupes issus
		// de la premiere etape de preprocessing.
		oaInitialGroups = BuildReliableSubGroups(kwftPreprocessedSource, ivPreprocessedToSubGroupsIndexes);
	}
	// Sinon, les groupes initiaux sont constitues directement depuis la table preprocessee
	else
		oaInitialGroups = BuildGroups(kwftPreprocessedSource);
	assert(oaInitialGroups != NULL);
	assert(bBuildReliableSubGroups == (ivPreprocessedToSubGroupsIndexes != NULL));

	// Affichage du nombre de groupes
	if (bPrintGroupNumbers)
		cout << "Nbre de gpes apres preprocessing 2 " << oaInitialGroups->GetSize() << "\t";

	////////////////////////////////////////////////////////////////////////////
	// Creation des groupes initiaux, et reduction eventuelle de leur nombre
	// pour obtenir un nombre faible de groupes (Few)
	//    Input: oaInitialGroups
	//    Output: oaInitialGroups (obligatoire)
	//            ivFewerInitialIndexes (facultatif)

	// Test si on a encore un nombre trop important de valeurs a grouper
	bFewGroups = false;
	ivFewerInitialIndexes = NULL;
	nTotalFrequency = kwftSource->GetTotalFrequency();
	if (bBuildFewGroups and oaInitialGroups->GetSize() > sqrt(1.0 * nTotalFrequency))
	{
		bFewGroups = true;

		// Calcul du seuil de frequence minimum des groupes a fusionner pour obtenir
		// au final un nombre limite de groupes
		nMinFrequency =
		    ComputeMinGroupFrequency(oaInitialGroups, nTotalFrequency, (int)ceil(sqrt(1.0 * nTotalFrequency)));

		// Fusion des petits groupes
		MergeSmallGroups(oaInitialGroups, nMinFrequency, false, ivFewerInitialIndexes);
	}
	assert(oaInitialGroups != NULL);
	assert(bFewGroups == (ivFewerInitialIndexes != NULL));

	// Affichage du nombre de groupes
	if (bPrintGroupNumbers)
		cout << "Nbre de groupes apres Merge " << oaInitialGroups->GetSize() << "\t";

	////////////////////////////////////////////////////////////////////////////
	// Algorithme principal d'optimisation du groupage
	//    Input: oaInitialGroups, oaInitialGroupMerges
	//    Output: oaNewGroups (obligatoire)
	//            ivOptimizedIndexes (obligatoire)

	// Initialisation des merges
	oaInitialGroupMerges = BuildGroupMerges(oaInitialGroups);

	// Recherche des groupes terminaux
	ivOptimizedIndexes = NULL;
	OptimizeGroups(0, oaInitialGroups, oaInitialGroupMerges, oaNewGroups, ivOptimizedIndexes);

	// Fabrication de la table de contingence
	kwftTarget = BuildTable(oaNewGroups);
	// Parametrage nInitialValueNumber, nGranularizedValueNumber
	kwftTarget->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
	kwftTarget->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
	// Parametrage granularite et poubelle
	kwftTarget->SetGranularity(kwftSource->GetGranularity());
	kwftTarget->SetGarbageModalityNumber(0);
	oaNewGroups->DeleteAll();

	// Calcul des index entre les groupe preprocesses et les groupes optimises
	// Cas ou il y a eu construction de sous-groupes puis reduction de leur nombre
	ivPreprocessedToOptimizedIndexes = NULL;
	if (bBuildReliableSubGroups and bFewGroups)
	{
		ivPreprocessedToOptimizedIndexes = ivPreprocessedToSubGroupsIndexes;
		for (i = 0; i < ivPreprocessedToOptimizedIndexes->GetSize(); i++)
			ivPreprocessedToOptimizedIndexes->SetAt(
			    i, ivOptimizedIndexes->GetAt(
				   ivFewerInitialIndexes->GetAt(ivPreprocessedToSubGroupsIndexes->GetAt(i))));
		delete ivFewerInitialIndexes;
		delete ivOptimizedIndexes;
	}
	// Cas ou il il y a eu construction de sous-groupes
	else if (bBuildReliableSubGroups and not bFewGroups)
	{
		ivPreprocessedToOptimizedIndexes = ivPreprocessedToSubGroupsIndexes;
		for (i = 0; i < ivPreprocessedToOptimizedIndexes->GetSize(); i++)
			ivPreprocessedToOptimizedIndexes->SetAt(
			    i, ivOptimizedIndexes->GetAt(ivPreprocessedToSubGroupsIndexes->GetAt(i)));
		delete ivOptimizedIndexes;
		assert(ivFewerInitialIndexes == NULL);
	}
	// Cas ou il il y a eu reduction du nombre de groupe
	else if (not bBuildReliableSubGroups and bFewGroups)
	{
		ivPreprocessedToOptimizedIndexes = ivFewerInitialIndexes;
		for (i = 0; i < ivPreprocessedToOptimizedIndexes->GetSize(); i++)
			ivPreprocessedToOptimizedIndexes->SetAt(
			    i, ivOptimizedIndexes->GetAt(ivFewerInitialIndexes->GetAt(i)));
		delete ivOptimizedIndexes;
		assert(ivPreprocessedToSubGroupsIndexes == NULL);
	}
	// Cas ou il n'y a rien eu depuis la preoptimisation
	else
	{
		ivPreprocessedToOptimizedIndexes = ivOptimizedIndexes;
		assert(ivPreprocessedToSubGroupsIndexes == NULL);
		assert(ivFewerInitialIndexes == NULL);
	}
	assert(ivPreprocessedToOptimizedIndexes != NULL);
	assert(ivPreprocessedToOptimizedIndexes->GetSize() == kwftPreprocessedSource->GetFrequencyVectorNumber());

	// Nettoyage
	delete oaInitialGroups;
	delete oaInitialGroupMerges;
	delete oaNewGroups;

	// Affichage du nombre de groupes
	if (bPrintGroupNumbers)
		cout << kwftTarget->GetFrequencyVectorNumber() << "\t";

	//////////////////////////////////////////////////////////////////////////
	// Post-optimisation a partir des sous-groupes issus du preprocessing

	// Post-optimisation du groupage
	if (bPostOptimization)
		PostOptimizeGrouping(kwftPreprocessedSource, kwftTarget, ivPreprocessedToOptimizedIndexes);

	// Destruction de la table preprocessee
	if (bPreprocessing)
	{
		assert(kwftPreprocessedSource != kwftSource);
		delete kwftPreprocessedSource;
	}

	// Affichage du nombre de groupes
	if (bPrintGroupNumbers)
		cout << kwftTarget->GetFrequencyVectorNumber() << "\t";

	/////////////////////////////////////////////////////////////////////////
	// Finalisation des resultats

	// Calcul des index des groupes pour les valeurs sources initiales
	// dans le cas ou il y a eu preprocessing intermediaire
	if (bPreprocessing)
	{
		ivGroups = new IntVector;
		ivGroups->SetSize(kwftSource->GetFrequencyVectorNumber());
		for (i = 0; i < ivGroups->GetSize(); i++)
			ivGroups->SetAt(
			    i, ivPreprocessedToOptimizedIndexes->GetAt(ivSourceToPreprocessedIndexes->GetAt(i)));
		delete ivSourceToPreprocessedIndexes;
		delete ivPreprocessedToOptimizedIndexes;
	}
	// Sinon, les index sont deja disponibles
	else
	{
		ivGroups = ivPreprocessedToOptimizedIndexes;
		assert(ivSourceToPreprocessedIndexes == NULL);
	}

	// Affichage de la table finale
	if (bPrintResults)
		cout << "Grouped table\n" << *kwftTarget << endl;

	// Affichage du cout MODL
	if (bPrintGroupNumbers)
		cout << ComputePartitionGlobalCost(kwftTarget) << endl;

	ensure(ivGroups->GetSize() == kwftSource->GetFrequencyVectorNumber());
	ensure(kwftSource->GetTotalFrequency() == kwftTarget->GetTotalFrequency());
}

int KWGrouperMODL::OptimizeGroups(int nExactGroupNumber, ObjectArray* oaInitialGroups,
				  ObjectArray* oaInitialGroupMerges, ObjectArray*& oaNewGroups,
				  IntVector*& ivGroups) const
{
	boolean bPrintInitialGroups = false;
	boolean bPrintMerges = false;
	boolean bPrintNewGroups = false;
	int nOptimumGroupNumber;
	SortedList groupMergeList(KWMODLGroupMergeCompare);
	KWMODLGroup* group1;
	KWMODLGroup* group2;
	KWMODLGroup* group;
	KWMODLGroupMerge* bestGroupMerge;
	KWMODLGroupMerge* groupMerge;
	IntVector ivActiveGroupIndexes;
	IntVector ivNewIndexes;
	int i;
	int j;
	int nIndex;
	int nNewIndex;
	int nLastActiveGroupIndex;
	int nGroupNumber;
	double dUnionCost;
	double dDeltaCost;
	double dTotalDeltaCost;
	double dBestTotalDeltaCost;
	boolean bContinue;
	// On est dans une methode d'optimisation sans recherche de poubelle
	int nGarbageModalityNumber = 0;

	require(oaInitialGroups != NULL);
	require(oaInitialGroups->GetSize() <= GetInitialValueNumber());
	require(oaInitialGroupMerges != NULL);
	require(nExactGroupNumber == 0 or (nExactGroupNumber >= 1 and nExactGroupNumber <= oaInitialGroups->GetSize()));

	// Affichage des groupes initiaux
	if (bPrintInitialGroups)
	{
		cout << "Debut OptimizeGroups : affichage groupes initiaux" << endl;
		for (i = 0; i < oaInitialGroups->GetSize(); i++)
			cout << *cast(KWMODLGroup*, oaInitialGroups->GetAt(i));
	}

	// Initialisation du vecteur d'index
	ivGroups = new IntVector;
	ivGroups->SetSize(oaInitialGroups->GetSize());
	for (i = 0; i < ivGroups->GetSize(); i++)
		ivGroups->SetAt(i, i);

	// Insertion des fusions de groupes dans une liste triee
	// par valeur decroissant
	for (i = 0; i < oaInitialGroups->GetSize(); i++)
	{
		for (j = 0; j < i; j++)
		{
			groupMerge = GetGroupMergeAt(oaInitialGroupMerges, oaInitialGroups->GetSize(), i, j);

			// Ajout de la fusion dans la liste triee, et memorisation de sa position
			groupMerge->SetPosition(groupMergeList.Add(groupMerge));
		}
	}

	// Initialisation du tableau des index des groupes en cours
	ivActiveGroupIndexes.SetSize(oaInitialGroups->GetSize());
	for (i = 0; i < ivActiveGroupIndexes.GetSize(); i++)
		ivActiveGroupIndexes.SetAt(i, i);

	// Initialisation du tableau des correspondances entre les index initiaux, et les
	// index dans le tableu des groupes actifs
	ivNewIndexes.SetSize(oaInitialGroups->GetSize());
	for (i = 0; i < ivNewIndexes.GetSize(); i++)
		ivNewIndexes.SetAt(i, i);

	// Groupages tant que amelioration
	bContinue = true;
	dTotalDeltaCost = 0;
	dBestTotalDeltaCost = 0;
	nGroupNumber = oaInitialGroups->GetSize();
	nOptimumGroupNumber = nGroupNumber;
	while (bContinue and nGroupNumber >= 2 and nGroupNumber > nExactGroupNumber)
	{
		// Recherche de la meilleure fusion (celle de plus petit DeltaCout)
		assert(groupMergeList.GetCount() == nGroupNumber * (nGroupNumber - 1) / 2);
		bestGroupMerge = cast(KWMODLGroupMerge*, groupMergeList.GetTail());

		// Test si la meilleure fusion doit etre effectuee
		dDeltaCost =
		    bestGroupMerge->GetDeltaCost() + ComputePartitionDeltaCost(nGroupNumber, nGarbageModalityNumber);
		bContinue = dDeltaCost < dEpsilon;

		// Test d'amelioration du meilleur cout total
		dTotalDeltaCost += dDeltaCost;
		if (dTotalDeltaCost < dBestTotalDeltaCost + dEpsilon)
		{
			dBestTotalDeltaCost = dTotalDeltaCost;
			nOptimumGroupNumber = nGroupNumber - 1;
		}

		// Test si contrainte de nombre max de groupes non respectee
		if (GetMaxGroupNumber() > 0 and nGroupNumber > GetMaxGroupNumber())
			bContinue = true;

		// Si nombre de groupes impose
		if (nExactGroupNumber != 0)
			bContinue = nGroupNumber > nExactGroupNumber;

		// Groupage effectif si amelioration
		if (bContinue)
		{
			nGroupNumber--;

			// Acces aux groupes sources de la fusion
			assert(bestGroupMerge->GetIndex1() < bestGroupMerge->GetIndex2());
			group1 = cast(KWMODLGroup*, oaInitialGroups->GetAt(bestGroupMerge->GetIndex1()));
			group2 = cast(KWMODLGroup*, oaInitialGroups->GetAt(bestGroupMerge->GetIndex2()));

			// Affichage de la fusion
			if (bPrintMerges)
			{
				cout << "Merge\t" << dDeltaCost << "\t" << nGroupNumber << "\n"
				     << "\t" << *bestGroupMerge << "\t" << *group1 << "\t" << *group2;
			}

			// Le premier groupe (de plus petit index) devient le support de la fusion
			// Calcul du vecteur d'effectifs du groupe fusionne
			assert(CheckFrequencyVector(group1->GetFrequencyVector()));
			assert(CheckFrequencyVector(group2->GetFrequencyVector()));
			AddFrequencyVector(group1->GetFrequencyVector(), group2->GetFrequencyVector());
			group1->SetCost(ComputeGroupCost(group1->GetFrequencyVector()));

			// Affichage du nouveau groupe
			if (bPrintMerges)
			{
				cout << "\tN" << *group1 << flush;
			}

			// Le deuxieme groupe source aura meme index de groupe terminal que le premier
			ivGroups->SetAt(group2->GetIndex(), group1->GetIndex());

			// Desormais, le deuxieme groupe source n'est plus actif
			// On recherche sa position dans la liste des index actifs (grace
			// a ivNewIndexes), et l'on remplace sa position par celle du dernier
			// groupe actif (dont on doit reactualiser la position dans ivNewIndexes)
			nNewIndex = ivNewIndexes.GetAt(group2->GetIndex());
			assert(ivActiveGroupIndexes.GetAt(nNewIndex) == group2->GetIndex());
			ivNewIndexes.SetAt(group2->GetIndex(), ivActiveGroupIndexes.GetSize() - 1);
			//
			nLastActiveGroupIndex = ivActiveGroupIndexes.GetAt(ivActiveGroupIndexes.GetSize() - 1);
			ivActiveGroupIndexes.SetAt(nNewIndex, nLastActiveGroupIndex);
			ivActiveGroupIndexes.SetSize(ivActiveGroupIndexes.GetSize() - 1);
			ivNewIndexes.SetAt(nLastActiveGroupIndex, nNewIndex);
			assert(ivActiveGroupIndexes.GetSize() == nGroupNumber);

			// On reactualise la liste des fusions, en supprimant de la liste
			// les fusions avec un des groupes sources, et en ajoutant les fusions avec
			// le nouveau groupe fusionne
			for (i = 0; i < ivActiveGroupIndexes.GetSize(); i++)
			{
				nIndex = ivActiveGroupIndexes.GetAt(i);

				// Retrait de la liste des fusions avec le second groupe source
				groupMerge = GetGroupMergeAt(oaInitialGroupMerges, oaInitialGroups->GetSize(),
							     group2->GetIndex(), nIndex);
				groupMergeList.RemoveAt(groupMerge->GetPosition());
				debug(groupMerge->SetPosition(NULL));

				// Retrait de la liste des fusions avec le premier groupe source
				if (nIndex != group1->GetIndex())
				{
					// Retrait de la liste
					groupMerge = GetGroupMergeAt(oaInitialGroupMerges, oaInitialGroups->GetSize(),
								     group1->GetIndex(), nIndex);
					groupMergeList.RemoveAt(groupMerge->GetPosition());
					debug(groupMerge->SetPosition(NULL));

					// Calcul du vecteur d'effectifs du groupe fusionne avec le
					// nouveau groupe (porte par group1)
					group = cast(KWMODLGroup*, oaInitialGroups->GetAt(nIndex));
					dUnionCost = ComputeGroupUnionCost(group1->GetFrequencyVector(),
									   group->GetFrequencyVector());

					// Ajout de la fusion dans la liste triee, et memorisation de sa position
					groupMerge->SetDeltaCost(dUnionCost - group1->GetCost() - group->GetCost());
					groupMerge->SetPosition(groupMergeList.Add(groupMerge));
					assert(groupMerge->GetIndex1() < groupMerge->GetIndex2());
				}
			}
		}
	}

	// Nettoyage des fusions
	oaInitialGroupMerges->DeleteAll();

	// Memorisation des nouveau groupes, et memorisation de leur index dans
	// le nouveau tableau de groupes
	ivNewIndexes.SetSize(ivGroups->GetSize());
	oaNewGroups = new ObjectArray;
	oaNewGroups->SetSize(ivActiveGroupIndexes.GetSize());
	for (i = 0; i < ivActiveGroupIndexes.GetSize(); i++)
	{
		nIndex = ivActiveGroupIndexes.GetAt(i);

		// Transfert du groupe recherche vers le tableau des nouveau groupes
		group = cast(KWMODLGroup*, oaInitialGroups->GetAt(nIndex));
		oaNewGroups->SetAt(i, group);

		// Memorisation de la correspondance entre l'ancien index et le nouveau
		ivNewIndexes.SetAt(nIndex, i);

		// Marquage dans l'ancien tableau (permettant de ne pas le detruire)
		assert(oaInitialGroups->GetAt(nIndex) == group);
		oaInitialGroups->SetAt(nIndex, NULL);
	}

	// Nettoyage des groupes initiaux fusionnes (non transferres)
	oaInitialGroups->DeleteAll();

	// Mise a jour du vecteur d'index: on trouve recursivement
	// l'index du groupe terminal issue des fussions successives du groupe initial
	// Memorisation des index utilises
	for (i = 0; i < ivGroups->GetSize(); i++)
	{
		// Recherche de l'index du groupe
		nIndex = ivGroups->GetAt(i);
		while (ivGroups->GetAt(nIndex) < nIndex)
			nIndex = ivGroups->GetAt(nIndex);

		// Memorisation de l'index du groupe
		assert(0 <= nIndex and nIndex < ivGroups->GetSize());
		ivGroups->SetAt(i, nIndex);
	}

	// Renumerotation des index de groupes
	for (i = 0; i < ivGroups->GetSize(); i++)
	{
		nIndex = ivNewIndexes.GetAt(ivGroups->GetAt(i));
		ivGroups->SetAt(i, nIndex);
	}

	// Affichage des nouveaux groupes
	if (bPrintNewGroups)
	{
		for (i = 0; i < oaNewGroups->GetSize(); i++)
			cout << *cast(KWMODLGroup*, oaNewGroups->GetAt(i));
	}

	ensure(nExactGroupNumber == 0 or nExactGroupNumber == nGroupNumber);
	return nOptimumGroupNumber;
}

void KWGrouperMODL::InitializeWorkingData(const KWFrequencyTable* kwftSource, int nInitialValueNumber) const
{
	boolean bDisplayResults = false;

	require(kwftSource != NULL);
	require(nInitialValueNumber >= 1);

	// Parametrage de la structure de cout
	groupingCosts->SetGranularity(kwftSource->GetGranularity());
	groupingCosts->SetTotalInstanceNumber(kwftSource->GetTotalFrequency());
	groupingCosts->SetValueNumber(nInitialValueNumber);
	groupingCosts->SetClassValueNumber(0);
	if (kwftSource->GetFrequencyVectorNumber() > 0)
		groupingCosts->SetClassValueNumber(cast(KWDenseFrequencyVector*, kwftSource->GetFrequencyVectorAt(0))
						       ->GetFrequencyVector()
						       ->GetSize());

	// Affichage des parametres du grouper
	if (bDisplayResults)
		cout << "KWGrouperMODL::InitializeWorkingData\tGranularity\t" << groupingCosts->GetGranularity()
		     << "\tValueNumber\t" << groupingCosts->GetValueNumber() << endl;
}

void KWGrouperMODL::CleanWorkingData() const
{
	// Parametrage de la structure de cout
	groupingCosts->SetValueNumber(0);
	groupingCosts->SetClassValueNumber(0);
	groupingCosts->SetGranularity(0);
	groupingCosts->SetTotalInstanceNumber(0);
}

void KWGrouperMODL::InitializeFrequencyVector(KWFrequencyVector* kwfvFrequencyVector) const
{
	IntVector* ivFrequencyVector;

	require(kwfvFrequencyVector != NULL);

	// Retaillage avec le bon nombre de classes
	ivFrequencyVector = cast(KWDenseFrequencyVector*, kwfvFrequencyVector)->GetFrequencyVector();
	ivFrequencyVector->SetSize(groupingCosts->GetClassValueNumber());
}

boolean KWGrouperMODL::CheckFrequencyVector(const KWFrequencyVector* kwfvFrequencyVector) const
{
	boolean bOk;
	IntVector* ivFrequencyVector;
	int i;

	require(kwfvFrequencyVector != NULL);

	// Controle de la taille du vecteur
	ivFrequencyVector = cast(KWDenseFrequencyVector*, kwfvFrequencyVector)->GetFrequencyVector();
	bOk = ivFrequencyVector->GetSize() == groupingCosts->GetClassValueNumber();

	// Controle du contenu: effectifs positifs
	if (bOk)
	{
		for (i = 0; i < ivFrequencyVector->GetSize(); i++)
		{
			if (ivFrequencyVector->GetAt(i) < 0)
			{
				bOk = false;
				break;
			}
		}
	}

	return bOk;
}

void KWGrouperMODL::AddFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
				       const KWFrequencyVector* kwfvAddedFrequencyVector) const
{
	IntVector* ivSourceFrequencyVector;
	IntVector* ivAddedFrequencyVector;
	int i;

	require(kwfvSourceFrequencyVector != NULL);
	require(kwfvAddedFrequencyVector != NULL);
	require(CheckFrequencyVector(kwfvSourceFrequencyVector));
	require(CheckFrequencyVector(kwfvAddedFrequencyVector));

	// Acces aux representations denses des vecteurs d'effectifs
	ivSourceFrequencyVector = cast(KWDenseFrequencyVector*, kwfvSourceFrequencyVector)->GetFrequencyVector();
	ivAddedFrequencyVector = cast(KWDenseFrequencyVector*, kwfvAddedFrequencyVector)->GetFrequencyVector();
	assert(ivSourceFrequencyVector->GetSize() == ivAddedFrequencyVector->GetSize());

	// Ajout des effectifs sources
	for (i = 0; i < ivSourceFrequencyVector->GetSize(); i++)
		ivSourceFrequencyVector->UpgradeAt(i, ivAddedFrequencyVector->GetAt(i));

	// Mise a jour du nombre de modalites de la ligne
	kwfvSourceFrequencyVector->SetModalityNumber(kwfvSourceFrequencyVector->GetModalityNumber() +
						     kwfvAddedFrequencyVector->GetModalityNumber());

	ensure(CheckFrequencyVector(kwfvSourceFrequencyVector));
}

void KWGrouperMODL::RemoveFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
					  const KWFrequencyVector* kwfvRemovedFrequencyVector) const
{
	IntVector* ivSourceFrequencyVector;
	IntVector* ivRemovedFrequencyVector;
	int i;

	require(kwfvSourceFrequencyVector != NULL);
	require(kwfvRemovedFrequencyVector != NULL);
	require(CheckFrequencyVector(kwfvSourceFrequencyVector));
	require(CheckFrequencyVector(kwfvRemovedFrequencyVector));
	require(kwfvRemovedFrequencyVector->ComputeTotalFrequency() <=
		kwfvSourceFrequencyVector->ComputeTotalFrequency());

	// Acces aux representations denses des vecteurs d'effectifs
	ivSourceFrequencyVector = cast(KWDenseFrequencyVector*, kwfvSourceFrequencyVector)->GetFrequencyVector();
	ivRemovedFrequencyVector = cast(KWDenseFrequencyVector*, kwfvRemovedFrequencyVector)->GetFrequencyVector();
	assert(ivSourceFrequencyVector->GetSize() == ivRemovedFrequencyVector->GetSize());

	// Supression des effectifs sources
	for (i = 0; i < ivSourceFrequencyVector->GetSize(); i++)
		ivSourceFrequencyVector->UpgradeAt(i, -ivRemovedFrequencyVector->GetAt(i));

	// Mise a jour du nombre de modalites
	kwfvSourceFrequencyVector->SetModalityNumber(kwfvSourceFrequencyVector->GetModalityNumber() -
						     kwfvRemovedFrequencyVector->GetModalityNumber());

	ensure(CheckFrequencyVector(kwfvSourceFrequencyVector));
}

void KWGrouperMODL::MergeFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector,
					  const KWFrequencyVector* kwfvMergedFrequencyVector1,
					  const KWFrequencyVector* kwfvMergedFrequencyVector2) const
{
	IntVector* ivSourceFrequencyVector;
	IntVector* ivMergedFrequencyVector1;
	IntVector* ivMergedFrequencyVector2;
	int i;

	require(kwfvSourceFrequencyVector != NULL);
	require(kwfvMergedFrequencyVector1 != NULL);
	require(kwfvMergedFrequencyVector2 != NULL);
	require(CheckFrequencyVector(kwfvSourceFrequencyVector));
	require(CheckFrequencyVector(kwfvMergedFrequencyVector1));
	require(CheckFrequencyVector(kwfvMergedFrequencyVector2));

	// Acces aux representations denses des vecteurs d'effectifs
	ivSourceFrequencyVector = cast(KWDenseFrequencyVector*, kwfvSourceFrequencyVector)->GetFrequencyVector();
	ivMergedFrequencyVector1 = cast(KWDenseFrequencyVector*, kwfvMergedFrequencyVector1)->GetFrequencyVector();
	ivMergedFrequencyVector2 = cast(KWDenseFrequencyVector*, kwfvMergedFrequencyVector2)->GetFrequencyVector();

	// Transfert des effectifs
	for (i = 0; i < ivSourceFrequencyVector->GetSize(); i++)
	{
		ivSourceFrequencyVector->SetAt(i,
					       ivMergedFrequencyVector1->GetAt(i) + ivMergedFrequencyVector2->GetAt(i));
	}

	// Mise a jour du nombre de modalites du Merge
	kwfvSourceFrequencyVector->SetModalityNumber(kwfvMergedFrequencyVector1->GetModalityNumber() +
						     kwfvMergedFrequencyVector2->GetModalityNumber());

	ensure(CheckFrequencyVector(kwfvSourceFrequencyVector));
	ensure(kwfvSourceFrequencyVector->ComputeTotalFrequency() ==
	       kwfvMergedFrequencyVector1->ComputeTotalFrequency() +
		   kwfvMergedFrequencyVector2->ComputeTotalFrequency());
}

double KWGrouperMODL::ComputeGroupUnionCost(const KWFrequencyVector* sourceGroup1,
					    const KWFrequencyVector* sourceGroup2) const
{
	double dCost;
	IntVector* ivSourceFrequencyVector1;
	IntVector* ivSourceFrequencyVector2;
	int i;

	require(sourceGroup1 != NULL);
	require(sourceGroup2 != NULL);
	require(CheckFrequencyVector(sourceGroup1));
	require(CheckFrequencyVector(sourceGroup2));

	// Acces aux representations denses des vecteurs d'effectifs
	ivSourceFrequencyVector1 = cast(KWDenseFrequencyVector*, sourceGroup1)->GetFrequencyVector();
	ivSourceFrequencyVector2 = cast(KWDenseFrequencyVector*, sourceGroup2)->GetFrequencyVector();
	assert(ivSourceFrequencyVector1->GetSize() == ivSourceFrequencyVector2->GetSize());

	// Cumul des effectifs sources dans le premier vecteur
	for (i = 0; i < ivSourceFrequencyVector1->GetSize(); i++)
		ivSourceFrequencyVector1->UpgradeAt(i, ivSourceFrequencyVector2->GetAt(i));

	// Evaluation du cout de l'union
	dCost = ComputeGroupCost(sourceGroup1);

	// Restitution de l'etat initial
	for (i = 0; i < ivSourceFrequencyVector1->GetSize(); i++)
		ivSourceFrequencyVector1->UpgradeAt(i, -ivSourceFrequencyVector2->GetAt(i));

	return dCost;
}

double KWGrouperMODL::ComputeGroupDiffCost(const KWFrequencyVector* sourceGroup,
					   const KWFrequencyVector* removedGroup) const
{
	double dCost;
	IntVector* ivSourceFrequencyVector;
	IntVector* ivRemovedFrequencyVector;
	int i;

	require(sourceGroup != NULL);
	require(removedGroup != NULL);
	require(CheckFrequencyVector(sourceGroup));
	require(CheckFrequencyVector(removedGroup));

	// Acces aux representations denses des vecteurs d'effectifs
	ivSourceFrequencyVector = cast(KWDenseFrequencyVector*, sourceGroup)->GetFrequencyVector();
	ivRemovedFrequencyVector = cast(KWDenseFrequencyVector*, removedGroup)->GetFrequencyVector();
	assert(ivSourceFrequencyVector->GetSize() == ivRemovedFrequencyVector->GetSize());

	// Utilisation du premier vecteur pour soustraire le second
	for (i = 0; i < ivSourceFrequencyVector->GetSize(); i++)
		ivSourceFrequencyVector->UpgradeAt(i, -ivRemovedFrequencyVector->GetAt(i));

	// Evaluation du cout de la difference
	dCost = ComputeGroupCost(sourceGroup);

	// Restitution de l'etat initial
	for (i = 0; i < ivSourceFrequencyVector->GetSize(); i++)
		ivSourceFrequencyVector->UpgradeAt(i, +ivRemovedFrequencyVector->GetAt(i));

	return dCost;
}

void KWGrouperMODL::MultipleClassesGroupWithGarbageSearch(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget,
							  IntVector*& ivGroups) const
{
	boolean bPrintResults = false;
	boolean bPrintGroupNumbers = false;
	boolean bPrintInitialGroups = false;
	boolean bPreprocessPureGroups = true;
	boolean bBuildReliableSubGroups = true;
	boolean bBuildFewGroups = true;
	boolean bPostOptimization = true;
	boolean bPreprocessing;
	boolean bFewGroups;
	KWFrequencyTable* kwftPreprocessedSource;
	ObjectArray* oaInitialGroups;
	ObjectArray* oaInitialGroupMerges;
	ObjectArray* oaNewGroups;
	IntVector* ivSourceToPreprocessedIndexes;
	IntVector* ivPreprocessedToSubGroupsIndexes;
	IntVector* ivFewerInitialIndexes;
	IntVector* ivOptimizedIndexes;
	IntVector* ivPreprocessedToOptimizedIndexes;
	int i;
	int nMinFrequency;
	int nTotalFrequency;
	boolean bDisplayModalityNumber = false;
	IntVector* ivSourceToEndPreprocesses;
	IntVector* ivOptimumNumbers;
	KWMODLGroup* group;
	int nMaxInitialGroupNumber;

	require(kwftSource != NULL);
	require(kwftSource->GetFrequencyVectorSize() > 1);

	// Affichage de la table initiale
	if (bPrintResults)
		cout << "MultipleClassesGroupWithGarbageSearch : Beginning \tInitial table\n" << *kwftSource << endl;

	// Affichage des statistiques initiales et du nombre de groupes
	if (bPrintGroupNumbers)
		cout << kwftSource->GetTotalFrequency() << "\t" << kwftSource->GetFrequencyVectorNumber() << "\t";

	//////////////////////////////////////////////////////////////////////////
	// Preprocessing 1: on transforme la table de contingence source en une
	// table preprocesse contenant des "sous-groupes" initiaux inseccables,
	// qui seront attribues en blocs a un groupe
	// Cela permet de reduire le nombre de valeurs initial pour les algorithmes
	// d'optimisation et de post-optimisation utilises ensuite
	//    Input: kwftSource
	//    Output: kwftPreprocessedSource (obligatoire)
	//            ivSourceToPreprocessedIndexes (facultatif)

	// Preprocessing: fusion les groupes purs ayant meme classe cible
	bPreprocessing = false;
	ivSourceToPreprocessedIndexes = NULL;
	kwftPreprocessedSource = NULL;
	if (bPreprocessPureGroups)
	{
		bPreprocessing = true;

		MergePureSourceValues(kwftSource, kwftPreprocessedSource, ivSourceToPreprocessedIndexes);
	}
	// Sinon, la table initiale est consideree comme la table preprocessee
	else
	{
		kwftPreprocessedSource = kwftSource;
	}

	assert(kwftPreprocessedSource != NULL);
	assert(bPreprocessPureGroups == (ivSourceToPreprocessedIndexes != NULL));

	// Affichage du nombre de groupes
	if (bPrintGroupNumbers)
		cout << "Nombre de groupes de la table preprocessee "
		     << kwftPreprocessedSource->GetFrequencyVectorNumber() << "\t";

	//////////////////////////////////////////////////////////////////////////
	// Preprocessing 2: on identifie les sous-groupes stables par l'ensemble
	// des groupages bi-classe afin de reduire encore le nombre de groupes
	// initiaux en nuisant le moins possible a la qualite du groupage final
	//    Input: kwftPreprocessedSource
	//    Output: oaInitialGroups (obligatoire)
	//            ivPreprocessedToSubGroupsIndexes (facultatif)

	// Test si on a encore un nombre trop important de valeurs a grouper
	ivPreprocessedToSubGroupsIndexes = NULL;
	oaInitialGroups = NULL;
	if (bBuildReliableSubGroups)
	{
		// Construction des sous-groupes stables en partant des sous-groupes issus
		// de la premiere etape de preprocessing
		oaInitialGroups = BuildReliableSubGroups(kwftPreprocessedSource, ivPreprocessedToSubGroupsIndexes);
	}
	// Sinon, les groupes initiaux sont constitues directement depuis la table preprocessee
	else
	{
		oaInitialGroups = BuildGroups(kwftPreprocessedSource);
	}

	assert(oaInitialGroups != NULL);
	assert(bBuildReliableSubGroups == (ivPreprocessedToSubGroupsIndexes != NULL));

	// Affichage du nombre de groupes
	if (bPrintGroupNumbers)
		cout << "Nbre de gpes apres second preprocessing\t " << oaInitialGroups->GetSize() << "\t";

	// Affichage
	if (bDisplayModalityNumber)
	{
		cout << "Nombre de modalites par groupe apres le second preprocessing : " << endl;
		for (i = 0; i < oaInitialGroups->GetSize(); i++)
		{
			group = cast(KWMODLGroup*, oaInitialGroups->GetAt(i));
			cout << i << "\t" << group->GetModalityNumber() << endl;
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// Creation des groupes initiaux, et reduction eventuelle de leur nombre
	// pour obtenir un nombre faible de groupes (Few)
	//    Input: oaInitialGroups
	//    Output: oaInitialGroups (obligatoire)
	//            ivFewerInitialIndexes (facultatif)

	// Test si on a encore un nombre trop important de valeurs a grouper
	bFewGroups = false;
	ivFewerInitialIndexes = NULL;
	nTotalFrequency = kwftSource->GetTotalFrequency();

	// Seuil du nombre de groupes en sqrt(N)
	// Un seuil en (int)sqrt(1.0*nTotalFrequency*log(nTotalFrequency * 1.0) / log(2.0))
	// conduit a une augmentation considerable du temps de calcul
	nMaxInitialGroupNumber = (int)sqrt(1.0 * nTotalFrequency);
	if (bBuildFewGroups and oaInitialGroups->GetSize() > nMaxInitialGroupNumber)
	{
		bFewGroups = true;

		// Calcul du seuil de frequence minimum des groupes a fusionner pour obtenir
		// au final un nombre limite de groupes
		nMinFrequency = ComputeMinGroupFrequency(oaInitialGroups, nTotalFrequency, nMaxInitialGroupNumber);

		// Fusion des petits groupes
		// CH V9 TODO ? Utiliser les deux modes bOneSingleGarbageGroup true et false ? False effectue moins de
		// fusions -> ces groupes pourront etre fusionnes ensuite
		MergeSmallGroups(oaInitialGroups, nMinFrequency, false, ivFewerInitialIndexes);
	}
	assert(oaInitialGroups != NULL);
	assert(bFewGroups == (ivFewerInitialIndexes != NULL));

	// Affichage du nombre de groupes
	if (bPrintGroupNumbers)
		cout << "Nbre de groupes apres Merge " << oaInitialGroups->GetSize() << "\t";

	////////////////////////////////////////////////////////////////////////////
	// Algorithme principal d'optimisation du groupage
	//    Input: oaInitialGroups, oaInitialGroupMerges
	//    Output: oaNewGroups (obligatoire)
	//            ivOptimizedIndexes (obligatoire)

	// Calcul du vecteur d'index recapitulant tous les eventuels pretraitement depuis la table source jusqu'au
	// tableau initial des groupes de modalites
	ComputeIndexesFromSourceToEndProcesses(ivSourceToPreprocessedIndexes, ivPreprocessedToSubGroupsIndexes,
					       ivFewerInitialIndexes, ivSourceToEndPreprocesses);

	// Initialisation des merges
	oaInitialGroupMerges = BuildGroupMerges(oaInitialGroups);

	// Affichage
	if (bDisplayModalityNumber)
	{
		cout << "Nombre de modalites par groupe avant l'optimisation principale : " << endl;
		for (i = 0; i < oaInitialGroups->GetSize(); i++)
		{
			group = cast(KWMODLGroup*, oaInitialGroups->GetAt(i));
			cout << i << "\t" << group->GetModalityNumber() << endl;
		}
	}

	// Recherche des groupes terminaux
	ivOptimizedIndexes = NULL;
	ivOptimumNumbers =
	    OptimizeGroupsWithGarbageSearch(0, oaInitialGroups, oaInitialGroupMerges, oaNewGroups, ivOptimizedIndexes);

	delete ivOptimumNumbers;

	if (bPrintInitialGroups)
	{
		cout << "Apres OptimizeGroupsWithGarbageSearch : affichage des groupes initiaux " << endl;
		for (i = 0; i < oaInitialGroups->GetSize(); i++)
			cout << *cast(KWMODLGroup*, oaInitialGroups->GetAt(i));
	}

	// Affichage du nombre de groupes apres l'optimisation principale
	if (bPrintGroupNumbers)
		cout << "Nbre de groupes apres l'optimisation principale\t" << oaNewGroups->GetSize() << "\t";

	//////////////////////////////////////////////////////////////////////////
	// Post-optimisation a partir des sous-groupes issus du preprocessing

	// Post-optimisation du groupage avec recherche de partition avec groupe poubelle
	if (bPostOptimization)
	{
		if (bDisplayModalityNumber)
		{
			cout << "Nombre de modalites par groupe avant la post-optimisation : " << endl;
			for (i = 0; i < oaNewGroups->GetSize(); i++)
			{
				group = cast(KWMODLGroup*, oaNewGroups->GetAt(i));
				cout << i << "\t" << group->GetModalityNumber() << endl;
			}
		}

		PostOptimizeGroupingWithGarbageSearch(oaInitialGroups, oaNewGroups, ivOptimizedIndexes, kwftSource,
						      kwftTarget);
	}
	else
	{
		kwftTarget = BuildTable(oaNewGroups);
		// Parametrage nInitialValueNumber, nGranularizedValueNumber
		kwftTarget->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
		kwftTarget->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
		// Parametrage granularite et poubelle
		kwftTarget->SetGranularity(kwftSource->GetGranularity());
		kwftTarget->SetGarbageModalityNumber(0);
	}

	// Nettoyage
	for (i = 0; i < oaInitialGroups->GetSize(); i++)
		delete cast(KWMODLGroup*, oaInitialGroups->GetAt(i));
	delete oaInitialGroups;

	if (oaNewGroups != NULL)
	{
		oaNewGroups->DeleteAll();
		delete oaNewGroups;
	}
	delete oaInitialGroupMerges;

	// Destruction de la table preprocessee
	if (bPreprocessing)
	{
		assert(kwftPreprocessedSource != kwftSource);
		delete kwftPreprocessedSource;
	}

	if (ivSourceToEndPreprocesses != NULL)
		delete ivSourceToEndPreprocesses;

	// Affichage du nombre de groupes
	if (bPrintGroupNumbers)
		cout << "Nbre de groupes apres post-optimisation \t" << kwftTarget->GetFrequencyVectorNumber() << "\t";

	/////////////////////////////////////////////////////////////////////////
	// Finalisation des resultats

	// Calcul des index entre les groupe preprocesses et les groupes optimises
	// Cas ou il y a eu construction de sous-groupes puis reduction de leur nombre
	ivPreprocessedToOptimizedIndexes = NULL;
	if (bBuildReliableSubGroups and bFewGroups)
	{
		ivPreprocessedToOptimizedIndexes = ivPreprocessedToSubGroupsIndexes;
		for (i = 0; i < ivPreprocessedToOptimizedIndexes->GetSize(); i++)
			ivPreprocessedToOptimizedIndexes->SetAt(
			    i, ivOptimizedIndexes->GetAt(
				   ivFewerInitialIndexes->GetAt(ivPreprocessedToSubGroupsIndexes->GetAt(i))));
		delete ivFewerInitialIndexes;
		delete ivOptimizedIndexes;
	}
	// Cas ou il il y a eu construction de sous-groupes
	else if (bBuildReliableSubGroups and not bFewGroups)
	{
		ivPreprocessedToOptimizedIndexes = ivPreprocessedToSubGroupsIndexes;
		for (i = 0; i < ivPreprocessedToOptimizedIndexes->GetSize(); i++)
			ivPreprocessedToOptimizedIndexes->SetAt(
			    i, ivOptimizedIndexes->GetAt(ivPreprocessedToSubGroupsIndexes->GetAt(i)));
		delete ivOptimizedIndexes;
		assert(ivFewerInitialIndexes == NULL);
	}
	// Cas ou il il y a eu reduction du nombre de groupe
	else if (not bBuildReliableSubGroups and bFewGroups)
	{
		ivPreprocessedToOptimizedIndexes = ivFewerInitialIndexes;
		for (i = 0; i < ivPreprocessedToOptimizedIndexes->GetSize(); i++)
			ivPreprocessedToOptimizedIndexes->SetAt(
			    i, ivOptimizedIndexes->GetAt(ivFewerInitialIndexes->GetAt(i)));
		delete ivOptimizedIndexes;
		assert(ivPreprocessedToSubGroupsIndexes == NULL);
	}
	// Cas ou il n'y a rien eu depuis la preoptimisation
	else
	{
		ivPreprocessedToOptimizedIndexes = ivOptimizedIndexes;
		assert(ivPreprocessedToSubGroupsIndexes == NULL);
		assert(ivFewerInitialIndexes == NULL);
	}
	assert(ivPreprocessedToOptimizedIndexes != NULL);
	// assert(ivPreprocessedToOptimizedIndexes->GetSize() == kwftPreprocessedSource->GetFrequencyVectorNumber());

	// Calcul des index des groupes pour les valeurs sources initiales
	// dans le cas ou il y a eu preprocessing intermediaire
	if (bPreprocessing)
	{
		ivGroups = new IntVector;
		ivGroups->SetSize(kwftSource->GetFrequencyVectorNumber());
		for (i = 0; i < ivGroups->GetSize(); i++)
			ivGroups->SetAt(
			    i, ivPreprocessedToOptimizedIndexes->GetAt(ivSourceToPreprocessedIndexes->GetAt(i)));
		delete ivSourceToPreprocessedIndexes;
		delete ivPreprocessedToOptimizedIndexes;
	}
	// Sinon, les index sont deja disponibles
	else
	{
		ivGroups = ivPreprocessedToOptimizedIndexes;

		assert(ivSourceToPreprocessedIndexes == NULL);
	}

	// Affichage de la table finale
	if (bPrintResults)
		cout << "MultipleClassesGroupWithGarbageSearch : End \tGrouped table\n" << *kwftTarget << endl;

	// Affichage du cout MODL
	if (bPrintGroupNumbers)
		cout << "Cout MODL de la grille optimisee \t" << ComputePartitionGlobalCost(kwftTarget) << endl;

	ensure(ivGroups->GetSize() == kwftSource->GetFrequencyVectorNumber());
	ensure(kwftSource->GetTotalFrequency() == kwftTarget->GetTotalFrequency());
	ensure(kwftSource->ComputeTotalValueNumber() == kwftTarget->ComputeTotalValueNumber());
}

IntVector* KWGrouperMODL::OptimizeGroupsWithGarbageSearch(int nExactGroupNumber, ObjectArray*& oaInitialGroups,
							  ObjectArray* oaInitialGroupMerges, ObjectArray*& oaNewGroups,
							  IntVector*& ivGroups) const
{
	boolean bPrintInitialGroups = false;
	boolean bPrintMerges = false;
	boolean bPrintNewGroups = false;
	int nOptimumGroupNumber;
	SortedList groupMergeList(KWMODLGroupMergeCompare);
	KWMODLGroup* group1;
	KWMODLGroup* group2;
	KWMODLGroup* group;
	KWMODLGroupMerge* bestGroupMerge;
	KWMODLGroupMerge* groupMerge;
	IntVector ivActiveGroupIndexes;
	IntVector ivNewIndexes;
	int i;
	int j;
	int nIndex;
	int nNewIndex;
	int nLastActiveGroupIndex;
	int nGroupNumber;
	double dUnionCost;
	double dDeltaCost;
	double dTotalDeltaCost;
	double dBestTotalDeltaCost;
	boolean bContinue;
	ObjectArray* oaCopyInitialGroups;
	IntVector* ivOptimumNumbers;
	IntVector ivActiveGroupIndexesWithGarbage;
	IntVector ivNewIndexesWithGarbage;
	SortedList frequencyList(KWMODLGroupModalityNumberCompare);
	double dGroupingWithGarbageDeltaCost;
	double dGroupingWithGarbagePartitionDeltaCost;
	double dCumulatedDeltaCostWithGarbage;
	double dPartitionWithGarbageCost;
	double dPrevPartitionWithGarbageCost;
	double dBestCumulatedDeltaCostWithGarbage;
	double dInitialModelDeltaCost;
	int nHigherSize;
	int nMergeSize;
	int nGarbageModalityNumber;
	int nOptimumGroupNumberWithGarbage;
	boolean bDisplayGarbage = false;

	require(oaInitialGroups != NULL);
	require(oaInitialGroups->GetSize() <= GetInitialValueNumber());
	require(oaInitialGroupMerges != NULL);
	require(nExactGroupNumber == 0 or (nExactGroupNumber >= 1 and nExactGroupNumber <= oaInitialGroups->GetSize()));

	// Affichage des groupes initiaux
	if (bPrintInitialGroups)
	{
		cout << "Debut OptimizeGroupsWithGarbageSearch : groupes initiaux" << endl;
		for (i = 0; i < oaInitialGroups->GetSize(); i++)
			cout << *cast(KWMODLGroup*, oaInitialGroups->GetAt(i));
	}

	// Initialisation du vecteur d'index
	ivGroups = new IntVector;
	ivGroups->SetSize(oaInitialGroups->GetSize());
	for (i = 0; i < ivGroups->GetSize(); i++)
		ivGroups->SetAt(i, i);

	oaCopyInitialGroups = new ObjectArray;
	oaCopyInitialGroups->SetSize(oaInitialGroups->GetSize());

	// On parcourt les groupes initiaux et pour chaque groupe :
	// Insertion du groupe dans la liste triee par nombre decroissant de modalites
	// Insertion des fusions du groupe dans une liste triee
	// par valeur decroissant
	// Copie du groupe
	for (i = 0; i < oaInitialGroups->GetSize(); i++)
	{
		// Ajout du groupe dans la liste triee par nombre de modalites et memorisation de sa position
		group = cast(KWMODLGroup*, oaInitialGroups->GetAt(i));
		group->SetPosition(frequencyList.Add(group));

		// Sauvegarde du groupe dans le tableau de Copy
		oaCopyInitialGroups->SetAt(i, group->Clone());

		for (j = 0; j < i; j++)
		{
			groupMerge = GetGroupMergeAt(oaInitialGroupMerges, oaInitialGroups->GetSize(), i, j);

			// Ajout de la fusion dans la liste triee, et memorisation de sa position
			groupMerge->SetPosition(groupMergeList.Add(groupMerge));
		}
	}

	// Initialisation du tableau des index des groupes en cours
	ivActiveGroupIndexes.SetSize(oaInitialGroups->GetSize());
	for (i = 0; i < ivActiveGroupIndexes.GetSize(); i++)
		ivActiveGroupIndexes.SetAt(i, i);

	// Initialisation du tableau des correspondances entre les index initiaux, et les
	// index dans le tableu des groupes actifs
	ivNewIndexes.SetSize(oaInitialGroups->GetSize());
	for (i = 0; i < ivNewIndexes.GetSize(); i++)
		ivNewIndexes.SetAt(i, i);

	// Groupages tant que amelioration
	bContinue = true;
	dTotalDeltaCost = 0;
	dBestTotalDeltaCost = 0;
	nGroupNumber = oaInitialGroups->GetSize();
	nOptimumGroupNumber = nGroupNumber;

	nGarbageModalityNumber = cast(KWMODLGroup*, frequencyList.GetHead())->GetModalityNumber();
	dCumulatedDeltaCostWithGarbage = 0;
	dBestCumulatedDeltaCostWithGarbage = 0;
	nOptimumGroupNumberWithGarbage = nGroupNumber;
	dPrevPartitionWithGarbageCost = 0;
	dGroupingWithGarbageDeltaCost = 0;

	// Variation de cout de la partition initiale sans/avec poubelle selon le plus gros element de frequencyList
	if (nGroupNumber > 2)
	{
		if (bDisplayGarbage)
		{
			cout << " Liste des nombres de modalites " << endl;
			frequencyList.Write(cout);
		}

		// Cout de partition initial avec modelisation poubelle
		dPartitionWithGarbageCost = ComputePartitionCost(
		    nGroupNumber, cast(KWMODLGroup*, frequencyList.GetHead())->GetModalityNumber());

		// Variation initiale de cout de modele pour la discretisation initiale : cout partition avec poubelle -
		// cout partition sans poubelle
		dInitialModelDeltaCost = dPartitionWithGarbageCost - ComputePartitionCost(nGroupNumber, 0);

		dPrevPartitionWithGarbageCost = dPartitionWithGarbageCost;
		dCumulatedDeltaCostWithGarbage = dInitialModelDeltaCost;
		dBestCumulatedDeltaCostWithGarbage = dCumulatedDeltaCostWithGarbage;
	}

	while (bContinue and nGroupNumber > 3 and nGroupNumber > nExactGroupNumber)
	{
		// Recherche de la meilleure fusion (celle de plus petit DeltaCout)
		assert(groupMergeList.GetCount() == nGroupNumber * (nGroupNumber - 1) / 2);
		bestGroupMerge = cast(KWMODLGroupMerge*, groupMergeList.GetTail());

		// Test si la meilleure fusion degrade le cout de la partition SANS poubelle
		dDeltaCost = bestGroupMerge->GetDeltaCost() + ComputePartitionDeltaCost(nGroupNumber, 0);
		bContinue = dDeltaCost < dEpsilon;

		// Calcul de la variation de cout entre les partitions avec poubelle avant et apres la meilleure fusion
		if (nGroupNumber > 3)
		{
			// Recherche du groupe d'effectif le plus eleve
			// Effectif le plus eleve avant le merge
			nHigherSize = cast(KWMODLGroup*, frequencyList.GetHead())->GetModalityNumber();

			// Acces aux groupes sources de la fusion
			assert(bestGroupMerge->GetIndex1() < bestGroupMerge->GetIndex2());
			group1 = cast(KWMODLGroup*, oaInitialGroups->GetAt(bestGroupMerge->GetIndex1()));
			group2 = cast(KWMODLGroup*, oaInitialGroups->GetAt(bestGroupMerge->GetIndex2()));

			// Somme des effectifs des 2 groupes impliques dans le merge
			nMergeSize = group1->GetModalityNumber() + group2->GetModalityNumber();
			if (nHigherSize > nMergeSize)
				nGarbageModalityNumber = nHigherSize;
			else
				nGarbageModalityNumber = nMergeSize;
			// Cout de partition avec un groupe en moins (suite au Merge) en prenant en compte comme groupe
			// poubelle celui qui contient le plus de modalites
			dPartitionWithGarbageCost = ComputePartitionCost(nGroupNumber - 1, nGarbageModalityNumber);

			// Variation de cout de partition entre partition courante avec poubelle et participation
			// precedente avec poubelle
			dGroupingWithGarbagePartitionDeltaCost =
			    dPartitionWithGarbageCost - dPrevPartitionWithGarbageCost;
			// Prise en compte de la variation de cout de codage des parties
			dGroupingWithGarbageDeltaCost =
			    dGroupingWithGarbagePartitionDeltaCost + bestGroupMerge->GetDeltaCost();

			// Test si la meilleure fusion degrade le cout de la partition AVEC poubelle
			if (bContinue)
				bContinue = dGroupingWithGarbageDeltaCost < dEpsilon;

			// Mise a jour de la variation de cout cumulee par rapport au cout initial
			dPrevPartitionWithGarbageCost = dPartitionWithGarbageCost;
			dCumulatedDeltaCostWithGarbage += dGroupingWithGarbageDeltaCost;
		}

		// Test d'amelioration du meilleur cout total SANS poubelle
		dTotalDeltaCost += dDeltaCost;
		if (dTotalDeltaCost < dBestTotalDeltaCost + dEpsilon)
		{
			dBestTotalDeltaCost = dTotalDeltaCost;
			nOptimumGroupNumber = nGroupNumber - 1;
		}

		// Test d'amelioration du meilleur cout total AVEC poubelle
		if (dCumulatedDeltaCostWithGarbage < dBestCumulatedDeltaCostWithGarbage + dEpsilon)
		{
			dBestCumulatedDeltaCostWithGarbage = dCumulatedDeltaCostWithGarbage;
			nOptimumGroupNumberWithGarbage = nGroupNumber - 1;
		}

		if (bDisplayGarbage)
			cout << nGroupNumber - 1 << "\t" << dTotalDeltaCost << "\t" << dCumulatedDeltaCostWithGarbage
			     << "\t" << nGarbageModalityNumber << endl;

		// Test si contrainte de nombre max de groupes non respectee
		if (GetMaxGroupNumber() > 0 and nGroupNumber > GetMaxGroupNumber())
			bContinue = true;

		// Si nombre de groupes impose
		if (nExactGroupNumber != 0)
			bContinue = nGroupNumber > nExactGroupNumber;

		// Groupage effectif si amelioration (avec et sans poubelle)
		if (bContinue)
		{
			nGroupNumber--;

			// Acces aux groupes sources de la fusion
			assert(bestGroupMerge->GetIndex1() < bestGroupMerge->GetIndex2());
			group1 = cast(KWMODLGroup*, oaInitialGroups->GetAt(bestGroupMerge->GetIndex1()));
			group2 = cast(KWMODLGroup*, oaInitialGroups->GetAt(bestGroupMerge->GetIndex2()));

			// Affichage de la fusion
			if (bPrintMerges)
			{
				cout << "Merge\t" << dDeltaCost << "\t" << nGroupNumber << "\n"
				     << "\t" << *bestGroupMerge << "\t" << *group1 << "\t" << *group2;
				cout << "Merge AVEC poubelle\t" << dGroupingWithGarbageDeltaCost << "\n";
			}

			// Supression des groupes a merger de la liste des effectifs en modalites
			frequencyList.RemoveAt(group1->GetPosition());
			debug(group1->SetPosition(NULL));
			frequencyList.RemoveAt(group2->GetPosition());
			debug(group2->SetPosition(NULL));

			// Le premier groupe (de plus petit index) devient le support de la fusion
			// Calcul du vecteur d'effectifs du groupe fusionne
			assert(CheckFrequencyVector(group1->GetFrequencyVector()));
			assert(CheckFrequencyVector(group2->GetFrequencyVector()));
			AddFrequencyVector(group1->GetFrequencyVector(), group2->GetFrequencyVector());
			group1->SetCost(ComputeGroupCost(group1->GetFrequencyVector()));

			// Re-insertion dans la liste triee par nombre de modalites
			group1->SetPosition(frequencyList.Add(group1));

			// Affichage du nouveau groupe
			if (bPrintMerges)
			{
				cout << "\tN" << *group1 << flush;
			}

			// Le deuxieme groupe source aura meme index de groupe terminal que le premier
			ivGroups->SetAt(group2->GetIndex(), group1->GetIndex());

			// Desormais, le deuxieme groupe source n'est plus actif
			// On recherche sa position dans la liste des index actifs (grace
			// a ivNewIndexes), et l'on remplace sa position par celle du dernier
			// groupe actif (dont on doit reactualiser la position dans ivNewIndexes)
			nNewIndex = ivNewIndexes.GetAt(group2->GetIndex());
			assert(ivActiveGroupIndexes.GetAt(nNewIndex) == group2->GetIndex());
			ivNewIndexes.SetAt(group2->GetIndex(), ivActiveGroupIndexes.GetSize() - 1);
			//
			nLastActiveGroupIndex = ivActiveGroupIndexes.GetAt(ivActiveGroupIndexes.GetSize() - 1);
			ivActiveGroupIndexes.SetAt(nNewIndex, nLastActiveGroupIndex);
			ivActiveGroupIndexes.SetSize(ivActiveGroupIndexes.GetSize() - 1);
			ivNewIndexes.SetAt(nLastActiveGroupIndex, nNewIndex);
			assert(ivActiveGroupIndexes.GetSize() == nGroupNumber);

			// On reactualise la liste des fusions, en supprimant de la liste
			// les fusions avec un des groupes sources, et en ajoutant les fusions avec
			// le nouveau groupe fusionne
			for (i = 0; i < ivActiveGroupIndexes.GetSize(); i++)
			{
				nIndex = ivActiveGroupIndexes.GetAt(i);

				// Retrait de la liste des fusions avec le second groupe source
				groupMerge = GetGroupMergeAt(oaInitialGroupMerges, oaInitialGroups->GetSize(),
							     group2->GetIndex(), nIndex);
				groupMergeList.RemoveAt(groupMerge->GetPosition());
				debug(groupMerge->SetPosition(NULL));

				// Retrait de la liste des fusions avec le premier groupe source
				if (nIndex != group1->GetIndex())
				{
					// Retrait de la liste
					groupMerge = GetGroupMergeAt(oaInitialGroupMerges, oaInitialGroups->GetSize(),
								     group1->GetIndex(), nIndex);
					groupMergeList.RemoveAt(groupMerge->GetPosition());
					debug(groupMerge->SetPosition(NULL));

					// Calcul du vecteur d'effectifs du groupe fusionne avec le
					// nouveau groupe (porte par group1)
					group = cast(KWMODLGroup*, oaInitialGroups->GetAt(nIndex));
					dUnionCost = ComputeGroupUnionCost(group1->GetFrequencyVector(),
									   group->GetFrequencyVector());

					// Ajout de la fusion dans la liste triee, et memorisation de sa position
					groupMerge->SetDeltaCost(dUnionCost - group1->GetCost() - group->GetCost());
					groupMerge->SetPosition(groupMergeList.Add(groupMerge));
					assert(groupMerge->GetIndex1() < groupMerge->GetIndex2());
				}
			}
		}
	}

	// Nettoyage des fusions
	oaInitialGroupMerges->DeleteAll();

	// Memorisation des nouveau groupes, et memorisation de leur index dans
	// le nouveau tableau de groupes
	ivNewIndexes.SetSize(ivGroups->GetSize());
	oaNewGroups = new ObjectArray;
	oaNewGroups->SetSize(ivActiveGroupIndexes.GetSize());
	for (i = 0; i < ivActiveGroupIndexes.GetSize(); i++)
	{
		nIndex = ivActiveGroupIndexes.GetAt(i);

		// Transfert du groupe recherche vers le tableau des nouveau groupes
		group = cast(KWMODLGroup*, oaInitialGroups->GetAt(nIndex));
		oaNewGroups->SetAt(i, group);

		// Memorisation de la correspondance entre l'ancien index et le nouveau
		ivNewIndexes.SetAt(nIndex, i);

		// Marquage dans l'ancien tableau (permettant de ne pas le detruire)
		assert(oaInitialGroups->GetAt(nIndex) == group);
		oaInitialGroups->SetAt(nIndex, NULL);
	}

	// Nettoyage des groupes initiaux fusionnes (non transferres)
	oaInitialGroups->DeleteAll();
	delete oaInitialGroups;

	// Recuperation du tableau des groupes initiaux
	oaInitialGroups = oaCopyInitialGroups;

	// Mise a jour du vecteur d'index: on trouve recursivement
	// l'index du groupe terminal issue des fussions successives du groupe initial
	// Memorisation des index utilises
	for (i = 0; i < ivGroups->GetSize(); i++)
	{
		// Recherche de l'index du groupe
		nIndex = ivGroups->GetAt(i);
		while (ivGroups->GetAt(nIndex) < nIndex)
			nIndex = ivGroups->GetAt(nIndex);

		// Memorisation de l'index du groupe
		assert(0 <= nIndex and nIndex < ivGroups->GetSize());
		ivGroups->SetAt(i, nIndex);
	}

	// Renumerotation des index de groupes
	for (i = 0; i < ivGroups->GetSize(); i++)
	{
		nIndex = ivNewIndexes.GetAt(ivGroups->GetAt(i));
		ivGroups->SetAt(i, nIndex);
	}

	// Affichage des nouveaux groupes
	if (bPrintNewGroups)
	{
		cout << "OptimizeGroupsWithGarbageSearch : affichage des groupes finaux " << endl;
		for (i = 0; i < oaNewGroups->GetSize(); i++)
			cout << *cast(KWMODLGroup*, oaNewGroups->GetAt(i));
	}

	// Affichage des groupes initiaux
	if (bPrintInitialGroups)
	{
		cout << "Fin OptimizeGroupsWithGarbageSearch : affichage des groupes initiaux " << endl;
		for (i = 0; i < oaInitialGroups->GetSize(); i++)
			cout << *cast(KWMODLGroup*, oaInitialGroups->GetAt(i));
	}

	if (bDisplayGarbage and nExactGroupNumber == 0)
		cout << "Nbre optimal de groupes avant degradation (avec ou sans poub) \tSANS poubelle\t"
		     << nOptimumGroupNumber << "\t AVEC poubelle \t" << nOptimumGroupNumberWithGarbage << endl;

	ensure(nExactGroupNumber == 0 or nExactGroupNumber == nGroupNumber);

	// Memorisation du vecteur des tailles optimales des partitions avec et sans poubelle
	ivOptimumNumbers = new IntVector;
	ivOptimumNumbers->Add(nOptimumGroupNumber);
	ivOptimumNumbers->Add(nOptimumGroupNumberWithGarbage);
	return ivOptimumNumbers;
}
