// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KDSelectionOperandExtractionTask;

#include "KWDatabaseTask.h"
#include "KDConstructionRule.h"
#include "KDConstructedRule.h"
#include "KWDRAll.h"
#include "KWQuantileBuilder.h"
#include "KDSelectionOperandAnalyser.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KDSelectionOperandExtractionTask
// Classe technique utilise par la classe KDSelectionOperandAnalyser
//
// Analyse de la base pour transformer toute extraire toutes les bornes ou valeur
// des operandes de selection basees sur des données de la base, a partir d'une
// specification conceptuelle de ces operandes de collection
class KDSelectionOperandExtractionTask : public KWDatabaseTask
{
public:
	// Constructeur
	KDSelectionOperandExtractionTask();
	~KDSelectionOperandExtractionTask();

	// Lecture de la base pour collecter un echantillon de valeurs par variable secondaire de selection
	// Entree:
	//   . selectionOperandAnalyser: parametrage des operandes a analyser
	// 	 . sourceDatabase: base a analyser, correction parametree par un dictionnaire permettant de lire chaque
	// variable secondaire et ses operandes Sortie: 	 . nombre d'enregistrement lus dans la base Methode
	// interruptible, retourne false si erreur ou interruption (avec message), true sinon
	boolean CollectSelectionOperandSamples(KDSelectionOperandAnalyser* selectionOperandAnalyser,
					       const KWDatabase* sourceDatabase, int& nReadObjectNumber);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	//////////////////////////////////////////////////////////////////////////////
	// Reproductibilité des resultats
	// Dans un premier temps, on va associer chaque instance secondaire
	// a une paire d'index (i1, i2) de facon reproductible.
	//  . dossiers principaux :
	//    .	i1:	la phase prealable d'indexation de la base pour la parallelisation permet de connaitre l'index
	//    i1 de
	//      la ligne du fichier contenant les instances principales des dossiers,
	//    . i2:	localement a chaque dossier, on initialise i2 = 0 et on incremente ce second index pour chaque
	//      nouvelle instance secondaire du dossier rencontree lors de son parcours,
	//  . tables externes:
	//    . i1: on utilise l'index i1 = 0 pour l'ensemble de toutes les instances des tables externes,
	//    . i2: on initialise i2 = 0 et on incremente ce second index pour chaque nouvelle instance secondaire de
	//      table externe rencontree lors de leur parcours complet.
	// On associe ensuite chaque paire d'index (i1, i2) a un nombre aleatoire en O(1) en utilisant un generateur de
	// nombre aleatoire capable de generer directement le ieme tirage. Comme on connait la taille S1 en nombre
	// d'instances principales de la base suite a la phase prealable d'indexation, on peut utiliser la technique des
	// leapfrogs en utilisant IthRandomNumber(n) avec n = i1 + S1 x i2. En pratique, on va utilise un majorant de
	// n1: la taille du fichier en octets, et on va prendre nombre premier au dessus de ce majorant, pour minimiser
	// les regularites si on depasse la capacite des entiers longs

	// Reimplementation des methodes virtuelles de tache
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean ComputeResourceRequirements() override;
	boolean MasterInitialize() override;
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;
	boolean MasterAggregateResults() override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveInitializePrepareDatabase() override;
	boolean SlaveInitializeOpenDatabase() override;
	boolean SlaveProcessStartDatabase() override;
	boolean SlaveProcessExploitDatabaseObject(const KWObject* kwoObject) override;
	boolean SlaveProcessStopDatabase(boolean bProcessEndedCorrectly) override;
	boolean SlaveFinalize(boolean bProcessEndedCorrectly) override;

	// Recherche du prochain nombre premier superieur ou egal a un valeur donnee
	longint GetNextPrimeNumber(longint lMinValue);
};
