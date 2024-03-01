// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHGenumHistogramCosts.h"

////////////////////////////////////////////////////////////////////////////
// Classe MHGenumHistogramCosts

MHGenumHistogramCosts::MHGenumHistogramCosts()
{
	nTotalBinNumber = 0;
	nPartileNumber = 0;

	// Redefinition du creator de vecteur d'effectif gere par la classe
	// (on detruit prealablement la version initialisee par la classe ancetre)
	delete kwfvFrequencyVectorCreator;
	kwfvFrequencyVectorCreator = new MHGenumHistogramVector;
}

MHGenumHistogramCosts::~MHGenumHistogramCosts() {}

void MHGenumHistogramCosts::SetTotalBinNumber(int nValue)
{
	require(nValue >= 0);
	nTotalBinNumber = nValue;
}

int MHGenumHistogramCosts::GetTotalBinNumber() const
{
	return nTotalBinNumber;
}

void MHGenumHistogramCosts::SetPartileNumber(int nValue)
{
	require(nValue >= 0);
	nPartileNumber = nValue;
}

int MHGenumHistogramCosts::GetPartileNumber() const
{
	return nPartileNumber;
}

KWUnivariatePartitionCosts* MHGenumHistogramCosts::Create() const
{
	return new MHGenumHistogramCosts;
}

double MHGenumHistogramCosts::ComputePartitionCost(int nPartNumber) const
{
	boolean bDisplayResults = false;
	int nActualPartileNumber;
	double dCost;

	require(nPartNumber >= 1);
	require(nTotalInstanceNumber > 0);
	require(nPartileNumber <= nTotalBinNumber);
	require(nPartileNumber == 0 or nPartNumber <= nPartileNumber);

	// Cout choix du nombre d'intervalles
	dCost = KWStat::NaturalNumbersUniversalCodeLength(nPartNumber);
	if (bDisplayResults)
		cout << "\tCout choix nombre de parties " << nPartNumber << "\t"
		     << KWStat::NaturalNumbersUniversalCodeLength(nPartNumber) << endl;

	// Choix du nombre de partiles
	nActualPartileNumber = nTotalBinNumber;
	if (nPartileNumber > 0)
	{
		nActualPartileNumber = nPartileNumber;
		dCost += KWStat::NaturalNumbersUniversalCodeLength(nPartileNumber);
		if (bDisplayResults)
			cout << "\tCout choix nombre de partiles " << nPartileNumber << "\t"
			     << KWStat::NaturalNumbersUniversalCodeLength(nPartileNumber) << endl;
	}

	// Partition des longueurs des intervalles
	dCost += KWStat::LnFactorial(nActualPartileNumber + nPartNumber - 1);
	dCost -= KWStat::LnFactorial(nActualPartileNumber);
	dCost -= KWStat::LnFactorial(nPartNumber - 1);
	if (bDisplayResults)
		cout << "\tCout choix des longueurs des intervalles "
		     << KWStat::LnFactorial(nActualPartileNumber + nPartNumber - 1) -
			    KWStat::LnFactorial(nActualPartileNumber) - KWStat::LnFactorial(nPartNumber - 1)
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
	if (bDisplayResults)
		cout << "Cout complet"
		     << " \t" << nPartNumber << "\t " << dCost << endl;
	return dCost;
}

double MHGenumHistogramCosts::ComputePartitionDeltaCost(int nPartNumber) const
{
	double dDeltaCost;

	require(nPartNumber > 1);

	dDeltaCost = ComputePartitionCost(nPartNumber - 1) - ComputePartitionCost(nPartNumber);
	return dDeltaCost;
}

double MHGenumHistogramCosts::ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const
{
	require(nGarbageModalityNumber == 0);
	return ComputePartitionDeltaCost(nPartNumber);
}

double MHGenumHistogramCosts::ComputePartCost(const KWFrequencyVector* part) const
{
	boolean bDisplay = false;
	double dCost;
	int nIntervalFrequency;
	int nIntervalLength;

	require(part != NULL);
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	// On extrait l'effectif et la longueur de l'intervalle
	nIntervalFrequency = cast(MHGenumHistogramVector*, part)->GetFrequency();
	nIntervalLength = cast(MHGenumHistogramVector*, part)->GetLength();

	// Partie de la vraisemblance multinomiale pour l'effectif
	dCost = -KWStat::LnFactorial(nIntervalFrequency);

	// Partie de la vraissemblance pour la longueur
	if (nIntervalLength > 0)
		dCost += nIntervalFrequency * log(nIntervalLength);

	// Affichage des details du cout
	if (bDisplay)
	{
		cout << "\tPart(" << nIntervalFrequency << ", " << nIntervalLength << ")\t" << dCost << endl;
	}
	return dCost;
}

double MHGenumHistogramCosts::ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const
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

void MHGenumHistogramCosts::WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const
{
	ost << "Part number\t" << nPartNumber << "\t" << ComputePartitionCost(nPartNumber) << "\n";
}

double MHGenumHistogramCosts::ComputePartitionConstructionCost(int nPartNumber) const
{
	return 0;
}

double MHGenumHistogramCosts::ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const
{
	double dCost;

	// Il faut retrancher le numerateur de la vraissemblanvce multinomiale pour avoir la partie prior du cout de
	// partition
	dCost = ComputePartitionCost(nPartNumber);
	dCost -= KWStat::LnFactorial(nTotalInstanceNumber);
	return dCost;
}

double MHGenumHistogramCosts::ComputePartModelCost(const KWFrequencyVector* part) const
{
	// Pas de partie prior pour le cout des partie
	return 0;
}

const ALString MHGenumHistogramCosts::GetClassLabel() const
{
	return "G-Enum histogram discretization costs";
}
