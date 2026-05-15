// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataGridOptimizer.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridOptimizer

KWDataGridOptimizer::KWDataGridOptimizer()
{
	classStats = NULL;
	dataGridCosts = NULL;
	attributeSubsetStatsOptimizationHandler = NULL;
	dEpsilon = 1e-6;
	optimizedInitialDataGrid = NULL;
	optimizedNullDataGrid = NULL;
	ResetProgressionIndicators();
}

KWDataGridOptimizer::~KWDataGridOptimizer()
{
	assert(optimizedInitialDataGrid == NULL);
	assert(optimizedNullDataGrid == NULL);
}

void KWDataGridOptimizer::Reset()
{
	classStats = NULL;
	dataGridCosts = NULL;
	ResetProgressionIndicators();
}

void KWDataGridOptimizer::SetDataGridCosts(const KWDataGridCosts* kwdgcCosts)
{
	dataGridCosts = kwdgcCosts;
}

const KWDataGridCosts* KWDataGridOptimizer::GetDataGridCosts() const
{
	return dataGridCosts;
}

KWDataGridOptimizerParameters* KWDataGridOptimizer::GetParameters()
{
	return &optimizationParameters;
}

void KWDataGridOptimizer::SetClassStats(KWClassStats* stats)
{
	classStats = stats;
}

KWClassStats* KWDataGridOptimizer::GetClassStats() const
{
	return classStats;
}

double KWDataGridOptimizer::OptimizeDataGrid(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid) const

{
	KWDataGridManager dataGridManager;
	double dBestCost;

	require(GetDataGridCosts() != NULL);
	require(GetDataGridCosts()->IsInitialized());
	require(initialDataGrid != NULL);
	require(optimizedDataGrid != NULL);

	// Debut de suivi des taches
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Data Grid optimization");

	//Initialisations
	ResetProgressionIndicators();
	timerOptimization.Start();
	optimizedInitialDataGrid = initialDataGrid;

	// Initialisation du modele null
	optimizedNullDataGrid = new KWDataGrid;
	dataGridManager.ExportNullDataGrid(initialDataGrid, optimizedNullDataGrid);
	assert(GetOptimizedNullDataGridCost() == GetDataGridCosts()->ComputeDataGridTotalCost(optimizedNullDataGrid));

	// Controle de la graine aleatoire pour avoir des resultats reproductibles
	SetRandomSeed(1);

	// Utilisation du modele nul pour la solution initiale
	SaveDataGrid(GetOptimizedNullDataGrid(), optimizedDataGrid);
	dBestCost = GetOptimizedNullDataGridCost();

	// Optimisation a partir d'une grille initiale complete si algorithme glouton
	if (not TaskProgression::IsInterruptionRequested())
		dBestCost = InternalOptimizeDataGrid(initialDataGrid, optimizedDataGrid);

	// Tri des parties par attribut, pour preparer les affichages de resultats
	// ainsi que les resultats de preparation des donnees
	optimizedDataGrid->SortAttributeParts();

	// Fin de suivi des taches
	TaskProgression::EndTask();

	// Nettoyage
	ResetProgressionIndicators();
	optimizedInitialDataGrid = NULL;
	delete optimizedNullDataGrid;
	optimizedNullDataGrid = NULL;

	ensure(optimizedDataGrid->AreAttributePartsSorted() or TaskProgression::IsInterruptionRequested());
	ensure(fabs(dBestCost - GetDataGridCosts()->ComputeDataGridTotalCost(optimizedDataGrid)) < dEpsilon);
	return dBestCost;
}

double KWDataGridOptimizer::SimplifyDataGrid(KWDataGrid* optimizedDataGrid) const
{
	KWDataGridManager dataGridManager;
	KWDataGridMerger dataGridMerger;
	double dSimplifiedGridCost;

	require(optimizedDataGrid != NULL);

	// Cas ou la contrainte est dekja respectee: on garde la grille telle quelle
	if (optimizationParameters.GetMaxPartNumber() == 0 or
	    optimizedDataGrid->ComputeMaxPartNumber() <= optimizationParameters.GetMaxPartNumber())
	{
		// Il faut recalculer le cout de la grille initiale
		dSimplifiedGridCost = GetDataGridCosts()->ComputeDataGridTotalCost(optimizedDataGrid);
	}
	// Cas ou il faut simplifier la grille
	else
	{
		// Export de la grille initiale vers un merger de grille
		dataGridManager.ExportDataGrid(optimizedDataGrid, &dataGridMerger);

		// Parametrage des couts et des contraintes
		dataGridMerger.SetDataGridCosts(GetDataGridCosts());
		dataGridMerger.SetMaxPartNumber(optimizationParameters.GetMaxPartNumber());
		assert(dataGridMerger.Check());

		// Post-traitement de simplification
		dSimplifiedGridCost = dataGridMerger.Merge();

		// Memorisation de la solution initiale
		dataGridManager.CopyDataGrid(&dataGridMerger, optimizedDataGrid);
	}
	ensure(fabs(dSimplifiedGridCost - GetDataGridCosts()->ComputeDataGridTotalCost(optimizedDataGrid)) < dEpsilon);
	return dSimplifiedGridCost;
}

void KWDataGridOptimizer::HandleOptimizationStep(const KWDataGrid* optimizedDataGrid,
						 const KWDataGrid* initialGranularizedDataGrid,
						 boolean bIsLastSaving) const
{
	// Integration de la granularite
	if (attributeSubsetStatsOptimizationHandler != NULL)
	{
		attributeSubsetStatsOptimizationHandler->HandleOptimizationStep(
		    optimizedDataGrid, initialGranularizedDataGrid, bIsLastSaving);
	}
}

void KWDataGridOptimizer::SetOptimizationHandler(const KWAttributeSubsetStats* attributeSubsetStats)
{
	attributeSubsetStatsOptimizationHandler = attributeSubsetStats;
}

const KWAttributeSubsetStats* KWDataGridOptimizer::GetOptimizationHandler()
{
	return attributeSubsetStatsOptimizationHandler;
}

const KWDataGrid* KWDataGridOptimizer::GetOptimizedInitialDataGrid() const
{
	require(optimizedInitialDataGrid != NULL);
	return optimizedInitialDataGrid;
}

const KWDataGrid* KWDataGridOptimizer::GetOptimizedNullDataGrid() const
{
	require(optimizedNullDataGrid != NULL);
	ensure(optimizedNullDataGrid->IsVarPartDataGrid() == GetOptimizedInitialDataGrid()->IsVarPartDataGrid());
	return optimizedNullDataGrid;
}

double KWDataGridOptimizer::GetOptimizedNullDataGridCost() const
{
	require(optimizedNullDataGrid != NULL);
	require(GetDataGridCosts()->IsInitialized());
	return GetDataGridCosts()->GetTotalDefaultCost();
}

double KWDataGridOptimizer::IterativeVNSOptimizeDataGrid(const KWDataGrid* initialDataGrid,
							 KWDataGrid* optimizedDataGrid) const
{
	boolean bDisplayResults = false;
	int nMaxLevel;
	int nLevel;
	double dCost;
	double dBestCost;
	KWDataGridManager dataGridManager;
	KWDataGrid currentDataGrid;
	int nNeighbourhoodLevelNumber;

	require(initialDataGrid != NULL);
	require(optimizedDataGrid != NULL);

	// On prend au minimum un niveau max de 1
	nMaxLevel = optimizationParameters.GetOptimizationLevel();
	if (nMaxLevel <= 0)
		nMaxLevel = 1;

	// Parametrage d'un niveau d'optimisation anytime si une limite de temps est indiquee
	// On le fait uniquement pour la derniere granularite, pour que le mode anytime ne
	// ne reste pas bloque des la premiere granularite intermediaire
	if (optimizationParameters.GetOptimizationTime() > 0)
		nMaxLevel = 20;

	// Initialisations
	dBestCost = dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);

	// Appel de VNS en augmentant le nombre de voisinnages d'un facteur 2 chaque fois
	for (nLevel = 0; nLevel < nMaxLevel; nLevel++)
	{
		// Calcul du nombre de voisinnage a considerer
		nNeighbourhoodLevelNumber = int(pow(2.0, nLevel));
		if (bDisplayResults)
			cout << "IterativeVNSOptimizeDataGrid: Level\t" << nLevel << endl;

		// Recopie de la meilleure solution dans une solution de travail courante
		dataGridManager.CopyDataGrid(optimizedDataGrid, &currentDataGrid);

		// Parametrage du profiling
		KWDataGridOptimizer::GetProfiler()->BeginMethod("VNS optimize");
		KWDataGridOptimizer::GetProfiler()->WriteKeyBoolean("Is VarPart", currentDataGrid.IsVarPartDataGrid());
		KWDataGridOptimizer::GetProfiler()->WriteKeyInt("Level", nLevel);
		KWDataGridOptimizer::GetProfiler()->WriteKeyInt("Neighbourhood level number",
								nNeighbourhoodLevelNumber);

		// Optimisation a partir de la nouvelle solution
		dCost = VNSOptimizeDataGrid(initialDataGrid, nNeighbourhoodLevelNumber, &currentDataGrid);
		if (dCost < dBestCost - dEpsilon)
		{
			dBestCost = dCost;
			SaveDataGrid(&currentDataGrid, optimizedDataGrid);
		}
		KWDataGridOptimizer::GetProfiler()->EndMethod("VNS optimize");

		// Test de fin de tache
		if (TaskProgression::IsInterruptionRequested())
			break;

		// Test si depassement de temps
		if (IsOptimizationTimeElapsed())
			break;
	}
	assert(dBestCost < DBL_MAX);

	ensure(fabs(dBestCost - dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid)) < dEpsilon);
	return dBestCost;
}

double KWDataGridOptimizer::VNSOptimizeDataGrid(const KWDataGrid* initialDataGrid, int nNeighbourhoodLevelNumber,
						KWDataGrid* optimizedDataGrid) const
{
	double dBestCost;
	double dCost;
	KWDataGridManager dataGridManager;
	KWDataGridMerger neighbourDataGrid;
	int nIndex;
	double dMinNeighbourhoodSize;
	double dDecreaseFactor;
	double dNeighbourhoodSize;
	ALString sTmp;

	require(initialDataGrid != NULL);
	require(optimizedDataGrid != NULL);
	require(nNeighbourhoodLevelNumber >= 0);

	// Initialisations
	neighbourDataGrid.SetDataGridCosts(dataGridCosts);
	dBestCost = dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);

	// Calcul d'une taille minimale de voisinnage, relative a la taille de la base
	// On souhaite impliquer au minimum 3 nouvelles parties par attribut
	dMinNeighbourhoodSize = 3.0 / (3 + initialDataGrid->GetGridFrequency());

	// Calcul du taux de decroissance des tailles de voisinnage de facon a obtenir
	// une taille minimale de voisinnage suffisante
	dDecreaseFactor = 1.0 / pow(dMinNeighbourhoodSize, 1.0 / (nNeighbourhoodLevelNumber + 1));

	// On optimise tant qu'on ne depasse pas la taille max de voisinnage
	nVNSNeighbourhoodLevelNumber = nNeighbourhoodLevelNumber;
	nIndex = 0;
	while (nIndex <= nNeighbourhoodLevelNumber)
	{
		nVNSNeighbourhoodLevelIndex = nIndex;
		dNeighbourhoodSize = pow(1.0 / dDecreaseFactor, nIndex);
		dVNSNeighbourhoodSize = dNeighbourhoodSize;

		// Optimisation d'une solution dans un voisinnage de la solution courante
		dCost = OptimizeNeighbourSolution(initialDataGrid, optimizedDataGrid, dNeighbourhoodSize,
						  &neighbourDataGrid, true);

		// Si amelioration: on la memorise
		if (dCost < dBestCost - dEpsilon)
		{
			dBestCost = dCost;
			SaveDataGrid(&neighbourDataGrid, optimizedDataGrid);

			// Gestion de la meilleure solution
			HandleOptimizationStep(optimizedDataGrid, initialDataGrid, false);
		}
		// Sinon: on passe a un niveau de voisinnage plus fin
		else
			nIndex++;

		// Test de fin de tache
		if (TaskProgression::IsInterruptionRequested())
			break;

		// Test si depassement de temps
		if (IsOptimizationTimeElapsed())
			break;
	}

	ensure(fabs(dBestCost - dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid)) < dEpsilon);
	ensure(optimizedDataGrid->Check());
	return dBestCost;
}

double KWDataGridOptimizer::OptimizeNeighbourSolution(const KWDataGrid* initialDataGrid,
						      const KWDataGrid* currentOptimizedDataGrid, double dNoiseRate,
						      KWDataGridMerger* neighbourOptimizedDataGrid,
						      boolean bDeepPostOptimization) const
{
	double dCost;
	ALString sTmp;

	// Generation d'une solution dans un voisinnage de la meilleure solution
	GenerateNeighbourSolution(initialDataGrid, currentOptimizedDataGrid, dNoiseRate, neighbourOptimizedDataGrid);

	// Optimisation de cette solution
	dCost = OptimizeSolution(initialDataGrid, neighbourOptimizedDataGrid, bDeepPostOptimization);
	return dCost;
}

void KWDataGridOptimizer::GenerateNeighbourSolution(const KWDataGrid* initialDataGrid,
						    const KWDataGrid* optimizedDataGrid, double dNoiseRate,
						    KWDataGridMerger* neighbourDataGridMerger) const
{
	const boolean bTrace = false;
	const boolean bTraceDetails = false;
	KWDataGridManager dataGridManager;
	KWDataGrid mandatoryDataGrid;
	int nMandatoryAttributeNumber;
	int nMaxAttributeNumber;
	int nMaxPartNumber;
	int nMaxContinuousPartNumber;
	int nMaxSymbolPartNumber;
	int nRequestedContinuousPartNumber;
	int nRequestedSymbolPartNumber;
	int nAttributeNumber;
	int nGridSize;
	ALString sTmp;

	require(initialDataGrid != NULL);
	require(optimizedDataGrid != NULL);
	require(neighbourDataGridMerger != NULL);
	require(0 <= dNoiseRate and dNoiseRate <= 1);
	require(not initialDataGrid->IsVarPartDataGrid() or
		initialDataGrid->GetInnerAttributes() == optimizedDataGrid->GetInnerAttributes());

	// Debut de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel(sTmp + "New initial solution (" + DoubleToString(dNoiseRate) + ")");
	KWDataGridOptimizer::GetProfiler()->BeginMethod("Generate neighbour solution");
	KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Neighbourhood size", dNoiseRate);
	if (bTrace)
	{
		TraceOptimizationDetails("GenerateNeighbourSolution", optimizedDataGrid, bTraceDetails);
		TraceOptimizationDetails("- initial solution", optimizedDataGrid, bTraceDetails);
	}

	// Initialisation de la taille de la grille prise en compte
	// La taille de la grille est initialise avec le nombre de cellules non vides de la grille initiale
	// pour une granularite donnee.
	// - a noter: on a toujours GetCellNumber <= GetGridFrequency.
	// - pour la granularite max, cela permet d'avoir un indicateur de sparsite de la grille plus fiable
	//   que le nombre d'individus (GetGridFrequency), qui reflete la proportion de cellules non vides.
	// - pour les granularites intermediaires, ce nombre est plus petit, ce qui entrainera la generation
	//   de grilles voisines aleatoires plus petites, et donc plus rapides a optimiser:
	//   - dans le cas non supervise, c'est ce que l'on souhaite pour obtenir des premieres solutions rapidement
	//   - dans le cas supervise, cette taille reduite limite l'espace de recherche avec un impact
	//     pontiellement negatif sur la qualite des solutions: on considere ce probleme comme negligeable
	nGridSize = initialDataGrid->GetCellNumber();

	// Calcul du nombre d'attributs a exporter
	nMaxAttributeNumber = 1 + (int)(log(nGridSize * 1.0) / log(2.0));
	nAttributeNumber = (int)(dNoiseRate * nMaxAttributeNumber);
	if (nAttributeNumber < 2)
		nAttributeNumber = 2;
	if (nAttributeNumber > initialDataGrid->GetAttributeNumber())
		nAttributeNumber = initialDataGrid->GetAttributeNumber();

	// Calcul des nombres de parties a exporter
	nMaxContinuousPartNumber = (int)(nGridSize / log(nGridSize + 1.0));
	nMaxSymbolPartNumber = (int)sqrt(nGridSize * 1.0);
	nMaxPartNumber = (int)pow(nGridSize * 1.0, 1.0 / nAttributeNumber);
	if (nMaxPartNumber > nGridSize)
		nMaxPartNumber = nGridSize;
	if (nMaxPartNumber < 2)
		nMaxPartNumber = 2;
	if (nMaxContinuousPartNumber > nMaxPartNumber)
		nMaxContinuousPartNumber = nMaxPartNumber;
	if (nMaxSymbolPartNumber > nMaxPartNumber)
		nMaxSymbolPartNumber = nMaxPartNumber;
	nRequestedContinuousPartNumber = 1 + (int)(dNoiseRate * nMaxContinuousPartNumber);
	nRequestedSymbolPartNumber = 1 + (int)(dNoiseRate * nMaxSymbolPartNumber);

	// Parametrage avance temporaire, pour etude sur les multinomiales hierarchiques (Marc Boulle)
	// Cf. classe d'etude KWHierarchicalMultinomialStudy
	if (optimizationParameters.GetInternalParameter() == "LargeNeighbourhoods")
	{
		nMaxContinuousPartNumber = (int)(nGridSize / 2);
		nMaxSymbolPartNumber = (int)sqrt(nGridSize * 1.0);
		nRequestedContinuousPartNumber = 1 + (int)(dNoiseRate * nMaxContinuousPartNumber);
		nRequestedSymbolPartNumber = 1 + (int)(dNoiseRate * nMaxSymbolPartNumber);
	}

	// Export d'un sous-ensemble d'attributs obligatoires (les attributs informatifs) en fonction du niveau de bruit
	// Les attributs obligatoires sont les attributs de la grille optimisee que l'on va conserver
	// Quand la grille initiale contient plus d'attributs que la grille optimisee (on ne peut pas toujours prendre
	// tous les attributs pour des questions de complexite algorithmique), on n'en conserve que certains et on
	// complete avec de nouveaux attributs absents de la grille optimisee Quand le NoiseRate est eleve (=1) alors
	// aucun des attributs n'est conserve avec ce statut d'obligatoire
	nMandatoryAttributeNumber = (int)ceil((1 - dNoiseRate) * optimizedDataGrid->GetAttributeNumber());
	dataGridManager.ExportRandomAttributes(optimizedDataGrid, &mandatoryDataGrid, nMandatoryAttributeNumber);

	// Exports d'attributs supplementaires
	// Pour un dNoiseRate de 1, tous les attributs sont ajoutes a concurrence de nMaxAttributeNumber
	neighbourDataGridMerger->DeleteAll();
	dataGridManager.AddRandomAttributes(initialDataGrid, neighbourDataGridMerger, &mandatoryDataGrid,
					    nAttributeNumber);

	// Export des parties
	dataGridManager.AddRandomParts(initialDataGrid, neighbourDataGridMerger, optimizedDataGrid,
				       nRequestedContinuousPartNumber, nRequestedSymbolPartNumber, 1.0);
	TaskProgression::DisplayProgression(25);

	// Export des cellules
	dataGridManager.ExportCells(initialDataGrid, neighbourDataGridMerger);

	// Fin de tache
	KWDataGridOptimizer::GetProfiler()->EndMethod("Generate neighbour solution");
	if (bTrace)
		TraceOptimizationDetails("- neighbourhood solution", neighbourDataGridMerger, bTraceDetails);
	TaskProgression::EndTask();
	ensure(neighbourDataGridMerger->Check());
}

double KWDataGridOptimizer::OptimizeSolution(const KWDataGrid* initialDataGrid, KWDataGridMerger* dataGridMerger,
					     boolean bDeepPostOptimization) const
{
	const boolean bTrace = false;
	const boolean bTraceDetails = false;
	KWDataGridPostOptimizer dataGridPostOptimizer;
	double dCost;
	ALString sTmp;

	require(GetDataGridCosts() != NULL);
	require(GetDataGridCosts()->IsInitialized());
	require(initialDataGrid != NULL);
	require(dataGridMerger != NULL);
	require(not initialDataGrid->IsVarPartDataGrid() or
		initialDataGrid->GetInnerAttributes() == dataGridMerger->GetInnerAttributes());

	// Suivi de la progression
	nVNSIteration++;
	DisplayProgression(initialDataGrid);

	// Initialisations
	dataGridMerger->SetDataGridCosts(dataGridCosts);
	dataGridPostOptimizer.SetDataGridCosts(dataGridCosts);

	// Cout initial si aucune optimisation, ou si une trace est demandee
	dCost = DBL_MAX;
	if ((not optimizationParameters.GetOptimize() and not optimizationParameters.GetPreOptimize() and
	     not optimizationParameters.GetPostOptimize()) or
	    bTrace or KWDataGridOptimizer::GetProfiler()->IsStarted())
		dCost = dataGridCosts->ComputeDataGridTotalCost(dataGridMerger);

	// Trace initiale
	KWDataGridOptimizer::GetProfiler()->BeginMethod("Optimize solution");
	KWDataGridOptimizer::GetProfiler()->WriteKeyString("Coclustering", dataGridMerger->GetObjectLabel());
	KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Cost", dCost);
	if (bTrace)
		TraceOptimizationDetails("OptimizeSolution", dataGridMerger, bTraceDetails);

	// Pre-optimisation de la grille
	if (optimizationParameters.GetPreOptimize() and not TaskProgression::IsInterruptionRequested() and
	    initialDataGrid->GetAttributeNumber() > 1)
	{
		KWDataGridOptimizer::GetProfiler()->BeginMethod("Pre-optimization");
		dCost = dataGridPostOptimizer.PostOptimizeDataGrid(initialDataGrid, dataGridMerger, false);
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("Coclustering", dataGridMerger->GetObjectLabel());
		KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Cost", dCost);
		KWDataGridOptimizer::GetProfiler()->EndMethod("Pre-optimization");
		if (bTrace)
			TraceOptimizationDetails("- pre-optimisation", dataGridMerger, bTraceDetails);
	}

	// Optimisation par fusion des groupes
	if (optimizationParameters.GetOptimize() and not TaskProgression::IsInterruptionRequested())
	{
		KWDataGridOptimizer::GetProfiler()->BeginMethod("Greedy merge optimization");
		dCost = dataGridMerger->Merge();
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("Coclustering", dataGridMerger->GetObjectLabel());
		KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Cost", dCost);
		KWDataGridOptimizer::GetProfiler()->EndMethod("Greedy merge optimization");
		if (bTrace)
			TraceOptimizationDetails("- optimisation", dataGridMerger, bTraceDetails);
	}

	// Post-optimisation de la grille selon le niveau de post-optimisation
	if (optimizationParameters.GetPostOptimize() and not TaskProgression::IsInterruptionRequested() and
	    initialDataGrid->GetAttributeNumber() > 1)
	{
		KWDataGridOptimizer::GetProfiler()->BeginMethod("Post-optimization");
		KWDataGridOptimizer::GetProfiler()->WriteKeyBoolean("Deep", bDeepPostOptimization);
		dCost =
		    dataGridPostOptimizer.PostOptimizeDataGrid(initialDataGrid, dataGridMerger, bDeepPostOptimization);
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("Coclustering", dataGridMerger->GetObjectLabel());
		KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Cost", dCost);
		KWDataGridOptimizer::GetProfiler()->EndMethod("Post-optimization");
		if (bTrace)
			TraceOptimizationDetails("- post-optimisation", dataGridMerger, bTraceDetails);
	}
	KWDataGridOptimizer::GetProfiler()->EndMethod("Optimize solution");

	// Recalcul du cout si la tache est interrompue, pour sortir avec un cout coherent
	if (TaskProgression::IsInterruptionRequested())
		dCost = dataGridCosts->ComputeDataGridTotalCost(dataGridMerger);

	// Retour du cout
	ensure(dataGridMerger->Check());
	ensure(fabs(dataGridCosts->ComputeDataGridTotalCost(dataGridMerger) - dCost) < dEpsilon);
	return dCost;
}

void KWDataGridOptimizer::SaveDataGrid(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const
{
	KWDataGridManager dataGridManager;

	// Memorisation de la grille
	dataGridManager.CopyDataGrid(sourceDataGrid, targetDataGrid);
}

boolean KWDataGridOptimizer::IsOptimizationNeeded(const KWDataGrid* dataGrid) const
{
	boolean bIsOptimizationNeeded;

	require(dataGrid != NULL);

	bIsOptimizationNeeded = true;
	int nSourceAttributNumber;
	int nSourceInformativeAttributNumber;

	// Cas supervise
	if (IsSupervisedDataGrid(dataGrid))
	{
		// Optimisation non necessaire si une seule partie cible
		if (dataGrid->GetTargetValueNumber() == 1 or
		    (dataGrid->GetTargetAttribute() != NULL and dataGrid->GetTargetAttribute()->GetPartNumber() <= 1))
			bIsOptimizationNeeded = false;
		// Cas avec partie cible avec plusieurs parties
		{
			// Calcul du nombre d'attributs sources
			nSourceAttributNumber = dataGrid->GetAttributeNumber();
			nSourceInformativeAttributNumber = dataGrid->GetInformativeAttributeNumber();

			// Correction si l'attribut cible fait partie directement de la grille
			if (dataGrid->GetTargetAttribute() != NULL)
			{
				nSourceAttributNumber--;
				nSourceInformativeAttributNumber--;
			}

			// Optimisation necessaire en univarie si attribut informatif (plusieurs parties)
			if (nSourceAttributNumber <= 1)
				bIsOptimizationNeeded = nSourceInformativeAttributNumber == 1;
			// Optimisation necessaire en multivarie si au moins deux attributs informatifs
			else
				bIsOptimizationNeeded = nSourceInformativeAttributNumber >= 2;
		}
	}
	// Cas non supervise
	else
	{
		assert(dataGrid->GetTargetValueNumber() == 0);

		// Optimisation non necessaire en non supervise s'il n'y a pas au moins deux attributs
		bIsOptimizationNeeded = (dataGrid->GetInformativeAttributeNumber() >= 2);
	}
	return bIsOptimizationNeeded;
}

boolean KWDataGridOptimizer::IsSupervisedDataGrid(const KWDataGrid* dataGrid) const
{
	boolean bIsSupervisedDataGrid;

	require(dataGrid != NULL);

	bIsSupervisedDataGrid = dataGrid->GetTargetAttribute() != NULL or dataGrid->GetTargetValueNumber() > 0;
	return bIsSupervisedDataGrid;
}

boolean KWDataGridOptimizer::IsOptimizationTimeElapsed() const
{
	require(timerOptimization.IsStarted());

	// Pas de limite par defaut
	if (optimizationParameters.GetOptimizationTime() == 0)
		return false;
	// Test de depassement sinon
	else
		return timerOptimization.GetElapsedTime() >= optimizationParameters.GetOptimizationTime();
}

void KWDataGridOptimizer::ResetProgressionIndicators() const
{
	timerOptimization.Reset();
	nVNSIteration = 0;
	nVNSNeighbourhoodLevelIndex = 0;
	nVNSNeighbourhoodLevelNumber = 0;
	dVNSNeighbourhoodSize = 0;
}

void KWDataGridOptimizer::DisplayProgression(const KWDataGrid* dataGrid) const
{
	double dTotalIterNumber;
	double dProgression;
	ALString sProgressionLabel;
	double dOptimizationTime;
	ALString sContext;
	ALString sTmp;

	// Calcul du temps d'optimisation
	dOptimizationTime = timerOptimization.GetElapsedTime();

	// Message d'avancement, avec prise en compte de la granularite uniquement si necessaire
	sProgressionLabel = sTmp + " Iter " + IntToString(nVNSIteration);
	if (nVNSIteration > 0 and nVNSNeighbourhoodLevelNumber > 0)
		sProgressionLabel += sTmp + "  VNS " + IntToString(nVNSNeighbourhoodLevelIndex) + "/" +
				     IntToString(nVNSNeighbourhoodLevelNumber) + " (" +
				     DoubleToString((int)(10000 * dVNSNeighbourhoodSize) / 10000.0) + ")";
	TaskProgression::DisplayLabel(sProgressionLabel);

	// Niveau d'avancement avec limite de temps
	if (optimizationParameters.GetOptimizationTime() > 0)
	{
		if (dOptimizationTime > optimizationParameters.GetOptimizationTime())
			TaskProgression::DisplayProgression(100);
		else
			TaskProgression::DisplayProgression(
			    (int)((dOptimizationTime * 100) / optimizationParameters.GetOptimizationTime()));
	}
	// Cas sans limite de temps
	else
	{
		// Calcul du nombre total d'iteration
		dTotalIterNumber = pow(2.0, optimizationParameters.GetOptimizationLevel());

		// Calcul de la progression courante
		// On tient compte du fait qu'avec l'algorithme VNS, le nombre d'iteration effectif peut depasser
		// le nombre d'iteration theorique obtenu dans le cas ou il n'y a aucune amelioration
		// La fonction tangente hyperbolique repond bien a ce besoin, en passant de 0 a 1 par les palier suivants:
		// - 10% -> 9.9%
		// - 50% -> 46%
		// - 100% -> 76%
		// - 200% -> 96%
		dProgression = tanh(nVNSIteration / dTotalIterNumber);
		TaskProgression::DisplayProgression((int)(dProgression * 100));
	}
}

void KWDataGridOptimizer::TraceOptimizationDetails(const ALString& sLabel, const KWDataGrid* optimizedDataGrid,
						   boolean bTraceDataGrid) const
{
	require(optimizedDataGrid != NULL);

	// Informations formattees sur une ligne
	cout << "Iter " << nVNSIteration << "\t";
	cout << "VNS " << nVNSNeighbourhoodLevelIndex << "/" << nVNSNeighbourhoodLevelNumber << "\t";
	cout << "Ngh " << std::fixed << std::setprecision(6) << dVNSNeighbourhoodSize << "\t";
	cout << optimizedDataGrid->GetObjectLabel() << "\t";
	cout << dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid) << "\t";
	cout << timerOptimization.GetElapsedTime() << "\t";
	cout << sLabel << endl;

	// Details sur une grille
	if (bTraceDataGrid)
		cout << *optimizedDataGrid << endl;
}

Profiler KWDataGridOptimizer::profiler;
