// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataGridOptimizerIxV.h"

KWDataGridOptimizerIxV::KWDataGridOptimizerIxV() {}

KWDataGridOptimizerIxV::~KWDataGridOptimizerIxV() {}

double KWDataGridOptimizerIxV::InternalOptimizeDataGrid(const KWDataGrid* initialDataGrid,
							KWDataGrid* optimizedDataGrid) const

{
	double dBestCost;

	require(GetDataGridCosts() != NULL);
	require(GetDataGridCosts()->IsInitialized());
	require(initialDataGrid != NULL);
	require(initialDataGrid->IsVarPartDataGrid());
	require(optimizedDataGrid != NULL);
	require(optimizedDataGrid->GetCellNumber() == 1);
	require(GetDataGridCosts()->ComputeDataGridTotalCost(optimizedDataGrid) == GetOptimizedNullDataGridCost());

	// Recherche d'une solution initiale meilleure que celle du modele null
	dBestCost = BuildInitialSolution(initialDataGrid, optimizedDataGrid);

	// Appel direct de la methode d'optimisation VNS, dont la partie generation de grille voisone est ici specialisee
	// en generant une surtokenisation aleatoire de la grille courante
	dBestCost = IterativeVNSOptimizeDataGrid(initialDataGrid, optimizedDataGrid);

	ensure(fabs(dBestCost - GetDataGridCosts()->ComputeDataGridTotalCost(optimizedDataGrid)) < dEpsilon);
	return dBestCost;
}

double KWDataGridOptimizerIxV::BuildInitialSolution(const KWDataGrid* initialDataGrid,
						    KWDataGrid* optimizedDataGrid) const

{
	double dCost;
	double dBestCost;
	KWDataGridInitialSolutionSearcherIV initialSolutionSearcher;
	KWDataGridMerger initialDataGridSolution;

	require(GetDataGridCosts() != NULL);
	require(GetDataGridCosts()->IsInitialized());
	require(initialDataGrid != NULL);
	require(initialDataGrid->IsVarPartDataGrid());
	require(optimizedDataGrid != NULL);
	require(optimizedDataGrid->GetCellNumber() == 1);
	require(GetDataGridCosts()->ComputeDataGridTotalCost(optimizedDataGrid) == GetOptimizedNullDataGridCost());

	// Recherche d'une solution initiale meilleure que celle du modele null
	initialSolutionSearcher.SetLearningSpec(GetLearningSpec());
	initialSolutionSearcher.SearchInitialSolution(initialDataGrid, &initialDataGridSolution);

	// Prise en compte si on a trouve meilleur que le meilleur null
	dBestCost = GetOptimizedNullDataGridCost();
	if (initialDataGridSolution.GetCellNumber() > 1)
	{
		dCost = GetDataGridCosts()->ComputeDataGridTotalCost(&initialDataGridSolution);
		if (dCost < GetOptimizedNullDataGridCost())
		{
			SaveDataGrid(&initialDataGridSolution, optimizedDataGrid);
			dBestCost = dCost;
		}
	}

	ensure(fabs(dBestCost - GetDataGridCosts()->ComputeDataGridTotalCost(optimizedDataGrid)) < dEpsilon);
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
	const KWDGInnerAttributes* innerAttributes;
	int nInstanceNumber;
	int nMaximumInitialTotalPartNumber;
	int nInitialTokenNumber;
	int nCurrentTokenNumber;
	int nTargetTokenNumber;
	int nSplittableInnerVariableNumber;
	int nMinimumAddedTokenNumber;
	int nRandomAddedTokenNumber;
	int nAddedPartNumber;
	int nAddedTokenNumber;
	double dCost;
	int n;
	ALString sTmp;

	require(initialDataGrid != NULL);
	require(initialDataGrid->IsVarPartDataGrid());
	require(0 <= dNoiseRate and dNoiseRate <= 1);

	///////////////////////////////////////////////////////////
	// Surtokenisation d'une grille

	// Nombre d'instances de la grille
	assert(initialDataGrid->GetAttributeAt(0) != initialDataGrid->GetVarPartAttribute());
	nInstanceNumber = initialDataGrid->GetAttributeAt(0)->GetInitialValueNumber();
	nMaximumInitialTotalPartNumber = nInstanceNumber;

	// Nombre de tokens de la grille en entree
	nInitialTokenNumber =
	    initialDataGrid->GetVarPartAttribute()->GetInnerAttributes()->ComputeTotalInnerAttributeVarParts();
	nCurrentTokenNumber =
	    currentOptimizedDataGrid->GetVarPartAttribute()->GetInnerAttributes()->ComputeTotalInnerAttributeVarParts();

	// Comptage du nombre de variable interne que l'on peut partitionner
	// Dans le cas de tres grand nombres de variables internes, par exemple les mots d'un texte,
	// un grande partie n'est que presente ou manquante, et il n'est pas possible de les decouper
	nSplittableInnerVariableNumber = 0;
	innerAttributes = initialDataGrid->GetVarPartAttribute()->GetInnerAttributes();
	for (n = 0; n < innerAttributes->GetInnerAttributeNumber(); n++)
	{
		if (innerAttributes->GetInnerAttributeAt(n)->GetPartNumber() > 0)
			nSplittableInnerVariableNumber++;
	}

	// Initialisation d'un nombre minimum de tokens, fonction du nombre de variable internes partitionnable
	// Proche de ce nombre pour les petit nombre, decroissant avec les grand nombre
	nMinimumAddedTokenNumber =
	    (int)(sqrt(nSplittableInnerVariableNumber) * log(nSplittableInnerVariableNumber + 1) / log(2));
	nMinimumAddedTokenNumber = min(nMinimumAddedTokenNumber, nSplittableInnerVariableNumber);

	// Calcul du nombre de tokens a ajouter en fonction du nombre du nombre de parties aleatoires a ajouter
	nAddedPartNumber =
	    ComputeNeighbourSolutionAddedPartNumber(initialDataGrid, initialDataGrid->GetAttributeNumber(), dNoiseRate);

	// Calcul du nombre de token ajoutes aleatoirement
	nRandomAddedTokenNumber = int((nMaximumInitialTotalPartNumber - nCurrentTokenNumber) * dNoiseRate);

	// On ajoute des tokens de facon aleatoire en fonction de la taille du voisinnage
	nAddedTokenNumber = nAddedPartNumber + max(nRandomAddedTokenNumber, nMinimumAddedTokenNumber);
	nTargetTokenNumber = nCurrentTokenNumber + nAddedTokenNumber;

	// On borne par le nombre max de tokens exploitables
	nTargetTokenNumber = min(nTargetTokenNumber, nMaximumInitialTotalPartNumber);
	nTargetTokenNumber = min(nTargetTokenNumber, nInitialTokenNumber);
	nTargetTokenNumber = max(nTargetTokenNumber, nCurrentTokenNumber);
	assert(nTargetTokenNumber >= nCurrentTokenNumber);

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
								    nTargetTokenNumber, &surtokenizedDataGrid);

	// Export de la grille antecedent de la grille sur-tokenisee, c'est a dire de la grille avec
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
	if (neighbourOptimizedDataGrid->GetInformativeAttributeNumber() > 1)
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
	KWDataGridOptimizer::GetProfiler()->BeginMethod("Post-optimize VarPart solution");
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
		KWDataGridOptimizer::GetProfiler()->BeginMethod("Post VarPart merge");
		dMergeDeltaCost = PostOptimizeVarPartSolutionByMergingVarParts(optimizedDataGrid);
		dBestCost += dMergeDeltaCost;
		KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Delta cost", dMergeDeltaCost);
		KWDataGridOptimizer::GetProfiler()->WriteKeyString("Coclustering", optimizedDataGrid->GetObjectLabel());
		KWDataGridOptimizer::GetProfiler()->WriteKeyDouble("Cost", dBestCost);
		KWDataGridOptimizer::GetProfiler()->EndMethod("Post VarPart merge");
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
	KWDataGridOptimizer::GetProfiler()->EndMethod("Post-optimize VarPart solution");
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
	    optimizedDataGrid, GetDataGridCosts(), &postOptimizedDataGrid);
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
							    &optimizedDataGridWithSingletonVarParts);
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
								       &ivGroups, nGroupNumber, optimizedDataGrid);
	}
	ensure(initialDataGrid->GetInnerAttributes()->ContainsSubVarParts(
	    optimizedDataGrid->GetVarPartAttribute()->GetInnerAttributes()));
	return bImprovement;
}
