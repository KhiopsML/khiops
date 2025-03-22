// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHEnumHistogramCost.h"

////////////////////////////////////////////////////////////////////////////
// Classe MHEnumHistogramCosts

MHEnumHistogramCosts::MHEnumHistogramCosts() {}

MHEnumHistogramCosts::~MHEnumHistogramCosts() {}

KWUnivariatePartitionCosts* MHEnumHistogramCosts::Create() const
{
	return new MHEnumHistogramCosts;
}

double MHEnumHistogramCosts::ComputePartitionCost(int nPartNumber) const
{
	boolean bDisplayResults = false;
	double dCost;

	require(nPartNumber >= 1);
	require(nTotalInstanceNumber > 0);

	// Cout choix du nombre d'intervalles
	dCost = KWStat::NaturalNumbersUniversalCodeLength(nPartNumber);
	if (bDisplayResults)
		cout << "\tCout choix nombre de parties " << nPartNumber << "\t"
		     << KWStat::NaturalNumbersUniversalCodeLength(nPartNumber) << endl;

	// Partition des longueurs des intervalles
	dCost += KWStat::LnFactorial(nTotalBinNumber + nPartNumber - 1);
	dCost -= KWStat::LnFactorial(nTotalBinNumber);
	dCost -= KWStat::LnFactorial(nPartNumber - 1);
	if (bDisplayResults)
		cout << "\tCout choix des longueurs des intervalles "
		     << KWStat::LnFactorial(nTotalBinNumber + nPartNumber - 1) - KWStat::LnFactorial(nTotalBinNumber) -
			    KWStat::LnFactorial(nPartNumber - 1)
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

double MHEnumHistogramCosts::ComputePartitionDeltaCost(int nPartNumber) const
{
	double dDeltaCost;

	require(nPartNumber > 1);

	dDeltaCost = ComputePartitionCost(nPartNumber - 1) - ComputePartitionCost(nPartNumber);
	return dDeltaCost;
}

double MHEnumHistogramCosts::ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const
{
	require(nGarbageModalityNumber == 0);
	return ComputePartitionDeltaCost(nPartNumber);
}

double MHEnumHistogramCosts::ComputePartCost(const KWFrequencyVector* part) const
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

double MHEnumHistogramCosts::ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const
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

void MHEnumHistogramCosts::WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const
{
	ost << "Part number\t" << nPartNumber << "\t" << ComputePartitionCost(nPartNumber) << "\n";
}

double MHEnumHistogramCosts::ComputePartitionConstructionCost(int nPartNumber) const
{
	return 0;
}

double MHEnumHistogramCosts::ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const
{
	double dCost;

	// Il faut retrancher le numerateur de la vraissemblanvce multinomiale pour avoir la partie prior du cout de
	// partition
	dCost = ComputePartitionCost(nPartNumber);
	dCost -= KWStat::LnFactorial(nTotalInstanceNumber);
	return dCost;
}

double MHEnumHistogramCosts::ComputePartModelCost(const KWFrequencyVector* part) const
{
	// Pas de partie prior pour le cout des partie
	return 0;
}

const ALString MHEnumHistogramCosts::GetClassLabel() const
{
	return "Enum histogram discretization costs";
}
