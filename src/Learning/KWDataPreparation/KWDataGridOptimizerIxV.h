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

	// Optimisation d'une solution voisine: specialisation au cas VarPart
	double OptimizeNeighbourSolution(const KWDataGrid* initialDataGrid, const KWDataGrid* currentOptimizedDataGrid,
					 double dNoiseRate, KWDataGridMerger* neighbourOptimizedDataGrid,
					 boolean bDeepPostOptimization) const override;

	/////////////////////////////////////////////////////////////////////////////
	// Post-optimisation d'une solution dediee au cas VarPart

	// Pilotage de la post-optimisation
	// En entree, la grille optimisee constitue une solution de depart, compatible avec la grille initiale.
	// En sortie, la grille optimisee est amelioree par la methode de post-optimisation.
	// - par fusion des partie de variables
	// - par deplacement de valeurs d'une partie de variables a une autre partie de variables
	// On retourne le cout le grille post-optimisee
	double PostOptimizeVarPartSolution(const KWDataGrid* initialDataGrid,
					   KWDataGridMerger* optimizedDataGrid) const;

	// Post-optimisation par fusion des parties de variable adjacentes dans chaque groupe de parties de la dimensionVarPart
	// En sortie, la grille optimisee est amelioree par la methode de post-optimisation, et on retourne la variation de cout
	double PostOptimizeVarPartSolutionByMergingVarParts(KWDataGrid* optimizedDataGrid) const;

	// Post-optimisation par deplacement de partie de variable elementaire d'un groupe a l'autre de la grille optimisee
	// En sortie, on renvoie la grille optimisee est modifiee en indiquant si on a detecte une amelioration potentielle
	// dans le code retour.
	// Attention, le cout resultant n'est pas forcement ameliore, et il faut completer cette post-optimisation par des
	// fusion de parties de variables, et rappeler cette methode plusieurs fois si necessaire
	boolean PostOptimizeVarPartSolutionByMovingVarParts(const KWDataGrid* initialDataGrid,
							    KWDataGrid* optimizedDataGrid) const;
};
