// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWPredictorBaseline.h"

KWPredictorBaseline::KWPredictorBaseline() {}

KWPredictorBaseline::~KWPredictorBaseline() {}

KWPredictor* KWPredictorBaseline::Create() const
{
	return new KWPredictorBaseline;
}

const ALString KWPredictorBaseline::GetName() const
{
	return "Baseline";
}

const ALString KWPredictorBaseline::GetPrefix() const
{
	return "B";
}

boolean KWPredictorBaseline::InternalTrain()
{
	KWDataPreparationClass dataPreparationClass;
	ObjectArray oaTheDataPreparationAttribute;

	require(Check());
	require(GetClassStats() != NULL);

	// Calcul des statistiques si necessaire
	if (not GetClassStats()->IsStatsComputed())
		GetClassStats()->ComputeStats();
	assert(not GetClassStats()->IsStatsComputed() or GetTargetDescriptiveStats() != NULL);

	// Apprentissage si au moins une classe cible
	if (GetLearningSpec()->IsTargetStatsComputed() and GetTargetDescriptiveStats()->GetValueNumber() > 0)
	{
		// Parametrage de la preparation de donnees
		dataPreparationClass.SetLearningSpec(GetLearningSpec());

		// Generation de la classe de preparation des donnees
		dataPreparationClass.ComputeDataPreparationFromClassStats(GetClassStats());

		// Construction d'un predicteur bayesien naif, sans aucun attribut predictif
		InternalTrainNB(&dataPreparationClass, &oaTheDataPreparationAttribute);
		return true;
	}
	return false;
}
