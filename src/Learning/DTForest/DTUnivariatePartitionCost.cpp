// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTUnivariatePartitionCost.h"

////////////////////////////////////////////////////////////////////////////
// Classe DTMODLDiscretizationCosts

DTMODLDiscretizationCosts::DTMODLDiscretizationCosts() {}

DTMODLDiscretizationCosts::~DTMODLDiscretizationCosts() {}

KWUnivariatePartitionCosts* DTMODLDiscretizationCosts::Create() const
{
	return new DTMODLDiscretizationCosts;
}

double DTMODLDiscretizationCosts::ComputePartitionCost(int nPartNumber) const
{
	// Cas d'utilisation de la granularite
	double dCost;
	boolean bDisplayResults = false;
	int nGranularityMax;

	require(nClassValueNumber > 1);
	require(nPartNumber >= 1);
	require(nPartNumber <= nValueNumber);
	require(nGranularity >= 1 or nPartNumber == 1);
	require(nTotalInstanceNumber > 0);

	nGranularityMax = (int)ceil(log(GetTotalInstanceNumber() * 1.0) / log(2.0));

	// Cout choix entre modele nul et modele informatif
	dCost = log(2.0);
	if (bDisplayResults)
		cout << "Choix modele informatif " << log(2.0) << endl;

	// Si modele informatif
	if (nPartNumber > 1 and nValueNumber > 1)
	{
		// Cout de selection/construction de l'attribut
		dCost += dAttributeCost;
		if (bDisplayResults)
			cout << " Cout de selection/construction de l'attribut " << dAttributeCost << endl;

		// Cout du choix de la granularite
		dCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nGranularity, nGranularityMax);

		if (bDisplayResults)
			cout << "Cout choix granularite " << nGranularity << " = "
			     << KWStat::BoundedNaturalNumbersUniversalCodeLength(nGranularity, nGranularityMax) << endl;

		// Nombre d'intervalles
		// dCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartNumber - 1, nValueNumber - 1);
		if (bDisplayResults)
			cout << "Cout choix nombre de parties " << nPartNumber << " parmi " << nValueNumber << "\t"
			     << KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartNumber - 1, nValueNumber - 1)
			     << endl;

		// Partition en intervalles
		// Nouveau codage avec description du choix des coupures selon une multinomiale
		dCost += log((nValueNumber - 1) * 1.0);

		if (bDisplayResults)
			cout << "Cout choix intervalles "
			     << KWStat::LnFactorial(nValueNumber + nPartNumber - 1) -
				    KWStat::LnFactorial(nValueNumber) - KWStat::LnFactorial(nPartNumber - 1)
			     << endl;
	}
	if (bDisplayResults)
		cout << "Cout complet avec granularite " << nGranularity << " = "
		     << " \tnPartNumber = " << nPartNumber << "\t " << dCost << endl;

	return dCost;
}

double DTMODLDiscretizationCosts::ComputePartitionDeltaCost(int nPartNumber) const
{
	double dDeltaCost;

	// Cas d'utilisation de la granularite
	require(nValueNumber > 0);
	require(nPartNumber > 1);
	require(nPartNumber <= nValueNumber);
	require(nClassValueNumber > 1);
	require(nGranularity >= 1);

	// Cas d'une partition en au moins deux intervalles
	if (nPartNumber > 2)
	{
		dDeltaCost = KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartNumber - 2, nValueNumber - 1) -
			     KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartNumber - 1, nValueNumber - 1);

		// Nouveau codage avec description du choix des coupures selon une multinomiale
		dDeltaCost = dDeltaCost + log(nPartNumber - 1.0) - log(nValueNumber - 1.0);
	}
	// Sinon, on compare le cout de la partition en un intervalle au cout du modele nul (1 intervalle)
	else
		dDeltaCost = ComputePartitionCost(nPartNumber - 1) - ComputePartitionCost(nPartNumber);

	// NV ARBREV9 ensure(fabs(ComputePartitionCost(nPartNumber - 1) - ComputePartitionCost(nPartNumber) -
	// dDeltaCost) < dEpsilon);
	return dDeltaCost;
}

double DTMODLDiscretizationCosts::ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const
{
	require(nGarbageModalityNumber == 0);
	return ComputePartitionDeltaCost(nPartNumber);
}

double DTMODLDiscretizationCosts::ComputePartCost(const KWFrequencyVector* part) const
{
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	IntVector* ivFrequencyVector;
	double dCost;
	int nFrequency;
	int nIntervalFrequency;
	int i;

	require(part != NULL);
	require(nClassValueNumber > 1);

	// Acces aux compteurs du vecteur d'effectif dense
	ivFrequencyVector = cast(KWDenseFrequencyVector*, part)->GetFrequencyVector();

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne
	dCost = 0;
	nIntervalFrequency = 0;
	for (i = 0; i < ivFrequencyVector->GetSize(); i++)
	{
		nFrequency = ivFrequencyVector->GetAt(i);
		dCost -= KWStat::LnFactorial(nFrequency);
		nIntervalFrequency += nFrequency;
	}
	dCost += KWStat::LnFactorial(nIntervalFrequency + nClassValueNumber - 1);
	dCost -= KWStat::LnFactorial(nClassValueNumber - 1);
	return dCost;
}

double DTMODLDiscretizationCosts::ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const
{
	require(partTable->GetFrequencyVectorAt(0)->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	double dCost;
	int i;

	require(partTable != NULL);
	require(nGranularity == partTable->GetGranularity());

	// Cout de partition plus somme des couts des parties
	dCost = ComputePartitionCost(partTable->GetFrequencyVectorNumber());

	for (i = 0; i < partTable->GetFrequencyVectorNumber(); i++)
		dCost += ComputePartCost(partTable->GetFrequencyVectorAt(i));

	return dCost;
}

void DTMODLDiscretizationCosts::WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const
{
	ost << "Part number\t" << nPartNumber << "\t" << ComputePartitionCost(nPartNumber) << "\n";
}

double DTMODLDiscretizationCosts::ComputePartitionConstructionCost(int nPartNumber) const
{
	if (nPartNumber > 1)
		return log(2.0) + dAttributeCost;
	else
		return log(2.0);
}

double DTMODLDiscretizationCosts::ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const
{
	require(nGarbageModalityNumber == 0);
	return ComputePartitionCost(nPartNumber);
}

double DTMODLDiscretizationCosts::ComputePartModelCost(const KWFrequencyVector* part) const
{
	IntVector* ivFrequencyVector;
	double dCost;
	int nFrequency;
	int nIntervalFrequency;
	int i;

	require(part != NULL);
	require(nClassValueNumber > 1);

	// Acces aux compteurs du vecteur d'effectif dense
	ivFrequencyVector = cast(KWDenseFrequencyVector*, part)->GetFrequencyVector();

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne
	dCost = 0;
	nIntervalFrequency = 0;
	for (i = 0; i < ivFrequencyVector->GetSize(); i++)
	{
		nFrequency = ivFrequencyVector->GetAt(i);
		nIntervalFrequency += nFrequency;
	}
	dCost += KWStat::LnFactorial(nIntervalFrequency + nClassValueNumber - 1);
	dCost -= KWStat::LnFactorial(nClassValueNumber - 1);
	dCost -= KWStat::LnFactorial(nIntervalFrequency);
	return dCost;
}

const ALString DTMODLDiscretizationCosts::GetClassLabel() const
{
	return "MODL discretization costs";
}

////////////////////////////////////////////////////////////////////////////
// Classe DTMODLGroupingCosts

DTMODLGroupingCosts::DTMODLGroupingCosts() {}

DTMODLGroupingCosts::~DTMODLGroupingCosts() {}

KWUnivariatePartitionCosts* DTMODLGroupingCosts::Create() const
{
	return new DTMODLGroupingCosts;
}

double DTMODLGroupingCosts::ComputePartitionCost(int nPartNumber) const
{
	require(GetValueNumber() < KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage());
	return ComputePartitionCost(nPartNumber, 0);
}

double DTMODLGroupingCosts::ComputePartitionCost(int nPartNumber, int nGarbageModalityNumber) const
{
	double dCost;
	int nInformativeValueNumber;
	int nInformativePartNumber;
	int nGranularityMax;

	require(nGranularity >= 0);
	require(nTotalInstanceNumber > 1 or nGranularity == 0);

	// Initialisations
	// Granularite maximale
	nGranularityMax = (int)ceil(log(GetTotalInstanceNumber() * 1.0) / log(2.0));
	// Nombre de valeurs informatives (hors groupe poubelle)
	nInformativeValueNumber = nValueNumber - nGarbageModalityNumber;

	// Nombre de parties informatives
	// Initialisation avec poubelle
	if (nGarbageModalityNumber > 0)
	{
		// Nombre total de parties - 1 pour le groupe poubelle
		nInformativePartNumber = nPartNumber - 1;

		// Le modele a 1 groupe + 1 groupe poubelle ne peut pas etre envisage
		assert(nInformativePartNumber > 1);
	}

	// Initialisation sans poubelle
	else
		nInformativePartNumber = nPartNumber;

	require(nInformativePartNumber <= nInformativeValueNumber);
	require(nInformativePartNumber >= 1);

	// Choix du modele nul ou modele informatif
	dCost = log(2.0);

	// Si modele informatif
	if (nInformativePartNumber > 1 and nInformativeValueNumber > 1)
	{
		// Cout de selection/construction de l'attribut
		dCost += dAttributeCost;

		// Choix de la granularite si mode granu
		if (nGranularity > 0)
			dCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nGranularity, nGranularityMax);

		// Si mode poubelle et si nombre total de modalites suffisant, cout de la hierarchie poubelle
		// if (nValueNumber > KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage())
		//	dCost += log(2.0);

		// Cas de l'absence de poubelle
		if (nGarbageModalityNumber == 0)
		{
			// Cout de codage du nombre de groupes
			dCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nInformativePartNumber - 1,
										  nInformativeValueNumber - 1);

			// Cout de codage du choix des groupes
			dCost += KWStat::LnBell(nInformativeValueNumber, nInformativePartNumber);
		}
		// Cas de la presence d'une poubelle
		else
		{
			// Cout du choix du nombre de modalites informatives hors poubelle parmi l'ensemble des
			// modalites
			dCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nInformativeValueNumber - 1,
										  nValueNumber - 2);

			// Cout du choix des modalites informatives parmi l'ensemble des modalites (tirage multinimial
			// avec elements distincts)
			dCost += nInformativeValueNumber * log(nValueNumber * 1.0) -
				 KWStat::LnFactorial(nInformativeValueNumber);

			// Cout de codage du nombre de groupes parmi les modalites informatives
			dCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nInformativePartNumber - 1,
										  nInformativeValueNumber - 1);

			// Cout de codage du choix des groupes
			dCost += KWStat::LnBell(nInformativeValueNumber, nInformativePartNumber);
		}
	}

	return dCost;
}

double DTMODLGroupingCosts::ComputePartitionDeltaCost(int nPartNumber) const
{
	require(GetValueNumber() < KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage());
	return ComputePartitionDeltaCost(nPartNumber, 0);
}
double DTMODLGroupingCosts::ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const
{
	// Cas d'utilisation de la granularite (granularite a 0 si pas de granu)
	double dDeltaCost;
	int nInformativeValueNumber;
	int nInformativePartNumber;

	require(nGranularity >= 0);
	require(nPartNumber >= 1);
	require(nValueNumber >= nPartNumber);
	require(nValueNumber > nGarbageModalityNumber);

	// Nombre de valeurs informatives : on enleve les eventuelles modalites du groupe poubelle
	nInformativeValueNumber = nValueNumber - nGarbageModalityNumber;
	// Nombre de parties informatives
	if (nGarbageModalityNumber > 0)
		nInformativePartNumber = nPartNumber - 1;
	else
		nInformativePartNumber = nPartNumber;

	// Cas d'une partition en au moins trois groupes informatifs (soit deux groupes informatifs apres
	// decrementation)
	if (nInformativePartNumber > 2)
	{
		dDeltaCost = KWStat::BoundedNaturalNumbersUniversalCodeLength(nInformativePartNumber - 2,
									      nInformativeValueNumber - 1) -
			     KWStat::BoundedNaturalNumbersUniversalCodeLength(nInformativePartNumber - 1,
									      nInformativeValueNumber - 1);
		dDeltaCost += KWStat::LnBell(nInformativeValueNumber, nInformativePartNumber - 1) -
			      KWStat::LnBell(nInformativeValueNumber, nInformativePartNumber);
	}
	// Sinon, on compare le cout de la partition en deux groupes informatives au cout du modele nul (1 groupe)
	else
		dDeltaCost = ComputePartitionCost(nPartNumber - 1, nGarbageModalityNumber) -
			     ComputePartitionCost(nPartNumber, nGarbageModalityNumber);

	ensure(fabs(ComputePartitionCost(nPartNumber - 1, nGarbageModalityNumber) -
		    ComputePartitionCost(nPartNumber, nGarbageModalityNumber) - dDeltaCost) < 1e-5);
	return dDeltaCost;
}

double DTMODLGroupingCosts::ComputePartCost(const KWFrequencyVector* part) const
{
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	IntVector* ivFrequencyVector;
	double dCost;
	int nFrequency;
	int nIntervalFrequency;
	int i;

	require(part != NULL);
	require(nClassValueNumber > 1);

	// Acces aux compteurs du vecteur d'effectif dense
	ivFrequencyVector = cast(KWDenseFrequencyVector*, part)->GetFrequencyVector();

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne
	dCost = 0;
	nIntervalFrequency = 0;
	for (i = 0; i < ivFrequencyVector->GetSize(); i++)
	{
		nFrequency = ivFrequencyVector->GetAt(i);
		dCost -= KWStat::LnFactorial(nFrequency);
		nIntervalFrequency += nFrequency;
	}
	dCost += KWStat::LnFactorial(nIntervalFrequency + nClassValueNumber - 1);
	dCost -= KWStat::LnFactorial(nClassValueNumber - 1);
	return dCost;
}

double DTMODLGroupingCosts::ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const
{
	require(partTable->GetFrequencyVectorAt(0)->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	double dCost;
	int i;

	require(partTable != NULL);
	require(nGranularity == partTable->GetGranularity());

	// Cout de partition plus somme des couts des parties
	dCost = ComputePartitionCost(partTable->GetFrequencyVectorNumber(), partTable->GetGarbageModalityNumber());

	for (i = 0; i < partTable->GetFrequencyVectorNumber(); i++)
		dCost += ComputePartCost(partTable->GetFrequencyVectorAt(i));

	return dCost;
}

void DTMODLGroupingCosts::WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const
{
	ost << "Part number\t" << nPartNumber << "\t" << ComputePartitionCost(nPartNumber, nGarbageModalityNumber)
	    << "\n";
}

double DTMODLGroupingCosts::ComputePartitionConstructionCost(int nPartNumber) const
{
	if (nPartNumber > 1)
		return log(2.0) + dAttributeCost;
	else
		return log(2.0);
}

double DTMODLGroupingCosts::ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const
{
	return ComputePartitionCost(nPartNumber, nGarbageModalityNumber);
}

double DTMODLGroupingCosts::ComputePartModelCost(const KWFrequencyVector* part) const
{
	IntVector* ivFrequencyVector;
	double dCost;
	int nFrequency;
	int nIntervalFrequency;
	int i;

	require(part != NULL);
	require(nClassValueNumber > 1);
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	// Acces aux compteurs du vecteur d'effectif dense
	ivFrequencyVector = cast(KWDenseFrequencyVector*, part)->GetFrequencyVector();

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne
	dCost = 0;
	nIntervalFrequency = 0;
	for (i = 0; i < ivFrequencyVector->GetSize(); i++)
	{
		nFrequency = ivFrequencyVector->GetAt(i);
		nIntervalFrequency += nFrequency;
	}
	dCost += KWStat::LnFactorial(nIntervalFrequency + nClassValueNumber - 1);
	dCost -= KWStat::LnFactorial(nClassValueNumber - 1);
	dCost -= KWStat::LnFactorial(nIntervalFrequency);
	return dCost;
}

const ALString DTMODLGroupingCosts::GetClassLabel() const
{
	return "MODL grouping costs";
}
