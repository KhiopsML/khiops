// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class NEWKWDataGridOptimizer;

#include "KWClassStats.h"
#include "KWDataGrid.h"
#include "KWDataGridCosts.h"
#include "KWDataGridMerger.h"
#include "KWDataGridManager.h"
#include "KWDataGridPostOptimizer.h"
#include "KWDataGridOptimizerParameters.h"
#include "SortedList.h"
#include "Profiler.h"
#include "Timer.h"

//////////////////////////////////////////////////////////////////////////////////
// Classe NEWKWDataGridOptimizer
// Optimisation d'une grille de donnees parametree par une structure de cout.
class NEWKWDataGridOptimizer : public Object
{
public:
	// Constructeur
	NEWKWDataGridOptimizer();
	~NEWKWDataGridOptimizer();

	// Reinitialisation
	virtual void Reset();

	// Parametrage de la structure des couts de la grille de donnees
	// Memoire: les specifications sont referencees et destinees a etre partagees par plusieurs algorithmes
	void SetDataGridCosts(const KWDataGridCosts* kwdgcCosts);
	const KWDataGridCosts* GetDataGridCosts() const;

	// Parametres d'optimisation
	// Memoire: l'objet rendu appartient a l'appele
	KWDataGridOptimizerParameters* GetParameters();

	// Parametrage (facultatif) par des statistiques sur le probleme d'apprentissage
	// Permet l'utilisation des statistiques univariees pour optimiser les grilles de donnees
	// Memoire: les specifications sont referencees et destinee a etre partagees
	void SetClassStats(KWClassStats* stats);
	KWClassStats* GetClassStats() const;

	// CH IV Begin
	// Parametrage (facultatif) par une grille initiale, dans le cas du coclustering instances * variables,
	// Permet l'utilisation de cette grille pour la creation de la grille avec parties de variables fusionnees
	// Memoire: les specifications sont referencees et destinees a etre partagees
	void SetInitialVarPartDataGrid(KWDataGrid* refDataGrid);
	KWDataGrid* GetInitialVarPartDataGrid() const;
	// CH IV End

	// Optimisation d'un grille pour une structure de cout donnees
	// En sortie, on trouve une nouvelle grille optimisee compatible avec la grille initiale,
	// ne conservant que les attributs non reduits a une seule partie
	// Les intervalles (resp. groupes) de la grille optimisee sont tries par valeur (resp. effectifs decroissants)
	// Integre un parcours des granularites
	// Retourne le cout de codage MODL de la grille post-optimisee
	// Integre un parcours des granularites
	// Dans le cas d'une grille VarPart avec des parties de variable, le cout retourne est celui de la grille
	// antecedente de la meilleure grille post-fusionnee (fusion des parties de variable consecutives dans un
	// cluster de parties de variables)
	double OptimizeDataGrid(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid) const;

	// Simplification d'une grille selon le parametre MaxPartNumber des parametres d'optimisation
	// La grille en parametre est en simplifiee si necessaire, en fusionnant iterativement les partie
	// tant que la contrainte de nombre max de partie n'est pas respectee
	// Ce parametre n'est pris en compte qu'en etape de post-traitement d'un grille deja optimisee
	//  . cela permet de faire en sorte que les grilles simplifiee sont toutes des sous-grille de la grille
	//  optimale,
	//    ce qui simplifie l'interpretabilite
	//  . cela simplifie l'implementation, qui n'est ici effectuee qu'en post-traitement
	// Retourne le cout de codage MODL de la grille simplifiee
	double SimplifyDataGrid(KWDataGrid* optimizedDataGrid) const;

	//////////////////////////////////////////////////////////////////
	// Parametrage avance

	// Methode appelee lors de l'optimisation a la fin de chaque etape d'optimisation
	// Permet par exemple de passer en mode anytime, en memorisant chaque solution intermediaire
	// Par defaut: redirige vers la methode HandleOptimizationStep attributeSubsetStats s'il est parametre
	virtual void HandleOptimizationStep(const KWDataGrid* optimizedDataGrid,
					    const KWDataGrid* initialGranularizedDataGrid, boolean bIsLastSaving) const;

	// Parametrage du contexte de gestion de la partie anytime de l'optimisation
	// Permet de rediriger la methode HandleOptimizationStep vers celle du attributeSubsetStats
	void SetAttributeSubsetStats(const KWAttributeSubsetStats* attributeSubsetStats);
	const KWAttributeSubsetStats* GetAttributeSubsetStats();

	//////////////////////////////////////////////////////////////////
	// Gestion d'un profiler dedie a l'optimisation des grilles
	// Ce profiler doit etre demarre depuis le point d'entree de l'optimisation,
	// et utilise par les methodes d'optimisation a profiler

	// Acces au profiler global permettant d'enregistrer toute une session de profiling
	// de l'optimisation d'une grille de coclustering
	static Profiler* GetProfiler();

	//////////////////////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// CH IV Refactoring: a supprimer
	friend class KWDataGridOptimizer;

	//////////////////////////////////////////////////////////////////////////////////
	// Initialisation de base avec grille terminale ou a base de grilles univariees

	// Mode d'optimisation legere utilise en coclustering
	// Le mode d'optimisation legere est utilise dans le cas de grille non supervisee, pour
	// lesquelles les granularites intermediaires ne font pas partie des parametres du modeles
	// (contrairement au cas supervise), et ne sont la que pour avoir des solution intermediaires
	// rapidement, sans necessite de raffiner la solution
	// En mode d'optimisation legere:
	// - il n'y a pas d'optmisation VNS, mais une seule optimisation
	// - la post-optimisation est legere
	// - il n'y a pas de post-optimisation VarPart
	void SetSlightOptimizationMode(boolean bValue) const;
	const boolean GetSlightOptimizationMode() const;

	// Optimisation d'une grille pour la granularite courante
	// Ajout arguments bIsLastGranularity
	double OptimizeGranularizedDataGrid(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid,
					    boolean bIsLastGranularity) const;

	// Post-optimisation de la granularite de la grille optimisee afin de lui attribuer la plus petite granularite
	// avec laquelle la partition est compatible
	// Sinon, un grille optimise avec un granularite fine peut par hasard avoir des frontiere coincidant
	// avec une granularite plus grossiere, et il faut le prendre en compte dans les cout de prior
	// Ne concerne que le cas supervise, pour lequel la granularite fait partie du modele
	// On rend le cout de la grille post-optimisee
	double PostOptimizeGranularity(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid,
				       const ObjectDictionary* odQuantileBuilders, int nLastExploredGranularity) const;

	//////////////////////////////////////////////////////////////////////////////////
	// Initialisation de base avec grille terminale ou a base de grilles univariees

	// Initialisation avec une grille terminale
	double InitializeWithTerminalDataGrid(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid) const;

	// Cette methode calcule pour chaque attribut de la grille initialDataGrid
	// le partitionnement univarie optimal associe au partitionnement obtenu par projection univariee de la grille
	// La granularite du partitionnement est celle de la grille initiale
	double OptimizeWithBestUnivariatePartitionForCurrentGranularity(const KWDataGrid* initialDataGrid,
									KWDataGrid* optimizedDataGrid) const;

	// Recherche d'une amelioration par croisement des partition univariees
	// En mode hors granularite, la methode utilise directement les partitions univariees stockees dans KWClassStats
	// En mode granularite, les partitions univariees optimales sont recalculees pour la granularite de la grille
	// initiale et sont utilisees pour construire une grille produit
	// bIsLastGranularity : Pour les granularites intermediaires, la post-optimisation n'est pas "deep"
	double OptimizeWithMultipleUnivariatePartitions(const KWDataGrid* initialDataGrid,
							KWDataGrid* optimizedDataGrid) const;

	//////////////////////////////////////////////////////////////////////////////////
	// Gestion de l'optimisation VNS (Variable Neighborhood Search)

	// Optimisation avec VNS
	// Traitement plus leger pour les granularites intermediaires en non supervise
	double VNSOptimize(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid,
			   boolean bIsLastGranularity) const;

	// Pilotage de la meta heuristique globale.
	// Compromis entre exploration globale (rapartir de solution random) et exploitation locale
	// d'un voisinnage autout de la meilleure solution
	// OptimisationVNS(Level):
	//    . NeibourhoodSize = 1 pour une taille de voisinage initiale maximale (1 pour solution random)
	//    . repeter de niveau 1 a Level
	//       - tant que amelioration, generer solution dans voisinnage et optimisation
	//       - diviser la taille de voisinnage par un facteur donne
	// Optimisation iterative(MaxLevel)
	//    . bestSolution = currentSolution
	//    . repeter de niveau 1 a MaxLevel
	//       - exploration globale de meme niveau que la meilleure solution courante:
	//          appeler OptimisationVNS(Level) en partant d'une nouvelle solution aleatoire (exploration globale)
	//       - on retient le meilleur de la solution courante et de la nouvelle solution exploree globalement (au
	//       meme niveau)
	//       - affinage de l'exploration locale au niveau superieur
	double IterativeVNSOptimizeDataGrid(const KWDataGrid* initialDataGrid, int nMaxLevel,
					    KWDataGrid* optimizedDataGrid) const;

	// Pilotage de la meta heuristique VNS, avec des voisinnages successifs de taille decroissante
	// selon un facteur geometrique (de DecreaseFactor^MinIndex a Factor^MaxIndex)
	double VNSOptimizeDataGrid(const KWDataGrid* initialDataGrid, double dDecreaseFactor, int nMinIndex,
				   int nMaxIndex, KWDataGrid* optimizedDataGrid, double dOptimizedDataGridCost) const;

	// CH IV Refactoring
	// Methode de post-optimisation d'un grille optimisee en redecoupant ses parties de variables
	// Methode temporaire permettant de reutiliser le code de la methode principale VNSOptimizeDataGrid
	// en isolant cette partie de post-optimisation specifique VarPart, et de supprimer l'ancienne
	// methode VNSOptimizeVarPartDataGrid
	// Parametre (a faire evoluer si necessaire):
	// - initialDataGrid: grille initiale
	// - neighbourDataGrid: grille courante en cours d'optimisation
	// - dNeighbourDataGridCost: cout de la grille courante
	// - mergedDataGrid: grille optimisee si amelioration
	// En sortie, la grille courante et son cout sont modifies suite a optimisation.
	// On rend la grille issue de la post-opimisation VarPart, ainsi que sa grille partitionned re drefrence
	// Le code retour est le meilleurs cout optenu apres post-optimisation VarPart
	// Attention, celui-ci est different de celui de la grille courante optimisee
	//
	// A terme, il faudra isoler ce service de post-optimisation pour le deplacer en quatrime sous-methode
	// en fin de la methode OptimizeSolution
	// - Pre-optimization
	// - Greedy merge optimization
	// - Post-optimization
	// - Post-optimization IV
	double VNSDataGridPostOptimizeVarPart(const KWDataGrid* initialDataGrid, KWDataGridMerger* neighbourDataGrid,
					      double& dNeighbourDataGridCost, KWDataGrid* mergedDataGrid,
					      KWDataGrid* partitionedReferencePostMergedDataGrid) const;

	// CH IV Begin
	// Pilotage de la meta heuristique VNS, avec des voisinages successifs de taille décroissante
	// selon un facteur geometrique (de DecreaseFactor^MinIndex a Factor^MaxIndex)
	// Les grilles generiques optimales sont post-fusionnees et les voisinages sont ceux des grilles antecedentes
	// des grilles de meilleur cout apres post-fusion En sortie : optimizedDataGrid contient la grille antecedent
	// avant post-fusion de la meilleure grille post-fusionne Le cout renvoye est le cout de cette optimizedDataGrid
	// dBestMergedDataGridCost contient le cout de la meilleure grille apres post-fusion (meilleur cout)
	// CH IV Refactoring: proto en vue de fusionner la methode avec VNSOptimizeDataGrid
	double VNSOptimizeVarPartDataGrid(const KWDataGrid* initialDataGrid, double dDecreaseFactor, int nMinIndex,
					  int nMaxIndex, KWDataGrid* optimizedDataGrid, double dOptimizedDataGridCost,
					  double& dBestMergedDataGridCost) const;
	// CH IV Refactoring: ancienne version avant decoupage en VNSOptimizeVarPartDataGrid et VNSDataGridPostOptimizeVarPart
	double DEPRECATED_VNSOptimizeVarPartDataGrid(const KWDataGrid* initialDataGrid, double dDecreaseFactor,
						     int nMinIndex, int nMaxIndex, KWDataGrid* optimizedDataGrid,
						     double dOptimizedDataGridCost,
						     double& dBestMergedDataGridCost) const;
	// CH IV End

	// Optimisation d'une solution
	// Optimisation et post-optimisation activee selon les parametres d'optimisations
	double OptimizeSolution(const KWDataGrid* initialDataGrid, KWDataGridMerger* dataGridMerger) const;

	// Creation d'une solution voisine d'une solution optimisee
	// On passe en parametre un pourcentage de perturbation compris
	// entre 0 (pas de perturbation) et 1 (solution aleatoire)
	void GenerateNeighbourSolution(const KWDataGrid* initialDataGrid, const KWDataGrid* optimizedDataGrid,
				       double dNoiseRate, KWDataGridMerger* neighbourDataGridMerger) const;

	//////////////////////////////////////////////////////////////////////////////////
	// Methodes utilitaires

	// Test si l'optimisation est necessaire pour une grille
	// Au moins deux parties par attribut source et cible
	boolean IsOptimizationNeeded(const KWDataGrid* initialDataGrid) const;

	// Calcul de la granularite max a explorer
	// Celle-ci depend du nombre de valeurs, mais est egalement tronquee de facon heuristique
	// dans certains cas (ex: regression) pour reduire les temps de calcul
	int ComputeMaxExploredGranularity(const KWDataGrid* initialDataGrid) const;

	// Test si le temps d'optimisation est depasse (si le parametre correspondant est actif (non null))
	boolean IsOptimizationTimeElapsed() const;

	// Affichage des caracteristiques d'optimisation si demande dans les parametre d'optimsiation
	// Cette methode gere egalement les messages et le niveau d'avancement de la barre de progesssion
	void DisplayOptimizationHeaderLine() const;
	void DisplayOptimizationDetails(const KWDataGrid* optimizedDataGrid, boolean bOptimized) const;

	//////////////////////////////////////////////////////////////////////////////
	// Variable de la classe

	// Parametrage de la structure des couts
	const KWDataGridCosts* dataGridCosts;

	// Parametres d'optimisation
	KWDataGridOptimizerParameters optimizationParameters;

	// Attribut de statistiques
	KWClassStats* classStats;

	// CH IV Begin
	// Grille de reference
	KWDataGrid* initialVarPartDataGrid;
	// CH IV End

	// Nettoyage des attribut non informatifs
	boolean bCleanNonInformativeVariables;

	// Mode d'optimisation legere dans le cas d'une granularite intermediaire en co-clustering non supervise
	mutable boolean bSlightOptimizationMode;

	// Informations d'indexage des solutions evaluees durant l'optimisation
	mutable Timer timerOptimization;
	mutable int nVNSIteration;
	mutable int nVNSLevel;
	mutable int nVNSMaxLevel;
	mutable double dVNSNeighbourhoodSize;

	// Contexte de gestion de la partie anytime de l'optimisation
	const KWAttributeSubsetStats* attributeSubsetStatsHandler;

	// Profiler
	static Profiler profiler;

	// Epsilon d'optimisation
	double dEpsilon;
};

////////////////////////
// Methodes en inline

inline Profiler* NEWKWDataGridOptimizer::GetProfiler()
{
	return &profiler;
}
