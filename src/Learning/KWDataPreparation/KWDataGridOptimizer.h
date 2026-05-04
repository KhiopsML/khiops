// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KWDataGridOptimizer;

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
// Classe KWDataGridOptimizer
// Optimisation d'une grille de donnees parametree par une structure de cout.
class KWDataGridOptimizer : public Object
{
public:
	// Constructeur
	KWDataGridOptimizer();
	~KWDataGridOptimizer();

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

	// Optimisation d'un grille pour une structure de cout donnee
	// En sortie, on trouve une nouvelle grille optimisee compatible avec la grille initiale
	// Retourne le cout de codage MODL de la grille optimisee
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
	void SetOptimizationHandler(const KWAttributeSubsetStats* attributeSubsetStats);
	const KWAttributeSubsetStats* GetOptimizationHandler();

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
	// Acces au modele initial, initialise par OptimizeDataGrid
	const KWDataGrid* GetOptimizedInitialDataGrid() const;

	// Acces au modele null, initialise par OptimizeDataGrid
	const KWDataGrid* GetOptimizedNullDataGrid() const;

	// Cout du modele null
	double GetOptimizedNullDataGridCost() const;

	// Optimisation interne d'une grille, a redefinir par sous-classe
	// On ne fait ici que l'optimisation, tout le contexte ayant ete mi en place par la methode appelante OptimizeDataGrid:
	// - suivi de tache
	// - initialisation du modele null
	// - memorisation de la grille initiale
	// - initialisation de la grille optimale avec le modele null
	// - ...
	// En sortie, on trouve une nouvelle grille optimisee compatible avec la grille initiale
	// Retourne le cout de codage MODL de la grille optimisee
	virtual double InternalOptimizeDataGrid(const KWDataGrid* initialDataGrid,
						KWDataGrid* optimizedDataGrid) const = 0;

	//////////////////////////////////////////////////////////////////////////////////
	// Gestion de l'optimisation VNS (Variable Neighborhood Search)
	//
	// Pilotage de la meta heuristique globale.
	// Compromis entre exploration globale (eapartir de solution random) et exploitation locale
	// d'un voisinnage autout de la meilleure solution
	// OptimisationVNS(NeighbourhoodLevelNumber):
	//    - NeibourhoodSize = 1 pour une taille de voisinage initiale maximale (1 pour solution random)
	//    - repeter de niveau 1 a NeighbourhoodLevelNumber
	//       - tant que amelioration, generer solution dans voisinnage et optimisation
	//       - diviser la taille de voisinnage par un facteur donne
	// Optimisation iterative
	//    - bestSolution = currentSolution
	//    - repeter de Level = 0 a OptimizationLevel
	//       - NeighbourhoodLevelNumber = 2^Level
	//       - exploration globale de meme niveau que la meilleure solution courante:
	//          appeler OptimisationVNS(NeighbourhoodLevelNumber) en partant d'une nouvelle solution aleatoire (exploration globale)
	//       - on retient le meilleur de la solution courante et de la nouvelle solution exploree globalement (au
	//       meme niveau)
	//       - affinage de l'exploration locale au niveau superieur

	// Optimisation VNS iterative, selon le niveau d'optimisation defini dans les parametre d'optimisation
	// La grille optimizedDataGrid contient en entree la meilleure solution courante
	// Cette solution est mise a jour si son cout est ameliore, et on renvoie son cout
	double IterativeVNSOptimizeDataGrid(const KWDataGrid* initialDataGrid, KWDataGrid* optimizedDataGrid) const;

	// Pilotage de la meta heuristique VNS, avec des voisinnages successifs de taille decroissante
	// selon un facteur geometrique
	// La grille optimizedDataGrid contient en entree une meilleure solution initiale
	// Cette solution est mise a jour si son cout est ameliore, et on renvoie son cout
	double VNSOptimizeDataGrid(const KWDataGrid* initialDataGrid, int nNeighbourhoodLevelNumber,
				   KWDataGrid* optimizedDataGrid) const;

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
	// On rend la grille issue de la post-optimisation VarPart, ainsi que sa grille partitionned de reference
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

	// Pilotage de la meta heuristique VNS, avec des voisinages successifs de taille decroissante
	// selon un facteur geometrique
	// La grille optimizedDataGrid contient en entree la meilleure solution courante
	// Cette solution est mise a jour si son cout est ameliore, et on renvoie son cout
	// Les grilles generiques optimales sont post-fusionnees et les voisinages sont ceux des grilles antecedentes
	// des grilles de meilleur cout apres post-fusion En sortie : optimizedDataGrid contient la grille antecedent
	// avant post-fusion de la meilleure grille post-fusionne Le cout renvoye est le cout de cette optimizedDataGrid
	// dBestMergedDataGridCost contient le cout de la meilleure grille apres post-fusion (meilleur cout)
	// CH IV Refactoring: proto en vue de fusionner la methode avec VNSOptimizeDataGrid
	double VNSOptimizeVarPartDataGrid(const KWDataGrid* initialDataGrid, int nNeighbourhoodLevelNumber,
					  KWDataGrid* optimizedDataGrid) const;

	// Optimisation d'une solution, selon le parametre de post-optimisation des solutions
	double OptimizeSolution(const KWDataGrid* initialDataGrid, KWDataGridMerger* dataGridMerger,
				boolean bDeepPostOptimization) const;

	// Creation d'une solution voisine d'une solution optimisee
	// On passe en parametre un pourcentage de perturbation compris
	// entre 0 (pas de perturbation) et 1 (solution aleatoire)
	void GenerateNeighbourSolution(const KWDataGrid* initialDataGrid, const KWDataGrid* optimizedDataGrid,
				       double dNoiseRate, KWDataGridMerger* neighbourDataGridMerger) const;

	//////////////////////////////////////////////////////////////////////////////////
	// Methodes utilitaires

	// Sauvegarde d'une grille source dans une grille cible
	void SaveDataGrid(const KWDataGrid* sourceDataGrid, KWDataGrid* targetDataGrid) const;

	// Test si l'optimisation est necessaire pour une grille
	// Au moins deux parties par attribut source et cible
	boolean IsOptimizationNeeded(const KWDataGrid* dataGrid) const;

	// Test si une grille est supervisee
	boolean IsSupervisedDataGrid(const KWDataGrid* dataGrid) const;

	// Test si le temps d'optimisation est depasse (si le parametre correspondant est actif (non null))
	boolean IsOptimizationTimeElapsed() const;

	// Reinitialisation des indicateurs de progression
	void ResetProgressionIndicators() const;

	// Affichage des informations de progression: messages et niveau d'avancement de la barre de progesssion
	virtual void DisplayProgression(const KWDataGrid* dataGrid) const;

	// Affichage des caracteristiques d'optimisation si demande dans le parametre d'optimsiation DisplayDetails
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

	// Grille du modele nul de la grille en cours d'optimisation
	mutable KWDataGrid* optimizedNullDataGrid;

	// Grille initiale en cours d'optimisation
	mutable const KWDataGrid* optimizedInitialDataGrid;

	// Informations d'indexation des solutions evaluees durant l'optimisation
	mutable Timer timerOptimization;
	mutable int nVNSIteration;
	mutable int nVNSNeighbourhoodLevelIndex;
	mutable int nVNSNeighbourhoodLevelNumber;
	mutable double dVNSNeighbourhoodSize;

	// Contexte de gestion de la partie anytime de l'optimisation
	const KWAttributeSubsetStats* attributeSubsetStatsOptimizationHandler;

	// Profiler
	static Profiler profiler;

	// Epsilon d'optimisation
	double dEpsilon;
};

////////////////////////
// Methodes en inline

inline Profiler* KWDataGridOptimizer::GetProfiler()
{
	return &profiler;
}
