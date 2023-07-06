// Copyright (c) 2023 Orange. All rights reserved.
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
	dEpsilon = 1e-6;
}

KWDataGridOptimizer::~KWDataGridOptimizer() {}

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
	boolean bDisplayResults = false;
	KWDataGrid granularizedDataGrid;
	KWDataGrid* granularizedOptimizedDataGrid;
	KWDataGridManager dataGridManager;
	double dGranularityBestCost;
	double dBestCost;
	double dTotalTime;
	int nGranularityIndex;
	int nGranularityMax;
	int nValueNumber;
	boolean bIsLastGranularity;
	boolean bOptimizationNeeded;
	boolean bWithMemoryMode;
	ALString sTmp;
	ObjectDictionary odQuantileBuilders;
	IntVector ivPreviousPartNumber;
	IntVector ivCurrentPartNumber;
	int nAttribute;
	boolean bIsGranularitySelected;
	double dRequiredIncreasingCoefficient;
	IntVector ivMaxPartNumbers;
	int nMinValueNumber = 500;
	boolean bIsGranularityMaxThresholded;
	double dBestMergedCost;
	// CH double dMergedCost;
	KWDataGrid granularizedPostMergedOptimizedDataGrid;
	KWDataGrid partitionedReferenceGranularizedPostMergedDataGrid;
	// CH double dFusionDeltaCost;
	int nCurrentExploredGranularity;
	int nLastExploredGranularity;

	dGranularityBestCost = DBL_MAX;
	dBestMergedCost = dGranularityBestCost;
	dTotalTime = 0;
	nValueNumber = initialDataGrid->GetGridFrequency();
	bWithMemoryMode = false;

	// Controle de la graine aleatoire pour avoir des resultats reproductibles
	SetRandomSeed(1);

	// Construction d'une grille terminale pour la solution initiale
	dBestCost = InitializeWithTerminalDataGrid(initialDataGrid, optimizedDataGrid);

	if (bDisplayResults)
		cout << "KWOptimize : Cout grille terminale independant granularite " << dBestCost << endl;

	if (bDisplayResults)
	{
		cout << "KWOptimize :Grille initiale avant optimisation" << endl;
		initialDataGrid->Write(cout);
	}

	// On determine si on peut potentiellement faire mieux que la grille terminale
	bOptimizationNeeded = true;
	if ((initialDataGrid->GetTargetAttribute() == NULL and initialDataGrid->GetTargetValueNumber() == 1) or
	    (initialDataGrid->GetTargetAttribute() == NULL and initialDataGrid->GetInformativeAttributeNumber() == 0) or
	    (initialDataGrid->GetTargetAttribute() != NULL and
	     initialDataGrid->GetTargetAttribute()->GetPartNumber() <= 1) or
	    (initialDataGrid->GetTargetAttribute() != NULL and initialDataGrid->GetInformativeAttributeNumber() <= 1))
		bOptimizationNeeded = false;

	// Cas ou la grille terminale est ameliorable
	if (bOptimizationNeeded)
	{
		// On parcourt les differentes granularites

		// Initialisation de la granularite maximale avec seuillage eventuel de la granularite max pour reduire
		// la complexite algorithmique
		bIsGranularityMaxThresholded = false;
		// Cas d'une une grille 2D supervise avec un attribut a predire avec groupage de la cible
		// (classification avec groupage ou regression) et un attribut explicatif numerique dont le nombre de
		// valeurs potentielle (= nombre d'instances) est > nMinValueNumber
		if (initialDataGrid->GetTargetAttribute() != NULL and initialDataGrid->GetTargetValueNumber() == 0 and
		    initialDataGrid->GetAttributeNumber() == 2 and
		    initialDataGrid->GetAttributeAt(0)->GetAttributeType() == KWType::Continuous and
		    (nValueNumber > nMinValueNumber))
			bIsGranularityMaxThresholded = true;
		// Cas d'une classification supervisee simple avec une paire de variables (numerique ou categorielle)
		// pour lesquelles le nombre de valeurs potentielles (= nombre d'instances) est > nMinValueNumber
		else if (initialDataGrid->GetTargetValueNumber() > 0 and initialDataGrid->GetAttributeNumber() == 2 and
			 nValueNumber > nMinValueNumber)
			bIsGranularityMaxThresholded = true;

		// Cas sans seuillage
		if (not bIsGranularityMaxThresholded)
			nGranularityMax = (int)ceil(log(nValueNumber * 1.0) / log(2.0));
		// Sinon seuillage
		else
			nGranularityMax =
			    (int)ceil(log(nMinValueNumber + sqrt((nValueNumber - nMinValueNumber) *
								 log(nValueNumber - nMinValueNumber) / log(2.0))) /
				      log(2.0));

		// Initialisation
		nGranularityIndex = 1;
		bIsLastGranularity = false;

		// Initialisation du facteur d'accroissement requis entre deux partitions traitees
		dRequiredIncreasingCoefficient = 2;

		// Initialisation des quantiles builders a partir de la grille source
		dataGridManager.SetSourceDataGrid(initialDataGrid);
		dataGridManager.InitializeQuantileBuildersBeforeGranularization(&odQuantileBuilders, &ivMaxPartNumbers);

		if (bDisplayResults)
			cout << "ivMaxPartNumbers Granularisation\t" << ivMaxPartNumbers << flush;

		// Initialisation des vecteurs de nombre de parties courant et precedent
		ivPreviousPartNumber.SetSize(ivMaxPartNumbers.GetSize());
		ivCurrentPartNumber.SetSize(ivMaxPartNumbers.GetSize());

		// Initialisation
		nCurrentExploredGranularity = -1;
		nLastExploredGranularity = -1;

		// Parcours des granularites
		while (nGranularityIndex <= nGranularityMax and not bIsLastGranularity)
		{
			// Arret si interruption utilisateur
			if (TaskProgression::IsInterruptionRequested())
				break;

			// Granularisation de la grille initiale
			dataGridManager.SetSourceDataGrid(initialDataGrid);
			dataGridManager.ExportGranularizedDataGrid(&granularizedDataGrid, nGranularityIndex,
								   &odQuantileBuilders);

			// Etude du nombre de parties attribut par attribut pour decider du traitement ou non a cette
			// granularite Parcours des attributs
			for (nAttribute = 0; nAttribute < granularizedDataGrid.GetAttributeNumber(); nAttribute++)
			{
				// Memorisation du nombre de parties de l'attribut granularise
				ivCurrentPartNumber.SetAt(
				    nAttribute, cast(KWDGAttribute*, granularizedDataGrid.GetAttributeAt(nAttribute))
						    ->GetPartNumber());
			}
			bIsLastGranularity = true;

			/* CH
			// Affichage pour grille individus * variables
			if (bDisplayResults and granularizedDataGrid.IsDataGridGeneric())
				cout << "Prepartitionnement\t" <<
			granularizedDataGrid.GetImpliedAttributes()->GetVarPartsGranularity() << "\tGranularite" <<
			nGranularityIndex
				     << "\tNombre de clusters" << ivCurrentPartNumber << flush;
			*/

			// Si on n'a pas encore atteint la granularite max
			if (nGranularityIndex < nGranularityMax)
			{
				for (nAttribute = 0; nAttribute < granularizedDataGrid.GetAttributeNumber();
				     nAttribute++)
				{
					// Cas ou le nombre de parties de l'attribut courant est inferieur au nombre max
					// de parties de l'attribut
					if (ivCurrentPartNumber.GetAt(nAttribute) < ivMaxPartNumbers.GetAt(nAttribute))
					{
						bIsLastGranularity = false;
						break;
					}
				}
			}

			// Cas ou cette granularite sera la derniere traitee
			if (bIsLastGranularity)
				// On positionne l'index de granularite au maximum afin que l'affichage soit adapte a ce
				// cas
				granularizedDataGrid.SetGranularity(nGranularityMax);

			bIsGranularitySelected = false;
			for (nAttribute = 0; nAttribute < granularizedDataGrid.GetAttributeNumber(); nAttribute++)
			{
				// Cas d'accroissement suffisant du nombre de parties
				if ((ivCurrentPartNumber.GetAt(nAttribute) >=
				     ivPreviousPartNumber.GetAt(nAttribute) * dRequiredIncreasingCoefficient) and
				    (ivCurrentPartNumber.GetAt(nAttribute) * dRequiredIncreasingCoefficient <=
				     ivMaxPartNumbers.GetAt(nAttribute)))
				{
					bIsGranularitySelected = true;
					break;
				}
			}

			// On ne traite pas les grilles avec un seul attribut informatif
			if (granularizedDataGrid.GetInformativeAttributeNumber() <= 1)
				bIsGranularitySelected = false;

			// Cas du traitement de la granularite courante
			if (bIsGranularitySelected or bIsLastGranularity)
			{
				// Memorisation des granularites exploitees
				nLastExploredGranularity = nCurrentExploredGranularity;
				nCurrentExploredGranularity = nGranularityIndex;

				// Initialisation de la grille granularisee optimisee a la grille terminale
				granularizedOptimizedDataGrid = new KWDataGrid;
				dGranularityBestCost = InitializeWithTerminalDataGrid(&granularizedDataGrid,
										      granularizedOptimizedDataGrid);

				if (bDisplayResults)
				{
					cout << "KWOptimize :Cout Grille initiale granularisee pour granularite = "
					     << IntToString(nGranularityIndex) << "\t"
					     << dataGridCosts->ComputeDataGridTotalCost(&granularizedDataGrid) << endl;
					granularizedDataGrid.Write(cout);
					granularizedDataGrid.WriteAttributes(cout);
					granularizedDataGrid.WriteAttributeParts(cout);
					cout << "KWOptimize : Cout grille terminale pour cette granularite\t"
					     << dGranularityBestCost << endl;
				}

				// Cas avec memoire : on part a la granularite courante de la meilleure grille
				// rencontree aux granularites precedentes, si elle est meilleure que la grille initiale
				// a cette granularite
				if (bWithMemoryMode and dBestCost < dGranularityBestCost)
					dataGridManager.CopyDataGrid(optimizedDataGrid, granularizedOptimizedDataGrid);

				// Optimisation de la grille granularisee
				dGranularityBestCost =
				    OptimizeGranularizedDataGrid(&granularizedDataGrid, granularizedOptimizedDataGrid,
								 bIsLastGranularity, dTotalTime);

				if (bDisplayResults)
				{
					cout << "KWOptimize : Apres OptimizeGranularizedDataGrid pour Granularite "
					     << nGranularityIndex << "\t Cout " << dGranularityBestCost << endl;
					granularizedOptimizedDataGrid->Write(cout);
				}

				/* CH
				// Le cout obtenu dGranularityBestCost est le cout de l'antecedent de la meilleure
				grille
				// L'amelioration de cout doit etre mesuree par rapport au cout de la grille
				post-fusionnee de granularizedOptimizedDataGrid if
				(granularizedOptimizedDataGrid->IsDataGridGeneric())
				{
					if (granularizedOptimizedDataGrid->GetInformativeAttributeNumber() > 0 and
				optimizationParameters.GetPostFusion())
					{
						dataGridManager.SetSourceDataGrid(granularizedOptimizedDataGrid);
						// Creation d'une nouvelle grille avec nouvelle description des PV

						// Calcul de la grille de reference post fusionnee a partir de
				granularizedDataGrid dFusionDeltaCost =
				dataGridManager.ExportMergedDataGridForVarPartAttributes(&granularizedPostMergedOptimizedDataGrid,
				dataGridCosts); assert(not granularizedPostMergedOptimizedDataGrid.AreVarPartsShared());

						// Calcul et verification du cout
						dMergedCost = dGranularityBestCost + dFusionDeltaCost;
						// Le cout precedent devra etre correct
						assert(dMergedCost * (1 - dEpsilon) <
				dataGridCosts->ComputeDataGridTotalCost(&granularizedPostMergedOptimizedDataGrid));
						assert(dataGridCosts->ComputeDataGridTotalCost(&granularizedPostMergedOptimizedDataGrid)
				< dMergedCost * (1 + dEpsilon));

						if (bDisplayResults)
						{
							cout << "KWOptimize : Niveau prepartitionnement \t" <<
				granularizedOptimizedDataGrid->GetImpliedAttributes()->GetVarPartsGranularity() << "\t
				Grille avant fusion \t" << dGranularityBestCost << "\n";
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
				coclustering if (bIsLastGranularity)
					{
						if (bDisplayResults)
							cout << "KWOptimize : Mise a jour de la memorisation du
				coclustering pour la derniere granularite " << endl;

						if
				(granularizedPostMergedOptimizedDataGrid.GetInformativeAttributeNumber() == 0)
							HandleOptimizationStep(optimizedDataGrid, &granularizedDataGrid,
				true);

						else
						{
							// La grille source est la grille de reference qui contient la
				partition la plus fine dataGridManager.SetSourceDataGrid(GetInitialDataGrid());
							dataGridManager.ExportDataGridWithSingletonVarParts(&granularizedPostMergedOptimizedDataGrid,
				&partitionedReferenceGranularizedPostMergedDataGrid, true);
							HandleOptimizationStep(&granularizedPostMergedOptimizedDataGrid,
				&partitionedReferenceGranularizedPostMergedDataGrid, false);
							// Nettoyage
							partitionedReferenceGranularizedPostMergedDataGrid.DeleteAll();
						}
					}

					// Nettoyage
					granularizedPostMergedOptimizedDataGrid.DeleteAll();
				}
				// Fin du cas grille generique : coclustering individus * variables

				// Sinon : cas variable * variable
				else
				*/
				{
					// Cas d'amelioration du cout
					if (dGranularityBestCost < dBestCost)
					{
						dBestCost = dGranularityBestCost;

						// Memorisation du nouvel optimum
						dataGridManager.CopyDataGrid(granularizedOptimizedDataGrid,
									     optimizedDataGrid);

						if (bDisplayResults)
						{
							cout << "KWOptimize :Grille granularizedOptimizedDataGrid"
							     << endl;
							granularizedOptimizedDataGrid->Write(cout);
						}
					}
					// Cas ou il s'agit de la derniere granularite : on met a jour les infos dans le
					// cas d'un coclustering
					if (bIsLastGranularity)
					{
						if (bDisplayResults)
							cout << "KWOptimize :Mise a jour de la memorisation du "
								"coclustering pour la derniere granularite "
							     << endl;

						HandleOptimizationStep(optimizedDataGrid, &granularizedDataGrid, true);
					}
				}

				// Nettoyage de la grille granularisee
				granularizedDataGrid.DeleteAll();

				// Nettoyage de la grille optimisee pour cette granularite
				delete granularizedOptimizedDataGrid;
				granularizedOptimizedDataGrid = NULL;

				// Cas d'un temps limite : mise a jour du temps restant par retrait du temps consacre a
				// cette granularite
				if (optimizationParameters.GetOptimizationTime() > 0)
				{
					// L'utilisation de la totalite du temps global alloue (OptimizationTime) peut
					// conduire a l'arret du parcours des granularites et nuire a la recherche de la
					// grille optimale
					if (optimizationParameters.GetOptimizationTime() - dTotalTime > 0)
					{
						optimizationParameters.SetOptimizationTime(
						    optimizationParameters.GetOptimizationTime() - (int)dTotalTime);
						if (bDisplayResults)
							cout << "KWOptimize :Temps restant apres optimisation a la "
								"granularite \t"
							     << nGranularityIndex << "\t "
							     << optimizationParameters.GetOptimizationTime() << endl;
					}
					else
					{
						break;
						// Affichage d'un warning pour eventuelle modification de l'optimisation
						// time
						AddWarning(sTmp +
							   "All the optimization time has been used but maximum "
							   "granularity has not been reached:" +
							   IntToString(nGranularityIndex) + " on " +
							   IntToString(nGranularityMax) +
							   ". You could obtain better results with greater "
							   "optimization time.");
						if (bDisplayResults)
							cout << "KWOptimize :Totalite du temps alloue ecoule apres la "
								"granularite \t"
							     << nGranularityIndex << endl;
					}
				}

				// Memorisation du nombre de parties par attribut pour comparaison a l'etape suivante
				ivPreviousPartNumber.CopyFrom(&ivCurrentPartNumber);
			}
			else
			{
				if (bDisplayResults)
					cout << "KWOptimize :Granularite " << nGranularityIndex
					     << " non traitee car identique a la precedente" << endl;
				// Nettoyage de la grille granularisee
				granularizedDataGrid.DeleteAll();
			}

			nGranularityIndex++;
		}

		// CH PostOptiGranu
		// Cas d'une grille conventionnelle hors coclustering de type individus * variables
		// CH if (not optimizedDataGrid->IsDataGridGeneric())
		{
			// Post-optimisation de la granularite : on attribue a la grille optimale la plus petite
			// granularite pour laquelle cette grille est definie
			if (nLastExploredGranularity != -1 and
			    optimizedDataGrid->GetGranularity() > nLastExploredGranularity + 1)
				PostOptimizeGranularity(initialDataGrid, optimizedDataGrid, odQuantileBuilders,
							nLastExploredGranularity);
		}
		// Fin CH PostOptiGranu

		// Nettoyage
		odQuantileBuilders.DeleteAll();
	}
	// Cas ou la grille terminale n'est pas ameliorable
	else
	{
		// Tri des parties par attribut, pour preparer les affichages de resultats
		// ainsi que les resultats de preparation des donnees
		optimizedDataGrid->SortAttributeParts();
	}

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

double KWDataGridOptimizer::OptimizeGranularizedDataGrid(const KWDataGrid* initialDataGrid,
							 KWDataGrid* optimizedDataGrid, boolean bIsLastGranularity,
							 double& dTotalComputeTime) const
{
	boolean bDisplayResults = false;
	KWDataGridManager dataGridManager;
	KWDataGridMerger dataGridMerger;
	KWDataGridPostOptimizer dataGridPostOptimizer;
	boolean bOptimizationNeeded;
	double dBestCost;
	clock_t tBegin;
	clock_t tEnd;
	ALString sTmp;

	require(initialDataGrid != NULL);
	require(initialDataGrid->Check());
	require(optimizedDataGrid != NULL);

	// Debut de suivi des taches
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel("Data Grid internal optimization");

	// Ligne d'entete des messages
	DisplayOptimizationHeaderLine();

	// Initialisations
	tBegin = clock();
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	dataGridMerger.SetDataGridCosts(dataGridCosts);
	dataGridPostOptimizer.SetDataGridCosts(dataGridCosts);

	// Initialisation du meilleur cout
	dBestCost = dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);

	if (bDisplayResults)
	{
		cout << "Debut OptimizeGranularizedDataGrid " << endl;
		cout << "Grille initiale" << endl;
		initialDataGrid->Write(cout);
		cout << "Grille optimale" << endl;
		optimizedDataGrid->Write(cout);
		cout << "Granularite courante \t " << initialDataGrid->GetGranularity()
		     << endl; //<< "\t Meilleure granu \t " << GetGranularity()->GetBestGranularity() << endl;
		cout << " Cout grille optimale a la granularite courante " << dBestCost << endl;
	}

	// On determine si on peut potentiellement faire mieux que la grille terminale
	bOptimizationNeeded = true;
	if ((initialDataGrid->GetTargetAttribute() == NULL and initialDataGrid->GetTargetValueNumber() == 1) or
	    (initialDataGrid->GetTargetAttribute() == NULL and initialDataGrid->GetInformativeAttributeNumber() == 0) or
	    (initialDataGrid->GetTargetAttribute() != NULL and
	     initialDataGrid->GetTargetAttribute()->GetPartNumber() <= 1) or
	    (initialDataGrid->GetTargetAttribute() != NULL and initialDataGrid->GetInformativeAttributeNumber() <= 1))
		bOptimizationNeeded = false;

	// Initialisation univariee
	if (bOptimizationNeeded and optimizationParameters.GetUnivariateInitialization() and
	    initialDataGrid->GetTargetValueNumber() > 0
	    // on ne fait pas d'univarie si un seul attribut
	    and initialDataGrid->GetAttributeNumber() > 1)
	{
		// Avec prise en compte de la granularite
		dBestCost =
		    OptimizeWithBestUnivariatePartitionForCurrentGranularity(initialDataGrid, optimizedDataGrid);

		// Recherche d'une amelioration par croisement des partitions univariees
		// Integre la granularite
		dBestCost = OptimizeWithMultipleUnivariatePartitions(initialDataGrid, optimizedDataGrid);
	}

	// Affichage
	if (bDisplayResults)
	{
		cout << "Grille optimisee avant VNS" << endl;
		optimizedDataGrid->Write(cout);
	}

	// Optimisation a partir d'une grille initiale complete si algorithme glouton
	if (bOptimizationNeeded)
	{
		// Optimisation avec algorithme greedy
		if (optimizationParameters.GetOptimizationAlgorithm() == "Greedy")
			dBestCost = GreedyOptimize(initialDataGrid, optimizedDataGrid);
		// Optimisation avec algorithme multi-start
		else if (optimizationParameters.GetOptimizationAlgorithm() == "MultiStart")
			dBestCost = MultiStartOptimize(initialDataGrid, optimizedDataGrid);
		// Optimisation avec algorithme VNS
		else if (optimizationParameters.GetOptimizationAlgorithm() == "VNS")
			dBestCost = VNSOptimize(initialDataGrid, optimizedDataGrid, bIsLastGranularity);
	}

	// Tri des parties par attribut, pour preparer les affichages de resultats
	// ainsi que les resultats de preparation des donnees
	optimizedDataGrid->SortAttributeParts();

	// Affichage de la grille finale avec ses couts
	if (optimizationParameters.GetDisplayDetails() and bDisplayResults)
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
		cout << "Granularite courante \t " << optimizedDataGrid->GetGranularity() << "\t Meilleure granu \t "
		     << endl; // GetGranularity()->GetBestGranularity() << endl;
		cout << " Cout meilleure grille " << dBestCost << endl;
	}

	// Affichage du temps de calcul
	tEnd = clock();
	dTotalComputeTime = (double)(tEnd - tBegin) / CLOCKS_PER_SEC;
	if (optimizationParameters.GetDisplayDetails())
		initialDataGrid->AddMessage(sTmp +
					    "Data grid optimization time: " + SecondsToString((int)dTotalComputeTime));

	// Fin de suivi des taches
	TaskProgression::EndTask();

	// Retour du meilleur cout de codage
	ensure(fabs(dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid) - dBestCost) < dEpsilon);
	return dBestCost;
}

void KWDataGridOptimizer::HandleOptimizationStep(const KWDataGrid* optimizedDataGrid,
						 const KWDataGrid* initialGranularizedDataGrid,
						 boolean bIsLastSaving) const
{
}

void KWDataGridOptimizer::PostOptimizeGranularity(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid,
						  ObjectDictionary& odQuantileBuilders,
						  int nLastExploredGranularity) const
{
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

	// Initialisation
	bLastExploredGranularity = false;
	bIncompatibleGranularity = false;
	nCurrentGranularity = optimizedDataGrid->GetGranularity() - 1;
	nBestGranularity = optimizedDataGrid->GetGranularity();

	require(nBestGranularity > nLastExploredGranularity + 1);

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
						 odQuantileBuilders.Lookup(attribute->GetAttributeName()));
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
						 odQuantileBuilders.Lookup(attribute->GetAttributeName()));

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
	if (bCleanNonInformativeVariables)
		dataGridManager.CopyInformativeDataGrid(&terminalDataGrid, optimizedDataGrid);
	else
		dataGridManager.CopyDataGrid(&terminalDataGrid, optimizedDataGrid);

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
	KWAttributeStats* attributeStats;
	KWDataGridManager dataGridManager;
	KWDataGrid univariateDataGrid;
	KWDGAttribute* initialAttribute;
	KWDGAttribute* targetAttribute;
	double dBestCost;
	double dCost;
	int nAttribute;
	int nTarget;
	boolean bEvaluated;
	boolean bImproved;
	boolean bDisplayResults = false;

	require(initialDataGrid != NULL);
	require(optimizedDataGrid != NULL);
	require(initialDataGrid->GetTargetValueNumber() > 0);

	// Initialisations
	dBestCost = dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);
	dataGridManager.SetSourceDataGrid(initialDataGrid);

	if (bDisplayResults)
		cout << " OptimizeWithBestUnivariate cout initial " << dBestCost << endl;

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
				// Export de la granularite
				univariateDataGrid.SetGranularity(initialDataGrid->GetGranularity());

				// Initialisation de la grille cible avec le nombre d'attributs demandes
				univariateDataGrid.Initialize(1, initialDataGrid->GetTargetValueNumber());

				// Initialisation des valeurs cibles
				for (nTarget = 0; nTarget < initialDataGrid->GetTargetValueNumber(); nTarget++)
				{
					univariateDataGrid.SetTargetValueAt(nTarget,
									    initialDataGrid->GetTargetValueAt(nTarget));
				}

				// Extraction de l'attribut source associe
				assert(initialAttribute != NULL);

				// Initialisation de l'attribut
				targetAttribute = univariateDataGrid.GetAttributeAt(0);
				// Recuperation des caracteristiques de l'attribute dont cout de selection/construction
				// de l'attribut
				targetAttribute->SetAttributeName(initialAttribute->GetAttributeName());
				targetAttribute->SetAttributeType(initialAttribute->GetAttributeType());
				targetAttribute->SetAttributeTargetFunction(
				    initialAttribute->GetAttributeTargetFunction());
				targetAttribute->SetCost(initialAttribute->GetCost());
				targetAttribute->SetInitialValueNumber(initialAttribute->GetInitialValueNumber());
				targetAttribute->SetGranularizedValueNumber(
				    initialAttribute->GetGranularizedValueNumber());

				// Transfert du parametrage du fourre-tout
				if (initialAttribute->GetCatchAllValueSet() != NULL)
					targetAttribute->InitializeCatchAllValueSet(
					    initialAttribute->GetCatchAllValueSet());

				dataGridManager.BuildDataGridAttributeFromGranularizedPartition(
				    initialAttribute, targetAttribute, classStats);
				bEvaluated = targetAttribute->GetPartNumber() > 1;

				// Export des nouvelles cellules
				univariateDataGrid.DeleteAllCells();
				dataGridManager.ExportCells(&univariateDataGrid);
			}

			// Cas ou la granularite de la meilleure partition univariee de l'attribut correspond
			// a la granularite courante de la grille
			else
			{
				dataGridManager.BuildDataGridFromUnivariateStats(&univariateDataGrid, attributeStats);

				// Transfert du parametrage du fourre-tout
				targetAttribute = univariateDataGrid.GetAttributeAt(0);
				if (initialAttribute->GetCatchAllValueSet() != NULL)
					targetAttribute->InitializeCatchAllValueSet(
					    initialAttribute->GetCatchAllValueSet());
			}
		}

		// Evaluation si la grille univariee a ete construite
		if (bEvaluated)
		{
			// if (bDisplayResults)
			// cout << " AVANT Cout de la grille univariee " << endl;

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
				dataGridManager.CopyDataGrid(&univariateDataGrid, optimizedDataGrid);
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
	double dBestCost;
	double dCost;
	KWDataGridManager dataGridManager;
	KWDataGridMerger multivariateDataGrid;
	KWDataGridPostOptimizer dataGridPostOptimizer;
	boolean bOk;
	boolean bDisplayResults = false;

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
		cout << " OptimizeWithMultipleUnivariatePartitions :: dBestCost initial " << dBestCost << endl;
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
		cout << " OptimizeWithMultipleUnivariatePartitions :: dBestCost multivarie initial " << dCost << endl;
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
		cout << " OptimizeWithMultipleUnivariatePartitions :: dBestCost multivarie optimise " << dCost << endl;
		cout << " Grille multivariee optimisee " << endl;
		cout << multivariateDataGrid;
	}

	// Affichage du cout final
	DisplayOptimizationDetails(&multivariateDataGrid, true);

	// Memorisation de la meilleure solution
	if (dCost < dBestCost - dEpsilon)
	{
		dBestCost = dCost;
		if (bCleanNonInformativeVariables)
			dataGridManager.CopyInformativeDataGrid(&multivariateDataGrid, optimizedDataGrid);
		else
			dataGridManager.CopyDataGrid(&multivariateDataGrid, optimizedDataGrid);
	}

	// Retour du cout
	ensure(fabs(dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid) - dBestCost) < dEpsilon);
	return dBestCost;
}

double KWDataGridOptimizer::GreedyOptimize(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid) const
{
	KWDataGridManager dataGridManager;
	KWDataGridMerger dataGridMerger;
	KWDataGridPostOptimizer dataGridPostOptimizer;
	double dBestCost;
	double dCost;

	// Initialisations
	dBestCost = dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	dataGridMerger.SetDataGridCosts(dataGridCosts);
	dataGridPostOptimizer.SetDataGridCosts(dataGridCosts);

	// Export complet
	dataGridManager.ExportDataGrid(&dataGridMerger);

	// Affichage du cout initial
	DisplayOptimizationDetails(&dataGridMerger, false);

	// Cout initial si aucune optimisation
	dCost = DBL_MAX;
	if (not optimizationParameters.GetOptimize() and not optimizationParameters.GetPreOptimize() and
	    not optimizationParameters.GetPostOptimize())
		dCost = dataGridCosts->ComputeDataGridTotalCost(&dataGridMerger);

	// Pre-optimisation de la grille
	if (optimizationParameters.GetPreOptimize())
		dCost = dataGridPostOptimizer.PostOptimizeDataGrid(initialDataGrid, &dataGridMerger, false);

	// Optimisation par fusion des groupes
	if (optimizationParameters.GetOptimize())
		dCost = dataGridMerger.Merge();

	// Post-optimisation de la grille
	if (optimizationParameters.GetPostOptimize())
		dCost = dataGridPostOptimizer.PostOptimizeDataGrid(initialDataGrid, &dataGridMerger, true);

	// Affichage du cout final
	DisplayOptimizationDetails(&dataGridMerger, true);

	// Memorisation de la meilleure solution
	if (dCost < dBestCost - dEpsilon)
	{
		dBestCost = dCost;
		if (bCleanNonInformativeVariables)
			dataGridManager.CopyInformativeDataGrid(&dataGridMerger, optimizedDataGrid);
		else
			dataGridManager.CopyDataGrid(&dataGridMerger, optimizedDataGrid);
	}

	// Retour du cout
	ensure(fabs(dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid) - dBestCost) < dEpsilon);
	return dBestCost;
}

double KWDataGridOptimizer::MultiStartOptimize(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid) const
{
	KWDataGridManager dataGridManager;
	KWDataGridMerger dataGridMerger;
	KWDataGridPostOptimizer dataGridPostOptimizer;
	double dBestCost;
	double dCost;
	const int nDisplayedTry = -1;
	int nTryNumber;
	int nTry;
	int nAttributeNumber;
	int nPartNumber;
	Timer timerOptimization;

	// Initialisations
	dBestCost = dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	dataGridMerger.SetDataGridCosts(dataGridCosts);
	dataGridPostOptimizer.SetDataGridCosts(dataGridCosts);

	// Multi-start, partant de grilles aleatoire de taille "raisonnable"
	nTryNumber = (int)pow(2.0, optimizationParameters.GetOptimizationLevel());
	for (nTry = 0; nTry < nTryNumber; nTry++)
	{
		timerOptimization.Start();

		// Reinitialisation
		dataGridMerger.DeleteAll();

		// Specification du nombre d'attributs et de parties
		nAttributeNumber = 1 + RandomInt((int)(log(initialDataGrid->GetGridFrequency() * 1.0) / log(2.0)));
		if (nAttributeNumber < 2)
			nAttributeNumber = 2;
		if (nAttributeNumber > initialDataGrid->GetAttributeNumber())
			nAttributeNumber = initialDataGrid->GetAttributeNumber();
		nPartNumber =
		    2 + 2 * RandomInt((int)pow(initialDataGrid->GetGridFrequency() * 1.0, 1.0 / nAttributeNumber));
		if (nPartNumber > initialDataGrid->GetGridFrequency())
			nPartNumber = initialDataGrid->GetGridFrequency();

		// Export d'une grille aleatoire
		dataGridManager.ExportRandomAttributes(&dataGridMerger, nAttributeNumber);
		dataGridManager.ExportRandomParts(&dataGridMerger, nPartNumber);
		dataGridManager.ExportCells(&dataGridMerger);

		// Affichage du cout initial
		DisplayOptimizationDetails(&dataGridMerger, false);

		// Cout initial si aucune optimisation
		dCost = DBL_MAX;
		if (not optimizationParameters.GetOptimize() and not optimizationParameters.GetPreOptimize() and
		    not optimizationParameters.GetPostOptimize())
			dCost = dataGridCosts->ComputeDataGridTotalCost(&dataGridMerger);

		// Pre-optimisation de la grille
		if (optimizationParameters.GetPreOptimize())
			dCost = dataGridPostOptimizer.PostOptimizeDataGrid(initialDataGrid, &dataGridMerger, false);

		// Optimisation par fusion des groupes
		if (optimizationParameters.GetOptimize())
			dCost = dataGridMerger.Merge();

		// Post-optimisation de la grille
		if (optimizationParameters.GetPostOptimize())
			dCost = dataGridPostOptimizer.PostOptimizeDataGrid(initialDataGrid, &dataGridMerger, true);

		// Affichage du cout final
		DisplayOptimizationDetails(&dataGridMerger, true);

		// Affichage d'une grille particuliere
		if (optimizationParameters.GetDisplayDetails() and nTry == nDisplayedTry)
		{
			cout << dataGridMerger << endl;
			if (dataGridMerger.GetAttributeNumber() == 2)
				dataGridMerger.WriteCrossTableStats(cout, 0);
			dataGridMerger.GetDataGridCosts()->WriteDataGridAllCosts(&dataGridMerger, cout);
			cout << endl;
			break;
		}

		// Memorisation de la meilleure solution
		if (dCost < dBestCost - dEpsilon)
		{
			dBestCost = dCost;
			if (bCleanNonInformativeVariables)
				dataGridManager.CopyInformativeDataGrid(&dataGridMerger, optimizedDataGrid);
			else
				dataGridManager.CopyDataGrid(&dataGridMerger, optimizedDataGrid);

			// Gestion de la meilleure solution
			HandleOptimizationStep(optimizedDataGrid, NULL, false);
		}

		// Arret si contrainte de temps depassee
		timerOptimization.Stop();
		if (optimizationParameters.GetOptimizationTime() > 0 and
		    timerOptimization.GetElapsedTime() > optimizationParameters.GetOptimizationTime())
			break;
	}

	// Retour du cout
	ensure(fabs(dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid) - dBestCost) < dEpsilon);
	return dBestCost;
}

double KWDataGridOptimizer::VNSOptimize(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid,
					boolean bIsLastGranularity) const
{
	KWDataGridVNSOptimizer dataGridVNSOptimizer;
	double dBestCost;

	// Optimisation VNS
	dataGridVNSOptimizer.SetDataGridCosts(dataGridCosts);
	dataGridVNSOptimizer.GetParameters()->CopyFrom(&optimizationParameters);
	dataGridVNSOptimizer.SetDataGridOptimizer(this);

	// Cas non supervise (co-clustering)
	// Mise a true du parametre bSlightOptimizationMode  pour les granularites intermediaires
	if (initialDataGrid->GetTargetValueNumber() == 0 and initialDataGrid->GetTargetAttribute() == NULL and
	    not bIsLastGranularity)
		dataGridVNSOptimizer.SetSlightOptimizationMode(true);
	// Sinon : cas supervise ou derniere granularite en co-clustering
	else
		dataGridVNSOptimizer.SetSlightOptimizationMode(false);

	dBestCost = dataGridVNSOptimizer.OptimizeDataGrid(initialDataGrid, optimizedDataGrid);

	// Retour du cout
	ensure(fabs(dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid) - dBestCost) < dEpsilon);
	return dBestCost;
}

void KWDataGridOptimizer::DisplayOptimizationHeaderLine() const
{
	if (optimizationParameters.GetDisplayDetails())
	{
		// Lignes d'entete
		cout << "Initial\t\t\t\tFinal\t\t\t\t\n";
		cout << "Att. number\tPart number\tCell number\tCost\t";
		cout << "Att. number\tPart number\tCell number\tCost\n";
	}
}

void KWDataGridOptimizer::DisplayOptimizationDetails(const KWDataGrid* optimizedDataGrid, boolean bOptimized) const
{
	if (optimizationParameters.GetDisplayDetails())
	{
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

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridVNSOptimizer

KWDataGridVNSOptimizer::KWDataGridVNSOptimizer()
{
	dataGridCosts = NULL;
	bCleanNonInformativeVariables = false;
	nVNSIteration = 0;
	nVNSLevel = 0;
	nVNSMaxLevel = 0;
	dVNSNeighbourhoodSize = 0;
	dEpsilon = 1e-6;
	dataGridOptimizer = NULL;
	bSlightOptimizationMode = false;
}

KWDataGridVNSOptimizer::~KWDataGridVNSOptimizer() {}

void KWDataGridVNSOptimizer::SetDataGridCosts(const KWDataGridCosts* kwdgcCosts)
{
	dataGridCosts = kwdgcCosts;
}

const KWDataGridCosts* KWDataGridVNSOptimizer::GetDataGridCosts() const
{
	return dataGridCosts;
}

KWDataGridOptimizerParameters* KWDataGridVNSOptimizer::GetParameters()
{
	return &optimizationParameters;
}

void KWDataGridVNSOptimizer::SetSlightOptimizationMode(boolean bValue)
{
	bSlightOptimizationMode = bValue;
}

const boolean KWDataGridVNSOptimizer::GetSlightOptimizationMode() const
{
	return bSlightOptimizationMode;
}

double KWDataGridVNSOptimizer::OptimizeDataGrid(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid) const
{
	int nMaxLevel;

	require(initialDataGrid != NULL);
	require(initialDataGrid->Check());
	require(optimizedDataGrid != NULL);

	// On prend au minimum un niveau max de 1
	nMaxLevel = optimizationParameters.GetOptimizationLevel();
	if (nMaxLevel <= 0)
		nMaxLevel = 1;

	// Optimisation
	return IterativeVNSOptimizeDataGrid(initialDataGrid, nMaxLevel, optimizedDataGrid);
}

void KWDataGridVNSOptimizer::SetDataGridOptimizer(const KWDataGridOptimizer* optimizer)
{
	dataGridOptimizer = optimizer;
}

const KWDataGridOptimizer* KWDataGridVNSOptimizer::GetDataGridOptimizer() const
{
	return dataGridOptimizer;
}

double KWDataGridVNSOptimizer::IterativeVNSOptimizeDataGrid(const KWDataGrid* initialDataGrid, int nMaxLevel,
							    KWDataGrid* optimizedDataGrid) const
{
	int nLevel;
	double dCost;
	double dBestCost;
	KWDataGridManager dataGridManager;
	KWDataGrid currentDataGrid;
	double dMaxNeighbourhoodSize;
	double dMinNeighbourhoodSize;
	double dDecreaseFactor;
	int nIndexNumber;
	boolean bDisplayResults = false;

	// On ne reverifie pas les precondition de la methode publique
	require(1 <= nMaxLevel);

	// Demarrage du timer
	timerVNS.Reset();
	timerVNS.Start();

	// Ligne d'entete des messages
	DisplayOptimizationHeaderLine();

	// Initialisations
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	dBestCost = dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);

	// Calcul d'une taille minimale de voisinnage, relative a la taille de la base
	// On souhaite impliquer au minimum 3 nouvelles parties par attribut
	dMinNeighbourhoodSize = 3.0 / (3 + initialDataGrid->GetGridFrequency());

	// La taile max est de 1
	dMaxNeighbourhoodSize = 1;

	// Appel de VNS en augmentant le nombre de voisinnages d'un facteur 2 chaque fois
	nVNSIteration = 0;
	for (nLevel = 0; nLevel < nMaxLevel; nLevel++)
	{
		if (bDisplayResults)
			cout << "IterativeVNSOptimizeDataGrid :: nLevel \t" << nLevel << endl;

		// Recopie de la meilleure solution dans une solution de travail courante
		dataGridManager.CopyDataGrid(optimizedDataGrid, &currentDataGrid);
		dCost = dataGridCosts->ComputeDataGridTotalCost(&currentDataGrid);

		// Calcul du nombre d'index
		nIndexNumber = int(pow(2.0, nLevel));

		// Calcul du taux de decroissance des taille de voisinnage de facon a obtenir
		// une taille minimale de voisinnage suffisante
		dDecreaseFactor = 1.0 / pow(dMinNeighbourhoodSize, 1.0 / (nIndexNumber + 1));

		// Optimisation a partir de la nouvelle solution
		dCost = VNSOptimizeDataGrid(initialDataGrid, dDecreaseFactor, 0, nIndexNumber, &currentDataGrid, dCost);
		if (dCost < dBestCost - dEpsilon)
		{
			dBestCost = dCost;
			if (bCleanNonInformativeVariables)
				dataGridManager.CopyInformativeDataGrid(&currentDataGrid, optimizedDataGrid);
			else
				dataGridManager.CopyDataGrid(&currentDataGrid, optimizedDataGrid);
		}

		// Test de fin de tache
		if (TaskProgression::IsInterruptionRequested())
			break;

		// Test si depassement de temps
		if (IsOptimizationTimeElapsed())
			break;

		// Test cas d'une unique iteration
		if (GetSlightOptimizationMode())
			break;
	}
	assert(dBestCost < DBL_MAX);

	// Arret du timer
	timerVNS.Stop();
	timerVNS.Reset();

	ensure(fabs(dBestCost - dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid)) < dEpsilon);
	return dBestCost;
}

double KWDataGridVNSOptimizer::VNSOptimizeDataGrid(const KWDataGrid* initialDataGrid, double dDecreaseFactor,
						   int nMinIndex, int nMaxIndex, KWDataGrid* optimizedDataGrid,
						   double dOptimizedDataGridCost) const
{
	double dBestCost;
	double dCost;
	KWDataGridManager dataGridManager;
	KWDataGridMerger neighbourDataGrid;
	int nIndex;
	double dNeighbourhoodSize;

	// On ne reverifie pas les precondition de la methode publique
	require(dDecreaseFactor > 1);
	require(0 <= nMinIndex);
	require(nMinIndex <= nMaxIndex);
	require(fabs(dOptimizedDataGridCost - dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid)) < dEpsilon);

	// Initialisations
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	neighbourDataGrid.SetDataGridCosts(dataGridCosts);
	dBestCost = dOptimizedDataGridCost;

	// On optimize tant qu'on ne depasse pas la taille max de voisinnage
	nVNSMaxLevel = nMaxIndex;
	nIndex = nMinIndex;
	while (nIndex <= nMaxIndex)
	{
		nVNSLevel = nIndex;
		dNeighbourhoodSize = pow(1.0 / dDecreaseFactor, nIndex);
		dVNSNeighbourhoodSize = dNeighbourhoodSize;

		// Generation d'une solution dans un voisinnage de la meilleure solution
		GenerateNeighbourSolution(initialDataGrid, optimizedDataGrid, dNeighbourhoodSize, &neighbourDataGrid);

		// Optimisation de cette solution
		dCost = OptimizeSolution(initialDataGrid, &neighbourDataGrid);

		// Si amelioration: on la memorise
		if (dCost < dBestCost - dEpsilon)
		{
			dBestCost = dCost;

			// Sauvegarde de la meilleure solution
			if (bCleanNonInformativeVariables)
				dataGridManager.CopyInformativeDataGrid(&neighbourDataGrid, optimizedDataGrid);
			else
				dataGridManager.CopyDataGrid(&neighbourDataGrid, optimizedDataGrid);

			// Gestion de la meilleure solution
			if (dataGridOptimizer != NULL)
				dataGridOptimizer->HandleOptimizationStep(optimizedDataGrid, initialDataGrid, false);
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

		// Test cas d'une unique iteration
		if (GetSlightOptimizationMode())
			break;
	}

	ensure(fabs(dBestCost - dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid)) < dEpsilon);
	return dBestCost;
}

double KWDataGridVNSOptimizer::OptimizeSolution(const KWDataGrid* initialDataGrid,
						KWDataGridMerger* dataGridMerger) const
{
	KWDataGridPostOptimizer dataGridPostOptimizer;
	double dCost;

	require(initialDataGrid != NULL);
	require(dataGridMerger != NULL);

	// Initialisations
	dataGridPostOptimizer.SetDataGridCosts(dataGridCosts);
	nVNSIteration++;

	// Affichage du cout initial
	DisplayOptimizationDetails(dataGridMerger, false);

	// Cout initial si aucune optimisation
	dCost = DBL_MAX;
	if (not optimizationParameters.GetOptimize() and not optimizationParameters.GetPreOptimize() and
	    not optimizationParameters.GetPostOptimize())
		dCost = dataGridCosts->ComputeDataGridTotalCost(dataGridMerger);

	// Pre-optimisation de la grille
	if (optimizationParameters.GetPreOptimize() and
	    not TaskProgression::IsInterruptionRequested()
	    // CH RefontePrior2-P-Inside
	    and initialDataGrid->GetAttributeNumber() > 1)
	// Fin CH RefontePrior2
	{
		dCost = dataGridPostOptimizer.PostOptimizeDataGrid(initialDataGrid, dataGridMerger, false);
	}

	// Optimisation par fusion des groupes
	if (optimizationParameters.GetOptimize() and not TaskProgression::IsInterruptionRequested())
		dCost = dataGridMerger->Merge();

	// Post-optimisation de la grille
	if (optimizationParameters.GetPostOptimize() and
	    not TaskProgression::IsInterruptionRequested()
	    // CH RefontePrior2-P-Inside
	    and initialDataGrid->GetAttributeNumber() > 1)
	// Fin CH RefontePrior2)
	{
		// Cas d'une optimisation legere (pour les granularites intermediaires en co-clustering)
		if (GetSlightOptimizationMode())
			dCost = dataGridPostOptimizer.PostOptimizeDataGrid(initialDataGrid, dataGridMerger, false);
		// Sinon
		else
			dCost = dataGridPostOptimizer.PostOptimizeDataGrid(initialDataGrid, dataGridMerger, true);
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

void KWDataGridVNSOptimizer::GenerateNeighbourSolution(const KWDataGrid* initialDataGrid,
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
	ALString sTmp;
	// CH RefontePrior2
	int nGridSize;
	// Fin CH RefontePrior2

	require(initialDataGrid != NULL);
	require(optimizedDataGrid != NULL);
	require(neighbourDataGridMerger != NULL);
	require(0 <= dNoiseRate and dNoiseRate <= 1);

	// Debut de tache
	TaskProgression::BeginTask();
	TaskProgression::DisplayMainLabel(sTmp + "New initial solution (" + DoubleToString(dNoiseRate) + ")");

	// Initialisation de la taille de la grille prise en compte
	// Cas avec granularite
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
	if (optimizationParameters.GetInternalParameter() == "LargeNeighborhoods")
	{
		nMaxContinuousPartNumber = (int)(nGridSize / 2);
		nMaxSymbolPartNumber = (int)sqrt(nGridSize * 1.0);
		nRequestedContinuousPartNumber = 1 + (int)(dNoiseRate * nMaxContinuousPartNumber);
		nRequestedSymbolPartNumber = 1 + (int)(dNoiseRate * nMaxSymbolPartNumber);
	}

	// Export d'un sous-ensemble d'attributs obligatoires (les attribut informatifs) en fonction du niveau de bruit
	nMandatoryAttributeNumber = (int)ceil((1 - dNoiseRate) * optimizedDataGrid->GetAttributeNumber());
	dataGridManager.SetSourceDataGrid(optimizedDataGrid);
	dataGridManager.ExportRandomAttributes(&mandatoryDataGrid, nMandatoryAttributeNumber);

	// Exports d'attributs supplementaires
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	neighbourDataGridMerger->DeleteAll();
	dataGridManager.AddRandomAttributes(neighbourDataGridMerger, &mandatoryDataGrid, nAttributeNumber);
	dataGridManager.AddRandomParts(neighbourDataGridMerger, optimizedDataGrid, nRequestedContinuousPartNumber,
				       nRequestedSymbolPartNumber, 1.0);
	TaskProgression::DisplayProgression(25);
	dataGridManager.ExportCells(neighbourDataGridMerger);

	// Fin de tache
	TaskProgression::EndTask();
}

void KWDataGridVNSOptimizer::DisplayOptimizationHeaderLine() const
{
	if (optimizationParameters.GetDisplayDetails())
	{
		// Lignes d'entete
		cout << "Time\tIter\tNeigh. size\t";
		cout << "Initial\t\t\t\tFinal\t\t\t\t\n";
		cout << "\t\t\tAtt. number\tPart number\tCell number\tCost\t";
		cout << "Att. number\tPart number\tCell number\tCost\n";
	}
}

void KWDataGridVNSOptimizer::DisplayOptimizationDetails(const KWDataGrid* optimizedDataGrid, boolean bOptimized) const
{
	double dVNSTime;

	// Calcul du temps ecoule
	timerVNS.Stop();
	dVNSTime = timerVNS.GetElapsedTime();
	timerVNS.Start();

	// Gestion de l'avancement avant chaque etape d'iteration
	if (not bOptimized)
	{
		ALString sTmp;
		int nTotalIterLevel;
		int nProgressionIterLevel;
		int nGranularityMax;

		if (optimizedDataGrid->GetGranularity() > 0)
			nGranularityMax = (int)ceil(log(optimizedDataGrid->GetGridFrequency() * 1.0) / log(2.0));
		else
			nGranularityMax = 0;

		// Message

		// CH RefontePrior2-G
		if (optimizedDataGrid->GetGranularity() == 0 or optimizedDataGrid->GetGranularity() == nGranularityMax)
			TaskProgression::DisplayLabel(sTmp + " VNS " + IntToString(nVNSIteration) + "  " +
						      IntToString(nVNSLevel) + "/" + IntToString(nVNSMaxLevel) + " (" +
						      DoubleToString((int)(10000 * dVNSNeighbourhoodSize) / 10000.0) +
						      ")");
		else
			TaskProgression::DisplayLabel(
			    sTmp + "Granularity " + IntToString(optimizedDataGrid->GetGranularity()) + "/" +
			    IntToString(nGranularityMax) + " VNS " + IntToString(nVNSIteration) + "  " +
			    IntToString(nVNSLevel) + "/" + IntToString(nVNSMaxLevel) + " (" +
			    DoubleToString((int)(10000 * dVNSNeighbourhoodSize) / 10000.0) + ")");

		// Niveau d'avancement
		if (optimizationParameters.GetOptimizationTime() > 0)
		{
			if (dVNSTime > optimizationParameters.GetOptimizationTime())
				TaskProgression::DisplayProgression(100);
			else
				TaskProgression::DisplayProgression(
				    (int)(dVNSTime * 100 / optimizationParameters.GetOptimizationTime()));
		}
		// cas sans limite de temps
		else
		{
			nTotalIterLevel = (int)pow(2.0, 2 + optimizationParameters.GetOptimizationLevel());
			nProgressionIterLevel = nVNSIteration;
			while (nTotalIterLevel < nProgressionIterLevel)
				nTotalIterLevel = nTotalIterLevel + (1 + nTotalIterLevel) / 2;
			TaskProgression::DisplayProgression((int)(nProgressionIterLevel * 100.0 / nTotalIterLevel));
		}
	}

	// Affichage des details d'optimisation
	if (optimizationParameters.GetDisplayDetails())
	{
		// Affichage de l'iteration
		if (not bOptimized)
			cout << dVNSTime << "\t" << nVNSIteration << "\t" << dVNSNeighbourhoodSize << "\t";

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

boolean KWDataGridVNSOptimizer::IsOptimizationTimeElapsed() const
{
	double dVNSTime;

	require(timerVNS.IsStarted());

	if (optimizationParameters.GetOptimizationTime() == 0)
		return false;
	else
	{
		// Calcul du temps ecoule
		timerVNS.Stop();
		dVNSTime = timerVNS.GetElapsedTime();
		timerVNS.Start();

		// Test de depassement
		return dVNSTime >= optimizationParameters.GetOptimizationTime();
	}
}
