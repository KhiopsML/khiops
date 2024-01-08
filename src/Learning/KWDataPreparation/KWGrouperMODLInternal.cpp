// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWGrouperMODLInternal.h"

int KWMODLLineCompareModalityNumber(const void* elem1, const void* elem2)
{
	KWMODLLine* line1;
	KWMODLLine* line2;

	line1 = cast(KWMODLLine*, *(Object**)elem1);
	line2 = cast(KWMODLLine*, *(Object**)elem2);

	// Comparaison du nombre de modalites de chaque ligne
	if (line1->GetModalityNumber() < line2->GetModalityNumber())
		return 1;
	else if (line1->GetModalityNumber() > line2->GetModalityNumber())
		return -1;
	else
		return 0;
}

/////////////////////////////////////////////////////////////////////////
// Classe KWGrouperMODLTwoClasses

KWGrouperMODLTwoClasses::KWGrouperMODLTwoClasses()
{
	SetDiscretizationCosts(new KWMODLGroupingCosts);
	bSearchGarbage = false;
	workingFrequencyList = NULL;
}

KWGrouperMODLTwoClasses::~KWGrouperMODLTwoClasses()
{
	// La liste de travail doit avoir ete supprimee au prealable
	assert(workingFrequencyList == NULL);
}

void KWGrouperMODLTwoClasses::Group(KWFrequencyTable* kwftSource, int nInitialSourceValueNumber,
				    KWFrequencyTable*& kwftTarget, IntVector*& ivGroups) const
{
	KWFrequencyTable* kwftSortedSource;
	IntVector ivInitialLineIndexes;
	int i;
	int nSource;
	int nGroupSize;
	int nGroupIndex;

	require(Check());
	require(kwftSource != NULL);
	require(kwftSource->GetFrequencyVectorNumber() >= 1);
	require(kwftSource->GetFrequencyVectorSize() == 2);
	require(1 <= nInitialSourceValueNumber);
	require(kwftSource->GetFrequencyVectorNumber() <= nInitialSourceValueNumber);

	// Initialisation des donnees de travail specifique
	InitializeWorkingData(kwftSource);
	discretizationCosts->SetValueNumber(nInitialSourceValueNumber);

	// Cas particulier ou il n'y a qu'une valeur source
	if (kwftSource->GetFrequencyVectorNumber() <= 1)
	{
		kwftTarget = kwftSource->Clone();
		ivGroups = new IntVector;
		ivGroups->SetSize(kwftSource->GetFrequencyVectorNumber());
	}
	// Sinon, discretization
	else
	{
		// Tri des lignes de contingence par proportion de la premiere classe cible
		kwftSortedSource = kwftSource->Clone();
		SortFrequencyTableByTargetRatio(kwftSortedSource, true, 0, &ivInitialLineIndexes, NULL);

		// Discretisation
		DiscretizeFrequencyTable(kwftSortedSource, kwftTarget);

		// Destruction de la table de contingence intermediaire
		delete kwftSortedSource;

		// Construction du tableau des index des groupes pour les valeurs initiales
		ivGroups = new IntVector;
		ivGroups->SetSize(kwftSource->GetFrequencyVectorNumber());
		nGroupIndex = 0;
		nGroupSize = 0;
		for (i = 0; i < ivInitialLineIndexes.GetSize(); i++)
		{
			nSource = ivInitialLineIndexes.GetAt(i);

			// On affecte le groupe courant a la ligne en cours
			ivGroups->SetAt(nSource, nGroupIndex);

			// Ajout d'un effectif de ligne a l'effectif du groupe courant
			assert(nGroupSize < kwftTarget->GetFrequencyVectorAt(nGroupIndex)->ComputeTotalFrequency());
			nGroupSize += kwftSource->GetFrequencyVectorAt(nSource)->ComputeTotalFrequency();
			assert(nGroupSize <= kwftTarget->GetFrequencyVectorAt(nGroupIndex)->ComputeTotalFrequency());

			// Si l'effectif du groupe est atteint, on initialise le groupe suivant
			if (nGroupSize == kwftTarget->GetFrequencyVectorAt(nGroupIndex)->ComputeTotalFrequency())
			{
				nGroupIndex++;
				assert(nGroupIndex <= kwftTarget->GetFrequencyVectorNumber());
				nGroupSize = 0;
			}
		}
	}
	ensure(kwftSource->GetFrequencyVectorSize() == kwftTarget->GetFrequencyVectorSize());
	ensure(kwftSource->GetFrequencyVectorNumber() >= kwftTarget->GetFrequencyVectorNumber());
	ensure(kwftSource->GetTotalFrequency() == kwftTarget->GetTotalFrequency());
	ensure(ivGroups->GetSize() == kwftSource->GetFrequencyVectorNumber());

	// Nettoyage des donnees de travail
	CleanWorkingData();
}

void KWGrouperMODLTwoClasses::GroupSortedTable(KWFrequencyTable* kwftSortedSource, int nInitialSourceValueNumber,
					       KWFrequencyTable*& kwftTarget, IntVector*& ivGroups) const
{
	int nSource;
	int nGroupSize;
	int nGroupIndex;

	require(Check());
	require(kwftSortedSource != NULL);
	require(kwftSortedSource->GetFrequencyVectorNumber() >= 1);
	require(kwftSortedSource->GetFrequencyVectorSize() == 2);
	require(1 <= nInitialSourceValueNumber);
	require(kwftSortedSource->GetFrequencyVectorNumber() <= nInitialSourceValueNumber);
	require(IsFrequencyTableSortedByTargetRatio(kwftSortedSource, true, 0));

	// Initialisation des donnees de travail specifique
	InitializeWorkingData(kwftSortedSource);
	discretizationCosts->SetValueNumber(nInitialSourceValueNumber);

	// Discretisation
	DiscretizeFrequencyTable(kwftSortedSource, kwftTarget);

	// Construction du tableau des index des groupes pour les valeurs initiales
	ivGroups = new IntVector;
	ivGroups->SetSize(kwftSortedSource->GetFrequencyVectorNumber());
	nGroupIndex = 0;
	nGroupSize = 0;
	for (nSource = 0; nSource < kwftSortedSource->GetFrequencyVectorNumber(); nSource++)
	{
		// On affecte le groupe courant a la ligne en cours
		ivGroups->SetAt(nSource, nGroupIndex);

		// Ajout d'un effectif de ligne a l'effectif du groupe courant
		assert(nGroupSize < kwftTarget->GetFrequencyVectorAt(nGroupIndex)->ComputeTotalFrequency());
		nGroupSize += kwftSortedSource->GetFrequencyVectorAt(nSource)->ComputeTotalFrequency();
		assert(nGroupSize <= kwftTarget->GetFrequencyVectorAt(nGroupIndex)->ComputeTotalFrequency());

		// Si l'effectif du groupe est atteint, on initialise le groupe suivant
		if (nGroupSize == kwftTarget->GetFrequencyVectorAt(nGroupIndex)->ComputeTotalFrequency())
		{
			nGroupIndex++;
			assert(nGroupIndex <= kwftTarget->GetFrequencyVectorNumber());
			nGroupSize = 0;
		}
	}

	// Nettoyage des donnees de travail
	CleanWorkingData();
}

void KWGrouperMODLTwoClasses::PostOptimizeGrouping(KWFrequencyTable* kwftSortedSource, int nInitialSourceValueNumber,
						   KWFrequencyTable* kwftInitialTarget, KWFrequencyTable*& kwftTarget,
						   IntVector*& ivGroups) const
{
	KWMODLLineDeepOptimization lineDeepOptimizationCreator(GetFrequencyVectorCreator());
	KWMODLLineDeepOptimization* headIntervalDeepOptimization;
	KWMODLLine* interval;
	int nSource;
	int nGroupSize;
	int nGroupIndex;

	require(Check());
	require(kwftSortedSource != NULL);
	require(kwftSortedSource->GetFrequencyVectorNumber() >= 1);
	require(kwftSortedSource->GetFrequencyVectorSize() == 2);
	require(1 <= nInitialSourceValueNumber);
	require(kwftSortedSource->GetFrequencyVectorNumber() <= nInitialSourceValueNumber);
	require(IsFrequencyTableSortedByTargetRatio(kwftSortedSource, true, 0));
	require(kwftInitialTarget != NULL);
	require(kwftInitialTarget->GetFrequencyVectorNumber() <= kwftSortedSource->GetFrequencyVectorNumber());
	require(kwftInitialTarget->GetFrequencyVectorSize() == kwftSortedSource->GetFrequencyVectorSize());

	// Initialisation des donnees de travail specifique
	InitializeWorkingData(kwftSortedSource);
	discretizationCosts->SetValueNumber(nInitialSourceValueNumber);

	// Construction de la liste des intervalles a partir de la table cible initiale
	headIntervalDeepOptimization =
	    cast(KWMODLLineDeepOptimization*,
		 BuildIntervalListFromFrequencyTable(&lineDeepOptimizationCreator, kwftInitialTarget));

	// Initialisation des couts des intervalles
	interval = headIntervalDeepOptimization;
	while (interval != NULL)
	{
		// Cout de l'intervalle
		interval->SetCost(ComputeIntervalCost(interval->GetFrequencyVector()));

		// Passage a l'intervalle suivant
		interval = interval->GetNext();
	}

	// Pour chaque intervalle, l'index reference doit etre celui de la derniere ligne de la
	// table initiale correspondant a l'intervalle
	interval = headIntervalDeepOptimization;
	nGroupIndex = 0;
	nGroupSize = 0;
	for (nSource = 0; nSource < kwftSortedSource->GetFrequencyVectorNumber(); nSource++)
	{
		// Ajout d'un effectif de ligne a l'effectif du groupe courant
		assert(nGroupSize < kwftInitialTarget->GetFrequencyVectorAt(nGroupIndex)->ComputeTotalFrequency());
		nGroupSize += kwftSortedSource->GetFrequencyVectorAt(nSource)->ComputeTotalFrequency();
		assert(nGroupSize <= kwftInitialTarget->GetFrequencyVectorAt(nGroupIndex)->ComputeTotalFrequency());

		// Si l'effectif du groupe est atteint, on memorise la ligne du groupe
		// et on initialise le groupe suivant
		if (nGroupSize == kwftInitialTarget->GetFrequencyVectorAt(nGroupIndex)->ComputeTotalFrequency())
		{
			// Memorisation de la derniere ligne du groupe
			interval->SetIndex(nSource);

			// Passage au groupe suivant
			nGroupIndex++;
			assert(nGroupIndex <= kwftInitialTarget->GetFrequencyVectorNumber());
			nGroupSize = 0;
			interval = interval->GetNext();
		}
	}

	// Post-optimisation recherchant des ameliorations locale basees sur des
	// Split, des MergeSplit et des MergeMergeSplit
	IntervalListPostOptimization(kwftSortedSource, headIntervalDeepOptimization);

	// Construction de la table finale a partir de la liste d'intervalles
	kwftTarget = BuildFrequencyTableFromIntervalList(headIntervalDeepOptimization);

	// Parametrisation du nombre de valeurs
	kwftTarget->SetInitialValueNumber(kwftSortedSource->GetInitialValueNumber());
	kwftTarget->SetGranularizedValueNumber(kwftSortedSource->GetGranularizedValueNumber());

	kwftTarget->SetGranularity(kwftSortedSource->GetGranularity());

	// Nettoyage
	DeleteIntervalList(headIntervalDeepOptimization);

	// Construction du tableau des index des groupes pour les valeurs initiales
	ivGroups = new IntVector;
	ivGroups->SetSize(kwftSortedSource->GetFrequencyVectorNumber());
	nGroupIndex = 0;
	nGroupSize = 0;
	for (nSource = 0; nSource < kwftSortedSource->GetFrequencyVectorNumber(); nSource++)
	{
		// On affecte le groupe courant a la ligne en cours
		ivGroups->SetAt(nSource, nGroupIndex);

		// Ajout d'un effectif de ligne a l'effectif du groupe courant
		assert(nGroupSize < kwftTarget->GetFrequencyVectorAt(nGroupIndex)->ComputeTotalFrequency());
		nGroupSize += kwftSortedSource->GetFrequencyVectorAt(nSource)->ComputeTotalFrequency();
		assert(nGroupSize <= kwftTarget->GetFrequencyVectorAt(nGroupIndex)->ComputeTotalFrequency());

		// Si l'effectif du groupe est atteint, on initialise le groupe suivant
		if (nGroupSize == kwftTarget->GetFrequencyVectorAt(nGroupIndex)->ComputeTotalFrequency())
		{
			nGroupIndex++;
			assert(nGroupIndex <= kwftTarget->GetFrequencyVectorNumber());
			nGroupSize = 0;
		}
	}

	// Nettoyage des donnees de travail
	CleanWorkingData();
}

const ALString KWGrouperMODLTwoClasses::GetName() const
{
	return "MODLTwoClasses";
}

void KWGrouperMODLTwoClasses::InitializeWorkingData(const KWFrequencyTable* kwftSource) const {}

void KWGrouperMODLTwoClasses::CleanWorkingData() const {}

void KWGrouperMODLTwoClasses::DiscretizeFrequencyTable(KWFrequencyTable* kwftSource,
						       KWFrequencyTable*& kwftTarget) const
{
	KWMODLLine* headInterval;
	KWFrequencyTable kwftNullFrequencyTable;
	KWMODLLineDeepOptimization* headIntervalWithGarbage;
	boolean bDisplayResults = false;
	KWFrequencyTable* kwftTargetWithoutGarbage;
	KWFrequencyTable* kwftTargetWithGarbage;
	double dCostWithoutGarbage;
	double dCostWithGarbage;
	int nGarbageModalityNumber;
	boolean bGarbageImproveBestMerge;

	require(Check());
	require(kwftSource != NULL);
	require(kwftSource->Check());

	kwftTargetWithoutGarbage = NULL;
	kwftTargetWithGarbage = NULL;

	// Cas particulier ou il n'y a qu'une valeur source
	if (kwftSource->GetFrequencyVectorNumber() <= 1)
	{
		kwftTarget = kwftSource->Clone();
	}
	// Sinon, discretization
	else
	{
		// Cas ou l'on n'est pas en mode poubelle ou que la poubelle n'est pas envisageable (pas assez de
		// modalites) :
		// - nombre initial de modalites inferieur au seuil de rentabilite d'un groupe poubelle
		// - mode sans poubelle
		// - nombre final de groupes demande = 2 ce qui exclut les partitions avec groupe poubelle qui doivent
		// contenir une partition informative (2 groupes) + le groupe poubelle
		if (kwftSource->GetFrequencyVectorNumber() <
			KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage() or
		    GetMaxIntervalNumber() == 2 or GetMaxIntervalNumber() == 1)
		{
			// Optimisation de la liste des intervalles
			headInterval = IntervalListOptimization(kwftSource);
			// Construction de la table finale a partir de la liste d'intervalles
			kwftTarget = BuildFrequencyTableFromIntervalList(headInterval);

			kwftTarget->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
			kwftTarget->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
			// Parametrisation de la granularite et de la taille du groupe poubelle
			kwftTarget->SetGranularity(kwftSource->GetGranularity());
			kwftTarget->SetGarbageModalityNumber(0);

			// Nettoyage
			DeleteIntervalList(headInterval);
		}
		// Cas du mode poubelle
		else
		{
			// Optimisation de la liste des intervalles SANS et AVEC poubelle
			headInterval = IntervalListOptimizationWithGarbage(kwftSource, headIntervalWithGarbage,
									   bGarbageImproveBestMerge);

			if (bDisplayResults)
			{
				cout << " Apres IntervalListBestMergeOptimizationWithGarbage " << endl;
				cout << " Liste SANS poubelle " << endl;
				WriteIntervalListReport(headInterval, cout);
				cout << " Liste AVEC poubelle " << endl;
				WriteIntervalListReport(headIntervalWithGarbage, cout);
			}

			// Construction de la table finale SANS poubelle a partir de la liste d'intervalles
			kwftTargetWithoutGarbage = BuildFrequencyTableFromIntervalList(headInterval);
			DeleteIntervalList(headInterval);

			// Parametrisation du nombre de valeurs
			kwftTargetWithoutGarbage->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
			kwftTargetWithoutGarbage->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());

			// Parametrisation de la granularite et de la taille du groupe poubelle
			kwftTargetWithoutGarbage->SetGranularity(kwftSource->GetGranularity());
			kwftTargetWithoutGarbage->SetGarbageModalityNumber(0);

			// Construction de la table finale AVEC poubelle a partir de la liste d'intervalles
			kwftTargetWithGarbage = BuildFrequencyTableFromIntervalList(headIntervalWithGarbage);
			DeleteIntervalList(headIntervalWithGarbage);

			// Parametrisation du nombre de valeurs
			kwftTargetWithGarbage->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
			kwftTargetWithGarbage->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());

			kwftTargetWithGarbage->SetGranularity(kwftSource->GetGranularity());

			// On doit recalculer la taille de la poubelle qui pourrait avoir change suite a la
			// post-optimisation
			// nGarbageModalityNumber = ComputeGarbageModalityNumberFromTables(kwftSource,
			// kwftTargetWithGarbage);
			nGarbageModalityNumber = ComputeGarbageModalityNumberFromTable(kwftTargetWithGarbage);
			kwftTargetWithGarbage->SetGarbageModalityNumber(nGarbageModalityNumber);

			if (bDisplayResults)
			{
				cout << " Apres construction des tables finales " << endl;
				cout << " Table SANS poubelle " << endl;
				kwftTargetWithoutGarbage->Write(cout);
				cout << " Table AVEC poubelle " << endl;
				kwftTargetWithGarbage->Write(cout);
			}

			// Calcul du cout des deux partitions avec et sans poubelle
			dCostWithoutGarbage = ComputePartitionGlobalCost(kwftTargetWithoutGarbage);
			dCostWithGarbage = ComputePartitionGlobalCost(kwftTargetWithGarbage);

			if (bDisplayResults)
				cout << "Cout sans poubelle\t" << dCostWithoutGarbage << "\tAvec poubelle de taille \t"
				     << kwftTargetWithGarbage->GetGarbageModalityNumber() << " Cout : \t "
				     << dCostWithGarbage << endl;

			// Parametrage de la meilleure partition
			// Cas AVEC groupe poubelle
			if (dCostWithGarbage < dCostWithoutGarbage)
			{
				kwftTarget = kwftTargetWithGarbage->Clone();
				if (bDisplayResults)
					cout << "AVEC";
			}
			// Sinon SANS groupe poubelle
			else
			{
				kwftTarget = kwftTargetWithoutGarbage->Clone();
				if (bDisplayResults)
					cout << "SANS";
			}

			delete kwftTargetWithGarbage;
			delete kwftTargetWithoutGarbage;
		}
	}
	ensure(kwftSource->GetFrequencyVectorNumber() >= kwftTarget->GetFrequencyVectorNumber());
	ensure(kwftSource->GetFrequencyVectorSize() == kwftTarget->GetFrequencyVectorSize());
	ensure(kwftSource->GetTotalFrequency() == kwftTarget->GetTotalFrequency());
	ensure(kwftSource->GetGranularity() == kwftTarget->GetGranularity());
	ensure(kwftSource->GetInitialValueNumber() == kwftTarget->GetInitialValueNumber());
	ensure(kwftSource->GetGranularizedValueNumber() == kwftTarget->GetGranularizedValueNumber());
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Suivi du nombre de modalites par ligne de contingence
boolean KWGrouperMODLTwoClasses::GetModalityNumberMonitoring() const
{
	return bSearchGarbage;
}
void KWGrouperMODLTwoClasses::SetModalityNumberMonitoring(boolean bValue) const
{
	bSearchGarbage = bValue;
}

SortedList* KWGrouperMODLTwoClasses::GetWorkingFrequencyList() const
{
	return workingFrequencyList;
}

void KWGrouperMODLTwoClasses::SetWorkingFrequencyList(SortedList* frequencyList) const
{
	require(frequencyList != NULL);
	require(workingFrequencyList == NULL);

	// Parametrage de la liste de travail par la liste en entree
	workingFrequencyList = frequencyList;

	SetModalityNumberMonitoring(true);
}

void KWGrouperMODLTwoClasses::ResetWorkingFrequencyList() const
{
	require(workingFrequencyList != NULL);
	workingFrequencyList = NULL;
	SetModalityNumberMonitoring(false);
}

void KWGrouperMODLTwoClasses::AddIntervalToWorkingFrequencyList(KWMODLLine* interval) const
{
	if (GetModalityNumberMonitoring())
	{
		require(interval != NULL);
		require(workingFrequencyList != NULL);

		// Ajout de l'intervalle dans la liste triee par nombre de modalites
		interval->SetPosition(workingFrequencyList->Add(interval));
	}
}
void KWGrouperMODLTwoClasses::RemoveIntervalFromWorkingFrequencyList(KWMODLLine* interval) const
{
	if (GetModalityNumberMonitoring())
	{
		require(interval != NULL);
		require(interval->GetPosition() != NULL);
		require(workingFrequencyList != NULL);

		// Retrait de l'intervalle de la liste triee par nombre de modalites
		workingFrequencyList->RemoveAt(interval->GetPosition());
		debug(interval->SetPosition(NULL));
	}
}

void KWGrouperMODLTwoClasses::InitializeFrequencySortedList(SortedList* frequencyList, KWMODLLine* headInterval) const
{
	boolean bDisplayResults = false;
	KWMODLLine* interval;

	require(frequencyList != NULL);
	require(frequencyList->GetCount() == 0);
	require(headInterval != NULL);

	// Insertion des effectifs
	interval = headInterval;
	while (interval != NULL)
	{
		// Insertion dans la liste triee, en memorisant la position d'insertion
		interval->SetPosition(frequencyList->Add(interval));

		// Passage a l'intervalle suivant
		interval = cast(KWMODLLine*, interval->GetNext());
	}
	if (bDisplayResults)
	{
		cout << "InitializeFrequencySortedList::Liste nombre de modalites apres initialisation " << endl;
		frequencyList->Write(cout);
	}
}

int KWGrouperMODLTwoClasses::ComputeModalityNumber(const KWFrequencyTable* kwftSource, int nFirstIndex,
						   int nLastIndex) const
{
	int nIndex;
	int nModalityNumber;

	// Initialisation
	nModalityNumber = 0;

	for (nIndex = nFirstIndex; nIndex <= nLastIndex; nIndex++)
	{
		nModalityNumber += kwftSource->GetFrequencyVectorAt(nIndex)->GetModalityNumber();
	}

	return nModalityNumber;
}

int KWGrouperMODLTwoClasses::ComputeGarbageModalityNumberFromTable(KWFrequencyTable* kwftTargetWithGarbage) const
{
	int nGroupIndex;
	int nModalityNumber;
	int nGarbageModalityNumber;

	require(kwftTargetWithGarbage != NULL);

	// Initialisation
	nGarbageModalityNumber = 0;

	// Parcours de la table avec poubelle pour calculer le nombre de modalites du groupe poubelle (celui qui en
	// contient le plus)
	for (nGroupIndex = 0; nGroupIndex < kwftTargetWithGarbage->GetFrequencyVectorNumber(); nGroupIndex++)
	{
		// Nombre de modalites de cette ligne de contingence
		nModalityNumber = kwftTargetWithGarbage->GetFrequencyVectorAt(nGroupIndex)->GetModalityNumber();

		// MaJ de la taille du groupe poubelle si le groupe courant est plus gros que les precedents
		if (nModalityNumber > nGarbageModalityNumber)
			nGarbageModalityNumber = nModalityNumber;
	}
	return nGarbageModalityNumber;
}

int KWGrouperMODLTwoClasses::GetIntervalListTotalModalityNumber(const KWMODLLine* headLine) const
{
	boolean bDisplayResults = false;
	int nModalityNumber;
	const KWMODLLine* line;

	if (bDisplayResults)
		cout << " nModalityNumber\t";

	// Parcours des elements de la liste pour compter le nombre de modalites par element
	nModalityNumber = 0;
	line = headLine;
	while (line != NULL)
	{
		nModalityNumber += line->GetModalityNumber();
		line = line->GetNext();

		if (bDisplayResults)
			cout << nModalityNumber << "\t";
	}

	if (bDisplayResults)
	{
		cout << "\nGetIntervalListTotalModalityNumber\tnModalityNumber\t" << nModalityNumber
		     << "\tdiscretizationCosts->GetValueNumber()\t" << discretizationCosts->GetValueNumber() << endl;
	}

	return nModalityNumber;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Pilotage de l'optimisation
KWMODLLine* KWGrouperMODLTwoClasses::IntervalListOptimizationWithGarbage(
    const KWFrequencyTable* kwftSource, KWMODLLineDeepOptimization*& headIntervalDeepOptimizationWithGarbage,
    boolean& bGarbageImproveBestMerge) const
{
	boolean bPrintOptimisationStatistics = false;
	boolean bPrintResults = false;
	KWMODLLineOptimization lineOptimizationCreator(GetFrequencyVectorCreator());
	KWMODLLineOptimization* headIntervalOptimization;
	KWMODLLineDeepOptimization lineDeepOptimizationCreator(GetFrequencyVectorCreator());
	KWMODLLineDeepOptimization* headIntervalDeepOptimization;
	KWMODLLine* headInterval;
	KWMODLLine* interval;
	KWMODLLineOptimization* headIntervalOptimizationWithGarbage;
	int nGarbageModalityNumber;

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

	nGarbageModalityNumber = 0;
	headIntervalOptimizationWithGarbage = NULL;
	if (bPrintResults)
	{
		cout << "Debut IntervalListOptimizationWithGarbage " << endl;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Algorithmes gloutons ascendants
	headInterval = NULL;
	if (GetParam() == GreedyMerge or GetParam() == OptimizedGreedyMerge)
	{
		// Construction de la liste des intervalles a partir de la table source
		headIntervalOptimization = cast(
		    KWMODLLineOptimization*, BuildIntervalListFromFrequencyTable(&lineOptimizationCreator, kwftSource));

		if (bPrintResults)
		{
			cout << "IntervalListOptimizationWithGarbage::BuildIntervalListFromFrequencyTable" << endl;
			cout << "Table source" << endl;
			kwftSource->Write(cout);
			cout << "Liste d'intervalles" << endl;
			interval = headIntervalOptimization;
			while (interval != NULL)
			{
				// Affichage de l'intervalle
				interval->Write(cout);

				// Passage a l'intervalle suivant
				interval = interval->GetNext();
			}
		}

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
		// Le cout de la partition avec poubelle est suivi en meme temps que le cout de la partition sans
		// poubelle Arret des qu'il y a degradation de l'un des deux couts
		IntervalListMergeOptimizationWithGarbagePartitionCost(headIntervalOptimization);
		headInterval = headIntervalOptimization;

		// Post-optimisation
		if (GetParam() == OptimizedGreedyMerge)
		{
			IntervalListBestMergeOptimizationWithGarbage(headIntervalOptimization,
								     headIntervalOptimizationWithGarbage,
								     nGarbageModalityNumber, bGarbageImproveBestMerge);

			assert(GetIntervalListTotalModalityNumber(headIntervalOptimizationWithGarbage) ==
			       discretizationCosts->GetValueNumber());

			if (bPrintResults)
			{
				cout << " Apres IntervalListBestMergeOptimizationWithGarbage" << endl;
				cout << " Liste headIntervalOptimization SANS poubelle " << endl;
				WriteIntervalListReport(headIntervalOptimization, cout);
				cout << "Nombre de modalites poubelle APRES "
					"IntervalListBestMergeOptimizationWithGarbage\t"
				     << nGarbageModalityNumber << endl;
				cout << " Liste headIntervalOptimizationWithGarbage AVEC poubelle " << endl;
				WriteIntervalListReport(headIntervalOptimizationWithGarbage, cout);
			}

			// Transfert de la liste optimisee SANS POUBELLE vers une liste dediee a la post-optimisation
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

			// Transfert de la liste optimisee AVEC POUBELLE vers une liste dediee a la post-optimisation
			// La premiere partie de l'optimisation basee uniquement sur des Merge utilise
			// la classe KWMODLLineOptimization pour des raisons de performance en place memoire.
			// La seconde partie utilise la classe KWMODLLineDeepOptimization pour pouvoir
			// gerer les Split, MergeSplit et MergeMergeSplit.
			headIntervalDeepOptimizationWithGarbage =
			    cast(KWMODLLineDeepOptimization*,
				 CloneIntervalList(&lineDeepOptimizationCreator, headIntervalOptimizationWithGarbage));
			DeleteIntervalList(headIntervalOptimizationWithGarbage);
			debug(headIntervalOptimizationWithGarbage = NULL);

			// Post-optimisation recherchant des ameliorations locale basees sur des
			// Split, des MergeSplit et des MergeMergeSplit
			IntervalListPostOptimizationWithGarbage(kwftSource, headIntervalDeepOptimizationWithGarbage,
								nGarbageModalityNumber);

			if (bPrintResults)
			{
				cout << " Apres Post-optimisations " << endl;
				cout << " Liste DeepOptimisation SANS poubelle " << endl;
				WriteIntervalListReport(headIntervalDeepOptimization, cout);
				cout << " Liste DeepOptimisation AVEC poubelle " << endl;
				WriteIntervalListReport(headIntervalDeepOptimizationWithGarbage, cout);
			}
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
		kwftTarget->SetGarbageModalityNumber(0);

		// Evaluation de la discretisation
		cout << "Fin d IntervalListOptimization " << endl;
		cout << "Table post-optimisee SANS poubelle \t" << ComputeDiscretizationCost(kwftTarget);

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

void KWGrouperMODLTwoClasses::IntervalListBestMergeOptimizationWithGarbage(
    KWMODLLineOptimization*& headInterval, KWMODLLineOptimization*& headIntervalWithGarbage,
    int& nGarbageModalityNumber, boolean& bGarbageImproveBestMerge) const
{
	KWMODLLineOptimization* headIntervalCopy;
	SortedList mergeList(KWMODLLineOptimizationCompareMergeDeltaCost);
	KWMODLLineOptimization* interval;
	KWMODLLineOptimization* intervalCopy;
	double dBestTotalCost;
	int nBestIntervalNumberWithGarbage;
	int nBestIntervalNumberWithoutGarbage;
	int nIntervalNumber;
	double dDiscretizationDeltaCost;
	double dTotalCost;
	SortedList frequencyList(KWMODLLineCompareModalityNumber);
	double dDiscretizationWithGarbageDeltaCost;
	double dDiscretizationWithGarbagePartitionDeltaCost;
	double dPartitionWithGarbageCost;
	double dPrevPartitionWithGarbageCost;
	double dBestTotalCostWithGarbage;
	double dTotalCostWithGarbage;
	double dInitialModelDeltaCost;
	int nMinIntervalNumber;
	int nHigherModalitySize;
	int nMergeModalitySize;
	boolean bDisplayGarbage = false;
	boolean bDisplayResults = false;

	require(headInterval != NULL);

	if (bDisplayResults)
		cout << "Debut d'IntervalListBestMergeOptimizationWithGarbage " << endl;

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

	// Initialisation de la liste triee du nombres de modalites par ligne de contingence
	InitializeFrequencySortedList(&frequencyList, headIntervalCopy);
	assert(frequencyList.GetCount() == GetIntervalListSize(headIntervalCopy));
	SetWorkingFrequencyList(&frequencyList);

	// Recherche iterative du meilleur merge; recherche mene a son terme
	// jusqu'a un seul intervalle terminal pour trouver un minimum local
	// Cette recherche est menee sur la copie de la liste originelle
	dTotalCost = 0;
	dBestTotalCost = dTotalCost;
	nIntervalNumber = mergeList.GetCount() + 1;
	nBestIntervalNumberWithGarbage = mergeList.GetCount() + 1;
	nBestIntervalNumberWithoutGarbage = mergeList.GetCount() + 1;

	dTotalCostWithGarbage = 0;
	dBestTotalCostWithGarbage = dTotalCostWithGarbage;
	dInitialModelDeltaCost = 0;
	nGarbageModalityNumber = cast(KWMODLLine*, frequencyList.GetHead())->GetModalityNumber();
	dPrevPartitionWithGarbageCost = 0;
	// Nombre minimal d'intervalle
	// Presence d'une poubelle : au moins deux intervalles informatifs sinon on se ramene au modele nul
	nMinIntervalNumber = 2;

	// Cout de la partition avec poubelle selon le plus gros element de frequencyList
	if (nIntervalNumber > 2)
	{
		// Cout de la partition initiale avec modelisation poubelle
		dPartitionWithGarbageCost = ComputePartitionCost(
		    nIntervalNumber, cast(KWMODLLine*, frequencyList.GetHead())->GetModalityNumber());

		// Variation initiale de cout de modele pour la discretisation initiale : cout partition avec poubelle -
		// cout partition sans poubelle
		dInitialModelDeltaCost = dPartitionWithGarbageCost - ComputePartitionCost(nIntervalNumber, 0);

		dPrevPartitionWithGarbageCost = dPartitionWithGarbageCost;
	}

	while (mergeList.GetCount() > 0)
	{
		// Recherche du meilleur merge (DeltaCost le plus petit)
		interval = cast(KWMODLLineOptimization*, mergeList.GetHead());

		// Variation du cout du au codage d'un intervalle en moins
		dDiscretizationDeltaCost = ComputePartitionDeltaCost(nIntervalNumber, 0);

		// Prise en compte de la variation de cout de codage des exceptions
		dDiscretizationDeltaCost += interval->GetMerge()->GetDeltaCost();

		// Cas d'un nombre suffisant d'intervalles (I>3) pour conserver une partition informative (I=2 +
		// poubelle) apres fusion
		if (nIntervalNumber > 3)
		{
			// Recherche du groupe d'effectif le plus eleve
			// Effectif le plus eleve avant le merge
			nHigherModalitySize = cast(KWMODLLine*, frequencyList.GetHead())->GetModalityNumber();
			// Effectif du merge
			nMergeModalitySize = interval->GetModalityNumber() + interval->GetPrev()->GetModalityNumber();
			if (nHigherModalitySize > nMergeModalitySize)
				nGarbageModalityNumber = nHigherModalitySize;
			else
				nGarbageModalityNumber = nMergeModalitySize;
			// Cout de partition en prenant en compte comme groupe poubelle celui qui contient le plus de
			// modalites Parametrage de la poubelle par l'effectif le plus eleve
			dPartitionWithGarbageCost = ComputePartitionCost(nIntervalNumber - 1, nGarbageModalityNumber);

			// Variation de cout de partition
			dDiscretizationWithGarbagePartitionDeltaCost =
			    dPartitionWithGarbageCost - dPrevPartitionWithGarbageCost;
			// Prise en compte de la variation de cout de codage des parties
			dDiscretizationWithGarbageDeltaCost =
			    dDiscretizationWithGarbagePartitionDeltaCost + interval->GetMerge()->GetDeltaCost();

			dTotalCostWithGarbage += dDiscretizationWithGarbageDeltaCost;

			// Mise a jour de la variation de cout cumulee par rapport au cout initial
			dPrevPartitionWithGarbageCost = dPartitionWithGarbageCost;
		}

		// Mise a jour la liste des intervalles
		UpdateMergeSortedList(&mergeList, headIntervalCopy, interval);
		nIntervalNumber--;

		// Test si diminution du cout total, ou si contrainte de nombre max
		// d'intervalles non respectee (avec tolerance de dEpsilon pour gerer
		// les probleme d'erreur numerique, en favorisant les Merge)
		dTotalCost += dDiscretizationDeltaCost;

		if (bDisplayGarbage)
			cout << nIntervalNumber << "\t" << dTotalCost << "\t" << dTotalCostWithGarbage << "\t"
			     << nGarbageModalityNumber << endl;

		// On ne memorise pas le dernier merge si poubelle
		if ((dTotalCost <= dBestTotalCost + dEpsilon and nIntervalNumber >= nMinIntervalNumber) or
		    (GetMaxIntervalNumber() > 0 and nIntervalNumber > GetMaxIntervalNumber()))
		{
			dBestTotalCost = dTotalCost;
			nBestIntervalNumberWithoutGarbage = nIntervalNumber;
		}
		// On ne regarde que les partitions avec poubelle avec au moins 2 intervalles informatifs
		if ((dTotalCostWithGarbage <= dBestTotalCostWithGarbage + dEpsilon and nIntervalNumber >= 3) or
		    (GetMaxIntervalNumber() > 0 and nIntervalNumber > GetMaxIntervalNumber()))
		{
			dBestTotalCostWithGarbage = dTotalCostWithGarbage;
			nBestIntervalNumberWithGarbage = nIntervalNumber;
		}
	}

	// Affichage des meilleurs modeles avec et sans poubelle
	if (bDisplayGarbage)
	{
		cout << "IntervalListBestMergeOptimizationWithGarbage " << endl;
		cout << " Sans poubelle meilleur nombre d'intervalles \t" << nBestIntervalNumberWithoutGarbage
		     << "\t Meilleur cout \t " << dBestTotalCost << endl;
		cout << " Avec poubelle meilleur nombre d'intervalles \t" << nBestIntervalNumberWithGarbage
		     << "\t Meilleur cout \t " << dInitialModelDeltaCost + dBestTotalCostWithGarbage << endl;
	}

	// Selection du nombre d'intervalles conduisant au meilleur cout avec ou sans poubelle
	if (dBestTotalCost < dBestTotalCostWithGarbage + dInitialModelDeltaCost)
		bGarbageImproveBestMerge = false;
	else
		bGarbageImproveBestMerge = true;

	assert(mergeList.GetCount() == 0);

	// Nettoyage des donnees de travail intermediaires
	DeleteIntervalList(headIntervalCopy);

	// Nettoyage de la liste triee par nombre de modalites et de la liste de travail
	frequencyList.RemoveAll();
	ResetWorkingFrequencyList();

	// Duplication de la structure de liste pour initialiser la partition avec poubelle
	headIntervalWithGarbage = cast(KWMODLLineOptimization*, CloneIntervalList(headInterval, headInterval));

	// Recopie des informations de merge vers la liste avec poubelle avant de construire la liste sans poubelle
	// definitive
	interval = headInterval;
	intervalCopy = headIntervalWithGarbage;
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

	// Construction de la liste associee a la meilleure partition SANS poubelle
	// Initialisation de la liste triee pour repartir du meme point de depart,
	InitializeMergeSortedList(&mergeList, headInterval);
	assert(mergeList.GetCount() == GetIntervalListSize(headInterval) - 1);

	// Initialisation de la liste triee du nombres de modalites par ligne de contingence
	InitializeFrequencySortedList(&frequencyList, headInterval);
	assert(frequencyList.GetCount() == GetIntervalListSize(headInterval));
	// Initialisation de la liste de travail
	SetWorkingFrequencyList(&frequencyList);

	// Recherche iterative du meilleur merge SANS poubelle, jusqu'a reobtenir le nombre
	// d'intervalle optimum pour la partition sans poubelle
	// Cette methode est moins couteuse que de memoriser la liste entiere
	// des intervalle a chaque amelioration des resultats
	nIntervalNumber = mergeList.GetCount() + 1;
	while (nIntervalNumber > nBestIntervalNumberWithoutGarbage)
	{
		// Recherche du meilleur merge (DeltaCost le plus petit)
		interval = cast(KWMODLLineOptimization*, mergeList.GetHead());

		// Mise a jour la liste des intervalles
		UpdateMergeSortedList(&mergeList, headInterval, interval);
		// CH RefontePrior2-P-Inside
		// UpdateFrequencyAndMergeSortedList(&mergeList, &frequencyList, headInterval, interval);
		// Fin CH RefontePrior2-P-Inside
		nIntervalNumber--;
		nExtraMergeNumber++;
	}

	// Construction de la liste associee a la meilleure partition AVEC poubelle
	// Reinitialisations des listes de merge et de frequences pour la partition avec poubelle
	mergeList.RemoveAll();
	InitializeMergeSortedList(&mergeList, headIntervalWithGarbage);
	assert(mergeList.GetCount() == GetIntervalListSize(headIntervalWithGarbage) - 1);

	// Initialisation de la liste triee par effectif de ligne de contingence et de la liste de travail associee
	frequencyList.RemoveAll();
	ResetWorkingFrequencyList();
	InitializeFrequencySortedList(&frequencyList, headIntervalWithGarbage);
	assert(frequencyList.GetCount() == GetIntervalListSize(headIntervalWithGarbage));
	SetWorkingFrequencyList(&frequencyList);

	// Recherche iterative du meilleur merge, jusqu'a reobtenir le nombre
	// d'intervalle optimum pour la partition sans poubelle
	// Cette methode est moins couteuse que de memoriser la liste entiere
	// des intervalle a chaque amelioration des resultats
	nIntervalNumber = mergeList.GetCount() + 1;
	nGarbageModalityNumber = cast(KWMODLLine*, frequencyList.GetHead())->GetModalityNumber();

	// Recherche iterative du meilleur merge AVEC poubelle, jusqu'a reobtenir le nombre
	// d'intervalle optimum pour la partition avec poubelle
	while (nIntervalNumber > nBestIntervalNumberWithGarbage)
	{
		// Recherche du meilleur merge (DeltaCost le plus petit)
		interval = cast(KWMODLLineOptimization*, mergeList.GetHead());

		// Mise a jour la liste des intervalles
		UpdateMergeSortedList(&mergeList, headIntervalWithGarbage, interval);
		// CH RefontePrior2-P-Inside
		// UpdateFrequencyAndMergeSortedList(&mergeList, &frequencyList, headIntervalWithGarbage, interval);
		// Fin CH RefontePrior2-P-Inside
		nIntervalNumber--;
		nExtraMergeNumber++;
		nGarbageModalityNumber = cast(KWMODLLine*, frequencyList.GetHead())->GetModalityNumber();
	}

	// Nettoyage liste de travail
	ResetWorkingFrequencyList();

	// Bilan
	if (bDisplayResults)
	{
		cout << " Fin d'IntervalListBestMergeOptimizationWithGarbage " << endl;
		cout << " Liste SANS poubelle " << endl;
		WriteIntervalListReport(headInterval, cout);
		cout << " Liste AVEC poubelle " << endl;
		WriteIntervalListReport(headIntervalWithGarbage, cout);
		cout << " Taille de la poubelle avant Post-optimisation \t" << nGarbageModalityNumber << endl;
	}
}

void KWGrouperMODLTwoClasses::IntervalListPostOptimizationWithGarbage(const KWFrequencyTable* kwftSource,
								      KWMODLLineDeepOptimization*& headInterval,
								      int nGarbageModalityNumber) const
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
	SortedList frequencyList(KWMODLLineCompareModalityNumber);
	int nNewGarbageModalityNumber;
	int nNewModalityNumber;
	POSITION position;
	KWMODLLine* interval;

	require(kwftSource != NULL);
	require(headInterval != NULL);
	require(nGarbageModalityNumber >= 0);

	InitializeFrequencySortedList(&frequencyList, headInterval);
	assert(frequencyList.GetCount() == GetIntervalListSize(headInterval));
	assert(nGarbageModalityNumber == cast(KWMODLLine*, frequencyList.GetHead())->GetModalityNumber());
	assert(GetIntervalListTotalModalityNumber(headInterval) == discretizationCosts->GetValueNumber());
	SetWorkingFrequencyList(&frequencyList);

	if (bPrintOptimisationDetails)
	{
		// Affichage de la liste initiale apres initialisation des listes d'amelioration
		cout << "Liste initiale au debut de IntervalListPostOptimisationWithGarbage AVANT initialisation des "
			"ameliorations"
		     << endl;
		interval = headInterval;
		while (interval != NULL)
		{
			interval->Write(cout);
			interval = interval->GetNext();
		}
	}

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

	assert(CheckPostOptimizationSortedLists(&splitList, &mergeSplitList, &mergeMergeSplitList, headInterval));

	if (bPrintOptimisationDetails)
	{
		// Affichage de la liste initiale apres initialisation des listes d'amelioration
		cout << "Liste initiale au debut de IntervalListPostOptimisationWithGarbage APRES initialisation des "
			"ameliorations"
		     << endl;
		interval = headInterval;
		while (interval != NULL)
		{
			interval->Write(cout);
			interval = interval->GetNext();
		}
	}

	// Affichage de l'en-tete des messages
	if (bPrintOptimisationDetails)
	{
		cout << "BestDeltaCost"
		     << "\t"
		     << "ModelDeltaCost"
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

		assert(GetIntervalListTotalModalityNumber(headInterval) == discretizationCosts->GetValueNumber());
		nGarbageModalityNumber = cast(KWMODLLine*, frequencyList.GetHead())->GetModalityNumber();

		// Test si arret de tache demandee
		if (nStepNumber % 10 == 0 and TaskProgression::IsInterruptionRequested())
			break;

		// Calcul du nombre d'inetrvalles surnumeraires pour la prise en compte de
		// la contrainte de nombre max d'intervalles
		nSurnumerousIntervalNumber = -1;
		if (GetMaxIntervalNumber() > 0)
			nSurnumerousIntervalNumber = nIntervalNumber - GetMaxIntervalNumber();

		// Recherche du meilleur Split et de son cout
		splitInterval = cast(KWMODLLineDeepOptimization*, splitList.GetHead());
		if (splitInterval->GetSplit()->GetDeltaCost() < dInfiniteCost)
		{
			// Dans le cas d'un Split, on etudie le nombre de modalites des deux intervalles
			// obtenus et on prend en compte la nouvelle taille de la poubelle si besoin
			// Cas ou l'intervalle du Split n'est pas le groupe poubelle
			if (splitInterval->GetPosition() != frequencyList.GetHeadPosition())
				dSplitDeltaCost =
				    -ComputePartitionDeltaCost(nIntervalNumber + 1, nGarbageModalityNumber) +
				    splitInterval->GetSplit()->GetDeltaCost();
			// Cas ou l'intervalle du meilleur Split etait le groupe poubelle
			else
			{
				// Initialisation de la nouvelle taille du groupe poubelle a celle du second element
				// dans la liste triee
				nNewGarbageModalityNumber = 0;
				position = frequencyList.GetHeadPosition();
				while (nNewGarbageModalityNumber == 0)
				{
					interval = cast(KWMODLLine*, frequencyList.GetNext(position));
					if (interval->GetPosition() != splitInterval->GetPosition())
						nNewGarbageModalityNumber = interval->GetModalityNumber();
				}

				// Comparaison avec le nombre de modalites de la premiere partie du Split
				if (splitInterval->GetSplit()->GetFirstSubLineFrequencyVector()->GetModalityNumber() >
				    nNewGarbageModalityNumber)
					nNewGarbageModalityNumber = splitInterval->GetSplit()
									->GetFirstSubLineFrequencyVector()
									->GetModalityNumber();
				// Comparaison avec le nombre de modalites de la seconde partie du Split
				else if (splitInterval->GetModalityNumber() - splitInterval->GetSplit()
										  ->GetFirstSubLineFrequencyVector()
										  ->GetModalityNumber() >
					 nNewGarbageModalityNumber)
					nNewGarbageModalityNumber =
					    splitInterval->GetModalityNumber() - splitInterval->GetSplit()
										     ->GetFirstSubLineFrequencyVector()
										     ->GetModalityNumber();

				// Calcul complet de la variation de cout de partition integrant la variation de la
				// taille du groupe poubelle dans le cout de partition
				dSplitDeltaCost = ComputePartitionCost(nIntervalNumber + 1, nNewGarbageModalityNumber) -
						  ComputePartitionCost(nIntervalNumber, nGarbageModalityNumber) +
						  splitInterval->GetSplit()->GetDeltaCost();
			}
		}
		else
		{
			splitInterval = NULL;
			dSplitDeltaCost = dInfiniteCost;
		}

		// Recherche du meilleur MergeSplit et de son cout
		if (mergeSplitList.GetCount() > 0)
		{
			mergeSplitInterval = cast(KWMODLLineDeepOptimization*, mergeSplitList.GetHead());

			// Cas d'un mergeSplit non viable
			if (mergeSplitInterval->GetMergeSplit()->GetDeltaCost() == dInfiniteCost)
			{
				mergeSplitInterval = NULL;
				dMergeSplitDeltaCost = dInfiniteCost;
			}

			// Sinon
			else
			{
				// Dans le cas d'un MergeSplit, on etudie le nombre de modalites des deux intervalles
				// modifiees et on prend en compte la nouvelle taille de la poubelle si besoin

				// Initialisation de la taille du groupe poubelle a la taille du premier element de la
				// liste triee non implique dans le MergeSplit
				nNewGarbageModalityNumber = 0;
				position = frequencyList.GetHeadPosition();
				while (nNewGarbageModalityNumber == 0)
				{
					interval = cast(KWMODLLine*, frequencyList.GetNext(position));
					if (interval->GetPosition() != mergeSplitInterval->GetPosition() and
					    interval->GetPosition() != mergeSplitInterval->GetPrev()->GetPosition())
						nNewGarbageModalityNumber = interval->GetModalityNumber();
				}

				// Mise a jour de la nouvelle taille du groupe poubelle en fonction du nombre de
				// modalite du premier intervalle obtenu apres le MergeSplit
				nNewModalityNumber = mergeSplitInterval->GetMergeSplit()
							 ->GetFirstSubLineFrequencyVector()
							 ->GetModalityNumber();
				if (nNewModalityNumber > nNewGarbageModalityNumber)
					nNewGarbageModalityNumber = nNewModalityNumber;
				// Mise a jour de la nouvelle taille du groupe poubelle en fonction du nombre de
				// modalite du second intervalle obtenu apres le MergeSplit
				nNewModalityNumber = mergeSplitInterval->GetModalityNumber() +
						     mergeSplitInterval->GetPrev()->GetModalityNumber() -
						     mergeSplitInterval->GetMergeSplit()
							 ->GetFirstSubLineFrequencyVector()
							 ->GetModalityNumber();
				if (nNewModalityNumber > nNewGarbageModalityNumber)
					nNewGarbageModalityNumber = nNewModalityNumber;

				dMergeSplitDeltaCost = mergeSplitInterval->GetMergeSplit()->GetDeltaCost();
				if (nNewGarbageModalityNumber != nGarbageModalityNumber)
					dMergeSplitDeltaCost +=
					    ComputePartitionCost(nIntervalNumber, nNewGarbageModalityNumber) -
					    ComputePartitionCost(nIntervalNumber, nGarbageModalityNumber);
			}
		}
		else
		{
			mergeSplitInterval = NULL;
			dMergeSplitDeltaCost = dInfiniteCost;
		}

		// Recherche du meilleur MergeMergeSplit et de son cout
		if (mergeMergeSplitList.GetCount() > 0 and (nGarbageModalityNumber == 0 or nIntervalNumber > 3))
		{
			mergeMergeSplitInterval = cast(KWMODLLineDeepOptimization*, mergeMergeSplitList.GetHead());

			// Cas d'un mergeMergeSplit non viable
			if (mergeMergeSplitInterval->GetMergeMergeSplit()->GetDeltaCost() == dInfiniteCost)
			{
				mergeMergeSplitInterval = NULL;
				dMergeMergeSplitDeltaCost = dInfiniteCost;
			}
			// Sinon
			else
			{
				// Dans le cas d'un MergeMergeSplit, on calcule le nombre de modalites des deux
				// intervalles obtenus et on prend en compte la nouvelle taille de la poubelle si besoin

				// Initialisation de la taille du groupe poubelle a la taille du premier element de la
				// liste triee non implique dans le MergeMergeSplit
				nNewGarbageModalityNumber = 0;
				position = frequencyList.GetHeadPosition();
				while (nNewGarbageModalityNumber == 0)
				{
					interval = cast(KWMODLLine*, frequencyList.GetNext(position));
					if (interval->GetPosition() != mergeMergeSplitInterval->GetPosition() and
					    interval->GetPosition() !=
						mergeMergeSplitInterval->GetPrev()->GetPosition() and
					    interval->GetPosition() !=
						mergeMergeSplitInterval->GetPrev()->GetPrev()->GetPosition())
						nNewGarbageModalityNumber = interval->GetModalityNumber();
				}

				// Mise a jour de la nouvelle taille du groupe poubelle en fonction du nombre de
				// modalite du premier intervalle obtenu apres le MergeMergeSplit
				nNewModalityNumber = mergeMergeSplitInterval->GetMergeMergeSplit()
							 ->GetFirstSubLineFrequencyVector()
							 ->GetModalityNumber();
				if (nNewModalityNumber > nNewGarbageModalityNumber)
					nNewGarbageModalityNumber = nNewModalityNumber;

				// Mise a jour de la nouvelle taille du groupe poubelle en fonction du nombre de
				// modalite du second intervalle obtenu apres le MergeMergeSplit
				nNewModalityNumber =
				    mergeMergeSplitInterval->GetModalityNumber() +
				    mergeMergeSplitInterval->GetPrev()->GetModalityNumber() +
				    mergeMergeSplitInterval->GetPrev()->GetPrev()->GetModalityNumber() -
				    mergeMergeSplitInterval->GetMergeMergeSplit()
					->GetFirstSubLineFrequencyVector()
					->GetModalityNumber();
				if (nNewModalityNumber > nNewGarbageModalityNumber)
					nNewGarbageModalityNumber = nNewModalityNumber;

				dMergeMergeSplitDeltaCost =
				    mergeMergeSplitInterval->GetMergeMergeSplit()->GetDeltaCost();

				if (nNewGarbageModalityNumber == nGarbageModalityNumber)
					dMergeMergeSplitDeltaCost +=
					    ComputePartitionDeltaCost(nIntervalNumber, nGarbageModalityNumber);
				else
					dMergeMergeSplitDeltaCost +=
					    ComputePartitionCost(nIntervalNumber - 1, nNewGarbageModalityNumber) -
					    ComputePartitionCost(nIntervalNumber, nGarbageModalityNumber);

				// Si la contrainte de nombre max d'intervalle n'est pas respectee,
				// le MergeMergeSplit devient prioritaire
				if (nSurnumerousIntervalNumber > 0)
					dMergeMergeSplitDeltaCost += dPriorityDeltaCost;
			}
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
				if (nIntervalNumber > 3)
					// Transmission parametre taille poubelle
					cout << ComputePartitionDeltaCost(nIntervalNumber, nGarbageModalityNumber);
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
				cout << dBestDeltaCost << "\t" <<
				    // Transmission parametre taille poubelle
				    ComputePartitionDeltaCost(nIntervalNumber, nGarbageModalityNumber) << "\t"
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
				cout << dBestDeltaCost << "\t" <<
				    // Transmission parametre taille poubelle
				    ComputePartitionDeltaCost(nIntervalNumber, nGarbageModalityNumber) << "\t"
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
				cout << dBestDeltaCost << "\t" <<
				    // Transmission parametre taille poubelle
				    -ComputePartitionDeltaCost(nIntervalNumber + 1, nGarbageModalityNumber) << "\t"
				     << nIntervalNumber << "\tSplit\t";
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
		assert(GetIntervalListTotalModalityNumber(headInterval) == discretizationCosts->GetValueNumber());
	}
	// Fin de l'utilisation de la liste de travail
	ResetWorkingFrequencyList();
}

void KWGrouperMODLTwoClasses::SortFrequencyTableByTargetRatio(KWFrequencyTable* kwftFrequencyTable, boolean bAscending,
							      int nTargetIndex, IntVector* ivInitialLineIndexes,
							      IntVector* ivSortedLineIndexes)
{
	ObjectArray oaLines;
	ObjectArray oaSortedLines;
	KWSortableValue* line;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	int nFrequencyVectorFrequency;
	int nSource;
	int nAscending;

	require(kwftFrequencyTable != NULL);
	require(kwftFrequencyTable->GetFrequencyVectorNumber() > 0);
	require(0 <= nTargetIndex and nTargetIndex < kwftFrequencyTable->GetFrequencyVectorSize());
	require(cast(KWDenseFrequencyVector*, kwftFrequencyTable->GetFrequencyVectorAt(0))
		    ->GetFrequencyVector()
		    ->GetSize() > 0);

	// Sens du tri
	if (bAscending)
		nAscending = 1;
	else
		nAscending = -1;

	// Initialisation d'un tableau memorisant la proportion de la classe
	// cible specifiee pour chaque ligne de la table de contingence
	oaLines.SetSize(kwftFrequencyTable->GetFrequencyVectorNumber());
	for (nSource = 0; nSource < kwftFrequencyTable->GetFrequencyVectorNumber(); nSource++)
	{
		line = new KWSortableValue;
		kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, kwftFrequencyTable->GetFrequencyVectorAt(nSource));
		nFrequencyVectorFrequency = kwdfvFrequencyVector->ComputeTotalFrequency();
		if (nFrequencyVectorFrequency > 0)
			line->SetSortValue(nAscending *
					   kwdfvFrequencyVector->GetFrequencyVector()->GetAt(nTargetIndex) * 1.0 /
					   nFrequencyVectorFrequency);
		line->SetIndex(nSource);
		oaLines.SetAt(nSource, line);
	}

	// Tri d'une copie de ce tableau
	oaSortedLines.CopyFrom(&oaLines);
	oaSortedLines.SetCompareFunction(KWSortableValueCompare);
	oaSortedLines.Sort();

	// Permutation des lignes pour trier la table
	// PermutateFrequencyTableLines(kwftFrequencyTable, &oaLines, &oaSortedLines, ivInitialLineIndexes,
	// ivSortedLineIndexes);
	kwftFrequencyTable->PermutateTableLines(&oaLines, &oaSortedLines, ivInitialLineIndexes, ivSortedLineIndexes);

	// Nettoyage des donnees de travail
	oaLines.DeleteAll();
}

void KWGrouperMODLTwoClasses::IntervalListMergeOptimizationWithGarbagePartitionCost(
    KWMODLLineOptimization*& headInterval) const
{
	SortedList mergeList(KWMODLLineOptimizationCompareMergeDeltaCost);
	SortedList frequencyList(KWMODLLineCompareModalityNumber);
	KWMODLLineOptimization* interval;
	double dDiscretizationDeltaCost;
	double dDiscretizationWithGarbageDeltaCost;
	double dDiscretizationWithGarbagePartitionDeltaCost;
	double dCumulatedDeltaCostWithGarbage;
	double dCumulatedDeltaCostWithoutGarbage;
	double dPartitionWithGarbageCost;
	double dPrevPartitionWithGarbageCost;
	int nHigherSize;
	int nMergeSize;
	int nGarbageModalityNumber;
	int nIntervalNumber;
	int nMinMergeNumber;
	boolean bDisplayResults = false;
	boolean bDisplayFinalResults = false;
	boolean bDisplayMerges = false;
	boolean bGarbageProfitable;

	require(headInterval != NULL);

	// Initialisation des Merges
	InitializeMergeList(headInterval);

	// Initialisation de la liste triee
	InitializeMergeSortedList(&mergeList, headInterval);
	assert(mergeList.GetCount() == GetIntervalListSize(headInterval) - 1);

	// Initialisation de la liste triee par effectif de ligne de contingence
	InitializeFrequencySortedList(&frequencyList, headInterval);
	SetWorkingFrequencyList(&frequencyList);
	assert(frequencyList.GetCount() == GetIntervalListSize(headInterval));
	assert(GetIntervalListTotalModalityNumber(headInterval) == discretizationCosts->GetValueNumber());

	// Recherche iterative du meilleur merge
	nIntervalNumber = mergeList.GetCount() + 1;

	// Nombre minimal de merges restants
	nMinMergeNumber = 0;

	// Taille initiale du groupe poubelle potentiel
	nGarbageModalityNumber = cast(KWMODLLine*, frequencyList.GetHead())->GetModalityNumber();

	// Cout de la partition initiale avec poubelle = groupe qui contient le plus de modalites
	dPartitionWithGarbageCost = ComputePartitionCost(nIntervalNumber, nGarbageModalityNumber);
	// - cout de la partition sans poubelle
	dDiscretizationWithGarbagePartitionDeltaCost =
	    dPartitionWithGarbageCost - ComputePartitionCost(nIntervalNumber, 0);
	// Variation de cout entre le modele initial considere avec poubelle et le modele initial sans poubelle
	// Considere comme la variation de cout avec poubelle avant les premiers merge (pour inclusion dans le cumul des
	// variations)
	dDiscretizationWithGarbageDeltaCost = dDiscretizationWithGarbagePartitionDeltaCost;

	if (bDisplayResults)
		cout << "nIntervalNumber apres fusion \tDeltaCout sans poubelle \t DeltaCout avec poubelle \t Taille "
			"poubelle"
		     << endl;

	// Initialisation des variations de cout cumulees depuis la partition initiale sans poubelle
	dCumulatedDeltaCostWithGarbage = dDiscretizationWithGarbageDeltaCost;
	dCumulatedDeltaCostWithoutGarbage = 0;
	dCumulatedDeltaCostWithGarbage < dCumulatedDeltaCostWithoutGarbage ? bGarbageProfitable = 1
									   : bGarbageProfitable = 0;

	// Cout de partition a l'etape precedente : initialisation
	dPrevPartitionWithGarbageCost = dPartitionWithGarbageCost;

	// Parcours des merges
	while (mergeList.GetCount() > nMinMergeNumber)
	{
		// Recherche du meilleur merge (DeltaCost le plus petit)
		interval = cast(KWMODLLineOptimization*, mergeList.GetHead());

		if (bDisplayMerges)
		{
			cout << "Meilleur merge" << endl;
			interval->Write(cout);
			cout << "Moins bon merge" << endl;
			KWMODLLineOptimization* worstInterval = cast(KWMODLLineOptimization*, mergeList.GetTail());
			if (worstInterval != NULL)
				worstInterval->Write(cout);
			cout << "Liste des merge" << endl;
			mergeList.Write(cout);
		}

		// Variation du cout du au codage d'un intervalle en moins
		dDiscretizationDeltaCost = ComputePartitionDeltaCost(nIntervalNumber, 0);

		// Prise en compte de la variation de cout de codage des exceptions
		dDiscretizationDeltaCost += interval->GetMerge()->GetDeltaCost();

		dCumulatedDeltaCostWithoutGarbage += dDiscretizationDeltaCost;
		// On evalue la variation de cout vers une partition avec groupe poubelle affecte au groupe contenant le
		// plus de modalites Restriction aux partitions informatives (I > 3) : il faut qu'apres le merge il
		// reste 2 intervalles informatifs et un groupe poubelle
		if (nIntervalNumber > 3)
		{
			// Recherche du groupe d'effectif le plus eleve
			// Effectif le plus eleve avant le merge
			nHigherSize = cast(KWMODLLine*, frequencyList.GetHead())->GetModalityNumber();
			// Effectif du merge
			nMergeSize = interval->GetModalityNumber() + interval->GetPrev()->GetModalityNumber();
			if (nHigherSize > nMergeSize)
				nGarbageModalityNumber = nHigherSize;
			else
				nGarbageModalityNumber = nMergeSize;
			// Cout de partition en prenant en compte comme groupe poubelle celui qui contient le plus de
			// modalites Parametrage de la poubelle par l'effectif le plus eleve
			dPartitionWithGarbageCost = ComputePartitionCost(nIntervalNumber - 1, nGarbageModalityNumber);

			// Variation de cout de partition courante avec poubelle et precedente avec poubelle
			dDiscretizationWithGarbagePartitionDeltaCost =
			    dPartitionWithGarbageCost - dPrevPartitionWithGarbageCost;
			// Prise en compte de la variation de cout de codage des parties
			dDiscretizationWithGarbageDeltaCost =
			    dDiscretizationWithGarbagePartitionDeltaCost + interval->GetMerge()->GetDeltaCost();

			// Mise a jour de la variation de cout cumulee par rapport au cout initial
			dPrevPartitionWithGarbageCost = dPartitionWithGarbageCost;
			dCumulatedDeltaCostWithGarbage += dDiscretizationWithGarbageDeltaCost;
			if (bDisplayResults)
			{
				cout << "DeltaCost Sans poubelle :" << dDiscretizationDeltaCost
				     << "\t Avec poubelle :" << dDiscretizationWithGarbageDeltaCost << endl;
				cout << "Nombre d'intervalles apres ce merge " << nIntervalNumber - 1
				     << "\t Variation cumulee du cout sans poubelle "
				     << dCumulatedDeltaCostWithoutGarbage << "\t Avec poubelle "
				     << dCumulatedDeltaCostWithGarbage << "\tTaillePoub " << nGarbageModalityNumber
				     << endl;
			}
		}
		else
		{
			if (bDisplayResults)
				cout << nIntervalNumber - 1 << "\t" << dCumulatedDeltaCostWithoutGarbage << endl;
			break;
		}

		// Est ce que la modelisation d'une poubelle est avantageuse
		dCumulatedDeltaCostWithGarbage < dCumulatedDeltaCostWithoutGarbage ? bGarbageProfitable = 1
										   : bGarbageProfitable = 0;

		// Gestion des problemes numeriques pour les valeur proches de zero
		if (fabs(dDiscretizationDeltaCost) < dEpsilon)
			dDiscretizationDeltaCost = 0;
		if (fabs(dDiscretizationWithGarbageDeltaCost) < dEpsilon)
			dDiscretizationWithGarbageDeltaCost = 0;

		// CH V9 TODO voir avec Marc : autres pistes pour condition d'arret des merges ?
		// On continue les merges :
		// {- s'il y a une amelioration du cout de la partition sans poubelle
		// ET
		// - s'il y a une amelioration du cout de la partition sans poubelle ou que la poubelle n'est pas encore
		// profitable
		// }
		// OU
		// - si contrainte de nombre max d'intervalles non respectee
		if ((dDiscretizationDeltaCost <= 0 and (nIntervalNumber > 3) and
		     ((bGarbageProfitable and dDiscretizationWithGarbageDeltaCost <= 0) or !bGarbageProfitable)) or
		    (GetMaxIntervalNumber() > 0 and nIntervalNumber > GetMaxIntervalNumber()))
		{
			// Affichage de la meilleure partition avec poubelle avant mise a jour
			if (bDisplayResults and dDiscretizationWithGarbageDeltaCost > 0)
			{
				KWMODLLineOptimization* intervalTemp;
				cout << "Meilleure table avec poubelle " << endl;
				intervalTemp = headInterval;
				while (intervalTemp != NULL)
				{
					// Affichage de la ligne courante
					cast(KWMODLLineOptimization*, intervalTemp)->Write(cout);

					// Intervalle suivant
					intervalTemp = cast(KWMODLLineOptimization*, intervalTemp->GetNext());
				}
			}

			UpdateMergeSortedList(&mergeList, headInterval, interval);
			assert(GetIntervalListTotalModalityNumber(headInterval) ==
			       discretizationCosts->GetValueNumber());
			nIntervalNumber--;
			nMergeNumber++;

			if (bDisplayResults and nIntervalNumber == 3)
			{
				// Affichage de la partition obtenue pour I=3 i.e. derniere partition avec groupe
				// poubelle
				// cout << "Table courante " << endl;
				interval = headInterval;
				while (interval != NULL)
				{
					// Affichage de la ligne courante
					cast(KWMODLLineOptimization*, interval)->Write(cout);

					// Intervalle suivant
					interval = cast(KWMODLLineOptimization*, interval->GetNext());
				}
			}
		}
		// Arret sinon
		else
			break;
	}

	// Nettoyage liste de travail
	ResetWorkingFrequencyList();

	// Affichage de la partition obtenue en fin de IntervalListMergeOptimizationWithGarbagePartitionCost
	if (bDisplayFinalResults)
	{
		cout << "Table obtenue a la fin d'IntervalListeMergeOptimizationWithGarbagePartitionCost " << endl;
		interval = headInterval;
		while (interval != NULL)
		{
			// Affichage de la ligne courante
			cast(KWMODLLineOptimization*, interval)->Write(cout);

			// Intervalle suivant
			interval = cast(KWMODLLineOptimization*, interval->GetNext());
		}
	}
}

boolean KWGrouperMODLTwoClasses::IsFrequencyTableSortedByTargetRatio(KWFrequencyTable* kwftFrequencyTable,
								     boolean bAscending, int nTargetIndex)
{
	boolean bSorted;
	int nSource;
	int nAscending;
	double dSortValue;
	double dPreviousSortValue;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	int nFrequencyVectorFrequency;

	require(kwftFrequencyTable != NULL);
	require(kwftFrequencyTable->GetFrequencyVectorNumber() > 0);
	require(0 <= nTargetIndex and nTargetIndex < kwftFrequencyTable->GetFrequencyVectorSize());
	require(cast(KWDenseFrequencyVector*, kwftFrequencyTable->GetFrequencyVectorAt(0))
		    ->GetFrequencyVector()
		    ->GetSize() > 0);

	// Sens du tri
	if (bAscending)
		nAscending = 1;
	else
		nAscending = -1;

	// Parcours des lignes de la table en verifier l'ordre
	bSorted = true;
	dSortValue = 0;
	for (nSource = 0; nSource < kwftFrequencyTable->GetFrequencyVectorNumber(); nSource++)
	{
		// Memorisation de la valeur precedente
		dPreviousSortValue = dSortValue;

		// Calcul de la valeur courante
		dSortValue = 0;
		kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, kwftFrequencyTable->GetFrequencyVectorAt(nSource));
		nFrequencyVectorFrequency = kwdfvFrequencyVector->ComputeTotalFrequency();
		if (nFrequencyVectorFrequency > 0)
			dSortValue = nAscending * kwdfvFrequencyVector->GetFrequencyVector()->GetAt(nTargetIndex) *
				     1.0 / nFrequencyVectorFrequency;

		// Comparaison avec la valeur precedente
		if (nSource > 0)
		{
			// Arret si les deux lignes ne sont pas ordonnees correctement
			if (dSortValue < dPreviousSortValue)
			{
				bSorted = false;
				break;
			}
		}
	}

	return bSorted;
}

///////////////////////////////////////////////////////////////
// Classe KWMODLGroup

KWMODLGroup* KWMODLGroup::Clone() const
{
	KWMODLGroup* kwmgClone;

	kwmgClone = new KWMODLGroup(kwfvFrequencyVector);
	kwmgClone->GetFrequencyVector()->CopyFrom(kwfvFrequencyVector);
	kwmgClone->SetCost(GetCost());
	kwmgClone->SetIndex(GetIndex());
	kwmgClone->SetPosition(GetPosition());
	return kwmgClone;
}

void KWMODLGroup::WriteHeaderLineReport(ostream& ost) const
{
	// Index
	ost << "Index";

	// Cout
	ost << "\tCost\t";

	// Position
	ost << "Position\t";

	// Libelle des valeurs cibles
	kwfvFrequencyVector->WriteHeaderLineReport(ost);
}

void KWMODLGroup::WriteLineReport(ostream& ost) const
{
	// Index
	ost << GetIndex();

	// Cout
	ost << "\t" << dCost << "\t";

	// Frequence des valeurs cibles
	kwfvFrequencyVector->WriteLineReport(ost);
}

void KWMODLGroup::Write(ostream& ost) const
{
	WriteHeaderLineReport(ost);
	ost << "\n";
	WriteLineReport(ost);
	ost << "\n";
}

int KWMODLGroupModalityNumberCompare(const void* elem1, const void* elem2)
{
	KWMODLGroup* group1;
	KWMODLGroup* group2;

	group1 = cast(KWMODLGroup*, *(Object**)elem1);
	group2 = cast(KWMODLGroup*, *(Object**)elem2);

	// Comparaison du nombre de modalites var valeurs decroissantes
	return (group2->GetModalityNumber() - group1->GetModalityNumber());
}

///////////////////////////////////////////////////////////////
// Classe KWMODLGroupMerge

KWMODLGroupMerge* KWMODLGroupMerge::Clone() const
{
	KWMODLGroupMerge* kwmgmClone;

	kwmgmClone = new KWMODLGroupMerge;
	kwmgmClone->SetDeltaCost(GetDeltaCost());
	kwmgmClone->SetIndex1(GetIndex2());
	kwmgmClone->SetIndex2(GetIndex2());
	kwmgmClone->SetPosition(GetPosition());
	return kwmgmClone;
}

void KWMODLGroupMerge::Write(ostream& ost) const
{
	ost << "Merge\t" << GetIndex1() << "\t" << GetIndex2() << "\t" << GetDeltaCost() << "\n";
}

int KWMODLGroupMergeCompare(const void* elem1, const void* elem2)
{
	double dResult;

	// Comparaison sur le critere de tri
	dResult = cast(KWMODLGroupMerge*, *(Object**)elem1)->GetDeltaCost() -
		  cast(KWMODLGroupMerge*, *(Object**)elem2)->GetDeltaCost();
	if (dResult > 0)
		return -1;
	else if (dResult < 0)
		return 1;
	else
		return 0;
}
