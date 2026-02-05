// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "MHDiscretizerTruncationMODLHistogram.h"

MHDiscretizerTruncationMODLHistogram::MHDiscretizerTruncationMODLHistogram()
{
	dTruncationEpsilon = 0;
}

MHDiscretizerTruncationMODLHistogram::~MHDiscretizerTruncationMODLHistogram() {}

KWDiscretizer* MHDiscretizerTruncationMODLHistogram::Create() const
{
	return new MHDiscretizerTruncationMODLHistogram;
}

MHFloatingPointFrequencyTableBuilder* MHDiscretizerTruncationMODLHistogram::NewFrequencyTableBuilder() const
{
	MHFloatingPointFrequencyTableBuilder* tableBuilder;
	MHTruncationFloatingPointFrequencyTableBuilder* truncationTableBuilder;

	// Cas standard
	if (dTruncationEpsilon == 0)
		tableBuilder = new MHFloatingPointFrequencyTableBuilder;
	// Cas specialise pour un epsilon de troncature strictement positif
	else
	{
		truncationTableBuilder = new MHTruncationFloatingPointFrequencyTableBuilder;
		truncationTableBuilder->SetTruncationEpsilon(dTruncationEpsilon);
		tableBuilder = truncationTableBuilder;
	}
	return tableBuilder;
}

MHTruncationFloatingPointFrequencyTableBuilder*
MHDiscretizerTruncationMODLHistogram::GetTruncationFrequencyTableBuilder() const
{
	return cast(MHTruncationFloatingPointFrequencyTableBuilder*, frequencyTableBuilder);
}

void MHDiscretizerTruncationMODLHistogram::MainDiscretizeBins(const ContinuousVector* cvSourceBinLowerValues,
							      const ContinuousVector* cvSourceBinUpperValues,
							      const IntVector* ivSourceBinFrequencies,
							      MHHistogram*& optimizedHistogram,
							      MHHistogram*& postprocessedOptimizedHistogram,
							      ObjectArray* oaCoarsenedHistograms) const
{
	require(oaCoarsenedHistograms != NULL);
	require(oaCoarsenedHistograms->GetSize() == 0);

	// Appel de la methode standard si pas de troncature
	if (not GetHistogramSpec()->GetTruncationManagementHeuristic())
		MHDiscretizerMODLHistogram::MainDiscretizeBins(cvSourceBinLowerValues, cvSourceBinUpperValues,
							       ivSourceBinFrequencies, optimizedHistogram,
							       postprocessedOptimizedHistogram, oaCoarsenedHistograms);
	// Sinon, appel d'une methode specialise pour la troncature
	else
		TruncationDiscretizeBins(cvSourceBinLowerValues, cvSourceBinUpperValues, ivSourceBinFrequencies,
					 optimizedHistogram, postprocessedOptimizedHistogram, oaCoarsenedHistograms);
}

void MHDiscretizerTruncationMODLHistogram::TruncationDiscretizeBins(const ContinuousVector* cvSourceBinLowerValues,
								    const ContinuousVector* cvSourceBinUpperValues,
								    const IntVector* ivSourceBinFrequencies,
								    MHHistogram*& optimizedHistogram,
								    MHHistogram*& postprocessedOptimizedHistogram,
								    ObjectArray* oaCoarsenedHistograms) const
{
	boolean bIsTruncationNecessary;
	ContinuousVector* cvSourceDeltaBinLowerValues;
	ContinuousVector* cvSourceDeltaBinUpperValues;
	IntVector* ivSourceDeltaBinFrequencies;
	MHHistogram* optimizedDeltaValueHistogram;
	MHHistogram* optimizedTruncationHistogram;
	MHHistogram* postprocessedOptimizedTruncationHistogram;
	ContinuousVector cvTransformedValues;
	MHHistogram* coarsenedHistogram;
	Continuous cMeadianValue;
	int n;

	require(GetHistogramSpec()->GetTruncationManagementHeuristic());
	require(oaCoarsenedHistograms != NULL);
	require(oaCoarsenedHistograms->GetSize() == 0);

	TraceBeginBlock("Truncation management heuristic");

	// Initialisation des resultats
	optimizedHistogram = NULL;
	postprocessedOptimizedHistogram = NULL;
	optimizedTruncationHistogram = NULL;
	postprocessedOptimizedTruncationHistogram = NULL;

	// Appel prealable de la methode standard pour avoir au minimum l'histogramme de base
	TraceBeginBlock("Standard histogram algorithm");
	MHDiscretizerMODLHistogram::MainDiscretizeBins(cvSourceBinLowerValues, cvSourceBinUpperValues,
						       ivSourceBinFrequencies, optimizedHistogram,
						       postprocessedOptimizedHistogram, oaCoarsenedHistograms);
	TraceEndBlock("Standard histogram algorithm");

	// Initialisation avec le epsilon de troncature utilisateur
	dTruncationEpsilon = GetHistogramSpec()->GetEpsilonBinWidth();

	// S'il n'y a pas de epsilon utilisateur impose, on recherche de facon heuristique le bon epsilon de troncature
	if (dTruncationEpsilon == 0)
	{
		// Cas particulier ou il n'y a que deux valeurs, mais un histogramme a plus de un intervalle, comportant
		// donc au moins un Dirac
		if (optimizedHistogram->GetDistinctValueNumber() == 2 and optimizedHistogram->GetIntervalNumber() > 1)
		{
			// L'heuristique basee sur l'histogrammes des variations de valeurs ne marche pas ici,
			// car il n'y a qu'une seule variation de valeur et il en faut au moins deux
			// Avec deux valeur, l'histogramme optimal pour l'estimation de densite peut comporter un ou
			// deux Diracs, ce qui est perturbant pour l'analyse exploratoire On force donc un comportement
			// similaire a la troncature en se basant sur la difference entre les deux valeurs
			dTruncationEpsilon = optimizedHistogram->GetMaxValue() - optimizedHistogram->GetMinValue();
			assert(dTruncationEpsilon > 0);
		}
		// Cas particulier ou il n'y a que trois valeurs, mais un histogramme a plus de un intervalle
		// On ajoute ce cas particulier un peu pousse pour aller un peu au dela des cas de variable binaires,
		// mais on ira pas plus loin dans la gestion des cas limites
		else if (optimizedHistogram->GetDistinctValueNumber() == 3 and
			 optimizedHistogram->GetIntervalNumber() > 1)
		{
			// Recherche de la troisieme valeur differente du min et du max
			cMeadianValue = optimizedHistogram->GetMinValue();
			for (n = cvSourceBinUpperValues->GetSize() - 2; n > 0; n--)
			{
				cMeadianValue = cvSourceBinUpperValues->GetAt(n);
				if (cMeadianValue < optimizedHistogram->GetMaxValue())
					break;
			}
			assert(optimizedHistogram->GetMinValue() < cMeadianValue);
			assert(cMeadianValue < optimizedHistogram->GetMaxValue());

			// On applique l'heuristique de troncature si la valeur medianne est exactement equidistante des
			// valeurs min et max Cela marchera donc au moins dans le cas des entiers
			if (optimizedHistogram->GetMaxValue() - cMeadianValue ==
			    cMeadianValue - optimizedHistogram->GetMinValue())
				dTruncationEpsilon = optimizedHistogram->GetMaxValue() - cMeadianValue;
		}
		// Cas general
		else
		{
			// Test si l'heuristique de troncature est necessaire
			bIsTruncationNecessary = IsTruncationNecessary(optimizedHistogram);

			// Recherche de l'epsilon de troncature sur la base des variations de valeurs
			if (bIsTruncationNecessary)
			{
				// Calcul de l'histogramme des variations de valeurs
				DiscretizeDeltaValues(cvSourceBinLowerValues, cvSourceBinUpperValues,
						      ivSourceBinFrequencies, cvSourceDeltaBinLowerValues,
						      cvSourceDeltaBinUpperValues, ivSourceDeltaBinFrequencies,
						      optimizedDeltaValueHistogram);
				assert(optimizedDeltaValueHistogram->ComputeTotalFrequency() ==
				       optimizedHistogram->ComputeTotalFrequency() - 1);

				// Calcul du epsilon de troncature
				dTruncationEpsilon =
				    ComputeTruncationEpsilon(cvSourceDeltaBinLowerValues, cvSourceDeltaBinUpperValues,
							     ivSourceDeltaBinFrequencies, optimizedDeltaValueHistogram);

				// Nettoyage
				if (cvSourceDeltaBinLowerValues != NULL)
					delete cvSourceDeltaBinLowerValues;
				if (cvSourceDeltaBinUpperValues != NULL)
					delete cvSourceDeltaBinUpperValues;
				if (ivSourceDeltaBinFrequencies != NULL)
					delete ivSourceDeltaBinFrequencies;
				delete optimizedDeltaValueHistogram;
			}
		}
	}

	// Calcul d'un histogramme avec le parametre de troncature
	if (dTruncationEpsilon > 0)
	{
		// On detruit prealablement les histogrammes simplifies issue de la version sans troncature
		oaCoarsenedHistograms->DeleteAll();

		// Calcul d'un histogramme avec les donnees tronquees
		TraceBeginBlock("Truncated histogram algorithm");
		GetHistogramSpec()->SetTruncationMode(true);
		MHDiscretizerMODLHistogram::MainDiscretizeBins(
		    cvSourceBinLowerValues, cvSourceBinUpperValues, ivSourceBinFrequencies,
		    optimizedTruncationHistogram, postprocessedOptimizedTruncationHistogram, oaCoarsenedHistograms);
		GetHistogramSpec()->SetTruncationMode(false);
		TraceEndBlock("Truncated histogram algorithm");

		// On garde l'histogramme optimise initial
		assert(optimizedHistogram != NULL);

		// On prend l'histogramme post-traite issue de l'heuristique de troncature
		if (postprocessedOptimizedHistogram != NULL)
			delete postprocessedOptimizedHistogram;
		if (postprocessedOptimizedTruncationHistogram != NULL)
		{
			postprocessedOptimizedHistogram = postprocessedOptimizedTruncationHistogram;
			delete optimizedTruncationHistogram;
		}
		else
			postprocessedOptimizedHistogram = optimizedTruncationHistogram;

		// On memorise le parametre de troncature utilise
		postprocessedOptimizedHistogram->SetTruncationEpsilon(dTruncationEpsilon);
		for (n = 0; n < oaCoarsenedHistograms->GetSize(); n++)
		{
			coarsenedHistogram = cast(MHHistogram*, oaCoarsenedHistograms->GetAt(n));
			coarsenedHistogram->SetTruncationEpsilon(dTruncationEpsilon);
		}

		// Nettoyage de la gestion de la troncature
		dTruncationEpsilon = 0;
	}
	TraceEndBlock("Truncation management heuristic");
	ensure(optimizedHistogram != NULL);
}

boolean MHDiscretizerTruncationMODLHistogram::IsTruncationNecessary(const MHHistogram* optimizedHistogram) const
{
	boolean bIsTruncationNecessary;
	ALString sTmp;

	require(optimizedHistogram != NULL);

	// Le critere est l'existence de spikes parmi les intervalles de l'histogramme
	bIsTruncationNecessary = optimizedHistogram->ComputeSpikeIntervalNumber() > 0;
	Trace(sTmp + "Is truncation necessary\t" + BooleanToString(bIsTruncationNecessary));
	return bIsTruncationNecessary;
}

void MHDiscretizerTruncationMODLHistogram::DiscretizeDeltaValues(const ContinuousVector* cvSourceBinLowerValues,
								 const ContinuousVector* cvSourceBinUpperValues,
								 const IntVector* ivSourceBinFrequencies,
								 ContinuousVector*& cvSourceDeltaBinLowerValues,
								 ContinuousVector*& cvSourceDeltaBinUpperValues,
								 IntVector*& ivSourceDeltaBinFrequencies,
								 MHHistogram*& optimizedDeltaValueHistogram) const
{
	boolean bExportDeltaValues = false;
	MHHistogramSpec referenceHistogramSpec;
	MHDiscretizerMODLHistogram deltaValueDiscretizerHistogram;
	MHHistogram* postProcessedOptimizedDeltaValueHistogram;
	ObjectArray oaCoarsenedHistograms;
	int n;
	Continuous cBinLowerValue;
	Continuous cBinUpperValue;
	Continuous cDeltaValueBinUpperValue;
	int nBinFrequency;
	MHBin* deltaValueBin;
	ObjectArray oaDeltaValueBins;

	require(cvSourceBinUpperValues != NULL);

	TraceBeginBlock("Compute histogram on delta values");

	// Initialisation des vectuers en sortie
	cvSourceDeltaBinLowerValues = NULL;
	cvSourceDeltaBinUpperValues = NULL;
	ivSourceDeltaBinFrequencies = NULL;

	// Calcul des bins des differences de valeurs
	for (n = 0; n < cvSourceBinUpperValues->GetSize(); n++)
	{
		// Initialisation dans le cas minmaliste d'un seul vecteur en entree
		cBinUpperValue = cvSourceBinUpperValues->GetAt(n);

		// Initialisation par defaut des autres caracteristiques du bin
		cBinLowerValue = cBinUpperValue;
		nBinFrequency = 1;

		// Innitialisation complementaires si les autres vecteurs sont disponibles
		if (cvSourceBinLowerValues != NULL)
			cBinLowerValue = cvSourceBinLowerValues->GetAt(n);
		if (ivSourceBinFrequencies != NULL)
			nBinFrequency = ivSourceBinFrequencies->GetAt(n);
		assert(cBinLowerValue <= cBinUpperValue);
		assert(nBinFrequency >= 0);

		// Cas d'un bin comportant au moins deux instances
		if (nBinFrequency > 1)
		{
			// Cas d'un bin ayant une seule valeur
			if (cBinLowerValue == cBinUpperValue)
			{
				deltaValueBin = new MHBin;
				deltaValueBin->Initialize(0, 0, nBinFrequency - 1);
				oaDeltaValueBins.Add(deltaValueBin);
			}
			// Cas d'un bin avec au moins deux valeurs
			else
			{
				// En principe, on devrait creer un intervalle de variations de valeurs autour de 0,
				// mais cela inhiberait systematiquement toute possibilite de troncature
				// On considere donc que toutes les valeurs des valeurs extremites, et on cree
				// donc une variation de valeur egale a la longueur de bin, et toute les autres a 0
				// Il s'agit d'un choix heuristique visant a garder au maximum la possibilite
				// d'activer l'heuristique de troncature, tout en restant conservateur, car il n'est de
				// toute facon pas possible d'avoir des intervalles plus petits que les bins

				// Ajout d'une variatrion egale a la longueur du bin
				cDeltaValueBinUpperValue =
				    KWContinuous::DoubleToContinuous(cBinUpperValue - cBinLowerValue);
				deltaValueBin = new MHBin;
				deltaValueBin->Initialize(cDeltaValueBinUpperValue, cDeltaValueBinUpperValue, 1);
				oaDeltaValueBins.Add(deltaValueBin);

				// Ajout des variations restantes de longueur 0
				deltaValueBin = new MHBin;
				deltaValueBin->Initialize(0, 0, nBinFrequency - 2);
				oaDeltaValueBins.Add(deltaValueBin);
			}
		}

		// Gestion de l'ecart entre deux bins successifs
		if (n > 0)
		{
			// Ajout d'une variation egale a la distance entre les bins
			cDeltaValueBinUpperValue =
			    KWContinuous::DoubleToContinuous(cBinLowerValue - cvSourceBinUpperValues->GetAt(n - 1));
			deltaValueBin = new MHBin;
			deltaValueBin->Initialize(cDeltaValueBinUpperValue, cDeltaValueBinUpperValue, 1);
			oaDeltaValueBins.Add(deltaValueBin);
		}
	}

	// Tri des bins de variation de valeur
	MHBin::SortBinArray(&oaDeltaValueBins);

	// Creation des vecteurs pour les bins de variation de valeur
	cvSourceDeltaBinUpperValues = new ContinuousVector;
	ivSourceDeltaBinFrequencies = new IntVector;
	for (n = 0; n < oaDeltaValueBins.GetSize(); n++)
	{
		deltaValueBin = cast(MHBin*, oaDeltaValueBins.GetAt(n));
		assert(deltaValueBin->GetLowerValue() == deltaValueBin->GetUpperValue());

		// Ajout si changement de variation de valeur
		if (n == 0 or deltaValueBin->GetUpperValue() >
				  cvSourceDeltaBinUpperValues->GetAt(cvSourceDeltaBinUpperValues->GetSize() - 1))
		{
			cvSourceDeltaBinUpperValues->Add(deltaValueBin->GetUpperValue());
			ivSourceDeltaBinFrequencies->Add(deltaValueBin->GetFrequency());
		}
		// Sinon, incrementation de l'effectif
		else
			ivSourceDeltaBinFrequencies->UpgradeAt(ivSourceDeltaBinFrequencies->GetSize() - 1,
							       deltaValueBin->GetFrequency());
	}

	// Nettoyage
	oaDeltaValueBins.DeleteAll();

	// Export des difference de valeur
	if (bExportDeltaValues)
	{
		ExportData(
		    cvSourceDeltaBinLowerValues, cvSourceDeltaBinUpperValues, ivSourceDeltaBinFrequencies,
		    FileService::BuildFilePathName(GetHistogramSpec()->GetResultFilesDirectory(), "DeltaValues.txt"));
	}

	// Specialisation des specification d'histogrammes, pour utiliser le central bin exponent minimal
	referenceHistogramSpec.CopyFrom(GetHistogramSpec());
	GetHistogramSpec()->SetDeltaCentralBinExponent(0);
	GetHistogramSpec()->SetDeltaValues(true);

	// On inhibe egalement toute contrainte utilisateur potentielles
	GetHistogramSpec()->SetEpsilonBinWidth(0);
	GetHistogramSpec()->SetTruncationManagementHeuristic(false);
	GetHistogramSpec()->SetSingularityRemovalHeuristic(false);

	// Calcul de l'histogramme
	deltaValueDiscretizerHistogram.MainDiscretizeBins(
	    cvSourceDeltaBinLowerValues, cvSourceDeltaBinUpperValues, ivSourceDeltaBinFrequencies,
	    optimizedDeltaValueHistogram, postProcessedOptimizedDeltaValueHistogram, &oaCoarsenedHistograms);

	// Aucune contrainte utilisateur n'est specifiee pour l'histogramme des variations de valeur
	assert(postProcessedOptimizedDeltaValueHistogram == NULL);

	// Memorisation de l'histogramme dans un fichier si les logs sont demandes
	if (GetHistogramSpec()->GetExportInternalLogFiles())
		deltaValueDiscretizerHistogram.WriteHistogramFile("Delta value histogram", optimizedDeltaValueHistogram,
								  GetHistogramSpec()->GetInternalHistogramFileName());

	// Restitution des specifications d'histogrammes
	GetHistogramSpec()->CopyFrom(&referenceHistogramSpec);

	TraceEndBlock("Compute histogram on delta values");
}

double MHDiscretizerTruncationMODLHistogram::ComputeTruncationEpsilon(
    ContinuousVector* cvSourceDeltaBinLowerValues, ContinuousVector* cvSourceDeltaBinUpperValues,
    IntVector* ivSourceDeltaBinFrequencies, const MHHistogram* optimizedDeltaValueHistogram) const
{
	const int nZeroIntervalIndex = 0;
	boolean bIsTruncationGap;
	double dTruncationGap;
	int nThirdIntervalFirstInstanceIndex;
	int nThirdIntervalLastInstanceIndex;
	int nInstanceIndex;
	int i;
	ALString sTmp;

	require(cvSourceDeltaBinLowerValues == NULL);
	require(cvSourceDeltaBinUpperValues != NULL);
	require(ivSourceDeltaBinFrequencies != NULL);
	require(optimizedDeltaValueHistogram != NULL);

	// Initialisations
	bIsTruncationGap = true;
	dTruncationGap = 0;

	// Test s'il y a au moins trois intervalles
	bIsTruncationGap =
	    bIsTruncationGap and optimizedDeltaValueHistogram->GetIntervalNumber() >= 3 + nZeroIntervalIndex;

	// Le premier intervalle doit etre non vide et ne contenir que la valeur 0
	bIsTruncationGap = bIsTruncationGap and
			   optimizedDeltaValueHistogram->GetConstIntervalAt(nZeroIntervalIndex)->GetFrequency() > 0;
	bIsTruncationGap = bIsTruncationGap and
			   optimizedDeltaValueHistogram->GetConstIntervalAt(nZeroIntervalIndex)->GetLowerValue() == 0;
	bIsTruncationGap = bIsTruncationGap and
			   optimizedDeltaValueHistogram->GetConstIntervalAt(nZeroIntervalIndex)->GetUpperValue() == 0;

	// Le deuxieme intervalle doit non vide
	bIsTruncationGap =
	    bIsTruncationGap and
	    optimizedDeltaValueHistogram->GetConstIntervalAt(nZeroIntervalIndex + 1)->GetFrequency() == 0;

	// Le troisieme intervalle doit etre non vide et de largeur strictement plus petite que le deuxieme interval
	bIsTruncationGap = bIsTruncationGap and
			   optimizedDeltaValueHistogram->GetConstIntervalAt(nZeroIntervalIndex + 2)->GetFrequency() > 0;
	bIsTruncationGap = bIsTruncationGap and
			   optimizedDeltaValueHistogram->GetConstIntervalAt(nZeroIntervalIndex + 2)->GetLength() <
			       optimizedDeltaValueHistogram->GetConstIntervalAt(nZeroIntervalIndex + 1)->GetLength();

	// Calcul du gap de troncature
	if (bIsTruncationGap)
	{
		assert(dTruncationGap == 0);

		// Premier et derniere instance du troisieme intervalle
		nThirdIntervalFirstInstanceIndex =
		    optimizedDeltaValueHistogram->GetConstIntervalAt(nZeroIntervalIndex)->GetFrequency();
		assert(optimizedDeltaValueHistogram->GetConstIntervalAt(nZeroIntervalIndex + 1)->GetFrequency() == 0);
		nThirdIntervalLastInstanceIndex =
		    nThirdIntervalFirstInstanceIndex +
		    optimizedDeltaValueHistogram->GetConstIntervalAt(nZeroIntervalIndex + 2)->GetFrequency();

		// Calcul de la moyenne des valeurs du troisieme intervalle
		nInstanceIndex = 0;
		for (i = 0; i < cvSourceDeltaBinUpperValues->GetSize(); i++)
		{
			// Prise en compte si on est au dela du trosieme intervalle
			if (nInstanceIndex >= nThirdIntervalFirstInstanceIndex)
			{
				// Prise en compte si on est avant la fin du troiseme intervalle
				if (nInstanceIndex < nThirdIntervalLastInstanceIndex)
				{
					assert(optimizedDeltaValueHistogram->GetConstIntervalAt(nZeroIntervalIndex + 2)
						   ->GetLowerBound() < cvSourceDeltaBinUpperValues->GetAt(i));
					assert(cvSourceDeltaBinUpperValues->GetAt(i) <=
					       optimizedDeltaValueHistogram->GetConstIntervalAt(nZeroIntervalIndex + 2)
						   ->GetUpperBound());
					dTruncationGap += cvSourceDeltaBinUpperValues->GetAt(i) *
							  ivSourceDeltaBinFrequencies->GetAt(i);
				}
				// On a fini sinon
				else
					break;
			}

			// Incrementaion de l'index d'instance
			nInstanceIndex += ivSourceDeltaBinFrequencies->GetAt(i);
		}
		dTruncationGap /=
		    optimizedDeltaValueHistogram->GetConstIntervalAt(nZeroIntervalIndex + 2)->GetFrequency();

		// On affine en utilisant l'arrondi procure par la conversion de double en Continuous
		dTruncationGap = KWContinuous::DoubleToContinuous(dTruncationGap);
	}
	Trace(sTmp + "Truncation epsilon\t" + KWContinuous::ContinuousToString(dTruncationGap));
	return dTruncationGap;
}

void MHDiscretizerTruncationMODLHistogram::FinalizeHistogram(MHHistogram* outputHistogram) const
{
	boolean bDisplay = false;
	MHHistogramInterval* previousInterval;
	MHHistogramInterval* outputInterval;
	boolean bAreIntervalsValid;
	int n;
	int nIndex;

	require(frequencyTableBuilder != NULL);
	require(outputHistogram != NULL);
	require(outputHistogram->Check());
	require(outputHistogram->GetIntervalNumber() > 0);
	require(outputHistogram->ComputeTotalFrequency() == frequencyTableBuilder->GetTotalFrequency());

	// Appel de la methode ancetre si pas de troncature
	if (dTruncationEpsilon == 0)
		MHDiscretizerMODLHistogram::FinalizeHistogram(outputHistogram);
	// Sinon, gestion de la troncature
	else
	{
		// Affichage de l'histogramme de depart
		if (bDisplay)
			cout << "Histogram with truncated data\n" << *outputHistogram << endl;

		// Modification temporaires des bornes de l'histogramme
		bAreIntervalsValid = true;
		for (n = 0; n < outputHistogram->GetIntervalNumber(); n++)
		{
			outputInterval = outputHistogram->GetIntervalAt(n);
			assert(outputInterval->CheckBounds());

			// Modification pour gerer la transformation inverse de la troncature
			outputInterval->SetLowerBound(GetTruncationFrequencyTableBuilder()->InverseTransformValue(
			    outputInterval->GetLowerBound()));
			outputInterval->SetUpperBound(GetTruncationFrequencyTableBuilder()->InverseTransformValue(
			    outputInterval->GetUpperBound()));

			// Au minimum le premier ou le dernier intervalle sont valides
			// Mais il peut y avoir des intervalles centraux dont la taille a ete reduit au point de rentrer
			// dans la singularite autour de 0 ou pour la borne inf du premier intervalle en cas de
			// troncature
			assert(outputInterval->CheckBounds() or
			       (n == 0 and outputInterval->GetLowerBound() <= KWContinuous::GetMinValue()) or
			       (n > 0 and n < outputHistogram->GetIntervalNumber() - 1));
			bAreIntervalsValid = bAreIntervalsValid and outputInterval->CheckBounds();
		}

		// Si les intervalles sont non valides, on va fusionner ceux qui sont dans la singularite autour de 0
		if (not bAreIntervalsValid)
		{
			nIndex = 0;
			for (n = 1; n < outputHistogram->GetIntervalNumber(); n++)
			{
				// Acces a l'intervalle precedent
				previousInterval = outputHistogram->GetIntervalAt(nIndex);
				assert(previousInterval->CheckBounds() or
				       previousInterval->GetLowerBound() <= KWContinuous::GetMinValue());

				// Acces a l'intervalle courant
				outputInterval = outputHistogram->GetIntervalAt(n);

				// On le prend en compte s'il est valide
				if (outputInterval->CheckBounds())
				{
					nIndex++;
					outputHistogram->GetIntervals()->SetAt(nIndex, outputInterval);
				}
				// Sinon on le fusionne avec l'intervalle precedent
				else
				{
					previousInterval->SetUpperBound(outputInterval->GetUpperBound());
					previousInterval->SetFrequency(previousInterval->GetFrequency() +
								       outputInterval->GetFrequency());

					// Prise en compte des valeurs
					assert(previousInterval->GetFrequency() > 0);
					if (previousInterval->GetLowerValue() == KWContinuous::GetMissingValue())
						previousInterval->SetLowerValue(outputInterval->GetLowerValue());
					if (outputInterval->GetUpperValue() != KWContinuous::GetMissingValue())
						previousInterval->SetUpperValue(outputInterval->GetUpperValue());
					assert(previousInterval->CheckBounds());

					// Destruction de l'intervalle veant d'etre fusionne
					delete outputInterval;
				}
			}
			outputHistogram->GetIntervals()->SetSize(nIndex + 1);
		}

		// Mise a jour des hyper-parametres
		outputHistogram->SetDomainLowerBound(GetTruncationFrequencyTableBuilder()->InverseTransformValue(
		    outputHistogram->GetDomainLowerBound()));
		outputHistogram->SetDomainUpperBound(GetTruncationFrequencyTableBuilder()->InverseTransformValue(
		    outputHistogram->GetDomainUpperBound()));

		// Memorisation de l'espilon de troncature
		outputHistogram->SetTruncationEpsilon(GetTruncationFrequencyTableBuilder()->GetTruncationEpsilon());

		// Appel de la methode ancetre avec le discretizer exploitant les valeurs initiales et les bornes des
		// intervalles transformees inverses
		AdjustHistogramIntervalBounds(GetTruncationFrequencyTableBuilder()->GetInitialBinsTableBuilder(),
					      outputHistogram);

		// Affichage de l'histogramme avec les donnees initiales
		if (bDisplay)
			cout << "Histogram with initial data\n" << *outputHistogram << endl;
	}
	ensure(outputHistogram->Check());
}
