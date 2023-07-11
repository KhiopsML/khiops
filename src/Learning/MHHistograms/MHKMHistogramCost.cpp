// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHKMHistogramCost.h"

////////////////////////////////////////////////////////////////////////////
// Classe MHKMHistogramCosts

MHKMHistogramCosts::MHKMHistogramCosts() {}

MHKMHistogramCosts::~MHKMHistogramCosts() {}

KWUnivariatePartitionCosts* MHKMHistogramCosts::Create() const
{
	return new MHKMHistogramCosts;
}

double MHKMHistogramCosts::ComputePartitionCost(int nPartNumber) const
{
	boolean bDisplayResults = false;
	double dCost;

	require(nPartNumber >= 1);
	require(nTotalInstanceNumber > 0);

	// Partition des longueurs des intervalles
	dCost = KWStat::LnFactorial(nTotalBinNumber);
	dCost -= KWStat::LnFactorial(nTotalBinNumber - nPartNumber + 1);
	dCost -= KWStat::LnFactorial(nPartNumber - 1);
	if (bDisplayResults)
		cout << "\tCout choix des longueurs des intervalles "
		     << KWStat::LnFactorial(nTotalBinNumber) - KWStat::LnFactorial(nTotalBinNumber - nPartNumber + 1) -
			    KWStat::LnFactorial(nPartNumber - 1)
		     << endl;

	// Partition des effectifs des intervalles selon le critere multinomial NML
	dCost += MHNMLStat::MultinomialCOMP(nPartNumber, nTotalInstanceNumber);
	if (bDisplayResults)
		cout << "\tCout choix des effectifs des intervalles "
		     << MHNMLStat::MultinomialCOMP(nPartNumber, nTotalInstanceNumber) << endl;

	// Numerateur de la vraissemblance NML des effectifs
	dCost += nTotalInstanceNumber * log(nTotalInstanceNumber);
	if (bDisplayResults)
		cout << "\tCout du numerateur de la vraissemblance NML des effectifs"
		     << " \t" << nTotalInstanceNumber * log(nTotalInstanceNumber) << endl;
	if (bDisplayResults)
		cout << "Cout complet"
		     << " \t" << nPartNumber << "\t " << dCost << endl;
	return dCost;
}

double MHKMHistogramCosts::ComputePartitionDeltaCost(int nPartNumber) const
{
	double dDeltaCost;

	require(nPartNumber > 1);

	dDeltaCost = ComputePartitionCost(nPartNumber - 1) - ComputePartitionCost(nPartNumber);
	return dDeltaCost;
}

double MHKMHistogramCosts::ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const
{
	require(nGarbageModalityNumber == 0);
	return ComputePartitionDeltaCost(nPartNumber);
}

double MHKMHistogramCosts::ComputePartCost(const KWFrequencyVector* part) const
{
	boolean bDisplay = false;
	double dCost;
	int nIntervalFrequency;
	int nIntervalLength;

	require(part != NULL);
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	// On extrait l'effectif et la longueur de l'intervalle
	nIntervalFrequency = cast(MHHistogramVector*, part)->GetFrequency();
	nIntervalLength = cast(MHHistogramVector*, part)->GetLength();

	// Partie de la vraisemblance NML pour l'effectif
	dCost = 0;
	if (nIntervalFrequency > 0)
		dCost = -nIntervalFrequency * log(nIntervalFrequency);

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

double MHKMHistogramCosts::ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const
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

void MHKMHistogramCosts::WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const
{
	ost << "Part number\t" << nPartNumber << "\t" << ComputePartitionCost(nPartNumber) << "\n";
}

double MHKMHistogramCosts::ComputePartitionConstructionCost(int nPartNumber) const
{
	return 0;
}

double MHKMHistogramCosts::ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const
{
	double dCost;

	// Il faut retrancher le numerateur de la vraissemblanvce NML pour avoir la partie prior du cout de partition
	dCost = ComputePartitionCost(nPartNumber);
	dCost -= nTotalInstanceNumber * log(nTotalInstanceNumber);
	return dCost;
}

double MHKMHistogramCosts::ComputePartModelCost(const KWFrequencyVector* part) const
{
	// Pas de partie prior pour le cout des partie
	return 0;
}

const ALString MHKMHistogramCosts::GetClassLabel() const
{
	return "KM histogram discretization costs";
}
