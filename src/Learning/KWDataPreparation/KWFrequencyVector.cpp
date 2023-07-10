// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWFrequencyVector.h"

////////////////////////////////////////////////////////////////////////////
// Classe KWFrequencyVector

void KWFrequencyVector::CopyFrom(const KWFrequencyVector* kwfvSource)
{
	nModalityNumber = kwfvSource->nModalityNumber;
	position = NULL;
}

KWFrequencyVector* KWFrequencyVector::Clone() const
{
	return new KWFrequencyVector;
}

int KWFrequencyVector::ComputeTotalFrequency() const
{
	return 0;
}

void KWFrequencyVector::WriteHeaderLineReport(ostream& ost) const
{
	ost << "Modality number\t";
}

void KWFrequencyVector::WriteLineReport(ostream& ost) const
{
	// Nombre de modalites
	ost << nModalityNumber << "\t";
	ost << position << "\t";
}

/////////////////////////////////////////////////////////////////////
int KWFrequencyVectorModalityNumberCompare(const void* elem1, const void* elem2)
{
	KWFrequencyVector* frequencyVector1;
	KWFrequencyVector* frequencyVector2;

	frequencyVector1 = cast(KWFrequencyVector*, *(Object**)elem1);
	frequencyVector2 = cast(KWFrequencyVector*, *(Object**)elem2);

	// Comparaison du nombre de modalites par valeurs decroissantes
	return (frequencyVector2->GetModalityNumber() - frequencyVector1->GetModalityNumber());
}

////////////////////////////////////////////////////////////////////
// Classe KWDenseFrequencyVector

int KWDenseFrequencyVector::GetSize() const
{
	return ivFrequencyVector.GetSize();
}

int KWDenseFrequencyVector::ComputeTotalFrequency() const
{
	int nTotalFrequency;
	int i;

	// Cumul des effectifs du vecteur
	nTotalFrequency = 0;
	for (i = 0; i < ivFrequencyVector.GetSize(); i++)
		nTotalFrequency += ivFrequencyVector.GetAt(i);
	return nTotalFrequency;
}

////////////////////////////////////////////////////////////////////
// Classe KWFrequencyTable

KWFrequencyTable::KWFrequencyTable()
{
	kwfvFrequencyVectorCreator = new KWDenseFrequencyVector;
	nTotalFrequency = -1;
	nGranularity = 0;
	nGarbageModalityNumber = 0;
	nInitialValueNumber = 0;
	nGranularizedValueNumber = 0;
}

KWFrequencyTable::~KWFrequencyTable()
{
	oaFrequencyVectors.DeleteAll();

	if (kwfvFrequencyVectorCreator != NULL)
		delete kwfvFrequencyVectorCreator;
}

void KWFrequencyTable::Initialize(int nFrequencyVectorNumber)
{
	int i;

	require(kwfvFrequencyVectorCreator != NULL);
	require(nFrequencyVectorNumber >= 0);

	// Nettoyage
	oaFrequencyVectors.DeleteAll();

	// Creation des vecteur d'effectifs
	oaFrequencyVectors.SetSize(nFrequencyVectorNumber);
	for (i = 0; i < oaFrequencyVectors.GetSize(); i++)
		oaFrequencyVectors.SetAt(i, kwfvFrequencyVectorCreator->Create());
}

int KWFrequencyTable::GetTotalFrequency() const
{
	if (nTotalFrequency == -1)
		nTotalFrequency = ComputeTotalFrequency();

	assert(nTotalFrequency == ComputeTotalFrequency());

	return nTotalFrequency;
}

int KWFrequencyTable::ComputePartialFrequency(int nFirstIndex, int nLastIndex) const
{
	int nPartialFrequency;
	int i;

	require(0 <= nFirstIndex);
	require(nFirstIndex <= nLastIndex);
	require(nLastIndex <= GetFrequencyVectorNumber());

	// Cumul des effectifs de la table
	nPartialFrequency = 0;
	for (i = nFirstIndex; i < nLastIndex; i++)
		nPartialFrequency += GetFrequencyVectorAt(i)->ComputeTotalFrequency();
	return nPartialFrequency;
}

void KWFrequencyTable::SetInitialValueNumber(int nValue)
{
	require(nValue >= 0);
	nInitialValueNumber = nValue;
}
int KWFrequencyTable::GetInitialValueNumber() const
{
	return nInitialValueNumber;
}
void KWFrequencyTable::SetGranularizedValueNumber(int nValue)
{
	require(nValue >= 0);
	nGranularizedValueNumber = nValue;
}
int KWFrequencyTable::GetGranularizedValueNumber() const
{
	return nGranularizedValueNumber;
}

void KWFrequencyTable::SetGranularity(int nModality)
{
	require(nModality >= 0);
	nGranularity = nModality;
}

int KWFrequencyTable::GetGranularity() const
{
	return nGranularity;
}

void KWFrequencyTable::SetGarbageModalityNumber(int nModality)
{
	require(nModality >= 0);
	nGarbageModalityNumber = nModality;
}

int KWFrequencyTable::GetGarbageModalityNumber() const
{
	return nGarbageModalityNumber;
}

void KWFrequencyTable::ComputeTargetFrequencies(IntVector* ivTargetFrequencies)
{
	KWDenseFrequencyVector refDenseFrequencyVector;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	IntVector* ivFrequencyVector;
	int nSource;
	int nTarget;
	int nSourceNumber;
	int nTargetNumber;

	require(ivTargetFrequencies != NULL);

	// Les vecteurs de la table doivent etre de type Dense
	require(oaFrequencyVectors.GetSize() == 0 or
		GetFrequencyVectorAt(0)->GetClassLabel() == refDenseFrequencyVector.GetClassLabel());

	// Reinitialisation du vecteur cible
	ivTargetFrequencies->SetSize(0);
	nSourceNumber = GetFrequencyVectorNumber();
	nTargetNumber = GetFrequencyVectorSize();
	ivTargetFrequencies->SetSize(nTargetNumber);

	// Alimentation des effectif cibles
	for (nSource = 0; nSource < nSourceNumber; nSource++)
	{
		// Acces au vecteur de la ligne et parametrage de sa taille (sense etre en representation dense)
		kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, GetFrequencyVectorAt(nSource));
		ivFrequencyVector = kwdfvFrequencyVector->GetFrequencyVector();

		// Mise a jour du vecteur d'effectif pour chaque modalite cible
		for (nTarget = 0; nTarget < nTargetNumber; nTarget++)
		{
			ivTargetFrequencies->UpgradeAt(nTarget, ivFrequencyVector->GetAt(nTarget));
		}
	}
}

double KWFrequencyTable::ComputeTargetEntropy(const IntVector* ivTargetFrequencies)
{
	double dResult;
	double dTableFrequency;
	double dProba;
	int nTarget;

	require(ivTargetFrequencies != NULL);

	// Parcours de la matrice pour calculer la valeur de l'entropie
	dResult = 0;
	dTableFrequency = ComputeTotalFrequency();
	if (dTableFrequency != 0)
	{
		for (nTarget = 0; nTarget < GetFrequencyVectorSize(); nTarget++)
		{
			dProba = ivTargetFrequencies->GetAt(nTarget) / dTableFrequency;
			if (dProba != 0)
				dResult += dProba * log(dProba);
		}
	}
	dResult /= -log(2.0);
	assert(dResult > -1e-10);
	dResult = fabs(dResult);
	if (dResult < 1e-10)
		dResult = 0;

	return dResult;
}

double KWFrequencyTable::ComputeMutualEntropy(const IntVector* ivTargetFrequencies)
{
	double dResult;
	double dTableFrequency;
	double dProba;
	int nSource;
	int nTarget;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	IntVector* ivFrequencyVector;

	require(ivTargetFrequencies != NULL);

	// Parcours de la matrice pour calculer la valeur de l'entropie
	dResult = 0;
	dTableFrequency = ComputeTotalFrequency(); // GetTableFrequency();
	if (dTableFrequency != 0)
	{
		for (nSource = 0; nSource < GetFrequencyVectorNumber(); nSource++)
		{
			kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, GetFrequencyVectorAt(nSource));
			ivFrequencyVector = kwdfvFrequencyVector->GetFrequencyVector();

			for (nTarget = 0; nTarget < GetFrequencyVectorSize(); nTarget++)
			{
				dProba = ivFrequencyVector->GetAt(nTarget) / dTableFrequency;
				if (dProba != 0)
					dResult +=
					    dProba *
					    log(dProba /
						((kwdfvFrequencyVector->ComputeTotalFrequency() / dTableFrequency) *
						 (ivTargetFrequencies->GetAt(nTarget) / dTableFrequency)));
			}
		}
	}
	dResult /= log(2.0);
	assert(dResult > -1e-10);
	dResult = fabs(dResult);
	if (dResult < 1e-10)
		dResult = 0;

	return dResult;
}

void KWFrequencyTable::CopyFrom(const KWFrequencyTable* kwftSource)
{
	int i;
	KWFrequencyVector* kwfvSource;
	KWFrequencyVector* kwfvCurrent;

	require(kwftSource != NULL);

	// Nettoyage
	oaFrequencyVectors.DeleteAll();

	// Recopie des caracteristique principales
	SetFrequencyVectorCreator(kwftSource->GetFrequencyVectorCreator()->Clone());
	nTotalFrequency = kwftSource->ComputeTotalFrequency();

	// Parametrage granularite et nombre de modalites poubelle
	SetGranularity(kwftSource->GetGranularity());
	SetGarbageModalityNumber(kwftSource->GetGarbageModalityNumber());
	SetInitialValueNumber(kwftSource->GetInitialValueNumber());
	SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());

	// Recopie des vecteurs apres les avoir crees
	Initialize(kwftSource->GetFrequencyVectorNumber());
	for (i = 0; i < kwftSource->GetFrequencyVectorNumber(); i++)
	{
		// Acces aux vecteurs d'effectifs source et courant
		kwfvCurrent = GetFrequencyVectorAt(i);
		kwfvSource = kwftSource->GetFrequencyVectorAt(i);

		// Recopie
		kwfvCurrent->CopyFrom(kwfvSource);
	}
}

KWFrequencyTable* KWFrequencyTable::Clone() const
{
	KWFrequencyTable* kwftClone;

	kwftClone = new KWFrequencyTable;
	kwftClone->CopyFrom(this);
	return kwftClone;
}

void KWFrequencyTable::ImportFrom(KWFrequencyTable* kwftSource)
{
	require(kwftSource != NULL);

	// Nettoyage
	oaFrequencyVectors.DeleteAll();

	// Recopie des caracteristique principales
	SetFrequencyVectorCreator(kwftSource->GetFrequencyVectorCreator()->Clone());
	nTotalFrequency = kwftSource->ComputeTotalFrequency();

	// Parametrage granularite et nombre de modalites poubelle
	SetGranularity(kwftSource->GetGranularity());
	SetGarbageModalityNumber(kwftSource->GetGarbageModalityNumber());
	SetInitialValueNumber(kwftSource->GetInitialValueNumber());
	SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());

	// Transfert des vecteurs
	oaFrequencyVectors.CopyFrom(&(kwftSource->oaFrequencyVectors));

	// Nettoyage de la table initiale
	kwftSource->RemoveAllFrequencyVectors();
}

void KWFrequencyTable::RemoveAllFrequencyVectors()
{
	nTotalFrequency = -1;
	nGranularity = 0;
	nGarbageModalityNumber = 0;
	nInitialValueNumber = 0;
	nGranularizedValueNumber = 0;
	oaFrequencyVectors.RemoveAll();
}

void KWFrequencyTable::DeleteAllFrequencyVectors()
{
	oaFrequencyVectors.DeleteAll();
	RemoveAllFrequencyVectors();
}

boolean KWFrequencyTable::Check() const
{
	boolean bOk = true;
	int i;
	KWFrequencyVector* kwfvVector;

	// Aucun vecteur ne doit etre vide, sauf s'il n'y a qu'un seul vecteur (cas du model null)
	if (oaFrequencyVectors.GetSize() > 1)
	{
		for (i = 0; i < oaFrequencyVectors.GetSize(); i++)
		{
			kwfvVector = GetFrequencyVectorAt(i);
			bOk = bOk and kwfvVector->ComputeTotalFrequency() > 0;
		}
	}
	return bOk;
}

void KWFrequencyTable::ComputeNullTable(KWFrequencyTable* kwftSource)
{
	KWDenseFrequencyVector refDenseFrequencyVector;
	IntVector* ivSourceFrequencyVector;

	require(kwftSource != NULL);

	// Les vecteurs de la table doivent etre de type Dense
	require(oaFrequencyVectors.GetSize() == 0 or
		GetFrequencyVectorAt(0)->GetClassLabel() == refDenseFrequencyVector.GetClassLabel());

	// Nettoyage
	oaFrequencyVectors.DeleteAll();

	// Initialisation
	SetFrequencyVectorCreator(kwftSource->GetFrequencyVectorCreator()->Clone());
	Initialize(1);

	// Parametrage granularite et nombre de modalites poubelle
	SetGranularity(kwftSource->GetGranularity());
	SetGarbageModalityNumber(kwftSource->GetGarbageModalityNumber());
	SetInitialValueNumber(kwftSource->GetInitialValueNumber());
	SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());

	// Initialisation des valeurs du vecteur a partir des cumuls par modalite cible
	ivSourceFrequencyVector = cast(KWDenseFrequencyVector*, GetFrequencyVectorAt(0))->GetFrequencyVector();
	kwftSource->ComputeTargetFrequencies(ivSourceFrequencyVector);
}

void KWFrequencyTable::ImportDataGridStats(const KWDataGridStats* dataGridStats)
{
	int nCell;
	int nCellNumber;
	int nCellFrequency;
	IntVector ivPartIndexes;
	int nSource;
	int nTarget;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	IntVector* ivFrequencyVector;

	require(dataGridStats != NULL);
	require(dataGridStats->Check());

	// Initialisation de la table
	Initialize(dataGridStats->ComputeSourceGridSize());
	for (nSource = 0; nSource < GetFrequencyVectorNumber(); nSource++)
	{
		// Acces au vecteur de la table (sense etre en representation dense)
		kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, GetFrequencyVectorAt(nSource));
		ivFrequencyVector = kwdfvFrequencyVector->GetFrequencyVector();
		ivFrequencyVector->SetSize(dataGridStats->ComputeTargetGridSize());
	}

	// Initilisation de la granularite
	nGranularity = dataGridStats->GetGranularity();

	// Initialisation  du nombre de valeurs initial et apres granularisation selon les caracteristiques du premier
	// attribut de la grille CH V9 Lot 8 TODO pas de sens s'il y a plusieurs attributs source ?
	nInitialValueNumber = dataGridStats->GetAttributeAt(0)->GetInitialValueNumber();
	nGranularizedValueNumber = dataGridStats->GetAttributeAt(0)->GetGranularizedValueNumber();

	// On commence a importer les effectifs des cellules de la grille
	// et calculer les statistiques sources, cibles et globales
	nCellNumber = GetFrequencyVectorNumber() * GetFrequencyVectorSize();
	ivPartIndexes.SetSize(dataGridStats->GetAttributeNumber());
	for (nCell = 0; nCell < nCellNumber; nCell++)
	{
		// Calcul de l'index des parties de la cellule
		dataGridStats->ComputePartIndexes(nCell, &ivPartIndexes);

		// Calcul des index sources et cibles
		nSource = dataGridStats->ComputeSourceCellIndex(&ivPartIndexes);
		nTarget = dataGridStats->ComputeTargetCellIndex(&ivPartIndexes);
		assert(0 <= nSource and nSource < GetFrequencyVectorNumber());
		assert(0 <= nTarget and nTarget < GetFrequencyVectorSize());

		// Recherche de l'effectif de la cellule
		nCellFrequency = dataGridStats->GetCellFrequencyAt(&ivPartIndexes);

		// Mise a jour de l'effectif dans la table de probabilites conditionnelle
		assert(cast(KWDenseFrequencyVector*, GetFrequencyVectorAt(nSource))
			   ->GetFrequencyVector()
			   ->GetAt(nTarget) == 0);
		cast(KWDenseFrequencyVector*, GetFrequencyVectorAt(nSource))
		    ->GetFrequencyVector()
		    ->SetAt(nTarget, nCellFrequency);
	}
	ensure(Check());
	ensure(dataGridStats->ComputeGridFrequency() == ComputeTotalFrequency());
}

void KWFrequencyTable::FilterEmptyFrequencyVectors(IntVector* ivNewIndexes)
{
	int nOldIndex;
	int nNewIndex;
	KWFrequencyVector* kwfvCurrent;

	require(ivNewIndexes != NULL);

	// Recopie des vecteurs
	ivNewIndexes->SetSize(oaFrequencyVectors.GetSize());
	nNewIndex = 0;
	for (nOldIndex = 0; nOldIndex < oaFrequencyVectors.GetSize(); nOldIndex++)
	{
		kwfvCurrent = GetFrequencyVectorAt(nOldIndex);

		// Pas de changement si vecteur non vide
		if (kwfvCurrent->ComputeTotalFrequency() > 0)
		{
			oaFrequencyVectors.SetAt(nNewIndex, kwfvCurrent);
			ivNewIndexes->SetAt(nOldIndex, nNewIndex);
			nNewIndex++;
		}
		// Sinon, supression du vecteur
		else
		{
			delete kwfvCurrent;
			ivNewIndexes->SetAt(nOldIndex, -1);
		}
	}

	// Retaillage du tableau
	oaFrequencyVectors.SetSize(nNewIndex);
}

boolean KWFrequencyTable::IsTableSortedBySourceFrequency(boolean bAscending) const
{
	boolean bSorted;
	int nSource;
	int nAscending;
	double dSortValue;
	double dPreviousSortValue;

	require(GetFrequencyVectorSize() > 0);

	// Sens du tri
	if (bAscending)
		nAscending = 1;
	else
		nAscending = -1;

	// Parcours des lignes de la table en verifier l'ordre
	bSorted = true;
	dSortValue = 0;
	for (nSource = 0; nSource < GetFrequencyVectorNumber(); nSource++)
	{
		// Memorisation de la valeur precedente
		dPreviousSortValue = dSortValue;

		// Calcul de la valeur courante
		dSortValue = nAscending * GetFrequencyVectorAt(nSource)->ComputeTotalFrequency();

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

void KWFrequencyTable::SortTableBySourceFrequency(boolean bAscending, IntVector* ivInitialLineIndexes,
						  IntVector* ivSortedLineIndexes)
{
	ObjectArray oaLines;
	ObjectArray oaSortedLines;
	KWSortableValue* line;
	int nSource;
	int nAscending;

	// Sens du tri
	if (bAscending)
		nAscending = 1;
	else
		nAscending = -1;

	// Initialisation d'un tableau memorisant la proportion de la classe
	// cible specifiee pour chaque ligne de la table de contingence
	oaLines.SetSize(GetFrequencyVectorNumber());
	for (nSource = 0; nSource < GetFrequencyVectorNumber(); nSource++)
	{
		line = new KWSortableValue;
		line->SetSortValue(nAscending * GetFrequencyVectorAt(nSource)->ComputeTotalFrequency());
		line->SetIndex(nSource);
		oaLines.SetAt(nSource, line);
	}

	// Tri d'une copie de ce tableau
	oaSortedLines.CopyFrom(&oaLines);
	oaSortedLines.SetCompareFunction(KWSortableValueCompare);
	oaSortedLines.Sort();

	// Permutation des lignes pour trier la table
	PermutateTableLines(&oaLines, &oaSortedLines, ivInitialLineIndexes, ivSortedLineIndexes);

	// Nettoyage des donnees de travail
	oaLines.DeleteAll();
}

void KWFrequencyTable::SortTableAndModalitiesBySourceFrequency(SymbolVector* svSourceModalities, boolean bAscending,
							       IntVector* ivInitialLineIndexes,
							       IntVector* ivSortedLineIndexes)
{
	ObjectArray oaLines;
	ObjectArray oaSortedLines;
	KWSortableValueSymbol* line;
	int nSource;
	int nAscending;

	require(svSourceModalities != NULL);
	require(svSourceModalities->GetSize() == GetFrequencyVectorNumber());

	// Sens du tri
	if (bAscending)
		nAscending = 1;
	else
		nAscending = -1;

	// Initialisation d'un tableau memorisant l'effectif de chaque ligne de la table de contingence
	oaLines.SetSize(GetFrequencyVectorNumber());
	for (nSource = 0; nSource < GetFrequencyVectorNumber(); nSource++)
	{
		line = new KWSortableValueSymbol;
		line->SetSortValue(nAscending * GetFrequencyVectorAt(nSource)->ComputeTotalFrequency());
		line->SetSymbol(svSourceModalities->GetAt(nSource));
		oaLines.SetAt(nSource, line);
	}

	// Tri d'une copie de ce tableau
	oaSortedLines.CopyFrom(&oaLines);
	oaSortedLines.SetCompareFunction(KWSortableValueSymbolCompare);
	oaSortedLines.Sort();

	// Permutation des lignes pour trier la table
	PermutateTableLines(&oaLines, &oaSortedLines, ivInitialLineIndexes, ivSortedLineIndexes);

	// Recopie des modalites symbole dans l'ordre du tri
	for (nSource = 0; nSource < GetFrequencyVectorNumber(); nSource++)
	{
		line = cast(KWSortableValueSymbol*, oaSortedLines.GetAt(nSource));
		svSourceModalities->SetAt(nSource, line->GetSymbol());
	}

	// Nettoyage des donnees de travail
	oaLines.DeleteAll();
}

void KWFrequencyTable::SortTableBySourceAndFirstModalityFrequency(IntVector* ivGroups)
{
	ObjectArray oaLines;
	ObjectArray oaSortedLines;
	KWSortableFrequencyVector* line;
	IntVector ivSortedIndexes;
	int nSource;
	int nGroup;

	// Initialisation d'un tableau memorisant la proportion de la classe
	// cible specifiee pour chaque ligne de la table de contingence
	oaLines.SetSize(GetFrequencyVectorNumber());
	for (nSource = 0; nSource < GetFrequencyVectorNumber(); nSource++)
	{
		line = new KWSortableFrequencyVector;
		line->SetIndex(nSource);
		line->SetFrequencyVector(GetFrequencyVectorAt(nSource));
		oaLines.SetAt(nSource, line);
	}

	// Initialisation du rang de la premiere modalite
	for (nSource = 0; nSource < ivGroups->GetSize(); nSource++)
	{
		nGroup = ivGroups->GetAt(nSource);
		line = cast(KWSortableFrequencyVector*, oaLines.GetAt(nGroup));
		if (line->GetFirstModalityFrequencyOrder() == -1)
			line->SetFirstModalityFrequencyOrder(nSource);
	}

	// Tri d'une copie de ce tableau
	oaSortedLines.CopyFrom(&oaLines);
	oaSortedLines.SetCompareFunction(KWSortableFrequencyVectorCompare);
	oaSortedLines.Sort();

	// Mis a jour d'ivGroups
	ivSortedIndexes.SetSize(ivGroups->GetSize());
	for (nSource = 0; nSource < oaSortedLines.GetSize(); nSource++)
	{
		line = cast(KWSortableFrequencyVector*, oaSortedLines.GetAt(nSource));
		ivSortedIndexes.SetAt(line->GetIndex(), nSource);
	}
	for (nSource = 0; nSource < ivGroups->GetSize(); nSource++)
		ivGroups->SetAt(nSource, ivSortedIndexes.GetAt(ivGroups->GetAt(nSource)));

	// Permutation des lignes pour trier la table
	PermutateTableLines(&oaLines, &oaSortedLines, NULL, NULL);

	// Nettoyage des donnees de travail
	oaLines.DeleteAll();
}

void KWFrequencyTable::PermutateTableLines(const ObjectArray* oaFromLines, const ObjectArray* oaToLines,
					   IntVector* ivInitialLineIndexes, IntVector* ivPermutatedLineIndexes)
{
	KWSortableIndex* line;
	int nBuffer;
	int nFrom;
	int nTarget;
	int nTo;
	int nTargetValueNumber;
	KWDenseFrequencyVector* kwdfvFromFrequencyVector;
	KWDenseFrequencyVector* kwdfvToFrequencyVector;

	require(this != NULL);
	require(oaFromLines != NULL);
	require(oaToLines != NULL);
	require(oaFromLines->GetSize() == GetFrequencyVectorNumber());
	require(oaToLines->GetSize() == GetFrequencyVectorNumber());

	// Memorisation des index initiaux pour la table de destination
	if (ivInitialLineIndexes != NULL)
	{
		// Memorisation des index initiaux
		// Ainsi, le tableau destination aura acces aux index initiaux
		for (nFrom = 0; nFrom < oaFromLines->GetSize(); nFrom++)
		{
			line = cast(KWSortableIndex*, oaFromLines->GetAt(nFrom));
			line->SetIndex(nFrom);
		}

		// Memorisation des index initiaux
		ivInitialLineIndexes->SetSize(oaToLines->GetSize());
		for (nTo = 0; nTo < oaToLines->GetSize(); nTo++)
		{
			line = cast(KWSortableIndex*, oaToLines->GetAt(nTo));
			nFrom = line->GetIndex();
			ivInitialLineIndexes->SetAt(nTo, nFrom);
			assert(oaFromLines->GetAt(nFrom) == line);
		}
	}

	// Memorisation des index de destination
	// Ainsi, le tableau origine aura acces aux index des lignes destinations
	// Cela sera necessaire a la permutaion des lignes de la table
	for (nTo = 0; nTo < oaToLines->GetSize(); nTo++)
	{
		line = cast(KWSortableIndex*, oaToLines->GetAt(nTo));
		line->SetIndex(nTo);
	}

	// Memorisation des index destinations pour la table initiale
	if (ivPermutatedLineIndexes != NULL)
	{
		ivPermutatedLineIndexes->SetSize(oaFromLines->GetSize());
		for (nFrom = 0; nFrom < oaFromLines->GetSize(); nFrom++)
		{
			line = cast(KWSortableIndex*, oaFromLines->GetAt(nFrom));
			nTo = line->GetIndex();
			ivPermutatedLineIndexes->SetAt(nFrom, nTo);
			assert(oaToLines->GetAt(nTo) == line);
		}
	}

	// Permutation des lignes de la table (sans necessite d'allouer une table intermediaire)
	// par transfert ligne a ligne
	// L'index est positionne a -1 quand une ligne a ete transferee
	nTargetValueNumber = GetFrequencyVectorSize();
	for (nFrom = 0; nFrom < oaFromLines->GetSize(); nFrom++)
	{
		line = cast(KWSortableIndex*, oaFromLines->GetAt(nFrom));

		// Recherche recursive de la ligne destination pour propager les
		// transferts de ligne en ne memorisant qu'une ligne de facon temporaire
		nTo = line->GetIndex();
		while (nTo != -1 and nTo != nFrom)
		{
			// Echange des lignes d'origine et de destination
			kwdfvFromFrequencyVector = cast(KWDenseFrequencyVector*, GetFrequencyVectorAt(nFrom));
			kwdfvToFrequencyVector = cast(KWDenseFrequencyVector*, GetFrequencyVectorAt(nTo));
			for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
			{
				nBuffer = kwdfvToFrequencyVector->GetFrequencyVector()->GetAt(nTarget);
				kwdfvToFrequencyVector->GetFrequencyVector()->SetAt(
				    nTarget, kwdfvFromFrequencyVector->GetFrequencyVector()->GetAt(nTarget));
				kwdfvFromFrequencyVector->GetFrequencyVector()->SetAt(nTarget, nBuffer);
			}

			// Echange des nombres de modalites par ligne
			nBuffer = kwdfvToFrequencyVector->GetModalityNumber();
			kwdfvToFrequencyVector->SetModalityNumber(kwdfvFromFrequencyVector->GetModalityNumber());
			kwdfvFromFrequencyVector->SetModalityNumber(nBuffer);

			// La ligne courante est desormais transferee, ce que l'on memorise
			line->SetIndex(-1);

			// On cherche a propager le transfert a la ligne qui vient d'etre ecrasee
			// (mais memorisee dans la ligne initiale d'index nLineFrom)
			line = cast(KWSortableIndex*, oaFromLines->GetAt(nTo));
			nTo = line->GetIndex();
		}
		line->SetIndex(-1);
	}
}

void KWFrequencyTable::Write(ostream& ost) const
{
	int i;
	KWFrequencyVector* kwfvCurrent;

	ost << "Nombre initial de valeurs\t" << nInitialValueNumber << "\tNombre de valeurs apres granularisation\t"
	    << nGranularizedValueNumber << endl;
	// Affichage de la granularite et de la taille du groupe poubelle
	ost << "Granularite\t" << nGranularity << "\tTaillePoubelle\t" << nGarbageModalityNumber << "\n";

	// Affichage de la table si non vide
	assert(oaFrequencyVectors.GetSize() == 0 or kwfvFrequencyVectorCreator != NULL);
	for (i = 0; i < GetFrequencyVectorNumber(); i++)
	{
		kwfvCurrent = GetFrequencyVectorAt(i);

		// Affichage du vecteur d'effectif
		if (i == 0)
		{
			ost << "Index\t";
			kwfvCurrent->WriteHeaderLineReport(ost);
			ost << "\n";
		}
		ost << i << "\t";
		kwfvCurrent->WriteLineReport(ost);
		ost << "\n";
	}
}

int KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage()
{
	require(nMinimumNumberOfModalitiesForGarbage > 0);
	return nMinimumNumberOfModalitiesForGarbage;
}

void KWFrequencyTable::SetMinimumNumberOfModalitiesForGarbage(int nValue)
{
	require(nValue > 0);
	nMinimumNumberOfModalitiesForGarbage = nValue;
}

boolean KWFrequencyTable::GetWriteGranularityAndGarbage()
{
	return bWriteGranularityAndGarbage;
}

void KWFrequencyTable::SetWriteGranularityAndGarbage(boolean bValue)
{
	bWriteGranularityAndGarbage = bValue;
}

int KWFrequencyTable::ComputeTotalFrequency() const
{
	int nTotalTableFrequency;
	int i;

	// Cumul des effectifs de la table
	nTotalTableFrequency = 0;
	for (i = 0; i < GetFrequencyVectorNumber(); i++)
		nTotalTableFrequency += GetFrequencyVectorAt(i)->ComputeTotalFrequency();
	return nTotalTableFrequency;
}

int KWFrequencyTable::ComputeTotalValueNumber() const
{
	int nTotalTableModalityNumber;
	int i;

	// Cumul des modalites de la table
	nTotalTableModalityNumber = 0;
	for (i = 0; i < GetFrequencyVectorNumber(); i++)
		nTotalTableModalityNumber += GetFrequencyVectorAt(i)->GetModalityNumber();
	return nTotalTableModalityNumber;
}

boolean KWFrequencyTable::bWriteGranularityAndGarbage = false;
int KWFrequencyTable::nMinimumNumberOfModalitiesForGarbage = 7;

////////////////////////////////////////////////////////////////////////
// Classe KWSortableFrequencyVector

void KWSortableFrequencyVector::Write(ostream& ost) const
{
	ost << kwfvVector << "\n" << nFirstModalityFrequencyOrder << "\n";
}

int KWSortableFrequencyVectorCompare(const void* elem1, const void* elem2)
{
	double dResult;

	// Comparaison sur l'effectif des vecteurs
	dResult = -(cast(KWSortableFrequencyVector*, *(Object**)elem1)->GetFrequencyVector()->ComputeTotalFrequency() -
		    cast(KWSortableFrequencyVector*, *(Object**)elem2)->GetFrequencyVector()->ComputeTotalFrequency());
	if (dResult > 0)
		return 1;
	else if (dResult < 0)
		return -1;
	// Cas d'egalite
	else
	{
		// Comparaison selon le rang des premieres modalites des vecteurs dans le vecteur initial des modalites
		// trie par effectif decroissant
		dResult = cast(KWSortableFrequencyVector*, *(Object**)elem1)->GetFirstModalityFrequencyOrder() -
			  cast(KWSortableFrequencyVector*, *(Object**)elem2)->GetFirstModalityFrequencyOrder();
		if (dResult > 0)
			return 1;
		else
			return -1;
	}
}
