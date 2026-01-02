// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWUnivariatePartitionCost.h"

////////////////////////////////////////////////////////////////////////////
// Classe KWUnivariatePartitionCosts

const double KWUnivariatePartitionCosts::dEpsilon = 1e-6;

KWUnivariatePartitionCosts::KWUnivariatePartitionCosts()
{
	nValueNumber = 0;
	nClassValueNumber = 0;
	dAttributeCost = 0;
	kwfvFrequencyVectorCreator = new KWDenseFrequencyVector;
	nGranularity = 0;
	nTotalInstanceNumber = 0;
}

KWUnivariatePartitionCosts::~KWUnivariatePartitionCosts()
{
	delete kwfvFrequencyVectorCreator;
}

KWUnivariatePartitionCosts* KWUnivariatePartitionCosts::Clone() const
{
	KWUnivariatePartitionCosts* clone;

	clone = Create();
	clone->CopyFrom(this);
	return clone;
}

KWUnivariatePartitionCosts* KWUnivariatePartitionCosts::Create() const
{
	return new KWUnivariatePartitionCosts;
}

void KWUnivariatePartitionCosts::CopyFrom(const KWUnivariatePartitionCosts* sourceCosts)
{
	require(sourceCosts != NULL);

	if (sourceCosts != this)
	{
		SetValueNumber(sourceCosts->GetValueNumber());
		SetClassValueNumber(sourceCosts->GetClassValueNumber());
		SetAttributeCost(sourceCosts->GetAttributeCost());
		delete kwfvFrequencyVectorCreator;
		kwfvFrequencyVectorCreator = sourceCosts->kwfvFrequencyVectorCreator->Create();
		SetGranularity(sourceCosts->GetGranularity());
		SetTotalInstanceNumber(sourceCosts->GetTotalInstanceNumber());
	}
}

double KWUnivariatePartitionCosts::ComputePartitionCost(int nPartNumber) const
{
	assert(false);
	return 0;
}

double KWUnivariatePartitionCosts::ComputePartitionCost(int nPartNumber, int nGarbageModalityNumber) const
{
	assert(false);
	return 0;
}

double KWUnivariatePartitionCosts::ComputePartitionDeltaCost(int nPartNumber) const
{
	assert(false);
	return 0;
}

double KWUnivariatePartitionCosts::ComputePartitionDeltaCost(int nInformativePartNumber,
							     int nGarbageModalityNumber) const
{
	assert(false);
	return 0;
}

double KWUnivariatePartitionCosts::ComputePartCost(const KWFrequencyVector* part) const
{
	require(part != NULL);
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());
	return 0;
}

double KWUnivariatePartitionCosts::ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const
{
	assert(false);
	return 0;
}

void KWUnivariatePartitionCosts::WritePartitionAllCosts(const KWFrequencyTable* partTable, ostream& ost) const
{
	int i;

	require(partTable != NULL);
	require(nGranularity == partTable->GetGranularity());
	require(partTable->GetFrequencyVectorAt(0)->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	// Cout globaux et de partition
	ost << "Global cost\t" << ComputePartitionGlobalCost(partTable) << "\n";

	WritePartitionCost(partTable->GetFrequencyVectorNumber(), partTable->GetGarbageModalityNumber(), ost);

	// Cout par partie
	for (i = 0; i < partTable->GetFrequencyVectorNumber(); i++)
	{
		// Entete
		if (i == 0)
		{
			ost << "Index\t";
			partTable->GetFrequencyVectorAt(0)->WriteHeaderLineReport(ost);
			ost << "\tCost\n";
		}

		// Detail d'une partie
		ost << i << "\t";
		WritePartCost(partTable->GetFrequencyVectorAt(i), ost);
	}
}

void KWUnivariatePartitionCosts::WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const
{
	assert(false);
	ost << "Erreur WritePartitionCost"
	    << "\n";
}

void KWUnivariatePartitionCosts::WritePartCost(const KWFrequencyVector* part, ostream& ost) const
{
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	part->WriteLineReport(ost);
	ost << "\t" << ComputePartCost(part) << "\n";
}

double KWUnivariatePartitionCosts::ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const
{
	return 0;
}

double KWUnivariatePartitionCosts::ComputePartModelCost(const KWFrequencyVector* part) const
{
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());
	return 0;
}

double KWUnivariatePartitionCosts::ComputePartitionConstructionCost(int nPartNumber) const
{
	return 0;
}

double KWUnivariatePartitionCosts::ComputePartConstructionCost(const KWFrequencyVector* part) const
{
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());
	return 0;
}

double KWUnivariatePartitionCosts::ComputePartitionPreparationCost(int nPartNumber, int nGarbageModalityNumber) const
{
	double dPreparationCost;
	int nPartitionPartNumber;

	if (nGarbageModalityNumber > 0)
		nPartitionPartNumber = nPartNumber - 1;
	else
		nPartitionPartNumber = nPartNumber;
	dPreparationCost = ComputePartitionModelCost(nPartitionPartNumber, nGarbageModalityNumber) -
			   ComputePartitionConstructionCost(nPartitionPartNumber);
	assert(dPreparationCost >= -dEpsilon);
	if (dPreparationCost < dEpsilon)
		dPreparationCost = 0;
	return dPreparationCost;
}

double KWUnivariatePartitionCosts::ComputePartPreparationCost(const KWFrequencyVector* part) const
{
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	double dPreparationCost;
	dPreparationCost = ComputePartCost(part) - ComputePartModelCost(part);
	assert(dPreparationCost >= -dEpsilon);
	if (dPreparationCost < dEpsilon)
		dPreparationCost = 0;
	return dPreparationCost;
}

double KWUnivariatePartitionCosts::ComputePartitionDataCost(int nPartNumber, int nGarbageModalityNumber) const
{
	double dDataCost;
	int nPartitionPartNumber;

	// Cas de la presence d'un groupe poubelle
	if (nGarbageModalityNumber > 0)
		nPartitionPartNumber = nPartNumber - 1;
	// Sinon
	else
		nPartitionPartNumber = nPartNumber;

	dDataCost = ComputePartitionCost(nPartitionPartNumber) -
		    ComputePartitionModelCost(nPartitionPartNumber, nGarbageModalityNumber);

	assert(dDataCost >= -dEpsilon);
	if (dDataCost < dEpsilon)
		dDataCost = 0;
	return dDataCost;
}

double KWUnivariatePartitionCosts::ComputePartDataCost(const KWFrequencyVector* part) const
{
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	double dDataCost;
	dDataCost = ComputePartCost(part) - ComputePartModelCost(part);
	assert(dDataCost >= -dEpsilon);
	if (dDataCost < dEpsilon)
		dDataCost = 0;
	return dDataCost;
}

double KWUnivariatePartitionCosts::ComputePartitionGlobalModelCost(const KWFrequencyTable* partTable) const
{
	require(partTable->GetFrequencyVectorAt(0)->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	boolean bDisplayResults = false;
	double dModelCost;
	int i;

	require(partTable != NULL);
	require(nGranularity == partTable->GetGranularity());
	require(nTotalInstanceNumber == 0 or nTotalInstanceNumber == partTable->GetTotalFrequency());

	// Cout de partition plus somme des couts des parties
	dModelCost =
	    ComputePartitionModelCost(partTable->GetFrequencyVectorNumber(), partTable->GetGarbageModalityNumber());
	if (bDisplayResults)
		cout << "ComputePartitionGlobalModelCost : partition " << dModelCost << endl;
	for (i = 0; i < partTable->GetFrequencyVectorNumber(); i++)
		dModelCost += ComputePartModelCost(partTable->GetFrequencyVectorAt(i));
	if (bDisplayResults)
		cout << "ComputePartitionGlobalModelCost : final " << dModelCost << endl;

	return dModelCost;
}

double KWUnivariatePartitionCosts::ComputePartitionGlobalConstructionCost(const KWFrequencyTable* partTable) const
{
	require(partTable->GetFrequencyVectorAt(0)->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	boolean bDisplayResults = false;
	double dConstructionCost;
	int i;
	int nPartitionPartNumber;

	require(partTable != NULL);
	require(nGranularity == partTable->GetGranularity());
	require(nTotalInstanceNumber == 0 or nTotalInstanceNumber == partTable->GetTotalFrequency());

	if (partTable->GetGarbageModalityNumber() > 0)
		nPartitionPartNumber = partTable->GetFrequencyVectorNumber() - 1;
	else
		nPartitionPartNumber = partTable->GetFrequencyVectorNumber();

	// Cout de partition plus somme des couts des parties
	dConstructionCost = ComputePartitionConstructionCost(nPartitionPartNumber);

	if (bDisplayResults)
		cout << "ComputePartitionGlobalConstructionCost : partition " << dConstructionCost << endl;
	for (i = 0; i < partTable->GetFrequencyVectorNumber(); i++)
		dConstructionCost += ComputePartConstructionCost(partTable->GetFrequencyVectorAt(i));
	if (bDisplayResults)
		cout << "ComputePartitionGlobalConstructionCost : fin " << dConstructionCost << endl;

	return dConstructionCost;
}

double KWUnivariatePartitionCosts::ComputePartitionGlobalPreparationCost(const KWFrequencyTable* partTable) const
{
	require(partTable->GetFrequencyVectorAt(0)->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	double dPreparationCost;

	require(partTable != NULL);
	require(nGranularity == partTable->GetGranularity());
	require(nTotalInstanceNumber == 0 or nTotalInstanceNumber == partTable->GetTotalFrequency());

	dPreparationCost =
	    ComputePartitionGlobalModelCost(partTable) - ComputePartitionGlobalConstructionCost(partTable);
	assert(dPreparationCost >= -dEpsilon);
	if (dPreparationCost < 0)
		dPreparationCost = 0;

	return dPreparationCost;
}

double KWUnivariatePartitionCosts::ComputePartitionGlobalDataCost(const KWFrequencyTable* partTable) const
{
	require(partTable->GetFrequencyVectorAt(0)->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	double dDataCost;

	require(partTable != NULL);
	require(nGranularity == partTable->GetGranularity());
	require(nTotalInstanceNumber == 0 or nTotalInstanceNumber == partTable->GetTotalFrequency());

	dDataCost = ComputePartitionGlobalCost(partTable) - ComputePartitionGlobalModelCost(partTable);
	assert(dDataCost >= -dEpsilon);
	if (dDataCost < 0)
		dDataCost = 0;

	return dDataCost;
}

void KWUnivariatePartitionCosts::Write(ostream& ost) const
{
	cout << "Univariate Partition Costs" << endl;
	cout << "Granularity\tTotalInstanceNumber\tValueNumber\tClassValueNumber\tAttributeCost\n";
	cout << nGranularity << "\t" << nTotalInstanceNumber << "\t" << nValueNumber << "\t" << nClassValueNumber
	     << "\t" << dAttributeCost << endl;
}

const ALString KWUnivariatePartitionCosts::GetClassLabel() const
{
	return "Univariate costs";
}

////////////////////////////////////////////////////////////////////////////
// Classe KWMODLDiscretizationCosts

KWMODLDiscretizationCosts::KWMODLDiscretizationCosts() {}

KWMODLDiscretizationCosts::~KWMODLDiscretizationCosts() {}

KWUnivariatePartitionCosts* KWMODLDiscretizationCosts::Create() const
{
	return new KWMODLDiscretizationCosts;
}

double KWMODLDiscretizationCosts::ComputePartitionCost(int nPartNumber) const
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
		dCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartNumber - 1, nValueNumber - 1);
		if (bDisplayResults)
			cout << "Cout choix nombre de parties " << nPartNumber << " parmi " << nValueNumber << "\t"
			     << KWStat::BoundedNaturalNumbersUniversalCodeLength(nPartNumber - 1, nValueNumber - 1)
			     << endl;

		// Partition en intervalles
		// Nouveau codage avec description du choix des coupures selon une multinomiale
		dCost += (nPartNumber - 1) * log((nValueNumber - 1) * 1.0);
		dCost -= KWStat::LnFactorial(nPartNumber - 1);

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

double KWMODLDiscretizationCosts::ComputePartitionDeltaCost(int nPartNumber) const
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

	ensure(fabs(ComputePartitionCost(nPartNumber - 1) - ComputePartitionCost(nPartNumber) - dDeltaCost) < dEpsilon);
	return dDeltaCost;
}

double KWMODLDiscretizationCosts::ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const
{
	require(nGarbageModalityNumber == 0);
	return ComputePartitionDeltaCost(nPartNumber);
}

double KWMODLDiscretizationCosts::ComputePartCost(const KWFrequencyVector* part) const
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

double KWMODLDiscretizationCosts::ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const
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

void KWMODLDiscretizationCosts::WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const
{
	ost << "Part number\t" << nPartNumber << "\t" << ComputePartitionCost(nPartNumber) << "\n";
}

double KWMODLDiscretizationCosts::ComputePartitionConstructionCost(int nPartNumber) const
{
	if (nPartNumber > 1)
		return log(2.0) + dAttributeCost;
	else
		return log(2.0);
}

double KWMODLDiscretizationCosts::ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const
{
	require(nGarbageModalityNumber == 0);
	return ComputePartitionCost(nPartNumber);
}

double KWMODLDiscretizationCosts::ComputePartModelCost(const KWFrequencyVector* part) const
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

const ALString KWMODLDiscretizationCosts::GetClassLabel() const
{
	return "MODL discretization costs";
}

////////////////////////////////////////////////////////////////////////////
// Classe KWMODLGroupingCosts

KWMODLGroupingCosts::KWMODLGroupingCosts() {}

KWMODLGroupingCosts::~KWMODLGroupingCosts() {}

KWUnivariatePartitionCosts* KWMODLGroupingCosts::Create() const
{
	return new KWMODLGroupingCosts;
}

double KWMODLGroupingCosts::ComputePartitionCost(int nPartNumber) const
{
	require(GetValueNumber() < KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage());
	return ComputePartitionCost(nPartNumber, 0);
}

double KWMODLGroupingCosts::ComputePartitionCost(int nPartNumber, int nGarbageModalityNumber) const
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
		if (nValueNumber > KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage())
			dCost += log(2.0);

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

double KWMODLGroupingCosts::ComputePartitionDeltaCost(int nPartNumber) const
{
	require(GetValueNumber() < KWFrequencyTable::GetMinimumNumberOfModalitiesForGarbage());
	return ComputePartitionDeltaCost(nPartNumber, 0);
}
double KWMODLGroupingCosts::ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const
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

double KWMODLGroupingCosts::ComputePartCost(const KWFrequencyVector* part) const
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

double KWMODLGroupingCosts::ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const
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

void KWMODLGroupingCosts::WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const
{
	ost << "Part number\t" << nPartNumber << "\t" << ComputePartitionCost(nPartNumber, nGarbageModalityNumber)
	    << "\n";
}

double KWMODLGroupingCosts::ComputePartitionConstructionCost(int nPartNumber) const
{
	if (nPartNumber > 1)
		return log(2.0) + dAttributeCost;
	else
		return log(2.0);
}

double KWMODLGroupingCosts::ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const
{
	return ComputePartitionCost(nPartNumber, nGarbageModalityNumber);
}

double KWMODLGroupingCosts::ComputePartModelCost(const KWFrequencyVector* part) const
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

const ALString KWMODLGroupingCosts::GetClassLabel() const
{
	return "MODL grouping costs";
}

////////////////////////////////////////////////////////////////////////////
// Classe KWUnivariateNullPartitionCosts

KWUnivariateNullPartitionCosts::KWUnivariateNullPartitionCosts()
{
	univariatePartitionCosts = NULL;
}

KWUnivariateNullPartitionCosts::~KWUnivariateNullPartitionCosts()
{
	if (univariatePartitionCosts != NULL)
		delete univariatePartitionCosts;
}

void KWUnivariateNullPartitionCosts::SetUnivariatePartitionCosts(KWUnivariatePartitionCosts* kwupcCosts)
{
	if (univariatePartitionCosts != NULL)
		delete univariatePartitionCosts;
	univariatePartitionCosts = kwupcCosts;
}

KWUnivariatePartitionCosts* KWUnivariateNullPartitionCosts::GetUnivariatePartitionCosts() const
{
	return univariatePartitionCosts;
}

KWUnivariatePartitionCosts* KWUnivariateNullPartitionCosts::Create() const
{
	return new KWUnivariateNullPartitionCosts;
}

void KWUnivariateNullPartitionCosts::CopyFrom(const KWUnivariatePartitionCosts* sourceCosts)
{
	const KWUnivariateNullPartitionCosts* sourceNullCost;

	require(sourceCosts != NULL);

	// Acces a'lobjet source dans son bon type
	sourceNullCost = cast(const KWUnivariateNullPartitionCosts*, sourceCosts);

	// Appel de la methode ancetre
	KWUnivariatePartitionCosts::CopyFrom(sourceNullCost);

	// Recopie du cout de base
	if (sourceNullCost != this)
		SetUnivariatePartitionCosts(sourceNullCost->GetUnivariatePartitionCosts());
}

double KWUnivariateNullPartitionCosts::ComputePartitionCost(int nPartNumber) const
{
	return 0;
}

double KWUnivariateNullPartitionCosts::ComputePartitionCost(int nPartNumber, int nGarbageModalityNumber) const
{
	return 0;
}

double KWUnivariateNullPartitionCosts::ComputePartitionDeltaCost(int nPartNumber) const
{
	return 0;
}

double KWUnivariateNullPartitionCosts::ComputePartitionDeltaCost(int nPartNumber, int nGarbageModalityNumber) const
{
	return 0;
}

double KWUnivariateNullPartitionCosts::ComputePartCost(const KWFrequencyVector* part) const
{
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	if (univariatePartitionCosts != NULL)
		return univariatePartitionCosts->ComputePartCost(part);
	else
		return 0;
}

double KWUnivariateNullPartitionCosts::ComputePartitionGlobalCost(const KWFrequencyTable* partTable) const
{
	double dCost;
	int i;

	require(partTable != NULL);
	require(univariatePartitionCosts->GetGranularity() == partTable->GetGranularity());
	require(partTable->GetFrequencyVectorAt(0)->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	// Cout de partition plus somme des couts des parties
	dCost = ComputePartitionCost(partTable->GetFrequencyVectorNumber());

	for (i = 0; i < partTable->GetFrequencyVectorNumber(); i++)
		dCost += ComputePartCost(partTable->GetFrequencyVectorAt(i));

	return dCost;
}
void KWUnivariateNullPartitionCosts::WritePartitionCost(int nPartNumber, int nGarbageModalityNumber, ostream& ost) const
{
	ost << "Part number\t" << nPartNumber << "\t" << ComputePartitionCost(nPartNumber) << "\n";
}

double KWUnivariateNullPartitionCosts::ComputePartitionModelCost(int nPartNumber, int nGarbageModalityNumber) const
{
	return ComputePartitionCost(nPartNumber);
}

double KWUnivariateNullPartitionCosts::ComputePartModelCost(const KWFrequencyVector* part) const
{
	require(part->GetClassLabel() == GetFrequencyVectorCreator()->GetClassLabel());

	if (univariatePartitionCosts != NULL)
		return univariatePartitionCosts->ComputePartModelCost(part);
	else
		return 0;
}

const ALString KWUnivariateNullPartitionCosts::GetClassLabel() const
{
	if (univariatePartitionCosts != NULL)
		return univariatePartitionCosts->GetClassLabel() + " (No partition cost)";
	else
		return KWUnivariatePartitionCosts::GetClassLabel() + " (No partition cost)";
}
