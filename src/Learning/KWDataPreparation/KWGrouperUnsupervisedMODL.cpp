// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWGrouperMODL.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe KWGrouperUnsupervisedMODL

KWGrouperUnsupervisedMODL::KWGrouperUnsupervisedMODL()
{
	// Activation du pretraitement effectue dans la methode Group pour ce grouper qui n'utilise pas le mode
	// granularite/poubelle
	bActivePreprocessing = true;
}

KWGrouperUnsupervisedMODL::~KWGrouperUnsupervisedMODL() {}

const ALString KWGrouperUnsupervisedMODL::GetName() const
{
	return "MODL";
}

KWGrouper* KWGrouperUnsupervisedMODL::Create() const
{
	return new KWGrouperUnsupervisedMODL;
}

int KWGrouperUnsupervisedMODL::ComputePreprocessedMaxLineNumber(KWFrequencyTable* table) const
{
	boolean bTwoLevelHierarchy = true;
	const int nMinTwoLevelLineNumber = 100;
	int nBestValueNumber;
	double dPriorCost;
	double dSecondgroupPriorCost;
	int nSecondGroupBestValueNumber;

	require(table != NULL);
	require(table->IsTableSortedBySourceFrequency(false));
	require(table->GetFrequencyVectorNumber() == table->GetInitialValueNumber());

	// Calcul de la coupure suite au meilleur encodage
	dPriorCost = ComputeBestPriorCost(table, 0, table->GetInitialValueNumber(), nBestValueNumber);

	// Si on a un vraie coupure, on recoupe la deuxieme partie en deux
	// On applique cette heuristique, car souvent, la premiere coupure fait trop peu de groupe meme avec peu de
	// valeurs (par exemple, seulement deux valeurs, donc trois groupes pour les 41 valeurs de native_country de la
	// base Adult). Et inversement, dans le cas de tres grand nombre de valeurs, le deuxieme niveau de hierarchie
	// cree trop de valeurs. (par exemple: plusieurs millieons de valeurs sur certaines variables de la base Criteo)
	// En se limitant au seuil de 100 valeurs, on assure une precision a au moins 1% pres, ce qui est suffisant
	// pour de l'analyse exploratoire
	// Et le seuil automatique par le codage multinomial apporte un service utile et pertinent
	// (par exemple, un histogramme de 13 barres pour seulement 15 valeurs de la Var37 de la base Criteo, qui
	// contient 45 millions d'individus, et ici un groupe "poubelle" de seulement 8000 individus, et )
	nSecondGroupBestValueNumber = 0;
	if (bTwoLevelHierarchy and nBestValueNumber < nMinTwoLevelLineNumber and
	    nBestValueNumber < table->GetInitialValueNumber() - 1)
	{
		dSecondgroupPriorCost = ComputeBestPriorCost(table, nBestValueNumber, table->GetInitialValueNumber(),
							     nSecondGroupBestValueNumber);

		// On ajoute la premiere partie du second groupe au premier groupe
		nBestValueNumber += nSecondGroupBestValueNumber;
	}

	// Dans le cas ou on a deux groupes non vides, on rajoute une ligne au nombre de valeurs du premier groupe,
	// cette derniere ligne correspondant au groupe poubelle qui contient toutes les valeur du deuxieme groupe
	if (nBestValueNumber < table->GetInitialValueNumber())
		nBestValueNumber++;

	// Prise en compte de la contrainte de nombre max de groupe
	if (GetMaxGroupNumber() > 0)
		nBestValueNumber = min(nBestValueNumber, GetMaxGroupNumber());
	return nBestValueNumber;
}

double KWGrouperUnsupervisedMODL::ComputeBestPriorCost(const KWFrequencyTable* table, int nStartIndex, int nStopIndex,
						       int& nBestFirstGroupValueNumber) const
{
	boolean bDisplay = false;
	const double dEpsilon = 1e-6;
	int nValueNumber;
	int nFrequency;
	double dStandardPriorCost;
	double dBestHierarchicalPriorCost;
	int nBestHierarchicalValueNumber;

	require(table != NULL);
	require(table->IsTableSortedBySourceFrequency(false));
	require(table->GetFrequencyVectorNumber() == table->GetInitialValueNumber());
	require(nStartIndex >= 0 and nStartIndex < nStopIndex);
	require(nStopIndex <= table->GetFrequencyVectorNumber());

	// Statistiques sur la sous table
	nValueNumber = nStopIndex - nStartIndex;
	nFrequency = ComputeSubTableFrequency(table, nStartIndex, nStopIndex);

	// Calcul du cout du prior selon l'encodage standard des multinomial
	dStandardPriorCost = ComputeMultinomialPriorCost(nFrequency, nValueNumber);
	if (bDisplay)
	{
		cout << "Unsupervised MODL\n";
		cout << "\tStandard cost\t" << nValueNumber << "\t" << nFrequency << "\t0\t0\t" << dStandardPriorCost
		     << "\n";
	}

	// Evaluation de toutes les coupures de la table en 2 partie pour trouver
	// l'encodage multinomial hierarchique optimal
	if (nStopIndex - nStartIndex > 1)
	{
		dBestHierarchicalPriorCost = ComputeBestHierarchicalMultinomialPriorCost(table, nStartIndex, nStopIndex,
											 nBestHierarchicalValueNumber);

		// Comparaison entre le critere standard et le meilleur critere hierarchique
		if (dBestHierarchicalPriorCost < dStandardPriorCost - dEpsilon)
			nBestFirstGroupValueNumber = nBestHierarchicalValueNumber;
		else
			nBestFirstGroupValueNumber = nValueNumber;
	}
	else
		nBestFirstGroupValueNumber = nValueNumber;
	if (bDisplay)
		cout << "\t=> Best value number\t" << nBestFirstGroupValueNumber << "\n";
	return nBestFirstGroupValueNumber;
}

double KWGrouperUnsupervisedMODL::ComputeBestHierarchicalMultinomialPriorCost(const KWFrequencyTable* table,
									      int nStartIndex, int nStopIndex,
									      int& nBestFirstGroupValueNumber) const
{
	boolean bDisplay = false;
	const double dEpsilon = 1e-6;
	int i;
	int nFrequency;
	int nValueNumber;
	double dHierarchicalPriorCost;
	double dBestHierarchicalPriorCost;
	int nBestHierarchicalValueNumber;
	int nFirstGroupValueNumber;
	int nFirstGroupFrequency;
	int nSecondGroupValueNumber;
	int nSecondGroupFrequency;

	require(table != NULL);
	require(table->IsTableSortedBySourceFrequency(false));
	require(table->GetFrequencyVectorNumber() == table->GetInitialValueNumber());
	require(nStartIndex >= 0 and nStartIndex + 1 < nStopIndex);
	require(nStopIndex <= table->GetFrequencyVectorNumber());

	// Statistiques sur la sous table
	nValueNumber = nStopIndex - nStartIndex;
	nFrequency = ComputeSubTableFrequency(table, nStartIndex, nStopIndex);

	// Evaluation de toutes les coupures de la table en 2 partie pour trouver
	// l'encodage multinomial hierarchique optimal
	dBestHierarchicalPriorCost = DBL_MAX;
	nBestHierarchicalValueNumber = -1;
	nFirstGroupValueNumber = 0;
	nFirstGroupFrequency = 0;
	for (i = nStartIndex; i < nStopIndex - 1; i++)
	{
		// Statistique sur le premier groupe
		nFirstGroupValueNumber++;
		nFirstGroupFrequency += table->GetFrequencyVectorAt(i)->ComputeTotalFrequency();

		// Statistique sur le second groupe
		nSecondGroupValueNumber = nValueNumber - nFirstGroupValueNumber;
		nSecondGroupFrequency = nFrequency - nFirstGroupFrequency;

		// Calcul du critere
		dHierarchicalPriorCost = ComputeHierarchicalMultinomialPriorCost(
		    nFirstGroupFrequency, nFirstGroupValueNumber, nSecondGroupFrequency, nSecondGroupValueNumber);

		// Test si amelioration du critere
		if (dHierarchicalPriorCost < dBestHierarchicalPriorCost - dEpsilon)
		{
			dBestHierarchicalPriorCost = dHierarchicalPriorCost;
			nBestHierarchicalValueNumber = nFirstGroupValueNumber;
		}
		if (bDisplay)
		{
			cout << "\tHierarchical cost\t" << nFirstGroupValueNumber << "\t" << nFirstGroupFrequency
			     << "\t" << nSecondGroupValueNumber << "\t" << nSecondGroupFrequency << "\t"
			     << dHierarchicalPriorCost << "\t" << dBestHierarchicalPriorCost << "\n";
		}
	}
	assert(nBestHierarchicalValueNumber != -1);

	// Retour des resultats obtenus
	nBestFirstGroupValueNumber = nBestHierarchicalValueNumber;
	return dBestHierarchicalPriorCost;
}

double KWGrouperUnsupervisedMODL::ComputeMultinomialPriorCost(int nFrequency, int nValueNumber) const
{
	double dCost;

	require(nFrequency > 0);
	require(nValueNumber > 0);

	// Cout d'encodage des parametre d'une distribution multinomiale
	dCost = KWStat::LnFactorial(nFrequency + nValueNumber - 1);
	dCost -= KWStat::LnFactorial(nFrequency);
	dCost -= KWStat::LnFactorial(nValueNumber - 1);
	return dCost;
}

double KWGrouperUnsupervisedMODL::ComputeHierarchicalMultinomialPriorCost(int nFirstGroupFrequency,
									  int nFirstGroupValueNumber,
									  int nSecondGroupFrequency,
									  int nSecondGroupValueNumber) const
{
	double dCost;
	int nFrequency;
	int nValueNumber;

	require(nFirstGroupFrequency > 0);
	require(nFirstGroupValueNumber > 0);
	require(nSecondGroupFrequency > 0);
	require(nSecondGroupValueNumber > 0);

	// Statistiques globales
	nFrequency = nFirstGroupFrequency + nSecondGroupFrequency;
	nValueNumber = nFirstGroupValueNumber + nSecondGroupValueNumber;

	// Choix du nombre de valeurs dans chaque groupe
	dCost = log(nValueNumber / 2);

	// Choix du nombre d'individus dans chaque groupe
	dCost += log(nFrequency + 1);

	// Choix de la distribution des valeurs entre les deux groupes
	dCost += KWStat::LnFactorial(nValueNumber);
	dCost -= KWStat::LnFactorial(nFirstGroupValueNumber);
	dCost -= KWStat::LnFactorial(nSecondGroupValueNumber);

	// Codage de la sous-multionomiale de chaque groupe
	dCost += ComputeMultinomialPriorCost(nFirstGroupFrequency, nFirstGroupValueNumber);
	dCost += ComputeMultinomialPriorCost(nSecondGroupFrequency, nSecondGroupValueNumber);
	return dCost;
}

int KWGrouperUnsupervisedMODL::ComputeSubTableFrequency(const KWFrequencyTable* table, int nStartIndex,
							int nStopIndex) const
{
	int nFrequency;
	int i;

	require(table != NULL);
	require(nStartIndex >= 0 and nStartIndex < nStopIndex);
	require(nStopIndex <= table->GetFrequencyVectorNumber());

	nFrequency = 0;
	for (i = nStartIndex; i < nStopIndex; i++)
		nFrequency += table->GetFrequencyVectorAt(i)->ComputeTotalFrequency();
	return nFrequency;
}

boolean KWGrouperUnsupervisedMODL::Check() const
{
	boolean bOk;
	bOk = dParam == 0;
	if (not bOk)
		AddError("The main parameter of the algorithm must be 0");
	return bOk;
}

const ALString KWGrouperUnsupervisedMODL::GetObjectLabel() const
{
	if (GetMinGroupFrequency() == 0 and GetMaxGroupNumber() == 0)
		return GetName();
	else
		return GetName() + "(" + IntToString(GetMinGroupFrequency()) + ", " + IntToString(GetMaxGroupNumber()) +
		       ")";
}