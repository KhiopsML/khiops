// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class DTDiscretizerMODL;

#include "KWDiscretizerMODL.h"
#include "KWDiscretizerMODLLine.h"
#include "KWFrequencyVector.h"
#include "KWUnivariatePartitionCost.h"
#include "SortedList.h"
#include "TaskProgression.h"
#include "DTConfig.h"
#include "KWQuantileBuilder.h"

//////////////////////////////////////////////////////////////////////////////////
// Algorithme MODL de fusion des lignes adjacentes d'une table de contingence
// Optimisation des discretisation pour l'a priori a trois etages
// Prise en compte de l'effectif minimum par intervalle dans le critere
class DTDiscretizerMODL : public KWDiscretizerMODL
{
public:
	// Constructeur
	DTDiscretizerMODL();
	~DTDiscretizerMODL();

	// Nom de l'algorithme
	const ALString GetName() const override;

	// Constructeur generique
	KWDiscretizer* Create() const override;

	// Calcul de la loi agregee pour des regroupements de lignes adjacentes
	// Renvoie 1-TauxCompression
	void Discretize(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget) const override;
	void DiscretizeOLD(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget) const;
	void DiscretizeNEW(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget) const;

	// si true alors nouvel alogo de dicretisation
	int nnew;
	int ntest;

	/////////////////////////////////////////////////////////////////
	// Implementation
protected:
	friend class KWDataGridManager;

	void DiscretizeGranularizedFrequencyTableNEW(KWFrequencyTable* kwftSource, KWFrequencyTable*& kwftTarget,
						     IntVector* ivGranularityValues,
						     IntVector* ivGranularizedValueNumber) const;
};
