// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataGridOptimizerIxV;

#include "KWDataGridOptimizer.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridOptimizerIxV
// Optimisation d'une grille de donnees dans le cas Instances x Variables
class KWDataGridOptimizerIxV : public KWDataGridOptimizer
{
public:
	// Constructeur
	KWDataGridOptimizerIxV();
	~KWDataGridOptimizerIxV();

	//////////////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Optimisation d'un grille
	double InternalOptimizeDataGrid(const KWDataGrid* initialDataGrid,
					KWDataGrid* optimizedDataGrid) const override;

	// Methode d'optimisation d'une grille dediee au cas instances x variables
	double OptimizeVarPartDataGrid(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid) const;
	double PROTO_OptimizeVarPartDataGrid(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid) const;
};
