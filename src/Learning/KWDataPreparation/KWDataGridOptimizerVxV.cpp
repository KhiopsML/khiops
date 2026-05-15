// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataGridOptimizerVxV.h"

KWDataGridOptimizerVxV::KWDataGridOptimizerVxV() {}

KWDataGridOptimizerVxV::~KWDataGridOptimizerVxV() {}

double KWDataGridOptimizerVxV::InternalOptimizeDataGrid(const KWDataGrid* initialDataGrid,
							KWDataGrid* optimizedDataGrid) const

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
	KWDataGrid granularizedPostMergedOptimizedDataGrid;
	KWDataGrid partitionedReferenceGranularizedPostMergedDataGrid;
	int nCurrentExploredGranularity;
	int nLastExploredGranularity;
	ALString sSuffix;
	ALString sTmp;

	require(GetDataGridCosts() != NULL);
	require(GetDataGridCosts()->IsInitialized());
	require(initialDataGrid != NULL);
	require(not initialDataGrid->IsVarPartDataGrid());
	require(optimizedDataGrid != NULL);
	require(optimizedDataGrid->GetCellNumber() == 1);
	require(GetDataGridCosts()->ComputeDataGridTotalCost(optimizedDataGrid) == GetOptimizedNullDataGridCost());

	//Initialisations
	dGranularityBestCost = DBL_MAX;
	dBestMergedCost = dGranularityBestCost;

	// La solution initiale en parametere est le modele null
	dBestCost = GetOptimizedNullDataGridCost();
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
		dataGridManager.InitializeQuantileBuilders(initialDataGrid, &odQuantileBuilders, &ivMaxPartNumbers);
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
			dataGridManager.ExportGranularizedDataGrid(initialDataGrid, &granularizedDataGrid,
								   nGranularityIndex, &odQuantileBuilders);

			//////////////////////////////////////////////////////////////////////////////////////////////
			// On determine si la granularite courante doit etre traitee
			// - bIsGranularitySelected: parce qu'elle differe suffisamment de la granularite precedente
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
				// On ne traite cette granularite que si elle est differe suffisamment de la precedente et de la
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

				// Utilisation du modele nul pour la solution initiale
				granularizedOptimizedDataGrid = new KWDataGrid;
				SaveDataGrid(GetOptimizedNullDataGrid(), granularizedOptimizedDataGrid);
				granularizedOptimizedDataGrid->SetGranularity(granularizedDataGrid.GetGranularity());
				dGranularityBestCost = GetOptimizedNullDataGridCost();
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
				KWDataGridOptimizer::GetProfiler()->WriteKeyInt("Granularity index", nGranularityIndex);
				if (bDisplayGranularities)
				{
					cout << "OptimizeDataGrid\tGranularite\t" << nGranularityIndex << "\n";
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

				// Cas d'amelioration du cout
				if (dGranularityBestCost < dBestCost - dEpsilon)
				{
					dBestCost = dGranularityBestCost;

					// Memorisation du nouvel optimum
					dataGridManager.CopyDataGrid(granularizedOptimizedDataGrid, optimizedDataGrid);
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

		// Post-optimisation de la granularite dans le le cas d'une grille supervisee pour laquelle
		// la granularite fait partie des parametres du modele
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

	ensure(optimizedDataGrid->AreAttributePartsSorted() or TaskProgression::IsInterruptionRequested());
	ensure(fabs(dBestCost - GetDataGridCosts()->ComputeDataGridTotalCost(optimizedDataGrid)) < dEpsilon);
	return dBestCost;
}

boolean KWDataGridOptimizerVxV::IsLastGranularity(const KWDataGrid* dataGrid) const
{
	require(dataGrid != NULL);
	return dataGrid->GetGranularity() == ComputeMaxExploredGranularity(dataGrid);
}

int KWDataGridOptimizerVxV::ComputeMaxExploredGranularity(const KWDataGrid* dataGrid) const
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

double KWDataGridOptimizerVxV::OptimizeGranularizedDataGrid(const KWDataGrid* initialDataGrid,
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

double KWDataGridOptimizerVxV::SlightOptimizeGranularizedDataGrid(const KWDataGrid* initialDataGrid,
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

	// Optimisation d'une solution dans un voisinnage de la solution courante
	dCost = OptimizeNeighbourSolution(initialDataGrid, optimizedDataGrid, 1, &neighbourDataGrid, false);

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

double KWDataGridOptimizerVxV::PostOptimizeGranularity(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid,
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

double
KWDataGridOptimizerVxV::OptimizeWithBestUnivariatePartitionForCurrentGranularity(const KWDataGrid* initialDataGrid,
										 KWDataGrid* optimizedDataGrid) const
{
	const boolean bTrace = false;
	const boolean bTraceDetails = false;
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
	KWDataGridOptimizer::GetProfiler()->BeginMethod("OptimizeWithBestUnivariatePartitionForCurrentGranularity");
	KWDataGridOptimizer::GetProfiler()->WriteKeyString("Coclustering", optimizedDataGrid->GetObjectLabel());
	KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Cost", dBestCost);
	if (bTrace)
		TraceOptimizationDetails("OptimizeWithBestUnivariatePartition", optimizedDataGrid, bTraceDetails);

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
				    initialDataGrid, &univariateDataGrid, nAttribute, classStats);
				bEvaluated = univariateDataGrid.GetAttributeAt(0)->GetPartNumber() > 1;
			}

			// Cas ou la granularite de la meilleure partition univariee de l'attribut correspond
			// a la granularite courante de la grille
			else
			{
				dataGridManager.BuildUnivariateDataGridFromAttributeStats(
				    initialDataGrid, &univariateDataGrid, attributeStats);

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
			KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Univariate cost", dCost);
			if (bTrace)
				TraceOptimizationDetails("- univariate datagrid", &univariateDataGrid, bTraceDetails);

			// Memorisation de la meilleure solution
			if (dCost < dBestCost - dEpsilon)
			{
				dBestCost = dCost;
				SaveDataGrid(&univariateDataGrid, optimizedDataGrid);
				bImproved = true;
			}
		}
	}
	KWDataGridOptimizer::GetProfiler()->EndMethod("OptimizeWithBestUnivariatePartitionForCurrentGranularity");

	// Retour du cout
	ensure(fabs(dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid) - dBestCost) == 0 or bImproved);
	ensure(fabs(dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid) - dBestCost) < dEpsilon);
	return dBestCost;
}

double KWDataGridOptimizerVxV::OptimizeWithMultipleUnivariatePartitions(const KWDataGrid* initialDataGrid,
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
	bOk = dataGridManager.BuildDataGridFromUnivariateProduct(initialDataGrid, &multivariateDataGrid, classStats);

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

void KWDataGridOptimizerVxV::DisplayProgression(const KWDataGrid* dataGrid) const
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
