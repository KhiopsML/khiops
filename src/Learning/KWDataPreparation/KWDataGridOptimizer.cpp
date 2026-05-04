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

	// Ligne d'entete des messages
	DisplayOptimizationHeaderLine();

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
		if (optimizedDataGrid->IsVarPartDataGrid())
		{
			KWDataGridManager dataGridManager;
			KWDataGrid initialFromOptimizedDataGrid;

			dataGridManager.ExportDataGridWithMergedInnerAttributes(GetOptimizedInitialDataGrid(),
										optimizedDataGrid->GetInnerAttributes(),
										&initialFromOptimizedDataGrid);
			attributeSubsetStatsOptimizationHandler->HandleOptimizationStep(
			    optimizedDataGrid, &initialFromOptimizedDataGrid, bIsLastSaving);
		}
		else
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
		// Cas d'un coclustering de variables
		if (not currentDataGrid.IsVarPartDataGrid())
		{
			dCost = VNSOptimizeDataGrid(initialDataGrid, nNeighbourhoodLevelNumber, &currentDataGrid);
			if (dCost < dBestCost - dEpsilon)
			{
				dBestCost = dCost;
				SaveDataGrid(&currentDataGrid, optimizedDataGrid);
			}
		}
		// Sinon, cas d'une grille VarPart avec attribut de type VarPart
		else
		{
			// On distingue le cout dMergedCost de la meilleure grille et le cout dCost de l'antecedent de
			// la meilleure grille sans fusion des PV adjacentes dans un meme cluster
			dCost =
			    VNSOptimizeVarPartDataGrid(initialDataGrid, nNeighbourhoodLevelNumber, &currentDataGrid);
			if (dCost < dBestCost - dEpsilon)
			{
				dBestCost = dCost;
				SaveDataGrid(&currentDataGrid, optimizedDataGrid);
			}
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

	// On ne reverifie pas les precondition de la methode publique
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

		// Generation d'une solution dans un voisinnage de la meilleure solution
		KWDataGridOptimizer::GetProfiler()->BeginMethod("Generate neighbour solution");
		KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Neighbourhood size", dNeighbourhoodSize);
		GenerateNeighbourSolution(initialDataGrid, optimizedDataGrid, dNeighbourhoodSize, &neighbourDataGrid);
		KWDataGridOptimizer::GetProfiler()->EndMethod("Generate neighbour solution");

		// Parametrage du profiling pour l'optimisation
		KWDataGridOptimizer::GetProfiler()->BeginMethod("Optimize solution");
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("VNS level",
								   sTmp + IntToString(nVNSNeighbourhoodLevelIndex) +
								       "/" + IntToString(nVNSNeighbourhoodLevelNumber));
		KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("VNS neighbourhood size", dVNSNeighbourhoodSize);

		// Optimisation de cette solution
		dCost = OptimizeSolution(initialDataGrid, &neighbourDataGrid, true);
		KWDataGridOptimizer::GetProfiler()->EndMethod("Optimize solution");

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
	return dBestCost;
}

double KWDataGridOptimizer::VNSDataGridPostOptimizeVarPart(const KWDataGrid* initialDataGrid,
							   KWDataGridMerger* neighbourDataGrid,
							   double& dNeighbourDataGridCost, KWDataGrid* mergedDataGrid,
							   KWDataGrid* partitionedReferencePostMergedDataGrid) const
{
	boolean bDisplayResults = false;
	boolean bDisplayDetails = false;
	double dMergedCost;
	double dFusionDeltaCost;
	KWDataGridManager dataGridManager;
	ALString sLabel;

	// On ne reverifie pas les preconditions de la methode publique
	require(initialDataGrid != NULL);
	require(neighbourDataGrid != NULL);
	require(neighbourDataGrid->GetDataGridCosts() == dataGridCosts);
	require(fabs(dNeighbourDataGridCost - dataGridCosts->ComputeDataGridTotalCost(neighbourDataGrid)) < dEpsilon);

	// Initialisation du cout
	dMergedCost = dNeighbourDataGridCost;
	if (bDisplayResults)
	{
		cout << "VNSDataGridPostOptimizeVarPart: grille a post-optimiser\t" << dNeighbourDataGridCost << "\n";
		if (bDisplayDetails)
			cout << *neighbourDataGrid;
	}

	// CH AB AF a voir avec Marc
	// Cas ou l'on peut fusionner les parties de variable des cluster
	// Probleme rencontre pour fusionner une grille non informative
	// Pas de fusion ici mais question qui se pose : pourquoi dans OptimizeMerge on utilise systematiquement
	// un CopyInformativeVariables et dans les methodes qui l'appellent cela est fait en fonction de
	// bCleanNonInformativeVariables qui est a false
	if (neighbourDataGrid->GetInformativeAttributeNumber() > 0 and optimizationParameters.GetVarPartPostMerge())
	{
		// Tri des attributs
		neighbourDataGrid->SortAttributeParts();

		// Creation d'une nouvelle grille avec nouvelle description des PV fusionnees
		dFusionDeltaCost = dataGridManager.ExportDataGridWithVarPartMergeOptimization(
		    neighbourDataGrid, mergedDataGrid, dataGridCosts);
		assert(dFusionDeltaCost <= dNeighbourDataGridCost * dEpsilon);

		// Calcul et verification du cout
		dMergedCost = dNeighbourDataGridCost + dFusionDeltaCost;
		assert(fabs(dMergedCost - dataGridCosts->ComputeDataGridTotalCost(mergedDataGrid)) <=
		       dNeighbourDataGridCost * dEpsilon);
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("Coclustering", mergedDataGrid->GetObjectLabel());
		KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Cost", dMergedCost);
		if (bDisplayResults)
		{
			cout << "\tCout apres fusion des var parts\t" << dMergedCost << "\n";
			if (bDisplayDetails)
				cout << *mergedDataGrid;
		}

		// Post-optimisation de l'attribut VarPart avec modification des frontieres uniquement dans le cas d'une optimisation approfondie
		// (i.e. derniere granularite)
		// Non active si initialisation bVarPartPostOptimize= false;
		if (mergedDataGrid->GetInformativeAttributeNumber() > 1 and
		    optimizationParameters.GetVarPartPostOptimize())
		{
			CCVarPartDataGridPostOptimizer varPartDataGridPostOptimizer;
			KWDataGrid newMergedDataGrid;
			KWDataGrid mergedMergedDataGrid;
			IntVector ivGroups;
			ALString sInnerAttributeName;
			double dVarPartFusionDeltaCost;
			double dNewMergedCost;
			double dMergedMergedCost;
			int nGroupNumber;
			boolean bImprovement;

			// Parametrage pour l'attribut VarPart
			varPartDataGridPostOptimizer.SetPostOptimizationAttributeName(
			    mergedDataGrid->GetVarPartAttribute()->GetAttributeName());
			if (bDisplayResults)
			{
				cout << "\tDebut PostOptimisation VarPart\n";
				if (bDisplayDetails)
				{
					cout << "\tGrille initiale du DataGridOptimizer\n";
					cout << *GetOptimizedInitialDataGrid();
					cout << "Grille initiale utilisee pour l'export\n";
					cout << *initialDataGrid;
				}
			}

			// Boucle : on continue a post-optimiser tant qu'au moins un deplacement de VarPart
			// permet d'ameliorer le critere
			bImprovement = true;
			int nImprovementNumber = 0;
			while (bImprovement)
			{
				nImprovementNumber++;

				// Parametrage du profiling
				KWDataGridOptimizer::GetProfiler()->BeginMethod("Post-optimization IV");
				KWDataGridOptimizer::GetProfiler()->WriteKeyInt("Improvement trial",
										nImprovementNumber);

				// Construction d'une grille de reference avec des clusters contenant une seule
				// PV a partir des PV apres fusion
				// Parametrage par la grille initiale de l'optimiseur
				assert(mergedDataGrid->Check());
				dataGridManager.ExportDataGridWithSingletonVarParts(
				    GetOptimizedInitialDataGrid(), mergedDataGrid,
				    partitionedReferencePostMergedDataGrid, false);
				ivGroups.SetSize(
				    partitionedReferencePostMergedDataGrid->GetVarPartAttribute()->GetPartNumber());
				assert(partitionedReferencePostMergedDataGrid->Check());
				assert(partitionedReferencePostMergedDataGrid->GetInnerAttributes() ==
				       mergedDataGrid->GetInnerAttributes());

				// Affichage de la grille courante
				if (bDisplayResults)
				{
					cout << "\tImprovementNumber\t" << nImprovementNumber << endl;
					cout << "\tGrille post-fusionnee\t" << dMergedCost << endl;
					if (bDisplayDetails)
						cout << *mergedDataGrid;
					if (bDisplayDetails)
					{
						cout << "VNSDataGridPostOptimizeVarPart : grille de reference" << endl;
						cout << *partitionedReferencePostMergedDataGrid;
					}
				}

				// Exploration des deplacements pour tous les attributs
				bImprovement = varPartDataGridPostOptimizer.PostOptimizeLightVarPartDataGrid(
				    partitionedReferencePostMergedDataGrid, mergedDataGrid, &ivGroups);
				if (bImprovement)
				{
					// Initialisation par reopie de la meilleure grille courante
					dataGridManager.CopyDataGrid(mergedDataGrid, &newMergedDataGrid);

					// Mise a jour de la grille pour l'optimisation de cet attribut
					nGroupNumber = mergedDataGrid->GetVarPartAttribute()->GetPartNumber();
					dataGridManager.UpdateVarPartDataGridFromVarPartGroups(
					    partitionedReferencePostMergedDataGrid, &newMergedDataGrid, &ivGroups,
					    nGroupNumber);
					dNewMergedCost = dataGridCosts->ComputeDataGridTotalCost(&newMergedDataGrid);
					if (bDisplayResults)
					{
						cout << "\tGrille mise a jour best deplacement\t" << dNewMergedCost
						     << "\n";
						if (bDisplayDetails)
							cout << newMergedDataGrid;
						assert(newMergedDataGrid.Check());
					}

					// Mise a jour de la grille fusionnee courante par la grille obtenue par
					// fusion de la grille comportant les deplacements
					mergedMergedDataGrid.DeleteAll();
					dVarPartFusionDeltaCost =
					    dataGridManager.ExportDataGridWithVarPartMergeOptimization(
						&newMergedDataGrid, &mergedMergedDataGrid, dataGridCosts);
					assert(dVarPartFusionDeltaCost <= dNewMergedCost * dEpsilon);

					// Calcul et verification du cout
					dMergedMergedCost = dNewMergedCost + dVarPartFusionDeltaCost;
					assert(fabs(dMergedMergedCost -
						    dataGridCosts->ComputeDataGridTotalCost(&mergedMergedDataGrid)) <=
					       dNewMergedCost * dEpsilon);
					if (bDisplayResults)
					{
						cout << "\tGrille best deplacement et fusionnee\t" << dMergedMergedCost
						     << "\n";
						if (bDisplayDetails)
							cout << mergedMergedDataGrid;
					}

					// Cas ou la post-optimisation permet d'ameliorer le cout
					if (dMergedMergedCost < dMergedCost * (1 - dEpsilon))
					{
						dMergedCost = dMergedMergedCost;
						KWDataGridOptimizer::GetProfiler()->WriteKeyString(
						    "Coclustering", mergedMergedDataGrid.GetObjectLabel());
						KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Cost", dMergedCost);

						// Recopie de la grille fusionnee dans mergedDataGrid
						dataGridManager.CopyDataGrid(&mergedMergedDataGrid, mergedDataGrid);

						// Export de la grille avec les clusters post-optimises et les
						// PV de la grille initiale. Grille sans fusion des PV voisines.
						// C'est cette grille antecedent de la meilleure grille qui sera
						// memorisee
						neighbourDataGrid->DeleteAll();
						// CH 231 : initialDataGrid a remplacer par GetInitialVarPartDataGrid() ?
						dataGridManager.ExportDataGridWithReferenceVarPartClusters(
						    initialDataGrid, mergedDataGrid, neighbourDataGrid);
						if (bDisplayResults)
						{
							cout << "\tGrille best deplacement et initiale\t"
							     << dataGridCosts->ComputeDataGridTotalCost(
								    neighbourDataGrid)
							     << "\n";
							if (bDisplayDetails)
								cout << *neighbourDataGrid;
						}
						dNeighbourDataGridCost =
						    dataGridCosts->ComputeDataGridTotalCost(neighbourDataGrid);
					}
					// Sinon, il n'y a pas amelioration
					else
						bImprovement = false;
				}

				// Nettoyage -> peut etre a supprimer pour mutualiser avec utilisation de cette
				// grille pour HangleOptimizationStep
				partitionedReferencePostMergedDataGrid->DeleteAll();
				ivGroups.SetSize(0);
				KWDataGridOptimizer::GetProfiler()->EndMethod("Post-optimization IV");
			}
			if (bDisplayResults)
				cout << "\tFin PostOptimisation VarPart" << endl;
		}
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("Coclustering", neighbourDataGrid->GetObjectLabel());
		KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Cost", dMergedCost);
	}

	ensure(fabs(dNeighbourDataGridCost - dataGridCosts->ComputeDataGridTotalCost(neighbourDataGrid)) < dEpsilon);
	ensure(fabs(dMergedCost - dataGridCosts->ComputeDataGridTotalCost(mergedDataGrid)) < dEpsilon or
	       (neighbourDataGrid->GetInformativeAttributeNumber() == 0 and dMergedCost == dNeighbourDataGridCost));
	return dMergedCost;
}

double KWDataGridOptimizer::VNSOptimizeVarPartDataGrid(const KWDataGrid* initialDataGrid, int nNeighbourhoodLevelNumber,
						       KWDataGrid* optimizedDataGrid) const
{
	boolean bDisplayMainSteps = false;
	double dBestCost;
	double dCost;
	double dMergedCost;
	double dBestMergedCost;
	KWDataGrid mergedDataGrid;
	KWDataGrid partitionedReferencePostMergedDataGrid;
	KWDataGridManager dataGridManager;
	KWDataGridMerger neighbourDataGrid;
	KWDataGrid initialFromOptimizedDataGrid;
	int nIndex;
	double dMinNeighbourhoodSize;
	double dDecreaseFactor;
	double dNeighbourhoodSize;
	ALString sTmp;

	// On ne reverifie pas les precondition de la methode publique
	require(nNeighbourhoodLevelNumber >= 0);

	// Initialisations
	neighbourDataGrid.SetDataGridCosts(dataGridCosts);
	dBestCost = dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);
	dBestMergedCost = dBestCost;

	// Calcul d'une taille minimale de voisinnage, relative a la taille de la base
	// On souhaite impliquer au minimum 3 nouvelles parties par attribut
	dMinNeighbourhoodSize = 3.0 / (3 + initialDataGrid->GetGridFrequency());

	// Calcul du taux de decroissance des tailles de voisinnage de facon a obtenir
	// une taille minimale de voisinnage suffisante
	dDecreaseFactor = 1.0 / pow(dMinNeighbourhoodSize, 1.0 / (nNeighbourhoodLevelNumber + 1));

	// On optimise tant qu'on ne depasse pas la taille max de voisinage
	nVNSNeighbourhoodLevelNumber = nNeighbourhoodLevelNumber;
	nIndex = 0;
	while (nIndex <= nNeighbourhoodLevelNumber)
	{
		nVNSNeighbourhoodLevelIndex = nIndex;
		dNeighbourhoodSize = pow(1.0 / dDecreaseFactor, nIndex);
		dVNSNeighbourhoodSize = dNeighbourhoodSize;

		// Generation d'une solution dans un voisinnage de la meilleure solution
		KWDataGridOptimizer::GetProfiler()->BeginMethod("Generate neighbour solution");
		KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Neighbourhood size", dNeighbourhoodSize);

		// Creation d 'une grille initiale avec les memes innerAttributes que l' optimizedDataGrid
		// On utilise la grille initialle la plus fine pour la retokenisation
		// DD 461 initialDataGrid utilisee deux fois pour sourceDataGrid et inputDataGrid : a revoir
		dataGridManager.ExportDataGridWithMergedInnerAttributes(GetOptimizedInitialDataGrid(),
									optimizedDataGrid->GetInnerAttributes(),
									&initialFromOptimizedDataGrid);
		assert(initialFromOptimizedDataGrid.Check());

		// Generation d'un voisin
		GenerateNeighbourSolution(&initialFromOptimizedDataGrid, optimizedDataGrid, dNeighbourhoodSize,
					  &neighbourDataGrid);
		KWDataGridOptimizer::GetProfiler()->EndMethod("Generate neighbour solution");

		// Display pour comparaison des deux modes avec/sans antecedent
		if (bDisplayMainSteps)
		{
			cout << "nIndex : " << nIndex << endl;
			cout << "grille initiale : " << initialFromOptimizedDataGrid << endl;
			cout << "grille de depart :" << *optimizedDataGrid << endl;
			cout << "grille voisine :" << neighbourDataGrid << endl;
		}

		// Parametrage du profiling pour l'optimisation
		KWDataGridOptimizer::GetProfiler()->BeginMethod("Optimize solution");
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("VNS level",
								   sTmp + IntToString(nVNSNeighbourhoodLevelIndex) +
								       "/" + IntToString(nVNSNeighbourhoodLevelNumber));
		KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("VNS neighbourhood size", dVNSNeighbourhoodSize);

		// Optimisation de cette solution
		// On utilise comme grille de reference celle qui a ete construite pour la generation de la grille voisine
		initialFromOptimizedDataGrid.DeleteAll();
		dataGridManager.ExportDataGridWithMergedInnerAttributes(GetOptimizedInitialDataGrid(),
									neighbourDataGrid.GetInnerAttributes(),
									&initialFromOptimizedDataGrid);
		dCost = OptimizeSolution(&initialFromOptimizedDataGrid, &neighbourDataGrid, true);
		KWDataGridOptimizer::GetProfiler()->EndMethod("Optimize solution");

		// Suivi et comparaison avec/sans antecedent
		if (bDisplayMainSteps)
			cout << "Apres OptimizeSolution" << neighbourDataGrid << endl;

		// Post-optimisation des parties de variables de la grille
		// A terme, a deplacer dans OptimizeSolution
		dMergedCost = dCost;
		if (initialDataGrid->IsVarPartDataGrid())
		{
			// On utilise comme grille de reference celle qui a ete construite pour la generation de la grille voisine
			dMergedCost =
			    VNSDataGridPostOptimizeVarPart(&initialFromOptimizedDataGrid, &neighbourDataGrid, dCost,
							   &mergedDataGrid, &partitionedReferencePostMergedDataGrid);

			// Suivi et comparaison avec/sans antecedent
			if (bDisplayMainSteps)
				cout << "Apres VNSDataGridPostOptimizeVarPart" << mergedDataGrid << endl;
		}

		// Si amelioration: on la memorise
		if (dMergedCost < dBestMergedCost - dEpsilon)
		{
			dBestCost = dCost;
			dBestMergedCost = dMergedCost;

			// Sauvegarde de la meilleure solution
			// On sauvegarde l'antecedent de la meilleure grille post mergee
			// On ne sauvegarde donc pas la meilleure grille post mergee car cela necessiterait de
			// modifier la grille initiale qui devrait etre en coherence avec cette grille post mergee.
			// La grille initiale est conservee pour une granularite donnee
			// La grille de reference est necessaire pour HandleOptimizationStep
			if (mergedDataGrid.GetInformativeAttributeNumber() == 0)
				SaveDataGrid(&neighbourDataGrid, optimizedDataGrid);
			else
			{
				// Sauvegarde de la grille optimisee fusionnee en remplacement de l'antecedent
				SaveDataGrid(&mergedDataGrid, optimizedDataGrid);
			}

			// Gestion de la meilleure solution
			// Cas sans post-optimisation des VarPart : pas de mergedDataGrid ni de partitionedReferencePostMergedDataGrid
			if (mergedDataGrid.GetInformativeAttributeNumber() == 0)
				HandleOptimizationStep(optimizedDataGrid, initialDataGrid, false);
			// Sinon
			else
			{
				// Initialisation de la grille source a la grille tokenisee la plus fine
				dataGridManager.ExportDataGridWithSingletonVarParts(
				    GetOptimizedInitialDataGrid(), &mergedDataGrid,
				    &partitionedReferencePostMergedDataGrid, true);
				HandleOptimizationStep(&mergedDataGrid, &partitionedReferencePostMergedDataGrid, false);

				// Nettoyage
				partitionedReferencePostMergedDataGrid.DeleteAll();
			}
		}
		// Sinon: on passe a un niveau de voisinage plus fin
		else
			nIndex++;

		// Nettoyage
		mergedDataGrid.DeleteAll();
		initialFromOptimizedDataGrid.DeleteAll();

		// Test de fin de tache
		if (TaskProgression::IsInterruptionRequested())
			break;

		// Test si depassement de temps
		if (IsOptimizationTimeElapsed())
			break;
	}

	// Memorisation du meilleur cout parmi les grilles post-fusionnees
	dBestCost = dBestMergedCost;
	ensure(fabs(dBestCost - dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid)) < dEpsilon);
	ensure(optimizedDataGrid->Check());
	return dBestCost;
}

double KWDataGridOptimizer::OptimizeSolution(const KWDataGrid* initialDataGrid, KWDataGridMerger* dataGridMerger,
					     boolean bDeepPostOptimization) const
{
	boolean bDisplay = false;
	KWDataGridPostOptimizer dataGridPostOptimizer;
	double dCost;
	ALString sSuffix;

	require(initialDataGrid != NULL);
	require(dataGridMerger != NULL);
	require(not initialDataGrid->IsVarPartDataGrid() or
		initialDataGrid->GetInnerAttributes() == dataGridMerger->GetInnerAttributes());

	// Suivi de la progression
	nVNSIteration++;
	DisplayProgression(initialDataGrid);

	// Initialisations
	dataGridPostOptimizer.SetDataGridCosts(dataGridCosts);

	// Affichage du cout initial
	DisplayOptimizationDetails(dataGridMerger, false);

	// Cout initial si aucune optimisation, ou si une trace est demandee
	dCost = DBL_MAX;
	if ((not optimizationParameters.GetOptimize() and not optimizationParameters.GetPreOptimize() and
	     not optimizationParameters.GetPostOptimize()) or
	    bDisplay or KWDataGridOptimizer::GetProfiler()->IsStarted())
		dCost = dataGridCosts->ComputeDataGridTotalCost(dataGridMerger);
	if (bDisplay)
		cout << "Affichage de l'evolution des couts" << dCost << "\n";
	KWDataGridOptimizer::GetProfiler()->WriteKeyString("Coclustering", dataGridMerger->GetObjectLabel());
	KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Cost", dCost);

	// Pre-optimisation de la grille
	if (optimizationParameters.GetPreOptimize() and not TaskProgression::IsInterruptionRequested() and
	    initialDataGrid->GetAttributeNumber() > 1)
	{
		KWDataGridOptimizer::GetProfiler()->BeginMethod("Pre-optimization");
		dCost = dataGridPostOptimizer.PostOptimizeDataGrid(initialDataGrid, dataGridMerger, false);
		if (bDisplay)
			cout << dCost << "\n";
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("Coclustering", dataGridMerger->GetObjectLabel());
		KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Cost", dCost);
		KWDataGridOptimizer::GetProfiler()->EndMethod("Pre-optimization");
	}

	// Optimisation par fusion des groupes
	if (optimizationParameters.GetOptimize() and not TaskProgression::IsInterruptionRequested())
	{
		if (not bDeepPostOptimization)
			sSuffix = " (slight)";
		KWDataGridOptimizer::GetProfiler()->BeginMethod("Greedy merge optimization" + sSuffix);
		dCost = dataGridMerger->Merge();
		if (bDisplay)
			cout << dCost << "\n";
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("Coclustering", dataGridMerger->GetObjectLabel());
		KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Cost", dCost);
		KWDataGridOptimizer::GetProfiler()->EndMethod("Greedy merge optimization" + sSuffix);
	}

	// Post-optimisation de la grille selon le niveau de post-optimisation
	if (optimizationParameters.GetPostOptimize() and not TaskProgression::IsInterruptionRequested() and
	    initialDataGrid->GetAttributeNumber() > 1)
	{
		KWDataGridOptimizer::GetProfiler()->BeginMethod("Post-optimization");
		dCost =
		    dataGridPostOptimizer.PostOptimizeDataGrid(initialDataGrid, dataGridMerger, bDeepPostOptimization);
		if (bDisplay)
			cout << dCost << "\n\n";
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("Coclustering", dataGridMerger->GetObjectLabel());
		KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Cost", dCost);
		KWDataGridOptimizer::GetProfiler()->EndMethod("Post-optimization");
	}

	// Recalcul du cout si la tache est interrompue, pour sortir avec un cout coherent
	if (TaskProgression::IsInterruptionRequested())
		dCost = dataGridCosts->ComputeDataGridTotalCost(dataGridMerger);

	// Affichage du cout final
	DisplayOptimizationDetails(dataGridMerger, true);

	// Retour du cout
	ensure(dataGridMerger->Check());
	ensure(fabs(dataGridCosts->ComputeDataGridTotalCost(dataGridMerger) - dCost) < dEpsilon);
	return dCost;
}

void KWDataGridOptimizer::GenerateNeighbourSolution(const KWDataGrid* initialDataGrid,
						    const KWDataGrid* optimizedDataGrid, double dNoiseRate,
						    KWDataGridMerger* neighbourDataGridMerger) const
{
	boolean bDisplayResults = false;
	boolean bDisplayDetails = false;
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
	KWDataGrid initialFromSurtokenizedDataGrid;
	KWDataGrid surtokenizedDataGrid;
	int nInitialCurrentTokenNumber;
	int nCurrentTokenNumber;
	int nTargetTokenNumber;
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
	if (bDisplayResults)
	{
		cout << "GenerateNeighbourSolution\t" << dNoiseRate << "\n";
		cout << "\tGetInitialVarPartDataGrid()\t"
		     << dataGridCosts->ComputeDataGridTotalCost(GetOptimizedInitialDataGrid()) << "\t"
		     << GetOptimizedInitialDataGrid()->GetObjectLabel() << "\n";
		cout << "\tinitialDataGrid\t" << dataGridCosts->ComputeDataGridTotalCost(initialDataGrid) << "\t"
		     << initialDataGrid->GetObjectLabel() << "\n";
		cout << "\toptimizedDataGrid\t" << dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid) << "\t"
		     << optimizedDataGrid->GetObjectLabel() << "\n";
		if (bDisplayDetails)
			cout << *optimizedDataGrid << "\n";
	}

	// Surtokenisation d'une grille
	if (initialDataGrid->IsVarPartDataGrid())
	{
		// Nombre de tokens de la grille en entree
		nInitialCurrentTokenNumber = GetOptimizedInitialDataGrid()
						 ->GetVarPartAttribute()
						 ->GetInnerAttributes()
						 ->ComputeTotalInnerAttributeVarParts();
		nCurrentTokenNumber =
		    initialDataGrid->GetVarPartAttribute()->GetInnerAttributes()->ComputeTotalInnerAttributeVarParts();

		// Nombre de token cibles
		nTargetTokenNumber =
		    nCurrentTokenNumber + (int)(dNoiseRate * (nInitialCurrentTokenNumber - nCurrentTokenNumber));

		// Correction du nombre de tokens
		nTargetTokenNumber = min(nTargetTokenNumber, initialDataGrid->GetGridFrequency());

		// Generation de la grille surtokenisee
		dataGridManager.ExportDataGridWithRandomizedInnerAttributes(
		    GetOptimizedInitialDataGrid(), optimizedDataGrid, &surtokenizedDataGrid, nTargetTokenNumber);
		if (bDisplayResults)
		{
			cout << "\tTargetTokenNumber\t" << nTargetTokenNumber << "\n";
			cout << "\tsurtokenizedDataGrid\t"
			     << dataGridCosts->ComputeDataGridTotalCost(&surtokenizedDataGrid) << "\t"
			     << surtokenizedDataGrid.GetObjectLabel() << "\n";
			if (bDisplayDetails)
				cout << surtokenizedDataGrid << "\n";
		}

		// Export de la grille antecdent de la grille sur-tokenisee
		dataGridManager.ExportDataGridWithMergedInnerAttributes(GetOptimizedInitialDataGrid(),
									surtokenizedDataGrid.GetInnerAttributes(),
									&initialFromSurtokenizedDataGrid);

		// On remplace le parametrage de la methode par un parametrage dediee au cas VarPart
		initialDataGrid = &initialFromSurtokenizedDataGrid;
		optimizedDataGrid = &surtokenizedDataGrid;
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
	// Surtokenisation: dans le cas VarPart, considerer tous les attributs comme obligatoires
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

	// Affichage final
	if (bDisplayResults)
	{
		cout << "\tneighbourDataGridMerger\t"
		     << dataGridCosts->ComputeDataGridTotalCost(neighbourDataGridMerger) << "\t"
		     << neighbourDataGridMerger->GetObjectLabel() << "\n";
		if (bDisplayDetails)
			cout << *neighbourDataGridMerger << "\n";
	}

	// Fin de tache
	TaskProgression::EndTask();
	ensure(neighbourDataGridMerger->Check());
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

void KWDataGridOptimizer::DisplayOptimizationHeaderLine() const
{
	if (optimizationParameters.GetDisplayDetails())
	{
		// Lignes d'entete
		cout << "Context\tTime\tIter\tNeigh. size\t";
		cout << "Initial\t\t\t\tFinal\t\t\t\t\n";
		cout << "\t\t\t\tAtt. number\tPart number\tCell number\tCost\t";
		cout << "Att. number\tPart number\tCell number\tCost\n";
	}
}

void KWDataGridOptimizer::DisplayOptimizationDetails(const KWDataGrid* optimizedDataGrid, boolean bOptimized) const
{
	ALString sContext;
	ALString sTmp;

	// Affichage des details d'optimisation
	if (optimizationParameters.GetDisplayDetails())
	{
		// Affichage de l'iteration
		if (not bOptimized)
		{
			sContext = sTmp + "G=" + IntToString(optimizedDataGrid->GetGranularity());
			if (optimizedDataGrid->IsVarPartDataGrid())
				sContext +=
				    sTmp + ", T=" +
				    IntToString(
					optimizedDataGrid->GetInnerAttributes()->ComputeTotalInnerAttributeVarParts());
			cout << sContext << "\t" << timerOptimization.GetElapsedTime() << "\t" << nVNSIteration << "\t"
			     << dVNSNeighbourhoodSize << "\t";
		}

		// Affichage des caracteristiques de la grille terminale
		cout << optimizedDataGrid->GetAttributeNumber() << "\t" << optimizedDataGrid->GetTotalPartNumber()
		     << "\t" << optimizedDataGrid->GetCellNumber() << "\t"
		     << dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);
		if (not bOptimized)
			cout << "\t";
		else
			cout << "\n";
		cout << flush;
	}
}

Profiler KWDataGridOptimizer::profiler;
