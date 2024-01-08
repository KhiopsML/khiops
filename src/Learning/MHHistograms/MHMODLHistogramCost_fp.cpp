// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHMODLHistogramCost_fp.h"

////////////////////////////////////////////////////////////////////////////
// Classe MHMODLHistogramCosts_fp

MHMODLHistogramCosts_fp::MHMODLHistogramCosts_fp()
{
	dHyperParametersCost = 0;
	nMinCentralBinExponent = 0;
	nMaxCentralBinExponent = 0;
	nCentralBinExponent = 0;
	nHierarchicalLevel = 0;
	dMinBinLength = 0;
	nPartileNumber = 0;

	// Redefinition du creator de vecteur d'effectif gere par la classe
	// (on detruit prealablement la version initialisee par la classe ancetre)
	delete kwfvFrequencyVectorCreator;
	kwfvFrequencyVectorCreator = new MHHistogramVector_fp;
}

MHMODLHistogramCosts_fp::~MHMODLHistogramCosts_fp() {}

void MHMODLHistogramCosts_fp::SetHyperParametersCost(double dValue)
{
	require(dHyperParametersCost >= 0);
	dHyperParametersCost = dValue;
}

double MHMODLHistogramCosts_fp::GetHyperParametersCost() const
{
	return dHyperParametersCost;
}

void MHMODLHistogramCosts_fp::SetMinCentralBinExponent(int nValue)
{
	nMinCentralBinExponent = nValue;
}

int MHMODLHistogramCosts_fp::GetMinCentralBinExponent() const
{
	return nMinCentralBinExponent;
}

void MHMODLHistogramCosts_fp::SetMaxCentralBinExponent(int nValue)
{
	nMaxCentralBinExponent = nValue;
}

int MHMODLHistogramCosts_fp::GetMaxCentralBinExponent() const
{
	return nMaxCentralBinExponent;
}

void MHMODLHistogramCosts_fp::SetCentralBinExponent(int nValue)
{
	require(nMinCentralBinExponent <= nValue and nValue <= nMaxCentralBinExponent);
	nCentralBinExponent = nValue;
}

int MHMODLHistogramCosts_fp::GetCentralBinExponent() const
{
	ensure(nMinCentralBinExponent <= nCentralBinExponent and nCentralBinExponent <= nMaxCentralBinExponent);
	return nCentralBinExponent;
}

void MHMODLHistogramCosts_fp::SetHierarchicalLevel(int nValue)
{
	require(nValue >= 0);
	nHierarchicalLevel = nValue;
}

int MHMODLHistogramCosts_fp::GetHierarchicalLevel() const
{
	return nHierarchicalLevel;
}

void MHMODLHistogramCosts_fp::SetMinBinLength(double dValue)
{
	require(dValue >= 0);
	dMinBinLength = dValue;
}

double MHMODLHistogramCosts_fp::GetMinBinLength() const
{
	return dMinBinLength;
}

void MHMODLHistogramCosts_fp::SetPartileNumber(int nValue)
{
	require(nValue >= 0);
	nPartileNumber = nValue;
}

int MHMODLHistogramCosts_fp::GetPartileNumber() const
{
	return nPartileNumber;
}

double MHMODLHistogramCosts_fp::ComputeFloatingPointBinCost(boolean bIsCentralBin, int nSign, int nExponent)
{
	require(nSign == -1 or nSign == 1);

	// Un bit pour le choix central vs exponent bit
	// Un bit pour le signe
	// Un bit pour le signe de l'exposant
	// Codage de la valeur absolue de l'exposant avec le prior universel de Rissanen pour les nombres entiers
	return 3 * log(2.0) + KWStat::NaturalNumbersUniversalCodeLength(abs(nExponent) + 1);
}

double MHMODLHistogramCosts_fp::ComputeCentralBinExponentCost(int nExponent)
{
	// Un bit pour le signe de l'exposant
	// Codage de la valeur absolue de l'exposant avec le prior universel de Rissanen pour les nombres entiers
	return log(2.0) + KWStat::NaturalNumbersUniversalCodeLength(abs(nExponent) + 1);
}

double MHMODLHistogramCosts_fp::ComputeDomainBoundsMantissaCost(int nMantissaBitNumber)
{
	require(nMantissaBitNumber >= 0);
	return KWStat::NaturalNumbersUniversalCodeLength(nMantissaBitNumber + 1) +
	       2 * ComputeMantissaCost(nMantissaBitNumber);
}

double MHMODLHistogramCosts_fp::ComputeMantissaCost(int nMantissaBitNumber)
{
	require(nMantissaBitNumber >= 0);
	return nMantissaBitNumber * log(2.0);
}

KWUnivariatePartitionCosts* MHMODLHistogramCosts_fp::Create() const
{
	return new MHMODLHistogramCosts_fp;
}

double MHMODLHistogramCosts_fp::ComputePartitionCost(int nPartNumber) const
{
	boolean bDisplayResults = false;
	double dCost = 0;

	require(nPartNumber >= 1);
	require(nTotalInstanceNumber > 0);
	require(dMinBinLength > 0);
	require(nPartileNumber > 0);
	require(nPartNumber <= nPartileNumber);

	// Hyper-parametres
	dCost += GetHyperParametersCost();
	if (bDisplayResults)
		cout << "\tCout des hyper-parametres\t" << GetHyperParametersCost() << endl;

	// Choix du niveau du central bin exponent par rapport au min central bin exponent
	dCost += KWStat::NaturalNumbersUniversalCodeLength(1 + GetMaxCentralBinExponent() - GetCentralBinExponent());
	if (bDisplayResults)
		cout << "\tCout du central bin exponent " << GetCentralBinExponent() << "\t"
		     << KWStat::NaturalNumbersUniversalCodeLength(1 + GetMaxCentralBinExponent() -
								  GetCentralBinExponent())
		     << endl;

	// Dans le cas d'une representation virgule fixe, on ignore les deux cout precedents
	if (EnforceFixedSizeBinsBehavior())
		dCost = 0;

	// Choix du niveau de hierarchie
	dCost += KWStat::NaturalNumbersUniversalCodeLength(1 + nHierarchicalLevel);
	if (bDisplayResults)
		cout << "\tCout choix niveau de hierarchie " << nHierarchicalLevel << "\t"
		     << KWStat::NaturalNumbersUniversalCodeLength(1 + nHierarchicalLevel) << endl;

	// Choix du nombre d'intervalles
	dCost += KWStat::NaturalNumbersUniversalCodeLength(nPartNumber);
	if (bDisplayResults)
		cout << "\tCout choix nombre de parties " << nPartNumber << "\t"
		     << KWStat::NaturalNumbersUniversalCodeLength(nPartNumber) << endl;

	// Partition des intervalles en bins
	dCost += KWStat::LnFactorial(GetPartileNumber() + nPartNumber - 1);
	dCost -= KWStat::LnFactorial(GetPartileNumber());
	dCost -= KWStat::LnFactorial(nPartNumber - 1);
	if (bDisplayResults)
		cout << "\tCout choix des longueurs des intervalles "
		     << KWStat::LnFactorial(GetPartileNumber() + nPartNumber - 1) -
			    KWStat::LnFactorial(GetPartileNumber()) - KWStat::LnFactorial(nPartNumber - 1)
		     << endl;

	// Partition des effectifs des intervalles
	dCost += KWStat::LnFactorial(nTotalInstanceNumber + nPartNumber - 1);
	dCost -= KWStat::LnFactorial(nTotalInstanceNumber);
	dCost -= KWStat::LnFactorial(nPartNumber - 1);
	if (bDisplayResults)
		cout << "\tCout choix des effectifs des intervalles "
		     << KWStat::LnFactorial(nTotalInstanceNumber + nPartNumber - 1) -
			    KWStat::LnFactorial(nTotalInstanceNumber) - KWStat::LnFactorial(nPartNumber - 1)
		     << endl;

	// Numerateur de la multinomiale de vraissemblance des effectifs
	dCost += KWStat::LnFactorial(nTotalInstanceNumber);
	if (bDisplayResults)
		cout << "\tCout du numerateur de la multinomiale des effectifs"
		     << " \t" << KWStat::LnFactorial(nTotalInstanceNumber) << endl;

	// Correction de la vraissemblance sur la longueur des intervalles
	dCost -= nTotalInstanceNumber * log(dMinBinLength);
	if (bDisplayResults)
		cout << "\tCout de correction de la vraissemblance sur la longueur des intervalles"
		     << " \t" << -nTotalInstanceNumber * log(dMinBinLength) << endl;

	// Cout complet
	if (bDisplayResults)
		cout << "Cout complet"
		     << " \t" << nPartNumber << "\t " << dCost << endl;
	return dCost;
}

double MHMODLHistogramCosts_fp::ComputePartitionDeltaCost(int nPartNumber) const
{
	double dDeltaCost;

	require(nPartNumber > 1);

	dDeltaCost = ComputePartitionCost(nPartNumber - 1) - ComputePartitionCost(nPartNumber);
	return dDeltaCost;
}

double MHMODLHistogramCosts_fp::ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const
{
	require(nGarbageModalityNumber == 0);
	return ComputePartitionDeltaCost(nPartNumber);
}

double MHMODLHistogramCosts_fp::ComputePartCost(const KWFrequencyVector* part) const
{
	boolean bDisplay = false;
	double dCost;
	int nIntervalFrequency;
	Continuous cLowerBound;
	Continuous cUpperBound;

	require(part != NULL);
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	// On extrait l'effectif et les bornes de l'intervalle
	nIntervalFrequency = cast(MHHistogramVector_fp*, part)->GetFrequency();
	cLowerBound = cast(MHHistogramVector_fp*, part)->GetLowerBound();
	cUpperBound = cast(MHHistogramVector_fp*, part)->GetUpperBound();

	// Partie de la vraisemblance multinomiale pour l'effectif
	dCost = -KWStat::LnFactorial(nIntervalFrequency);

	// Partie de la vraissemblance pour la longueur
	if (cUpperBound > cLowerBound)
		dCost += nIntervalFrequency * log(cUpperBound - cLowerBound);

	// Affichage des details du cout
	if (bDisplay)
	{
		cout << "\tPart(" << nIntervalFrequency << ", " << cUpperBound - cUpperBound << ")\t" << dCost << endl;
	}
	return dCost;
}

double MHMODLHistogramCosts_fp::ComputeReferenceNullCost(int nFrequency, int nExtremeValueMantissaBinBitNumber)
{
	double dReferenceNullCost;

	require(nFrequency >= 0);
	require(nExtremeValueMantissaBinBitNumber >= 0);

	// Prise en compte des hyper-parametres pour un ensemble ]1,2]
	// Un seil exponent bin tres simple, et pas de mantisse pour coder les hyper-parametres
	dReferenceNullCost = MHMODLHistogramCosts_fp::ComputeFloatingPointBinCost(false, 1, 1);
	dReferenceNullCost += MHMODLHistogramCosts_fp::ComputeFloatingPointBinCost(false, 1, 2);
	dReferenceNullCost += MHMODLHistogramCosts_fp::ComputeDomainBoundsMantissaCost(0);

	// Cout des choix du central bin exponent, de la granularite et du nombre d'intervalles
	dReferenceNullCost += 3 * KWStat::NaturalNumbersUniversalCodeLength(1);

	// Ajout du cout de codage des instances pour le modele null
	dReferenceNullCost += nExtremeValueMantissaBinBitNumber * log(2) * nFrequency;
	return dReferenceNullCost;
}

double MHMODLHistogramCosts_fp::ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const
{
	double dCost;
	int i;

	require(partTable->GetFrequencyVectorAt(0)->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());
	require(partTable != NULL);

	// Cout de partition plus somme des couts des parties
	dCost = ComputePartitionCost(partTable->GetFrequencyVectorNumber());
	for (i = 0; i < partTable->GetFrequencyVectorNumber(); i++)
		dCost += ComputePartCost(partTable->GetFrequencyVectorAt(i));
	return dCost;
}

void MHMODLHistogramCosts_fp::WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const
{
	ost << "Part number\t" << nPartNumber << "\t" << ComputePartitionCost(nPartNumber) << "\n";
}

double MHMODLHistogramCosts_fp::ComputePartitionConstructionCost(int nPartNumber) const
{
	return 0;
}

double MHMODLHistogramCosts_fp::ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const
{
	double dCost;

	// Il faut retrancher les partie vraissemblance pour avoir la partie prior du cout de partition
	dCost = ComputePartitionCost(nPartNumber);
	dCost -= KWStat::LnFactorial(nTotalInstanceNumber);
	dCost += nTotalInstanceNumber * log(dMinBinLength);
	return dCost;
}

double MHMODLHistogramCosts_fp::ComputePartModelCost(const KWFrequencyVector* part) const
{
	// Pas de partie prior pour le cout des partie
	return 0;
}

const ALString MHMODLHistogramCosts_fp::GetClassLabel() const
{
	return "G-Enum-fp histogram discretization costs";
}

boolean MHMODLHistogramCosts_fp::EnforceFixedSizeBinsBehavior()

{
	static boolean bIsInitialized = false;
	static boolean bEnforceFixedSizeBinsBehavior = false;

	// Determination du mode expert au premier appel
	if (not bIsInitialized)
	{
		ALString sEnforceFixedSizeBinsBehavior;

		// Recherche des variables d'environnement
		sEnforceFixedSizeBinsBehavior = p_getenv("EnforceFixedSizeBinsBehavior");
		sEnforceFixedSizeBinsBehavior.MakeLower();

		// Determination du mode expert
		if (sEnforceFixedSizeBinsBehavior == "true")
			bEnforceFixedSizeBinsBehavior = true;
		else if (sEnforceFixedSizeBinsBehavior == "false")
			bEnforceFixedSizeBinsBehavior = false;

		// Memorisation du flag d'initialisation
		bIsInitialized = true;
	}
	return bEnforceFixedSizeBinsBehavior;
}
