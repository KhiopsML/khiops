// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataGridOptimizerIxV.h"

KWDataGridOptimizerIxV::KWDataGridOptimizerIxV() {}

KWDataGridOptimizerIxV::~KWDataGridOptimizerIxV() {}

double KWDataGridOptimizerIxV::InternalOptimizeDataGrid(const KWDataGrid* initialDataGrid,
							KWDataGrid* optimizedDataGrid) const

{
	boolean bNewPROTO = false;
	double dBestCost;

	require(GetDataGridCosts() != NULL);
	require(GetDataGridCosts()->IsInitialized());
	require(initialDataGrid != NULL);
	require(initialDataGrid->IsVarPartDataGrid());
	require(optimizedDataGrid != NULL);
	require(optimizedDataGrid->GetCellNumber() == 1);
	require(GetDataGridCosts()->ComputeDataGridTotalCost(optimizedDataGrid) == GetOptimizedNullDataGridCost());

	// Test du remplacement de la methode actuelle, par son proto
	if (bNewPROTO)
		dBestCost = PROTO_OptimizeVarPartDataGrid(initialDataGrid, optimizedDataGrid);
	else
		dBestCost = OptimizeVarPartDataGrid(initialDataGrid, optimizedDataGrid);

	ensure(optimizedDataGrid->AreAttributePartsSorted() or TaskProgression::IsInterruptionRequested());
	ensure(fabs(dBestCost - GetDataGridCosts()->ComputeDataGridTotalCost(optimizedDataGrid)) < dEpsilon);
	return dBestCost;
}

double KWDataGridOptimizerIxV::OptimizeVarPartDataGrid(const KWDataGrid* initialDataGrid,
						       KWDataGrid* optimizedDataGrid) const
{
	boolean bDisplayResults = false;
	boolean bDisplayPartitionLevel = false;
	boolean bDisplayPrePartitioning = false;
	ObjectDictionary odInnerAttributesQuantileBuilders;
	KWDataGrid nullDataGrid;
	KWDataGrid partitionedDataGrid;
	KWDataGrid partitionedOptimizedDataGrid;
	KWDataGridManager dataGridManager;
	IntVector ivMaxPartNumbers;
	int nPrePartitionIndex;
	int nPrePartitionMax;
	boolean bIsLastPrePartitioning;
	double dRequiredIncreasingCoefficient;
	int nValueNumber;
	IntVector ivPreviousPartNumber;
	IntVector ivCurrentPartNumber;
	boolean bIsPrePartitioningSelected;
	double dPartitionBestCost;
	double dBestCost;
	int nInnerAttribute;
	int nAttribute;
	int nInnerAttributeCumulated;
	KWDGAttribute* varPartAttribute;
	KWDGAttribute* innerAttribute;
	KWDataGrid partitionedPostMergedOptimizedDataGrid;
	KWDataGrid partitionedReferencePostMergedDataGrid;
	double dMergedCost;
	double dBestMergedCost;
	double dFusionDeltaCost;
	IntVector ivUsedPrePartitioning;
	Timer timer;
	ALString sTmp;

	require(initialDataGrid != NULL);

	// On parcourt les differentes tailles de pre-partitionnement des attributs de type VarPart par
	// puissance de 2 Ce parcours est similaire a celui des granularites mais il porte exclusivement sur le
	// partitionnement des parties de variable des attributs internes A chaque pre-partitionnement est
	// associe un cout seuil par defaut : c'est le cout du pseudo-modele nul qui contient 1 cluster par
	// attribut avec le nombre de parties de variables du pre-partitionnement Le vrai modele nul contient un
	// cluster par attribut et une seule partie de variable par attribut interne dans l'attribut de grille
	// de type VarPart
	nValueNumber = initialDataGrid->GetGridFrequency();
	nPrePartitionMax = (int)ceil(log(nValueNumber * 1.0) / log(2.0));

	// Initialisation
	nPrePartitionIndex = 1;
	bIsLastPrePartitioning = false;

	// Initialisation du facteur d'accroissement requis entre deux pre-partitionnements
	dRequiredIncreasingCoefficient = 2;

	// Initialisation d'un quantile builder pour chaque attribut interne dans un attribut de grile de type
	// de type VarPart La grille initiale comporte un cluster par partie de variable pour ses attributs de
	// grille de type VarPart
	dataGridManager.InitializeInnerAttributesQuantileBuilders(initialDataGrid, &odInnerAttributesQuantileBuilders,
								  &ivMaxPartNumbers);
	if (bDisplayPrePartitioning)
	{
		cout << "ivMaxPartNumbers\t" << ivMaxPartNumbers;
		cout << flush;
	}

	// Initialisation des vecteurs de nombre de parties courant et precedent
	ivPreviousPartNumber.SetSize(ivMaxPartNumbers.GetSize());
	ivCurrentPartNumber.SetSize(ivMaxPartNumbers.GetSize());

	// Export de la grille du (vrai) modele nul : un seul cluster par attribut et une seule partie de
	// variable par attribut interne
	dataGridManager.ExportNullDataGrid(initialDataGrid, &nullDataGrid);
	dBestCost = GetDataGridCosts()->ComputeDataGridTotalCost(&nullDataGrid);
	dataGridManager.CopyDataGrid(&nullDataGrid, optimizedDataGrid);
	dBestMergedCost = dBestCost;

	// CH AB n'est plus necessaire c'est le cout du vrai modele nul qui doit etre utilise comme reference
	// Initialisation du meilleur cout au cout du modele nul conditionnellement au pre-partitionnement
	// Il ne s'agit donc pas ici du cout du VRAI modele nul (un seul cluster par attribut et une seule
	// partie de variable par attribut interne)
	// dBestCost = coclusteringDataGridCosts->GetTotalDefaultCost();
	// dBestMergedCost = dBestCost;
	// cout << "Cout du modele nul associe a la grille de reference\t" << dBestCost << "\n";

	// Parcours des tailles de pre-partitionnement en parties de variable
	timer.Start();
	while (nPrePartitionIndex <= nPrePartitionMax and not bIsLastPrePartitioning)
	{
		// Arret si interruption utilisateur
		if (TaskProgression::IsInterruptionRequested())
			break;

		// Pre-partitionnement des attributs internes de la grille initiale
		dataGridManager.ExportGranularizedDataGridForVarPartAttributes(
		    initialDataGrid, &partitionedDataGrid, nPrePartitionIndex, &odInnerAttributesQuantileBuilders);
		if (bDisplayResults)
		{
			cout << "CCOptimize :partitionedDataGrid pour le pre-partitionnement " << nPrePartitionIndex
			     << endl;
			partitionedDataGrid.Write(cout);
		}

		// Etude du nombre de parties des attributs internes pour decider du traitement ou non de ce
		// pre-partitionnement
		nInnerAttributeCumulated = 0;
		varPartAttribute = partitionedDataGrid.GetVarPartAttribute();
		if (varPartAttribute != NULL)
		{
			for (nInnerAttribute = 0; nInnerAttribute < varPartAttribute->GetInnerAttributeNumber();
			     nInnerAttribute++)
			{
				innerAttribute = varPartAttribute->GetInnerAttributeAt(nInnerAttribute);

				// Memorisation du nombre de parties de l'attribut interne granularise
				ivCurrentPartNumber.SetAt(nInnerAttributeCumulated, innerAttribute->GetPartNumber());
				nInnerAttributeCumulated++;
			}
		}

		// Analyse du nombre de parties par attribut interne granularise pour determiner si la grille
		// pre-partitionnee est la derniere
		bIsLastPrePartitioning = true;

		// Si on n'a pas encore atteint la granularite max pour les attributs internes partitionnes
		if (nPrePartitionIndex < nPrePartitionMax)
		{
			for (nAttribute = 0; nAttribute < ivMaxPartNumbers.GetSize(); nAttribute++)
			{
				// Cas ou le nombre de parties de l'attribut courant est inferieur au nombre max
				// de parties de l'attribut
				if (ivCurrentPartNumber.GetAt(nAttribute) < ivMaxPartNumbers.GetAt(nAttribute))
				{
					bIsLastPrePartitioning = false;
					break;
				}
			}
		}

		// CH a supprimer car granu max sur N nbre total d'observations et pas sur Nk nombre
		// d'observations par variable -> commentaire a comprendre ! Cas ou cette granularite sera la
		// derniere traitee
		if (bIsLastPrePartitioning)
			// On positionne l'index de granularite au maximum afin que l'affichage soit adapte a ce cas
			partitionedDataGrid.GetEditableInnerAttributes()->SetVarPartGranularity(nPrePartitionMax);

		// Analyse du nombre de parties par attribut interne granularise pour determiner si la grille
		// pre-partitionnee sera optimise Il faut pour cela qu'elle soit suffisamment differente de la
		// grille analysee precedemment
		bIsPrePartitioningSelected = false;
		for (nAttribute = 0; nAttribute < ivCurrentPartNumber.GetSize(); nAttribute++)
		{
			// Cas d'accroissement suffisant du nombre de parties : le cas d'un attribut suffit pour
			// justifier le traitement de cette granularite
			if ((ivCurrentPartNumber.GetAt(nAttribute) >=
			     ivPreviousPartNumber.GetAt(nAttribute) * dRequiredIncreasingCoefficient) and
			    (ivCurrentPartNumber.GetAt(nAttribute) * dRequiredIncreasingCoefficient <=
			     ivMaxPartNumbers.GetAt(nAttribute)))
			{
				bIsPrePartitioningSelected = true;
				break;
			}
		}

		// On ne traite pas les grilles avec un seul attribut informatif
		if (partitionedDataGrid.GetInformativeAttributeNumber() <= 1)
			bIsPrePartitioningSelected = false;

		// Cas du traitement de la granularite courante
		if (bIsPrePartitioningSelected or bIsLastPrePartitioning)
		{
			// Affichage du niveau de pre-partitionnement et du nombre de parties associe
			if (bDisplayPartitionLevel)
				cout << "Niveau de pre-partitionnement\t" << nPrePartitionIndex
				     << "\tNombre de parties \t" << ivCurrentPartNumber;

			// Memorisation des pre-partitionnements exploites
			ivUsedPrePartitioning.Add(nPrePartitionIndex);

			// Initialisation du modele par defaut : ce modele depend du partitionnement des
			// attributs internes
			cast(KWDataGridCosts*, GetDataGridCosts())->InitializeDefaultCosts(&partitionedDataGrid);

			// Optimisation de la grille pre-partitionnee
			// Le cout dPartitionBestCost est le cout de la grille antecedente de la meilleure
			// grille post-fusionnee (fusion des parties de variables consecutives dans un meme
			// cluster)
			KWDataGridOptimizer::GetProfiler()->BeginMethod("Optimize VarPart prepartition");
			KWDataGridOptimizer::GetProfiler()->WriteKeyInt("Pre-partition index", nPrePartitionIndex);

			// Utilisation du modele nul pour la solution initiale
			SaveDataGrid(GetOptimizedNullDataGrid(), &partitionedOptimizedDataGrid);
			dPartitionBestCost = GetOptimizedNullDataGridCost();

			// Optimisation a partir d'une grille initiale complete si algorithme glouton
			dPartitionBestCost =
			    IterativeVNSOptimizeDataGrid(&partitionedDataGrid, &partitionedOptimizedDataGrid);
			KWDataGridOptimizer::GetProfiler()->EndMethod("Optimize VarPart prepartition");

			// Affichage
			if (bDisplayPartitionLevel or bDisplayResults)
				cout << "Apres OptimizeGranularizedDataGrid pour Granularite " << nPrePartitionIndex
				     << "\t" << dPartitionBestCost << endl;
			if (bDisplayResults)
			{
				partitionedOptimizedDataGrid.WriteAttributes(cout);
				partitionedOptimizedDataGrid.WriteAttributeParts(cout);
			}

			// Utilisation d'une grille post-mergee partitionedPostMergedOptimizedDataGrid
			// pour cette granularite de pre-partitionnement
			// CH AF Il faudrait aussi recalculer la post-optimisation VarPart de la grille
			// post-mergee mais de toute facon cette meilleure grille aura ete memorisee avec
			// HandleOptimisationStep dans l'algo VNS -> commentaire a comprendre

			if (partitionedOptimizedDataGrid.GetInformativeAttributeNumber() > 0 and
			    optimizationParameters.GetVarPartPostMerge())
			{
				// Creation d'une nouvelle grille avec nouvelle description des PV et calcul de
				// la variation de cout liee a la fusion des PV
				dFusionDeltaCost = dataGridManager.ExportDataGridWithVarPartMergeOptimization(
				    &partitionedOptimizedDataGrid, &partitionedPostMergedOptimizedDataGrid,
				    GetDataGridCosts());

				// Calcul et verification du cout
				dMergedCost = dPartitionBestCost + dFusionDeltaCost;
				// Le cout precedent devra etre correct
				assert(dMergedCost * (1 - dEpsilon) < GetDataGridCosts()->ComputeDataGridTotalCost(
									  &partitionedPostMergedOptimizedDataGrid));
				assert(GetDataGridCosts()->ComputeDataGridTotalCost(
					   &partitionedPostMergedOptimizedDataGrid) < dMergedCost * (1 + dEpsilon));
				if (bDisplayResults or bDisplayPartitionLevel)
				{
					cout << "CCOptimize : Grille avant fusion\t" << dPartitionBestCost << "\n";
					cout << "CCOptimize : Grille fusionnee \t" << dMergedCost << "\n";
				}
				if (bDisplayResults)
				{
					cout << "CCOptimize : Grille avant fusion \t" << dPartitionBestCost << "\n";
					cout << "CCOptimize : Grille fusionnee  \t" << dMergedCost << "\n";
					partitionedPostMergedOptimizedDataGrid.Write(cout);
					cout << flush;
				}
			}
			else
				dMergedCost = dPartitionBestCost;

			if (dMergedCost < dBestMergedCost - dEpsilon)
			{
				dBestMergedCost = dMergedCost;
				dBestCost = dPartitionBestCost;
				if (bDisplayResults)
					cout << "CCCoclusteringBuilder : amelioration du cout et memorisation "
						"de la grille sans post-optimisation VarPart"
					     << endl;

				// Memorisation de l'optimum post-fusionne
				if (partitionedOptimizedDataGrid.GetInformativeAttributeNumber() > 0 and
				    optimizationParameters.GetVarPartPostMerge())
				{
					dataGridManager.CopyDataGrid(&partitionedPostMergedOptimizedDataGrid,
								     optimizedDataGrid);
				}
				else
				{
					dataGridManager.CopyDataGrid(&partitionedOptimizedDataGrid, optimizedDataGrid);
				}
			}

			// Cas ou il s'agit de la derniere granularite : on met a jour les infos du coclustering
			if (bIsLastPrePartitioning)
			{
				if (bDisplayResults)
					cout << "CCOptimize : Mise a jour de la memorisation du coclustering "
						"pour la derniere granularite "
					     << endl;

				if (optimizedDataGrid->GetInformativeAttributeNumber() > 0)
				{
					// Construction d'une grille initiale compatible avec les parties
					// de variables fusionnees au niveau des attributs internes
					// Necessaire pour la memorisation de la grille post-mergee
					// La grille source contient des clusters mono-parties de variables,
					// avec des PV issues du pre-partitionnement
					// La grille optimisee contient des clusters de PV,avec des PV
					// eventuellement issues d'une fusion des PV de la grille source
					// On construit une grille qui contient des clusters mono-PV avec
					// les PV issues de la fusion de la grille optimisee
					dataGridManager.ExportDataGridWithSingletonVarParts(
					    initialDataGrid, optimizedDataGrid, &partitionedReferencePostMergedDataGrid,
					    true);
					if (bDisplayResults)
						cout << "CCCoclusteringBuilder : memorisation d'une grille "
							"potentiellement sans post-optimisation VarPart"
						     << endl;

					HandleOptimizationStep(optimizedDataGrid,
							       &partitionedReferencePostMergedDataGrid, true);
					if (bDisplayResults or bDisplayPartitionLevel)
					{
						cout << "CCOptimize :Derniere grille apres "
							"HandleOptimizationStep de cout\t"
						     << dBestMergedCost << endl;
					}
					if (bDisplayResults)
					{
						cout << "CCOptimize :partitionedReferencePostMergedDataGrid" << endl;
						partitionedReferencePostMergedDataGrid.Write(cout);
						optimizedDataGrid->Write(cout);
					}
				}
				else
				{
					HandleOptimizationStep(optimizedDataGrid, &partitionedDataGrid, true);
				}
			}

			// Nettoyage
			partitionedPostMergedOptimizedDataGrid.DeleteAll();
			partitionedReferencePostMergedDataGrid.DeleteAll();

			// Nettoyage de la grille optimisee pour cette granularite
			partitionedOptimizedDataGrid.DeleteAll();

			// Cas d'un temps limite : mise a jour du temps restant par retrait du temps consacre a
			// cette granularite
			if (optimizationParameters.GetOptimizationTime() > 0)
			{
				// L'utilisation de la totalite du temps global alloue (OptimizationTime) peut
				// conduire a l'arret du parcours des granularites et nuire a la recherche de la
				// grille optimale
				if (optimizationParameters.GetOptimizationTime() - timer.GetElapsedTime() < 0)
				{
					// Affichage d'un warning pour eventuelle modification de l'optimisation
					// time
					AddWarning(sTmp +
						   "All the optimization time has been used but maximum "
						   "tokenization has not been reached: " +
						   IntToString(nPrePartitionIndex) + " on " +
						   IntToString(nPrePartitionMax) +
						   ". You could obtain better results with greater "
						   "optimization time.");
					if (bDisplayResults)
						cout << "Totalite du temps alloue ecoule apres la granularite "
							"de pre-partitionnement de l'attribut VarPart "
						     << nPrePartitionIndex << endl;
					break;
				}
			}

			// Memorisation du nombre de parties par attribut pour comparaison a l'etape suivante
			ivPreviousPartNumber.CopyFrom(&ivCurrentPartNumber);
		}

		// Sinon : pas de traitement pour cette granularite
		else
		{
			if (bDisplayResults)
				cout << "CCOptimize :Pre-partitionnement des attributs internes " << nPrePartitionIndex
				     << " non traite car trop proche de la precedente" << endl;
		}

		// Nettoyage de la grille granularisee
		partitionedDataGrid.DeleteAll();

		nPrePartitionIndex++;
	}
	if (bDisplayPrePartitioning)
	{
		cout << "Recapitulatif des pre-partitionnements utilises " << endl;
		for (nPrePartitionIndex = 0; nPrePartitionIndex < ivUsedPrePartitioning.GetSize(); nPrePartitionIndex++)
			cout << ivUsedPrePartitioning.GetAt(nPrePartitionIndex) << endl;
	}

	// Nettoyage
	odInnerAttributesQuantileBuilders.DeleteAll();

	// CH IV probleme est ce que le code qui suit est a conserver
	//  Cas ou la grille terminale n'est pas ameliorable else
	//{
	// Tri des parties par attribut, pour preparer les affichages de resultats
	// ainsi que les resultats de preparation des donnees
	// optimizedDataGrid->SortAttributeParts();
	//}
	//}
	return dBestCost;
}

double KWDataGridOptimizerIxV::PROTO_OptimizeVarPartDataGrid(const KWDataGrid* initialDataGrid,
							     KWDataGrid* optimizedDataGrid) const
{
	boolean bDisplayResults = false;
	boolean bDisplayTokenization = false;
	ObjectDictionary odInnerAttributesQuantileBuilders;
	IntVector ivGranularityTotalPartNumbers;
	int nMaximumInitialTotalPartNumber;
	int nInitialPrePartitionIndex;
	KWDataGrid nullDataGrid;
	KWDataGrid partitionedDataGrid;
	KWDataGrid partitionedOptimizedDataGrid;
	KWDataGridManager dataGridManager;
	IntVector ivMaxPartNumbers;
	int i;
	int nValueNumber;
	double dBestCost;
	KWDataGrid partitionedPostMergedOptimizedDataGrid;
	KWDataGrid partitionedReferencePostMergedDataGrid;
	IntVector ivUsedPrePartitioning;
	ALString sTmp;

	require(initialDataGrid != NULL);

	// Initialisation d'un quantile builder pour chaque attribut interne dans un attribut de grile de type
	// de type VarPart La grille initiale comporte un cluster par partie de variable pour ses attributs de
	// grille de type VarPart
	dataGridManager.InitializeInnerAttributesQuantileBuilders(initialDataGrid, &odInnerAttributesQuantileBuilders,
								  &ivMaxPartNumbers);

	// Calcul des nombre totaux de partiles pour chaque granularite potentielles
	dataGridManager.ComputeGranularizedTotalPartNumbers(&odInnerAttributesQuantileBuilders,
							    &ivGranularityTotalPartNumbers);

	// Estimation du nombre total de partie maximal a utiliser pour la solution initiale
	//DDD Heuristique a ajuster
	nValueNumber = initialDataGrid->GetGridFrequency();
	nMaximumInitialTotalPartNumber = int(ceil(sqrt(nValueNumber * log(nValueNumber * 1.0) / log(2.0))));

	// Recherche du niveau de tokenisation maximal permettant de ne pas depasser ce total
	nInitialPrePartitionIndex = 0;
	for (i = 0; i < ivGranularityTotalPartNumbers.GetSize(); i++)
	{
		if (ivGranularityTotalPartNumbers.GetAt(i) <= nMaximumInitialTotalPartNumber)
			nInitialPrePartitionIndex = i;
		else
			break;
	}

	// Affichage des informations de tokenization
	if (bDisplayTokenization)
	{
		cout << "MaxTokenNumbers\t" << ivMaxPartNumbers;
		cout << "GranularityTotalTokenNumbers\t" << ivGranularityTotalPartNumbers;
		cout << "InitialPrePartitionIndex\t" << nInitialPrePartitionIndex << "\n";
		cout << "InitialTotalTokenNumber\t" << ivGranularityTotalPartNumbers.GetAt(nInitialPrePartitionIndex)
		     << endl;
	}

	// Export de la grille du (vrai) modele nul : un seul cluster par attribut et une seule partie de
	// variable par attribut interne
	dataGridManager.ExportNullDataGrid(initialDataGrid, &nullDataGrid);
	dBestCost = GetDataGridCosts()->ComputeDataGridTotalCost(&nullDataGrid);
	dataGridManager.CopyDataGrid(&nullDataGrid, optimizedDataGrid);

	// Traitement s'il n'y a pas d'interruption utilisateur
	if (not TaskProgression::IsInterruptionRequested())
	{
		// Pre-partitionnement des attributs internes de la grille initiale
		dataGridManager.ExportGranularizedDataGridForVarPartAttributes(initialDataGrid, &partitionedDataGrid,
									       nInitialPrePartitionIndex,
									       &odInnerAttributesQuantileBuilders);
		assert(partitionedDataGrid.GetInformativeAttributeNumber() > 1);
		if (bDisplayResults)
		{
			cout << "CCOptimize :partitionedDataGrid pour le pre-partitionnement "
			     << nInitialPrePartitionIndex << endl;
			partitionedDataGrid.Write(cout);
		}

		// Optimisation de la grille pre-partitionnee
		KWDataGridOptimizer::GetProfiler()->BeginMethod("Optimize VarPart prepartition");
		KWDataGridOptimizer::GetProfiler()->WriteKeyInt("Pre-partition index", nInitialPrePartitionIndex);

		// Utilisation du modele nul pour la solution initiale
		SaveDataGrid(GetOptimizedNullDataGrid(), optimizedDataGrid);
		dBestCost = GetOptimizedNullDataGridCost();

		// Optimisation a partir d'une grille initiale complete si algorithme glouton
		dBestCost = IterativeVNSOptimizeDataGrid(&partitionedDataGrid, optimizedDataGrid);
		KWDataGridOptimizer::GetProfiler()->EndMethod("Optimize VarPart prepartition");

		// Affichage
		if (bDisplayResults or bDisplayTokenization)
		{
			cout << "Apres OptimizeGranularizedDataGrid pour Granularite " << nInitialPrePartitionIndex
			     << "\t" << dBestCost << endl;
		}
		if (bDisplayResults)
		{
			optimizedDataGrid->WriteAttributes(cout);
			optimizedDataGrid->WriteAttributeParts(cout);
		}
	}

	// Nettoyage de la grille granularisee
	partitionedDataGrid.DeleteAll();

	// Nettoyage
	odInnerAttributesQuantileBuilders.DeleteAll();
	return dBestCost;
}
