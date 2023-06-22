// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "DTDiscretizerMODL.h"
#include "DTUnivariatePartitionCost.h"
#include "DTConfig.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe DTDiscretizerMODL

DTDiscretizerMODL::DTDiscretizerMODL()
{
	nMergeNumber = 0;
	nExtraMergeNumber = 0;
	nSplitNumber = 0;
	nExtraSplitNumber = 0;
	nMergeSplitNumber = 0;
	nMergeMergeSplitNumber = 0;
	delete discretizationCosts;
	discretizationCosts = new DTMODLDiscretizationCosts;
	dEpsilon = 1e-6;
	nnew = 1;
	ntest = 0;
}

DTDiscretizerMODL::~DTDiscretizerMODL()
{
	ALString sTmp;

	// delete discretizationCosts;
}

const ALString DTDiscretizerMODL::GetName() const
{
	return "DTMODL";
}

KWDiscretizer* DTDiscretizerMODL::Create() const
{
	return new DTDiscretizerMODL;
}

void DTDiscretizerMODL::Discretize(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget) const
{
	if (ntest == 1)
	{
		cout << "avant \nkwftSource Granuilarity : " << kwftSource->GetGranularity() << endl;
		kwftSource->Write(std::cout);
	}

	// Discretisation de la table granularisee
	if (nnew == 1)
		DiscretizeNEW(kwftSource, kwftTarget);
	else
		DiscretizeOLD(kwftSource, kwftTarget);
	if (ntest == 1)
	{
		cout << "apres \nkwftSource Granuilarity : " << kwftSource->GetGranularity() << endl;
		kwftSource->Write(std::cout);
		cout << "kwftTarget Granuilarity : " << kwftTarget->GetGranularity() << endl;
		kwftTarget->Write(std::cout);
	}
}

void DTDiscretizerMODL::DiscretizeNEW(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget) const
{
	KWFrequencyTable* kwftGranularizedTable;
	// KWFrequencyTable* kwftMergedTable;
	KWFrequencyTable* kwftDiscretizedGranularizedTable;
	double dCost;
	double dBestCost;
	int nGranularity;
	int nGranularityMax;
	boolean bIsLastGranularity;
	int nInstanceNumber;
	boolean bDisplayResults = false;
	boolean bDisplayResults2 = false;
	KWQuantileIntervalBuilder quantileBuilder;
	IntVector ivInputFrequencies;
	int nSourceIndex;
	boolean bIsGranularitySelected;
	int nCurrentPartileNumber;
	int nPreviousPartileNumber;
	IntVector ivGranularityMinByValues;
	IntVector ivGranularizedValueNumber;
	// IntVector ivGarbageModalityNumber;

	double dRequiredIncreasingCoefficient = 1.5;

	StartTimer(DTTimerDiscretize);
	dBestCost = DBL_MAX;
	kwftTarget = NULL;
	nInstanceNumber = kwftSource->GetTotalFrequency();
	nCurrentPartileNumber = 0;
	kwftGranularizedTable = NULL;
	kwftDiscretizedGranularizedTable = NULL;

	// cout << "*****   NEW VAR" << endl;
	// cout << "*****   " << kwftSource->GetTotalFrequency() << endl;
	//  Cas d'une base vide
	if (nInstanceNumber == 0 or kwftSource->GetFrequencyVectorNumber() == 1)
		kwftTarget = kwftSource->Clone();
	// Sinon
	else
	{
		// Granularite max
		nGranularityMax = (int)ceil(log(nInstanceNumber * 1.0) / log(2.0));
		bIsLastGranularity = false;
		nPreviousPartileNumber = 0;

		// On analyse toutes les granularite de 1 a Max
		if (bDisplayResults2)
		{
			cout << "GranulariteMax = " << nGranularityMax << endl;
		}
		ivGranularityMinByValues.SetSize(kwftSource->GetFrequencyVectorNumber());
		ivGranularizedValueNumber.SetSize(kwftSource->GetFrequencyVectorNumber());
		nGranularity = 1;

		// Initialisation du vecteur d'effectifs de la table
		for (nSourceIndex = 0; nSourceIndex < kwftSource->GetFrequencyVectorNumber(); nSourceIndex++)
		{
			ivInputFrequencies.Add(kwftSource->GetFrequencyVectorAt(nSourceIndex)->ComputeTotalFrequency());
			ivGranularityMinByValues.SetAt(nSourceIndex, 0);
			ivGranularizedValueNumber.SetAt(nSourceIndex, kwftSource->GetGranularizedValueNumber());
		}

		// Initialisation du quantileBuilder utilise pour chaque granularisation
		quantileBuilder.InitializeFrequencies(&ivInputFrequencies);

		// Nettoyage
		ivInputFrequencies.SetSize(0);

		if (bDisplayResults2)
		{
			cout << "kwftSource : " << endl;
			kwftSource->Write(std::cout);
		}
		while (nGranularity <= nGranularityMax and not bIsLastGranularity)
		{
			// Arret si interruption utilisateur
			if (TaskProgression::IsInterruptionRequested())
			{
				if (kwftTarget != NULL)
					delete kwftTarget;
				kwftTarget = new KWFrequencyTable;
				kwftTarget->ComputeNullTable(kwftSource);
				break;
			}

			// Calcul de la table de contingence associee a la granularite a partir de kwctSource
			GranularizeFrequencyTable(kwftSource, kwftGranularizedTable, nGranularity, &quantileBuilder);

			// Extraction du nombre de partiles de la table a la granularite courante
			nCurrentPartileNumber = kwftGranularizedTable->GetFrequencyVectorNumber();

			// Test s'il s'agit de la derniere granularite a traiter (si le nombre de parties est maximal)
			bIsLastGranularity = (kwftSource->GetFrequencyVectorNumber() == nCurrentPartileNumber);

			// Test d'une granularite eligible
			// Le nombre de partiles pour la granularite courante est il :
			// - superieur d'au moins un facteur dRequiredIncreasingCoefficient au nombre de partiles pour
			// la derniere granularite traitee ET
			// - inferieur d'au moins un facteur dRequiredIncreasingCoefficient au nombre de partiles de la
			// granularite max
			bIsGranularitySelected =
			    (nCurrentPartileNumber >= dRequiredIncreasingCoefficient * nPreviousPartileNumber) and
			    (nCurrentPartileNumber * dRequiredIncreasingCoefficient <=
			     kwftSource->GetFrequencyVectorNumber());

			if (bDisplayResults)
			{
				cout << "Granularite = " << nGranularity << endl;
				cout << "Nbre valeurs explicatives table source\t"
				     << kwftSource->GetFrequencyVectorNumber() << "\tTaille table granularisee\t "
				     << kwftGranularizedTable->GetFrequencyVectorNumber() << endl;
			}

			// Cas du traitement de la granularite courante
			if (bIsGranularitySelected or bIsLastGranularity)
			{
				// Calcul de la table de contingence issue de la fusion des intervalles purs
				// MergeFrequencyTablePureIntervals(kwftGranularizedTable, kwftMergedTable);

				// if (bDisplayResults)
				//{
				//	cout << "Granularite = " << nGranularity << endl;
				//	cout << "Nbre valeurs explicatives table source\t" <<
				// kwftSource->GetFrequencyVectorNumber()
				//	     << "\tTaille table granularisee\t " <<
				// kwftGranularizedTable->GetFrequencyVectorNumber()
				//	     << "\tTable granularise\t" << *kwftGranularizedTable
				//	     << "\tTaille table apres fusion valeurs pures\t" <<
				// kwftMergedTable->GetFrequencyVectorNumber() << endl;
				// }

				// Initialisation ivGranularityMinByValues
				for (nSourceIndex = 0; nSourceIndex < quantileBuilder.GetIntervalNumber();
				     nSourceIndex++)
				{

					if (ivGranularityMinByValues.GetAt(
						quantileBuilder.GetIntervalLastValueIndexAt(nSourceIndex)) == 0)
					{
						ivGranularityMinByValues.SetAt(
						    quantileBuilder.GetIntervalLastValueIndexAt(nSourceIndex),
						    nGranularity);
						ivGranularizedValueNumber.SetAt(
						    quantileBuilder.GetIntervalLastValueIndexAt(nSourceIndex),
						    kwftGranularizedTable->GetGranularizedValueNumber());
					}
				}

				// Nettoyage
				// delete kwftGranularizedTable;
				// kwftGranularizedTable = NULL;
				// TIME NV
				if (bDisplayResults2)
				{
					cout << "Granularite = " << nGranularity << endl;
					cout << "Nbre valeurs explicatives table source\t"
					     << kwftSource->GetFrequencyVectorNumber()
					     << "\tTaille table apres fusion valeurs pures\t"
					     << kwftGranularizedTable->GetFrequencyVectorNumber() << endl;
					cout << "kwftGranularizedTable : " << endl;
					kwftGranularizedTable->Write(std::cout);
					for (int i = 0; i < kwftGranularizedTable->GetFrequencyVectorNumber(); i++)
					{
						cout << "taille : " << i << " / "
						     << kwftGranularizedTable->GetFrequencyVectorAt(i)
							    ->ComputeTotalFrequency()
						     << endl;
					}
					cout << "quantileBuilder.GetIntervalNumber = "
					     << quantileBuilder.GetIntervalNumber() << endl;
					cout << "quantileBuilder.GetInstanceNumber = "
					     << quantileBuilder.GetInstanceNumber() << endl;
					for (int i = 0; i < quantileBuilder.GetIntervalNumber(); i++)
					{

						cout << "partile : " << i << " / "
						     << quantileBuilder.GetIntervalQuantileIndexAt(i) << " / "
						     << quantileBuilder.GetIntervalFirstValueIndexAt(i) << " / "
						     << quantileBuilder.GetIntervalLastValueIndexAt(i) << endl;
					}
				}
			}

			if (!bIsLastGranularity)
			{
				delete kwftGranularizedTable;
				kwftGranularizedTable = NULL;
			}
			// Passage a la granularite suivante
			nPreviousPartileNumber = nCurrentPartileNumber;
			nGranularity++;
		}
		// libere la memoire si il n y a pas d interuption utilisateur
		if (not TaskProgression::IsInterruptionRequested())
		{
			StartTimer(DTTimerDiscretizeGFT);
			if (bDisplayResults2)
			{
				for (nSourceIndex = 0; nSourceIndex < kwftSource->GetFrequencyVectorNumber();
				     nSourceIndex++)
				{
					cout << "ivGranularityMinByValues : " << nSourceIndex << " / "
					     << ivGranularityMinByValues.GetAt(nSourceIndex) << endl;
				}
			}

			for (nSourceIndex = 0; nSourceIndex < ivGranularityMinByValues.GetSize(); nSourceIndex++)
			{

				if (ivGranularityMinByValues.GetAt(nSourceIndex) == 0)
				{
					ivGranularityMinByValues.SetAt(nSourceIndex, nGranularityMax);
					ivGranularizedValueNumber.SetAt(nSourceIndex,
									kwftSource->GetFrequencyVectorNumber());
				}
			}
			// Discretisation de la table granularisee
			DiscretizeGranularizedFrequencyTableNEW(kwftGranularizedTable, kwftDiscretizedGranularizedTable,
								&ivGranularityMinByValues, &ivGranularizedValueNumber);

			// Memorisation du meilleur cout avec ComputeDiscretizationCost et de la granularite associee
			dCost = ComputeDiscretizationCost(kwftDiscretizedGranularizedTable);

			StopTimer(DTTimerDiscretizeGFT);
			// SecondsToString(timerDiscretizeGFT.GetElapsedTime()));
			//  Nettoyage
			// delete kwftMergedTable;
			// kwftMergedTable = NULL;

			if (dCost < dBestCost)
			{
				dBestCost = dCost;
				// Destruction de l'optimum precedent
				if (kwftTarget != NULL)
					delete kwftTarget;
				// Memorisation du nouvel optimum
				kwftTarget = new KWFrequencyTable;
				kwftTarget->CopyFrom(kwftDiscretizedGranularizedTable);
			}
			if (bDisplayResults)
			{
				cout << "Granularite\tConstr. cost\tPrep. cost\tData cost\tTotal cost\tGroups" << endl;
				cout << nGranularity << "\t"
				     << ComputeDiscretizationConstructionCost(kwftDiscretizedGranularizedTable) << "\t"
				     << ComputeDiscretizationPreparationCost(kwftDiscretizedGranularizedTable) << "\t"
				     << ComputeDiscretizationDataCost(kwftDiscretizedGranularizedTable) << "\t"
				     << ComputeDiscretizationCost(kwftDiscretizedGranularizedTable) << "\t"
				     << kwftDiscretizedGranularizedTable->GetFrequencyVectorNumber() << endl;
			}

			// Nettoyage
			delete kwftGranularizedTable;
			kwftGranularizedTable = NULL;
			delete kwftDiscretizedGranularizedTable;
			kwftDiscretizedGranularizedTable = NULL;
		}

		if (bDisplayResults)
			cout << "Meilleure granularite discretisation " << kwftTarget->GetGranularity() << " sur  "
			     << nGranularityMax << endl;
	}
	ensure(kwftTarget != NULL);
	StopTimer(DTTimerDiscretize);
	// if (kwftTarget->GetGranularity() == 1 && kwftTarget->GetFrequencyVectorNumber() == 1)
	//	kwftTarget->SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
	if (ntest == 1)
	{
		cout << "kwftTarget->GetFrequencyVectorNumber()" << kwftTarget->GetFrequencyVectorNumber() << endl;
		cout << "dBestCost : " << dBestCost << endl;
		cout << "Granularity : " << kwftTarget->GetGranularity() << endl;
		cout << "GranularizedValueNumber : " << kwftTarget->GetGranularizedValueNumber() << endl;
		cout << "GarbageModalityNumber : " << kwftTarget->GetGarbageModalityNumber() << endl;
	}
}

void DTDiscretizerMODL::DiscretizeOLD(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget) const
{
	KWFrequencyTable* kwftGranularizedTable;
	KWFrequencyTable* kwftMergedTable;
	KWFrequencyTable* kwftDiscretizedGranularizedTable;
	double dCost;
	double dBestCost;
	int nGranularity;
	int nGranularityMax;
	boolean bIsLastGranularity;
	int nInstanceNumber;
	boolean bDisplayResults = false;
	KWQuantileIntervalBuilder quantileBuilder;
	IntVector ivInputFrequencies;
	int nSourceIndex;
	boolean bIsGranularitySelected;
	int nCurrentPartileNumber;
	int nPreviousPartileNumber;

	double dRequiredIncreasingCoefficient = 1.5;

	StartTimer(DTTimerDiscretize);
	dBestCost = DBL_MAX;
	kwftTarget = NULL;
	nInstanceNumber = kwftSource->GetTotalFrequency();
	nCurrentPartileNumber = 0;
	kwftGranularizedTable = NULL;
	kwftDiscretizedGranularizedTable = NULL;

	// cout << "*****   NEW VAR" << endl;
	// cout << "*****   " << kwftSource->GetTotalFrequency() << endl;
	//  Cas d'une base vide
	if (nInstanceNumber == 0 or kwftSource->GetFrequencyVectorNumber() == 1)
		kwftTarget = kwftSource->Clone();
	// Sinon
	else
	{
		// Granularite max
		nGranularityMax = (int)ceil(log(nInstanceNumber * 1.0) / log(2.0));
		bIsLastGranularity = false;
		nPreviousPartileNumber = 0;

		// On analyse toutes les granularite de 1 a Max
		nGranularity = 1;

		// Initialisation du vecteur d'effectifs de la table
		for (nSourceIndex = 0; nSourceIndex < kwftSource->GetFrequencyVectorNumber(); nSourceIndex++)
			ivInputFrequencies.Add(kwftSource->GetFrequencyVectorAt(nSourceIndex)->ComputeTotalFrequency());

		// Initialisation du quantileBuilder utilise pour chaque granularisation
		quantileBuilder.InitializeFrequencies(&ivInputFrequencies);

		// Nettoyage
		ivInputFrequencies.SetSize(0);

		while (nGranularity <= nGranularityMax and not bIsLastGranularity)
		{
			// Arret si interruption utilisateur
			if (TaskProgression::IsInterruptionRequested())
			{
				if (kwftTarget != NULL)
					delete kwftTarget;
				kwftTarget = new KWFrequencyTable;
				kwftTarget->ComputeNullTable(kwftSource);
				break;
			}

			// Calcul de la table de contingence associee a la granularite a partir de kwctSource
			GranularizeFrequencyTable(kwftSource, kwftGranularizedTable, nGranularity, &quantileBuilder);

			// Extraction du nombre de partiles de la table a la granularite courante
			nCurrentPartileNumber = kwftGranularizedTable->GetFrequencyVectorNumber();

			// Test s'il s'agit de la derniere granularite a traiter (si le nombre de parties est maximal)
			bIsLastGranularity = (kwftSource->GetFrequencyVectorNumber() == nCurrentPartileNumber);

			// Test d'une granularite eligible
			// Le nombre de partiles pour la granularite courante est il :
			// - superieur d'au moins un facteur dRequiredIncreasingCoefficient au nombre de partiles pour
			// la derniere granularite traitee ET
			// - inferieur d'au moins un facteur dRequiredIncreasingCoefficient au nombre de partiles de la
			// granularite max
			bIsGranularitySelected =
			    (nCurrentPartileNumber >= dRequiredIncreasingCoefficient * nPreviousPartileNumber) and
			    (nCurrentPartileNumber * dRequiredIncreasingCoefficient <=
			     kwftSource->GetFrequencyVectorNumber());

			if (bDisplayResults)
			{
				cout << "Granularite = " << nGranularity << endl;
				cout << "Nbre valeurs explicatives table source\t"
				     << kwftSource->GetFrequencyVectorNumber() << "\tTaille table granularisee\t "
				     << kwftGranularizedTable->GetFrequencyVectorNumber() << endl;
			}

			// Cas du traitement de la granularite courante
			if (bIsGranularitySelected or bIsLastGranularity)
			{
				// Calcul de la table de contingence issue de la fusion des intervalles purs
				MergeFrequencyTablePureIntervals(kwftGranularizedTable, kwftMergedTable);

				if (bDisplayResults)
				{
					cout << "Granularite = " << nGranularity << endl;
					cout << "Nbre valeurs explicatives table source\t"
					     << kwftSource->GetFrequencyVectorNumber()
					     << "\tTaille table granularisee\t "
					     << kwftGranularizedTable->GetFrequencyVectorNumber()
					     << "\tTable granularise\t" << *kwftGranularizedTable
					     << "\tTaille table apres fusion valeurs pures\t"
					     << kwftMergedTable->GetFrequencyVectorNumber() << endl;
				}

				// Nettoyage
				delete kwftGranularizedTable;
				kwftGranularizedTable = NULL;
				// TIME NV

				StartTimer(DTTimerDiscretizeGFT);

				// Discretisation de la table granularisee
				DiscretizeGranularizedFrequencyTable(kwftMergedTable, kwftDiscretizedGranularizedTable);

				// Memorisation du meilleur cout avec ComputeDiscretizationCost et de la granularite
				// associee
				dCost = ComputeDiscretizationCost(kwftDiscretizedGranularizedTable);

				StopTimer(DTTimerDiscretizeGFT);
				// SecondsToString(timerDiscretizeGFT.GetElapsedTime()));
				//  Nettoyage
				delete kwftMergedTable;
				kwftMergedTable = NULL;

				if (dCost < dBestCost)
				{
					dBestCost = dCost;
					// Destruction de l'optimum precedent
					if (kwftTarget != NULL)
						delete kwftTarget;
					// Memorisation du nouvel optimum
					kwftTarget = new KWFrequencyTable;
					kwftTarget->CopyFrom(kwftDiscretizedGranularizedTable);
				}
				if (bDisplayResults)
				{
					cout << "Granularite\tConstr. cost\tPrep. cost\tData cost\tTotal cost\tGroups"
					     << endl;
					cout << nGranularity << "\t"
					     << ComputeDiscretizationConstructionCost(kwftDiscretizedGranularizedTable)
					     << "\t"
					     << ComputeDiscretizationPreparationCost(kwftDiscretizedGranularizedTable)
					     << "\t" << ComputeDiscretizationDataCost(kwftDiscretizedGranularizedTable)
					     << "\t" << ComputeDiscretizationCost(kwftDiscretizedGranularizedTable)
					     << "\t" << kwftDiscretizedGranularizedTable->GetFrequencyVectorNumber()
					     << endl;
				}
				// Nettoyage
				delete kwftDiscretizedGranularizedTable;
				kwftDiscretizedGranularizedTable = NULL;
			}
			// Cas ou l'on n'etudie pas cette granularite associee a la meme table que celle etudie
			// precedemment
			else
			{
				delete kwftGranularizedTable;
				kwftGranularizedTable = NULL;

				if (bDisplayResults)
				{
					cout << "Granularite\t" << nGranularity
					     << "\t non traitee car identique a la precedente" << endl;
				}
			}

			// Passage a la granularite suivante
			nPreviousPartileNumber = nCurrentPartileNumber;
			nGranularity++;
		}

		if (bDisplayResults)
			cout << "Meilleure granularite discretisation " << kwftTarget->GetGranularity() << " sur  "
			     << nGranularityMax << endl;
	}
	ensure(kwftTarget != NULL);
	StopTimer(DTTimerDiscretize);

	cout << "kwftTarget->GetFrequencyVectorNumber()" << kwftTarget->GetFrequencyVectorNumber() << endl;
	cout << "dBestCost : " << dBestCost << endl;
	cout << "Granularity : " << kwftTarget->GetGranularity() << endl;
	cout << "GranularizedValueNumber : " << kwftTarget->GetGranularizedValueNumber() << endl;
	cout << "GarbageModalityNumber : " << kwftTarget->GetGarbageModalityNumber() << endl;
}

void DTDiscretizerMODL::DiscretizeGranularizedFrequencyTableNEW(KWFrequencyTable* kwftSource,
								KWFrequencyTable*& kwftTarget,
								IntVector* ivGranularityValues,
								IntVector* ivGranularizedValueNumber) const
{
	KWFrequencyTable kwftbin;
	KWFrequencyTable* kwftnull;
	KWDenseFrequencyVector *kwdfvSourceFrequencyVectornull, *kwdfvSourceFrequencyVectorcurent;
	KWDenseFrequencyVector *kwdfvSourceFrequencyVector0, *kwdfvSourceFrequencyVector1;
	KWDenseFrequencyVector *kwdfvSourceFrequencyVectortarget0, *kwdfvSourceFrequencyVectortarget1;
	int nvalue, nTargetValueNumber;
	int nfv, nsum, nsourcesize;
	double dCost, dCostnull;
	double dBestCost = 0;
	int nvfmax = -1;

	require(Check());
	require(kwftSource != NULL);

	nTargetValueNumber = kwftSource->GetFrequencyVectorSize();
	nsourcesize = kwftSource->GetFrequencyVectorNumber();

	// Cas particulier ou il n'y a qu'une valeur source
	if (kwftSource->GetFrequencyVectorNumber() <= 1)
	{
		kwftTarget = kwftSource->Clone();
	}
	// Sinon, discretization
	else
	{
		// initilisation
		kwftnull = new KWFrequencyTable;
		kwftnull->ComputeNullTable(kwftSource);
		kwftnull->SetGranularity(1);
		kwftnull->SetGranularizedValueNumber(2);
		kwdfvSourceFrequencyVectornull = cast(KWDenseFrequencyVector*, kwftnull->GetFrequencyVectorAt(0));
		// Initialisation de la taille des resultats binaire
		kwftbin.Initialize(2);
		kwdfvSourceFrequencyVector0 = cast(KWDenseFrequencyVector*, kwftbin.GetFrequencyVectorAt(0));
		kwdfvSourceFrequencyVector0->GetFrequencyVector()->SetSize(nTargetValueNumber);
		kwdfvSourceFrequencyVector1 = cast(KWDenseFrequencyVector*, kwftbin.GetFrequencyVectorAt(1));
		kwdfvSourceFrequencyVector1->GetFrequencyVector()->SetSize(nTargetValueNumber);
		kwdfvSourceFrequencyVector1->GetFrequencyVector()->CopyFrom(
		    kwdfvSourceFrequencyVectornull->GetFrequencyVector());
		for (nvalue = 0; nvalue < nTargetValueNumber; nvalue++)
		{
			kwdfvSourceFrequencyVector0->GetFrequencyVector()->SetAt(nvalue, 0);
		}
		// kwftnull.Write(cout);

		dCostnull = ComputeDiscretizationCost(kwftnull);
		// cout << "cout null : " << dCostnull << endl;

		// Construction de la table finale a partir de la liste d'intervalles
		// kwftTarget = BuildFrequencyTableFromIntervalList(headInterval);
		// Parametrisation du nombre de valeurs
		kwftbin.SetInitialValueNumber(kwftSource->GetInitialValueNumber());
		kwftbin.SetGranularizedValueNumber(kwftSource->GetGranularizedValueNumber());
		// Parametrisation de la granularite et de la poubelle
		kwftbin.SetGranularity(kwftSource->GetGranularity());
		kwftbin.SetGarbageModalityNumber(kwftSource->GetGarbageModalityNumber());
		// dBestCost = ComputeDiscretizationCost(&kwftbin);
		// dBestCost = dCostnull;
		kwftTarget = kwftbin.Clone();
		kwdfvSourceFrequencyVectortarget0 = cast(KWDenseFrequencyVector*, kwftTarget->GetFrequencyVectorAt(0));
		kwdfvSourceFrequencyVectortarget1 = cast(KWDenseFrequencyVector*, kwftTarget->GetFrequencyVectorAt(1));
		// cout << "cout best 0 : " << dBestCost << endl;
		// kwftTarget->Write(cout);
		for (nfv = 0; nfv < nsourcesize - 1; nfv++)
		{
			kwdfvSourceFrequencyVectorcurent =
			    cast(KWDenseFrequencyVector*, kwftSource->GetFrequencyVectorAt(nfv));

			////////// calcul de la nouvelle FrequencyVector
			for (nvalue = 0; nvalue < nTargetValueNumber; nvalue++)
			{
				nsum = kwdfvSourceFrequencyVector0->GetFrequencyVector()->GetAt(nvalue) +
				       kwdfvSourceFrequencyVectorcurent->GetFrequencyVector()->GetAt(nvalue);
				kwdfvSourceFrequencyVector0->GetFrequencyVector()->SetAt(nvalue, nsum);
				nsum = kwdfvSourceFrequencyVector1->GetFrequencyVector()->GetAt(nvalue) -
				       kwdfvSourceFrequencyVectorcurent->GetFrequencyVector()->GetAt(nvalue);
				kwdfvSourceFrequencyVector1->GetFrequencyVector()->SetAt(nvalue, nsum);
			}

			/// Calcul du cout de la binerisation courentes
			// dCost = ComputeDiscretizationCost(&kwftbin);
			// cout << "cost sans granunalarity : " << dCost << " \ " << kwftbin.GetGranularity() << endl;
			kwftbin.SetGranularity(ivGranularityValues->GetAt(nfv));

			dCost = ComputeDiscretizationCost(&kwftbin);
			// cout << "cost avec granunalarity : " << dCost << " \ " << kwftbin.GetGranularity() << endl;

			// si plus petit alors affectation de la cible
			if (nfv == 0)
			{
				dBestCost = dCost;
				// kwftTarget->Write(cout);
				kwdfvSourceFrequencyVectortarget0->CopyFrom(kwdfvSourceFrequencyVector0);
				kwdfvSourceFrequencyVectortarget1->CopyFrom(kwdfvSourceFrequencyVector1);
				nvfmax = nfv;
			}
			if (dCost < dBestCost)
			{
				// cout << "cout best " << nfv + 1 << " : " << dCost << endl;
				dBestCost = dCost;
				// kwftTarget->Write(cout);
				kwdfvSourceFrequencyVectortarget0->CopyFrom(kwdfvSourceFrequencyVector0);
				kwdfvSourceFrequencyVectortarget1->CopyFrom(kwdfvSourceFrequencyVector1);
				nvfmax = nfv;
			}
		}

		// cout << "nvfmax / nsourcesize : " << nvfmax << " / " << nsourcesize << endl;

		if (dCostnull < dBestCost)
		{
			// cout << "NULL Cost" << endl;
			// cout << "cout best " << nfv + 1 << " : " << dCost << endl;
			delete kwftTarget;
			// kwftTarget->Write(cout);
			kwftTarget = kwftnull;
		}
		else
		{

			delete kwftnull;
			// si plus petit alors affectation de la cible
			kwftTarget->SetGranularity(ivGranularityValues->GetAt(nvfmax));
			kwftTarget->SetGranularizedValueNumber(ivGranularizedValueNumber->GetAt(nvfmax));
		}
		// cout << "dCostnull : " << dCostnull << endl;
		// return;
		//  Nettoyage
		// DeleteIntervalList(headInterval);
	}

	// cout << "*********** NVF MAX = " << nvfmax << endl;
	ensure(kwftSource->GetFrequencyVectorSize() == kwftTarget->GetFrequencyVectorSize());
	ensure(kwftSource->GetFrequencyVectorNumber() >= kwftTarget->GetFrequencyVectorNumber());
	ensure(kwftSource->GetTotalFrequency() == kwftTarget->GetTotalFrequency());
}