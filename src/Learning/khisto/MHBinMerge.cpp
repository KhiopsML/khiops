// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHBinMerge.h"

void MHBinMerge::UpdateMergeCriteria(const MHBinMerge* otherBin, boolean bFloatingPointGrid, boolean bSaturatedBins)
{
	SetMergeGranularity(ComputeMergeGranularity(otherBin, bFloatingPointGrid));
	if (not bSaturatedBins)
		SetMergeLikelihoodLoss(ComputeMergeLikelihoodLoss(otherBin));
}

boolean MHBinMerge::CheckMergeCriteria(const MHBinMerge* otherBin, boolean bFloatingPointGrid,
				       boolean bSaturatedBins) const
{
	boolean bOk = true;

	bOk = bOk and GetMergeGranularity() == ComputeMergeGranularity(otherBin, bFloatingPointGrid);
	if (not bSaturatedBins)
		bOk = bOk and GetMergeLikelihoodLoss() == ComputeMergeLikelihoodLoss(otherBin);
	return bOk;
}

int MHBinMerge::FindSmallestGridGranularity(double dLowerValue, double dUpperValue, boolean bFloatingPointGrid) const
{
	if (bFloatingPointGrid)
		return FindSmallestFloatingPointGridGranularity(dLowerValue, dUpperValue);
	else
		return FindSmallestEqualWidthGridGranularity(dLowerValue, dUpperValue);
}

void MHBinMerge::WriteMergeHeaderLine(ostream& ost) const
{
	ost << "Granularity\tLikelihooLoss";
}

void MHBinMerge::WriteMerge(ostream& ost) const
{
	ost << GetMergeGranularity() << '\t';
	ost << KWContinuous::ContinuousToString(GetMergeLikelihoodLoss());
}

void MHBinMerge::Test()
{
	MHBinMerge binMerge;
	double cLower;
	double cUpper;
	int nGranularity;

	// Test pour de la recherche de la granularite pour des valeurs pareticulieres
	cLower = 4.87517301;
	cUpper = 4.87755145;
	nGranularity = binMerge.FindSmallestGridGranularity(-cUpper, -cLower, true);
	nGranularity = binMerge.FindSmallestGridGranularity(cLower, cUpper, true);
	cout << cLower << "\t" << cUpper << "\t" << nGranularity << endl;
}

int MHBinMerge::ComputeMergeGranularity(const MHBinMerge* otherBin, boolean bFloatingPointGrid) const
{
	require(Check());
	require(otherBin != NULL);
	require(otherBin->Check());
	require(GetUpperValue() < otherBin->GetLowerValue());

	return FindSmallestGridGranularity(GetLowerValue(), otherBin->GetUpperValue(), bFloatingPointGrid);
}

int MHBinMerge::FindSmallestEqualWidthGridGranularity(double dLowerValue, double dUpperValue) const
{
	int nGranularity;
	double dEpsilon;
	double dLowerBound;
	int nK;

	require(dLowerValue < dUpperValue);

	// Cas particulier de valeurs de part et d'autre de 0: aucune fusion n'est possible
	if (dLowerValue <= 0 and dUpperValue > 0)
		return 1000000;

	// On a k 2^g < lower_value < upper_value <= (k + 1)2^g
	// donc 2^g > upper_value - lower_value
	// On a aussi (k + 1 / 2) 2^g < upper_value,
	// Sinon, 2k 2^(g - 1) < lower_value < upper_value <= (2k + 1)2^(g - 1)
	// Donc, 2^(g - 1) < upper_value, et g < log2(upper_value) + 1

	// Ici, on part de la born inf
	nGranularity = (int)ceil(log(dUpperValue - dLowerValue) / log(2)) - 1;

	// Epsilon
	dEpsilon = pow(2, nGranularity);

	// On doit avoir [lower_value; upper_value] dans] lower_bound, upper_bound], avec upper_bound = lower_bound +
	// 2^g
	nK = (int)floor(dLowerValue / dEpsilon);
	dLowerBound = nK * dEpsilon;
	if (dLowerBound == dLowerValue)
		dLowerBound -= dEpsilon;

	// Recherche de la plus petit granularite compatible
	while (dUpperValue > dLowerBound + dEpsilon)
	{
		nGranularity += 1;
		dEpsilon *= 2;

		// Mise a jour de la borne inf
		nK = (int)floor(dLowerValue / dEpsilon);
		dLowerBound = nK * dEpsilon;
		if (dLowerBound == dLowerValue)
			dLowerBound -= dEpsilon;
	}

	// Verifications
	ensure(dLowerBound < dLowerValue);
	ensure(dUpperValue <= dLowerBound + dEpsilon);
	ensure(dEpsilon == pow(2, nGranularity));
	return nGranularity;
}

int MHBinMerge::FindSmallestFloatingPointGridGranularity(double dLowerValue, double dUpperValue) const
{
	const boolean bHierarchicalExponents = true;
	int nGranularity;
	Continuous cLowerValueMantissa;
	int nLowerValueExponent;
	Continuous cUpperValueMantissa;
	int nUpperValueExponent;
	int nMinExponent;
	int nMaxExponent;

	require(dLowerValue < dUpperValue);

	// Cas particulier de valeurs de part et d'autre de 0: aucune fusion n'est possible
	if (dLowerValue <= 0 and dUpperValue > 0)
		return 1000000;

	// Extraction de la representaion vigule flotante des deux valeurs
	cLowerValueMantissa = MHFloatingPointFrequencyTableBuilder::ExtractMantissa(dLowerValue, nLowerValueExponent);
	cUpperValueMantissa = MHFloatingPointFrequencyTableBuilder::ExtractMantissa(dUpperValue, nUpperValueExponent);

	// Recherche d'un bin d'exposant si les exposants sont differents
	if (nLowerValueExponent != nUpperValueExponent)
	{
		// Traitement des exposant de facon hierarchique: approche privilegiees
		// On traite l'exposant selon une grilles de type EqualWidth
		// Cela signifie que les deux exposants doivent etre des multiples de 2^g
		// Cela permet de mieux etaler les bins de type exposant quand le nombre max de bins est tres petit
		if (bHierarchicalExponents)
		{
			// Rappel: les exponent bins sont de type: ]-2^(e+1), -2^e] ou ]2^e, 2^(e+1)]
			// On souhaite centrer la hierarchie autour de -1 = -2^0 et 1=2^0
			// de qui necessite de decaller les exposants
			nMinExponent = 1 + min(nLowerValueExponent, nUpperValueExponent);
			nMaxExponent = 1 + max(nLowerValueExponent, nUpperValueExponent);

			// Cas ou les exposant etaient de signe differents
			if (nMinExponent <= 0 and nMaxExponent > 0)
				// Aucune fusion n'est possible, de facon moins prioritraire au cas de deux valeurs de
				// signes differents
				nGranularity = 100000;
			// Cas ou les exposants sont de meme signe
			else
			{
				nGranularity = FindSmallestEqualWidthGridGranularity(nMinExponent, nMaxExponent);

				// Ajout d'un increment pour avoir une hierarchie entre exposant d'abord, et mantisse
				// ensuite
				nGranularity += 10000;
			}
		}
		// Traitement de facon standard
		// Ici, seul l'exposant le plus grand est pris en compte, sans gestion d'une hierarchie dans les
		// exposants Cela a tendancce a perdre de l'information en priorite dans les bins de petits exposants La
		// difference avec l'approche hierarchique ne se fait sentir que quand le nombre max de bins est tres
		// petit Des que a un nombre de bins suffisant pour arriver au niveaudes mantissa bins, les deux
		// approches sont identiques
		else
		{
			nGranularity = max(nLowerValueExponent, nUpperValueExponent);

			// Ajout d'un increment pour avoir une hierarchie entre exposant d'abord, et mantisse ensuite
			nGranularity += 10000;
		}
	}
	// Sinon, on traite les mantissa bins, de facon indentique au cas des bin de type EqualWidth
	else
	{
		assert(cLowerValueMantissa < cUpperValueMantissa);
		nGranularity = FindSmallestEqualWidthGridGranularity(cLowerValueMantissa, cUpperValueMantissa);
		assert(nGranularity <= 0);
	}
	return nGranularity;
}

double MHBinMerge::ComputeMergeLikelihoodLoss(const MHBinMerge* otherBin) const
{
	double dLikelihoodLoss;

	require(Check());
	require(otherBin != NULL);
	require(otherBin->Check());
	require(GetUpperValue() < otherBin->GetLowerValue());

	// Initialisation avec les terme multinomiaux
	dLikelihoodLoss = KWStat::LnFactorial(GetFrequency());
	dLikelihoodLoss += KWStat::LnFactorial(otherBin->GetFrequency());
	dLikelihoodLoss -= KWStat::LnFactorial(GetFrequency() + otherBin->GetFrequency());

	// Calcul de la partie d'indexation des epsilon bin, en tenant compte des intervalles singuliers
	// On considere qu'un intervalle singulier ne coutait rien a coder pour la partie bin index
	dLikelihoodLoss +=
	    (GetFrequency() + otherBin->GetFrequency()) * log(otherBin->GetUpperValue() - GetLowerValue());
	if (not IsSingular())
		dLikelihoodLoss -= GetFrequency() * log(GetUpperValue() - GetLowerValue());
	if (not otherBin->IsSingular())
		dLikelihoodLoss -=
		    otherBin->GetFrequency() * log(otherBin->GetUpperValue() - otherBin->GetLowerValue());
	return dLikelihoodLoss;
}

int MHBinMergeCompareBin(const void* elem1, const void* elem2)
{
	MHBinMerge* bin1;
	MHBinMerge* bin2;

	bin1 = cast(MHBinMerge*, *(Object**)elem1);
	bin2 = cast(MHBinMerge*, *(Object**)elem2);
	return bin1->CompareBin(bin2);
}

int MHBinMergeComparePotentiallyIncludedBin(const void* elem1, const void* elem2)
{
	MHBinMerge* bin1;
	MHBinMerge* bin2;

	bin1 = cast(MHBinMerge*, *(Object**)elem1);
	bin2 = cast(MHBinMerge*, *(Object**)elem2);
	return bin1->ComparePotentiallyIncludedBin(bin2);
}

int MHBinMergeCompareBinMerge(const void* elem1, const void* elem2)
{
	MHBinMerge* bin1;
	MHBinMerge* bin2;

	bin1 = cast(MHBinMerge*, *(Object**)elem1);
	bin2 = cast(MHBinMerge*, *(Object**)elem2);
	return bin1->CompareBinMerge(bin2);
}
