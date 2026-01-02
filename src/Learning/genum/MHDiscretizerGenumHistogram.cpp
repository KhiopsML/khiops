// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHDiscretizerGenumHistogram.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDiscretizerEqualWidth

MHDiscretizerGenumHistogram::MHDiscretizerGenumHistogram()
{
	// Specialisation des cout de discretisation pour les histogrammes
	SetDiscretizationCosts(new MHGenumHistogramCosts);
	bIsTraceOpened = false;
	nTraceBlockLevel = 0;
	bIsTraceBlockStart = false;
}

MHDiscretizerGenumHistogram::~MHDiscretizerGenumHistogram()
{
	assert(not bIsTraceOpened);
	assert(nTraceBlockLevel == 0);
	assert(not bIsTraceBlockStart);
}

const ALString MHDiscretizerGenumHistogram::GetName() const
{
	return "Histogram Genum";
}

KWDiscretizer* MHDiscretizerGenumHistogram::Create() const
{
	return new MHDiscretizerGenumHistogram;
}

boolean MHDiscretizerGenumHistogram::IsUsingSourceValues() const
{
	return true;
}

void MHDiscretizerGenumHistogram::DiscretizeValues(ContinuousVector* cvSourceValues, IntVector* ivTargetIndexes,
						   int nTargetValueNumber, KWFrequencyTable*& kwftTarget,
						   ContinuousVector*& cvBounds) const
{
	MHHistogram* optimizedHistogram;
	int n;
	int nMissingValueNumber;
	ContinuousVector cvActualValues;
	double dEpsilonBinNumber;

	// Comptage des valeurs manquantes
	nMissingValueNumber = 0;
	for (n = 0; n < cvSourceValues->GetSize(); n++)
	{
		if (cvSourceValues->GetAt(n) == KWContinuous::GetMissingValue())
			nMissingValueNumber++;
	}

	// Extraction des valeurs presentes
	for (n = nMissingValueNumber; n < cvSourceValues->GetSize(); n++)
		cvActualValues.Add(cvSourceValues->GetAt(n));

	// Parametrage de la fonction de cout
	if (GetHistogramSpec()->GetHistogramCriterion() == "G-Enum")
		cast(MHDiscretizerGenumHistogram*, this)->SetDiscretizationCosts(new MHGenumHistogramCosts);
	else if (GetHistogramSpec()->GetHistogramCriterion() == "Enum")
		cast(MHDiscretizerGenumHistogram*, this)->SetDiscretizationCosts(new MHEnumHistogramCosts);
	else if (GetHistogramSpec()->GetHistogramCriterion() == "KM")
		cast(MHDiscretizerGenumHistogram*, this)->SetDiscretizationCosts(new MHKMHistogramCosts);

	// Parametrage du nombre de bin dans le cas de la methode Enum ou KM
	if (GetHistogramSpec()->GetHistogramCriterion() == "KM" or
	    GetHistogramSpec()->GetHistogramCriterion() == "Enum")
	{
		// On se base sur autant de bin qu'il y en a dans l'etendues des valeurs, plus un pour les extremites
		if (GetHistogramSpec()->GetEpsilonBinWidth() > 0)
		{
			dEpsilonBinNumber = GetHistogramSpec()->GetMaxEpsilonBinNumber();
			if (cvActualValues.GetSize() > 0)
				dEpsilonBinNumber =
				    1 + (cvActualValues.GetAt(cvActualValues.GetSize() - 1) - cvActualValues.GetAt(0)) /
					    GetHistogramSpec()->GetEpsilonBinWidth();
			if (dEpsilonBinNumber > GetHistogramSpec()->GetMaxEpsilonBinNumber())
				dEpsilonBinNumber = GetHistogramSpec()->GetMaxEpsilonBinNumber();
			GetHistogramSpec()->SetEpsilonBinNumber((int)floor(dEpsilonBinNumber + 0.5));
		}
	}

	// Appel de la methode principale de pilotage de la discretisation
	MainDiscretizeValues(&cvActualValues, optimizedHistogram);

	// Exploitation de l'histogramme
	ExploitHistogram(optimizedHistogram);

	// Transformation en une table de contingence non supervisee
	PreparedCleanedStandardFrequencyTable(optimizedHistogram, nMissingValueNumber, kwftTarget, cvBounds);

	// Nettoyage
	delete optimizedHistogram;
}

MHGenumHistogramSpec MHDiscretizerGenumHistogram::histogramSpec;

MHGenumHistogramSpec* MHDiscretizerGenumHistogram::GetHistogramSpec()
{
	return &histogramSpec;
}

void MHDiscretizerGenumHistogram::ComputeHistogramFromBins(const ContinuousVector* cvSourceValues,
							   MHHistogram*& optimizedHistogram) const
{
	require(cvSourceValues != NULL);
	require(cvSourceValues->GetSize() > 0);

	// Creation du repertoire en sortie si necessaire
	if (GetHistogramSpec()->GetResultFilesDirectory() != "")
		FileService::MakeDirectories(GetHistogramSpec()->GetResultFilesDirectory());

	// Calcul des histogrammes
	MainDiscretizeValues(cvSourceValues, optimizedHistogram);
}

KWFrequencyTable* MHDiscretizerGenumHistogram::NewFrequencyTable() const
{
	KWFrequencyTable* frequencyTable;

	// On renvoie une table de frequence parametree pour la creation de MHGenumHistogramVector
	frequencyTable = new KWFrequencyTable;
	frequencyTable->SetFrequencyVectorCreator(GetFrequencyVectorCreator()->Clone());
	return frequencyTable;
}

void MHDiscretizerGenumHistogram::MainDiscretizeValues(const ContinuousVector* cvSourceValues,
						       MHHistogram*& optimizedHistogram) const
{
	KWFrequencyTable* optimizedHistogramFrequencyTable;
	Timer timer;
	ALString sTmp;

	require(cvSourceValues != NULL);

	TraceOpen();

	// Pilotage de la discretisation, avec ou sans gestion des outliers
	Trace(sTmp + "Input dataset size\t" + IntToString(cvSourceValues->GetSize()));
	timer.Start();
	if (GetHistogramSpec()->GetOutlierManagementHeuristic())
		OutlierDiscretizeValues(cvSourceValues, optimizedHistogram);
	else
	{
		// On passe par un histogramme algorithmique pour l'optimisation
		TypicalDiscretizeValues(cvSourceValues, optimizedHistogramFrequencyTable);

		// On transforme le resultat en histogramme
		optimizedHistogram = new MHGenumHistogram;
		BuildOutputHistogram(cvSourceValues, 0, cvSourceValues->GetSize(), optimizedHistogramFrequencyTable,
				     optimizedHistogram);

		// Nettoyage
		delete optimizedHistogramFrequencyTable;
	}
	timer.Stop();
	optimizedHistogram->SetComputationTime(timer.GetElapsedTime());
	TraceClose();
}

void MHDiscretizerGenumHistogram::OutlierDiscretizeValues(const ContinuousVector* cvSourceValues,
							  MHHistogram*& optimizedHistogram) const
{
	boolean bStudyPWCHThresold = false;
	boolean bIsPWCH;
	KWFrequencyTable* optimizedHistogramFrequencyTable;
	KWFrequencyTable* optimizedLogTrHistogramFrequencyTable;
	ObjectArray oaDataSubsets;
	double dTotalBinNumber;
	int nTotalBinNumber;
	ALString sTmp;

	require(cvSourceValues != NULL);
	require(GetHistogramSpec()->GetOutlierManagementHeuristic());

	// Boucle de test de des tailles differentes d'histogramme pour le critere PWCH
	if (bStudyPWCHThresold)
	{
		Trace(sTmp + "PWCH threshold study");
		nTotalBinNumber = GetHistogramSpec()->GetMaxEpsilonBinNumber();
		dTotalBinNumber = nTotalBinNumber;
		while (nTotalBinNumber >= 10)
		{
			bIsPWCH = IsDatasetPWCH(cvSourceValues, nTotalBinNumber);
			Trace(sTmp + "\tTest PWCH\t" + IntToString(nTotalBinNumber) + "\t" + BooleanToString(bIsPWCH));
			dTotalBinNumber /= sqrt(10);
			nTotalBinNumber = (int)dTotalBinNumber;
		}
	}

	// Test si les donnees sont PWCH
	nTotalBinNumber = GetHistogramSpec()->GetOutlierEpsilonBinNumber();
	bIsPWCH = IsDatasetPWCH(cvSourceValues, nTotalBinNumber);
	Trace(sTmp + "Is PWCH\t" + BooleanToString(bIsPWCH));

	// Utilisation de la methode standard si les donnees sont bien conditionnees pour les histogrammes
	if (bIsPWCH)
	{
		// On passe par un histogramme algorithmique pour l'optimisation
		TypicalDiscretizeValues(cvSourceValues, optimizedHistogramFrequencyTable);

		// On transforme le resultat en histogramme
		optimizedHistogram = new MHGenumHistogram;
		BuildOutputHistogram(cvSourceValues, 0, cvSourceValues->GetSize(), optimizedHistogramFrequencyTable,
				     optimizedHistogram);

		// Nettoyage
		delete optimizedHistogramFrequencyTable;
	}
	// Sinon, on passe par l'heuristique de gestion des outliers
	else
	{
		TraceBeginBlock("Outlier management heuristic");

		// Discretisation des donnes log-transformees si necessaire
		DiscretizeLogTrValues(cvSourceValues, optimizedLogTrHistogramFrequencyTable);

		// Simplification de l'histogramme construit sur la log-transformation des valeurs
		SimplifyLogTrHistogram(cvSourceValues, optimizedLogTrHistogramFrequencyTable, &oaDataSubsets);
		delete optimizedLogTrHistogramFrequencyTable;

		// Decoupage si necessaire des sous-ensembles PICH restants issus des phases precedentes
		SplitPICHLogTrDataSubsets(cvSourceValues, &oaDataSubsets);

		// Construction de sous-histogrammes pour chaque sous ensemble
		BuildSubHistograms(cvSourceValues, &oaDataSubsets);

		// Construction d'un histogramme par agregation d'histogrammes par partie
		BuildAggregatedHistogram(cvSourceValues, &oaDataSubsets, optimizedHistogram);
		oaDataSubsets.DeleteAll();
		TraceEndBlock("Outlier management heuristic");
	}
}

void MHDiscretizerGenumHistogram::TypicalDiscretizeValues(const ContinuousVector* cvSourceValues,
							  KWFrequencyTable*& optimizedHistogramFrequencyTable) const
{
	boolean bDisplay = false;
	KWFrequencyTable* initialHistogramFrequencyTable;
	boolean bIsTruncationManagementHeuristicNecessary;
	KWFrequencyTable* optimizedHistogramFrequencyTableToClean;
	int nTotalBinNumber;
	int nIntervalNumber;
	int nEmptyIntervalNumber;
	int nFrequency;
	boolean bIsFirstIntervalSingleton;
	boolean bIsLastIntervalSingleton;
	ALString sTmp;

	require(cvSourceValues != NULL);

	TraceBeginBlock("Main heuristic");

	// Calcul de la table initiale
	nTotalBinNumber = GetHistogramSpec()->GetEpsilonBinNumber();
	ComputeInitialFrequencyTable(cvSourceValues, nTotalBinNumber, initialHistogramFrequencyTable);
	if (bDisplay)
		WriteHistogramFrequencyTable("Initial histogram\n", cvSourceValues, initialHistogramFrequencyTable,
					     cout);

	// Parametrage de l'algorithme utilise
	if (GetHistogramSpec()->GetOptimalAlgorithm())
		cast(MHDiscretizerGenumHistogram*, this)->SetParam(Optimal);
	else
		cast(MHDiscretizerGenumHistogram*, this)->SetParam(OptimizedGreedyMerge);

	// Appel de la methode de discretisation MODL
	if (GetHistogramSpec()->GetGranularizedModel())
		GranularizedDiscretizeValues(cvSourceValues, initialHistogramFrequencyTable,
					     optimizedHistogramFrequencyTable);
	else
		StandardDiscretizeValues(cvSourceValues, initialHistogramFrequencyTable,
					 optimizedHistogramFrequencyTable);

	// Memorisation de l'histogramme dans un fichier si les logs sont demandes
	if (bDisplay)
		WriteHistogramFrequencyTable("Optimized histogram\n", cvSourceValues, optimizedHistogramFrequencyTable,
					     cout);
	if (GetHistogramSpec()->GetExportInternalLogFiles())
	{
		WriteHistogramFrequencyTableFile("Optimized histogram\n", cvSourceValues,
						 optimizedHistogramFrequencyTable,
						 GetHistogramSpec()->GetInternalHistogramFileName());
	}

	// Heuristique de gestion des valeurs tronquees
	if (GetHistogramSpec()->GetTruncationManagementHeuristic())
	{
		///////////////////////////////////////////////////////////////////////////////////////////////////////////
		// On declenche l'heuristique de troncature si:
		//   . au moins un intervalle est vide
		//   . on n'est pas dans le cas booleen, qui se reduit a trois intervalles, dont deux intervalles
		//   singletons . on n'est pas dans le cas d'un outlier, avec un intervalle singleton en premier suivi
		//   d'un intervalle vide
		//     ou intervalle singleton en dernier precede d'un intervalle vide
		// Ensuite, on effectue les traitement supplementaires suivants:
		//  . calcul d'un histogramme optimal pour les variations de valeurs deltaX
		//  . et si ce dernier contient un premier intervalle singulier, la longueur du deuxime intervalle
		//    fournit un epslion de troncature, et il faut alors calcul un troisieme histogramme optimal a
		//    partir des valeurs X sur la base de l'epsilon de troncature

		// Il faut au moins un intervalle vide
		nEmptyIntervalNumber = ComputeHistogramEmptyIntervalNumber(optimizedHistogramFrequencyTable);
		bIsTruncationManagementHeuristicNecessary = nEmptyIntervalNumber >= 1;

		// Traitement des exception
		if (bIsTruncationManagementHeuristicNecessary)
		{
			assert(optimizedHistogramFrequencyTable->GetFrequencyVectorNumber() >= 3);

			// Nombre d'intervalles
			nIntervalNumber = optimizedHistogramFrequencyTable->GetFrequencyVectorNumber();

			// On determine si le premier intervalle est singleton
			nFrequency =
			    cast(MHGenumHistogramVector*, optimizedHistogramFrequencyTable->GetFrequencyVectorAt(0))
				->GetFrequency();
			assert(nFrequency > 0);
			bIsFirstIntervalSingleton = cvSourceValues->GetAt(0) == cvSourceValues->GetAt(nFrequency - 1);

			// On determine si le dernier intervalle est singleton
			nFrequency = cast(MHGenumHistogramVector*,
					  optimizedHistogramFrequencyTable->GetFrequencyVectorAt(nIntervalNumber - 1))
					 ->GetFrequency();
			assert(nFrequency > 0);
			bIsLastIntervalSingleton = cvSourceValues->GetAt(cvSourceValues->GetSize() - nFrequency) ==
						   cvSourceValues->GetAt(cvSourceValues->GetSize() - 1);

			// On ne traite pas le cas booleen
			if (optimizedHistogramFrequencyTable->GetFrequencyVectorNumber() == 3 and
			    bIsFirstIntervalSingleton and bIsLastIntervalSingleton)
				bIsTruncationManagementHeuristicNecessary = false;
			// On ne traite pas le cas d'un seul outlier
			else if (nEmptyIntervalNumber == 1)
			{
				// Cas d'un oulier au debut
				if (bIsFirstIntervalSingleton)
				{
					nFrequency = cast(MHGenumHistogramVector*,
							  optimizedHistogramFrequencyTable->GetFrequencyVectorAt(1))
							 ->GetFrequency();
					if (nFrequency == 0)
						bIsTruncationManagementHeuristicNecessary = false;
				}
				// Cas d'un oulier a la fin
				else if (bIsLastIntervalSingleton)
				{
					nFrequency = cast(MHGenumHistogramVector*,
							  optimizedHistogramFrequencyTable->GetFrequencyVectorAt(
							      nIntervalNumber - 2))
							 ->GetFrequency();
					if (nFrequency == 0)
						bIsTruncationManagementHeuristicNecessary = false;
				}
			}
		}
		Trace(sTmp + "Is TMH necessary\t" + BooleanToString(bIsTruncationManagementHeuristicNecessary));

		// On ne la declenche que si necessaire
		if (bIsTruncationManagementHeuristicNecessary)
		{
			// Memorisation de l'histogramme courant
			optimizedHistogramFrequencyTableToClean = optimizedHistogramFrequencyTable;

			// Application de l'heuristique de troncature
			TruncationPostOptimization(cvSourceValues, optimizedHistogramFrequencyTableToClean,
						   optimizedHistogramFrequencyTable);

			// On prend en compte le nouvel histogramme s'il a ete produit
			if (optimizedHistogramFrequencyTable == NULL)
				optimizedHistogramFrequencyTable = optimizedHistogramFrequencyTableToClean;
			else
				delete optimizedHistogramFrequencyTableToClean;
			if (bDisplay)
				WriteHistogramFrequencyTable("Post-optimized truncation histogram\n", cvSourceValues,
							     optimizedHistogramFrequencyTable, cout);
		}
	}

	// Nettoyage
	delete initialHistogramFrequencyTable;
	TraceEndBlock("Main heuristic");
}

void MHDiscretizerGenumHistogram::StandardDiscretizeValues(const ContinuousVector* cvSourceValues,
							   KWFrequencyTable* initialHistogramFrequencyTable,
							   KWFrequencyTable*& optimizedHistogramFrequencyTable) const
{
	require(cvSourceValues != NULL);
	require(initialHistogramFrequencyTable != NULL);

	TraceBeginBlock("Enum algorithm");

	// Appel de la methode de de discretisation definie dans la classe KWDiscretizerMODL
	DiscretizeGranularizedFrequencyTable(initialHistogramFrequencyTable, optimizedHistogramFrequencyTable);
	TraceEndBlock("Enum algorithm");
}

void MHDiscretizerGenumHistogram::GranularizedDiscretizeValues(
    const ContinuousVector* cvSourceValues, KWFrequencyTable* initialHistogramFrequencyTable,
    KWFrequencyTable*& optimizedHistogramFrequencyTable) const
{
	boolean bDisplayInitialTable = false;
	boolean bDisplayDetails = false;
	fstream fLog;
	boolean bLogOk;
	KWFrequencyTable* granularizedHistogramFrequencyTable;
	KWFrequencyTable* optimizedGranularizedHistogramFrequencyTable;
	const int nMaxSuccessiveDecreaseNumber = 2;
	int nTotalIncreaseNumber;
	int nSuccessiveDecreaseNumber;
	int nIndex = 0;
	int nLastDecreaseIndex = 0;
	double dPreviousCost;
	double dBestCost;
	double dCost;
	int nIntervalNumber;
	double dPartitionCost;
	int nHierarchyLevel;
	int nPartileNumber;
	int nTotalBinNumber;
	int nMaxPartileNumber;
	int nActualPartileNumber;
	int nLastActualPartileNumber;
	ALString sTmp;

	require(cvSourceValues != NULL);
	require(initialHistogramFrequencyTable != NULL);

	TraceBeginBlock("G-Enum algorithm");

	// Affichage de la table innitiale
	if (bDisplayInitialTable)
		WriteHistogramFrequencyTable("Initial histogram\n", cvSourceValues, initialHistogramFrequencyTable,
					     cout);

	// Memorisation des informations dans un fichier de resultats
	bLogOk = false;
	if (GetHistogramSpec()->GetExportInternalLogFiles())
	{
		// Ouverture du fichier de log
		bLogOk = FileService::OpenOutputFile(GetHistogramSpec()->GetInternalOptimizationFileName(""), fLog);
		if (bLogOk)
		{
			// Affichage d'une ligne d'entete
			fLog << "Granularity\tActual partiles\tIntervals\tEpsilon bins\tlog*(G)\tlog*(K)\tPartition "
				"cost\tCost"
			     << endl;
		}
		Trace("Optimization file\t" + GetHistogramSpec()->GetInternalOptimizationFileName(""));
	}

	// Calcul du nombre max de partiles a evaluer
	nTotalBinNumber = ComputeHistogramTotalBinNumber(initialHistogramFrequencyTable);
	nMaxPartileNumber = GetHistogramSpec()->ComputeMaxPartileNumber(
	    cvSourceValues->GetSize(), cvSourceValues->GetAt(0), cvSourceValues->GetAt(cvSourceValues->GetSize() - 1));
	nMaxPartileNumber = min(nMaxPartileNumber, nTotalBinNumber);

	// Optimisation de la granularite
	// On s'arrete quand on depasse un nombre max d'essais consecutifs sans amelioration
	dBestCost = DBL_MAX;
	nHierarchyLevel = 0;
	nPartileNumber = 1;
	nLastActualPartileNumber = 1;
	optimizedHistogramFrequencyTable = NULL;
	nTotalIncreaseNumber = 0;
	nSuccessiveDecreaseNumber = 0;
	nLastDecreaseIndex = 0;
	nIndex = 0;
	dPreviousCost = DBL_MAX;
	while (nPartileNumber <= nMaxPartileNumber)
	{
		nHierarchyLevel++;

		// Construction d'une version granularisee de la table, apres avoir parametree correctement le nombre de
		// partiles
		BuildGranularizedFrequencyTable(initialHistogramFrequencyTable, nPartileNumber,
						granularizedHistogramFrequencyTable);
		if (bDisplayDetails)
			WriteHistogramFrequencyTable(sTmp + "Granularized histogram\t" + IntToString(nPartileNumber) +
							 "\n",
						     cvSourceValues, granularizedHistogramFrequencyTable, cout);

		// On saute ce nombre de partiles s'il n'a pas conduit a au moins 10% de parties en plus
		nActualPartileNumber = granularizedHistogramFrequencyTable->GetFrequencyVectorNumber();
		if (nPartileNumber > 1 and nPartileNumber != nMaxPartileNumber and
		    nActualPartileNumber < 1.10 * nLastActualPartileNumber)
		{
			// On peut detruire la version granularisee initiale
			delete granularizedHistogramFrequencyTable;

			// Passage directe a la granularite suivante
			if (nPartileNumber <= nMaxPartileNumber / 2)
				nPartileNumber *= 2;
			else if (nPartileNumber < nMaxPartileNumber)
				nPartileNumber = nMaxPartileNumber;
			else
				break;
			continue;
		}
		nLastActualPartileNumber = nActualPartileNumber;

		// Appel de la methode de de discretisation definie dans la classe KWDiscretizerMODL
		DiscretizeGranularizedFrequencyTable(granularizedHistogramFrequencyTable,
						     optimizedGranularizedHistogramFrequencyTable);

		// On peut detruire la version granularisee initiale
		delete granularizedHistogramFrequencyTable;

		// On doit reparametre les couts pour collecter des stats sur les couts obtenus
		InitializeWorkingData(optimizedGranularizedHistogramFrequencyTable);
		nIntervalNumber = optimizedGranularizedHistogramFrequencyTable->GetFrequencyVectorNumber();
		dCost =
		    GetDiscretizationCosts()->ComputePartitionGlobalCost(optimizedGranularizedHistogramFrequencyTable);
		dPartitionCost = GetDiscretizationCosts()->ComputePartitionModelCost(nIntervalNumber, 0);
		CleanWorkingData();

		// Affichage
		if (bLogOk)
			fLog << nPartileNumber << "\t" << nActualPartileNumber << "\t" << nIntervalNumber << "\t"
			     << ComputeHistogramTotalBinNumber(optimizedGranularizedHistogramFrequencyTable) << "\t"
			     << KWStat::KWStat::NaturalNumbersUniversalCodeLength(nPartileNumber) << "\t"
			     << KWStat::KWStat::NaturalNumbersUniversalCodeLength(nIntervalNumber) << "\t"
			     << KWContinuous::ContinuousToString(dPartitionCost) << "\t"
			     << KWContinuous::ContinuousToString(dCost) << endl;

		// Memorisation si amelioration du cout
		if (dCost < dBestCost - dEpsilon)
		{
			dBestCost = dCost;
			if (optimizedHistogramFrequencyTable != NULL)
				delete optimizedHistogramFrequencyTable;
			optimizedHistogramFrequencyTable = optimizedGranularizedHistogramFrequencyTable;

			// Memorisation de la granularite, dans la variable disponible dans les tables d'effectifs
			optimizedHistogramFrequencyTable->SetGranularizedValueNumber(nPartileNumber);

			// Incrementation du nombre d'ameliorations (sans compter celle du modele nul initial)
			if (nPartileNumber > 1)
				nTotalIncreaseNumber++;
		}
		else
			delete optimizedGranularizedHistogramFrequencyTable;

		// Gestion du nombre max d'essai successif sans amelioration du cout
		if (nIndex > 0)
		{
			// Compte du nombre de deterioration successives du cout
			if (dCost > dPreviousCost)
			{
				if (nLastDecreaseIndex == nIndex - 1)
					nSuccessiveDecreaseNumber++;
				else
					nSuccessiveDecreaseNumber = 1;
				nLastDecreaseIndex = nIndex;
			}
			else
			{
				nSuccessiveDecreaseNumber = 0;
				nLastDecreaseIndex = nIndex + 1;
			}
			dPreviousCost = dCost;

			// Arret possible si un nombre minimum de partiles a ete teste
			if (nPartileNumber > sqrt(cvSourceValues->GetSize() * (1 + log(1 + cvSourceValues->GetSize()))))
			{
				// Arret si aucune amelioration et que l'on atteint presque le nombre total d'instance
				// ou en tout cas le nombre d'instance maximal permettant de detecter un pattern
				// complexe (comme des creneaux)
				if (nTotalIncreaseNumber == 0)
				{
					// Cas d'un petit nombre d'instances: on attente que le nombre de partiles
					// depasse le nombre d'instances
					if (cvSourceValues->GetSize() < 128)
					{
						if (nPartileNumber > cvSourceValues->GetSize())
							break;
					}
					// Sinon, on limite progressivement le critere d'arret, avec des transition
					// continues, lineaires par morceau
					else if (cvSourceValues->GetSize() < 256)
					{
						if (nPartileNumber > 128 + (cvSourceValues->GetSize() - 128) / 2)
							break;
					}
					else if (cvSourceValues->GetSize() < 512)
					{
						if (nPartileNumber > 192 + (cvSourceValues->GetSize() - 256) / 4)
							break;
					}
					// Asymptotiquement, on se limite a n/8, qui permet de detecter des patterns
					// parmi les plus complexes
					else
					{
						if (nPartileNumber > 256 + (cvSourceValues->GetSize() - 512) / 8)
							break;
					}
				}

				// Arret si au dela d'un seuil minimum du nombre d'instances si on a deja detecte un
				// pattern et que plusieurs essai successifs n'on rien ameliore
				if (nTotalIncreaseNumber > 0 and
				    nSuccessiveDecreaseNumber > nMaxSuccessiveDecreaseNumber)
					break;
			}
		}

		// Partile suivant, par puissances de 2
		nIndex++;
		if (nPartileNumber <= nMaxPartileNumber / 2)
			nPartileNumber *= 2;
		else if (nPartileNumber < nMaxPartileNumber)
			nPartileNumber = nMaxPartileNumber;
		else
			break;
	}

	// Nettoyage du parametrage des couts
	cast(MHGenumHistogramCosts*, GetDiscretizationCosts())->SetPartileNumber(0);

	// Fermeture du fichier de log
	if (bLogOk)
		FileService::CloseOutputFile(GetHistogramSpec()->GetInternalOptimizationFileName(""), fLog);

	// Infos de trace
	Trace(sTmp + "Best cost\t" + DoubleToString(dBestCost));
	Trace(sTmp + "Best cost\t" + DoubleToString(dBestCost));
	Trace(sTmp + "Best granularity\t" +
	      IntToString(optimizedHistogramFrequencyTable->GetGranularizedValueNumber()));
	Trace(sTmp + "Best interval number\t" +
	      IntToString(optimizedHistogramFrequencyTable->GetFrequencyVectorNumber()));
	TraceEndBlock("G-Enum algorithm");

	assert(optimizedHistogramFrequencyTable != NULL);
}

void MHDiscretizerGenumHistogram::DiscretizeLogTrValues(const ContinuousVector* cvSourceValues,
							KWFrequencyTable*& optimizedLogTrHistogramFrequencyTable) const
{
	ContinuousVector cvLogTrValues;
	MHGenumHistogramSpec refHistogramSpec;

	require(cvSourceValues != NULL);

	TraceBeginBlock("Compute histogram on LogTr(values)");

	// Parametrage du discretiseur dans pour les donnes log-transformees
	refHistogramSpec.CopyFrom(GetHistogramSpec());
	GetHistogramSpec()->SetTruncationManagementHeuristic(false);
	GetHistogramSpec()->SetOutlierManagementHeuristic(false);
	GetHistogramSpec()->SetLogTrValues(true);

	// Calcul du vecteur de valeur log-transforme
	ComputeLogTrData(cvSourceValues, &cvLogTrValues);

	// Export du fichier des donnees transformees si les logs sont demandes
	if (GetHistogramSpec()->GetExportInternalLogFiles())
		ExportLogTrData(cvSourceValues, GetHistogramSpec()->GetInternalOutlierLogTrDataFileName(), "X",
				"LogTrX");

	// Discretisation des donnees log-transformes
	TypicalDiscretizeValues(&cvLogTrValues, optimizedLogTrHistogramFrequencyTable);

	// Restitution du parametrage initial
	GetHistogramSpec()->CopyFrom(&refHistogramSpec);
	TraceEndBlock("Compute histogram on LogTr(values)");
}

void MHDiscretizerGenumHistogram::SimplifyLogTrHistogram(const ContinuousVector* cvSourceValues,
							 const KWFrequencyTable* optimizedLogTrHistogramFrequencyTable,
							 ObjectArray* oaDataSubsets) const
{
	boolean bDisplay = false;
	int nOutlierEpsilonBinNumber;
	MHGenumHistogramVector* histogramVector;
	MHDataSubset* dataSubset;
	MHDataSubset* dataSubset1;
	MHDataSubset* dataSubset2;
	int i;
	int nNewSize;
	int nFrequency;
	int nFirstIndex;
	int nLastIndex;
	boolean bIsDataSubsetPWCH;
	int nUpdateNumber;
	int nMergeStep;
	ALString sTmp;

	require(cvSourceValues != NULL);
	require(optimizedLogTrHistogramFrequencyTable != NULL);
	require(cvSourceValues->GetSize() == ComputeHistogramTotalFrequency(optimizedLogTrHistogramFrequencyTable));
	require(oaDataSubsets != NULL);
	require(oaDataSubsets->GetSize() == 0);

	TraceBeginBlock("Simplify LogTr histogram");

	// Recherche du seuil de nombre de bin utilise pour la detection des outliers
	nOutlierEpsilonBinNumber = GetHistogramSpec()->GetOutlierEpsilonBinNumber();

	// Parcours des intervalles pour initialiser les sous ensembles avec leur statut PWCH
	nFirstIndex = 0;
	nLastIndex = 0;
	for (i = 0; i < optimizedLogTrHistogramFrequencyTable->GetFrequencyVectorNumber(); i++)
	{
		histogramVector =
		    cast(MHGenumHistogramVector*, optimizedLogTrHistogramFrequencyTable->GetFrequencyVectorAt(i));

		// On ignore les intervalles d'effectif nul
		nFrequency = histogramVector->GetFrequency();
		if (nFrequency == 0)
			continue;

		// Caracteristiques du sous-ensemble
		nLastIndex = nFirstIndex + nFrequency;

		// Creation du sous-ensemble
		dataSubset = new MHDataSubset;
		dataSubset->SetDatasetValues(cvSourceValues);
		dataSubset->SetFirstValueIndex(nFirstIndex);
		dataSubset->SetLastValueIndex(nLastIndex);

		// Test si le sous-ensemble correspondant est PWCH
		if (dataSubset->IsSingleton())
			dataSubset->SetPWCH(true);
		else
		{
			bIsDataSubsetPWCH =
			    IsDataSubsetPWCH(cvSourceValues, nFirstIndex, nLastIndex, nOutlierEpsilonBinNumber);
			dataSubset->SetPWCH(bIsDataSubsetPWCH);
		}
		dataSubset->SetTerminalPrev(not dataSubset->GetPWCH());
		dataSubset->SetTerminalNext(not dataSubset->GetPWCH());
		oaDataSubsets->Add(dataSubset);

		// Preparation du sous-ensemble suivant
		nFirstIndex = nLastIndex;
	}
	if (bDisplay)
		DisplayDataSubsetsArray(sTmp + "Initial data subsets\t" + IntToString(oaDataSubsets->GetSize()) + "\n",
					oaDataSubsets, cout);
	Trace(sTmp + "Initial data subsets\t" + IntToString(oaDataSubsets->GetSize()));

	// Boucle d'amelioration pour diminuer au miximum le nombre de sous-ensemble de donnees
	// A chaque etape de la boucle, on envisage la fusuon des sous-ensemble par paire
	// ce qui assure que l'algorithme est globalement en O(n log n)
	nUpdateNumber = 1;
	nMergeStep = 0;
	while (nUpdateNumber > 0)
	{
		// Parcours des sous-ensemble par paire pour tentre de les fusionner
		nMergeStep++;
		nUpdateNumber = 0;
		i = 0;
		nNewSize = 0;
		while (i < oaDataSubsets->GetSize())
		{
			// Cas particulier d'un jeu de donnees en fin de tableau
			if (i == oaDataSubsets->GetSize() - 1)
			{
				dataSubset1 = cast(MHDataSubset*, oaDataSubsets->GetAt(i));
				oaDataSubsets->SetAt(nNewSize, dataSubset1);
				nNewSize++;
				i += 1;
			}
			// Sinon, il y a au moins une fusion a envisager
			else
			{
				// Acces aux deux sous-ensemble a fusionner eventuellement
				dataSubset1 = cast(MHDataSubset*, oaDataSubsets->GetAt(i));
				dataSubset2 = cast(MHDataSubset*, oaDataSubsets->GetAt(i + 1));
				assert(dataSubset1->GetLastValueIndex() == dataSubset2->GetFirstValueIndex());

				// Pas de fusion possible si le premier sous-ensemble est terminal pour le suivant
				// ou si le second sous ensemble est terminal pour le precedent
				if (dataSubset1->GetTerminalNext() or dataSubset2->GetTerminalPrev())
				{
					oaDataSubsets->SetAt(nNewSize, dataSubset1);
					nNewSize++;
					i += 1;
				}
				// Sinon, tentative de fusion
				else
				{
					// On regarde si les deux ensembles fusionnes sont PWCH
					assert(dataSubset1->GetPWCH());
					assert(dataSubset2->GetPWCH());
					bIsDataSubsetPWCH = IsDataSubsetPWCH(
					    cvSourceValues, dataSubset1->GetFirstValueIndex(),
					    dataSubset2->GetLastValueIndex(), nOutlierEpsilonBinNumber);

					// Si Ok, on les fusionne
					if (bIsDataSubsetPWCH)
					{
						nUpdateNumber++;
						dataSubset1->SetLastValueIndex(dataSubset2->GetLastValueIndex());
						oaDataSubsets->SetAt(nNewSize, dataSubset1);
						nNewSize++;
						delete dataSubset2;
						i += 2;
					}
					// Sinon, on saute le premier sous ensemble en specifiant les indicateur
					// terminal
					else
					{
						nUpdateNumber++;
						dataSubset1->SetTerminalNext(true);
						dataSubset2->SetTerminalPrev(true);
						oaDataSubsets->SetAt(nNewSize, dataSubset1);
						nNewSize++;
						i += 1;
					}
				}
			}
		}

		// Retaillage du tableau des sous-ensembles
		oaDataSubsets->SetSize(nNewSize);

		// Affichage
		if (bDisplay)
			DisplayDataSubsetsArray(sTmp + "Merged data subsets\t" + IntToString(nMergeStep) + "\t" +
						    IntToString(oaDataSubsets->GetSize()) + "\n",
						oaDataSubsets, cout);
	}

	// On passe le dernier sous-ensemble en status Terminal
	cast(MHDataSubset*, oaDataSubsets->GetAt(oaDataSubsets->GetSize() - 1))->SetTerminalPrev(true);

	// Trace de fin
	Trace(sTmp + "Merge loop number\t" + IntToString(nMergeStep));
	Trace(sTmp + "Final data subsets\t" + IntToString(oaDataSubsets->GetSize()));
	TraceEndBlock("Simplify LogTr histogram");
	ensure(oaDataSubsets->GetSize() > 0);
	ensure(cast(MHDataSubset*, oaDataSubsets->GetAt(0))->GetFirstValueIndex() == 0);
	ensure(cast(MHDataSubset*, oaDataSubsets->GetAt(oaDataSubsets->GetSize() - 1))->GetLastValueIndex() ==
	       cvSourceValues->GetSize());
}

void MHDiscretizerGenumHistogram::SplitPICHLogTrDataSubsets(const ContinuousVector* cvSourceValues,
							    ObjectArray* oaDataSubsets) const
{
	int i;
	MHDataSubset* dataSubset;
	ObjectArray oaInitialDataSubsets;
	ObjectArray oaSplittedDataSubsets;
	ALString sTmp;

	require(cvSourceValues != NULL);
	require(oaDataSubsets != NULL);

	TraceBeginBlock("Split PICH LogTr data subsets");
	Trace(sTmp + "Initial data subsets\t" + IntToString(oaDataSubsets->GetSize()));

	// Recopie des data subsets initiaux
	oaInitialDataSubsets.CopyFrom(oaDataSubsets);

	// Parcours des data subsets pour traiter ceux qui sont PICH
	oaDataSubsets->RemoveAll();
	for (i = 0; i < oaInitialDataSubsets.GetSize(); i++)
	{
		dataSubset = cast(MHDataSubset*, oaInitialDataSubsets.GetAt(i));

		// On garde le meme data subset s'il est PWCH
		if (dataSubset->GetPWCH())
			oaDataSubsets->Add(dataSubset);
		// Sinon, on le remplace par son redecoupage
		else
		{
			// Decoupage du data subset
			SplitPICHLogTrDataSubset(cvSourceValues, dataSubset, &oaSplittedDataSubsets);

			// Insertion des nouveau deta subsets issus du redecoupage
			oaDataSubsets->InsertObjectArrayAt(oaDataSubsets->GetSize(), &oaSplittedDataSubsets);
			oaSplittedDataSubsets.RemoveAll();

			// Destruction de l'ancien data subset
			delete dataSubset;
		}
	}
	Trace(sTmp + "Final data subsets\t" + IntToString(oaDataSubsets->GetSize()));
	TraceEndBlock("Split PICH LogTr data subsets");
}

void MHDiscretizerGenumHistogram::SplitPICHLogTrDataSubset(const ContinuousVector* cvSourceValues,
							   const MHDataSubset* dataSubset,
							   ObjectArray* oaSplittedDataSubsets) const
{
	int i;
	Continuous cValue;
	int nLastNegativeValueIndex;
	int nFirstPositiveValueIndex;
	MHDataSubset halfDataSubset;
	ObjectArray oaHalfSplittedDataSubsets;
	MHDataSubset* splittedDataSubset;
	int nPartileNumber;
	int nTotalBinNumber;
	double dPICHBinNumberThreshold;
	double dExpectedPartileFrequency;
	double dExpectedPartileDensestBinFrequency;
	double dPartileBoundRatio;
	int nPartile;
	double dPartileUpperBound;
	int nPartileFirstValueIndex;
	int nPartileLastValueIndex;

	require(cvSourceValues != NULL);
	require(dataSubset != NULL);
	require(not dataSubset->GetPWCH());
	require(oaSplittedDataSubsets != NULL);
	require(oaSplittedDataSubsets->GetSize() == 0);

	// Cas d'une seule valeur
	if (dataSubset->IsSingleton())
	{
		// On renvoie tout le data subset
		oaSplittedDataSubsets->Add(dataSubset->Clone());
	}
	// Cas ou les valeurs sont toutes du meme signe
	else if (dataSubset->GetLastValue() < 0 or 0 < dataSubset->GetFirstValue())
	{
		//////////////////////////////////////////////////////////////////////////////////////////
		// Recherche du nombre de partile optimal pour obtenir des data subset probablement PWCH
		// (cf. document sur la gestion des outliers dans les histogrammes

		// Nombre total de bin a prendre en compte
		nTotalBinNumber = GetHistogramSpec()->GetEpsilonBinNumber();

		// Calcul du seuil d'effectif pour le critere PICH
		dPICHBinNumberThreshold = sqrt(nTotalBinNumber) * log(nTotalBinNumber + 1.0);

		// Boucle d'optimisation sur les partiles
		dPartileBoundRatio = 0;
		for (nPartileNumber = 1; nPartileNumber < sqrt(dataSubset->GetSize()); nPartileNumber++)
		{
			// Estimation de l'effectif d'un partile
			dExpectedPartileFrequency = dataSubset->GetSize() / nPartileNumber;

			// Calcul du ratio des bornes du partile, en fonction du signe des valeurs
			if (dataSubset->GetFirstValue() > 0)
				dPartileBoundRatio =
				    exp(log(dataSubset->GetLastValue() / dataSubset->GetFirstValue()) / nPartileNumber);
			else
				dPartileBoundRatio =
				    exp(log(dataSubset->GetFirstValue() / dataSubset->GetLastValue()) / nPartileNumber);

			// Estimation de l'effectif du bin le plus dense du partile
			dExpectedPartileDensestBinFrequency =
			    dExpectedPartileFrequency * log((1 + (dPartileBoundRatio - 1) / dPICHBinNumberThreshold)) /
			    log(dPartileBoundRatio);

			// Arret si le critere PWCH est satisfait
			// (en fait avec un facteur 2, car seul un bin dans le cas particulier d'une densite uniforme
			// sur l'espace des logs)
			if (dExpectedPartileDensestBinFrequency < 2 * log(dExpectedPartileFrequency))
				break;
		}
		assert(dPartileBoundRatio > 0);

		// Cas particulier d'un seul partile: on garde tout le data subset
		if (nPartileNumber == 1)
			oaSplittedDataSubsets->Add(dataSubset->Clone());
		// Cas de plusieurs partiles: on doit decouper le data subset selon les partiles
		else
		{
			assert(
			    (dataSubset->GetFirstValue() > 0 and
			     dPartileBoundRatio ==
				 exp(log(dataSubset->GetLastValue() / dataSubset->GetFirstValue()) / nPartileNumber)) or
			    (dataSubset->GetLastValue() < 0 and
			     dPartileBoundRatio ==
				 exp(log(dataSubset->GetFirstValue() / dataSubset->GetLastValue()) / nPartileNumber)));

			// Boucle d'optimisation sur les partiles
			nPartileFirstValueIndex = dataSubset->GetFirstValueIndex();
			dPartileUpperBound = dataSubset->GetFirstValue();
			for (nPartile = 0; nPartile < nPartileNumber; nPartile++)
			{
				// Borne sup du partile courant, en fonction du signe des valeurs
				if (dataSubset->GetFirstValue() > 0)
					dPartileUpperBound *= dPartileBoundRatio;
				else
					dPartileUpperBound /= dPartileBoundRatio;
				if (nPartile == nPartileNumber - 1)
					dPartileUpperBound = dataSubset->GetLastValue();

				// Recherche de la premiere valeur depassant cette borne sup
				nPartileLastValueIndex = dataSubset->GetLastValueIndex();
				for (i = nPartileFirstValueIndex; i < dataSubset->GetLastValueIndex(); i++)
				{
					if (cvSourceValues->GetAt(i) > dPartileUpperBound)
					{
						nPartileLastValueIndex = i;
						break;
					}
				}

				// Ajout d'un nouveau data subset si non vide
				if (nPartileLastValueIndex > nPartileFirstValueIndex)
				{
					// Creation d'un nouveau split
					splittedDataSubset = dataSubset->Clone();
					oaSplittedDataSubsets->Add(splittedDataSubset);

					// Parametrage des index de ses bornes
					splittedDataSubset->SetFirstValueIndex(nPartileFirstValueIndex);
					splittedDataSubset->SetLastValueIndex(nPartileLastValueIndex);
				}

				// Preparation du partile suivant
				nPartileFirstValueIndex = nPartileLastValueIndex;
			}
		}
	}
	// Cas ou 0 fait partie de l'etendue du  data subset
	else
	{
		assert(dataSubset->GetFirstValue() <= 0);
		assert(dataSubset->GetLastValue() >= 0);

		// Recherche de la plus grande valeur negative
		nLastNegativeValueIndex = -1;
		if (dataSubset->GetFirstValue() < 0)
		{
			for (i = dataSubset->GetFirstValueIndex(); i < dataSubset->GetLastValueIndex(); i++)
			{
				cValue = cvSourceValues->GetAt(i);
				if (cValue < 0)
					nLastNegativeValueIndex = i;
				else
					break;
			}
		}

		// Recherche de la plus petite positive
		nFirstPositiveValueIndex = -1;
		if (dataSubset->GetLastValue() > 0)
		{
			assert(nLastNegativeValueIndex >= 0 or dataSubset->GetFirstValue() == 0);
			for (i = nLastNegativeValueIndex + 1; i < dataSubset->GetLastValueIndex(); i++)
			{
				cValue = cvSourceValues->GetAt(i);
				if (cValue > 0)
				{
					nFirstPositiveValueIndex = i;
					break;
				}
			}
		}

		// Verification
		assert(nLastNegativeValueIndex >= 0 or dataSubset->GetFirstValue() == 0);
		assert(nLastNegativeValueIndex == -1 or cvSourceValues->GetAt(nLastNegativeValueIndex) < 0);
		assert(nLastNegativeValueIndex == -1 or cvSourceValues->GetAt(nLastNegativeValueIndex + 1) >= 0);
		assert(nFirstPositiveValueIndex >= 0 or dataSubset->GetLastValue() == 0);
		assert(nFirstPositiveValueIndex == -1 or cvSourceValues->GetAt(nFirstPositiveValueIndex) > 0);
		assert(nFirstPositiveValueIndex == -1 or cvSourceValues->GetAt(nFirstPositiveValueIndex - 1) <= 0);
		assert(nLastNegativeValueIndex >= 0 or nFirstPositiveValueIndex >= 0);

		// Traitement de la partie des valeurs negatives
		if (nLastNegativeValueIndex >= 0)
		{
			// Parametrage d'un data subset correspondant aux valeurs negatives
			halfDataSubset.CopyFrom(dataSubset);
			halfDataSubset.SetLastValueIndex(nLastNegativeValueIndex + 1);

			// Decoupage en sous parties pour les incorporer dans le resultat
			SplitPICHLogTrDataSubset(cvSourceValues, &halfDataSubset, &oaHalfSplittedDataSubsets);
			oaSplittedDataSubsets->InsertObjectArrayAt(0, &oaHalfSplittedDataSubsets);
			oaHalfSplittedDataSubsets.RemoveAll();
		}

		// Traitement de l'eventuel partie restreinte a la valeur 0
		if (nLastNegativeValueIndex == -1 or nFirstPositiveValueIndex == -1 or
		    nLastNegativeValueIndex + 1 < nFirstPositiveValueIndex)
		{
			// Creation d'un nouveau split
			splittedDataSubset = dataSubset->Clone();
			oaSplittedDataSubsets->Add(splittedDataSubset);

			// Parametrage des index de ses bornes
			if (nLastNegativeValueIndex == -1)
			{
				assert(dataSubset->GetFirstValue() == 0);
				assert(cvSourceValues->GetAt(nFirstPositiveValueIndex - 1) == 0);
				splittedDataSubset->SetFirstValueIndex(0);
				splittedDataSubset->SetLastValueIndex(nFirstPositiveValueIndex);
			}
			else if (nFirstPositiveValueIndex == -1)
			{
				assert(dataSubset->GetLastValue() == 0);
				assert(cvSourceValues->GetAt(nLastNegativeValueIndex + 1) == 0);
				splittedDataSubset->SetFirstValueIndex(nLastNegativeValueIndex + 1);
				assert(splittedDataSubset->GetLastValue() == 0);
			}
			else if (nLastNegativeValueIndex < nFirstPositiveValueIndex)
			{
				assert(cvSourceValues->GetAt(nLastNegativeValueIndex + 1) == 0);
				assert(cvSourceValues->GetAt(nFirstPositiveValueIndex - 1) == 0);
				splittedDataSubset->SetFirstValueIndex(nLastNegativeValueIndex + 1);
				splittedDataSubset->SetLastValueIndex(nFirstPositiveValueIndex);
			}
		}

		// Traitement de la partie des valeurs positives
		if (nFirstPositiveValueIndex >= 0)
		{
			// Parametrage d'un data subset correspondant aux valeurs negatives
			halfDataSubset.CopyFrom(dataSubset);
			halfDataSubset.SetFirstValueIndex(nFirstPositiveValueIndex);

			// Decoupage en sous parties pour les incorporer dans le resultat
			SplitPICHLogTrDataSubset(cvSourceValues, &halfDataSubset, &oaHalfSplittedDataSubsets);
			oaSplittedDataSubsets->InsertObjectArrayAt(oaSplittedDataSubsets->GetSize(),
								   &oaHalfSplittedDataSubsets);
			oaHalfSplittedDataSubsets.RemoveAll();
		}
	}

	// Numerotation des index de decoupage des sous-ensemble
	for (i = 0; i < oaSplittedDataSubsets->GetSize(); i++)
	{
		splittedDataSubset = cast(MHDataSubset*, oaSplittedDataSubsets->GetAt(i));
		splittedDataSubset->SetPICHSplitIndex(i + 1);
	}
}

void MHDiscretizerGenumHistogram::BuildSubHistograms(const ContinuousVector* cvSourceValues,
						     const ObjectArray* oaDataSubsets) const
{
	MHDataSubset* dataSubset;
	int i;
	ContinuousVector cvDataSubsetValues;
	int n;
	KWFrequencyTable* subHistogramFrequencyTable;
	MHGenumHistogramVector* histogramVector;
	ALString sTmp;

	require(cvSourceValues != NULL);
	require(oaDataSubsets != NULL);

	TraceBeginBlock("Build sub histograms");

	// Construction d'un sous histogramme par partie
	for (i = 0; i < oaDataSubsets->GetSize(); i++)
	{
		// Acces au sous-ensemble
		dataSubset = cast(MHDataSubset*, oaDataSubsets->GetAt(i));

		// Verifications
		assert(dataSubset->GetDatasetValues() == cvSourceValues);
		assert(i == 0 or dataSubset->GetFirstValueIndex() ==
				     cast(MHDataSubset*, oaDataSubsets->GetAt(i - 1))->GetLastValueIndex());

		// Construction d'un histogramme pour la sous-partie si possible
		// Maintenant que meme les data subset PICH ont ete redecoupes, tous les
		// data subset sont traites, sauf les singletons
		Trace(sTmp + "Build sub histogram " + IntToString(i + 1));
		if (not dataSubset->IsSingleton())
		{
			// Extraction des valeurs du sous-ensembles
			cvDataSubsetValues.SetSize(dataSubset->GetSize());
			for (n = 0; n < cvDataSubsetValues.GetSize(); n++)
				cvDataSubsetValues.SetAt(n,
							 cvSourceValues->GetAt(dataSubset->GetFirstValueIndex() + n));

			// Construction d'un histogramme
			GetHistogramSpec()->SetOutlierSplitIndex(i + 1);
			TypicalDiscretizeValues(&cvDataSubsetValues, subHistogramFrequencyTable);
		}
		// Sinon, construction d'un histogramme mono-intervalle autour d'une seule valeur
		else
		{
			// On cree un seul intervalle constitue d'un seul bin dans ce cas particulier
			subHistogramFrequencyTable = NewFrequencyTable();
			subHistogramFrequencyTable->SetFrequencyVectorNumber(1);
			histogramVector =
			    cast(MHGenumHistogramVector*, subHistogramFrequencyTable->GetFrequencyVectorAt(0));
			histogramVector->SetFrequency(dataSubset->GetSize());
			histogramVector->SetLength(1);
			Trace(
			    sTmp + "Build single interval histogram (" +
			    KWContinuous::ContinuousToString(cvSourceValues->GetAt(dataSubset->GetFirstValueIndex())) +
			    ", " +
			    KWContinuous::ContinuousToString(
				cvSourceValues->GetAt(dataSubset->GetLastValueIndex() - 1)) +
			    ") " + IntToString(histogramVector->GetFrequency()) + " " +
			    IntToString(histogramVector->GetLength()));
		}

		// Memorisation de l'histogramme
		dataSubset->SetHistogram(subHistogramFrequencyTable);

		// Construction d'un histogramme entre le dernier intervalle de l'histogramme precedent et le
		// premier intervalle de l'histogramme courant
		if (i > 0)
		{
			Trace(sTmp + "Build boundary histogram " + IntToString(i) + "_" + IntToString(i + 1));
			GetHistogramSpec()->SetOutlierSplitIndex(i + 1);
			BuildBoundaryHistogram(cvSourceValues, cast(MHDataSubset*, oaDataSubsets->GetAt(i - 1)),
					       cast(MHDataSubset*, oaDataSubsets->GetAt(i)));
		}
	}
	GetHistogramSpec()->SetOutlierSplitIndex(-1);
	TraceEndBlock("Build sub histograms");
}

void MHDiscretizerGenumHistogram::BuildAggregatedHistogram(const ContinuousVector* cvSourceValues,
							   const ObjectArray* oaDataSubsets,
							   MHHistogram*& aggregatedHistogram) const
{
	boolean bDisplay = false;
	MHGenumHistogram* outputSubHistogram;
	MHGenumHistogram* outputBoundaryHistogram;
	MHDataSubset* previousDataSubset;
	MHDataSubset* dataSubset;
	MHGenumHistogramInterval* lastPreviousInterval;
	MHGenumHistogramInterval* firstNextInterval;
	int nBoundaryFirstIntervalFrequency;
	int nBoundarySecondIntervalFrequency;
	int nBoundaryFrequency;
	const KWFrequencyTable* previousDataSubsetHistogramFrequencyTable;
	MHGenumHistogramVector* initialLastPreviousHistogramVector;
	int nInitialBoundaryFirstIntervalFrequency;
	int nInitialBoundaryFrequency;
	int nBoundaryIntervalIndex;
	int i;
	int n;
	int nTotalFrequency;
	MHGenumHistogramInterval* interval;
	MHGenumHistogramInterval* nextInterval;
	MHGenumHistogramInterval* boundaryInterval;
	int nInitialBoundaryStartFrequency;
	int nBoundaryStartFrequency;
	int nBoundaryIntervalFrequency;
	Continuous cPreviousHistogramEpsilonBinLength;
	Continuous cHistogramEpsilonBinLength;
	Continuous cBoundaryEpsilonBinLength;
	Continuous cEmptyIntervalCenter;
	Continuous cMinValue;
	Continuous cMaxValue;
	ALString sTmp;

	require(cvSourceValues != NULL);
	require(oaDataSubsets != NULL);

	TraceBeginBlock("Build aggregated histogram");

	// Creation de l'histogramme en sortie
	aggregatedHistogram = new MHGenumHistogram;

	// Creation des histogrammes de travail
	outputSubHistogram = new MHGenumHistogram;
	outputBoundaryHistogram = new MHGenumHistogram;

	// Parcours des sous-ensembles de donnees pour agreger les histogrammes
	cPreviousHistogramEpsilonBinLength = 0;
	cHistogramEpsilonBinLength = 0;
	nTotalFrequency = 0;
	for (i = 0; i < oaDataSubsets->GetSize(); i++)
	{
		// Acces au sous-ensemble
		dataSubset = cast(MHDataSubset*, oaDataSubsets->GetAt(i));

		// Trace
		Trace(sTmp + "Data subset\t" + IntToString(i + 1) + "\tbuild output sub-histogram\t" +
		      IntToString(dataSubset->GetSize()) + "\t" + IntToString(dataSubset->GetFirstValueIndex()) + "\t" +
		      IntToString(dataSubset->GetLastValueIndex()) + "\t" +
		      IntToString(dataSubset->GetHistogram()->GetFrequencyVectorNumber()) +
		      " intervals, PWCH: " + BooleanToString(dataSubset->GetPWCH()));
		Trace(sTmp + "\tData subset boundary values (" +
		      KWContinuous::ContinuousToString(cvSourceValues->GetAt(dataSubset->GetFirstValueIndex())) + ", " +
		      KWContinuous::ContinuousToString(cvSourceValues->GetAt(dataSubset->GetLastValueIndex() - 1)) +
		      ")");

		// Creation d'un sous-histogramme en sortie pour le sous-ensemble de donnees
		BuildOutputHistogram(cvSourceValues, dataSubset->GetFirstValueIndex(), dataSubset->GetLastValueIndex(),
				     dataSubset->GetHistogram(), outputSubHistogram);
		assert(outputSubHistogram->ComputeTotalFrequency() == dataSubset->GetSize());

		// Mise a jour de l'effectif total traite
		nTotalFrequency += dataSubset->GetSize();

		// Memorisation du precedent epsilon bin length
		cPreviousHistogramEpsilonBinLength = cHistogramEpsilonBinLength;

		// Calcul du epsilon de precision dans le cas general
		cMinValue = outputSubHistogram->GetConstIntervalAt(0)->GetLowerValue();
		cMaxValue = outputSubHistogram->GetConstIntervalAt(outputSubHistogram->GetIntervalNumber() - 1)
				->GetUpperValue();
		cHistogramEpsilonBinLength = GetHistogramSpec()->ComputeEpsilonBinLength(
		    outputSubHistogram->ComputeTotalBinLength(), cMinValue, cMaxValue);
		assert(cHistogramEpsilonBinLength > 0);

		// Trace
		Trace(sTmp + "\tSub histogram boundaries (" +
		      KWContinuous::ContinuousToString(outputSubHistogram->GetConstIntervalAt(0)->GetLowerBound()) +
		      ", " +
		      KWContinuous::ContinuousToString(
			  outputSubHistogram->GetConstIntervalAt(outputSubHistogram->GetIntervalNumber() - 1)
			      ->GetUpperBound()) +
		      ") epsilonBin: " + KWContinuous::ContinuousToString(cHistogramEpsilonBinLength));

		// Annotation des intervalles par l'index du sous jeu de donnees
		for (n = 0; n < outputSubHistogram->GetIntervalNumber(); n++)
		{
			interval = cast(MHGenumHistogramInterval*, outputSubHistogram->GetIntervalAt(n));
			interval->SetDataSubsetIndex(i + 1);
			interval->SetPICH(not dataSubset->GetPWCH());
			interval->SetPICHSplitIndex(dataSubset->GetPICHSplitIndex());
		}

		// Affichage
		if (bDisplay)
			cout << "Data subset " << i << "\n" << *outputSubHistogram << endl;

		// Creation d'un sous-histogrammes en sortie pour la frontiere entre sous-ensemble de donnees
		if (i > 0)
		{
			assert(aggregatedHistogram->GetIntervalNumber() > 0);
			assert(cPreviousHistogramEpsilonBinLength > 0);

			// Recherche des deux intervalles utilises pour l'histogramme frontiere
			// Attention: le denier intervalle courant n'est pas necessairement le dernier intervalle
			// du sous-ensemble precedent, qui a peut-etre ete modifier a l'etape precedente
			lastPreviousInterval =
			    cast(MHGenumHistogramInterval*,
				 aggregatedHistogram->GetIntervalAt(aggregatedHistogram->GetIntervalNumber() - 1));
			firstNextInterval = cast(MHGenumHistogramInterval*, outputSubHistogram->GetIntervalAt(0));

			// Recherche des effectifs de ces deux intervalles
			nBoundaryFirstIntervalFrequency = lastPreviousInterval->GetFrequency();
			nBoundarySecondIntervalFrequency = firstNextInterval->GetFrequency();
			nBoundaryFrequency = nBoundaryFirstIntervalFrequency + nBoundarySecondIntervalFrequency;

			// Acces au sous-ensemble precedent
			previousDataSubset = cast(MHDataSubset*, oaDataSubsets->GetAt(i - 1));

			/////////////////////////////////////////////////////////////////////////////////////////
			// On retrouve l'histogramme frontiere initial
			// Potentiellement different de la fronyiere courante, car le precedent dernier intervalle
			// a potentiellement ete modifie

			// Recherche des caracteristiques de la frontiere initiale, avant agregation
			nInitialBoundaryFrequency =
			    ComputeHistogramTotalFrequency(dataSubset->GetPrevBoundaryHistogram());
			previousDataSubsetHistogramFrequencyTable = previousDataSubset->GetHistogram();
			initialLastPreviousHistogramVector =
			    cast(MHGenumHistogramVector*,
				 previousDataSubsetHistogramFrequencyTable->GetFrequencyVectorAt(
				     previousDataSubsetHistogramFrequencyTable->GetFrequencyVectorNumber() - 1));
			nInitialBoundaryFirstIntervalFrequency = initialLastPreviousHistogramVector->GetFrequency();
			assert(nInitialBoundaryFirstIntervalFrequency + nBoundarySecondIntervalFrequency ==
			       nInitialBoundaryFrequency);

			// Creation d'un sous-histogramme en sortie pour la frontiere entre sous-ensembles de donnees
			BuildOutputHistogram(cvSourceValues,
					     dataSubset->GetFirstValueIndex() - nInitialBoundaryFirstIntervalFrequency,
					     dataSubset->GetFirstValueIndex() + nBoundarySecondIntervalFrequency,
					     dataSubset->GetPrevBoundaryHistogram(), outputBoundaryHistogram);
			assert(outputBoundaryHistogram->ComputeTotalFrequency() == nInitialBoundaryFrequency);

			// Trace
			Trace(sTmp + "Data subset boundary\t" + IntToString(i) + "\t" + IntToString(i + 1) + "\t" +
			      IntToString(nInitialBoundaryFrequency) + " instances\t" +
			      IntToString(dataSubset->GetFirstValueIndex() - nBoundaryFirstIntervalFrequency) + "\t" +
			      IntToString(dataSubset->GetFirstValueIndex()) + "\t" +
			      IntToString(dataSubset->GetFirstValueIndex() + nBoundarySecondIntervalFrequency));
			Trace(sTmp + "\tData subset boundary values\t" +
			      KWContinuous::ContinuousToString(cvSourceValues->GetAt(dataSubset->GetFirstValueIndex() -
										     nBoundaryFirstIntervalFrequency)) +
			      "\t" +
			      KWContinuous::ContinuousToString(cvSourceValues->GetAt(
				  dataSubset->GetFirstValueIndex() + nBoundarySecondIntervalFrequency - 1)));
			Trace(sTmp + "\tInitial previous boundary histogram\t" +
			      IntToString(ComputeHistogramTotalFrequency(dataSubset->GetPrevBoundaryHistogram())) +
			      " instances\t" +
			      IntToString(dataSubset->GetPrevBoundaryHistogram()->GetFrequencyVectorNumber()) +
			      " intervals\t" +
			      IntToString(dataSubset->GetFirstValueIndex() - nInitialBoundaryFirstIntervalFrequency) +
			      "\t" + IntToString(dataSubset->GetFirstValueIndex()) + "\t" +
			      IntToString(dataSubset->GetFirstValueIndex() + nBoundarySecondIntervalFrequency));

			/////////////////////////////////////////////////////////////////////////////////////////
			// Recherche de l'intervalle de histogramme frontiere qui traverse la frontiere
			// entre les jeux de donnees

			// Recherche de l'intervalle frontiere
			boundaryInterval = NULL;
			nInitialBoundaryStartFrequency = 0;
			for (n = 0; n < outputBoundaryHistogram->GetIntervalNumber(); n++)
			{
				interval = cast(MHGenumHistogramInterval*, outputBoundaryHistogram->GetIntervalAt(n));

				// Cas ou on traverse la frontiere avec une marge de part et d'autre
				if (nInitialBoundaryStartFrequency < nInitialBoundaryFirstIntervalFrequency and
				    nInitialBoundaryStartFrequency + interval->GetFrequency() >
					nInitialBoundaryFirstIntervalFrequency)
				{
					boundaryInterval = interval;
					break;
				}
				// Cas ou on est sur la frontiere avec un intervalle vide
				else if (nInitialBoundaryStartFrequency == nInitialBoundaryFirstIntervalFrequency and
					 interval->GetFrequency() == 0)
				{
					assert(n > 0);
					assert(outputBoundaryHistogram->GetIntervalAt(n - 1)->GetFrequency() > 0);
					boundaryInterval = interval;
					break;
				}
				// Cas ou on termine sur la frontiere
				else if (nInitialBoundaryStartFrequency + interval->GetFrequency() ==
					 nInitialBoundaryFirstIntervalFrequency)
				{
					assert(n < outputBoundaryHistogram->GetIntervalNumber() - 1);
					nextInterval = cast(MHGenumHistogramInterval*,
							    outputBoundaryHistogram->GetIntervalAt(n + 1));

					// On doit prendre soit l'intervalle courant qui termine sur la frontiere, soit
					// le suivant qui commence sur la frontiere On prend celui qui englobe le plus
					// la frontiere
					if (interval->GetFrequency() == 0)
						boundaryInterval = interval;
					else if (nextInterval->GetFrequency() == 0)
						boundaryInterval = nextInterval;
					else if (interval->GetUpperBound() - interval->GetUpperValue() >=
						 nextInterval->GetLowerValue() - nextInterval->GetLowerBound())
						boundaryInterval = interval;
					else
						boundaryInterval = nextInterval;

					// Ne pas oublier de mettre a jour l'effectif avant l'intervalle
					if (boundaryInterval == nextInterval)
						nInitialBoundaryStartFrequency += interval->GetFrequency();
					break;
				}

				// Ajout de l'effectif de l'intervalle
				nInitialBoundaryStartFrequency += interval->GetFrequency();
			}
			assert(boundaryInterval != NULL);
			assert(nInitialBoundaryStartFrequency <= nInitialBoundaryFirstIntervalFrequency);
			assert(nInitialBoundaryStartFrequency + boundaryInterval->GetFrequency() >=
			       nInitialBoundaryFirstIntervalFrequency);
			assert(nInitialBoundaryStartFrequency + boundaryInterval->GetFrequency() <=
			       nInitialBoundaryFrequency);
			if (nBoundaryFrequency != nInitialBoundaryFrequency)
			{
				Trace(sTmp + "\tData subset initial boundary interval " + IntToString(i) + " and " +
				      IntToString(i + 1) + "\t" +
				      IntToString(dataSubset->GetFirstValueIndex() -
						  nInitialBoundaryFirstIntervalFrequency +
						  nInitialBoundaryStartFrequency) +
				      "\t" + IntToString(nInitialBoundaryStartFrequency) + "\t" +
				      IntToString(boundaryInterval->GetFrequency()) + "\t" +
				      boundaryInterval->GetObjectLabel());
			}

			// On indique que les deux intervalles concernes sont sur la frontiere
			lastPreviousInterval->SetBinLength(0);
			lastPreviousInterval->SetPreviousDataSubsetBoundary(true);
			firstNextInterval->SetBinLength(0);
			firstNextInterval->SetPreviousDataSubsetBoundary(true);

			// Initilisation de l'effectif et du debut de l'intervalle frontiere par rapport a la nouvelle
			// frontiere
			nBoundaryStartFrequency = nInitialBoundaryStartFrequency;
			nBoundaryIntervalFrequency = boundaryInterval->GetFrequency();

			// Si la frontiere s'est agrandie, il faut decaler d'autant le debut
			if (nBoundaryFrequency > nInitialBoundaryFrequency)
			{
				// Si le debut est strictement positif, on decale le debut
				if (nInitialBoundaryStartFrequency > 0)
				{
					nBoundaryStartFrequency += (nBoundaryFrequency - nInitialBoundaryFrequency);
					Trace("\t\tLarger boundary\tLarger boundary start");
				}
				// Sinon, il faut changer la taille de l'intervalle et mettre le debut a 0
				// En effet, on choisit ici de fusionner avec la difference de frontiere si
				// l'intervalle frontiere est le premier intervalle
				else
				{
					nBoundaryIntervalFrequency += (nBoundaryFrequency - nInitialBoundaryFrequency);
					nBoundaryStartFrequency = 0;
					Trace("\t\tLarger boundary\tLarger boundary frequency");
				}
			}
			// Si la frontiere a diminue
			else if (nBoundaryFrequency < nInitialBoundaryFrequency)
			{
				// Si le debut depasse la difference, seul le debut est impacte
				if (nInitialBoundaryStartFrequency >= (nInitialBoundaryFrequency - nBoundaryFrequency))
				{
					nBoundaryStartFrequency -= (nInitialBoundaryFrequency - nBoundaryFrequency);
					Trace("\t\tSmaller boundary\tSmaller boundary start");
				}
				// Sinon, il faut changer la taille de l'intervalle et mettre le debut a 0
				else
				{
					nBoundaryIntervalFrequency -=
					    ((nInitialBoundaryFrequency - nBoundaryFrequency) -
					     nInitialBoundaryStartFrequency);
					nBoundaryStartFrequency = 0;
					Trace("\t\tSmaller boundary\tSmaller boundary frequency");
				}
			}
			Trace(sTmp + "\tData subset boundary interval " + IntToString(i) + " and " +
			      IntToString(i + 1) + "\t" +
			      IntToString(dataSubset->GetFirstValueIndex() - nBoundaryFirstIntervalFrequency +
					  nBoundaryStartFrequency) +
			      "\t" + IntToString(nBoundaryStartFrequency) + "\t" +
			      IntToString(nBoundaryIntervalFrequency) + "\t" + boundaryInterval->GetObjectLabel());
			assert(nBoundaryIntervalFrequency >= 0);
			assert(nBoundaryStartFrequency >= 0);
			assert(nBoundaryStartFrequency <= nBoundaryFirstIntervalFrequency);
			assert(nBoundaryStartFrequency + nBoundaryIntervalFrequency >= nBoundaryFirstIntervalFrequency);
			assert(nBoundaryStartFrequency + nBoundaryIntervalFrequency <= nBoundaryFrequency);

			// Calcul de l'index global du debut de l'intervalle frontiere
			nBoundaryIntervalIndex = dataSubset->GetFirstValueIndex() - nBoundaryFirstIntervalFrequency +
						 nBoundaryStartFrequency;
			assert(nBoundaryIntervalIndex <= dataSubset->GetFirstValueIndex());
			assert(nBoundaryIntervalIndex + nBoundaryIntervalFrequency >= dataSubset->GetFirstValueIndex());

			///////////////////////////////////////////////////////////////////////////////////
			// Ajout d'un intervalle s'il demarre strictement apres le debut de l'intervalle precedent
			// et termine strictement avant la fin de l'intervalle suivant
			if (nBoundaryStartFrequency > 0 and
			    nBoundaryStartFrequency + nBoundaryIntervalFrequency < nBoundaryFrequency)
			{
				assert(nBoundaryIntervalIndex > 0);
				Trace("\tNew boundary interval");

				// Ajout de l'intervalle en dupliquant l'intervalle frontiere
				boundaryInterval = cast(MHGenumHistogramInterval*, boundaryInterval->Clone());
				aggregatedHistogram->GetIntervals()->Add(boundaryInterval);
				boundaryInterval->SetDataSubsetIndex(i + 1);
				boundaryInterval->SetBinLength(0);
				boundaryInterval->SetFrequency(nBoundaryIntervalFrequency);
				boundaryInterval->SetPreviousDataSubsetBoundary(true);

				// On modifie les caracteristiques de l'intervalle precedent
				lastPreviousInterval->SetUpperValue(cvSourceValues->GetAt(nBoundaryIntervalIndex - 1));
				lastPreviousInterval->SetFrequency(nBoundaryStartFrequency);
				assert(lastPreviousInterval->GetFrequency() > 0);

				// On modifie les caracteristiques de l'intervalle suivant
				firstNextInterval->SetLowerValue(
				    cvSourceValues->GetAt(nBoundaryIntervalIndex + nBoundaryIntervalFrequency));
				firstNextInterval->SetFrequency(nBoundaryFrequency - nBoundaryStartFrequency -
								nBoundaryIntervalFrequency);
				assert(lastPreviousInterval->GetFrequency() + boundaryInterval->GetFrequency() +
					   firstNextInterval->GetFrequency() ==
				       nBoundaryFrequency);
				assert(firstNextInterval->GetFrequency() > 0);

				// Cas particulier ou l'intervalle central est vide
				if (boundaryInterval->GetFrequency() == 0)
				{
					assert(boundaryInterval->GetLowerValue() == KWContinuous::GetMissingValue());
					assert(boundaryInterval->GetUpperValue() == KWContinuous::GetMissingValue());
					assert(nBoundaryFirstIntervalFrequency - nBoundaryStartFrequency == 0);

					// Calcul du centre de l'intervalle vide
					cEmptyIntervalCenter = (lastPreviousInterval->GetUpperValue() +
								firstNextInterval->GetLowerValue()) /
							       2;
					assert(lastPreviousInterval->GetUpperValue() < cEmptyIntervalCenter and
					       cEmptyIntervalCenter < firstNextInterval->GetLowerValue());

					// Modification de la borne sup du premier intervalle
					cBoundaryEpsilonBinLength =
					    min(cPreviousHistogramEpsilonBinLength,
						(cEmptyIntervalCenter - lastPreviousInterval->GetUpperValue()) / 2);
					lastPreviousInterval->SetUpperBound(lastPreviousInterval->GetUpperValue() +
									    cBoundaryEpsilonBinLength / 2);

					// Modification de la borne inf du dernier intervalle
					cBoundaryEpsilonBinLength =
					    min(cHistogramEpsilonBinLength,
						(firstNextInterval->GetLowerValue() - cEmptyIntervalCenter) / 2);
					firstNextInterval->SetLowerBound(firstNextInterval->GetLowerValue() -
									 cBoundaryEpsilonBinLength / 2);
					assert(lastPreviousInterval->GetUpperBound() <
					       firstNextInterval->GetLowerBound());

					// Modification des bornes de l'intervalle central
					boundaryInterval->SetLowerBound(lastPreviousInterval->GetUpperBound());
					boundaryInterval->SetUpperBound(firstNextInterval->GetLowerBound());
				}
				// Cas general sinon
				else
				{
					// Modification des bornes entre les deux premiers intervalles
					ComputeBoundaryIntervalBounds(
					    cvSourceValues, dataSubset,
					    nBoundaryFirstIntervalFrequency - nBoundaryStartFrequency,
					    cPreviousHistogramEpsilonBinLength, cHistogramEpsilonBinLength,
					    lastPreviousInterval, boundaryInterval);

					// Modification des bornes entre les deux derniers intervalles
					ComputeBoundaryIntervalBounds(
					    cvSourceValues, dataSubset,
					    nBoundaryFirstIntervalFrequency - nBoundaryStartFrequency,
					    cPreviousHistogramEpsilonBinLength, cHistogramEpsilonBinLength,
					    boundaryInterval, firstNextInterval);
				}

				// Trace
				Trace(sTmp + "\tBoundary interval 1/3: " + lastPreviousInterval->GetObjectLabel());
				Trace(sTmp + "\tBoundary interval 2/3: " + boundaryInterval->GetObjectLabel());
				Trace(sTmp + "\tBoundary interval 3/3: " + firstNextInterval->GetObjectLabel());
			}
			///////////////////////////////////////////////////////////////////////////////////
			// Supression d'un intervalle si l'intervalle frontiere englobe tout
			else if (nBoundaryIntervalFrequency == nBoundaryFrequency)
			{
				assert(nBoundaryStartFrequency == 0);
				Trace("\tMerged boundary interval");

				// On remplace la borne inf et les caracteristiques associees du premier intervalle
				// suivant
				firstNextInterval->SetLowerBound(lastPreviousInterval->GetLowerBound());
				firstNextInterval->SetLowerValue(lastPreviousInterval->GetLowerValue());
				firstNextInterval->SetFrequency(nBoundaryFrequency);
				assert(firstNextInterval->GetFrequency() > 0);

				// On supprime le dernier intervalle courant et l'intervalle frontiere
				aggregatedHistogram->GetIntervals()->SetSize(
				    aggregatedHistogram->GetIntervals()->GetSize() - 1);
				delete lastPreviousInterval;

				// Trace
				Trace(sTmp + "\tBoundary interval 1/1: " + firstNextInterval->GetObjectLabel());
			}
			///////////////////////////////////////////////////////////////////////////////////
			// Modification des frontieres des intervalles sinon
			else
			{
				// Cas ou l'intervalle frontiere est strictement au debut
				if (nBoundaryStartFrequency == 0)
				{
					assert(nBoundaryStartFrequency + nBoundaryIntervalFrequency <
					       nBoundaryFrequency);
					assert(nBoundaryIntervalFrequency < nBoundaryFrequency);
					Trace("\tLeft modified boundary interval");

					// On modifie les autres caracteristiques de l'intervalle precedent
					lastPreviousInterval->SetUpperValue(boundaryInterval->GetUpperValue());
					lastPreviousInterval->SetFrequency(nBoundaryIntervalFrequency);

					// On modifie les caracteristiques de l'intervalle suivant
					firstNextInterval->SetLowerValue(
					    cvSourceValues->GetAt(nBoundaryIntervalIndex + nBoundaryIntervalFrequency));
					firstNextInterval->SetFrequency(nBoundaryFrequency -
									nBoundaryIntervalFrequency);
				}
				// Cas ou l'intervalle frontiere est strictement a la fin
				else
				{
					assert(nBoundaryStartFrequency > 0);
					assert(nBoundaryIntervalFrequency < nBoundaryFrequency);
					assert(nBoundaryIntervalIndex > 0);
					Trace("\tRight modified boundary interval");

					// On modifie les autres caracteristiques de l'intervalle precedent
					lastPreviousInterval->SetUpperValue(
					    cvSourceValues->GetAt(nBoundaryIntervalIndex - 1));
					lastPreviousInterval->SetFrequency(nBoundaryFrequency -
									   nBoundaryIntervalFrequency);

					// On modifie les caracteristiques de l'intervalle suivant
					firstNextInterval->SetLowerValue(boundaryInterval->GetLowerValue());
					firstNextInterval->SetFrequency(nBoundaryIntervalFrequency);
				}

				// Les intervalles de debut et de fin ne peut etre vides
				assert(lastPreviousInterval->GetFrequency() > 0);
				assert(firstNextInterval->GetFrequency() > 0);

				// Modification des bornes en fonction de la densite respective des intervalles
				// et du plus petit des epsilon-bin
				ComputeBoundaryIntervalBounds(cvSourceValues, dataSubset,
							      nBoundaryFirstIntervalFrequency - nBoundaryStartFrequency,
							      cPreviousHistogramEpsilonBinLength,
							      cHistogramEpsilonBinLength, lastPreviousInterval,
							      firstNextInterval);

				// Trace
				Trace(sTmp + "\tBoundary interval 1/2: " + lastPreviousInterval->GetObjectLabel());
				Trace(sTmp + "\tBoundary interval 2/2: " + firstNextInterval->GetObjectLabel());
			}
		}

		// Destruction des intervalles de l'histogramme frontiere
		outputBoundaryHistogram->GetIntervals()->DeleteAll();

		// Transfert des intervalles du sous-histogramme vers l'histogramme final
		aggregatedHistogram->GetIntervals()->InsertObjectArrayAt(aggregatedHistogram->GetIntervalNumber(),
									 outputSubHistogram->GetIntervals());
		outputSubHistogram->GetIntervals()->RemoveAll();
		assert(aggregatedHistogram->ComputeTotalFrequency() == nTotalFrequency);
	}
	assert(aggregatedHistogram->ComputeTotalFrequency() == cvSourceValues->GetSize());
	assert(aggregatedHistogram->Check());
	assert(aggregatedHistogram->CheckValues(cvSourceValues));

	// Nettoyage
	outputSubHistogram->GetIntervals()->RemoveAll();
	delete outputSubHistogram;
	delete outputBoundaryHistogram;
	TraceEndBlock("Build aggregated histogram");
}

int MHDiscretizerGenumHistogram::BuildBoundaryHistogram(const ContinuousVector* cvSourceValues,
							MHDataSubset* dataSubset1, MHDataSubset* dataSubset2) const
{
	ContinuousVector cvDataSubsetValues;
	int n;
	MHGenumHistogramVector* histogramVector;
	const KWFrequencyTable* histogramFrequencyTable;
	KWFrequencyTable* boundaryHistogramFrequencyTable;
	int nPreviousHistogramLastIntervalFirstValueIndex;
	int nHistogramFirstIntervalLastValueIndex;

	require(cvSourceValues != NULL);
	require(dataSubset1 != NULL);
	require(dataSubset2 != NULL);
	require(dataSubset1->GetLastValueIndex() == dataSubset2->GetFirstValueIndex());
	require(dataSubset1->GetHistogram() != NULL);
	require(dataSubset2->GetHistogram() != NULL);

	// Index de la premiere instance du dernier intervalle de l'histogramme precedent
	histogramFrequencyTable = dataSubset1->GetHistogram();
	histogramVector = cast(MHGenumHistogramVector*, histogramFrequencyTable->GetFrequencyVectorAt(
							    histogramFrequencyTable->GetFrequencyVectorNumber() - 1));
	nPreviousHistogramLastIntervalFirstValueIndex =
	    dataSubset1->GetLastValueIndex() - histogramVector->GetFrequency();
	assert(nPreviousHistogramLastIntervalFirstValueIndex < dataSubset1->GetLastValueIndex());

	// Index de la dernier instance du premier intervalle de l'histogramme courant
	histogramFrequencyTable = dataSubset2->GetHistogram();
	histogramVector = cast(MHGenumHistogramVector*, histogramFrequencyTable->GetFrequencyVectorAt(0));
	nHistogramFirstIntervalLastValueIndex = dataSubset2->GetFirstValueIndex() + histogramVector->GetFrequency();
	assert(dataSubset2->GetFirstValueIndex() < nHistogramFirstIntervalLastValueIndex);

	// Extraction des valeurs
	cvDataSubsetValues.SetSize(nHistogramFirstIntervalLastValueIndex -
				   nPreviousHistogramLastIntervalFirstValueIndex);
	for (n = 0; n < cvDataSubsetValues.GetSize(); n++)
		cvDataSubsetValues.SetAt(n, cvSourceValues->GetAt(nPreviousHistogramLastIntervalFirstValueIndex + n));

	// Construction d'un histogramme
	GetHistogramSpec()->SetOutlierBoundary(true);
	TypicalDiscretizeValues(&cvDataSubsetValues, boundaryHistogramFrequencyTable);
	GetHistogramSpec()->SetOutlierBoundary(false);

	// Memorisation de l'histogramme
	dataSubset2->SetPrevBoundaryHistogram(boundaryHistogramFrequencyTable);
	return 0;
}

void MHDiscretizerGenumHistogram::ComputeBoundaryIntervalBounds(const ContinuousVector* cvSourceValues,
								MHDataSubset* dataSubset, int nBoundaryDistance,
								double dPreviousEpsilonBinLength,
								double dEpsilonBinLength,
								MHHistogramInterval* firstBoundaryInterval,
								MHHistogramInterval* secondBoundaryInterval) const
{
	int nCompareDensity;
	Continuous cDefaultBound;
	double dBoundEpsilonBinLength;
	Continuous cNewUpperBound;
	ALString sTmp;

	require(cvSourceValues != NULL);
	require(dataSubset != NULL);
	require(nBoundaryDistance >= 0);
	require(dPreviousEpsilonBinLength > 0);
	require(dEpsilonBinLength > 0);
	require(firstBoundaryInterval != NULL);
	require(secondBoundaryInterval != NULL);
	require(firstBoundaryInterval->GetFrequency() > 0);
	require(secondBoundaryInterval->GetFrequency() > 0);
	require(firstBoundaryInterval->GetUpperValue() < secondBoundaryInterval->GetLowerValue());

	// Trace
	Trace(sTmp + "\tCompute boundary intervals\t" + IntToString(dataSubset->GetFirstValueIndex()) + "\t" +
	      IntToString(nBoundaryDistance) + "\t" + IntToString(firstBoundaryInterval->GetFrequency()) + "\t" +
	      IntToString(secondBoundaryInterval->GetFrequency()));

	// Comparaison de la densite des intervalles
	nCompareDensity = firstBoundaryInterval->CompareDensity(secondBoundaryInterval);

	// Calcul de la borne par defaut entre les intervalles
	cDefaultBound = KWContinuous::GetLowerMeanValue(firstBoundaryInterval->GetUpperValue(),
							secondBoundaryInterval->GetLowerValue());

	// Recherche du epsilon a utiliser pour la borne
	if (nBoundaryDistance == 0)
		dBoundEpsilonBinLength = min(dPreviousEpsilonBinLength, dEpsilonBinLength);
	else if (nBoundaryDistance < 0)
		dBoundEpsilonBinLength = dPreviousEpsilonBinLength;
	else
		dBoundEpsilonBinLength = dEpsilonBinLength;

	// Si meme densite (cela ne devrait par arriver en theorie, sauf effet de bord)
	if (nCompareDensity == 0)
		cNewUpperBound = cDefaultBound;
	// Sinon, on prend la frontiere la plus proche de l'intervalle le plus dense
	else if (nCompareDensity > 0)
	{
		cNewUpperBound = firstBoundaryInterval->GetUpperValue() + dBoundEpsilonBinLength / 2;
		cNewUpperBound = min(cNewUpperBound, cDefaultBound);
	}
	else
	{
		cNewUpperBound = secondBoundaryInterval->GetLowerValue() - dBoundEpsilonBinLength / 2;
		if (cNewUpperBound == secondBoundaryInterval->GetLowerValue())
			cNewUpperBound = cDefaultBound;
		else
			cNewUpperBound = max(cNewUpperBound, cDefaultBound);
	}
	assert(cNewUpperBound < secondBoundaryInterval->GetLowerValue());

	// Modification des bornes des intervalles frontierres
	firstBoundaryInterval->SetUpperBound(cNewUpperBound);
	secondBoundaryInterval->SetLowerBound(cNewUpperBound);
	ensure(firstBoundaryInterval->GetUpperValue() <= firstBoundaryInterval->GetUpperBound());
	ensure(secondBoundaryInterval->GetLowerBound() <= secondBoundaryInterval->GetLowerValue());
}

void MHDiscretizerGenumHistogram::DisplayDataSubsetsArray(const ALString& sTitle, ObjectArray* oaDataSubsets,
							  ostream& ost) const
{
	MHDataSubset* dataSubset;
	int i;

	require(oaDataSubsets != NULL);

	// Parcours des sous-ensembles
	ost << sTitle;
	for (i = 0; i < oaDataSubsets->GetSize(); i++)
	{
		dataSubset = cast(MHDataSubset*, oaDataSubsets->GetAt(i));
		assert(i == 0 or dataSubset->GetFirstValueIndex() ==
				     cast(MHDataSubset*, oaDataSubsets->GetAt(i - 1))->GetLastValueIndex());

		// Affichage de l'entete
		if (i == 0)
		{
			dataSubset->WriteHeaderLine(ost);
			ost << "\n";
		}

		// Afficahge de la ligne
		ost << *dataSubset << "\n";
	}
}

boolean MHDiscretizerGenumHistogram::IsDatasetPWCH(const ContinuousVector* cvSourceValues, int nTotalBinNumber) const
{
	return not IsDatasetPICH(cvSourceValues, nTotalBinNumber);
}

boolean MHDiscretizerGenumHistogram::IsDatasetPICH(const ContinuousVector* cvSourceValues, int nTotalBinNumber) const
{
	return IsDataSubsetPICH(cvSourceValues, 0, cvSourceValues->GetSize(), nTotalBinNumber);
}

boolean MHDiscretizerGenumHistogram::IsDataSubsetPWCH(const ContinuousVector* cvSourceValues, int nFirstIndex,
						      int nLastIndex, int nTotalBinNumber) const
{
	return not IsDataSubsetPICH(cvSourceValues, nFirstIndex, nLastIndex, nTotalBinNumber);
}

boolean MHDiscretizerGenumHistogram::IsDataSubsetPICH(const ContinuousVector* cvSourceValues, int nFirstIndex,
						      int nLastIndex, int nTotalBinNumber) const
{
	boolean bDisplay = false;
	boolean bIsPICH;
	double dThresholdPICH;
	Continuous cMin;
	Continuous cMax;
	double dEpsilonBinLength;
	double dBinStart;
	Continuous cX;
	int nBinIndex;
	int nLastBinIndex;
	int nLastBinFrequency;
	int nLastBinDistinctValueNumber;
	Continuous cLastX;
	int n;

	require(cvSourceValues != NULL);
	require(0 <= nFirstIndex and nFirstIndex < nLastIndex);
	require(nLastIndex <= cvSourceValues->GetSize());
	require(nFirstIndex == 0 or cvSourceValues->GetAt(nFirstIndex - 1) < cvSourceValues->GetAt(nFirstIndex));
	require(nLastIndex == cvSourceValues->GetSize() or
		cvSourceValues->GetAt(nLastIndex - 1) < cvSourceValues->GetAt(nLastIndex));
	require(nTotalBinNumber >= 2);

	// Calcul du seuil d'effectif maximum par bin contenant des valeurs distinctes
	bIsPICH = false;
	dThresholdPICH = GetHistogramSpec()->GetOutlierMaxBinFrequency(nLastIndex - nFirstIndex + 1);

	// Calcul du epsilon de precision
	cMin = cvSourceValues->GetAt(nFirstIndex);
	cMax = cvSourceValues->GetAt(nLastIndex - 1);
	dEpsilonBinLength = GetHistogramSpec()->ComputeEpsilonBinLength(nTotalBinNumber, cMin, cMax);
	dBinStart = GetHistogramSpec()->ComputeHistogramLowerBound(nTotalBinNumber, cMin, cMax);
	assert(dEpsilon > 0);
	if (bDisplay)
		cout << "IsDataPICH\t" << nLastIndex - nFirstIndex << "\t" << nTotalBinNumber << "\t"
		     << dEpsilonBinLength << endl;

	// Initialisation des caracteristiques liee a la premiere valeur se trouvant dans le premier bin
	cLastX = cvSourceValues->GetAt(nFirstIndex);
	nLastBinIndex = 0;
	nLastBinFrequency = 1;
	nLastBinDistinctValueNumber = 1;

	// Comptage du nombre d'intervalles finaux, en tenant compte que plusieurs
	// valeurs distinctes peuvent tomber dans le meme bin elementaire
	for (n = nFirstIndex + 1; n < nLastIndex; n++)
	{
		// Rercherche de la valeur et de son index de bin
		cX = cvSourceValues->GetAt(n);
		nBinIndex = ComputeBinIndex(cX, dBinStart, dEpsilonBinLength, cMin, cMax, nTotalBinNumber);
		assert(cLastX <= cX);
		assert(nLastBinIndex <= nBinIndex);
		assert(nBinIndex < nTotalBinNumber);

		// Cas sans changement de bin
		if (nBinIndex == nLastBinIndex)
		{
			nLastBinFrequency++;
			if (cX > cLastX)
				nLastBinDistinctValueNumber++;
		}

		// Affichage des caracteristiques du bin courant
		if (bDisplay)
		{
			cout << "\t" << n << "\t" << cX << "\t" << nBinIndex << "\t" << nLastBinIndex << "\t"
			     << nLastBinFrequency << "\t"
			     << "\t" << nLastBinDistinctValueNumber << "\t" << cLastX << "\n";
		}

		// Si changement de bin ou fin du dernier bin
		if (nBinIndex > nLastBinIndex or n == nLastIndex - 1)
		{
			// Test si declenchement du seuil
			if (nLastBinDistinctValueNumber >= 2 and nLastBinFrequency >= dThresholdPICH)
			{
				bIsPICH = true;
				break;
			}

			// Reinitialisation du bin suivant
			nLastBinIndex = nBinIndex;
			nLastBinFrequency = 1;
			nLastBinDistinctValueNumber = 1;
		}

		// Preparation du bin suivant
		cLastX = cX;
	}
	return bIsPICH;
}

void MHDiscretizerGenumHistogram::ComputeDataLogTrParameters(const ContinuousVector* cvSourceValues, Continuous& cMinM,
							     Continuous& cMinP, Continuous& cMimDeltaLogM,
							     Continuous& cMimDeltaLogP) const
{
	boolean bComputeFromDataset = false;
	int n;
	Continuous cX;
	Continuous cXPrev;

	require(cvSourceValues != NULL);

	// Calcul des parametres a partir de la base
	if (bComputeFromDataset)
	{
		// Initialisation de valeurs par defaut
		cMinM = KWContinuous::GetMaxValue();
		cMinP = KWContinuous::GetMaxValue();
		cMimDeltaLogM = KWContinuous::GetMaxValue();
		cMimDeltaLogP = KWContinuous::GetMaxValue();

		// Calcul des valeurs minimale autour de zero, et des differences de log de valeurs successives
		for (n = 0; n < cvSourceValues->GetSize(); n++)
		{
			// Mise a jour des statistiques sur les points les plus proches de 0
			cX = cvSourceValues->GetAt(n);
			if (cX < 0)
				cMinM = min(cMinM, -cX);
			else if (cX > 0)
				cMinP = min(cMinP, cX);

			// Mise a jour des statistiques sur les differences de valeurs les plus petites
			if (n > 0)
			{
				cXPrev = cvSourceValues->GetAt(n - 1);
				assert(cXPrev <= cX);
				if (cXPrev < cX)
				{
					if (cX < 0)
						cMimDeltaLogM = min(cMimDeltaLogM, log(-cXPrev) - log(-cX));
					else if (cXPrev > 0)
						cMimDeltaLogP = min(cMimDeltaLogP, log(cX) - log(cXPrev));
				}
			}
		}

		// On prend les parametres par defaut si on a pa pu les alimenter de puis la base
		if (cMinM == KWContinuous::GetMaxValue())
			cMinM = KWContinuous::GetEpsilonValue();
		if (cMinP == KWContinuous::GetMaxValue())
			cMinP = KWContinuous::GetEpsilonValue();
		if (cMimDeltaLogM == KWContinuous::GetMaxValue())
			cMimDeltaLogM = pow(10, -KWContinuous::GetDigitNumber());
		if (cMimDeltaLogP == KWContinuous::GetMaxValue())
			cMimDeltaLogP = pow(10, -KWContinuous::GetDigitNumber());
	}
	// Utilisation de parametres generiques valides pour tous les continus
	// Plus performant pour la detection d'outliers, une seule valeur extreme negative sera positionnee loin de 0
	else
	{
		cMinM = KWContinuous::GetEpsilonValue();
		cMinP = KWContinuous::GetEpsilonValue();
		cMimDeltaLogM = pow(10, -KWContinuous::GetDigitNumber());
		cMimDeltaLogP = pow(10, -KWContinuous::GetDigitNumber());
	}
	assert(cMinM > 0 and cMinP > 0);
	assert(cMimDeltaLogM > 0 and cMimDeltaLogP > 0);
}

Continuous MHDiscretizerGenumHistogram::LogTr(Continuous cX, Continuous cMinM, Continuous cMinP,
					      Continuous cMimDeltaLogM, Continuous cMimDeltaLogP) const
{
	Continuous cLogTrX;

	require(cX != KWContinuous::GetMissingValue());
	require(cMinM > 0 and cMinP > 0);
	require(cMimDeltaLogM > 0 and cMimDeltaLogP > 0);

	// Calcul de la log transformation de la valeur en entree
	if (cX < 0)
		cLogTrX = -cMimDeltaLogM - (log(-cX) - log(cMinM));
	else if (cX > 0)
		cLogTrX = cMimDeltaLogP + (log(cX) - log(cMinP));
	else
		cLogTrX = 0;

	// On convertit en valeur Continuous
	cLogTrX = KWContinuous::DoubleToContinuous(cLogTrX);
	return cLogTrX;
}

Continuous MHDiscretizerGenumHistogram::InvLogTr(Continuous cLogTrX, Continuous cMinM, Continuous cMinP,
						 Continuous cMimDeltaLogM, Continuous cMimDeltaLogP) const
{
	Continuous cX;

	require(cLogTrX != KWContinuous::GetMissingValue());
	require(cMinM > 0 and cMinP > 0);
	require(cMimDeltaLogM > 0 and cMimDeltaLogP > 0);

	// Calcul de la log transformation de la valeur en entree
	if (cLogTrX < 0)
		cX = -exp(-cLogTrX - cMimDeltaLogM + log(cMinM));
	else if (cLogTrX > 0)
		cX = exp(cLogTrX - cMimDeltaLogP + log(cMinP));
	else
		cX = 0;

	// On convertit en valeur Continuous
	cX = KWContinuous::DoubleToContinuous(cX);
	return cX;
}

void MHDiscretizerGenumHistogram::ComputeLogTrData(const ContinuousVector* cvSourceValues,
						   ContinuousVector* cvLogTrValues) const
{
	boolean bDisplay = false;
	Continuous cMinM;
	Continuous cMinP;
	Continuous cMimDeltaLogM;
	Continuous cMimDeltaLogP;
	int n;
	Continuous cX;
	Continuous cLogTrX;
	debug(Continuous cInvLogTrX);

	require(cvSourceValues != NULL);
	require(cvLogTrValues != NULL);

	// Calcul des parametre de log transformation
	ComputeDataLogTrParameters(cvSourceValues, cMinM, cMinP, cMimDeltaLogM, cMimDeltaLogP);
	if (bDisplay)
	{
		cout << "TestLogTransformation\t" << cvSourceValues->GetSize() << "\n";
		cout << "\tcMinM\t" << cMinM << "\n";
		cout << "\tcMinP\t" << cMinP << "\n";
		cout << "\tcMimDeltaLogM\t" << cMimDeltaLogM << "\n";
		cout << "\tcMimDeltaLogP\t" << cMimDeltaLogP << "\n";
	}

	// Parcours de toutes les donnees en les log-transformant et inversement
	cvLogTrValues->SetSize(cvSourceValues->GetSize());
	for (n = 0; n < cvSourceValues->GetSize(); n++)
	{
		cX = cvSourceValues->GetAt(n);
		cLogTrX = LogTr(cX, cMinM, cMinP, cMimDeltaLogM, cMimDeltaLogP);
		cvLogTrValues->SetAt(n, cLogTrX);

		// Verification
		debug(cInvLogTrX = InvLogTr(cLogTrX, cMinM, cMinP, cMimDeltaLogM, cMimDeltaLogP));
		debug(assert(fabs(cX - cInvLogTrX) <= 1e-7 * max(fabs(cX), fabs(cInvLogTrX))));
	}
}

void MHDiscretizerGenumHistogram::ExportLogTrData(const ContinuousVector* cvSourceValues, const ALString& sFileName,
						  const ALString& sInitialLabel, const ALString& sLogTrLabel) const
{
	fstream fLogTrData;
	boolean bOk;
	Continuous cMinM;
	Continuous cMinP;
	Continuous cMimDeltaLogM;
	Continuous cMimDeltaLogP;
	int n;
	Continuous cX;
	Continuous cLogTrX;
	ALString sTmp;

	require(cvSourceValues != NULL);

	// Ouverture du fichier en sortie
	bOk = FileService::OpenOutputFile(sFileName, fLogTrData);
	if (bOk)
	{
		// Ecriture de l'entete
		if (sInitialLabel != "")
			fLogTrData << sInitialLabel;
		if (sInitialLabel != "" and sLogTrLabel != "")
			fLogTrData << "\t";
		if (sLogTrLabel != "")
			fLogTrData << sLogTrLabel;
		fLogTrData << "\n";

		// Calcul des parametre de log transformation
		ComputeDataLogTrParameters(cvSourceValues, cMinM, cMinP, cMimDeltaLogM, cMimDeltaLogP);

		// Parcours de toutes les donnees en les log-transformant et inversement
		for (n = 0; n < cvSourceValues->GetSize(); n++)
		{
			cX = cvSourceValues->GetAt(n);
			cLogTrX = LogTr(cX, cMinM, cMinP, cMimDeltaLogM, cMimDeltaLogP);

			// Ecriture des resultats
			if (sInitialLabel != "")
				fLogTrData << cX;
			if (sInitialLabel != "" and sLogTrLabel != "")
				fLogTrData << "\t";
			if (sLogTrLabel != "")
				fLogTrData << cLogTrX;
			fLogTrData << "\n";
		}

		// Fermeture du fichier
		FileService::CloseOutputFile(sFileName, fLogTrData);

		// Trace
		Trace("LogTr(value) file\t" + sFileName);
		Trace(sTmp + "MinM\t" + KWContinuous::ContinuousToString(cMinM));
		Trace(sTmp + "MinP\t" + KWContinuous::ContinuousToString(cMinP));
		Trace(sTmp + "MimDeltaLogM\t" + KWContinuous::ContinuousToString(cMimDeltaLogM));
		Trace(sTmp + "MimDeltaLogP\t" + KWContinuous::ContinuousToString(cMimDeltaLogP));
	}
}

void MHDiscretizerGenumHistogram::ExportData(const ContinuousVector* cvSourceValues, const ALString& sFileName) const
{
	fstream fData;
	boolean bOk;
	int n;
	Continuous cX;

	require(cvSourceValues != NULL);

	// Ouverture du fichier en sortie
	bOk = FileService::OpenOutputFile(sFileName, fData);
	if (bOk)
	{
		// Entete
		fData << "X\n";

		// Parcours de toutes les donnees en les log-transformant et inversement
		for (n = 0; n < cvSourceValues->GetSize(); n++)
		{
			cX = cvSourceValues->GetAt(n);
			fData << KWContinuous::ContinuousToString(cX) << "\n";
		}

		// Fermeture du fichier
		FileService::CloseOutputFile(sFileName, fData);
	}
}

void MHDiscretizerGenumHistogram::TruncationPostOptimization(
    const ContinuousVector* cvSourceValues, const KWFrequencyTable* histogramFrequencyTable,
    KWFrequencyTable*& postOptimizedlHistogramFrequencyTable) const
{
	KWFrequencyTable* initialTruncatedHistogramFrequencyTable;
	Continuous cTruncationEpsilon;
	Continuous cMin;
	Continuous cMax;
	double dTruncationTotalBinNumber;
	int nTruncationTotalBinNumber;

	require(cvSourceValues != NULL);
	require(histogramFrequencyTable != NULL);
	require(GetHistogramSpec()->GetTruncationManagementHeuristic());

	TraceBeginBlock("Truncation management heuristic");

	// On determine s'il faut les traiter les donnees tronquees
	cTruncationEpsilon = ComputeTruncationEpsilon(cvSourceValues, histogramFrequencyTable);

	// On recalcule la discretisation uniquement si necessaire
	postOptimizedlHistogramFrequencyTable = NULL;
	if (cTruncationEpsilon > 0)
	{
		// Calcul d'un nombre total de bin correspondant au epsilon de troncature
		cMin = cvSourceValues->GetAt(0);
		cMax = cvSourceValues->GetAt(cvSourceValues->GetSize() - 1);
		dTruncationTotalBinNumber = (cMax - cMin) / cTruncationEpsilon;
		assert(dTruncationTotalBinNumber < GetHistogramSpec()->GetMaxEpsilonBinNumber());
		nTruncationTotalBinNumber = int(floor(0.5 + dTruncationTotalBinNumber)) + 1;

		// On modifie temporairement le parametrage de la discretisation
		assert(not GetHistogramSpec()->GetTruncationMode());
		GetHistogramSpec()->SetTruncationManagementHeuristic(false);
		GetHistogramSpec()->SetTruncationMode(true);

		// Calcul de la table initiale
		ComputeInitialFrequencyTable(cvSourceValues, nTruncationTotalBinNumber,
					     initialTruncatedHistogramFrequencyTable);

		// Appel de la methode de discretisation MODL
		if (GetHistogramSpec()->GetGranularizedModel())
			GranularizedDiscretizeValues(cvSourceValues, initialTruncatedHistogramFrequencyTable,
						     postOptimizedlHistogramFrequencyTable);
		else
			StandardDiscretizeValues(cvSourceValues, initialTruncatedHistogramFrequencyTable,
						 postOptimizedlHistogramFrequencyTable);

		// Nettoyage
		delete initialTruncatedHistogramFrequencyTable;

		// Memorisation de l'histogramme dans un fichier si les logs sont demandes
		if (GetHistogramSpec()->GetExportInternalLogFiles())
		{
			WriteHistogramFrequencyTableFile("Optimized truncation histogram\n", cvSourceValues,
							 postOptimizedlHistogramFrequencyTable,
							 GetHistogramSpec()->GetInternalHistogramFileName());
		}

		// Reinitialisation du parametrage
		GetHistogramSpec()->SetTruncationManagementHeuristic(true);
		GetHistogramSpec()->SetTruncationMode(false);
	}
	TraceEndBlock("Truncation management heuristic");
}

void MHDiscretizerGenumHistogram::DiscretizeDeltaValues(const ContinuousVector* cvSourceValues,
							ContinuousVector* cvSourceDeltaValues,
							KWFrequencyTable*& optimizedDeltaValueHistogramFrequencyTable,
							int& nLargestNonEmptyIntervalBinLength) const
{
	boolean bDisplay = false;
	boolean bExportDeltaValues = false;
	int nTotalBinNumber;
	KWFrequencyTable* initialHistogramFrequencyTable;
	int n;

	require(cvSourceValues != NULL);

	TraceBeginBlock("Compute histogram on delta values");

	// Calcul du vecteur des differences de valeurs
	cvSourceDeltaValues->SetSize(cvSourceValues->GetSize() - 1);
	for (n = 0; n < cvSourceDeltaValues->GetSize(); n++)
		cvSourceDeltaValues->SetAt(
		    n, KWContinuous::DoubleToContinuous(cvSourceValues->GetAt(n + 1) - cvSourceValues->GetAt(n)));
	cvSourceDeltaValues->Sort();

	// Export des difference de valeur
	if (bExportDeltaValues)
	{
		ExportData(cvSourceDeltaValues, FileService::BuildFilePathName(
						    GetHistogramSpec()->GetResultFilesDirectory(), "DeltaValues.txt"));
	}

	// Calcul de la table initiale
	nTotalBinNumber = GetHistogramSpec()->GetEpsilonBinNumber();
	ComputeInitialFrequencyTable(cvSourceDeltaValues, nTotalBinNumber, initialHistogramFrequencyTable);
	if (bDisplay)
		WriteHistogramFrequencyTable("Initial delta histogram\n", cvSourceDeltaValues,
					     initialHistogramFrequencyTable, cout);

	// Longueur en bin du plus grand intervalle non vide
	nLargestNonEmptyIntervalBinLength = ComputeLargestNonEmptyIntervalBinLength(initialHistogramFrequencyTable);

	// Parametrage de l'algorithme utilise
	if (GetHistogramSpec()->GetOptimalAlgorithm())
		cast(MHDiscretizerGenumHistogram*, this)->SetParam(Optimal);
	else
		cast(MHDiscretizerGenumHistogram*, this)->SetParam(OptimizedGreedyMerge);

	// Appel de la methode de discretisation MODL
	GetHistogramSpec()->SetDeltaValues(true);
	if (GetHistogramSpec()->GetGranularizedModel())
		GranularizedDiscretizeValues(cvSourceDeltaValues, initialHistogramFrequencyTable,
					     optimizedDeltaValueHistogramFrequencyTable);
	else
		StandardDiscretizeValues(cvSourceDeltaValues, initialHistogramFrequencyTable,
					 optimizedDeltaValueHistogramFrequencyTable);

	// Memorisation de l'histogramme dans un fichier si les logs sont demandes
	if (bDisplay)
		WriteHistogramFrequencyTable("Optimized delta histogram\n", cvSourceDeltaValues,
					     optimizedDeltaValueHistogramFrequencyTable, cout);
	if (GetHistogramSpec()->GetExportInternalLogFiles())
	{
		WriteHistogramFrequencyTableFile("Optimized delta histogram\n", cvSourceDeltaValues,
						 optimizedDeltaValueHistogramFrequencyTable,
						 GetHistogramSpec()->GetInternalHistogramFileName());
	}
	GetHistogramSpec()->SetDeltaValues(false);

	// Nettoyage
	delete initialHistogramFrequencyTable;
	TraceEndBlock("Compute histogram on delta values");
}

Continuous MHDiscretizerGenumHistogram::ComputeTruncationEpsilon(const ContinuousVector* cvSourceValues,
								 const KWFrequencyTable* histogramFrequencyTable) const
{
	boolean bDisplay = false;
	Continuous cTruncationEpsilon;
	int nSingularIntervalBinLength;
	boolean bIsZeroVariationSingular;
	MHGenumHistogramVector* histogramVector;
	ContinuousVector cvSourceDeltaValues;
	KWFrequencyTable* histogramDeltaValueFrequencyTable;
	int nLargestNonEmptyIntervalBinLength;
	int nDeltaInterval1Frequency;
	Continuous cDeltaInterval1MeanValue;
	Continuous cDeltaInterval3MinValue;
	Continuous cMin;
	Continuous cMax;
	double dTruncationTotalBinNumber;
	ALString sTmp;

	require(cvSourceValues != NULL);
	require(histogramFrequencyTable != NULL);

	// Calcul de l'histogrammes de variations de valeurs
	DiscretizeDeltaValues(cvSourceValues, &cvSourceDeltaValues, histogramDeltaValueFrequencyTable,
			      nLargestNonEmptyIntervalBinLength);

	// Calcul de la longueur en bin des intervalles singuliers pour l'histogramme des variations de valeur
	nSingularIntervalBinLength = ComputeSingularIntervalBinLength(histogramDeltaValueFrequencyTable);

	// Correction avec la largeur du plus grand intervalle elementaire, pour gerer proprement
	// les cas aux limites de la precision numerique
	nSingularIntervalBinLength = max(nSingularIntervalBinLength, nLargestNonEmptyIntervalBinLength);

	// On determine d'abord si la variation nulle est isolee dans un intervalle singulier
	bIsZeroVariationSingular = histogramDeltaValueFrequencyTable->GetFrequencyVectorNumber() >= 2;
	if (bIsZeroVariationSingular)
	{
		// Test si le premier interval est singulier, autour de la plus petite variation de valeur
		histogramVector =
		    cast(MHGenumHistogramVector*, histogramDeltaValueFrequencyTable->GetFrequencyVectorAt(0));
		bIsZeroVariationSingular = bIsZeroVariationSingular and histogramVector->GetFrequency() > 2;
		bIsZeroVariationSingular =
		    bIsZeroVariationSingular and histogramVector->GetLength() <= nSingularIntervalBinLength;
	}
	if (bIsZeroVariationSingular)
	{
		// Test si le second interval est regulier et vide
		histogramVector =
		    cast(MHGenumHistogramVector*, histogramDeltaValueFrequencyTable->GetFrequencyVectorAt(1));
		bIsZeroVariationSingular = bIsZeroVariationSingular and histogramVector->GetFrequency() == 0;
		bIsZeroVariationSingular =
		    bIsZeroVariationSingular and histogramVector->GetLength() > 2 * nSingularIntervalBinLength;
	}

	// Calcul de l'epsilon de troncature
	cTruncationEpsilon = 0;
	if (bIsZeroVariationSingular)
	{
		//////////////////////////////////////////////////////////////////////////////////////
		// En principe, on se base sur la largeur du deuxieme intervalle, plus 1 bin pour le premier
		// intervalle -la moitie du premier et la moitie du deuxieme, comme dans les articles).
		// Mais pour gerer les limites de la precision numerique, on a du prendre en compte
		// des bins elementaires de largeur strictement plus grande que 1 dans le cxas de valeurs
		// Continuous successives non separables (comme 0 et 1e-100).
		// On utilise ici une nouvelle heuristique en se basant d'une part sur la moyenne des valeurs
		// du permier intervalle singulier, d'autre part sur la premier valeur du troisieme intervalle.
		// Dans le cas standard, cela revient au meme. Et dans les cas "pathologiques", on s'adapte
		// naturellement aux traitements aux limites de la precision numerique.
		// Tout ceci a ete teste extensivement dans LearningTest\TestKhiops\HistogramsLimits\GaussianTruncations

		// Recherche de la valeur moyenne des valeurs du premier iuntervalle
		histogramVector =
		    cast(MHGenumHistogramVector*, histogramDeltaValueFrequencyTable->GetFrequencyVectorAt(0));
		nDeltaInterval1Frequency = histogramVector->GetFrequency();
		assert(nDeltaInterval1Frequency > 0);
		cDeltaInterval1MeanValue =
		    (cvSourceDeltaValues.GetAt(0) + cvSourceDeltaValues.GetAt(nDeltaInterval1Frequency - 1)) / 2;

		// On verifie que le deuxieme vecteur est vide
		debug(histogramVector =
			  cast(MHGenumHistogramVector*, histogramDeltaValueFrequencyTable->GetFrequencyVectorAt(1)));
		debug(assert(histogramVector->GetFrequency() == 0));

		// Recherche de la plus petite valeur du troiseme intervalle
		cDeltaInterval3MinValue = cvSourceDeltaValues.GetAt(nDeltaInterval1Frequency);

		// On en deduit le epsilon de troncature (ne pas oublier de rajouter nSingularIntervalBinLength pour
		// l'intervalle singulier autour de 0)
		cTruncationEpsilon = cDeltaInterval3MinValue - cDeltaInterval1MeanValue;
		assert(cTruncationEpsilon > 0);

		// Calcul d'un nombre total de bin correspondant au epsilon de troncature
		cMin = cvSourceValues->GetAt(0);
		cMax = cvSourceValues->GetAt(cvSourceValues->GetSize() - 1);
		dTruncationTotalBinNumber = (cMax - cMin) / cTruncationEpsilon;

		// On annule l'heuristique de truncature si cela donne un nombre de bin trop grand
		// Cela correspond a un cas aux limites de la precision numerique
		if (dTruncationTotalBinNumber > GetHistogramSpec()->GetMaxEpsilonBinNumber() / 2)
			cTruncationEpsilon = 0;
	}

	// Affichage des criteres de decision
	if (bDisplay)
	{
		cout << "ComputeTruncationEpsilon\n";
		cout << "\tDeltaValue interval number\t"
		     << histogramDeltaValueFrequencyTable->GetFrequencyVectorNumber() << "\n";
		if (histogramDeltaValueFrequencyTable->GetFrequencyVectorNumber() > 2)
		{
			histogramVector =
			    cast(MHGenumHistogramVector*, histogramDeltaValueFrequencyTable->GetFrequencyVectorAt(0));
			cout << "\t\tFirst interval\t" << histogramVector->GetFrequency() << "\t"
			     << histogramVector->GetLength() << "\n";
			histogramVector =
			    cast(MHGenumHistogramVector*, histogramDeltaValueFrequencyTable->GetFrequencyVectorAt(1));
			cout << "\t\tSecond interval\t" << histogramVector->GetFrequency() << "\t"
			     << histogramVector->GetLength() << "\n";
		}
		cout << "\tSingular interval number"
		     << "\t" << ComputeHistogramSingularIntervalNumber(histogramFrequencyTable) << "\n";
		cout << "\tMin value\t" << cvSourceDeltaValues.GetAt(0) << "\n";
		cout << "\tMax value\t" << cvSourceDeltaValues.GetAt(cvSourceDeltaValues.GetSize() - 1) << "\n";
		cout << "\tEpsilon bin number\t" << ComputeHistogramTotalBinNumber(histogramDeltaValueFrequencyTable)
		     << "\n";
		cout << "\tBin epsilon length\t"
		     << GetHistogramSpec()->ComputeEpsilonBinLength(
			    ComputeHistogramTotalBinNumber(histogramDeltaValueFrequencyTable),
			    cvSourceDeltaValues.GetAt(0), cvSourceDeltaValues.GetAt(cvSourceDeltaValues.GetSize() - 1))
		     << "\n";
		cout << "\tTruncationEpsilon\t" << cTruncationEpsilon << "\n";
	}
	Trace(sTmp + "Truncation epsilon\t" + KWContinuous::ContinuousToString(cTruncationEpsilon));

	// Nettoyage
	delete histogramDeltaValueFrequencyTable;
	return cTruncationEpsilon;
}

int MHDiscretizerGenumHistogram::ComputeHistogramTotalFrequency(const KWFrequencyTable* histogramFrequencyTable) const
{
	int nTotalFrequency;
	MHGenumHistogramVector* histogramVector;
	int n;

	require(histogramFrequencyTable != NULL);

	// Parcours des intervalles
	nTotalFrequency = 0;
	for (n = 0; n < histogramFrequencyTable->GetFrequencyVectorNumber(); n++)
	{
		histogramVector = cast(MHGenumHistogramVector*, histogramFrequencyTable->GetFrequencyVectorAt(n));
		nTotalFrequency += histogramVector->GetFrequency();
	}
	return nTotalFrequency;
}

int MHDiscretizerGenumHistogram::ComputeHistogramTotalBinNumber(const KWFrequencyTable* histogramFrequencyTable) const
{
	int nTotalBinNumber;
	MHGenumHistogramVector* histogramVector;
	int n;

	require(histogramFrequencyTable != NULL);

	// Parcours des intervalles
	nTotalBinNumber = 0;
	for (n = 0; n < histogramFrequencyTable->GetFrequencyVectorNumber(); n++)
	{
		histogramVector = cast(MHGenumHistogramVector*, histogramFrequencyTable->GetFrequencyVectorAt(n));
		nTotalBinNumber += histogramVector->GetLength();
	}
	return nTotalBinNumber;
}

int MHDiscretizerGenumHistogram::ComputeHistogramEmptyIntervalNumber(
    const KWFrequencyTable* histogramFrequencyTable) const
{
	int nEmptyIntervalNumber;
	MHGenumHistogramVector* histogramVector;
	int n;

	require(histogramFrequencyTable != NULL);

	// Parcours des intervalles
	nEmptyIntervalNumber = 0;
	for (n = 0; n < histogramFrequencyTable->GetFrequencyVectorNumber(); n++)
	{
		histogramVector = cast(MHGenumHistogramVector*, histogramFrequencyTable->GetFrequencyVectorAt(n));
		if (histogramVector->GetFrequency() == 0)
			nEmptyIntervalNumber++;
	}
	return nEmptyIntervalNumber;
}

int MHDiscretizerGenumHistogram::ComputeHistogramSingletonIntervalNumber(
    const ContinuousVector* cvSourceValues, const KWFrequencyTable* histogramFrequencyTable) const
{
	int nSingletonIntervalNumber;
	MHGenumHistogramVector* histogramVector;
	int n;
	int nFrequency;
	int nTotalFrequency;

	require(cvSourceValues != NULL);
	require(histogramFrequencyTable != NULL);

	// Parcours des intervalles
	nSingletonIntervalNumber = 0;
	nTotalFrequency = 0;
	for (n = 0; n < histogramFrequencyTable->GetFrequencyVectorNumber(); n++)
	{
		histogramVector = cast(MHGenumHistogramVector*, histogramFrequencyTable->GetFrequencyVectorAt(n));
		nFrequency = histogramVector->GetFrequency();
		if (nFrequency > 0 and
		    cvSourceValues->GetAt(nTotalFrequency) == cvSourceValues->GetAt(nTotalFrequency + nFrequency - 1))
			nSingletonIntervalNumber++;
		nTotalFrequency += nFrequency;
	}
	return nSingletonIntervalNumber;
}

int MHDiscretizerGenumHistogram::ComputeHistogramSingularIntervalNumber(
    const KWFrequencyTable* histogramFrequencyTable) const
{
	int nSingularIntervalBinLength;
	int nSingularIntervalNumber;
	int n;

	require(histogramFrequencyTable != NULL);

	// Calcul de la longueur en bin des intervalles singuliers
	nSingularIntervalBinLength = ComputeSingularIntervalBinLength(histogramFrequencyTable);

	// Parcours des intervalles
	nSingularIntervalNumber = 0;
	for (n = 0; n < histogramFrequencyTable->GetFrequencyVectorNumber(); n++)
	{
		if (IsHistogramSingularInterval(histogramFrequencyTable, n, nSingularIntervalBinLength))
			nSingularIntervalNumber++;
	}
	return nSingularIntervalNumber;
}

boolean MHDiscretizerGenumHistogram::IsHistogramSingularInterval(const KWFrequencyTable* histogramFrequencyTable,
								 int nIntervalIndex,
								 int nSingularIntervalBinLength) const
{
	boolean bIsSingular;
	MHGenumHistogramVector* histogramVector;

	require(histogramFrequencyTable != NULL);
	require(0 <= nIntervalIndex and nIntervalIndex < histogramFrequencyTable->GetFrequencyVectorNumber());
	require(nSingularIntervalBinLength >= 1);

	// Il doit y avoir au moins trois intervalles
	// Avec deux intervalles, il ne peut y avoir un intervalle vide en debut ou fin
	bIsSingular = histogramFrequencyTable->GetFrequencyVectorNumber() >= 3;

	// Il doit avoir une largeur d'intervalle singulier
	if (bIsSingular)
	{
		histogramVector =
		    cast(MHGenumHistogramVector*, histogramFrequencyTable->GetFrequencyVectorAt(nIntervalIndex));
		bIsSingular =
		    histogramVector->GetFrequency() > 0 and histogramVector->GetLength() <= nSingularIntervalBinLength;
	}

	// L'intervalle precedent doit etre vide
	if (bIsSingular and nIntervalIndex > 0)
	{
		histogramVector =
		    cast(MHGenumHistogramVector*, histogramFrequencyTable->GetFrequencyVectorAt(nIntervalIndex - 1));
		bIsSingular = histogramVector->GetFrequency() == 0;
	}

	// L'intervalle suivant doit etre vide
	if (bIsSingular and nIntervalIndex < histogramFrequencyTable->GetFrequencyVectorNumber() - 1)
	{
		histogramVector =
		    cast(MHGenumHistogramVector*, histogramFrequencyTable->GetFrequencyVectorAt(nIntervalIndex + 1));
		bIsSingular = histogramVector->GetFrequency() == 0;
	}
	return bIsSingular;
}

int MHDiscretizerGenumHistogram::ComputeSingularIntervalBinLength(const KWFrequencyTable* histogramFrequencyTable) const
{
	const double dEpsilonThreshold = 0.01;
	int nTotalBinNumber;
	int nGranularizedValueNumber;
	int nSingularIntervalBinLength;

	require(histogramFrequencyTable != NULL);

	// Calcul de la longueur en bin des intervalles singuliers, en fonction de la taille du plus petit partile
	nTotalBinNumber = ComputeHistogramTotalBinNumber(histogramFrequencyTable);
	nGranularizedValueNumber = histogramFrequencyTable->GetGranularizedValueNumber();

	// On rajoute un epsilon pour avoir un seuil de detection peu sensible aux bruit numerique
	nSingularIntervalBinLength = int(ceil(nTotalBinNumber * (1.0 + dEpsilonThreshold) / nGranularizedValueNumber));
	nSingularIntervalBinLength = min(nSingularIntervalBinLength, nTotalBinNumber);
	ensure(nSingularIntervalBinLength >= 1);
	return nSingularIntervalBinLength;
}

int MHDiscretizerGenumHistogram::ComputeLargestNonEmptyIntervalBinLength(
    const KWFrequencyTable* histogramFrequencyTable) const
{
	int nLargestNonEmptyIntervalBinLength;
	int n;
	MHGenumHistogramVector* histogramVector;
	int nIntervalFrequency;
	int nIntervalLength;

	require(histogramFrequencyTable != NULL);

	// Parcours des intervalles
	nLargestNonEmptyIntervalBinLength = 0;
	for (n = 0; n < histogramFrequencyTable->GetFrequencyVectorNumber(); n++)
	{
		histogramVector = cast(MHGenumHistogramVector*, histogramFrequencyTable->GetFrequencyVectorAt(n));
		nIntervalFrequency = histogramVector->GetFrequency();
		nIntervalLength = histogramVector->GetLength();
		if (nIntervalFrequency > 0)
			nLargestNonEmptyIntervalBinLength = max(nLargestNonEmptyIntervalBinLength, nIntervalLength);
	}
	return nLargestNonEmptyIntervalBinLength;
}

int MHDiscretizerGenumHistogram::ComputeBinIndex(Continuous cX, double dBinStart, double dEpsilonBinLength,
						 Continuous cMinValue, Continuous cMaxValue, int nTotalBinNumber) const
{
	int nLowerBinIndex;
	int nUpperBinIndex;

	require(dBinStart <= cMinValue);
	require(cX >= cMinValue);
	require(cX <= cMaxValue);
	require(nTotalBinNumber > 0);

	// Index des bins avant et apres la valeur (attention aux egalites)
	if (cX == cMinValue)
		nLowerBinIndex = 0;
	else if (cX == cMaxValue)
		nLowerBinIndex = nTotalBinNumber - 1;
	else
	{
		nLowerBinIndex = int(floor((cX - dBinStart) / dEpsilonBinLength));
		nUpperBinIndex =
		    ComputeNextBinIndex(cX, dBinStart, dEpsilonBinLength, cMinValue, cMaxValue, nTotalBinNumber);
		if (nLowerBinIndex == nUpperBinIndex)
			nLowerBinIndex--;
	}
	return nLowerBinIndex;
}

int MHDiscretizerGenumHistogram::ComputeNextBinIndex(Continuous cX, double dBinStart, double dEpsilonBinLength,
						     Continuous cMinValue, Continuous cMaxValue,
						     int nTotalBinNumber) const
{
	int nUpperBinIndex;

	require(dBinStart <= cMinValue);
	require(cX >= cMinValue);
	require(cX <= cMaxValue);
	require(nTotalBinNumber > 0);

	// Index du bin apres la valeur (attention aux egalites)
	if (cX == cMinValue)
		nUpperBinIndex = 1;
	else if (cX == cMaxValue)
		nUpperBinIndex = nTotalBinNumber;
	else
		nUpperBinIndex = int(ceil((cX - dBinStart) / dEpsilonBinLength));
	return nUpperBinIndex;
}

void MHDiscretizerGenumHistogram::ComputeInitialFrequencyTable(const ContinuousVector* cvSourceValues,
							       int nTotalBinNumber,
							       KWFrequencyTable*& initialHistogramFrequencyTable) const
{
	boolean bDisplay = false;
	KWQuantileIntervalBuilder quantileBuilder;
	Continuous cMinValue;
	Continuous cMaxValue;
	Continuous cEpsilonBinLength;
	Continuous cBinStart;
	int nLastBinIndex;
	int nLowerBinIndex;
	int nUpperBinIndex;
	Continuous cValue;
	Continuous cClosestPreviousValue;
	Continuous cClosestNextValue;
	int n;
	IntVector ivIntervalFrequencies;
	IntVector ivIntervalLengths;
	MHGenumHistogramVector* histogramVector;
	int nCorrectedTotalBinNumber;
	int nBinNumber;

	require(cvSourceValues != NULL);
	require(cvSourceValues->GetSize() > 1);
	require(cvSourceValues->GetAt(0) <= cvSourceValues->GetAt(cvSourceValues->GetSize() - 1));
	require(nTotalBinNumber >= 1);

	// Initialisation du calculateur de quantiles pour avoir les stats par valeur
	quantileBuilder.InitializeValues(cvSourceValues);

	// Correction si necessaire du nombre total de bin a prendre en compte
	nCorrectedTotalBinNumber = nTotalBinNumber;
	if (cvSourceValues->GetAt(0) == cvSourceValues->GetAt(cvSourceValues->GetSize() - 1))
		nCorrectedTotalBinNumber = 1;

	// Calcul du epsilon de precision
	cMinValue = quantileBuilder.GetValueAt(0);
	cMaxValue = quantileBuilder.GetValueAt(quantileBuilder.GetValueNumber() - 1);
	cEpsilonBinLength = GetHistogramSpec()->ComputeEpsilonBinLength(nCorrectedTotalBinNumber, cMinValue, cMaxValue);
	cBinStart = GetHistogramSpec()->ComputeHistogramLowerBound(nCorrectedTotalBinNumber, cMinValue, cMaxValue);
	assert(cEpsilonBinLength > 0);
	if (bDisplay)
		cout << "ComputeInitialFrequencyTable\t" << nTotalBinNumber << "\t" << cEpsilonBinLength << endl;

	// Calcul des effectif et des longueurs des intervalles, en tenant compte que plusieurs
	// valeurs distinctes peuvent tomber dans le meme bin elementaire
	// On impose egalement que deux valeurs successives doit etre separables dans la limite de
	// de la precision de la mantisse. En effet, on ne peut creer un intervalle vide entre
	// deux valeurs successives si la precision numerique ne permet pas d'avoir des valeurs
	// encodees strictement entre ces deux valeurs.
	// Cela a pour consquence qu'aux limites de la precision numerique, les bins peuvent etre
	// desequilibres en taille et que certaines valeurs elementaires theoriquement isolees
	// dans des bin ne les sont plus en pratique
	nLastBinIndex = 0;
	for (n = 0; n < quantileBuilder.GetValueNumber(); n++)
	{
		// Acces a la valeur
		cValue = quantileBuilder.GetValueAt(n);

		// Calcul de la valeur precedente et suivante plus proche
		// On assure ici une symetrie par rapport a l'ordre des valeurs
		cClosestPreviousValue = max(MHContinuousLimits::ComputeClosestLowerBound(cValue), cMinValue);
		cClosestNextValue = min(MHContinuousLimits::ComputeClosestUpperBound(cValue), cMaxValue);

		// Index du bin avant la valeur precedente et apres la valeur suivante
		nLowerBinIndex = ComputeBinIndex(cClosestPreviousValue, cBinStart, cEpsilonBinLength, cMinValue,
						 cMaxValue, nCorrectedTotalBinNumber);
		nUpperBinIndex = ComputeNextBinIndex(cClosestNextValue, cBinStart, cEpsilonBinLength, cMinValue,
						     cMaxValue, nCorrectedTotalBinNumber);

		// Traitement des intervalles vide entre deux valeurs
		if (n > 0)
		{
			// Ajout d'un intervalle vide si on depasse le bin precedent
			if (nLowerBinIndex > nLastBinIndex)
			{
				nBinNumber = nLowerBinIndex - nLastBinIndex;
				nLastBinIndex = nLowerBinIndex;

				// Initialisation d'un intervalle vide
				ivIntervalFrequencies.Add(0);
				ivIntervalLengths.Add(nBinNumber);
				if (bDisplay)
					cout << "\t" << ivIntervalFrequencies.GetSize() << "\t" << nLastBinIndex << "\t"
					     << nLowerBinIndex << "\t" << nUpperBinIndex << "\t0\t" << cValue << "\t"
					     << cClosestPreviousValue << "\t" << cClosestNextValue << endl;
			}
		}

		// Ajout d'un intervalle contenant la valeur si on depasse le bin precedent
		// ou si dernier intervalle
		if (nUpperBinIndex > nLastBinIndex)
		{
			nBinNumber = nUpperBinIndex - nLastBinIndex;
			nLastBinIndex = nUpperBinIndex;
			assert(nBinNumber >= 1);

			// Initialisation d'un intervalle de longueur 1 bin pour chaque valeur
			ivIntervalFrequencies.Add(quantileBuilder.GetValueFrequencyAt(n));
			ivIntervalLengths.Add(nBinNumber);
			if (bDisplay)
				cout << "\t" << ivIntervalFrequencies.GetSize() << "\t" << nLastBinIndex << "\t"
				     << nLowerBinIndex << "\t" << nUpperBinIndex << "\t"
				     << quantileBuilder.GetValueFrequencyAt(n) << "\t" << cValue << "\t"
				     << cClosestPreviousValue << "\t" << cClosestNextValue << endl;
		}
		// Sinon, mise a jour de l'effectif de l'intervalle precedent
		else
		{
			ivIntervalFrequencies.UpgradeAt(ivIntervalFrequencies.GetSize() - 1,
							quantileBuilder.GetValueFrequencyAt(n));
		}

		// Verification sur le traitement de la derniere valeur
		assert(n < quantileBuilder.GetValueNumber() - 1 or nUpperBinIndex == nCorrectedTotalBinNumber);
		assert(n < quantileBuilder.GetValueNumber() - 1 or nLastBinIndex == nCorrectedTotalBinNumber);
	}

	// Creation d'une table avec un intervalle par valeur plus entre chaque valeur
	// On suppose ici que chaque valeur est distance de la precedente d'au moins epsilon
	initialHistogramFrequencyTable = NewFrequencyTable();
	initialHistogramFrequencyTable->SetFrequencyVectorNumber(ivIntervalFrequencies.GetSize());
	for (n = 0; n < ivIntervalFrequencies.GetSize(); n++)
	{
		// Initialisation d'un intervalle de longueur 1 bin pour chaque valeur
		histogramVector =
		    cast(MHGenumHistogramVector*, initialHistogramFrequencyTable->GetFrequencyVectorAt(n));
		histogramVector->SetFrequency(ivIntervalFrequencies.GetAt(n));
		histogramVector->SetLength(ivIntervalLengths.GetAt(n));
	}

	// Affichage du resultat
	if (bDisplay)
		WriteHistogramFrequencyTable("Initial table", cvSourceValues, initialHistogramFrequencyTable, cout);

	// Verification de coherence
	ensure(ComputeHistogramTotalFrequency(initialHistogramFrequencyTable) == cvSourceValues->GetSize());
	ensure(ComputeHistogramTotalBinNumber(initialHistogramFrequencyTable) == nCorrectedTotalBinNumber);
}

void MHDiscretizerGenumHistogram::BuildGranularizedFrequencyTable(
    const KWFrequencyTable* initialHistogramFrequencyTable, int nPartileNumber,
    KWFrequencyTable*& granularizedHistogramFrequencyTable) const
{
	boolean bDisplay = false;
	int nTotalBinNumber;
	double dPartileBinLength;
	MHGenumHistogramVector* histogramVector;
	MHGenumHistogramVector* granularizedHistogramVector;
	int nTotalFrequency;
	int nTotalLength;
	int nCurrentPartileBeginLength;
	int nCurrentPartileEndLength;
	int nIntervalFrequency;
	int nIntervalLength;
	int nRemainingIntervalLength;
	int n;
	int nCurrentPartileIndex;
	int nNextPartileIndex;
	int nSkippedLength;
	int nSkippedLengthBis;
	IntVector ivFilteredPartilesFrequencies;
	IntVector ivFilteredPartilesLengths;

	require(initialHistogramFrequencyTable != NULL);
	require(initialHistogramFrequencyTable->GetFrequencyVectorNumber() > 0);
	require(initialHistogramFrequencyTable->GetFrequencyVectorCreator()->GetClassLabel() ==
		GetDiscretizationCosts()->GetFrequencyVectorCreator()->GetClassLabel());
	require(1 <= nPartileNumber and
		nPartileNumber <= ComputeHistogramTotalBinNumber(initialHistogramFrequencyTable));

	// Longueur des bin granularises
	nTotalBinNumber = ComputeHistogramTotalBinNumber(initialHistogramFrequencyTable);
	dPartileBinLength = nTotalBinNumber * 1.0 / nPartileNumber;

	/////////////////////////////////////////////////////////////////////////////////////
	// Les partiles ( bins granularises)  sont calcules comme une partition entiere des
	// bins elementaires et de sont pas tous necessairement de la meme taille.
	// Par exemple, 100 bin elementaires divises en 3 partiles en 3 donne une partition en (33, 34, 33).
	// Chaque partile d'index p, 0 <= p < P, eest defini dans l'intervalle d'index de bin elementaires
	// [int(floor(p * dPartileBinLength+0.5)); int(floor((p+1) * dPartileBinLength+0.5))[
	//
	// Rappelons que les bin elementaire vides on ete prealablement fusionnes, car cela est optimal
	// pour les algorithmes d'optimisation des histogrammes. Ils peuvent etre redecoupes pour constituer
	// les partiles. Par contre, les bin elementaires non vide sont insecables
	// Les partiles sont ajustes au mieux en fonction des bins elementaires.
	// Si un bin elementaire est vide, est sera redecoupe si necessaire. Sinon, on le placera
	// au mieux du bon cote de la frontiere du partiles.
	// Notons enfin que deux partiles vides consecutifs seront fusionnes innconditionnellement, cela
	// etant optimal pour les algorithmes d'optimisation des histogrammes.

	// Initialisation des caracteristiques du premier partile
	ivFilteredPartilesFrequencies.Add(0);
	ivFilteredPartilesLengths.Add(0);
	nCurrentPartileIndex = 0;
	nCurrentPartileBeginLength = 0;
	nCurrentPartileEndLength = int(floor(dPartileBinLength + 0.5));

	// Parcours des vecteurs de la table initiale
	// Les valeur de TotalFrequency et TotalLength indique la posiiton courante dans
	// le parcours des bin elementaires
	nTotalFrequency = 0;
	nTotalLength = 0;
	for (n = 0; n < initialHistogramFrequencyTable->GetFrequencyVectorNumber(); n++)
	{
		histogramVector =
		    cast(MHGenumHistogramVector*, initialHistogramFrequencyTable->GetFrequencyVectorAt(n));

		// Recherche des caracteristiques de l'intervalle
		nIntervalFrequency = histogramVector->GetFrequency();
		nIntervalLength = histogramVector->GetLength();
		assert(nIntervalLength > 0);

		// Prise en compte de l'intervalle courant pour mettre a jour les partiles
		nRemainingIntervalLength = nIntervalLength;

		// On ajoute tout l'intervalle courant au dernier partile si on ne depasse pas sa taille
		assert(nTotalLength < nCurrentPartileEndLength);
		if (nTotalLength + nRemainingIntervalLength <= nCurrentPartileEndLength)
		{
			if (bDisplay)
				cout << "A\t" << n << "\t" << nIntervalLength << "\t" << nRemainingIntervalLength
				     << "\t" << nTotalLength << "\t" << nCurrentPartileEndLength << endl;

			// Seul cas ou on met a jour l'effectif
			ivFilteredPartilesFrequencies.UpgradeAt(ivFilteredPartilesFrequencies.GetSize() - 1,
								nIntervalFrequency);
			ivFilteredPartilesLengths.UpgradeAt(ivFilteredPartilesLengths.GetSize() - 1,
							    nRemainingIntervalLength);
			nTotalLength += nRemainingIntervalLength;
			nTotalFrequency += nIntervalFrequency;
			nRemainingIntervalLength = 0;
		}
		// On ajoute tout l'intervalle entier si son effectif est non nul, soit au dernier partile,
		// soit au suivant, en fonction de la taille restante
		else if (nIntervalFrequency > 0)
		{
			if (bDisplay)
				cout << "B\t" << n << "\t" << nIntervalLength << "\t" << nRemainingIntervalLength
				     << "\t" << nTotalLength << "\t" << nCurrentPartileEndLength << endl;

			// Creation potentielle d'un nouveau partile si le partile courant est non vide
			//   . soit la longueur necessaire est superieure a celle d'un partile
			//   . soit la longueur necessaire est supeieure au double de la place restante
			//   . soit le dernier partile est non vide
			if (nTotalLength > nCurrentPartileBeginLength and
			    (nRemainingIntervalLength > dPartileBinLength or
			     nRemainingIntervalLength > 2 * (nCurrentPartileEndLength - nTotalLength) or
			     ivFilteredPartilesFrequencies.GetAt(ivFilteredPartilesFrequencies.GetSize() - 1) > 0))
			{
				// Fusion du partile courant avec le precedent si les deux sont vides
				if (ivFilteredPartilesFrequencies.GetSize() > 1 and
				    ivFilteredPartilesFrequencies.GetAt(ivFilteredPartilesFrequencies.GetSize() - 1) ==
					0 and
				    ivFilteredPartilesFrequencies.GetAt(ivFilteredPartilesFrequencies.GetSize() - 2) ==
					0)
				{
					// Ajout de la longueur courant dans le partile precedent
					ivFilteredPartilesLengths.UpgradeAt(
					    ivFilteredPartilesLengths.GetSize() - 2,
					    ivFilteredPartilesLengths.GetAt(ivFilteredPartilesLengths.GetSize() - 1));

					// Reinitialisation de la longueur courante pour preparer un nouveau partile
					ivFilteredPartilesLengths.SetAt(ivFilteredPartilesLengths.GetSize() - 1, 0);
				}
				// Ajout d'un partile sinon
				else
				{
					ivFilteredPartilesFrequencies.Add(0);
					ivFilteredPartilesLengths.Add(0);
				}
				assert(ivFilteredPartilesFrequencies.GetSize() <= 2 or
				       ivFilteredPartilesFrequencies.GetAt(ivFilteredPartilesFrequencies.GetSize() -
									   2) > 0 or
				       ivFilteredPartilesFrequencies.GetAt(ivFilteredPartilesFrequencies.GetSize() -
									   3) > 0);
			}

			// On demarre quoi qu'il arrive un nouveau partile
			nCurrentPartileIndex++;
			assert(nCurrentPartileIndex <= nPartileNumber or
			       (nCurrentPartileIndex == nPartileNumber and
				n == initialHistogramFrequencyTable->GetFrequencyVectorNumber() - 1));

			// Calcul de sa taille
			nCurrentPartileBeginLength = nCurrentPartileEndLength;
			nCurrentPartileEndLength = int(floor((nCurrentPartileIndex + 1) * dPartileBinLength + 0.5));

			// Mis a jour du partile courant
			ivFilteredPartilesFrequencies.UpgradeAt(ivFilteredPartilesFrequencies.GetSize() - 1,
								nIntervalFrequency);
			ivFilteredPartilesLengths.UpgradeAt(ivFilteredPartilesLengths.GetSize() - 1,
							    nRemainingIntervalLength);
			nTotalLength += nRemainingIntervalLength;
			nTotalFrequency += nIntervalFrequency;
			nRemainingIntervalLength = 0;
			assert(nTotalLength > nCurrentPartileBeginLength);
		}
		// Sinon, on remplit completement le partile courant avec des bin elementaires vides
		else
		{
			if (bDisplay)
				cout << "C\t" << n << "\t" << nIntervalLength << "\t" << nRemainingIntervalLength
				     << "\t" << nTotalLength << "\t" << nCurrentPartileEndLength << endl;

			// On est forcement dans le cas d'un intervalle vide
			assert(nIntervalFrequency == 0);
			nRemainingIntervalLength -= (nCurrentPartileEndLength - nTotalLength);
			assert(nRemainingIntervalLength >= 0);
			ivFilteredPartilesLengths.UpgradeAt(ivFilteredPartilesLengths.GetSize() - 1,
							    nCurrentPartileEndLength - nTotalLength);
			nTotalLength = nCurrentPartileEndLength;
		}

		// Passage au partile suivant si necessaire
		if (nTotalLength >= nCurrentPartileEndLength and nTotalLength < nTotalBinNumber)
		{
			assert(initialHistogramFrequencyTable->GetFrequencyVectorNumber() - 1);

			// Recalcul de l'index du partile courant, qui peut avoir rempli avec un bin elementaire
			// non vide large de plusieurs partiles (attention aux problemes d'arrondis)
			nCurrentPartileIndex = int(floor((nTotalLength + 0.5) / dPartileBinLength));
			assert(nCurrentPartileIndex <= nPartileNumber or
			       (nCurrentPartileIndex == nPartileNumber and
				n == initialHistogramFrequencyTable->GetFrequencyVectorNumber() - 1));

			// Calcul de sa taille
			nCurrentPartileBeginLength = int(floor(nCurrentPartileIndex * dPartileBinLength + 0.5));
			nCurrentPartileEndLength = int(floor((nCurrentPartileIndex + 1) * dPartileBinLength + 0.5));
			assert(nCurrentPartileBeginLength <= nTotalLength and nTotalLength < nCurrentPartileEndLength);

			// Nouveau partile si dernier partile en cours non vide et s'il reste des vecteur initiaux a
			// traiter
			if (ivFilteredPartilesFrequencies.GetAt(ivFilteredPartilesFrequencies.GetSize() - 1) > 0)
			{
				ivFilteredPartilesFrequencies.Add(0);
				ivFilteredPartilesLengths.Add(0);
				assert(ivFilteredPartilesFrequencies.GetSize() <= 2 or
				       ivFilteredPartilesFrequencies.GetAt(ivFilteredPartilesFrequencies.GetSize() -
									   2) > 0 or
				       ivFilteredPartilesFrequencies.GetAt(ivFilteredPartilesFrequencies.GetSize() -
									   3) > 0);
			}
		}

		// Gestion de la fin de la longueur
		if (nRemainingIntervalLength > 0)
		{
			// On part d'un partile vide, que l'on va remplir au maximum
			assert(nIntervalFrequency == 0);
			assert(ivFilteredPartilesFrequencies.GetAt(ivFilteredPartilesFrequencies.GetSize() - 1) == 0);
			assert(nTotalLength < nTotalBinNumber);

			// On saute potentiellement plusieurs partiles si on depasse la capacite du partile courant
			if (nTotalLength + nRemainingIntervalLength >= nCurrentPartileEndLength)
			{
				// On determine jusqu'a quel index de partile on peut aller
				nNextPartileIndex =
				    int(floor((nTotalLength + nRemainingIntervalLength + 0.5) / dPartileBinLength));
				nSkippedLength = int(floor(nNextPartileIndex * dPartileBinLength + 0.5)) -
						 int(floor(nCurrentPartileIndex * dPartileBinLength + 0.5));
				assert(nSkippedLength <= nRemainingIntervalLength);

				// Correction potentielle dans le cas ou il manque un partile (attention aux problemes
				// d'arrondi)
				nSkippedLengthBis = int(floor((nNextPartileIndex + 1) * dPartileBinLength + 0.5)) -
						    int(floor(nCurrentPartileIndex * dPartileBinLength + 0.5));
				if (nRemainingIntervalLength >= nSkippedLengthBis)
				{
					nNextPartileIndex++;
					nSkippedLength = nSkippedLengthBis;
				}
				assert(nSkippedLength <= nRemainingIntervalLength and
				       nRemainingIntervalLength <= nSkippedLength + dPartileBinLength);

				// On remplit le partile courant avec la longueur a sauter
				if (bDisplay)
					cout << "D\t" << n << "\t" << nIntervalLength << "\t"
					     << nRemainingIntervalLength << "\t" << nTotalLength << "\t"
					     << nCurrentPartileEndLength << "\t" << nSkippedLength << endl;
				ivFilteredPartilesLengths.UpgradeAt(ivFilteredPartilesLengths.GetSize() - 1,
								    nSkippedLength);
				nTotalLength += nSkippedLength;
				nRemainingIntervalLength -= nSkippedLength;
				assert(nRemainingIntervalLength >= 0);

				// Passage au partile suivant
				nCurrentPartileIndex = nNextPartileIndex;
				assert(nCurrentPartileIndex <= nPartileNumber or
				       (nCurrentPartileIndex == nPartileNumber and
					n == initialHistogramFrequencyTable->GetFrequencyVectorNumber() - 1));

				// Calcul de sa taille
				nCurrentPartileBeginLength = int(floor(nCurrentPartileIndex * dPartileBinLength + 0.5));
				nCurrentPartileEndLength =
				    int(floor((nCurrentPartileIndex + 1) * dPartileBinLength + 0.5));

				// Nouveau partile si necessaire
				if (n < initialHistogramFrequencyTable->GetFrequencyVectorNumber() - 1 or
				    nRemainingIntervalLength > 0)
				{
					ivFilteredPartilesFrequencies.Add(0);
					ivFilteredPartilesLengths.Add(0);
					assert(ivFilteredPartilesFrequencies.GetSize() <= 2 or
					       ivFilteredPartilesFrequencies.GetAt(
						   ivFilteredPartilesFrequencies.GetSize() - 2) > 0 or
					       ivFilteredPartilesFrequencies.GetAt(
						   ivFilteredPartilesFrequencies.GetSize() - 3) > 0);
				}
			}

			// Finalisation avec le dernier partile courant
			if (nRemainingIntervalLength > 0)
			{
				assert(nTotalLength < nTotalBinNumber);
				assert(nRemainingIntervalLength <
				       nCurrentPartileEndLength - nCurrentPartileBeginLength);
				if (bDisplay)
					cout << "E\t" << n << "\t" << nIntervalLength << "\t"
					     << nRemainingIntervalLength << "\t" << nTotalLength << "\t"
					     << nCurrentPartileEndLength << endl;
				ivFilteredPartilesLengths.UpgradeAt(ivFilteredPartilesLengths.GetSize() - 1,
								    nRemainingIntervalLength);
				nTotalLength += nRemainingIntervalLength;
				nRemainingIntervalLength = 0;
			}
		}
		assert(nRemainingIntervalLength == 0);
		assert(fabs(nCurrentPartileEndLength - nCurrentPartileBeginLength - dPartileBinLength) < 1);
	}
	assert(nTotalFrequency == ComputeHistogramTotalFrequency(initialHistogramFrequencyTable));
	assert(nTotalLength == nTotalBinNumber);

	// Creation de la table cible a partir des vecteurs d'effeftifs et de longueurs
	// qui ont ete nettoyes de leurs partile vides consecutifs
	granularizedHistogramFrequencyTable = NewFrequencyTable();
	granularizedHistogramFrequencyTable->SetGranularizedValueNumber(nPartileNumber);
	granularizedHistogramFrequencyTable->SetFrequencyVectorNumber(ivFilteredPartilesFrequencies.GetSize());
	for (n = 0; n < granularizedHistogramFrequencyTable->GetFrequencyVectorNumber(); n++)
	{
		granularizedHistogramVector =
		    cast(MHGenumHistogramVector*, granularizedHistogramFrequencyTable->GetFrequencyVectorAt(n));
		granularizedHistogramVector->SetFrequency(ivFilteredPartilesFrequencies.GetAt(n));
		granularizedHistogramVector->SetLength(ivFilteredPartilesLengths.GetAt(n));

		// On verifie que deux partiles de suite ne peuvent etre vide
		assert(n == 0 or granularizedHistogramVector->GetFrequency() > 0 or
		       cast(MHGenumHistogramVector*, granularizedHistogramFrequencyTable->GetFrequencyVectorAt(n - 1))
			       ->GetFrequency() > 0);
	}

	// Verification de coherence
	ensure(ComputeHistogramTotalFrequency(granularizedHistogramFrequencyTable) ==
	       ComputeHistogramTotalFrequency(initialHistogramFrequencyTable));
	ensure(ComputeHistogramTotalBinNumber(granularizedHistogramFrequencyTable) ==
	       ComputeHistogramTotalBinNumber(initialHistogramFrequencyTable));
	ensure(granularizedHistogramFrequencyTable->GetGranularizedValueNumber() == nPartileNumber);
}

void MHDiscretizerGenumHistogram::WriteHistogramFrequencyTable(const ALString& sTitle,
							       const ContinuousVector* cvSourceValues,
							       const KWFrequencyTable* histogramFrequencyTable,
							       ostream& ost) const
{
	MHGenumHistogram outputHistogram;

	// Construction d'un histogramme en sortie a partir de l'histogramme algorithmique
	BuildOutputHistogram(cvSourceValues, 0, cvSourceValues->GetSize(), histogramFrequencyTable, &outputHistogram);

	// Ecriture de l'histogramme
	WriteHistogram(sTitle, &outputHistogram, ost);
}

void MHDiscretizerGenumHistogram::WriteHistogramFrequencyTableFile(const ALString& sTitle,
								   const ContinuousVector* cvSourceValues,
								   const KWFrequencyTable* histogramFrequencyTable,
								   const ALString& sFileName) const
{
	MHGenumHistogram outputHistogram;

	// Construction d'un histogramme en sortie a partir de l'histogramme algorithmique
	BuildOutputHistogram(cvSourceValues, 0, cvSourceValues->GetSize(), histogramFrequencyTable, &outputHistogram);

	// Ecriture de l'histogramme
	WriteHistogramFile(sTitle, &outputHistogram, sFileName);
}

void MHDiscretizerGenumHistogram::WriteHistogram(const ALString& sTitle, const MHHistogram* histogram,
						 ostream& ost) const
{
	require(histogram != NULL);

	// Informations generale sur les specifications
	ost << sTitle;
	ost << "Histogram algorithm\n";
	ost << "Version\t" << GetHistogramSpec()->GetVersion() << "\n";
	GetHistogramSpec()->Write(ost);
	ost << "\n";

	// Informations sur le jeu de donnees et le modele
	histogram->Write(ost);
}

void MHDiscretizerGenumHistogram::WriteHistogramFile(const ALString& sTitle, const MHHistogram* histogram,
						     const ALString& sFileName) const
{
	fstream fstOutput;
	boolean bOk;

	Trace("Histogram file\t" + sFileName);
	bOk = FileService::OpenOutputFile(sFileName, fstOutput);
	if (bOk)
	{
		WriteHistogram(sTitle, histogram, fstOutput);
		FileService::CloseOutputFile(sFileName, fstOutput);
	}
}

void MHDiscretizerGenumHistogram::ExploitHistogram(const MHHistogram* histogram) const
{
	require(histogram != NULL);

	// TO DO: faire ce que l'on veut ici...

	// Memorisation des informations dans un fichier de resultats
	if (GetHistogramSpec()->GetHistogramFileName("") != "")
	{
		WriteHistogramFile("", histogram, GetHistogramSpec()->GetHistogramFileName(""));
	}
}

void MHDiscretizerGenumHistogram::BuildOutputHistogram(const ContinuousVector* cvSourceValues, int nFirstIndex,
						       int nLastIndex, const KWFrequencyTable* histogramFrequencyTable,
						       MHHistogram* outputHistogram) const
{
	boolean bDisplay = false;
	MHGenumHistogramInterval* outputInterval;
	int nTotalFrequency;
	int nTotalLength;
	MHGenumHistogramVector* histogramVector;
	int n;
	int nIntervalFrequency;
	int nIntervalLength;
	Continuous cMinValue;
	Continuous cMaxValue;
	int nTotalBinNumber;
	Continuous cEpsilonBinLength;
	Continuous cMinLowerBound;
	Continuous cMaxUpperBound;
	Continuous cLowerValue;
	Continuous cUpperValue;
	Continuous cLowerBound;
	Continuous cUpperBound;
	double dIntervalCost;
	double dHistogramPartitionModelCost;
	double dHistogramCost;
	double dNullHistogramCost;
	MHGenumHistogramVector nullHistogramVector;

	require(cvSourceValues != NULL);
	require(0 <= nFirstIndex and nFirstIndex < nLastIndex);
	require(nLastIndex <= cvSourceValues->GetSize());
	require(nFirstIndex == 0 or cvSourceValues->GetAt(nFirstIndex - 1) < cvSourceValues->GetAt(nFirstIndex));
	require(nLastIndex == cvSourceValues->GetSize() or
		cvSourceValues->GetAt(nLastIndex - 1) < cvSourceValues->GetAt(nLastIndex));
	require(histogramFrequencyTable != NULL);
	require(histogramFrequencyTable->GetGranularizedValueNumber() == 0 or
		GetHistogramSpec()->GetGranularizedModel());
	require(ComputeHistogramTotalFrequency(histogramFrequencyTable) == nLastIndex - nFirstIndex);
	require(outputHistogram != NULL);
	require(outputHistogram->GetIntervalNumber() == 0);

	// Initialisation des donnees de travail pour acceder aux informations de cout
	InitializeWorkingData(histogramFrequencyTable);

	///////////////////////////////////////////////////////////////
	// Gestion des caracteristiques globales de l'histogramme

	// Initialisation d'un vecteur correspondant a l'intervalle unique du modele null
	nullHistogramVector.SetFrequency(nLastIndex - nFirstIndex);
	nullHistogramVector.SetLength(cast(MHGenumHistogramCosts*, GetDiscretizationCosts())->GetTotalBinNumber());

	// Calcul du cout de l'histogramme
	dHistogramCost = GetDiscretizationCosts()->ComputePartitionGlobalCost(histogramFrequencyTable);
	dHistogramPartitionModelCost =
	    GetDiscretizationCosts()->ComputePartitionModelCost(histogramFrequencyTable->GetFrequencyVectorNumber(), 0);

	// Calcul du cout de l'histogramme null, en parametrant le nombre de partiles a 1
	if (GetHistogramSpec()->GetGranularizedModel())
		cast(MHGenumHistogramCosts*, GetDiscretizationCosts())->SetPartileNumber(1);
	dNullHistogramCost = GetDiscretizationCosts()->ComputePartitionCost(1) +
			     GetDiscretizationCosts()->ComputePartCost(&nullHistogramVector);

	// Memorisation des informations globales
	outputHistogram->SetHistogramCriterion(GetHistogramSpec()->GetHistogramCriterion());
	outputHistogram->SetGranularity(histogramFrequencyTable->GetGranularizedValueNumber());
	outputHistogram->SetNullCost(dNullHistogramCost);
	outputHistogram->SetCost(dHistogramCost);
	outputHistogram->SetPartitionCost(dHistogramPartitionModelCost);

	///////////////////////////////////////////////////////////////
	// Gestion des caracteristiques par intervalle

	// Valeur extremes et nombre de bins
	cMinValue = cvSourceValues->GetAt(nFirstIndex);
	cMaxValue = cvSourceValues->GetAt(nLastIndex - 1);
	nTotalBinNumber = ComputeHistogramTotalBinNumber(histogramFrequencyTable);
	assert(nTotalBinNumber >= 1);

	// Calcul du epsilon de precision
	cEpsilonBinLength = GetHistogramSpec()->ComputeEpsilonBinLength(nTotalBinNumber, cMinValue, cMaxValue);

	// Borne inf et sup du domaine total de valeur
	cMinLowerBound = GetHistogramSpec()->ComputeHistogramLowerBound(nTotalBinNumber, cMinValue, cMaxValue);
	cMaxUpperBound = GetHistogramSpec()->ComputeHistogramUpperBound(nTotalBinNumber, cMinValue, cMaxValue);

	// Cas particulier d'un intervalle reduit a un seul bin
	if (nTotalBinNumber == 1)
	{
		assert(histogramFrequencyTable->GetFrequencyVectorNumber() == 1);
		histogramVector = cast(MHGenumHistogramVector*, histogramFrequencyTable->GetFrequencyVectorAt(0));

		// Recherche des caracteristiques de l'intervalle
		nIntervalFrequency = histogramVector->GetFrequency();
		nIntervalLength = histogramVector->GetLength();
		dIntervalCost = GetDiscretizationCosts()->ComputePartCost(histogramVector);

		// Creation de l'intervalle en sortie
		outputInterval = new MHGenumHistogramInterval;
		outputHistogram->GetIntervals()->Add(outputInterval);

		// Specification de l'intervalle en sortie
		outputInterval->SetLowerBound(cMinLowerBound);
		outputInterval->SetUpperBound(cMaxUpperBound);
		outputInterval->SetFrequency(nIntervalFrequency);
		outputInterval->SetLowerValue(cMinValue);
		outputInterval->SetUpperValue(cMaxValue);
		outputInterval->SetBinLength(nIntervalLength);
		outputInterval->SetCost(dIntervalCost);
	}
	// Cas general
	else
	{
		assert(cMinValue < cMaxValue);

		// Parcours des vecteurs de l'histogramme algorithmique pour creer les intervalles
		nTotalFrequency = 0;
		nTotalLength = 0;
		for (n = 0; n < histogramFrequencyTable->GetFrequencyVectorNumber(); n++)
		{
			histogramVector =
			    cast(MHGenumHistogramVector*, histogramFrequencyTable->GetFrequencyVectorAt(n));

			// On verifie que deux intervalles de suite ne peuvent etre vide
			assert(n == 0 or histogramVector->GetFrequency() > 0 or
			       cast(MHGenumHistogramVector*, histogramFrequencyTable->GetFrequencyVectorAt(n - 1))
				       ->GetFrequency() > 0);

			// Recherche des caracteristiques de l'intervalle
			nIntervalFrequency = histogramVector->GetFrequency();
			nIntervalLength = histogramVector->GetLength();
			if (nIntervalFrequency > 0)
			{
				cLowerValue = cvSourceValues->GetAt(nFirstIndex + nTotalFrequency);
				cUpperValue =
				    cvSourceValues->GetAt(nFirstIndex + nTotalFrequency + nIntervalFrequency - 1);
			}
			else
			{
				cLowerValue = KWContinuous::GetMissingValue();
				cUpperValue = cLowerValue;
			}

			// Borne inf, en prenant la borne sup de l'intervalle precedent
			if (n == 0)
				cLowerBound = cMinLowerBound;
			else
				cLowerBound = cast(MHHistogramInterval*, outputHistogram->GetIntervals()->GetAt(n - 1))
						  ->GetUpperBound();

			// Calcul de la borne sup
			if (n == histogramFrequencyTable->GetFrequencyVectorNumber() - 1)
				cUpperBound = cMaxUpperBound;
			else
			{
				////////////////////////////////////////////////////////////////////////////////////////
				// Calcul de la borne sup d'un intervalle
				// En theorie, ce calcul ne pose aucun probleme
				// En pratique, il yl des probleme d'arrondi potentiel entre les calculs menes au niveau
				// de la precision des doubles, la projection des bornes sur l'espace des Continuous,
				// l'utilisation des fonction approchees ComputeClosestLowerBound et
				// ComputeClosestUpperBound de MHContinuousLimits Rappelons que les intervalles sont de
				// la forme ]LowerBound; UpperBound], et donc que LowerBound < LowerValue et UpperValue
				// <= UpperBoound pour les intervalles non vide Les instructions ci-dessous visent a
				// s'assurer des ces contrainte en deplacant legerement bornes issue du calcul initial.
				// Bien que tres rarement declenchees, cette methodes le sont neanmoins quand on arrive
				// aux limites de la precision numerique.

				// Calcul de la borne sup en fonction de la longueur cumulee des intervalles
				cUpperBound = cMinLowerBound + (nTotalLength + nIntervalLength) * cEpsilonBinLength;

				// On projette la valeur sur les Continuous
				assert(cvSourceValues->GetAt(nFirstIndex + nTotalFrequency + nIntervalFrequency - 1) <
				       cvSourceValues->GetAt(nFirstIndex + nTotalFrequency + nIntervalFrequency));
				cUpperBound = KWContinuous::DoubleToContinuous(cUpperBound);

				// Si on est inferieur a la borne inf, on deplace legerement la borne sup
				if (cUpperBound <= cLowerBound)
					cUpperBound = MHContinuousLimits::ComputeClosestUpperBound(cLowerBound);

				// Si la projection rend la valeur plus petite que la valeur max de l'intervalle dans le
				// cas d'un intervalle non vide, on prend cette valeur max
				if (nIntervalFrequency > 0 and cUpperBound < cUpperValue)
					cUpperBound = cUpperValue;
				// Si la projection rend la valeur plus grande que la plus petite valeur suivante,
				// on cherche la plus grande valeur strictement plus petite que cette valeur
				else if (cUpperBound >=
					 cvSourceValues->GetAt(nFirstIndex + nTotalFrequency + nIntervalFrequency))
				{
					// On prend la plus petite valeur plus petite que la borne sup
					cUpperBound =
					    cvSourceValues->GetAt(nFirstIndex + nTotalFrequency + nIntervalFrequency);
					cUpperBound = MHContinuousLimits::ComputeClosestLowerBound(cUpperBound);

					// On recompare a la valeur max de l'intervalle
					if (nIntervalFrequency > 0 and cUpperBound < cUpperValue)
						cUpperBound = cUpperValue;
					// On recompare a la borne inf de l'intervalle
					else if (cUpperBound <= cLowerBound)
						cUpperBound = KWContinuous::GetLowerMeanValue(
						    cLowerBound, cvSourceValues->GetAt(nFirstIndex + nTotalFrequency +
										       nIntervalFrequency));
				}

				// Verification sauf pour le premier intervalle
				assert(n < histogramFrequencyTable->GetFrequencyVectorNumber() - 1);
				assert(n == 0 or cLowerBound < cvSourceValues->GetAt(nFirstIndex + nTotalFrequency));
				assert(n == 0 or cvSourceValues->GetAt(nFirstIndex + nTotalFrequency +
								       nIntervalFrequency - 1) <= cUpperBound);
				assert(n == 0 or cUpperBound < cvSourceValues->GetAt(nFirstIndex + nTotalFrequency +
										     nIntervalFrequency));
				assert(n == 0 or cLowerBound < cUpperBound);
				assert(n == 0 or nIntervalFrequency == 0 or cLowerBound < cLowerValue);
				assert(nIntervalFrequency == 0 or cUpperValue <= cUpperBound);
			}

			// Cout de l'intervalle
			dIntervalCost = GetDiscretizationCosts()->ComputePartCost(histogramVector);

			// Creation de l'intervalle en sortie
			outputInterval = new MHGenumHistogramInterval;
			outputHistogram->GetIntervals()->Add(outputInterval);

			// Specification de l'intervalle en sortie
			outputInterval->SetLowerBound(cLowerBound);
			outputInterval->SetUpperBound(cUpperBound);
			outputInterval->SetFrequency(nIntervalFrequency);
			outputInterval->SetLowerValue(cLowerValue);
			outputInterval->SetUpperValue(cUpperValue);
			outputInterval->SetBinLength(nIntervalLength);
			outputInterval->SetCost(dIntervalCost);

			// On verifie que deux intervalles de suite ne peuvent etre vide
			assert(outputHistogram->GetIntervals()->GetSize() <= 1 or outputInterval->GetFrequency() > 0 or
			       outputHistogram->GetConstIntervalAt(outputHistogram->GetIntervals()->GetSize() - 2)
				       ->GetFrequency() > 0);

			// Affichage
			if (bDisplay)
			{
				cout << outputHistogram->GetIntervals()->GetSize() << "\t"
				     << KWContinuous::ContinuousToString(outputInterval->GetLowerBound()) << "\t"
				     << KWContinuous::ContinuousToString(outputInterval->GetUpperBound()) << "\t"
				     << outputInterval->GetFrequency() << "\t"
				     << KWContinuous::ContinuousToString(outputInterval->GetLowerValue()) << "\t"
				     << KWContinuous::ContinuousToString(outputInterval->GetUpperValue()) << "\t"
				     << outputInterval->GetBinLength() << endl;
			}

			// Cumul des effectifs et des longueurs
			nTotalFrequency += nIntervalFrequency;
			nTotalLength += nIntervalLength;
		}
	}

	// Nettoyage des donnees de travail
	CleanWorkingData();
	ensure(outputHistogram->Check());
	ensure(ComputeHistogramTotalFrequency(histogramFrequencyTable) == outputHistogram->ComputeTotalFrequency());
}

void MHDiscretizerGenumHistogram::TraceOpen() const
{
	require(not bIsTraceOpened);
	require(nTraceBlockLevel == 0);

	// Ouverture de la trace seulement si demande
	if (GetHistogramSpec()->GetExportInternalLogFiles())
	{
		bIsTraceOpened = FileService::OpenOutputFile(GetHistogramSpec()->GetInternalTraceFileName(), fstTrace);

		// Ecriture d'une entete
		if (bIsTraceOpened)
		{
			TraceBeginBlock(GetName());
			fstTrace << *GetHistogramSpec() << "\n";
			bIsTraceBlockStart = false;
		}
	}
}

void MHDiscretizerGenumHistogram::TraceClose() const
{
	if (bIsTraceOpened)
	{
		TraceEndBlock(GetName());
		FileService::CloseOutputFile(GetHistogramSpec()->GetInternalTraceFileName(), fstTrace);
	}
	bIsTraceOpened = false;
	ensure(not bIsTraceBlockStart);
	ensure(nTraceBlockLevel == 0);
}

void MHDiscretizerGenumHistogram::TraceBeginBlock(const ALString& sBlockName) const
{
	require(nTraceBlockLevel >= 0);
	if (bIsTraceOpened)
	{
		if (not bIsTraceBlockStart)
			Trace("");
		Trace("Begin\t" + sBlockName);
		nTraceBlockLevel++;
		bIsTraceBlockStart = true;
	}
}

void MHDiscretizerGenumHistogram::TraceEndBlock(const ALString& sBlockName) const
{
	if (bIsTraceOpened)
	{
		assert(nTraceBlockLevel >= 0);
		nTraceBlockLevel--;
		Trace("End\t" + sBlockName);
	}
	ensure(nTraceBlockLevel >= 0);
}

void MHDiscretizerGenumHistogram::Trace(const ALString& sLine) const
{
	int i;

	if (bIsTraceOpened)
	{
		for (i = 0; i < nTraceBlockLevel; i++)
			fstTrace << "    ";
		fstTrace << sLine << "\n";
		fstTrace << flush;
		bIsTraceBlockStart = false;
	}
}

void MHDiscretizerGenumHistogram::PreparedCleanedStandardFrequencyTable(const MHHistogram* histogram,
									int nMissingValueNumber,
									KWFrequencyTable*& standardFrequencyTable,
									ContinuousVector*& cvBounds) const
{
	boolean bDeleteEmptyIntervals = true;
	const MHHistogramInterval* sourceInterval;
	MHGenumHistogramVector* targetVector;
	int n;
	int nFinalIntervalNumber;
	int nIntervalIndex;

	require(histogram != NULL);
	require(nMissingValueNumber >= 0);

	// Comptage du nombre de ligne final, en eliminant si necessaire les lignes vides
	nFinalIntervalNumber = histogram->GetIntervalNumber();
	if (bDeleteEmptyIntervals)
	{
		nFinalIntervalNumber = 0;
		for (n = 0; n < histogram->GetIntervalNumber(); n++)
		{
			sourceInterval = histogram->GetConstIntervalAt(n);
			if (sourceInterval->GetFrequency() > 0)
				nFinalIntervalNumber++;
		}
	}

	// Ajout si necessaire d'un intervalle pour les valeurs manquantes
	if (nMissingValueNumber > 0)
		nFinalIntervalNumber++;

	///////////////////////////////////////////////////////////////
	// Transformation en une table de contingence non supervisee

	// Creation de la table en sortie
	standardFrequencyTable = NewFrequencyTable();
	standardFrequencyTable->SetFrequencyVectorNumber(nFinalIntervalNumber);
	cvBounds = NULL;

	// Ajout eventuel d'un premier intervalle pour les valeurs manquantes
	if (nMissingValueNumber > 0)
	{
		targetVector = cast(MHGenumHistogramVector*, standardFrequencyTable->GetFrequencyVectorAt(0));
		targetVector->SetFrequency(nMissingValueNumber);
	}

	// Ajout des autres intervalles
	nIntervalIndex = 0;
	if (nMissingValueNumber > 0)
		nIntervalIndex++;
	for (n = 0; n < histogram->GetIntervalNumber(); n++)
	{
		sourceInterval = histogram->GetConstIntervalAt(n);

		// Ajout uniquement des intervalles non vides
		if (sourceInterval->GetFrequency() > 0)
		{
			targetVector =
			    cast(MHGenumHistogramVector*, standardFrequencyTable->GetFrequencyVectorAt(nIntervalIndex));
			targetVector->SetFrequency(sourceInterval->GetFrequency());
			nIntervalIndex++;
		}
	}
}

void MHDiscretizerGenumHistogram::InitializeFrequencyVector(KWFrequencyVector* kwfvFrequencyVector) const
{
	MHGenumHistogramVector* frequencyVector;

	require(kwfvFrequencyVector != NULL);

	// Acces aux vecteurs dans le bon type
	frequencyVector = cast(MHGenumHistogramVector*, kwfvFrequencyVector);

	// On n'appelle pas la methode ancetre, qui se base sur des KWDenseFrequencyVector
	frequencyVector->SetFrequency(0);
	frequencyVector->SetLength(0);
}

boolean MHDiscretizerGenumHistogram::CheckFrequencyVector(const KWFrequencyVector* kwfvFrequencyVector) const
{
	require(kwfvFrequencyVector != NULL);
	return true;
}

void MHDiscretizerGenumHistogram::InitializeWorkingData(const KWFrequencyTable* histogramFrequencyTable) const
{
	// On n'appelle pas la methode ancetre, qui se base sur des KWDenseFrequencyVector

	// Parametrage correct des couts
	GetDiscretizationCosts()->SetTotalInstanceNumber(ComputeHistogramTotalFrequency(histogramFrequencyTable));

	// Parametrage du nombre total de bin
	cast(MHGenumHistogramCosts*, GetDiscretizationCosts())
	    ->SetTotalBinNumber(ComputeHistogramTotalBinNumber(histogramFrequencyTable));

	// Parametrage du nombre de partiles pour les couts
	cast(MHGenumHistogramCosts*, GetDiscretizationCosts())
	    ->SetPartileNumber(histogramFrequencyTable->GetGranularizedValueNumber());
}

void MHDiscretizerGenumHistogram::CleanWorkingData() const
{
	// On n'appelle pas la methode ancetre, qui se base sur des KWDenseFrequencyVector

	// Nettoyage du reste du parametrage
	GetDiscretizationCosts()->SetTotalInstanceNumber(0);
	cast(MHGenumHistogramCosts*, GetDiscretizationCosts())->SetTotalBinNumber(1);
	cast(MHGenumHistogramCosts*, GetDiscretizationCosts())->SetPartileNumber(0);
}

void MHDiscretizerGenumHistogram::AddFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
						     const KWFrequencyVector* kwfvAddedFrequencyVector) const
{
	MHGenumHistogramVector* sourcePartFrequencyVector;
	const MHGenumHistogramVector* addedPartFrequencyVector;

	// Acces aux vecteurs dans le bon type
	sourcePartFrequencyVector = cast(MHGenumHistogramVector*, kwfvSourceFrequencyVector);
	addedPartFrequencyVector = cast(const MHGenumHistogramVector*, kwfvAddedFrequencyVector);

	// Prise en compte de l'effectif et de la longueur
	sourcePartFrequencyVector->SetFrequency(sourcePartFrequencyVector->GetFrequency() +
						addedPartFrequencyVector->GetFrequency());
	sourcePartFrequencyVector->SetLength(sourcePartFrequencyVector->GetLength() +
					     addedPartFrequencyVector->GetLength());
}

void MHDiscretizerGenumHistogram::RemoveFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
							const KWFrequencyVector* kwfvRemovedFrequencyVector) const
{
	MHGenumHistogramVector* sourcePartFrequencyVector;
	const MHGenumHistogramVector* removedPartFrequencyVector;

	// Acces aux vecteurs dans le bon type
	sourcePartFrequencyVector = cast(MHGenumHistogramVector*, kwfvSourceFrequencyVector);
	removedPartFrequencyVector = cast(const MHGenumHistogramVector*, kwfvRemovedFrequencyVector);

	// Prise en compte de l'effectif et de la longueur
	sourcePartFrequencyVector->SetFrequency(sourcePartFrequencyVector->GetFrequency() -
						removedPartFrequencyVector->GetFrequency());
	sourcePartFrequencyVector->SetLength(sourcePartFrequencyVector->GetLength() -
					     removedPartFrequencyVector->GetLength());
}

void MHDiscretizerGenumHistogram::MergeTwoFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector,
							   const KWFrequencyVector* kwfvMergedFrequencyVector1,
							   const KWFrequencyVector* kwfvMergedFrequencyVector2) const
{
	// Fusion des deux intervalles
	kwfvSourceFrequencyVector->CopyFrom(kwfvMergedFrequencyVector1);
	AddFrequencyVector(kwfvSourceFrequencyVector, kwfvMergedFrequencyVector2);
}

void MHDiscretizerGenumHistogram::MergeThreeFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector,
							     const KWFrequencyVector* kwfvMergedFrequencyVector1,
							     const KWFrequencyVector* kwfvMergedFrequencyVector2,
							     const KWFrequencyVector* kwfvMergedFrequencyVector3) const
{
	// Fusion des trois intervalles
	kwfvSourceFrequencyVector->CopyFrom(kwfvMergedFrequencyVector1);
	AddFrequencyVector(kwfvSourceFrequencyVector, kwfvMergedFrequencyVector2);
	AddFrequencyVector(kwfvSourceFrequencyVector, kwfvMergedFrequencyVector3);
}

void MHDiscretizerGenumHistogram::SplitFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
						       KWFrequencyVector* kwfvNewFrequencyVector,
						       const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const
{
	// Coupure de l'intervalle
	kwfvNewFrequencyVector->CopyFrom(kwfvSourceFrequencyVector);
	RemoveFrequencyVector(kwfvNewFrequencyVector, kwfvFirstSubFrequencyVectorSpec);
	kwfvSourceFrequencyVector->CopyFrom(kwfvFirstSubFrequencyVectorSpec);
}

void MHDiscretizerGenumHistogram::MergeSplitFrequencyVectors(
    KWFrequencyVector* kwfvSourceFrequencyVector1, KWFrequencyVector* kwfvSourceFrequencyVector2,
    const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const
{
	// Fusion des deux intervalles dans le premier
	AddFrequencyVector(kwfvSourceFrequencyVector2, kwfvSourceFrequencyVector1);

	// Coupure du nouvel interval
	RemoveFrequencyVector(kwfvSourceFrequencyVector2, kwfvFirstSubFrequencyVectorSpec);
	kwfvSourceFrequencyVector1->CopyFrom(kwfvFirstSubFrequencyVectorSpec);
}

void MHDiscretizerGenumHistogram::MergeMergeSplitFrequencyVectors(
    KWFrequencyVector* kwfvSourceFrequencyVector1, const KWFrequencyVector* kwfvSourceFrequencyVector2,
    KWFrequencyVector* kwfvSourceFrequencyVector3, const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const
{
	// Fusion des trois intervalles dans le dernier
	AddFrequencyVector(kwfvSourceFrequencyVector3, kwfvSourceFrequencyVector1);
	AddFrequencyVector(kwfvSourceFrequencyVector3, kwfvSourceFrequencyVector2);

	// Coupure du nouvel interval
	RemoveFrequencyVector(kwfvSourceFrequencyVector3, kwfvFirstSubFrequencyVectorSpec);
	kwfvSourceFrequencyVector1->CopyFrom(kwfvFirstSubFrequencyVectorSpec);
}
