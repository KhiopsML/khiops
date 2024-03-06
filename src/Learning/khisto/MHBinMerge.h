// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "Object.h"
#include "KWContinuous.h"
#include "KWStat.h"
#include "MHFloatingPointFrequencyTableBuilder.h"
#include "MHBin.h"

///////////////////////////////////////////////////////////////////////////////
// Classe MHBinMerge
// Bin elementaire ([lower value, upper value], frequency)
// avec caracteristiques de la fusion avec son bin suivant
// Classe technique, utile pur l'optimisation de la fusion des bins
class MHBinMerge : public MHBin
{
public:
	// Constructeur
	MHBinMerge();
	~MHBinMerge();

	// Reinitialisation
	void Reset();

	////////////////////////////////////////////////////////////////////////////////
	// Gestion de la fusion du bin avec le bin suivant

	// Granularite de la fusion
	void SetMergeGranularity(int nValue);
	int GetMergeGranularity() const;

	// Perte en vraisemblance de la fusion, en ignorant les intervalles singuliers
	void SetMergeLikelihoodLoss(double dValue);
	double GetMergeLikelihoodLoss() const;

	// Position de la fusion dans la liste triee des fusion
	void SetMergePosition(POSITION position);
	POSITION GetMergePosition() const;

	// Mise a jour des criteres de fusion avec un bin suivant
	// On indique si on gere des bins de type floating-point ou equal-width
	// On indique egalement si on est dans le cas d'un resume sature, auquel cas on
	// ignore le critere de vraisemblance
	void UpdateMergeCriteria(const MHBinMerge* otherBin, boolean bFloatingPointGrid, boolean bSaturatedBins);

	// Verification des criteres de fusion avec un bin suivant
	boolean CheckMergeCriteria(const MHBinMerge* otherBin, boolean bFloatingPointGrid,
				   boolean bSaturatedBins) const;

	////////////////////////////////////////////////////////////////////////////////
	// Comparaisons et autre services

	// Comparaison avec un autre bin, uniquement sur l'ordre
	int CompareBin(const MHBinMerge* otherBin) const;

	// Test si le bin est inclus dans un autre bin
	boolean IsIncludedIn(const MHBinMerge* otherBin) const;

	// Comparaison avec un autre bin, sur l'ordre, avec egalite si un bin peut etre inclu dans un autre
	int ComparePotentiallyIncludedBin(const MHBinMerge* otherBin) const;

	// Comparaison avec une autre fusion de bin
	int CompareBinMerge(const MHBinMerge* otherBinMerge) const;

	// Recherche de la plus petite granularite d'un grille pouvant contenir deux valeurs dans un seul bin
	int FindSmallestGridGranularity(double dLowerValue, double dUpperValue, boolean bFloatingPointGrid) const;

	////////////////////////////////////////////////////////////////////////////////
	// Divers

	// Ecriture des criteres de merges
	void WriteMergeHeaderLine(ostream& ost) const;
	void WriteMerge(ostream& ost) const;

	// Methode de test
	static void Test();

	////////////////////////////////////////////////////////
	//// Implementation
protected:
	// Calcul de la granularite d'une fusion, c'est a dire de la plus petite grille permettant de contenir deux bins
	// successifs
	int ComputeMergeGranularity(const MHBinMerge* otherBin, boolean bFloatingPointGrid) const;

	// Variantes de la methode de recherche de la de la granularite
	int FindSmallestEqualWidthGridGranularity(double dLowerValue, double dUpperValue) const;
	int FindSmallestFloatingPointGridGranularity(double dLowerValue, double dUpperValue) const;

	// Calcul de la perte en vraisemnblance suite a fusion de bins deux bins successifs
	double ComputeMergeLikelihoodLoss(const MHBinMerge* otherBin) const;

	// Criteres d'evaluation de la fusion avec un bin suivant
	int nMergeGranularity;
	double dMergeLikelihoodLoss;
	POSITION mergePosition;
};

// Fonctionde compaisons
int MHBinMergeCompareBin(const void* elem1, const void* elem2);
int MHBinMergeComparePotentiallyIncludedBin(const void* elem1, const void* elem2);
int MHBinMergeCompareBinMerge(const void* elem1, const void* elem2);

// Methode en inline

inline MHBinMerge::MHBinMerge()
{
	Reset();
}

inline MHBinMerge::~MHBinMerge() {}

inline void MHBinMerge::Reset()
{
	cLowerValue = 0;
	cUpperValue = 0;
	nFrequency = 0;
	nMergeGranularity = 0;
	dMergeLikelihoodLoss = 0;
	mergePosition = NULL;
}

inline void MHBinMerge::SetMergeGranularity(int nValue)
{
	nMergeGranularity = nValue;
}

inline int MHBinMerge::GetMergeGranularity() const
{
	return nMergeGranularity;
}

inline void MHBinMerge::SetMergeLikelihoodLoss(double dValue)
{
	dMergeLikelihoodLoss = dValue;
}

inline double MHBinMerge::GetMergeLikelihoodLoss() const
{
	return dMergeLikelihoodLoss;
}

inline void MHBinMerge::SetMergePosition(POSITION position)
{
	mergePosition = position;
}

inline POSITION MHBinMerge::GetMergePosition() const
{
	return mergePosition;
}

inline int MHBinMerge::CompareBin(const MHBinMerge* otherBin) const
{
	require(Check());
	require(otherBin->Check());
	return KWContinuous::Compare(GetUpperValue(), otherBin->GetUpperValue());
}

inline boolean MHBinMerge::IsIncludedIn(const MHBinMerge* otherBin) const
{
	require(Check());
	require(otherBin->Check());
	return GetLowerValue() >= otherBin->GetLowerValue() and GetUpperValue() <= otherBin->GetUpperValue();
}

inline int MHBinMerge::ComparePotentiallyIncludedBin(const MHBinMerge* otherBin) const
{
	require(Check());
	require(otherBin->Check());
	if (IsIncludedIn(otherBin) or otherBin->IsIncludedIn(this))
		return 0;
	else
	{
		assert(GetUpperValue() < otherBin->GetLowerValue() or GetLowerValue() > otherBin->GetUpperValue());
		if (GetUpperValue() < otherBin->GetLowerValue())
			return -1;
		else
			return 1;
	}
}

inline int MHBinMerge::CompareBinMerge(const MHBinMerge* otherBinMerge) const
{
	int nCompare;

	require(Check());
	require(otherBinMerge->Check());

	nCompare = GetMergeGranularity() - otherBinMerge->GetMergeGranularity();
	if (nCompare == 0)
		nCompare = KWContinuous::Compare(GetMergeLikelihoodLoss(), otherBinMerge->GetMergeLikelihoodLoss());
	return nCompare;
}
