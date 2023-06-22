// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDiscretizerUnsupervised.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDiscretizerUsingSourceValues

KWDiscretizerUsingSourceValues::KWDiscretizerUsingSourceValues() {}

KWDiscretizerUsingSourceValues::~KWDiscretizerUsingSourceValues() {}

boolean KWDiscretizerUsingSourceValues::IsUsingSourceValues() const
{
	return true;
}

boolean KWDiscretizerUsingSourceValues::Check() const
{
	boolean bOk;

	// Verification de l'absence de parametrage
	bOk = dParam == 0;
	if (not bOk)
		AddError("The main parameter of the algorithm must be 0");

	// Impossible de parametrer la frequence minimum par intervalle
	if (GetMinIntervalFrequency() > 0)
	{
		bOk = false;
		AddError(
		    "Specification of the min frequency per interval inconsistent with unsupervized discretization");
	}
	return bOk;
}

const ALString KWDiscretizerUsingSourceValues::GetObjectLabel() const
{
	if (GetMaxIntervalNumber() > 0)
		return GetName() + "(" + IntToString(GetMaxIntervalNumber()) + ")";
	else
		return GetName();
}

void KWDiscretizerUsingSourceValues::EqualBinsDiscretizeValues(boolean bIsEqualFrequency,
							       ContinuousVector* cvSourceValues,
							       IntVector* ivTargetIndexes, int nTargetValueNumber,
							       KWFrequencyTable*& kwftTarget) const
{
	const int nDefaultMaxIntervalNumber = 10;
	int nIntervalNumber;
	KWQuantileIntervalBuilder quantileBuilder;
	ObjectArray oaCumulativeTargetFrequencies;

	require(Check());
	require(GetMaxIntervalNumber() >= 0);
	require(cvSourceValues != NULL);
	require(ivTargetIndexes != NULL);
	require(nTargetValueNumber >= 0);

	// Si pas d'instance, on renvoie une table de contingence NULL
	kwftTarget = NULL;
	if (cvSourceValues->GetSize() == 0)
		return;

	// Nombre d'intervalles
	nIntervalNumber = GetMaxIntervalNumber();
	if (nIntervalNumber == 0)
		nIntervalNumber = nDefaultMaxIntervalNumber;

	// Initialisation du calculateur de quantiles
	quantileBuilder.InitializeValues(cvSourceValues);

	// Calcul des vecteurs de frequences cumulees par modalite cible
	ComputeCumulativeTargetFrequencies(&quantileBuilder, ivTargetIndexes, nTargetValueNumber,
					   &oaCumulativeTargetFrequencies);

	// Discretisation
	if (bIsEqualFrequency)
		quantileBuilder.ComputeQuantiles(nIntervalNumber);
	else
		quantileBuilder.ComputeEqualWidthQuantiles(nIntervalNumber);

	// Creation de la table d'effectif resultat
	ComputeTargetFrequencyTable(&quantileBuilder, &oaCumulativeTargetFrequencies, kwftTarget);

	// Nettoyage
	oaCumulativeTargetFrequencies.DeleteAll();
}

void KWDiscretizerUsingSourceValues::ComputeCumulativeTargetFrequencies(
    KWQuantileIntervalBuilder* quantileBuilder, IntVector* ivTargetIndexes, int nTargetValueNumber,
    ObjectArray* oaCumulativeTargetFrequencies) const
{
	boolean bDisplayResults = false;
	IntVector* ivCumulativeFrequencies;
	int nTarget;
	int i;
	int nValueNumber;
	int nValueIndex;
	int nTargetIndex;
	int nInstanceNumber;

	require(Check());
	require(quantileBuilder != NULL);
	require(ivTargetIndexes != NULL);
	require(quantileBuilder->IsFrequencyInitialized());
	require(quantileBuilder->GetInstanceNumber() == ivTargetIndexes->GetSize());
	require(nTargetValueNumber >= 0);
	require(oaCumulativeTargetFrequencies != NULL);
	require(oaCumulativeTargetFrequencies->GetSize() == 0);

	// Initialisation des des vecteurs de frequences cumulees par modalite cible
	nInstanceNumber = quantileBuilder->GetInstanceNumber();
	nValueNumber = quantileBuilder->GetValueNumber();
	oaCumulativeTargetFrequencies->SetSize(nTargetValueNumber);
	for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
	{
		ivCumulativeFrequencies = new IntVector;
		ivCumulativeFrequencies->SetSize(nValueNumber);
		oaCumulativeTargetFrequencies->SetAt(nTarget, ivCumulativeFrequencies);
	}

	// Initialisation du vecteur des cumuls par valeur cible, pour chaque index
	// de valeur source
	nValueIndex = 0;
	for (i = 0; i < nInstanceNumber; i++)
	{
		nTargetIndex = ivTargetIndexes->GetAt(i);

		// Detection des changements de valeur
		if (i > quantileBuilder->GetValueCumulatedFrequencAt(nValueIndex) - 1)
		{
			// Memorisation du nouvel index de valeur
			nValueIndex++;

			// Memorisation des cumuls precedents par valeur cible
			for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
			{
				ivCumulativeFrequencies =
				    cast(IntVector*, oaCumulativeTargetFrequencies->GetAt(nTarget));
				ivCumulativeFrequencies->SetAt(nValueIndex,
							       ivCumulativeFrequencies->GetAt(nValueIndex - 1));
			}
		}

		// Incrementation du cumul pour la valeur cible correspondant a l'instance en cours
		ivCumulativeFrequencies = cast(IntVector*, oaCumulativeTargetFrequencies->GetAt(nTargetIndex));
		ivCumulativeFrequencies->UpgradeAt(nValueIndex, 1);
	}

	// Affichage des resultats intermediaires
	if (bDisplayResults)
	{
		// Affichage des resultat pour toutes les valeurs
		cout << "Value\tCumulated target frequencies\n";
		nValueIndex = 0;
		for (nValueIndex = 0; nValueIndex < nValueNumber; nValueIndex++)
		{
			// Valeur
			cout << quantileBuilder->GetValueAt(nValueIndex);

			// Cumul des frequences par modalite cible
			for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
			{
				ivCumulativeFrequencies =
				    cast(IntVector*, oaCumulativeTargetFrequencies->GetAt(nTarget));
				cout << "\t" << ivCumulativeFrequencies->GetAt(nValueIndex);
			}
			cout << "\n";
		}
		cout << endl;
	}
}

void KWDiscretizerUsingSourceValues::ComputeTargetFrequencyTable(KWQuantileIntervalBuilder* quantileBuilder,
								 ObjectArray* oaCumulativeTargetFrequencies,
								 KWFrequencyTable*& kwftTarget) const
{
	IntVector ivIntervalLastValueIndexes;
	IntVector* ivCumulativeFrequencies;
	int nTargetValueNumber;
	int nPreviousIntervalValueIndex;
	int nIntervalValueIndex;
	int nIntervalIndex;
	int nTarget;
	int nFrequency;
	KWDenseFrequencyVector* kwdfvFrequencyVector;
	IntVector* ivFrequencyVector;

	require(Check());
	require(quantileBuilder != NULL);
	require(quantileBuilder->IsComputed());
	require(oaCumulativeTargetFrequencies != NULL);
	require(oaCumulativeTargetFrequencies->GetSize() > 0);

	// Creation de la table d'effectifs avec le bon nombre d'intervalles
	kwftTarget = new KWFrequencyTable;
	nTargetValueNumber = oaCumulativeTargetFrequencies->GetSize();
	kwftTarget->Initialize(quantileBuilder->GetIntervalNumber());
	kwftTarget->SetInitialValueNumber(quantileBuilder->GetValueNumber());
	kwftTarget->SetGranularizedValueNumber(quantileBuilder->GetValueNumber());

	// Calcul de la discretisation avec la taille des intervalles specifies
	nPreviousIntervalValueIndex = -1;
	for (nIntervalIndex = 0; nIntervalIndex < quantileBuilder->GetIntervalNumber(); nIntervalIndex++)
	{
		nIntervalValueIndex = quantileBuilder->GetIntervalLastValueIndexAt(nIntervalIndex);

		// Acces au vecteur (sense etre en representation dense)
		kwdfvFrequencyVector = cast(KWDenseFrequencyVector*, kwftTarget->GetFrequencyVectorAt(nIntervalIndex));

		// Recopie de son contenu
		ivFrequencyVector = kwdfvFrequencyVector->GetFrequencyVector();
		ivFrequencyVector->SetSize(nTargetValueNumber);

		// Calcul des frequences cible de l'intervalle en cours, par difference
		// des cumul entre la borne sup de l'intervalle et celle du precedent
		for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
		{
			// Acces au vecteur des frequence cumulees
			ivCumulativeFrequencies = cast(IntVector*, oaCumulativeTargetFrequencies->GetAt(nTarget));
			assert(ivCumulativeFrequencies->GetSize() == quantileBuilder->GetValueNumber());

			// Calcul de la difference, et memorisation
			nFrequency = ivCumulativeFrequencies->GetAt(nIntervalValueIndex);
			if (nPreviousIntervalValueIndex >= 0)
				nFrequency -= ivCumulativeFrequencies->GetAt(nPreviousIntervalValueIndex);
			ivFrequencyVector->SetAt(nTarget, nFrequency);
		}
		nPreviousIntervalValueIndex = nIntervalValueIndex;
	}
	assert(kwftTarget->GetTotalFrequency() == quantileBuilder->GetInstanceNumber());
}

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDiscretizerEqualWidth

KWDiscretizerEqualWidth::KWDiscretizerEqualWidth() {}

KWDiscretizerEqualWidth::~KWDiscretizerEqualWidth() {}

const ALString KWDiscretizerEqualWidth::GetName() const
{
	return "EqualWidth";
}

KWDiscretizer* KWDiscretizerEqualWidth::Create() const
{
	return new KWDiscretizerEqualWidth;
}

void KWDiscretizerEqualWidth::DiscretizeValues(ContinuousVector* cvSourceValues, IntVector* ivTargetIndexes,
					       int nTargetValueNumber, KWFrequencyTable*& kwftTarget) const
{
	EqualBinsDiscretizeValues(false, cvSourceValues, ivTargetIndexes, nTargetValueNumber, kwftTarget);
}

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDiscretizerEqualFrequency

KWDiscretizerEqualFrequency::KWDiscretizerEqualFrequency() {}

KWDiscretizerEqualFrequency::~KWDiscretizerEqualFrequency() {}

const ALString KWDiscretizerEqualFrequency::GetName() const
{
	return "EqualFrequency";
}

KWDiscretizer* KWDiscretizerEqualFrequency::Create() const
{
	return new KWDiscretizerEqualFrequency;
}

void KWDiscretizerEqualFrequency::DiscretizeValues(ContinuousVector* cvSourceValues, IntVector* ivTargetIndexes,
						   int nTargetValueNumber, KWFrequencyTable*& kwftTarget) const
{
	EqualBinsDiscretizeValues(true, cvSourceValues, ivTargetIndexes, nTargetValueNumber, kwftTarget);
}

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDiscretizerMODLEqualBins

KWDiscretizerMODLEqualBins::KWDiscretizerMODLEqualBins()
{
	bAreCostPositive = true;
}

KWDiscretizerMODLEqualBins::~KWDiscretizerMODLEqualBins() {}

void KWDiscretizerMODLEqualBins::MODLEqualBinsDiscretizeValues(boolean bIsEqualFrequency,
							       ContinuousVector* cvSourceValues,
							       IntVector* ivTargetIndexes, int nTargetValueNumber,
							       KWFrequencyTable*& kwftTarget) const
{
	boolean bDisplayDiscretizationTables = false;
	boolean bDisplayDiscretizationCosts = false;
	const double dEpsilon = 1e-6;
	IntVector ivContingencyLine;
	int nIntervalValueIndex;
	int nPreviousIntervalValueIndex;
	int nTarget;
	int nValueNumber;
	int nInstanceNumber;
	KWQuantileIntervalBuilder quantileBuilder;
	IntVector ivValueIndexes;
	ObjectArray oaCumulativeTargetFrequencies;
	IntVector* ivCumulativeFrequencies;
	IntVector ivTotalCumulativeFrequencies;
	int nMODLMaxIntervalNumber;
	int nIntervalIndex;
	int nIntervalNumber;
	int nActualIntervalNumber;
	int nPreviousIntervalFrequency;
	int nIntervalFrequency;
	double dDiscretizationCost;
	double dIntervalCost;
	double dBestDiscretizationCost;
	int nBestIntervalNumber;
	int nBestActualIntervalNumber;
	int nBestIntervalSize;

	require(Check());
	require(cvSourceValues != NULL);
	require(ivTargetIndexes != NULL);
	require(nTargetValueNumber >= 0);

	// Si pas d'instance, on renvoie une table de contingence NULL
	kwftTarget = NULL;
	if (cvSourceValues->GetSize() == 0)
		return;

	///////////////////////////////////////////////////////////////////////////
	// Calcul de resultats intermediaires
	// Calcul des cumuls de frequences par modalite cible

	// Initialisation du calculateur de quantiles
	quantileBuilder.InitializeValues(cvSourceValues);
	nValueNumber = quantileBuilder.GetValueNumber();
	nInstanceNumber = cvSourceValues->GetSize();

	// Calcul des vecteurs de frequences cumulees par modalite cible
	ComputeCumulativeTargetFrequencies(&quantileBuilder, ivTargetIndexes, nTargetValueNumber,
					   &oaCumulativeTargetFrequencies);

	////////////////////////////////////////////////////////////////////
	// Recherche du meilleur nombre de partiles
	// La strategie est d'explorer toutes les discretisations en frequences
	// egales en faisant varier le nombre d'intervalles
	// D'un nombre d'intervalle k, on deduit une frequence par intervalles
	// egale a ent(n/k). Il n'y a en fait qu'explorer les frequences entieres
	// distinctes egales a ent(n/1), ent(n/2), ent(n/3), ..., ent(n/n).
	// L'evaluation d'une discretisation en k intervalles est en O(k). En effet,
	// l'utilisation des frequence cible cumulees permet d'evaluer la distribution
	// des frequence cible d'un intervalle par simple difference de deux
	// cumuls, donc en O(1).
	// L'algorithme complet d'evaluation de toutes les discretisation demande alors
	//	n/1 etapes pour les parties de frequence 1,
	//	n/2 etapes pour les parties de frequence 2,
	//	n/3 etapes pour les parties de frequence 3,
	//  ...
	//	n/n etapes pour les partie de frequence n,
	// c'est a dire n*(1/1 + 1/2 + 1/3 + ... + 1/n) etapes en tout.
	// la complexite de recherche est alors en n*log(n)

	// Calcul du nombre max de partiles
	nMODLMaxIntervalNumber = GetMaxIntervalNumber();
	if (nMODLMaxIntervalNumber == 0)
		nMODLMaxIntervalNumber = cvSourceValues->GetSize();

	// Initialisation de la ligne de contingence (variable de travail)
	ivContingencyLine.SetSize(nTargetValueNumber);

	// Evaluation des discretisations pour chaque nombre de partiles
	nBestIntervalNumber = -1;
	nBestIntervalSize = -1;
	nBestActualIntervalNumber = -1;
	nPreviousIntervalFrequency = -1;
	dBestDiscretizationCost = DBL_MAX;
	for (nIntervalNumber = 1; nIntervalNumber <= nMODLMaxIntervalNumber; nIntervalNumber++)
	{
		// Calcul de la taille des partiles: on calcule une nouvelle
		// discretisation uniquement si cette taille a change
		nIntervalFrequency = (int)ceil(cvSourceValues->GetSize() * 1.0 / nIntervalNumber);
		assert(nIntervalFrequency >= 1);
		if (nIntervalFrequency == nPreviousIntervalFrequency)
			continue;
		nPreviousIntervalFrequency = nIntervalFrequency;

		// Entete pour l'affichage d'une table de discretization
		if (bDisplayDiscretizationTables)
			cout << "\n\nIntervalNumber: " << nIntervalNumber << endl;

		// Calcul du nombre effectifs d'intervalles
		// qui peut varier en fonction des individus multiples par valeur source
		if (bIsEqualFrequency)
			nActualIntervalNumber = quantileBuilder.ComputeQuantiles(nIntervalNumber);
		else
			nActualIntervalNumber = quantileBuilder.ComputeEqualWidthQuantiles(nIntervalNumber);

		// Calcul du cout de la discretisation avec la taille des intervalles specifies
		dDiscretizationCost = ComputePartitionCost(nIntervalNumber, nInstanceNumber);
		assert(not bAreCostPositive or dDiscretizationCost >= 0);
		nPreviousIntervalValueIndex = -1;
		for (nIntervalIndex = 0; nIntervalIndex < nActualIntervalNumber; nIntervalIndex++)
		{
			nIntervalValueIndex = quantileBuilder.GetIntervalLastValueIndexAt(nIntervalIndex);

			// Calcul des frequences cible de l'intervalle en cours, par difference
			// des cumul entre la borne sup de l'intervalle et celle du precedent
			for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
			{
				// Acces au vecteur des frequence cumulees
				ivCumulativeFrequencies =
				    cast(IntVector*, oaCumulativeTargetFrequencies.GetAt(nTarget));

				// Calcul de la difference
				ivContingencyLine.SetAt(nTarget, ivCumulativeFrequencies->GetAt(nIntervalValueIndex));
				if (nPreviousIntervalValueIndex >= 0)
					ivContingencyLine.UpgradeAt(
					    nTarget, -ivCumulativeFrequencies->GetAt(nPreviousIntervalValueIndex));
			}
			nPreviousIntervalValueIndex = nIntervalValueIndex;

			// Ajout du cout de l'intervalle
			dIntervalCost = ComputeIntervalCost(&ivContingencyLine);
			assert(not bAreCostPositive or dIntervalCost >= 0);
			dDiscretizationCost += dIntervalCost;

			// Affichage d'une ligne de table de discretisation
			if (bDisplayDiscretizationTables)
			{
				for (nTarget = 0; nTarget < nTargetValueNumber; nTarget++)
					cout << "\t" << ivContingencyLine.GetAt(nTarget);
				cout << endl;
			}

			// Si cout intermediaire deja trop important, on arrete
			// l'evaluation en cours
			if (bAreCostPositive and dDiscretizationCost >= dBestDiscretizationCost)
			{
				// Pas d'optimsaition des calculs si affichage des details
				if (not bDisplayDiscretizationTables and not bDisplayDiscretizationCosts)
					break;
			}
		}

		// Affichage du cout de discretisation
		if (bDisplayDiscretizationCosts)
			cout << "Interval number\t" << nIntervalNumber << "\t" << nActualIntervalNumber << "\t"
			     << nIntervalFrequency << "\t" << ComputePartitionCost(nIntervalNumber, nInstanceNumber)
			     << "\t" << dDiscretizationCost << endl;

		// Test si amelioration du cout de discretisation
		if (dDiscretizationCost < dBestDiscretizationCost - dEpsilon)
		{
			dBestDiscretizationCost = dDiscretizationCost;
			nBestIntervalNumber = nIntervalNumber;
			nBestIntervalSize = nIntervalFrequency;
			nBestActualIntervalNumber = nActualIntervalNumber;
		}

		// Arret de la recherche si autant d'intervalles que de valeurs
		assert(nActualIntervalNumber <= nValueNumber and nActualIntervalNumber <= nIntervalNumber);
		if (nActualIntervalNumber == nValueNumber)
			break;
	}

	// Affichage du cout de la meilleure discretisation
	if (bDisplayDiscretizationCosts)
		cout << "Best interval number\t" << nBestIntervalNumber << "\t" << nBestActualIntervalNumber << "\t"
		     << nBestIntervalSize << "\t" << ComputePartitionCost(nBestIntervalNumber, nInstanceNumber) << "\t"
		     << dBestDiscretizationCost << endl;

	///////////////////////////////////////////////////////////////////////
	// Calcul de la table de contingence correspondant a la meilleure discretization

	// Discretisation
	if (bIsEqualFrequency)
		quantileBuilder.ComputeQuantiles(nBestIntervalNumber);
	else
		quantileBuilder.ComputeEqualWidthQuantiles(nBestIntervalNumber);

	// Creation de la table d'effectifs resultat
	ComputeTargetFrequencyTable(&quantileBuilder, &oaCumulativeTargetFrequencies, kwftTarget);

	// Nettoyage
	oaCumulativeTargetFrequencies.DeleteAll();
}

double KWDiscretizerMODLEqualBins::ComputePartitionCost(int nIntervalNumber, int nValueNumber) const
{
	double dCost;

	require(nIntervalNumber > 0);
	require(nValueNumber > 0);

	// Cout choix entre modele nul et modele informatif
	dCost = log(2.0);

	// Utilisation du codage universel des entiers
	if (nIntervalNumber > 1)
		dCost += KWStat::BoundedNaturalNumbersUniversalCodeLength(nIntervalNumber - 1, nValueNumber - 1);

	return dCost;
}

double KWDiscretizerMODLEqualBins::OldComputePartitionCost(int nIntervalNumber) const
{
	double dCost;

	require(nIntervalNumber > 0);

	// Utilisation du codage universel des entier
	dCost = KWStat::NaturalNumbersUniversalCodeLength(nIntervalNumber);

	return dCost;
}

double KWDiscretizerMODLEqualBins::ComputeIntervalCost(IntVector* ivFrequencyVector) const
{
	double dCost;
	int nClassValueNumber;
	int nFrequency;
	int nIntervalFrequency;
	int i;

	require(ivFrequencyVector != NULL);

	// Cout de codage des instances de la ligne et de la loi multinomiale de la ligne
	dCost = 0;
	nIntervalFrequency = 0;
	nClassValueNumber = ivFrequencyVector->GetSize();
	for (i = 0; i < nClassValueNumber; i++)
	{
		nFrequency = ivFrequencyVector->GetAt(i);
		dCost -= KWStat::LnFactorial(nFrequency);
		nIntervalFrequency += nFrequency;
	}
	dCost += KWStat::LnFactorial(nIntervalFrequency + nClassValueNumber - 1);
	dCost -= KWStat::LnFactorial(nClassValueNumber - 1);
	return dCost;
}

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDiscretizerMODLEqualWidth

KWDiscretizerMODLEqualWidth::KWDiscretizerMODLEqualWidth()
{
	bAreCostPositive = true;
}

KWDiscretizerMODLEqualWidth::~KWDiscretizerMODLEqualWidth() {}

const ALString KWDiscretizerMODLEqualWidth::GetName() const
{
	return "MODLEqualWidth";
}

KWDiscretizer* KWDiscretizerMODLEqualWidth::Create() const
{
	return new KWDiscretizerMODLEqualWidth;
}

void KWDiscretizerMODLEqualWidth::DiscretizeValues(ContinuousVector* cvSourceValues, IntVector* ivTargetIndexes,
						   int nTargetValueNumber, KWFrequencyTable*& kwftTarget) const
{
	MODLEqualBinsDiscretizeValues(false, cvSourceValues, ivTargetIndexes, nTargetValueNumber, kwftTarget);
}

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDiscretizerMODLEqualFrequency

KWDiscretizerMODLEqualFrequency::KWDiscretizerMODLEqualFrequency()
{
	bAreCostPositive = true;
}

KWDiscretizerMODLEqualFrequency::~KWDiscretizerMODLEqualFrequency() {}

const ALString KWDiscretizerMODLEqualFrequency::GetName() const
{
	return "MODLEqualFrequency";
}

KWDiscretizer* KWDiscretizerMODLEqualFrequency::Create() const
{
	return new KWDiscretizerMODLEqualFrequency;
}

void KWDiscretizerMODLEqualFrequency::DiscretizeValues(ContinuousVector* cvSourceValues, IntVector* ivTargetIndexes,
						       int nTargetValueNumber, KWFrequencyTable*& kwftTarget) const
{
	MODLEqualBinsDiscretizeValues(true, cvSourceValues, ivTargetIndexes, nTargetValueNumber, kwftTarget);
}