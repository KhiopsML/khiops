// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHMODLHistogramCost.h"

////////////////////////////////////////////////////////////////////////////
// Classe MHMODLHistogramCosts

MHMODLHistogramCosts::MHMODLHistogramCosts()
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
	kwfvFrequencyVectorCreator = new MHMODLHistogramVector;
}

MHMODLHistogramCosts::~MHMODLHistogramCosts() {}

void MHMODLHistogramCosts::SetHyperParametersCost(double dValue)
{
	require(dHyperParametersCost >= 0);
	dHyperParametersCost = dValue;
}

double MHMODLHistogramCosts::GetHyperParametersCost() const
{
	return dHyperParametersCost;
}

void MHMODLHistogramCosts::SetMinCentralBinExponent(int nValue)
{
	nMinCentralBinExponent = nValue;
}

int MHMODLHistogramCosts::GetMinCentralBinExponent() const
{
	return nMinCentralBinExponent;
}

void MHMODLHistogramCosts::SetMaxCentralBinExponent(int nValue)
{
	nMaxCentralBinExponent = nValue;
}

int MHMODLHistogramCosts::GetMaxCentralBinExponent() const
{
	return nMaxCentralBinExponent;
}

void MHMODLHistogramCosts::SetCentralBinExponent(int nValue)
{
	require(nMinCentralBinExponent <= nValue and nValue <= nMaxCentralBinExponent);
	nCentralBinExponent = nValue;
}

int MHMODLHistogramCosts::GetCentralBinExponent() const
{
	ensure(nMinCentralBinExponent <= nCentralBinExponent and nCentralBinExponent <= nMaxCentralBinExponent);
	return nCentralBinExponent;
}

void MHMODLHistogramCosts::SetHierarchicalLevel(int nValue)
{
	require(nValue >= 0);
	nHierarchicalLevel = nValue;
}

int MHMODLHistogramCosts::GetHierarchicalLevel() const
{
	return nHierarchicalLevel;
}

void MHMODLHistogramCosts::SetMinBinLength(double dValue)
{
	require(dValue >= 0);
	dMinBinLength = dValue;
}

double MHMODLHistogramCosts::GetMinBinLength() const
{
	return dMinBinLength;
}

void MHMODLHistogramCosts::SetPartileNumber(int nValue)
{
	require(nValue >= 0);
	nPartileNumber = nValue;
}

int MHMODLHistogramCosts::GetPartileNumber() const
{
	return nPartileNumber;
}

double MHMODLHistogramCosts::ComputeFloatingPointBinCost(boolean bIsCentralBin, int nSign, int nExponent)
{
	require(nSign == -1 or nSign == 1);

	// Un bit pour le choix central vs exponent bit
	// Un bit pour le signe
	// Un bit pour le signe de l'exposant
	// Codage de la valeur absolue de l'exposant avec le prior universel de Rissanen pour les nombres entiers
	return 3 * log(2.0) + KWStat::NaturalNumbersUniversalCodeLength(abs(nExponent) + 1);
}

double MHMODLHistogramCosts::ComputeCentralBinExponentCost(int nExponent)
{
	// Un bit pour le signe de l'exposant
	// Codage de la valeur absolue de l'exposant avec le prior universel de Rissanen pour les nombres entiers
	return log(2.0) + KWStat::NaturalNumbersUniversalCodeLength(abs(nExponent) + 1);
}

double MHMODLHistogramCosts::ComputeDomainBoundsMantissaCost(int nMantissaBitNumber)
{
	require(nMantissaBitNumber >= 0);
	return KWStat::NaturalNumbersUniversalCodeLength(nMantissaBitNumber + 1) +
	       2 * ComputeMantissaCost(nMantissaBitNumber);
}

double MHMODLHistogramCosts::ComputeMantissaCost(int nMantissaBitNumber)
{
	require(nMantissaBitNumber >= 0);
	return nMantissaBitNumber * log(2.0);
}

KWUnivariatePartitionCosts* MHMODLHistogramCosts::Create() const
{
	return new MHMODLHistogramCosts;
}

double MHMODLHistogramCosts::ComputePartitionCost(int nPartNumber) const
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

double MHMODLHistogramCosts::ComputePartitionDeltaCost(int nPartNumber) const
{
	double dDeltaCost;

	require(nPartNumber > 1);

	dDeltaCost = ComputePartitionCost(nPartNumber - 1) - ComputePartitionCost(nPartNumber);
	return dDeltaCost;
}

double MHMODLHistogramCosts::ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const
{
	require(nGarbageModalityNumber == 0);
	return ComputePartitionDeltaCost(nPartNumber);
}

double MHMODLHistogramCosts::ComputePartCost(const KWFrequencyVector* part) const
{
	boolean bDisplay = false;
	double dCost;
	int nIntervalFrequency;
	Continuous cLowerBound;
	Continuous cUpperBound;

	require(part != NULL);
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	// On extrait l'effectif et les bornes de l'intervalle
	nIntervalFrequency = cast(MHMODLHistogramVector*, part)->GetFrequency();
	cLowerBound = cast(MHMODLHistogramVector*, part)->GetLowerBound();
	cUpperBound = cast(MHMODLHistogramVector*, part)->GetUpperBound();

	// Partie de la vraisemblance multinomiale pour l'effectif
	dCost = -KWStat::LnFactorial(nIntervalFrequency);

	// Partie de la vraissemblance pour la longueur
	if (cUpperBound > cLowerBound)
		dCost += nIntervalFrequency * log(cUpperBound - cLowerBound);

	// Affichage des details du cout
	if (bDisplay)
	{
		cout << "\tPart(" << nIntervalFrequency << ", " << cUpperBound - cLowerBound << ")\t" << dCost << endl;
	}
	return dCost;
}

double MHMODLHistogramCosts::ComputeReferenceNullCost(int nFrequency, int nExtremeValueMantissaBinBitNumber)
{
	double dReferenceNullCost;

	require(nFrequency >= 0);
	require(nExtremeValueMantissaBinBitNumber >= 0);

	// Prise en compte des hyper-parametres pour un ensemble ]1,2]
	// Un seul exponent bin tres simple, et pas de mantisse pour coder les hyper-parametres
	dReferenceNullCost = MHMODLHistogramCosts::ComputeFloatingPointBinCost(false, 1, 0);
	dReferenceNullCost += MHMODLHistogramCosts::ComputeFloatingPointBinCost(false, 1, 1);
	dReferenceNullCost += MHMODLHistogramCosts::ComputeDomainBoundsMantissaCost(0);

	// Cout des choix du central bin exponent, de la granularite et du nombre d'intervalles
	dReferenceNullCost += 3 * KWStat::NaturalNumbersUniversalCodeLength(1);

	// Ajout du cout de codage des instances pour le modele null
	dReferenceNullCost += nExtremeValueMantissaBinBitNumber * log(2) * nFrequency;
	return dReferenceNullCost;
}

double MHMODLHistogramCosts::ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const
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

void MHMODLHistogramCosts::WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const
{
	ost << "Part number\t" << nPartNumber << "\t" << ComputePartitionCost(nPartNumber) << "\n";
}

double MHMODLHistogramCosts::ComputePartitionConstructionCost(int nPartNumber) const
{
	return 0;
}

double MHMODLHistogramCosts::ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const
{
	double dCost;

	// Il faut retrancher les partie vraissemblance pour avoir la partie prior du cout de partition
	dCost = ComputePartitionCost(nPartNumber);
	dCost -= KWStat::LnFactorial(nTotalInstanceNumber);
	dCost += nTotalInstanceNumber * log(dMinBinLength);
	return dCost;
}

double MHMODLHistogramCosts::ComputePartModelCost(const KWFrequencyVector* part) const
{
	// Pas de partie prior pour le cout des partie
	return 0;
}

const ALString MHMODLHistogramCosts::GetClassLabel() const
{
	return "G-Enum-fp histogram discretization costs";
}