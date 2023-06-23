// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHDiscretizerHistogramMODL_fp.h"

// Directive permettant d'obtenir les assertions dans le fichier de trace, meme en mode release
#undef TraceAssertions
// #define TraceAssertions
#ifdef TraceAssertions
static ALString sGlobalTmp;

#undef assert
#define assert(p)                                                                                                      \
	((p) ? (void)0                                                                                                 \
	     : (void)Trace(sGlobalTmp + "assert failed (" + #p + ") " + __FILE__ + " " + IntToString(__LINE__)))

#undef require
#define require(p)                                                                                                     \
	((p) ? (void)0                                                                                                 \
	     : (void)Trace(sGlobalTmp + "require failed (" + #p + ") " + __FILE__ + " " + IntToString(__LINE__)))

#undef ensure
#define ensure(p)                                                                                                      \
	((p) ? (void)0                                                                                                 \
	     : (void)Trace(sGlobalTmp + "ensure failed (" + #p + ") " + __FILE__ + " " + IntToString(__LINE__)))
#endif // TraceAssertions

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDiscretizerEqualWidth

MHDiscretizerHistogramMODL_fp::MHDiscretizerHistogramMODL_fp()
{
	frequencyTableBuilder = NULL;

	// Specialisation des cout de discretisation pour les histogrammes
	SetDiscretizationCosts(new MHMODLHistogramCosts_fp);
	bIsTraceOpened = false;
	nTraceBlockLevel = 0;
	bIsTraceBlockStart = false;
}

MHDiscretizerHistogramMODL_fp::~MHDiscretizerHistogramMODL_fp()
{
	assert(frequencyTableBuilder == NULL);
	assert(not bIsTraceOpened);
	assert(nTraceBlockLevel == 0);
	assert(not bIsTraceBlockStart);
}

const ALString MHDiscretizerHistogramMODL_fp::GetName() const
{
	return "HistogramMODL";
}

KWDiscretizer* MHDiscretizerHistogramMODL_fp::Create() const
{
	return new MHDiscretizerHistogramMODL_fp;
}

boolean MHDiscretizerHistogramMODL_fp::IsUsingSourceValues() const
{
	return true;
}

void MHDiscretizerHistogramMODL_fp::DiscretizeValues(ContinuousVector* cvSourceValues, IntVector* ivTargetIndexes,
						     int nTargetValueNumber, KWFrequencyTable*& kwftTarget) const
{
	const ALString sCoarseningPrefix = ".c";
	MHHistogram nullHistogram;
	MHHistogram* optimizedHistogram;
	MHHistogram* postprocessedOptimizedHistogram;
	ObjectArray oaCoarsenedHistograms;
	MHHistogram* coarsenedHistogram;
	int n;
	int nMissingValueNumber;
	ContinuousVector cvActualValues;

	require(GetHistogramSpec()->GetHistogramCriterion() == "G-Enum-fp");

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

	// Debut de la trace
	TraceOpen();
	TraceBeginBlock("G-Enum-fp algorithm");

	// Appel de la methode principale de pilotage de la discretisation, si au moins une valeur
	if (cvActualValues.GetSize() > 0)
	{
		MainDiscretizeBins(NULL, &cvActualValues, NULL, optimizedHistogram, postprocessedOptimizedHistogram,
				   &oaCoarsenedHistograms);

		// Fin de la trace
		TraceEndBlock("G-Enum-fp algorithm");
		TraceClose();

		// Exploitation des histogrammes
		if (postprocessedOptimizedHistogram != NULL)
		{
			ExploitHistogram(optimizedHistogram, ".raw");
			ExploitHistogram(postprocessedOptimizedHistogram, "");
		}
		else
			ExploitHistogram(optimizedHistogram, "");
		for (n = 0; n < oaCoarsenedHistograms.GetSize(); n++)
		{
			coarsenedHistogram = cast(MHHistogram*, oaCoarsenedHistograms.GetAt(n));
			ExploitHistogram(coarsenedHistogram,
					 sCoarseningPrefix + IntToString(coarsenedHistogram->GetCoarseningIndex()));
		}

		// Transformation en une table de contingence non supervisee
		if (postprocessedOptimizedHistogram != NULL)
			PreparedCleanedStandardFrequencyTable(postprocessedOptimizedHistogram, nMissingValueNumber,
							      kwftTarget);
		else
			PreparedCleanedStandardFrequencyTable(optimizedHistogram, nMissingValueNumber, kwftTarget);

		// Nettoyage
		delete optimizedHistogram;
		if (postprocessedOptimizedHistogram != NULL)
			delete postprocessedOptimizedHistogram;
		oaCoarsenedHistograms.DeleteAll();
	}
	// Sinon, on se base sur un histogramme vide
	else
	{
		PreparedCleanedStandardFrequencyTable(&nullHistogram, nMissingValueNumber, kwftTarget);
	}
}

MHHistogramSpec MHDiscretizerHistogramMODL_fp::histogramSpec;

MHHistogramSpec* MHDiscretizerHistogramMODL_fp::GetHistogramSpec()
{
	return &histogramSpec;
}

void MHDiscretizerHistogramMODL_fp::ComputeHistogramFromBins(const ContinuousVector* cvSourceBinLowerValues,
							     const ContinuousVector* cvSourceBinUpperValues,
							     const IntVector* ivSourceBinFrequencies) const
{
	const ALString sCoarseningPrefix = ".c";
	MHHistogram* optimizedHistogram;
	MHHistogram* postprocessedOptimizedHistogram;
	ObjectArray oaCoarsenedHistograms;
	MHHistogram* coarsenedHistogram;
	int n;

	require(cvSourceBinUpperValues != NULL);
	require(cvSourceBinUpperValues->GetSize() > 0);

	// Creation du repertoire en sortie si necessaire
	if (GetHistogramSpec()->GetResultFilesDirectory() != "")
		FileService::MakeDirectories(GetHistogramSpec()->GetResultFilesDirectory());

	// Calcul des histogrammes
	MainDiscretizeBins(cvSourceBinLowerValues, cvSourceBinUpperValues, ivSourceBinFrequencies, optimizedHistogram,
			   postprocessedOptimizedHistogram, &oaCoarsenedHistograms);

	// Exploitation des histogrammes
	if (postprocessedOptimizedHistogram != NULL)
	{
		ExploitHistogram(optimizedHistogram, ".raw");
		ExploitHistogram(postprocessedOptimizedHistogram, "");
	}
	else
		ExploitHistogram(optimizedHistogram, "");
	for (n = 0; n < oaCoarsenedHistograms.GetSize(); n++)
	{
		coarsenedHistogram = cast(MHHistogram*, oaCoarsenedHistograms.GetAt(n));
		ExploitHistogram(coarsenedHistogram,
				 sCoarseningPrefix + IntToString(coarsenedHistogram->GetCoarseningIndex()));
	}

	// Nettoyage
	delete optimizedHistogram;
	if (postprocessedOptimizedHistogram != NULL)
		delete postprocessedOptimizedHistogram;
	oaCoarsenedHistograms.DeleteAll();
}

KWFrequencyTable* MHDiscretizerHistogramMODL_fp::NewFrequencyTable() const
{
	KWFrequencyTable* frequencyTable;

	// On renvoie une table de frequence parametree pour la creation de MHHistogramVector
	frequencyTable = new KWFrequencyTable;
	frequencyTable->SetFrequencyVectorCreator(GetFrequencyVectorCreator()->Clone());
	return frequencyTable;
}

MHFloatingPointFrequencyTableBuilder* MHDiscretizerHistogramMODL_fp::NewFrequencyTableBuilder() const
{
	return new MHFloatingPointFrequencyTableBuilder;
}

void MHDiscretizerHistogramMODL_fp::MainDiscretizeBins(const ContinuousVector* cvSourceBinLowerValues,
						       const ContinuousVector* cvSourceBinUpperValues,
						       const IntVector* ivSourceBinFrequencies,
						       MHHistogram*& optimizedHistogram,
						       MHHistogram*& postprocessedOptimizedHistogram,
						       ObjectArray* oaCoarsenedHistograms) const
{
	boolean bDisplay = false;
	MHHistogram* coarsenedHistogram;
	int n;
	Timer timer;
	ALString sTmp;

	require(cvSourceBinUpperValues != NULL);
	require(cvSourceBinUpperValues->GetSize() > 0);
	require(oaCoarsenedHistograms != NULL);
	require(oaCoarsenedHistograms->GetSize() == 0);

	// Pilotage de la discretisation, avec ou sans gestion des outliers
	Trace(sTmp + "Input dataset size\t" + IntToString(cvSourceBinUpperValues->GetSize()));
	timer.Start();

	// Initialisation du constructeur de table de contingence
	frequencyTableBuilder = NewFrequencyTableBuilder();
	frequencyTableBuilder->InitializeBins(cvSourceBinLowerValues, cvSourceBinUpperValues, ivSourceBinFrequencies);

	// Parametrage de l'algorithme utilise
	if (GetHistogramSpec()->GetOptimalAlgorithm())
		cast(MHDiscretizerHistogramMODL_fp*, this)->SetParam(Optimal);
	else
		cast(MHDiscretizerHistogramMODL_fp*, this)->SetParam(OptimizedGreedyMerge);

	// Optimisation de l'exposant du central bin
	OptimizeCentralBinExponent(optimizedHistogram, postprocessedOptimizedHistogram, oaCoarsenedHistograms);

	// Memorisation de l'histogramme dans un fichier si les logs sont demandes
	if (bDisplay)
		WriteHistogram("Optimized histogram\n", optimizedHistogram, cout);
	if (GetHistogramSpec()->GetExportInternalLogFiles())
	{
		WriteHistogramFile("Optimized histogram\n", optimizedHistogram,
				   GetHistogramSpec()->GetInternalHistogramFileName());
	}

	// Finalisation des histogrammes, avec l'ajustement des bornes
	FinalizeHistogram(optimizedHistogram);
	if (postprocessedOptimizedHistogram != NULL)
		FinalizeHistogram(postprocessedOptimizedHistogram);
	for (n = 0; n < oaCoarsenedHistograms->GetSize(); n++)
	{
		coarsenedHistogram = cast(MHHistogram*, oaCoarsenedHistograms->GetAt(n));
		FinalizeHistogram(coarsenedHistogram);
	}

	// Simplification eventuelle de l'histogramme
	if (GetHistogramSpec()->GetMaxIntervalNumber() > 0)
	{
		/*DDD SimplifyHistogram
	    MHHistogram* inputOptimizedHistogram;

	    // Memorisation de l'histogramme optimise courant
		inputOptimizedHistogram = optimizedHistogram;

		// Simplification de l'histogramme
		optimizedHistogram = new MHHistogram;
		SimplifyHistogram(inputOptimizedHistogram, optimizedHistogram);

		// Nettoyage de la version non simplifiee de l'histogramme
		delete inputOptimizedHistogram;
		*/
	}
	timer.Stop();
	optimizedHistogram->SetComputationTime(timer.GetElapsedTime());

	// Nettoyage
	frequencyTableBuilder->Clean();
	delete frequencyTableBuilder;
	frequencyTableBuilder = NULL;
}

void MHDiscretizerHistogramMODL_fp::OptimizeCentralBinExponent(MHHistogram*& optimizedHistogram,
							       MHHistogram*& postprocessedOptimizedHistogram,
							       ObjectArray* oaCoarsenedHistograms) const
{
	boolean bFullOptimizationStudy = false;
	MHHistogram* newOptimizedHistogram;
	MHHistogram* newPostprocessedOptimizedHistogram;
	ObjectArray oaNewCoarsenedHistograms;
	int nBeginExponent;
	int nEndExponent;
	int nExponent;
	int nBestExponent;
	ALString sTmp;

	require(frequencyTableBuilder->IsInitialized());
	require(oaCoarsenedHistograms != NULL);
	require(oaCoarsenedHistograms->GetSize() == 0);

	TraceBeginBlock("Optimize central bin exponent");

	// Etude d'optimisation
	if (bFullOptimizationStudy)
		StudyOptimizeCentralBinExponent();

	// Cas d'un exposant utilisateur impose
	if (GetHistogramSpec()->GetDeltaCentralBinExponent() > -1)
	{
		TraceBeginBlock("Optimize with user central bin exponent");

		// Optimisation avec l'exposant demande
		frequencyTableBuilder->SetCentralBinExponent(
		    min(frequencyTableBuilder->GetMaxOptimizedCentralBinExponent(),
			frequencyTableBuilder->GetMinOptimizedCentralBinExponent() +
			    GetHistogramSpec()->GetDeltaCentralBinExponent()));
		Trace(sTmp + "User exponent\t" + IntToString(frequencyTableBuilder->GetCentralBinExponent()));
		OptimizeGranularity(optimizedHistogram, postprocessedOptimizedHistogram, oaCoarsenedHistograms);
		TraceEndBlock("Optimize with user central bin exponent");
	}
	// Optimisation de l'exposant granularite sinon
	else
	{
		// Optimisation avec l'exposant initial, le plus petit possible
		Trace(sTmp + "Initial exponent\t" +
		      IntToString(frequencyTableBuilder->GetMinOptimizedCentralBinExponent()));
		frequencyTableBuilder->SetCentralBinExponent(
		    frequencyTableBuilder->GetMinOptimizedCentralBinExponent());
		OptimizeGranularity(optimizedHistogram, postprocessedOptimizedHistogram, oaCoarsenedHistograms);

		// Recherche dichotomique du plus grand exposant preservant les points de coupures
		if (frequencyTableBuilder->GetMinOptimizedCentralBinExponent() ==
		    frequencyTableBuilder->GetMaxOptimizedCentralBinExponent())
			Trace("One single exponent");
		else
		{
			Trace("Optimize exponent");

			// Cas particulier d'un histogramme en un seul intervalle: on va chercher directement le plus
			// grand exposant
			if (optimizedHistogram->GetIntervalNumber() == 1)
			{
				Trace("One single interval");
				nBestExponent = frequencyTableBuilder->GetMaxOptimizedCentralBinExponent();
			}
			// Recherche dichotomique sinon
			else
			{
				// Initialisation des exposants
				nBeginExponent = frequencyTableBuilder->GetMinOptimizedCentralBinExponent();
				nEndExponent = frequencyTableBuilder->GetMaxOptimizedCentralBinExponent();
				nBestExponent = nBeginExponent;

				// Recherche dichotomique
				while (nBeginExponent < nEndExponent)
				{
					nExponent = (nBeginExponent + nEndExponent + 1) / 2;

					// On change l'exposant de bin central
					frequencyTableBuilder->SetCentralBinExponent(nExponent);
					Trace(sTmp + "\tTest exponent\t" +
					      IntToString(frequencyTableBuilder->GetCentralBinExponent()));

					// On se deplace vers la fin ou le debut selon le resultat
					if (IsHistogramConsistentWithCentralBinExponent(optimizedHistogram))
					{
						nBestExponent = nExponent;
						nBeginExponent = nExponent;
					}
					else
						nEndExponent = nExponent - 1;
				}
			}

			// On ne fait un deuxieme essai que si necessaire
			Trace(sTmp + "Final exponent\t" + IntToString(frequencyTableBuilder->GetCentralBinExponent()));
			if (nBestExponent != frequencyTableBuilder->GetMinOptimizedCentralBinExponent())
			{
				// On change si necessaire l'exposant de bin central
				if (nBestExponent != frequencyTableBuilder->GetCentralBinExponent())
					frequencyTableBuilder->SetCentralBinExponent(nBestExponent);

				// Optimisation avec l'exposant initial, le plus petit possible
				OptimizeGranularity(newOptimizedHistogram, newPostprocessedOptimizedHistogram,
						    &oaNewCoarsenedHistograms);

				// On garde le meilleur des deux histogrammes pour l'histogramme optimisee
				if (newOptimizedHistogram->GetGranularizedLevel() >
				    optimizedHistogram->GetGranularizedLevel())
				{
					// On memorise le nouvel histogramme
					delete optimizedHistogram;
					optimizedHistogram = newOptimizedHistogram;

					// Ainsi que sa version post-traitee
					if (postprocessedOptimizedHistogram != NULL)
						delete postprocessedOptimizedHistogram;
					postprocessedOptimizedHistogram = newPostprocessedOptimizedHistogram;

					// Et les histogrammes simplifies
					oaCoarsenedHistograms->DeleteAll();
					oaCoarsenedHistograms->CopyFrom(&oaNewCoarsenedHistograms);
					oaNewCoarsenedHistograms.RemoveAll();
				}
				else
				{
					// On garde l'histogramme initial
					nBestExponent = frequencyTableBuilder->GetMinOptimizedCentralBinExponent();
					frequencyTableBuilder->SetCentralBinExponent(nBestExponent);
					delete newOptimizedHistogram;

					// Ainsi que sa version post-traitee
					if (newPostprocessedOptimizedHistogram != NULL)
						delete newPostprocessedOptimizedHistogram;

					// Et les histogrammes simplifies
					oaNewCoarsenedHistograms.DeleteAll();
				}
				assert(nBestExponent == frequencyTableBuilder->GetCentralBinExponent());

				// Trace finale
				Trace(sTmp + "Overall best Level(G)\t" +
				      DoubleToString(optimizedHistogram->GetGranularizedLevel()));
				Trace(sTmp + "Overall best cost\t" + DoubleToString(optimizedHistogram->GetCost()));
				Trace(sTmp + "Overall best exponent\t" +
				      IntToString(optimizedHistogram->GetCentralBinExponent()));
				Trace(sTmp + "Overall best hierarchy level\t" +
				      IntToString(optimizedHistogram->GetHierarchyLevel()));
				Trace(sTmp + "Overall best interval number\t" +
				      IntToString(optimizedHistogram->GetIntervalNumber()));

				// Trace finale eventuelle pour l'histogramme post-traite
				if (postprocessedOptimizedHistogram != NULL)
				{
					Trace(sTmp + "  Overall best post-processed Level(G)\t" +
					      DoubleToString(postprocessedOptimizedHistogram->GetGranularizedLevel()));
					Trace(sTmp + "  Overall best post-processed cost\t" +
					      DoubleToString(postprocessedOptimizedHistogram->GetCost()));
					Trace(sTmp + "  Overall best post-processed exponent\t" +
					      IntToString(postprocessedOptimizedHistogram->GetCentralBinExponent()));
					Trace(sTmp + "  Overall best post-processed hierarchy level\t" +
					      IntToString(postprocessedOptimizedHistogram->GetHierarchyLevel()));
					Trace(sTmp + "  Overall best post-processed interval number\t" +
					      IntToString(postprocessedOptimizedHistogram->GetIntervalNumber()));
				}
			}
		}
	}
	TraceEndBlock("Optimize central bin exponent");
}

boolean MHDiscretizerHistogramMODL_fp::IsHistogramConsistentWithCentralBinExponent(const MHHistogram* histogram) const
{
	boolean bIsConsistent = false;
	int nInterval;
	const MHHistogramInterval* interval;
	const MHHistogramInterval* prevInterval;
	boolean bEmptyGap;
	Continuous cLowerBound;
	Continuous cUpperBound;
	Continuous cPrevLowerBound;
	Continuous cPrevUpperBound;

	require(frequencyTableBuilder->IsInitialized());
	require(histogram != NULL);
	require(frequencyTableBuilder->GetMinOptimizedCentralBinExponent() <= histogram->GetCentralBinExponent() and
		histogram->GetCentralBinExponent() <= frequencyTableBuilder->GetMaxOptimizedCentralBinExponent());
	require(frequencyTableBuilder->GetMinOptimizedCentralBinExponent() <=
		    frequencyTableBuilder->GetCentralBinExponent() and
		frequencyTableBuilder->GetCentralBinExponent() <=
		    frequencyTableBuilder->GetMaxOptimizedCentralBinExponent());

	// Parcours des intervalles de l'histogramme
	for (nInterval = 1; nInterval < histogram->GetIntervalNumber(); nInterval++)
	{
		interval = histogram->GetConstIntervalAt(nInterval);

		// On ne regarde que les intervalle contenant des valeurs
		assert(interval->GetFrequency() > 0 or nInterval < histogram->GetIntervalNumber() - 1);
		if (interval->GetFrequency() > 0)
		{
			// On recherche un interval precedent non vide
			prevInterval = histogram->GetConstIntervalAt(nInterval - 1);
			bEmptyGap = false;
			if (prevInterval->GetFrequency() == 0)
			{
				assert(nInterval > 1);
				prevInterval = histogram->GetConstIntervalAt(nInterval - 2);
				bEmptyGap = true;
			}

			// On cherche les bornes du bin elementaire de la derniere valeur de l'intervalle non vide
			// precedent
			frequencyTableBuilder->ExtractFloatingPointBinBounds(
			    prevInterval->GetUpperValue(),
			    min(histogram->GetHierarchyLevel(), frequencyTableBuilder->GetMaxHierarchyLevel()),
			    cPrevLowerBound, cPrevUpperBound);

			// On cherche les bornes du bin elementaire de la derniere valeur de l'intervalle non vide
			// precedent
			frequencyTableBuilder->ExtractFloatingPointBinBounds(
			    interval->GetUpperValue(),
			    min(histogram->GetHierarchyLevel(), frequencyTableBuilder->GetMaxHierarchyLevel()),
			    cLowerBound, cUpperBound);
			assert(cPrevLowerBound <= cUpperBound);

			// En cas d'intervalle vide intermediaire, il faut que les bin elementaires soit strictement
			// distincts
			if (bEmptyGap)
				bIsConsistent = cPrevUpperBound < cLowerBound;
			// Sinon, il suffit qu'il soit distinct
			else
				bIsConsistent = cPrevUpperBound <= cLowerBound;

			// Arret si non consistence
			if (not bIsConsistent)
				break;
		}
	}
	return bIsConsistent;
}

void MHDiscretizerHistogramMODL_fp::StudyOptimizeCentralBinExponent() const
{
	MHHistogram* optimizedHistogram;
	MHHistogram* postprocessedOptimizedHistogram;
	ObjectArray oaCoarsenedHistograms;
	int nInitialDeltaCentralBinExponent;
	int nExponent;
	fstream fLog;
	fstream fPostprocessedLog;
	boolean bLogOk;
	boolean bPostprocessedLogOk;
	ALString sTmp;

	require(frequencyTableBuilder->IsInitialized());

	// Etude d'optimisation
	TraceBeginBlock("Full central bin exponent optimization study");

	// Memorisation de la valeur initial du parametre utilisateur
	nInitialDeltaCentralBinExponent = GetHistogramSpec()->GetDeltaCentralBinExponent();

	// Gestion d'un fichier d'optimisation specifique
	bLogOk = false;
	bPostprocessedLogOk = false;
	if (GetHistogramSpec()->GetExportInternalLogFiles())
	{
		// Ouverture du fichier de log
		bLogOk = FileService::OpenOutputFile(GetHistogramSpec()->GetInternalOptimizationFileName(".CBE"), fLog);
		if (bLogOk)
			fLog << "Central bin exponent\tIntervals\tLevel(G)\tCost\tNull cost\n";
		Trace("Central bin exponent optimization file\t" +
		      GetHistogramSpec()->GetInternalOptimizationFileName(".CBE"));

		// Ouverture du fichier de log pour la version post-traitee
		bPostprocessedLogOk = FileService::OpenOutputFile(
		    GetHistogramSpec()->GetInternalOptimizationFileName(".CBE.postprocessed"), fPostprocessedLog);
		if (bPostprocessedLogOk)
			fPostprocessedLog << "Central bin exponent\tIntervals\tLevel(G)\tCost\tNull cost\n";
		Trace("Central bin exponent post-processed optimization file\t" +
		      GetHistogramSpec()->GetInternalOptimizationFileName(".CBE.postprocessed"));
	}

	// Parcours des exposants possibles
	for (nExponent = frequencyTableBuilder->GetMinOptimizedCentralBinExponent();
	     nExponent <= frequencyTableBuilder->GetMaxOptimizedCentralBinExponent(); nExponent++)
	{
		frequencyTableBuilder->SetCentralBinExponent(nExponent);
		GetHistogramSpec()->SetDeltaCentralBinExponent(
		    nExponent - frequencyTableBuilder->GetMinOptimizedCentralBinExponent());
		OptimizeGranularity(optimizedHistogram, postprocessedOptimizedHistogram, &oaCoarsenedHistograms);

		// Ecriture des caracteristiques dans les log
		if (bLogOk)
		{
			fLog << nExponent << "\t";
			fLog << optimizedHistogram->GetIntervalNumber() << "\t";
			fLog << optimizedHistogram->GetGranularizedLevel() << "\t";
			fLog << optimizedHistogram->GetCost() << "\t";
			fLog << optimizedHistogram->GetNullCost() << endl;
		}

		// Ecriture des caracteristiques dans les log dans le cas post-optimise
		if (bPostprocessedLogOk and postprocessedOptimizedHistogram != NULL)
		{
			fPostprocessedLog << nExponent << "\t";
			fPostprocessedLog << postprocessedOptimizedHistogram->GetIntervalNumber() << "\t";
			fPostprocessedLog << postprocessedOptimizedHistogram->GetGranularizedLevel() << "\t";
			fPostprocessedLog << postprocessedOptimizedHistogram->GetCost() << "\t";
			fPostprocessedLog << postprocessedOptimizedHistogram->GetNullCost() << endl;
		}

		// Nettoyage
		delete optimizedHistogram;
		if (postprocessedOptimizedHistogram)
			delete postprocessedOptimizedHistogram;
		oaCoarsenedHistograms.DeleteAll();
	}

	// Fermeture des fichiers de log
	if (bLogOk)
		FileService::CloseOutputFile(GetHistogramSpec()->GetInternalOptimizationFileName(".CBE"), fLog);
	if (bPostprocessedLogOk)
		FileService::CloseOutputFile(GetHistogramSpec()->GetInternalOptimizationFileName(".CBE.postprocessed"),
					     fPostprocessedLog);

	// On restitue l'exposant de depart
	frequencyTableBuilder->SetCentralBinExponent(frequencyTableBuilder->GetMinOptimizedCentralBinExponent());
	GetHistogramSpec()->SetDeltaCentralBinExponent(nInitialDeltaCentralBinExponent);
	TraceEndBlock("Full central bin exponent optimization study");
}

void MHDiscretizerHistogramMODL_fp::OptimizeGranularity(MHHistogram*& optimizedHistogram,
							MHHistogram*& postprocessedOptimizedHistogram,
							ObjectArray* oaCoarsenedHistograms) const
{
	boolean bDisplayDetails = false;
	fstream fLog;
	boolean bLogOk;
	ALString sOptimizationPrefix;
	KWFrequencyTable* granularizedHistogramFrequencyTable;
	KWFrequencyTable* optimizedGranularizedHistogramFrequencyTable;
	MHHistogramTable_fp* optimizedHistogramFrequencyTable;
	MHHistogram optimizedGranularizedHistogram;
	ObjectArray oaCollectedCoarsenedHistograms;
	MHHistogram* coarsenedHistogram;
	boolean bRemovedIntervals;
	const int nMaxSuccessiveDecreaseNumber = 2;
	int nTotalIncreaseNumber;
	int nSuccessiveDecreaseNumber;
	int nHierarchyLevel;
	int nIndex;
	int nLastDecreaseIndex;
	double dPreviousCost;
	double dBestCost;
	double dCost;
	int nPartileNumber;
	int nActualPartileNumber;
	int nLastActualPartileNumber;
	boolean bFilter;
	int n;
	ALString sTmp;

	require(frequencyTableBuilder->IsInitialized());
	require(oaCoarsenedHistograms != NULL);
	require(oaCoarsenedHistograms->GetSize() == 0);

	TraceBeginBlock("Optimize granularity");

	// Memorisation des informations dans un fichier de resultats
	bLogOk = false;
	if (GetHistogramSpec()->GetExportInternalLogFiles())
	{
		// Ouverture du fichier de log
		sOptimizationPrefix = sTmp + ".dCBE" +
				      IntToString(frequencyTableBuilder->GetCentralBinExponent() -
						  frequencyTableBuilder->GetMinOptimizedCentralBinExponent());
		bLogOk = FileService::OpenOutputFile(
		    GetHistogramSpec()->GetInternalOptimizationFileName(sOptimizationPrefix), fLog);
		if (bLogOk)
			fLog << "Granularity\tActual partiles\tIntervals\tEmpty intervals\tSpike intervals\tHierarchy "
				"level\tEpsilon bins\tlog*(G)\tlog*(K)\tPartition cost\tCost"
			     << endl;
		Trace("Optimization file\t" + GetHistogramSpec()->GetInternalOptimizationFileName(sOptimizationPrefix));
	}

	// Optimisation de la granularite
	// On s'arrete quand on depasse un nombre max d'essais consecutifs sans amelioration
	optimizedHistogram = new MHHistogram;
	postprocessedOptimizedHistogram = NULL;
	bRemovedIntervals = false;
	dBestCost = DBL_MAX;
	nPartileNumber = 1;
	nLastActualPartileNumber = 1;
	nTotalIncreaseNumber = 0;
	nSuccessiveDecreaseNumber = 0;
	nLastDecreaseIndex = 0;
	nIndex = 0;
	dPreviousCost = DBL_MAX;
	nHierarchyLevel = 0;
	while (nHierarchyLevel <= frequencyTableBuilder->GetMaxSafeHierarchyLevel())
	{
		// Initialisation du nombre de partiles
		nPartileNumber = frequencyTableBuilder->GetTotalBinNumberAt(nHierarchyLevel);

		// Construction d'une version granularisee de la table, apres avoir parametre correctement le nombre de
		// partiles
		frequencyTableBuilder->BuildFrequencyTable(nHierarchyLevel, granularizedHistogramFrequencyTable);
		if (bDisplayDetails)
			WriteHistogramFrequencyTable(sTmp + "Granularized histogram\t" + IntToString(nHierarchyLevel) +
							 "\t" + IntToString(nPartileNumber) + "\n",
						     granularizedHistogramFrequencyTable, cout);

		// On saute ce nombre de partiles s'il n'a pas conduit a au moins 10% de parties en plus
		nActualPartileNumber = granularizedHistogramFrequencyTable->GetFrequencyVectorNumber();
		if (nPartileNumber > 1 and nHierarchyLevel != frequencyTableBuilder->GetMaxSafeHierarchyLevel() and
		    nActualPartileNumber < 1.10 * nLastActualPartileNumber)
		{
			// On peut detruire la version granularisee initiale
			delete granularizedHistogramFrequencyTable;

			// Passage direct au niveau suivant
			nHierarchyLevel++;
			continue;
		}
		nLastActualPartileNumber = nActualPartileNumber;

		// Appel de la methode de discretisation definie dans la classe KWDiscretizerMODL
		DiscretizeGranularizedFrequencyTable(granularizedHistogramFrequencyTable,
						     optimizedGranularizedHistogramFrequencyTable);
		if (bDisplayDetails)
			WriteHistogramFrequencyTable(sTmp + "Optimized granularized histogram\t" +
							 IntToString(nPartileNumber) + "\n",
						     granularizedHistogramFrequencyTable, cout);

		// Nettoyage de la table table initiale, desormais inutile
		delete granularizedHistogramFrequencyTable;

		// On tranforme la KWFrequencyTable en MHHistogramTable_fp specialisee pour les histogrammes
		optimizedHistogramFrequencyTable = new MHHistogramTable_fp;
		optimizedHistogramFrequencyTable->ImportFrom(optimizedGranularizedHistogramFrequencyTable);

		// Memorisation des specifications propres aux histogrammes
		optimizedHistogramFrequencyTable->SetGranularizedValueNumber(nPartileNumber);
		optimizedHistogramFrequencyTable->SetCentralBinExponent(frequencyTableBuilder->GetCentralBinExponent());
		optimizedHistogramFrequencyTable->SetHierarchyLevel(nHierarchyLevel);
		optimizedHistogramFrequencyTable->SetMinBinLength(frequencyTableBuilder->GetMinBinLength());

		// Construction d'un histogramme a partir de l'histogramme algorithmique optimise
		BuildOutputHistogram(optimizedHistogramFrequencyTable, &optimizedGranularizedHistogram);

		// Acces a son cout
		dCost = optimizedGranularizedHistogram.GetCost();

		// Nettoyage des tables de travail
		delete optimizedGranularizedHistogramFrequencyTable;
		delete optimizedHistogramFrequencyTable;

		// Memorisation si amelioration du cout
		if (dCost < dBestCost)
		{
			dBestCost = dCost;

			// Memorisation de la version post-traitee si necessaire
			// On arrete pas la boucle d'optimisation pour produire le meilleur histogramme possible en
			// version principale, et la version post-traite en version secondaire
			if ((GetHistogramSpec()->GetMaxHierarchyLevel() > 0 and
			     optimizedGranularizedHistogram.GetHierarchyLevel() >
				 GetHistogramSpec()->GetMaxHierarchyLevel()) or
			    (GetHistogramSpec()->GetSingularityRemovalHeuristic() and
			     optimizedGranularizedHistogram.ComputeSingularIntervalNumber() > 0))
			{
				// Memorisation uniquement la premiere fois dans la version secondaire
				if (postprocessedOptimizedHistogram == NULL)
				{
					postprocessedOptimizedHistogram = optimizedHistogram->Clone();

					// On memorise s'il s'agit d'une suppression d'intervalles
					if (GetHistogramSpec()->GetMaxHierarchyLevel() == 0 or
					    optimizedGranularizedHistogram.GetGranularity() <=
						GetHistogramSpec()->GetMaxHierarchyLevel())
					{
						assert(GetHistogramSpec()->GetSingularityRemovalHeuristic() and
						       optimizedGranularizedHistogram.ComputeSingularIntervalNumber() >
							   0);
						bRemovedIntervals = true;
					}
				}
				assert(postprocessedOptimizedHistogram->GetHierarchyLevel() <=
					   GetHistogramSpec()->GetMaxHierarchyLevel() or
				       postprocessedOptimizedHistogram->ComputeSingularIntervalNumber() == 0);
			}

			// Memorisation des versions simplifiees au fur et a mesure, en filtrant les version inutiles
			if (GetHistogramSpec()->GetMaxCoarsenedHistogramNumber() > 0)
			{
				// On ne prend en compte ni le premier, ni le dernier modele, ni les modeles apres la
				// version post-optimisee
				if (0 < nHierarchyLevel and
				    nHierarchyLevel < frequencyTableBuilder->GetMaxSafeHierarchyLevel() and
				    postprocessedOptimizedHistogram == NULL)
				{
					coarsenedHistogram = new MHHistogram;
					coarsenedHistogram->CopyFrom(&optimizedGranularizedHistogram);
					oaCollectedCoarsenedHistograms.Add(coarsenedHistogram);
				}
			}

			// Memorisation des caracteristiques du nouveau meilleur histogramme
			optimizedHistogram->CopyFrom(&optimizedGranularizedHistogram);

			// Incrementation du nombre d'ameliorations (sans compter celle du modele nul initial)
			if (nPartileNumber > 1)
				nTotalIncreaseNumber++;
		}

		// Affichage
		if (bLogOk)
		{
			fLog << nPartileNumber << "\t";
			fLog << nActualPartileNumber << "\t";
			fLog << optimizedGranularizedHistogram.GetIntervalNumber() << "\t";
			fLog << optimizedGranularizedHistogram.ComputeEmptyIntervalNumber() << "\t";
			fLog << optimizedGranularizedHistogram.ComputeSingularIntervalNumber() << "\t";
			fLog << optimizedGranularizedHistogram.GetHierarchyLevel() << "\t";
			fLog << optimizedGranularizedHistogram.GetGranularity() << "\t";
			fLog << KWStat::KWStat::NaturalNumbersUniversalCodeLength(nPartileNumber) << "\t";
			fLog << KWStat::KWStat::NaturalNumbersUniversalCodeLength(
				    optimizedGranularizedHistogram.GetIntervalNumber())
			     << "\t";
			fLog << KWContinuous::ContinuousToString(optimizedGranularizedHistogram.GetPartitionCost())
			     << "\t";
			fLog << KWContinuous::ContinuousToString(dCost) << endl;
		}

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
			if (nPartileNumber > sqrt(frequencyTableBuilder->GetTotalFrequency() *
						  (1 + log(1 + frequencyTableBuilder->GetTotalFrequency()))))
			{
				// Arret si aucune amelioration et que l'on atteint presque le nombre total d'instance
				// ou en tout cas le nombre d'instance maximal permettant de detecter un pattern
				// complexe (comme des creneaux)
				if (nTotalIncreaseNumber == 0)
				{
					// Cas d'un petit nombre d'instances: on attente que le nombre de partiles
					// depasse le nombre d'instances
					if (frequencyTableBuilder->GetTotalFrequency() < 128)
					{
						if (nPartileNumber > frequencyTableBuilder->GetTotalFrequency())
							break;
					}
					// Sinon, on limite progressivement le critere d'arret, avec des transition
					// continues, lineaires par morceau
					else if (frequencyTableBuilder->GetTotalFrequency() < 256)
					{
						if (nPartileNumber >
						    128 + (frequencyTableBuilder->GetTotalFrequency() - 128) / 2)
							break;
					}
					else if (frequencyTableBuilder->GetTotalFrequency() < 512)
					{
						if (nPartileNumber >
						    192 + (frequencyTableBuilder->GetTotalFrequency() - 256) / 4)
							break;
					}
					// Asymptotiquement, on se limite a n/8, qui permet de detecter des patterns
					// parmi les plus complexes
					else
					{
						if (nPartileNumber >
						    256 + (frequencyTableBuilder->GetTotalFrequency() - 512) / 8)
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

		// Niveau hierarchique suivant
		nIndex++;
		nHierarchyLevel++;
	}

	// Memorisation du nombre d'intervalles supprimes dans la version post-traitee si necessaire
	if (postprocessedOptimizedHistogram != NULL and bRemovedIntervals)
		postprocessedOptimizedHistogram->SetRemovedSingularIntervalsNumber(
		    optimizedHistogram->ComputeSingularIntervalNumber());

	// Rangement des histogramme simplifie dans l'ordre du plus complexe au plus simple, en filtrant les modules
	// redondants
	assert(oaCollectedCoarsenedHistograms.GetSize() == 0 or
	       GetHistogramSpec()->GetMaxCoarsenedHistogramNumber() > 0);
	for (n = oaCollectedCoarsenedHistograms.GetSize() - 1; n >= 0; n--)
	{
		coarsenedHistogram = cast(MHHistogram*, oaCollectedCoarsenedHistograms.GetAt(n));

		// On determine si on doit garder ou non l'histogramme
		bFilter = oaCoarsenedHistograms->GetSize() >= GetHistogramSpec()->GetMaxCoarsenedHistogramNumber();
		bFilter = bFilter or coarsenedHistogram->GetIntervalNumber() == 1;
		bFilter = bFilter or coarsenedHistogram->GetHierarchyLevel() == optimizedHistogram->GetHierarchyLevel();
		bFilter = bFilter or (postprocessedOptimizedHistogram != NULL and
				      coarsenedHistogram->GetHierarchyLevel() ==
					  postprocessedOptimizedHistogram->GetHierarchyLevel());

		// Filtrage si necessaire
		if (bFilter)
		{
			delete coarsenedHistogram;
			coarsenedHistogram = NULL;
		}
		// Sinon, on garde l'histogramme simplfie
		else
		{
			oaCoarsenedHistograms->Add(coarsenedHistogram);
			coarsenedHistogram->SetCoarseningIndex(oaCoarsenedHistograms->GetSize());
		}
	}
	assert(oaCoarsenedHistograms->GetSize() <= GetHistogramSpec()->GetMaxCoarsenedHistogramNumber());

	// Nettoyage du parametrage des couts
	cast(MHMODLHistogramCosts_fp*, GetDiscretizationCosts())->SetPartileNumber(0);

	// Fermeture du fichier de log
	if (bLogOk)
		FileService::CloseOutputFile(GetHistogramSpec()->GetInternalOptimizationFileName(sOptimizationPrefix),
					     fLog);

	// Infos de trace
	if (IsTraceOpened())
	{
		Trace(sTmp + "Best Level(G)\t" + DoubleToString(optimizedHistogram->GetGranularizedLevel()));
		Trace(sTmp + "Best hierarchy level\t" + IntToString(optimizedHistogram->GetHierarchyLevel()));
		Trace(sTmp + "Best interval number\t" + IntToString(optimizedHistogram->GetIntervalNumber()));

		// Trace finale eventuelle pour l'histogramme post-traite
		if (postprocessedOptimizedHistogram != NULL)
		{
			Trace(sTmp + "  Best post-processed Level(G)\t" +
			      DoubleToString(postprocessedOptimizedHistogram->GetGranularizedLevel()));
			Trace(sTmp + "  Best post-processed hierarchy level\t" +
			      IntToString(postprocessedOptimizedHistogram->GetHierarchyLevel()));
			Trace(sTmp + "  Best post-processed interval number\t" +
			      IntToString(postprocessedOptimizedHistogram->GetIntervalNumber()));
		}
	}
	TraceEndBlock("Optimize granularity");

	ensure(optimizedHistogram != NULL);
}

void MHDiscretizerHistogramMODL_fp::SimplifyHistogram(const MHHistogram* optimizedHistogram,
						      MHHistogram*& simplifiedHistogram) const
{
	/*DDD SimplifyHistogram
	int nReferenceMaxIntervalNumber;
	KWFrequencyTable* initialHistogramFrequencyTable;
	KWFrequencyTable* granularizedHistogramFrequencyTable;
	KWFrequencyTable* optimizedGranularizedHistogramFrequencyTable;
	ALString sTmp;

	require(cvSourceValues != NULL);
	require(optimizedHistogram != NULL);
	require(optimizedHistogram->ComputeTotalFrequency() == cvSourceValues->GetSize());

	TraceBeginBlock("Simplify histogram");
	Trace(sTmp + "Max interval number\t" + IntToString(GetHistogramSpec()->GetMaxIntervalNumber()));
	Trace(sTmp + "Data subset number\t" + LongintToString(optimizedHistogram->GetOutlierDataSubsetNumber()));
	Trace(sTmp + "Bin number\t" +
	KWContinuous::ContinuousToString(optimizedHistogram->ComputeLargeScaleTotalBinLength())); Trace(sTmp +
	"Granularity\t" + LongintToString(optimizedHistogram->GetGranularity()));

	// Si l'heuristique OMH est utilisee, la simplification n'est pas encore implementee
	if (optimizedHistogram->GetOutlierDataSubsetNumber() > 1)
	{
		// Recopie de la version en entree de l'histogramme
		simplifiedHistogram->CopyFrom(optimizedHistogram);
	}
	// Sinon, on va recalculer l'histogramme en limitant le nombre d'intervalles
	else
	{
		// Calcul de la table initiale avec le bon nombre de bin
		ComputeInitialFrequencyTable(cvSourceValues, optimizedHistogram->ComputeTotalBinLength(),
	initialHistogramFrequencyTable);

		// Construction d'une version granularisee de la table, apres avoir parametree correctement le nombre de
	partiles BuildGranularizedFrequencyTable(initialHistogramFrequencyTable, optimizedHistogram->GetGranularity(),
	granularizedHistogramFrequencyTable);

		// Contrainte sur le nombre max d'intervalles si demande par l'utilisateur et qu'on est dans le mode
	standard nReferenceMaxIntervalNumber = GetMaxIntervalNumber(); cast(MHDiscretizerHistogramMODL_fp*,
	this)->SetMaxIntervalNumber(GetHistogramSpec()->GetMaxIntervalNumber());

		// Appel de la methode de de discretisation definie dans la classe KWDiscretizerMODL
		DiscretizeGranularizedFrequencyTable(granularizedHistogramFrequencyTable,
	optimizedGranularizedHistogramFrequencyTable); cast(MHDiscretizerHistogramMODL_fp*,
	this)->SetMaxIntervalNumber(nReferenceMaxIntervalNumber);

		// On peut detruire la version granularisee initiale
		delete initialHistogramFrequencyTable;
		delete granularizedHistogramFrequencyTable;

		// On transforme le resultat en histogramme
		BuildOutputHistogram(cvSourceValues, optimizedGranularizedHistogramFrequencyTable, simplifiedHistogram);

		// Nettoyage
		delete optimizedGranularizedHistogramFrequencyTable;
	}
	TraceEndBlock("Simplify histogram");
	*/
}

void MHDiscretizerHistogramMODL_fp::ExportData(const ContinuousVector* cvSourceBinLowerValues,
					       const ContinuousVector* cvSourceBinUpperValues,
					       const IntVector* ivSourceBinFrequencies, const ALString& sFileName) const
{
	fstream fData;
	boolean bOk;
	int n;

	require(cvSourceBinUpperValues != NULL);

	// Ouverture du fichier en sortie
	bOk = FileService::OpenOutputFile(sFileName, fData);
	if (bOk)
	{
		// Entete
		if (cvSourceBinLowerValues != NULL)
			fData << "Lower value\tUpper value";
		else
			fData << "Value";
		if (ivSourceBinFrequencies != NULL)
			fData << "\tFrequency";
		fData << "\n";

		// Parcours de toutes les donnees
		for (n = 0; n < cvSourceBinUpperValues->GetSize(); n++)
		{
			if (cvSourceBinLowerValues != NULL)
				fData << KWContinuous::ContinuousToString(cvSourceBinLowerValues->GetAt(n)) << "\t";
			fData << KWContinuous::ContinuousToString(cvSourceBinUpperValues->GetAt(n));
			if (ivSourceBinFrequencies != NULL)
				fData << "\t" << ivSourceBinFrequencies->GetAt(n);
			fData << "\n";
		}

		// Fermeture du fichier
		FileService::CloseOutputFile(sFileName, fData);
	}
}

int MHDiscretizerHistogramMODL_fp::ComputeHistogramTotalFrequency(const KWFrequencyTable* histogramFrequencyTable) const
{
	int nTotalFrequency;
	MHHistogramVector_fp* histogramVector;
	int n;

	require(histogramFrequencyTable != NULL);

	// Parcours des intervalles
	nTotalFrequency = 0;
	for (n = 0; n < histogramFrequencyTable->GetFrequencyVectorNumber(); n++)
	{
		histogramVector = cast(MHHistogramVector_fp*, histogramFrequencyTable->GetFrequencyVectorAt(n));
		nTotalFrequency += histogramVector->GetFrequency();
	}
	return nTotalFrequency;
}

void MHDiscretizerHistogramMODL_fp::WriteHistogramFrequencyTable(const ALString& sTitle,
								 const KWFrequencyTable* histogramFrequencyTable,
								 ostream& ost) const
{
	MHHistogram outputHistogram;

	// Construction d'un histogramme en sortie a partir de l'histogramme algorithmique
	BuildOutputHistogram(histogramFrequencyTable, &outputHistogram);

	// Ecriture de l'histogramme
	WriteHistogram(sTitle, &outputHistogram, ost);
}

void MHDiscretizerHistogramMODL_fp::WriteHistogramFrequencyTableFile(const ALString& sTitle,
								     const KWFrequencyTable* histogramFrequencyTable,
								     const ALString& sFileName) const
{
	MHHistogram outputHistogram;

	// Construction d'un histogramme en sortie a partir de l'histogramme algorithmique
	BuildOutputHistogram(histogramFrequencyTable, &outputHistogram);

	// Ecriture de l'histogramme
	WriteHistogramFile(sTitle, &outputHistogram, sFileName);
}

void MHDiscretizerHistogramMODL_fp::WriteHistogram(const ALString& sTitle, const MHHistogram* histogram,
						   ostream& ost) const
{
	require(histogram != NULL);

	// Informations generale sur les specifications
	ost << sTitle;
	ost << "Histogram algorithm\n";
	ost << "Version\t" << MHHistogramSpec::GetVersion() << "\n";
	GetHistogramSpec()->Write(ost);
	ost << "\n";

	// Informations sur le jeu de donnees et le modele
	histogram->Write(ost);
}

void MHDiscretizerHistogramMODL_fp::WriteHistogramFile(const ALString& sTitle, const MHHistogram* histogram,
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

void MHDiscretizerHistogramMODL_fp::ExploitHistogram(const MHHistogram* histogram, const ALString& sSuffix) const
{
	require(histogram != NULL);

	// TO DO: faire ce que l'on veut ici...

	// Memorisation des informations dans un fichier de resultats
	if (GetHistogramSpec()->GetHistogramFileName(sSuffix) != "")
	{
		WriteHistogramFile("", histogram, GetHistogramSpec()->GetHistogramFileName(sSuffix));
	}
}

void MHDiscretizerHistogramMODL_fp::BuildOutputHistogram(const KWFrequencyTable* histogramFrequencyTable,
							 MHHistogram* outputHistogram) const
{
	boolean bDisplay = false;
	MHHistogramInterval* outputInterval;
	int nTotalFrequency;
	KWFrequencyTable* nulFrequencyTable;
	MHHistogramVector_fp* histogramVector;
	ObjectArray oaHistogramVectors;
	int n;
	int nIntervalFrequency;
	Continuous cLowerValue;
	Continuous cUpperValue;
	Continuous cLowerBound;
	Continuous cUpperBound;
	double dIntervalCost;
	double dHistogramPartitionModelCost;
	double dHistogramCost;
	double dNullHistogramCost;
	MHMODLHistogramCosts_fp* floatingPointHistogramCost;
	double dReferenceNullHistogramCost;

	require(histogramFrequencyTable != NULL);
	require(histogramFrequencyTable->GetGranularizedValueNumber() == 0 or
		GetHistogramSpec()->GetGranularizedModel());
	require(outputHistogram != NULL);
	require(frequencyTableBuilder->IsInitialized());

	// Nettoyage initial de l'histogramme en sortie
	outputHistogram->Clean();

	///////////////////////////////////////////////////////////////
	// Gestion des caracteristiques globales de l'histogramme

	// Acces a la version specialise des couts
	floatingPointHistogramCost = cast(MHMODLHistogramCosts_fp*, GetDiscretizationCosts());

	// Construction d'un histogramme nul
	frequencyTableBuilder->BuildNulFrequencyTable(nulFrequencyTable);

	// Initialisation des donnees de travail pour acceder aux informations de cout pour le modele nul
	InitializeWorkingData(nulFrequencyTable);

	// Calcul du cout de l'histogramme null, en parametrant le niveau hierarchique a 0 nombre de partiles a 1
	dNullHistogramCost = floatingPointHistogramCost->ComputePartitionGlobalCost(nulFrequencyTable);

	// Calcul du cout de reference de l'histogramme null dans l'ensemble [1,2], avec la precision maximale par
	// defaut
	dReferenceNullHistogramCost = floatingPointHistogramCost->ComputeReferenceNullCost(
	    frequencyTableBuilder->GetTotalFrequency(), frequencyTableBuilder->GetDefaultTotalHierarchyLevel());

	// Nettoyage
	delete nulFrequencyTable;
	CleanWorkingData();

	// Initialisation des donnees de travail pour acceder aux informations de cout pour le modele courant
	InitializeWorkingData(histogramFrequencyTable);

	// Calcul du cout de l'histogramme
	dHistogramCost = GetDiscretizationCosts()->ComputePartitionGlobalCost(histogramFrequencyTable);
	dHistogramPartitionModelCost =
	    GetDiscretizationCosts()->ComputePartitionModelCost(histogramFrequencyTable->GetFrequencyVectorNumber(), 0);

	// Memorisation des informations globales
	outputHistogram->SetDistinctValueNumber(frequencyTableBuilder->GetDistinctValueNumber());
	outputHistogram->SetHistogramCriterion(GetHistogramSpec()->GetHistogramCriterion());
	outputHistogram->SetGranularity(histogramFrequencyTable->GetGranularizedValueNumber());
	outputHistogram->SetNullCost(dNullHistogramCost);
	outputHistogram->SetReferenceNullCost(dReferenceNullHistogramCost);
	outputHistogram->SetCost(dHistogramCost);
	outputHistogram->SetPartitionCost(dHistogramPartitionModelCost);

	// Memorisation des informations specifiques aux histogrammes a virgule flottante
	outputHistogram->SetDomainLowerBound(frequencyTableBuilder->GetDomainLowerBound());
	outputHistogram->SetDomainUpperBound(frequencyTableBuilder->GetDomainUpperBound());
	outputHistogram->SetDomainBoundsMantissaBitNumber(frequencyTableBuilder->GetDomainBoundsMantissaBitNumber());
	outputHistogram->SetHierarchyLevel(cast(MHHistogramTable_fp*, histogramFrequencyTable)->GetHierarchyLevel());
	outputHistogram->SetMainBinHierarchyRootLevel(frequencyTableBuilder->GetMainBinHierarchyRootLevel());
	outputHistogram->SetMaxHierarchyLevel(frequencyTableBuilder->GetMaxHierarchyLevel());
	outputHistogram->SetMaxSafeHierarchyLevel(frequencyTableBuilder->GetMaxSafeHierarchyLevel());
	outputHistogram->SetCentralBinExponent(frequencyTableBuilder->GetCentralBinExponent());
	outputHistogram->SetMinCentralBinExponent(frequencyTableBuilder->GetMinCentralBinExponent());
	outputHistogram->SetMaxCentralBinExponent(frequencyTableBuilder->GetMaxCentralBinExponent());
	outputHistogram->SetMainBinNumber(frequencyTableBuilder->GetMainBinNumber());
	outputHistogram->SetMinBinLength(frequencyTableBuilder->GetMinBinLength());

	///////////////////////////////////////////////////////////////
	// Gestion des caracteristiques par intervalle

	// On commence a ranger les intervalles "utiles" dans un tableau, en eliminant potentiellement le premier
	// et le dernier intervalle s'ils sont vides, en raison d'un ecart entre bornes du domaines et valeurs min ou
	// max Ce nettoyage preable permet de simplifier ensuite la boucle de creation des intervalles et les controles
	// associes
	for (n = 0; n < histogramFrequencyTable->GetFrequencyVectorNumber(); n++)
	{
		histogramVector = cast(MHHistogramVector_fp*, histogramFrequencyTable->GetFrequencyVectorAt(n));

		// Filtrage des intervalles extremites s'ils sont vide
		if (n == 0)
		{
			if (histogramVector->GetFrequency() > 0)
				oaHistogramVectors.Add(histogramVector);
		}
		else if (n == histogramFrequencyTable->GetFrequencyVectorNumber() - 1)
		{
			if (histogramVector->GetFrequency() > 0)
				oaHistogramVectors.Add(histogramVector);
		}
		else
			oaHistogramVectors.Add(histogramVector);
	}

	// Parcours des intervalles utiles de l'histogramme algorithmique pour creer les intervalles
	nTotalFrequency = 0;
	for (n = 0; n < oaHistogramVectors.GetSize(); n++)
	{
		histogramVector = cast(MHHistogramVector_fp*, oaHistogramVectors.GetAt(n));

		// On verifie que deux intervalles de suite ne peuvent etre vide
		assert(n == 0 or histogramVector->GetFrequency() > 0 or
		       cast(MHHistogramVector_fp*, oaHistogramVectors.GetAt(n - 1))->GetFrequency() > 0);

		// Recherche des caracteristiques de l'intervalle
		nIntervalFrequency = histogramVector->GetFrequency();
		if (nIntervalFrequency > 0)
		{
			cLowerValue = frequencyTableBuilder->GetBinLowerValueAtFrequency(nTotalFrequency);
			cUpperValue = frequencyTableBuilder->GetBinUpperValueAtFrequency(nTotalFrequency +
											 nIntervalFrequency - 1);
		}
		else
		{
			cLowerValue = KWContinuous::GetMissingValue();
			cUpperValue = cLowerValue;
		}

		// Recopie des bornes des intervalles
		cLowerBound = histogramVector->GetLowerBound();
		cUpperBound = histogramVector->GetUpperBound();

		// Controles d'integrite
		assert(n == 0 or cLowerBound < frequencyTableBuilder->GetBinLowerValueAtFrequency(nTotalFrequency));
		assert(n == 0 or frequencyTableBuilder->GetBinUpperValueAtFrequency(
				     nTotalFrequency + nIntervalFrequency - 1) <= cUpperBound);
		assert(n < oaHistogramVectors.GetSize() - 1 or
		       nTotalFrequency + nIntervalFrequency == frequencyTableBuilder->GetTotalFrequency());
		assert(n == oaHistogramVectors.GetSize() - 1 or
		       cUpperBound <
			   frequencyTableBuilder->GetBinLowerValueAtFrequency(nTotalFrequency + nIntervalFrequency));
		assert(n == 0 or cLowerBound < cUpperBound);
		assert(n == 0 or nIntervalFrequency == 0 or cLowerBound < cLowerValue);
		assert(nIntervalFrequency == 0 or cUpperValue <= cUpperBound);

		// Cout de l'intervalle
		dIntervalCost = GetDiscretizationCosts()->ComputePartCost(histogramVector);

		// Creation de l'intervalle en sortie
		outputInterval = new MHHistogramInterval;
		outputHistogram->GetIntervals()->Add(outputInterval);

		// Specification de l'intervalle en sortie
		outputInterval->SetLowerBound(cLowerBound);
		outputInterval->SetUpperBound(cUpperBound);
		outputInterval->SetFrequency(nIntervalFrequency);
		outputInterval->SetLowerValue(cLowerValue);
		outputInterval->SetUpperValue(cUpperValue);
		outputInterval->SetCost(dIntervalCost);
		assert(outputInterval->Check());

		// On verifie que deux intervalles de suite ne peuvent etre vide
		assert(outputHistogram->GetIntervalNumber() <= 1 or outputInterval->GetFrequency() > 0 or
		       outputHistogram->GetConstIntervalAt(outputHistogram->GetIntervalNumber() - 2)->GetFrequency() >
			   0);

		// Affichage
		if (bDisplay)
		{
			if (outputHistogram->GetIntervals()->GetSize() == 1)
			{
				cout << "BuildOutputHistogram\n";
				cout
				    << "Index\tLower bound\tUpper bound\tFrequency\tLength\tLower value\tUpper value\n";
			}
			cout << outputHistogram->GetIntervals()->GetSize() << "\t"
			     << KWContinuous::ContinuousToString(outputInterval->GetLowerBound()) << "\t"
			     << KWContinuous::ContinuousToString(outputInterval->GetUpperBound()) << "\t"
			     << outputInterval->GetFrequency() << "\t"
			     << outputInterval->GetUpperBound() - outputInterval->GetLowerBound() << "\t"
			     << KWContinuous::ContinuousToString(outputInterval->GetLowerValue()) << "\t"
			     << KWContinuous::ContinuousToString(outputInterval->GetUpperValue()) << endl;
		}

		// Cumul des effectifs
		nTotalFrequency += nIntervalFrequency;
	}

	// Nettoyage des donnees de travail
	CleanWorkingData();
	ensure(ComputeHistogramTotalFrequency(histogramFrequencyTable) == outputHistogram->ComputeTotalFrequency());
}

void MHDiscretizerHistogramMODL_fp::FinalizeHistogram(MHHistogram* outputHistogram) const
{
	require(frequencyTableBuilder != NULL);
	require(outputHistogram != NULL);

	AdjustHistogramIntervalBounds(frequencyTableBuilder, outputHistogram);
}

void MHDiscretizerHistogramMODL_fp::AdjustHistogramIntervalBounds(
    const MHFloatingPointFrequencyTableBuilder* tableBuilder, MHHistogram* outputHistogram) const
{
	boolean bDisplay = false;
	MHHistogramInterval* outputInterval;
	int nTotalFrequency;
	int n;
	int nIntervalFrequency;
	Continuous cLowerValue;
	Continuous cUpperValue;
	Continuous cNextLowerValue;
	Continuous cLowerBound;
	Continuous cUpperBound;

	require(tableBuilder != NULL);
	require(outputHistogram != NULL);
	require(outputHistogram->GetIntervalNumber() > 0);
	require(outputHistogram->ComputeTotalFrequency() == tableBuilder->GetTotalFrequency());

	// Affichage de l'histogramme de depart
	if (bDisplay)
		cout << "Initial histogram\n" << *outputHistogram << endl;

	// Mise a jour du nombre de valeurs distinctes
	outputHistogram->SetDistinctValueNumber(tableBuilder->GetDistinctValueNumber());

	// Parcours des vecteurs de l'histogramme algorithmique pour creer les intervalles
	nTotalFrequency = 0;
	for (n = 0; n < outputHistogram->GetIntervalNumber(); n++)
	{
		outputInterval = outputHistogram->GetIntervalAt(n);
		assert(outputInterval->CheckBounds());
		assert(n == outputHistogram->GetIntervalNumber() - 1 or
		       outputInterval->GetUpperBound() == outputHistogram->GetIntervalAt(n + 1)->GetLowerBound());

		// On verifie que deux intervalles de suite ne peuvent etre vide
		assert(n == 0 or outputInterval->GetFrequency() > 0 or
		       outputHistogram->GetIntervalAt(n - 1)->GetFrequency() > 0);

		// Recherche des caracteristiques de l'intervalle
		nIntervalFrequency = outputInterval->GetFrequency();
		if (nIntervalFrequency > 0)
		{
			cLowerValue = tableBuilder->GetBinLowerValueAtFrequency(nTotalFrequency);
			cUpperValue =
			    tableBuilder->GetBinUpperValueAtFrequency(nTotalFrequency + nIntervalFrequency - 1);
		}
		else
		{
			cLowerValue = KWContinuous::GetMissingValue();
			cUpperValue = KWContinuous::GetMissingValue();
		}

		// Borne inf, en prenant la borne sup de l'intervalle precedent, sauf pour le premier intervalle
		if (n == 0)
		{
			cLowerBound = KWContinuous::DoubleToContinuous(outputInterval->GetLowerBound());

			// On ignore le cas d'un premier intervalle vide
			if (cLowerValue != KWContinuous::GetMissingValue())
			{
				// Deplacement de la borne inf si probleme suite aux arrondis
				if (cLowerBound != KWContinuous::GetMinValue() and cLowerBound >= cLowerValue)
					cLowerBound = MHContinuousLimits::ComputeClosestLowerBound(cLowerValue);
			}
		}
		else
			cLowerBound =
			    cast(MHHistogramInterval*, outputHistogram->GetIntervals()->GetAt(n - 1))->GetUpperBound();

		// Calcul de la borne sup, sauf pour le dernier intervalle
		if (n == outputHistogram->GetIntervalNumber() - 1)
		{
			cUpperBound = KWContinuous::DoubleToContinuous(outputInterval->GetUpperBound());

			// On ignore la cas d'un dernier intervalle vide
			if (cUpperValue != KWContinuous::GetMissingValue())
			{
				// Deplacement de la borne sup si probleme suite aux arrondis
				if (cUpperBound < cUpperValue)
					cUpperBound = cUpperValue;
			}
		}
		else
		{
			////////////////////////////////////////////////////////////////////////////////////////
			// Calcul de la borne sup d'un intervalle
			// En theorie, ce calcul ne pose aucun probleme
			// En pratique, il y a des problemes d'arrondi potentiel entre les calculs menes au niveau
			// de la precision des doubles, la projection des bornes sur l'espace des Continuous,
			// l'utilisation des fonction approchees ComputeClosestLowerBound et ComputeClosestUpperBound
			// de MHContinuousLimits.
			// Rappelons que les intervalles sont de la forme ]LowerBound; UpperBound], et donc
			// que LowerBound < LowerValue et UpperValue <= UpperBoound pour les intervalles non vide
			// Les instructions ci-dessous visent a s'assurer des ces contraintes en deplacant legerement
			// bornes issue du calcul initial. Bien que tres rarement declenchees, cette methodes le
			// sont neanmoins quand on arrive aux limites de la precision numerique.

			// Recherche de la valeur inf du prochain intervalle
			cNextLowerValue =
			    tableBuilder->GetBinLowerValueAtFrequency(nTotalFrequency + nIntervalFrequency);
			assert(cUpperValue < cNextLowerValue);

			// On projette la valeur sur les Continuous
			cUpperBound = KWContinuous::DoubleToContinuous(outputInterval->GetUpperBound());

			// Si on est inferieur a la borne inf, on deplace legerement la borne sup
			if (cUpperBound <= cLowerBound)
				cUpperBound = MHContinuousLimits::ComputeClosestUpperBound(cLowerBound);

			// Si la projection rend la valeur plus petite que la valeur max de l'intervalle dans le cas
			// d'un intervalle non vide, on prend cette valeur max
			if (nIntervalFrequency > 0 and cUpperBound < cUpperValue)
				cUpperBound = cUpperValue;
			// Si la projection rend la valeur plus grande que la plus petite valeur suivante,
			// on cherche la plus grande valeur strictement plus petite que cette valeur
			else if (cUpperBound >= cNextLowerValue)
			{
				// On prend la plus petite valeur plus petite que la borne sup
				cUpperBound = MHContinuousLimits::ComputeClosestLowerBound(cNextLowerValue);

				// On recompare a la valeur max de l'intervalle
				if (nIntervalFrequency > 0 and cUpperBound < cUpperValue)
					cUpperBound = cUpperValue;
				// On recompare a la borne inf de l'intervalle
				else if (cUpperBound <= cLowerBound)
					cUpperBound = KWContinuous::GetLowerMeanValue(cLowerBound, cNextLowerValue);
			}
		}

		// Controles d'integrite
		assert(n == 0 or cLowerBound < tableBuilder->GetBinLowerValueAtFrequency(nTotalFrequency));
		assert(n == 0 or tableBuilder->GetBinUpperValueAtFrequency(nTotalFrequency + nIntervalFrequency - 1) <=
				     cUpperBound);
		assert(n < outputHistogram->GetIntervalNumber() - 1 or
		       nTotalFrequency + nIntervalFrequency == tableBuilder->GetTotalFrequency());
		assert(n == outputHistogram->GetIntervalNumber() - 1 or
		       cUpperBound < tableBuilder->GetBinLowerValueAtFrequency(nTotalFrequency + nIntervalFrequency));
		assert(n == 0 or cLowerBound < cUpperBound);
		assert(n == 0 or nIntervalFrequency == 0 or cLowerBound < cLowerValue);
		assert(nIntervalFrequency == 0 or cUpperValue <= cUpperBound);

		// Specification de l'intervalle en sortie
		outputInterval->SetLowerBound(cLowerBound);
		outputInterval->SetUpperBound(cUpperBound);
		outputInterval->SetLowerValue(cLowerValue);
		outputInterval->SetUpperValue(cUpperValue);
		assert(outputInterval->Check());

		// On verifie que deux intervalles de suite ne peuvent etre vide
		assert(outputHistogram->GetIntervalNumber() <= 1 or outputInterval->GetFrequency() > 0 or
		       outputHistogram->GetConstIntervalAt(outputHistogram->GetIntervalNumber() - 1)->GetFrequency() >
			   0);

		// Cumul des effectifs
		nTotalFrequency += nIntervalFrequency;
	}

	// Affichage de l'histogramme final
	if (bDisplay)
		cout << "Final histogram\n" << *outputHistogram << endl;
}

void MHDiscretizerHistogramMODL_fp::TraceOpen() const
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

void MHDiscretizerHistogramMODL_fp::TraceClose() const
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

boolean MHDiscretizerHistogramMODL_fp::IsTraceOpened() const
{
	return bIsTraceOpened;
}

void MHDiscretizerHistogramMODL_fp::TraceBeginBlock(const ALString& sBlockName) const
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

void MHDiscretizerHistogramMODL_fp::TraceEndBlock(const ALString& sBlockName) const
{
	if (bIsTraceOpened)
	{
		assert(nTraceBlockLevel >= 0);
		nTraceBlockLevel--;
		Trace("End\t" + sBlockName);
	}
	ensure(nTraceBlockLevel >= 0);
}

void MHDiscretizerHistogramMODL_fp::Trace(const ALString& sLine) const
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

void MHDiscretizerHistogramMODL_fp::PreparedCleanedStandardFrequencyTable(
    const MHHistogram* histogram, int nMissingValueNumber, KWFrequencyTable*& standardFrequencyTable) const
{
	boolean bDeleteEmptyIntervals = true;
	const MHHistogramInterval* sourceInterval;
	MHHistogramVector_fp* targetVector;
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
	standardFrequencyTable->Initialize(nFinalIntervalNumber);

	// Ajout eventuel d'un premier intervalle pour les valeurs manquantes
	if (nMissingValueNumber > 0)
	{
		targetVector = cast(MHHistogramVector_fp*, standardFrequencyTable->GetFrequencyVectorAt(0));
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
			    cast(MHHistogramVector_fp*, standardFrequencyTable->GetFrequencyVectorAt(nIntervalIndex));
			targetVector->SetFrequency(sourceInterval->GetFrequency());
			nIntervalIndex++;
		}
	}
}

void MHDiscretizerHistogramMODL_fp::InitializeFrequencyVector(KWFrequencyVector* kwfvFrequencyVector) const
{
	MHHistogramVector_fp* frequencyVector;

	require(kwfvFrequencyVector != NULL);

	// Acces aux vecteurs dans le bon type
	frequencyVector = cast(MHHistogramVector_fp*, kwfvFrequencyVector);

	// On n'appelle pas la methode ancetre, qui se base sur des KWDenseFrequencyVector
	frequencyVector->SetFrequency(0);
	frequencyVector->SetLowerBound(0);
	frequencyVector->SetUpperBound(0);
}

boolean MHDiscretizerHistogramMODL_fp::CheckFrequencyVector(const KWFrequencyVector* kwfvFrequencyVector) const
{
	require(kwfvFrequencyVector != NULL);
	return true;
}

void MHDiscretizerHistogramMODL_fp::InitializeWorkingData(const KWFrequencyTable* histogramFrequencyTable) const
{
	MHMODLHistogramCosts_fp* floatingPointDiscretizationCosts;
	MHHistogramTable_fp* floatingPointHistogramFrequencyTable;
	double dHyperparametersCost;

	require(frequencyTableBuilder->IsInitialized());

	// On n'appelle pas la methode ancetre, qui se base sur des KWDenseFrequencyVector

	// Calcul du cout des hyper-parametres
	// Il est a noter qu'utiliser ici les valeurs min et lmax, ou les bornes du domaines revient au meme, car ce
	// sont les memes main bins qui sont concernes
	dHyperparametersCost = MHMODLHistogramCosts_fp::ComputeFloatingPointBinCost(
	    false, frequencyTableBuilder->GetMinValue() > 0 ? 1 : -1, frequencyTableBuilder->GetMinValueExponent());
	dHyperparametersCost += MHMODLHistogramCosts_fp::ComputeFloatingPointBinCost(
	    false, frequencyTableBuilder->GetMaxValue() > 0 ? 1 : -1, frequencyTableBuilder->GetMaxValueExponent());
	if (frequencyTableBuilder->GetMinValue() <= 0 and frequencyTableBuilder->GetMaxValue() > 0)
		dHyperparametersCost += MHMODLHistogramCosts_fp::ComputeCentralBinExponentCost(
		    frequencyTableBuilder->GetMinCentralBinExponent());
	dHyperparametersCost += MHMODLHistogramCosts_fp::ComputeDomainBoundsMantissaCost(
	    frequencyTableBuilder->GetDomainBoundsMantissaBitNumber());

	// Parametrage correct des couts
	floatingPointHistogramFrequencyTable = cast(MHHistogramTable_fp*, histogramFrequencyTable);
	floatingPointDiscretizationCosts = cast(MHMODLHistogramCosts_fp*, GetDiscretizationCosts());
	floatingPointDiscretizationCosts->SetTotalInstanceNumber(
	    ComputeHistogramTotalFrequency(histogramFrequencyTable));
	floatingPointDiscretizationCosts->SetHyperParametersCost(dHyperparametersCost);
	floatingPointDiscretizationCosts->SetMinCentralBinExponent(frequencyTableBuilder->GetMinCentralBinExponent());
	floatingPointDiscretizationCosts->SetMaxCentralBinExponent(frequencyTableBuilder->GetMaxCentralBinExponent());
	floatingPointDiscretizationCosts->SetCentralBinExponent(
	    floatingPointHistogramFrequencyTable->GetCentralBinExponent());
	floatingPointDiscretizationCosts->SetHierarchicalLevel(
	    floatingPointHistogramFrequencyTable->GetHierarchyLevel());
	floatingPointDiscretizationCosts->SetMinBinLength(floatingPointHistogramFrequencyTable->GetMinBinLength());
	floatingPointDiscretizationCosts->SetPartileNumber(histogramFrequencyTable->GetGranularizedValueNumber());
}

void MHDiscretizerHistogramMODL_fp::CleanWorkingData() const
{
	MHMODLHistogramCosts_fp* floatingPointDiscretizationCosts;

	// On n'appelle pas la methode ancetre, qui se base sur des KWDenseFrequencyVector

	// Nettoyage du reste du parametrage
	floatingPointDiscretizationCosts = cast(MHMODLHistogramCosts_fp*, GetDiscretizationCosts());
	floatingPointDiscretizationCosts->SetTotalInstanceNumber(0);
	floatingPointDiscretizationCosts->SetHyperParametersCost(0);
	floatingPointDiscretizationCosts->SetMinBinLength(0);
	floatingPointDiscretizationCosts->SetMinCentralBinExponent(0);
	floatingPointDiscretizationCosts->SetMaxCentralBinExponent(0);
	floatingPointDiscretizationCosts->SetCentralBinExponent(0);
	floatingPointDiscretizationCosts->SetHierarchicalLevel(0);
	floatingPointDiscretizationCosts->SetPartileNumber(0);
}

void MHDiscretizerHistogramMODL_fp::AddFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
						       const KWFrequencyVector* kwfvAddedFrequencyVector) const
{
	boolean bDisplay = false;
	MHHistogramVector_fp* sourcePartFrequencyVector;
	const MHHistogramVector_fp* addedPartFrequencyVector;

	// Acces aux vecteurs dans le bon type
	sourcePartFrequencyVector = cast(MHHistogramVector_fp*, kwfvSourceFrequencyVector);
	addedPartFrequencyVector = cast(const MHHistogramVector_fp*, kwfvAddedFrequencyVector);
	assert((sourcePartFrequencyVector->GetLowerBound() == 0 and sourcePartFrequencyVector->GetUpperBound() == 0) or
	       sourcePartFrequencyVector->GetUpperBound() == addedPartFrequencyVector->GetLowerBound() or
	       sourcePartFrequencyVector->GetLowerBound() == addedPartFrequencyVector->GetUpperBound());

	// Affichage initial
	if (bDisplay)
	{
		cout << "AddFrequencyVector\n";
		cout << "Source\t" << *sourcePartFrequencyVector << "\n";
		cout << "Added\t" << *addedPartFrequencyVector << "\n";
	}

	// Prise en compte de l'effectif et des bornes
	sourcePartFrequencyVector->SetFrequency(sourcePartFrequencyVector->GetFrequency() +
						addedPartFrequencyVector->GetFrequency());

	// Recopie des bornes le cas d'un vecteur non initialise
	if (sourcePartFrequencyVector->GetLowerBound() == 0 and sourcePartFrequencyVector->GetUpperBound() == 0)
	{
		sourcePartFrequencyVector->SetLowerBound(addedPartFrequencyVector->GetLowerBound());
		sourcePartFrequencyVector->SetUpperBound(addedPartFrequencyVector->GetUpperBound());
	}
	// Mise a jour des bornes sinon
	else
	{
		sourcePartFrequencyVector->SetLowerBound(
		    min(sourcePartFrequencyVector->GetLowerBound(), addedPartFrequencyVector->GetLowerBound()));
		sourcePartFrequencyVector->SetUpperBound(
		    max(sourcePartFrequencyVector->GetUpperBound(), addedPartFrequencyVector->GetUpperBound()));
	}

	// Affichage final
	if (bDisplay)
	{
		cout << "Result\t" << *sourcePartFrequencyVector << "\n";
	}
}

void MHDiscretizerHistogramMODL_fp::RemoveFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
							  const KWFrequencyVector* kwfvRemovedFrequencyVector) const
{
	boolean bDisplay = false;
	MHHistogramVector_fp* sourcePartFrequencyVector;
	const MHHistogramVector_fp* removedPartFrequencyVector;

	// Acces aux vecteurs dans le bon type
	sourcePartFrequencyVector = cast(MHHistogramVector_fp*, kwfvSourceFrequencyVector);
	removedPartFrequencyVector = cast(const MHHistogramVector_fp*, kwfvRemovedFrequencyVector);
	assert(sourcePartFrequencyVector->GetLowerBound() != 0 or sourcePartFrequencyVector->GetUpperBound() != 0);
	assert(sourcePartFrequencyVector->GetLowerBound() == removedPartFrequencyVector->GetLowerBound() or
	       sourcePartFrequencyVector->GetUpperBound() == removedPartFrequencyVector->GetUpperBound());

	// Affichage initial
	if (bDisplay)
	{
		cout << "RemoveFrequencyVector\n";
		cout << "Source\t" << *sourcePartFrequencyVector << "\n";
		cout << "Removed\t" << *removedPartFrequencyVector << "\n";
	}

	// Prise en compte de l'effectif et des bornes
	sourcePartFrequencyVector->SetFrequency(sourcePartFrequencyVector->GetFrequency() -
						removedPartFrequencyVector->GetFrequency());
	if (sourcePartFrequencyVector->GetLowerBound() == removedPartFrequencyVector->GetLowerBound())
	{
		assert(sourcePartFrequencyVector->GetUpperBound() > removedPartFrequencyVector->GetUpperBound());
		sourcePartFrequencyVector->SetLowerBound(removedPartFrequencyVector->GetUpperBound());
	}
	else
	{
		assert(sourcePartFrequencyVector->GetLowerBound() < removedPartFrequencyVector->GetLowerBound());
		sourcePartFrequencyVector->SetUpperBound(removedPartFrequencyVector->GetLowerBound());
	}

	// Affichage final
	if (bDisplay)
	{
		cout << "Result\t" << *sourcePartFrequencyVector << "\n";
	}
}

void MHDiscretizerHistogramMODL_fp::MergeTwoFrequencyVectors(KWFrequencyVector* kwfvSourceFrequencyVector,
							     const KWFrequencyVector* kwfvMergedFrequencyVector1,
							     const KWFrequencyVector* kwfvMergedFrequencyVector2) const
{
	// Fusion des deux intervalles
	kwfvSourceFrequencyVector->CopyFrom(kwfvMergedFrequencyVector1);
	AddFrequencyVector(kwfvSourceFrequencyVector, kwfvMergedFrequencyVector2);
}

void MHDiscretizerHistogramMODL_fp::MergeThreeFrequencyVectors(
    KWFrequencyVector* kwfvSourceFrequencyVector, const KWFrequencyVector* kwfvMergedFrequencyVector1,
    const KWFrequencyVector* kwfvMergedFrequencyVector2, const KWFrequencyVector* kwfvMergedFrequencyVector3) const
{
	// Fusion des trois intervalles
	kwfvSourceFrequencyVector->CopyFrom(kwfvMergedFrequencyVector1);
	AddFrequencyVector(kwfvSourceFrequencyVector, kwfvMergedFrequencyVector2);
	AddFrequencyVector(kwfvSourceFrequencyVector, kwfvMergedFrequencyVector3);
}

void MHDiscretizerHistogramMODL_fp::SplitFrequencyVector(KWFrequencyVector* kwfvSourceFrequencyVector,
							 KWFrequencyVector* kwfvNewFrequencyVector,
							 const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const
{
	// Coupure de l'intervalle
	kwfvNewFrequencyVector->CopyFrom(kwfvSourceFrequencyVector);
	RemoveFrequencyVector(kwfvNewFrequencyVector, kwfvFirstSubFrequencyVectorSpec);
	kwfvSourceFrequencyVector->CopyFrom(kwfvFirstSubFrequencyVectorSpec);
}

void MHDiscretizerHistogramMODL_fp::MergeSplitFrequencyVectors(
    KWFrequencyVector* kwfvSourceFrequencyVector1, KWFrequencyVector* kwfvSourceFrequencyVector2,
    const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const
{
	// Fusion des deux intervalles dans le premier
	AddFrequencyVector(kwfvSourceFrequencyVector2, kwfvSourceFrequencyVector1);

	// Coupure du nouvel interval
	RemoveFrequencyVector(kwfvSourceFrequencyVector2, kwfvFirstSubFrequencyVectorSpec);
	kwfvSourceFrequencyVector1->CopyFrom(kwfvFirstSubFrequencyVectorSpec);
}

void MHDiscretizerHistogramMODL_fp::MergeMergeSplitFrequencyVectors(
    KWFrequencyVector* kwfvSourceFrequencyVector1, const KWFrequencyVector* kwfvSourceFrequencyVector2,
    KWFrequencyVector* kwfvSourceFrequencyVector3, const KWFrequencyVector* kwfvFirstSubFrequencyVectorSpec) const
{
	// Fusion des trois intervalles dans le dernier
	AddFrequencyVector(kwfvSourceFrequencyVector3, kwfvSourceFrequencyVector2);
	AddFrequencyVector(kwfvSourceFrequencyVector3, kwfvSourceFrequencyVector1);

	// Coupure du nouvel interval
	RemoveFrequencyVector(kwfvSourceFrequencyVector3, kwfvFirstSubFrequencyVectorSpec);
	kwfvSourceFrequencyVector1->CopyFrom(kwfvFirstSubFrequencyVectorSpec);
}
