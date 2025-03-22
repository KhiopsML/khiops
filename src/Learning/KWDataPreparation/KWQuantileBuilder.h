// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

// Service  de calcul de quantiles intervalles ou groupes
class KWQuantileBuilder;
class KWQuantileIntervalBuilder;
class KWQuantileGroupBuilder;

#include "KWType.h"
#include "KWContinuous.h"
#include "KWSymbol.h"
#include "KWSortableIndex.h"
#include "Vector.h"

//////////////////////////////////////////////////////////////////////////////
// Service de calcul de quantiles intervalles a partir d'un vecteur de valeurs continues
class KWQuantileBuilder : public Object
{
public:
	// Constructeur
	KWQuantileBuilder();
	~KWQuantileBuilder();

	// Type de quantile collectes
	virtual int GetType() const = 0;

	// Test si initialise
	boolean IsFrequencyInitialized() const;

	// Nombre d'instances
	int GetInstanceNumber() const;

	// Nombre de valeurs uniques
	virtual int GetValueNumber() const;

	//////////////////////////////////////////////////////////////////////////
	// Calcul de quantiles

	// Calcul des quantiles
	virtual int ComputeQuantiles(int nQuantileNumber) = 0;

	// Test si quantiles calcules
	boolean IsComputed() const;

	// Nombre de quantiles demandes lors du dernier calcul
	int GetQuantileNumber() const;

	//////////////////////////////////////////////////////////////////////////
	// Acces aux resultats par quantile

	// Nombre de quantiles effectivement produits
	virtual int GetComputedQuantileNumber() const = 0;

	// Effectif d'un quantile
	virtual int GetQuantileFrequencyAt(int nQuantileIndex) const = 0;

	// Affichage des quantiles
	virtual void WriteQuantiles(ostream& ost) const = 0;

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Statistiques sur les valeurs en entrees
	int nInstanceNumber;
	int nValueNumber;

	// Indicateur d'initialisation
	boolean bIsFrequencyInitialized;
	boolean bIsValueInitialized;

	// Effectifs cumules par valeur en entrees
	IntVector ivValuesCumulatedFrequencies;

	// Nombre de quantiles demandes
	int nRequestedQuantileNumber;
};

//////////////////////////////////////////////////////////////////////////////
// Service de calcul de quantiles intervalles a partir d'un vecteur de valeurs continues
class KWQuantileIntervalBuilder : public KWQuantileBuilder
{
public:
	// Constructeur
	KWQuantileIntervalBuilder();
	~KWQuantileIntervalBuilder();

	// Type de quantile collectes: Continuous
	int GetType() const override;

	//////////////////////////////////////////////////////////////////////////
	// Initialisation et calcul de statistiques sur les valeurs a traiter

	// Initialisation a partir d'un vecteur de valeurs continues triees,
	// comportant potentiellement des doublons
	// Permet d'acceder a tous les services, portant sur les valeurs et les effectifs
	// Memoire: les valeurs appartiennent a l'appelant
	void InitializeValues(const ContinuousVector* cvInputValues);

	// Initialisation a partir d'un vecteur d'effectifs correspondant aux valeurs continues triees,
	// comportant potentiellement des doublons
	// Dans ce cas, les services portant sur les valeurs ne seront pas accessibles
	// Memoire: les effctifs appartiennent a l'appelant
	void InitializeFrequencies(const IntVector* ivInputFrequencies);

	// Test si initialise par les valeurs
	boolean IsValueInitialized() const;

	// Acces a la frequence des valeurs
	int GetValueFrequencyAt(int nIndex) const;

	// Acces a la frequence cumulee des valeurs
	int GetValueCumulatedFrequencAt(int nIndex) const;

	// Acces aux valeurs (prerequis: initialisation valeur)
	Continuous GetValueAt(int nIndex) const;

	// Valeur min, en ignorant les valeur manquantes
	Continuous GetMinValue() const;

	// Valeur max
	Continuous GetMaxValue() const;

	// Nombre d'instances avec valeur manquante (prerequis: initialisation valeur)
	int GetMissingValueNumber() const;

	//////////////////////////////////////////////////////////////////////////
	// Calcul de quantiles

	// Calcul de quantiles d'effectif egal
	// En raison de doublons potentiels sur les valeurs, le nombre d'intervalles produits peut
	// etre inferieur au nombre de quantiles demandes
	// Pour chaque frontiere de quantile, si le point de coupure theorique tombe au milieu d'une plage de
	// valeurs egales, on recherche le point de coupure:
	//  . le plus proche (valeur distincte la plus proche en nombre d'instances, vers le debut ou la fin)
	//  . si deux valeurs a egale distances, on prend la plus proche du bord (debut ou fin)
	//  . si encore egalite, on prend la plus proche du debut
	// On retourne le nombre de quantiles effectivement produits
	int ComputeQuantiles(int nQuantileNumber) override;

	// Calcul de quantiles de largeur egale (prerequis: initialisation valeur)
	// En cas de valeur manquante, un intervalle est cree uniquement pour la valeur manquante
	// On retourne le nombre de quantiles demandes, y compris les intervalles vides
	int ComputeEqualWidthQuantiles(int nQuantileNumber);

	//////////////////////////////////////////////////////////////////////////
	// Acces aux resultats par quantile

	// Indique si on a demande des quantiles en EqualWidth
	boolean IsEqualWidth() const;

	// Nombre d'intervalles effectivement produits
	int GetIntervalNumber() const;

	// Index des instances extremites de chaque intervalle
	int GetIntervalFirstInstanceIndexAt(int nIntervalIndex) const;
	int GetIntervalLastInstanceIndexAt(int nIntervalIndex) const;

	// Index des valeurs extremites de chaque intervalle
	int GetIntervalFirstValueIndexAt(int nIntervalIndex) const;
	int GetIntervalLastValueIndexAt(int nIntervalIndex) const;

	// Effectif d'un intervalle
	int GetIntervalFrequencyAt(int nIntervalIndex) const;

	// Index de quantile pour chaque intervalle
	// En effet, certains quantiles n'ont pas abouti a la creation d'un intervalle
	int GetIntervalQuantileIndexAt(int nIntervalIndex) const;

	// Bornes inf et sup de chaque intervalle (prerequis: initialisation valeur)
	Continuous GetIntervalLowerBoundAt(int nIntervalIndex) const;
	Continuous GetIntervalUpperBoundAt(int nIntervalIndex) const;

	// Affichage des intervalles
	void WriteIntervals(ostream& ost) const;

	// Redefinition des methodes virtuelles
	int GetComputedQuantileNumber() const override;
	int GetQuantileFrequencyAt(int nQuantileIndex) const override;
	void WriteQuantiles(ostream& ost) const override;

	// Methode de test
	static void Test();

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Methode de test pour un vecteur de valeur et un nombre de quantiles
	static void TestWithValues(const ALString& sTestLabel, const ContinuousVector* cvInputValues,
				   int nQuantileNumber);

	// Calcul des bornes d'un intervale EqualWidth pour un domaine numerique donne
	// En cas de valeurs manquantes, le premier intervalle est dedie a celles ci
	Continuous ComputeEqualWidthIntervalLowerBound(int nIntervalIndex, int nIntervalNumber, Continuous cMinValue,
						       Continuous cMaxValue, int nMissingNumber) const;
	Continuous ComputeEqualWidthIntervalUpperBound(int nIntervalIndex, int nIntervalNumber, Continuous cMinValue,
						       Continuous cMaxValue, int nMissingNumber) const;

	// Recherche de l'index d'un effectif cumule (entre 0 et n) dans le vecteur des effectifs cumules, entre deux
	// index extremites On rend l'index de la plus petite valeur dont l'effectif cumule est superieur ou egal a
	// l'effectif cumule recherche En entree, si bSequentialSearch est a true, la recherche est sequentielle. Sinon
	// elle est dichotomique
	int SearchCumulativeFrequencyIndex(boolean bSequentialSearch, int nSearchedCumulativeFrequency, int nLowerIndex,
					   int nUpperIndex) const;

	// Recherche de l'index d'une valeur dans le vecteur des valeurs uniques, entre deux index extremites
	// On rend l'index de la plus grande valeur inferieure ou egale a la valeur recherchee
	int SearchValueIndex(Continuous cSearchedValue, int nLowerIndex, int nUpperIndex) const;

	// Statistiques sur les valeurs en entrees
	ContinuousVector cvValues;

	// Resultats de calcul des quantiles
	boolean bIsEqualWidth;
	IntVector ivIntervalQuantileIndexes;
	IntVector ivIntervalUpperValueIndexes;
};

//////////////////////////////////////////////////////////////////////////////
// Service de calcul de quantiles groupes a partir d'un vecteur de valeurs symbol
class KWQuantileGroupBuilder : public KWQuantileBuilder
{
public:
	// Constructeur
	KWQuantileGroupBuilder();
	~KWQuantileGroupBuilder();

	// Type de quantile collectes: Symbol
	int GetType() const override;

	//////////////////////////////////////////////////////////////////////////
	// Initialisation et calcul de statistiques sur les valeurs a traiter

	// Initialisation a partir d'un vecteur de valeurs continues symbol triees par cle numerique,
	// comportant potentiellement des doublons
	// Permet d'acceder a tous les services, portant sur les valeurs et les effectifs
	// Memoire: les valeurs appartiennent a l'appelant
	void InitializeValues(const SymbolVector* svInputValues);

	// Initialisation a partir d'un vecteur d'effectifs correspondant aux valeurs symbol
	// triee par effectif decroissant, puis par valeur
	// Dans ce cas, les services portant sur les valeurs ne seront pas accessibles
	// Memoire: les effectifs appartiennent a l'appelant
	void InitializeFrequencies(const IntVector* ivInputFrequencies);

	// Test si initialise
	boolean IsValueInitialized() const;

	// Acces a la frequence cumulee des valeurs
	int GetValueFrequencyAt(int nIndex) const;

	// Acces a la frequence cumulee des valeurs
	int GetValueCumulatedFrequencAt(int nIndex) const;

	// Acces aux valeurs, triees par effectifs decroissant,
	// et si egalite selon l'ordre lexicographique (prerequis: initialisation valeur)
	Symbol& GetValueAt(int nIndex) const;

	//////////////////////////////////////////////////////////////////////////
	// Calcul de quantiles

	// Calcul de quantiles d'effectif egal
	// Il s'agit en fait de rechercher toutes les valeurs dont l'effectif est au moins
	// celui d'un partile theorique (InstanceNumber/QuantileNumber).
	// Les petites valeurs sont rangees dans un dernier groupe poubelle
	int ComputeQuantiles(int nQuantileNumber) override;

	//////////////////////////////////////////////////////////////////////////
	// Acces aux resultats par quantile

	// Nombre de groupes effectivement produits
	// Les premiers groupes sont des singletons, dont la valeur est
	// directement avec l'index du groupe
	// Le denier groupe est le groupe poubelle, contenant toutes les petites valeurs
	int GetGroupNumber() const;

	// Index des valeurs extremites de chaque groupe
	// Seul le groupe poubelle (le dernier) peut avoir plusieurs valeurs,
	// dont les index vont de l'index du dernier groupe a l'index de la derniere valeur
	int GetGroupFirstValueIndexAt(int nGroupIndex) const;
	int GetGroupLastValueIndexAt(int nGroupIndex) const;

	// Effectif d'un groupe
	int GetGroupFrequencyAt(int nGroupIndex) const;

	// Nombre de valeurs d'un groupe
	int GetGroupValueNumberAt(int nGroupIndex) const;

	// Index de quantile pour chaque groupe
	// Les index de quantiles des groupes singletons coincident avec leur index de groupe
	// L'index de quantile du groupe poubelle est celui du dernier quantile demande
	// Les autres quantiles sont vides
	int GetGroupQuantileIndexAt(int nGroupIndex) const;

	// Affichage des groupes
	void WriteGroups(ostream& ost) const;

	// Redefinition des methodes virtuelles
	int GetComputedQuantileNumber() const override;
	int GetQuantileFrequencyAt(int nQuantileIndex) const override;
	void WriteQuantiles(ostream& ost) const override;

	// Methode de test
	static void Test();

	/////////////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Methode de test pour un vecteur de valeur et un nombre de quantiles
	static void TestWithValues(const ALString& sTestLabel, const SymbolVector* svValues, int nQuantileNumber);

	// Recherche de l'index d'un effectif dans le vecteur des effectifs cumules, entre deux index extremites
	// On rend l'index de la plus grande valeur dont l'effectif est superieur ou egal a l'effectif recherche
	// On rend 0 si meme la premiere valeur n'atteint pas l'effectif recherche
	int SearchFrequencyIndex(int nSearchedFrequency, int nLowerIndex, int nUpperIndex) const;

	// Statistiques sur les valeurs en entrees
	SymbolVector svValues;

	// Resultats de calcul des quantiles
	int nGroupNumber;
};

///////////////////////////////////////////
// Methodes en inline

inline boolean KWQuantileBuilder::IsFrequencyInitialized() const
{
	return bIsFrequencyInitialized;
}

inline int KWQuantileBuilder::GetInstanceNumber() const
{
	require(IsFrequencyInitialized());
	return nInstanceNumber;
}

inline int KWQuantileBuilder::GetValueNumber() const
{
	require(IsFrequencyInitialized());
	assert(nValueNumber == ivValuesCumulatedFrequencies.GetSize());
	return nValueNumber;
}

inline boolean KWQuantileBuilder::IsComputed() const
{
	return (nRequestedQuantileNumber > 0);
}

inline int KWQuantileBuilder::GetQuantileNumber() const
{
	require(IsComputed());
	return nRequestedQuantileNumber;
}

///

inline int KWQuantileIntervalBuilder::GetType() const
{
	return KWType::Continuous;
}

inline boolean KWQuantileIntervalBuilder::IsValueInitialized() const
{
	assert(cvValues.GetSize() == 0 or cvValues.GetSize() == nValueNumber);
	return bIsValueInitialized;
}

inline int KWQuantileIntervalBuilder::GetValueFrequencyAt(int nIndex) const
{
	require(IsFrequencyInitialized());
	require(0 <= nIndex and nIndex < GetValueNumber());
	if (nIndex == 0)
		return ivValuesCumulatedFrequencies.GetAt(nIndex);
	else
		return ivValuesCumulatedFrequencies.GetAt(nIndex) - ivValuesCumulatedFrequencies.GetAt(nIndex - 1);
}

inline int KWQuantileIntervalBuilder::GetValueCumulatedFrequencAt(int nIndex) const
{
	require(IsFrequencyInitialized());
	require(0 <= nIndex and nIndex < GetValueNumber());
	return ivValuesCumulatedFrequencies.GetAt(nIndex);
}

inline Continuous KWQuantileIntervalBuilder::GetValueAt(int nIndex) const
{
	require(IsFrequencyInitialized());
	require(IsValueInitialized());
	require(0 <= nIndex and nIndex < GetValueNumber());
	return cvValues.GetAt(nIndex);
}

inline Continuous KWQuantileIntervalBuilder::GetMinValue() const
{
	Continuous cMinValue;
	require(IsFrequencyInitialized());
	require(IsValueInitialized());
	cMinValue = cvValues.GetAt(0);
	if (cMinValue == KWContinuous::GetMissingValue() and cvValues.GetSize() > 1)
		cMinValue = cvValues.GetAt(1);
	return cMinValue;
}

inline Continuous KWQuantileIntervalBuilder::GetMaxValue() const
{
	require(IsFrequencyInitialized());
	require(IsValueInitialized());
	return cvValues.GetAt(cvValues.GetSize() - 1);
}

inline int KWQuantileIntervalBuilder::GetMissingValueNumber() const
{
	require(IsFrequencyInitialized());
	require(IsValueInitialized());
	if (cvValues.GetSize() == 0 or cvValues.GetAt(0) != KWContinuous::GetMissingValue())
		return 0;
	else
		return ivValuesCumulatedFrequencies.GetAt(0);
}

inline boolean KWQuantileIntervalBuilder::IsEqualWidth() const
{
	require(IsComputed());
	return bIsEqualWidth;
}

inline int KWQuantileIntervalBuilder::GetIntervalNumber() const
{
	require(IsComputed());
	return ivIntervalUpperValueIndexes.GetSize();
}

inline int KWQuantileIntervalBuilder::GetIntervalFirstInstanceIndexAt(int nIntervalIndex) const
{
	require(IsComputed());
	require(0 <= nIntervalIndex and nIntervalIndex < GetIntervalNumber());
	if (nIntervalIndex == 0)
		return 0;
	else
		return ivValuesCumulatedFrequencies.GetAt(ivIntervalUpperValueIndexes.GetAt(nIntervalIndex - 1));
}

inline int KWQuantileIntervalBuilder::GetIntervalLastInstanceIndexAt(int nIntervalIndex) const
{
	require(IsComputed());
	require(0 <= nIntervalIndex and nIntervalIndex < GetIntervalNumber());
	return ivValuesCumulatedFrequencies.GetAt(ivIntervalUpperValueIndexes.GetAt(nIntervalIndex)) - 1;
}

inline int KWQuantileIntervalBuilder::GetIntervalFirstValueIndexAt(int nIntervalIndex) const
{
	require(IsComputed());
	require(0 <= nIntervalIndex and nIntervalIndex < GetIntervalNumber());
	if (nIntervalIndex == 0)
		return 0;
	else
		return ivIntervalUpperValueIndexes.GetAt(nIntervalIndex - 1) + 1;
}

inline int KWQuantileIntervalBuilder::GetIntervalLastValueIndexAt(int nIntervalIndex) const
{
	require(IsComputed());
	require(0 <= nIntervalIndex and nIntervalIndex < GetIntervalNumber());
	return ivIntervalUpperValueIndexes.GetAt(nIntervalIndex);
}

inline int KWQuantileIntervalBuilder::GetIntervalFrequencyAt(int nIntervalIndex) const
{
	require(IsComputed());
	require(0 <= nIntervalIndex and nIntervalIndex < GetIntervalNumber());
	if (nIntervalIndex == 0)
		return ivValuesCumulatedFrequencies.GetAt(ivIntervalUpperValueIndexes.GetAt(nIntervalIndex));
	else
		return ivValuesCumulatedFrequencies.GetAt(ivIntervalUpperValueIndexes.GetAt(nIntervalIndex)) -
		       ivValuesCumulatedFrequencies.GetAt(ivIntervalUpperValueIndexes.GetAt(nIntervalIndex - 1));
}

inline int KWQuantileIntervalBuilder::GetIntervalQuantileIndexAt(int nIntervalIndex) const
{
	require(IsComputed());
	require(0 <= nIntervalIndex and nIntervalIndex < GetIntervalNumber());
	return ivIntervalQuantileIndexes.GetAt(nIntervalIndex);
}

///

inline int KWQuantileGroupBuilder::GetType() const
{
	return KWType::Symbol;
}

inline boolean KWQuantileGroupBuilder::IsValueInitialized() const
{
	assert(svValues.GetSize() == 0 or svValues.GetSize() == nValueNumber);
	return bIsValueInitialized;
}

inline int KWQuantileGroupBuilder::GetValueFrequencyAt(int nIndex) const
{
	require(IsFrequencyInitialized());
	require(0 <= nIndex and nIndex < GetValueNumber());
	if (nIndex == 0)
		return ivValuesCumulatedFrequencies.GetAt(nIndex);
	else
		return ivValuesCumulatedFrequencies.GetAt(nIndex) - ivValuesCumulatedFrequencies.GetAt(nIndex - 1);
}

inline int KWQuantileGroupBuilder::GetValueCumulatedFrequencAt(int nIndex) const
{
	require(IsFrequencyInitialized());
	require(0 <= nIndex and nIndex < GetValueNumber());
	return ivValuesCumulatedFrequencies.GetAt(nIndex);
}

inline Symbol& KWQuantileGroupBuilder::GetValueAt(int nIndex) const
{
	require(IsFrequencyInitialized());
	require(IsValueInitialized());
	require(0 <= nIndex and nIndex < GetValueNumber());
	return svValues.GetAt(nIndex);
}

inline int KWQuantileGroupBuilder::GetGroupNumber() const
{
	require(IsComputed());
	return nGroupNumber;
}

inline int KWQuantileGroupBuilder::GetGroupFirstValueIndexAt(int nGroupIndex) const
{
	require(IsComputed());
	require(0 <= nGroupIndex and nGroupIndex < GetGroupNumber());
	return nGroupIndex;
}

inline int KWQuantileGroupBuilder::GetGroupLastValueIndexAt(int nGroupIndex) const
{
	require(IsComputed());
	require(0 <= nGroupIndex and nGroupIndex < GetGroupNumber());
	if (nGroupIndex < GetGroupNumber() - 1)
		return nGroupIndex;
	else
		return GetValueNumber() - 1;
}

inline int KWQuantileGroupBuilder::GetGroupFrequencyAt(int nGroupIndex) const
{
	require(IsComputed());
	require(0 <= nGroupIndex and nGroupIndex < GetGroupNumber());
	if (GetGroupNumber() == 1)
		return GetInstanceNumber();
	else
	{
		if (nGroupIndex == 0)
			return ivValuesCumulatedFrequencies.GetAt(0);
		else if (nGroupIndex < GetGroupNumber() - 1)
			return ivValuesCumulatedFrequencies.GetAt(nGroupIndex) -
			       ivValuesCumulatedFrequencies.GetAt(nGroupIndex - 1);
		else
			return GetInstanceNumber() - ivValuesCumulatedFrequencies.GetAt(GetGroupNumber() - 2);
	}
}

inline int KWQuantileGroupBuilder::GetGroupValueNumberAt(int nGroupIndex) const
{
	require(IsComputed());
	require(0 <= nGroupIndex and nGroupIndex < GetGroupNumber());
	if (nGroupIndex < GetGroupNumber() - 1)
		return 1;
	else
		return GetValueNumber() - GetGroupNumber() + 1;
}

inline int KWQuantileGroupBuilder::GetGroupQuantileIndexAt(int nGroupIndex) const
{
	require(IsComputed());
	require(0 <= nGroupIndex and nGroupIndex < GetGroupNumber());
	if (nGroupIndex < GetGroupNumber() - 1)
		return nGroupIndex;
	else
		return GetQuantileNumber() - 1;
}
