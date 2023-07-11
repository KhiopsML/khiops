// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWContinuous.h"

//////////////////////////////////////
// Classe MHContinuousLimits
// Services de classe de gestion des limùites liees au type Continuous
class MHContinuousLimits : public Object
{
public:
	// Recherche de la plus petite valeur Continuous strictement inferieure ou superieurs a une valeur donnee
	// C'est toujours possible sauf pour les valeurs extremes
	// Note qu'il s'agit d'un resultat approche et non exact
	static Continuous ComputeClosestLowerBound(Continuous cValue);
	static Continuous ComputeClosestUpperBound(Continuous cValue);

	// Calcul du nombre de valeurs distinctes que l'on peut coder dans un intervalle,
	// aux limites de la precision numerique
	static double ComputeNumberDistinctValues(Continuous cMinValue, Continuous cMaxValue);

	// Verification d'une valeur Continuous, notamment de ses bornes et qu'elle ne soit pas manquante
	static boolean CheckContinuousValueBounds(Continuous cValue);
};
