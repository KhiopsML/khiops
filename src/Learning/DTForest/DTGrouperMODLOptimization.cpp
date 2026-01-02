// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTGrouperMODL.h"

////////////////////////////////////////////////////////////////
// Preprocessing

void DTGrouperMODL::MergePureSourceValues(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftPreprocessedSource,
					  IntVector*& ivGroups) const

{
	boolean bDisplayResults = false;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	int nSource;
	int nTarget;
	int nTargetNumber;
	int nFrequency;
	int nGroupTargetIndex;
	ObjectArray oaPureGroups;
	IntVector ivPureGroupIndexes;
	IntVector ivPureGroupFrequencies;
	IntVector ivPureGroupModalityNumbers;
	int nNewIndex;

	require(kwftSource != NULL);

	// Initialisation du vecteur d'index
	ivGroups = new IntVector;
	ivGroups->SetSize(kwftSource->GetFrequencyVectorNumber());

	// Recherche du nombre de classes cibles
	nTargetNumber = kwftSource->GetFrequencyVectorSize();

	// Initialisation du vecteur des index des groupes purs, representant les
	// positions de ces groupes dans le tableau des groupes preprocesses
	ivPureGroupIndexes.SetSize(nTargetNumber);
	for (nTarget = 0; nTarget < nTargetNumber; nTarget++)
		ivPureGroupIndexes.SetAt(nTarget, -1);

	// Initialisation des frequences des groupes purs
	ivPureGroupFrequencies.SetSize(nTargetNumber);

	// Initialisation du vecteur des nombres de modalites par groupe pur
	ivPureGroupModalityNumbers.SetSize(nTargetNumber);

	// Parcours des valeurs initiales
	nNewIndex = 0;
	nFrequency = 0;
	for (nSource = 0; nSource < kwftSource->GetFrequencyVectorNumber(); nSource++)
	{
		kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, kwftSource->GetFrequencyVectorAt(nSource));

		// Evaluation si la valeur source est pure
		nGroupTargetIndex = -1;
		for (nTarget = 0; nTarget < nTargetNumber; nTarget++)
		{
			// Ces tests permettent de savoir si une seule classe
			// cible est presente, et si oui de la memoriser
			nFrequency = kwdfvFrequencyVector->GetFrequencyVector()->GetAt(nTarget);
			if (nFrequency > 0)
			{
				// Valeur pure si effectif case egal a effectif ligne
				if (nFrequency == kwdfvFrequencyVector->ComputeTotalFrequency())
					nGroupTargetIndex = nTarget;

				// On peut arreter
				break;
			}
		}

		// Si valeur non pure, on calcule son nouvel index
		if (nGroupTargetIndex == -1)
		{
			ivGroups->SetAt(nSource, nNewIndex);
			nNewIndex++;
		}
		// Sinon, on memorise ses caracteristiques
		else
		{
			// Mise a jour de la frequence du groupe pur correspondant
			assert(nFrequency == kwdfvFrequencyVector->GetFrequencyVector()->GetAt(nGroupTargetIndex));
			ivPureGroupFrequencies.UpgradeAt(nGroupTargetIndex, nFrequency);

			// Mise a jour du nombre de modalites du groupe pur correspondant
			ivPureGroupModalityNumbers.UpgradeAt(nGroupTargetIndex,
							     kwdfvFrequencyVector->GetModalityNumber());

			// Cas ou c'est le premier groupe pur pour cette classe cible,
			if (ivPureGroupIndexes.GetAt(nGroupTargetIndex) == -1)
			{
				ivPureGroupIndexes.SetAt(nGroupTargetIndex, nNewIndex);
				ivGroups->SetAt(nSource, nNewIndex);
				nNewIndex++;
			}
			// Cas ou le groupe pur existe deja
			else
				ivGroups->SetAt(nSource, ivPureGroupIndexes.GetAt(nGroupTargetIndex));
		}
	}
	assert(nNewIndex <= kwftSource->GetFrequencyVectorNumber());

	// Creation de la table source preprocessee
	kwftPreprocessedSource = new KWFrequencyTable;
	kwftPreprocessedSource->SetFrequencyVectorCreator(GetFrequencyVectorCreator()->Clone());
	kwftPreprocessedSource->SetFrequencyVectorNumber(nNewIndex);
	kwftPreprocessedSource->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
	kwftPreprocessedSource->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
	// Parametrage granularite et poubelle
	kwftPreprocessedSource->SetGranularity(kwftSource->GetGranularity());
	kwftPreprocessedSource->SetGarbageModalityNumber(kwftSource->GetGarbageModalityNumber());

	// Alimentation de la table preprocesse par recopie de la table initiale,
	// pour toutes les valeurs initiales (pures et non pures)
	for (nSource = 0; nSource < kwftSource->GetFrequencyVectorNumber(); nSource++)
	{
		// Recherche de l'index du groupe preprocesse
		nNewIndex = ivGroups->GetAt(nSource);

		// Recopie de la ligne
		kwftPreprocessedSource->GetFrequencyVectorAt(nNewIndex)->CopyFrom(
		    kwftSource->GetFrequencyVectorAt(nSource));
	}

	// Reactualisation des effectifs pour les groupes purs
	for (nTarget = 0; nTarget < nTargetNumber; nTarget++)
	{
		// Index du groupe preprocesse correspondant au groupe pur
		nNewIndex = ivPureGroupIndexes.GetAt(nTarget);

		// Mise a jour de son effectif et du nombre de modalites si groupe existant
		if (nNewIndex != -1)
		{
			kwdfvFrequencyVector =
			    cast(KWDenseFrequencyVector*, kwftPreprocessedSource->GetFrequencyVectorAt(nNewIndex));
			InitializeFrequencyVector(kwdfvFrequencyVector);
			kwdfvFrequencyVector->GetFrequencyVector()->SetAt(nTarget,
									  ivPureGroupFrequencies.GetAt(nTarget));
			kwdfvFrequencyVector->SetModalityNumber(ivPureGroupModalityNumbers.GetAt(nTarget));
		}
	}

	// Affichage des resultats
	if (bDisplayResults)
	{
		cout << "Source Table\n" << *kwftSource << "\n";
		cout << "Group Indexes:";
		for (nSource = 0; nSource < ivGroups->GetSize(); nSource++)
			cout << "\t" << ivGroups->GetAt(nSource);
		cout << "\n";
		cout << "Preprocessed Source Table\n" << *kwftPreprocessedSource << endl;
		cout << "Modality Numbers per pure group\n" << ivPureGroupModalityNumbers << endl;
	}

	// Verification de l'effectif global de la table
	ensure(kwftSource->GetTotalFrequency() == kwftPreprocessedSource->GetTotalFrequency());
}

ObjectArray* DTGrouperMODL::BuildReliableSubGroups(KWFrequencyTable* kwftSource, IntVector*& ivGroups) const
{
	boolean bDisplayResults = false;
	boolean bDisplayBiClassGroupings = false;
	ObjectArray* oaGroups;
	KWMODLGroup* group;
	DTGrouperMODLTwoClasses grouperMODLTwoClasses;
	KWUnivariateNullPartitionCosts* grouperNullPartitionCost;
	KWFrequencyTable kwftBiClassSource;
	KWFrequencyTable* kwftBiClassGroups;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	KWDenseFrequencyVector* kwdfvBiClassFrequencyVector;
	IntVector* ivBiClassGroupIndexes;
	ObjectArray oaAllBiClassGroupIndexes;
	IntVector ivIndexMaxValues;
	IntVector ivIndexValues;
	int nSource;
	int nTarget;
	int nTargetNumber;
	ALString sPartialGroupKey;
	int nPartialGroupIndex;
	ObjectDictionary odPartialGroupIndexes;

	require(kwftSource != NULL);

	// Creation du vecteur d'index
	ivGroups = new IntVector;
	ivGroups->SetSize(kwftSource->GetFrequencyVectorNumber());

	// Creation du tableau des sous-groupes
	oaGroups = new ObjectArray;

	// Recherche du nombre de classes cibles
	nTargetNumber = kwftSource->GetFrequencyVectorSize();

	// Parametrage d'un cout de groupage sans cout de partition, de facon a favoriser la construction de groupe
	// initiaux,
	grouperNullPartitionCost = new KWUnivariateNullPartitionCosts;
	grouperNullPartitionCost->SetUnivariatePartitionCosts(GetGroupingCosts()->Clone());

	// On parametre le nombre de classes aux niveau standard et au niveau du Null partition cost,
	// car ce parametre est utilise dans des assert de la classe ancetre
	grouperNullPartitionCost->SetClassValueNumber(2);
	grouperNullPartitionCost->GetUnivariatePartitionCosts()->SetClassValueNumber(2);

	// Parametrage du groupeur bi-classes
	grouperMODLTwoClasses.SetMaxIntervalNumber(GetMaxGroupNumber());
	grouperMODLTwoClasses.SetDiscretizationCosts(grouperNullPartitionCost);

	// Groupages bi-classes pour toutes les modalites cibles
	oaAllBiClassGroupIndexes.SetSize(nTargetNumber);
	ivIndexMaxValues.SetSize(nTargetNumber);
	kwftBiClassSource.SetFrequencyVectorCreator(GetFrequencyVectorCreator()->Clone());
	kwftBiClassSource.SetFrequencyVectorNumber(kwftSource->GetFrequencyVectorNumber());
	kwftBiClassSource.SetInitialValueNumber(kwftSource->GetInitialValueNumber());
	kwftBiClassSource.SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
	// Parametrage granularite et poubelle
	kwftBiClassSource.SetGranularity(kwftSource->GetGranularity());
	kwftBiClassSource.SetGarbageModalityNumber(kwftSource->GetGarbageModalityNumber());

	for (nTarget = 0; nTarget < nTargetNumber; nTarget++)
	{
		// Initialisation d'une table de contingence source bi-classe en prenant
		// une classe en index 0 et toutes les autres en index 1
		for (nSource = 0; nSource < kwftSource->GetFrequencyVectorNumber(); nSource++)
		{
			// Initialisation du vecteur d'effectif bi-classe
			kwdfvBiClassFrequencyVector =
			    cast(KWDenseFrequencyVector*, kwftBiClassSource.GetFrequencyVectorAt(nSource));
			kwdfvBiClassFrequencyVector->GetFrequencyVector()->SetSize(2);

			// Alimentation du vecteur bi-classe
			kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, kwftSource->GetFrequencyVectorAt(nSource));
			kwdfvBiClassFrequencyVector->GetFrequencyVector()->SetAt(
			    0, kwdfvFrequencyVector->GetFrequencyVector()->GetAt(nTarget));
			kwdfvBiClassFrequencyVector->GetFrequencyVector()->SetAt(
			    1, kwdfvFrequencyVector->ComputeTotalFrequency() -
				   kwdfvFrequencyVector->GetFrequencyVector()->GetAt(nTarget));

			// Initialisation du nombre de modalites
			kwdfvBiClassFrequencyVector->SetModalityNumber(
			    kwftSource->GetFrequencyVectorAt(nSource)->GetModalityNumber());
		}
		assert(kwftBiClassSource.GetTotalFrequency() == kwftSource->GetTotalFrequency());

		// Groupage bi-classes de cette table
		grouperMODLTwoClasses.Group(&kwftBiClassSource, GetInitialValueNumber(), kwftBiClassGroups,
					    ivBiClassGroupIndexes);

		// Affichage des resultats
		if (bDisplayBiClassGroupings)
		{
			cout << "Bi-class source Table for target " << nTarget << "\n" << kwftBiClassSource << "\n";
			cout << "Group Indexes:";
			for (nSource = 0; nSource < ivBiClassGroupIndexes->GetSize(); nSource++)
				cout << "\t" << ivBiClassGroupIndexes->GetAt(nSource);
			cout << "\n";
			cout << "Bi-class groups\n" << *kwftBiClassGroups << endl;
		}

		// Memorisation des index des groupes
		oaAllBiClassGroupIndexes.SetAt(nTarget, ivBiClassGroupIndexes);
		ivIndexMaxValues.SetAt(nTarget, kwftBiClassGroups->GetFrequencyVectorNumber());

		// Nettoyage
		delete kwftBiClassGroups;
	}

	// Calcul des cles des vecteurs d'index de groupes partiels pour chaque valeur
	// source et creation des index de groupes partiels
	nPartialGroupIndex = 0;
	ivIndexValues.SetSize(nTargetNumber);
	for (nSource = 0; nSource < kwftSource->GetFrequencyVectorNumber(); nSource++)
	{
		// Initialisation du vecteur d'index
		for (nTarget = 0; nTarget < nTargetNumber; nTarget++)
		{
			ivBiClassGroupIndexes = cast(IntVector*, oaAllBiClassGroupIndexes.GetAt(nTarget));
			ivIndexValues.SetAt(nTarget, ivBiClassGroupIndexes->GetAt(nSource));
		}

		// Calcul de la cle du sous-groupe partiel
		sPartialGroupKey = BuildKeyFromIndexes(&ivIndexMaxValues, &ivIndexValues);

		// Creation si necessaire d'un nouveau groupe
		group = cast(KWMODLGroup*, odPartialGroupIndexes.Lookup(sPartialGroupKey));
		if (group == NULL)
		{
			// Creation du sous groupes
			group = new KWMODLGroup(GetFrequencyVectorCreator());
			InitializeFrequencyVector(group->GetFrequencyVector());
			group->SetIndex(nPartialGroupIndex);

			// Rangement dans la table des groupes resultata, et dans le dictionnaire
			// des sous-groupes partiels en cours d'identification
			oaGroups->Add(group);
			odPartialGroupIndexes.SetAt(sKey, group);

			// Passage a l'index suivant
			nPartialGroupIndex++;
			assert(odPartialGroupIndexes.GetCount() == nPartialGroupIndex);
			assert(oaGroups->GetSize() == odPartialGroupIndexes.GetCount());
		}

		// Transfert des effectif sources vers le groupe
		AddFrequencyVector(group->GetFrequencyVector(), kwftSource->GetFrequencyVectorAt(nSource));

		// Memorisation de l'index du sous-groupe partiel lie a la valeur source
		ivGroups->SetAt(nSource, group->GetIndex());
	}
	assert(odPartialGroupIndexes.GetCount() == nPartialGroupIndex);
	assert(oaGroups->GetSize() == odPartialGroupIndexes.GetCount());

	// Initialisation de l'evaluation des sous-groupes
	for (nPartialGroupIndex = 0; nPartialGroupIndex < oaGroups->GetSize(); nPartialGroupIndex++)
	{
		group = cast(KWMODLGroup*, oaGroups->GetAt(nPartialGroupIndex));
		group->SetCost(ComputeGroupCost(group->GetFrequencyVector()));
	}

	// Nettoyage
	oaAllBiClassGroupIndexes.DeleteAll();

	// Affichage des resultats
	if (bDisplayResults)
	{
		cout << "Source Table\n" << *kwftSource << endl;
		cout << "Group Indexes:";
		for (nSource = 0; nSource < ivGroups->GetSize(); nSource++)
			cout << "\t" << ivGroups->GetAt(nSource);
		cout << endl;
		cout << "Preprocessed Source Table\n";
		for (nPartialGroupIndex = 0; nPartialGroupIndex < oaGroups->GetSize(); nPartialGroupIndex++)
		{
			group = cast(KWMODLGroup*, oaGroups->GetAt(nPartialGroupIndex));
			cout << *group;
		}
		cout << endl;
	}

	return oaGroups;
}

const ALString& DTGrouperMODL::BuildKeyFromIndexes(IntVector* ivIndexMaxValues, IntVector* ivIndexValues) const
{
	const int nBase = 16;
	const int nMaxCharPerIndex = 7;
	int nIndexMaxValue;
	int i;
	int nOffset;
	int nMaxValueNumber;
	int nValue;
	int nCharOffset;

	require(ivIndexMaxValues != NULL);
	require(ivIndexValues != NULL);
	require(ivIndexValues->GetSize() == ivIndexMaxValues->GetSize());

	// Allocation d'une taille max pour la cle
	sKey.GetBufferSetLength(nMaxCharPerIndex * ivIndexValues->GetSize() + 1);

	// Boucle de codage des index dans la cle
	nOffset = 0;
	for (i = 0; i < ivIndexValues->GetSize(); i++)
	{
		// Valeur max de l'index
		nIndexMaxValue = ivIndexMaxValues->GetAt(i);
		assert(0 <= nIndexMaxValue);
		assert(nIndexMaxValue < pow(1.0 * nBase, 1.0 * nMaxCharPerIndex));

		// Codage de l'index en base 16 (nBase), avec autant de caracteres
		// que le nombre de caracteres potentiels max (codage de longueur fixe)
		nValue = ivIndexValues->GetAt(i);
		nMaxValueNumber = 1;
		while (nMaxValueNumber < nIndexMaxValue + 1)
		{
			nMaxValueNumber *= nBase;
			assert(nMaxValueNumber > 0);

			// Recherche du prochain caractere de la cle
			nCharOffset = nValue % nBase;
			sKey.SetAt(nOffset, char('A' + nCharOffset));
			nOffset++;
			assert(nOffset < nMaxCharPerIndex * ivIndexValues->GetSize());

			// Preparation pour le caractere suivant
			nValue /= nBase;
		}
	}

	// Retaillage de la cle
	sKey.ReleaseBuffer(nOffset);

	return sKey;
}

ObjectArray* DTGrouperMODL::BuildGroups(KWFrequencyTable* kwftTable) const
{
	ObjectArray* oaGroups;
	KWMODLGroup* group;
	int nSource;

	require(kwftTable != NULL);

	// Creation du tableau de groupe par recopie de la table de contingence
	oaGroups = new ObjectArray;
	oaGroups->SetSize(kwftTable->GetFrequencyVectorNumber());
	for (nSource = 0; nSource < kwftTable->GetFrequencyVectorNumber(); nSource++)
	{
		// Creation d'un groupe dans le tableau
		group = new KWMODLGroup(GetFrequencyVectorCreator());
		oaGroups->SetAt(nSource, group);

		// Initialisation de ce groupe
		group->GetFrequencyVector()->CopyFrom(kwftTable->GetFrequencyVectorAt(nSource));
		group->SetIndex(nSource);
		group->SetCost(ComputeGroupCost(group->GetFrequencyVector()));
	}
	return oaGroups;
}

KWFrequencyTable* DTGrouperMODL::BuildTable(ObjectArray* oaGroups) const
{
	KWFrequencyTable* kwftTable;
	KWMODLGroup* group;
	int nSource;

	require(oaGroups != NULL);

	// Creation de la table de contingence en se basant sur le nombre de classes
	// de son premier groupe
	kwftTable = new KWFrequencyTable;
	kwftTable->SetFrequencyVectorCreator(GetFrequencyVectorCreator()->Clone());
	if (oaGroups->GetSize() > 0)
		kwftTable->SetFrequencyVectorNumber(oaGroups->GetSize());

	// Initialisation de la table de contingence par recopie du tableau de groupes
	for (nSource = 0; nSource < kwftTable->GetFrequencyVectorNumber(); nSource++)
	{
		// Acces a un groupe du tableau
		group = cast(KWMODLGroup*, oaGroups->GetAt(nSource));

		// Initialisation de la ligne de la table correspondant a ce groupe
		kwftTable->GetFrequencyVectorAt(nSource)->CopyFrom(group->GetFrequencyVector());
	}
	return kwftTable;
}

ObjectArray* DTGrouperMODL::BuildGroupMerges(ObjectArray* oaGroups) const
{
	ObjectArray* oaGroupMerges;
	KWMODLGroup* group1;
	KWMODLGroup* group2;
	KWMODLGroupMerge* groupMerge;
	double dUnionCost;
	int i;
	int j;

	require(oaGroups != NULL);

	// Initialisation du tableau des fusions de groupes
	oaGroupMerges = new ObjectArray;
	oaGroupMerges->SetSize(oaGroups->GetSize() * oaGroups->GetSize());

	// On remplit la diagonale inferieure avec les fusions des groupes
	for (i = 0; i < oaGroups->GetSize(); i++)
	{
		// Acces au premier groupe de la fusion
		group1 = cast(KWMODLGroup*, oaGroups->GetAt(i));

		// Parcours des groupes pour evaluer les fusions possibles
		for (j = 0; j < i; j++)
		{
			// Acces au second groupe de la fusion
			group2 = cast(KWMODLGroup*, oaGroups->GetAt(j));

			// Creation d'une fusion dans le tableau
			groupMerge = new KWMODLGroupMerge;
			SetGroupMergeAt(oaGroupMerges, oaGroups->GetSize(), i, j, groupMerge);

			// Calcul du vecteur d'effectifs du groupe fusionne
			assert(CheckFrequencyVector(group1->GetFrequencyVector()));
			assert(CheckFrequencyVector(group2->GetFrequencyVector()));
			dUnionCost = ComputeGroupUnionCost(group1->GetFrequencyVector(), group2->GetFrequencyVector());

			// Initialisation de la fusion
			groupMerge->SetIndex1(j);
			groupMerge->SetIndex2(i);
			groupMerge->SetDeltaCost(dUnionCost - group1->GetCost() - group2->GetCost());
			assert(groupMerge->GetIndex1() < groupMerge->GetIndex2());
		}
	}

	return oaGroupMerges;
}

void DTGrouperMODL::MergeSmallGroups(ObjectArray* oaInitialGroups, int nMinFrequency, boolean bOneSingleGarbageGroup,
				     IntVector*& ivGroups) const
{
	boolean bDisplayResults = false;
	ObjectArray oaMergedGroups;
	IntVector ivMergedGroupIndexes;
	KWMODLGroup* mergedGroup;
	KWMODLGroup* group;
	KWDenseFrequencyVector* kwdfvGroupFrequencyVector;
	int i;
	int nMergedGroupIndex;
	int nTargetNumber;
	int nTarget;
	int nGroupFrequency;
	int nNewIndex;
	int nMaxFrequency;
	int nBestTarget;

	require(oaInitialGroups != NULL);

	// Affichage de la table initiale
	if (bDisplayResults)
	{
		cout << endl;
		for (i = 0; i < oaInitialGroups->GetSize(); i++)
		{
			group = cast(KWMODLGroup*, oaInitialGroups->GetAt(i));
			cout << *group << flush;
		}
	}

	// Initialisation du vecteur d'index
	ivGroups = new IntVector;
	ivGroups->SetSize(oaInitialGroups->GetSize());

	// Calcul du nombre de classe cibles
	nTargetNumber = 0;
	if (oaInitialGroups->GetSize() > 0)
	{
		group = cast(KWMODLGroup*, oaInitialGroups->GetAt(0));
		kwdfvGroupFrequencyVector = cast(KWDenseFrequencyVector*, group->GetFrequencyVector());
		nTargetNumber = kwdfvGroupFrequencyVector->GetFrequencyVector()->GetSize();
	}

	// Initialisation du tableau memorisant les groupes merges
	oaMergedGroups.SetSize(nTargetNumber);
	ivMergedGroupIndexes.SetSize(nTargetNumber);

	// Parcours des groupes initiaux pour merger en une passe les groupes
	// n'atteignant pas l'effectif minimum
	nNewIndex = 0;
	mergedGroup = NULL;
	nMergedGroupIndex = -1;
	for (i = 0; i < oaInitialGroups->GetSize(); i++)
	{
		group = cast(KWMODLGroup*, oaInitialGroups->GetAt(i));
		kwdfvGroupFrequencyVector = cast(KWDenseFrequencyVector*, group->GetFrequencyVector());

		// Calcul de la frequence du groupe
		nGroupFrequency = kwdfvGroupFrequencyVector->ComputeTotalFrequency();

		// Si effectif suffisant,  on bascule le groupe a son nouvel index
		if (nGroupFrequency >= nMinFrequency)
		{
			ivGroups->SetAt(i, nNewIndex);
			oaInitialGroups->SetAt(nNewIndex, group);
			group->SetIndex(nNewIndex);
			nNewIndex++;
		}
		// Sinon, on gere la fusion du groupe dans un groupe d'accueil dedie
		else
		{
			// Recherche de l'index de la classe cible majoritaire
			nBestTarget = -1;
			if (not bOneSingleGarbageGroup)
			{
				nMaxFrequency = -1;
				for (nTarget = 0; nTarget < nTargetNumber; nTarget++)
				{
					if (kwdfvGroupFrequencyVector->GetFrequencyVector()->GetAt(nTarget) >
					    nMaxFrequency)
					{
						nMaxFrequency =
						    kwdfvGroupFrequencyVector->GetFrequencyVector()->GetAt(nTarget);
						nBestTarget = nTarget;
					}
				}
			}
			// Cas ou on force un seul groupe pour les valeurs peu ferequentes
			else
				nBestTarget = 0;
			assert(nBestTarget != -1);

			// Recherche du groupe a fusionner
			mergedGroup = cast(KWMODLGroup*, oaMergedGroups.GetAt(nBestTarget));
			nMergedGroupIndex = ivMergedGroupIndexes.GetAt(nBestTarget);

			// Si c'est la premiere fois, on memorise le groupe de fusions des petits
			// groupes (en prenant le groupe lui-meme) pour cette classe majoritaire
			if (mergedGroup == NULL)
			{
				// Initialisation du groupe de fusion
				mergedGroup = group;
				nMergedGroupIndex = nNewIndex;
				oaMergedGroups.SetAt(nBestTarget, mergedGroup);
				ivMergedGroupIndexes.SetAt(nBestTarget, nMergedGroupIndex);

				// Memorisation du groupage
				ivGroups->SetAt(i, nNewIndex);
				oaInitialGroups->SetAt(nNewIndex, group);
				group->SetIndex(nNewIndex);
				nNewIndex++;
			}
			// Sinon, on le detruit apres avoir pris en compte ses caracteristiques
			else
			{
				ivGroups->SetAt(i, nMergedGroupIndex);
				AddFrequencyVector(mergedGroup->GetFrequencyVector(), group->GetFrequencyVector());
				delete group;
			}
		}
	}

	// Retaillage du tableau des groupes preprocesses
	oaInitialGroups->SetSize(nNewIndex);

	// On recalcule la valeur des groupes d'acceuil des fusions de petits groupes
	for (nTarget = 0; nTarget < nTargetNumber; nTarget++)
	{
		mergedGroup = cast(KWMODLGroup*, oaMergedGroups.GetAt(nTarget));
		if (mergedGroup != NULL)
			mergedGroup->SetCost(ComputeGroupCost(mergedGroup->GetFrequencyVector()));
	}

	// Affichage du vecteur d'index et de la table finale
	if (bDisplayResults)
	{
		for (i = 0; i < ivGroups->GetSize(); i++)
			cout << "\t" << ivGroups->GetAt(i);
		cout << endl;
		for (i = 0; i < oaInitialGroups->GetSize(); i++)
		{
			group = cast(KWMODLGroup*, oaInitialGroups->GetAt(i));
			cout << *group << flush;
		}
	}
}

int DTGrouperMODL::ComputeMinGroupFrequency(ObjectArray* oaInitialGroups, int nTotalFrequency,
					    int nOutputMaxGroupNumber) const
{
	int nOutputMinGroupFrequency;
	int nSupMinFrequency;
	IntVector ivGroupNumbers;
	KWMODLGroup* group;
	int i;
	int nGroupFrequency;
	int nGroupNumber;

	require(oaInitialGroups != NULL);
	require(nTotalFrequency >= 0);
	require(nOutputMaxGroupNumber >= 1);

	// Calcul de la borne sup de l'effectif minimal des groupes a fusionner
	nSupMinFrequency = (int)ceil(nTotalFrequency * 1.0 / nOutputMaxGroupNumber);

	// Calcul des nombres de groupes pour chaque frequence inferieur a cette borne sup
	ivGroupNumbers.SetSize(nSupMinFrequency + 1);
	for (i = 0; i < oaInitialGroups->GetSize(); i++)
	{
		group = cast(KWMODLGroup*, oaInitialGroups->GetAt(i));

		// Calcul de la frequence du groupe
		nGroupFrequency = group->GetFrequencyVector()->ComputeTotalFrequency();

		// Incrementation du nombre de groupes ayant cette frequence, si elle est
		// plus petite que la borne de frequence min
		if (nGroupFrequency <= nSupMinFrequency)
			ivGroupNumbers.UpgradeAt(nGroupFrequency, 1);
	}

	// Calcul de la taille min des groupes a fusionner pour obtenir moins de MaxGroupNumber
	// Les statistiques collectees permettent d'eviter le tri des frequence par frequence
	// et font ainsi passer l'algorithme de O(N.log(N)) a O(N)
	nOutputMinGroupFrequency = 1;
	nGroupNumber = oaInitialGroups->GetSize();
	for (nGroupFrequency = 1; nGroupFrequency <= nSupMinFrequency; nGroupFrequency++)
	{
		// On arrete si le nombre de groupe max est atteint
		if (nGroupNumber < nOutputMaxGroupNumber)
		{
			nOutputMinGroupFrequency = nGroupFrequency;
			break;
		}
		// Sinon, on decremente le nombre de groupes en cours
		else
			nGroupNumber -= ivGroupNumbers.GetAt(nGroupFrequency);
	}

	assert(nGroupNumber < nOutputMaxGroupNumber);
	return nOutputMinGroupFrequency;
}

void DTGrouperMODL::ComputeIndexesFromSourceToEndProcesses(IntVector*& ivSourceToPreprocessedIndexes,
							   IntVector*& ivPreprocessedToSubGroupsIndexes,
							   IntVector*& ivFewerInitialIndexes,
							   IntVector*& ivSourceToEndPreprocesses) const
{
	int i;
	boolean bMergePureGroups = (ivSourceToPreprocessedIndexes != NULL);
	boolean bSubGroups = (ivPreprocessedToSubGroupsIndexes != NULL);
	boolean bFewGroups = (ivFewerInitialIndexes != NULL);

	// Initialisation
	ivSourceToEndPreprocesses = NULL;
	// Cas ou les groupes purs n'ont pas ete fusionnes
	if (!bMergePureGroups)
	{
		// Cas ou il y a eu construction de sous-groupes puis reduction de leur nombre
		if (bSubGroups and bFewGroups)
		{
			ivSourceToEndPreprocesses = new IntVector;
			ivSourceToEndPreprocesses->SetSize(ivPreprocessedToSubGroupsIndexes->GetSize());
			for (i = 0; i < ivSourceToEndPreprocesses->GetSize(); i++)
				ivSourceToEndPreprocesses->SetAt(
				    i, ivFewerInitialIndexes->GetAt(ivPreprocessedToSubGroupsIndexes->GetAt(i)));
		}
		// Cas ou il y a eu construction de sous-groupes
		else if (bSubGroups and not bFewGroups)
		{
			ivSourceToEndPreprocesses = new IntVector;
			ivSourceToEndPreprocesses->SetSize(ivPreprocessedToSubGroupsIndexes->GetSize());
			for (i = 0; i < ivSourceToEndPreprocesses->GetSize(); i++)
				ivSourceToEndPreprocesses->SetAt(i, ivPreprocessedToSubGroupsIndexes->GetAt(
									ivPreprocessedToSubGroupsIndexes->GetAt(i)));
			assert(ivFewerInitialIndexes == NULL);
		}
		// Cas ou il y a eu reduction du nombre de groupes
		// C'est LE cas en mode poubelle
		else if (not bSubGroups and bFewGroups)
		{
			ivSourceToEndPreprocesses = new IntVector;
			ivSourceToEndPreprocesses->SetSize(ivFewerInitialIndexes->GetSize());
			for (i = 0; i < ivSourceToEndPreprocesses->GetSize(); i++)
				ivSourceToEndPreprocesses->SetAt(i, ivFewerInitialIndexes->GetAt(i));
			assert(ivPreprocessedToSubGroupsIndexes == NULL);
		}
	}

	// Cas ou il y a eu fusion des groupes purs
	else
	{
		// Cas ou il y a eu construction de sous-groupes puis reduction de leur nombre
		if (bSubGroups and bFewGroups)
		{
			ivSourceToEndPreprocesses = new IntVector;
			ivSourceToEndPreprocesses->SetSize(ivSourceToPreprocessedIndexes->GetSize());
			for (i = 0; i < ivSourceToEndPreprocesses->GetSize(); i++)
				ivSourceToEndPreprocesses->SetAt(
				    i, ivFewerInitialIndexes->GetAt(ivPreprocessedToSubGroupsIndexes->GetAt(
					   ivSourceToPreprocessedIndexes->GetAt(i))));
		}
		// Cas ou il y a eu construction de sous-groupes
		else if (bSubGroups and not bFewGroups)
		{
			ivSourceToEndPreprocesses = new IntVector;
			ivSourceToEndPreprocesses->SetSize(ivSourceToPreprocessedIndexes->GetSize());
			for (i = 0; i < ivSourceToEndPreprocesses->GetSize(); i++)
				ivSourceToEndPreprocesses->SetAt(i, ivPreprocessedToSubGroupsIndexes->GetAt(
									ivSourceToPreprocessedIndexes->GetAt(i)));
			assert(ivFewerInitialIndexes == NULL);
		}
		// Cas ou il y a eu reduction du nombre de groupes
		else if (not bSubGroups and bFewGroups)
		{
			ivSourceToEndPreprocesses = new IntVector;
			ivSourceToEndPreprocesses->SetSize(ivSourceToPreprocessedIndexes->GetSize());
			for (i = 0; i < ivSourceToEndPreprocesses->GetSize(); i++)
				ivSourceToEndPreprocesses->SetAt(
				    i, ivFewerInitialIndexes->GetAt(ivSourceToPreprocessedIndexes->GetAt(i)));
			assert(ivPreprocessedToSubGroupsIndexes == NULL);
		}
		// Cas ou il n'y a rien eu d'autre que la fusion des groupes purs
		else
		{
			ivSourceToEndPreprocesses = ivSourceToPreprocessedIndexes->Clone();
			assert(ivPreprocessedToSubGroupsIndexes == NULL);
			assert(ivFewerInitialIndexes == NULL);
		}
	}
}

/////////////////////////////////////////////////////////////////////////
// PostOptimisation

void DTGrouperMODL::PostOptimizeGrouping(KWFrequencyTable* kwftSource, KWFrequencyTable* kwftTarget,
					 IntVector* ivGroups) const
{
	boolean bPrintGroupNumbers = false;
	const int nMaxTestedForcedMergeNumber = 3;
	int nTestedForcedMergeNumber;
	boolean bSingletonGroupingTested;
	KWFrequencyTable* kwftExhaustiveMergeTarget;
	IntVector* ivExhaustiveMergeGroups;
	IntVector ivNewGroups;
	KWFrequencyTable* kwftCurrentTarget;
	KWFrequencyTable* kwftNewTarget;
	double dBestCost;
	double dNewCost;
	int nSource;

	require(kwftSource != NULL);
	require(kwftTarget != NULL);
	require(ivGroups != NULL);
	require(kwftSource->GetFrequencyVectorSize() == kwftTarget->GetFrequencyVectorSize());
	require(kwftSource->GetFrequencyVectorNumber() >= kwftTarget->GetFrequencyVectorNumber());
	require(ivGroups->GetSize() == kwftSource->GetFrequencyVectorNumber());
	require(kwftSource->GetFrequencyVectorNumber() < KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage() or
		GetMaxGroupNumber() == 2 or GetMaxGroupNumber() == 1);

	// Arret si un seul groupe
	if (kwftTarget->GetFrequencyVectorNumber() <= 1)
		return;

	// Statistiques initiales et nombre de groupes initiaux
	if (bPrintGroupNumbers)
		cout << kwftSource->GetTotalFrequency() << "\t" << kwftSource->GetFrequencyVectorNumber() << "\t"
		     << kwftTarget->GetFrequencyVectorNumber() << "\t";

	// Algorithme ExhaustiveMerge
	// S'il reste beaucoup de groupes, on realise dans une premiere etape
	// un algorithme ExhaustiveMerge en O(n.log(n)) afin de reduire
	// rapidement le nombre de groupes
	// On se limite a MinGroupNumber=nMaxTestedForcedMergeNumber+2, car
	// l'etape suivante ira potentiellement jusqu'a MinGroupNumber=2 en
	// nMaxTestedForcedMergeNumber iteration, et l'etape finale teste
	// les groupes singletons
	if (kwftTarget->GetFrequencyVectorNumber() > nMaxTestedForcedMergeNumber + 2)
	{
		// Optimisation de la table
		ExhaustiveMergeGrouping(nMaxTestedForcedMergeNumber + 2, kwftTarget, kwftExhaustiveMergeTarget,
					ivExhaustiveMergeGroups);

		// La nouvelle table cible est recopiee dans la precedente
		kwftTarget->CopyFrom(kwftExhaustiveMergeTarget);
		delete kwftExhaustiveMergeTarget;

		// Recalcul des index des groupes
		for (nSource = 0; nSource < ivGroups->GetSize(); nSource++)
			ivGroups->SetAt(nSource, ivExhaustiveMergeGroups->GetAt(ivGroups->GetAt(nSource)));
		delete ivExhaustiveMergeGroups;
	}
	assert(kwftSource->GetTotalFrequency() == kwftTarget->GetTotalFrequency());

	// Initialisation du flag de groupage en un seul groupe
	bSingletonGroupingTested = nMaxTestedForcedMergeNumber == 1 or kwftTarget->GetFrequencyVectorNumber() == 1;

	// Nombre de groupes apres ExhaustiveMerge
	if (bPrintGroupNumbers)
		cout << kwftTarget->GetFrequencyVectorNumber() << "\t";

	// Initialisation de la meilleure solution, en post-optimisant la solution initiale
	if (kwftTarget->GetFrequencyVectorNumber() > 1)
		PostOptimizeGroups(kwftSource, kwftTarget, ivGroups);

	// Memorisation de son cout
	dBestCost = ComputePartitionGlobalCost(kwftTarget);

	// Recherche iterative d'ameliorations par fusion de groupes
	nTestedForcedMergeNumber = 0;
	if (kwftTarget->GetFrequencyVectorNumber() > 1 and nTestedForcedMergeNumber < nMaxTestedForcedMergeNumber)
	{
		// Initialisation des donnees de travail
		kwftCurrentTarget = kwftTarget->Clone();
		ivNewGroups.CopyFrom(ivGroups);

		// On force les fusions et on memorise la meilleure solution rencontree,
		// en limitant le nombre maximum d'etapes successives infructueuses
		while (kwftCurrentTarget->GetFrequencyVectorNumber() > 1 and
		       nTestedForcedMergeNumber < nMaxTestedForcedMergeNumber)
		{
			nTestedForcedMergeNumber++;

			// On force la meilleure fusion de groupe
			ForceBestGroupMerge(kwftSource, kwftCurrentTarget, kwftNewTarget, &ivNewGroups);

			// On memorise le fait de tester le groupage en un seul groupe
			if (kwftNewTarget->GetFrequencyVectorNumber() == 1)
				bSingletonGroupingTested = true;

			// Post-optimisation de ce nouveau groupage
			PostOptimizeGroups(kwftSource, kwftNewTarget, &ivNewGroups);

			// Evaluation du cout correspondant
			dNewCost = ComputePartitionGlobalCost(kwftNewTarget);

			// Memorisation si amelioration
			if (dNewCost < dBestCost + dEpsilon)
			{
				dBestCost = dNewCost;

				// Recopie de la solution
				ivGroups->CopyFrom(&ivNewGroups);
				kwftTarget->CopyFrom(kwftNewTarget);

				// Remise a zero du compteur d'etapes infructueuses
				nTestedForcedMergeNumber = 0;
			}

			// On change de table de contingence courante, et on nettoie les donnees
			// devenues inutiles
			delete kwftCurrentTarget;
			kwftCurrentTarget = kwftNewTarget;
		}

		// Nettoyage
		delete kwftCurrentTarget;
	}
	assert(kwftSource->GetTotalFrequency() == kwftTarget->GetTotalFrequency());

	// Nombre de groupes apres les ameliorations par fusions iteratives
	if (bPrintGroupNumbers)
		cout << kwftTarget->GetFrequencyVectorNumber() << "\t";

	// Test final: comparaison avec une solution de groupage en un seul groupe
	if (not bSingletonGroupingTested)
	{
		assert(kwftTarget->GetFrequencyVectorNumber() > 1);

		// Optimisation de la table
		BuildSingletonGrouping(kwftTarget, kwftExhaustiveMergeTarget);

		// Evaluation du cout correspondant
		dNewCost = ComputePartitionGlobalCost(kwftExhaustiveMergeTarget);

		// Memorisation si amelioration
		if (dNewCost < dBestCost + dEpsilon)
		{
			// La nouvelle table cible remplace la precedente
			kwftTarget->CopyFrom(kwftExhaustiveMergeTarget);

			// Recalcul des index des groupes
			for (nSource = 0; nSource < ivGroups->GetSize(); nSource++)
				ivGroups->SetAt(nSource, 0);
		}

		// Nettoyage
		delete kwftExhaustiveMergeTarget;
	}

	// Nombre de groupes final
	if (bPrintGroupNumbers)
		cout << kwftTarget->GetFrequencyVectorNumber() << endl;

	ensure(kwftSource->GetTotalFrequency() == kwftTarget->GetTotalFrequency());
	ensure(ivGroups->GetSize() == kwftSource->GetFrequencyVectorNumber());
}

void DTGrouperMODL::ExhaustiveMergeGrouping(int nMinGroupNumber, KWFrequencyTable* kwftSource,
					    KWFrequencyTable*& kwftTarget, IntVector*& ivGroups) const
{
	ObjectArray* oaInitialGroups;
	ObjectArray* oaInitialGroupMerges;
	ObjectArray* oaNewGroups;
	int nOptimumGroupNumber;
	int nSource;

	require(kwftSource != NULL);
	require(1 <= nMinGroupNumber and nMinGroupNumber <= kwftSource->GetFrequencyVectorNumber());
	require(kwftSource->GetFrequencyVectorNumber() < KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage() or
		GetMaxGroupNumber() == 2 or GetMaxGroupNumber() == 1);

	///////////////////////////////////////////////////////////////////
	// Premiere etape: determination du nombre de groupes optimum en
	// memorisant le meilleur groupage rencontre au cours d'une suite
	// de merges forces

	// Initialisation des groupes
	oaInitialGroups = BuildGroups(kwftSource);

	// Initialisation des merges
	oaInitialGroupMerges = BuildGroupMerges(oaInitialGroups);

	// Recherche du nombre optimal de groupes
	nOptimumGroupNumber =
	    OptimizeGroups(nMinGroupNumber, oaInitialGroups, oaInitialGroupMerges, oaNewGroups, ivGroups);
	assert(ivGroups->GetSize() == kwftSource->GetFrequencyVectorNumber());

	// Nettoyage des donnees de travail initiales
	delete oaInitialGroups;
	delete oaInitialGroupMerges;

	////////////////////////////////////////////////////////////////
	// Deuxieme etape: on recalcule le groupage avec le nombre de
	// groupes optimaux

	// Cas particulier: le groupage initial etait correct
	if (nOptimumGroupNumber == kwftSource->GetFrequencyVectorNumber())
	{
		// Nettoyage des donnees calculees
		oaNewGroups->DeleteAll();
		delete oaNewGroups;

		// Creation de la table cible
		kwftTarget = kwftSource->Clone();

		// Reaffectation des index de groupes cibles
		for (nSource = 0; nSource < ivGroups->GetSize(); nSource++)
			ivGroups->SetAt(nSource, nSource);
	}
	// Cas particulier: le nombre optimal de groupes correspond au nombre
	// de groupes rendus
	else if (nOptimumGroupNumber == oaNewGroups->GetSize())
	{
		// Utilisation des donnees calculees pour fabrique la table cible
		// (le vecteur d'index est correct)
		kwftTarget = BuildTable(oaNewGroups);
		// Parametrage nInitialValueNumber, nGranularizedValueNumber
		kwftTarget->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
		kwftTarget->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
		// Parametrage granularite
		kwftTarget->SetGranularity(kwftSource->GetGranularity());
		kwftTarget->SetGarbageModalityNumber(0);

		// Nettoyage des donnees calculees
		oaNewGroups->DeleteAll();
		delete oaNewGroups;
	}
	// Cas general ou l'on doit recalculer le groupage
	else
	{
		// Nettoyage des donnees calculees
		oaNewGroups->DeleteAll();
		delete oaNewGroups;
		delete ivGroups;

		// Initialisation des groupes
		oaInitialGroups = BuildGroups(kwftSource);

		// Initialisation des merges
		oaInitialGroupMerges = BuildGroupMerges(oaInitialGroups);

		// Construction du nombre optimal de groupes
		OptimizeGroups(nOptimumGroupNumber, oaInitialGroups, oaInitialGroupMerges, oaNewGroups, ivGroups);

		// Nettoyage des donnees de travail initiales
		delete oaInitialGroups;
		delete oaInitialGroupMerges;

		// Utilisation des donnees calculees pour fabrique la table cible
		// (le vecteur d'index est correct)
		kwftTarget = BuildTable(oaNewGroups);
		// Parametrage nInitialValueNumber, nGranularizedValueNumber
		kwftTarget->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
		kwftTarget->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
		// Parametrage granularite et poubelle
		kwftTarget->SetGranularity(kwftSource->GetGranularity());
		kwftTarget->SetGarbageModalityNumber(0);

		// Nettoyage des donnees calculees
		oaNewGroups->DeleteAll();
		delete oaNewGroups;
	}

	ensure(kwftTarget->GetFrequencyVectorNumber() >= nMinGroupNumber);
	ensure(ivGroups->GetSize() == kwftSource->GetFrequencyVectorNumber());
}

void DTGrouperMODL::PostOptimizeGroups(KWFrequencyTable* kwftSource, KWFrequencyTable* kwftTarget,
				       IntVector* ivGroups) const
{
	boolean bPrintInitialTables = false;
	boolean bPrintOptimizations = false;
	boolean bPrintFinalTable = false;
	boolean bContinue;
	int nModalityNumber;
	int nGroupNumber;
	DoubleVector dvGroupCosts;
	DoubleVector dvGroupOutDeltaCosts;
	DoubleVector dvGroupInDeltaCosts;
	int nModality;
	int nGroup;
	int nCurrentGroup;
	double dCost;
	double dDeltaCost;
	double dBestDeltaCost;
	int nBestModality;
	int nBestGroup;
	int nOutGroup;
	int nInGroup;

	require(kwftSource != NULL);
	require(kwftTarget != NULL);
	require(ivGroups != NULL);
	require(kwftSource->GetFrequencyVectorSize() == kwftTarget->GetFrequencyVectorSize());
	require(kwftSource->GetFrequencyVectorNumber() >= kwftTarget->GetFrequencyVectorNumber());
	require(ivGroups->GetSize() == kwftSource->GetFrequencyVectorNumber());

	// Arret si un seul groupe
	if (kwftTarget->GetFrequencyVectorNumber() <= 1)
		return;

	// Affichage des tables initiales
	if (bPrintInitialTables)
		cout << *kwftSource << endl << *kwftTarget << endl;

	// Memorisation des nombre de modalites, groupes et classes cibles
	nModalityNumber = kwftSource->GetFrequencyVectorNumber();
	nGroupNumber = kwftTarget->GetFrequencyVectorNumber();

	// Initialisation des valeurs de groupes
	dvGroupCosts.SetSize(nGroupNumber);
	for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
		dvGroupCosts.SetAt(nGroup, ComputeGroupCost(kwftTarget->GetFrequencyVectorAt(nGroup)));

	// Initialisation des valeurs de "depart" des groupes
	// Pour chaque modalite source, on evalue le DeltaCout engendre par le depart
	// de son groupe de rattachement
	dvGroupOutDeltaCosts.SetSize(nModalityNumber);
	for (nModality = 0; nModality < nModalityNumber; nModality++)
	{
		// Recherche du groupe de rattachement de la modalite
		nCurrentGroup = ivGroups->GetAt(nModality);

		// Calcul du cout du groupe apres le depart de la modalite,
		// en se basant sur les nouveaux effectifs du groupe
		dCost = ComputeGroupDiffCost(kwftTarget->GetFrequencyVectorAt(nCurrentGroup),
					     kwftSource->GetFrequencyVectorAt(nModality));

		// Memorisation du DeltaCout
		dvGroupOutDeltaCosts.SetAt(nModality, dCost - dvGroupCosts.GetAt(nCurrentGroup));
	}

	// Initialisation des valeurs d'"arrivee" des groupes
	// Pour chaque modalite source, on evalue le DeltaCout engendre par l'arrivee
	// dans un nouveau groupe de rattachement
	dvGroupInDeltaCosts.SetSize(nModalityNumber * nGroupNumber);
	for (nModality = 0; nModality < nModalityNumber; nModality++)
	{
		// Recherche du groupe de rattachement de la modalite
		nCurrentGroup = ivGroups->GetAt(nModality);

		// Parcours des groupes de rattachement possible
		for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
		{
			// On n'evalue que les nouveaux groupes potentiels
			if (nGroup != nCurrentGroup)
			{
				// Calcul du cout du groupe apres l'arrivee de la modalite,
				// en se basant sur les nouveaux effectifs du groupe
				dCost = ComputeGroupUnionCost(kwftTarget->GetFrequencyVectorAt(nGroup),
							      kwftSource->GetFrequencyVectorAt(nModality));

				// Memorisation du DeltaCout
				dvGroupInDeltaCosts.SetAt(nModality * nGroupNumber + nGroup,
							  dCost - dvGroupCosts.GetAt(nGroup));
			}
		}
	}

	// Tant qu'il y a amelioration, on continue a chercher les meilleurs
	// deplacement de modalites entre groupes
	bContinue = true;
	while (bContinue)
	{
		// Recherche de la meilleure amelioration par deplacement de modalite
		dBestDeltaCost = dEpsilon;
		nBestModality = -1;
		nBestGroup = -1;
		for (nModality = 0; nModality < nModalityNumber; nModality++)
		{
			// Recherche du groupe de rattachement de la modalite
			nCurrentGroup = ivGroups->GetAt(nModality);

			// Recherche du meilleur transfert de groupe
			for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
			{
				// On n'evalue que les nouveaux groupes potentiels
				if (nGroup != nCurrentGroup)
				{
					// Evaluation de la variation de cout globale
					dDeltaCost = dvGroupOutDeltaCosts.GetAt(nModality) +
						     dvGroupInDeltaCosts.GetAt(nModality * nGroupNumber + nGroup);

					// Memorisation si amelioration
					if (dDeltaCost < dBestDeltaCost)
					{
						dBestDeltaCost = dDeltaCost;
						nBestModality = nModality;
						nBestGroup = nGroup;
					}
				}
			}
		}

		// Evaluation de l'interet de l'optimization
		bContinue = dBestDeltaCost < -dEpsilon;

		// Si amelioration, mise a jour des calculs de transfert de modalites
		if (bContinue)
		{
			// Affichage du transfert
			if (bPrintOptimizations)
			{
				cout << "Transfert\t" << dBestDeltaCost << "\t" << nBestModality << "\t" << nBestGroup
				     << "\t";
				kwftSource->GetFrequencyVectorAt(nBestModality)->WriteLineReport(cout);
				cout << endl;
			}

			// Memorisation des index des groupes de depart et d'arrivee
			nOutGroup = ivGroups->GetAt(nBestModality);
			nInGroup = nBestGroup;

			// Modification du cout des groupes de depart et d'arrivee
			dvGroupCosts.UpgradeAt(nOutGroup, dvGroupOutDeltaCosts.GetAt(nBestModality));
			dvGroupCosts.UpgradeAt(nInGroup,
					       dvGroupInDeltaCosts.GetAt(nBestModality * nGroupNumber + nInGroup));

			// Modification du cout de depart pour la meilleure modalite
			dvGroupOutDeltaCosts.SetAt(nBestModality,
						   -dvGroupInDeltaCosts.GetAt(nBestModality * nGroupNumber + nInGroup));

			// Modification des effectifs des groupes de depart et d'arrivee
			AddFrequencyVector(kwftTarget->GetFrequencyVectorAt(nInGroup),
					   kwftSource->GetFrequencyVectorAt(nBestModality));
			RemoveFrequencyVector(kwftTarget->GetFrequencyVectorAt(nOutGroup),
					      kwftSource->GetFrequencyVectorAt(nBestModality));
			assert(fabs(ComputeGroupCost(kwftTarget->GetFrequencyVectorAt(nOutGroup)) -
				    dvGroupCosts.GetAt(nOutGroup)) < dEpsilon);
			assert(fabs(ComputeGroupCost(kwftTarget->GetFrequencyVectorAt(nInGroup)) -
				    dvGroupCosts.GetAt(nInGroup)) < dEpsilon);

			// Memorisation de l'index du nouveau groupe
			ivGroups->SetAt(nBestModality, nInGroup);

			// Modification des couts d'arrivee pour la meilleure modalite

			// Parcours des groupes de rattachement possible
			for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
			{
				// On n'evalue que les nouveaux groupes potentiels
				if (nGroup != nInGroup)
				{
					// Calcul du cout du groupe apres l'arrivee de la modalite,
					// en se basant sur les nouveaux effectifs du groupe
					dCost = ComputeGroupUnionCost(kwftTarget->GetFrequencyVectorAt(nGroup),
								      kwftSource->GetFrequencyVectorAt(nBestModality));

					// Memorisation du DeltaCout
					dvGroupInDeltaCosts.SetAt(nBestModality * nGroupNumber + nGroup,
								  dCost - dvGroupCosts.GetAt(nGroup));
				}
				// Utile ? ?
				// CH V9 TODO: a checker
				// On re-initialise le cout d'arrivee de la modalite dans son groupe de depart
				else
				{
					dvGroupInDeltaCosts.SetAt(nBestModality * nGroupNumber + nGroup, 0);
				}
			}

			// Recalcul pour les deux groupes concernes des valeurs d'arrivee des modalites
			for (nModality = 0; nModality < nModalityNumber; nModality++)
			{
				if (nModality == nBestModality)
					continue;

				// Recherche du groupe de rattachement de la modalite
				nCurrentGroup = ivGroups->GetAt(nModality);

				// Evaluation du cout de depart pour le groupe de depart ou d'arrivee
				if (nCurrentGroup == nOutGroup or nCurrentGroup == nInGroup)
				{
					// Calcul du cout du groupe apres le depart de la modalite,
					// en se basant sur les nouveaux effectifs du groupe
					dCost = ComputeGroupDiffCost(kwftTarget->GetFrequencyVectorAt(nCurrentGroup),
								     kwftSource->GetFrequencyVectorAt(nModality));

					// Memorisation du DeltaCout
					dvGroupOutDeltaCosts.SetAt(nModality,
								   dCost - dvGroupCosts.GetAt(nCurrentGroup));
				}

				// Evaluation du cout d'arrivee pour le groupe de depart
				if (nOutGroup != nCurrentGroup)
				{
					// Calcul du cout du groupe apres l'arrivee de la modalite,
					// en se basant sur les nouveaux effectifs du groupe
					dCost = ComputeGroupUnionCost(kwftTarget->GetFrequencyVectorAt(nOutGroup),
								      kwftSource->GetFrequencyVectorAt(nModality));

					// Memorisation du DeltaCout
					dvGroupInDeltaCosts.SetAt(nModality * nGroupNumber + nOutGroup,
								  dCost - dvGroupCosts.GetAt(nOutGroup));
				}

				// Evaluation  du cout d'arrivee pour le groupe d'arrivee
				if (nInGroup != nCurrentGroup)
				{
					// Calcul du cout du groupe apres l'arrivee de la modalite,
					// en se basant sur les nouveaux effectifs du groupe
					dCost = ComputeGroupUnionCost(kwftTarget->GetFrequencyVectorAt(nInGroup),
								      kwftSource->GetFrequencyVectorAt(nModality));

					// Memorisation du DeltaCout
					dvGroupInDeltaCosts.SetAt(nModality * nGroupNumber + nInGroup,
								  dCost - dvGroupCosts.GetAt(nInGroup));
				}
			}
		}
	}

	// Affichage de la table finale
	if (bPrintFinalTable)
		cout << *kwftTarget << endl;

	ensure(kwftSource->GetFrequencyVectorSize() == kwftTarget->GetFrequencyVectorSize());
}

void DTGrouperMODL::FastPostOptimizeGroups(KWFrequencyTable* kwftSource, KWFrequencyTable* kwftTarget,
					   IntVector* ivGroups, int nMaxStepNumber) const
{
	boolean bPrintInitialTables = false;
	boolean bPrintOptimizations = false;
	boolean bPrintFinalTable = false;
	boolean bSearchBestOptim = false;
	boolean bContinue;
	longint lDisplayFreshness;
	int nStepNumber;
	int nModalityNumber;
	int nGroupNumber;
	int nTargetNumber;
	IntVector ivModalityIndexes;
	IntVector ivGroupIndexes;
	DoubleVector dvGroupCosts;
	int nModality;
	int nGroup;
	int n1;
	int n2;
	int nStart;
	double dOutDeltaCost;
	double dInDeltaCost;
	double dDeltaCost;
	int nOutGroup;
	int nInGroup;
	double dBestDeltaCost;
	double dBestInDeltaCost;
	int nBestInGroup;

	require(kwftSource != NULL);
	require(kwftTarget != NULL);
	require(ivGroups != NULL);
	require(kwftSource->GetFrequencyVectorSize() == kwftTarget->GetFrequencyVectorSize());
	require(kwftSource->GetFrequencyVectorNumber() >= kwftTarget->GetFrequencyVectorNumber());
	require(ivGroups->GetSize() == kwftSource->GetFrequencyVectorNumber());
	require(0 <= nMaxStepNumber);

	// Arret si un seul groupe
	if (kwftTarget->GetFrequencyVectorNumber() <= 1)
		return;

	// Fraicheur d'affichage pour la gestion de la barre de progression
	lDisplayFreshness = 0;

	// Affichage des tables initiales
	if (bPrintInitialTables)
		cout << *kwftSource << endl << *kwftTarget << endl;

	// Memorisation des nombre de modalites, groupes et classes cibles
	nModalityNumber = kwftSource->GetFrequencyVectorNumber();
	nGroupNumber = kwftTarget->GetFrequencyVectorNumber();
	nTargetNumber = kwftSource->GetFrequencyVectorSize();

	// Initialisation des vecteurs d'index des modalites et des groupes
	ivModalityIndexes.SetSize(nModalityNumber);
	for (nModality = 0; nModality < nModalityNumber; nModality++)
		ivModalityIndexes.SetAt(nModality, nModality);
	ivGroupIndexes.SetSize(nGroupNumber);
	for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
		ivGroupIndexes.SetAt(nGroup, nGroup);

	// Initialisation des valeurs de groupes
	dvGroupCosts.SetSize(nGroupNumber);
	for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
		dvGroupCosts.SetAt(nGroup, ComputeGroupCost(kwftTarget->GetFrequencyVectorAt(nGroup)));

	// Tant qu'il y a amelioration, on continue a chercher le premier
	// deplacement interessant de modalites entre groupes
	bContinue = true;
	nStepNumber = 0;
	while (bContinue and nStepNumber < nMaxStepNumber)
	{
		// Par defaut, on ne continue pas
		bContinue = false;
		nStepNumber++;

		// Test si arret de tache demandee
		lDisplayFreshness++;
		if (TaskProgression::IsRefreshNecessary(lDisplayFreshness) and
		    TaskProgression::IsInterruptionRequested())
			break;

		// Perturbation aleatoire des index de modalites et de groupes
		ivModalityIndexes.Shuffle();
		ivGroupIndexes.Shuffle();

		// Parcours de toutes les modalites
		for (n1 = 0; n1 < ivModalityIndexes.GetSize(); n1++)
		{
			nModality = ivModalityIndexes.GetAt(n1);

			// Recherche du groupe de rattachement de la modalite
			nOutGroup = ivGroups->GetAt(nModality);

			// Calcul du cout du groupe apres le depart de la modalite,
			// en se basant sur les nouveaux effectifs du groupe
			dOutDeltaCost = ComputeGroupDiffCost(kwftTarget->GetFrequencyVectorAt(nOutGroup),
							     kwftSource->GetFrequencyVectorAt(nModality));
			dOutDeltaCost -= dvGroupCosts.GetAt(nOutGroup);

			// Parcours des groupes cible potentiels
			// (de facon "aleatoire": on ne refait pas le Shuffle des index des groupes)
			dBestDeltaCost = 0;
			dBestInDeltaCost = 0;
			nBestInGroup = 0;
			nStart = RandomInt(nGroupNumber);
			for (n2 = 0; n2 < nGroupNumber; n2++)
			{
				nInGroup = ivGroupIndexes.GetAt((nStart + n2) % nGroupNumber);

				// Test si arret de tache demandee
				lDisplayFreshness++;
				if (TaskProgression::IsRefreshNecessary(lDisplayFreshness) and
				    TaskProgression::IsInterruptionRequested())
					break;

				// On n'evalue que les nouveaux groupes potentiels
				if (nInGroup != nOutGroup)
				{
					// Calcul du cout du groupe apres l'arrivee de la modalite,
					// en se basant sur les nouveaux effectifs du groupe
					dInDeltaCost =
					    ComputeGroupUnionCost(kwftTarget->GetFrequencyVectorAt(nInGroup),
								  kwftSource->GetFrequencyVectorAt(nModality));
					dInDeltaCost -= dvGroupCosts.GetAt(nInGroup);

					// Evaluation de la variation de cout globale
					dDeltaCost = dOutDeltaCost + dInDeltaCost;

					// Memorisation si amelioration
					if (dDeltaCost < dBestDeltaCost - dEpsilon)
					{
						dBestDeltaCost = dDeltaCost;
						dBestInDeltaCost = dInDeltaCost;
						nBestInGroup = nInGroup;

						// L'arret a la premiere amelioration permet une optimisation plus
						// "douce", compatible avec des amelioration alternee par variable de
						// grille
						if (not bSearchBestOptim)
							break;
					}
				}
			}

			// Test si arret de tache demandee
			lDisplayFreshness++;
			if (TaskProgression::IsRefreshNecessary(lDisplayFreshness) and
			    TaskProgression::IsInterruptionRequested())
				break;

			// On effectue si necessaire le meilleur transfert de groupe
			if (dBestDeltaCost < -dEpsilon)
			{
				// Affichage du transfert
				if (bPrintOptimizations)
				{
					cout << "Transfert\t" << dBestDeltaCost << "\t" << nModality << "\t"
					     << nBestInGroup << "\t";
					kwftSource->GetFrequencyVectorAt(nModality)->WriteLineReport(cout);
					cout << endl;
				}

				// Modification du cout des groupes de depart et d'arrivee
				dvGroupCosts.UpgradeAt(nOutGroup, dOutDeltaCost);
				dvGroupCosts.UpgradeAt(nBestInGroup, dBestInDeltaCost);

				// Modification des effectifs des groupes de depart et d'arrivee
				AddFrequencyVector(kwftTarget->GetFrequencyVectorAt(nBestInGroup),
						   kwftSource->GetFrequencyVectorAt(nModality));
				RemoveFrequencyVector(kwftTarget->GetFrequencyVectorAt(nOutGroup),
						      kwftSource->GetFrequencyVectorAt(nModality));
				assert(fabs(ComputeGroupCost(kwftTarget->GetFrequencyVectorAt(nOutGroup)) -
					    dvGroupCosts.GetAt(nOutGroup)) < dEpsilon);
				assert(fabs(ComputeGroupCost(kwftTarget->GetFrequencyVectorAt(nBestInGroup)) -
					    dvGroupCosts.GetAt(nBestInGroup)) < dEpsilon);

				// Memorisation de l'index du nouveau groupe
				ivGroups->SetAt(nModality, nBestInGroup);

				// On continue de rechercher les ameliorations
				bContinue = true;
			}
		}
	}

	// Affichage de la table finale
	if (bPrintFinalTable)
		cout << *kwftTarget << endl;

	ensure(kwftSource->GetFrequencyVectorSize() == kwftTarget->GetFrequencyVectorSize());
}

void DTGrouperMODL::FastPostOptimizeGroupsWithGarbage(KWFrequencyTable* kwftSource, KWFrequencyTable* kwftTarget,
						      IntVector* ivGroups, int nMaxStepNumber,
						      SortedList* frequencyList) const
{
	boolean bPrintInitialTables = false;
	boolean bPrintOptimizations = false;
	boolean bPrintFinalTable = false;
	boolean bSearchBestOptim = false;
	boolean bPrintDeltaCosts = false;
	boolean bContinue;
	longint lDisplayFreshness;
	int nStepNumber;
	int nModalityNumber;
	int nGroupNumber;
	int nTargetNumber;
	IntVector ivModalityIndexes;
	IntVector ivGroupIndexes;
	DoubleVector dvGroupCosts;
	int nModality;
	int nGroup;
	int n1;
	int n2;
	int nStart;
	double dOutDeltaCost;
	double dInDeltaCost;
	double dDeltaCost;
	int nOutGroup;
	int nInGroup;
	double dBestDeltaCost;
	double dBestInDeltaCost;
	int nBestInGroup;
	POSITION position;
	KWFrequencyVector* frequencyVector;
	int nGarbageModalityNumber;
	int nGarbageModalityNumberMax;
	int nNewGarbageModalityNumber;
	int nTrueGroupNumber;
	int nNewTrueGroupNumber;
	boolean bLastModality;
	boolean bBestLastModality;
	double dPartitionDeltaCost;

	require(kwftSource != NULL);
	require(kwftTarget != NULL);
	require(ivGroups != NULL);
	require(kwftSource->GetFrequencyVectorSize() == kwftTarget->GetFrequencyVectorSize());
	require(kwftSource->GetFrequencyVectorNumber() >= kwftTarget->GetFrequencyVectorNumber());
	require(ivGroups->GetSize() == kwftSource->GetFrequencyVectorNumber());
	require(0 <= nMaxStepNumber);
	require(frequencyList != NULL);

	// Arret si un seul groupe
	if (kwftTarget->GetFrequencyVectorNumber() <= 1)
		return;

	// Fraicheur d'affichage pour la gestion de la barre de progression
	lDisplayFreshness = 0;

	// Affichage des tables initiales
	if (bPrintInitialTables)
		cout << *kwftSource << endl << *kwftTarget << endl;

	// Memorisation des nombre de modalites, groupes et classes cibles
	nModalityNumber = kwftSource->GetFrequencyVectorNumber();
	nGroupNumber = kwftTarget->GetFrequencyVectorNumber();
	nTargetNumber = kwftSource->GetFrequencyVectorSize();
	// Initialisation du nombre de modalites du groupe poubelle
	nGarbageModalityNumber = cast(KWFrequencyVector*, frequencyList->GetHead())->GetModalityNumber();
	nNewGarbageModalityNumber = 0;
	nTrueGroupNumber = nGroupNumber;

	// Parcours de la liste pour compter le nombre total de modalites
	nGarbageModalityNumberMax = 0;
	position = frequencyList->GetHeadPosition();
	while (position != NULL)
	{
		nGarbageModalityNumberMax +=
		    cast(KWFrequencyVector*, frequencyList->GetNext(position))->GetModalityNumber();
	}
	nGarbageModalityNumberMax = nGarbageModalityNumberMax - nGroupNumber + 1;

	// Initialisation des vecteurs d'index des modalites et des groupes
	ivModalityIndexes.SetSize(nModalityNumber);
	for (nModality = 0; nModality < nModalityNumber; nModality++)
		ivModalityIndexes.SetAt(nModality, nModality);
	ivGroupIndexes.SetSize(nGroupNumber);
	for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
		ivGroupIndexes.SetAt(nGroup, nGroup);

	// Initialisation des valeurs de groupes
	dvGroupCosts.SetSize(nGroupNumber);
	for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
		dvGroupCosts.SetAt(nGroup, ComputeGroupCost(kwftTarget->GetFrequencyVectorAt(nGroup)));

	// Tant qu'il y a amelioration, on continue a chercher le premier
	// deplacement interessant de modalites entre groupes
	bContinue = true;
	nStepNumber = 0;
	bLastModality = false;
	bBestLastModality = false;
	while (bContinue and nStepNumber < nMaxStepNumber)
	{
		// Par defaut, on ne continue pas
		bContinue = false;
		nStepNumber++;

		// Test si arret de tache demandee
		lDisplayFreshness++;
		if (TaskProgression::IsRefreshNecessary(lDisplayFreshness) and
		    TaskProgression::IsInterruptionRequested())
			break;

		// Perturbation aleatoire des index de modalites et de groupes
		ivModalityIndexes.Shuffle();
		ivGroupIndexes.Shuffle();

		// Parcours de toutes les modalites
		for (n1 = 0; n1 < ivModalityIndexes.GetSize(); n1++)
		{
			nModality = ivModalityIndexes.GetAt(n1);

			// Recherche du groupe de rattachement de la modalite
			nOutGroup = ivGroups->GetAt(nModality);

			// Calcul du cout du groupe apres le depart de la modalite,
			// en se basant sur les nouveaux effectifs du groupe
			dOutDeltaCost = ComputeGroupDiffCost(kwftTarget->GetFrequencyVectorAt(nOutGroup),
							     kwftSource->GetFrequencyVectorAt(nModality));
			dOutDeltaCost -= dvGroupCosts.GetAt(nOutGroup);

			// Parcours des groupes cible potentiels
			// (de facon "aleatoire": on ne refait pas le Shuffle des index des groupes)
			dDeltaCost = 0;
			dBestDeltaCost = 0;
			dBestInDeltaCost = 0;
			nBestInGroup = 0;
			nStart = RandomInt(nGroupNumber);
			if (kwftTarget->GetGarbageModalityNumber() > 0)
				nGarbageModalityNumber =
				    cast(KWFrequencyVector*, frequencyList->GetHead())->GetModalityNumber();
			else
				nGarbageModalityNumber = 0;
			for (n2 = 0; n2 < nGroupNumber; n2++)
			{
				nInGroup = ivGroupIndexes.GetAt((nStart + n2) % nGroupNumber);

				// Test si arret de tache demandee
				lDisplayFreshness++;
				if (TaskProgression::IsRefreshNecessary(lDisplayFreshness) and
				    TaskProgression::IsInterruptionRequested())
					break;

				// On n'evalue que les nouveaux groupes potentiels
				if (nInGroup != nOutGroup)
				{
					// Calcul du cout du groupe apres l'arrivee de la modalite,
					// en se basant sur les nouveaux effectifs du groupe
					dInDeltaCost =
					    ComputeGroupUnionCost(kwftTarget->GetFrequencyVectorAt(nInGroup),
								  kwftSource->GetFrequencyVectorAt(nModality));
					dInDeltaCost -= dvGroupCosts.GetAt(nInGroup);

					// Initialisations
					// Nombre de groupes apres ce deplacement
					nNewTrueGroupNumber = nTrueGroupNumber;
					// Derniere modalite de son groupe
					bLastModality = false;

					// Variation du nombre de groupes suite a ce deplacement
					// Cas ou le groupe devient vide apres le depart de la modalite
					if (kwftTarget->GetFrequencyVectorAt(nOutGroup)->ComputeTotalFrequency() ==
					    kwftSource->GetFrequencyVectorAt(nModality)->ComputeTotalFrequency())
					{
						bLastModality = true;
						nNewTrueGroupNumber--;
					}
					// Cas d'une modalite qui arrive dans un groupe qui etait vide
					if (kwftTarget->GetFrequencyVectorAt(nInGroup)->ComputeTotalFrequency() == 0)
						// Incrementation du nombre de groupes
						nNewTrueGroupNumber++;

					// Taille du groupe poubelle apres ce deplacement
					nNewGarbageModalityNumber = nGarbageModalityNumber;
					// Variation de la taille du groupe poubelle
					// Uniquement en presence d'un groupe poubelle
					if (kwftTarget->GetGarbageModalityNumber() > 0)
					{
						// Cas ou le deplacement est envisageable : au moins 2 groupes et 1
						// groupe poubelle apres deplacement
						if ((bLastModality and nTrueGroupNumber > 3) or
						    (not bLastModality and nTrueGroupNumber >= 3))
						{
							// Cas ou la modalite part du groupe poubelle
							if (kwftTarget->GetFrequencyVectorAt(nOutGroup)
								->GetPosition() == frequencyList->GetHeadPosition())
							{
								// On decremente la taille du groupe poubelle du nombre
								// de modalites deplacees En coclustering, la modalite
								// peut etre une super modalite donc contenir plus d'une
								// modalite
								nNewGarbageModalityNumber =
								    nNewGarbageModalityNumber -
								    kwftSource->GetFrequencyVectorAt(nModality)
									->GetModalityNumber();

								// Comparaison avec le nombre de modalites du groupe qui
								// accueillerait la modalite
								if (nNewGarbageModalityNumber <
								    kwftTarget->GetFrequencyVectorAt(nInGroup)
									    ->GetModalityNumber() +
									kwftSource->GetFrequencyVectorAt(nModality)
									    ->GetModalityNumber())
									nNewGarbageModalityNumber =
									    kwftTarget->GetFrequencyVectorAt(nInGroup)
										->GetModalityNumber() +
									    kwftSource->GetFrequencyVectorAt(nModality)
										->GetModalityNumber();

								position = frequencyList->GetHeadPosition();
								frequencyVector =
								    cast(KWFrequencyVector*,
									 frequencyList->GetNext(position));
								frequencyVector =
								    cast(KWFrequencyVector*,
									 frequencyList->GetNext(position));
								// Comparaison avec le nombre de modalites du second
								// groupe par nombre de modalites
								if (nNewGarbageModalityNumber <
								    frequencyVector->GetModalityNumber())
									nNewGarbageModalityNumber =
									    frequencyVector->GetModalityNumber();
							}

							// Comparaison avec le nombre de modalites du groupe d'arrivee
							if (nNewGarbageModalityNumber <
							    kwftTarget->GetFrequencyVectorAt(nInGroup)
								    ->GetModalityNumber() +
								kwftSource->GetFrequencyVectorAt(nModality)
								    ->GetModalityNumber())
								nNewGarbageModalityNumber =
								    kwftTarget->GetFrequencyVectorAt(nInGroup)
									->GetModalityNumber() +
								    kwftSource->GetFrequencyVectorAt(nModality)
									->GetModalityNumber();
						}
					}

					dPartitionDeltaCost = 0;

					// Cas ou le nombre de groupes ou la taille du groupe poubelle a evolue :
					// evaluation de la variation du cout de partition
					if (nNewTrueGroupNumber != nTrueGroupNumber or
					    nNewGarbageModalityNumber != nGarbageModalityNumber)
					{
						// Cas de deplacements non envisageables
						// Cas d'un nombre de modalites dans le groupe poubelle trop elevee par
						// rapport a la taille de la partition Ou cas d'un nombre de groupes
						// trop faible en presence d'un groupe poubelle
						if ((nNewGarbageModalityNumber > nGarbageModalityNumberMax) or
						    (nNewTrueGroupNumber < 3))
						{
							// Deplacement non envisageable
							dPartitionDeltaCost = DBL_MAX;
						}
						else
						{
							assert(nGarbageModalityNumber > 0 or
							       kwftTarget->GetGarbageModalityNumber() == 0);
							dPartitionDeltaCost =
							    ComputePartitionCost(nNewTrueGroupNumber,
										 nNewGarbageModalityNumber) -
							    ComputePartitionCost(nTrueGroupNumber,
										 nGarbageModalityNumber);
						}
					}

					// Evaluation de la variation globale du cout
					dDeltaCost = dOutDeltaCost + dInDeltaCost + dPartitionDeltaCost;

					if (bPrintDeltaCosts and dDeltaCost < dBestDeltaCost - dEpsilon)
					{
						cout << "Modality\t" << nModality << "\tOutGroup\t" << nOutGroup
						     << "\tInGroup\t" << nInGroup << "\tGroupNumber\t" << nGroupNumber
						     << "\tTrueGroupNumber\t " << nTrueGroupNumber << "\t Garbage \t"
						     << (kwftTarget->GetGarbageModalityNumber() > 0)
						     << "\tGarbageNumber\t" << nGarbageModalityNumber
						     << "\tNewGarbageNumber\t" << nNewGarbageModalityNumber << endl;
						cout << "BestDeltaCost\t" << dBestDeltaCost << "\t DeltaCost \t"
						     << dDeltaCost << "\tOutDeltaCost\t" << dOutDeltaCost
						     << "\tInDeltaCost\t" << dInDeltaCost << "\tPartitionDeltaCost\t"
						     << dPartitionDeltaCost << endl;
						cout << "Table avant deplacement " << endl;
						cout << *kwftTarget << endl;
					}

					// Memorisation si amelioration
					if (dDeltaCost < dBestDeltaCost - dEpsilon)
					{
						dBestDeltaCost = dDeltaCost;
						dBestInDeltaCost = dInDeltaCost;
						nBestInGroup = nInGroup;
						bBestLastModality = bLastModality;

						// L'arret a la premiere amelioration permet une optimisation plus
						// "douce", compatible avec des amelioration alternee par variable de
						// grille
						if (not bSearchBestOptim)
							break;
					}
				}
			}

			// Test si arret de tache demandee
			lDisplayFreshness++;
			if (TaskProgression::IsRefreshNecessary(lDisplayFreshness) and
			    TaskProgression::IsInterruptionRequested())
				break;

			// On effectue si necessaire le meilleur transfert de groupe
			if (dBestDeltaCost < -dEpsilon)
			{
				// Affichage du transfert
				if (bPrintOptimizations)
				{
					cout << "Etape\t" << nStepNumber << "\t" << dBestDeltaCost << "\t" << nModality
					     << "\t(" << nOutGroup << "," << nBestInGroup << ")\t" << bBestLastModality
					     << "\t";
					kwftSource->GetFrequencyVectorAt(nModality)->WriteLineReport(cout);
					cout << endl;
				}

				// Modification du cout des groupes de depart et d'arrivee
				dvGroupCosts.UpgradeAt(nOutGroup, dOutDeltaCost);
				dvGroupCosts.UpgradeAt(nBestInGroup, dBestInDeltaCost);

				// Retrait des groupes de depart et d'arrivee de la liste des nombres de modalites
				frequencyList->RemoveAt(kwftTarget->GetFrequencyVectorAt(nBestInGroup)->GetPosition());
				debug(kwftTarget->GetFrequencyVectorAt(nBestInGroup)->SetPosition(NULL));
				frequencyList->RemoveAt(kwftTarget->GetFrequencyVectorAt(nOutGroup)->GetPosition());
				debug(kwftTarget->GetFrequencyVectorAt(nOutGroup)->SetPosition(NULL));

				// Variation du nombre de groupes
				// Cas ou l'arrivee de la modalite remplit un groupe qui etait vide
				if (kwftTarget->GetFrequencyVectorAt(nBestInGroup)->ComputeTotalFrequency() == 0)
				{
					nTrueGroupNumber++;
				}

				// Modification des effectifs des groupes de depart et d'arrivee
				AddFrequencyVector(kwftTarget->GetFrequencyVectorAt(nBestInGroup),
						   kwftSource->GetFrequencyVectorAt(nModality));
				RemoveFrequencyVector(kwftTarget->GetFrequencyVectorAt(nOutGroup),
						      kwftSource->GetFrequencyVectorAt(nModality));
				assert(fabs(ComputeGroupCost(kwftTarget->GetFrequencyVectorAt(nOutGroup)) -
					    dvGroupCosts.GetAt(nOutGroup)) < dEpsilon);
				assert(fabs(ComputeGroupCost(kwftTarget->GetFrequencyVectorAt(nBestInGroup)) -
					    dvGroupCosts.GetAt(nBestInGroup)) < dEpsilon);

				// Variation du nombre de groupes
				// Cas ou le depart de la modalite laisse un groupe vide
				if (kwftTarget->GetFrequencyVectorAt(nOutGroup)->ComputeTotalFrequency() == 0)
				{
					assert(bBestLastModality);
					nTrueGroupNumber--;
				}

				// Reinsertion des groupes de depart et d'arrivee dans la liste des groupes triee par
				// nombre de modalites
				kwftTarget->GetFrequencyVectorAt(nBestInGroup)
				    ->SetPosition(frequencyList->Add(kwftTarget->GetFrequencyVectorAt(nBestInGroup)));
				kwftTarget->GetFrequencyVectorAt(nOutGroup)->SetPosition(
				    frequencyList->Add(kwftTarget->GetFrequencyVectorAt(nOutGroup)));
				// Mise a jour de la taille du groupe poubelle de la table
				if (kwftTarget->GetGarbageModalityNumber() > 0)
				{
					kwftTarget->SetGarbageModalityNumber(
					    cast(KWFrequencyVector*, frequencyList->GetHead())->GetModalityNumber());
					assert(nNewGarbageModalityNumber == kwftTarget->GetGarbageModalityNumber());
				}

				// Memorisation de l'index du nouveau groupe
				ivGroups->SetAt(nModality, nBestInGroup);

				// On continue de rechercher les ameliorations
				bContinue = true;
			}
		}
	}

	// Affichage de la table finale
	if (bPrintFinalTable)
		cout << *kwftTarget << endl;

	ensure(kwftSource->GetFrequencyVectorSize() == kwftTarget->GetFrequencyVectorSize());
}

void DTGrouperMODL::ForceBestGroupMerge(KWFrequencyTable* kwftSource, KWFrequencyTable* kwftTarget,
					KWFrequencyTable*& kwftNewTarget, IntVector* ivGroups) const
{
	DoubleVector dvGroupsCosts;
	IntVector ivNewGroups;
	int i;
	int nGroup;
	int nGroup1;
	int nGroup2;
	int nNewGroup;
	double dCost;
	double dDeltaCost;
	double dBestDeltaCost;
	int nBestGroup1;
	int nBestGroup2;

	require(kwftSource != NULL);
	require(kwftTarget != NULL);
	require(ivGroups != NULL);
	require(kwftSource->GetFrequencyVectorSize() == kwftTarget->GetFrequencyVectorSize());
	require(kwftSource->GetFrequencyVectorNumber() >= kwftTarget->GetFrequencyVectorNumber());
	require(ivGroups->GetSize() == kwftSource->GetFrequencyVectorNumber());
	require(kwftTarget->GetFrequencyVectorNumber() >= 2);

	// Calcul des couts des groupes
	dvGroupsCosts.SetSize(kwftTarget->GetFrequencyVectorNumber());
	for (nGroup = 0; nGroup < dvGroupsCosts.GetSize(); nGroup++)
		dvGroupsCosts.SetAt(nGroup, ComputeGroupCost(kwftTarget->GetFrequencyVectorAt(nGroup)));

	// Evaluation de toutes les fusions
	dBestDeltaCost = DBL_MAX;
	nBestGroup1 = -1;
	nBestGroup2 = -1;
	for (nGroup1 = 0; nGroup1 < kwftTarget->GetFrequencyVectorNumber(); nGroup1++)
	{
		for (nGroup2 = nGroup1 + 1; nGroup2 < kwftTarget->GetFrequencyVectorNumber(); nGroup2++)
		{
			// Evaluation du cout apres fusion des groupes
			dCost = ComputeGroupUnionCost(kwftTarget->GetFrequencyVectorAt(nGroup1),
						      kwftTarget->GetFrequencyVectorAt(nGroup2));

			// Memorisation de la meilleure variation de cout
			dDeltaCost = dCost - dvGroupsCosts.GetAt(nGroup1) - dvGroupsCosts.GetAt(nGroup2);
			if (dDeltaCost < dBestDeltaCost)
			{
				dBestDeltaCost = dDeltaCost;
				nBestGroup1 = nGroup1;
				nBestGroup2 = nGroup2;
			}
		}
	}
	assert(nBestGroup1 != -1 and nBestGroup2 != -1);
	assert(nBestGroup1 < nBestGroup2);

	// Creation de la nouvelle table
	kwftNewTarget = new KWFrequencyTable;
	kwftNewTarget->SetFrequencyVectorCreator(GetFrequencyVectorCreator()->Clone());
	kwftNewTarget->SetFrequencyVectorNumber(kwftTarget->GetFrequencyVectorNumber() - 1);
	kwftNewTarget->SetInitialValueNumber(kwftTarget->GetInitialValueNumber());
	kwftNewTarget->SetGranularizedValueNumber(kwftTarget->GetGranularizedValueNumber());
	kwftNewTarget->SetGranularity(kwftTarget->GetGranularity());
	kwftNewTarget->SetGarbageModalityNumber(kwftTarget->GetGarbageModalityNumber());

	// Calcul des index des nouveaux groupes pour chaque ancien groupe
	ivNewGroups.SetSize(kwftTarget->GetFrequencyVectorNumber());
	for (nGroup = 0; nGroup < nBestGroup2; nGroup++)
		ivNewGroups.SetAt(nGroup, nGroup);
	ivNewGroups.SetAt(nBestGroup2, nBestGroup1);
	for (nGroup = nBestGroup2 + 1; nGroup < kwftTarget->GetFrequencyVectorNumber(); nGroup++)
		ivNewGroups.SetAt(nGroup, nGroup - 1);

	// Recopie de l'ancienne table
	InitializeFrequencyVector(kwftNewTarget->GetFrequencyVectorAt(nBestGroup1));
	for (nGroup = 0; nGroup < kwftTarget->GetFrequencyVectorNumber(); nGroup++)
	{
		// Gestion de la fusion pour les groupes sources
		if (nGroup == nBestGroup1 or nGroup == nBestGroup2)
		{
			AddFrequencyVector(kwftNewTarget->GetFrequencyVectorAt(nBestGroup1),
					   kwftTarget->GetFrequencyVectorAt(nGroup));
		}
		// Recopie au nouvel emplacement sinon
		else
		{
			nNewGroup = ivNewGroups.GetAt(nGroup);
			kwftNewTarget->GetFrequencyVectorAt(nNewGroup)->CopyFrom(
			    kwftTarget->GetFrequencyVectorAt(nGroup));
		}
	}

	// Modification du vecteur d'affectation des groupes
	for (i = 0; i < ivGroups->GetSize(); i++)
		ivGroups->SetAt(i, ivNewGroups.GetAt(ivGroups->GetAt(i)));

	ensure(kwftNewTarget->GetTotalFrequency() == kwftTarget->GetTotalFrequency());
}

void DTGrouperMODL::BuildSingletonGrouping(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget) const
{
	int nSource;

	require(kwftSource != NULL);

	// Creation de la table de contingence cible avec un seul groupe
	kwftTarget = new KWFrequencyTable;
	kwftTarget->SetFrequencyVectorCreator(GetFrequencyVectorCreator()->Clone());
	kwftTarget->SetFrequencyVectorNumber(1);
	kwftTarget->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
	kwftTarget->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
	// Parametrage granularite et poubelle
	// Le groupe poubelle est obligatoirement vide pour une table a un seul groupe
	kwftTarget->SetGranularity(kwftSource->GetGranularity());
	kwftTarget->SetGarbageModalityNumber(0);

	InitializeFrequencyVector(kwftTarget->GetFrequencyVectorAt(0));

	// Initialisation a partir de la table source
	for (nSource = 0; nSource < kwftSource->GetFrequencyVectorNumber(); nSource++)
		AddFrequencyVector(kwftTarget->GetFrequencyVectorAt(0), kwftSource->GetFrequencyVectorAt(nSource));

	ensure(kwftTarget->GetTotalFrequency() == kwftSource->GetTotalFrequency());
}

void DTGrouperMODL::PostOptimizeGroupsWithGarbageSearch(ObjectArray* oaInitialGroups, ObjectArray* oaNewGroups,
							IntVector* ivGroups, SortedList* frequencyList) const
{
	boolean bPrintInitialGroups = false;
	boolean bPrintOptimizations = false;
	boolean bPrintFinalTable = false;
	boolean bContinue;
	int nModalityNumber;
	int nGroupNumber;
	int nTargetNumber;
	DoubleVector dvGroupCosts;
	DoubleVector dvGroupOutDeltaCosts;
	DoubleVector dvGroupInDeltaCosts;
	int nModality;
	int nGroup;
	int nCurrentGroup;
	double dCost;
	double dDeltaCost;
	double dBestDeltaCost;
	int nBestModality;
	int nBestGroup;
	int nOutGroup;
	int nInGroup;
	boolean bCheckFrequencies = false;
	int nGarbageModalityNumber;
	int nNewGarbageModalityNumber;
	int nTransferredModalityNumber;
	POSITION position;
	KWMODLGroup* group;
	int nTrueGroupNumber;
	int nNewGroupNumber;
	int nNewGroup;
	IntVector ivNonEmptyGroups;

	require(oaInitialGroups != NULL);
	require(oaNewGroups != NULL);
	require(ivGroups != NULL);
	require(cast(KWMODLGroup*, oaInitialGroups->GetAt(0))->GetFrequencyVector()->GetSize() ==
		cast(KWMODLGroup*, oaNewGroups->GetAt(0))->GetFrequencyVector()->GetSize());
	require(oaInitialGroups->GetSize() >= oaNewGroups->GetSize());
	require(ivGroups->GetSize() == oaInitialGroups->GetSize());
	require(frequencyList != NULL);

	// Arret si un seul groupe
	if (oaNewGroups->GetSize() <= 1)
		return;

	// Affichage des tables initiales
	if (bPrintInitialGroups)
		cout << *oaInitialGroups << endl << *oaNewGroups << endl;

	// Memorisation des nombre de modalites, groupes et classes cibles
	nModalityNumber = oaInitialGroups->GetSize();
	nGroupNumber = oaNewGroups->GetSize();
	nTargetNumber = cast(KWMODLGroup*, oaInitialGroups->GetAt(0))->GetFrequencyVector()->GetSize();
	nTrueGroupNumber = nGroupNumber;

	// Initialisation du nombre de modalites du groupe poubelle
	nGarbageModalityNumber = cast(KWMODLGroup*, frequencyList->GetHead())->GetModalityNumber();
	nNewGarbageModalityNumber = 0;

	// On n'exclut aucun groupe
	// nGarbageGroupIndex = -1;

	// Initialisation des valeurs de groupes
	dvGroupCosts.SetSize(nGroupNumber);
	for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
		dvGroupCosts.SetAt(
		    nGroup, ComputeGroupCost(cast(KWMODLGroup*, oaNewGroups->GetAt(nGroup))->GetFrequencyVector()));

	//  Verification que chaque groupe d'oaNewGroups a meme vecteur de frequences que la somme des vecteurs de
	//  chaque modalite associe a ce groupe
	if (bCheckFrequencies)
	{
		ObjectArray oaCheckGroups;
		cout << "nModality\tnGroup\n";
		oaCheckGroups.SetSize(oaNewGroups->GetSize());
		for (nGroup = 0; nGroup < oaCheckGroups.GetSize(); nGroup++)
		{
			oaCheckGroups.SetAt(nGroup, NULL);
		}
		for (nModality = 0; nModality < nModalityNumber; nModality++)
		{
			// Recherche du groupe de rattachement de la modalite
			nCurrentGroup = ivGroups->GetAt(nModality);

			cout << nModality << "\t" << nCurrentGroup << endl;
			if (oaCheckGroups.GetAt(nCurrentGroup) == NULL)
				oaCheckGroups.SetAt(nCurrentGroup,
						    cast(KWMODLGroup*, oaInitialGroups->GetAt(nModality))->Clone());
			else
				AddFrequencyVector(
				    cast(KWMODLGroup*, oaCheckGroups.GetAt(nCurrentGroup))->GetFrequencyVector(),
				    cast(KWMODLGroup*, oaInitialGroups->GetAt(nModality))->GetFrequencyVector());
		}
		cout << "Comparaison effectif groupes" << endl;
		for (nGroup = 0; nGroup < oaCheckGroups.GetSize(); nGroup++)
		{
			cout << "Cumul par modalite " << endl;
			cast(KWMODLGroup*, oaCheckGroups.GetAt(nGroup))->Write(cout);
			cout << "Groupe optimise " << endl;
			cast(KWMODLGroup*, oaNewGroups->GetAt(nGroup))->Write(cout);
		}
		for (nGroup = 0; nGroup < oaCheckGroups.GetSize(); nGroup++)
		{
			delete cast(KWMODLGroup*, oaCheckGroups.GetAt(nGroup));
		}
	}

	// Initialisation des valeurs de "depart" des groupes
	// Pour chaque modalite source, on evalue le DeltaCout engendre par le depart
	// de son groupe de rattachement
	dvGroupOutDeltaCosts.SetSize(nModalityNumber);
	for (nModality = 0; nModality < nModalityNumber; nModality++)
	{
		// Recherche du groupe de rattachement de la modalite
		nCurrentGroup = ivGroups->GetAt(nModality);

		// Calcul du cout du groupe apres le depart de la modalite,
		// en se basant sur les nouveaux effectifs du groupe
		dCost =
		    ComputeGroupDiffCost(cast(KWMODLGroup*, oaNewGroups->GetAt(nCurrentGroup))->GetFrequencyVector(),
					 cast(KWMODLGroup*, oaInitialGroups->GetAt(nModality))->GetFrequencyVector());

		// Memorisation du DeltaCout
		dvGroupOutDeltaCosts.SetAt(nModality, dCost - dvGroupCosts.GetAt(nCurrentGroup));
	}

	// Initialisation des valeurs d'"arrivee" des groupes
	// Pour chaque modalite source, on evalue le DeltaCout engendre par l'arrivee
	// dans un nouveau groupe de rattachement
	dvGroupInDeltaCosts.SetSize(nModalityNumber * nGroupNumber);

	for (nModality = 0; nModality < nModalityNumber; nModality++)
	{
		// Recherche du groupe de rattachement de la modalite
		nCurrentGroup = ivGroups->GetAt(nModality);

		// Parcours des groupes de rattachement possible
		for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
		{
			// On n'evalue que les nouveaux groupes potentiels
			if (nGroup != nCurrentGroup)
			{
				// Calcul du cout du groupe apres l'arrivee de la modalite,
				// en se basant sur les nouveaux effectifs du groupe
				dCost = ComputeGroupUnionCost(
				    cast(KWMODLGroup*, oaNewGroups->GetAt(nGroup))->GetFrequencyVector(),
				    cast(KWMODLGroup*, oaInitialGroups->GetAt(nModality))->GetFrequencyVector());

				// Memorisation du DeltaCout
				dvGroupInDeltaCosts.SetAt(nModality * nGroupNumber + nGroup,
							  dCost - dvGroupCosts.GetAt(nGroup));
			}
		}
	}

	// Tant qu'il y a amelioration, on continue a chercher les meilleurs
	// deplacement de modalites entre groupes
	bContinue = true;
	bContinue = (nTrueGroupNumber > 3);
	while (bContinue)
	{
		// Recherche de la meilleure amelioration par deplacement de modalite
		dBestDeltaCost = dEpsilon;
		nBestModality = -1;
		nBestGroup = -1;
		nGarbageModalityNumber = cast(KWMODLGroup*, frequencyList->GetHead())->GetModalityNumber();
		for (nModality = 0; nModality < nModalityNumber; nModality++)
		{
			// Recherche du groupe de rattachement de la modalite
			nCurrentGroup = ivGroups->GetAt(nModality);

			// Nombre de modalites deplacees (inclut le cas d'une super modalite en coclustering)
			nTransferredModalityNumber = cast(KWMODLGroup*, oaInitialGroups->GetAt(nModality))
							 ->GetFrequencyVector()
							 ->GetModalityNumber();

			// Recherche du meilleur transfert de groupe
			for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
			{
				// On n'evalue que les nouveaux groupes potentiels
				if (nGroup != nCurrentGroup)
				{
					// Evaluation de la variation de cout globale
					dDeltaCost = dvGroupOutDeltaCosts.GetAt(nModality) +
						     dvGroupInDeltaCosts.GetAt(nModality * nGroupNumber + nGroup);

					// Evolution de la taille du groupe poubelle
					nNewGarbageModalityNumber = nGarbageModalityNumber;
					group = cast(KWMODLGroup*, oaNewGroups->GetAt(nCurrentGroup));
					// Cas ou la modalite part du groupe poubelle
					if (group->GetPosition() == frequencyList->GetHeadPosition())
					{
						nNewGarbageModalityNumber -= nTransferredModalityNumber;

						position = frequencyList->GetHeadPosition();
						group = cast(KWMODLGroup*, frequencyList->GetNext(position));
						group = cast(KWMODLGroup*, frequencyList->GetNext(position));
						// Comparaison avec le nombre de modalites du second groupe par nombre
						// de modalites
						if (nNewGarbageModalityNumber < group->GetModalityNumber())
							nNewGarbageModalityNumber = group->GetModalityNumber();
					}

					// Comparaison avec le nombre de modalites du groupe d'arrivee
					if (nNewGarbageModalityNumber <
					    cast(KWMODLGroup*, oaNewGroups->GetAt(nGroup))->GetModalityNumber() +
						nTransferredModalityNumber)
						nNewGarbageModalityNumber =
						    cast(KWMODLGroup*, oaNewGroups->GetAt(nGroup))
							->GetModalityNumber() +
						    nTransferredModalityNumber;

					// Evolution du nombre de groupes non vides
					nNewGroupNumber = nTrueGroupNumber;
					// Cas d'un groupe vide apres le depart de la modalite
					if (nTransferredModalityNumber ==
					    cast(KWMODLGroup*, oaNewGroups->GetAt(nCurrentGroup))->GetModalityNumber())
						nNewGroupNumber--;
					// Cas de l'arrivee de la modalite dans un groupe vide
					if (cast(KWMODLGroup*, oaNewGroups->GetAt(nGroup))->GetModalityNumber() == 0)
						nNewGroupNumber++;

					// Prise en compte de la variation du cout de partition si la taille de la
					// poubelle a evolue
					if (nGarbageModalityNumber != nNewGarbageModalityNumber or
					    nNewGroupNumber != nTrueGroupNumber)
					{
						// Cas d'un deplacement non envisageable : nombre de groupes trop faible
						// en presence d'un groupe poubelle
						if (nNewGroupNumber < 3)
							dDeltaCost = DBL_MAX;
						else
							dDeltaCost += ComputePartitionCost(nNewGroupNumber,
											   nNewGarbageModalityNumber) -
								      ComputePartitionCost(nTrueGroupNumber,
											   nGarbageModalityNumber);
					}

					// Memorisation si amelioration
					if (dDeltaCost < dBestDeltaCost)
					{
						dBestDeltaCost = dDeltaCost;
						nBestModality = nModality;
						nBestGroup = nGroup;
					}
				}
			}
		}

		// Evaluation de l'interet de l'optimization
		bContinue = dBestDeltaCost < -dEpsilon and nTrueGroupNumber > 3;

		// Si amelioration, mise a jour des calculs de transfert de modalites
		if (bContinue)
		{
			// Affichage du transfert
			if (bPrintOptimizations)
			{
				cout << "Transfert\t" << dBestDeltaCost << "\t" << nBestModality << "\t" << nBestGroup
				     << "\t";
				cast(KWMODLGroup*, oaInitialGroups->GetAt(nBestModality))->WriteLineReport(cout);
				cout << endl;
			}

			// Memorisation des index des groupes de depart et d'arrivee
			nOutGroup = ivGroups->GetAt(nBestModality);
			nInGroup = nBestGroup;

			// Modification du cout des groupes de depart et d'arrivee
			dvGroupCosts.UpgradeAt(nOutGroup, dvGroupOutDeltaCosts.GetAt(nBestModality));
			dvGroupCosts.UpgradeAt(nInGroup,
					       dvGroupInDeltaCosts.GetAt(nBestModality * nGroupNumber + nInGroup));

			// Modification du cout de depart pour la meilleure modalite
			dvGroupOutDeltaCosts.SetAt(nBestModality,
						   -dvGroupInDeltaCosts.GetAt(nBestModality * nGroupNumber + nInGroup));

			if (bCheckFrequencies)
			{
				cout << "Effectifs modalite " << endl;
				cast(KWMODLGroup*, oaInitialGroups->GetAt(nBestModality))->Write(cout);
				cout << "Effectifs groupe depart avant deplacement " << endl;
				cast(KWMODLGroup*, oaNewGroups->GetAt(nOutGroup))->Write(cout);
				cout << "Effectifs groupe arrivee avant deplacement " << endl;
				cast(KWMODLGroup*, oaNewGroups->GetAt(nInGroup))->Write(cout);
			}

			// Retrait des groupes de depart et d'arrivee de la liste des nombres de modalites
			frequencyList->RemoveAt(cast(KWMODLGroup*, oaNewGroups->GetAt(nInGroup))->GetPosition());
			debug(cast(KWMODLGroup*, oaNewGroups->GetAt(nInGroup))->SetPosition(NULL));
			frequencyList->RemoveAt(cast(KWMODLGroup*, oaNewGroups->GetAt(nOutGroup))->GetPosition());
			debug(cast(KWMODLGroup*, oaNewGroups->GetAt(nOutGroup))->SetPosition(NULL));

			// Variation du nombre de groupes
			// Cas ou l'arrivee de la modalite remplit un groupe qui etait vide
			if (cast(KWMODLGroup*, oaNewGroups->GetAt(nInGroup))->GetModalityNumber() == 0)
				nTrueGroupNumber++;

			// Modification des effectifs des groupes de depart et d'arrivee
			AddFrequencyVector(
			    cast(KWMODLGroup*, oaNewGroups->GetAt(nInGroup))->GetFrequencyVector(),
			    cast(KWMODLGroup*, oaInitialGroups->GetAt(nBestModality))->GetFrequencyVector());
			RemoveFrequencyVector(
			    cast(KWMODLGroup*, oaNewGroups->GetAt(nOutGroup))->GetFrequencyVector(),
			    cast(KWMODLGroup*, oaInitialGroups->GetAt(nBestModality))->GetFrequencyVector());
			assert(fabs(ComputeGroupCost(
					cast(KWMODLGroup*, oaNewGroups->GetAt(nOutGroup))->GetFrequencyVector()) -
				    dvGroupCosts.GetAt(nOutGroup)) < dEpsilon);
			assert(fabs(ComputeGroupCost(
					cast(KWMODLGroup*, oaNewGroups->GetAt(nInGroup))->GetFrequencyVector()) -
				    dvGroupCosts.GetAt(nInGroup)) < dEpsilon);

			// Variation du nombre de groupes
			// Cas ou le depart de la modalite laisse un groupe vide
			if (cast(KWMODLGroup*, oaNewGroups->GetAt(nOutGroup))->GetModalityNumber() == 0)
				nTrueGroupNumber--;

			// Reinsertion des groupes de depart et d'arrivee dans la liste des groupes triee par nombre de
			// modalites
			cast(KWMODLGroup*, oaNewGroups->GetAt(nInGroup))
			    ->SetPosition(frequencyList->Add(cast(KWMODLGroup*, oaNewGroups->GetAt(nInGroup))));
			cast(KWMODLGroup*, oaNewGroups->GetAt(nOutGroup))
			    ->SetPosition(frequencyList->Add(cast(KWMODLGroup*, oaNewGroups->GetAt(nOutGroup))));

			if (bCheckFrequencies)
			{
				cout << "Effectifs modalite " << endl;
				cast(KWMODLGroup*, oaInitialGroups->GetAt(nBestModality))->Write(cout);
				cout << "Effectifs groupe depart apres deplacement " << endl;
				cast(KWMODLGroup*, oaNewGroups->GetAt(nOutGroup))->Write(cout);
				cout << "Effectifs groupe arrivee apres deplacement " << endl;
				cast(KWMODLGroup*, oaNewGroups->GetAt(nInGroup))->Write(cout);
			}

			// Memorisation de l'index du nouveau groupe
			ivGroups->SetAt(nBestModality, nInGroup);

			// Modification des couts d'arrivee pour la meilleure modalite
			// Parcours des groupes de rattachement possible
			for (nGroup = 0; nGroup < nGroupNumber; nGroup++)
			{
				// On n'evalue que les nouveaux groupes potentiels
				if (nGroup != nInGroup)
				{
					// Calcul du cout du groupe apres l'arrivee de la modalite,
					// en se basant sur les nouveaux effectifs du groupe
					dCost = ComputeGroupUnionCost(
					    cast(KWMODLGroup*, oaNewGroups->GetAt(nGroup))->GetFrequencyVector(),
					    cast(KWMODLGroup*, oaInitialGroups->GetAt(nBestModality))
						->GetFrequencyVector());

					// Memorisation du DeltaCout
					dvGroupInDeltaCosts.SetAt(nBestModality * nGroupNumber + nGroup,
								  dCost - dvGroupCosts.GetAt(nGroup));
				}
				// Utile ? ?
				// On re-initialise le cout d'arrivee de la modalite dans le groupe ou elle vient
				// d'arriver
				else
				{
					dvGroupInDeltaCosts.SetAt(nBestModality * nGroupNumber + nGroup, 0);
				}
			}

			// Recalcul pour les deux groupes concernes des valeurs d'arrivee des modalites
			for (nModality = 0; nModality < nModalityNumber; nModality++)
			{
				if (nModality == nBestModality)
					continue;

				// Recherche du groupe de rattachement de la modalite
				nCurrentGroup = ivGroups->GetAt(nModality);

				if (bCheckFrequencies)
				{
					cout << "Effectifs modalite " << endl;
					cast(KWMODLGroup*, oaInitialGroups->GetAt(nModality))->Write(cout);
					cout << "Effectifs groupe " << endl;
					cast(KWMODLGroup*, oaNewGroups->GetAt(nCurrentGroup))->Write(cout);
				}

				// Evaluation du cout de depart pour les modalites du groupe de depart ou d'arrivee
				if (nCurrentGroup == nOutGroup or nCurrentGroup == nInGroup)
				{
					// Calcul du cout du groupe apres le depart de la modalite,
					// en se basant sur les nouveaux effectifs du groupe
					dCost = ComputeGroupDiffCost(
					    cast(KWMODLGroup*, oaNewGroups->GetAt(nCurrentGroup))->GetFrequencyVector(),
					    cast(KWMODLGroup*, oaInitialGroups->GetAt(nModality))
						->GetFrequencyVector());

					// Memorisation du DeltaCout
					dvGroupOutDeltaCosts.SetAt(nModality,
								   dCost - dvGroupCosts.GetAt(nCurrentGroup));
				}

				// Evaluation du cout d'arrivee pour les modalites du groupe de depart
				if (nOutGroup != nCurrentGroup)
				{
					// Calcul du cout du groupe apres l'arrivee de la modalite,
					// en se basant sur les nouveaux effectifs du groupe
					dCost = ComputeGroupUnionCost(
					    cast(KWMODLGroup*, oaNewGroups->GetAt(nOutGroup))->GetFrequencyVector(),
					    cast(KWMODLGroup*, oaInitialGroups->GetAt(nModality))
						->GetFrequencyVector());

					// Memorisation du DeltaCout
					dvGroupInDeltaCosts.SetAt(nModality * nGroupNumber + nOutGroup,
								  dCost - dvGroupCosts.GetAt(nOutGroup));
				}

				// Evaluation  du cout d'arrivee pour les modalites du groupe d'arrivee
				if (nInGroup != nCurrentGroup)
				{
					// Calcul du cout du groupe apres l'arrivee de la modalite,
					// en se basant sur les nouveaux effectifs du groupe
					dCost = ComputeGroupUnionCost(
					    cast(KWMODLGroup*, oaNewGroups->GetAt(nInGroup))->GetFrequencyVector(),
					    cast(KWMODLGroup*, oaInitialGroups->GetAt(nModality))
						->GetFrequencyVector());

					// Memorisation du DeltaCout
					dvGroupInDeltaCosts.SetAt(nModality * nGroupNumber + nInGroup,
								  dCost - dvGroupCosts.GetAt(nInGroup));
				}
			}
		}
	}

	// Presence d'au moins un groupe vide
	if (nTrueGroupNumber < nGroupNumber)
	{
		// Recopie des vecteurs
		ivNonEmptyGroups.SetSize(oaNewGroups->GetSize());
		nNewGroup = 0;
		for (nGroup = 0; nGroup < oaNewGroups->GetSize(); nGroup++)
		{
			// Pas de changement si vecteur non vide
			if (cast(KWMODLGroup*, oaNewGroups->GetAt(nGroup))->GetModalityNumber() > 0)
			{
				oaNewGroups->SetAt(nNewGroup, cast(KWMODLGroup*, oaNewGroups->GetAt(nGroup)));
				ivNonEmptyGroups.SetAt(nGroup, nNewGroup);
				nNewGroup++;
			}
			// Sinon, supression du vecteur
			else
			{
				delete cast(KWMODLGroup*, oaNewGroups->GetAt(nGroup));
				ivNonEmptyGroups.SetAt(nGroup, -1);
			}
		}

		// Retaillage du tableau
		oaNewGroups->SetSize(nNewGroup);

		// Mise a jour des index
		for (nGroup = 0; nGroup < ivGroups->GetSize(); nGroup++)
		{
			nNewGroup = ivNonEmptyGroups.GetAt(ivGroups->GetAt(nGroup));
			assert(-1 <= nNewGroup and nNewGroup <= ivGroups->GetAt(nGroup));
			if (nNewGroup == -1)
				nNewGroup = 0;
			ivGroups->SetAt(nGroup, nNewGroup);
		}
	}

	// Affichage de la table finale
	if (bPrintFinalTable)
		cout << *oaInitialGroups << endl;

	//  Verification que chaque groupe d'oaNewGroups a meme vecteur de frequences que la somme des vecteurs de
	//  chaque modalite associe a ce groupe
	if (bCheckFrequencies)
	{
		ObjectArray oaCheckGroups;
		cout << "Fin PostOptimizeGroupsWithGarbageSearch " << endl;
		cout << "nModality\tnGroup\n";
		oaCheckGroups.SetSize(oaNewGroups->GetSize());
		for (nGroup = 0; nGroup < oaCheckGroups.GetSize(); nGroup++)
		{
			oaCheckGroups.SetAt(nGroup, NULL);
		}
		for (nModality = 0; nModality < nModalityNumber; nModality++)
		{
			// Recherche du groupe de rattachement de la modalite
			nCurrentGroup = ivGroups->GetAt(nModality);

			cout << nModality << "\t" << nCurrentGroup << endl;
			if (oaCheckGroups.GetAt(nCurrentGroup) == NULL)
				oaCheckGroups.SetAt(nCurrentGroup,
						    cast(KWMODLGroup*, oaInitialGroups->GetAt(nModality))->Clone());
			else
				AddFrequencyVector(
				    cast(KWMODLGroup*, oaCheckGroups.GetAt(nCurrentGroup))->GetFrequencyVector(),
				    cast(KWMODLGroup*, oaInitialGroups->GetAt(nModality))->GetFrequencyVector());
		}
		cout << "Comparaison effectif groupes" << endl;
		for (nGroup = 0; nGroup < oaCheckGroups.GetSize(); nGroup++)
		{
			cout << "Cumul par modalite " << endl;
			cast(KWMODLGroup*, oaCheckGroups.GetAt(nGroup))->Write(cout);
			cout << "Groupe optimise " << endl;
			cast(KWMODLGroup*, oaNewGroups->GetAt(nGroup))->Write(cout);
		}
		for (nGroup = 0; nGroup < oaCheckGroups.GetSize(); nGroup++)
		{
			delete cast(KWMODLGroup*, oaCheckGroups.GetAt(nGroup));
		}
	}

	// ensure(kwftSource->GetFrequencyVectorSize() == kwftTarget->GetFrequencyVectorSize());
}

void DTGrouperMODL::PostOptimizeGroupingWithGarbageSearch(ObjectArray* oaInitialGroups, ObjectArray*& oaNewGroups,
							  IntVector* ivGroups, KWFrequencyTable*& kwftSource,
							  KWFrequencyTable*& kwftTarget) const
{
	boolean bPrintGroupNumbers = false;
	const int nMaxTestedForcedMergeNumber = 3;
	int nTestedForcedMergeNumber;
	boolean bSingletonGroupingTested;
	IntVector* ivExhaustiveMergeGroups;
	IntVector* ivExhaustiveMergeGroupsWithGarbage;
	IntVector ivNewGroups;
	IntVector* ivGroupsWithoutGarbage;
	IntVector* ivGroupsWithGarbage;
	IntVector ivNewGroupsWithGarbage;
	KWFrequencyTable* kwftTableFromInitialGroups;
	KWFrequencyTable* kwftCurrentTarget;
	KWFrequencyTable* kwftNewTarget;
	KWFrequencyTable* kwftTargetWithGarbage;
	KWFrequencyTable* kwftNewTargetWithGarbage;
	ObjectArray* oaCurrentGroups;
	ObjectArray* oaExhaustiveMergeGroupsWithoutGarbage;
	ObjectArray* oaExhaustiveMergeGroupsWithGarbage;
	double dBestCost;
	double dNewCost;
	double dBestCostWithGarbage;
	double dNewCostWithGarbage;
	int nSource;
	int nGarbageModalityNumber;
	SortedList frequencyList(KWMODLGroupModalityNumberCompare);
	KWMODLGroup* group;
	boolean dDisplayModalityNumber = false;

	require(oaInitialGroups != NULL);
	require(oaNewGroups != NULL);
	require(ivGroups != NULL);
	require(oaInitialGroups->GetSize() >= oaNewGroups->GetSize());
	require(ivGroups->GetSize() == oaInitialGroups->GetSize());

	// Arret si un seul groupe
	if (oaNewGroups->GetSize() <= 1)
	{
		kwftTarget = BuildTable(oaNewGroups);

		// Parametrage nInitialValueNumber, nGranularizedValueNumber
		kwftTarget->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
		kwftTarget->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());

		// Parametrage granularite et poubelle
		kwftTarget->SetGranularity(kwftSource->GetGranularity());
		kwftTarget->SetGarbageModalityNumber(0);
		return;
	}

	// Statistiques initiales et nombre de groupes initiaux
	if (bPrintGroupNumbers)
		cout << // kwftSource->ComputeTotalFrequency() << "\t" <<
		    oaInitialGroups->GetSize() << "\t" << oaNewGroups->GetSize() << "\t";

	// Initialisation
	kwftTargetWithGarbage = NULL;
	oaExhaustiveMergeGroupsWithoutGarbage = NULL;
	oaExhaustiveMergeGroupsWithGarbage = NULL;
	ivExhaustiveMergeGroups = NULL;
	ivExhaustiveMergeGroupsWithGarbage = NULL;
	ivGroupsWithGarbage = ivGroups->Clone();
	ivGroupsWithoutGarbage = ivGroups->Clone();
	// Algorithme ExhaustiveMerge
	// S'il reste beaucoup de groupes, on realise dans une premiere etape
	// un algorithme ExhaustiveMerge en O(n.log(n)) afin de reduire
	// rapidement le nombre de groupes
	// On se limite a MinGroupNumber=nMaxTestedForcedMergeNumber+2, car
	// l'etape suivante ira potentiellement jusqu'a MinGroupNumber=2 en
	// nMaxTestedForcedMergeNumber iteration, et l'etape finale teste
	// les groupes singletons
	if (oaNewGroups->GetSize() > nMaxTestedForcedMergeNumber + 2)
	{
		// Mise a jour des index des groupes selon le tableau oaNewGroups
		// cout << "Index groupe avant Maj \t Apres MaJ \n";
		for (nSource = 0; nSource < oaNewGroups->GetSize(); nSource++)
		{
			group = cast(KWMODLGroup*, oaNewGroups->GetAt(nSource));
			group->SetIndex(ivGroups->GetAt(group->GetIndex()));
		}

		// Suivi du nombre de modalites par groupe
		if (dDisplayModalityNumber)
		{
			cout << "Nombre de modalites par groupe avant ExhaustiveMergeGrouping : " << endl;
			for (nSource = 0; nSource < oaNewGroups->GetSize(); nSource++)
			{
				group = cast(KWMODLGroup*, oaNewGroups->GetAt(nSource));
				cout << nSource << "\t" << group->GetModalityNumber() << endl;
			}
		}

		// Optimisation de la table
		ExhaustiveMergeGroupingWithGarbageSearch(
		    nMaxTestedForcedMergeNumber + 2, oaNewGroups, oaExhaustiveMergeGroupsWithoutGarbage,
		    ivExhaustiveMergeGroups, oaExhaustiveMergeGroupsWithGarbage, ivExhaustiveMergeGroupsWithGarbage);

		if (dDisplayModalityNumber)
		{
			cout << "Nombre de modalites par groupe SANS poubelle apres ExhaustiveMergeGrouping : " << endl;
			for (nSource = 0; nSource < oaExhaustiveMergeGroupsWithoutGarbage->GetSize(); nSource++)
			{
				group = cast(KWMODLGroup*, oaExhaustiveMergeGroupsWithoutGarbage->GetAt(nSource));
				cout << nSource << "\t" << group->GetModalityNumber() << endl;
			}

			cout << "Nombre de modalites par groupe AVEC poubelle apres ExhaustiveMergeGrouping : " << endl;
			for (nSource = 0; nSource < oaExhaustiveMergeGroupsWithGarbage->GetSize(); nSource++)
			{
				group = cast(KWMODLGroup*, oaExhaustiveMergeGroupsWithGarbage->GetAt(nSource));
				cout << nSource << "\t" << group->GetModalityNumber() << endl;
			}
		}

		// Recalcul des index des groupes
		for (nSource = 0; nSource < ivGroupsWithoutGarbage->GetSize(); nSource++)
			ivGroupsWithoutGarbage->SetAt(nSource,
						      ivExhaustiveMergeGroups->GetAt(ivGroups->GetAt(nSource)));

		// Recalcul des index des groupes de la partition avec poubelle
		for (nSource = 0; nSource < ivGroupsWithGarbage->GetSize(); nSource++)
			ivGroupsWithGarbage->SetAt(nSource,
						   ivExhaustiveMergeGroupsWithGarbage->GetAt(ivGroups->GetAt(nSource)));

		// Nettoyage
		delete ivExhaustiveMergeGroups;
		ivExhaustiveMergeGroups = NULL;
		if (ivExhaustiveMergeGroupsWithGarbage->GetSize() > 0)
			delete ivExhaustiveMergeGroupsWithGarbage;
	}

	// Post-optimisation de la partition SANS poubelle
	// S'il y a eu reduction du nombre de groupes
	if (oaExhaustiveMergeGroupsWithoutGarbage != NULL)
		oaNewGroups = oaExhaustiveMergeGroupsWithoutGarbage;

	// La suite de la post-optimisation est effectuee sur des tables de contingences
	// (on perd le nombre de modalites par groupe mais on ne revient plus par la suite
	// sur une evaluation avec groupe poubelle pour cette branche)
	kwftTableFromInitialGroups = BuildTable(oaInitialGroups);
	kwftTarget = BuildTable(oaNewGroups);
	// Parametrage granularite et poubelle
	kwftTableFromInitialGroups->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
	kwftTableFromInitialGroups->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
	kwftTableFromInitialGroups->SetGranularity(kwftSource->GetGranularity());
	kwftTableFromInitialGroups->SetGarbageModalityNumber(0);
	kwftTarget->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
	kwftTarget->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
	kwftTarget->SetGranularity(kwftSource->GetGranularity());
	kwftTarget->SetGarbageModalityNumber(0);

	// Initialisation du flag de groupage en un seul groupe
	bSingletonGroupingTested = nMaxTestedForcedMergeNumber == 1 or kwftTarget->GetFrequencyVectorNumber() == 1;

	// Nombre de groupes apres ExhaustiveMerge
	if (bPrintGroupNumbers)
		cout << kwftTarget->GetFrequencyVectorNumber() << "\t";

	// Initialisation de la meilleure solution, en post-optimisant la solution initiale
	if (kwftTarget->GetFrequencyVectorNumber() > 1)
		PostOptimizeGroups(kwftTableFromInitialGroups, kwftTarget, ivGroupsWithoutGarbage);

	// Initialisation de kwftTarget
	// Memorisation de son cout
	dBestCost = ComputePartitionGlobalCost(kwftTarget);

	// Recherche iterative d'ameliorations par fusion de groupes
	nTestedForcedMergeNumber = 0;
	if (kwftTarget->GetFrequencyVectorNumber() > 1 and nTestedForcedMergeNumber < nMaxTestedForcedMergeNumber)
	{
		// Initialisation des donnees de travail
		kwftCurrentTarget = kwftTarget->Clone();
		ivNewGroups.CopyFrom(ivGroupsWithoutGarbage);

		// On force les fusions et on memorise la meilleure solution rencontree,
		// en limitant le nombre maximum d'etapes successives infructueuses
		while (kwftCurrentTarget->GetFrequencyVectorNumber() > 1 and
		       nTestedForcedMergeNumber < nMaxTestedForcedMergeNumber)
		{
			nTestedForcedMergeNumber++;

			// On force la meilleure fusion de groupe
			ForceBestGroupMerge(kwftTableFromInitialGroups, kwftCurrentTarget, kwftNewTarget, &ivNewGroups);

			// On memorise le fait de tester le groupage en un seul groupe
			if (kwftNewTarget->GetFrequencyVectorNumber() == 1)
				bSingletonGroupingTested = true;

			// Post-optimisation de ce nouveau groupage
			PostOptimizeGroups(kwftTableFromInitialGroups, kwftNewTarget, &ivNewGroups);

			// Evaluation du cout correspondant
			dNewCost = ComputePartitionGlobalCost(kwftNewTarget);

			// Memorisation si amelioration
			if (dNewCost < dBestCost + dEpsilon)
			{
				dBestCost = dNewCost;

				// Recopie de la solution
				ivGroupsWithoutGarbage->CopyFrom(&ivNewGroups);
				kwftTarget->CopyFrom(kwftNewTarget);

				// Remise a zero du compteur d'etapes infructueuses
				nTestedForcedMergeNumber = 0;
			}

			// On change de table de contingence courante, et on nettoie les donnees
			// devenues inutiles
			delete kwftCurrentTarget;
			kwftCurrentTarget = kwftNewTarget;
		}

		// Nettoyage
		delete kwftCurrentTarget;
	}
	assert(kwftTableFromInitialGroups->GetTotalFrequency() == kwftTarget->GetTotalFrequency());

	// Nombre de groupes apres les ameliorations par fusions iteratives
	if (bPrintGroupNumbers)
		cout << kwftTarget->GetFrequencyVectorNumber() << "\t";

	// Post optimisation de la partition AVEC poubelle
	if (oaExhaustiveMergeGroupsWithGarbage != NULL)
	{
		// Nettoyage
		for (nSource = 0; nSource < oaNewGroups->GetSize(); nSource++)
			delete cast(KWMODLGroup*, oaNewGroups->GetAt(nSource));
		delete oaNewGroups;

		// Initialisation des groupes selon la partition avec poubelle
		oaNewGroups = oaExhaustiveMergeGroupsWithGarbage;
	}

	// Initialisation du meilleur cout avec poubelle
	dBestCostWithGarbage = dBestCost + dEpsilon;

	// Post-optimisation si le nombre de groupes est >=3 (sinon pas de groupe poubelle viable)
	if (oaNewGroups->GetSize() > 2)
	{
		// Initialisation du flag de groupage en un seul groupe
		if (!bSingletonGroupingTested)
			bSingletonGroupingTested = oaNewGroups->GetSize() == 1;

		// Nombre de groupes apres ExhaustiveMerge
		if (bPrintGroupNumbers)
			cout << oaNewGroups->GetSize() << "\t";

		// Initialisation de la liste triee des groupes selon le nombre de modalites
		for (nSource = 0; nSource < oaNewGroups->GetSize(); nSource++)
			cast(KWMODLGroup*, oaNewGroups->GetAt(nSource))
			    ->SetPosition(frequencyList.Add(cast(KWMODLGroup*, oaNewGroups->GetAt(nSource))));
		if (dDisplayModalityNumber)
		{
			cout << "PostOptimizeGroupingWithGarbageSearch::Liste triee avant la post-optimisation de la "
				"table avec poubelle "
			     << endl;
			cout << frequencyList;
		}

		// Initialisation de la meilleure solution, en post-optimisant la solution initiale
		if (oaNewGroups->GetSize() > 1)
			PostOptimizeGroupsWithGarbageSearch(oaInitialGroups, oaNewGroups, ivGroupsWithGarbage,
							    &frequencyList);

		// Dans le cas ou la table est reduite a un groupe + le groupe poubelle
		if (oaNewGroups->GetSize() == 2)
		{
			// Fusion des deux groupes
			cout << " un seul groupe informatif hors poubelle" << endl;
		}

		// Taille de la poubelle (le groupe qui contient le plus de modalites)
		nGarbageModalityNumber = cast(KWMODLGroup*, frequencyList.GetHead())->GetModalityNumber();
		// cout << " nGarbageModalityNumber " << nGarbageModalityNumber << endl;

		// Initialisation de kwftTargetWithGarbage
		kwftTargetWithGarbage = BuildTable(oaNewGroups);
		// Parametrage granularite et poubelle
		kwftTargetWithGarbage->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
		kwftTargetWithGarbage->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
		kwftTargetWithGarbage->SetGranularity(kwftSource->GetGranularity());
		kwftTargetWithGarbage->SetGarbageModalityNumber(nGarbageModalityNumber);

		// Memorisation de son cout
		dBestCostWithGarbage = ComputePartitionGlobalCost(kwftTargetWithGarbage);

		// cout << "Recherche iterative d'ameliorations par fusion de groupes " << endl;
		//  Recherche iterative d'ameliorations par fusion de groupes
		nTestedForcedMergeNumber = 0;
		if (kwftTargetWithGarbage->GetFrequencyVectorNumber() > 3 and
		    nTestedForcedMergeNumber < nMaxTestedForcedMergeNumber)
		{
			// Ne devrait pas etre sur les tables mais sur tableaux de groupe
			// Initialisation des donnees de travail
			// kwftCurrentTarget = kwftTargetWithGarbage->Clone();
			ivNewGroupsWithGarbage.CopyFrom(ivGroupsWithGarbage);

			oaCurrentGroups = new ObjectArray;
			for (nSource = 0; nSource < oaNewGroups->GetSize(); nSource++)
			{
				oaCurrentGroups->Add(cast(KWMODLGroup*, oaNewGroups->GetAt(nSource))->Clone());
			}

			// On force les fusions et on memorise la meilleure solution rencontree,
			// en limitant le nombre maximum d'etapes successives infructueuses
			// Le critere d'arret sur la taille de la table source est adapte selon la presence d'un groupe
			// poubelle
			while (oaCurrentGroups->GetSize() > 3 and
			       nTestedForcedMergeNumber < nMaxTestedForcedMergeNumber)
			{
				nTestedForcedMergeNumber++;

				for (nSource = 0; nSource < oaNewGroups->GetSize(); nSource++)
				{
					delete cast(KWMODLGroup*, oaNewGroups->GetAt(nSource));
				}
				delete oaNewGroups;

				// Re-initialisation de la liste des groupes par nombre de modalites a partir du tableau
				// oaCurrentGroups
				frequencyList.RemoveAll();
				for (nSource = 0; nSource < oaCurrentGroups->GetSize(); nSource++)
					cast(KWMODLGroup*, oaCurrentGroups->GetAt(nSource))
					    ->SetPosition(
						frequencyList.Add(cast(KWMODLGroup*, oaCurrentGroups->GetAt(nSource))));

				// cout << "Affichage liste avant ForceBestGroupMergeWGGI " << endl;
				// cout << frequencyList;

				// On force la meilleure fusion de groupe en prenant en compte la variation de cout de
				// partition et l'evolution eventuelle de la taille du groupe poubelle a l'issue du
				// Merge
				ForceBestGroupMergeWithGarbageSearch(oaInitialGroups, oaCurrentGroups, oaNewGroups,
								     &ivNewGroupsWithGarbage, &frequencyList);

				// On memorise le fait de tester le groupage en un seul groupe
				if (oaNewGroups->GetSize() == 1)
					bSingletonGroupingTested = true;

				// Re-initialisation de la liste des groupes par nombre de modalites a partir du tableau
				// oaNewGroups
				frequencyList.RemoveAll();
				for (nSource = 0; nSource < oaNewGroups->GetSize(); nSource++)
					cast(KWMODLGroup*, oaNewGroups->GetAt(nSource))
					    ->SetPosition(
						frequencyList.Add(cast(KWMODLGroup*, oaNewGroups->GetAt(nSource))));

				// Post-optimisation de ce nouveau groupage
				PostOptimizeGroupsWithGarbageSearch(oaInitialGroups, oaNewGroups,
								    &ivNewGroupsWithGarbage, &frequencyList);

				// oaCurrentGroups devient oaNewGroups
				for (nSource = 0; nSource < oaCurrentGroups->GetSize(); nSource++)
				{
					delete cast(KWMODLGroup*, oaCurrentGroups->GetAt(nSource));
				}
				delete oaCurrentGroups;
				oaCurrentGroups = new ObjectArray;
				for (nSource = 0; nSource < oaNewGroups->GetSize(); nSource++)
				{
					oaCurrentGroups->Add(cast(KWMODLGroup*, oaNewGroups->GetAt(nSource))->Clone());
				}

				// Nombre de modalites du groupe poubelle
				nGarbageModalityNumber =
				    cast(KWMODLGroup*, frequencyList.GetHead())->GetModalityNumber();

				// Initialisation de kwftTargetWithGarbage
				kwftNewTargetWithGarbage = BuildTable(oaNewGroups);
				kwftNewTargetWithGarbage->SetInitialValueNumber(kwftSource->GetInitialValueNumber());
				kwftNewTargetWithGarbage->SetGranularizedValueNumber(
				    kwftSource->GetGranularizedValueNumber());
				// Parametrage granularite et poubelle
				kwftNewTargetWithGarbage->SetGranularity(kwftSource->GetGranularity());
				kwftNewTargetWithGarbage->SetGarbageModalityNumber(nGarbageModalityNumber);

				// Evaluation du cout correspondant
				dNewCostWithGarbage = ComputePartitionGlobalCost(kwftNewTargetWithGarbage);

				// Memorisation si amelioration
				if (dNewCostWithGarbage < dBestCostWithGarbage + dEpsilon)
				{
					dBestCostWithGarbage = dNewCostWithGarbage;

					// Recopie de la solution
					ivGroupsWithGarbage->CopyFrom(&ivNewGroupsWithGarbage);
					kwftTargetWithGarbage->CopyFrom(kwftNewTargetWithGarbage);

					// Remise a zero du compteur d'etapes infructueuses
					nTestedForcedMergeNumber = 0;
				}
				delete kwftNewTargetWithGarbage;
			}

			// Nettoyage
			for (nSource = 0; nSource < oaCurrentGroups->GetSize(); nSource++)
			{
				delete cast(KWMODLGroup*, oaCurrentGroups->GetAt(nSource));
			}
			delete oaCurrentGroups;
		}
		assert(kwftTableFromInitialGroups->GetTotalFrequency() == kwftTargetWithGarbage->GetTotalFrequency());

		// Nombre de groupes apres les ameliorations par fusions iteratives
		if (bPrintGroupNumbers)
			cout << kwftTargetWithGarbage->GetFrequencyVectorNumber() << "\t";
	}

	// Nettoyage
	for (nSource = 0; nSource < oaNewGroups->GetSize(); nSource++)
	{
		delete cast(KWMODLGroup*, oaNewGroups->GetAt(nSource));
		oaNewGroups->SetAt(nSource, NULL);
	}

	// Choix de la meilleure partition (avec ou sans poubelle)
	// On peut alors retourner une table car il faut des tables pour calculer ComputePartitionGlobalCost
	if (dBestCostWithGarbage < dBestCost + dEpsilon)
	{
		// La nouvelle table cible remplace la precedente
		kwftTarget->CopyFrom(kwftTargetWithGarbage);

		// Recalcul des index des groupes
		for (nSource = 0; nSource < ivGroups->GetSize(); nSource++)
			ivGroups->SetAt(nSource, ivGroupsWithGarbage->GetAt(nSource));
	}
	else
	{
		// Recalcul des index des groupes
		for (nSource = 0; nSource < ivGroups->GetSize(); nSource++)
			ivGroups->SetAt(nSource, ivGroupsWithoutGarbage->GetAt(nSource));
	}

	if (kwftTargetWithGarbage != NULL)
		delete kwftTargetWithGarbage;

	// Test final: comparaison avec une solution de groupage en un seul groupe
	if (not bSingletonGroupingTested)
	{
		assert(kwftTarget->GetFrequencyVectorNumber() > 1);

		// Optimisation de la table
		BuildSingletonGrouping(kwftTarget, kwftCurrentTarget);

		// Evaluation du cout correspondant
		dNewCost = ComputePartitionGlobalCost(kwftCurrentTarget);

		// Memorisation si amelioration
		if (dNewCost < dBestCost + dEpsilon)
		{
			// La nouvelle table cible remplace la precedente
			kwftTarget->CopyFrom(kwftCurrentTarget);

			// Recalcul des index des groupes
			for (nSource = 0; nSource < ivGroups->GetSize(); nSource++)
				ivGroups->SetAt(nSource, 0);
		}

		// Nettoyage
		delete kwftCurrentTarget;
	}

	// Nombre de groupes final
	if (bPrintGroupNumbers)
		cout << kwftTarget->GetFrequencyVectorNumber() << endl;

	delete ivGroupsWithGarbage;
	delete ivGroupsWithoutGarbage;

	ensure(kwftTableFromInitialGroups->GetTotalFrequency() == kwftTarget->GetTotalFrequency());
	ensure(ivGroups->GetSize() == kwftTableFromInitialGroups->GetFrequencyVectorNumber());

	delete kwftTableFromInitialGroups;
}

void DTGrouperMODL::ExhaustiveMergeGroupingWithGarbageSearch(int nMinGroupNumber, ObjectArray* oaInitialGroups,
							     ObjectArray*& oaNewGroupsWithoutGarbage,
							     IntVector*& ivGroupsWithoutGarbage,
							     ObjectArray*& oaNewGroupsWithGarbage,
							     IntVector*& ivGroupsWithGarbage) const
{
	ObjectArray* oaInitialGroupMerges;
	ObjectArray* oaNewGroups;
	int nOptimumGroupNumberWithoutGarbage;
	int nOptimumGroupNumberWithGarbage;
	int nSource;
	IntVector* ivOptimumNumbers;
	IntVector* ivGroups;
	IntVector* ivSecondGroups;

	require(oaInitialGroups != NULL);
	require(1 <= nMinGroupNumber and nMinGroupNumber <= oaInitialGroups->GetSize());

	///////////////////////////////////////////////////////////////////
	// Premiere etape: determination du nombre de groupes optimum en
	// memorisant le meilleur groupage rencontre au cours d'une suite
	// de merges forces

	// Initialisation des merges
	oaInitialGroupMerges = BuildGroupMerges(oaInitialGroups);

	// Recherche du nombre optimal de groupes sans et avec poubelle
	ivOptimumNumbers = OptimizeGroupsWithGarbageSearch(nMinGroupNumber, oaInitialGroups, oaInitialGroupMerges,
							   oaNewGroups, ivGroups);

	// Nettoyage des donnees de travail initiales
	delete oaInitialGroupMerges;

	nOptimumGroupNumberWithoutGarbage = ivOptimumNumbers->GetAt(0);
	nOptimumGroupNumberWithGarbage = ivOptimumNumbers->GetAt(1);

	////////////////////////////////////////////////////////////////
	// Cas 1 : on recalcule le groupage avec le nombre de
	// groupes optimaux SANS puis AVEC poubelle
	if (nOptimumGroupNumberWithoutGarbage > nOptimumGroupNumberWithGarbage)
	{
		// Initialisation des merges
		oaInitialGroupMerges = BuildGroupMerges(oaInitialGroups);

		// Construction du nombre optimal de groupes
		OptimizeGroups(nOptimumGroupNumberWithoutGarbage, oaInitialGroups, oaInitialGroupMerges,
			       oaNewGroupsWithoutGarbage, ivGroupsWithoutGarbage);

		// Nettoyage des donnees de travail initiales
		delete oaInitialGroupMerges;

		// Initialisation du tableau des groupes initiaux
		delete oaInitialGroups;
		oaInitialGroups = new ObjectArray;
		for (nSource = 0; nSource < oaNewGroupsWithoutGarbage->GetSize(); nSource++)
		{
			oaInitialGroups->Add(cast(KWMODLGroup*, oaNewGroupsWithoutGarbage->GetAt(nSource))->Clone());

			// Mise a jour des index des groupes
			cast(KWMODLGroup*, oaInitialGroups->GetAt(nSource))->SetIndex(nSource);
		}

		// Initialisation des merges
		oaInitialGroupMerges = BuildGroupMerges(oaInitialGroups);

		// Construction du nombre optimal de groupes
		OptimizeGroups(nOptimumGroupNumberWithGarbage, oaInitialGroups, oaInitialGroupMerges,
			       oaNewGroupsWithGarbage, ivSecondGroups);

		ivGroupsWithGarbage = new IntVector;
		for (nSource = 0; nSource < ivGroupsWithoutGarbage->GetSize(); nSource++)
			ivGroupsWithGarbage->Add(ivSecondGroups->GetAt(ivGroupsWithoutGarbage->GetAt(nSource)));
		delete ivSecondGroups;

		// Nettoyage des donnees de travail initiales
		delete oaInitialGroups;
		delete oaInitialGroupMerges;
	}

	////////////////////////////////////////////////////////////////
	// Cas 2 : on recalcule le groupage avec le nombre de
	// groupes optimaux AVEC puis SANS poubelle
	else
	{
		// Initialisation des merges
		oaInitialGroupMerges = BuildGroupMerges(oaInitialGroups);

		// Construction du nombre optimal de groupes
		OptimizeGroups(nOptimumGroupNumberWithGarbage, oaInitialGroups, oaInitialGroupMerges,
			       oaNewGroupsWithGarbage, ivGroupsWithGarbage);

		// Nettoyage des donnees de travail initiales
		delete oaInitialGroupMerges;

		// Initialisation du table des groupes initiaux
		delete oaInitialGroups;
		oaInitialGroups = new ObjectArray;

		for (nSource = 0; nSource < oaNewGroupsWithGarbage->GetSize(); nSource++)
		{
			KWMODLGroup* group = cast(KWMODLGroup*, oaNewGroupsWithGarbage->GetAt(nSource))->Clone();
			group->SetIndex(ivGroupsWithGarbage->GetAt(group->GetIndex()));
			oaInitialGroups->Add(group);
		}

		// Initialisation des merges
		oaInitialGroupMerges = BuildGroupMerges(oaInitialGroups);

		// Construction du nombre optimal de groupes
		OptimizeGroups(nOptimumGroupNumberWithoutGarbage, oaInitialGroups, oaInitialGroupMerges,
			       oaNewGroupsWithoutGarbage, ivSecondGroups);

		ivGroupsWithoutGarbage = new IntVector;
		for (nSource = 0; nSource < ivGroupsWithGarbage->GetSize(); nSource++)
			ivGroupsWithoutGarbage->Add(ivSecondGroups->GetAt(ivGroupsWithGarbage->GetAt(nSource)));
		delete ivSecondGroups;

		// Nettoyage des donnees de travail initiales
		delete oaInitialGroups;
		delete oaInitialGroupMerges;
	}

	// Nettoyage
	delete ivGroups;
	delete ivOptimumNumbers;
	for (nSource = 0; nSource < oaNewGroups->GetSize(); nSource++)
		delete cast(KWMODLGroup*, oaNewGroups->GetAt(nSource));
	delete oaNewGroups;

	ensure(oaNewGroupsWithGarbage->GetSize() >= nMinGroupNumber);
	ensure(oaNewGroupsWithoutGarbage->GetSize() >= nMinGroupNumber);
}

void DTGrouperMODL::ForceBestGroupMergeWithGarbageSearch(ObjectArray* oaSource, ObjectArray* oaTarget,
							 ObjectArray*& oaNewTarget, IntVector* ivGroups,
							 SortedList* frequencyList) const
{
	DoubleVector dvGroupsCosts;
	IntVector ivNewGroups;
	int i;
	int nGroup;
	int nGroup1;
	int nGroup2;
	int nNewGroup;
	double dCost;
	double dDeltaCost;
	double dBestDeltaCost;
	int nBestGroup1;
	int nBestGroup2;
	KWMODLGroup* group;
	int nGroupNumber;
	int nGarbageModalityNumber;
	int nMergeGarbageModalityNumber;
	POSITION position;
	boolean bPrintValueNumberList = false;

	require(oaSource != NULL);
	require(oaTarget != NULL);
	require(ivGroups != NULL);
	require(cast(KWMODLGroup*, oaSource->GetAt(0))->GetFrequencyVector()->GetSize() ==
		cast(KWMODLGroup*, oaTarget->GetAt(0))->GetFrequencyVector()->GetSize());
	require(oaSource->GetSize() >= oaTarget->GetSize());
	require(ivGroups->GetSize() == oaSource->GetSize());
	require(oaTarget->GetSize() >= 2);

	if (bPrintValueNumberList)
	{
		cout << "ForceBestGroupMergeWGI : Affichage liste au debut " << endl;
		cout << "Taille de la liste " << frequencyList->GetCount() << endl;

		position = frequencyList->GetHeadPosition();
		while (position != NULL)
		{
			group = cast(KWMODLGroup*, frequencyList->GetNext(position));
			cout << *group << endl;
		}
	}

	nGroupNumber = oaTarget->GetSize();

	// Calcul des couts des groupes
	dvGroupsCosts.SetSize(nGroupNumber);
	for (nGroup = 0; nGroup < dvGroupsCosts.GetSize(); nGroup++)
		dvGroupsCosts.SetAt(
		    nGroup, ComputeGroupCost(cast(KWMODLGroup*, oaTarget->GetAt(nGroup))->GetFrequencyVector()));

	// Evaluation de toutes les fusions
	dBestDeltaCost = DBL_MAX;
	nBestGroup1 = -1;
	nBestGroup2 = -1;
	// Initialisation de la taille du groupe poubelle
	nGarbageModalityNumber = cast(KWMODLGroup*, frequencyList->GetHead())->GetModalityNumber();

	for (nGroup1 = 0; nGroup1 < nGroupNumber; nGroup1++)
	{
		for (nGroup2 = nGroup1 + 1; nGroup2 < nGroupNumber; nGroup2++)
		{
			// Evaluation du cout apres fusion des groupes
			dCost =
			    ComputeGroupUnionCost(cast(KWMODLGroup*, oaTarget->GetAt(nGroup1))->GetFrequencyVector(),
						  cast(KWMODLGroup*, oaTarget->GetAt(nGroup2))->GetFrequencyVector());

			// Memorisation de la meilleure variation de cout
			dDeltaCost = dCost - dvGroupsCosts.GetAt(nGroup1) - dvGroupsCosts.GetAt(nGroup2);

			// Nombre de modalites du groupe merge
			nMergeGarbageModalityNumber =
			    cast(KWMODLGroup*, oaTarget->GetAt(nGroup1))->GetFrequencyVector()->GetModalityNumber() +
			    cast(KWMODLGroup*, oaTarget->GetAt(nGroup2))->GetFrequencyVector()->GetModalityNumber();
			// Prise en compte de la variation du cout de partition lors du Merge
			// Si evolution du groupe poubelle
			if (nMergeGarbageModalityNumber > nGarbageModalityNumber)
				dDeltaCost += ComputePartitionCost(nGroupNumber - 1, nMergeGarbageModalityNumber) -
					      ComputePartitionCost(nGroupNumber, nGarbageModalityNumber);
			// Sinon
			else
				dDeltaCost += ComputePartitionCost(nGroupNumber - 1, nGarbageModalityNumber) -
					      ComputePartitionCost(nGroupNumber, nGarbageModalityNumber);

			if (dDeltaCost < dBestDeltaCost)
			{
				dBestDeltaCost = dDeltaCost;
				nBestGroup1 = nGroup1;
				nBestGroup2 = nGroup2;
			}
		}
	}
	assert(nBestGroup1 != -1 and nBestGroup2 != -1);
	assert(nBestGroup1 < nBestGroup2);

	// Mise a jour de la liste triee des groupes par nombre de modalites
	// Retrait des deux groupes qui vont etre fusionnes
	frequencyList->RemoveAt(cast(KWMODLGroup*, oaTarget->GetAt(nBestGroup1))->GetPosition());
	debug(cast(KWMODLGroup*, oaTarget->GetAt(nBestGroup1))->SetPosition(NULL));
	frequencyList->RemoveAt(cast(KWMODLGroup*, oaTarget->GetAt(nBestGroup2))->GetPosition());
	debug(cast(KWMODLGroup*, oaTarget->GetAt(nBestGroup2))->SetPosition(NULL));

	// Creation du nouveau tableau de parties
	oaNewTarget = new ObjectArray;
	oaNewTarget->SetSize(oaTarget->GetSize() - 1);
	for (nGroup = 0; nGroup < oaNewTarget->GetSize(); nGroup++)
	{
		group = new KWMODLGroup(GetFrequencyVectorCreator());
		InitializeFrequencyVector(group->GetFrequencyVector());
		oaNewTarget->SetAt(nGroup, group);
	}

	// Calcul des index des nouveaux groupes pour chaque ancien groupe
	ivNewGroups.SetSize(oaTarget->GetSize());
	for (nGroup = 0; nGroup < nBestGroup2; nGroup++)
		ivNewGroups.SetAt(nGroup, nGroup);
	ivNewGroups.SetAt(nBestGroup2, nBestGroup1);
	for (nGroup = nBestGroup2 + 1; nGroup < oaTarget->GetSize(); nGroup++)
		ivNewGroups.SetAt(nGroup, nGroup - 1);

	// Alimentation du nouveau tableau de parties
	for (nGroup = 0; nGroup < oaTarget->GetSize(); nGroup++)
	{
		// Gestion de la fusion pour les groupes sources
		if (nGroup == nBestGroup1 or nGroup == nBestGroup2)
		{
			// Les deux groupes sont fusionnes a la position nBestGroup1
			AddFrequencyVector(cast(KWMODLGroup*, oaNewTarget->GetAt(nBestGroup1))->GetFrequencyVector(),
					   cast(KWMODLGroup*, oaTarget->GetAt(nGroup))->GetFrequencyVector());
		}
		// Recopie au nouvel emplacement sinon
		else
		{
			nNewGroup = ivNewGroups.GetAt(nGroup);

			cast(KWMODLGroup*, oaNewTarget->GetAt(nNewGroup))
			    ->GetFrequencyVector()
			    ->CopyFrom(cast(KWMODLGroup*, oaTarget->GetAt(nGroup))->GetFrequencyVector());
		}
	}

	// Mise a jour de la liste triee des groupes par nombre de modalites
	// Ajout du groupe fusionne
	cast(KWMODLGroup*, oaTarget->GetAt(nBestGroup1))
	    ->SetPosition(frequencyList->Add(cast(KWMODLGroup*, oaTarget->GetAt(nBestGroup1))));

	// Modification du vecteur d'affectation des groupes
	for (i = 0; i < ivGroups->GetSize(); i++)
		ivGroups->SetAt(i, ivNewGroups.GetAt(ivGroups->GetAt(i)));

	// ensure(kwftNewTarget->ComputeTotalFrequency() == kwftTarget->ComputeTotalFrequency());
}
