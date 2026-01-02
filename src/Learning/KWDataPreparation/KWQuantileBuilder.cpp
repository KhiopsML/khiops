// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWQuantileBuilder.h"

/////////////////////////////////////////////////////////////////////////
// Classe KWQuantileIntervalBuilder

KWQuantileBuilder::KWQuantileBuilder()
{
	bIsFrequencyInitialized = false;
	bIsValueInitialized = false;
	nInstanceNumber = 0;
	nValueNumber = 0;
	nRequestedQuantileNumber = 0;
}

KWQuantileBuilder::~KWQuantileBuilder() {}

/////////////////////////////////////////////////////////////////////////
// Classe KWQuantileIntervalBuilder

KWQuantileIntervalBuilder::KWQuantileIntervalBuilder()
{
	bIsFrequencyInitialized = false;
	bIsValueInitialized = false;
	nInstanceNumber = 0;
	nValueNumber = 0;
	nRequestedQuantileNumber = 0;
	bIsEqualWidth = false;
}

KWQuantileIntervalBuilder::~KWQuantileIntervalBuilder() {}

void KWQuantileIntervalBuilder::InitializeValues(const ContinuousVector* cvInputValues)
{
	Continuous cLastValue;
	int i;

	require(cvInputValues != NULL);

	// Reinitialisation des resultats
	bIsValueInitialized = true;
	bIsFrequencyInitialized = true;
	nRequestedQuantileNumber = 0;
	bIsEqualWidth = false;
	ivIntervalQuantileIndexes.SetSize(0);
	ivIntervalUpperValueIndexes.SetSize(0);

	// Memorisation du nombre d'instances
	nInstanceNumber = cvInputValues->GetSize();

	// Memorisation des valeurs uniques
	cvValues.SetSize(0);
	ivValuesCumulatedFrequencies.SetSize(0);
	if (cvInputValues->GetSize() > 0)
	{
		// Memorisation de la premiere valeur
		cLastValue = cvInputValues->GetAt(0);
		cvValues.Add(cLastValue);

		// Memorisation des valeurs precedentes
		for (i = 1; i < cvInputValues->GetSize(); i++)
		{
			// Incrementation a chaque changement de valeur (dans la liste triee)
			if (cvInputValues->GetAt(i) != cLastValue)
			{
				assert(cvInputValues->GetAt(i) > cLastValue);

				// Memorisation d'une nouvelle valeur
				cLastValue = cvInputValues->GetAt(i);
				cvValues.Add(cLastValue);

				// Memorisation de l'effectif cumule de la valeur precedente
				ivValuesCumulatedFrequencies.Add(i);
			}
		}
		ivValuesCumulatedFrequencies.Add(i);
	}
	nValueNumber = cvValues.GetSize();
	ensure(cvValues.GetSize() <= cvInputValues->GetSize());
	ensure(cvValues.GetSize() == ivValuesCumulatedFrequencies.GetSize());
	ensure(ivValuesCumulatedFrequencies.GetSize() == 0 or
	       ivValuesCumulatedFrequencies.GetAt(ivValuesCumulatedFrequencies.GetSize() - 1) ==
		   cvInputValues->GetSize());
}

void KWQuantileIntervalBuilder::InitializeFrequencies(const IntVector* ivInputFrequencies)
{
	int i;

	require(ivInputFrequencies != NULL);

	// Reinitialisation des resultats
	bIsValueInitialized = false;
	bIsFrequencyInitialized = true;
	nRequestedQuantileNumber = 0;
	bIsEqualWidth = false;
	ivIntervalQuantileIndexes.SetSize(0);
	ivIntervalUpperValueIndexes.SetSize(0);

	// Calcul et memorisation des effectifs cumules
	nInstanceNumber = 0;
	nValueNumber = ivInputFrequencies->GetSize();
	cvValues.SetSize(0);
	ivValuesCumulatedFrequencies.SetSize(nValueNumber);
	for (i = 0; i < nValueNumber; i++)
	{
		assert(ivInputFrequencies->GetAt(i) > 0);
		nInstanceNumber += ivInputFrequencies->GetAt(i);
		ivValuesCumulatedFrequencies.SetAt(i, nInstanceNumber);
	}
	ensure(ivValuesCumulatedFrequencies.GetSize() == 0 or
	       ivValuesCumulatedFrequencies.GetAt(ivValuesCumulatedFrequencies.GetSize() - 1) == nInstanceNumber);
}

int KWQuantileIntervalBuilder::ComputeQuantiles(int nQuantileNumber)
{
	const double dEpsilon = 1e-10;
	int nQuantile;
	double dSearchedCumulativeFrequency;
	int nSearchedCumulativeFrequency;
	int nRefUniqueIndex;
	int nQuantileUpperValueIndex;
	double dDistance1;
	double dDistance2;
	int nLastValidQuantileUpperValueIndex;
	boolean bSequentialSearch;

	require(IsFrequencyInitialized());
	require(GetValueNumber() > 0);
	require(nQuantileNumber >= 1);

	// Initialisation des resultats
	nRequestedQuantileNumber = nQuantileNumber;
	bIsEqualWidth = false;
	ivIntervalQuantileIndexes.SetSize(0);
	ivIntervalUpperValueIndexes.SetSize(0);

	// Calcul des dernier index des quantiles
	nRefUniqueIndex = 0;
	nLastValidQuantileUpperValueIndex = 0;
	bSequentialSearch = false;
	for (nQuantile = 0; nQuantile < nQuantileNumber; nQuantile++)
	{
		// Calcul de l'index en passant par un double pour eviter les problemes de depassement de capacite des int
		dSearchedCumulativeFrequency = nInstanceNumber * (nQuantile + 1.0) / nQuantileNumber;

		// On biaise vers une des extremites
		if (nQuantile >= nQuantileNumber / 2)
			dSearchedCumulativeFrequency += dEpsilon;
		else
			dSearchedCumulativeFrequency -= dEpsilon;
		nSearchedCumulativeFrequency = (int)floor(dSearchedCumulativeFrequency + 0.5);
		assert(0 <= nSearchedCumulativeFrequency and nSearchedCumulativeFrequency <= nInstanceNumber);

		// Passage eventuel a une recherche sequentielle si le nombre moyen de valeur a traiter par quantile est
		// petit Le seuil de passage du dichotomique au sequentiel a ete ajuste empiriquement
		if (not bSequentialSearch)
			bSequentialSearch = ((nQuantileNumber - nQuantile) * 32 >= nValueNumber - nRefUniqueIndex);

		// Index de la valeur unique correspondante
		nRefUniqueIndex = SearchCumulativeFrequencyIndex(bSequentialSearch, nSearchedCumulativeFrequency,
								 nRefUniqueIndex, nValueNumber - 1);

		// Recherche du point de coupure le plus proche
		nQuantileUpperValueIndex = nRefUniqueIndex;
		if (nQuantileUpperValueIndex > 0)
		{
			// Calcul de la distance par rapport a la valeur precedente
			dDistance1 = fabs(dSearchedCumulativeFrequency -
					  ivValuesCumulatedFrequencies.GetAt(nRefUniqueIndex - 1));

			// Calcul de la distance par rapport a la valeur courante
			dDistance2 =
			    fabs(ivValuesCumulatedFrequencies.GetAt(nRefUniqueIndex) - dSearchedCumulativeFrequency);

			// On se base sur la valeur la plus proche
			if (dDistance1 <= dDistance2)
				nQuantileUpperValueIndex--;
		}

		// Creation d'un intervalle si quantile non vide
		if (nQuantile == 0 or nQuantileUpperValueIndex > nLastValidQuantileUpperValueIndex)
		{
			nLastValidQuantileUpperValueIndex = nQuantileUpperValueIndex;
			ivIntervalQuantileIndexes.Add(nQuantile);
			ivIntervalUpperValueIndexes.Add(nLastValidQuantileUpperValueIndex);
		}
	}
	ensure(ivIntervalQuantileIndexes.GetSize() == ivIntervalUpperValueIndexes.GetSize());
	ensure(0 < ivIntervalUpperValueIndexes.GetSize() and
	       ivIntervalUpperValueIndexes.GetSize() <= nRequestedQuantileNumber);
	return ivIntervalUpperValueIndexes.GetSize();
}

int KWQuantileIntervalBuilder::ComputeEqualWidthQuantiles(int nQuantileNumber)
{
	int nMissingNumber;
	Continuous cMinValue;
	Continuous cMaxValue;
	Continuous cIntervalUpperBound;
	int nQuantile;
	int nRefUniqueIndex;
	int nLastValidQuantileUpperValueIndex;

	require(IsFrequencyInitialized());
	require(IsValueInitialized());
	require(GetValueNumber() > 0);
	require(nQuantileNumber >= 1);

	// Initialisation des resultats
	nRequestedQuantileNumber = nQuantileNumber;
	bIsEqualWidth = true;
	ivIntervalQuantileIndexes.SetSize(0);
	ivIntervalUpperValueIndexes.SetSize(0);

	// Recherche des valeurs extremes
	nMissingNumber = GetMissingValueNumber();
	cMinValue = GetMinValue();
	cMaxValue = GetMaxValue();

	// Calcul des dernier index des quantiles
	nRefUniqueIndex = 0;
	nLastValidQuantileUpperValueIndex = 0;
	for (nQuantile = 0; nQuantile < nQuantileNumber; nQuantile++)
	{
		// Borne sup de l'intervalle
		cIntervalUpperBound = ComputeEqualWidthIntervalUpperBound(nQuantile, nQuantileNumber, cMinValue,
									  cMaxValue, nMissingNumber);

		// Attention: la valeur recherchee peut se trouver avant l'index de la derniere valeur trouvee
		if (nRefUniqueIndex > 0)
			nRefUniqueIndex--;

		// Index de la valeur unique correspondante
		nRefUniqueIndex = SearchValueIndex(cIntervalUpperBound, nRefUniqueIndex, nValueNumber - 1);

		// Creation d'un intervalle, potentiellement vide
		nLastValidQuantileUpperValueIndex = nRefUniqueIndex;
		ivIntervalQuantileIndexes.Add(nQuantile);
		ivIntervalUpperValueIndexes.Add(nLastValidQuantileUpperValueIndex);
	}
	ensure(ivIntervalQuantileIndexes.GetSize() == ivIntervalUpperValueIndexes.GetSize());
	ensure(0 < ivIntervalUpperValueIndexes.GetSize() and
	       ivIntervalUpperValueIndexes.GetSize() <= nRequestedQuantileNumber);
	return ivIntervalUpperValueIndexes.GetSize();
}

Continuous KWQuantileIntervalBuilder::GetIntervalLowerBoundAt(int nIntervalIndex) const
{
	int nLowerValueIndexes;

	require(IsValueInitialized());
	require(IsComputed());
	require(0 <= nIntervalIndex and nIntervalIndex < GetIntervalNumber());

	if (IsEqualWidth())
		return ComputeEqualWidthIntervalLowerBound(nIntervalIndex, GetIntervalNumber(), GetMinValue(),
							   GetMaxValue(), GetMissingValueNumber());
	else
	{
		nLowerValueIndexes = GetIntervalFirstValueIndexAt(nIntervalIndex);
		if (nLowerValueIndexes == 0)
			return KWContinuous::GetMissingValue();
		else
			return KWContinuous::GetHumanReadableLowerMeanValue(cvValues.GetAt(nLowerValueIndexes),
									    cvValues.GetAt(nLowerValueIndexes - 1));
	}
}

Continuous KWQuantileIntervalBuilder::GetIntervalUpperBoundAt(int nIntervalIndex) const
{
	int nUpperValueIndexes;

	require(IsValueInitialized());
	require(IsComputed());
	require(0 <= nIntervalIndex and nIntervalIndex < GetIntervalNumber());

	if (IsEqualWidth())
		return ComputeEqualWidthIntervalUpperBound(nIntervalIndex, GetIntervalNumber(), GetMinValue(),
							   GetMaxValue(), GetMissingValueNumber());
	else
	{
		nUpperValueIndexes = GetIntervalLastValueIndexAt(nIntervalIndex);
		if (nUpperValueIndexes == GetValueNumber() - 1)
			return KWContinuous::GetMaxValue();
		else
			return KWContinuous::GetHumanReadableLowerMeanValue(cvValues.GetAt(nUpperValueIndexes),
									    cvValues.GetAt(nUpperValueIndexes + 1));
	}
}

void KWQuantileIntervalBuilder::WriteIntervals(ostream& ost) const
{
	int nInterval;

	require(IsComputed());

	// Entete
	ost << "Quantiles";
	if (IsEqualWidth())
		ost << "EqualWidth";
	ost << " (" << GetQuantileNumber() << ") -> " << GetIntervalNumber() << " intervals" << endl;

	// Affichage des intervals
	for (nInterval = 0; nInterval < GetIntervalNumber(); nInterval++)
	{
		cout << "\t" << GetIntervalFrequencyAt(nInterval);
		if (IsValueInitialized())
		{
			cout << "\t" << KWContinuous::ContinuousToString(GetIntervalLowerBoundAt(nInterval));
			cout << "\t" << KWContinuous::ContinuousToString(GetIntervalUpperBoundAt(nInterval));
		}
		cout << endl;
	}
}

int KWQuantileIntervalBuilder::GetComputedQuantileNumber() const
{
	return GetIntervalNumber();
}

int KWQuantileIntervalBuilder::GetQuantileFrequencyAt(int nQuantileIndex) const
{
	return GetIntervalFrequencyAt(nQuantileIndex);
}

void KWQuantileIntervalBuilder::WriteQuantiles(ostream& ost) const
{
	WriteIntervals(ost);
}

void KWQuantileIntervalBuilder::Test()
{
	ContinuousVector cvInputValues;
	int i;

	// Test avec des valeurs toutes differentes
	cvInputValues.SetSize(10);
	for (i = 0; i < cvInputValues.GetSize(); i++)
		cvInputValues.SetAt(i, i);
	TestWithValues("Distinct values", &cvInputValues, 3);
	TestWithValues("Distinct values", &cvInputValues, 4);
	TestWithValues("Distinct values", &cvInputValues, 5);

	// Test avec des valeurs confondues
	cvInputValues.SetSize(10);
	for (i = 0; i < cvInputValues.GetSize(); i++)
		cvInputValues.SetAt(i, i / 3);
	TestWithValues("Few values", &cvInputValues, 3);
	TestWithValues("Few values", &cvInputValues, 4);
	TestWithValues("Few values", &cvInputValues, 5);

	// Test avec des valeurs confondues (variante)
	cvInputValues.SetSize(10);
	for (i = 0; i < cvInputValues.GetSize(); i++)
		cvInputValues.SetAt(i, (i + 2) / 3);
	TestWithValues("Few values (variant)", &cvInputValues, 3);
	TestWithValues("Few values (variant)", &cvInputValues, 4);
	TestWithValues("Few values (variant)", &cvInputValues, 5);

	// Test avec deux valeurs, la rare au debut
	cvInputValues.SetSize(10);
	for (i = 0; i < cvInputValues.GetSize(); i++)
		cvInputValues.SetAt(i, 1);
	cvInputValues.SetAt(0, 0);
	TestWithValues("Two values, rare at begin", &cvInputValues, 4);

	// Test avec deux valeurs, la rare a la fin
	cvInputValues.SetSize(10);
	for (i = 0; i < cvInputValues.GetSize(); i++)
		cvInputValues.SetAt(i, 0);
	cvInputValues.SetAt(cvInputValues.GetSize() - 1, 1);
	TestWithValues("Two values, rare at end", &cvInputValues, 4);

	// Test avec une seule valeur
	cvInputValues.SetSize(10);
	for (i = 0; i < cvInputValues.GetSize(); i++)
		cvInputValues.SetAt(i, 0);
	TestWithValues("One value", &cvInputValues, 4);

	// Test avec une seule valeur et une missing
	cvInputValues.SetSize(10);
	for (i = 0; i < cvInputValues.GetSize(); i++)
		cvInputValues.SetAt(i, 0);
	cvInputValues.SetAt(0, KWContinuous::GetMissingValue());
	TestWithValues("One value plus one missing", &cvInputValues, 4);

	// Test avec des valeurs manquantes plus une missing
	cvInputValues.SetSize(10);
	for (i = 0; i < cvInputValues.GetSize(); i++)
		cvInputValues.SetAt(i, KWContinuous::GetMissingValue());
	cvInputValues.SetAt(cvInputValues.GetSize() - 1, 0);
	TestWithValues("Missing values plus one actual value", &cvInputValues, 4);
}

void KWQuantileIntervalBuilder::TestWithValues(const ALString& sTestLabel, const ContinuousVector* cvInputValues,
					       int nQuantileNumber)
{
	boolean bDisplayValues = false;
	boolean bDisplayQuantileIndexes = false;
	KWQuantileIntervalBuilder quantileBuilder;
	KWQuantileIntervalBuilder quantileBuilderFromFrequencies;
	IntVector ivFrequencies;
	int nIntervalNumber;
	int i;
	boolean bOk;

	require(cvInputValues != NULL);
	require(nQuantileNumber >= 1);

	// Initialisation
	quantileBuilder.InitializeValues(cvInputValues);
	cout << sTestLabel << " (" << nQuantileNumber << " quantiles)" << endl;

	// Affichage des valeurs
	cout << "  Values (" << cvInputValues->GetSize() << "): ";
	for (i = 0; i < cvInputValues->GetSize(); i++)
	{
		if (i > 0)
			cout << ", ";
		cout << KWContinuous::ContinuousToString(cvInputValues->GetAt(i));
		if (i >= 10)
		{
			if (i < cvInputValues->GetSize() - 1)
				cout << ", ...";
			break;
		}
	}
	cout << endl;

	// Affichage des valeurs uniques
	if (bDisplayValues)
	{
		cout << "  Unique values (" << quantileBuilder.GetValueNumber() << "): ";
		for (i = 0; i < quantileBuilder.GetValueNumber(); i++)
		{
			if (i > 0)
				cout << ", ";
			cout << KWContinuous::ContinuousToString(quantileBuilder.GetValueAt(i));
			cout << " (" << quantileBuilder.ivValuesCumulatedFrequencies.GetAt(i) << ")";
			if (i >= 10)
			{
				if (i < quantileBuilder.GetValueNumber() - 1)
					cout << ", ...";
				break;
			}
		}
		cout << endl;
	}

	// Calcul et affichage des quantiles
	nIntervalNumber = quantileBuilder.ComputeQuantiles(nQuantileNumber);
	if (bDisplayQuantileIndexes)
	{
		cout << "  Interval quantile indexes: ";
		for (i = 0; i < nIntervalNumber; i++)
		{
			if (i > 0)
				cout << ", ";
			cout << quantileBuilder.GetIntervalQuantileIndexAt(i);
			if (i >= 10)
			{
				if (i < nIntervalNumber - 1)
					cout << ", ...";
				break;
			}
		}
		cout << endl;
	}

	// Affichage des intervales
	quantileBuilder.WriteIntervals(cout);

	// Initialisation a partir des effectifs
	cout << "Build quantiles from frequencies: ";
	ivFrequencies.SetSize(quantileBuilder.GetValueNumber());
	for (i = 0; i < quantileBuilder.GetValueNumber(); i++)
		ivFrequencies.SetAt(i, quantileBuilder.GetValueFrequencyAt(i));
	quantileBuilderFromFrequencies.InitializeFrequencies(&ivFrequencies);
	quantileBuilderFromFrequencies.ComputeQuantiles(nQuantileNumber);

	// On verifie que l'on trouve les meme intervalles
	bOk = (quantileBuilder.GetIntervalNumber() == quantileBuilderFromFrequencies.GetIntervalNumber());
	if (bOk)
	{
		for (i = 0; i < quantileBuilder.GetIntervalNumber(); i++)
			bOk = bOk and quantileBuilder.GetIntervalFrequencyAt(i) ==
					  quantileBuilderFromFrequencies.GetIntervalFrequencyAt(i);
	}
	cout << BooleanToString(bOk) << endl;
}

Continuous KWQuantileIntervalBuilder::ComputeEqualWidthIntervalLowerBound(int nIntervalIndex, int nIntervalNumber,
									  Continuous cMinValue, Continuous cMaxValue,
									  int nMissingNumber) const
{
	require(0 <= nIntervalIndex and nIntervalIndex < nIntervalNumber);
	require(cMinValue <= cMaxValue);
	require(nIntervalNumber >= 1);
	require(nMissingNumber >= 0);

	// On se base sur le calcul de la borne sup de l'interval precedent
	return nIntervalIndex == 0 ? cMinValue
				   : ComputeEqualWidthIntervalUpperBound(nIntervalIndex - 1, nIntervalNumber, cMinValue,
									 cMaxValue, nMissingNumber);
}

Continuous KWQuantileIntervalBuilder::ComputeEqualWidthIntervalUpperBound(int nIntervalIndex, int nIntervalNumber,
									  Continuous cMinValue, Continuous cMaxValue,
									  int nMissingNumber) const
{
	require(0 <= nIntervalIndex and nIntervalIndex < nIntervalNumber);
	require(cMinValue <= cMaxValue);
	require(cMinValue != KWContinuous::GetMissingValue() or cMaxValue == KWContinuous::GetMissingValue());
	require(nIntervalNumber >= 1);
	require(nMissingNumber >= 0);

	// Si l'on est sur le dernier intervalle, on renvoie la valeur max
	if (nIntervalIndex == nIntervalNumber - 1)
		return cMaxValue;
	// Cas sans valeurs manquantes
	else if (nMissingNumber == 0)
	{
		if (cMinValue == cMaxValue)
			return cMaxValue;
		else
			// On passe par une normalisation de la representation des Continuous
			return KWContinuous::DoubleToContinuous(
			    cMinValue + (nIntervalIndex + 1) * (cMaxValue - cMinValue) / nIntervalNumber);
	}
	// Cas avec valeurs manquantes
	else
	{
		// Le premier intervalle est dedie aux valeurs manquantes
		if (nIntervalIndex == 0)
			return KWContinuous::GetMissingValue();
		// Il faut diviser la plage de valeur restante avec un intervalle en moins
		else
		{
			if (cMinValue == cMaxValue)
				return cMaxValue;
			else
				// On passe par une normalisation de la representation des Continuous
				return KWContinuous::DoubleToContinuous(
				    cMinValue + nIntervalIndex * (cMaxValue - cMinValue) / (nIntervalNumber - 1));
		}
	}
}

int KWQuantileIntervalBuilder::SearchCumulativeFrequencyIndex(boolean bSequentialSearch,
							      int nSearchedCumulativeFrequency, int nLowerIndex,
							      int nUpperIndex) const
{
	int nIndex;

	require(IsFrequencyInitialized());
	require(ivValuesCumulatedFrequencies.GetSize() > 0);
	require(0 <= nLowerIndex and nLowerIndex < ivValuesCumulatedFrequencies.GetSize());
	require(nLowerIndex <= nUpperIndex and nLowerIndex < ivValuesCumulatedFrequencies.GetSize());
	require(0 <= nSearchedCumulativeFrequency and nSearchedCumulativeFrequency <= GetInstanceNumber());

	// Cas d'une recherche sequentielle
	if (bSequentialSearch)
	{
		nIndex = nLowerIndex;
		while (ivValuesCumulatedFrequencies.GetAt(nIndex) < nSearchedCumulativeFrequency)
			nIndex++;
	}
	// Sinon recherche dichotomique de l'intervalle
	else
	{
		nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
		while (nLowerIndex + 1 < nUpperIndex)
		{
			// Deplacement des bornes de recherche en fonction
			// de la comparaison avec la borne courante
			if (nSearchedCumulativeFrequency <= ivValuesCumulatedFrequencies.GetAt(nIndex))
				nUpperIndex = nIndex;
			else
				nLowerIndex = nIndex;

			// Modification du prochain intervalle teste
			nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
		}
		assert(nLowerIndex <= nUpperIndex);
		assert(nUpperIndex <= nLowerIndex + 1);

		// On compare par rapport aux deux index restant
		if (nSearchedCumulativeFrequency <= ivValuesCumulatedFrequencies.GetAt(nLowerIndex))
			nIndex = nLowerIndex;
		else
			nIndex = nUpperIndex;
	}

	ensure(nSearchedCumulativeFrequency <= ivValuesCumulatedFrequencies.GetAt(nIndex));
	ensure(nIndex == 0 or ivValuesCumulatedFrequencies.GetAt(nIndex - 1) < nSearchedCumulativeFrequency);
	return nIndex;
}

int KWQuantileIntervalBuilder::SearchValueIndex(Continuous cSearchedValue, int nLowerIndex, int nUpperIndex) const
{
	int nIndex;

	require(IsFrequencyInitialized());
	require(cvValues.GetSize() > 0);
	require(0 <= nLowerIndex and nLowerIndex < cvValues.GetSize());
	require(nLowerIndex <= nUpperIndex and nLowerIndex < cvValues.GetSize());
	require(cvValues.GetAt(nLowerIndex) <= cSearchedValue);
	require(cSearchedValue <= cvValues.GetAt(nUpperIndex));

	// Recherche dichotomique de l'intervalle
	nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
	while (nLowerIndex + 1 < nUpperIndex)
	{
		// Deplacement des bornes de recherche en fonction
		// de la comparaison avec la borne courante
		if (cSearchedValue <= cvValues.GetAt(nIndex))
			nUpperIndex = nIndex;
		else
			nLowerIndex = nIndex;

		// Modification du prochain intervalle teste
		nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
	}
	assert(nLowerIndex <= nUpperIndex);
	assert(nUpperIndex <= nLowerIndex + 1);

	// On compare par rapport aux deux index restant
	if (cvValues.GetAt(nUpperIndex) > cSearchedValue)
		nIndex = nLowerIndex;
	else
		nIndex = nUpperIndex;
	ensure(cvValues.GetAt(nIndex) <= cSearchedValue);
	ensure(nIndex == cvValues.GetSize() - 1 or cSearchedValue < cvValues.GetAt(nIndex + 1));
	return nIndex;
}

/////////////////////////////////////////////////////////////////////////
// Classe KWQuantileGroupBuilder

KWQuantileGroupBuilder::KWQuantileGroupBuilder()
{
	bIsFrequencyInitialized = false;
	bIsValueInitialized = false;
	nInstanceNumber = 0;
	nValueNumber = 0;
	nRequestedQuantileNumber = 0;
	nGroupNumber = 0;
}

KWQuantileGroupBuilder::~KWQuantileGroupBuilder() {}

void KWQuantileGroupBuilder::InitializeValues(const SymbolVector* svInputValues)
{
	Symbol sValue;
	KWSortableFrequencySymbol* sortableValueFrequency;
	ObjectArray oaValueFrequencies;
	int nCumulatedFrequency;
	int i;

	require(svInputValues != NULL);

	// Reinitialisation des resultats
	bIsValueInitialized = true;
	bIsFrequencyInitialized = true;
	nRequestedQuantileNumber = 0;
	nGroupNumber = 0;

	// Memorisation du nombre d'instances
	nInstanceNumber = svInputValues->GetSize();

	// Collecte des valeurs et de leur effectif
	// On utilise des SortableSymbol, en prenant leur SortValue pour memoriser la valeur
	// et l'index pour memoriser l'effectif
	sortableValueFrequency = NULL;
	for (i = 0; i < svInputValues->GetSize(); i++)
	{
		sValue = svInputValues->GetAt(i);
		assert(i == 0 or sValue >= svInputValues->GetAt(i - 1));

		// Creation d'un nouvel enregistrement si necessaire
		if (i == 0 or sValue > svInputValues->GetAt(i - 1))
		{
			// Memorisation de la valeur et de l'index de la premiere instance correspondant a la valeur
			// L'index est memorise pour des evolutions eventuelles des services de la classe
			sortableValueFrequency = new KWSortableFrequencySymbol;
			sortableValueFrequency->SetSortValue(sValue);
			sortableValueFrequency->SetIndex(i);
			oaValueFrequencies.Add(sortableValueFrequency);
		}

		// Mise a jour de l'effectif de la valeur
		sortableValueFrequency->SetFrequency(sortableValueFrequency->GetFrequency() + 1);
	}

	// Tri des valeurs par effectif
	oaValueFrequencies.SetCompareFunction(KWSortableFrequencySymbolCompare);
	oaValueFrequencies.Sort();

	// Memorisation des valeurs uniques
	nValueNumber = oaValueFrequencies.GetSize();
	svValues.SetSize(nValueNumber);
	ivValuesCumulatedFrequencies.SetSize(nValueNumber);
	nCumulatedFrequency = 0;
	for (i = 0; i < nValueNumber; i++)
	{
		sortableValueFrequency = cast(KWSortableFrequencySymbol*, oaValueFrequencies.GetAt(i));
		svValues.SetAt(i, sortableValueFrequency->GetSortValue());
		nCumulatedFrequency += sortableValueFrequency->GetFrequency();
		ivValuesCumulatedFrequencies.SetAt(i, nCumulatedFrequency);
	}

	// Nettoyage
	oaValueFrequencies.DeleteAll();
	ensure(svValues.GetSize() <= svInputValues->GetSize());
	ensure(svValues.GetSize() == ivValuesCumulatedFrequencies.GetSize());
	ensure(ivValuesCumulatedFrequencies.GetSize() == 0 or
	       ivValuesCumulatedFrequencies.GetAt(ivValuesCumulatedFrequencies.GetSize() - 1) ==
		   svInputValues->GetSize());
}

void KWQuantileGroupBuilder::InitializeFrequencies(const IntVector* ivInputFrequencies)
{
	int i;

	require(ivInputFrequencies != NULL);

	// Reinitialisation des resultats
	nRequestedQuantileNumber = 0;

	// Calcul et memorisation des effectifs cumules
	bIsValueInitialized = false;
	bIsFrequencyInitialized = true;
	nInstanceNumber = 0;
	nValueNumber = ivInputFrequencies->GetSize();
	svValues.SetSize(0);
	ivValuesCumulatedFrequencies.SetSize(nValueNumber);
	for (i = 0; i < nValueNumber; i++)
	{
		assert(ivInputFrequencies->GetAt(i) > 0);
		assert(i == 0 or ivInputFrequencies->GetAt(i - 1) >= ivInputFrequencies->GetAt(i));
		nInstanceNumber += ivInputFrequencies->GetAt(i);
		ivValuesCumulatedFrequencies.SetAt(i, nInstanceNumber);
	}
	ensure(ivValuesCumulatedFrequencies.GetSize() == 0 or
	       ivValuesCumulatedFrequencies.GetAt(ivValuesCumulatedFrequencies.GetSize() - 1) == nInstanceNumber);
}

int KWQuantileGroupBuilder::ComputeQuantiles(int nQuantileNumber)
{
	IntVector ivQuantileUpperValueIndexes;
	int nSearchedMinFrequency;
	int nLastValueIndex;
	int nIndex;

	require(IsFrequencyInitialized());
	require(GetValueNumber() > 0);
	require(nQuantileNumber >= 1);

	// Initialisation des resultats
	nRequestedQuantileNumber = nQuantileNumber;
	nGroupNumber = 0;

	// Calcul de l'effectif min recherche
	nSearchedMinFrequency = (int)ceil(GetInstanceNumber() * 1.0 / nQuantileNumber);

	// Index de la derniere valeur d'effectif depassant l'effectif recherche
	if (nQuantileNumber >= nValueNumber)
		nLastValueIndex = nValueNumber - 1;
	else
		nLastValueIndex = nQuantileNumber - 1;
	nIndex = SearchFrequencyIndex(nSearchedMinFrequency, 0, nLastValueIndex);

	// Determination du nombre de groupes en prenant en compte le groupe poubelle
	nGroupNumber = nIndex + 1;
	if (nGroupNumber < nValueNumber)
	{
		if (nIndex > 0 or GetValueCumulatedFrequencAt(0) >= nSearchedMinFrequency)
			nGroupNumber++;
	}
	return nGroupNumber;
}

void KWQuantileGroupBuilder::WriteGroups(ostream& ost) const
{
	int nGroup;
	int nValue;

	require(IsComputed());

	// Entete
	ost << "Quantiles (" << GetQuantileNumber() << ") -> " << GetGroupNumber() << " groups" << endl;

	// Affichage des intervals
	for (nGroup = 0; nGroup < GetGroupNumber(); nGroup++)
	{
		cout << "\t" << GetGroupFrequencyAt(nGroup);
		cout << "\t" << GetGroupValueNumberAt(nGroup);
		if (IsValueInitialized())
		{
			cout << "\t" << GetValueAt(GetGroupFirstValueIndexAt(nGroup));
			for (nValue = 1; nValue < GetGroupValueNumberAt(nGroup); nValue++)
			{
				cout << ", " << GetValueAt(GetGroupFirstValueIndexAt(nGroup) + nValue);
				if (nValue >= 3)
				{
					cout << ",...";
					break;
				}
			}
		}
		cout << endl;
	}
}

int KWQuantileGroupBuilder::GetComputedQuantileNumber() const
{
	return GetGroupNumber();
}

int KWQuantileGroupBuilder::GetQuantileFrequencyAt(int nQuantileIndex) const
{
	return GetGroupFrequencyAt(nQuantileIndex);
}

void KWQuantileGroupBuilder::WriteQuantiles(ostream& ost) const
{
	WriteGroups(ost);
}

void KWQuantileGroupBuilder::Test()
{
	SymbolVector svValues;
	int i;
	ALString sTmp;

	// Test avec des valeurs toutes differentes
	svValues.SetSize(10);
	for (i = 0; i < svValues.GetSize(); i++)
		svValues.SetAt(i, Symbol(sTmp + "v" + IntToString(i)));
	TestWithValues("Distinct values", &svValues, 3);
	TestWithValues("Distinct values", &svValues, 4);
	TestWithValues("Distinct values", &svValues, 5);

	// Test avec des valeurs confondues
	svValues.SetSize(10);
	for (i = 0; i < svValues.GetSize(); i++)
		svValues.SetAt(i, Symbol(sTmp + "v" + IntToString(i / 3)));
	TestWithValues("Few values", &svValues, 3);
	TestWithValues("Few values", &svValues, 4);
	TestWithValues("Few values", &svValues, 5);

	// Test avec des valeurs confondues (variante)
	svValues.SetSize(10);
	for (i = 0; i < svValues.GetSize(); i++)
		svValues.SetAt(i, Symbol(sTmp + "v" + IntToString((i + 2) / 3)));
	TestWithValues("Few values (variant)", &svValues, 3);
	TestWithValues("Few values (variant)", &svValues, 4);
	TestWithValues("Few values (variant)", &svValues, 5);

	// Test avec deux valeurs, la rare au debut
	svValues.SetSize(10);
	for (i = 0; i < svValues.GetSize(); i++)
		svValues.SetAt(i, Symbol("v1"));
	svValues.SetAt(0, Symbol("v0"));
	TestWithValues("Two values, rare at begin", &svValues, 4);

	// Test avec deux valeurs, la rare a la fin
	svValues.SetSize(10);
	for (i = 0; i < svValues.GetSize(); i++)
		svValues.SetAt(i, Symbol("v0"));
	svValues.SetAt(svValues.GetSize() - 1, Symbol("v1"));
	TestWithValues("Two values, rare at end", &svValues, 4);

	// Test avec une seule valeur
	svValues.SetSize(10);
	for (i = 0; i < svValues.GetSize(); i++)
		svValues.SetAt(i, Symbol("v0"));
	TestWithValues("One value", &svValues, 4);
}

void KWQuantileGroupBuilder::TestWithValues(const ALString& sTestLabel, const SymbolVector* svValues,
					    int nQuantileNumber)
{
	boolean bDisplayValues = false;
	boolean bDisplayQuantileIndexes = false;
	KWQuantileGroupBuilder quantileBuilder;
	KWQuantileGroupBuilder quantileBuilderFromFrequencies;
	IntVector ivFrequencies;
	SymbolVector svSortedValues;
	int nGroupNumber;
	int i;
	boolean bOk;

	require(svValues != NULL);
	require(nQuantileNumber >= 1);

	// Tri des valeurs initiales
	svSortedValues.CopyFrom(svValues);
	svSortedValues.SortKeys();

	// Initialisation
	quantileBuilder.InitializeValues(&svSortedValues);
	cout << sTestLabel << " (" << nQuantileNumber << " quantiles)" << endl;

	// Affichage des valeurs
	cout << "  Values (" << svValues->GetSize() << "): ";
	for (i = 0; i < svValues->GetSize(); i++)
	{
		if (i > 0)
			cout << ", ";
		cout << svValues->GetAt(i);
		if (i >= 10)
		{
			if (i < svValues->GetSize() - 1)
				cout << ", ...";
			break;
		}
	}
	cout << endl;

	// Affichage des valeurs uniques
	if (bDisplayValues)
	{
		cout << "  Unique values (" << quantileBuilder.GetValueNumber() << "): ";
		for (i = 0; i < quantileBuilder.svValues.GetSize(); i++)
		{
			if (i > 0)
				cout << ", ";
			cout << quantileBuilder.svValues.GetAt(i);
			cout << " (" << quantileBuilder.ivValuesCumulatedFrequencies.GetAt(i) << ")";
			if (i >= 10)
			{
				if (i < quantileBuilder.GetValueNumber() - 1)
					cout << ", ...";
				break;
			}
		}
		cout << endl;
	}

	// Calcul et affichage des quantiles
	nGroupNumber = quantileBuilder.ComputeQuantiles(nQuantileNumber);
	if (bDisplayQuantileIndexes)
	{
		cout << "  Group quantile indexes: ";
		for (i = 0; i < nGroupNumber; i++)
		{
			if (i > 0)
				cout << ", ";
			cout << quantileBuilder.GetGroupQuantileIndexAt(i);
			if (i >= 10)
			{
				if (i < nGroupNumber - 1)
					cout << ", ...";
				break;
			}
		}
		cout << endl;
	}

	// Affichage des valeurs des groupes
	quantileBuilder.WriteGroups(cout);

	// Initialisation a partir des effectifs
	cout << "Build quantiles from frequencies: ";
	ivFrequencies.SetSize(quantileBuilder.GetValueNumber());
	for (i = 0; i < quantileBuilder.GetValueNumber(); i++)
		ivFrequencies.SetAt(i, quantileBuilder.GetValueFrequencyAt(i));
	quantileBuilderFromFrequencies.InitializeFrequencies(&ivFrequencies);
	quantileBuilderFromFrequencies.ComputeQuantiles(nQuantileNumber);

	// On verifie que l'on trouve les meme intervalles
	bOk = (quantileBuilder.GetGroupNumber() == quantileBuilderFromFrequencies.GetGroupNumber());
	if (bOk)
	{
		for (i = 0; i < quantileBuilder.GetGroupNumber(); i++)
			bOk = bOk and quantileBuilder.GetGroupFrequencyAt(i) ==
					  quantileBuilderFromFrequencies.GetGroupFrequencyAt(i);
	}
	cout << BooleanToString(bOk) << endl;
}

int KWQuantileGroupBuilder::SearchFrequencyIndex(int nSearchedFrequency, int nLowerIndex, int nUpperIndex) const
{
	int nIndex;

	require(IsFrequencyInitialized());
	require(ivValuesCumulatedFrequencies.GetSize() > 0);
	require(0 <= nLowerIndex and nLowerIndex < ivValuesCumulatedFrequencies.GetSize());
	require(nLowerIndex <= nUpperIndex and nLowerIndex < ivValuesCumulatedFrequencies.GetSize());
	require(0 <= nSearchedFrequency and nSearchedFrequency <= GetInstanceNumber());

	// Recherche dichotomique de l'intervalle
	nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
	while (nLowerIndex + 1 < nUpperIndex)
	{
		// Deplacement des bornes de recherche en fonction
		// de la comparaison avec la borne courante
		if (nSearchedFrequency > GetValueFrequencyAt(nIndex))
			nUpperIndex = nIndex;
		else
			nLowerIndex = nIndex;

		// Modification du prochain intervalle teste
		nIndex = (nLowerIndex + nUpperIndex + 1) / 2;
	}
	assert(nLowerIndex <= nUpperIndex);
	assert(nUpperIndex <= nLowerIndex + 1);

	// On compare par rapport aux deux index restant
	if (GetValueFrequencyAt(nUpperIndex) < nSearchedFrequency)
		nIndex = nLowerIndex;
	else
		nIndex = nUpperIndex;
	ensure(nSearchedFrequency <= GetValueFrequencyAt(nIndex) or nIndex == 0);
	ensure(nIndex == ivValuesCumulatedFrequencies.GetSize() - 1 or
	       GetValueFrequencyAt(nIndex + 1) < nSearchedFrequency);
	return nIndex;
}
