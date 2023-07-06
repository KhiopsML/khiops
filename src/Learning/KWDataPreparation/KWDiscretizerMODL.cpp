// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDiscretizerMODL.h"

boolean KWDiscretizerMODLFamily::IsMODLFamily() const
{
	return true;
}

KWMODLHistogramResults* KWDiscretizerMODLFamily::BuildMODLHistogramResults() const
{
	return NULL;
}

KWMODLHistogramResults* KWDiscretizerMODLFamily::CreateMODLHistogramResults() const
{
	return NULL;
}

const PLSharedObject* KWDiscretizerMODLFamily::GetMODLHistogramResultsSharedObject() const
{
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDiscretizerMODL

double KWDiscretizerMODL::dPriorityDeltaCost = -1e9;

double KWDiscretizerMODL::dInfiniteCost = 1e20;

KWDiscretizerMODL::KWDiscretizerMODL()
{
	nMergeNumber = 0;
	nExtraMergeNumber = 0;
	nSplitNumber = 0;
	nExtraSplitNumber = 0;
	nMergeSplitNumber = 0;
	nMergeMergeSplitNumber = 0;
	discretizationCosts = new KWMODLDiscretizationCosts;
	dEpsilon = 1e-6;
}

KWDiscretizerMODL::~KWDiscretizerMODL()
{
	delete discretizationCosts;
}

const ALString KWDiscretizerMODL::GetName() const
{
	return "MODL";
}

KWDiscretizer* KWDiscretizerMODL::Create() const
{
	return new KWDiscretizerMODL;
}

void KWDiscretizerMODL::SetDiscretizationCosts(KWUnivariatePartitionCosts* kwupcCosts)
{
	require(kwupcCosts != NULL);
	require(discretizationCosts != NULL);
	delete discretizationCosts;
	discretizationCosts = kwupcCosts;
}

KWUnivariatePartitionCosts* KWDiscretizerMODL::GetDiscretizationCosts() const
{
	ensure(discretizationCosts != NULL);
	return discretizationCosts;
}

void KWDiscretizerMODL::Discretize(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget) const
{
	KWFrequencyTable* kwftGranularizedTable;
	KWFrequencyTable* kwftMergedTable;
	KWFrequencyTable* kwftDiscretizedGranularizedTable;
	double dCost;
	double dBestCost;
	int nGranularity;
	int nGranularityMax;
	boolean bIsLastGranularity;
	int nInstanceNumber;
	boolean bDisplayResults = false;
	KWQuantileIntervalBuilder quantileBuilder;
	IntVector ivInputFrequencies;
	int nSourceIndex;
	boolean bIsGranularitySelected;
	int nCurrentPartileNumber;
	int nPreviousPartileNumber;
	double dRequiredIncreasingCoefficient = 1.5;
	int nCurrentExploredGranularity;
	int nLastExploredGranularity;

	require(kwftSource != NULL);
	require(kwftSource->Check());

	// Initialisations
	dBestCost = DBL_MAX;
	kwftTarget = NULL;
	nInstanceNumber = kwftSource->GetTotalFrequency();
	nCurrentPartileNumber = 0;
	kwftGranularizedTable = NULL;
	kwftDiscretizedGranularizedTable = NULL;

	// Cas d'une base vide
	if (nInstanceNumber == 0 or kwftSource->GetFrequencyVectorNumber() == 1)
		kwftTarget = kwftSource->Clone();
	// Sinon
	else
	{
		// Granularite max
		nGranularityMax = (int)ceil(log(nInstanceNumber * 1.0) / log(2.0));
		bIsLastGranularity = false;
		nPreviousPartileNumber = 0;

		// On analyse toutes les granularite de 1 a Max
		nGranularity = 1;

		// Initialisation du vecteur d'effectifs de la table
		for (nSourceIndex = 0; nSourceIndex < kwftSource->GetFrequencyVectorNumber(); nSourceIndex++)
			ivInputFrequencies.Add(kwftSource->GetFrequencyVectorAt(nSourceIndex)->ComputeTotalFrequency());

		// Initialisation du quantileBuilder utilise pour chaque granularisation
		quantileBuilder.InitializeFrequencies(&ivInputFrequencies);

		// Initialisation
		nCurrentExploredGranularity = -1;
		nLastExploredGranularity = -1;

		// Nettoyage
		ivInputFrequencies.SetSize(0);
		while (nGranularity <= nGranularityMax and not bIsLastGranularity)
		{
			// Arret si interruption utilisateur
			if (TaskProgression::IsInterruptionRequested())
			{
				if (kwftTarget != NULL)
					delete kwftTarget;
				kwftTarget = new KWFrequencyTable;
				kwftTarget->ComputeNullTable(kwftSource);
				break;
			}

			// Calcul de la table de contingence associee a la granularite a partir de kwctSource
			GranularizeFrequencyTable(kwftSource, kwftGranularizedTable, nGranularity, &quantileBuilder);

			// Extraction du nombre de partiles de la table a la granularite courante
			nCurrentPartileNumber = kwftGranularizedTable->GetFrequencyVectorNumber();

			// Test s'il s'agit de la derniere granularite a traiter (si le nombre de parties est maximal)
			bIsLastGranularity = (kwftSource->GetFrequencyVectorNumber() == nCurrentPartileNumber);

			// Test d'une granularite eligible
			// Le nombre de partiles pour la granularite courante est il :
			// - superieur d'au moins un facteur dRequiredIncreasingCoefficient au nombre de partiles pour
			// la derniere granularite traitee ET
			// - inferieur d'au moins un facteur dRequiredIncreasingCoefficient au nombre de partiles de la
			// granularite max
			bIsGranularitySelected =
			    (nCurrentPartileNumber >= dRequiredIncreasingCoefficient * nPreviousPartileNumber) and
			    (nCurrentPartileNumber * dRequiredIncreasingCoefficient <=
			     kwftSource->GetFrequencyVectorNumber());

			if (bDisplayResults)
			{
				cout << "Granularite = " << nGranularity << endl;
				cout << "Nbre valeurs explicatives table source\t"
				     << kwftSource->GetFrequencyVectorNumber() << "\tTaille table granularisee\t "
				     << kwftGranularizedTable->GetFrequencyVectorNumber() << endl;
			}

			// Cas du traitement de la granularite courante
			if (bIsGranularitySelected or bIsLastGranularity)
			{
				// Memorisation de la derniere granularite exploree
				nLastExploredGranularity = nCurrentExploredGranularity;
				nCurrentExploredGranularity = nGranularity;

				// Calcul de la table de contingence issue de la fusion des intervalles purs
				MergeFrequencyTablePureIntervals(kwftGranularizedTable, kwftMergedTable);

				if (bDisplayResults)
				{
					cout << "Granularite = " << nGranularity << endl;
					cout << "Nbre valeurs explicatives table source\t"
					     << kwftSource->GetFrequencyVectorNumber()
					     << "\tTaille table granularisee\t "
					     << kwftGranularizedTable->GetFrequencyVectorNumber()
					     << "\tTable granularise\t" << *kwftGranularizedTable
					     << "\tTaille table apres fusion valeurs pures\t"
					     << kwftMergedTable->GetFrequencyVectorNumber() << endl;
				}

				// Nettoyage
				delete kwftGranularizedTable;
				kwftGranularizedTable = NULL;

				// Discretisation de la table granularisee
				DiscretizeGranularizedFrequencyTable(kwftMergedTable, kwftDiscretizedGranularizedTable);

				// Memorisation du meilleur cout avec ComputeDiscretizationCost et de la granularite
				// associee
				dCost = ComputeDiscretizationCost(kwftDiscretizedGranularizedTable);

				// Nettoyage
				delete kwftMergedTable;
				kwftMergedTable = NULL;

				if (dCost < dBestCost)
				{
					dBestCost = dCost;
					// Destruction de l'optimum precedent
					if (kwftTarget != NULL)
						delete kwftTarget;
					// Memorisation du nouvel optimum
					kwftTarget = new KWFrequencyTable;
					kwftTarget->CopyFrom(kwftDiscretizedGranularizedTable);
				}
				if (bDisplayResults)
				{
					cout << "Granularite\tConstr. cost\tPrep. cost\tData cost\tTotal cost\tGroups"
					     << endl;
					cout << nGranularity << "\t"
					     << ComputeDiscretizationConstructionCost(kwftDiscretizedGranularizedTable)
					     << "\t"
					     << ComputeDiscretizationPreparationCost(kwftDiscretizedGranularizedTable)
					     << "\t" << ComputeDiscretizationDataCost(kwftDiscretizedGranularizedTable)
					     << "\t" << ComputeDiscretizationCost(kwftDiscretizedGranularizedTable)
					     << "\t" << kwftDiscretizedGranularizedTable->GetFrequencyVectorNumber()
					     << endl;
				}
				// Nettoyage
				delete kwftDiscretizedGranularizedTable;
				kwftDiscretizedGranularizedTable = NULL;
			}
			// Cas ou l'on n'etudie pas cette granularite associee a la meme table que celle etudie
			// precedemment
			else
			{
				delete kwftGranularizedTable;
				kwftGranularizedTable = NULL;

				if (bDisplayResults)
				{
					cout << "Granularite\t" << nGranularity
					     << "\t non traitee car identique a la precedente" << endl;
				}
			}

			// Passage a la granularite suivante
			nPreviousPartileNumber = nCurrentPartileNumber;
			nGranularity++;
		}

		// Post-optimisation de la granularite : on attribue a la partition optimale la plus petite granularite
		// pour laquelle cette partition est definie
		if (nLastExploredGranularity != -1 and kwftTarget->GetGranularity() > nLastExploredGranularity + 1)
		{
			if (bDisplayResults)
				cout << " Grille avant optimisation granularite " << *kwftTarget;
			PostOptimizeGranularity(kwftTarget, &quantileBuilder, nLastExploredGranularity);
			if (bDisplayResults)
				cout << " Grille apres optimisation granularite " << *kwftTarget;
		}
		if (bDisplayResults)
			cout << "Meilleure granularite discretisation " << kwftTarget->GetGranularity() << " sur "
			     << nGranularityMax << endl;
	}
	ensure(kwftTarget != NULL);
}

void KWDiscretizerMODL::DiscretizeGranularizedFrequencyTable(KWFrequencyTable* kwftSource,
							     KWFrequencyTable*& kwftTarget) const
{
	KWMODLLine* headInterval;

	require(Check());
	require(kwftSource != NULL);

	// Cas particulier ou il n'y a qu'une valeur source
	if (kwftSource->GetFrequencyVectorNumber() <= 1)
	{
		kwftTarget = kwftSource->Clone();
	}
	// Sinon, discretization
	else
	{
		// Optimisation de la liste des intervalles
		headInterval = IntervalListOptimization(kwftSource);

		// Construction de la table finale a partir de la liste d'intervalles
		kwftTarget = BuildFrequencyTableFromIntervalList(headInterval);
		// Parametrisation du nombre de valeurs
		kwftTarget->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
		kwftTarget->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
		// Parametrisation de la granularite et de la poubelle
		kwftTarget->SetGranularity(kwftSource->GetGranularity());
		kwftTarget->SetGarbageModalityNumber(kwftSource->GetGarbageModalityNumber());

		// Nettoyage
		DeleteIntervalList(headInterval);
	}
	ensure(kwftSource->GetFrequencyVectorSize() == kwftTarget->GetFrequencyVectorSize());
	ensure(kwftSource->GetFrequencyVectorNumber() >= kwftTarget->GetFrequencyVectorNumber());
	ensure(kwftSource->GetTotalFrequency() == kwftTarget->GetTotalFrequency());
}

double KWDiscretizerMODL::ComputeDiscretizationCost(KWFrequencyTable* kwftDiscretizedTable) const
{
	double dCost;

	require(kwftDiscretizedTable != NULL);

	// Cas particuliers: cout nul
	if (kwftDiscretizedTable->GetTotalFrequency() == 0 or kwftDiscretizedTable->GetFrequencyVectorSize() <= 1)
		return 0;

	// Initialisation des donnees de travail
	InitializeWorkingData(kwftDiscretizedTable);

	// Cout global du modele
	dCost = ComputePartitionGlobalCost(kwftDiscretizedTable);

	// Nettoyage des donnees de travail
	CleanWorkingData();

	return dCost;
}

double KWDiscretizerMODL::ComputeDiscretizationModelCost(KWFrequencyTable* kwftDiscretizedTable) const
{
	double dCost;

	require(kwftDiscretizedTable != NULL);

	// Cas particuliers: cout nul
	if (kwftDiscretizedTable->GetTotalFrequency() == 0 or kwftDiscretizedTable->GetFrequencyVectorSize() <= 1)
		return 0;

	// Initialisation des donnees de travail
	InitializeWorkingData(kwftDiscretizedTable);

	// Cout global du modele
	dCost = discretizationCosts->ComputePartitionGlobalModelCost(kwftDiscretizedTable);

	// Nettoyage des donnees de travail
	CleanWorkingData();

	return dCost;
}

double KWDiscretizerMODL::ComputeDiscretizationConstructionCost(KWFrequencyTable* kwftDiscretizedTable) const
{
	double dCost;

	require(kwftDiscretizedTable != NULL);

	// Cas particuliers: cout nul
	if (kwftDiscretizedTable->GetTotalFrequency() == 0 or kwftDiscretizedTable->GetFrequencyVectorSize() <= 1)
		return 0;

	// Initialisation des donnees de travail
	InitializeWorkingData(kwftDiscretizedTable);

	// Cout global du modele
	dCost = discretizationCosts->ComputePartitionGlobalConstructionCost(kwftDiscretizedTable);

	// Nettoyage des donnees de travail
	CleanWorkingData();

	return dCost;
}

double KWDiscretizerMODL::ComputeDiscretizationPreparationCost(KWFrequencyTable* kwftDiscretizedTable) const
{
	double dCost;

	require(kwftDiscretizedTable != NULL);

	// Cas particuliers: cout nul
	if (kwftDiscretizedTable->GetTotalFrequency() == 0 or kwftDiscretizedTable->GetFrequencyVectorSize() <= 1)
		return 0;

	// Initialisation des donnees de travail
	InitializeWorkingData(kwftDiscretizedTable);

	// Cout global du modele
	dCost = discretizationCosts->ComputePartitionGlobalPreparationCost(kwftDiscretizedTable);

	// Nettoyage des donnees de travail
	CleanWorkingData();

	return dCost;
}

double KWDiscretizerMODL::ComputeDiscretizationDataCost(KWFrequencyTable* kwftDiscretizedTable) const
{
	double dCost;

	require(kwftDiscretizedTable != NULL);

	// Cas particuliers: cout nul
	if (kwftDiscretizedTable->GetTotalFrequency() == 0 or kwftDiscretizedTable->GetFrequencyVectorSize() <= 1)
		return 0;

	// Initialisation des donnees de travail
	InitializeWorkingData(kwftDiscretizedTable);

	// Cout global du modele
	dCost = discretizationCosts->ComputePartitionGlobalDataCost(kwftDiscretizedTable);

	// Nettoyage des donnees de travail
	CleanWorkingData();

	return dCost;
}

boolean KWDiscretizerMODL::Check() const
{
	boolean bOk;
	ALString sTmp;

	// Test si parametre entier
	bOk = dParam == (int)dParam;
	if (not bOk)
		AddError("The main parameter of the algorithm must be an integer");

	// Test si le parametre correspond a une valeur possible
	if (bOk)
	{
		bOk = OptimizedGreedyMerge <= dParam and dParam <= Optimal;
		if (not bOk)
		{
			AddError("Incorrect value for the main parameter of the algorithm");
			AddSimpleMessage(sTmp + "\t" + IntToString(OptimizedGreedyMerge) + ": " +
					 "Greedy bottom-up algorithm, with post-optimization");
			AddSimpleMessage(sTmp + "\t" + IntToString(GreedyMerge) + ": " + "Greedy bottom-up algorithm");
			AddSimpleMessage(sTmp + "\t" + IntToString(OptimizedGreedySplit) + ": " +
					 "Greedy top-down algorithm, with post-optimization");
			AddSimpleMessage(sTmp + "\t" + IntToString(GreedySplit) + ": " + "Greedy top-down algorithm");
			AddSimpleMessage(sTmp + "\t" + IntToString(Optimal) + ": " +
					 "Optimal algorithm (warning: O(N^3))");
		}
	}

	return bOk;
}

void KWDiscretizerMODL::GranularizeFrequencyTable(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget,
						  int nGranularity, KWQuantileIntervalBuilder* quantileBuilder) const
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
	nPartileNumber = (int)pow(2.0, nGranularity);

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
			cout << " Debut Granularize avec QuantileIntervalBuilder = " << nGranularity
			     << "\t Nbre de partiles = " << nPartileNumber << "\t nInstanceNumber " << nInstanceNumber
			     << endl;
			cout << "Nbre valeurs explicatives table initiale " << kwftSource->GetFrequencyVectorNumber()
			     << endl;
		}

		// Calcul des quantiles
		quantileBuilder->ComputeQuantiles(nPartileNumber);

		// Initialisation du nombre effectif de partiles (peut etre inferieur au nombre theorique du fait de
		// doublons)
		nActualPartileNumber = quantileBuilder->GetIntervalNumber();

		// Cas d'un nombre de partiles egal au nombre de valeurs distinctes (granularisation maximale)
		if (nActualPartileNumber == nSourceValueNumber)
		{
			kwftTarget = kwftSource->Clone();
			kwftTarget->SetGranularity(nGranularity);
			kwftTarget->SetGarbageModalityNumber(0);
		}
		// Sinon
		else
		{
			// Creation de la table de contingence cible
			kwftTarget = new KWFrequencyTable;
			kwftTarget->SetFrequencyVectorCreator(GetFrequencyVectorCreator()->Clone());
			kwftTarget->SetFrequencyVectorNumber(nActualPartileNumber);

			// Initialisation du nombre initial de valeurs et du nombre de valeurs apres granularisation
			// Dans le cas d'une variable numerique, on conserve le nombre theorique de valeurs apres
			// granularisation
			kwftTarget->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
			kwftTarget->SetGranularizedValueNumber(nPartileNumber);

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
				for (nSourceIndex = quantileBuilder->GetIntervalFirstValueIndexAt(nPartileIndex);
				     nSourceIndex <= quantileBuilder->GetIntervalLastValueIndexAt(nPartileIndex);
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
	assert(kwftSource->GetTotalFrequency() == kwftTarget->GetTotalFrequency());
}

void KWDiscretizerMODL::TestGranularizeFrequencyTable()
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
	KWDiscretizerMODL discretizerMODL;
	KWQuantileIntervalBuilder quantileBuilder;
	IntVector ivInputFrequencies;

	// Test de validite
	cout << "Initialisation d'une matrice de correlation" << endl;
	// nSourceNumber = AcquireRangedInt("Nombre de valeurs pour la loi source", 2, 100, 10);
	// nTargetNumber = AcquireRangedInt("Nombre de valeurs pour la loi cible", 2, 20, 5);
	nSourceNumber = 100;
	nTargetNumber = 2;
	nTableFrequency = 1000;
	kwftTest.SetFrequencyVectorNumber(nSourceNumber);

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

	// Initialisation du quantileBuilder utilise pour chaque granularisation
	quantileBuilder.InitializeFrequencies(&ivInputFrequencies);

	kwftTestTarget = NULL;
	// Parcours des granularites
	for (nGranularity = 1; nGranularity <= (int)ceil(log(kwftTest.GetTotalFrequency() * 1.0) / log(2.0));
	     nGranularity++)
	{
		discretizerMODL.GranularizeFrequencyTable(&kwftTest, kwftTestTarget, nGranularity, &quantileBuilder);
		cout << "Table apres granularisation avec la granularite" << nGranularity << " avec "
		     << pow(2, nGranularity) << " partiles " << endl;
		cout << *kwftTestTarget << endl;
		delete kwftTestTarget;
		kwftTestTarget = NULL;
	}
}

void KWDiscretizerMODL::PostOptimizeGranularity(KWFrequencyTable* kwftTarget,
						KWQuantileIntervalBuilder* quantileBuilder,
						int nLastExploredGranularity) const
{
	int nCurrentGranularity;
	int nBestGranularity;
	int nGranularityPartileNumber;
	int nPartitionIntervalIndex;
	int nGranularisationIntervalIndex;
	int nPartitionCumulatedFrequency;
	int nGranularisationCumulatedFrequency;
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

			// Cas ou le nombre reel de cette granularisation est superieur ou egal a la taille de la
			// partition
			if (nGranularityPartileNumber >= kwftTarget->GetFrequencyVectorNumber())
			{
				// Initialisation
				nPartitionIntervalIndex = 0;
				nPartitionCumulatedFrequency =
				    kwftTarget->GetFrequencyVectorAt(nPartitionIntervalIndex)->ComputeTotalFrequency();
				nGranularisationIntervalIndex = 0;
				nGranularisationCumulatedFrequency =
				    quantileBuilder->GetIntervalLastInstanceIndexAt(nGranularisationIntervalIndex) + 1;

				// Parcours des intervalles de la partition optimale et des intervalles de la
				// granularisation tant qu'il y a compatibilite
				while (not bIncompatibleGranularity and
				       nPartitionIntervalIndex < kwftTarget->GetFrequencyVectorNumber())
				{
					while (nPartitionCumulatedFrequency > nGranularisationCumulatedFrequency)
					{
						nGranularisationIntervalIndex++;
						nGranularisationCumulatedFrequency =
						    quantileBuilder->GetIntervalLastInstanceIndexAt(
							nGranularisationIntervalIndex) +
						    1;
					}

					if (nPartitionCumulatedFrequency <
					    quantileBuilder->GetIntervalLastInstanceIndexAt(
						nGranularisationIntervalIndex) +
						1)
						bIncompatibleGranularity = true;
					nPartitionIntervalIndex++;
					if (nPartitionIntervalIndex < kwftTarget->GetFrequencyVectorNumber())
						nPartitionCumulatedFrequency +=
						    kwftTarget->GetFrequencyVectorAt(nPartitionIntervalIndex)
							->ComputeTotalFrequency();
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

void KWDiscretizerMODL::MergeFrequencyTablePureIntervals(KWFrequencyTable* kwftSource,
							 KWFrequencyTable*& kwftTarget) const
{
	ObjectArray oaFrequencies;
	IntVector* ivFrequencies;
	IntVector* ivSourceFrequencies;
	int nTargetIndex;
	int nSourceIndex;
	int nSourceFrequency;
	int nPureTargetPrevIndex;
	int nPureTargetIndex;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	IntVector* ivFrequencyVector;

	nPureTargetPrevIndex = -1;
	ivFrequencies = NULL;
	// Parcours des intervalles sources pour reperage des fusions d'intervalles purs a effectuer
	for (nSourceIndex = 0; nSourceIndex < kwftSource->GetFrequencyVectorNumber(); nSourceIndex++)
	{
		// Extraction du vecteur d'effectifs
		ivSourceFrequencies =
		    cast(KWDenseFrequencyVector*, kwftSource->GetFrequencyVectorAt(nSourceIndex))->GetFrequencyVector();

		// Test si intervalle pur
		// Nombre d'instances de l'intervalle source
		nSourceFrequency = kwftSource->GetFrequencyVectorAt(nSourceIndex)->ComputeTotalFrequency();
		nPureTargetIndex = 0;
		while (ivSourceFrequencies->GetAt(nPureTargetIndex) == 0)
		{
			nPureTargetIndex++;
		}
		// Cas du premier intervalle
		if (nSourceIndex == 0)
			oaFrequencies.Add(ivSourceFrequencies->Clone());
		// Cas des intervalles suivants
		else
		{
			// Cas d'un intervalle pur
			if (ivSourceFrequencies->GetAt(nPureTargetIndex) == nSourceFrequency)
			{
				// Cas ou l'intervalle precedent etait pur Et qu'ils sont purs pour la meme valeur cible
				if (nPureTargetPrevIndex > -1 and nPureTargetPrevIndex == nPureTargetIndex)
				{
					ivFrequencies =
					    cast(IntVector*, oaFrequencies.GetAt(oaFrequencies.GetSize() - 1));

					// Ajout des effectifs de l'intervalle a l'intervalle precedent (fusion)
					for (nTargetIndex = 0; nTargetIndex < ivFrequencies->GetSize(); nTargetIndex++)
						ivFrequencies->SetAt(nTargetIndex,
								     ivFrequencies->GetAt(nTargetIndex) +
									 ivSourceFrequencies->GetAt(nTargetIndex));
				}
				// Sinon
				else
				{
					// Memorisation de la valeur cible de cet intervalle pur
					nPureTargetPrevIndex = nPureTargetIndex;

					// Ajout du vecteur d'effectifs
					oaFrequencies.Add(ivSourceFrequencies->Clone());
				}
			}
			// Cas d'un intervalle non pur
			else
			{
				// Taggage de l'intervalle comme non pur
				nPureTargetPrevIndex = -1;

				// Ajout du vecteur d'effectifs
				oaFrequencies.Add(ivSourceFrequencies->Clone());
			}
		}
	}

	// Creation de la table de contingence issue de la fusion
	kwftTarget = new KWFrequencyTable;
	kwftTarget->SetFrequencyVectorCreator(GetFrequencyVectorCreator()->Clone());
	kwftTarget->SetFrequencyVectorNumber(oaFrequencies.GetSize());
	kwftTarget->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
	kwftTarget->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
	kwftTarget->SetGranularity(kwftSource->GetGranularity());
	kwftTarget->SetGarbageModalityNumber(kwftSource->GetGarbageModalityNumber());

	for (nSourceIndex = 0; nSourceIndex < kwftTarget->GetFrequencyVectorNumber(); nSourceIndex++)
	{
		ivFrequencies = cast(IntVector*, oaFrequencies.GetAt(nSourceIndex));

		// Acces au vecteur (sense etre en representation dense)
		kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, kwftTarget->GetFrequencyVectorAt(nSourceIndex));

		// Recopie de son contenu
		ivFrequencyVector = kwdfvFrequencyVector->GetFrequencyVector();

		ivFrequencyVector->CopyFrom(ivFrequencies);

		// Nettoyage
		delete ivFrequencies;
	}
}

void KWDiscretizerMODL::DiscretizeFrequencyTable(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget) const
{
	KWMODLLine* headInterval;
	KWFrequencyTable kwftNullFrequencyTable;

	require(Check());
	require(kwftSource != NULL);
	require(kwftSource->Check());

	// Cas particulier ou il n'y a qu'une valeur source
	if (kwftSource->GetFrequencyVectorNumber() <= 1)
	{
		kwftTarget = kwftSource->Clone();
	}
	// Sinon, discretization
	else
	{
		// Optimisation de la liste des intervalles
		headInterval = IntervalListOptimization(kwftSource);

		// Construction de la table finale a partir de la liste d'intervalles
		kwftTarget = BuildFrequencyTableFromIntervalList(headInterval);

		// Parametrisation du nombre de valeurs
		kwftTarget->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
		kwftTarget->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
		// Parametrisation de la granularite et de la taille du groupe poubelle
		kwftTarget->SetGranularity(kwftSource->GetGranularity());
		kwftTarget->SetGarbageModalityNumber(0);

		// Nettoyage
		DeleteIntervalList(headInterval);
	}
	ensure(kwftSource->GetFrequencyVectorNumber() >= kwftTarget->GetFrequencyVectorNumber());
	ensure(kwftSource->GetFrequencyVectorSize() == kwftTarget->GetFrequencyVectorSize());
	ensure(kwftSource->GetTotalFrequency() == kwftTarget->GetTotalFrequency());
}

//////////////////////////////////////////////////////////////////////////////////////////
// Gestion des liste d'intervalles
KWFrequencyTable* KWDiscretizerMODL::BuildFrequencyTableFromIntervalList(KWMODLLine* headLine) const
{
	KWFrequencyTable* kwftFrequencyTable;
	int nLineNumber;
	KWMODLLine* line;
	int nSource;

	// Creation de la table
	kwftFrequencyTable = new KWFrequencyTable;

	// Calcul du nombre de ligne
	nLineNumber = GetIntervalListSize(headLine);

	// Dimensionnement et initialisation du contenu
	if (nLineNumber > 0)
	{
		// Dimensionnement
		kwftFrequencyTable->SetFrequencyVectorCreator(GetFrequencyVectorCreator()->Clone());
		kwftFrequencyTable->SetFrequencyVectorNumber(nLineNumber);

		// Initialisation
		line = headLine;
		for (nSource = 0; nSource < kwftFrequencyTable->GetFrequencyVectorNumber(); nSource++)
		{
			// Verifications de validite de la ligne courante
			check(line);

			// Transfert de la ligne courante
			kwftFrequencyTable->GetFrequencyVectorAt(nSource)->CopyFrom(line->GetFrequencyVector());

			// Ligne suivante
			line = line->GetNext();
		}
		assert(line == NULL);
	}
	return kwftFrequencyTable;
}

KWMODLLine* KWDiscretizerMODL::BuildIntervalListFromFrequencyTable(KWMODLLine* lineCreator,
								   const KWFrequencyTable* kwftTable) const
{
	KWMODLLine* headLine;
	IntVector ivIntervalLastLineIndexes;
	int i;

	require(kwftTable != NULL);
	require(lineCreator != NULL);

	// Creation d'un vecteur d'index de fin d'intervalle complet: un intervalle par vecteur d'effectif de la table
	ivIntervalLastLineIndexes.SetSize(kwftTable->GetFrequencyVectorNumber());
	for (i = 0; i < ivIntervalLastLineIndexes.GetSize(); i++)
		ivIntervalLastLineIndexes.SetAt(i, i);

	// Construction de la liste des intervalles complete
	headLine =
	    BuildIntervalListFromFrequencyTableAndIntervalBounds(lineCreator, kwftTable, &ivIntervalLastLineIndexes);

	ensure(kwftTable->GetFrequencyVectorNumber() == GetIntervalListSize(headLine));
	return headLine;
}

KWMODLLine* KWDiscretizerMODL::BuildIntervalListFromFrequencyTableAndIntervalBounds(
    KWMODLLine* lineCreator, const KWFrequencyTable* kwftTable, const IntVector* ivIntervalLastLineIndexes) const
{
	int nSource;
	KWMODLLine* headLine;
	KWMODLLine* line;
	int nFirstLineIndex;
	int nLastLineIndex;
	int nIntervalIndex;

	require(lineCreator != NULL);
	require(kwftTable != NULL);
	require(ivIntervalLastLineIndexes != NULL);
	require(ivIntervalLastLineIndexes->GetSize() > 0);

	// Creation des intervalles
	// (en partant de la fin pour faciliter le chainage dans la liste)
	headLine = NULL;
	for (nIntervalIndex = ivIntervalLastLineIndexes->GetSize() - 1; nIntervalIndex >= 0; nIntervalIndex--)
	{
		// Calcul de l'index de la premiere ligne de l'intervalle
		if (nIntervalIndex == 0)
			nFirstLineIndex = 0;
		else
			nFirstLineIndex = ivIntervalLastLineIndexes->GetAt(nIntervalIndex - 1) + 1;
		assert(0 <= nFirstLineIndex and nFirstLineIndex < kwftTable->GetFrequencyVectorNumber());

		// Calcul de l'index de la derniere ligne de l'intervalle
		nLastLineIndex = ivIntervalLastLineIndexes->GetAt(nIntervalIndex);
		assert(0 <= nLastLineIndex and nLastLineIndex < kwftTable->GetFrequencyVectorNumber());
		assert(nFirstLineIndex <= nLastLineIndex);

		// Creation et initialisation d'un intervalle
		line = lineCreator->Create();
		line->SetIndex(nLastLineIndex);
		InitializeFrequencyVector(line->GetFrequencyVector());

		// Recopie des effectifs des lignes constituant l'intervalle
		line->GetFrequencyVector()->CopyFrom(kwftTable->GetFrequencyVectorAt(nFirstLineIndex));
		for (nSource = nFirstLineIndex + 1; nSource <= nLastLineIndex; nSource++)
		{
			AddFrequencyVector(line->GetFrequencyVector(), kwftTable->GetFrequencyVectorAt(nSource));
		}

		// Cout de l'intervalle
		line->SetCost(ComputeIntervalCost(line->GetFrequencyVector()));

		// Chainage dans la liste
		if (headLine != NULL)
		{
			headLine->SetPrev(line);
			line->SetNext(headLine);
		}
		headLine = line;
	}
	ensure(ivIntervalLastLineIndexes->GetSize() == GetIntervalListSize(headLine));
	return headLine;
}

KWMODLLine* KWDiscretizerMODL::BuildUniqueIntervalFromFrequencyTable(KWMODLLine* lineCreator,
								     const KWFrequencyTable* kwftTable) const
{
	IntVector ivIntervalLastLineIndexes;
	KWMODLLine* headLine;

	require(lineCreator != NULL);
	require(kwftTable != NULL);

	// Parametrage d'une unique borne d'intervalle
	ivIntervalLastLineIndexes.SetSize(1);
	ivIntervalLastLineIndexes.SetAt(0, kwftTable->GetFrequencyVectorNumber() - 1);

	// Creation d'une liste mono-intervalle
	headLine =
	    BuildIntervalListFromFrequencyTableAndIntervalBounds(lineCreator, kwftTable, &ivIntervalLastLineIndexes);

	ensure(kwftTable->GetTotalFrequency() == headLine->GetFrequencyVector()->ComputeTotalFrequency());
	return headLine;
}

void KWDiscretizerMODL::WriteIntervalListReport(const KWMODLLine* headLine, ostream& ost) const
{
	const KWMODLLine* line;

	require(headLine != NULL);

	// Affichage de l'entete
	// Recherche de la fin de la liste (le debut peut contenir un entete tronque: les Merges
	// se font par exemple a partir du deuxiemme element)
	line = headLine;
	while (line != NULL and line->GetNext() != NULL)
		line = line->GetNext();
	if (line != NULL)
	{
		line->WriteHeaderLineReport(ost);
		ost << "\n";
	}

	// Affichage des intervalles
	line = headLine;
	while (line != NULL)
	{
		line->WriteLineReport(ost);
		ost << "\n";
		line = line->GetNext();
	}
}

KWMODLLine* KWDiscretizerMODL::CloneIntervalList(KWMODLLine* lineCreator, KWMODLLine* headLine) const
{
	KWMODLLine* headLineClone;
	KWMODLLine* line;
	KWMODLLine* lineClone;
	KWMODLLine* lineCloneNext;

	require(lineCreator != NULL);

	// On renvoie NULL si la liste initiale est NULL
	if (headLine == NULL)
		headLineClone = NULL;
	// Sinon, on duplique la liste
	else
	{
		// Duplication de la tete de liste
		headLineClone = lineCreator->Create();
		headLineClone->CopyFrom(headLine);

		// Duplication des elements suivants de la liste
		line = headLine;
		lineClone = headLineClone;
		while (line->GetNext() != NULL)
		{
			// Duplication de l'element suivant
			lineCloneNext = lineCreator->Create();
			lineCloneNext->CopyFrom(line->GetNext());

			// Chainage de l'element suivant
			lineClone->SetNext(lineCloneNext);
			lineCloneNext->SetPrev(lineClone);

			// Passage a la suite
			line = line->GetNext();
			lineClone = lineClone->GetNext();
		}
	}
	ensure(GetIntervalListSize(headLine) == GetIntervalListSize(headLineClone));
	return headLineClone;
}

double KWDiscretizerMODL::ComputeIntervalListPartitionGlobalCost(KWMODLLine* headLine,
								 const KWFrequencyTable* kwftTable) const
{
	boolean bPrintDetails = false;
	double dGlobalPartitionCost;
	int nListSize;
	KWMODLLine* line;

	require(headLine != NULL);
	require(kwftTable != NULL);

	// Initialisation de la structure de cout
	InitializeWorkingData(kwftTable);
	if (bPrintDetails)
		cout << "\nIntervalListPartitionGlobalCost"
		     << "\n";

	// Parcours des elements de la liste pour prendre en compte le cout des intervalles
	dGlobalPartitionCost = 0;
	nListSize = 0;
	line = headLine;
	while (line != NULL)
	{
		dGlobalPartitionCost += ComputeIntervalCost(line->GetFrequencyVector());
		nListSize++;
		if (bPrintDetails)
			cout << "\tLine " << nListSize << "\t" << ComputeIntervalCost(line->GetFrequencyVector())
			     << "\n";
		line = line->GetNext();
	}

	// On ajoute le cout de partition
	dGlobalPartitionCost += ComputePartitionCost(nListSize);
	if (bPrintDetails)
	{
		cout << "\t"
		     << "Partition"
		     << "\t" << ComputePartitionCost(nListSize) << "\n";
		cout << "\t"
		     << "Global cost"
		     << "\t" << dGlobalPartitionCost << "\n";
	}

	// Nettoyage de la structure de cout
	CleanWorkingData();

	return dGlobalPartitionCost;
}

int KWDiscretizerMODL::GetIntervalListSize(const KWMODLLine* headLine) const
{
	int nListSize;
	const KWMODLLine* line;

	// Parcours des elements de la liste pour les compter
	nListSize = 0;
	line = headLine;
	while (line != NULL)
	{
		nListSize++;
		line = line->GetNext();
	}
	return nListSize;
}

void KWDiscretizerMODL::DeleteIntervalList(KWMODLLine* headLine) const
{
	KWMODLLine* line;
	KWMODLLine* nextLine;

	// Destruction de chaque elements de la liste
	line = headLine;
	while (line != NULL)
	{
		nextLine = line->GetNext();
		delete line;
		line = nextLine;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Pilotage de l'optimisation

KWMODLLine* KWDiscretizerMODL::IntervalListOptimization(const KWFrequencyTable* kwftSource) const
{
	boolean bPrintOptimisationStatistics = false;
	boolean bPrintResults = false;
	KWMODLLineOptimization lineOptimizationCreator(GetFrequencyVectorCreator());
	KWMODLLineOptimization* headIntervalOptimization;
	KWMODLLineDeepOptimization lineDeepOptimizationCreator(GetFrequencyVectorCreator());
	KWMODLLineDeepOptimization* headIntervalDeepOptimization;
	KWMODLLine* headInterval;
	KWMODLLine* interval;

	require(Check());
	require(kwftSource != NULL);
	require(kwftSource->GetFrequencyVectorNumber() > 1);

	// Memorisation des donnees sur la table de contingence source
	InitializeWorkingData(kwftSource);

	// Initialisation des statistiques d'optimisation
	nMergeNumber = 0;
	nExtraMergeNumber = 0;
	nSplitNumber = 0;
	nExtraSplitNumber = 0;
	nMergeSplitNumber = 0;
	nMergeMergeSplitNumber = 0;

	////////////////////////////////////////////////////////////////////////////////
	// Algorithmes gloutons ascendants
	headInterval = NULL;
	if (GetParam() == GreedyMerge or GetParam() == OptimizedGreedyMerge)
	{
		// Construction de la liste des intervalles a partir de la table source
		headIntervalOptimization = cast(
		    KWMODLLineOptimization*, BuildIntervalListFromFrequencyTable(&lineOptimizationCreator, kwftSource));

		// Initialisation des couts des intervalles
		interval = headIntervalOptimization;
		while (interval != NULL)
		{
			// Cout de l'intervalle
			interval->SetCost(ComputeIntervalCost(interval->GetFrequencyVector()));

			// Passage a l'intervalle suivant
			interval = interval->GetNext();
		}

		// Optimisation basee sur une recherche ascendante gloutonne
		IntervalListMergeOptimization(headIntervalOptimization);
		headInterval = headIntervalOptimization;

		// Post-optimisation
		if (GetParam() == OptimizedGreedyMerge)
		{
			// Optimisation basee sur une recherche ascendante menee jusqu'a un seul
			// intervalle terminal, et memorisation de la meilleure solution rencontree
			IntervalListBestMergeOptimization(headIntervalOptimization);

			// Transfert de la liste optimisee vers une liste dediee a la post-optimisation
			// La premiere partie de l'optimisation basee uniquement sur des Merge utilise
			// la classe KWMODLLineOptimization pour des raisons de performance en place memoire.
			// La seconde partie utilise la classe KWMODLLineDeepOptimization pour pouvoir
			// gerer les Split, MergeSplit et MergeMergeSplit.
			headIntervalDeepOptimization =
			    cast(KWMODLLineDeepOptimization*,
				 CloneIntervalList(&lineDeepOptimizationCreator, headIntervalOptimization));
			DeleteIntervalList(headIntervalOptimization);
			debug(headIntervalOptimization = NULL);

			// Post-optimisation recherchant des ameliorations locale basees sur des
			// Split, des MergeSplit et des MergeMergeSplit
			IntervalListPostOptimization(kwftSource, headIntervalDeepOptimization);
			headInterval = headIntervalDeepOptimization;
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	// Algorithmes gloutons descendants
	else if (GetParam() == GreedySplit or GetParam() == OptimizedGreedySplit)
	{
		// Construction d'un intervalle unique, fusion de toutes les lignes
		// d'une table de contingence
		headIntervalDeepOptimization =
		    cast(KWMODLLineDeepOptimization*,
			 BuildUniqueIntervalFromFrequencyTable(&lineDeepOptimizationCreator, kwftSource));
		assert(headIntervalDeepOptimization->GetNext() == NULL);

		// Initialisation du cout de l'intervalle
		headIntervalDeepOptimization->SetCost(
		    ComputeIntervalCost(headIntervalDeepOptimization->GetFrequencyVector()));

		// Optimisation basee sur une recherche descendante gloutonne
		IntervalListSplitOptimization(kwftSource, headIntervalDeepOptimization);
		headInterval = headIntervalDeepOptimization;

		// Post-optimisation
		if (GetParam() == OptimizedGreedySplit)
		{
			// Optimisation basee sur une recherche descendante menee jusqu'aux
			// intervalles elementaires, et memorisation de la meilleure solution rencontree
			IntervalListBestSplitOptimization(kwftSource, headIntervalDeepOptimization);

			// Post-optimisation rechercheant des amelioration locale basees sur des
			// Split, des MergeSplit et des MergeMergeSplit
			IntervalListPostOptimization(kwftSource, headIntervalDeepOptimization);
			headInterval = headIntervalDeepOptimization;
		}
	}
	///////////////////////////////////////////////////////////////////////////////
	// Algorithme optimal
	else
	{
		assert(GetParam() == Optimal);
		headInterval = ComputeOptimalDiscretization(kwftSource);
	}

	// Affichage des statistiques d'optimisation
	if (bPrintOptimisationStatistics)
	{
		cout << kwftSource->GetTotalFrequency() << "\t" << kwftSource->GetFrequencyVectorNumber() << "\t"
		     << nMergeNumber << "\t" << nExtraMergeNumber << "\t" << nSplitNumber << "\t" << nExtraSplitNumber
		     << "\t" << nMergeSplitNumber << "\t" << nMergeMergeSplitNumber << "\t"
		     << GetIntervalListSize(headInterval);
	}

	// Affichage des resultats
	if (bPrintResults)
	{
		KWFrequencyTable* kwftTarget;
		int nInterval;

		// Construction de la table finale a partir de la liste d'intervalles
		kwftTarget = BuildFrequencyTableFromIntervalList(headInterval);

		kwftTarget->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
		kwftTarget->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
		kwftTarget->SetGranularity(kwftSource->GetGranularity());
		kwftTarget->SetGarbageModalityNumber(kwftSource->GetGarbageModalityNumber());

		// Evaluation de la discretisation
		cout << "IntervalListOptimization" << endl;
		cout << "\t" << ComputeDiscretizationCost(kwftTarget);

		// Effectifs des intervalles
		for (nInterval = 0; nInterval < kwftTarget->GetFrequencyVectorNumber(); nInterval++)
			cout << "\t" << kwftTarget->GetFrequencyVectorAt(nInterval)->ComputeTotalFrequency();

		// Nettoyage
		delete kwftTarget;
	}

	if (bPrintOptimisationStatistics or bPrintResults)
		cout << endl;

	// Nettoyage
	CleanWorkingData();
	nMergeNumber = 0;
	nExtraMergeNumber = 0;
	nSplitNumber = 0;
	nExtraSplitNumber = 0;
	nMergeSplitNumber = 0;
	nMergeMergeSplitNumber = 0;

	ensure(headInterval != NULL);
	return headInterval;
}

void KWDiscretizerMODL::IntervalListBestMergeOptimization(KWMODLLineOptimization*& headInterval) const
{
	KWMODLLineOptimization* headIntervalCopy;
	SortedList mergeList(KWMODLLineOptimizationCompareMergeDeltaCost);
	KWMODLLineOptimization* interval;
	KWMODLLineOptimization* intervalCopy;
	double dBestTotalCost;
	int nBestIntervalNumber;
	int nIntervalNumber;
	double dDiscretizationDeltaCost;
	double dTotalCost;
	int nMinIntervalNumber;

	require(headInterval != NULL);

	// Duplication de la structure de la liste
	headIntervalCopy = cast(KWMODLLineOptimization*, CloneIntervalList(headInterval, headInterval));

	// Recopie des informations de merge
	interval = headInterval;
	intervalCopy = headIntervalCopy;
	while (interval != NULL)
	{
		// Recopie des informations de merge
		intervalCopy->GetMerge()->CopyFrom(interval->GetMerge());

		// On verifie que les informations de Merge ont ete initialisee
		assert(interval->GetPrev() == NULL or fabs(interval->GetMerge()->GetDeltaCost() -
							   (interval->GetMerge()->GetCost() - interval->GetCost() -
							    interval->GetPrev()->GetCost())) < dEpsilon);

		// Intervalle suivant
		interval = cast(KWMODLLineOptimization*, interval->GetNext());
		intervalCopy = cast(KWMODLLineOptimization*, intervalCopy->GetNext());
	}

	// Initialisation de la liste triee
	InitializeMergeSortedList(&mergeList, headIntervalCopy);
	assert(mergeList.GetCount() == GetIntervalListSize(headIntervalCopy) - 1);

	// Recherche iterative du meilleur merge; recherche mene a son terme
	// jusqu'a un seul intervalle terminal pour trouver un minimum local
	// Cette recherche est menee sur la copie de la liste originelle
	dTotalCost = 0;
	dBestTotalCost = dTotalCost;
	nIntervalNumber = mergeList.GetCount() + 1;
	nBestIntervalNumber = mergeList.GetCount() + 1;
	nMinIntervalNumber = 1;

	while (mergeList.GetCount() > 0)
	{
		// Recherche du meilleur merge (DeltaCost le plus petit)
		interval = cast(KWMODLLineOptimization*, mergeList.GetHead());

		// Variation du cout du au codage d'un intervalle en moins
		// Pas de groupe poubelle : discretisation ou groupage sans recherche de poubelle
		dDiscretizationDeltaCost = ComputePartitionDeltaCost(nIntervalNumber, 0);

		// Prise en compte de la variation de cout de codage des exceptions
		dDiscretizationDeltaCost += interval->GetMerge()->GetDeltaCost();

		// Mise a jour la liste des intervalles
		UpdateMergeSortedList(&mergeList, headIntervalCopy, interval);
		nIntervalNumber--;

		// Test si diminution du cout total, ou si contrainte de nombre max
		// d'intervalles non respectee (avec tolerance de dEpsilon pour gerer
		// les probleme d'erreur numerique, en favorisant les Merge)
		dTotalCost += dDiscretizationDeltaCost;
		// CH RefontePrior2-P
		// On ne memorise pas le dernier merge si poubelle
		if ((dTotalCost <= dBestTotalCost + dEpsilon and nIntervalNumber >= nMinIntervalNumber)
		    // Fin CH RefontePrior2
		    or (GetMaxIntervalNumber() > 0 and nIntervalNumber > GetMaxIntervalNumber()))
		{
			dBestTotalCost = dTotalCost;
			nBestIntervalNumber = nIntervalNumber;
		}
	}
	assert(mergeList.GetCount() == 0);

	// Nettoyage des donnees de travail intermediaires
	DeleteIntervalList(headIntervalCopy);

	// Initialisation de la liste triee pour repartir du meme point de depart,
	InitializeMergeSortedList(&mergeList, headInterval);
	assert(mergeList.GetCount() == GetIntervalListSize(headInterval) - 1);

	// Recherche iterative du meilleur merge, jusqu'a reobtenir le nombre
	// d'intervalle optimum
	// Cette methode est moins couteuse que de memoriser la liste entiere
	// des intervalle a chaque amelioration des resultats
	nIntervalNumber = mergeList.GetCount() + 1;
	while (nIntervalNumber > nBestIntervalNumber)
	{
		// Recherche du meilleur merge (DeltaCost le plus petit)
		interval = cast(KWMODLLineOptimization*, mergeList.GetHead());

		// Mise a jour la liste des intervalles
		UpdateMergeSortedList(&mergeList, headInterval, interval);
		nIntervalNumber--;
		nExtraMergeNumber++;
	}
}

void KWDiscretizerMODL::IntervalListBestSplitOptimization(const KWFrequencyTable* kwftSource,
							  KWMODLLineDeepOptimization* headInterval) const
{
	KWMODLLineDeepOptimization* headIntervalCopy;
	SortedList splitList(KWMODLLineDeepOptimizationCompareSplitDeltaCost);
	KWMODLLineDeepOptimization* interval;
	KWMODLLineDeepOptimization* intervalCopy;
	double dBestTotalCost;
	int nBestIntervalNumber;
	int nIntervalNumber;
	double dDiscretizationDeltaCost;
	double dTotalCost;

	require(headInterval != NULL);

	// Duplication de la structure de la liste
	headIntervalCopy = cast(KWMODLLineDeepOptimization*, CloneIntervalList(headInterval, headInterval));

	// Recopie des informations de Split
	interval = headInterval;
	intervalCopy = headIntervalCopy;
	while (interval != NULL)
	{
		// Recopie des informations de Split
		intervalCopy->GetSplit()->CopyFrom(interval->GetSplit());

		// Intervalle suivant
		interval = cast(KWMODLLineDeepOptimization*, interval->GetNext());
		intervalCopy = cast(KWMODLLineDeepOptimization*, intervalCopy->GetNext());
	}

	// Initialisation de la liste triee
	InitializeSplitSortedList(&splitList, headIntervalCopy);
	assert(splitList.GetCount() == GetIntervalListSize(headIntervalCopy));

	// Recherche iterative du meilleur split; recherche mene a son terme
	// jusqu'aux intervalles elementaires pour trouver un minimum local
	// Cette recherche est menee sur la copie de la liste originelle
	dTotalCost = 0;
	dBestTotalCost = dTotalCost;
	nIntervalNumber = splitList.GetCount();
	nBestIntervalNumber = splitList.GetCount();
	while (splitList.GetCount() > 0)
	{
		// Recherche du meilleur split (DeltaCost le plus petit)
		interval = cast(KWMODLLineDeepOptimization*, splitList.GetHead());

		// Arret de la recherche si pas de Split possible, ou si le nombre
		// max d'intervalle est atteint
		if (interval->GetSplit()->GetDeltaCost() == dInfiniteCost or
		    (GetMaxIntervalNumber() > 0 and nIntervalNumber > GetMaxIntervalNumber()))
			break;

		// Variation du cout du au codage d'un intervalle en moins
		dDiscretizationDeltaCost = -ComputePartitionDeltaCost(nIntervalNumber + 1);

		// Prise en compte de la variation de cout de codage des exceptions
		dDiscretizationDeltaCost += interval->GetSplit()->GetDeltaCost();

		// Mise a jour la liste des intervalles
		UpdateSplitSortedList(kwftSource, &splitList, headIntervalCopy, interval);
		nIntervalNumber++;

		// Test si diminution du cout total, ou si contrainte de nombre max
		// d'intervalles non respectee (avec marge de dEpsilon, pour gerer les
		// erreurs numeriques en defavorisant les Splits)
		dTotalCost += dDiscretizationDeltaCost;
		if (dTotalCost < dBestTotalCost - dEpsilon)
		{
			dBestTotalCost = dTotalCost;
			nBestIntervalNumber = nIntervalNumber;
		}
	}

	// Nettoyage des donnees de travail intermediaires
	DeleteIntervalList(headIntervalCopy);
	splitList.RemoveAll();

	// Initialisation de la liste triee pour repartir du meme point de depart,
	InitializeSplitSortedList(&splitList, headInterval);
	assert(splitList.GetCount() == GetIntervalListSize(headInterval));

	// Recherche iterative du meilleur split, jusqu'a reobtenir le nombre
	// d'intervalle optimum
	// Cette methode est moins couteuse que de memoriser la liste entiere
	// des intervalle a chaque amelioration des resultats
	nIntervalNumber = splitList.GetCount();
	while (nIntervalNumber < nBestIntervalNumber)
	{
		// Recherche du meilleur split (DeltaCost le plus petit)
		interval = cast(KWMODLLineDeepOptimization*, splitList.GetHead());

		// Mise a jour la liste des intervalles
		UpdateSplitSortedList(kwftSource, &splitList, headInterval, interval);
		nIntervalNumber++;
		nExtraSplitNumber++;
	}
}

void KWDiscretizerMODL::IntervalListPostOptimization(const KWFrequencyTable* kwftSource,
						     KWMODLLineDeepOptimization*& headInterval) const
{
	boolean bPrintOptimisationDetails = false;
	SortedList splitList(KWMODLLineDeepOptimizationCompareSplitDeltaCost);
	SortedList mergeSplitList(KWMODLLineDeepOptimizationCompareMergeSplitDeltaCost);
	SortedList mergeMergeSplitList(KWMODLLineDeepOptimizationCompareMergeMergeSplitDeltaCost);
	boolean bContinue;
	KWMODLLineDeepOptimization* splitInterval;
	KWMODLLineDeepOptimization* mergeSplitInterval;
	KWMODLLineDeepOptimization* mergeMergeSplitInterval;
	double dSplitDeltaCost;
	double dMergeSplitDeltaCost;
	double dMergeMergeSplitDeltaCost;
	double dBestDeltaCost;
	int nIntervalNumber;
	int nSurnumerousIntervalNumber;
	int nStepNumber;
	PeriodicTest periodicTestOptimize;

	require(kwftSource != NULL);
	require(headInterval != NULL);

	// Initialisation des listes d'ameliorations
	InitializeSplitList(kwftSource, headInterval);
	InitializeMergeSplitList(kwftSource, headInterval);
	InitializeMergeMergeSplitList(kwftSource, headInterval);

	// Insertion des ameliorations dans des liste triee
	InitializeSplitSortedList(&splitList, headInterval);
	InitializeMergeSplitSortedList(&mergeSplitList, headInterval);
	InitializeMergeMergeSplitSortedList(&mergeMergeSplitList, headInterval);
	assert(CheckPostOptimizationSortedLists(&splitList, &mergeSplitList, &mergeMergeSplitList, headInterval));

	// Calcul du nombre d'intervalle initial
	nIntervalNumber = splitList.GetCount();
	assert(nIntervalNumber == GetIntervalListSize(headInterval));

	// Affichage de l'en-tete des messages
	if (bPrintOptimisationDetails)
	{
		cout << "Best Delta Cost"
		     << "\t"
		     << "Model Delta Cost"
		     << "\t"
		     << "Intervalles"
		     << "\t"
		     << "Optimisation"
		     << "\t";
		headInterval->WriteHeaderLineReport(cout);
		cout << endl;
	}

	// Recherche iterative de la meilleure amelioration
	bContinue = true;
	nStepNumber = 0;
	while (bContinue)
	{
		nStepNumber++;

		// Test si arret de tache demandee
		if (periodicTestOptimize.IsTestAllowed(0) and TaskProgression::IsInterruptionRequested())
			break;

		// Calcul du nombre d'inetrvalles surnumeraires pour la prise en compte de
		// la contrainte de nombre max d'intervalles
		nSurnumerousIntervalNumber = -1;
		if (GetMaxIntervalNumber() > 0)
			nSurnumerousIntervalNumber = nIntervalNumber - GetMaxIntervalNumber();

		// Recherche du meilleur Split et de son cout
		splitInterval = cast(KWMODLLineDeepOptimization*, splitList.GetHead());
		if (splitInterval->GetSplit()->GetDeltaCost() < dInfiniteCost)
			dSplitDeltaCost = -ComputePartitionDeltaCost(nIntervalNumber + 1, 0) +
					  splitInterval->GetSplit()->GetDeltaCost();
		else
		{
			splitInterval = NULL;
			dSplitDeltaCost = dInfiniteCost;
		}

		// Recherche du meilleur MergeSplit et de son cout
		if (mergeSplitList.GetCount() > 0)
		{
			mergeSplitInterval = cast(KWMODLLineDeepOptimization*, mergeSplitList.GetHead());
			dMergeSplitDeltaCost = mergeSplitInterval->GetMergeSplit()->GetDeltaCost();
		}
		else
		{
			mergeSplitInterval = NULL;
			dMergeSplitDeltaCost = dInfiniteCost;
		}

		// Recherche du meilleur MergeMergeSplit et de son cout
		if (mergeMergeSplitList.GetCount() > 0)
		{
			mergeMergeSplitInterval = cast(KWMODLLineDeepOptimization*, mergeMergeSplitList.GetHead());
			dMergeMergeSplitDeltaCost = ComputePartitionDeltaCost(nIntervalNumber, 0) +
						    mergeMergeSplitInterval->GetMergeMergeSplit()->GetDeltaCost();

			// Si la contrainte de nombre max d'intervalle n'est pas respectee,
			// le MergeMergeSplit devient prioritaire
			if (nSurnumerousIntervalNumber > 0)
				dMergeMergeSplitDeltaCost += dPriorityDeltaCost;
		}
		else
		{
			mergeMergeSplitInterval = NULL;
			dMergeMergeSplitDeltaCost = dInfiniteCost;
		}

		// Gestion des problemes numeriques pour les valeur proches de zero
		if (fabs(dSplitDeltaCost) < dEpsilon)
			dSplitDeltaCost = 0;
		if (fabs(dMergeSplitDeltaCost) < dEpsilon)
			dMergeSplitDeltaCost = 0;
		if (fabs(dMergeMergeSplitDeltaCost) < dEpsilon)
			dMergeMergeSplitDeltaCost = 0;

		// Calcul de la plus petite variation de cout, en privilegiant les
		// optimisation diminuant le nombre d'intervalles
		// Initialisation avec un cout infini, pour gestion de la contrainte
		// de nombre max d'intervalles
		dBestDeltaCost = dInfiniteCost;

		// Prise en compte des Split si la contrainte de nombre max d'intervalles
		// est respectee apres le Split
		if (nSurnumerousIntervalNumber < 0)
			dBestDeltaCost = dSplitDeltaCost;

		// Prise en compte des MergeSplit si la contrainte de nombre max d'intervalles
		// est respectee
		if (nSurnumerousIntervalNumber <= 0 and dMergeSplitDeltaCost < dBestDeltaCost + dEpsilon)
			dBestDeltaCost = dMergeSplitDeltaCost;

		// Prise en compte des MergeMergeSplit
		if (dMergeMergeSplitDeltaCost < dBestDeltaCost + dEpsilon)
			dBestDeltaCost = dMergeMergeSplitDeltaCost;

		// Pas d'optimisation si la variation de cout est strictement positive
		if (dBestDeltaCost > 0)
		{
			bContinue = false;

			// Affichage d'un message
			if (bPrintOptimisationDetails)
			{
				cout << dBestDeltaCost << "\t";
				if (nIntervalNumber > 1)
					cout << ComputePartitionDeltaCost(nIntervalNumber, 0);
				cout << "\t";
				cout << nIntervalNumber << endl;
			}
		}
		// On effectue en priorite les MergeMergeSplit
		else if (dBestDeltaCost == dMergeMergeSplitDeltaCost)
		{
			// Affichage d'un message
			if (bPrintOptimisationDetails)
			{
				cout << dBestDeltaCost << "\t" << ComputePartitionDeltaCost(nIntervalNumber, 0) << "\t"
				     << nIntervalNumber << "\tMergeMergeSplit\t";
				mergeMergeSplitInterval->WriteLineReport(cout);
				cout << endl;
			}

			// Mise a jour des listes d'optimisation
			UpdatePostOptimizationSortedListsWithMergeMergeSplit(kwftSource, &splitList, &mergeSplitList,
									     &mergeMergeSplitList, headInterval,
									     mergeMergeSplitInterval);
			nIntervalNumber--;
			nMergeMergeSplitNumber++;
		}
		// En cas de variation nulle, arret si pas de diminution du nombre d'intervalles
		else if (dBestDeltaCost == 0)
		{
			bContinue = false;

			// Affichage d'un message
			if (bPrintOptimisationDetails)
			{
				cout << dBestDeltaCost << "\t" << ComputePartitionDeltaCost(nIntervalNumber, 0) << "\t"
				     << nIntervalNumber << endl;
			}
		}
		// Test si MergeSplit (prioritaire par rapport au Split)
		else if (dBestDeltaCost == dMergeSplitDeltaCost)
		{
			// Affichage d'un message
			if (bPrintOptimisationDetails)
			{
				cout << dBestDeltaCost << "\t" << 0 << "\t" << nIntervalNumber << "\tMergeSplit\t";
				mergeSplitInterval->WriteLineReport(cout);
				cout << endl;
			}

			// Mise a jour des listes d'optimisation
			UpdatePostOptimizationSortedListsWithMergeSplit(kwftSource, &splitList, &mergeSplitList,
									&mergeMergeSplitList, headInterval,
									mergeSplitInterval);
			nMergeSplitNumber++;
		}
		// Test si Split
		else
		{
			// Affichage d'un message
			if (bPrintOptimisationDetails)
			{
				cout << dBestDeltaCost << "\t" << -ComputePartitionDeltaCost(nIntervalNumber + 1, 0)
				     << "\t" << nIntervalNumber << "\tSplit\t";
				splitInterval->WriteLineReport(cout);
				cout << endl;
			}

			// Mise a jour des listes d'optimisation
			assert(fabs(dBestDeltaCost - dSplitDeltaCost) < dEpsilon);
			UpdatePostOptimizationSortedListsWithSplit(kwftSource, &splitList, &mergeSplitList,
								   &mergeMergeSplitList, headInterval, splitInterval);
			nIntervalNumber++;
			nSplitNumber++;
		}
	}
}

void KWDiscretizerMODL::IntervalListBoundaryPostOptimization(const KWFrequencyTable* kwftSource,
							     KWMODLLineDeepOptimization*& headInterval) const
{
	boolean bPrintOptimisationDetails = false;
	SortedList mergeSplitList(KWMODLLineDeepOptimizationCompareMergeSplitDeltaCost);
	boolean bContinue;
	KWMODLLineDeepOptimization* mergeSplitInterval;
	double dBestDeltaCost;
	int nIntervalNumber;
	int nStepNumber;
	PeriodicTest periodicTestOptimize;

	require(kwftSource != NULL);
	require(headInterval != NULL);

	// Initialisation de la liste d'ameliorations
	InitializeMergeSplitList(kwftSource, headInterval);

	// Insertion des ameliorations dans une liste triee
	InitializeMergeSplitSortedList(&mergeSplitList, headInterval);

	// Calcul du nombre d'intervalle initial
	nIntervalNumber = mergeSplitList.GetCount() + 1;
	assert(nIntervalNumber == GetIntervalListSize(headInterval));

	// Affichage de l'en-tete des messages
	if (bPrintOptimisationDetails)
	{
		cout << "Best Delta Cost"
		     << "\t"
		     << "Model Delta Cost"
		     << "\t"
		     << "Intervalles"
		     << "\t"
		     << "Optimisation"
		     << "\t";
		headInterval->WriteHeaderLineReport(cout);
		cout << endl;
	}

	// Recherche iterative de la meilleure amelioration
	bContinue = true;
	nStepNumber = 0;
	while (bContinue)
	{
		nStepNumber++;

		// Test si arret de tache demandee
		if (periodicTestOptimize.IsTestAllowed(0) and TaskProgression::IsInterruptionRequested())
			break;

		// Recherche du meilleur MergeSplit et de son cout
		if (mergeSplitList.GetCount() > 0)
		{
			mergeSplitInterval = cast(KWMODLLineDeepOptimization*, mergeSplitList.GetHead());
			dBestDeltaCost = mergeSplitInterval->GetMergeSplit()->GetDeltaCost();
		}
		else
		{
			mergeSplitInterval = NULL;
			dBestDeltaCost = dInfiniteCost;
		}

		// Gestion des problemes numeriques pour les valeur proches de zero
		if (fabs(dBestDeltaCost) < dEpsilon)
			dBestDeltaCost = 0;

		// Pas d'optimisation si la variation de cout est positive ou nulle
		if (dBestDeltaCost >= 0)
		{
			bContinue = false;

			// Affichage d'un message
			if (bPrintOptimisationDetails)
			{
				cout << dBestDeltaCost << "\t";
				if (nIntervalNumber > 1)
					cout << ComputePartitionDeltaCost(nIntervalNumber);
				cout << "\t";
				cout << nIntervalNumber << endl;
			}
		}
		// Test si MergeSplit
		else
		{
			// Affichage d'un message
			if (bPrintOptimisationDetails)
			{
				cout << dBestDeltaCost << "\t" << 0 << "\t" << nIntervalNumber << "\tMergeSplit\t";
				mergeSplitInterval->WriteLineReport(cout);
				cout << endl;
			}

			// Mise a jour des listes d'optimisation
			UpdateBoundaryPostOptimizationSortedListWithMergeSplit(kwftSource, &mergeSplitList,
									       headInterval, mergeSplitInterval);
			nMergeSplitNumber++;
		}
	}
}

void KWDiscretizerMODL::UpdatePostOptimizationSortedListsWithSplit(const KWFrequencyTable* kwftSource,
								   SortedList* splitList, SortedList* mergeSplitList,
								   SortedList* mergeMergeSplitList,
								   KWMODLLineDeepOptimization*& headInterval,
								   KWMODLLineDeepOptimization* interval) const
{
	KWMODLLineDeepOptimization* prevInterval;
	KWMODLLineDeepOptimization* prevPrevInterval;
	KWMODLLineDeepOptimization* nextInterval;
	KWMODLLineDeepOptimization* nextNextInterval;
	KWMODLLineDeepOptimization* newInterval;

	require(kwftSource != NULL);
	require(splitList != NULL);
	require(mergeSplitList != NULL);
	require(mergeMergeSplitList != NULL);
	require(headInterval != NULL);
	require(interval != NULL);
	require(CheckFrequencyVector(interval->GetFrequencyVector()));
	require(CheckPostOptimizationSortedLists(splitList, mergeSplitList, mergeMergeSplitList, headInterval));

	// Recherche des intervalles precedents et suivants
	prevInterval = cast(KWMODLLineDeepOptimization*, interval->GetPrev());
	prevPrevInterval = NULL;
	if (prevInterval != NULL)
		prevPrevInterval = cast(KWMODLLineDeepOptimization*, prevInterval->GetPrev());
	nextInterval = cast(KWMODLLineDeepOptimization*, interval->GetNext());
	nextNextInterval = NULL;
	if (nextInterval != NULL)
		nextNextInterval = cast(KWMODLLineDeepOptimization*, nextInterval->GetNext());

	////////////////////////////////////////////////////////////////////////////////
	// On supprime les intervalles necessaires des listes

	// On retire l'intervalle de la liste triee des Split
	splitList->RemoveAt(interval->GetSplit()->GetPosition());
	debug(interval->GetSplit()->SetPosition(NULL));

	// On retire les deux intervalles (potentiels) de la liste triee des MergeSplit
	if (prevInterval != NULL)
	{
		mergeSplitList->RemoveAt(interval->GetMergeSplit()->GetPosition());
		debug(interval->GetMergeSplit()->SetPosition(NULL));
	}
	if (nextInterval != NULL)
	{
		mergeSplitList->RemoveAt(nextInterval->GetMergeSplit()->GetPosition());
		debug(nextInterval->GetMergeSplit()->SetPosition(NULL));
	}

	// On retire les trois intervalles (potentiels) de la liste triee des MergeMergeSplit
	if (prevPrevInterval != NULL)
	{
		mergeMergeSplitList->RemoveAt(interval->GetMergeMergeSplit()->GetPosition());
		debug(interval->GetMergeMergeSplit()->SetPosition(NULL));
	}
	if (nextInterval != NULL and prevInterval != NULL)
	{
		mergeMergeSplitList->RemoveAt(nextInterval->GetMergeMergeSplit()->GetPosition());
		debug(nextInterval->GetMergeMergeSplit()->SetPosition(NULL));
	}
	if (nextNextInterval != NULL)
	{
		mergeMergeSplitList->RemoveAt(nextNextInterval->GetMergeMergeSplit()->GetPosition());
		debug(nextNextInterval->GetMergeMergeSplit()->SetPosition(NULL));
	}

	// On retire l'intervalle splitte de la liste triee selon les effectifs
	RemoveIntervalFromWorkingFrequencyList(interval);

	/////////////////////////////////////////////////////////////////////////////////////
	// Mise a jour des intervalles en fonction de l'optimisation effectuee

	// Modification en cas de Split
	// Le nouvel intervalle est insere apres l'intervalle courant
	newInterval = new KWMODLLineDeepOptimization(GetFrequencyVectorCreator());
	newInterval->SetNext(nextInterval);
	if (nextInterval != NULL)
		nextInterval->SetPrev(newInterval);
	interval->SetNext(newInterval);
	newInterval->SetPrev(interval);

	// Reconstruction des nouveaux vecteurs d'effectifs
	InitializeFrequencyVector(newInterval->GetFrequencyVector());
	SplitFrequencyVector(interval->GetFrequencyVector(), newInterval->GetFrequencyVector(),
			     interval->GetSplit()->GetFirstSubLineFrequencyVector());

	// Transfert des informations vers les deux nouveaux intervalles
	newInterval->SetIndex(interval->GetIndex());
	interval->SetIndex(interval->GetSplit()->GetFirstSubLineIndex());
	interval->SetCost(interval->GetSplit()->GetFirstSubLineCost());
	newInterval->SetCost(interval->GetSplit()->GetSecondSubLineCost());

	// Verification des nombres de modalites
	// Par defaut, quand il n'y a pas de suivi du nombre de modalites, les deux membres valent 0
	assert(newInterval->GetModalityNumber() ==
	       ComputeModalityNumber(kwftSource, interval->GetIndex() + 1, newInterval->GetIndex()));
	if (prevInterval != NULL)
		assert(interval->GetModalityNumber() ==
		       ComputeModalityNumber(kwftSource, prevInterval->GetIndex() + 1, interval->GetIndex()));
	else
		assert(interval->GetModalityNumber() == ComputeModalityNumber(kwftSource, 0, interval->GetIndex()));

	// Insertion des intervalles dans la liste triee des effectifs
	AddIntervalToWorkingFrequencyList(newInterval);
	AddIntervalToWorkingFrequencyList(interval);

	////////////////////////////////////////////////////////////////////////////////////
	// Calcul des nouvelles optimisations potentielles induites par les changements, et
	// insertion dans les liste

	// Calcul des nouveaux Splits bases sur le nouvel intervalle,
	// et reinsertion dans la liste triee
	ComputeIntervalSplit(kwftSource, interval);
	interval->GetSplit()->SetPosition(splitList->Add(interval));
	ComputeIntervalSplit(kwftSource, newInterval);
	newInterval->GetSplit()->SetPosition(splitList->Add(newInterval));

	// Calcul des nouveaux MergeSplits bases sur le nouvel intervalle,
	// et reinsertion dans la liste triee
	if (prevInterval != NULL)
	{
		ComputeIntervalMergeSplit(kwftSource, interval);
		interval->GetMergeSplit()->SetPosition(mergeSplitList->Add(interval));
	}
	ComputeIntervalMergeSplit(kwftSource, newInterval);
	newInterval->GetMergeSplit()->SetPosition(mergeSplitList->Add(newInterval));
	if (nextInterval != NULL)
	{
		ComputeIntervalMergeSplit(kwftSource, nextInterval);
		nextInterval->GetMergeSplit()->SetPosition(mergeSplitList->Add(nextInterval));
	}

	// Calcul des nouveaux MergeMergeSplits bases sur le nouvel intervalle,
	// et reinsertion dans la liste triee
	if (prevPrevInterval != NULL)
	{
		ComputeIntervalMergeMergeSplit(kwftSource, interval);
		interval->GetMergeMergeSplit()->SetPosition(mergeMergeSplitList->Add(interval));
	}
	if (prevInterval != NULL)
	{
		ComputeIntervalMergeMergeSplit(kwftSource, newInterval);
		newInterval->GetMergeMergeSplit()->SetPosition(mergeMergeSplitList->Add(newInterval));
	}
	if (nextInterval != NULL)
	{
		ComputeIntervalMergeMergeSplit(kwftSource, nextInterval);
		nextInterval->GetMergeMergeSplit()->SetPosition(mergeMergeSplitList->Add(nextInterval));
	}
	if (nextNextInterval != NULL)
	{
		ComputeIntervalMergeMergeSplit(kwftSource, nextNextInterval);
		nextNextInterval->GetMergeMergeSplit()->SetPosition(mergeMergeSplitList->Add(nextNextInterval));
	}
	ensure(CheckPostOptimizationSortedLists(splitList, mergeSplitList, mergeMergeSplitList, headInterval));
}

void KWDiscretizerMODL::UpdatePostOptimizationSortedListsWithMergeSplit(const KWFrequencyTable* kwftSource,
									SortedList* splitList,
									SortedList* mergeSplitList,
									SortedList* mergeMergeSplitList,
									KWMODLLineDeepOptimization*& headInterval,
									KWMODLLineDeepOptimization* interval) const
{
	KWMODLLineDeepOptimization* prevInterval;
	KWMODLLineDeepOptimization* prevPrevInterval;
	KWMODLLineDeepOptimization* nextInterval;
	KWMODLLineDeepOptimization* nextNextInterval;

	require(kwftSource != NULL);
	require(splitList != NULL);
	require(mergeSplitList != NULL);
	require(mergeMergeSplitList != NULL);
	require(headInterval != NULL);
	require(headInterval->GetNext() != NULL);
	require(interval != NULL);
	require(interval->GetPrev() != NULL);
	require(CheckFrequencyVector(interval->GetFrequencyVector()));
	require(CheckFrequencyVector(interval->GetPrev()->GetFrequencyVector()));
	require(CheckPostOptimizationSortedLists(splitList, mergeSplitList, mergeMergeSplitList, headInterval));

	// Recherche des intervalles precedents et suivants
	prevInterval = cast(KWMODLLineDeepOptimization*, interval->GetPrev());
	prevPrevInterval = cast(KWMODLLineDeepOptimization*, prevInterval->GetPrev());
	nextInterval = cast(KWMODLLineDeepOptimization*, interval->GetNext());
	nextNextInterval = NULL;
	if (nextInterval != NULL)
		nextNextInterval = cast(KWMODLLineDeepOptimization*, nextInterval->GetNext());

	////////////////////////////////////////////////////////////////////////////////
	// On supprime les intervalles necessaires des listes

	// On retire les deux intervalles de la liste triee des Split
	splitList->RemoveAt(interval->GetSplit()->GetPosition());
	debug(interval->GetSplit()->SetPosition(NULL));
	splitList->RemoveAt(prevInterval->GetSplit()->GetPosition());
	debug(prevInterval->GetSplit()->SetPosition(NULL));

	// On retire les trois intervalles (potentiels) de la liste triee des MergeSplit
	mergeSplitList->RemoveAt(interval->GetMergeSplit()->GetPosition());
	debug(interval->GetMergeSplit()->SetPosition(NULL));
	if (prevPrevInterval != NULL)
	{
		mergeSplitList->RemoveAt(prevInterval->GetMergeSplit()->GetPosition());
		debug(prevInterval->GetMergeSplit()->SetPosition(NULL));
	}
	if (nextInterval != NULL)
	{
		mergeSplitList->RemoveAt(nextInterval->GetMergeSplit()->GetPosition());
		debug(nextInterval->GetMergeSplit()->SetPosition(NULL));
	}

	// On retire les quatre intervalles (potentiels) de la liste triee des MergeMergeSplit
	if (prevPrevInterval != NULL and prevPrevInterval->GetPrev() != NULL)
	{
		mergeMergeSplitList->RemoveAt(prevInterval->GetMergeMergeSplit()->GetPosition());
		debug(prevInterval->GetMergeMergeSplit()->SetPosition(NULL));
	}
	if (prevPrevInterval != NULL)
	{
		mergeMergeSplitList->RemoveAt(interval->GetMergeMergeSplit()->GetPosition());
		debug(interval->GetMergeMergeSplit()->SetPosition(NULL));
	}
	if (nextInterval != NULL)
	{
		mergeMergeSplitList->RemoveAt(nextInterval->GetMergeMergeSplit()->GetPosition());
		debug(nextInterval->GetMergeMergeSplit()->SetPosition(NULL));
	}
	if (nextNextInterval != NULL)
	{
		mergeMergeSplitList->RemoveAt(nextNextInterval->GetMergeMergeSplit()->GetPosition());
		debug(nextNextInterval->GetMergeMergeSplit()->SetPosition(NULL));
	}

	// On retire les 2 intervalles impliques dans le MergeSplit de la liste triee selon les effectifs
	RemoveIntervalFromWorkingFrequencyList(interval);
	RemoveIntervalFromWorkingFrequencyList(prevInterval);

	/////////////////////////////////////////////////////////////////////////////////////
	// Mise a jour des intervalles en fonction de l'optimisation effectuee

	// Modification en cas de MergeSplit
	// On change effectivement les bornes de l'intervalle en recopiant les
	// informations de MergeSplit
	prevInterval->SetIndex(interval->GetMergeSplit()->GetFirstSubLineIndex());
	prevInterval->SetCost(interval->GetMergeSplit()->GetFirstSubLineCost());
	interval->SetCost(interval->GetMergeSplit()->GetSecondSubLineCost());

	// Reconstruction des nouveaux vecteurs d'effectifs
	MergeSplitFrequencyVectors(prevInterval->GetFrequencyVector(), interval->GetFrequencyVector(),
				   interval->GetMergeSplit()->GetFirstSubLineFrequencyVector());

	// Verification des nombres de modalites
	assert(interval->GetModalityNumber() ==
	       ComputeModalityNumber(kwftSource, prevInterval->GetIndex() + 1, interval->GetIndex()));
	if (prevPrevInterval != NULL)
		assert(prevInterval->GetModalityNumber() ==
		       ComputeModalityNumber(kwftSource, prevPrevInterval->GetIndex() + 1, prevInterval->GetIndex()));
	else
		assert(prevInterval->GetModalityNumber() ==
		       ComputeModalityNumber(kwftSource, 0, prevInterval->GetIndex()));

	// Insertion des intervalles dans la liste triee des effectifs
	AddIntervalToWorkingFrequencyList(prevInterval);
	AddIntervalToWorkingFrequencyList(interval);

	////////////////////////////////////////////////////////////////////////////////////
	// Calcul des nouvelles optimisations potentielles induites par les changements, et
	// insertion dans les liste

	// Calcul des nouveaux Splits bases sur le nouvel intervalle,
	// et reinsertion dans la liste triee
	ComputeIntervalSplit(kwftSource, prevInterval);
	prevInterval->GetSplit()->SetPosition(splitList->Add(prevInterval));
	ComputeIntervalSplit(kwftSource, interval);
	interval->GetSplit()->SetPosition(splitList->Add(interval));

	// Calcul des nouveaux MergeSplits bases sur le nouvel intervalle,
	// et reinsertion dans la liste triee
	interval->GetMergeSplit()->SetDeltaCost(0);
	interval->GetMergeSplit()->SetPosition(mergeSplitList->Add(interval));
	if (prevPrevInterval != NULL)
	{
		ComputeIntervalMergeSplit(kwftSource, prevInterval);
		prevInterval->GetMergeSplit()->SetPosition(mergeSplitList->Add(prevInterval));
	}
	if (nextInterval != NULL)
	{
		ComputeIntervalMergeSplit(kwftSource, nextInterval);
		nextInterval->GetMergeSplit()->SetPosition(mergeSplitList->Add(nextInterval));
	}

	// Calcul des nouveaux MergeMergeSplits bases sur le nouvel intervalle,
	// et reinsertion dans la liste triee
	if (prevPrevInterval != NULL and prevPrevInterval->GetPrev() != NULL)
	{
		ComputeIntervalMergeMergeSplit(kwftSource, prevInterval);
		prevInterval->GetMergeMergeSplit()->SetPosition(mergeMergeSplitList->Add(prevInterval));
	}
	if (prevPrevInterval != NULL)
	{
		ComputeIntervalMergeMergeSplit(kwftSource, interval);
		interval->GetMergeMergeSplit()->SetPosition(mergeMergeSplitList->Add(interval));
	}
	if (nextInterval != NULL)
	{
		ComputeIntervalMergeMergeSplit(kwftSource, nextInterval);
		nextInterval->GetMergeMergeSplit()->SetPosition(mergeMergeSplitList->Add(nextInterval));
	}
	if (nextNextInterval != NULL)
	{
		ComputeIntervalMergeMergeSplit(kwftSource, nextNextInterval);
		nextNextInterval->GetMergeMergeSplit()->SetPosition(mergeMergeSplitList->Add(nextNextInterval));
	}
	ensure(CheckPostOptimizationSortedLists(splitList, mergeSplitList, mergeMergeSplitList, headInterval));
}

void KWDiscretizerMODL::UpdatePostOptimizationSortedListsWithMergeMergeSplit(const KWFrequencyTable* kwftSource,
									     SortedList* splitList,
									     SortedList* mergeSplitList,
									     SortedList* mergeMergeSplitList,
									     KWMODLLineDeepOptimization*& headInterval,
									     KWMODLLineDeepOptimization* interval) const
{
	KWMODLLineDeepOptimization* prevInterval;
	KWMODLLineDeepOptimization* prevPrevInterval;
	KWMODLLineDeepOptimization* nextInterval;
	KWMODLLineDeepOptimization* nextNextInterval;

	require(kwftSource != NULL);
	require(splitList != NULL);
	require(mergeSplitList != NULL);
	require(mergeMergeSplitList != NULL);
	require(headInterval != NULL);
	require(headInterval->GetNext() != NULL);
	require(headInterval->GetNext()->GetNext() != NULL);
	require(interval != NULL);
	require(interval->GetPrev() != NULL);
	require(interval->GetPrev()->GetPrev() != NULL);
	require(CheckFrequencyVector(interval->GetFrequencyVector()));
	require(CheckFrequencyVector(interval->GetPrev()->GetFrequencyVector()));
	require(CheckFrequencyVector(interval->GetPrev()->GetPrev()->GetFrequencyVector()));
	require(CheckPostOptimizationSortedLists(splitList, mergeSplitList, mergeMergeSplitList, headInterval));

	// Recherche des intervalles precedents et suivants
	prevInterval = cast(KWMODLLineDeepOptimization*, interval->GetPrev());
	prevPrevInterval = cast(KWMODLLineDeepOptimization*, prevInterval->GetPrev());
	nextInterval = cast(KWMODLLineDeepOptimization*, interval->GetNext());
	nextNextInterval = NULL;
	if (nextInterval != NULL)
		nextNextInterval = cast(KWMODLLineDeepOptimization*, nextInterval->GetNext());

	////////////////////////////////////////////////////////////////////////////////
	// On supprime les intervalles necessaires des listes

	// On retire les trois intervalles (potentiels) de la liste triee des Split
	splitList->RemoveAt(prevPrevInterval->GetSplit()->GetPosition());
	debug(prevPrevInterval->GetSplit()->SetPosition(NULL));
	splitList->RemoveAt(prevInterval->GetSplit()->GetPosition());
	debug(prevInterval->GetSplit()->SetPosition(NULL));
	splitList->RemoveAt(interval->GetSplit()->GetPosition());
	debug(interval->GetSplit()->SetPosition(NULL));

	// On retire les quatre intervalles (potentiels) de la liste triee des MergeSplit
	if (prevPrevInterval->GetPrev() != NULL)
	{
		mergeSplitList->RemoveAt(prevPrevInterval->GetMergeSplit()->GetPosition());
		debug(prevPrevInterval->GetMergeSplit()->SetPosition(NULL));
	}
	mergeSplitList->RemoveAt(prevInterval->GetMergeSplit()->GetPosition());
	debug(prevInterval->GetMergeSplit()->SetPosition(NULL));
	mergeSplitList->RemoveAt(interval->GetMergeSplit()->GetPosition());
	debug(interval->GetMergeSplit()->SetPosition(NULL));
	if (nextInterval != NULL)
	{
		mergeSplitList->RemoveAt(nextInterval->GetMergeSplit()->GetPosition());
		debug(nextInterval->GetMergeSplit()->SetPosition(NULL));
	}

	// On retire les cinq intervalles (potentiels) de la liste triee des MergeMergeSplit
	mergeMergeSplitList->RemoveAt(interval->GetMergeMergeSplit()->GetPosition());
	debug(interval->GetMergeMergeSplit()->SetPosition(NULL));
	if (prevPrevInterval->GetPrev() != NULL)
	{
		mergeMergeSplitList->RemoveAt(prevInterval->GetMergeMergeSplit()->GetPosition());
		debug(prevInterval->GetMergeMergeSplit()->SetPosition(NULL));
		if (prevPrevInterval->GetPrev()->GetPrev() != NULL)
		{
			mergeMergeSplitList->RemoveAt(prevPrevInterval->GetMergeMergeSplit()->GetPosition());
			debug(prevPrevInterval->GetMergeMergeSplit()->SetPosition(NULL));
		}
	}
	if (nextInterval != NULL)
	{
		mergeMergeSplitList->RemoveAt(nextInterval->GetMergeMergeSplit()->GetPosition());
		debug(nextInterval->GetMergeMergeSplit()->SetPosition(NULL));
	}
	if (nextNextInterval != NULL)
	{
		mergeMergeSplitList->RemoveAt(nextNextInterval->GetMergeMergeSplit()->GetPosition());
		debug(nextNextInterval->GetMergeMergeSplit()->SetPosition(NULL));
	}

	// On retire les 3 intervalles intervenant dans le MergeMergeSplit de la liste triee selon les effectifs
	RemoveIntervalFromWorkingFrequencyList(interval);
	RemoveIntervalFromWorkingFrequencyList(prevInterval);
	RemoveIntervalFromWorkingFrequencyList(prevPrevInterval);

	/////////////////////////////////////////////////////////////////////////////////////
	// Mise a jour des intervalles en fonction de l'optimisation effectuee

	// Modification en cas de MergeMergeSplit
	// On change effectivement les bornes de l'intervalle en recopiant les
	// informations de MergeMergeSplit
	prevPrevInterval->SetIndex(interval->GetMergeMergeSplit()->GetFirstSubLineIndex());
	prevPrevInterval->SetCost(interval->GetMergeMergeSplit()->GetFirstSubLineCost());
	interval->SetCost(interval->GetMergeMergeSplit()->GetSecondSubLineCost());

	// Reconstruction des nouveaux vecteurs d'effectifs
	MergeMergeSplitFrequencyVectors(prevPrevInterval->GetFrequencyVector(), prevInterval->GetFrequencyVector(),
					interval->GetFrequencyVector(),
					interval->GetMergeMergeSplit()->GetFirstSubLineFrequencyVector());

	// Verification des nombres de modalites
	assert(interval->GetModalityNumber() ==
	       ComputeModalityNumber(kwftSource, prevPrevInterval->GetIndex() + 1, interval->GetIndex()));
	if (prevPrevInterval->GetPrev() != NULL)
		assert(prevPrevInterval->GetModalityNumber() ==
		       ComputeModalityNumber(kwftSource, prevPrevInterval->GetPrev()->GetIndex() + 1,
					     prevPrevInterval->GetIndex()));
	else
		assert(prevPrevInterval->GetModalityNumber() ==
		       ComputeModalityNumber(kwftSource, 0, prevPrevInterval->GetIndex()));

	// Chainage des deux intervalles extremites, et supression de l'intervalle du milieu
	prevPrevInterval->SetNext(interval);
	interval->SetPrev(prevPrevInterval);
	delete prevInterval;

	// Insertion des intervalles dans la liste triee des effectifs
	AddIntervalToWorkingFrequencyList(prevPrevInterval);
	AddIntervalToWorkingFrequencyList(interval);

	////////////////////////////////////////////////////////////////////////////////////
	// Calcul des nouvelles optimisations potentielles induites par les changements, et
	// insertion dans les liste

	// Calcul des nouveaux Splits bases sur le nouvel intervalle,
	// et reinsertion dans la liste triee
	ComputeIntervalSplit(kwftSource, prevPrevInterval);
	prevPrevInterval->GetSplit()->SetPosition(splitList->Add(prevPrevInterval));
	ComputeIntervalSplit(kwftSource, interval);
	interval->GetSplit()->SetPosition(splitList->Add(interval));

	// Calcul des nouveaux MergeSplits bases sur le nouvel intervalle,
	// et reinsertion dans la liste triee
	if (prevPrevInterval->GetPrev() != NULL)
	{
		ComputeIntervalMergeSplit(kwftSource, prevPrevInterval);
		prevPrevInterval->GetMergeSplit()->SetPosition(mergeSplitList->Add(prevPrevInterval));
	}
	ComputeIntervalMergeSplit(kwftSource, interval);
	interval->GetMergeSplit()->SetPosition(mergeSplitList->Add(interval));
	if (nextInterval != NULL)
	{
		ComputeIntervalMergeSplit(kwftSource, nextInterval);
		nextInterval->GetMergeSplit()->SetPosition(mergeSplitList->Add(nextInterval));
	}

	// Calcul des nouveaux MergeMergeSplits bases sur le nouvel intervalle,
	// et reinsertion dans la liste triee
	if (prevPrevInterval->GetPrev() != NULL)
	{
		ComputeIntervalMergeMergeSplit(kwftSource, interval);
		interval->GetMergeMergeSplit()->SetPosition(mergeMergeSplitList->Add(interval));
		if (prevPrevInterval->GetPrev()->GetPrev() != NULL)
		{
			ComputeIntervalMergeMergeSplit(kwftSource, prevPrevInterval);
			prevPrevInterval->GetMergeMergeSplit()->SetPosition(mergeMergeSplitList->Add(prevPrevInterval));
		}
	}
	if (nextInterval != NULL)
	{
		ComputeIntervalMergeMergeSplit(kwftSource, nextInterval);
		nextInterval->GetMergeMergeSplit()->SetPosition(mergeMergeSplitList->Add(nextInterval));
	}
	if (nextNextInterval != NULL)
	{
		ComputeIntervalMergeMergeSplit(kwftSource, nextNextInterval);
		nextNextInterval->GetMergeMergeSplit()->SetPosition(mergeMergeSplitList->Add(nextNextInterval));
	}
	ensure(CheckPostOptimizationSortedLists(splitList, mergeSplitList, mergeMergeSplitList, headInterval));
}

void KWDiscretizerMODL::UpdateBoundaryPostOptimizationSortedListWithMergeSplit(
    const KWFrequencyTable* kwftSource, SortedList* mergeSplitList, KWMODLLineDeepOptimization*& headInterval,
    KWMODLLineDeepOptimization* interval) const
{
	KWMODLLineDeepOptimization* prevInterval;
	KWMODLLineDeepOptimization* prevPrevInterval;
	KWMODLLineDeepOptimization* nextInterval;
	KWMODLLineDeepOptimization* nextNextInterval;

	require(kwftSource != NULL);
	require(mergeSplitList != NULL);
	require(headInterval != NULL);
	require(headInterval->GetNext() != NULL);
	require(interval != NULL);
	require(interval->GetPrev() != NULL);
	require(CheckFrequencyVector(interval->GetFrequencyVector()));
	require(CheckFrequencyVector(interval->GetPrev()->GetFrequencyVector()));

	// Recherche des intervalles precedents et suivants
	prevInterval = cast(KWMODLLineDeepOptimization*, interval->GetPrev());
	prevPrevInterval = cast(KWMODLLineDeepOptimization*, prevInterval->GetPrev());
	nextInterval = cast(KWMODLLineDeepOptimization*, interval->GetNext());
	nextNextInterval = NULL;
	if (nextInterval != NULL)
		nextNextInterval = cast(KWMODLLineDeepOptimization*, nextInterval->GetNext());

	////////////////////////////////////////////////////////////////////////////////
	// On supprime les intervalles necessaires des listes

	// On retire les trois intervalles (potentiels) de la liste triee des MergeSplit
	mergeSplitList->RemoveAt(interval->GetMergeSplit()->GetPosition());
	debug(interval->GetMergeSplit()->SetPosition(NULL));
	if (prevPrevInterval != NULL)
	{
		mergeSplitList->RemoveAt(prevInterval->GetMergeSplit()->GetPosition());
		debug(prevInterval->GetMergeSplit()->SetPosition(NULL));
	}
	if (nextInterval != NULL)
	{
		mergeSplitList->RemoveAt(nextInterval->GetMergeSplit()->GetPosition());
		debug(nextInterval->GetMergeSplit()->SetPosition(NULL));
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Mise a jour des intervalles en fonction de l'optimisation effectuee

	// Modification en cas de MergeSplit
	// On change effectivement les bornes de l'intervalle en recopiant les
	// informations de MergeSplit
	prevInterval->SetIndex(interval->GetMergeSplit()->GetFirstSubLineIndex());
	prevInterval->SetCost(interval->GetMergeSplit()->GetFirstSubLineCost());
	interval->SetCost(interval->GetMergeSplit()->GetSecondSubLineCost());

	// Reconstruction des nouveaux vecteurs d'effectifs
	MergeSplitFrequencyVectors(prevInterval->GetFrequencyVector(), interval->GetFrequencyVector(),
				   interval->GetMergeSplit()->GetFirstSubLineFrequencyVector());

	////////////////////////////////////////////////////////////////////////////////////
	// Calcul des nouvelles optimisations potentielles induites par les changements, et
	// insertion dans les liste

	// Calcul des nouveaux MergeSplits bases sur le nouvel intervalle,
	// et reinsertion dans la liste triee
	interval->GetMergeSplit()->SetDeltaCost(0);
	interval->GetMergeSplit()->SetPosition(mergeSplitList->Add(interval));
	if (prevPrevInterval != NULL)
	{
		ComputeIntervalMergeSplit(kwftSource, prevInterval);
		prevInterval->GetMergeSplit()->SetPosition(mergeSplitList->Add(prevInterval));
	}
	if (nextInterval != NULL)
	{
		ComputeIntervalMergeSplit(kwftSource, nextInterval);
		nextInterval->GetMergeSplit()->SetPosition(mergeSplitList->Add(nextInterval));
	}
}

boolean KWDiscretizerMODL::CheckPostOptimizationSortedLists(SortedList* splitList, SortedList* mergeSplitList,
							    SortedList* mergeMergeSplitList,
							    KWMODLLineDeepOptimization* headInterval) const
{
	boolean bOk = true;
	ALString sTmp;

	require(splitList != NULL);
	require(mergeSplitList != NULL);
	require(mergeMergeSplitList != NULL);
	require(headInterval != NULL);

	// Cas ou il n'y a qu'un intervalle
	if (headInterval->GetNext() == NULL)
	{
		// Il faut un Split
		if (splitList->GetCount() != 1)
		{
			bOk = false;
			AddError(sTmp + "There are " + IntToString(splitList->GetCount()) + " Split for one interval");
		}

		// Il faut zero MergeSplit
		if (mergeSplitList->GetCount() != 0)
		{
			bOk = false;
			AddError(sTmp + "There are " + IntToString(mergeSplitList->GetCount()) +
				 " MergeSplit for one interval");
		}

		// Il faut zero MergeMergeSplit
		if (mergeMergeSplitList->GetCount() != 0)
		{
			bOk = false;
			AddError(sTmp + "There are " + IntToString(mergeMergeSplitList->GetCount()) +
				 " MergeMergeSplit for one interval");
		}
	}
	// Cas ou il y a au moins deux intervalles
	else
	{
		// Il faut au moins un MergeSplit
		if (mergeSplitList->GetCount() == 0)
		{
			bOk = false;
			AddError("There is not MergeSplit for at least two intervals");
		}

		// Coherence entre les cardinaux des listes
		if (splitList->GetCount() != mergeSplitList->GetCount() + 1)
		{
			bOk = false;
			AddError(sTmp + "There are " + IntToString(splitList->GetCount()) + " Split and " +
				 IntToString(mergeSplitList->GetCount()) + " MergeSplit");
		}
		if (splitList->GetCount() != mergeMergeSplitList->GetCount() + 2)
		{
			bOk = false;
			AddError(sTmp + "There are " + IntToString(splitList->GetCount()) + " Split and " +
				 IntToString(mergeMergeSplitList->GetCount()) + " MergeMergeSplit");
		}
	}

	return bOk;
}

void KWDiscretizerMODL::AddIntervalToWorkingFrequencyList(KWMODLLine* interval) const {}
void KWDiscretizerMODL::RemoveIntervalFromWorkingFrequencyList(KWMODLLine* interval) const {}
int KWDiscretizerMODL::ComputeModalityNumber(const KWFrequencyTable* kwftSource, int nFirstIndex, int LastIndex) const
{
	return 0;
}
