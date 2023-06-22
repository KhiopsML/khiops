// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDiscretizerMODL.h"

///////////////////////////////////////////////////////////////////////////////////////
// Optimisation basee sur des Merges

void KWDiscretizerMODL::IntervalListMergeOptimization(KWMODLLineOptimization*& headInterval) const
{
	SortedList mergeList(KWMODLLineOptimizationCompareMergeDeltaCost);
	KWMODLLineOptimization* interval;
	double dDiscretizationDeltaCost;
	int nIntervalNumber;

	boolean bDisplayResult = false;

	require(headInterval != NULL);

	// Initialisation des Merges
	InitializeMergeList(headInterval);

	// Initialisation de la liste triee
	InitializeMergeSortedList(&mergeList, headInterval);
	assert(mergeList.GetCount() == GetIntervalListSize(headInterval) - 1);

	// Recherche iterative du meilleur merge
	nIntervalNumber = mergeList.GetCount() + 1;
	while (mergeList.GetCount() > 0)
	{
		// Recherche du meilleur merge (DeltaCost le plus petit)
		interval = cast(KWMODLLineOptimization*, mergeList.GetHead());

		if (bDisplayResult)
		{
			cout << "Meilleur merge IntervalListMergeOptimization" << endl;
			interval->Write(cout);
			this->GetDiscretizationCosts()->Write(cout);
		}

		if (bDisplayResult)
			this->GetDiscretizationCosts()->Write(cout);

		// Transmission taille poubelle : vide car discretiseur ou grouper sans recherche de poubelle
		// Variation du cout du au codage d'un intervalle en moins
		dDiscretizationDeltaCost = ComputePartitionDeltaCost(nIntervalNumber, 0);

		if (bDisplayResult)
			cout << "PartitionDeltaCost\t" << dDiscretizationDeltaCost;

		// Prise en compte de la variation de cout de codage des exceptions
		dDiscretizationDeltaCost += interval->GetMerge()->GetDeltaCost();
		if (bDisplayResult)
			cout << "\tDiscretizationDeltaCost\t" << dDiscretizationDeltaCost << endl;

		// Gestion des problemes numeriques pour les valeur proches de zero
		if (fabs(dDiscretizationDeltaCost) < dEpsilon)
			dDiscretizationDeltaCost = 0;

		// On continue s'il y a une amelioration, ou si contrainte de nombre max
		// d'intervalles non respectee
		if (dDiscretizationDeltaCost <= 0 or
		    (GetMaxIntervalNumber() > 0 and nIntervalNumber > GetMaxIntervalNumber()))
		{
			UpdateMergeSortedList(&mergeList, headInterval, interval);
			nIntervalNumber--;
			nMergeNumber++;
		}
		// Arret sinon
		else
			break;
	}
}

void KWDiscretizerMODL::ComputeIntervalMerge(KWMODLLineOptimization* interval) const
{
	require(interval != NULL);
	require(interval->GetPrev() != NULL);
	require(CheckFrequencyVector(interval->GetFrequencyVector()));
	require(CheckFrequencyVector(interval->GetPrev()->GetFrequencyVector()));

	// Initialisation de la taille du vecteur des frequences cible du merge
	InitializeFrequencyVector(interval->GetMerge()->GetFrequencyVector());

	// Calcul des frequences cibles du merge
	MergeTwoFrequencyVectors(interval->GetMerge()->GetFrequencyVector(), interval->GetPrev()->GetFrequencyVector(),
				 interval->GetFrequencyVector());

	// Cout du merge
	interval->GetMerge()->SetCost(ComputeIntervalCost(interval->GetMerge()->GetFrequencyVector()));

	// Variation du cout suite au merge
	interval->GetMerge()->SetDeltaCost(interval->GetMerge()->GetCost() - interval->GetCost() -
					   interval->GetPrev()->GetCost());

	// Prise en compte de la contrainte d'effectif minimum, en ajoutant un bonus
	// si une des lignes sources du merge ne respecte pas la contrainte d'effectif
	// minimum
	if (GetMinIntervalFrequency() > 0)
	{
		if (interval->GetFrequencyVector()->ComputeTotalFrequency() < GetMinIntervalFrequency() or
		    interval->GetPrev()->GetFrequencyVector()->ComputeTotalFrequency() < GetMinIntervalFrequency())
			interval->GetMerge()->SetDeltaCost(interval->GetMerge()->GetDeltaCost() + dPriorityDeltaCost);
	}
}

void KWDiscretizerMODL::InitializeMergeList(KWMODLLineOptimization* headInterval) const
{
	KWMODLLineOptimization* interval;

	require(headInterval != NULL);

	interval = cast(KWMODLLineOptimization*, headInterval->GetNext());
	while (interval != NULL)
	{
		// Calcul d'un merge de deux intervalles
		ComputeIntervalMerge(interval);

		// Passage a l'intervalle suivant
		interval = cast(KWMODLLineOptimization*, interval->GetNext());
	}
}

void KWDiscretizerMODL::InitializeMergeSortedList(SortedList* mergeList, KWMODLLineOptimization* headInterval) const
{
	KWMODLLineOptimization* interval;

	require(mergeList != NULL);
	require(mergeList->GetCount() == 0);
	require(headInterval != NULL);

	// Insertion des merges (intervalles ayant un predecesseur)
	interval = cast(KWMODLLineOptimization*, headInterval->GetNext());
	while (interval != NULL)
	{
		// Insertion dans la liste triee, en memorisant la position d'insertion
		interval->GetMerge()->SetPosition(mergeList->Add(interval));

		// Passage a l'intervalle suivant
		interval = cast(KWMODLLineOptimization*, interval->GetNext());
	}
}

void KWDiscretizerMODL::UpdateMergeSortedList(SortedList* mergeList, KWMODLLineOptimization*& headInterval,
					      KWMODLLineOptimization* interval) const
{
	KWMODLLineOptimization* prevInterval;
	KWMODLLineOptimization* nextInterval;

	require(mergeList != NULL);
	require(headInterval != NULL);
	require(headInterval->GetNext() != NULL);
	require(interval != NULL);
	require(interval->GetPrev() != NULL);

	// Recherche des intervalles precedents et suivants
	prevInterval = cast(KWMODLLineOptimization*, interval->GetPrev());
	nextInterval = cast(KWMODLLineOptimization*, interval->GetNext());

	// On retire les deux intervalles merges de la liste triee selon les effectifs
	RemoveIntervalFromWorkingFrequencyList(interval);
	RemoveIntervalFromWorkingFrequencyList(prevInterval);

	// On retire les trois intervalles (potentiels) de la liste triee selon les variations de cout de merge
	mergeList->RemoveAt(interval->GetMerge()->GetPosition());
	debug(interval->GetMerge()->SetPosition(NULL));
	if (prevInterval->GetPrev() != NULL)
	{
		mergeList->RemoveAt(prevInterval->GetMerge()->GetPosition());
		debug(prevInterval->GetMerge()->SetPosition(NULL));
	}
	if (nextInterval != NULL)
	{
		mergeList->RemoveAt(nextInterval->GetMerge()->GetPosition());
		debug(nextInterval->GetMerge()->SetPosition(NULL));
	}

	// On fusionne effectivement l'intervalle en recopiant les information du merge
	interval->GetFrequencyVector()->CopyFrom(interval->GetMerge()->GetFrequencyVector());
	interval->SetCost(interval->GetMerge()->GetCost());

	// Reinsertion dans l'intervalle dans la liste triee des effectifs
	AddIntervalToWorkingFrequencyList(interval);

	// Mise a jour de la liste globale, en remplacant l'intervalle precedent
	// par le precedent du precedent
	if (prevInterval->GetPrev() == NULL)
	{
		interval->SetPrev(NULL);
		delete prevInterval;
		prevInterval = NULL;

		// L'element courant devient la tete de liste
		headInterval = interval;
	}
	else
	{
		prevInterval = cast(KWMODLLineOptimization*, prevInterval->GetPrev());
		delete prevInterval->GetNext();
		interval->SetPrev(prevInterval);
		prevInterval->SetNext(interval);
	}

	// Calcul des nouveaux merges bases sur le nouvel intervalle,
	// et reinsertion dans la liste triee
	if (prevInterval != NULL)
	{
		ComputeIntervalMerge(interval);
		interval->GetMerge()->SetPosition(mergeList->Add(interval));
	}
	if (nextInterval != NULL)
	{
		ComputeIntervalMerge(nextInterval);
		nextInterval->GetMerge()->SetPosition(mergeList->Add(nextInterval));
	}
}

///////////////////////////////////////////////////////////////////////////////////////
// Optimisation basee sur des Splits

void KWDiscretizerMODL::IntervalListSplitOptimization(const KWFrequencyTable* kwftSource,
						      KWMODLLineDeepOptimization*& headInterval) const
{
	SortedList splitList(KWMODLLineDeepOptimizationCompareSplitDeltaCost);
	KWMODLLineDeepOptimization* interval;
	double dDiscretizationDeltaCost;
	int nIntervalNumber;

	require(kwftSource != NULL);
	require(headInterval != NULL);

	// Initialisation des Splits
	InitializeSplitList(kwftSource, headInterval);

	// Initialisation de la liste triee des Split
	InitializeSplitSortedList(&splitList, headInterval);
	assert(splitList.GetCount() == GetIntervalListSize(headInterval));

	// Recherche iterative du meilleur Split
	nIntervalNumber = splitList.GetCount();
	while (splitList.GetCount() > 0)
	{
		// Recherche du meilleur merge (DeltaCost le plus petit)
		interval = cast(KWMODLLineDeepOptimization*, splitList.GetHead());

		// Prise en compte si au moins une coupure possible
		if (interval->GetSplit()->GetDeltaCost() < dInfiniteCost)
			dDiscretizationDeltaCost =
			    -ComputePartitionDeltaCost(nIntervalNumber + 1) + interval->GetSplit()->GetDeltaCost();
		else
		{
			interval = NULL;
			dDiscretizationDeltaCost = 1;
		}

		// Gestion des problemes numeriques pour les valeur proches de zero
		if (fabs(dDiscretizationDeltaCost) < dEpsilon)
			dDiscretizationDeltaCost = 0;

		// On continue s'il y a amelioration stricte, et si la contrainte de nombre
		// max d'intervalles est respectee apres le Split
		// Si la variation de cout est positive ou nulle: on arrete
		if (dDiscretizationDeltaCost < 0 and
		    (GetMaxIntervalNumber() == 0 or nIntervalNumber < GetMaxIntervalNumber()))
		{
			UpdateSplitSortedList(kwftSource, &splitList, headInterval, interval);
			nIntervalNumber++;
			nSplitNumber++;
		}
		// Arret sinon
		else
			break;
	}
}

void KWDiscretizerMODL::ComputeIntervalSplit(const KWFrequencyTable* kwftSource,
					     KWMODLLineDeepOptimization* interval) const
{
	int nFirstIndex;
	int nLastIndex;

	require(kwftSource != NULL);
	require(interval != NULL);
	require(CheckFrequencyVector(interval->GetFrequencyVector()));

	// Initialisation du meilleur point de coupure avec la solution courante
	interval->GetSplit()->GetFirstSubLineFrequencyVector()->CopyFrom(interval->GetFrequencyVector());
	interval->GetSplit()->SetDeltaCost(dInfiniteCost);

	// Calcul des bornes de recherche de la coupure
	if (interval->GetPrev() == NULL)
		nFirstIndex = 0;
	else
		nFirstIndex = interval->GetPrev()->GetIndex() + 1;
	nLastIndex = interval->GetIndex();
	assert(interval->GetPrev() == NULL or nFirstIndex <= nLastIndex);
	assert(nLastIndex < kwftSource->GetFrequencyVectorNumber());

	// On cherche a ameliorer la coupure s'il existe au moins un point de coupure potentiel
	if (nFirstIndex < nLastIndex)
		ComputeBestSplit(kwftSource, nFirstIndex, nLastIndex, interval->GetCost(), interval->GetSplit());
}

void KWDiscretizerMODL::InitializeSplitList(const KWFrequencyTable* kwftSource,
					    KWMODLLineDeepOptimization* headInterval) const
{
	KWMODLLineDeepOptimization* interval;

	// CH RefontePrior2-P-Inside
	boolean bDisplaySplit = false;
	// Fin CH RefontePrior2

	require(kwftSource != NULL);
	require(headInterval != NULL);

	// Initialisation des Splits des intervalles
	interval = cast(KWMODLLineDeepOptimization*, headInterval);

	if (bDisplaySplit)
		cout << "Initialisation des Split " << endl;

	while (interval != NULL)
	{
		// Calcul d'un Split d'intervalle
		ComputeIntervalSplit(kwftSource, interval);

		if (bDisplaySplit)
		{
			cout << "Intervalle " << endl;
			interval->Write(cout);
			cout << " Son meilleur split" << endl;
			interval->GetSplit()->Write(cout);
		}

		// Passage a l'intervalle suivant
		interval = cast(KWMODLLineDeepOptimization*, interval->GetNext());
	}
}

void KWDiscretizerMODL::InitializeSplitSortedList(SortedList* splitList, KWMODLLineDeepOptimization* headInterval) const
{
	KWMODLLineDeepOptimization* interval;

	require(splitList != NULL);
	require(splitList->GetCount() == 0);
	require(headInterval != NULL);

	// Insertion des Splits (tous les intervalles)
	interval = headInterval;
	while (interval != NULL)
	{
		// Insertion dans la liste triee, en memorisant la position d'insertion
		interval->GetSplit()->SetPosition(splitList->Add(interval));

		// Passage a l'intervalle suivant
		interval = cast(KWMODLLineDeepOptimization*, interval->GetNext());
	}
}

void KWDiscretizerMODL::UpdateSplitSortedList(const KWFrequencyTable* kwftSource, SortedList* splitList,
					      KWMODLLineDeepOptimization*& headInterval,
					      KWMODLLineDeepOptimization* interval) const
{
	KWMODLLineDeepOptimization* newInterval;

	require(kwftSource != NULL);
	require(splitList != NULL);
	require(headInterval != NULL);
	require(interval != NULL);
	require(CheckFrequencyVector(interval->GetFrequencyVector()));

	// On retire l'intervalle de la liste triee
	splitList->RemoveAt(interval->GetSplit()->GetPosition());
	debug(interval->GetSplit()->SetPosition(NULL));

	// Le nouvel intervalle est insere apres l'intervalle courant
	newInterval = new KWMODLLineDeepOptimization(GetFrequencyVectorCreator());
	newInterval->SetNext(interval->GetNext());
	if (newInterval->GetNext() != NULL)
		newInterval->GetNext()->SetPrev(newInterval);
	interval->SetNext(newInterval);
	newInterval->SetPrev(interval);

	// Reconstruction des nouveaux vecteurs d'effectifs
	InitializeFrequencyVector(newInterval->GetFrequencyVector());
	SplitFrequencyVector(interval->GetFrequencyVector(), newInterval->GetFrequencyVector(),
			     interval->GetSplit()->GetFirstSubLineFrequencyVector());

	// Transfert des informations vers les deux nouveaux intervalles
	newInterval->SetIndex(interval->GetIndex());
	interval->SetIndex(interval->GetSplit()->GetFirstSubLineIndex());
	interval->SetCost(interval->GetSplit()->GetFirstSubLineCost());
	newInterval->SetCost(interval->GetSplit()->GetSecondSubLineCost());

	// Calcul des nouveaux Splits bases sur le nouvel intervalle,
	// et reinsertion dans la liste triee
	ComputeIntervalSplit(kwftSource, interval);
	interval->GetSplit()->SetPosition(splitList->Add(interval));
	ComputeIntervalSplit(kwftSource, newInterval);
	newInterval->GetSplit()->SetPosition(splitList->Add(newInterval));
}

///////////////////////////////////////////////////////////////////////////////////////
// Optimisation basee sur des MergeSplits

void KWDiscretizerMODL::IntervalListMergeSplitOptimization(const KWFrequencyTable* kwftSource,
							   KWMODLLineDeepOptimization*& headInterval) const
{
	SortedList mergeSplitList(KWMODLLineDeepOptimizationCompareMergeSplitDeltaCost);
	KWMODLLineDeepOptimization* interval;
	double dDiscretizationDeltaCost;

	require(kwftSource != NULL);
	require(headInterval != NULL);

	// Initialisation des MergeSplits
	InitializeMergeSplitList(kwftSource, headInterval);

	// Initialisation de la liste triee des MergeSplit
	InitializeMergeSplitSortedList(&mergeSplitList, headInterval);
	assert(mergeSplitList.GetCount() == GetIntervalListSize(headInterval) - 1);

	// Recherche iterative du meilleur MergeSplit
	while (mergeSplitList.GetCount() > 0)
	{
		// Recherche du meilleur merge (DeltaCost le plus petit)
		interval = cast(KWMODLLineDeepOptimization*, mergeSplitList.GetHead());

		// Variation du cout du au changement du point de coupure
		dDiscretizationDeltaCost = interval->GetMergeSplit()->GetDeltaCost();

		// Gestion des problemes numeriques pour les valeur proches de zero
		if (fabs(dDiscretizationDeltaCost) < dEpsilon)
			dDiscretizationDeltaCost = 0;

		// On continue s'il y a amelioration stricte
		if (dDiscretizationDeltaCost < 0)
		{
			UpdateMergeSplitSortedList(kwftSource, &mergeSplitList, headInterval, interval);
			nMergeSplitNumber++;
		}
		// Arret sinon
		else
			break;
	}
}

void KWDiscretizerMODL::ComputeIntervalMergeSplit(const KWFrequencyTable* kwftSource,
						  KWMODLLineDeepOptimization* interval) const
{
	int nFirstIndex;
	int nLastIndex;

	require(kwftSource != NULL);
	require(interval != NULL);
	require(interval->GetPrev() != NULL);
	require(CheckFrequencyVector(interval->GetPrev()->GetFrequencyVector()));
	require(CheckFrequencyVector(interval->GetFrequencyVector()));

	// Initialisation du meilleur point de coupure avec le premier intervalle plein
	InitializeFrequencyVector(interval->GetMergeSplit()->GetFirstSubLineFrequencyVector());
	MergeTwoFrequencyVectors(interval->GetMergeSplit()->GetFirstSubLineFrequencyVector(),
				 interval->GetPrev()->GetFrequencyVector(), interval->GetFrequencyVector());
	interval->GetMergeSplit()->SetDeltaCost(dInfiniteCost);

	// Calcul des bornes de recherche de la coupure
	if (interval->GetPrev()->GetPrev() == NULL)
		nFirstIndex = 0;
	else
		nFirstIndex = interval->GetPrev()->GetPrev()->GetIndex() + 1;
	nLastIndex = interval->GetIndex();
	assert(nFirstIndex < nLastIndex);
	assert(nLastIndex < kwftSource->GetFrequencyVectorNumber());

	// On cherche a ameliorer la coupure s'il existe plus de un point de coupure potentiel
	if (nFirstIndex + 1 < nLastIndex)
		ComputeBestSplit(kwftSource, nFirstIndex, nLastIndex,
				 interval->GetCost() + interval->GetPrev()->GetCost(), interval->GetMergeSplit());
}

void KWDiscretizerMODL::InitializeMergeSplitList(const KWFrequencyTable* kwftSource,
						 KWMODLLineDeepOptimization* headInterval) const
{
	KWMODLLineDeepOptimization* interval;
	boolean bDisplayMergeSplit = false;

	require(kwftSource != NULL);
	require(headInterval != NULL);

	// On creer un tableau d'effectif (vide) pour le premier intervalle, pour pouvoir
	// utiliser la methode d'affichage WriteIntervalListReport sans probleme de colonnage des infos
	interval = cast(KWMODLLineDeepOptimization*, headInterval);
	InitializeFrequencyVector(interval->GetMergeSplit()->GetFirstSubLineFrequencyVector());
	interval = cast(KWMODLLineDeepOptimization*, interval->GetNext());

	// Initialisation des MergeSplits entre un intervalle et son predecesseur
	while (interval != NULL)
	{
		// Calcul d'un merge de deux intervalles
		ComputeIntervalMergeSplit(kwftSource, interval);

		if (bDisplayMergeSplit)
		{
			cout << "Intervalle " << endl;
			interval->Write(cout);
			cout << " Son meilleur MergeSplit" << endl;
			interval->GetMergeSplit()->Write(cout);
		}

		// Passage a l'intervalle suivant
		interval = cast(KWMODLLineDeepOptimization*, interval->GetNext());
	}
}

void KWDiscretizerMODL::InitializeMergeSplitSortedList(SortedList* mergeSplitList,
						       KWMODLLineDeepOptimization* headInterval) const
{
	KWMODLLineDeepOptimization* interval;

	require(mergeSplitList != NULL);
	require(mergeSplitList->GetCount() == 0);
	require(headInterval != NULL);

	// Insertion des MergeSplits (intervalles ayant un predecesseur)
	interval = cast(KWMODLLineDeepOptimization*, headInterval->GetNext());
	while (interval != NULL)
	{
		// Insertion dans la liste triee, en memorisant la position d'insertion
		check(interval->GetPrev());
		interval->GetMergeSplit()->SetPosition(mergeSplitList->Add(interval));

		// Passage a l'intervalle suivant
		interval = cast(KWMODLLineDeepOptimization*, interval->GetNext());
	}
}

void KWDiscretizerMODL::UpdateMergeSplitSortedList(const KWFrequencyTable* kwftSource, SortedList* mergeSplitList,
						   KWMODLLineDeepOptimization*& headInterval,
						   KWMODLLineDeepOptimization* interval) const
{
	KWMODLLineDeepOptimization* prevInterval;
	KWMODLLineDeepOptimization* nextInterval;

	require(kwftSource != NULL);
	require(mergeSplitList != NULL);
	require(headInterval != NULL);
	require(headInterval->GetNext() != NULL);
	require(interval != NULL);
	require(interval->GetPrev() != NULL);
	require(CheckFrequencyVector(interval->GetFrequencyVector()));
	require(CheckFrequencyVector(interval->GetPrev()->GetFrequencyVector()));

	// Recherche des intervalles precedents et suivants
	prevInterval = cast(KWMODLLineDeepOptimization*, interval->GetPrev());
	nextInterval = cast(KWMODLLineDeepOptimization*, interval->GetNext());

	// On retire les trois intervalles (potentiels) de la liste triee
	mergeSplitList->RemoveAt(interval->GetMergeSplit()->GetPosition());
	debug(interval->GetMergeSplit()->SetPosition(NULL));
	if (prevInterval->GetPrev() != NULL)
	{
		mergeSplitList->RemoveAt(prevInterval->GetMergeSplit()->GetPosition());
		debug(prevInterval->GetMergeSplit()->SetPosition(NULL));
	}
	if (nextInterval != NULL)
	{
		mergeSplitList->RemoveAt(nextInterval->GetMergeSplit()->GetPosition());
		debug(nextInterval->GetMergeSplit()->SetPosition(NULL));
	}

	// On change effectivement les bornes de l'intervalle en recopiant les
	// informations de MergeSplit
	prevInterval->SetIndex(interval->GetMergeSplit()->GetFirstSubLineIndex());
	prevInterval->SetCost(interval->GetMergeSplit()->GetFirstSubLineCost());
	interval->SetCost(interval->GetMergeSplit()->GetSecondSubLineCost());

	// Reconstruction des nouveaux vecteurs d'effectifs
	MergeSplitFrequencyVectors(prevInterval->GetFrequencyVector(), interval->GetFrequencyVector(),
				   interval->GetMergeSplit()->GetFirstSubLineFrequencyVector());

	// Calcul des nouveaux MergeSplits bases sur le nouvel intervalle,
	// et reinsertion dans la liste triee
	interval->GetMergeSplit()->SetDeltaCost(0);
	interval->GetMergeSplit()->SetPosition(mergeSplitList->Add(interval));
	if (prevInterval->GetPrev() != NULL)
	{
		ComputeIntervalMergeSplit(kwftSource, prevInterval);
		prevInterval->GetMergeSplit()->SetPosition(mergeSplitList->Add(prevInterval));
	}
	if (nextInterval != NULL)
	{
		ComputeIntervalMergeSplit(kwftSource, nextInterval);
		nextInterval->GetMergeSplit()->SetPosition(mergeSplitList->Add(nextInterval));
	}
}

///////////////////////////////////////////////////////////////////////////////////////
// Optimisation basee sur des MergeMergeSplits

void KWDiscretizerMODL::IntervalListMergeMergeSplitOptimization(const KWFrequencyTable* kwftSource,
								KWMODLLineDeepOptimization*& headInterval) const
{
	SortedList mergeMergeSplitList(KWMODLLineDeepOptimizationCompareMergeMergeSplitDeltaCost);
	KWMODLLineDeepOptimization* interval;
	double dDiscretizationDeltaCost;
	int nIntervalNumber;

	require(kwftSource != NULL);
	require(headInterval != NULL);

	// Initialisation des MergeMergeSplits
	InitializeMergeMergeSplitList(kwftSource, headInterval);

	// Initialisation de la liste triee des MergeMergeSplit
	InitializeMergeMergeSplitSortedList(&mergeMergeSplitList, headInterval);

	// Calcul du nombre d'intervalle initial
	if (mergeMergeSplitList.GetCount() > 0)
		nIntervalNumber = mergeMergeSplitList.GetCount() + 2;
	else if (headInterval->GetNext() == NULL)
		nIntervalNumber = 1;
	else
		nIntervalNumber = 2;
	assert(nIntervalNumber == GetIntervalListSize(headInterval));

	// Recherche iterative du meilleur MergeMergeSplit
	while (mergeMergeSplitList.GetCount() > 0)
	{
		// Recherche du meilleur merge (DeltaCost le plus petit)
		interval = cast(KWMODLLineDeepOptimization*, mergeMergeSplitList.GetHead());

		// Variation du cout du au codage d'un intervalle en moins
		dDiscretizationDeltaCost = ComputePartitionDeltaCost(nIntervalNumber);

		// Prise en compte de la variation de cout de codage des exceptions
		dDiscretizationDeltaCost += interval->GetMergeMergeSplit()->GetDeltaCost();

		// Gestion des problemes numeriques pour les valeur proches de zero
		if (fabs(dDiscretizationDeltaCost) < dEpsilon)
			dDiscretizationDeltaCost = 0;

		// On continue s'il y a une amelioration, ou si contrainte de nombre max
		// d'intervalles non respectee
		if (dDiscretizationDeltaCost <= 0 or
		    (GetMaxIntervalNumber() > 0 and nIntervalNumber > GetMaxIntervalNumber()))
		{
			UpdateMergeMergeSplitSortedList(kwftSource, &mergeMergeSplitList, headInterval, interval);
			nIntervalNumber--;
			nMergeMergeSplitNumber++;
		}
		// Arret sinon
		else
			break;
	}
}

void KWDiscretizerMODL::ComputeIntervalMergeMergeSplit(const KWFrequencyTable* kwftSource,
						       KWMODLLineDeepOptimization* interval) const
{
	KWMODLLineDeepOptimization* prevInterval;
	KWMODLLineDeepOptimization* prevPrevInterval;
	int nFirstIndex;
	int nLastIndex;

	require(kwftSource != NULL);
	require(interval != NULL);
	require(interval->GetPrev() != NULL);
	require(interval->GetPrev()->GetPrev() != NULL);
	require(CheckFrequencyVector(interval->GetFrequencyVector()));
	require(CheckFrequencyVector(interval->GetPrev()->GetFrequencyVector()));
	require(CheckFrequencyVector(interval->GetPrev()->GetPrev()->GetFrequencyVector()));

	// Recherche des intervalles predecesseurs
	prevInterval = cast(KWMODLLineDeepOptimization*, interval->GetPrev());
	prevPrevInterval = cast(KWMODLLineDeepOptimization*, prevInterval->GetPrev());

	// Initialisation de la solution en remplissant totalement le premier sous-intervalle
	InitializeFrequencyVector(interval->GetMergeMergeSplit()->GetFirstSubLineFrequencyVector());
	MergeThreeFrequencyVectors(interval->GetMergeMergeSplit()->GetFirstSubLineFrequencyVector(),
				   prevPrevInterval->GetFrequencyVector(), prevInterval->GetFrequencyVector(),
				   interval->GetFrequencyVector());
	interval->GetMergeMergeSplit()->SetDeltaCost(dInfiniteCost);

	// Calcul des bornes de recherche de la coupure
	if (prevPrevInterval->GetPrev() == NULL)
		nFirstIndex = 0;
	else
		nFirstIndex = prevPrevInterval->GetPrev()->GetIndex() + 1;
	nLastIndex = interval->GetIndex();
	assert(nFirstIndex + 1 < nLastIndex);
	assert(nLastIndex < kwftSource->GetFrequencyVectorNumber());

	// Recherche du meilleur point de coupure
	ComputeBestSplit(kwftSource, nFirstIndex, nLastIndex,
			 interval->GetCost() + interval->GetPrev()->GetCost() +
			     interval->GetPrev()->GetPrev()->GetCost(),
			 interval->GetMergeMergeSplit());
}

void KWDiscretizerMODL::InitializeMergeMergeSplitList(const KWFrequencyTable* kwftSource,
						      KWMODLLineDeepOptimization* headInterval) const
{
	KWMODLLineDeepOptimization* interval;
	boolean bDisplayMergeMergeSplit = false;

	require(kwftSource != NULL);
	require(headInterval != NULL);

	// On creer un tableau d'effectif (vide) pour les deux premiers intervalle, pour pouvoir
	// utiliser la methode d'affichage WriteIntervalListReport sans probleme de colonnage des infos
	interval = cast(KWMODLLineDeepOptimization*, headInterval);
	InitializeFrequencyVector(interval->GetMergeMergeSplit()->GetFirstSubLineFrequencyVector());
	interval = cast(KWMODLLineDeepOptimization*, interval->GetNext());
	if (interval != NULL)
	{
		InitializeFrequencyVector(interval->GetMergeMergeSplit()->GetFirstSubLineFrequencyVector());
		interval = cast(KWMODLLineDeepOptimization*, interval->GetNext());
	}

	// Initialisation des MergeMergeSplits des intervalles
	while (interval != NULL)
	{
		// Calcul d'un MergeMergeSplit de trois intervalles
		ComputeIntervalMergeMergeSplit(kwftSource, interval);

		if (bDisplayMergeMergeSplit)
		{
			cout << "Intervalle " << endl;
			interval->Write(cout);
			cout << " Son meilleur MergeMergeSplit" << endl;
			interval->GetMergeMergeSplit()->Write(cout);
		}

		// Passage a l'intervalle suivant
		interval = cast(KWMODLLineDeepOptimization*, interval->GetNext());
	}
}

void KWDiscretizerMODL::InitializeMergeMergeSplitSortedList(SortedList* mergeMergeSplitList,
							    KWMODLLineDeepOptimization* headInterval) const
{
	KWMODLLineDeepOptimization* interval;

	require(mergeMergeSplitList != NULL);
	require(mergeMergeSplitList->GetCount() == 0);
	require(headInterval != NULL);

	// Insertion des merges (intervalles ayant deux predecesseurs)
	interval = cast(KWMODLLineDeepOptimization*, headInterval->GetNext());
	if (interval != NULL)
		interval = cast(KWMODLLineDeepOptimization*, interval->GetNext());
	while (interval != NULL)
	{
		// Insertion dans la liste triee, en memorisant la position d'insertion
		check(interval->GetPrev());
		interval->GetMergeMergeSplit()->SetPosition(mergeMergeSplitList->Add(interval));

		// Passage a l'intervalle suivant
		interval = cast(KWMODLLineDeepOptimization*, interval->GetNext());
	}
}

void KWDiscretizerMODL::UpdateMergeMergeSplitSortedList(const KWFrequencyTable* kwftSource,
							SortedList* mergeMergeSplitList,
							KWMODLLineDeepOptimization*& headInterval,
							KWMODLLineDeepOptimization* interval) const
{
	KWMODLLineDeepOptimization* prevInterval;
	KWMODLLineDeepOptimization* prevPrevInterval;
	KWMODLLineDeepOptimization* nextInterval;
	KWMODLLineDeepOptimization* nextNextInterval;

	require(kwftSource != NULL);
	require(mergeMergeSplitList != NULL);
	require(headInterval != NULL);
	require(headInterval->GetNext() != NULL);
	require(headInterval->GetNext()->GetNext() != NULL);
	require(interval != NULL);
	require(interval->GetPrev() != NULL);
	require(interval->GetPrev()->GetPrev() != NULL);
	require(CheckFrequencyVector(interval->GetFrequencyVector()));
	require(CheckFrequencyVector(interval->GetPrev()->GetFrequencyVector()));
	require(CheckFrequencyVector(interval->GetPrev()->GetPrev()->GetFrequencyVector()));

	// Recherche des intervalles precedents et suivants
	prevInterval = cast(KWMODLLineDeepOptimization*, interval->GetPrev());
	prevPrevInterval = cast(KWMODLLineDeepOptimization*, prevInterval->GetPrev());
	nextInterval = cast(KWMODLLineDeepOptimization*, interval->GetNext());
	nextNextInterval = NULL;
	if (nextInterval != NULL)
		nextNextInterval = cast(KWMODLLineDeepOptimization*, nextInterval->GetNext());

	// On retire les cinq intervalles (potentiels) de la liste triee
	mergeMergeSplitList->RemoveAt(interval->GetMergeMergeSplit()->GetPosition());
	debug(interval->GetMergeMergeSplit()->SetPosition(NULL));
	if (prevPrevInterval->GetPrev() != NULL)
	{
		mergeMergeSplitList->RemoveAt(prevInterval->GetMergeMergeSplit()->GetPosition());
		debug(prevInterval->GetMergeMergeSplit()->SetPosition(NULL));
		if (prevPrevInterval->GetPrev()->GetPrev() != NULL)
		{
			mergeMergeSplitList->RemoveAt(prevPrevInterval->GetMergeMergeSplit()->GetPosition());
			debug(prevPrevInterval->GetMergeMergeSplit()->SetPosition(NULL));
		}
	}
	if (nextInterval != NULL)
	{
		mergeMergeSplitList->RemoveAt(nextInterval->GetMergeMergeSplit()->GetPosition());
		debug(nextInterval->GetMergeMergeSplit()->SetPosition(NULL));
	}
	if (nextNextInterval != NULL)
	{
		mergeMergeSplitList->RemoveAt(nextNextInterval->GetMergeMergeSplit()->GetPosition());
		debug(nextNextInterval->GetMergeMergeSplit()->SetPosition(NULL));
	}

	// On change effectivement les bornes de l'intervalle en recopiant les
	// informations de MergeMergeSplit
	prevPrevInterval->SetIndex(interval->GetMergeMergeSplit()->GetFirstSubLineIndex());
	prevPrevInterval->SetCost(interval->GetMergeMergeSplit()->GetFirstSubLineCost());
	interval->SetCost(interval->GetMergeMergeSplit()->GetSecondSubLineCost());

	// Reconstruction des nouveaux vecteurs d'effectifs
	MergeMergeSplitFrequencyVectors(prevPrevInterval->GetFrequencyVector(), prevInterval->GetFrequencyVector(),
					interval->GetFrequencyVector(),
					interval->GetMergeMergeSplit()->GetFirstSubLineFrequencyVector());

	// Chainage des deux intervalles extremites, et supression de l'intervalle du milieu
	prevPrevInterval->SetNext(interval);
	interval->SetPrev(prevPrevInterval);
	delete prevInterval;

	// Calcul des nouveaux MergeMergeSplits bases sur le nouvel intervalle,
	// et reinsertion dans la liste triee
	if (prevPrevInterval->GetPrev() != NULL)
	{
		ComputeIntervalMergeMergeSplit(kwftSource, interval);
		interval->GetMergeMergeSplit()->SetPosition(mergeMergeSplitList->Add(interval));
		if (prevPrevInterval->GetPrev()->GetPrev() != NULL)
		{
			ComputeIntervalMergeMergeSplit(kwftSource, prevPrevInterval);
			prevPrevInterval->GetMergeMergeSplit()->SetPosition(mergeMergeSplitList->Add(prevPrevInterval));
		}
	}
	if (nextInterval != NULL)
	{
		ComputeIntervalMergeMergeSplit(kwftSource, nextInterval);
		nextInterval->GetMergeMergeSplit()->SetPosition(mergeMergeSplitList->Add(nextInterval));
	}
	if (nextNextInterval != NULL)
	{
		ComputeIntervalMergeMergeSplit(kwftSource, nextNextInterval);
		nextNextInterval->GetMergeMergeSplit()->SetPosition(mergeMergeSplitList->Add(nextNextInterval));
	}
}

//////////////////////////////////////////////////////////////////
// Recherche d'une discretisation optimale

KWMODLLine* KWDiscretizerMODL::ComputeOptimalDiscretization(const KWFrequencyTable* kwftSource) const
{
	KWMODLLine lineCreator(GetFrequencyVectorCreator());
	KWMODLLineOptimalDiscretization* headLineOptimalDiscretization;
	KWMODLLine* headInterval;
	double dModelCost;
	double dTotalCost;
	double dBestTotalCost;
	IntVector ivBestLastLineIndexes;
	int i;
	boolean bNewInterval;

	require(kwftSource != NULL);

	// Initialisation des discretisations optimales a un seul intervalle
	headLineOptimalDiscretization = InitializeOptimalDiscretizations(kwftSource);

	// Memorisation de la solution initiale (a un seul intervalle)
	dModelCost = ComputePartitionCost(headLineOptimalDiscretization->GetIntervalNumber());
	dBestTotalCost = dModelCost + headLineOptimalDiscretization->GetDiscretizationCost();
	ivBestLastLineIndexes.SetSize(headLineOptimalDiscretization->GetIntervalNumber());
	for (i = 0; i < ivBestLastLineIndexes.GetSize(); i++)
		ivBestLastLineIndexes.SetAt(i, headLineOptimalDiscretization->GetLastLineIndexAt(i));

	// On rajoute des intervalles tant que c'est possible
	while (headLineOptimalDiscretization->GetIntervalNumber() <
	       headLineOptimalDiscretization->GetMaxIntervalNumber())
	{
		// Mise a jour des discretisations optimales en ajoutant un intervalle
		bNewInterval = UpdateOptimalDiscretizations(kwftSource, headLineOptimalDiscretization);

		// arret si pas de nouvel intervalle produit
		if (not bNewInterval)
			break;

		// Calcul du cout du model (le calcul par DeltaCost est plus economique)
		dModelCost -= ComputePartitionDeltaCost(headLineOptimalDiscretization->GetIntervalNumber());
		assert(fabs(dModelCost - ComputePartitionCost(headLineOptimalDiscretization->GetIntervalNumber())) <
		       dEpsilon);

		// Test si amelioration
		dTotalCost = dModelCost + headLineOptimalDiscretization->GetDiscretizationCost();
		if (dTotalCost < dBestTotalCost - dEpsilon)
		{
			dBestTotalCost = dTotalCost;

			// Memorisation de la solution
			ivBestLastLineIndexes.SetSize(headLineOptimalDiscretization->GetIntervalNumber());
			for (i = 0; i < ivBestLastLineIndexes.GetSize(); i++)
				ivBestLastLineIndexes.SetAt(i, headLineOptimalDiscretization->GetLastLineIndexAt(i));
		}
	}

	// Initialisation de la liste des intervalles
	headInterval =
	    BuildIntervalListFromFrequencyTableAndIntervalBounds(&lineCreator, kwftSource, &ivBestLastLineIndexes);

	// Nettoyage
	DeleteIntervalList(headLineOptimalDiscretization);

	return headInterval;
}

KWMODLLineOptimalDiscretization*
KWDiscretizerMODL::InitializeOptimalDiscretizations(const KWFrequencyTable* kwftSource) const
{
	KWMODLLine lineCreator(GetFrequencyVectorCreator());
	KWMODLLineOptimalDiscretization lineOptimalDiscretizationCreator(GetFrequencyVectorCreator());
	KWMODLLineOptimalDiscretization* headInterval;
	KWMODLLineOptimalDiscretization* interval;
	KWMODLLine* headUniqueInterval;
	KWFrequencyVector* kwfvFrequencyVector;
	int nSourceValueNumber;
	int nTotalFrequency;
	int nMaxNumber;

	require(kwftSource != NULL);

	// Construction de la liste des intervalles a partir de la table source
	headInterval = cast(KWMODLLineOptimalDiscretization*,
			    BuildIntervalListFromFrequencyTable(&lineOptimalDiscretizationCreator, kwftSource));

	// Initialisation d'un intervalle unique contenant toute la table de contingence
	// pour obtenir une vecteur d'effectif de travail, initialement contenant tout
	headUniqueInterval = cast(KWMODLLine*, BuildUniqueIntervalFromFrequencyTable(&lineCreator, kwftSource));
	kwfvFrequencyVector = headUniqueInterval->GetFrequencyVector();
	assert(kwfvFrequencyVector->ComputeTotalFrequency() == kwftSource->GetTotalFrequency());

	// Initialisation des intervalles
	nSourceValueNumber = kwftSource->GetFrequencyVectorNumber();
	nTotalFrequency = kwfvFrequencyVector->ComputeTotalFrequency();
	interval = headInterval;
	while (interval != NULL)
	{
		// Cout de l'intervalle
		interval->SetCost(ComputeIntervalCost(interval->GetFrequencyVector()));

		// Nombre max d'intervalles d'une discretisation base sur la fin des lignes
		// de la table de contingence (initialement: Index = rang de l'intervalle)
		// Les dicretisation commeancant vers la fin de la table ont moins
		// d'intervalles.
		// On limite egalement par la contrainte du nombre max d'intervalle et
		// de l'effectif minimum par intervalle (sauf pour la discretisation globale)
		nMaxNumber = nSourceValueNumber - interval->GetIndex();
		if (GetMaxIntervalNumber() > 0 and nMaxNumber > GetMaxIntervalNumber())
			nMaxNumber = GetMaxIntervalNumber();
		if (GetMinIntervalFrequency() > 0 and nMaxNumber > nTotalFrequency / GetMinIntervalFrequency())
			nMaxNumber = nTotalFrequency / GetMinIntervalFrequency();
		if (nMaxNumber == 0 and interval == headInterval)
			nMaxNumber = 1;
		interval->SetMaxIntervalNumber(nMaxNumber);

		// Nombre d'intervalle effectif: un pour l'initialisation
		// (si son effectif minimum est suffisant)
		if (interval == headInterval or GetMinIntervalFrequency() == 0 or
		    kwfvFrequencyVector->ComputeTotalFrequency() >= GetMinIntervalFrequency())
		{
			// Derniere ligne de l'intervalle
			interval->SetIntervalNumber(1);
			interval->SetLastLineIndexAt(0, nSourceValueNumber - 1);
		}

		// Cout de la discretisation basee sur un intervalle, union des lignes
		// de la fin du tableau de contingence
		interval->SetDiscretizationCost(ComputeIntervalCost(kwfvFrequencyVector));

		// Mise a jour de l'intervalle en supprimant la contribution de la ligne courante
		RemoveFrequencyVector(kwfvFrequencyVector, interval->GetFrequencyVector());
		assert(kwfvFrequencyVector->ComputeTotalFrequency() >= 0);

		// Passage a l'intervalle suivant
		interval = cast(KWMODLLineOptimalDiscretization*, interval->GetNext());
	}

	// Nettoyage
	DeleteIntervalList(headUniqueInterval);

	return headInterval;
}

boolean KWDiscretizerMODL::UpdateOptimalDiscretizations(const KWFrequencyTable* kwftSource,
							KWMODLLineOptimalDiscretization* headInterval) const
{
	boolean bNewInterval;
	int nInitialIntervalNumber;
	KWMODLLineOptimalDiscretization* interval;
	KWMODLLineOptimalDiscretization* intervalEndDiscretisation;
	KWMODLLineOptimalDiscretization* intervalBestEndDiscretisation;
	KWFrequencyVector* kwfvFirstIntervalFrequencyVector;
	double dDiscretizationCost;
	double dBestDiscretizationCost;
	int i;

	require(kwftSource != NULL);

	// Creation du vecteur d'effectif de travail
	kwfvFirstIntervalFrequencyVector = GetFrequencyVectorCreator()->Create();

	// Recherche pour chaque intervalle de la meilleure discretisation composee
	// d'un premier intervalle, et d'une fin de discretisation en (K-1) intervalles
	nInitialIntervalNumber = headInterval->GetIntervalNumber();
	interval = headInterval;
	while (interval != NULL)
	{
		// Arret de la recherche s'il ne reste pas suffisament d'intervalles potentiels
		if (interval->GetIntervalNumber() == interval->GetMaxIntervalNumber() or
		    interval->GetIntervalNumber() < nInitialIntervalNumber)
			break;

		// Initialisation du premier intervalle de la discretisation restante avec la
		// ligne courante de la table de contingence
		kwfvFirstIntervalFrequencyVector->CopyFrom(interval->GetFrequencyVector());

		// Recherche parmi les fins de discretisation possible
		intervalEndDiscretisation = cast(KWMODLLineOptimalDiscretization*, interval->GetNext());
		intervalBestEndDiscretisation = NULL;
		dBestDiscretizationCost = dInfiniteCost;
		while (intervalEndDiscretisation != NULL)
		{
			// Arret si pas assez d'intervalles disponibles
			if (intervalEndDiscretisation->GetIntervalNumber() < interval->GetIntervalNumber())
				break;

			// Evaluation du cout de la discretisation de la fin de la table:
			// cout du premier intervalle + cout fin de discretisation
			// (si son effectif minimum est suffisant)
			dDiscretizationCost = dInfiniteCost;
			if (GetMinIntervalFrequency() == 0 or
			    kwfvFirstIntervalFrequencyVector->ComputeTotalFrequency() >= GetMinIntervalFrequency())
				dDiscretizationCost = ComputeIntervalCost(kwfvFirstIntervalFrequencyVector) +
						      intervalEndDiscretisation->GetDiscretizationCost();

			// Test si amelioration
			if (dDiscretizationCost < dBestDiscretizationCost)
			{
				intervalBestEndDiscretisation = intervalEndDiscretisation;
				dBestDiscretizationCost = dDiscretizationCost;
			}

			// Mise a jour de l'intervalle en ajoutant la contribution de la ligne courante
			AddFrequencyVector(kwfvFirstIntervalFrequencyVector,
					   intervalEndDiscretisation->GetFrequencyVector());

			// Fin de discretisation suivante
			intervalEndDiscretisation =
			    cast(KWMODLLineOptimalDiscretization*, intervalEndDiscretisation->GetNext());
		}

		// Memorisation de la meilleure solution trouvee
		if (intervalBestEndDiscretisation != NULL)
		{
			assert(interval->GetIntervalNumber() == intervalBestEndDiscretisation->GetIntervalNumber());

			// Incrementation du nombre d'intervalles
			interval->SetIntervalNumber(interval->GetIntervalNumber() + 1);
			assert(interval->GetIntervalNumber() == nInitialIntervalNumber + 1);

			// Derniere ligne du premier intervalle
			interval->SetLastLineIndexAt(0, intervalBestEndDiscretisation->GetIndex() - 1);

			// Recopie des bornes des autres intervalles
			for (i = 0; i < intervalBestEndDiscretisation->GetIntervalNumber(); i++)
				interval->SetLastLineIndexAt(i + 1,
							     intervalBestEndDiscretisation->GetLastLineIndexAt(i));

			// Cout de la meilleure discretisation
			interval->SetDiscretizationCost(dBestDiscretizationCost);
		}

		// Passage a l'intervalle suivant
		interval = cast(KWMODLLineOptimalDiscretization*, interval->GetNext());
	}

	// On regarde si un intervalle supplementaire a ete genere
	assert(nInitialIntervalNumber <= headInterval->GetIntervalNumber() and
	       headInterval->GetIntervalNumber() <= nInitialIntervalNumber + 1);
	bNewInterval = nInitialIntervalNumber < headInterval->GetIntervalNumber();

	// Nettoyage
	delete kwfvFirstIntervalFrequencyVector;

	return bNewInterval;
}

///////////////////////////////////////////////////////////////////////////////////////
// Methodes diverses

void KWDiscretizerMODL::ComputeBestSplit(const KWFrequencyTable* kwftSource, int nFirstIndex, int nLastIndex,
					 double dInitialCost, KWMODLLineSplit* lineSplit) const
{
	KWFrequencyVector* kwfvFirstSubLineFrequencyVector;
	KWFrequencyVector* kwfvSecondSubLineFrequencyVector;
	double dCost1;
	double dCost2;
	double dTotalCost;
	double dPreviousCost1;
	double dPreviousCost2;
	double dPreviousTotalCost;
	double dBestTotalCost;
	double dDeltaCost;
	int nIndex;
	boolean bSavePreviousSolution;
	boolean bSaveCurrentSolution;
	boolean bBetterTotalCost;
	boolean bPreviousSolutionOptimal;
	debug(int nTotalFrequency);

	require(kwftSource != NULL);
	require(0 <= nFirstIndex);
	require(nFirstIndex <= nLastIndex);
	require(nLastIndex < kwftSource->GetFrequencyVectorNumber());
	require(lineSplit != NULL);
	require(CheckFrequencyVector(lineSplit->GetFirstSubLineFrequencyVector()));
	require(lineSplit->GetFirstSubLineFrequencyVector()->ComputeTotalFrequency() > 0);
	require(kwftSource->ComputePartialFrequency(nFirstIndex, nLastIndex + 1) ==
		lineSplit->GetFirstSubLineFrequencyVector()->ComputeTotalFrequency());

	// Creation des vecteurs d'effectif de travail
	kwfvFirstSubLineFrequencyVector = GetFrequencyVectorCreator()->Create();
	kwfvSecondSubLineFrequencyVector = GetFrequencyVectorCreator()->Create();

	// Initialisation avec un cout infini (signifie pas de solution)
	lineSplit->SetDeltaCost(dInfiniteCost);
	lineSplit->SetFirstSubLineIndex(nLastIndex);
	lineSplit->SetFirstSubLineCost(ComputeIntervalCost(lineSplit->GetFirstSubLineFrequencyVector()));
	lineSplit->SetSecondSubLineCost(0);
	lineSplit->SetDeltaCost(dInfiniteCost);
	debug(nTotalFrequency = lineSplit->GetFirstSubLineFrequencyVector()->ComputeTotalFrequency());

	// On cherche a ameliorer la coupure s'il existe au moins un point de coupure potentiel
	if (nFirstIndex < nLastIndex)
	{
		// Initialisation de la taille du vecteur des frequences cible des sous-intervalles
		InitializeFrequencyVector(kwfvFirstSubLineFrequencyVector);
		InitializeFrequencyVector(kwfvSecondSubLineFrequencyVector);

		// Initialisation du premier sous-intervalle
		kwfvFirstSubLineFrequencyVector->CopyFrom(lineSplit->GetFirstSubLineFrequencyVector());
		debug(assert(kwfvFirstSubLineFrequencyVector->ComputeTotalFrequency() +
				 kwfvSecondSubLineFrequencyVector->ComputeTotalFrequency() ==
			     nTotalFrequency));

		// On evalue toutes les coupures potentielles en remplissant progressivement le second
		// sous-intervalle (en laissant au moins une ligne dans le premier-sous intervalle)
		dBestTotalCost = dInfiniteCost;
		dPreviousTotalCost = dBestTotalCost;
		dPreviousCost1 = dPreviousTotalCost;
		dPreviousCost2 = 0;
		bPreviousSolutionOptimal = false;
		for (nIndex = nLastIndex; nIndex > nFirstIndex; nIndex--)
		{
			// Transfert d'une ligne du tableau d'effectifs initial du premier intervalle
			// vers le second
			RemoveFrequencyVector(kwfvFirstSubLineFrequencyVector,
					      kwftSource->GetFrequencyVectorAt(nIndex));
			AddFrequencyVector(kwfvSecondSubLineFrequencyVector, kwftSource->GetFrequencyVectorAt(nIndex));
			assert(CheckFrequencyVector(kwfvFirstSubLineFrequencyVector));
			assert(CheckFrequencyVector(kwfvSecondSubLineFrequencyVector));
			debug(assert(kwfvFirstSubLineFrequencyVector->ComputeTotalFrequency() +
					 kwfvSecondSubLineFrequencyVector->ComputeTotalFrequency() ==
				     nTotalFrequency));

			// Evaluation du Split uniquement si la contrainte d'effectif minimum est respectee
			if (GetMinIntervalFrequency() == 0 or
			    (kwfvFirstSubLineFrequencyVector->ComputeTotalFrequency() >= GetMinIntervalFrequency() and
			     kwfvSecondSubLineFrequencyVector->ComputeTotalFrequency() >= GetMinIntervalFrequency()))
			{
				// Evaluation des couts de chaque ligne
				dCost1 = ComputeIntervalCost(kwfvFirstSubLineFrequencyVector);
				dCost2 = ComputeIntervalCost(kwfvSecondSubLineFrequencyVector);

				// Evaluation du cout total, et memorisation de la meilleure solution
				dTotalCost = dCost1 + dCost2;

				// Memorisation de la solution
				// Optimisation des memorisations effective: ce n'est pas fait a chaque amelioration
				bSaveCurrentSolution = false;
				bSavePreviousSolution = false;

				// La nouvelle solution est elle significativement meilleure
				bBetterTotalCost = dTotalCost < dBestTotalCost * (1 - dEpsilon);

				// Memorisation de la solution courante si amelioration et pas de prochain essai
				// (potentiel) (pas optimise si contrainte d'effectif minimum)
				if (bBetterTotalCost and (GetMinIntervalFrequency() != 0 or nIndex == nFirstIndex + 1))
					bSaveCurrentSolution = true;

				// Memorisation de la solution precedente si celle-ci etait optimale et si la nouvelle
				// solution est elle moins performante
				if (not bBetterTotalCost and bPreviousSolutionOptimal and nIndex < nLastIndex and
				    not bSaveCurrentSolution)
					bSavePreviousSolution = true;
				assert(not(bSaveCurrentSolution and bSavePreviousSolution));

				// Sauvegarde de la version courante
				if (bSaveCurrentSolution)
				{
					lineSplit->SetFirstSubLineIndex(nIndex - 1);
					lineSplit->GetFirstSubLineFrequencyVector()->CopyFrom(
					    kwfvFirstSubLineFrequencyVector);
					lineSplit->SetFirstSubLineCost(dCost1);
					lineSplit->SetSecondSubLineCost(dCost2);
					dDeltaCost = dTotalCost - dInitialCost;
					// Seuillage des variations de cout inferieur a dEpsilon
					if (fabs(dDeltaCost) <
					    dEpsilon) // error: call of overloaded 'abs(double&)' is ambiguous
						lineSplit->SetDeltaCost(0.0);
					else
						lineSplit->SetDeltaCost(dDeltaCost);
					assert(kwftSource->ComputePartialFrequency(
						   nFirstIndex, lineSplit->GetFirstSubLineIndex() + 1) ==
					       lineSplit->GetFirstSubLineFrequencyVector()->ComputeTotalFrequency());
				}
				// Sauvegarde de la version precedente (base sur nIndex+1: il faut la reconstruire)
				else if (bSavePreviousSolution)
				{
					assert(nIndex > nFirstIndex);
					lineSplit->SetFirstSubLineIndex(nIndex);
					lineSplit->GetFirstSubLineFrequencyVector()->CopyFrom(
					    kwfvFirstSubLineFrequencyVector);
					AddFrequencyVector(lineSplit->GetFirstSubLineFrequencyVector(),
							   kwftSource->GetFrequencyVectorAt(nIndex));
					lineSplit->SetFirstSubLineCost(dPreviousCost1);
					lineSplit->SetSecondSubLineCost(dPreviousCost2);
					dDeltaCost = dPreviousTotalCost - dInitialCost;
					// Seuillage des variations de cout inferieur a dEpsilon
					if (fabs(dDeltaCost) <
					    dEpsilon) // error: call of overloaded 'abs(double&)' is ambiguous
						lineSplit->SetDeltaCost(0.0);
					else
						lineSplit->SetDeltaCost(dDeltaCost);
					assert(kwftSource->ComputePartialFrequency(
						   nFirstIndex, lineSplit->GetFirstSubLineIndex() + 1) ==
					       lineSplit->GetFirstSubLineFrequencyVector()->ComputeTotalFrequency());
				}

				bPreviousSolutionOptimal = false;
				// Memorisation du meilleur score
				if (bBetterTotalCost)
				{
					dBestTotalCost = dTotalCost;
					bPreviousSolutionOptimal = true;
				}

				// Memorisation des couts precedents
				dPreviousCost1 = dCost1;
				dPreviousCost2 = dCost2;
				dPreviousTotalCost = dTotalCost;
			}
		}
	}

	// Nettoyage
	delete kwfvFirstSubLineFrequencyVector;
	delete kwfvSecondSubLineFrequencyVector;
	ensure(kwftSource->ComputePartialFrequency(nFirstIndex, lineSplit->GetFirstSubLineIndex() + 1) ==
	       lineSplit->GetFirstSubLineFrequencyVector()->ComputeTotalFrequency());
}

void KWDiscretizerMODL::InitializeWorkingData(const KWFrequencyTable* kwftSource) const
{
	boolean bDisplayResults = false;
	int nPartileNumber;

	require(kwftSource != NULL);

	// Parametrage de la structure de cout
	discretizationCosts->SetGranularity(kwftSource->GetGranularity());
	discretizationCosts->SetTotalInstanceNumber(kwftSource->GetTotalFrequency());

	nPartileNumber = (int)pow(2, kwftSource->GetGranularity());
	if (nPartileNumber > discretizationCosts->GetTotalInstanceNumber() or kwftSource->GetGranularity() == 0)
		nPartileNumber = discretizationCosts->GetTotalInstanceNumber();
	discretizationCosts->SetValueNumber(nPartileNumber);

	discretizationCosts->SetClassValueNumber(0);
	if (kwftSource->GetFrequencyVectorNumber() > 0)
		discretizationCosts->SetClassValueNumber(
		    cast(KWDenseFrequencyVector*, kwftSource->GetFrequencyVectorAt(0))
			->GetFrequencyVector()
			->GetSize());

	// Affichage des parametres du discretiseur
	if (bDisplayResults)
		cout << "KWDiscretizerMODL::InitializeWorkingData\tGranularity\t"
		     << discretizationCosts->GetGranularity() << "\tValueNumber\t"
		     << discretizationCosts->GetValueNumber() << endl;
}

void KWDiscretizerMODL::CleanWorkingData() const
{
	// Parametrage de la structure de cout
	discretizationCosts->SetValueNumber(0);
	discretizationCosts->SetClassValueNumber(0);
	discretizationCosts->SetGranularity(0);
	discretizationCosts->SetTotalInstanceNumber(0);
}

void KWDiscretizerMODL::InitializeFrequencyVector(KWFrequencyVector* kwfvFrequencyVector) const
{
	IntVector* ivFrequencyVector;

	require(kwfvFrequencyVector != NULL);

	// Retaillage avec le bon nombre de classes
	ivFrequencyVector = cast(KWDenseFrequencyVector*, kwfvFrequencyVector)->GetFrequencyVector();
	ivFrequencyVector->SetSize(discretizationCosts->GetClassValueNumber());
}

boolean KWDiscretizerMODL::CheckFrequencyVector(const KWFrequencyVector* kwfvFrequencyVector) const
{
	boolean bOk;
	IntVector* ivFrequencyVector;
	int i;

	require(kwfvFrequencyVector != NULL);

	// Controle de la taille du vecteur
	ivFrequencyVector = cast(KWDenseFrequencyVector*, kwfvFrequencyVector)->GetFrequencyVector();
	bOk = ivFrequencyVector->GetSize() == discretizationCosts->GetClassValueNumber();

	// Controle du contenu: effectifs positifs
	if (bOk)
	{
		for (i = 0; i < ivFrequencyVector->GetSize(); i++)
		{
			if (ivFrequencyVector->GetAt(i) < 0)
			{
				bOk = false;
				break;
			}
		}
	}

	return bOk;
}

void KWDiscretizerMODL::AddFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
					   const KWFrequencyVector* kwfvAddedFrequencyVector) const
{
	IntVector* ivSourceFrequencyVector;
	IntVector* ivAddedFrequencyVector;
	int i;

	require(kwfvSourceFrequencyVector != NULL);
	require(kwfvAddedFrequencyVector != NULL);
	require(CheckFrequencyVector(kwfvSourceFrequencyVector));
	require(CheckFrequencyVector(kwfvAddedFrequencyVector));

	// Acces aux representations denses des vecteurs d'effectifs
	ivSourceFrequencyVector = cast(KWDenseFrequencyVector*, kwfvSourceFrequencyVector)->GetFrequencyVector();
	ivAddedFrequencyVector = cast(KWDenseFrequencyVector*, kwfvAddedFrequencyVector)->GetFrequencyVector();
	assert(ivSourceFrequencyVector->GetSize() == ivAddedFrequencyVector->GetSize());

	// Ajout des effectifs sources
	for (i = 0; i < ivSourceFrequencyVector->GetSize(); i++)
		ivSourceFrequencyVector->UpgradeAt(i, ivAddedFrequencyVector->GetAt(i));

	// Mise a jour du nombre de modalites de la ligne
	kwfvSourceFrequencyVector->SetModalityNumber(kwfvSourceFrequencyVector->GetModalityNumber() +
						     kwfvAddedFrequencyVector->GetModalityNumber());

	ensure(CheckFrequencyVector(kwfvSourceFrequencyVector));
}

void KWDiscretizerMODL::RemoveFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
					      const KWFrequencyVector* kwfvRemovedFrequencyVector) const
{
	IntVector* ivSourceFrequencyVector;
	IntVector* ivRemovedFrequencyVector;
	int i;

	require(kwfvSourceFrequencyVector != NULL);
	require(kwfvRemovedFrequencyVector != NULL);
	require(CheckFrequencyVector(kwfvSourceFrequencyVector));
	require(CheckFrequencyVector(kwfvRemovedFrequencyVector));

	// Acces aux representations denses des vecteurs d'effectifs
	ivSourceFrequencyVector = cast(KWDenseFrequencyVector*, kwfvSourceFrequencyVector)->GetFrequencyVector();
	ivRemovedFrequencyVector = cast(KWDenseFrequencyVector*, kwfvRemovedFrequencyVector)->GetFrequencyVector();
	assert(ivSourceFrequencyVector->GetSize() == ivRemovedFrequencyVector->GetSize());

	// Supression des effectifs sources
	for (i = 0; i < ivSourceFrequencyVector->GetSize(); i++)
		ivSourceFrequencyVector->UpgradeAt(i, -ivRemovedFrequencyVector->GetAt(i));

	// Mise a jour du nombre de modalites
	kwfvSourceFrequencyVector->SetModalityNumber(kwfvSourceFrequencyVector->GetModalityNumber() -
						     kwfvRemovedFrequencyVector->GetModalityNumber());

	ensure(CheckFrequencyVector(kwfvSourceFrequencyVector));
}

void KWDiscretizerMODL::MergeTwoFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector,
						 const KWFrequencyVector* kwfvMergedFrequencyVector1,
						 const KWFrequencyVector* kwfvMergedFrequencyVector2) const
{
	IntVector* ivSourceFrequencyVector;
	IntVector* ivMergedFrequencyVector1;
	IntVector* ivMergedFrequencyVector2;
	int i;

	require(kwfvSourceFrequencyVector != NULL);
	require(kwfvMergedFrequencyVector1 != NULL);
	require(kwfvMergedFrequencyVector2 != NULL);
	require(CheckFrequencyVector(kwfvSourceFrequencyVector));
	require(CheckFrequencyVector(kwfvMergedFrequencyVector1));
	require(CheckFrequencyVector(kwfvMergedFrequencyVector2));

	// Acces aux representations denses des vecteurs d'effectifs
	ivSourceFrequencyVector = cast(KWDenseFrequencyVector*, kwfvSourceFrequencyVector)->GetFrequencyVector();
	ivMergedFrequencyVector1 = cast(KWDenseFrequencyVector*, kwfvMergedFrequencyVector1)->GetFrequencyVector();
	ivMergedFrequencyVector2 = cast(KWDenseFrequencyVector*, kwfvMergedFrequencyVector2)->GetFrequencyVector();

	// Transfert des effectifs
	for (i = 0; i < ivSourceFrequencyVector->GetSize(); i++)
	{
		ivSourceFrequencyVector->SetAt(i,
					       ivMergedFrequencyVector1->GetAt(i) + ivMergedFrequencyVector2->GetAt(i));
	}

	// Mise a jour du nombre de modalites du Merge
	kwfvSourceFrequencyVector->SetModalityNumber(kwfvMergedFrequencyVector1->GetModalityNumber() +
						     kwfvMergedFrequencyVector2->GetModalityNumber());

	ensure(CheckFrequencyVector(kwfvSourceFrequencyVector));
}

void KWDiscretizerMODL::MergeThreeFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector,
						   const KWFrequencyVector* kwfvMergedFrequencyVector1,
						   const KWFrequencyVector* kwfvMergedFrequencyVector2,
						   const KWFrequencyVector* kwfvMergedFrequencyVector3) const
{
	IntVector* ivSourceFrequencyVector;
	IntVector* ivMergedFrequencyVector1;
	IntVector* ivMergedFrequencyVector2;
	IntVector* ivMergedFrequencyVector3;
	int i;

	require(kwfvSourceFrequencyVector != NULL);
	require(kwfvMergedFrequencyVector1 != NULL);
	require(kwfvMergedFrequencyVector2 != NULL);
	require(kwfvMergedFrequencyVector3 != NULL);
	require(CheckFrequencyVector(kwfvSourceFrequencyVector));
	require(CheckFrequencyVector(kwfvMergedFrequencyVector1));
	require(CheckFrequencyVector(kwfvMergedFrequencyVector2));
	require(CheckFrequencyVector(kwfvMergedFrequencyVector3));

	// Acces aux representations denses des vecteurs d'effectifs
	ivSourceFrequencyVector = cast(KWDenseFrequencyVector*, kwfvSourceFrequencyVector)->GetFrequencyVector();
	ivMergedFrequencyVector1 = cast(KWDenseFrequencyVector*, kwfvMergedFrequencyVector1)->GetFrequencyVector();
	ivMergedFrequencyVector2 = cast(KWDenseFrequencyVector*, kwfvMergedFrequencyVector2)->GetFrequencyVector();
	ivMergedFrequencyVector3 = cast(KWDenseFrequencyVector*, kwfvMergedFrequencyVector3)->GetFrequencyVector();

	// Transfert des effectifs
	for (i = 0; i < ivSourceFrequencyVector->GetSize(); i++)
	{
		ivSourceFrequencyVector->SetAt(i, ivMergedFrequencyVector1->GetAt(i) +
						      ivMergedFrequencyVector2->GetAt(i) +
						      ivMergedFrequencyVector3->GetAt(i));
	}

	kwfvSourceFrequencyVector->SetModalityNumber(kwfvMergedFrequencyVector1->GetModalityNumber() +
						     kwfvMergedFrequencyVector2->GetModalityNumber() +
						     kwfvMergedFrequencyVector3->GetModalityNumber());

	ensure(CheckFrequencyVector(kwfvSourceFrequencyVector));
}

void KWDiscretizerMODL::SplitFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
					     KWFrequencyVector* kwfvNewFrequencyVector,
					     const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const
{
	IntVector* ivSourceFrequencyVector;
	IntVector* ivNewFrequencyVector;
	IntVector* ivFirstSubFrequencyVectorSpec;
	int i;
	int nFrequency;

	require(kwfvSourceFrequencyVector != NULL);
	require(kwfvNewFrequencyVector != NULL);
	require(kwfvFirstSubFrequencyVectorSpec != NULL);
	require(CheckFrequencyVector(kwfvSourceFrequencyVector));
	require(CheckFrequencyVector(kwfvNewFrequencyVector));
	require(CheckFrequencyVector(kwfvFirstSubFrequencyVectorSpec));

	// Acces aux representations denses des vecteurs d'effectifs
	ivSourceFrequencyVector = cast(KWDenseFrequencyVector*, kwfvSourceFrequencyVector)->GetFrequencyVector();
	ivNewFrequencyVector = cast(KWDenseFrequencyVector*, kwfvNewFrequencyVector)->GetFrequencyVector();
	ivFirstSubFrequencyVectorSpec =
	    cast(KWDenseFrequencyVector*, kwfvFirstSubFrequencyVectorSpec)->GetFrequencyVector();

	// Transfert des effectifs
	for (i = 0; i < ivSourceFrequencyVector->GetSize(); i++)
	{
		nFrequency = ivFirstSubFrequencyVectorSpec->GetAt(i);
		ivNewFrequencyVector->SetAt(i, ivSourceFrequencyVector->GetAt(i) - nFrequency);
		ivSourceFrequencyVector->SetAt(i, nFrequency);
	}

	// Nombre de modalites du nouveau vecteur par soustraction
	kwfvNewFrequencyVector->SetModalityNumber(kwfvSourceFrequencyVector->GetModalityNumber() -
						  kwfvFirstSubFrequencyVectorSpec->GetModalityNumber());
	kwfvSourceFrequencyVector->SetModalityNumber(kwfvFirstSubFrequencyVectorSpec->GetModalityNumber());

	ensure(CheckFrequencyVector(kwfvSourceFrequencyVector));
	ensure(CheckFrequencyVector(kwfvNewFrequencyVector));
}

void KWDiscretizerMODL::MergeSplitFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector1,
						   KWFrequencyVector* kwfvSourceFrequencyVector2,
						   const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const
{
	IntVector* ivSourceFrequencyVector1;
	IntVector* ivSourceFrequencyVector2;
	IntVector* ivFirstSubFrequencyVectorSpec;
	int i;
	int nFrequency;

	require(kwfvSourceFrequencyVector1 != NULL);
	require(kwfvSourceFrequencyVector2 != NULL);
	require(kwfvFirstSubFrequencyVectorSpec != NULL);
	require(CheckFrequencyVector(kwfvSourceFrequencyVector1));
	require(CheckFrequencyVector(kwfvSourceFrequencyVector2));
	require(CheckFrequencyVector(kwfvFirstSubFrequencyVectorSpec));

	// Acces aux representations denses des vecteurs d'effectifs
	ivSourceFrequencyVector1 = cast(KWDenseFrequencyVector*, kwfvSourceFrequencyVector1)->GetFrequencyVector();
	ivSourceFrequencyVector2 = cast(KWDenseFrequencyVector*, kwfvSourceFrequencyVector2)->GetFrequencyVector();
	ivFirstSubFrequencyVectorSpec =
	    cast(KWDenseFrequencyVector*, kwfvFirstSubFrequencyVectorSpec)->GetFrequencyVector();

	// Transfert des effectifs
	for (i = 0; i < ivSourceFrequencyVector1->GetSize(); i++)
	{
		nFrequency = ivFirstSubFrequencyVectorSpec->GetAt(i);
		ivSourceFrequencyVector2->UpgradeAt(i, ivSourceFrequencyVector1->GetAt(i) - nFrequency);
		ivSourceFrequencyVector1->SetAt(i, nFrequency);
	}

	// Mise a jour du nombre de modalites
	kwfvSourceFrequencyVector2->SetModalityNumber(kwfvSourceFrequencyVector1->GetModalityNumber() +
						      kwfvSourceFrequencyVector2->GetModalityNumber() -
						      kwfvFirstSubFrequencyVectorSpec->GetModalityNumber());
	kwfvSourceFrequencyVector1->SetModalityNumber(kwfvFirstSubFrequencyVectorSpec->GetModalityNumber());

	ensure(CheckFrequencyVector(kwfvSourceFrequencyVector1));
	ensure(CheckFrequencyVector(kwfvSourceFrequencyVector2));
}

void KWDiscretizerMODL::MergeMergeSplitFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector1,
							const KWFrequencyVector* kwfvSourceFrequencyVector2,
							KWFrequencyVector* kwfvSourceFrequencyVector3,
							const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const
{
	IntVector* ivSourceFrequencyVector1;
	IntVector* ivSourceFrequencyVector2;
	IntVector* ivSourceFrequencyVector3;
	IntVector* ivFirstSubFrequencyVectorSpec;
	int i;
	int nFrequency;

	require(kwfvSourceFrequencyVector1 != NULL);
	require(kwfvSourceFrequencyVector2 != NULL);
	require(kwfvSourceFrequencyVector3 != NULL);
	require(kwfvFirstSubFrequencyVectorSpec != NULL);
	require(CheckFrequencyVector(kwfvSourceFrequencyVector1));
	require(CheckFrequencyVector(kwfvSourceFrequencyVector2));
	require(CheckFrequencyVector(kwfvSourceFrequencyVector3));
	require(CheckFrequencyVector(kwfvFirstSubFrequencyVectorSpec));

	// Acces aux representations denses des vecteurs d'effectifs
	ivSourceFrequencyVector1 = cast(KWDenseFrequencyVector*, kwfvSourceFrequencyVector1)->GetFrequencyVector();
	ivSourceFrequencyVector2 = cast(KWDenseFrequencyVector*, kwfvSourceFrequencyVector2)->GetFrequencyVector();
	ivSourceFrequencyVector3 = cast(KWDenseFrequencyVector*, kwfvSourceFrequencyVector3)->GetFrequencyVector();
	ivFirstSubFrequencyVectorSpec =
	    cast(KWDenseFrequencyVector*, kwfvFirstSubFrequencyVectorSpec)->GetFrequencyVector();

	// Transfert des effectifs
	for (i = 0; i < ivSourceFrequencyVector1->GetSize(); i++)
	{
		nFrequency = ivFirstSubFrequencyVectorSpec->GetAt(i);
		ivSourceFrequencyVector3->UpgradeAt(i, ivSourceFrequencyVector1->GetAt(i) +
							   ivSourceFrequencyVector2->GetAt(i) - nFrequency);
		ivSourceFrequencyVector1->SetAt(i, nFrequency);
	}

	kwfvSourceFrequencyVector3->SetModalityNumber(
	    kwfvSourceFrequencyVector1->GetModalityNumber() + kwfvSourceFrequencyVector2->GetModalityNumber() +
	    kwfvSourceFrequencyVector3->GetModalityNumber() - kwfvFirstSubFrequencyVectorSpec->GetModalityNumber());
	kwfvSourceFrequencyVector1->SetModalityNumber(kwfvFirstSubFrequencyVectorSpec->GetModalityNumber());

	ensure(CheckFrequencyVector(kwfvSourceFrequencyVector1));
	ensure(CheckFrequencyVector(kwfvSourceFrequencyVector3));
}

int KWMODLLineDeepOptimizationCompareMergeSplitDeltaCost(const void* elem1, const void* elem2)
{
	KWMODLLineDeepOptimization* line1;
	KWMODLLineDeepOptimization* line2;

	line1 = cast(KWMODLLineDeepOptimization*, *(Object**)elem1);
	line2 = cast(KWMODLLineDeepOptimization*, *(Object**)elem2);

	// Comparaison de la variation de cout suite a une MergeSplit
	if (line1->GetMergeSplit()->GetDeltaCost() < line2->GetMergeSplit()->GetDeltaCost())
		return -1;
	else if (line1->GetMergeSplit()->GetDeltaCost() > line2->GetMergeSplit()->GetDeltaCost())
		return 1;
	else
		return 0;
}

int KWMODLLineDeepOptimizationCompareSplitDeltaCost(const void* elem1, const void* elem2)
{
	KWMODLLineDeepOptimization* line1;
	KWMODLLineDeepOptimization* line2;

	line1 = cast(KWMODLLineDeepOptimization*, *(Object**)elem1);
	line2 = cast(KWMODLLineDeepOptimization*, *(Object**)elem2);

	// Comparaison de la variation de cout suite a une MergeSplit
	if (line1->GetSplit()->GetDeltaCost() < line2->GetSplit()->GetDeltaCost())
		return -1;
	else if (line1->GetSplit()->GetDeltaCost() > line2->GetSplit()->GetDeltaCost())
		return 1;
	else
		return 0;
}

int KWMODLLineDeepOptimizationCompareMergeMergeSplitDeltaCost(const void* elem1, const void* elem2)
{
	KWMODLLineDeepOptimization* line1;
	KWMODLLineDeepOptimization* line2;

	line1 = cast(KWMODLLineDeepOptimization*, *(Object**)elem1);
	line2 = cast(KWMODLLineDeepOptimization*, *(Object**)elem2);

	// Comparaison de la variation de cout suite a une MergeSplit
	if (line1->GetMergeMergeSplit()->GetDeltaCost() < line2->GetMergeMergeSplit()->GetDeltaCost())
		return -1;
	else if (line1->GetMergeMergeSplit()->GetDeltaCost() > line2->GetMergeMergeSplit()->GetDeltaCost())
		return 1;
	else
		return 0;
}