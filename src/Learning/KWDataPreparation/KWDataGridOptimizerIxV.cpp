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

	ensure(fabs(dBestCost - GetDataGridCosts()->ComputeDataGridTotalCost(optimizedDataGrid)) < dEpsilon);
	return dBestCost;
}

double KWDataGridOptimizerIxV::OptimizeVarPartDataGrid(const KWDataGrid* initialDataGrid,
						       KWDataGrid* optimizedDataGrid) const
{
	const boolean bTrace = false;
	const boolean bTraceDetails = false;
	boolean bTracePartitionLevel = false;
	boolean bTracePrePartitioning = false;
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
	if (bTracePrePartitioning)
		cout << "ivMaxPartNumbers\t" << ivMaxPartNumbers;

	// Initialisation des vecteurs de nombre de parties courant et precedent
	ivPreviousPartNumber.SetSize(ivMaxPartNumbers.GetSize());
	ivCurrentPartNumber.SetSize(ivMaxPartNumbers.GetSize());

	// Export de la grille du (vrai) modele nul : un seul cluster par attribut et une seule partie de
	// variable par attribut interne
	dataGridManager.ExportNullDataGrid(initialDataGrid, &nullDataGrid);
	dBestCost = GetDataGridCosts()->ComputeDataGridTotalCost(&nullDataGrid);
	dataGridManager.CopyDataGrid(&nullDataGrid, optimizedDataGrid);
	dBestMergedCost = dBestCost;

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
		if (bTrace)
		{
			cout << "CCOptimize :partitionedDataGrid pour le pre-partitionnement " << nPrePartitionIndex
			     << endl;
			if (bTraceDetails)
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
			if (bTracePartitionLevel)
				cout << "Niveau de pre-partitionnement\t" << nPrePartitionIndex
				     << "\tNombre de parties \t" << ivCurrentPartNumber;

			// Memorisation des pre-partitionnements exploites
			ivUsedPrePartitioning.Add(nPrePartitionIndex);

			// Initialisation du modele par defaut : ce modele depend du partitionnement des
			// attributs internes
			cast(KWDataGridCosts*, GetDataGridCosts())->InitializeDefaultCosts(&partitionedDataGrid);

			// Optimisation de la grille pre-partitionnee
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
			if (bTracePartitionLevel or bTrace)
				cout << "Apres OptimizeGranularizedDataGrid pour Granularite " << nPrePartitionIndex
				     << "\t" << dPartitionBestCost << "\n";
			if (bTraceDetails)
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
				if (bTrace or bTracePartitionLevel)
				{
					cout << "CCOptimize : Grille avant fusion\t" << dPartitionBestCost << "\n";
					cout << "CCOptimize : Grille fusionnee \t" << dMergedCost << "\n";
				}
				if (bTrace)
				{
					cout << "CCOptimize : Grille avant fusion \t" << dPartitionBestCost << "\n";
					cout << "CCOptimize : Grille fusionnee  \t" << dMergedCost << "\n";
					if (bTraceDetails)
						partitionedPostMergedOptimizedDataGrid.Write(cout);
				}
			}
			else
				dMergedCost = dPartitionBestCost;

			if (dMergedCost < dBestMergedCost - dEpsilon)
			{
				dBestMergedCost = dMergedCost;
				dBestCost = dPartitionBestCost;
				if (bTrace)
					cout << "CCCoclusteringBuilder : amelioration du cout et memorisation "
						"de la grille sans post-optimisation VarPart"
					     << "\n";

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
				if (bTrace)
					cout << "CCOptimize : Mise a jour de la memorisation du coclustering "
						"pour la derniere granularite "
					     << "\n";

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
					if (bTrace)
						cout << "CCCoclusteringBuilder : memorisation d'une grille "
							"potentiellement sans post-optimisation VarPart"
						     << "\n";

					HandleOptimizationStep(optimizedDataGrid,
							       &partitionedReferencePostMergedDataGrid, true);
					if (bTrace or bTracePartitionLevel)
					{
						cout << "CCOptimize :Derniere grille apres "
							"HandleOptimizationStep de cout\t"
						     << dBestMergedCost << endl;
					}
					if (bTraceDetails)
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
					if (bTrace)
						cout << "Totalite du temps alloue ecoule apres la granularite "
							"de pre-partitionnement de l'attribut VarPart "
						     << nPrePartitionIndex << "\n";
					break;
				}
			}

			// Memorisation du nombre de parties par attribut pour comparaison a l'etape suivante
			ivPreviousPartNumber.CopyFrom(&ivCurrentPartNumber);
		}

		// Sinon : pas de traitement pour cette granularite
		else
		{
			if (bTrace)
				cout << "CCOptimize :Pre-partitionnement des attributs internes " << nPrePartitionIndex
				     << " non traite car trop proche de la precedente" << "\n";
		}

		// Nettoyage de la grille granularisee
		partitionedDataGrid.DeleteAll();

		nPrePartitionIndex++;
	}
	if (bTracePrePartitioning)
	{
		cout << "Recapitulatif des pre-partitionnements utilises " << endl;
		for (nPrePartitionIndex = 0; nPrePartitionIndex < ivUsedPrePartitioning.GetSize(); nPrePartitionIndex++)
			cout << ivUsedPrePartitioning.GetAt(nPrePartitionIndex) << endl;
	}

	// Nettoyage
	odInnerAttributesQuantileBuilders.DeleteAll();
	return dBestCost;
}

double KWDataGridOptimizerIxV::PROTO_OptimizeVarPartDataGrid(const KWDataGrid* initialDataGrid,
							     KWDataGrid* optimizedDataGrid) const
{
	boolean bTrace = false;
	boolean bTraceDetails = false;
	boolean bTraceTokenization = false;
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
	int nInstanceNumber;
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
	assert(not initialDataGrid->GetAttributeAt(0)->IsInnerAttribute());
	nInstanceNumber = initialDataGrid->GetAttributeAt(0)->GetInitialValueNumber();
	nValueNumber = initialDataGrid->GetGridFrequency();
	nMaximumInitialTotalPartNumber = nInstanceNumber;
	//DDD (ceil(sqrt(nValueNumber * log(nValueNumber * 1.0) / log(2.0))));

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
	if (bTraceTokenization)
	{
		cout << "Variables\t" << initialDataGrid->GetInnerAttributes()->GetInnerAttributeNumber() << "\n";
		cout << "Instances\t" << nInstanceNumber << "\n";
		cout << "Total variable values\t" << initialDataGrid->GetVarPartAttribute()->GetInitialValueNumber()
		     << "\n";
		cout << "Grid frequency\t" << nValueNumber << "\n";
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
		if (bTrace)
		{
			cout << "CCOptimize :partitionedDataGrid pour le pre-partitionnement "
			     << nInitialPrePartitionIndex << endl;
			if (bTraceDetails)
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
		if (bTrace or bTraceTokenization)
		{
			cout << "Apres OptimizeGranularizedDataGrid pour Granularite " << nInitialPrePartitionIndex
			     << "\t" << dBestCost << endl;
		}
		if (bTraceDetails)
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

double KWDataGridOptimizerIxV::OptimizeNeighbourSolution(const KWDataGrid* initialDataGrid,
							 const KWDataGrid* currentOptimizedDataGrid, double dNoiseRate,
							 KWDataGridMerger* neighbourOptimizedDataGrid,
							 boolean bDeepPostOptimization) const
{
	const boolean bTrace = false;
	const boolean bTraceDetails = false;
	KWDataGridManager dataGridManager;
	KWDataGrid mergedDataGrid;
	KWDataGrid partitionedReferencePostMergedDataGrid;
	KWDataGrid initialFromSurtokenizedDataGrid;
	KWDataGrid surtokenizedDataGrid;
	int nInitialCurrentTokenNumber;
	int nCurrentTokenNumber;
	int nTargetTokenNumber;
	double dCost;
	ALString sTmp;

	require(initialDataGrid != NULL);
	require(initialDataGrid->IsVarPartDataGrid());

	///////////////////////////////////////////////////////////
	// Surtokenisation d'une grille

	// Nombre de tokens de la grille en entree
	nInitialCurrentTokenNumber =
	    initialDataGrid->GetVarPartAttribute()->GetInnerAttributes()->ComputeTotalInnerAttributeVarParts();
	nCurrentTokenNumber =
	    currentOptimizedDataGrid->GetVarPartAttribute()->GetInnerAttributes()->ComputeTotalInnerAttributeVarParts();

	// Nombre de token cibles
	nTargetTokenNumber =
	    nCurrentTokenNumber + (int)(dNoiseRate * (nInitialCurrentTokenNumber - nCurrentTokenNumber));

	// Correction du nombre de tokens
	nTargetTokenNumber = min(nTargetTokenNumber, initialDataGrid->GetGridFrequency());

	// Debut du profiling de la surtokenisation
	KWDataGridOptimizer::GetProfiler()->BeginMethod("Surtokenization solution");
	KWDataGridOptimizer::GetProfiler()->WriteKeyInt("Target token number", nTargetTokenNumber);
	KWDataGridOptimizer::GetProfiler()->WriteKeyString("Initial coclustering", initialDataGrid->GetObjectLabel());
	KWDataGridOptimizer::GetProfiler()->WriteKeyString("Optimized coclustering",
							   currentOptimizedDataGrid->GetObjectLabel());
	if (bTrace)
	{
		TraceOptimizationDetails(sTmp + "SurtokenizeSolution " + IntToString(nTargetTokenNumber),
					 currentOptimizedDataGrid, bTraceDetails);
		TraceOptimizationDetails("- initial solution", initialDataGrid, bTraceDetails);
	}

	// Generation de la grille surtokenisee
	dataGridManager.ExportDataGridWithRandomizedInnerAttributes(initialDataGrid, currentOptimizedDataGrid,
								    &surtokenizedDataGrid, nTargetTokenNumber);

	// Export de la grille antecedent de la grille sur-tokenisee, c'est a dire de la grille avec une
	// chaque partie de variable dans un groupe singleton
	dataGridManager.ExportDataGridWithMergedInnerAttributes(
	    initialDataGrid, surtokenizedDataGrid.GetInnerAttributes(), &initialFromSurtokenizedDataGrid);

	// Fin du profiling de la surtokenisation
	KWDataGridOptimizer::GetProfiler()->WriteKeyString("Surtokenized coclustering",
							   surtokenizedDataGrid.GetObjectLabel());
	KWDataGridOptimizer::GetProfiler()->WriteKeyString("Initial from surtokenized coclustering",
							   initialFromSurtokenizedDataGrid.GetObjectLabel());
	KWDataGridOptimizer::GetProfiler()->EndMethod("Surtokenization solution");
	if (bTrace)
	{
		TraceOptimizationDetails("- surtokenized datagrid", &surtokenizedDataGrid, bTraceDetails);
		TraceOptimizationDetails("- initial from surtokenized datagrid", &initialFromSurtokenizedDataGrid,
					 bTraceDetails);
	}

	//////////////////////////////////////////////////////////////////////
	// Appel de la methode ancetre avec la grille sur-tokenisee

	dCost = KWDataGridOptimizer::OptimizeNeighbourSolution(&initialFromSurtokenizedDataGrid, &surtokenizedDataGrid,
							       dNoiseRate, neighbourOptimizedDataGrid,
							       bDeepPostOptimization);

	//////////////////////////////////////////////////////////////////////
	// Post-optimisation de la grille resultat

	// On utilise comme grille de reference celle qui a ete construite pour la generation de la grille voisine
	dCost = PostOptimizeVarPartSolution(&initialFromSurtokenizedDataGrid, neighbourOptimizedDataGrid);
	return dCost;
}

double KWDataGridOptimizerIxV::PostOptimizeVarPartSolution(const KWDataGrid* initialDataGrid,
							   KWDataGridMerger* optimizedDataGrid) const
{
	const boolean bTrace = false;
	const boolean bTraceDetails = false;
	double dInitialBestCost;
	double dBestCost;
	double dMergeDeltaCost;
	KWDataGridManager dataGridManager;
	KWDataGrid postOptimizedDataGrid;
	double dPostOptimizedCost;
	boolean bImprovement;
	ALString sLabel;

	// On ne reverifie pas les preconditions de la methode publique
	require(initialDataGrid != NULL);
	require(optimizedDataGrid != NULL);
	require(optimizedDataGrid->GetDataGridCosts() == GetDataGridCosts());

	// Calcul du cout initial
	dInitialBestCost = dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);
	dBestCost = dInitialBestCost;

	// Debut du profiling de la surtokenisation
	KWDataGridOptimizer::GetProfiler()->BeginMethod("PostOptimize VarPart solution");
	if (bTrace)
	{
		TraceOptimizationDetails("PostOptimizeVarPartSolution", optimizedDataGrid, bTraceDetails);
		TraceOptimizationDetails("- initial solution", initialDataGrid, bTraceDetails);
	}

	// Post-optimisation de la grille
	if (optimizedDataGrid->GetInformativeAttributeNumber() > 1 and optimizationParameters.GetVarPartPostMerge() and
	    not TaskProgression::IsInterruptionRequested())
	{
		// Fusion des parties de variable adjacentes
		dMergeDeltaCost = PostOptimizeVarPartSolutionByMergingVarParts(optimizedDataGrid);
		dBestCost += dMergeDeltaCost;
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("Coclustering", optimizedDataGrid->GetObjectLabel());
		KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Cost", dBestCost);
		if (bTrace)
			TraceOptimizationDetails("- after merging var parts", optimizedDataGrid, bTraceDetails);

		// Post-optimisation de l'attribut VarPart avec deplacement des parties de variable
		if (optimizedDataGrid->GetInformativeAttributeNumber() > 1 and
		    optimizationParameters.GetVarPartPostOptimize() and not TaskProgression::IsInterruptionRequested())
		{
			// Recopie de la grille fusionnee dans une grille de travail pour la post-optimisation
			dataGridManager.CopyDataGrid(optimizedDataGrid, &postOptimizedDataGrid);

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

				// Deplacement des parties de variable pour ameliorer le critere
				bImprovement = PostOptimizeVarPartSolutionByMovingVarParts(initialDataGrid,
											   &postOptimizedDataGrid);
				if (bImprovement)
				{
					// Fusion des parties de variable adjacentes pour la grille obtenue par deplacement des parties de variable
					dPostOptimizedCost =
					    dataGridCosts->ComputeDataGridTotalCost(&postOptimizedDataGrid);
					dMergeDeltaCost =
					    PostOptimizeVarPartSolutionByMergingVarParts(&postOptimizedDataGrid);
					dPostOptimizedCost += dMergeDeltaCost;

					// Cas ou la post-optimisation permet d'ameliorer le cout
					if (dPostOptimizedCost < dBestCost * (1 - dEpsilon))
					{
						dBestCost = dPostOptimizedCost;
						KWDataGridOptimizer::GetProfiler()->WriteKeyString(
						    "Coclustering", postOptimizedDataGrid.GetObjectLabel());
						KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Cost", dBestCost);
						if (bTrace)
							TraceOptimizationDetails("- after moving and merging var parts",
										 &postOptimizedDataGrid, bTraceDetails);

						// Recopie de la grille fusionnee dans la grille optimisee
						dataGridManager.CopyDataGrid(&postOptimizedDataGrid, optimizedDataGrid);
						assert(fabs(dBestCost - dataGridCosts->ComputeDataGridTotalCost(
									    optimizedDataGrid)) <=
						       dBestCost * dEpsilon);
					}
					// Sinon, il n'y a pas amelioration
					else
						bImprovement = false;
				}
				KWDataGridOptimizer::GetProfiler()->EndMethod("Post-optimization IV");
			}
		}
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("Coclustering", optimizedDataGrid->GetObjectLabel());
		KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Cost", dBestCost);
	}
	KWDataGridOptimizer::GetProfiler()->EndMethod("PostOptimize VarPart solution");
	ensure(fabs(dBestCost - dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid)) < dEpsilon * dBestCost);
	ensure(dBestCost <= dInitialBestCost);
	return dBestCost;
}

double KWDataGridOptimizerIxV::PostOptimizeVarPartSolutionByMergingVarParts(KWDataGrid* optimizedDataGrid) const
{
	double dMergeDeltaCost;
	KWDataGridManager dataGridManager;
	KWDataGrid postOptimizedDataGrid;

	// Creation d'une nouvelle grille avec nouvelle description des PV fusionnees
	dMergeDeltaCost = dataGridManager.ExportDataGridWithVarPartMergeOptimization(
	    optimizedDataGrid, &postOptimizedDataGrid, GetDataGridCosts());
	assert(dMergeDeltaCost <= 0);

	// Recopie si necessaire de la grille fusionnee dans la grille optimisee
	if (dMergeDeltaCost < 0)
		dataGridManager.CopyDataGrid(&postOptimizedDataGrid, optimizedDataGrid);
	return dMergeDeltaCost;
}

boolean KWDataGridOptimizerIxV::PostOptimizeVarPartSolutionByMovingVarParts(const KWDataGrid* initialDataGrid,
									    KWDataGrid* optimizedDataGrid) const
{
	boolean bImprovement;
	CCVarPartDataGridPostOptimizer varPartDataGridPostOptimizer;
	KWDataGridManager dataGridManager;
	KWDataGrid optimizedDataGridWithSingletonVarParts;
	IntVector ivGroups;
	int nGroupNumber;
	ALString sInnerAttributeName;

	require(initialDataGrid != NULL);
	require(optimizedDataGrid != NULL);
	require(initialDataGrid->GetInnerAttributes()->ContainsSubVarParts(
	    optimizedDataGrid->GetVarPartAttribute()->GetInnerAttributes()));

	// Parametrage du post-optimiseur pour l'attribut VarPart
	varPartDataGridPostOptimizer.SetPostOptimizationAttributeName(
	    optimizedDataGrid->GetVarPartAttribute()->GetAttributeName());

	// Construction d'une grille de reference avec des clusters contenant une seule
	// PV a partir des PV de la grille optimisee
	dataGridManager.ExportDataGridWithSingletonVarParts(initialDataGrid, optimizedDataGrid,
							    &optimizedDataGridWithSingletonVarParts, false);
	ivGroups.SetSize(optimizedDataGridWithSingletonVarParts.GetVarPartAttribute()->GetPartNumber());
	assert(optimizedDataGridWithSingletonVarParts.Check());
	assert(optimizedDataGridWithSingletonVarParts.GetInnerAttributes() == optimizedDataGrid->GetInnerAttributes());

	// Exploration des deplacements pour tous les attributs
	bImprovement = varPartDataGridPostOptimizer.PostOptimizeLightVarPartDataGrid(
	    &optimizedDataGridWithSingletonVarParts, optimizedDataGrid, &ivGroups);

	// Cas ou au moins un deplacement permet d'ameliorer le critere
	if (bImprovement)
	{
		// Mise a jour de la grille pour l'optimisation de cet attribut
		nGroupNumber = optimizedDataGrid->GetVarPartAttribute()->GetPartNumber();
		dataGridManager.UpdateVarPartDataGridFromVarPartGroups(&optimizedDataGridWithSingletonVarParts,
								       optimizedDataGrid, &ivGroups, nGroupNumber);
	}
	ensure(initialDataGrid->GetInnerAttributes()->ContainsSubVarParts(
	    optimizedDataGrid->GetVarPartAttribute()->GetInnerAttributes()));
	return bImprovement;
}
