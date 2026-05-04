// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataGridOptimizerVxV;

#include "KWDataGridOptimizer.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe KWDataGridOptimizerVxV
// Optimisation d'une grille de donnees dans le cas Variable x Variable
class KWDataGridOptimizerVxV : public KWDataGridOptimizer
{
public:
	// Constructeur
	KWDataGridOptimizerVxV();
	~KWDataGridOptimizerVxV();

	//////////////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	//////////////////////////////////////////////////////////////////////////////////
	// Optimisation d'une grille avec gestion de la granularite

	// Optimisation d'un grille pour une structure de cout donnee
	// En sortie, on trouve une nouvelle grille optimisee compatible avec la grille initiale
	// Retourne le cout de codage MODL de la grille optimisee
	// Integre un parcours des granularites
	double InternalOptimizeDataGrid(const KWDataGrid* initialDataGrid,
					KWDataGrid* optimizedDataGrid) const override;

	// Test si on est a la derniere granularite pour une grille
	// c'est a dire si on a atteint la granularite max
	boolean IsLastGranularity(const KWDataGrid* dataGrid) const;

	// Calcul de la granularite max a explorer
	// Celle-ci depend du nombre de valeurs, mais est egalement tronquee de facon heuristique
	// dans certains cas (ex: regression) pour reduire les temps de calcul
	int ComputeMaxExploredGranularity(const KWDataGrid* dataGrid) const;

	// Optimisation d'une grille pour la granularite courante
	// L'optimisation est poussee, avec declenchement de l'algorithme VNS
	double OptimizeGranularizedDataGrid(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid) const;

	// Optimisation legere d'une grille
	// L'optimisation legere est utilise dans le cas de grille non supervisee, pour
	// lesquelles les granularites intermediaires ne font pas partie des parametres du modeles
	// (contrairement au cas supervise), et ne sont la que pour avoir des solution intermediaires
	// rapidement, sans necessite de raffiner la solution
	// En mode d'optimisation legere:
	// - il n'y a pas d'optmisation VNS, mais une seule optimisation
	// - la post-optimisation est legere
	// - il n'y a pas de post-optimisation VarPart
	// La grille optimizedDataGrid contient en entree une meilleure solution initiale
	// Cette solution est mise a jour si son cout est ameliore, et on renvoie son cout
	double SlightOptimizeGranularizedDataGrid(const KWDataGrid* initialDataGrid,
						  KWDataGrid* optimizedDataGrid) const;

	// Post-optimisation de la granularite de la grille optimisee afin de lui attribuer la plus petite granularite
	// avec laquelle la partition est compatible
	// Sinon, un grille optimise avec un granularite fine peut par hasard avoir des frontieres coincidant
	// avec une granularite plus grossiere, et il faut le prendre en compte dans les cout de prior
	// Ne concerne que le cas supervise, pour lequel la granularite fait partie du modele
	// On rend le cout de la grille post-optimisee
	double PostOptimizeGranularity(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid,
				       const ObjectDictionary* odQuantileBuilders, int nLastExploredGranularity) const;

	//////////////////////////////////////////////////////////////////////////////////
	// Initialisation de base a base de grilles univariees

	// Cette methode calcule pour chaque attribut de la grille initialDataGrid
	// le partitionnement univarie optimal associe au partitionnement obtenu par projection univariee de la grille
	// La granularite du partitionnement est celle de la grille initiale
	// La grille optimizedDataGrid contient en entree la meilleure solution courante
	// Cette solution est mise a jour si son cout est ameliore, et on renvoie son cout
	double OptimizeWithBestUnivariatePartitionForCurrentGranularity(const KWDataGrid* initialDataGrid,
									KWDataGrid* optimizedDataGrid) const;

	// Recherche d'une amelioration par croisement des partition univariees
	// En mode hors granularite, la methode utilise directement les partitions univariees stockees dans KWClassStats
	// En mode granularite, les partitions univariees optimales sont recalculees pour la granularite de la grille
	// initiale et sont utilisees pour construire une grille produit
	// La grille optimizedDataGrid contient en entree la meilleure solution courante
	// Cette solution est mise a jour si son cout est ameliore, et on renvoie son cout
	double OptimizeWithMultipleUnivariatePartitions(const KWDataGrid* initialDataGrid,
							KWDataGrid* optimizedDataGrid) const;

	// Affichage des informations de progression: messages et niveau d'avancement de la barre de progesssion
	void DisplayProgression(const KWDataGrid* dataGrid) const override;
};
