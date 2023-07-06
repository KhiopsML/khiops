// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "MHDiscretizerMODLHistogram.h"
#include "MHTruncationFloatingPointFrequencyTableBuilder.h"
#include "MHBin.h"

//////////////////////////////////////////////////////////////////////////////////
// Construction d'histogramme en representation virgule flottante
// avec gestion d'une heuristique de troncature
class MHDiscretizerTruncationMODLHistogram : public MHDiscretizerMODLHistogram
{
public:
	// Constructeur
	MHDiscretizerTruncationMODLHistogram();
	~MHDiscretizerTruncationMODLHistogram();

	// Constructeur generique
	KWDiscretizer* Create() const;

	/////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Methode de creation d'un frequencyTableBuilder
	MHFloatingPointFrequencyTableBuilder* NewFrequencyTableBuilder() const override;

	// Acces au frequencyTableBuilder dans son type specialise
	MHTruncationFloatingPointFrequencyTableBuilder* GetTruncationFrequencyTableBuilder() const;

	// Methode principale de pilotage de la construction des histogrammes
	// prenant en compte l'ensemble du parametrage
	// Redefinition pour gerer la troncature
	void MainDiscretizeBins(const ContinuousVector* cvSourceBinLowerValues,
				const ContinuousVector* cvSourceBinUpperValues, const IntVector* ivSourceBinFrequencies,
				MHHistogram*& optimizedHistogram, MHHistogram*& postprocessedOptimizedHistogram,
				ObjectArray* oaCoarsenedHistograms) const override;

	// Construction d'histogramme avec prise en compte de la troncature
	void TruncationDiscretizeBins(const ContinuousVector* cvSourceBinLowerValues,
				      const ContinuousVector* cvSourceBinUpperValues,
				      const IntVector* ivSourceBinFrequencies, MHHistogram*& optimizedHistogram,
				      MHHistogram*& postprocessedOptimizedHistogram,
				      ObjectArray* oaCoarsenedHistograms) const;

	// Test si l'heuristique de troncature est necessaire, en inspectant les intervalles d'un histogramme
	boolean IsTruncationNecessary(const MHHistogram* optimizedHistogram) const;

	// Calcul d'un histogrammes pour les variations de valeurs
	// Permet d'identifier la presence de donnees tronquees, qui donneront lieu a des intervalles singuliers
	// Entree:
	//   . les vecteurs de descriptions des bins sources
	// Sorties:
	//   . les vecteurs de descriptions des bins de variation de valeurs
	//   . optimizedDeltaValueHistogram: histogramme optimal pour les variations de valeurs
	// Memoire:
	// Les sortrie sont a detruire par l'appelant. Attention, certains vecteurs de description de bins
	// peuvent etre a null
	void DiscretizeDeltaValues(const ContinuousVector* cvSourceBinLowerValues,
				   const ContinuousVector* cvSourceBinUpperValues,
				   const IntVector* ivSourceBinFrequencies,
				   ContinuousVector*& cvSourceDeltaBinLowerValues,
				   ContinuousVector*& cvSourceDeltaBinUpperValues,
				   IntVector*& ivSourceDeltaBinFrequencies,
				   MHHistogram*& optimizedDeltaValueHistogram) const;

	// Calcul d'un epsilon de troncature a partir d'un histogramme de variations de valeurs
	// On rend 0 s'il n'y a pas lieu de gerer les troncatures
	double ComputeTruncationEpsilon(ContinuousVector* cvSourceDeltaBinLowerValues,
					ContinuousVector* cvSourceDeltaBinUpperValues,
					IntVector* ivSourceDeltaBinFrequencies,
					const MHHistogram* optimizedDeltaValueHistogram) const;

	// Finalisation de l'histogramme
	// Dans le cas d'une troncature, certains intervalles construits avec le TruncationBinaryEspilon (plus grand que
	// le TruncationEpsilon) peuvent entrer en collision avec la singularite autour de 0 quand il sont retransformes
	// pour leur taille initiale. Dans ce cas, on fusionne les intervalles posant probleme avec leur predecesseur
	void FinalizeHistogram(MHHistogram* outputHistogram) const override;

	// Gestion de la troncature
	mutable double dTruncationEpsilon;
};