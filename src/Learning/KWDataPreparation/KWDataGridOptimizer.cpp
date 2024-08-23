// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataGridOptimizer.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridOptimizer

KWDataGridOptimizer::KWDataGridOptimizer()
{
	classStats = NULL;
	dataGridCosts = NULL;
	bCleanNonInformativeVariables = false;
	initialVarPartDataGrid = NULL;
	attributeSubsetStatsHandler = NULL;
	dEpsilon = 1e-6;
	ResetProgressionIndicators();
}

KWDataGridOptimizer::~KWDataGridOptimizer() {}

void KWDataGridOptimizer::Reset()
{
	classStats = NULL;
	dataGridCosts = NULL;
	bCleanNonInformativeVariables = false;
	initialVarPartDataGrid = NULL;
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
// CH IV Begin
void KWDataGridOptimizer::SetInitialVarPartDataGrid(KWDataGrid* refDataGrid)
{
	require(refDataGrid != NULL);
	require(refDataGrid->IsVarPartDataGrid());
	initialVarPartDataGrid = refDataGrid;
}

KWDataGrid* KWDataGridOptimizer::GetInitialVarPartDataGrid() const
{
	return initialVarPartDataGrid;
}
// CH IV End
double KWDataGridOptimizer::OptimizeDataGrid(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid) const

{
	boolean bDisplayResults = false;
	boolean bDisplayGranularities = false;
	KWDataGrid granularizedDataGrid;
	KWDataGrid* granularizedOptimizedDataGrid;
	KWDataGridManager dataGridManager;
	double dGranularityBestCost;
	double dBestCost;
	boolean bIsOptimizationNeeded;
	ObjectDictionary odQuantileBuilders;
	IntVector ivMaxPartNumbers;
	IntVector ivPreviousPartNumber;
	IntVector ivCurrentPartNumber;
	const double dMinimalGranularityIncreasingCoefficient = 2;
	int nMaxExploredGranularity;
	int nGranularityIndex;
	boolean bIsLastGranularity;
	boolean bIsGranularitySelected;
	int nAttribute;
	double dBestMergedCost;
	double dMergedCost;
	KWDataGrid granularizedPostMergedOptimizedDataGrid;
	KWDataGrid partitionedReferenceGranularizedPostMergedDataGrid;
	double dFusionDeltaCost;
	int nCurrentExploredGranularity;
	int nLastExploredGranularity;
	ALString sSuffix;
	ALString sTmp;

	require(initialDataGrid != NULL);
	require(optimizedDataGrid != NULL);

	// Debut de suivi des taches
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Data Grid optimization");

	// Ligne d'entete des messages
	DisplayOptimizationHeaderLine();

	//Initialisations
	dGranularityBestCost = DBL_MAX;
	dBestMergedCost = dGranularityBestCost;
	ResetProgressionIndicators();
	timerOptimization.Start();

	// Controle de la graine aleatoire pour avoir des resultats reproductibles
	SetRandomSeed(1);

	// Construction d'une grille terminale pour la solution initiale
	dBestCost = InitializeWithTerminalDataGrid(initialDataGrid, optimizedDataGrid);
	if (bDisplayResults)
	{
		cout << "OptimizeDataGrid: Cout grille terminale independant de la granularite " << dBestCost << endl;
		cout << "OptimizeDataGrid: Grille initiale avant optimisation" << endl;
		initialDataGrid->Write(cout);
	}

	// On determine si on peut potentiellement faire mieux que la grille terminale
	bIsOptimizationNeeded = IsOptimizationNeeded(initialDataGrid);

	// Cas ou la grille terminale est ameliorable
	if (bIsOptimizationNeeded and not TaskProgression::IsInterruptionRequested())
	{
		// Calcul de la granularite max a explorer
		nMaxExploredGranularity = ComputeMaxExploredGranularity(initialDataGrid);

		// Initialisation des quantiles builders a partir de la grille source
		dataGridManager.SetSourceDataGrid(initialDataGrid);
		dataGridManager.InitializeQuantileBuilders(&odQuantileBuilders, &ivMaxPartNumbers);
		if (bDisplayResults)
			cout << "ivMaxPartNumbers Granularisation\t" << ivMaxPartNumbers << flush;

		// Initialisation des vecteurs de nombre de parties courant et precedent
		ivPreviousPartNumber.SetSize(ivMaxPartNumbers.GetSize());
		ivCurrentPartNumber.SetSize(ivMaxPartNumbers.GetSize());

		// Initialisation
		nCurrentExploredGranularity = -1;
		nLastExploredGranularity = -1;

		// Parcours des granularites
		nGranularityIndex = 1;
		bIsLastGranularity = false;
		while (nGranularityIndex <= nMaxExploredGranularity and not bIsLastGranularity)
		{
			// Arret si interruption utilisateur
			if (TaskProgression::IsInterruptionRequested())
				break;

			// Granularisation de la grille initiale
			dataGridManager.SetSourceDataGrid(initialDataGrid);
			dataGridManager.ExportGranularizedDataGrid(&granularizedDataGrid, nGranularityIndex,
								   &odQuantileBuilders);

			//////////////////////////////////////////////////////////////////////////////////////////////
			// On determine si la granularite courante doit etre traitee
			// - bIsGranularitySelected: parce qu'elle differe suffisament de la granularite precedente
			//   et de la granularite max
			// - bIsLastGranularity: par ce que c'est la derniere

			// Calcul du nombre de nombre de parties par attribut attribut pour la granularite en cours
			for (nAttribute = 0; nAttribute < granularizedDataGrid.GetAttributeNumber(); nAttribute++)
			{
				// Memorisation du nombre de parties de l'attribut granularise
				ivCurrentPartNumber.SetAt(
				    nAttribute, cast(KWDGAttribute*, granularizedDataGrid.GetAttributeAt(nAttribute))
						    ->GetPartNumber());
			}

			// On determine si on a atteint la granularite max
			bIsLastGranularity = true;
			if (nGranularityIndex < nMaxExploredGranularity)
			{
				for (nAttribute = 0; nAttribute < granularizedDataGrid.GetAttributeNumber();
				     nAttribute++)
				{
					// La granularite n'est pas max si le nombre de parties courant est inferieur
					// au nombre max de parties pour au moins un attribut
					if (ivCurrentPartNumber.GetAt(nAttribute) < ivMaxPartNumbers.GetAt(nAttribute))
					{
						bIsLastGranularity = false;
						break;
					}
				}
			}

			// Dans le cas de la granularite max, on positionne l'index de granularite au maximum
			// pour des raisons d'affichage (pour ne pas afficher la granularite courante)
			if (bIsLastGranularity)
				granularizedDataGrid.SetGranularity(nMaxExploredGranularity);
			assert(bIsLastGranularity == IsLastGranularity(&granularizedDataGrid));

			// Test si on doit traiter la granularite
			bIsGranularitySelected = false;
			if (IsOptimizationNeeded(&granularizedDataGrid))
			{
				// Dans le cas d'une grille avec un attribut instances et un attribut VarPart, tant que
				// l'attribut instances ne contient qu'une seule partie, la granularite n'est pas selectionnee.
				// La granularisation de l'attribut instances est reduite au fourre-tout (1 seule partie)
				// tant que le nombre d'observations par instances n'est pas superieur a l'effectif minimal N/2^G
				// Pour un nombre d'observations egal au nombre de variables pour toutes les instances,
				// il faut atteindre G tel que G > Gmax - log(K) / log(2)
				//
				// On ne traite cette granularite que si elle est differe suffisament de la precedente et de la
				for (nAttribute = 0; nAttribute < granularizedDataGrid.GetAttributeNumber();
				     nAttribute++)
				{
					// Cas d'accroissement suffisant du nombre de parties
					if ((ivCurrentPartNumber.GetAt(nAttribute) >=
					     ivPreviousPartNumber.GetAt(nAttribute) *
						 dMinimalGranularityIncreasingCoefficient) and
					    (ivCurrentPartNumber.GetAt(nAttribute) *
						 dMinimalGranularityIncreasingCoefficient <=
					     ivMaxPartNumbers.GetAt(nAttribute)))
					{
						bIsGranularitySelected = true;
						break;
					}
				}
			}

			// Affichage de l'info, sur la granularite
			if (bDisplayGranularities)
			{
				cout << "OptimizeDataGrid: granularity " << nGranularityIndex << ": "
				     << BooleanToString(bIsGranularitySelected);
				if (bIsLastGranularity)
					cout << " (last)";
				cout << endl;
			}

			// Exploration de la granularite courante
			if (IsOptimizationNeeded(&granularizedDataGrid) and
			    (bIsGranularitySelected or bIsLastGranularity))
			{
				assert(bIsGranularitySelected or bIsLastGranularity);

				// Memorisation des granularites exploitees
				nLastExploredGranularity = nCurrentExploredGranularity;
				nCurrentExploredGranularity = nGranularityIndex;

				// Initialisation de la grille granularisee optimisee a la grille terminale
				granularizedOptimizedDataGrid = new KWDataGrid;
				dGranularityBestCost = InitializeWithTerminalDataGrid(&granularizedDataGrid,
										      granularizedOptimizedDataGrid);
				if (bDisplayResults)
				{
					cout
					    << "OptimizeDataGrid: Cout Grille initiale granularisee pour granularite = "
					    << IntToString(nGranularityIndex) << "\t"
					    << dataGridCosts->ComputeDataGridTotalCost(&granularizedDataGrid) << endl;
					granularizedDataGrid.Write(cout);
					granularizedDataGrid.WriteAttributes(cout);
					granularizedDataGrid.WriteAttributeParts(cout);
					cout << "OptimizeDataGrid: Cout grille terminale pour cette granularite\t"
					     << dGranularityBestCost << endl;
				}

				// Arret si interruption utilisateur
				if (TaskProgression::IsInterruptionRequested())
					break;

				// Parametrage du profiling
				sSuffix = "";
				if (bIsLastGranularity)
					sSuffix = " (last)";
				KWDataGridOptimizer::GetProfiler()->BeginMethod("Optimize granularity" + sSuffix);
				KWDataGridOptimizer::GetProfiler()->WriteKeyString("Granularity index",
										   IntToString(nGranularityIndex));
				if (optimizedDataGrid->IsVarPartDataGrid())
					KWDataGridOptimizer::GetProfiler()->WriteKeyString(
					    "VarPart granularity",
					    IntToString(
						optimizedDataGrid->GetInnerAttributes()->GetVarPartGranularity()));

				// CH IV VNS
				if (bDisplayGranularities)
				{
					cout << "OptimizeDataGrid\tGranularite\t" << nGranularityIndex << "\n";
					if (granularizedDataGrid.IsVarPartDataGrid())
						cout << "attribut VarPart granularise : initial value number\t"
						     << granularizedDataGrid.GetVarPartAttribute()
							    ->GetInitialValueNumber()
						     << endl;
				}

				// Optimisation de la grille granularisee
				// Cas non supervise pour les granularites intermediaires: optimisation legere
				if (not IsSupervisedDataGrid(initialDataGrid) and not bIsLastGranularity)
					dGranularityBestCost = SlightOptimizeGranularizedDataGrid(
					    &granularizedDataGrid, granularizedOptimizedDataGrid);
				// Cas supervise ou derniere granularite en non supervise: optimisation profonde
				else
					dGranularityBestCost = OptimizeGranularizedDataGrid(
					    &granularizedDataGrid, granularizedOptimizedDataGrid);

				// Fin du parametrage du profiling
				KWDataGridOptimizer::GetProfiler()->EndMethod("Optimize granularity" + sSuffix);
				if (bDisplayResults)
				{
					cout << "OptimizeDataGrid: Apres OptimizeGranularizedDataGrid pour Granularite "
					     << nGranularityIndex << "\t Cout " << dGranularityBestCost << endl;
					granularizedOptimizedDataGrid->Write(cout);
				}

				// CH IV Begin
				// Dans le cas d'un coclustering instances * variables, le cout obtenu
				// dGranularityBestCost est le cout de l'antecedent de la meilleure grille avant fusion
				// des PV d'un meme cluster L'amelioration de cout doit etre mesuree par rapport au cout
				// de la grille post-fusionnee de granularizedOptimizedDataGrid
				if (granularizedOptimizedDataGrid->IsVarPartDataGrid())
				{
					if (granularizedOptimizedDataGrid->GetInformativeAttributeNumber() > 0 and
					    optimizationParameters.GetVarPartPostMerge())
					{
						dataGridManager.SetSourceDataGrid(granularizedOptimizedDataGrid);
						// Creation d'une nouvelle grille avec nouvelle description des PV

						// Calcul de la grille de reference post fusionnee a partir de granularizedDataGrid
						dFusionDeltaCost =
						    dataGridManager.ExportDataGridWithVarPartMergeOptimization(
							&granularizedPostMergedOptimizedDataGrid, dataGridCosts);
						assert(not granularizedPostMergedOptimizedDataGrid.GetVarPartsShared());

						// Calcul et verification du cout
						dMergedCost = dGranularityBestCost + dFusionDeltaCost;
						// Le cout precedent devra etre correct
						assert(dMergedCost * (1 - dEpsilon) <
						       dataGridCosts->ComputeDataGridTotalCost(
							   &granularizedPostMergedOptimizedDataGrid));
						assert(dataGridCosts->ComputeDataGridTotalCost(
							   &granularizedPostMergedOptimizedDataGrid) <
						       dMergedCost * (1 + dEpsilon));
						if (bDisplayResults)
						{
							cout << "OptimizeDataGrid: Niveau prepartitionnement \t"
							     << granularizedOptimizedDataGrid->GetInnerAttributes()
								    ->GetVarPartGranularity()
							     << "\t Grille avant fusion \t" << dGranularityBestCost
							     << "\n";
							granularizedOptimizedDataGrid->Write(cout);
						}
					}
					else
						dMergedCost = dGranularityBestCost;

					if (dMergedCost < dBestMergedCost - dEpsilon)
					{
						dBestMergedCost = dMergedCost;
						dBestCost = dGranularityBestCost;

						// Memorisation de l'antecedent du nouvel optimum avant post-fusion
						dataGridManager.CopyDataGrid(granularizedOptimizedDataGrid,
									     optimizedDataGrid);
					}

					// Cas ou il s'agit de la derniere granularite : on met a jour les infos du
					// coclustering
					if (bIsLastGranularity)
					{
						if (bDisplayResults)
							cout << "OptimizeDataGrid: Mise a jour de la memorisation du "
								"coclustering pour la derniere granularite "
							     << endl;

						if (granularizedPostMergedOptimizedDataGrid
							.GetInformativeAttributeNumber() == 0)
							HandleOptimizationStep(optimizedDataGrid, &granularizedDataGrid,
									       true);

						else
						{
							// La grille source est la grille de reference qui contient la
							// partition la plus fine
							dataGridManager.SetSourceDataGrid(GetInitialVarPartDataGrid());
							dataGridManager.ExportDataGridWithSingletonVarParts(
							    &granularizedPostMergedOptimizedDataGrid,
							    &partitionedReferenceGranularizedPostMergedDataGrid, true);
							HandleOptimizationStep(
							    &granularizedPostMergedOptimizedDataGrid,
							    &partitionedReferenceGranularizedPostMergedDataGrid, false);
							// Nettoyage
							partitionedReferenceGranularizedPostMergedDataGrid.DeleteAll();
						}
					}

					// Nettoyage
					granularizedPostMergedOptimizedDataGrid.DeleteAll();
				}
				// CH IV End

				// Sinon : cas coclustering de variables
				else
				{
					// Cas d'amelioration du cout
					if (dGranularityBestCost < dBestCost - dEpsilon)
					{
						dBestCost = dGranularityBestCost;

						// Memorisation du nouvel optimum
						dataGridManager.CopyDataGrid(granularizedOptimizedDataGrid,
									     optimizedDataGrid);
						if (bDisplayResults)
						{
							cout << "OptimizeDataGrid: Grille granularizedOptimizedDataGrid"
							     << endl;
							granularizedOptimizedDataGrid->Write(cout);
						}
					}
					// Cas ou il s'agit de la derniere granularite : on met a jour les infos dans le
					// cas d'un coclustering
					if (bIsLastGranularity)
					{
						if (bDisplayResults)
							cout << "OptimizeDataGrid: Mise a jour de la memorisation du "
								"coclustering pour la derniere granularite "
							     << endl;

						HandleOptimizationStep(optimizedDataGrid, &granularizedDataGrid, true);
					}
				}

				// Nettoyage de la grille optimisee pour cette granularite
				delete granularizedOptimizedDataGrid;
				granularizedOptimizedDataGrid = NULL;

				// Cas d'un temps limite: warning si arret avant avoir atteint granularite max
				if (IsOptimizationTimeElapsed() and not bIsLastGranularity)
				{
					// Affichage d'un warning pour eventuelle modification de l'optimisation time
					AddWarning(sTmp + "All the optimization time has been used until granularity " +
						   IntToString(nGranularityIndex) + ", but maximum granularity (" +
						   IntToString(nGranularityIndex) + ") has not been reached." +
						   "You could obtain better results with larger optimization time.");
					if (bDisplayResults)
						cout << "OptimizeDataGrid :Totalite du temps alloue ecoule apres la "
							"granularite \t"
						     << nGranularityIndex << endl;

					// Arret
					break;
				}

				// Memorisation du nombre de parties par attribut pour comparaison a l'etape suivante
				ivPreviousPartNumber.CopyFrom(&ivCurrentPartNumber);
			}

			// Nettoyage de la grille granularisee
			granularizedDataGrid.DeleteAll();

			// Granularite suivante
			nGranularityIndex++;
		}

		// Post-optimisation de la grnularite dans le le cas d'unze grille supervisee pour laquelle
		// la granularite fait partie des parametre du modele
		if (IsSupervisedDataGrid(initialDataGrid))
		{
			// Post-optimisation de la granularite : on attribue a la grille optimale la plus petite
			// granularite pour laquelle cette grille est definie
			if (nLastExploredGranularity != -1 and
			    optimizedDataGrid->GetGranularity() > nLastExploredGranularity + 1)
				dBestCost = PostOptimizeGranularity(initialDataGrid, optimizedDataGrid,
								    &odQuantileBuilders, nLastExploredGranularity);
		}

		// Nettoyage
		odQuantileBuilders.DeleteAll();
	}

	// Fin de suivi des taches
	TaskProgression::EndTask();

	// Nettoyage
	ResetProgressionIndicators();

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
		dataGridManager.SetSourceDataGrid(optimizedDataGrid);
		dataGridManager.ExportDataGrid(&dataGridMerger);

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
	if (attributeSubsetStatsHandler != NULL)
		attributeSubsetStatsHandler->HandleOptimizationStep(optimizedDataGrid, initialGranularizedDataGrid,
								    bIsLastSaving);
}

void KWDataGridOptimizer::SetAttributeSubsetStats(const KWAttributeSubsetStats* attributeSubsetStats)
{
	attributeSubsetStatsHandler = attributeSubsetStats;
}

const KWAttributeSubsetStats* KWDataGridOptimizer::GetAttributeSubsetStats()
{
	return attributeSubsetStatsHandler;
}

double KWDataGridOptimizer::OptimizeGranularizedDataGrid(const KWDataGrid* initialDataGrid,
							 KWDataGrid* optimizedDataGrid) const
{
	boolean bDisplayResults = false;
	double dBestCost;
	ALString sTmp;

	require(initialDataGrid != NULL);
	require(initialDataGrid->Check());
	require(IsOptimizationNeeded(initialDataGrid));
	require(optimizedDataGrid != NULL);

	// Initialisation du meilleur cout
	dBestCost = dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);
	if (bDisplayResults)
	{
		cout << "Debut OptimizeGranularizedDataGrid " << endl;
		cout << "Grille initiale" << endl;
		initialDataGrid->Write(cout);
		cout << "Grille optimale" << endl;
		optimizedDataGrid->Write(cout);
		cout << "Granularite courante \t " << initialDataGrid->GetGranularity() << endl;
		cout << " Cout grille optimale a la granularite courante " << dBestCost << endl;
	}

	// Optimisation univariee
	if (optimizationParameters.GetUnivariateInitialization() and
	    initialDataGrid->GetTargetValueNumber() > 0
	    // on ne fait pas d'univarie si un seul attribut
	    and initialDataGrid->GetAttributeNumber() > 1)
	{
		// Recherche d'une amelioration univariee avec prise en compte de la granularite
		if (not TaskProgression::IsInterruptionRequested())
			dBestCost = OptimizeWithBestUnivariatePartitionForCurrentGranularity(initialDataGrid,
											     optimizedDataGrid);

		// Recherche d'une amelioration par croisement des partitions univariees
		// Integre la granularite
		if (not TaskProgression::IsInterruptionRequested())
			dBestCost = OptimizeWithMultipleUnivariatePartitions(initialDataGrid, optimizedDataGrid);
	}

	// Affichage
	if (bDisplayResults)
	{
		cout << "Grille optimisee avant VNS" << endl;
		optimizedDataGrid->Write(cout);
	}

	// Optimisation a partir d'une grille initiale complete si algorithme glouton
	if (not TaskProgression::IsInterruptionRequested())
		dBestCost = IterativeVNSOptimizeDataGrid(initialDataGrid, optimizedDataGrid);

	// Tri des parties par attribut, pour preparer les affichages de resultats
	// ainsi que les resultats de preparation des donnees
	optimizedDataGrid->SortAttributeParts();

	// Affichage de la grille finale avec ses couts
	if (bDisplayResults)
	{
		// Grille optimisee
		cout << "Optimized grid\n";
		if (optimizedDataGrid->GetAttributeNumber() == 2)
			optimizedDataGrid->WriteCrossTableStats(cout, 0);
		dataGridCosts->WriteDataGridAllCosts(optimizedDataGrid, cout);
		cout << endl;

		// Grille finale
		cout << *optimizedDataGrid << endl;
	}
	// Grille finale
	if (bDisplayResults)
	{
		cout << "Granularite courante \t " << optimizedDataGrid->GetGranularity() << endl;
		cout << " Cout meilleure grille " << dBestCost << endl;
	}

	// Retour du meilleur cout de codage
	ensure(fabs(dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid) - dBestCost) < dEpsilon);
	return dBestCost;
}

double KWDataGridOptimizer::SlightOptimizeGranularizedDataGrid(const KWDataGrid* initialDataGrid,
							       KWDataGrid* optimizedDataGrid) const
{
	boolean bDisplayResults = false;
	KWDataGridMerger neighbourDataGrid;
	double dBestCost;
	double dCost;
	ALString sTmp;

	require(initialDataGrid != NULL);
	require(initialDataGrid->Check());
	require(IsOptimizationNeeded(initialDataGrid));
	require(not IsSupervisedDataGrid(initialDataGrid));
	require(initialDataGrid->GetGranularity() < ComputeMaxExploredGranularity(initialDataGrid));
	require(optimizedDataGrid != NULL);

	// Initialisation du meilleur cout
	dBestCost = dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);
	if (bDisplayResults)
	{
		cout << "Debut SlightOptimizeGranularizedDataGrid " << endl;
		cout << "Grille initiale" << endl;
		initialDataGrid->Write(cout);
		cout << "Grille optimale" << endl;
		optimizedDataGrid->Write(cout);
		cout << "Granularite courante \t " << initialDataGrid->GetGranularity() << endl;
		cout << " Cout grille optimale a la granularite courante " << dBestCost << endl;
	}

	// Parametrage des couts
	neighbourDataGrid.SetDataGridCosts(GetDataGridCosts());

	// Generation d'une solution dans un voisinage de la meilleure solution
	KWDataGridOptimizer::GetProfiler()->BeginMethod("Generate neighbour solution");
	KWDataGridOptimizer::GetProfiler()->WriteKeyString("Neighbourhood size", "1");
	GenerateNeighbourSolution(initialDataGrid, optimizedDataGrid, 1, &neighbourDataGrid);
	KWDataGridOptimizer::GetProfiler()->EndMethod("Generate neighbour solution");

	// Optimisation de cette solution
	KWDataGridOptimizer::GetProfiler()->BeginMethod("Slight optimize solution");
	dCost = OptimizeSolution(initialDataGrid, &neighbourDataGrid, false);
	KWDataGridOptimizer::GetProfiler()->EndMethod("Slight optimize solution");

	// Si amelioration: on la memorise
	if (dCost < dBestCost - dEpsilon)
	{
		dBestCost = dCost;
		SaveDataGrid(&neighbourDataGrid, optimizedDataGrid);

		// Gestion de la meilleure solution
		HandleOptimizationStep(optimizedDataGrid, initialDataGrid, false);
	}

	// Tri des parties par attribut, pour preparer les affichages de resultats
	// ainsi que les resultats de preparation des donnees
	optimizedDataGrid->SortAttributeParts();

	// Affichage de la grille finale avec ses couts
	if (bDisplayResults)
	{
		// Grille optimisee
		cout << "Optimized grid\n";
		if (optimizedDataGrid->GetAttributeNumber() == 2)
			optimizedDataGrid->WriteCrossTableStats(cout, 0);
		dataGridCosts->WriteDataGridAllCosts(optimizedDataGrid, cout);
		cout << endl;

		// Grille finale
		cout << *optimizedDataGrid << endl;
	}
	// Grille finale
	if (bDisplayResults)
	{
		cout << "Granularite courante \t " << optimizedDataGrid->GetGranularity() << endl;
		cout << " Cout meilleure grille " << dBestCost << endl;
	}

	// Retour du meilleur cout de codage
	ensure(fabs(dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid) - dBestCost) < dEpsilon);
	return dBestCost;
}

double KWDataGridOptimizer::PostOptimizeGranularity(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid,
						    const ObjectDictionary* odQuantileBuilders,
						    int nLastExploredGranularity) const
{
	debug(double dInitialOptimizedCost);
	double dPostOptimizedCost;
	int nCurrentGranularity;
	int nBestGranularity;
	int nGranularityPartileNumber;
	int nGranularisationIntervalIndex;
	int nPartitionCumulatedFrequency;
	int nGranularisationCumulatedFrequency;
	int nAttributeIndex;
	boolean bIncompatibleGranularity;
	boolean bLastExploredGranularity;
	KWDGAttribute* attribute;
	KWDGAttribute* initialAttribute;
	KWDGPart* part;
	KWQuantileGroupBuilder* quantileGroupBuilder;
	KWQuantileIntervalBuilder* quantileIntervalBuilder;
	ObjectArray oaInitialParts;
	int nCatchAllFirstModalityIndex;
	int nCatchAllModalityIndex;
	KWDGPart* catchAllPart;
	IntVector ivPartileNumber;
	IntVector ivBestPartileNumber;

	require(initialDataGrid != NULL);
	require(optimizedDataGrid != NULL);
	require(IsSupervisedDataGrid(initialDataGrid));
	require(odQuantileBuilders != NULL);
	require(nLastExploredGranularity > 0);
	require(optimizedDataGrid->GetGranularity() > nLastExploredGranularity + 1);

	// Initialisation
	bLastExploredGranularity = false;
	bIncompatibleGranularity = false;
	nCurrentGranularity = optimizedDataGrid->GetGranularity() - 1;
	nBestGranularity = optimizedDataGrid->GetGranularity();
	debug(dInitialOptimizedCost = GetDataGridCosts()->ComputeDataGridTotalCost(optimizedDataGrid));

	// Initialisation du nombre de partiles par attribut
	nAttributeIndex = 0;
	while (nAttributeIndex < optimizedDataGrid->GetAttributeNumber())
	{
		// Extraction de l'attribut courant
		attribute = optimizedDataGrid->GetAttributeAt(nAttributeIndex);

		ivBestPartileNumber.Add(attribute->GetGranularizedValueNumber());
		nAttributeIndex++;
	}

	// Boucle descendante sur les granularites jusqu'a rencontrer l'avant derniere granularite exploree ou une
	// granularite incompatible avec la partition
	while (not bLastExploredGranularity and not bIncompatibleGranularity)
	{
		nGranularityPartileNumber = (int)pow(2.0, nCurrentGranularity);

		// Initialisation de l'attribut etudie
		nAttributeIndex = 0;
		ivPartileNumber.SetSize(0);

		// Boucle sur les attributs tant que l'on n'a pas rencontre un attribut dont la granularisation est
		// incompatible avec la grille
		while (nAttributeIndex < optimizedDataGrid->GetAttributeNumber() and not bIncompatibleGranularity)
		{
			// Extraction de l'attribut courant
			attribute = optimizedDataGrid->GetAttributeAt(nAttributeIndex);

			// Reinitialisation du tableau des parties
			oaInitialParts.SetSize(0);

			// Cas ou le nombre de partiles theorique de cette granularite est superieur ou egal a la taille
			// de la partition de l'attribut courant
			if (nGranularityPartileNumber >= attribute->GetPartNumber())
			{
				// Cas d'un attribut continu
				if (attribute->GetAttributeType() == KWType::Continuous)
				{
					quantileIntervalBuilder =
					    cast(KWQuantileIntervalBuilder*,
						 odQuantileBuilders->Lookup(attribute->GetAttributeName()));
					// Memorisation du nombre de partiles (theorique pour attribut numerique,
					// effectif pour attribut categoriel)
					ivPartileNumber.Add(nGranularityPartileNumber);
					nGranularityPartileNumber =
					    quantileIntervalBuilder->ComputeQuantiles(nGranularityPartileNumber);

					// Cas ou le nombre reel de cette granularisation est superieur ou egal a la
					// taille de la partition
					if (nGranularityPartileNumber >= attribute->GetPartNumber())
					{
						part = attribute->GetHeadPart();
						nPartitionCumulatedFrequency = part->GetPartFrequency();

						// Initialisation
						nGranularisationIntervalIndex = 0;
						nGranularisationCumulatedFrequency =
						    quantileIntervalBuilder->GetIntervalLastInstanceIndexAt(
							nGranularisationIntervalIndex) +
						    1;

						// Parcours des intervalles de la partition optimale et des intervalles
						// de la granularisation tant qu'il y a compatibilite
						while (not bIncompatibleGranularity and part != NULL)
						{
							while (nPartitionCumulatedFrequency >
							       nGranularisationCumulatedFrequency)
							{
								nGranularisationIntervalIndex++;
								nGranularisationCumulatedFrequency =
								    quantileIntervalBuilder
									->GetIntervalLastInstanceIndexAt(
									    nGranularisationIntervalIndex) +
								    1;
							}

							if (nPartitionCumulatedFrequency <
							    quantileIntervalBuilder->GetIntervalLastInstanceIndexAt(
								nGranularisationIntervalIndex) +
								1)
								bIncompatibleGranularity = true;
							// Partie suivante
							attribute->GetNextPart(part);
							if (part != NULL)
								nPartitionCumulatedFrequency +=
								    part->GetPartFrequency();
						}
					}
					else
						bIncompatibleGranularity = true;
				}
				// Cas d'un attribut categoriel
				else
				{
					quantileGroupBuilder =
					    cast(KWQuantileGroupBuilder*,
						 odQuantileBuilders->Lookup(attribute->GetAttributeName()));
					nGranularityPartileNumber =
					    quantileGroupBuilder->ComputeQuantiles(nGranularityPartileNumber);

					// Memorisation du nombre de partiles (theorique pour attribut numerique,
					// effectif pour attribut categoriel)
					ivPartileNumber.Add(nGranularityPartileNumber);

					// Cas ou le nombre reel de cette granularisation est superieur ou egal a la
					// taille de la partition
					if (nGranularityPartileNumber >= attribute->GetPartNumber())
					{
						attribute->BuildIndexingStructure();

						// Export des parties de l'attribut initial
						initialAttribute =
						    initialDataGrid->SearchAttribute(attribute->GetAttributeName());
						initialAttribute->ExportParts(&oaInitialParts);
						// Extraction de l'index de la 1ere modalite du fourre-tout. Le
						// fourre-tout est le dernier groupe du quantileBuilder
						nCatchAllFirstModalityIndex =
						    quantileGroupBuilder->GetGroupFirstValueIndexAt(
							nGranularityPartileNumber - 1);

						// Extraction de la partie a ajouter dans le groupe
						catchAllPart =
						    cast(KWDGPart*, oaInitialParts.GetAt(nCatchAllFirstModalityIndex));

						// Parcours des autres modalites du fourre-tout pour verifier qu'elles
						// sont dans le meme groupe que la premiere modalite du fourre-tout
						nCatchAllModalityIndex = nCatchAllFirstModalityIndex + 1;
						while (not bIncompatibleGranularity and
						       nCatchAllModalityIndex <=
							   quantileGroupBuilder->GetGroupLastValueIndexAt(
							       nGranularityPartileNumber - 1))
						{
							part = cast(KWDGPart*,
								    oaInitialParts.GetAt(nCatchAllModalityIndex));
							if (part != catchAllPart)
								bIncompatibleGranularity = true;
							nCatchAllModalityIndex++;
						}
						attribute->DeleteIndexingStructure();
					}
					else
						bIncompatibleGranularity = true;
				}
			}
			else
				bIncompatibleGranularity = true;

			nAttributeIndex++;
		}

		// Cas d'une granularite compatible : memorisation de la granularite et du nombre de partiles par
		// attribut de la grille
		if (not bIncompatibleGranularity)
		{
			nBestGranularity = nCurrentGranularity;
			ivBestPartileNumber.CopyFrom(&ivPartileNumber);
		}

		// Prochaine granularite
		nCurrentGranularity--;
		bLastExploredGranularity = (nCurrentGranularity == nLastExploredGranularity);
	}

	// Mise a jour de la granularite de la grille et du nombre de partiles pour chaque attribut
	optimizedDataGrid->SetGranularity(nBestGranularity);
	nAttributeIndex = 0;
	while (nAttributeIndex < optimizedDataGrid->GetAttributeNumber())
	{
		// Extraction de l'attribut courant
		attribute = optimizedDataGrid->GetAttributeAt(nAttributeIndex);

		attribute->SetGranularizedValueNumber(ivBestPartileNumber.GetAt(nAttributeIndex));
		nAttributeIndex++;
	}

	// On renvoie le cout le grillle post-optimisee
	dPostOptimizedCost = GetDataGridCosts()->ComputeDataGridTotalCost(optimizedDataGrid);
	debug(ensure(dPostOptimizedCost <= dInitialOptimizedCost));
	return dPostOptimizedCost;
}

double KWDataGridOptimizer::InitializeWithTerminalDataGrid(const KWDataGrid* initialDataGrid,
							   KWDataGrid* optimizedDataGrid) const
{
	double dBestCost;
	KWDataGridManager dataGridManager;
	KWDataGrid terminalDataGrid;

	// Initialisations
	dataGridManager.SetSourceDataGrid(initialDataGrid);

	// Construction d'une grille terminale pour la solution initiale
	dataGridManager.ExportTerminalDataGrid(&terminalDataGrid);
	dBestCost = dataGridCosts->GetTotalDefaultCost();

	// Memorisation de la meilleure solution initiale
	SaveDataGrid(&terminalDataGrid, optimizedDataGrid);

	// Tri des parties par attribut, pour preparer les affichages de resultats
	// ainsi que les resultats de preparation des donnees
	optimizedDataGrid->SortAttributeParts();

	// Affichage du resulat (ici: grilles initiale et optimisee sont confondues)
	DisplayOptimizationDetails(optimizedDataGrid, false);
	DisplayOptimizationDetails(optimizedDataGrid, true);

	// Retour du cout
	ensure(fabs(dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid) - dBestCost) < dEpsilon);
	return dBestCost;
}

double
KWDataGridOptimizer::OptimizeWithBestUnivariatePartitionForCurrentGranularity(const KWDataGrid* initialDataGrid,
									      KWDataGrid* optimizedDataGrid) const
{
	boolean bDisplayResults = false;
	KWAttributeStats* attributeStats;
	KWDataGridManager dataGridManager;
	KWDataGrid univariateDataGrid;
	KWDGAttribute* initialAttribute;
	KWDGAttribute* targetAttribute;
	double dBestCost;
	double dCost;
	int nAttribute;
	boolean bEvaluated;
	boolean bImproved;

	require(initialDataGrid != NULL);
	require(optimizedDataGrid != NULL);
	require(initialDataGrid->GetTargetValueNumber() > 0);

	// Initialisations
	dBestCost = dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	if (bDisplayResults)
		cout << "OptimizeWithBestUnivariatePartition: cout initial " << dBestCost << endl;

	// Retour si pas de statistiques univariees disponibles
	if (classStats == NULL)
		return dBestCost;

	// Recherche des grilles univariees deduites des statistiques univariees
	bImproved = false;

	// Parcours des attributs
	for (nAttribute = 0; nAttribute < initialDataGrid->GetAttributeNumber(); nAttribute++)
	{
		initialAttribute = initialDataGrid->GetAttributeAt(nAttribute);

		// Initialisation de la grille univariee (uniquement si plus de une partie dans la partition)
		univariateDataGrid.DeleteAll();
		bEvaluated = false;

		// Extraction de la partition stockee dans l'attributeStats
		attributeStats = classStats->LookupAttributeStats(initialAttribute->GetAttributeName());
		bEvaluated = attributeStats->GetLevel() > 0;

		// Cas d'un attribut informatif
		if (bEvaluated)
		{
			// Cas ou la granularite de la meilleure partition univarie de l'attribut ne correspond pas
			// a la granularite courante de la grille
			if (attributeStats->GetPreparedDataGridStats()->GetGranularity() !=
			    initialDataGrid->GetGranularity())
			{
				dataGridManager.BuildUnivariateDataGridFromGranularizedPartition(
				    &univariateDataGrid, nAttribute, classStats);
				bEvaluated = univariateDataGrid.GetAttributeAt(0)->GetPartNumber() > 1;
			}

			// Cas ou la granularite de la meilleure partition univariee de l'attribut correspond
			// a la granularite courante de la grille
			else
			{
				dataGridManager.BuildUnivariateDataGridFromAttributeStats(&univariateDataGrid,
											  attributeStats);

				// Transfert du parametrage du fourre-tout
				targetAttribute = univariateDataGrid.GetAttributeAt(0);
				targetAttribute->InitializeCatchAllValueSet(initialAttribute->GetCatchAllValueSet());
			}
		}

		// Evaluation si la grille univariee a ete construite
		if (bEvaluated)
		{
			// Evaluation de la grille
			dCost = dataGridCosts->ComputeDataGridTotalCost(&univariateDataGrid);
			if (bDisplayResults)
				cout << " APRES Cout de la grille univariee " << dCost << endl;

			// Affichage du resulat (ici: grilles initiale et optimisee sont confondues)
			DisplayOptimizationDetails(&univariateDataGrid, false);
			DisplayOptimizationDetails(&univariateDataGrid, true);

			// Memorisation de la meilleure solution
			if (dCost < dBestCost - dEpsilon)
			{
				dBestCost = dCost;
				SaveDataGrid(&univariateDataGrid, optimizedDataGrid);
				bImproved = true;
			}
		}
	}

	// Retour du cout
	ensure(fabs(dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid) - dBestCost) == 0 or bImproved);
	ensure(fabs(dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid) - dBestCost) < dEpsilon);
	return dBestCost;
}

double KWDataGridOptimizer::OptimizeWithMultipleUnivariatePartitions(const KWDataGrid* initialDataGrid,
								     KWDataGrid* optimizedDataGrid) const
{
	boolean bDisplayResults = false;
	double dBestCost;
	double dCost;
	KWDataGridManager dataGridManager;
	KWDataGridMerger multivariateDataGrid;
	KWDataGridPostOptimizer dataGridPostOptimizer;
	boolean bOk;

	require(initialDataGrid != NULL);
	require(optimizedDataGrid != NULL);
	require(initialDataGrid->GetTargetValueNumber() > 0);

	// Initialisations
	dBestCost = dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	multivariateDataGrid.SetDataGridCosts(dataGridCosts);
	dataGridPostOptimizer.SetDataGridCosts(dataGridCosts);
	if (bDisplayResults)
	{
		cout << "OptimizeWithMultipleUnivariatePartitions: dBestCost initial " << dBestCost << endl;
		cout << " Grille initiale " << endl;
		cout << *optimizedDataGrid;
	}

	// Retour si pas assez de statistiques univariees disponibles
	if (classStats == NULL or classStats->GetInformativeAttributeNumber() <= 1)
		return dBestCost;

	// Construction d'une grille par croisement des partition univariee
	// Traitement avec calcul des partitions univariees pour cette granularite
	bOk = dataGridManager.BuildDataGridFromUnivariateProduct(&multivariateDataGrid, classStats);

	if (not bOk)
		return dBestCost;

	// Affichage du cout initial
	DisplayOptimizationDetails(&multivariateDataGrid, false);

	// Cout initial si aucune optimisation
	dCost = DBL_MAX;
	if (not optimizationParameters.GetOptimize() and not optimizationParameters.GetPreOptimize() and
	    not optimizationParameters.GetPostOptimize())
		dCost = dataGridCosts->ComputeDataGridTotalCost(&multivariateDataGrid);

	if (bDisplayResults)
	{
		dCost = dataGridCosts->ComputeDataGridTotalCost(&multivariateDataGrid);
		cout << "OptimizeWithMultipleUnivariatePartitions: dBestCost multivarie initial " << dCost << endl;
		cout << " Grille initiale " << endl;
		cout << multivariateDataGrid;
	}

	// Pre-optimisation de la grille
	if (optimizationParameters.GetPreOptimize())
		dCost = dataGridPostOptimizer.PostOptimizeDataGrid(initialDataGrid, &multivariateDataGrid, false);

	// Optimisation par fusion des groupes
	if (optimizationParameters.GetOptimize())
		dCost = multivariateDataGrid.Merge();

	// Post-optimisation de la grille
	if (optimizationParameters.GetPostOptimize())
		dCost = dataGridPostOptimizer.PostOptimizeDataGrid(initialDataGrid, &multivariateDataGrid, true);

	if (bDisplayResults)
	{
		cout << "OptimizeWithMultipleUnivariatePartitions: dBestCost multivarie optimise " << dCost << endl;
		cout << " Grille multivariee optimisee " << endl;
		cout << multivariateDataGrid;
	}

	// Affichage du cout final
	DisplayOptimizationDetails(&multivariateDataGrid, true);

	// Memorisation de la meilleure solution
	if (dCost < dBestCost - dEpsilon)
	{
		dBestCost = dCost;
		SaveDataGrid(&multivariateDataGrid, optimizedDataGrid);
	}

	// Retour du cout
	ensure(fabs(dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid) - dBestCost) < dEpsilon);
	return dBestCost;
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
	// CH IV Begin
	double dMergedCost;
	double dBestMergedCost;
	// CH IV End

	require(initialDataGrid != NULL);
	require(optimizedDataGrid != NULL);

	// On prend au minimum un niveau max de 1
	nMaxLevel = optimizationParameters.GetOptimizationLevel();
	if (nMaxLevel <= 0)
		nMaxLevel = 1;

	// Parametrage d'un niveau d'optimisation anytime si une limite de temps est indiquee
	// On le fait uniquement pour la derniere granularite, pour que le mode anytime ne
	// ne reste pas bloque des la premiere granularite intermediaire
	if (optimizationParameters.GetOptimizationTime() > 0 and IsLastGranularity(initialDataGrid))
		nMaxLevel = 20;

	// Initialisations
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	dBestCost = dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);
	dBestMergedCost = dBestCost;

	// Appel de VNS en augmentant le nombre de voisinnages d'un facteur 2 chaque fois
	for (nLevel = 0; nLevel < nMaxLevel; nLevel++)
	{
		// Calcul du nombre de voisinnage a considerer
		nNeighbourhoodLevelNumber = int(pow(2.0, nLevel));
		if (bDisplayResults)
			cout << "IterativeVNSOptimizeDataGrid: Level\t" << nLevel << endl;

		// Recopie de la meilleure solution dans une solution de travail courante
		dataGridManager.SetSourceDataGrid(initialDataGrid);
		dataGridManager.CopyDataGrid(optimizedDataGrid, &currentDataGrid);

		// Parametrage du profiling
		KWDataGridOptimizer::GetProfiler()->BeginMethod("VNS optimize");
		KWDataGridOptimizer::GetProfiler()->WriteKeyString(
		    "Is VarPart", BooleanToString(currentDataGrid.IsVarPartDataGrid()));
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("Level", IntToString(nLevel));
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("Neighbourhood level number",
								   IntToString(nNeighbourhoodLevelNumber));

		// Optimisation a partir de la nouvelle solution
		// CH IV Begin
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
			dCost = VNSOptimizeVarPartDataGrid(initialDataGrid, nNeighbourhoodLevelNumber, &currentDataGrid,
							   dMergedCost);
			if (dMergedCost < dBestMergedCost - dEpsilon)
			{
				dBestCost = dCost;
				dBestMergedCost = dMergedCost;
				SaveDataGrid(&currentDataGrid, optimizedDataGrid);
			}
		}
		KWDataGridOptimizer::GetProfiler()->EndMethod("VNS optimize");
		// CH IV End

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
	dataGridManager.SetSourceDataGrid(initialDataGrid);
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
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("Neighbourhood size",
								   DoubleToString(dNeighbourhoodSize));
		GenerateNeighbourSolution(initialDataGrid, optimizedDataGrid, dNeighbourhoodSize, &neighbourDataGrid);
		KWDataGridOptimizer::GetProfiler()->EndMethod("Generate neighbour solution");

		// Parametrage du profiling pour l'optimisation
		KWDataGridOptimizer::GetProfiler()->BeginMethod("Optimize solution");
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("VNS level",
								   sTmp + IntToString(nVNSNeighbourhoodLevelIndex) +
								       "/" + IntToString(nVNSNeighbourhoodLevelNumber));
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("VNS neighbourhood size",
								   DoubleToString(dVNSNeighbourhoodSize));

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

// CH IV Begin
double KWDataGridOptimizer::VNSDataGridPostOptimizeVarPart(const KWDataGrid* initialDataGrid,
							   KWDataGridMerger* neighbourDataGrid,
							   double& dNeighbourDataGridCost, KWDataGrid* mergedDataGrid,
							   KWDataGrid* partitionedReferencePostMergedDataGrid) const
{
	boolean bDisplayResults = false;
	double dMergedCost;
	double dFusionDeltaCost;
	KWDataGridManager dataGridManager;
	ALString sLabel;

	// On ne reverifie pas les preconditions de la methode publique
	require(initialDataGrid != NULL);
	require(neighbourDataGrid != NULL);
	require(neighbourDataGrid->GetDataGridCosts() == dataGridCosts);
	require(IsLastGranularity(neighbourDataGrid));
	require(fabs(dNeighbourDataGridCost - dataGridCosts->ComputeDataGridTotalCost(neighbourDataGrid)) < dEpsilon);

	// Initialisation du cout
	dMergedCost = dNeighbourDataGridCost;

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

		// Initialisation du datagrid manager
		dataGridManager.SetSourceDataGrid(neighbourDataGrid);

		// Creation d'une nouvelle grille avec nouvelle description des PV fusionnees
		dFusionDeltaCost =
		    dataGridManager.ExportDataGridWithVarPartMergeOptimization(mergedDataGrid, dataGridCosts);
		assert(not mergedDataGrid->GetVarPartsShared());

		// Calcul et verification du cout
		dMergedCost = dNeighbourDataGridCost + dFusionDeltaCost;

		// Cas ou le cout de la grille avec PV voisines fusionnees est plus eleve que le cout avant fusion
		if (dMergedCost > dNeighbourDataGridCost * (1 + dEpsilon) and bDisplayResults)
		{
			sLabel = "PROBLEME: degradation du cout lors de la fusion des parties de variables "
				 "contigues:\tcout fusionne\t";
			sLabel += DoubleToString(dMergedCost);
			sLabel += "\tcout\t";
			sLabel += DoubleToString(dMergedCost - dNeighbourDataGridCost);
			AddWarning(sLabel);
			cout << sLabel << "\n";
			cout << "Grille avant fusion\n";
			neighbourDataGrid->Write(cout);
			cout << "Grille apres fusion\n";
			mergedDataGrid->Write(cout);
		}

		// Post-optimisation de l'attribut VarPart uniquement dans le cas d'une optimisation approfondie
		// (i.e. derniere granularite)
		if (mergedDataGrid->GetInformativeAttributeNumber() > 1 and
		    optimizationParameters.GetVarPartPostOptimize())
		{
			CCVarPartDataGridPostOptimizer varPartDataGridPostOptimizer;
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
				cout << "VNSDataGridPostOptimizeVarPart: grille a post-optimiser" << endl;
				mergedDataGrid->Write(cout);
				cout << "Debut PostOptimisation VarPart" << endl;
				cout << "VNSDataGridPostOptimizeVarPart: grille initiale du DataGridOptimizer" << endl;
				GetInitialVarPartDataGrid()->Write(cout);
				cout << "VNSDataGridPostOptimizeVarPart: grille initiale utilisee pour l'export"
				     << endl;
				initialDataGrid->Write(cout);
				cout << flush;
			}

			// Boucle : on continue a post-optimiser tant qu'au moins un deplacement de VarPart
			// permet d'ameliorer le critere
			bImprovement = true;
			int nImprovementNumber = 0;
			while (bImprovement)
			{
				nImprovementNumber++;

				// Construction d'une grille de reference avec des clusters contenant une seule
				// PV a partir des PV apres fusion Parametrage par la grille initiale de
				// l'optimiseur
				dataGridManager.SetSourceDataGrid(GetInitialVarPartDataGrid());
				assert(mergedDataGrid->Check());
				dataGridManager.ExportDataGridWithSingletonVarParts(
				    mergedDataGrid, partitionedReferencePostMergedDataGrid, false);
				ivGroups.SetSize(
				    partitionedReferencePostMergedDataGrid->GetVarPartAttribute()->GetPartNumber());
				assert(partitionedReferencePostMergedDataGrid->Check());
				assert(partitionedReferencePostMergedDataGrid->GetVarPartsShared());
				assert(partitionedReferencePostMergedDataGrid->GetInnerAttributes() ==
				       mergedDataGrid->GetInnerAttributes());

				// Affichage de la grille courante
				if (bDisplayResults)
				{
					cout << "nImprovementNumber\t" << nImprovementNumber << endl;
					cout << "VNSDataGridPostOptimizeVarPart: grille post-fusionnee cout\t"
					     << dMergedCost << endl;
					mergedDataGrid->Write(cout);
					cout << "VNSDataGridPostOptimizeVarPart : grille de reference" << endl;
					partitionedReferencePostMergedDataGrid->Write(cout);
					cout << flush;
				}

				// Parametrage du profiling
				KWDataGridOptimizer::GetProfiler()->BeginMethod("Post-optimization IV");
				KWDataGridOptimizer::GetProfiler()->WriteKeyString("Improvement number",
										   IntToString(nImprovementNumber));

				// Exploration des deplacements pour tous les attributs
				bImprovement = varPartDataGridPostOptimizer.PostOptimizeLightVarPartDataGrid(
				    partitionedReferencePostMergedDataGrid, mergedDataGrid, &ivGroups);
				KWDataGridOptimizer::GetProfiler()->EndMethod("Post-optimization IV");

				if (bImprovement)
				{
					// Mise a jour de la grille pour l'optimisation de cet attribut
					nGroupNumber = mergedDataGrid->GetVarPartAttribute()->GetPartNumber();
					dataGridManager.SetSourceDataGrid(partitionedReferencePostMergedDataGrid);
					dataGridManager.UpdateVarPartDataGridFromVarPartGroups(mergedDataGrid,
											       &ivGroups, nGroupNumber);
					if (bDisplayResults)
					{
						dNewMergedCost =
						    dataGridCosts->ComputeDataGridTotalCost(mergedDataGrid);
						cout << "VNSDataGridPostOptimizeVarPart: grille mise a jour best "
							"deplacement\tCout\t"
						     << dNewMergedCost << endl;
						mergedDataGrid->Write(cout);
						assert(mergedDataGrid->Check());
					}

					// Mise a jour de la grille fusionnee courante par la grille obtenue par
					// fusion de la grille comportant les deplacements
					dataGridManager.SetSourceDataGrid(mergedDataGrid);
					dVarPartFusionDeltaCost =
					    dataGridManager.ExportDataGridWithVarPartMergeOptimization(
						&mergedMergedDataGrid, dataGridCosts);
					dMergedMergedCost =
					    dataGridCosts->ComputeDataGridTotalCost(&mergedMergedDataGrid);
					if (bDisplayResults)
					{
						cout << "VNSDataGridPostOptimizeVarPart: grille best deplacement "
							"et fusionnee\tCout\t"
						     << dMergedMergedCost << endl;
						mergedMergedDataGrid.Write(cout);
					}

					// Cas ou la post-optimisation permet d'ameliorer le cout
					if (dMergedMergedCost < dMergedCost)
					{
						dMergedCost = dMergedMergedCost;

						// Export de la grille avec les clusters post-optimises et les
						// PV de la grille initiale. Grille sans fusion des PV voisines.
						// C'est cette grille antecedent de la meilleure grille qui sera
						// memorisee
						neighbourDataGrid->DeleteAll();
						dataGridManager.SetSourceDataGrid(initialDataGrid);
						dataGridManager.ExportDataGridWithReferenceVarPartClusters(
						    mergedDataGrid, neighbourDataGrid);
						if (bDisplayResults)
						{
							cout << "VNSDataGridPostOptimizeVarPart: grille best "
								"deplacement et initiale"
							     << endl;
							neighbourDataGrid->Write(cout);
						}
						dNeighbourDataGridCost =
						    dataGridCosts->ComputeDataGridTotalCost(neighbourDataGrid);
					}

					// On remplace mergedDataGrid par la nouvelle grille integrant les
					// deplacements de post-optimisation puis fusionnee 23/11/22 : pourquoi
					// effectuer ce remplacement systematiquement et pas uniquement dans le
					// cas ou la poste optimisation permet d'ameliorer le cout ? on est
					// quand meme dans le cas if(bImprovement) donc il y a amelioration du
					// fait de la post-optimisation
					mergedDataGrid->DeleteAll();
					dataGridManager.SetSourceDataGrid(&mergedMergedDataGrid);
					dataGridManager.ExportDataGrid(mergedDataGrid);
					mergedDataGrid->SetVarPartsShared(false);
					mergedMergedDataGrid.SetVarPartsShared(true);
					mergedMergedDataGrid.DeleteAll();
					assert(mergedDataGrid->Check());
				}
				// Nettoyage -> peut etre a supprimer pour mutualiser avec utilisation de cette
				// grille pour HangleOptimizationStep
				partitionedReferencePostMergedDataGrid->DeleteAll();
				ivGroups.SetSize(0);
			}
			if (bDisplayResults)
				cout << "Fin PostOptimisation VarPart" << endl;
		}
	}
	ensure(fabs(dNeighbourDataGridCost - dataGridCosts->ComputeDataGridTotalCost(neighbourDataGrid)) < dEpsilon);
	return dMergedCost;
}

double KWDataGridOptimizer::VNSOptimizeVarPartDataGrid(const KWDataGrid* initialDataGrid, int nNeighbourhoodLevelNumber,
						       KWDataGrid* optimizedDataGrid,
						       double& dBestMergedDataGridCost) const
{
	double dBestCost;
	double dCost;
	double dMergedCost;
	double dBestMergedCost;
	KWDataGrid mergedDataGrid;
	KWDataGrid partitionedReferencePostMergedDataGrid;
	KWDataGridManager dataGridManager;
	KWDataGridMerger neighbourDataGrid;
	int nIndex;
	double dMinNeighbourhoodSize;
	double dDecreaseFactor;
	double dNeighbourhoodSize;
	ALString sTmp;

	// On ne reverifie pas les precondition de la methode publique
	require(nNeighbourhoodLevelNumber >= 0);

	// Initialisations
	dataGridManager.SetSourceDataGrid(initialDataGrid);
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
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("Neighbourhood size",
								   DoubleToString(dNeighbourhoodSize));
		GenerateNeighbourSolution(initialDataGrid, optimizedDataGrid, dNeighbourhoodSize, &neighbourDataGrid);
		KWDataGridOptimizer::GetProfiler()->EndMethod("Generate neighbour solution");

		// Parametrage du profiling pour l'optimisation
		KWDataGridOptimizer::GetProfiler()->BeginMethod("Optimize solution");
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("VNS level",
								   sTmp + IntToString(nVNSNeighbourhoodLevelIndex) +
								       "/" + IntToString(nVNSNeighbourhoodLevelNumber));
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("VNS neighbourhood size",
								   DoubleToString(dVNSNeighbourhoodSize));

		// Optimisation de cette solution
		dCost = OptimizeSolution(initialDataGrid, &neighbourDataGrid, true);
		KWDataGridOptimizer::GetProfiler()->EndMethod("Optimize solution");

		// Post-optimisation des parties de variables de la grille
		// A terme, a deplacer dans OptimizeSolution
		dMergedCost = dCost;
		if (initialDataGrid->IsVarPartDataGrid())
			dMergedCost =
			    VNSDataGridPostOptimizeVarPart(initialDataGrid, &neighbourDataGrid, dCost, &mergedDataGrid,
							   &partitionedReferencePostMergedDataGrid);

		// Si amelioration: on la memorise
		if (dMergedCost < dBestMergedCost - dEpsilon)
		{
			dBestCost = dCost;
			dBestMergedCost = dMergedCost;

			// Sauvegarde de la meilleure solution
			// On sauvegarde l'antecedent de la meilleure grille post mergee
			// On ne sauvegarde donc pas la meilleure grille post mergee car cela necessiterait de
			// modifier la grille initiale qui devrait etre en coherence avec cette grille post mergee.
			// La grille initiale est conservee pour une granularite donnee La grille de reference
			// est necessaire pour HandleOptimizationStep
			SaveDataGrid(&neighbourDataGrid, optimizedDataGrid);

			// Gestion de la meilleure solution
			if (mergedDataGrid.GetInformativeAttributeNumber() == 0)
				HandleOptimizationStep(optimizedDataGrid, initialDataGrid, false);
			else
			{
				dataGridManager.SetSourceDataGrid(GetInitialVarPartDataGrid());
				dataGridManager.ExportDataGridWithSingletonVarParts(
				    &mergedDataGrid, &partitionedReferencePostMergedDataGrid, true);
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

		// Test de fin de tache
		if (TaskProgression::IsInterruptionRequested())
			break;

		// Test si depassement de temps
		if (IsOptimizationTimeElapsed())
			break;
	}

	// Memorisation du meilleur cout parmi les grilles post-fusionnees
	dBestMergedDataGridCost = dBestMergedCost;
	ensure(fabs(dBestCost - dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid)) < dEpsilon);
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

	// Suivi de la progression
	nVNSIteration++;
	DisplayProgression(initialDataGrid);

	// Initialisations
	dataGridPostOptimizer.SetDataGridCosts(dataGridCosts);

	// Affichage du cout initial
	DisplayOptimizationDetails(dataGridMerger, false);

	// Cout initial si aucune optimisation
	dCost = DBL_MAX;
	if (not optimizationParameters.GetOptimize() and not optimizationParameters.GetPreOptimize() and
	    not optimizationParameters.GetPostOptimize())
		dCost = dataGridCosts->ComputeDataGridTotalCost(dataGridMerger);
	if (bDisplay)
		cout << "Affichage de l'evolution des couts" << dCost << "\n";

	// Pre-optimisation de la grille
	if (optimizationParameters.GetPreOptimize() and not TaskProgression::IsInterruptionRequested() and
	    initialDataGrid->GetAttributeNumber() > 1)
	{
		KWDataGridOptimizer::GetProfiler()->BeginMethod("Pre-optimization");
		dCost = dataGridPostOptimizer.PostOptimizeDataGrid(initialDataGrid, dataGridMerger, false);
		if (bDisplay)
			cout << dCost << "\n";
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
		KWDataGridOptimizer::GetProfiler()->EndMethod("Greedy merge optimization" + sSuffix);
	}

	// Post-optimisation de la grille selon le niveau de post-optimisation
	if (optimizationParameters.GetPostOptimize() and not TaskProgression::IsInterruptionRequested() and
	    initialDataGrid->GetAttributeNumber() > 1)
	{
		KWDataGridOptimizer::GetProfiler()->BeginMethod("Post-optimization");
		dCost =
		    dataGridPostOptimizer.PostOptimizeDataGrid(initialDataGrid, dataGridMerger, bDeepPostOptimization);
		KWDataGridOptimizer::GetProfiler()->EndMethod("Post-optimization");
		if (bDisplay)
			cout << dCost << "\n\n";
	}

	// Recalcul du cout si la tache est interrompue, pour sortir avec un cout coherent
	if (TaskProgression::IsInterruptionRequested())
		dCost = dataGridCosts->ComputeDataGridTotalCost(dataGridMerger);

	// Affichage du cout final
	DisplayOptimizationDetails(dataGridMerger, true);

	// Retour du cout
	ensure(fabs(dataGridCosts->ComputeDataGridTotalCost(dataGridMerger) - dCost) < dEpsilon);
	return dCost;
}

void KWDataGridOptimizer::GenerateNeighbourSolution(const KWDataGrid* initialDataGrid,
						    const KWDataGrid* optimizedDataGrid, double dNoiseRate,
						    KWDataGridMerger* neighbourDataGridMerger) const
{
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

	// CH IV Refactoring : DDDDD
	// Test du remplacement de la methode actuelle, par son proto
	boolean bNewPROTO = true;
	static int nCount = 0;
	nCount++;
	if (initialDataGrid->IsVarPartDataGrid() and bNewPROTO and nCount == 1)
	{
		// Appel de PROTOGenerateNeighbourSolution, permettant de tester les nouvelle methodes sans les activer vraiment
		//PROTOGenerateNeighbourSolution(initialDataGrid, optimizedDataGrid, dNoiseRate, neighbourDataGridMerger);

		// CH IV Surtokenisation
		// Code a reprendre pour construire PROTOGenerateNeighbourSolution
		KWDataGrid* surtokenizedDataGrid;
		int nTargetTokenNumber = 100;
		boolean bDisplayResults = false;
		int nInitialSeed;

		// Correction du nombre de tokens
		nTargetTokenNumber = min(nTargetTokenNumber, initialDataGrid->GetGridFrequency());

		// Memorisation de la graine initiale
		nInitialSeed = GetRandomSeed();

		dataGridManager.SetSourceDataGrid(GetInitialVarPartDataGrid());

		surtokenizedDataGrid = new KWDataGrid;

		dataGridManager.ExportDataGridWithRandomizedInnerAttributes(
		    optimizedDataGrid, GetInitialVarPartDataGrid()->GetInnerAttributes(), surtokenizedDataGrid,
		    nTargetTokenNumber);

		if (bDisplayResults)
		{
			cout << "nTargetTokenNumber\t" << nTargetTokenNumber << endl;
			surtokenizedDataGrid->Write(cout);
		}

		surtokenizedDataGrid->DeleteAll();
		delete surtokenizedDataGrid;

		nTargetTokenNumber = 5;
		surtokenizedDataGrid = new KWDataGrid;

		dataGridManager.ExportDataGridWithRandomizedInnerAttributes(
		    optimizedDataGrid, GetInitialVarPartDataGrid()->GetInnerAttributes(), surtokenizedDataGrid,
		    nTargetTokenNumber);

		if (bDisplayResults)
		{
			cout << "nTargetTokenNumber\t" << nTargetTokenNumber << endl;
			surtokenizedDataGrid->Write(cout);
		}

		surtokenizedDataGrid->DeleteAll();
		delete surtokenizedDataGrid;

		// Restitution de la graine initiale
		SetRandomSeed(nInitialSeed);
	}

	// Debut de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel(sTmp + "New initial solution (" + DoubleToString(dNoiseRate) + ")");

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
	// CH IV Surtokenisation: dans la cas VarPart, considerer tous les attributs comme obligatoires
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

	// Export d'un sous-ensemble d'attributs obligatoires (les attribut informatifs) en fonction du niveau de bruit
	// Les attributs obligatoires sont les attributs de la grille optimisee que l'on va conserver
	// Quand la grille initiale contient plus d'attributs que la grille optimisee (on ne peut pas toujours prendre
	// tous les attributs pour des questions de complexite algorithmique), on n'en conserve que certains et on
	// complete avec de nouveaux attributs absents de la grille optimisee Quand le NoiseRate est eleve (=1) alors
	// aucun des attributs n'est conserve avec ce statut d'obligatoire
	nMandatoryAttributeNumber = (int)ceil((1 - dNoiseRate) * optimizedDataGrid->GetAttributeNumber());
	dataGridManager.SetSourceDataGrid(optimizedDataGrid);
	dataGridManager.ExportRandomAttributes(&mandatoryDataGrid, nMandatoryAttributeNumber);

	// Exports d'attributs supplementaires
	// Pour un dNoiseRate de 1, tous les attributs sont ajoutes a concurrence de nMaxAttributeNumber
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	neighbourDataGridMerger->DeleteAll();
	dataGridManager.AddRandomAttributes(neighbourDataGridMerger, &mandatoryDataGrid, nAttributeNumber);

	// Export des parties
	dataGridManager.AddRandomParts(neighbourDataGridMerger, optimizedDataGrid, nRequestedContinuousPartNumber,
				       nRequestedSymbolPartNumber, 1.0);
	TaskProgression::DisplayProgression(25);

	// Export des cellules
	dataGridManager.ExportCells(neighbourDataGridMerger);

	// Fin de tache
	TaskProgression::EndTask();
}

void KWDataGridOptimizer::PROTOGenerateNeighbourSolution(const KWDataGrid* initialDataGrid,
							 const KWDataGrid* optimizedDataGrid, double dNoiseRate,
							 KWDataGridMerger* neighbourDataGridMerger) const
{
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

	// Debut de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel(sTmp + "New surtokenized initial solution (" + DoubleToString(dNoiseRate) +
					  ")");

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
	// -> dans le cas VarPart, considerer tous les attributs comme obligatoires
	nAttributeNumber = optimizedDataGrid->GetAttributeNumber();

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

	// Cas VarPart : export de l'ensemble des attributs en tant qu'attributs obligatoires
	dataGridManager.SetSourceDataGrid(optimizedDataGrid);
	dataGridManager.ExportRandomAttributes(&mandatoryDataGrid, nAttributeNumber);

	// Creer un copie temporaire du neighbourDataGridMerger pour tester la surtokenisation: neighbourDataGridMergerTest
	// - copie pointant sur les meme inner attributes
	// - mis en place de nouveaux inner attributes issus de la surtokenisation
	// - export; affichage pour mise au point
	// - nettoyage

	// Potentiellement a faire
	// - Creer un copie temporaire du neighbourDataGridMerger pour tester la surtokenisation: neighbourDataGridMergerPROTO
	// - creer prealablement un copie de optimizedDataGrid: surtokenizedOptimizedDataGrid
	//   - surtokenizer son attribut VarPart via InitializeVarPartAttributeWithNewInnerAttributes
	// - ajout des parties:
	//     dataGridManager.AddRandomParts(neighbourDataGridMergerPROTO, surtokenizedOptimizedDataGrid,
	//                                   nRequestedContinuousPartNumber, nRequestedSymbolPartNumber, 1.0);

	// Exports d'attributs supplementaires
	// Pour un dNoiseRate de 1, tous les attributs sont ajoutes a concurrence de nMaxAttributeNumber
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	neighbourDataGridMerger->DeleteAll();
	dataGridManager.AddRandomAttributes(neighbourDataGridMerger, &mandatoryDataGrid, nAttributeNumber);

	// Export des parties
	dataGridManager.AddRandomParts(neighbourDataGridMerger, optimizedDataGrid, nRequestedContinuousPartNumber,
				       nRequestedSymbolPartNumber, 1.0);
	TaskProgression::DisplayProgression(25);

	// Export des cellules
	dataGridManager.ExportCells(neighbourDataGridMerger);

	// Fin de tache
	TaskProgression::EndTask();
}

void KWDataGridOptimizer::SaveDataGrid(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const
{
	KWDataGridManager dataGridManager;

	// Memorisation de la grille
	if (bCleanNonInformativeVariables)
		dataGridManager.CopyInformativeDataGrid(sourceDataGrid, targetDataGrid);
	else
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

boolean KWDataGridOptimizer::IsLastGranularity(const KWDataGrid* dataGrid) const
{
	require(dataGrid != NULL);
	return dataGrid->GetGranularity() == ComputeMaxExploredGranularity(dataGrid);
}

int KWDataGridOptimizer::ComputeMaxExploredGranularity(const KWDataGrid* dataGrid) const
{
	int nMaxExploredGranularity;
	const int nMinValueNumber = 500;
	int nValueNumber;
	boolean bIsGranularityMaxThresholded;

	require(dataGrid != NULL);

	// Nombre total de valeurs potentielles, base sur le nombre d'individus de la grille
	nValueNumber = dataGrid->GetGridFrequency();

	// Seuillage eventuel de la granularite max pour reduire la complexite algorithmique,
	// (sauf dans le cas des petits nombre de valeurs)
	bIsGranularityMaxThresholded = false;
	if (nValueNumber > nMinValueNumber)
	{
		// Cas d'une grille 2D supervise avec un attribut a predire avec groupage de la cible
		// (classification avec groupage ou regression) et un attribut explicatif numerique
		if (dataGrid->GetTargetAttribute() != NULL and dataGrid->GetTargetValueNumber() == 0 and
		    dataGrid->GetAttributeNumber() == 2 and
		    dataGrid->GetAttributeAt(0)->GetAttributeType() == KWType::Continuous)
			bIsGranularityMaxThresholded = true;
		// Cas d'une classification supervisee simple avec une paire de variables (numerique ou categorielle)
		else if (dataGrid->GetTargetValueNumber() > 0 and dataGrid->GetAttributeNumber() >= 2 and
			 nValueNumber > nMinValueNumber)
			bIsGranularityMaxThresholded = true;
	}

	// Cas sans seuillage
	if (not bIsGranularityMaxThresholded)
		nMaxExploredGranularity = (int)ceil(log(nValueNumber * 1.0) / log(2.0));
	// Sinon seuillage
	else
		nMaxExploredGranularity =
		    (int)ceil(log(nMinValueNumber + sqrt((nValueNumber - nMinValueNumber) *
							 log(nValueNumber - nMinValueNumber) / log(2.0))) /
			      log(2.0));
	return nMaxExploredGranularity;
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
	int nMaxExploredGranularity;
	ALString sProgressionLabel;
	double dOptimizationTime;
	ALString sContext;
	ALString sTmp;

	// Calcul du temps d'optimisation
	dOptimizationTime = timerOptimization.GetElapsedTime();

	// Calcul de la granularite max exploree
	nMaxExploredGranularity = 0;
	if (dataGrid->GetGranularity() > 0)
		nMaxExploredGranularity = ComputeMaxExploredGranularity(dataGrid);

	// Message d'avancement, avec prise en compte de la granularite uniquement si necessaire
	sProgressionLabel = sTmp + " Iter " + IntToString(nVNSIteration);
	if (nVNSIteration > 0 and nVNSNeighbourhoodLevelNumber > 0)
		sProgressionLabel += sTmp + "  VNS " + IntToString(nVNSNeighbourhoodLevelIndex) + "/" +
				     IntToString(nVNSNeighbourhoodLevelNumber) + " (" +
				     DoubleToString((int)(10000 * dVNSNeighbourhoodSize) / 10000.0) + ")";
	if (dataGrid->GetGranularity() > 0 and dataGrid->GetGranularity() < nMaxExploredGranularity)
		sProgressionLabel += sTmp + "  Granularity " + IntToString(dataGrid->GetGranularity()) + "/" +
				     IntToString(nMaxExploredGranularity) + " ";
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
		// Calcul du nombre total d'iteration, en combinant granularite et VNS
		if (IsSupervisedDataGrid(dataGrid))
		{
			dTotalIterNumber = pow(2.0, optimizationParameters.GetOptimizationLevel());
			dTotalIterNumber *= nMaxExploredGranularity;
		}
		else
		{
			dTotalIterNumber = pow(2.0, optimizationParameters.GetOptimizationLevel());
			dTotalIterNumber += nMaxExploredGranularity - 1;
		}

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
