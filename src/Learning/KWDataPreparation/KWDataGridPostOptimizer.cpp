// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDataGridPostOptimizer.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridPostOptimizer

KWDataGridPostOptimizer::KWDataGridPostOptimizer()
{
	dataGridCosts = NULL;
	dEpsilon = 1e-10;
}

KWDataGridPostOptimizer::~KWDataGridPostOptimizer() {}

void KWDataGridPostOptimizer::SetDataGridCosts(const KWDataGridCosts* kwdgcCosts)
{
	dataGridCosts = kwdgcCosts;
}

const KWDataGridCosts* KWDataGridPostOptimizer::GetDataGridCosts() const
{
	return dataGridCosts;
}

double KWDataGridPostOptimizer::PostOptimizeDataGrid(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid,
						     boolean bDeepPostOptimization) const
{
	boolean bDisplayResults = false;
	double dBestCost;
	double dCost;
	boolean bImproved;
	KWDataGridManager dataGridManager;
	KWDGPODiscretizer dataGridUnivariateDiscretizer;
	KWDGPOGrouper dataGridUnivariateGrouper;
	KWDataGrid* univariateInitialDataGrid;
	IntVector ivAttributeIndexes;
	int nI;
	int nAttribute;
	KWDGAttribute* dataGridAttribute;
	int nMaxStepNumber;
	int nStepNumber;
	ALString sTaskLabel;

	require(optimizedDataGrid != NULL);
	require(initialDataGrid != NULL);
	require(optimizedDataGrid->Check());
	require(initialDataGrid->Check());

	// Debut de tache
	TaskProgression::BeginTask();
	if (bDeepPostOptimization)
		sTaskLabel = "Data grid local optimisation";
	else
		sTaskLabel = "Data grid fast local optimisation";
	TaskProgression::DisplayMainLabel(sTaskLabel);

	// Verification de la compatibilite entre grille optimisee et grille initiale
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	require(dataGridManager.CheckDataGrid(optimizedDataGrid));

	// Calcul du cout initial
	dBestCost = dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid);

	// Affichage de la grille initiale avec ses couts
	if (bDisplayResults)
	{
		cout << "\nPost-optimisation: grille initiale" << sTaskLabel << endl;

		if (optimizedDataGrid->GetAttributeNumber() == 2)
			optimizedDataGrid->WriteCrossTableStats(cout, 0);
		dataGridCosts->WriteDataGridAllCosts(optimizedDataGrid, cout);
		cout << endl;
	}

	// Affichage des resultats initiaux
	if (bDisplayResults)
	{
		cout << "Local optimisation" << endl;
		cout << "Initial\t" << dBestCost << "\t" << optimizedDataGrid->GetObjectLabel() << endl;
	}

	// Creation d'un tableau des index des attributs
	ivAttributeIndexes.SetSize(optimizedDataGrid->GetAttributeNumber());
	for (nI = 0; nI < ivAttributeIndexes.GetSize(); nI++)
		ivAttributeIndexes.SetAt(nI, nI);

	// Permutation aleatoire de l'ordre des attributs
	ivAttributeIndexes.Shuffle();

	// Nombre max d'etapes
	if (bDeepPostOptimization)
		nMaxStepNumber = (int)log(initialDataGrid->GetGridFrequency() * 1.0);
	else
		nMaxStepNumber = 4;

	// Optimisation tant que amelioration sur au moins un attribut
	bImproved = true;
	nStepNumber = 0;
	while (bImproved and nStepNumber < nMaxStepNumber)
	{
		bImproved = false;
		nStepNumber++;

		// Test si arret demande
		if (TaskProgression::IsInterruptionRequested())
			break;

		// Permutation aleatoire de l'ordre des attributs
		// (inutile et couteux si seulement deux attributs)
		if (ivAttributeIndexes.GetSize() > 2)
			ivAttributeIndexes.Shuffle();

		// Optimisation sur les attributs
		for (nI = 0; nI < ivAttributeIndexes.GetSize(); nI++)
		{
			nAttribute = ivAttributeIndexes.GetAt(nI);
			dataGridAttribute = optimizedDataGrid->GetAttributeAt(nAttribute);

			// Niveau d'avancement de la tache
			if (TaskProgression::IsInterruptionRequested())
				break;
			TaskProgression::DisplayProgression(
			    (nI + 1 + (nStepNumber - 1) * ivAttributeIndexes.GetSize()) * 100 /
			    (nMaxStepNumber * ivAttributeIndexes.GetSize()));
			TaskProgression::DisplayMainLabel(sTaskLabel);
			TaskProgression::DisplayLabel(dataGridAttribute->GetAttributeName() + " (" +
						      IntToString(nStepNumber) + "/" + IntToString(nMaxStepNumber) +
						      ")");

			// Construction d'une grille initiale pour l'optimisation univariee
			univariateInitialDataGrid = BuildUnivariateInitialDataGrid(
			    optimizedDataGrid, initialDataGrid, dataGridAttribute->GetAttributeName());

			// Discretisation univariee si attribut continu
			dCost = 0;
			if (dataGridAttribute->GetAttributeType() == KWType::Continuous)
			{
				dataGridUnivariateDiscretizer.SetPostOptimizationAttributeName(
				    dataGridAttribute->GetAttributeName());
				dCost = dataGridUnivariateDiscretizer.PostOptimizeDataGrid(
				    univariateInitialDataGrid, dataGridCosts, optimizedDataGrid, bDeepPostOptimization);
			}
			// Groupement de valeur univariee si attribut groupable
			else
			{
				assert(KWType::IsCoclusteringGroupableType(dataGridAttribute->GetAttributeType()));

				dataGridUnivariateGrouper.SetPostOptimizationAttributeName(
				    dataGridAttribute->GetAttributeName());
				dCost = dataGridUnivariateGrouper.PostOptimizeDataGrid(
				    univariateInitialDataGrid, dataGridCosts, optimizedDataGrid, bDeepPostOptimization);
			}

			// Le cout precedent devra etre correct
			assert(dCost * (1 - dEpsilon) < dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid));
			assert(dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid) < dCost * (1 + dEpsilon));

			// Nettoyage
			delete univariateInitialDataGrid;

			// Memorisation de l'amelioration
			assert(dCost <= dBestCost * (1 + dEpsilon));
			if (dCost < dBestCost * (1 - dEpsilon))
			{
				dBestCost = dCost;
				bImproved = true;
			}

			// Affichage de l'amelioration
			if (bDisplayResults)
			{
				cout << dataGridAttribute->GetAttributeName() << "\t" << dCost << "\t" << bImproved
				     << "\t" << optimizedDataGrid->GetObjectLabel() << endl;
			}
		}
	}

	// Affichage de la grille finale avec ses couts
	if (bDisplayResults)
	{
		cout << "Local optimisation: grille finale apres " << nStepNumber << " etapes d'amelioration " << endl;
		if (optimizedDataGrid->GetAttributeNumber() == 2)
			optimizedDataGrid->WriteCrossTableStats(cout, 0);
		dataGridCosts->WriteDataGridAllCosts(optimizedDataGrid, cout);
		cout << endl;
	}

	// Fin de tache
	TaskProgression::EndTask();

	// Verification de la compatibilite entre grille optimisee et grille initiale
	ensure(dataGridManager.CheckDataGrid(optimizedDataGrid));
	ensure(dBestCost * (1 - dEpsilon) < dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid));
	ensure(dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid) < dBestCost * (1 + dEpsilon));

	return dBestCost;
}

KWDataGrid*
KWDataGridPostOptimizer::BuildUnivariateInitialDataGrid(const KWDataGrid* optimizedDataGrid,
							const KWDataGrid* initialDataGrid,
							const ALString& sPostOptimizationAttributeName) const
{
	boolean bDisplayResults = false;
	KWDataGrid* univariateInitialDataGrid;
	KWDataGridManager dataGridManager;
	KWDGAttribute* postOptimizationAttribute;

	require(optimizedDataGrid != NULL);
	require(initialDataGrid != NULL);
	require(optimizedDataGrid->SearchAttribute(sPostOptimizationAttributeName) != NULL);
	require(initialDataGrid->SearchAttribute(sPostOptimizationAttributeName) != NULL);

	// CH IV Begin
	// Extension pour un attribut de type VarPart : code identique
	// Il faut une coherence (egalite) entre le KWDGInnerAttributes utilise par la grille optimisee et celui utilise
	// pour l'attribut de la grille initiale
	// (tolerance dans le cas d'une grille hierarchique qui exploite un clone des inner attributes)
	require(not initialDataGrid->IsVarPartDataGrid() or
		initialDataGrid->GetInnerAttributes() == optimizedDataGrid->GetInnerAttributes() or
		optimizedDataGrid->GetClassLabel() == "Hierarchichal data grid");
	// CH IV End

	// Creation de la grille univariee
	univariateInitialDataGrid = new KWDataGrid;

	// Export des attributs et des parties de la grille optimisee
	dataGridManager.SetSourceDataGrid(optimizedDataGrid);
	dataGridManager.ExportAttributes(univariateInitialDataGrid);
	dataGridManager.ExportParts(univariateInitialDataGrid);

	// On reinitialise a vide les partie pour l'attribut a post-optimiser
	postOptimizationAttribute = univariateInitialDataGrid->SearchAttribute(sPostOptimizationAttributeName);
	postOptimizationAttribute->DeleteAllParts();

	// Export des parties les plus fines (de la grille initiale) pour la grille a optimiser
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	dataGridManager.ExportAttributeParts(univariateInitialDataGrid, sPostOptimizationAttributeName);

	// Export des cellules pour la grille initiale univariee
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	dataGridManager.ExportCells(univariateInitialDataGrid);

	// Affichage des resultats
	if (bDisplayResults)
	{
		cout << "Preparation d'une grille pour l'optimisation univariee\t" << sPostOptimizationAttributeName
		     << endl;
		cout << "Grille initiale\n" << *initialDataGrid << endl;
		cout << "Grille optimisee\n" << *optimizedDataGrid << endl;
		cout << "Grille preparee\n" << *univariateInitialDataGrid << endl;
	}

	// Retour de la grille preparee
	ensure(univariateInitialDataGrid->GetAttributeNumber() == optimizedDataGrid->GetAttributeNumber());
	ensure(univariateInitialDataGrid->GetTotalPartNumber() ==
	       optimizedDataGrid->GetTotalPartNumber() +
		   initialDataGrid->SearchAttribute(sPostOptimizationAttributeName)->GetPartNumber() -
		   optimizedDataGrid->SearchAttribute(sPostOptimizationAttributeName)->GetPartNumber());
	ensure(univariateInitialDataGrid->GetCellNumber() >= optimizedDataGrid->GetCellNumber());
	return univariateInitialDataGrid;
}

////////////////////////////////////////////////////////////////////////////////////
// Classe KWDGPODiscretizer

KWDGPODiscretizer::KWDGPODiscretizer()
{
	// Redefinition de la structure des cout de la discretisation
	SetDiscretizationCosts(new KWDataGridUnivariateCosts);
}

KWDGPODiscretizer::~KWDGPODiscretizer()
{
	// Le destructeur de la classe ancetre detruit le creator de vecteur d'effectif
}

void KWDGPODiscretizer::SetPostOptimizationAttributeName(const ALString& sValue)
{
	sPostOptimizationAttributeName = sValue;
}

const ALString& KWDGPODiscretizer::GetPostOptimizationAttributeName() const
{
	return sPostOptimizationAttributeName;
}

double KWDGPODiscretizer::PostOptimizeDataGrid(const KWDataGrid* initialDataGrid, const KWDataGridCosts* dataGridCosts,
					       KWDataGrid* optimizedDataGrid, boolean bDeepPostOptimization) const
{
	boolean bDisplayResults = false;
	double dCost;
	KWDataGridUnivariateCosts* dataGridUnivariateCosts;
	KWDataGridManager dataGridManager;
	KWFrequencyTable initialFrequencyTable;
	double dOptimizedCost;
	KWMODLLineDeepOptimization lineDeepOptimizationCreator(GetFrequencyVectorCreator());
	KWMODLLineDeepOptimization* headIntervalOptimization;
	double dUniqueIntervalCost;
	KWMODLLine lineCreator(GetFrequencyVectorCreator());
	KWMODLLine* headUniqueInterval;

	require(optimizedDataGrid != NULL);
	require(initialDataGrid != NULL);
	require(dataGridCosts != NULL);
	require(optimizedDataGrid->Check());
	require(initialDataGrid->Check());
	require(optimizedDataGrid->SearchAttribute(GetPostOptimizationAttributeName()) != NULL);
	require(initialDataGrid->SearchAttribute(GetPostOptimizationAttributeName()) != NULL);
	require(optimizedDataGrid->GetTotalPartNumber() -
		    optimizedDataGrid->SearchAttribute(GetPostOptimizationAttributeName())->GetPartNumber() ==
		initialDataGrid->GetTotalPartNumber() -
		    initialDataGrid->SearchAttribute(GetPostOptimizationAttributeName())->GetPartNumber());

	// Affichage de la grille initiale avec ses couts
	if (bDisplayResults)
	{
		cout << "\nPost-optimisation (discretisation): grille initiale" << endl;
		cout << "\tAttribut optimise: " << GetPostOptimizationAttributeName() << endl;

		if (optimizedDataGrid->GetAttributeNumber() == 2)
			optimizedDataGrid->WriteCrossTableStats(cout, 0);
		dataGridCosts->WriteDataGridAllCosts(optimizedDataGrid, cout);
		cout << endl;
	}

	// Verification de la compatibilite entre grille optimisee et grille initiale
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	require(dataGridManager.CheckDataGrid(optimizedDataGrid));

	// Parametrage des couts d'optimisation univarie de la grille
	dataGridUnivariateCosts = cast(KWDataGridUnivariateCosts*, GetDiscretizationCosts());
	dataGridUnivariateCosts->SetPostOptimizationAttributeName(GetPostOptimizationAttributeName());
	dataGridUnivariateCosts->SetDataGridCosts(dataGridCosts);
	dataGridUnivariateCosts->InitializeUnivariateCostParameters(optimizedDataGrid);

	// Construction d'une table d'effectifs selon l'attribut a post-optimiser, pour la grille initiale
	InitializeFrequencyTableFromDataGrid(&initialFrequencyTable, initialDataGrid);

	// Construction de la liste des intervalles a partir de la table initiale et des index
	// de fin d'intervalle de la grille optimisee
	headIntervalOptimization = cast(KWMODLLineDeepOptimization*,
					BuildIntervalListFromFrequencyTableAndOptimizedDataGrid(
					    &lineDeepOptimizationCreator, &initialFrequencyTable, optimizedDataGrid));

	// Post-optimisation intensive
	if (bDeepPostOptimization)
	{
		// Post-optimisation basee sur une recherche locale des ameliorations
		// baseee sur des MergeSplit, des Split et des MergeMergeSplit
		IntervalListPostOptimization(&initialFrequencyTable, headIntervalOptimization);

		// Calcul du cout en se basant sur l'evaluation univariee
		dOptimizedCost =
		    ComputeIntervalListPartitionGlobalCost(headIntervalOptimization, &initialFrequencyTable);

		// Construction d'un intervalle unique, fusion de toutes les lignes de la table initiale
		// (pour l'attribut a optimiser)
		headUniqueInterval = BuildUniqueIntervalFromFrequencyTable(&lineCreator, &initialFrequencyTable);

		// Cout dans le cas d'un intervalle unique
		dUniqueIntervalCost =
		    ComputeIntervalListPartitionGlobalCost(headUniqueInterval, &initialFrequencyTable);

		// Mise a jour de la grille a optimiser en fonction du meilleur cout
		dCost = DBL_MAX;
		if (dUniqueIntervalCost < dOptimizedCost + dEpsilon)
		{
			dCost = dUniqueIntervalCost;
			UpdateDataGridFromIntervalList(optimizedDataGrid, initialDataGrid, headUniqueInterval);
		}
		else
		{
			dCost = dOptimizedCost;
			UpdateDataGridFromIntervalList(optimizedDataGrid, initialDataGrid, headIntervalOptimization);
		}

		// Nettoyage
		DeleteIntervalList(headIntervalOptimization);
		DeleteIntervalList(headUniqueInterval);
	}
	// Post-optimisation legere
	else
	{
		// Post-optimisation par deplacement de bornes
		IntervalListBoundaryPostOptimization(&initialFrequencyTable, headIntervalOptimization);

		// Calcul du cout en se basant sur l'evaluation univariee
		dOptimizedCost =
		    ComputeIntervalListPartitionGlobalCost(headIntervalOptimization, &initialFrequencyTable);

		// Mise a jour de la grille a optimiser en fonction du meilleur cout
		dCost = dOptimizedCost;
		UpdateDataGridFromIntervalList(optimizedDataGrid, initialDataGrid, headIntervalOptimization);

		// Nettoyage
		DeleteIntervalList(headIntervalOptimization);
	}

	// Affichage de la grille finale avec ses couts
	if (bDisplayResults)
	{
		cout << "Post-optimisation (discretisation): grille finale" << endl;
		if (optimizedDataGrid->GetAttributeNumber() == 2)
			optimizedDataGrid->WriteCrossTableStats(cout, 0);
		dataGridCosts->WriteDataGridAllCosts(optimizedDataGrid, cout);
		cout << endl;
	}

	// Verification du cout avec l'evaluation multi-variee
	assert(fabs(dCost - dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid)) < dEpsilon);

	// Verification de la compatibilite entre grille optimisee et grille initiale
	ensure(dataGridManager.CheckDataGrid(optimizedDataGrid));

	return dCost;
}

void KWDGPODiscretizer::InitializeFrequencyTableFromDataGrid(KWFrequencyTable* kwftFrequencyTable,
							     const KWDataGrid* dataGrid) const
{
	boolean bDisplayResults = false;
	NumericKeyDictionary nkdHashCells;
	KWDGAttribute* dataGridAttribute;
	KWDGPart* dataGridPart;
	KWDGPOPartFrequencyVector* partFrequencyVector;
	int nInterval;
	Continuous cPreviousIntervalUpperBound;

	require(kwftFrequencyTable != NULL);
	require(kwftFrequencyTable->GetFrequencyVectorNumber() == 0);
	require(dataGrid != NULL);
	require(dataGrid->SearchAttribute(GetPostOptimizationAttributeName()) != NULL);

	// Recherche de l'attribut a post-optimiser
	dataGridAttribute = dataGrid->SearchAttribute(GetPostOptimizationAttributeName());

	// Initialisation d'un dictionnaire qui a chaque cellule de la grille (identifiee par sa
	// signature globale) associe une cellule caracterisee par sa signature partielle
	InitializeHashCellDictionary(&nkdHashCells, dataGrid);

	// Initialisation de la table d'effectif
	kwftFrequencyTable->SetFrequencyVectorCreator(GetFrequencyVectorCreator()->Clone());
	kwftFrequencyTable->SetFrequencyVectorNumber(dataGridAttribute->GetPartNumber());
	kwftFrequencyTable->SetInitialValueNumber(dataGridAttribute->GetInitialValueNumber());
	kwftFrequencyTable->SetGranularizedValueNumber(dataGridAttribute->GetGranularizedValueNumber());
	kwftFrequencyTable->SetGranularity(dataGrid->GetGranularity());
	kwftFrequencyTable->SetGarbageModalityNumber(dataGridAttribute->GetGarbageModalityNumber());

	// Initialisation des intervalles du tableau d'effectif a partir des parties de l'attribut a post-optimiser
	nInterval = 0;
	cPreviousIntervalUpperBound = KWDGInterval::GetMinLowerBound();
	dataGridPart = dataGridAttribute->GetHeadPart();
	while (dataGridPart != NULL)
	{
		// Verification de l'ordre des intervalles
		assert(dataGridPart->GetInterval()->GetLowerBound() == cPreviousIntervalUpperBound);
		cPreviousIntervalUpperBound = dataGridPart->GetInterval()->GetUpperBound();

		// Acces au vecteur d'effectif de l'intervalle
		partFrequencyVector =
		    cast(KWDGPOPartFrequencyVector*, kwftFrequencyTable->GetFrequencyVectorAt(nInterval));
		nInterval++;

		// Initialisation du vecteur d'effectif a partir de la partie correspondante
		InitializePartFrequencyVector(partFrequencyVector, dataGridPart, &nkdHashCells);

		// Partie suivante
		dataGridAttribute->GetNextPart(dataGridPart);
	}

	// Affichage des resultats
	if (bDisplayResults)
	{
		cout << "Construction d'une table d'effectifs a partir d'une grille" << endl;
		cout << "Grille en entree\n" << *dataGrid << endl;
		cout << "Table d'effectifs en sortie\n" << *kwftFrequencyTable << endl;
	}
}

void KWDGPODiscretizer::InitializePartFrequencyVector(KWDGPOPartFrequencyVector* partFrequencyVector,
						      const KWDGPart* part,
						      const NumericKeyDictionary* nkdHashCells) const
{
	KWDGPOCellFrequencyVector* cellFrequencyVector;
	KWDGCell* cell;
	KWDGCell* hashCell;

	require(partFrequencyVector != NULL);
	require(partFrequencyVector->GetCellCost() == 0);
	require(partFrequencyVector->ComputeTotalFrequency() == 0);
	require(partFrequencyVector->GetModalityNumber() == 0);
	require(part != NULL);
	require(part->GetPartType() == KWType::Continuous);
	require(part->GetAttribute()->GetAttributeName() == GetPostOptimizationAttributeName());
	require(nkdHashCells != NULL);

	// Initialisation "standard" du vecteur d'effectif
	InitializeFrequencyVector(partFrequencyVector);

	// Parcours des cellules de la partie pour creer les vecteurs d'effectif par cellule
	cell = part->GetHeadCell();
	while (cell != NULL)
	{
		// Recherche de l'objet (cellule) representatnt la signature exogene
		hashCell = cast(KWDGCell*, nkdHashCells->Lookup(cell));
		check(hashCell);

		// Creation et initialisation d'un vecteur d'effectif pour la cellule
		cellFrequencyVector = new KWDGPOCellFrequencyVector;
		InitializeCellFrequencyVector(cellFrequencyVector, cell, hashCell);

		// Insertion dans le vecteur d'effectif, et mise a jour des effectif et cout
		partFrequencyVector->SetFrequency(partFrequencyVector->GetFrequency() +
						  cellFrequencyVector->GetCellFrequency());
		partFrequencyVector->SetCellCost(partFrequencyVector->GetCellCost() + cellFrequencyVector->GetCost());
		partFrequencyVector->AddCell(cellFrequencyVector);

		// Cellule suivante
		part->GetNextCell(cell);
	}
	ensure(partFrequencyVector->ComputeTotalFrequency() == part->GetPartFrequency());
}

void KWDGPODiscretizer::InitializeCellFrequencyVector(KWDGPOCellFrequencyVector* cellFrequencyVector,
						      const KWDGCell* cell, const KWDGCell* hashCell) const
{
	int nTarget;

	require(cellFrequencyVector != NULL);
	require(cellFrequencyVector->GetHashObject() == NULL);
	require(cellFrequencyVector->GetCellFrequency() == 0);
	require(cell != NULL);
	require(hashCell != NULL);

	// Initialisation de la cle de hash identifiant la cellule
	cellFrequencyVector->SetHashObject(hashCell);

	// Initialisation des compteurs d'effectifs de la cellule
	if (cell->GetTargetValueNumber() == 0)
		cellFrequencyVector->SetCellFrequency(cell->GetCellFrequency());
	else
	{
		// Initialisation du vecteur des effectifs des valeurs cibles
		cellFrequencyVector->ivFrequencyVector.SetSize(cell->GetTargetValueNumber());
		for (nTarget = 0; nTarget < cell->GetTargetValueNumber(); nTarget++)
			cellFrequencyVector->SetTargetFrequencyAt(nTarget, cell->GetTargetFrequencyAt(nTarget));
	}

	// Initialisation du cout de la cellule
	cellFrequencyVector->SetCost(ComputeCellCost(cellFrequencyVector));

	ensure(cellFrequencyVector->GetHashObject() != NULL);
	ensure(cellFrequencyVector->GetCellFrequency() == cell->GetCellFrequency());
}

// Index de l'attribut a post-optimiser (et donc a ignorer pour le calcul de la signature exogene
static int nKWDGPODiscretizerPostOptimizationAttributeIndex = -1;

// Bug detecte sur debian 12 avec la version 12.2.0 de gcc. Ce bug apparait en release mais pas en debug.
// On force la compilation en O1 car il doit y avoir une sur-optimisation de gcc en mode 02 :
// il y a un segmentation fault, gdb indique que cell1 est a NULL.
// En ajoutant la ligne suivante (inutile) apres les cast, le bug disparait
// if (cell1==NULL or cell2==NULL) exit(1);
// Bug similaire dans IntVectorSorter.h, classe IntVectorSorter
#if defined NDEBUG && defined __GNUC__ && !defined __clang__
#if __GNUC__ >= 12
#pragma GCC push_options
#pragma GCC optimize("O1")
#endif
#endif

// Fonction de comparaison de deux cellules basee sur leur signature exogene
int KWDGPODiscretizerCompareCell(const void* elem1, const void* elem2)
{
	KWDGCell* cell1;
	KWDGCell* cell2;
	int nAttribute;
	int nCompare;

	require(nKWDGPODiscretizerPostOptimizationAttributeIndex != -1);
	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux cellules
	cell1 = cast(KWDGCell*, *(Object**)elem1);
	cell2 = cast(KWDGCell*, *(Object**)elem2);
	assert(cell1->GetAttributeNumber() == cell2->GetAttributeNumber());

	// Comparaison
	for (nAttribute = 0; nAttribute < cell1->GetAttributeNumber(); nAttribute++)
	{
		// Prise en compte de l'attribut si ce n'est pas l'attribut a post-optimiser
		if (nAttribute != nKWDGPODiscretizerPostOptimizationAttributeIndex)
		{
			if (cell1->GetPartAt(nAttribute) == cell2->GetPartAt(nAttribute))
				nCompare = 0;
			else if (cell1->GetPartAt(nAttribute) > cell2->GetPartAt(nAttribute))
				nCompare = 1;
			else
				nCompare = -1;
			if (nCompare != 0)
				return nCompare;
		}
	}
	return 0;
}

#if defined NDEBUG && defined __GNUC__ && !defined __clang__
#if __GNUC__ >= 12
#pragma GCC pop_options
#endif
#endif

void KWDGPODiscretizer::InitializeHashCellDictionary(NumericKeyDictionary* nkdHashCells,
						     const KWDataGrid* dataGrid) const
{
	ObjectArray oaDataGridCells;
	int nCell;
	KWDGCell* previousCell;
	KWDGCell* cell;
	KWDGCell* hashCell;

	require(nkdHashCells != NULL);
	require(nkdHashCells->GetCount() == 0);
	require(dataGrid != NULL);
	require(dataGrid->SearchAttribute(GetPostOptimizationAttributeName()) != NULL);
	require(nKWDGPODiscretizerPostOptimizationAttributeIndex == -1);

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Tri des cellules par signature partielle pour identifier l'ensemble des celulles partielles
	// Lors de la creation des vecteur d'effectifs des parties de l'attribut a post-optimiser,
	// on utilise comme identifiant de hash pour chaque cellule le pointeur de la premiere cellule
	// ayant meme signature partielle (en utilisant le tableau des cellules triees).

	// Recherche de l'index de l'attribut a post-optimiser, pour parametrer la fonction de comparaison
	nKWDGPODiscretizerPostOptimizationAttributeIndex =
	    dataGrid->SearchAttribute(GetPostOptimizationAttributeName())->GetAttributeIndex();

	// Rangement des cellules de la grille dans un tableau
	oaDataGridCells.SetSize(dataGrid->GetCellNumber());
	nCell = 0;
	cell = dataGrid->GetHeadCell();
	while (cell != NULL)
	{
		// Ajout dans le tableau
		oaDataGridCells.SetAt(nCell, cell);
		nCell++;

		// Cellule suivante
		dataGrid->GetNextCell(cell);
	}

	// Tri du tableau par signature exogene
	oaDataGridCells.SetCompareFunction(KWDGPODiscretizerCompareCell);
	oaDataGridCells.Sort();

	// Parcours du tableau associe les cellules a leur signature exogene
	// Les signatures sont les cellule correspondant a chaque changement de valeur de la fonction de tri
	hashCell = NULL;
	previousCell = NULL;
	for (nCell = 0; nCell < oaDataGridCells.GetSize(); nCell++)
	{
		cell = cast(KWDGCell*, oaDataGridCells.GetAt(nCell));

		// Detection de changement de valeur
		if (previousCell == NULL or KWDGPODiscretizerCompareCell(&previousCell, &cell) != 0)
			hashCell = cell;
		previousCell = cell;

		// On associe la cellule a sa signature exogene
		nkdHashCells->SetAt(cell, hashCell);
	}

	// Reinitialisationde l'index de l'attribut a post-optimiser
	nKWDGPODiscretizerPostOptimizationAttributeIndex = -1;

	ensure(nKWDGPODiscretizerPostOptimizationAttributeIndex == -1);
	ensure(nkdHashCells->GetCount() == dataGrid->GetCellNumber());
}

KWMODLLine* KWDGPODiscretizer::BuildIntervalListFromFrequencyTableAndOptimizedDataGrid(
    KWMODLLine* lineCreator, const KWFrequencyTable* kwftTable, const KWDataGrid* optimizedDataGrid) const
{
	boolean bDisplayResults = false;
	KWMODLLine* headLine;
	IntVector ivIntervalLastLineIndexes;
	KWDGAttribute* dataGridAttribute;
	KWDGPart* dataGridPart;
	KWDGPOPartFrequencyVector* partFrequencyVector;
	int nFrequencyVector;
	int nCumulativeFrequency;
	Continuous cPreviousIntervalUpperBound;

	require(lineCreator != NULL);
	require(kwftTable != NULL);
	require(optimizedDataGrid != NULL);
	require(kwftTable->GetTotalFrequency() == optimizedDataGrid->GetGridFrequency());

	// Recherche de l'attribut a post-optimiser
	dataGridAttribute = optimizedDataGrid->SearchAttribute(GetPostOptimizationAttributeName());

	// Initialisation des index de fin d'intervalle dans le tableau d'effectif a partir
	// des parties de l'attribut a post-optimiser
	nCumulativeFrequency = 0;
	cPreviousIntervalUpperBound = KWDGInterval::GetMinLowerBound();
	dataGridPart = dataGridAttribute->GetHeadPart();
	for (nFrequencyVector = 0; nFrequencyVector < kwftTable->GetFrequencyVectorNumber(); nFrequencyVector++)
	{
		partFrequencyVector =
		    cast(KWDGPOPartFrequencyVector*, kwftTable->GetFrequencyVectorAt(nFrequencyVector));
		check(dataGridPart);

		// Cumul de l'effectif des vecteurs d'effectifs
		nCumulativeFrequency += partFrequencyVector->ComputeTotalFrequency();

		// Test si l'on atteint l'effectif d'une partie de la grille optimisee
		if (nCumulativeFrequency == dataGridPart->GetPartFrequency())
		{
			// Memorisation de l'index du dernier vecteur utilise pour la partie
			ivIntervalLastLineIndexes.Add(nFrequencyVector);

			// On passe a la partie suivante (avec verification de l'odre des intervalles)
			assert(dataGridPart->GetInterval()->GetLowerBound() == cPreviousIntervalUpperBound);
			cPreviousIntervalUpperBound = dataGridPart->GetInterval()->GetUpperBound();
			dataGridAttribute->GetNextPart(dataGridPart);

			// Reinitialisation du cumul de frequence
			nCumulativeFrequency = 0;
		}
	}

	// Construction de la liste des intervalles a partir d'une table de contingence et des
	// bornes des intervalles (index des dernieres lignes)
	headLine =
	    BuildIntervalListFromFrequencyTableAndIntervalBounds(lineCreator, kwftTable, &ivIntervalLastLineIndexes);

	// Affichage des resultats
	if (bDisplayResults)
	{
		cout << "Construction d'une liste d'intervalles a partir d'une grille optimisee" << endl;
		cout << "Table d'effectifs en entree\n" << *kwftTable << endl;
		cout << "Grille en entree\n" << *optimizedDataGrid << endl;
		cout << "Liste d'intervalles en sortie\n";
		WriteIntervalListReport(headLine, cout);
		cout << endl;
	}

	// Retour
	ensure(GetIntervalListSize(headLine) == dataGridAttribute->GetPartNumber());
	return headLine;
}

void KWDGPODiscretizer::UpdateDataGridFromIntervalList(KWDataGrid* optimizedDataGrid, const KWDataGrid* initialDataGrid,
						       KWMODLLine* headLine) const
{
	boolean bDisplayResults = false;
	KWDataGridManager dataGridManager;
	KWDGAttribute* initialAttribute;
	KWDGAttribute* optimizedAttribute;
	KWMODLLine* line;
	KWDGPart* initialPart;
	KWDGPart* optimizedPart;
	Continuous cPreviousIntervalUpperBound;
	int nOptimizedIntervalFrequency;
	int nFrequency;

	require(optimizedDataGrid != NULL);
	require(initialDataGrid != NULL);
	require(optimizedDataGrid->SearchAttribute(GetPostOptimizationAttributeName()) != NULL);
	require(initialDataGrid->SearchAttribute(GetPostOptimizationAttributeName()) != NULL);

	// Acces aux attributs des grilles initiale et optimise pour l'attribut de post-optimisation
	initialAttribute = initialDataGrid->SearchAttribute(sPostOptimizationAttributeName);
	optimizedAttribute = optimizedDataGrid->SearchAttribute(sPostOptimizationAttributeName);

	// On vide la grille optimisee de ses cellules, en preservant ses attribut et leur partition
	optimizedDataGrid->DeleteAllCells();

	// On reinitialise a vide les partie pour l'attribut a post-optimiser
	optimizedAttribute->DeleteAllParts();

	// Parcours de la liste optimisee pour determiner les definitions des intervalles
	// Pour cela, on se synchonise avec les definition d'intervalles de la grille initiale,
	// en les fusionnant sur la base des effectif par intervalles vises
	cPreviousIntervalUpperBound = KWDGInterval::GetMinLowerBound();
	initialPart = NULL;
	line = headLine;
	while (line != NULL)
	{
		// Effectif de l'intervalle a viser
		nOptimizedIntervalFrequency = line->GetFrequencyVector()->ComputeTotalFrequency();

		// Creation d'une nouvelle partie optimisee
		optimizedPart = optimizedAttribute->AddPart();

		// Initialisation sa borne inf
		optimizedPart->GetInterval()->SetLowerBound(cPreviousIntervalUpperBound);

		// Parcours des parties initiales jusqu'a obtenir l'effectif vise
		nFrequency = 0;
		while (nFrequency < nOptimizedIntervalFrequency)
		{
			// Acces a la partie suivante
			if (initialPart == NULL)
				initialPart = initialAttribute->GetHeadPart();
			else
				initialAttribute->GetNextPart(initialPart);

			// Verification de l'ordre des intervalles
			assert(initialPart->GetInterval()->GetLowerBound() == cPreviousIntervalUpperBound);
			cPreviousIntervalUpperBound = initialPart->GetInterval()->GetUpperBound();

			// Prise en compte de l'effectif
			nFrequency += initialPart->GetPartFrequency();
		}
		assert(initialPart != NULL);

		// Initialisation de la borne sup
		optimizedPart->GetInterval()->SetUpperBound(cPreviousIntervalUpperBound);
		assert(optimizedPart->GetInterval()->Check());

		// Ligne suivante
		line = line->GetNext();
	}

	// Export des cellules pour la grille initiale univariee
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	dataGridManager.ExportCells(optimizedDataGrid);

	// Affichage des resultats
	if (bDisplayResults)
	{
		cout << "Preparation d'une grille pour l'optimisation univariee\t" << sPostOptimizationAttributeName
		     << endl;
		cout << "Grille initiale\n" << *initialDataGrid << endl;
		cout << "Liste de vecteurs d'effectifs optimisee\n";
		WriteIntervalListReport(headLine, cout);
		cout << "Grille optimisee\n" << *optimizedDataGrid << endl;
	}

	// Verification de la grille preparee
	ensure(GetIntervalListSize(headLine) ==
	       optimizedDataGrid->SearchAttribute(sPostOptimizationAttributeName)->GetPartNumber());
	ensure(initialDataGrid->GetGridFrequency() == optimizedDataGrid->GetGridFrequency());
	ensure(initialDataGrid->GetCellNumber() >= optimizedDataGrid->GetCellNumber());
}

void KWDGPODiscretizer::InitializeFrequencyVector(KWFrequencyVector* kwfvFrequencyVector) const {}

boolean KWDGPODiscretizer::CheckFrequencyVector(const KWFrequencyVector* kwfvFrequencyVector) const
{
	require(kwfvFrequencyVector != NULL);
	return true;
}

void KWDGPODiscretizer::InitializeWorkingData(const KWFrequencyTable* kwftSource) const
{
	require(kwftSource != NULL);
}

void KWDGPODiscretizer::CleanWorkingData() const {}

void KWDGPODiscretizer::AddFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
					   const KWFrequencyVector* kwfvAddedFrequencyVector) const
{
	boolean bDisplayResults = false;
	KWDGPOPartFrequencyVector* sourcePartFrequencyVector;
	const KWDGPOPartFrequencyVector* addedPartFrequencyVector;
	POSITION positionVector;
	KWDGPOCellFrequencyVector* sourceCellFrequencyVector;
	KWDGPOCellFrequencyVector* addedCellFrequencyVector;
	double dPartCost;
	debug(int nInitialFrequency);

	require(kwfvSourceFrequencyVector != NULL);
	require(kwfvAddedFrequencyVector != NULL);
	require(kwfvSourceFrequencyVector != kwfvAddedFrequencyVector);
	require(CheckFrequencyVector(kwfvSourceFrequencyVector));
	require(CheckFrequencyVector(kwfvAddedFrequencyVector));

	// Affichage des parties initiales
	if (bDisplayResults)
	{
		cout << "Ajout d'une partie\n";
		cout << "SourcePart\n" << *kwfvSourceFrequencyVector << endl;
		cout << "AddedPart\n" << *kwfvAddedFrequencyVector << endl;
	}

	// Acces aux vecteur d'effectif dans leur bon type
	sourcePartFrequencyVector = cast(KWDGPOPartFrequencyVector*, kwfvSourceFrequencyVector);
	addedPartFrequencyVector = cast(KWDGPOPartFrequencyVector*, kwfvAddedFrequencyVector);
	debug(nInitialFrequency = sourcePartFrequencyVector->ComputeTotalFrequency());

	// Gestion des nombres de valeurs
	sourcePartFrequencyVector->SetModalityNumber(sourcePartFrequencyVector->GetModalityNumber() +
						     addedPartFrequencyVector->GetModalityNumber());

	// Parcours des cellules a ajouter
	dPartCost = sourcePartFrequencyVector->GetCellCost();
	positionVector = addedPartFrequencyVector->GetHeadPosition();
	while (positionVector != NULL)
	{
		addedCellFrequencyVector = addedPartFrequencyVector->GetNextPosition(positionVector);

		// Recherche de la cellule correspondante dans la partie a enrichir
		sourceCellFrequencyVector =
		    sourcePartFrequencyVector->LookupCell(addedCellFrequencyVector->GetHashObject());

		// Ajout de l'effectif de la cellule a la partie
		sourcePartFrequencyVector->SetFrequency(sourcePartFrequencyVector->GetFrequency() +
							addedCellFrequencyVector->GetCellFrequency());

		// Creation d'une nouvelle cellule si necessaire
		if (sourceCellFrequencyVector == NULL)
		{
			// Creation d'une nouvelle cellule
			sourceCellFrequencyVector = new KWDGPOCellFrequencyVector;
			sourceCellFrequencyVector->CopyFrom(addedCellFrequencyVector);

			// Ajout dans la partie
			sourcePartFrequencyVector->AddCell(sourceCellFrequencyVector);

			// Mise a jour du cout de la partie
			dPartCost += sourceCellFrequencyVector->GetCost();
		}
		else
		// Sinon, concatenation du contenu de la cellule
		{
			// On enleve au prealable le cout de la cellule a enrichir
			dPartCost -= sourceCellFrequencyVector->GetCost();

			// On met a jour la cellule et de son cout
			sourceCellFrequencyVector->AddFrequenciesFrom(addedCellFrequencyVector);
			sourceCellFrequencyVector->SetCost(ComputeCellCost(sourceCellFrequencyVector));

			// On ajoute le cout de la cellule modifiee
			dPartCost += sourceCellFrequencyVector->GetCost();
		}
	}
	sourcePartFrequencyVector->SetCellCost(dPartCost);

	// Affichage de la partie modifiee
	if (bDisplayResults)
		cout << "ModifiedSourcePart\n" << *kwfvSourceFrequencyVector << endl;

	ensure(sourcePartFrequencyVector->GetCellNumber() >= addedPartFrequencyVector->GetCellNumber());
	debug(ensure(sourcePartFrequencyVector->ComputeTotalFrequency() ==
		     nInitialFrequency + addedPartFrequencyVector->ComputeTotalFrequency()));
}

void KWDGPODiscretizer::RemoveFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
					      const KWFrequencyVector* kwfvRemovedFrequencyVector) const
{
	boolean bDisplayResults = false;
	KWDGPOPartFrequencyVector* sourcePartFrequencyVector;
	const KWDGPOPartFrequencyVector* removedPartFrequencyVector;
	POSITION positionVector;
	KWDGPOCellFrequencyVector* sourceCellFrequencyVector;
	KWDGPOCellFrequencyVector* removedCellFrequencyVector;
	double dPartCost;
	debug(int nInitialFrequency);

	require(kwfvSourceFrequencyVector != NULL);
	require(kwfvRemovedFrequencyVector != NULL);
	require(kwfvSourceFrequencyVector != kwfvRemovedFrequencyVector);
	require(CheckFrequencyVector(kwfvSourceFrequencyVector));
	require(CheckFrequencyVector(kwfvRemovedFrequencyVector));

	// Affichage des parties initiales
	if (bDisplayResults)
	{
		cout << "Supression d'une partie\n";
		cout << "SourcePart\n" << *kwfvSourceFrequencyVector << endl;
		cout << "RemovedPart\n" << *kwfvRemovedFrequencyVector << endl;
	}

	// Acces aux vecteur d'effectif dans leur bon type
	sourcePartFrequencyVector = cast(KWDGPOPartFrequencyVector*, kwfvSourceFrequencyVector);
	removedPartFrequencyVector = cast(KWDGPOPartFrequencyVector*, kwfvRemovedFrequencyVector);
	assert(sourcePartFrequencyVector->GetFrequency() == sourcePartFrequencyVector->ComputeTotalFrequency());
	assert(removedPartFrequencyVector->GetFrequency() == removedPartFrequencyVector->ComputeTotalFrequency());
	debug(nInitialFrequency = sourcePartFrequencyVector->ComputeTotalFrequency());

	// Gestion des nombres de valeurs
	sourcePartFrequencyVector->SetModalityNumber(sourcePartFrequencyVector->GetModalityNumber() -
						     removedPartFrequencyVector->GetModalityNumber());

	// Parcours des cellules a retrancher
	dPartCost = sourcePartFrequencyVector->GetCellCost();
	positionVector = removedPartFrequencyVector->GetHeadPosition();
	while (positionVector != NULL)
	{
		removedCellFrequencyVector = removedPartFrequencyVector->GetNextPosition(positionVector);
		assert(removedCellFrequencyVector->ComputeTotalFrequency() > 0);

		// Recherche de la cellule correspondante dans la partie a enrichir
		sourceCellFrequencyVector =
		    sourcePartFrequencyVector->LookupCell(removedCellFrequencyVector->GetHashObject());
		check(sourceCellFrequencyVector);
		assert(sourceCellFrequencyVector->ComputeTotalFrequency() >=
		       removedCellFrequencyVector->ComputeTotalFrequency());

		// Supression de l'effectif de la cellule de la partie
		sourcePartFrequencyVector->SetFrequency(sourcePartFrequencyVector->GetFrequency() -
							removedCellFrequencyVector->GetCellFrequency());

		// On enleve au prealable le cout de la cellule initiale
		dPartCost -= sourceCellFrequencyVector->GetCost();

		// Si la cellule devient vide, on la supprime
		if (sourceCellFrequencyVector->ComputeTotalFrequency() ==
		    removedCellFrequencyVector->ComputeTotalFrequency())
			sourcePartFrequencyVector->DeleteCell(sourceCellFrequencyVector);
		// Sinon, on retranche les effectifs de la cellule a supprimer
		else
		{
			// On met a jour la cellule et son cout
			sourceCellFrequencyVector->RemoveFrequenciesFrom(removedCellFrequencyVector);
			assert(sourceCellFrequencyVector->ComputeTotalFrequency() > 0);
			sourceCellFrequencyVector->SetCost(ComputeCellCost(sourceCellFrequencyVector));

			// On ajoute le cout de la cellule modifiee
			dPartCost += sourceCellFrequencyVector->GetCost();
		}
	}
	sourcePartFrequencyVector->SetCellCost(dPartCost);

	// Affichage de la partie modifiee
	if (bDisplayResults)
		cout << "ModifiedSourcePart\n" << *kwfvSourceFrequencyVector << endl;

	ensure(sourcePartFrequencyVector->GetFrequency() == sourcePartFrequencyVector->ComputeTotalFrequency());
	debug(ensure(sourcePartFrequencyVector->ComputeTotalFrequency() ==
		     nInitialFrequency - removedPartFrequencyVector->ComputeTotalFrequency()));
}

void KWDGPODiscretizer::MergeTwoFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector,
						 const KWFrequencyVector* kwfvMergedFrequencyVector1,
						 const KWFrequencyVector* kwfvMergedFrequencyVector2) const
{
	require(kwfvSourceFrequencyVector != NULL);
	require(kwfvMergedFrequencyVector1 != NULL);
	require(kwfvMergedFrequencyVector2 != NULL);
	require(CheckFrequencyVector(kwfvSourceFrequencyVector));
	require(CheckFrequencyVector(kwfvMergedFrequencyVector1));
	require(CheckFrequencyVector(kwfvMergedFrequencyVector2));

	// On copie le vecteur source ayant le plus de cellules, et on ajoute l'autre
	if (cast(KWDGPOPartFrequencyVector*, kwfvMergedFrequencyVector1)->GetCellNumber() >=
	    cast(KWDGPOPartFrequencyVector*, kwfvMergedFrequencyVector2)->GetCellNumber())
	{
		kwfvSourceFrequencyVector->CopyFrom(kwfvMergedFrequencyVector1);
		AddFrequencyVector(kwfvSourceFrequencyVector, kwfvMergedFrequencyVector2);
	}
	else
	{
		kwfvSourceFrequencyVector->CopyFrom(kwfvMergedFrequencyVector2);
		AddFrequencyVector(kwfvSourceFrequencyVector, kwfvMergedFrequencyVector1);
	}
	ensure(kwfvSourceFrequencyVector->ComputeTotalFrequency() ==
	       kwfvMergedFrequencyVector1->ComputeTotalFrequency() +
		   kwfvMergedFrequencyVector2->ComputeTotalFrequency());
}

void KWDGPODiscretizer::MergeThreeFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector,
						   const KWFrequencyVector* kwfvMergedFrequencyVector1,
						   const KWFrequencyVector* kwfvMergedFrequencyVector2,
						   const KWFrequencyVector* kwfvMergedFrequencyVector3) const
{
	int nCellNumber1;
	int nCellNumber2;
	int nCellNumber3;

	require(kwfvSourceFrequencyVector != NULL);
	require(kwfvMergedFrequencyVector1 != NULL);
	require(kwfvMergedFrequencyVector2 != NULL);
	require(kwfvMergedFrequencyVector3 != NULL);
	require(CheckFrequencyVector(kwfvSourceFrequencyVector));
	require(CheckFrequencyVector(kwfvMergedFrequencyVector1));
	require(CheckFrequencyVector(kwfvMergedFrequencyVector2));
	require(CheckFrequencyVector(kwfvMergedFrequencyVector3));

	// Recherche des nombre de cellules par partie a fusionner
	nCellNumber1 = cast(KWDGPOPartFrequencyVector*, kwfvMergedFrequencyVector1)->GetCellNumber();
	nCellNumber2 = cast(KWDGPOPartFrequencyVector*, kwfvMergedFrequencyVector2)->GetCellNumber();
	nCellNumber3 = cast(KWDGPOPartFrequencyVector*, kwfvMergedFrequencyVector3)->GetCellNumber();

	// On copie le vecteur source ayant le plus de cellules, et on ajoute les autres
	if (nCellNumber1 >= nCellNumber2 and nCellNumber1 >= nCellNumber3)
	{
		kwfvSourceFrequencyVector->CopyFrom(kwfvMergedFrequencyVector1);
		AddFrequencyVector(kwfvSourceFrequencyVector, kwfvMergedFrequencyVector2);
		AddFrequencyVector(kwfvSourceFrequencyVector, kwfvMergedFrequencyVector3);
	}
	else if (nCellNumber2 >= nCellNumber1 and nCellNumber2 >= nCellNumber3)
	{
		kwfvSourceFrequencyVector->CopyFrom(kwfvMergedFrequencyVector2);
		AddFrequencyVector(kwfvSourceFrequencyVector, kwfvMergedFrequencyVector1);
		AddFrequencyVector(kwfvSourceFrequencyVector, kwfvMergedFrequencyVector3);
	}
	else
	{
		assert(nCellNumber3 >= nCellNumber1 and nCellNumber3 >= nCellNumber2);
		kwfvSourceFrequencyVector->CopyFrom(kwfvMergedFrequencyVector3);
		AddFrequencyVector(kwfvSourceFrequencyVector, kwfvMergedFrequencyVector1);
		AddFrequencyVector(kwfvSourceFrequencyVector, kwfvMergedFrequencyVector2);
	}
	ensure(kwfvSourceFrequencyVector->ComputeTotalFrequency() ==
	       kwfvMergedFrequencyVector1->ComputeTotalFrequency() +
		   kwfvMergedFrequencyVector2->ComputeTotalFrequency() +
		   kwfvMergedFrequencyVector3->ComputeTotalFrequency());
}

void KWDGPODiscretizer::SplitFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
					     KWFrequencyVector* kwfvNewFrequencyVector,
					     const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const
{
	debug(int nInitialFrequency);

	// Memorisation de l'effectif initial
	debug(nInitialFrequency = kwfvSourceFrequencyVector->ComputeTotalFrequency());

	// Coupure de l'intervalle
	kwfvNewFrequencyVector->CopyFrom(kwfvSourceFrequencyVector);
	RemoveFrequencyVector(kwfvNewFrequencyVector, kwfvFirstSubFrequencyVectorSpec);
	kwfvSourceFrequencyVector->CopyFrom(kwfvFirstSubFrequencyVectorSpec);

	// Verification de l'effectif final
	debug(ensure(nInitialFrequency == kwfvSourceFrequencyVector->ComputeTotalFrequency() +
					      kwfvNewFrequencyVector->ComputeTotalFrequency()));
}

void KWDGPODiscretizer::MergeSplitFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector1,
						   KWFrequencyVector* kwfvSourceFrequencyVector2,
						   const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const
{
	debug(int nInitialFrequency);

	// Memorisation de l'effectif initial
	debug(nInitialFrequency = kwfvSourceFrequencyVector1->ComputeTotalFrequency() +
				  kwfvSourceFrequencyVector2->ComputeTotalFrequency());

	// Fusion des deux intervalles dans le premier
	AddFrequencyVector(kwfvSourceFrequencyVector2, kwfvSourceFrequencyVector1);

	// Coupure du nouvel interval
	RemoveFrequencyVector(kwfvSourceFrequencyVector2, kwfvFirstSubFrequencyVectorSpec);
	kwfvSourceFrequencyVector1->CopyFrom(kwfvFirstSubFrequencyVectorSpec);

	// Verification de l'effectif final
	debug(ensure(nInitialFrequency == kwfvSourceFrequencyVector1->ComputeTotalFrequency() +
					      kwfvSourceFrequencyVector2->ComputeTotalFrequency()));
}

void KWDGPODiscretizer::MergeMergeSplitFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector1,
							const KWFrequencyVector* kwfvSourceFrequencyVector2,
							KWFrequencyVector* kwfvSourceFrequencyVector3,
							const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const
{
	debug(int nInitialFrequency);

	// Memorisation de l'effectif initial
	debug(nInitialFrequency = kwfvSourceFrequencyVector1->ComputeTotalFrequency() +
				  kwfvSourceFrequencyVector2->ComputeTotalFrequency() +
				  kwfvSourceFrequencyVector3->ComputeTotalFrequency());

	// Fusion des trois intervalles dans le dernier
	AddFrequencyVector(kwfvSourceFrequencyVector3, kwfvSourceFrequencyVector1);
	AddFrequencyVector(kwfvSourceFrequencyVector3, kwfvSourceFrequencyVector2);

	// Coupure du nouvel interval
	RemoveFrequencyVector(kwfvSourceFrequencyVector3, kwfvFirstSubFrequencyVectorSpec);
	kwfvSourceFrequencyVector1->CopyFrom(kwfvFirstSubFrequencyVectorSpec);

	// Verification de l'effectif final
	debug(ensure(nInitialFrequency == kwfvSourceFrequencyVector1->ComputeTotalFrequency() +
					      kwfvSourceFrequencyVector3->ComputeTotalFrequency()));
}

double KWDGPODiscretizer::ComputeCellCost(const KWDGPOCellFrequencyVector* cellFrequencyVector) const
{
	return cast(KWDataGridUnivariateCosts*, GetDiscretizationCosts())->ComputeCellCost(cellFrequencyVector);
}

////////////////////////////////////////////////////////////////////////////////////
// Classe KWDGPOGrouper

KWDGPOGrouper::KWDGPOGrouper()
{
	// Redefinition de la structure des cout de la discretisation
	SetGroupingCosts(new KWDataGridUnivariateCosts);
}

KWDGPOGrouper::~KWDGPOGrouper()
{
	// Le destructeur de la classe ancetre detruit le creator de vecteur d'effectif
}

void KWDGPOGrouper::SetPostOptimizationAttributeName(const ALString& sValue)
{
	sPostOptimizationAttributeName = sValue;
}

const ALString& KWDGPOGrouper::GetPostOptimizationAttributeName() const
{
	return sPostOptimizationAttributeName;
}

double KWDGPOGrouper::PostOptimizeDataGrid(const KWDataGrid* initialDataGrid, const KWDataGridCosts* dataGridCosts,
					   KWDataGrid* optimizedDataGrid, boolean bDeepPostOptimization) const
{
	boolean bDisplayResults = false;
	double dCost;
	KWDataGridUnivariateCosts* dataGridUnivariateCosts;
	KWDataGridManager dataGridManager;
	KWFrequencyTable initialFrequencyTable;
	IntVector ivGroups;
	int nGroupNumber;
	KWFrequencyTable groupedFrequencyTable;
	IntVector ivNonEmptyGroups;
	int nIndex;
	int nNewIndex;
	int nMaxStepNumber;
	int nGroup;
	// CH IV Begin
	boolean bEMAlgorithm = false;
	// CH IV End

	require(optimizedDataGrid != NULL);
	require(initialDataGrid != NULL);
	require(dataGridCosts != NULL);
	require(optimizedDataGrid->Check());
	require(initialDataGrid->Check());
	require(optimizedDataGrid->SearchAttribute(GetPostOptimizationAttributeName()) != NULL);
	require(initialDataGrid->SearchAttribute(GetPostOptimizationAttributeName()) != NULL);
	require(optimizedDataGrid->GetTotalPartNumber() -
		    optimizedDataGrid->SearchAttribute(GetPostOptimizationAttributeName())->GetPartNumber() ==
		initialDataGrid->GetTotalPartNumber() -
		    initialDataGrid->SearchAttribute(GetPostOptimizationAttributeName())->GetPartNumber());

	// Affichage de la grille initiale avec ses couts
	if (bDisplayResults)
	{
		cout << "\nPost-optimisation (groupement de valeurs): grille initiale" << endl;
		cout << "\tAttribut optimise: " << GetPostOptimizationAttributeName() << endl;

		if (optimizedDataGrid->GetAttributeNumber() == 2)
			optimizedDataGrid->WriteCrossTableStats(cout, 0);
		dataGridCosts->WriteDataGridAllCosts(optimizedDataGrid, cout);
		cout << endl;
	}

	// Verification de la compatibilite entre grille optimisee et grille initiale
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	require(dataGridManager.CheckDataGrid(optimizedDataGrid));

	// Parametrage des couts d'optimisation univarie de la grille
	dataGridUnivariateCosts = cast(KWDataGridUnivariateCosts*, GetGroupingCosts());
	dataGridUnivariateCosts->SetPostOptimizationAttributeName(GetPostOptimizationAttributeName());
	dataGridUnivariateCosts->SetDataGridCosts(dataGridCosts);
	dataGridUnivariateCosts->InitializeUnivariateCostParameters(optimizedDataGrid);

	// Construction d'une table d'effectif selon l'attribut a post-optimiser, pour la grille initiale
	InitializeFrequencyTableFromDataGrid(&initialFrequencyTable, initialDataGrid);

	// Initialisation des index de groupes
	InitializeGroupIndexes(&ivGroups, initialDataGrid, optimizedDataGrid);
	nGroupNumber = optimizedDataGrid->SearchAttribute(GetPostOptimizationAttributeName())->GetPartNumber();

	// Initialisation d'un tableau d'effectif groupe a partir d'une grille initiale et des index des groupes
	InitializeGroupedFrequencyTableFromDataGrid(&groupedFrequencyTable, &initialFrequencyTable, &ivGroups,
						    nGroupNumber);

	// Initialisation des donnees de suivi du groupe poubelle pour la table
	SortedList frequencyList(KWFrequencyVectorModalityNumberCompare);
	// Initialisation de la liste des nombres de modalites par ligne de contingence
	for (nGroup = 0; nGroup < groupedFrequencyTable.GetFrequencyVectorNumber(); nGroup++)
	{
		// Ajout de la ligne dans la liste triee par nombre de modalites et memorisation de sa position
		groupedFrequencyTable.GetFrequencyVectorAt(nGroup)->SetPosition(
		    frequencyList.Add(groupedFrequencyTable.GetFrequencyVectorAt(nGroup)));
	}
	// Cas ou l'attribut de grille a un groupe poubelle
	if (optimizedDataGrid->SearchAttribute(GetPostOptimizationAttributeName())->GetGarbagePart() != NULL)
	{
		if (bDisplayResults)
			cout << "PostOptimizeDataGrid:GarbageVarParts:Cas ou l'attribut de grille a un groupe poubelle"
			     << endl;
		// Initialisation de la taille du groupe poubelle
		groupedFrequencyTable.SetGarbageModalityNumber(
		    cast(KWFrequencyVector*, frequencyList.GetHead())->GetModalityNumber());
	}

	// Post-optimisation du groupage
	// Il n'est pas utile d'utiliser PostOptimizeGrouping, meme en cas d'optimisation intense
	if (bDeepPostOptimization)
		nMaxStepNumber = (int)log(initialDataGrid->GetGridFrequency() * 1.0);
	else
		nMaxStepNumber = 2;

	// CH IV Begin
	// Travail en cours pour etudier une approche EM plutot que l'algorithme initial de post-optimisation
	if (not bEMAlgorithm)
		FastPostOptimizeGroupsWithGarbage(&initialFrequencyTable, &groupedFrequencyTable, &ivGroups,
						  nMaxStepNumber, &frequencyList);
	else
		EMPostOptimizeGroupsWithGarbage(&initialFrequencyTable, &groupedFrequencyTable, &ivGroups,
						nMaxStepNumber, &frequencyList);
	// CH IV End

	nGroupNumber = groupedFrequencyTable.GetFrequencyVectorNumber();

	// Supression des eventuels groupes vides
	groupedFrequencyTable.FilterEmptyFrequencyVectors(&ivNonEmptyGroups);
	if (nGroupNumber != groupedFrequencyTable.GetFrequencyVectorNumber())
	{
		nGroupNumber = groupedFrequencyTable.GetFrequencyVectorNumber();
		for (nIndex = 0; nIndex < ivGroups.GetSize(); nIndex++)
		{
			nNewIndex = ivNonEmptyGroups.GetAt(ivGroups.GetAt(nIndex));
			assert(-1 <= nNewIndex and nNewIndex <= ivGroups.GetAt(nIndex));
			if (nNewIndex == -1)
				nNewIndex = 0;
			ivGroups.SetAt(nIndex, nNewIndex);
		}
	}

	// Mise a jour d'une grille a optimiser a partir d'une grille initiale et des index de groupes
	UpdateDataGridWithGarbageFromGroups(optimizedDataGrid, initialDataGrid, &ivGroups, nGroupNumber);

	// Si absence de poubelle dans la table de contingence, on desactive la poubelle de l'attribut de grille
	if (groupedFrequencyTable.GetGarbageModalityNumber() == 0)
	{
		optimizedDataGrid->SearchAttribute(GetPostOptimizationAttributeName())->SetGarbagePart(NULL);
	}
	// Cas d'un groupe poubelle
	else
	{
		assert(
		    groupedFrequencyTable.GetGarbageModalityNumber() ==
		    optimizedDataGrid->SearchAttribute(GetPostOptimizationAttributeName())->GetGarbageModalityNumber());
		dataGridUnivariateCosts->InitializeUnivariateCostParameters(optimizedDataGrid);
	}

	// Calcul du cout en se basant sur l'evaluation univariee
	dCost = ComputePartitionGlobalCost(&groupedFrequencyTable);

	// Affichage de la grille finale avec ses couts
	if (bDisplayResults)
	{
		boolean bDebugPostOptimization = false;

		// Debug de la post-optimisation
		if (bDebugPostOptimization)
		{
			cout << "\nPost-optimisation (groupement de valeurs): univariate costs" << endl;
			groupingCosts->WritePartitionAllCosts(&groupedFrequencyTable, cout);
		}
		cout << "\nPost-optimisation (groupement de valeurs): grille finale" << endl;
		if (optimizedDataGrid->GetAttributeNumber() == 2)
			optimizedDataGrid->WriteCrossTableStats(cout, 0);
		dataGridCosts->WriteDataGridAllCosts(optimizedDataGrid, cout);
		cout << endl;
	}

	// Verification du cout avec l'evaluation multi-variee
	assert(fabs(dCost - dataGridCosts->ComputeDataGridTotalCost(optimizedDataGrid)) < dEpsilon);

	// Verification de la compatibilite entre grille optimisee et grille initiale
	ensure(dataGridManager.CheckDataGrid(optimizedDataGrid));

	return dCost;
}

void KWDGPOGrouper::InitializeFrequencyTableFromDataGrid(KWFrequencyTable* kwftFrequencyTable,
							 const KWDataGrid* dataGrid) const
{
	boolean bDisplayResults = false;
	NumericKeyDictionary nkdHashCells;
	KWDGAttribute* dataGridAttribute;
	KWDGPart* dataGridPart;
	KWDGPOPartFrequencyVector* partFrequencyVector;
	int nGroup;

	require(kwftFrequencyTable != NULL);
	require(kwftFrequencyTable->GetFrequencyVectorNumber() == 0);
	require(dataGrid != NULL);
	require(dataGrid->SearchAttribute(GetPostOptimizationAttributeName()) != NULL);

	// Recherche de l'attribut a post-optimiser
	dataGridAttribute = dataGrid->SearchAttribute(GetPostOptimizationAttributeName());

	// Initialisation d'un dictionnaire qui a chaque cellule de la grille (identifiee par sa
	// signature globale) associe une cellule caracterisee par sa signature partielle
	InitializeHashCellDictionary(&nkdHashCells, dataGrid);

	// Initialisation de la table d'effectif
	kwftFrequencyTable->SetFrequencyVectorCreator(GetFrequencyVectorCreator()->Clone());
	kwftFrequencyTable->SetFrequencyVectorNumber(dataGridAttribute->GetPartNumber());
	kwftFrequencyTable->SetInitialValueNumber(dataGridAttribute->GetInitialValueNumber());
	kwftFrequencyTable->SetGranularizedValueNumber(dataGridAttribute->GetGranularizedValueNumber());
	// Parametrage granularite et poubelle
	kwftFrequencyTable->SetGranularity(dataGrid->GetGranularity());
	kwftFrequencyTable->SetGarbageModalityNumber(dataGridAttribute->GetGarbageModalityNumber());

	// Initialisation des groupes du tableau d'effectif a partir des parties de l'attribut a post-optimiser
	nGroup = 0;
	dataGridPart = dataGridAttribute->GetHeadPart();
	while (dataGridPart != NULL)
	{
		// Acces au vecteur d'effectif du groupe
		partFrequencyVector =
		    cast(KWDGPOPartFrequencyVector*, kwftFrequencyTable->GetFrequencyVectorAt(nGroup));
		nGroup++;

		// Initialisation du vecteur d'effectif a partir de la partie correspondante
		InitializePartFrequencyVector(partFrequencyVector, dataGridPart, &nkdHashCells);

		// Partie suivante
		dataGridAttribute->GetNextPart(dataGridPart);
	}

	// Affichage des resultats
	if (bDisplayResults)
	{
		cout << "Construction d'une table d'effectifs a partir d'une grille" << endl;
		cout << "Grille en entree\n" << *dataGrid << endl;
		cout << "Table d'effectifs en sortie\n" << *kwftFrequencyTable << endl;
	}
}

void KWDGPOGrouper::InitializePartFrequencyVector(KWDGPOPartFrequencyVector* partFrequencyVector, const KWDGPart* part,
						  const NumericKeyDictionary* nkdHashCells) const
{
	KWDGPOCellFrequencyVector* cellFrequencyVector;
	KWDGCell* cell;
	KWDGCell* hashCell;

	require(partFrequencyVector != NULL);
	require(partFrequencyVector->GetCellCost() == 0);
	require(partFrequencyVector->ComputeTotalFrequency() == 0);
	require(partFrequencyVector->GetModalityNumber() == 0);
	require(part != NULL);
	require(KWType::IsCoclusteringGroupableType(part->GetPartType()));
	require(part->GetAttribute()->GetAttributeName() == GetPostOptimizationAttributeName());
	require(nkdHashCells != NULL);

	// Initialisation "standard" du vecteur d'effectif
	InitializeFrequencyVector(partFrequencyVector);

	// Memorisation du nombre de valeurs associees a la partie
	partFrequencyVector->SetModalityNumber(part->GetValueSet()->GetValueNumber());

	// Parcours des cellules de la partie pour creer les vecteurs d'effectif par cellule
	cell = part->GetHeadCell();
	while (cell != NULL)
	{
		// Recherche de l'objet (cellule) representatnt la signature exogene
		hashCell = cast(KWDGCell*, nkdHashCells->Lookup(cell));
		check(hashCell);

		// Creation et initialisation d'un vecteur d'effectif pour la cellule
		cellFrequencyVector = new KWDGPOCellFrequencyVector;
		InitializeCellFrequencyVector(cellFrequencyVector, cell, hashCell);

		// Insertion dans le vecteur d'effectif, et mise a jour des effectif et cout
		partFrequencyVector->SetFrequency(partFrequencyVector->GetFrequency() +
						  cellFrequencyVector->GetCellFrequency());
		partFrequencyVector->SetCellCost(partFrequencyVector->GetCellCost() + cellFrequencyVector->GetCost());
		partFrequencyVector->AddCell(cellFrequencyVector);

		// Cellule suivante
		part->GetNextCell(cell);
	}
	ensure(partFrequencyVector->ComputeTotalFrequency() == part->GetPartFrequency());
}

void KWDGPOGrouper::InitializeCellFrequencyVector(KWDGPOCellFrequencyVector* cellFrequencyVector, const KWDGCell* cell,
						  const KWDGCell* hashCell) const
{
	int nTarget;

	require(cellFrequencyVector != NULL);
	require(cellFrequencyVector->GetHashObject() == NULL);
	require(cellFrequencyVector->GetCellFrequency() == 0);
	require(cell != NULL);
	require(hashCell != NULL);

	// Initialisation de la cle de hash identifiant la cellule
	cellFrequencyVector->SetHashObject(hashCell);

	// Initialisation des compteurs d'effectifs de la cellule
	if (cell->GetTargetValueNumber() == 0)
		cellFrequencyVector->SetCellFrequency(cell->GetCellFrequency());
	else
	{
		// Initialisation du vecteur des effectifs des valeurs cibles
		cellFrequencyVector->ivFrequencyVector.SetSize(cell->GetTargetValueNumber());
		for (nTarget = 0; nTarget < cell->GetTargetValueNumber(); nTarget++)
			cellFrequencyVector->SetTargetFrequencyAt(nTarget, cell->GetTargetFrequencyAt(nTarget));
	}

	// Initialisation du cout de la cellule
	cellFrequencyVector->SetCost(ComputeCellCost(cellFrequencyVector));

	ensure(cellFrequencyVector->GetHashObject() != NULL);
	ensure(cellFrequencyVector->GetCellFrequency() == cell->GetCellFrequency());
}

// Index de l'attribut a post-optimiser (et donc a ignorer pour le calcul de la signature exogene
static int nKWDGPOGrouperPostOptimizationAttributeIndex = -1;

// On force l'optimisation en O1 car il y a un probleme avec gcc v12
// Cf. pbm similaire pour la methode KWDGPODiscretizerCompareCell
#if defined NDEBUG && defined __GNUC__ && !defined __clang__
#if __GNUC__ >= 12
#pragma GCC push_options
#pragma GCC optimize("O1")
#endif
#endif

// Fonction de comparaison de deux cellules basee sur leur signature exogene
int KWDGPOGrouperCompareCell(const void* elem1, const void* elem2)
{
	KWDGCell* cell1;
	KWDGCell* cell2;
	int nAttribute;
	int nCompare;

	require(nKWDGPOGrouperPostOptimizationAttributeIndex != -1);
	require(elem1 != NULL);
	require(elem2 != NULL);

	// Acces aux cellules
	cell1 = cast(KWDGCell*, *(Object**)elem1);
	cell2 = cast(KWDGCell*, *(Object**)elem2);
	assert(cell1->GetAttributeNumber() == cell2->GetAttributeNumber());

	// Comparaison
	for (nAttribute = 0; nAttribute < cell1->GetAttributeNumber(); nAttribute++)
	{
		// Prise en compte de l'attribut si ce n'est pas l'attribut a post-optimiser
		if (nAttribute != nKWDGPOGrouperPostOptimizationAttributeIndex)
		{
			if (cell1->GetPartAt(nAttribute) == cell2->GetPartAt(nAttribute))
				nCompare = 0;
			else if (cell1->GetPartAt(nAttribute) > cell2->GetPartAt(nAttribute))
				nCompare = 1;
			else
				nCompare = -1;
			if (nCompare != 0)
				return nCompare;
		}
	}
	return 0;
}

#if defined NDEBUG && defined __GNUC__ && !defined __clang__
#if __GNUC__ >= 12
#pragma GCC pop_options
#endif
#endif

void KWDGPOGrouper::InitializeHashCellDictionary(NumericKeyDictionary* nkdHashCells, const KWDataGrid* dataGrid) const
{
	ObjectArray oaDataGridCells;
	int nCell;
	KWDGCell* previousCell;
	KWDGCell* cell;
	KWDGCell* hashCell;

	require(nkdHashCells != NULL);
	require(nkdHashCells->GetCount() == 0);
	require(dataGrid != NULL);
	require(dataGrid->SearchAttribute(GetPostOptimizationAttributeName()) != NULL);
	require(nKWDGPOGrouperPostOptimizationAttributeIndex == -1);

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Tri des cellules par signature partielle pour identifier l'ensemble des celulles partielles
	// Lors de la creation des vecteur d'effectifs des parties de l'attribut a post-optimiser,
	// on utilise comme identifiant de hash pour chaque cellule le pointeur de la premiere cellule
	// ayant meme signature partielle (en utilisant le tableau des cellules triees).

	// Recherche de l'index de l'attribut a post-optimiser, pour parametrer la fonction de comparaison
	nKWDGPOGrouperPostOptimizationAttributeIndex =
	    dataGrid->SearchAttribute(GetPostOptimizationAttributeName())->GetAttributeIndex();

	// Rangement des cellules de la grille dans un tableau
	oaDataGridCells.SetSize(dataGrid->GetCellNumber());
	nCell = 0;
	cell = dataGrid->GetHeadCell();
	while (cell != NULL)
	{
		// Ajout dans le tableau
		oaDataGridCells.SetAt(nCell, cell);
		nCell++;

		// Cellule suivante
		dataGrid->GetNextCell(cell);
	}

	// Tri du tableau par signature exogene
	oaDataGridCells.SetCompareFunction(KWDGPOGrouperCompareCell);
	oaDataGridCells.Sort();

	// Parcours du tableau associe les cellules a leur signature exogene
	// Les signatures sont les cellule correspondant a chaque changement de valeur de la fonction de tri
	hashCell = NULL;
	previousCell = NULL;
	for (nCell = 0; nCell < oaDataGridCells.GetSize(); nCell++)
	{
		cell = cast(KWDGCell*, oaDataGridCells.GetAt(nCell));

		// Detection de changement de valeur
		if (previousCell == NULL or KWDGPOGrouperCompareCell(&previousCell, &cell) != 0)
			hashCell = cell;
		previousCell = cell;

		// On associe la cellule a sa signature exogene
		nkdHashCells->SetAt(cell, hashCell);
	}

	// Reinitialisationde l'index de l'attribut a post-optimiser
	nKWDGPOGrouperPostOptimizationAttributeIndex = -1;

	ensure(nKWDGPOGrouperPostOptimizationAttributeIndex == -1);
	ensure(nkdHashCells->GetCount() == dataGrid->GetCellNumber());
}

void KWDGPOGrouper::InitializeGroupIndexes(IntVector* ivGroups, const KWDataGrid* initialDataGrid,
					   const KWDataGrid* optimizedDataGrid) const
{
	KWDGAttribute* initialAttribute;
	KWDGAttribute* optimizedAttribute;
	KWSortableIndex* kwsiOptimizedPartIndex;
	NumericKeyDictionary nkdOptimizedPartIndexes;
	KWDGPart* initialPart;
	KWDGPart* optimizedPart;
	int nIndex;

	require(ivGroups != NULL);
	require(initialDataGrid != NULL);
	require(optimizedDataGrid != NULL);

	// Recherche de l'attribut a post-optimiser
	initialAttribute = initialDataGrid->SearchAttribute(GetPostOptimizationAttributeName());
	optimizedAttribute = optimizedDataGrid->SearchAttribute(GetPostOptimizationAttributeName());
	check(initialAttribute);
	check(optimizedAttribute);

	// Memorisation des index des parties optimisees
	optimizedPart = optimizedAttribute->GetHeadPart();
	nIndex = 0;
	while (optimizedPart != NULL)
	{
		// Creation d'un objet memorisant l'index
		kwsiOptimizedPartIndex = new KWSortableIndex;
		kwsiOptimizedPartIndex->SetIndex(nIndex);

		// Memorisation dans un dictionaire
		nkdOptimizedPartIndexes.SetAt(optimizedPart, kwsiOptimizedPartIndex);

		// Partie suivante
		optimizedAttribute->GetNextPart(optimizedPart);
		nIndex++;
	}

	// Indexation
	assert(not initialAttribute->IsIndexed());
	optimizedAttribute->BuildIndexingStructure();

	// Recherche de l'index de groupe de chaque part initiale
	ivGroups->SetSize(initialAttribute->GetPartNumber());
	initialPart = initialAttribute->GetHeadPart();
	nIndex = 0;
	while (initialPart != NULL)
	{
		// Recherche de sa partie groupee correspondante
		optimizedPart = optimizedAttribute->LookupGroupablePart(initialPart->GetValueSet()->GetHeadValue());

		// Memorisation de l'index de ce groupe
		kwsiOptimizedPartIndex = cast(KWSortableIndex*, nkdOptimizedPartIndexes.Lookup(optimizedPart));
		check(kwsiOptimizedPartIndex);
		ivGroups->SetAt(nIndex, kwsiOptimizedPartIndex->GetIndex());

		// Partie suivante
		initialAttribute->GetNextPart(initialPart);
		nIndex++;
	}

	// Nettoyage
	optimizedAttribute->DeleteIndexingStructure();
	nkdOptimizedPartIndexes.DeleteAll();

	ensure(ivGroups->GetSize() == initialAttribute->GetPartNumber());
}

int KWDGPOGrouper::InitializeGroupIndexesAndGarbageIndex(IntVector* ivGroups, const KWDataGrid* initialDataGrid,
							 const KWDataGrid* optimizedDataGrid) const
{
	KWDGAttribute* initialAttribute;
	KWDGAttribute* optimizedAttribute;
	KWSortableIndex* kwsiOptimizedPartIndex;
	NumericKeyDictionary nkdOptimizedPartIndexes;
	KWDGPart* initialPart;
	KWDGPart* optimizedPart;
	int nIndex;
	int nGarbageGroupIndex;

	require(ivGroups != NULL);
	require(initialDataGrid != NULL);
	require(optimizedDataGrid != NULL);

	// Recherche de l'attribut a post-optimiser
	initialAttribute = initialDataGrid->SearchAttribute(GetPostOptimizationAttributeName());
	optimizedAttribute = optimizedDataGrid->SearchAttribute(GetPostOptimizationAttributeName());
	check(initialAttribute);
	check(optimizedAttribute);
	assert(KWType::IsCoclusteringGroupableType(initialAttribute->GetAttributeType()));

	// Memorisation des index des parties optimisees
	optimizedPart = optimizedAttribute->GetHeadPart();
	nIndex = 0;
	// Initialisation de l'index du groupe poubelle
	nGarbageGroupIndex = -1;

	while (optimizedPart != NULL)
	{
		// Creation d'un objet memorisant l'index
		kwsiOptimizedPartIndex = new KWSortableIndex;
		kwsiOptimizedPartIndex->SetIndex(nIndex);

		// Memorisation dans un dictionaire
		nkdOptimizedPartIndexes.SetAt(optimizedPart, kwsiOptimizedPartIndex);

		// Cas du groupe poubelle : memorisation de son index
		if (optimizedPart == optimizedAttribute->GetGarbagePart())
			nGarbageGroupIndex = nIndex;

		// Partie suivante
		optimizedAttribute->GetNextPart(optimizedPart);
		nIndex++;
	}

	// Indexation
	assert(not initialAttribute->IsIndexed());
	optimizedAttribute->BuildIndexingStructure();

	// Recherche de l'index de groupe de chaque part initiale
	ivGroups->SetSize(initialAttribute->GetPartNumber());
	initialPart = initialAttribute->GetHeadPart();
	nIndex = 0;
	while (initialPart != NULL)
	{
		assert(KWType::IsCoclusteringGroupableType(optimizedAttribute->GetAttributeType()));

		// Recherche de sa partie groupee correspondante
		optimizedPart = optimizedAttribute->LookupGroupablePart(initialPart->GetValueSet()->GetHeadValue());

		// Memorisation de l'index de ce groupe
		kwsiOptimizedPartIndex = cast(KWSortableIndex*, nkdOptimizedPartIndexes.Lookup(optimizedPart));
		check(kwsiOptimizedPartIndex);
		ivGroups->SetAt(nIndex, kwsiOptimizedPartIndex->GetIndex());

		// Partie suivante
		initialAttribute->GetNextPart(initialPart);
		nIndex++;
	}

	// Nettoyage
	optimizedAttribute->DeleteIndexingStructure();
	nkdOptimizedPartIndexes.DeleteAll();

	ensure(ivGroups->GetSize() == initialAttribute->GetPartNumber());

	return nGarbageGroupIndex;
}

void KWDGPOGrouper::InitializeGroupedFrequencyTableFromDataGrid(KWFrequencyTable* groupedFrequencyTable,
								const KWFrequencyTable* initialFrequencyTable,
								const IntVector* ivGroups, int nGroupNumber) const
{
	KWFrequencyVector* initialFrequencyVector;
	KWFrequencyVector* groupedFrequencyVector;
	int nInitial;
	int nGroup;

	require(groupedFrequencyTable != NULL);
	require(initialFrequencyTable != NULL);
	require(ivGroups != NULL);
	require(ivGroups->GetSize() == initialFrequencyTable->GetFrequencyVectorNumber());
	require(nGroupNumber >= 0);

	// Initialisation de la table d'effectif groupee
	groupedFrequencyTable->SetFrequencyVectorCreator(initialFrequencyTable->GetFrequencyVectorCreator()->Clone());
	groupedFrequencyTable->SetFrequencyVectorNumber(nGroupNumber);
	groupedFrequencyTable->SetInitialValueNumber(initialFrequencyTable->GetInitialValueNumber());
	groupedFrequencyTable->SetGranularizedValueNumber(initialFrequencyTable->GetGranularizedValueNumber());
	// Parametrage granularite
	groupedFrequencyTable->SetGranularity(initialFrequencyTable->GetGranularity());

	// Alimentation de la table d'effectif groupee
	for (nInitial = 0; nInitial < initialFrequencyTable->GetFrequencyVectorNumber(); nInitial++)
	{
		initialFrequencyVector = initialFrequencyTable->GetFrequencyVectorAt(nInitial);

		// Recherche de l'index du groupe correspondant
		nGroup = ivGroups->GetAt(nInitial);
		assert(0 <= nGroup and nGroup < nGroupNumber);

		// Concatenation de l'effectif initial dans le groupe
		groupedFrequencyVector = groupedFrequencyTable->GetFrequencyVectorAt(nGroup);
		AddFrequencyVector(groupedFrequencyVector, initialFrequencyVector);
	}

	ensure(groupedFrequencyTable->GetFrequencyVectorNumber() <= initialFrequencyTable->GetFrequencyVectorNumber());
	ensure(groupedFrequencyTable->GetTotalFrequency() <= initialFrequencyTable->GetTotalFrequency());
}

void KWDGPOGrouper::UpdateDataGridWithGarbageFromGroups(KWDataGrid* optimizedDataGrid,
							const KWDataGrid* initialDataGrid, const IntVector* ivGroups,
							int nGroupNumber) const
{
	boolean bDisplayResults = false;
	KWDataGridManager dataGridManager;
	KWDGAttribute* initialAttribute;
	KWDGAttribute* optimizedAttribute;
	KWDGPart* initialPart;
	KWDGPart* optimizedPart;
	int nGroup;
	int nInitial;
	ObjectArray oaOptimizedParts;

	require(optimizedDataGrid != NULL);
	require(initialDataGrid != NULL);
	require(optimizedDataGrid->SearchAttribute(GetPostOptimizationAttributeName()) != NULL);
	require(initialDataGrid->SearchAttribute(GetPostOptimizationAttributeName()) != NULL);

	// Acces aux attributs des grilles initiale et optimise pour l'attribut de post-optimisation
	initialAttribute = initialDataGrid->SearchAttribute(sPostOptimizationAttributeName);
	optimizedAttribute = optimizedDataGrid->SearchAttribute(sPostOptimizationAttributeName);
	assert(KWType::IsCoclusteringGroupableType(initialAttribute->GetAttributeType()));

	// On vide la grille optimisee de ses cellules, en preservant ses attribut et leur partition
	optimizedDataGrid->DeleteAllCells();

	// On reinitialise a vide les partie pour l'attribut a post-optimiser
	optimizedAttribute->DeleteAllParts();

	// Reinitialisation a vide du groupe poubelle
	optimizedAttribute->SetGarbagePart(NULL);

	// Creation des parties de l'attribut groupee et memorisation dans un tableau
	oaOptimizedParts.SetSize(nGroupNumber);
	for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
	{
		// Creation d'une nouvelle partie optimisee
		optimizedPart = optimizedAttribute->AddPart();
		oaOptimizedParts.SetAt(nGroup, optimizedPart);
	}

	// Parcours des parties initiales pour determiner les definitions des groupes
	initialPart = initialAttribute->GetHeadPart();
	nInitial = 0;
	while (initialPart != NULL)
	{
		// Recherche de l'index du groupe correspondant
		nGroup = ivGroups->GetAt(nInitial);
		assert(0 <= nGroup and nGroup < nGroupNumber);

		// Recherche de la partie optimisee a mettre a jour
		optimizedPart = cast(KWDGPart*, oaOptimizedParts.GetAt(nGroup));
		assert(KWType::IsCoclusteringGroupableType(optimizedAttribute->GetAttributeType()));

		// Mise a jour de la definition du groupe
		optimizedPart->GetValueSet()->UpgradeFrom(initialPart->GetValueSet());

		// Mise a jour du groupe poubelle comme le groupe contenant le plus de valeurs
		if (optimizedPart->GetValueSet()->GetValueNumber() > optimizedAttribute->GetGarbageModalityNumber())
			optimizedAttribute->SetGarbagePart(optimizedPart);

		// Partie initiale suivante
		initialAttribute->GetNextPart(initialPart);
		nInitial++;
	}

	// Export des cellules pour la grille initiale univariee
	dataGridManager.SetSourceDataGrid(initialDataGrid);
	dataGridManager.ExportCells(optimizedDataGrid);

	// Affichage des resultats
	if (bDisplayResults)
	{
		cout << "Preparation d'une grille pour l'optimisation univariee\t" << sPostOptimizationAttributeName
		     << endl;
		cout << "Grille initiale\n" << *initialDataGrid << endl;
		cout << "Grille optimisee\n" << *optimizedDataGrid << endl;
	}

	// Verification de la grille preparee
	ensure(optimizedAttribute->GetPartNumber() == nGroupNumber);
	ensure(initialDataGrid->GetGridFrequency() == optimizedDataGrid->GetGridFrequency());
	ensure(initialDataGrid->GetCellNumber() >= optimizedDataGrid->GetCellNumber());
}

void KWDGPOGrouper::InitializeFrequencyVector(KWFrequencyVector* kwfvFrequencyVector) const {}

boolean KWDGPOGrouper::CheckFrequencyVector(const KWFrequencyVector* kwfvFrequencyVector) const
{
	require(kwfvFrequencyVector != NULL);
	return true;
}

void KWDGPOGrouper::InitializeWorkingData(const KWFrequencyTable* kwftSource, int nInitialValueNumber) const
{
	require(kwftSource != NULL);
}

void KWDGPOGrouper::CleanWorkingData() const {}

void KWDGPOGrouper::AddFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
				       const KWFrequencyVector* kwfvAddedFrequencyVector) const
{
	boolean bDisplayResults = false;
	KWDGPOPartFrequencyVector* sourcePartFrequencyVector;
	const KWDGPOPartFrequencyVector* addedPartFrequencyVector;
	POSITION positionVector;
	KWDGPOCellFrequencyVector* sourceCellFrequencyVector;
	KWDGPOCellFrequencyVector* addedCellFrequencyVector;
	double dPartCost;
	debug(int nInitialFrequency);

	require(kwfvSourceFrequencyVector != NULL);
	require(kwfvAddedFrequencyVector != NULL);
	require(kwfvSourceFrequencyVector != kwfvAddedFrequencyVector);
	require(CheckFrequencyVector(kwfvSourceFrequencyVector));
	require(CheckFrequencyVector(kwfvAddedFrequencyVector));

	// Affichage des parties initiales
	if (bDisplayResults)
	{
		cout << "Ajout d'une partie\n";
		cout << "SourcePart\n" << *kwfvSourceFrequencyVector << endl;
		cout << "AddedPart\n" << *kwfvAddedFrequencyVector << endl;
	}

	// Acces aux vecteur d'effectif dans leur bon type
	sourcePartFrequencyVector = cast(KWDGPOPartFrequencyVector*, kwfvSourceFrequencyVector);
	addedPartFrequencyVector = cast(KWDGPOPartFrequencyVector*, kwfvAddedFrequencyVector);
	debug(nInitialFrequency = sourcePartFrequencyVector->ComputeTotalFrequency());

	// Gestion des nombres de valeurs
	sourcePartFrequencyVector->SetModalityNumber(sourcePartFrequencyVector->GetModalityNumber() +
						     addedPartFrequencyVector->GetModalityNumber());

	// Parcours des cellules a ajouter
	dPartCost = sourcePartFrequencyVector->GetCellCost();
	positionVector = addedPartFrequencyVector->GetHeadPosition();
	while (positionVector != NULL)
	{
		addedCellFrequencyVector = addedPartFrequencyVector->GetNextPosition(positionVector);

		// Recherche de la cellule correspondante dans la partie a enrichir
		sourceCellFrequencyVector =
		    sourcePartFrequencyVector->LookupCell(addedCellFrequencyVector->GetHashObject());

		// Ajout de l'effectif de la cellule a la partie
		sourcePartFrequencyVector->SetFrequency(sourcePartFrequencyVector->GetFrequency() +
							addedCellFrequencyVector->GetCellFrequency());

		// Creation d'une nouvelle cellule si necessaire
		if (sourceCellFrequencyVector == NULL)
		{
			// Creation d'une nouvelle cellule
			sourceCellFrequencyVector = new KWDGPOCellFrequencyVector;
			sourceCellFrequencyVector->CopyFrom(addedCellFrequencyVector);

			// Ajout dans la partie
			sourcePartFrequencyVector->AddCell(sourceCellFrequencyVector);

			// Mise a jour du cout de la partie
			dPartCost += sourceCellFrequencyVector->GetCost();
		}
		else
		// Sinon, concatenation du contenu de la cellule
		{
			// On enleve au prealable le cout de la cellule a enrichir
			dPartCost -= sourceCellFrequencyVector->GetCost();

			// On met a jour la cellule et de son cout
			sourceCellFrequencyVector->AddFrequenciesFrom(addedCellFrequencyVector);
			sourceCellFrequencyVector->SetCost(ComputeCellCost(sourceCellFrequencyVector));

			// On ajoute le cout de la cellule modifiee
			dPartCost += sourceCellFrequencyVector->GetCost();
		}
	}
	sourcePartFrequencyVector->SetCellCost(dPartCost);

	// Affichage de la partie modifiee
	if (bDisplayResults)
		cout << "ModifiedSourcePart\n" << *kwfvSourceFrequencyVector << endl;

	ensure(sourcePartFrequencyVector->GetCellNumber() >= addedPartFrequencyVector->GetCellNumber());
	debug(ensure(sourcePartFrequencyVector->ComputeTotalFrequency() ==
		     nInitialFrequency + addedPartFrequencyVector->ComputeTotalFrequency()));
}

void KWDGPOGrouper::RemoveFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
					  const KWFrequencyVector* kwfvRemovedFrequencyVector) const
{
	boolean bDisplayResults = false;
	KWDGPOPartFrequencyVector* sourcePartFrequencyVector;
	const KWDGPOPartFrequencyVector* removedPartFrequencyVector;
	POSITION positionVector;
	KWDGPOCellFrequencyVector* sourceCellFrequencyVector;
	KWDGPOCellFrequencyVector* removedCellFrequencyVector;
	double dPartCost;
	debug(int nInitialFrequency);

	require(kwfvSourceFrequencyVector != NULL);
	require(kwfvRemovedFrequencyVector != NULL);
	require(kwfvSourceFrequencyVector != kwfvRemovedFrequencyVector);
	require(CheckFrequencyVector(kwfvSourceFrequencyVector));
	require(CheckFrequencyVector(kwfvRemovedFrequencyVector));

	// Affichage des parties initiales
	if (bDisplayResults)
	{
		cout << "Supression d'une partie\n";
		cout << "SourcePart\n" << *kwfvSourceFrequencyVector << endl;
		cout << "RemovedPart\n" << *kwfvRemovedFrequencyVector << endl;
	}

	// Acces aux vecteur d'effectif dans leur bon type
	sourcePartFrequencyVector = cast(KWDGPOPartFrequencyVector*, kwfvSourceFrequencyVector);
	removedPartFrequencyVector = cast(KWDGPOPartFrequencyVector*, kwfvRemovedFrequencyVector);
	assert(sourcePartFrequencyVector->GetFrequency() == sourcePartFrequencyVector->ComputeTotalFrequency());
	assert(removedPartFrequencyVector->GetFrequency() == removedPartFrequencyVector->ComputeTotalFrequency());
	debug(nInitialFrequency = sourcePartFrequencyVector->ComputeTotalFrequency());

	// Gestion des nombres de valeurs
	sourcePartFrequencyVector->SetModalityNumber(sourcePartFrequencyVector->GetModalityNumber() -
						     removedPartFrequencyVector->GetModalityNumber());

	// Parcours des cellules a retrancher
	dPartCost = sourcePartFrequencyVector->GetCellCost();
	positionVector = removedPartFrequencyVector->GetHeadPosition();
	while (positionVector != NULL)
	{
		removedCellFrequencyVector = removedPartFrequencyVector->GetNextPosition(positionVector);
		assert(removedCellFrequencyVector->ComputeTotalFrequency() > 0);

		// Recherche de la cellule correspondante dans la partie a enrichir
		sourceCellFrequencyVector =
		    sourcePartFrequencyVector->LookupCell(removedCellFrequencyVector->GetHashObject());
		check(sourceCellFrequencyVector);
		assert(sourceCellFrequencyVector->ComputeTotalFrequency() >=
		       removedCellFrequencyVector->ComputeTotalFrequency());

		// Supression de l'effectif de la cellule de la partie
		sourcePartFrequencyVector->SetFrequency(sourcePartFrequencyVector->GetFrequency() -
							removedCellFrequencyVector->GetCellFrequency());

		// On enleve au prealable le cout de la cellule initiale
		dPartCost -= sourceCellFrequencyVector->GetCost();

		// Si la cellule devient vide, on la supprime
		if (sourceCellFrequencyVector->ComputeTotalFrequency() ==
		    removedCellFrequencyVector->ComputeTotalFrequency())
			sourcePartFrequencyVector->DeleteCell(sourceCellFrequencyVector);
		// Sinon, on retranche les effectifs de la cellule a supprimer
		else
		{
			// On met a jour la cellule et son cout
			sourceCellFrequencyVector->RemoveFrequenciesFrom(removedCellFrequencyVector);
			assert(sourceCellFrequencyVector->ComputeTotalFrequency() > 0);
			sourceCellFrequencyVector->SetCost(ComputeCellCost(sourceCellFrequencyVector));

			// On ajoute le cout de la cellule modifiee
			dPartCost += sourceCellFrequencyVector->GetCost();
		}
	}
	sourcePartFrequencyVector->SetCellCost(dPartCost);

	// Affichage de la partie modifiee
	if (bDisplayResults)
		cout << "ModifiedSourcePart\n" << *kwfvSourceFrequencyVector << endl;

	ensure(sourcePartFrequencyVector->GetFrequency() == sourcePartFrequencyVector->ComputeTotalFrequency());
	debug(ensure(sourcePartFrequencyVector->ComputeTotalFrequency() ==
		     nInitialFrequency - removedPartFrequencyVector->ComputeTotalFrequency()));
}

void KWDGPOGrouper::MergeFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector,
					  const KWFrequencyVector* kwfvMergedFrequencyVector1,
					  const KWFrequencyVector* kwfvMergedFrequencyVector2) const
{
	require(kwfvSourceFrequencyVector != NULL);
	require(kwfvMergedFrequencyVector1 != NULL);
	require(kwfvMergedFrequencyVector2 != NULL);
	require(CheckFrequencyVector(kwfvSourceFrequencyVector));
	require(CheckFrequencyVector(kwfvMergedFrequencyVector1));
	require(CheckFrequencyVector(kwfvMergedFrequencyVector2));

	// On copie le vecteur source ayant le plus de cellules, et on ajoute l'autre
	if (cast(KWDGPOPartFrequencyVector*, kwfvMergedFrequencyVector1)->GetCellNumber() >=
	    cast(KWDGPOPartFrequencyVector*, kwfvMergedFrequencyVector2)->GetCellNumber())
	{
		kwfvSourceFrequencyVector->CopyFrom(kwfvMergedFrequencyVector1);
		AddFrequencyVector(kwfvSourceFrequencyVector, kwfvMergedFrequencyVector2);
	}
	else
	{
		kwfvSourceFrequencyVector->CopyFrom(kwfvMergedFrequencyVector2);
		AddFrequencyVector(kwfvSourceFrequencyVector, kwfvMergedFrequencyVector1);
	}
	ensure(kwfvSourceFrequencyVector->ComputeTotalFrequency() ==
	       kwfvMergedFrequencyVector1->ComputeTotalFrequency() +
		   kwfvMergedFrequencyVector2->ComputeTotalFrequency());
}

double KWDGPOGrouper::ComputeGroupUnionCost(const KWFrequencyVector* sourceGroup1,
					    const KWFrequencyVector* sourceGroup2) const
{
	double dCost;
	KWDGPOPartFrequencyVector* sourcePartFrequencyVector;
	const KWDGPOPartFrequencyVector* addedPartFrequencyVector;
	POSITION positionVector;
	KWDGPOCellFrequencyVector* sourceCellFrequencyVector;
	KWDGPOCellFrequencyVector* addedCellFrequencyVector;
	debug(int nInitialFrequency);
	debug(int nFinalFrequency);

	require(sourceGroup1 != NULL);
	require(sourceGroup2 != NULL);
	require(CheckFrequencyVector(sourceGroup1));
	require(CheckFrequencyVector(sourceGroup2));

	// Acces aux vecteur d'effectif dans leur bon type, en prenant pour source
	// celui ayant le plus de cellules
	if (cast(KWDGPOPartFrequencyVector*, sourceGroup1)->GetCellNumber() >=
	    cast(KWDGPOPartFrequencyVector*, sourceGroup2)->GetCellNumber())
	{
		sourcePartFrequencyVector = cast(KWDGPOPartFrequencyVector*, sourceGroup1);
		addedPartFrequencyVector = cast(KWDGPOPartFrequencyVector*, sourceGroup2);
	}
	else
	{
		sourcePartFrequencyVector = cast(KWDGPOPartFrequencyVector*, sourceGroup2);
		addedPartFrequencyVector = cast(KWDGPOPartFrequencyVector*, sourceGroup1);
	}

	// Verifications initiales
	assert(sourcePartFrequencyVector->GetFrequency() == sourcePartFrequencyVector->ComputeTotalFrequency());
	assert(addedPartFrequencyVector->GetFrequency() == addedPartFrequencyVector->ComputeTotalFrequency());
	debug(nInitialFrequency = sourcePartFrequencyVector->ComputeTotalFrequency() +
				  addedPartFrequencyVector->ComputeTotalFrequency());

	// Prise en compte du cout de partie de l'union
	dCost = cast(KWDataGridUnivariateCosts*, GetGroupingCosts())->ComputePartUnionCost(sourceGroup1, sourceGroup2);

	// Parcours des cellules a retrancher
	dCost += sourcePartFrequencyVector->GetCellCost();
	positionVector = addedPartFrequencyVector->GetHeadPosition();
	debug(nFinalFrequency = sourcePartFrequencyVector->ComputeTotalFrequency());
	while (positionVector != NULL)
	{
		addedCellFrequencyVector = addedPartFrequencyVector->GetNextPosition(positionVector);
		assert(addedCellFrequencyVector->ComputeTotalFrequency() > 0);

		// Recherche de la cellule correspondante dans la partie a enrichir
		sourceCellFrequencyVector =
		    sourcePartFrequencyVector->LookupCell(addedCellFrequencyVector->GetHashObject());

		// Simulation du cout d'une nouvelle cellule si necessaire
		if (sourceCellFrequencyVector == NULL)
		{
			dCost += addedCellFrequencyVector->GetCost();
			debug(nFinalFrequency += addedCellFrequencyVector->ComputeTotalFrequency());
		}
		// Sinon, simulation de la fusion des contenus de cellule
		else
		{
			// On enleve au prealable le cout de la cellule a enrichir
			dCost -= sourceCellFrequencyVector->GetCost();
			debug(nFinalFrequency -= sourceCellFrequencyVector->ComputeTotalFrequency());

			// On met a jour les effectif de la cellule
			sourceCellFrequencyVector->AddFrequenciesFrom(addedCellFrequencyVector);
			debug(nFinalFrequency += sourceCellFrequencyVector->ComputeTotalFrequency());

			// Prise en compte du nouveau cout
			dCost += ComputeCellCost(sourceCellFrequencyVector);

			// Onrestitue les effectif initiaux de la cellule
			sourceCellFrequencyVector->RemoveFrequenciesFrom(addedCellFrequencyVector);
		}
	}

	// Verifications finales
	assert(sourcePartFrequencyVector->GetFrequency() == sourcePartFrequencyVector->ComputeTotalFrequency());
	assert(addedPartFrequencyVector->GetFrequency() == addedPartFrequencyVector->ComputeTotalFrequency());
	debug(assert(nInitialFrequency == sourcePartFrequencyVector->ComputeTotalFrequency() +
					      addedPartFrequencyVector->ComputeTotalFrequency()));
	debug(assert(nInitialFrequency == nFinalFrequency));

	return dCost;
}

double KWDGPOGrouper::ComputeGroupDiffCost(const KWFrequencyVector* sourceGroup,
					   const KWFrequencyVector* removedGroup) const
{
	double dCost;
	KWDGPOPartFrequencyVector* sourcePartFrequencyVector;
	const KWDGPOPartFrequencyVector* removedPartFrequencyVector;
	POSITION positionVector;
	KWDGPOCellFrequencyVector* sourceCellFrequencyVector;
	KWDGPOCellFrequencyVector* removedCellFrequencyVector;
	debug(int nInitialFrequency);
	debug(int nFinalFrequency);

	require(sourceGroup != NULL);
	require(removedGroup != NULL);
	require(CheckFrequencyVector(sourceGroup));
	require(CheckFrequencyVector(removedGroup));

	// Acces aux vecteur d'effectif dans leur bon type
	sourcePartFrequencyVector = cast(KWDGPOPartFrequencyVector*, sourceGroup);
	removedPartFrequencyVector = cast(KWDGPOPartFrequencyVector*, removedGroup);

	// Verifications initiales
	assert(sourcePartFrequencyVector->GetFrequency() == sourcePartFrequencyVector->ComputeTotalFrequency());
	assert(removedPartFrequencyVector->GetFrequency() == removedPartFrequencyVector->ComputeTotalFrequency());
	debug(nInitialFrequency = sourcePartFrequencyVector->ComputeTotalFrequency() -
				  removedPartFrequencyVector->ComputeTotalFrequency());

	// Prise en compte du cout de partie de la difference
	dCost = cast(KWDataGridUnivariateCosts*, GetGroupingCosts())->ComputePartDiffCost(sourceGroup, removedGroup);

	// Parcours des cellules a retrancher
	dCost += sourcePartFrequencyVector->GetCellCost();
	positionVector = removedPartFrequencyVector->GetHeadPosition();
	debug(nFinalFrequency = sourcePartFrequencyVector->ComputeTotalFrequency());
	while (positionVector != NULL)
	{
		removedCellFrequencyVector = removedPartFrequencyVector->GetNextPosition(positionVector);
		assert(removedCellFrequencyVector->ComputeTotalFrequency() > 0);

		// Recherche de la cellule correspondante dans la partie a enrichir
		sourceCellFrequencyVector =
		    sourcePartFrequencyVector->LookupCell(removedCellFrequencyVector->GetHashObject());
		check(sourceCellFrequencyVector);
		assert(sourceCellFrequencyVector->ComputeTotalFrequency() >=
		       removedCellFrequencyVector->ComputeTotalFrequency());

		// On enleve au prealable le cout de la cellule initiale
		dCost -= sourceCellFrequencyVector->GetCost();

		// Si la cellule reste non vide, on evalue son nouveau cout
		debug(nFinalFrequency -= removedCellFrequencyVector->ComputeTotalFrequency());
		if (sourceCellFrequencyVector->ComputeTotalFrequency() !=
		    removedCellFrequencyVector->ComputeTotalFrequency())
		{
			// On retranche les effectifs de la cellule a supprimer
			sourceCellFrequencyVector->RemoveFrequenciesFrom(removedCellFrequencyVector);
			assert(sourceCellFrequencyVector->ComputeTotalFrequency() > 0);

			// On ajoute le cout de la cellule modifiee
			dCost += ComputeCellCost(sourceCellFrequencyVector);

			// On restitue les effectifs initiaux
			sourceCellFrequencyVector->AddFrequenciesFrom(removedCellFrequencyVector);
		}
	}

	// Verifications finales
	assert(sourcePartFrequencyVector->GetFrequency() == sourcePartFrequencyVector->ComputeTotalFrequency());
	assert(removedPartFrequencyVector->GetFrequency() == removedPartFrequencyVector->ComputeTotalFrequency());
	debug(assert(nInitialFrequency == sourcePartFrequencyVector->ComputeTotalFrequency() -
					      removedPartFrequencyVector->ComputeTotalFrequency()));
	debug(assert(nInitialFrequency == nFinalFrequency));

	return dCost;
}

double KWDGPOGrouper::ComputeCellCost(const KWDGPOCellFrequencyVector* cellFrequencyVector) const
{
	return cast(KWDataGridUnivariateCosts*, GetGroupingCosts())->ComputeCellCost(cellFrequencyVector);
}

////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridUnivariateCosts

KWDataGridUnivariateCosts::KWDataGridUnivariateCosts()
{
	// Parametrage des cout
	dataGridCosts = NULL;
	dataGridCostParameter = NULL;
	attributeCostParameter = NULL;
	partCostParameter = NULL;
	cellCostParameter = NULL;
	dExcludedConstantCost = 0;
	nFreshness = 0;
	nInitializationFreshness = 0;

	// Redefinition du creator de vecteur d'effectif gere par la classe
	// (on detruit prealablement la version initialisee par la classe ancetre)
	delete kwfvFrequencyVectorCreator;
	kwfvFrequencyVectorCreator = new KWDGPOPartFrequencyVector;

	// Cout lie au partitionnement d'un attribut cible
	attributeSourceCostParameter = NULL;
	partSourceCostParameter = NULL;
	attributeTargetCostParameter = NULL;
}

KWDataGridUnivariateCosts::~KWDataGridUnivariateCosts()
{
	CleanUnivariateCostParameters();
	// Le destructeur de la classe ancetre detruit le creator de vecteur d'effectif
}

void KWDataGridUnivariateCosts::SetPostOptimizationAttributeName(const ALString& sValue)
{
	sPostOptimizationAttributeName = sValue;
	nFreshness++;
}

const ALString& KWDataGridUnivariateCosts::GetPostOptimizationAttributeName() const
{
	return sPostOptimizationAttributeName;
}

void KWDataGridUnivariateCosts::SetDataGridCosts(const KWDataGridCosts* kwdgcCosts)
{
	dataGridCosts = kwdgcCosts;
	nFreshness++;
}

const KWDataGridCosts* KWDataGridUnivariateCosts::GetDataGridCosts() const
{
	return dataGridCosts;
}

void KWDataGridUnivariateCosts::InitializeUnivariateCostParameters(const KWDataGrid* optimizedDataGrid)
{
	KWDGAttribute* postOptimizedAttribute;
	int nAttribute;
	KWDGAttribute* attribute;
	KWDGPart* part;
	int nPartitionSize;

	require(GetPostOptimizationAttributeName() != "");
	require(GetDataGridCosts() != NULL);
	require(optimizedDataGrid != NULL);
	require(optimizedDataGrid->SearchAttribute(GetPostOptimizationAttributeName()) != NULL);

	// Nettoyage initial du parametrage des couts
	CleanUnivariateCostParameters();

	// Recherche de l'attribut a post-optimiser
	postOptimizedAttribute = optimizedDataGrid->SearchAttribute(GetPostOptimizationAttributeName());
	check(postOptimizedAttribute);

	// Memorisation des caracteristique de la grille, en excluant l'attribut a post-optimiser
	dataGridCostParameter = new KWDataGridCostParameter;
	dataGridCostParameter->nGridFrequency = optimizedDataGrid->GetGridFrequency();
	dataGridCostParameter->dLnGridSize =
	    optimizedDataGrid->GetLnGridSize() - log((double)postOptimizedAttribute->GetPartNumber());
	dataGridCostParameter->nInformativeAttributeNumber = optimizedDataGrid->GetInformativeAttributeNumber();
	if (postOptimizedAttribute->GetPartNumber() > 1)
		dataGridCostParameter->nInformativeAttributeNumber--;
	dataGridCostParameter->nGranularity = optimizedDataGrid->GetGranularity();

	// Memorisation des caracteristiques de l'attribut a post-optimiser
	attributeCostParameter = new KWDGAttributeCostParameter;
	attributeCostParameter->dataGrid = dataGridCostParameter;
	attributeCostParameter->SetAttributeName(postOptimizedAttribute->GetAttributeName());
	attributeCostParameter->SetAttributeType(postOptimizedAttribute->GetAttributeType());
	attributeCostParameter->SetAttributeTargetFunction(postOptimizedAttribute->GetAttributeTargetFunction());
	attributeCostParameter->SetCost(postOptimizedAttribute->GetCost());
	attributeCostParameter->nPartNumber = postOptimizedAttribute->GetPartNumber();
	attributeCostParameter->nAttributeIndex = postOptimizedAttribute->GetAttributeIndex();
	attributeCostParameter->SetInitialValueNumber(postOptimizedAttribute->GetInitialValueNumber());
	attributeCostParameter->SetGranularizedValueNumber(postOptimizedAttribute->GetGranularizedValueNumber());
	attributeCostParameter->nGarbageModalityNumber = postOptimizedAttribute->GetGarbageModalityNumber();

	// On memorise les attributs internes en status partage
	if (postOptimizedAttribute->GetAttributeType() == KWType::VarPart)
	{
		attributeCostParameter->SetInnerAttributes(postOptimizedAttribute->GetInnerAttributes());
	}

	// Caracteristiques generales du cout en univarie
	nValueNumber = attributeCostParameter->GetGranularizedValueNumber();
	nClassValueNumber = dataGridCostParameter->GetTargetValueNumber();
	nGranularity = dataGridCostParameter->GetGranularity();
	nTotalInstanceNumber = dataGridCostParameter->nGridFrequency;

	// Creation de la partie (en la liant a son attribut)
	partCostParameter = cast(KWDGPartCostParameter*, attributeCostParameter->AddPart());

	// Creation d'une cellule
	cellCostParameter = new KWDGCellCostParameter;

	// Calcul de la partie du cout de grille portee par les autres attributs
	dExcludedConstantCost = 0;
	for (nAttribute = 0; nAttribute < optimizedDataGrid->GetAttributeNumber(); nAttribute++)
	{
		attribute = optimizedDataGrid->GetAttributeAt(nAttribute);

		// Prise en compte des attributs sauf de l'attribut post-optimise
		if (attribute->GetAttributeName() != GetPostOptimizationAttributeName())
		{
			// Ajout du cout de l'attribut
			dExcludedConstantCost +=
			    dataGridCosts->ComputeAttributeCost(attribute, attribute->GetPartNumber());

			// Ajout du cout des parties de l'attribut dans le cas d'un critere de cout additif
			// (ou de l'optimisation d'un attribut source pour un critere non additif)
			if (not attributeCostParameter->GetAttributeTargetFunction())
			{
				part = attribute->GetHeadPart();
				while (part != NULL)
				{
					dExcludedConstantCost += dataGridCosts->ComputePartCost(part);
					attribute->GetNextPart(part);
				}
			}
			// Initialisations supplementaires pour l'optimisation d'un attribut cible d'un critere non
			// additif
			else
			{
				// Implemente uniquement dans le cas d'un attribut source et d'un attribut cible
				assert(optimizedDataGrid->GetAttributeNumber() == 2);
				assert(not attribute->GetAttributeTargetFunction());
				assert(attributeCostParameter->GetAttributeTargetFunction());

				// On memorise le lien entre la grille et l'attribut cible
				dataGridCostParameter->SetTargetAttribute(attributeCostParameter);

				// Initialisation du vecteur memorisant les couts de partie source
				dvTargetPartitionedCost.SetSize(optimizedDataGrid->GetGridFrequency());
				for (nPartitionSize = 0; nPartitionSize < optimizedDataGrid->GetGridFrequency();
				     nPartitionSize++)
					dvTargetPartitionedCost.SetAt(nPartitionSize, -1.0);

				// Memorisation des caracteristiques de l'attribut source
				attributeSourceCostParameter = new KWDGAttributeCostParameter;
				attributeSourceCostParameter->dataGrid = dataGridCostParameter;
				attributeSourceCostParameter->SetAttributeName(attribute->GetAttributeName());
				attributeSourceCostParameter->SetAttributeType(attribute->GetAttributeType());
				attributeSourceCostParameter->SetAttributeTargetFunction(
				    attribute->GetAttributeTargetFunction());
				attributeSourceCostParameter->SetInitialValueNumber(attribute->GetInitialValueNumber());
				attributeSourceCostParameter->SetGranularizedValueNumber(
				    attribute->GetGranularizedValueNumber());
				attributeSourceCostParameter->nPartNumber = attribute->GetPartNumber();
				attributeSourceCostParameter->SetCost(attribute->GetCost());
				attributeSourceCostParameter->nAttributeIndex = attribute->GetAttributeIndex();

				// Creation de la partie (en la liant a son attribut)
				partSourceCostParameter =
				    cast(KWDGPartCostParameter*, attributeSourceCostParameter->AddPart());

				// Memorisation des effectifs des parties source
				part = attribute->GetHeadPart();
				while (part != NULL)
				{
					ivFixedSourceFrequencies.Add(part->GetPartFrequency());
					attribute->GetNextPart(part);
				}
			}
		}
	}

	// Memorisation si necessaire des caracteristiques de l'attribut cible
	if (optimizedDataGrid->GetTargetAttribute() != NULL and
	    optimizedDataGrid->GetTargetAttribute()->GetAttributeName() != GetPostOptimizationAttributeName())
	{
		assert(dataGridCostParameter->GetTargetAttribute() == NULL);
		attributeTargetCostParameter = new KWDGAttributeCostParameter;
		attributeTargetCostParameter->dataGrid = dataGridCostParameter;
		attributeTargetCostParameter->SetAttributeName(
		    optimizedDataGrid->GetTargetAttribute()->GetAttributeName());
		attributeTargetCostParameter->SetAttributeType(
		    optimizedDataGrid->GetTargetAttribute()->GetAttributeType());
		attributeTargetCostParameter->SetAttributeTargetFunction(
		    optimizedDataGrid->GetTargetAttribute()->GetAttributeTargetFunction());
		attributeTargetCostParameter->SetInitialValueNumber(
		    optimizedDataGrid->GetTargetAttribute()->GetInitialValueNumber());
		attributeTargetCostParameter->SetGranularizedValueNumber(
		    optimizedDataGrid->GetTargetAttribute()->GetGranularizedValueNumber());
		attributeTargetCostParameter->SetCost(optimizedDataGrid->GetTargetAttribute()->GetCost());
		attributeTargetCostParameter->nPartNumber = optimizedDataGrid->GetTargetAttribute()->GetPartNumber();
		attributeTargetCostParameter->nAttributeIndex =
		    optimizedDataGrid->GetTargetAttribute()->GetAttributeIndex();
		assert(dataGridCostParameter->GetTargetAttribute() == attributeTargetCostParameter);
	}

	// Prise en compte des attributs hors grille et des valeurs symboliques
	dExcludedConstantCost += dataGridCosts->ComputeDataGridTotalMissingAttributeCost(optimizedDataGrid);
	dExcludedConstantCost += dataGridCosts->GetAllValuesDefaultCost();

	// Memorisation de la fraicheur d'initialisation
	nInitializationFreshness = nFreshness;
}

void KWDataGridUnivariateCosts::CleanUnivariateCostParameters()
{
	assert(partCostParameter == NULL or partCostParameter->GetAttribute() == attributeCostParameter);
	if (dataGridCostParameter != NULL)
		delete dataGridCostParameter;
	if (attributeCostParameter != NULL)
		delete attributeCostParameter;
	if (cellCostParameter != NULL)
		delete cellCostParameter;
	dataGridCostParameter = NULL;
	attributeCostParameter = NULL;
	partCostParameter = NULL;
	cellCostParameter = NULL;
	dExcludedConstantCost = 0;
	nValueNumber = 0;
	nClassValueNumber = 0;

	// Parametrage des couts d'attribut cible
	assert(partSourceCostParameter == NULL or
	       partSourceCostParameter->GetAttribute() == attributeSourceCostParameter);
	if (attributeSourceCostParameter != NULL)
		delete attributeSourceCostParameter;
	attributeSourceCostParameter = NULL;
	partSourceCostParameter = NULL;
	if (attributeTargetCostParameter != NULL)
		delete attributeTargetCostParameter;
	attributeTargetCostParameter = NULL;
	ivFixedSourceFrequencies.SetSize(0);
	dvTargetPartitionedCost.SetSize(0);
}

boolean KWDataGridUnivariateCosts::IsInitialized() const
{
	return nInitializationFreshness == nFreshness;
}

int KWDataGridUnivariateCosts::GetValueNumber() const
{
	require(IsInitialized());
	return attributeCostParameter->GetGranularizedValueNumber();
}

KWUnivariatePartitionCosts* KWDataGridUnivariateCosts::Create() const
{
	return new KWDataGridUnivariateCosts;
}

double KWDataGridUnivariateCosts::ComputePartitionCost(int nPartNumber) const
{
	double dCost;
	double dLnGridSize;
	int nInformativeAttributeNumber;

	require(IsInitialized());
	require(nPartNumber >= 1);

	// Initialisation des parametres grilles et attributs de la structure de cout
	dLnGridSize = dataGridCostParameter->dLnGridSize + log((double)nPartNumber);
	nInformativeAttributeNumber = dataGridCostParameter->nInformativeAttributeNumber;
	if (nPartNumber > 1)
		nInformativeAttributeNumber++;

	// Calcul des couts
	dCost = dExcludedConstantCost +
		dataGridCosts->ComputeDataGridCost(dataGridCostParameter, dLnGridSize, nInformativeAttributeNumber) +
		dataGridCosts->ComputeAttributeCost(attributeCostParameter, nPartNumber);

	// Couts supplementaires dans le cas de l'optimisation d'un attribut cible pour un critere non additif
	if (attributeCostParameter->GetAttributeTargetFunction())
	{
		int nPartSourceNumber;
		double dSourceCost;

		// Mise a jour de la taille de la partition cible dans la classe de cout
		attributeCostParameter->nPartNumber = nPartNumber;

		// Dans le cas d'un attribut cible, ajout de la somme des couts de parties de l'attribut source
		assert(dataGridCostParameter->nInformativeAttributeNumber == 0 or
		       dvTargetPartitionedCost.GetSize() >= nPartNumber);
		if (dvTargetPartitionedCost.GetSize() >= nPartNumber) // BBBBB > nPartNumber)
		{
			if (dvTargetPartitionedCost.GetAt(nPartNumber - 1) == -1.0)
			{
				// Parcours des parties
				dSourceCost = 0;
				for (nPartSourceNumber = 0; nPartSourceNumber < ivFixedSourceFrequencies.GetSize();
				     nPartSourceNumber++)
				{
					partSourceCostParameter->nPartFrequency =
					    ivFixedSourceFrequencies.GetAt(nPartSourceNumber);
					dSourceCost += dataGridCosts->ComputePartCost(partSourceCostParameter);
				}

				// Memorisation du cout pour cette taille de partition
				dvTargetPartitionedCost.SetAt(nPartNumber - 1, dSourceCost);
			}

			// Ajout de cette somme de couts
			dCost += dvTargetPartitionedCost.GetAt(nPartNumber - 1);
		}
	}

	return dCost;
}

double KWDataGridUnivariateCosts::ComputePartitionCost(int nPartNumber, int nGarbageModalityNumber) const
{
	// Parametrage de la taille du groupe poubelle
	attributeCostParameter->SetGarbageModalityNumber(nGarbageModalityNumber);
	return ComputePartitionCost(nPartNumber);
}

double KWDataGridUnivariateCosts::ComputePartCost(const KWFrequencyVector* part) const
{
	double dCost;

	require(IsInitialized());
	require(part != NULL);
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	// Initialisation du parametrage de la partie
	partCostParameter->nPartFrequency = cast(KWDGPOPartFrequencyVector*, part)->GetFrequency();

	// Specification du nombre de valeurs pour une partie d'un attribut groupable
	// Attention: on utilise ici un acces direct au nombre de valeur du ValueSet pour permettre
	// d'utiliser ce parametre dans les calculs de cout
	// (mais il y a incompletude du ValueSet, qui n'est specifie que pour ce qui est utile pour le calcul des couts)
	if (KWType::IsCoclusteringGroupableType(partCostParameter->GetPartType()))
	{
		// On peut modifier directement le nombre de valeur de facon generique, puisque que l'on est en mode emule
		// via les sous-classes KWDGSymbolValueSetCostParameter et KWDGVarPartSetCostParameter
		// N'etant jamais la partie par defaut en mode emule, on a le bon nombre de valeurs
		assert(not partCostParameter->GetValueSet()->IsDefaultPart());
		cast(KWDGValueSet*, partCostParameter->GetValueSet())
		    ->SetValueNumber(cast(KWDGPOPartFrequencyVector*, part)->GetModalityNumber());
	}

	// Cout: cout local de la partie + somme des cout des cellules (maintenu dans la partie)
	dCost =
	    dataGridCosts->ComputePartCost(partCostParameter) + cast(KWDGPOPartFrequencyVector*, part)->GetCellCost();
	return dCost;
}

double KWDataGridUnivariateCosts::ComputePartitionDeltaCost(int nPartNumber) const
{
	double dDeltaCost;

	require(nPartNumber > 1);

	// Variation du cout du au codage d'un intervalle en moins
	dDeltaCost = ComputePartitionCost(nPartNumber - 1) - ComputePartitionCost(nPartNumber);
	return dDeltaCost;
}

double KWDataGridUnivariateCosts::ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const
{
	require(nGarbageModalityNumber == 0);
	return ComputePartitionDeltaCost(nPartNumber);
}

double KWDataGridUnivariateCosts::ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const
{
	double dCost;
	int i;

	require(partTable != NULL);
	require(nGranularity == partTable->GetGranularity());
	require(partTable->GetFrequencyVectorAt(0)->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	// Cout de partition plus somme des couts des parties
	dCost = ComputePartitionCost(partTable->GetFrequencyVectorNumber());

	for (i = 0; i < partTable->GetFrequencyVectorNumber(); i++)
		dCost += ComputePartCost(partTable->GetFrequencyVectorAt(i));

	return dCost;
}

double KWDataGridUnivariateCosts::ComputePartUnionCost(const KWFrequencyVector* sourcePart1,
						       const KWFrequencyVector* sourcePart2) const
{
	double dCost;

	require(IsInitialized());
	require(sourcePart1 != NULL);
	require(sourcePart2 != NULL);
	require(sourcePart1->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());
	require(sourcePart2->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	// Initialisation du parametrage de la partie
	partCostParameter->nPartFrequency = cast(KWDGPOPartFrequencyVector*, sourcePart1)->GetFrequency() +
					    cast(KWDGPOPartFrequencyVector*, sourcePart2)->GetFrequency();

	// Specification du nombre de valeurs pour une partie symbolique
	// Attention: on utilise ici un acces direct au nombre de valeur du ValueSet pour permettre
	// d'utiliser ce parametre dans les calculs de cout
	// (mais il y a incompletude du ValueSet, qui n'est specifie que pour ce qui est utile pour le calcul des couts)
	if (KWType::IsCoclusteringGroupableType(partCostParameter->GetPartType()))
	{
		// On peut modifier directement le nombre de valeur de facon generique, puisque que l'on est en mode emule
		// via les sous-classes KWDGSymbolValueSetCostParameter et KWDGVarPartSetCostParameter
		cast(KWDGValueSet*, partCostParameter->GetValueSet())
		    ->SetValueNumber(cast(KWDGPOPartFrequencyVector*, sourcePart1)->GetModalityNumber() +
				     cast(KWDGPOPartFrequencyVector*, sourcePart2)->GetModalityNumber());
	}

	// Cout: cout local de la partie + somme des cout des cellules (maintenu dans la partie)
	dCost = dataGridCosts->ComputePartCost(partCostParameter);
	return dCost;
}

double KWDataGridUnivariateCosts::ComputePartDiffCost(const KWFrequencyVector* sourcePart,
						      const KWFrequencyVector* removedPart) const
{
	double dCost;

	require(IsInitialized());
	require(sourcePart != NULL);
	require(removedPart != NULL);
	require(cast(KWDGPOPartFrequencyVector*, sourcePart)->GetFrequency() >=
		cast(KWDGPOPartFrequencyVector*, removedPart)->GetFrequency());
	require(sourcePart->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());
	require(removedPart->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	// Initialisation du parametrage de la partie
	partCostParameter->nPartFrequency = cast(KWDGPOPartFrequencyVector*, sourcePart)->GetFrequency() -
					    cast(KWDGPOPartFrequencyVector*, removedPart)->GetFrequency();

	// Specification du nombre de valeurs pour une partie symbolique
	// Attention: on utilise ici un acces direct au nombre de valeur du ValueSet pour permettre
	// d'utiliser ce parametre dans les calculs de cout
	// (mais il y a incompletude du ValueSet, qui n'est specifie que pour ce qui est utile pour le calcul des couts)
	if (KWType::IsCoclusteringGroupableType(partCostParameter->GetPartType()))
	{
		// On peut modifier directement le nombre de valeur de facon generique, puisque que l'on est en mode emule
		// via les sous-classes KWDGSymbolValueSetCostParameter et KWDGVarPartSetCostParameter
		cast(KWDGValueSet*, partCostParameter->GetValueSet())
		    ->SetValueNumber(cast(KWDGPOPartFrequencyVector*, sourcePart)->GetModalityNumber() -
				     cast(KWDGPOPartFrequencyVector*, removedPart)->GetModalityNumber());
	}

	// Cout: cout local de la partie + somme des cout des cellules (maintenu dans la partie)
	dCost = dataGridCosts->ComputePartCost(partCostParameter);
	return dCost;
}

double KWDataGridUnivariateCosts::ComputeCellCost(const KWDGPOCellFrequencyVector* cellFrequencyVector) const
{
	double dCost;

	require(IsInitialized());
	require(cellFrequencyVector != NULL);

	// Initialisation du parametrage de la cellule
	cellCostParameter->nCellFrequency = cellFrequencyVector->nCellFrequency;
	cellCostParameter->ivFrequencyVector.CopyFrom(&(cellFrequencyVector->ivFrequencyVector));

	// Cout de la cellule
	dCost = dataGridCosts->ComputeCellCost(cellCostParameter);
	return dCost;
}

double KWDataGridUnivariateCosts::ComputePartitionModelCost(int nPartNumber) const
{
	// Non implemente: a ne pas utiliser
	assert(false);
	return 0;
}

double KWDataGridUnivariateCosts::ComputePartModelCost(const KWFrequencyVector* part) const
{
	// Non implemente: a ne pas utiliser
	assert(false);
	return 0;
}

const ALString KWDataGridUnivariateCosts::GetClassLabel() const
{
	return "MODL grid univariate discretization costs";
}

////////////////////////////////////////////////////////////////////////////////////////////
// Sous-classes de grille servant de parametre de cout a la classe KWDataGridUnivariateCosts

boolean KWDataGridCostParameter::GetEmulated() const
{
	return true;
}

KWDGAttributeCostParameter::KWDGAttributeCostParameter()
{
	nGarbageModalityNumber = 0;
}

KWDGAttributeCostParameter::~KWDGAttributeCostParameter() {}

void KWDGAttributeCostParameter::SetGarbageModalityNumber(int nValue)
{
	nGarbageModalityNumber = nValue;
}
int KWDGAttributeCostParameter::GetGarbageModalityNumber() const
{
	return nGarbageModalityNumber;
}

KWDGPart* KWDGAttributeCostParameter::NewPart() const
{
	return new KWDGPartCostParameter;
}

boolean KWDGAttributeCostParameter::GetEmulated() const
{
	return true;
}

void KWDGPartCostParameter::SetPartType(int nValue)
{
	require(GetPartType() == KWType::Unknown);
	// CH IV Begin
	require(KWType::IsCoclusteringType(nValue));

	// Creation de l'objet interval ou ensemble de valeur selon le type
	if (nValue == KWType::Continuous)
		partValues = new KWDGInterval;
	else if (nValue == KWType::Symbol)
		partValues = new KWDGSymbolValueSetCostParameter;
	else
		partValues = new KWDGVarPartSetCostParameter;
	ensure(GetPartType() != KWType::Unknown);
}

boolean KWDGPartCostParameter::GetEmulated() const
{
	return true;
}

boolean KWDGSymbolValueSetCostParameter::GetEmulated() const
{
	return true;
}

// CH IV Begin
boolean KWDGVarPartSetCostParameter::GetEmulated() const
{
	return true;
}
// CH IV End
boolean KWDGCellCostParameter::GetEmulated() const
{
	return true;
}

////////////////////////////////////////////////////////////////////////////////////
// Classe KWDGPOPartFrequencyVector

KWDGPOPartFrequencyVector::KWDGPOPartFrequencyVector()
{
	dCellCost = 0;
	nFrequency = 0;
}

KWDGPOPartFrequencyVector::~KWDGPOPartFrequencyVector()
{
	nkdCellFrequencyVectors.DeleteAll();
}

int KWDGPOPartFrequencyVector::GetSize() const
{
	return INT_MAX;
}

void KWDGPOPartFrequencyVector::SetCellCost(double dValue)
{
	dCellCost = dValue;
}

double KWDGPOPartFrequencyVector::GetCellCost() const
{
	ensure(fabs(dCellCost - ComputeTotalCellCost()) < 1e-5);
	return dCellCost;
}

void KWDGPOPartFrequencyVector::SetFrequency(int nValue)
{
	require(nValue >= 0);
	nFrequency = nValue;
}

int KWDGPOPartFrequencyVector::GetFrequency() const
{
	ensure(nFrequency == ComputeTotalFrequency());
	return nFrequency;
}

int KWDGPOPartFrequencyVector::GetCellNumber() const
{
	return nkdCellFrequencyVectors.GetCount();
}

KWDGPOCellFrequencyVector* KWDGPOPartFrequencyVector::LookupCell(const Object* cellHashObject)
{
	KWDGPOCellFrequencyVector* cellFrequencyVector;

	cellFrequencyVector = cast(KWDGPOCellFrequencyVector*, nkdCellFrequencyVectors.Lookup(cellHashObject));
	ensure(cellFrequencyVector == NULL or cellFrequencyVector->GetHashObject() == cellHashObject);
	return cellFrequencyVector;
}

void KWDGPOPartFrequencyVector::AddCell(KWDGPOCellFrequencyVector* cellFrequencyVector)
{
	require(cellFrequencyVector != NULL);
	require(cellFrequencyVector->GetHashObject() != NULL);
	require(LookupCell(cellFrequencyVector->GetHashObject()) == NULL);

	nkdCellFrequencyVectors.SetAt(cellFrequencyVector->GetHashObject(), cellFrequencyVector);
}

void KWDGPOPartFrequencyVector::DeleteCell(KWDGPOCellFrequencyVector* cellFrequencyVector)
{
	require(cellFrequencyVector != NULL);
	require(cellFrequencyVector->GetHashObject() != NULL);
	require(LookupCell(cellFrequencyVector->GetHashObject()) == cellFrequencyVector);

	nkdCellFrequencyVectors.RemoveKey(cellFrequencyVector->GetHashObject());
	delete cellFrequencyVector;
}

POSITION KWDGPOPartFrequencyVector::GetHeadPosition() const
{
	return nkdCellFrequencyVectors.GetStartPosition();
}

KWDGPOCellFrequencyVector* KWDGPOPartFrequencyVector::GetNextPosition(POSITION& nextPosition) const
{
	KWDGPOCellFrequencyVector* cellFrequencyVector;
	NUMERIC rKey;
	Object* rValue;

	nkdCellFrequencyVectors.GetNextAssoc(nextPosition, rKey, rValue);
	cellFrequencyVector = cast(KWDGPOCellFrequencyVector*, rValue);
	return cellFrequencyVector;
}

KWFrequencyVector* KWDGPOPartFrequencyVector::Create() const
{
	return new KWDGPOPartFrequencyVector;
}

void KWDGPOPartFrequencyVector::CopyFrom(const KWFrequencyVector* kwfvSource)
{
	const KWDGPOPartFrequencyVector* sourcePartFrequencyVector;
	POSITION positionVector;
	KWDGPOCellFrequencyVector* sourceCellFrequencyVector;
	KWDGPOCellFrequencyVector* cellFrequencyVector;

	require(kwfvSource != NULL);

	// Appel de la methode ancetre
	KWFrequencyVector::CopyFrom(kwfvSource);

	// Cast de la source dans le bon type
	sourcePartFrequencyVector = cast(KWDGPOPartFrequencyVector*, kwfvSource);

	// Recopie des caracteristiques globales
	dCellCost = sourcePartFrequencyVector->GetCellCost();
	nFrequency = sourcePartFrequencyVector->GetFrequency();

	// Recopie des cellules
	nkdCellFrequencyVectors.DeleteAll();
	positionVector = sourcePartFrequencyVector->GetHeadPosition();
	while (positionVector != NULL)
	{
		sourceCellFrequencyVector = sourcePartFrequencyVector->GetNextPosition(positionVector);

		// Creation d'une nouvelle cellule
		cellFrequencyVector = new KWDGPOCellFrequencyVector;
		cellFrequencyVector->CopyFrom(sourceCellFrequencyVector);

		// Ajout dans la partie
		AddCell(cellFrequencyVector);
	}
	ensure(GetCellNumber() == sourcePartFrequencyVector->GetCellNumber());
	ensure(ComputeTotalFrequency() == sourcePartFrequencyVector->ComputeTotalFrequency());
}

KWFrequencyVector* KWDGPOPartFrequencyVector::Clone() const
{
	KWDGPOPartFrequencyVector* kwfvClone;

	kwfvClone = new KWDGPOPartFrequencyVector;
	kwfvClone->CopyFrom(this);
	return kwfvClone;
}

double KWDGPOPartFrequencyVector::ComputeTotalCellCost() const
{
	double dTotalCost;
	POSITION positionVector;
	KWDGPOCellFrequencyVector* cellFrequencyVector;

	// Parcours des cellules pour calculer leur effectif total
	dTotalCost = 0;
	positionVector = GetHeadPosition();
	while (positionVector != NULL)
	{
		cellFrequencyVector = GetNextPosition(positionVector);
		dTotalCost += cellFrequencyVector->GetCost();
	}
	return dTotalCost;
}

int KWDGPOPartFrequencyVector::ComputeTotalFrequency() const
{
	int nTotalFrequency;
	POSITION positionVector;
	KWDGPOCellFrequencyVector* cellFrequencyVector;

	// Parcours des cellules pour calculer leur effectif total
	nTotalFrequency = 0;
	positionVector = GetHeadPosition();
	while (positionVector != NULL)
	{
		cellFrequencyVector = GetNextPosition(positionVector);
		nTotalFrequency += cellFrequencyVector->GetCellFrequency();
	}
	return nTotalFrequency;
}

void KWDGPOPartFrequencyVector::WriteHeaderLineReport(ostream& ost) const
{
	boolean bDisplayCellSummary = true;
	POSITION positionVector;
	KWDGPOCellFrequencyVector* cellFrequencyVector;

	// Entete pour une partie
	KWFrequencyVector::WriteHeaderLineReport(ost);
	ost << "Cells\tFrequency\tCost\t";

	// Entete pour la premiere cellule
	if (bDisplayCellSummary)
	{
		positionVector = GetHeadPosition();
		if (positionVector != NULL)
		{
			cellFrequencyVector = GetNextPosition(positionVector);
			cellFrequencyVector->WriteHeaderLineReport(ost);
		}
	}
}

void KWDGPOPartFrequencyVector::WriteLineReport(ostream& ost) const
{
	boolean bDisplayCellSummary = true;
	POSITION positionVector;
	KWDGPOCellFrequencyVector* cellFrequencyVector;
	KWDGPOCellFrequencyVector cellFrequencyVectorBuffer;
	KWDGPOCellFrequencyVector* cellFrequencyVectorSummary;

	// Resume de l'intervalle
	KWFrequencyVector::WriteLineReport(ost);
	ost << nkdCellFrequencyVectors.GetCount() << "\t";
	ost << ComputeTotalFrequency() << "\t";
	ost << ComputeTotalCellCost() << "\t";

	// Resume des cellules
	if (bDisplayCellSummary)
	{
		// Parcours des cellules pour en constituer le resume
		cellFrequencyVectorSummary = NULL;
		positionVector = GetHeadPosition();
		while (positionVector != NULL)
		{
			cellFrequencyVector = GetNextPosition(positionVector);

			// Creation/initialisation si premiere cellule
			if (cellFrequencyVectorSummary == NULL)
			{
				cellFrequencyVectorSummary = new KWDGPOCellFrequencyVector;

				// On passe par une cellule "buffer" pour supprimer l'identifiant de cellule
				cellFrequencyVectorBuffer.CopyFrom(cellFrequencyVector);
				cellFrequencyVectorBuffer.SetHashObject(NULL);
				cellFrequencyVectorSummary->CopyFrom(&cellFrequencyVectorBuffer);
			}
			// Sinon, ajout des effectifs au resume
			else
			{
				// On passe par une cellule "buffer" pour supprimer l'identifiant de cellule
				// et avoir le meme identifiant NULL dans le resume et le buffer
				cellFrequencyVectorBuffer.CopyFrom(cellFrequencyVector);
				cellFrequencyVectorBuffer.SetHashObject(NULL);
				cellFrequencyVectorSummary->AddFrequenciesFrom(&cellFrequencyVectorBuffer);
				cellFrequencyVectorSummary->SetCost(cellFrequencyVectorSummary->GetCost() +
								    cellFrequencyVectorBuffer.GetCost());
			}
		}

		// Affichage du resume des cellules
		if (cellFrequencyVectorSummary != NULL)
		{
			cellFrequencyVectorSummary->WriteLineReport(ost);
			delete cellFrequencyVectorSummary;
		}
	}
}

void KWDGPOPartFrequencyVector::Write(ostream& ost) const
{
	POSITION positionVector;
	KWDGPOCellFrequencyVector* cellFrequencyVector;
	boolean bFirstCell;

	// Description synthetique
	WriteHeaderLineReport(ost);
	ost << "\n";
	WriteLineReport(ost);
	ost << "\n";

	// Une ligne par cellule
	bFirstCell = true;
	positionVector = GetHeadPosition();
	while (positionVector != NULL)
	{
		cellFrequencyVector = GetNextPosition(positionVector);

		// Entete des cellules
		if (bFirstCell)
		{
			ost << "\t";
			cellFrequencyVector->WriteHeaderLineReport(ost);
			ost << "\n";
			bFirstCell = false;
		}

		// Ligne pour la cellule
		ost << "\t";
		cellFrequencyVector->WriteLineReport(ost);
		ost << "\n";
	}
}

const ALString KWDGPOPartFrequencyVector::GetClassLabel() const
{
	return "Sparse frequency vector";
}

////////////////////////////////////////////////////////////////////////////////////
// Classe KWDGPOCellFrequencyVector

KWDGPOCellFrequencyVector::KWDGPOCellFrequencyVector()
{
	dCost = 0;
	nCellFrequency = 0;
	oHashObject = NULL;
}

KWDGPOCellFrequencyVector::~KWDGPOCellFrequencyVector() {}

void KWDGPOCellFrequencyVector::SetCost(double dValue)
{
	dCost = dValue;
}

double KWDGPOCellFrequencyVector::GetCost() const
{
	return dCost;
}

void KWDGPOCellFrequencyVector::SetHashObject(const Object* object)
{
	oHashObject = object;
}

const Object* KWDGPOCellFrequencyVector::GetHashObject() const
{
	return oHashObject;
}

int KWDGPOCellFrequencyVector::GetCellFrequency() const
{
	ensure(ivFrequencyVector.GetSize() == 0 or ComputeTotalFrequency() == nCellFrequency);
	return nCellFrequency;
}

void KWDGPOCellFrequencyVector::SetCellFrequency(int nFrequency)
{
	require(nFrequency >= 0);
	require(GetTargetValueNumber() == 0);
	nCellFrequency = nFrequency;
}

int KWDGPOCellFrequencyVector::GetTargetValueNumber() const
{
	return ivFrequencyVector.GetSize();
}

int KWDGPOCellFrequencyVector::GetTargetFrequencyAt(int nTarget) const
{
	require(0 <= nTarget and nTarget < GetTargetValueNumber());
	ensure(ivFrequencyVector.GetAt(nTarget) >= 0);
	return ivFrequencyVector.GetAt(nTarget);
}

void KWDGPOCellFrequencyVector::SetTargetFrequencyAt(int nTarget, int nFrequency)
{
	require(0 <= nTarget and nTarget < GetTargetValueNumber());
	require(0 <= nFrequency);
	nCellFrequency += nFrequency - ivFrequencyVector.GetAt(nTarget);
	ivFrequencyVector.SetAt(nTarget, nFrequency);
}

void KWDGPOCellFrequencyVector::UpgradeTargetFrequencyAt(int nTarget, int nDeltaFrequency)
{
	require(0 <= nTarget and nTarget < GetTargetValueNumber());
	require(0 <= nDeltaFrequency);
	nCellFrequency += nDeltaFrequency;
	ivFrequencyVector.UpgradeAt(nTarget, nDeltaFrequency);
	assert(nCellFrequency >= 0);
	assert(ivFrequencyVector.GetAt(nTarget) >= 0);
}

void KWDGPOCellFrequencyVector::CopyFrom(const KWDGPOCellFrequencyVector* cell)
{
	require(cell != NULL);

	// Copie de l'identifiant
	SetHashObject(cell->GetHashObject());

	// Copie du cout
	dCost = cell->dCost;

	// Copie de l'effectif global de la cellule
	nCellFrequency = cell->nCellFrequency;
	assert(nCellFrequency >= 0);

	// Copie de l'effectif par classe cible
	ivFrequencyVector.CopyFrom(&(cell->ivFrequencyVector));
}

void KWDGPOCellFrequencyVector::AddFrequenciesFrom(const KWDGPOCellFrequencyVector* cell)
{
	int nTarget;

	require(cell != NULL);
	require(GetTargetValueNumber() == cell->GetTargetValueNumber());
	require(cell->GetHashObject() == GetHashObject());

	// Mise a jour de l'effectif global de la cellule
	nCellFrequency += cell->nCellFrequency;
	assert(nCellFrequency >= 0);

	// Mise a jour de l'effectif par classe cible
	for (nTarget = 0; nTarget < ivFrequencyVector.GetSize(); nTarget++)
	{
		ivFrequencyVector.UpgradeAt(nTarget, cell->ivFrequencyVector.GetAt(nTarget));
		assert(ivFrequencyVector.GetAt(nTarget) >= 0);
	}
}

void KWDGPOCellFrequencyVector::RemoveFrequenciesFrom(const KWDGPOCellFrequencyVector* cell)
{
	int nTarget;

	require(cell != NULL);
	require(GetTargetValueNumber() == cell->GetTargetValueNumber());
	require(cell->GetHashObject() == GetHashObject());

	// Mise a jour de l'effectif global de la cellule
	nCellFrequency -= cell->nCellFrequency;
	assert(nCellFrequency >= 0);

	// Mise a jour de l'effectif par classe cible
	for (nTarget = 0; nTarget < ivFrequencyVector.GetSize(); nTarget++)
	{
		ivFrequencyVector.UpgradeAt(nTarget, -cell->ivFrequencyVector.GetAt(nTarget));
		assert(ivFrequencyVector.GetAt(nTarget) >= 0);
	}
}

void KWDGPOCellFrequencyVector::MergeFrequenciesFrom(const KWDGPOCellFrequencyVector* cell1,
						     const KWDGPOCellFrequencyVector* cell2)
{
	int nTarget;

	require(cell1 != NULL);
	require(GetTargetValueNumber() == cell1->GetTargetValueNumber());
	require(cell2 != NULL);
	require(GetTargetValueNumber() == cell2->GetTargetValueNumber());
	require(cell1->GetHashObject() == cell2->GetHashObject());

	// Mise a jour de l'effectif global de la cellule
	nCellFrequency = cell1->nCellFrequency + cell2->nCellFrequency;
	assert(nCellFrequency >= 0);

	// Mise a jour de l'effectif par classe cible
	for (nTarget = 0; nTarget < ivFrequencyVector.GetSize(); nTarget++)
	{
		ivFrequencyVector.SetAt(nTarget, cell1->ivFrequencyVector.GetAt(nTarget) +
						     cell2->ivFrequencyVector.GetAt(nTarget));
		assert(ivFrequencyVector.GetAt(nTarget) >= 0);
	}
}

void KWDGPOCellFrequencyVector::WriteHeaderLineReport(ostream& ost) const
{
	int nTargetValue;

	// Identifiant de la cellule
	ost << "ID\t";

	// Affichage des effectifs par classe cible
	for (nTargetValue = 0; nTargetValue < ivFrequencyVector.GetSize(); nTargetValue++)
	{
		ost << "C" << nTargetValue + 1 << "\t";
	}
	ost << "Frequency\t";
	ost << "Cost\t";
}

void KWDGPOCellFrequencyVector::WriteLineReport(ostream& ost) const
{
	int nTargetValue;

	// Affichage de l'identifiant de la cellule
	if (oHashObject != NULL)
		ost << oHashObject->GetObjectLabel();
	ost << "\t";

	// Affichage des effectifs par classe cible
	for (nTargetValue = 0; nTargetValue < ivFrequencyVector.GetSize(); nTargetValue++)
	{
		ost << ivFrequencyVector.GetAt(nTargetValue) << "\t";
	}
	ost << nCellFrequency << "\t";
	ost << dCost << "\t";
}

int KWDGPOCellFrequencyVector::ComputeTotalFrequency() const
{
	int nTotalFrequency;
	int nTarget;

	// Calcul de l'effectif total
	// Calcul de l'effectif total dans le cas supervise
	if (ivFrequencyVector.GetSize() > 0)
	{
		nTotalFrequency = 0;
		for (nTarget = 0; nTarget < ivFrequencyVector.GetSize(); nTarget++)
			nTotalFrequency += ivFrequencyVector.GetAt(nTarget);
	}
	// Sinon, effectif de la cellule
	else
		nTotalFrequency = nCellFrequency;
	return nTotalFrequency;
}

// CH IV Begin
//////////////////////////////////////////////////////////////////////////////////
// Classe CCVarPartDataGridPostOptimizer

CCVarPartDataGridPostOptimizer::CCVarPartDataGridPostOptimizer()
{
	// dataGridCosts = NULL;
	dEpsilon = 1e-10;
	sPostOptimizationAttributeName = "";
}

CCVarPartDataGridPostOptimizer::~CCVarPartDataGridPostOptimizer() {}

void CCVarPartDataGridPostOptimizer::SetPostOptimizationAttributeName(const ALString& sValue)
{
	sPostOptimizationAttributeName = sValue;
}

const ALString& CCVarPartDataGridPostOptimizer::GetPostOptimizationAttributeName() const
{
	return sPostOptimizationAttributeName;
}

boolean CCVarPartDataGridPostOptimizer::PostOptimizeLightVarPartDataGrid(const KWDataGrid* referenceDataGrid,
									 KWDataGrid* optimizedDataGrid,
									 IntVector* ivGroups) const
{
	KWDGAttribute* innerAttribute;
	KWDGAttribute* innerOptimizedAttribute;
	KWDGAttribute* varPartReferenceAttribute;
	KWDGAttribute* varPartOptimizedAttribute;
	KWDataGridManager dataGridManager;
	IntVector ivVarPartIndexes;
	IntVector ivFrozenParts;
	int nDeltaVarPartNumber;
	int nDeltaVarPartNumberClusterOut;
	int nDeltaClusterNumber;
	KWDGPart* innerPart;
	KWDGPart* innerInOptimizedPart;
	KWDGPart* innerOutOptimizedPart;
	KWDGPart* innerPartCluster;
	KWDGPart* clusterPart;
	KWDGPart* clusterPartIn;
	KWDGPart* clusterPartOut;
	KWDGPart* prevPartCluster = NULL;
	KWDGPart* currentPartCluster = NULL;
	KWDGPart* nextPartCluster = NULL;
	KWDGValue* varPartValue;
	int nInnerPart;
	int nPartIndex;
	int nCurrentClusterIndex;
	int nPrevClusterIndex;
	int nNextClusterIndex;
	int nClusterIndex;
	ObjectArray oaClusterParts;
	ObjectArray oaInnerParts;
	NumericKeyDictionary nkdClusterIndexes;
	NumericKeyDictionary nkdReferenceClusterIndexes;
	double dVariationCost = 0;
	IntObject* clusterIndex;
	double dBestVariationCost;
	int nBestVarPartIndex;
	int nBestClusterIndex;
	int nInnerAttribute;
	int nImprovementNumber;
	ALString sInnerAttributeName;
	boolean bDisplayCosts = false;
	boolean bDisplayImprovement = false;
	boolean bAllNegativeVariation = true;
	boolean bBestNegativeVariation = false;
	boolean bBestNegativeVariationForEachAttribute = false;

	// Trois cas de memorisation des deplacements
	// bAllNegativeVariation : tous les deplacements ameliorant le critere sont memorises
	// bBestNegativeVariationForEachAttribute : pour chaque attribut, le meilleur deplacement est memorise
	// bBestNegativeVariation : le meilleur deplacement parmi tous les attributs est memorise

	require(optimizedDataGrid->IsVarPartDataGrid());
	require(referenceDataGrid->IsVarPartDataGrid());
	require(ivGroups != NULL);

	// Initialisation du nombre d'ameliorations
	nImprovementNumber = 0;

	// Initialisation du meilleur cout
	dBestVariationCost = 0;
	nBestClusterIndex = -1;
	nBestVarPartIndex = -1;

	// Re-initialisation du vecteur des clusters
	ivGroups->SetSize(0);

	// Verification de la compatibilite entre grille optimisee et grille initiale
	dataGridManager.SetSourceDataGrid(referenceDataGrid);
	// CH IV: si cette verification est inutile, la supprimer
	// require(dataGridManager.CheckDataGrid(optimizedDataGrid));

	// Extraction de l'attribut VarPart dans la grille pre-partitionnee de reference
	varPartReferenceAttribute = referenceDataGrid->SearchAttribute(sPostOptimizationAttributeName);
	varPartReferenceAttribute->BuildIndexingStructure();

	// Extraction de l'attribut VarPart dans la grille optimisee courante
	varPartOptimizedAttribute = optimizedDataGrid->SearchAttribute(sPostOptimizationAttributeName);
	varPartOptimizedAttribute->BuildIndexingStructure();

	if (bDisplayCosts)
	{
		cout << "CCVarPartDataGridPostOptimizer:Table a optimiser\n";
		optimizedDataGrid->Write(cout);
		cout << "CCVarPartDataGridPostOptimizer: Table de reference avec PV elementaires\n";
		referenceDataGrid->Write(cout);
		cout << "CCVarPartDataGridPostOptimizer:Attribut VarPart\tVarPartNumber\t"
		     << varPartOptimizedAttribute->GetInitialValueNumber() << "\tClusterNumber\t"
		     << varPartOptimizedAttribute->GetPartNumber() << endl;
	}

	// Creation d'un dictionnaire des VarPart dans l'attribut optimise
	nPartIndex = 0;
	clusterPart = varPartOptimizedAttribute->GetHeadPart();
	while (clusterPart != NULL)
	{
		clusterIndex = new IntObject;
		clusterIndex->SetInt(nPartIndex);
		nkdClusterIndexes.SetAt((NUMERIC)clusterPart, clusterIndex);
		nPartIndex++;
		varPartOptimizedAttribute->GetNextPart(clusterPart);
	}

	// Creation d'un dictionnaire donnant pour chaque partie de l'attribut de reference le cluster auquel cette
	// partie appartient dans l'attribut optimise Parcours des parties de l'attribut VarPart (parcours commun a tous
	// les attributs internes)
	nPartIndex = 0;
	innerPartCluster = varPartReferenceAttribute->GetHeadPart();
	while (innerPartCluster != NULL)
	{
		innerPart = innerPartCluster->GetVarPartSet()->GetHeadValue()->GetVarPart();
		clusterPart = varPartOptimizedAttribute->LookupVarPart(innerPart);
		ivGroups->Add(cast(IntObject*, nkdClusterIndexes.Lookup((NUMERIC)clusterPart))->GetInt());
		clusterIndex = new IntObject;
		clusterIndex->SetInt(nPartIndex);
		nkdReferenceClusterIndexes.SetAt((NUMERIC)innerPart, clusterIndex);
		nPartIndex++;
		varPartReferenceAttribute->GetNextPart(innerPartCluster);
	}

	if (bDisplayCosts)
	{
		cout << "Association PV - cluster optimise" << endl;
		// Pour chaque PV on affiche l'index du cluster optimise auquel elle appartient
		innerPartCluster = varPartReferenceAttribute->GetHeadPart();

		for (nPartIndex = 0; nPartIndex < ivGroups->GetSize(); nPartIndex++)
		{
			innerPart = innerPartCluster->GetVarPartSet()->GetHeadValue()->GetVarPart();
			cout << "PV\t" << *innerPart << "\t" << ivGroups->GetAt(nPartIndex) << "\n";
			varPartReferenceAttribute->GetNextPart(innerPartCluster);
		}
	}

	// Boucle sur les attributs internes pour une post-optimisation univariee
	for (nInnerAttribute = 0; nInnerAttribute < referenceDataGrid->GetInnerAttributes()->GetInnerAttributeNumber();
	     nInnerAttribute++)
	{
		// Attribut interne a optimiser
		innerAttribute = referenceDataGrid->GetInnerAttributes()->GetInnerAttributeAt(nInnerAttribute);

		// Cas d'un attribut Continuous
		if (innerAttribute->GetAttributeType() == KWType::Continuous)
		{
			if (bBestNegativeVariationForEachAttribute)
			{
				// Initialisation du meilleur cout pour cet attribut
				dBestVariationCost = 0;
				nBestClusterIndex = -1;
				nBestVarPartIndex = -1;
			}

			// Association partie cluster dans la grille optimisee initiale
			// Parcours des parties de l'attribut interne (parcours specifique a l'attribut interne)
			nInnerPart = 0;
			innerPart = innerAttribute->GetHeadPart();
			oaClusterParts.SetSize(innerAttribute->GetPartNumber());
			while (innerPart != NULL)
			{
				// Recherche de la partie de l'attribut correspondant
				clusterPart = varPartOptimizedAttribute->LookupVarPart(innerPart);
				oaClusterParts.SetAt(nInnerPart, clusterPart);
				innerAttribute->GetNextPart(innerPart);
				nInnerPart++;
			}

			// Affichage des clusters des PV de l'attribut interne
			if (bDisplayCosts)
			{
				cout << "CCVarPartDataGridPostOptimizer:Clusters des PV de l'attribut\t"
				     << innerAttribute->GetAttributeName() << endl;
				oaClusterParts.Write(cout);
				cout << flush;
			}

			// Indexation des parties de l'attribut pour melange aleatoire
			ivVarPartIndexes.SetSize(innerAttribute->GetPartNumber());
			for (nInnerPart = 0; nInnerPart < innerAttribute->GetPartNumber(); nInnerPart++)
				ivVarPartIndexes.SetAt(nInnerPart, nInnerPart);
			// Melange aleatoire
			ivVarPartIndexes.Shuffle();

			// Initialisation du vecteur indiquant les parties "depart" gelees : intervalle a ne pas
			// deplacer car implique dans un deplacement memorise
			ivFrozenParts.SetSize(innerAttribute->GetPartNumber());
			ivFrozenParts.Initialize();

			// Parcours des parties de l'attribut interne selon l'ordre du melange aleatoire
			innerAttribute->ExportParts(&oaInnerParts);
			for (nInnerPart = 0; nInnerPart < oaInnerParts.GetSize(); nInnerPart++)
			{
				// Index de la partie si on ne les prend pas dans l'ordre
				nPartIndex = ivVarPartIndexes.GetAt(nInnerPart);
				innerPart = cast(KWDGPart*, oaInnerParts.GetAt(nPartIndex));

				// Intervalle eligible
				if (ivFrozenParts.GetAt(nPartIndex) < 0)
				{
					if (bDisplayCosts)
						cout << "intervalle non eligible\t" << *innerPart << endl;
				}
				else
				{
					// Extraction du cluster de la grille optimisee dans lequel se trouve la PV a
					// deplacer
					currentPartCluster = cast(KWDGPart*, oaClusterParts.GetAt(nPartIndex));

					// Index de ce cluster
					nCurrentClusterIndex =
					    cast(IntObject*, nkdClusterIndexes.Lookup((NUMERIC)currentPartCluster))
						->GetInt();

					// Extraction du cluster de la PV dans la grille de reference (avec une PV par
					// cluster)
					innerPartCluster = varPartReferenceAttribute->LookupVarPart(innerPart);

					// Variation du nombre de cluster
					nDeltaClusterNumber = 0;
					// Cas ou le cluster est vide apres le depart de l'intervalle
					if (currentPartCluster->GetPartFrequency() == innerPart->GetPartFrequency())
						nDeltaClusterNumber = 1;

					// Cas d'une PV autre que la premiere
					if (nPartIndex > 0)
					{
						// Extraction et index du cluster de la PV precedente
						prevPartCluster = cast(KWDGPart*, oaClusterParts.GetAt(nPartIndex - 1));
						nPrevClusterIndex =
						    cast(IntObject*, nkdClusterIndexes.Lookup((NUMERIC)prevPartCluster))
							->GetInt();
					}
					else
					{
						prevPartCluster = NULL;
						nPrevClusterIndex = -1;
					}
					// Cas d'une PV autre que la derniere
					if (nPartIndex < oaInnerParts.GetSize() - 1)
					{
						// Extraction et index du cluster de la PV suivante
						nextPartCluster = cast(KWDGPart*, oaClusterParts.GetAt(nPartIndex + 1));
						nNextClusterIndex =
						    cast(IntObject*, nkdClusterIndexes.Lookup((NUMERIC)nextPartCluster))
							->GetInt();
					}
					else
					{
						nextPartCluster = NULL;
						nNextClusterIndex = -1;
					}
					// Deplacement de l'intervalle courant vers le cluster de l'intervalle precedent
					if (prevPartCluster != NULL and currentPartCluster != prevPartCluster)
					{
						// Cas d'une double fusion au niveau des PV de l'innerAttribute : si le
						// cluster de l'intervalle precedent est egalement le cluster de
						// l'intervalle suivant Dans ce cas l'innerAttribute perd 2 PV. Le
						// cluster d'arrivee en perd 1
						if (prevPartCluster == nextPartCluster)
						{
							assert(nPrevClusterIndex == nNextClusterIndex);
							nDeltaVarPartNumber = 2;
							nDeltaVarPartNumberClusterOut = 1;
						}
						// Sinon, simple fusion dans le cluster d'arrivee (celui de l'intervalle
						// precedent)
						else
						{
							assert(nPrevClusterIndex != nNextClusterIndex);
							// Deux cas : l'intervalle courant etait ou pas avec
							// l'intervalle suivant Cas ou l'intervalle courant n'etait pas
							// avec l'intervalle suivant
							if (currentPartCluster != nextPartCluster)
							{
								// L'innerAttribute perd 1 PV. Le cluster de depart perd
								// une PV Le cluster d'arrivee 0 (les 2 PV fusionnent
								// dans la meme donc il y a le meme nombre de PV dans le
								// cluster)
								nDeltaVarPartNumber = 1;
								nDeltaVarPartNumberClusterOut = 1;
							}
							// Sinon, aucune perte de PV, juste un transfert
							else
							{
								nDeltaVarPartNumber = 0;
								nDeltaVarPartNumberClusterOut = 0;
							}
						}

						// Calcul du cout de ce deplacement
						// Variation de cout de l'attribut VarPart et de l'attribut interne
						dVariationCost = ComputeVarPartsContinuousAttributeVariationCost(
						    varPartOptimizedAttribute, nDeltaClusterNumber, nDeltaVarPartNumber,
						    innerAttribute->GetAttributeName());

						// Variation de cout du cluster de depart (cluster et cellules associes)
						dVariationCost += ComputeClusterVariationCost(
						    currentPartCluster, innerPart->GetPartFrequency(),
						    nDeltaVarPartNumberClusterOut, true);

						dVariationCost += ComputeClusterCellVariationCost(
						    currentPartCluster, innerPartCluster, true);

						// Variation de cout du cluster d'arrivee
						// Le cluster d'arrivee perd zero ou un intervalle selon le type de
						// fusion simple ou double
						dVariationCost += ComputeClusterVariationCost(
						    prevPartCluster, innerPart->GetPartFrequency(),
						    nDeltaVarPartNumber - nDeltaVarPartNumberClusterOut, false);

						dVariationCost += ComputeClusterCellVariationCost(
						    prevPartCluster, innerPartCluster, false);

						// Affichage de l'intervalle, de la variation du nombre de parties de
						// variable et du cout de son deplacement
						if (bDisplayCosts)
							cout << "CCVarPartDataGridPostOptimizer:Intervalle\t"
							     << *innerPart << "VarPartVariationNumber vers prev\t"
							     << nDeltaVarPartNumber << "\tDeltaCost\t" << dVariationCost
							     << "\n";

						// Cas d'un deplacement ameliorant le critere : construction de la
						// nouvelle grille pour verifier l'egalite des couts
						if (dVariationCost < dBestVariationCost and
						    (bBestNegativeVariation or bBestNegativeVariationForEachAttribute))
						{
							if (bDisplayCosts or bDisplayImprovement)
							{
								cout << "PO numerique" << endl;
								cout << "PV d'index\t" << nInnerPart << "\t"
								     << *innerPart << "\tdVariationCost\t"
								     << dVariationCost << endl;
								cout << "PV qui appartient au cluster reference"
								     << *innerPartCluster;
								cout << "Cluster dans la grille optimisee d'index\t"
								     << nCurrentClusterIndex << "\t"
								     << *currentPartCluster;
								cout << "Index du cluster dans la grille optimisee "
									"d'apres ivGroups\t"
								     << ivGroups->GetAt(nInnerPart) << endl;
								cout << "Nouvel index de PV \t"
								     << cast(IntObject*,
									     nkdReferenceClusterIndexes.Lookup(
										 (NUMERIC)innerPart))
									    ->GetInt()
								     << endl;
								cout << "Nouvel index de cluster \t"
								     << ivGroups->GetAt(
									    cast(IntObject*,
										 nkdReferenceClusterIndexes.Lookup(
										     (NUMERIC)innerPart))
										->GetInt())
								     << endl;
							}

							dBestVariationCost = dVariationCost;
							nBestVarPartIndex =
							    cast(IntObject*,
								 nkdReferenceClusterIndexes.Lookup((NUMERIC)innerPart))
								->GetInt();
							nBestClusterIndex = nPrevClusterIndex;
						}
						// Si on veut memoriser tous les deplacements qui ameliorent le critere
						if (dVariationCost < 0 and bAllNegativeVariation)
						{
							ivGroups->SetAt(
							    cast(IntObject*,
								 nkdReferenceClusterIndexes.Lookup((NUMERIC)innerPart))
								->GetInt(),
							    nPrevClusterIndex);
							nImprovementNumber++;

							// On marque les intervalles precedent et eventuellement suivant
							// comme non eligibles pour un eventuel deplacement car implique
							// dans la fusion de ce deplacement
							ivFrozenParts.SetAt(nPartIndex - 1, -1);
							if (nDeltaVarPartNumber == 2)
								ivFrozenParts.SetAt(nPartIndex + 1, -1);
							if (bDisplayCosts or bDisplayImprovement)
							{
								cout << "PV d'index\t" << nInnerPart << "\t"
								     << *innerPart << "\tdVariationCost\t"
								     << dVariationCost << endl;
								cout << "Nouvel index de cluster \t"
								     << ivGroups->GetAt(
									    cast(IntObject*,
										 nkdReferenceClusterIndexes.Lookup(
										     (NUMERIC)innerPart))
										->GetInt())
								     << endl;
							}
						}
					}

					// Deplacement de l'intervalle courant vers le cluster de l'intervalle suivant
					if (nextPartCluster != NULL and currentPartCluster != nextPartCluster)
					{
						// Cas d'une simple fusion (la double fusion aura ete vu lors du
						// deplacement de l'intervalle courant vers le cluster de l'intervalle
						// precedent
						if (nextPartCluster != prevPartCluster)
						{
							assert(nPrevClusterIndex != nNextClusterIndex);

							// Cas ou l'intervalle courant n'est pas avec le precedent
							if (currentPartCluster != prevPartCluster)
							{
								nDeltaVarPartNumber = 1;
								nDeltaVarPartNumberClusterOut = 1;
							}
							else
							{
								nDeltaVarPartNumber = 0;
								nDeltaVarPartNumberClusterOut = 0;
							}

							// Calcul du cout de ce deplacement
							// Variation de cout de l'attribut VarPart et de l'attribut
							// interne
							dVariationCost =
							    ComputeVarPartsContinuousAttributeVariationCost(
								varPartOptimizedAttribute, nDeltaClusterNumber,
								nDeltaVarPartNumber,
								innerAttribute->GetAttributeName());

							// Variation de cout du cluster de depart (cluster et cellules
							// associes)
							dVariationCost += ComputeClusterVariationCost(
							    currentPartCluster, innerPart->GetPartFrequency(),
							    nDeltaVarPartNumberClusterOut, true);

							dVariationCost += ComputeClusterCellVariationCost(
							    currentPartCluster, innerPartCluster, true);

							// Variation de cout du cluster d'arrivee
							// Le cluster de depart perd un ou deux intervalle selon le type
							// de fusion simple ou double : valeur calculee dans
							// nDeltaVarPartNumber
							dVariationCost += ComputeClusterVariationCost(
							    nextPartCluster, innerPart->GetPartFrequency(),
							    nDeltaVarPartNumber - nDeltaVarPartNumberClusterOut, false);

							dVariationCost += ComputeClusterCellVariationCost(
							    nextPartCluster, innerPartCluster, false);

							// Affichage de l'intervalle et de la variation du nombre de
							// parties de variable et du cout de son deplacement
							if (bDisplayCosts)
								cout << "CCVarPartDataGridPostOptimizer:Intervalle\t"
								     << *innerPart
								     << "VarPartVariationNumber vers next\t"
								     << nDeltaVarPartNumber << "\tDeltaCost\t"
								     << dVariationCost << "\n";

							// Cas d'un deplacement ameliorant le critere : construction de
							// la nouvelle grille pour verifier l'egalite des couts
							if (dVariationCost < dBestVariationCost and
							    (bBestNegativeVariation or
							     bBestNegativeVariationForEachAttribute))
							{
								if (bDisplayCosts or bDisplayImprovement)
								{
									cout << "PO numerique" << endl;
									cout << "PV d'index\t" << nInnerPart << "\t"
									     << *innerPart;
									cout << "PV qui appartient au cluster reference"
									     << *innerPartCluster;
									cout << "Cluster dans la grille optimisee "
										"d'index\t"
									     << nCurrentClusterIndex << "\t"
									     << *currentPartCluster;
									cout << "Index du cluster dans la grille "
										"optimisee d'apres ivGroups\t"
									     << ivGroups->GetAt(nInnerPart) << endl;
									cout << "Nouvel index de PV \t"
									     << cast(IntObject*,
										     nkdReferenceClusterIndexes.Lookup(
											 (NUMERIC)innerPart))
										    ->GetInt()
									     << endl;
									cout
									    << "Nouvel index de cluster \t"
									    << ivGroups->GetAt(
										   cast(IntObject*,
											nkdReferenceClusterIndexes
											    .Lookup((NUMERIC)innerPart))
										       ->GetInt())
									    << endl;
								}

								dBestVariationCost = dVariationCost;
								nBestVarPartIndex =
								    cast(IntObject*, nkdReferenceClusterIndexes.Lookup(
											 (NUMERIC)innerPart))
									->GetInt();
								nBestClusterIndex = nNextClusterIndex;
							}
							// Si on veut tous les deplacements qui ameliorent le critere on
							// peut directement mettre a jour
							if (dVariationCost < 0 and bAllNegativeVariation)
							{
								// pas nInnerPart
								ivGroups->SetAt(
								    cast(IntObject*, nkdReferenceClusterIndexes.Lookup(
											 (NUMERIC)innerPart))
									->GetInt(),
								    nNextClusterIndex);
								nImprovementNumber++;

								// On marque l'intervalle suivant non eligible pour un
								// future deplacement car implique dans cette fusion
								ivFrozenParts.SetAt(nPartIndex + 1, -1);

								if (bDisplayCosts or bDisplayImprovement)
								{
									cout << "PO numerique" << endl;
									cout << "PV d'index\t" << nInnerPart << "\t"
									     << *innerPart << "\tdVariationCost\t"
									     << dVariationCost << endl;
									cout
									    << "Nouvel index de cluster \t"
									    << ivGroups->GetAt(
										   cast(IntObject*,
											nkdReferenceClusterIndexes
											    .Lookup((NUMERIC)innerPart))
										       ->GetInt())
									    << endl;
								}
							}
						}
					}
				}
			}

			// Realisation du meilleur deplacement rencontre
			if (nBestVarPartIndex > 0 and bBestNegativeVariationForEachAttribute)
			{
				ivGroups->SetAt(nBestVarPartIndex, nBestClusterIndex);
				nImprovementNumber++;
			}

			// Nettoyage
			oaInnerParts.SetSize(0);
			oaClusterParts.SetSize(0);
			ivVarPartIndexes.SetSize(0);
		}
		// Cas d'un attribut Categoriel
		else
		{
			// Extraction de l'attribut interne dans la grille optimisee
			// Les PV y sont fusionnees comme dans les clusters
			innerOptimizedAttribute = optimizedDataGrid->GetInnerAttributes()->LookupInnerAttribute(
			    innerAttribute->GetAttributeName());
			innerOptimizedAttribute->BuildIndexingStructure();

			if (bBestNegativeVariationForEachAttribute)
			{
				// Initialisation du meilleur cout pour cet attribut
				dBestVariationCost = 0;
				nBestClusterIndex = -1;
				nBestVarPartIndex = -1;
			}

			// Association partie cluster dans la grille optimisee initiale
			// Parcours des parties de l'attribut interne (parcours specifique a l'attribut interne)
			nInnerPart = 0;
			innerPart = innerAttribute->GetHeadPart();
			oaClusterParts.SetSize(innerAttribute->GetPartNumber());
			while (innerPart != NULL)
			{
				// Recherche de la partie de l'attribut correspondant
				clusterPart = varPartOptimizedAttribute->LookupVarPart(innerPart);
				oaClusterParts.SetAt(nInnerPart, clusterPart);
				innerAttribute->GetNextPart(innerPart);
				nInnerPart++;
			}

			// Affichage des clusters des PV de l'attribut interne
			if (bDisplayCosts)
			{
				// cout << "CCVarPartDataGridPostOptimizer:Clusters des PV de l'attribut\t" <<
				// innerAttribute->GetAttributeName() << endl; oaClusterParts.Write(cout);
				cout << flush;
			}

			// Indexation des parties de l'attribut pour melange aleatoire
			ivVarPartIndexes.SetSize(innerAttribute->GetPartNumber());
			for (nInnerPart = 0; nInnerPart < innerAttribute->GetPartNumber(); nInnerPart++)
				ivVarPartIndexes.SetAt(nInnerPart, nInnerPart);
			// Melange aleatoire
			ivVarPartIndexes.Shuffle();

			// Initialisation du vecteur indiquant les parties destination gelees : parties mono modalite ne
			// pouvant accueillir de nouvelle modalite car la modalite qu'elle contienne a ete retenu dans
			// un deplacement
			ivFrozenParts.SetSize(varPartOptimizedAttribute->GetPartNumber());
			ivFrozenParts.Initialize();

			// Parcours des PV de l'attribut interne optimise pour memoriser les clusters de l'attribut
			// optimise dans lesquels cet attribut est present Seuls ces clusters seront eligibles a
			// recevoir une PV de l'attribut
			innerInOptimizedPart = innerOptimizedAttribute->GetHeadPart();
			while (innerInOptimizedPart != NULL)
			{
				// Extraction du cluster dans l'attribut optimise
				currentPartCluster = varPartOptimizedAttribute->LookupVarPart(innerInOptimizedPart);

				// Index de ce cluster
				nCurrentClusterIndex =
				    cast(IntObject*, nkdClusterIndexes.Lookup((NUMERIC)currentPartCluster))->GetInt();

				// Memorisation du cluster concerne par l'attribut interne
				ivFrozenParts.SetAt(nCurrentClusterIndex, 1);

				// Partie suivante
				innerOptimizedAttribute->GetNextPart(innerInOptimizedPart);
			}

			// Parcours aleatoire des parties de l'attribut interne dans la grille de reference
			// Dans la grille optimisee, ces parties sont eventuellement fusionnees quand elles font partie
			// du meme cluster de l'attribut VarPart
			innerAttribute->ExportParts(&oaInnerParts);
			for (nInnerPart = 0; nInnerPart < oaInnerParts.GetSize(); nInnerPart++)
			{
				// Index de la partie si on ne les prend pas dans l'ordre
				nPartIndex = ivVarPartIndexes.GetAt(nInnerPart);
				innerPart = cast(KWDGPart*, oaInnerParts.GetAt(nPartIndex));

				// Extraction du cluster de la grille optimisee dans lequel se trouve la PV a deplacer
				clusterPartOut = cast(KWDGPart*, oaClusterParts.GetAt(nPartIndex));

				// Index de ce cluster
				nCurrentClusterIndex =
				    cast(IntObject*, nkdClusterIndexes.Lookup((NUMERIC)clusterPartOut))->GetInt();

				// Extraction du cluster de la PV dans la grille de reference (avec une PV par cluster)
				innerPartCluster = varPartReferenceAttribute->LookupVarPart(innerPart);

				// Variation du nombre de cluster
				nDeltaClusterNumber = 0;
				// Cas ou le cluster est vide apres le depart du groupe de modalites
				if (clusterPartOut->GetPartFrequency() == innerPart->GetPartFrequency())
					nDeltaClusterNumber = 1;

				// Variation du nombre de PV
				// Cas general : pas de variation du nombre de PV car la PV groupe de modalites est
				// toujours deplacee dans un cluster ou l'attribut interne est deja present
				nDeltaVarPartNumber = 0;
				//  Cas ou la PV deplacee etait la seule PV de l'attribut interne presente dans son
				//  cluster de depart
				innerOutOptimizedPart = innerOptimizedAttribute->LookupSymbolPart(
				    innerPart->GetValueSet()->GetHeadValue()->GetSymbolValue());
				if (innerOutOptimizedPart->GetValueSet()->GetValueNumber() ==
				    innerPart->GetValueSet()->GetValueNumber())
					nDeltaVarPartNumber = 1;

				// Parcours des clusters d'arrivee eligibles
				nClusterIndex = 0;
				clusterPartIn = varPartOptimizedAttribute->GetHeadPart();
				while (clusterPartIn != NULL)
				{
					// Cluster eligible a recevoir la partie de variable
					if (nClusterIndex != nCurrentClusterIndex and
					    ivFrozenParts.GetAt(nClusterIndex) > 0)
					{
						// Extraction de la partie de variable de l'attribut interne qui
						// accueillerait la partie de variable deplacee
						varPartValue = clusterPartIn->GetVarPartSet()->GetHeadValue();
						while (varPartValue->GetVarPart()->GetAttribute()->GetAttributeName() !=
						       innerPart->GetAttribute()->GetAttributeName())
						{
							clusterPartIn->GetVarPartSet()->GetNextValue(varPartValue);
						}
						innerInOptimizedPart = varPartValue->GetVarPart();

						// Calcul du cout de ce deplacement
						// Variation de cout de l'attribut VarPart et de l'attribut interne
						dVariationCost = ComputeVarPartsSymbolAttributeVariationCost(
						    varPartOptimizedAttribute, nDeltaClusterNumber, nDeltaVarPartNumber,
						    innerAttribute->GetAttributeName(), innerInOptimizedPart,
						    innerOutOptimizedPart, innerPart);

						// Variation de cout du cluster de depart (cluster et cellules associes)
						dVariationCost += ComputeClusterVariationCost(
						    clusterPartOut, innerPart->GetPartFrequency(), nDeltaVarPartNumber,
						    true);
						dVariationCost += ComputeClusterCellVariationCost(
						    clusterPartOut, innerPartCluster, true);

						// Variation de cout du cluster d'arrivee
						// Le cluster d'arrivee garde le meme nombre de PV car le groupe deplace
						// rejoint un groupe deja existant du meme attribut interne
						dVariationCost += ComputeClusterVariationCost(
						    clusterPartIn, innerPart->GetPartFrequency(), 0, false);
						dVariationCost += ComputeClusterCellVariationCost(
						    clusterPartIn, innerPartCluster, false);

						// Affichage de l'intervalle, de la variation du nombre de parties de
						// variable et du cout de son deplacement
						if (bDisplayCosts)
							cout << "CCVarPartDataGridPostOptimizer:Intervalle\t"
							     << *innerPart << "VarPartVariationNumber vers prev\t"
							     << nDeltaVarPartNumber << "\tDeltaCost\t" << dVariationCost
							     << "\n";

						// Cas d'un deplacement ameliorant le critere : construction de la
						// nouvelle grille pour verifier l'egalite des couts
						if (dVariationCost < dBestVariationCost and
						    (bBestNegativeVariation or bBestNegativeVariationForEachAttribute))
						{
							if (bDisplayCosts or bDisplayImprovement)
							{
								cout << "PO categoriel" << endl;
								cout << "PV d'index de depart\t" << nInnerPart << "\t"
								     << *innerPart << "\tdVariationCost\t"
								     << dVariationCost << endl;
								cout << "PV qui appartient au cluster reference"
								     << *innerPartCluster;
								cout << "Cluster de depart dans la grille optimisee "
									"d'index\t"
								     << nCurrentClusterIndex << "\t" << *clusterPartOut;
								cout << "Index du cluster dans la grille optimisee "
									"d'apres ivGroups\t"
								     << ivGroups->GetAt(nInnerPart) << endl;
								cout << "Nouvel index de PV \t"
								     << cast(IntObject*,
									     nkdReferenceClusterIndexes.Lookup(
										 (NUMERIC)innerPart))
									    ->GetInt()
								     << endl;
								cout << "Nouvel index de cluster \t"
								     << ivGroups->GetAt(
									    cast(IntObject*,
										 nkdReferenceClusterIndexes.Lookup(
										     (NUMERIC)innerPart))
										->GetInt())
								     << endl;
							}

							dBestVariationCost = dVariationCost;
							nBestVarPartIndex =
							    cast(IntObject*,
								 nkdReferenceClusterIndexes.Lookup((NUMERIC)innerPart))
								->GetInt();
							nBestClusterIndex = nClusterIndex;
						}
						// Si on veut memoriser tous les deplacements qui ameliorent le critere
						if (dVariationCost < 0 and bAllNegativeVariation)
						{
							ivGroups->SetAt(
							    cast(IntObject*,
								 nkdReferenceClusterIndexes.Lookup((NUMERIC)innerPart))
								->GetInt(),
							    nClusterIndex);
							nImprovementNumber++;

							// Si le cluster de depart contenait une seule PV de l'attribut
							// interne, on marque ce cluster comme non eligible pour la
							// suite des deplacements de cet attribut
							if (nDeltaVarPartNumber == 1)
								ivFrozenParts.SetAt(nCurrentClusterIndex, 0);

							// Affichage
							if (bDisplayCosts or bDisplayImprovement)
							{
								cout << "PO categoriel" << endl;
								cout << "PV d'index\t" << nInnerPart << "\t"
								     << *innerPart << "\tdVariationCost\t"
								     << dVariationCost << endl;
								cout << "Nouvel index de cluster \t"
								     << ivGroups->GetAt(
									    cast(IntObject*,
										 nkdReferenceClusterIndexes.Lookup(
										     (NUMERIC)innerPart))
										->GetInt())
								     << endl;
							}
						}
					}
					nClusterIndex++;
					varPartOptimizedAttribute->GetNextPart(clusterPartIn);
				}
			}

			// Realisation du meilleur deplacement rencontre
			if (nBestVarPartIndex > 0 and bBestNegativeVariationForEachAttribute)
			{
				ivGroups->SetAt(nBestVarPartIndex, nBestClusterIndex);
				nImprovementNumber++;
			}

			// Nettoyage
			oaInnerParts.SetSize(0);
			oaClusterParts.SetSize(0);
			ivVarPartIndexes.SetSize(0);
			innerOptimizedAttribute->DeleteIndexingStructure();
		}
	}

	// Realisation du meilleur deplacement rencontre
	if (nBestVarPartIndex > 0 and bBestNegativeVariation)
	{
		ivGroups->SetAt(nBestVarPartIndex, nBestClusterIndex);
		nImprovementNumber++;
	}
	// Affichage de tous les deplacements memorises
	if (bDisplayCosts)
	{
		cout << "Association PV de l'attribut de reference - cluster optimise, nombre de modifications\t"
		     << nImprovementNumber << endl;
		// Pour chaque PV on affiche l'index du cluster optimise auquel elle appartient
		innerPartCluster = varPartReferenceAttribute->GetHeadPart();

		for (nPartIndex = 0; nPartIndex < ivGroups->GetSize(); nPartIndex++)
		{
			innerPart = innerPartCluster->GetVarPartSet()->GetHeadValue()->GetVarPart();
			cout << "PV\t" << *innerPart << "\t" << ivGroups->GetAt(nPartIndex) << "\n";
			varPartReferenceAttribute->GetNextPart(innerPartCluster);
		}
		cout << flush;
	}

	// Nettoyage
	varPartOptimizedAttribute->DeleteIndexingStructure();
	varPartReferenceAttribute->DeleteIndexingStructure();
	nkdClusterIndexes.DeleteAll();
	nkdReferenceClusterIndexes.DeleteAll();

	return (nImprovementNumber > 0);
}

// CH IV Refactoring: supprimer les lignes commentees de cette methode?
// CH IV Refactoring : a faire en fonction de la conservation ou non du groupe poubelle pour les attributs internes
double CCVarPartDataGridPostOptimizer::ComputeVarPartsSymbolAttributeVariationCost(
    KWDGAttribute* attribute, int nClusterNumberVariation, int nVarPartsNumberVariation, ALString sInnerAttributeName,
    KWDGPart* varPartIn, KWDGPart* varPartOut, KWDGPart* innerPart) const
{
	int nPartileNumber;
	int nClusterNumber;
	// int nGarbageModalityNumber;
	int nPartitionSize;
	int nValueNumber;
	int nPartFrequency;
	double dVariationAttributeCost = 0;
	double dAttributeCost;
	KWDGAttribute* innerAttribute;

	require(attribute != NULL);
	require(attribute->GetInitialValueNumber() > 0);
	require(attribute->GetAttributeType() == KWType::VarPart);
	require(attribute->GetInnerAttributes()->LookupInnerAttribute(sInnerAttributeName)->GetAttributeType() ==
		KWType::Symbol);

	innerAttribute = attribute->GetInnerAttributes()->LookupInnerAttribute(sInnerAttributeName);
	nPartileNumber = attribute->GetInitialValueNumber();
	nClusterNumber = attribute->GetPartNumber();

	// Sans prise en compte granularite : pas de sens en non supervise
	require(nClusterNumber > 0);
	require(nClusterNumber >= 1);
	require(nClusterNumber <= nPartileNumber);
	// Une partition avec poubelle contient au moins 2 parties informatives + 1 groupe poubelle
	require(attribute->GetGarbageModalityNumber() == 0 or nClusterNumber >= 3);
	require(varPartIn->GetPartType() == KWType::Symbol);
	require(varPartOut->GetPartType() == KWType::Symbol);

	// Cout nul si partition nulle
	// cas ou l'on passerait de 2 clusters a 1 seul cluster
	if (nClusterNumber == 1)
		dAttributeCost = 0;
	else
	{
		assert(nPartileNumber > 1);

		assert(not GetVarPartAttributeGarbage());
		// CH IV Refactoring: nettoyer le code ci-dessous?
		// CH IV Refactoring : a faire en fonction de la conservation ou non du groupe poubelle pour les attributs internes
		//// Cout de structure si attribut  parties de variable et poubelle
		// if (attribute->GetAttributeType() == KWType::VarPart and GetVarPartAttributeGarbage())
		//{
		//	// Taille de la poubelle
		//	nGarbageModalityNumber = attribute->GetGarbageModalityNumber();

		//	// Cout de codage du choix des modalites informatives (hors poubelle)
		//	if (nGarbageModalityNumber > 0)
		//	{
		//		// Cout de codage de la taille de la non-poubelle
		//		dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartileNumber -
		// nGarbageModalityNumber - 1, nPartileNumber - 2);

		//		// Choix des modalites hors poubelle selon un tirage multinomial avec un tirage par
		// variable 		dAttributeCost += (nPartileNumber - nGarbageModalityNumber) * log(nPartileNumber
		// * 1.0) - KWStat::LnFactorial(nPartileNumber - nGarbageModalityNumber);

		//		// Cout de codage du nombre de parties informatives (nPartitionSize-1)
		//		dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nClusterNumber - 2,
		// nPartileNumber - nGarbageModalityNumber - 1);

		//		// Cout de codage du choix des parties informatives (nPartitionSize - 1)
		//		dAttributeCost += KWStat::LnBell(nPartileNumber - nGarbageModalityNumber, nClusterNumber
		//- 1);
		//	}
		//	else
		//	{
		//		// Cout de codage du nombre de parties
		//		dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nClusterNumber - 1,
		// nPartileNumber - 1);

		//		// Cout de codage du choix des parties
		//		dAttributeCost += KWStat::LnBell(nPartileNumber, nClusterNumber);
		//	}
		//}
		//// Cout de structure si attribut de type parties de variable sans prise en compte d'un groupe poubelle
		//// CH AB AF temporaire : obsolete a l'integration definitive du groupe poubelle
		// else
		//{
		//  On retire le cout avec l'ancien nombre de clusters et de PV
		//  Cout de codage du nombre de clusters de parties de variable
		dVariationAttributeCost -=
		    KWStat::BoundedNaturalNumbersUniversalCodeLength(nClusterNumber - 1, nPartileNumber - 1);

		// Cout de codage du choix des parties de variable
		dVariationAttributeCost -= KWStat::LnBell(nPartileNumber, nClusterNumber);

		// On ajoute le cout avec le nouveau nombre de clusters et de PV
		// Cout de codage du nombre de clusters de parties de variable
		dVariationAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(
		    nClusterNumber - nClusterNumberVariation - 1, nPartileNumber - nVarPartsNumberVariation - 1);

		// Cout de codage du choix des parties de variable
		dVariationAttributeCost +=
		    KWStat::LnBell(nPartileNumber - nVarPartsNumberVariation, nClusterNumber - nClusterNumberVariation);

		//}
	}

	// Variation de cout de l'attribut interne (obligatoirement categoriel ici)
	nPartitionSize = innerAttribute->GetPartNumber();
	nPartileNumber = innerAttribute->GetInitialValueNumber();

	// Cas d'un attribut interne qui ne fait pas partie du modele nul
	if (innerAttribute->GetPartNumber() > 1 or nPartitionSize > 1)
	{
		// Cas d'un attribut interne categoriel avec groupe poubelle a integrer plus tard
		assert(not GetInnerAttributeGarbage());
		// CH IV Refactoring: nettoyer le code ci-dessous?
		// CH IV Refactoring : a faire en fonction de la conservation ou non du groupe poubelle pour les attributs internes
		// if (GetInnerAttributeGarbage())
		//{
		//	// Taille de la poubelle
		//	nGarbageModalityNumber = attribute->GetGarbageModalityNumber();

		//	// Cout de codage du choix d'une poubelle ou pas
		//	if (nPartileNumber > KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage())
		//		dInnerAttributeCost += log(2.0);

		//	// Cout de codage du choix des modalites informatives (hors poubelle)
		//	if (nGarbageModalityNumber > 0)
		//	{
		//		// Cout de codage de la taille de la non-poubelle : superieur ou egal a 1 (mais pas a 2
		// comme en classification supervisee) 		dInnerAttributeCost +=
		// KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartileNumber - nGarbageModalityNumber,
		// nPartileNumber - 1);

		//		// Choix des modalites hors poubelle selon un tirage multinomial avec un tirage par
		// variable 		dInnerAttributeCost += (nPartileNumber - nGarbageModalityNumber) *
		// log(nPartileNumber * 1.0)
		//- KWStat::LnFactorial(nPartileNumber - nGarbageModalityNumber);

		//		// Cout de codage du nombre de parties informatives (nPartitionSize-1)
		//		dInnerAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize -
		// 1, nPartileNumber - nGarbageModalityNumber);

		//		// Cout de codage du choix des parties informatives (nPartitionSize - 1)
		//		dInnerAttributeCost += KWStat::LnBell(nPartileNumber - nGarbageModalityNumber,
		// nPartitionSize - 1);
		//	}
		//	else
		//	{
		//		// Cout de codage du nombre de parties entre 1 et nPartileNumber (la partition peut etre
		// de taille 1) 		dInnerAttributeCost +=
		// KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize, nPartileNumber);

		//		// Cout de codage du choix des parties
		//		dInnerAttributeCost += KWStat::LnBell(nPartileNumber, nPartitionSize);
		//	}
		//}
		//// CH AB AF temporaire : obsolete apres integration groupe poubelle
		// else
		{
			// Cas d'une variation du nombre de PV pour cet attribut
			if (nVarPartsNumberVariation > 0)
			{
				// On retire le cout avec l'ancienne taille de partition
				dVariationAttributeCost -=
				    KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize, nPartileNumber);
				dVariationAttributeCost -= KWStat::LnBell(nPartileNumber, nPartitionSize);

				// On ajoute le cout avec la nouvelle taille de partitition
				// nPartileNumber ne bouge pas : il s'agit du nombre de modalites initiale
				// (independamment du prepartitionnement) Jk taille de la partition : soit juste apres
				// le prepartitionnement, soit en tenant compte des fusions pour les modalites dans le
				// cluster de PV
				dVariationAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(
				    nPartitionSize - nVarPartsNumberVariation, nPartileNumber);
				dVariationAttributeCost +=
				    KWStat::LnBell(nPartileNumber, nPartitionSize - nVarPartsNumberVariation);
			}

			// Variation de cout de la partie de variable de depart
			// On retire le cout avant deplacement
			nValueNumber = varPartOut->GetValueSet()->GetValueNumber();
			nPartFrequency = varPartOut->GetPartFrequency();
			// Distribution des valeurs dans la partie
			dVariationAttributeCost -= KWStat::LnFactorial(nPartFrequency + nValueNumber - 1);
			dVariationAttributeCost += KWStat::LnFactorial(nValueNumber - 1);
			dVariationAttributeCost += KWStat::LnFactorial(nPartFrequency);

			// On ajoute le cout de la partie de depart apres deplacement
			// Si la partie n'est pas vide apres deplacement
			nValueNumber =
			    varPartOut->GetValueSet()->GetValueNumber() - innerPart->GetValueSet()->GetValueNumber();
			if (nValueNumber > 0)
			{
				nPartFrequency = varPartOut->GetPartFrequency() - innerPart->GetPartFrequency();
				assert(nVarPartsNumberVariation > 0);
				// Distribution des valeurs dans la partie
				dVariationAttributeCost += KWStat::LnFactorial(nPartFrequency + nValueNumber - 1);
				dVariationAttributeCost -= KWStat::LnFactorial(nValueNumber - 1);
				dVariationAttributeCost -= KWStat::LnFactorial(nPartFrequency);
			}

			// Variation de cout de la partie de variable d'arrivee
			// On retire le cout de la partie d'arrivee avant deplacement
			nValueNumber = varPartIn->GetValueSet()->GetValueNumber();
			nPartFrequency = varPartIn->GetPartFrequency();
			// Distribution des valeurs dans la partie
			dVariationAttributeCost -= KWStat::LnFactorial(nPartFrequency + nValueNumber - 1);
			dVariationAttributeCost += KWStat::LnFactorial(nValueNumber - 1);
			dVariationAttributeCost += KWStat::LnFactorial(nPartFrequency);

			// On ajoute le cout de la partie d'arrivee apres deplacement
			nValueNumber =
			    varPartIn->GetValueSet()->GetValueNumber() + innerPart->GetValueSet()->GetValueNumber();
			nPartFrequency = varPartIn->GetPartFrequency() + innerPart->GetPartFrequency();
			// Distribution des valeurs dans la partie
			dVariationAttributeCost += KWStat::LnFactorial(nPartFrequency + nValueNumber - 1);
			dVariationAttributeCost -= KWStat::LnFactorial(nValueNumber - 1);
			dVariationAttributeCost -= KWStat::LnFactorial(nPartFrequency);
		}
	}

	return dVariationAttributeCost;
}

double CCVarPartDataGridPostOptimizer::ComputeVarPartsContinuousAttributeVariationCost(
    KWDGAttribute* attribute, int nClusterNumberVariation, int nVarPartsNumberVariation,
    ALString sInnerAttributeName) const
{
	int nPartileNumber;
	int nClusterNumber;
	// int nGarbageModalityNumber;
	int nPartitionSize;
	double dVariationAttributeCost = 0;
	double dAttributeCost;
	KWDGAttribute* innerAttribute;

	require(attribute != NULL);
	require(attribute->GetInitialValueNumber() > 0);
	require(attribute->GetAttributeType() == KWType::VarPart);
	require(attribute->GetInnerAttributes()->LookupInnerAttribute(sInnerAttributeName)->GetAttributeType() ==
		KWType::Continuous);

	innerAttribute = attribute->GetInnerAttributes()->LookupInnerAttribute(sInnerAttributeName);
	nPartileNumber = attribute->GetInitialValueNumber();
	nClusterNumber = attribute->GetPartNumber();

	// Sans prise en compte granularite : pas de sens en non supervise
	require(nClusterNumber > 0);
	require(nClusterNumber >= 1);
	require(nClusterNumber <= nPartileNumber);
	// Une partition avec poubelle contient au moins 2 parties informatives + 1 groupe poubelle
	require(attribute->GetGarbageModalityNumber() == 0 or nClusterNumber >= 3);

	// Cout nul si partition nulle
	// cas ou l'on passerait de 2 clusters a 1 seul cluster
	if (nClusterNumber == 1)
		dAttributeCost = 0;
	else
	{
		assert(nPartileNumber > 1);

		assert(not GetVarPartAttributeGarbage());
		// CH IV Refactoring: nettoyer ci dessous?
		// CH IV Refactoring : a faire en fonction de la conservation ou non du groupe poubelle pour les attributs internes
		//// Cout de structure si attribut  parties de variable et poubelle
		// if (attribute->GetAttributeType() == KWType::VarPart and GetVarPartAttributeGarbage())
		//{
		//	// Taille de la poubelle
		//	nGarbageModalityNumber = attribute->GetGarbageModalityNumber();

		//	// Cout de codage du choix des modalites informatives (hors poubelle)
		//	if (nGarbageModalityNumber > 0)
		//	{
		//		// Cout de codage de la taille de la non-poubelle
		//		dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartileNumber -
		// nGarbageModalityNumber - 1, nPartileNumber - 2);

		//		// Choix des modalites hors poubelle selon un tirage multinomial avec un tirage par
		// variable 		dAttributeCost += (nPartileNumber - nGarbageModalityNumber) * log(nPartileNumber
		// * 1.0) - KWStat::LnFactorial(nPartileNumber - nGarbageModalityNumber);

		//		// Cout de codage du nombre de parties informatives (nPartitionSize-1)
		//		dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nClusterNumber - 2,
		// nPartileNumber - nGarbageModalityNumber - 1);

		//		// Cout de codage du choix des parties informatives (nPartitionSize - 1)
		//		dAttributeCost += KWStat::LnBell(nPartileNumber - nGarbageModalityNumber, nClusterNumber
		//- 1);
		//	}
		//	else
		//	{
		//		// Cout de codage du nombre de parties
		//		dAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nClusterNumber - 1,
		// nPartileNumber - 1);

		//		// Cout de codage du choix des parties
		//		dAttributeCost += KWStat::LnBell(nPartileNumber, nClusterNumber);
		//	}
		//}
		//// Cout de structure si attribut de type parties de variable sans prise en compte d'un groupe poubelle
		//// CH AB AF temporaire : obsolete a l'integration definitive du groupe poubelle
		// else
		//{
		//  Cas ou il reste au moins deux clusters apres variation
		if (nClusterNumber - nClusterNumberVariation > 1)
		{
			// Cout de codage du nombre de clusters de parties de variable
			dVariationAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(
			    nClusterNumber - nClusterNumberVariation - 1,
			    nPartileNumber - nVarPartsNumberVariation - 1);

			// Cout de codage du choix des parties de variable
			dVariationAttributeCost += KWStat::LnBell(nPartileNumber - nVarPartsNumberVariation,
								  nClusterNumber - nClusterNumberVariation);
		}

		// Cout de codage du nombre de clusters de parties de variable
		dVariationAttributeCost -=
		    KWStat::BoundedNaturalNumbersUniversalCodeLength(nClusterNumber - 1, nPartileNumber - 1);

		// Cout de codage du choix des parties de variable
		dVariationAttributeCost -= KWStat::LnBell(nPartileNumber, nClusterNumber);
		//}
	}

	// Variation de cout de l'attribut interne (obligatoirement numerique ici)
	nPartitionSize = innerAttribute->GetPartNumber();
	nPartileNumber = innerAttribute->GetInitialValueNumber();

	// Cas d'un attribut interne qui ne fait pas partie du modele nul
	if (innerAttribute->GetPartNumber() > 1 or nPartitionSize > 1)
	{
		// Cas d'un attribut numerique
		if (innerAttribute->GetAttributeType() == KWType::Continuous)
		{
			// On retire le cout avec l'ancienne taille de partition
			dVariationAttributeCost -=
			    KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize, nPartileNumber);

			// On ajoute le cout avec la nouvelle taille de partitition
			dVariationAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(
			    nPartitionSize - nVarPartsNumberVariation, nPartileNumber);

			// Pas de variation de cout lie aux parties (intervalles)
		}

		// Cas d'un attribut categoriel
		else
		{
			// A integrer plus tard
			assert(not GetInnerAttributeGarbage());
			// CH IV Refactoring: nettoyer ci dessous?
			// CH IV Refactoring : a faire en fonction de la conservation ou non du groupe poubelle pour les attributs internes
			// if (GetInnerAttributeGarbage())
			//{
			//	// Taille de la poubelle
			//	nGarbageModalityNumber = attribute->GetGarbageModalityNumber();

			//	// Cout de codage du choix d'une poubelle ou pas
			//	if (nPartileNumber > KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage())
			//		dInnerAttributeCost += log(2.0);

			//	// Cout de codage du choix des modalites informatives (hors poubelle)
			//	if (nGarbageModalityNumber > 0)
			//	{
			//		// Cout de codage de la taille de la non-poubelle : superieur ou egal a 1 (mais
			// pas a 2 comme en classification supervisee) 		dInnerAttributeCost +=
			// KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartileNumber - nGarbageModalityNumber,
			// nPartileNumber - 1);

			//		// Choix des modalites hors poubelle selon un tirage multinomial avec un tirage
			// par variable 		dInnerAttributeCost += (nPartileNumber - nGarbageModalityNumber)
			// * log(nPartileNumber * 1.0) - KWStat::LnFactorial(nPartileNumber - nGarbageModalityNumber);

			//		// Cout de codage du nombre de parties informatives (nPartitionSize-1)
			//		dInnerAttributeCost +=
			// KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize - 1, nPartileNumber -
			// nGarbageModalityNumber);

			//		// Cout de codage du choix des parties informatives (nPartitionSize - 1)
			//		dInnerAttributeCost += KWStat::LnBell(nPartileNumber - nGarbageModalityNumber,
			// nPartitionSize - 1);
			//	}
			//	else
			//	{
			//		// Cout de codage du nombre de parties entre 1 et nPartileNumber (la partition
			// peut etre de taille 1) 		dInnerAttributeCost +=
			// KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartitionSize, nPartileNumber);

			//		// Cout de codage du choix des parties
			//		dInnerAttributeCost += KWStat::LnBell(nPartileNumber, nPartitionSize);
			//	}
			//}
			//// CH AB AF temporaire : obsolete apres integration groupe poubelle
			// else
			{
				// Cas d'une variation du nombre de PV pour cet attribut
				if (nVarPartsNumberVariation > 0)
				{
					// On retire le cout avec l'ancienne taille de partition
					dVariationAttributeCost -= KWStat::BoundedNaturalNumbersUniversalCodeLength(
					    nPartitionSize, nPartileNumber);
					dVariationAttributeCost -= KWStat::LnBell(nPartileNumber, nPartitionSize);

					// On ajoute le cout avec la nouvelle taille de partitition
					// nPartileNumber ne bouge pas : il s'agit du nombre de modalites initiale
					// (independamment du prepartitionnement) Jk taille de la partition : soit juste
					// apres le prepartitionnement, soit en tenant compte des fusions pour les
					// modalites dans le cluster de PV
					dVariationAttributeCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(
					    nPartitionSize - nVarPartsNumberVariation, nPartileNumber);
					dVariationAttributeCost +=
					    KWStat::LnBell(nPartileNumber, nPartitionSize - nVarPartsNumberVariation);
				}

				// Variation de cout des parties des attributs internes dans le deplacement (depart et
				// arrivee) Depend du nombre d'observations et du nombre de modalites associe a ce qui
				// est deplace (modalite ou groupe de modalites) On n'a pas ces elements dans la methode
				// Continuous donc il faut une methode pour attribut interne categoriel
				//	innerPart = innerAttribute->GetHeadPart();
				//	// Prise en compte de la variation de cout des parties de depart et d'arrivee du
				// deplacement 	while (innerPart != NULL)
				//	{
				//		dAttributeCost += ComputeInnerAttributePartCost(innerPart);
				//		innerAttribute->GetNextPart(innerPart);
				//	}
			}
		}
	}

	return dVariationAttributeCost;
}

double CCVarPartDataGridPostOptimizer::ComputeClusterVariationCost(KWDGPart* part, int nDeltaObservationNumber,
								   int nDeltaValueNumber, boolean bDeparture) const
{
	int nValueNumber;
	int nClusterFrequency;
	double dVariationCost;
	int nVariationSign;

	dVariationCost = 0;

	// Signe de la variation d'effectif nDeltaObservationNumber : +1 en cas de depart, -1 en cas d'arrivee
	if (bDeparture)
		nVariationSign = 1;
	else
		nVariationSign = -1;

	// Nombre de modalites du cluster de parties de variable
	nValueNumber = part->GetVarPartSet()->GetValueNumber();
	nClusterFrequency = part->GetPartFrequency();

	// On retire le cout du cluster avec son effectif et son nombre de PV
	dVariationCost -= KWStat::LnFactorial(nClusterFrequency + nValueNumber - 1);
	dVariationCost += KWStat::LnFactorial(nValueNumber - 1);

	// On ajoute le cout du nouveau cluster avec nouvel effectif et nouveau nombre de PV
	// Uniquement dans le cas ou le cluster n'est pas vide apres depart de la PV
	if (nClusterFrequency - nVariationSign * nDeltaObservationNumber > 0)
	{
		dVariationCost += KWStat::LnFactorial(nClusterFrequency - nVariationSign * nDeltaObservationNumber +
						      nValueNumber - nDeltaValueNumber - 1);
		dVariationCost -= KWStat::LnFactorial(nValueNumber - nDeltaValueNumber - 1);
	}

	return dVariationCost;
}

double CCVarPartDataGridPostOptimizer::ComputeClusterCellVariationCost(KWDGPart* cluster, KWDGPart* varPartCluster,
								       boolean bDeparture) const
{
	double dVariationCost;
	KWDGCell* varPartCell;
	KWDGCell* clusterCell;
	KWDGPart* identifierPart;
	NumericKeyDictionary nkdCells;
	int nVariationSign;

	require(cluster->GetPartType() == KWType::VarPart);
	require(varPartCluster->GetPartType() == KWType::VarPart);
	require(cluster->GetCellNumber() >= varPartCluster->GetCellNumber() or not bDeparture);

	// Signe de la variation d'effectif : +1 en cas de depart, -1 en cas d'arrivee
	if (bDeparture)
		nVariationSign = 1;
	else
		nVariationSign = -1;

	// Initialisation
	dVariationCost = 0;

	// Indexation des cellules du cluster de la grille optimise par la premiere valeur de leur partie d'Identifier
	clusterCell = cluster->GetHeadCell();
	while (clusterCell != NULL)
	{
		// Extraction du cluster Identifier
		identifierPart = clusterCell->GetPartAt(0);
		assert(identifierPart->GetPartType() == KWType::Symbol);
		nkdCells.SetAt((NUMERIC)identifierPart->GetValueSet()->GetHeadValue()->GetNumericKeyValue(),
			       clusterCell);

		// Partie suivante
		cluster->GetNextCell(clusterCell);
	}

	// Parcours des cellules de la partie de variable
	varPartCell = varPartCluster->GetHeadCell();
	while (varPartCell != NULL)
	{
		identifierPart = varPartCell->GetPartAt(0);
		assert(identifierPart->GetPartType() == KWType::Symbol);
		clusterCell =
		    cast(KWDGCell*,
			 nkdCells.Lookup((NUMERIC)identifierPart->GetValueSet()->GetHeadValue()->GetNumericKeyValue()));

		// Calcul de la variation de cout du a la variation d'effectifs (croissant ou decroissant selon le
		// depart ou l'arrivee de la PV)
		if (clusterCell != NULL)
		{
			// On retire le cout de la cellule avec l'effectif avant variation
			dVariationCost += KWStat::LnFactorial(clusterCell->GetCellFrequency());

			// On ajoute le cout de la cellule d'effectif mis a jour
			dVariationCost -= KWStat::LnFactorial(clusterCell->GetCellFrequency() -
							      nVariationSign * varPartCell->GetCellFrequency());
		}
		// Cas d'un cluster d'arrivee ou la cellule pour ce cluster d'identifiant est vide
		else
		{
			assert(not bDeparture);
			// Le cout de la cellule avant variation est nul (effectif nul)
			// On ajoute le cout de la cellule d'effectif mis a jour
			dVariationCost -= KWStat::LnFactorial(varPartCell->GetCellFrequency());
		}
		varPartCluster->GetNextCell(varPartCell);
	}

	return dVariationCost;
}
// CH IV End
