// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

class KDSelectionOperandSamplingTask;

#include "KWDatabaseTask.h"
#include "KDSelectionOperandDataSampler.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KDSelectionOperandSamplingTask
// Classe technique utilise par la classe KDSelectionOperandAnalyser
//
// Analyse de la base pour transformer toute extraire toutes les bornes ou valeur
// des operandes de selection basees sur des données de la base, a partir d'une
// specification conceptuelle de ces operandes de collection
class KDSelectionOperandSamplingTask : public KWDatabaseTask
{
public:
	// Constructeur
	KDSelectionOperandSamplingTask();
	~KDSelectionOperandSamplingTask();

	// Lecture de la base pour collecter un echantillon de valeurs par variable secondaire de selection
	// Entree:
	//   . selectionOperandAnalyser: parametrage des operandes a analyser
	// 	 . sourceDatabase: base a analyser, correction parametree par un dictionnaire permettant de lire chaque
	// variable secondaire et ses operandes Sortie: 	 . nombre d'enregistrement lus dans la base Methode
	// interruptible, retourne false si erreur ou interruption (avec message), true sinon
	boolean CollectSelectionOperandSamples(KDSelectionOperandDataSampler* selectionOperandDataSampler,
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
	// On associe ensuite chaque paire d'index (i1, i2) a un nombre aleatoire en O(1) en utilisant un generateur de
	// nombre aleatoire capable de generer directement le ieme tirage. Comme on connait la taille S1 en nombre
	// d'instances principales de la base suite a la phase prealable d'indexation, on peut utiliser la technique des
	// leapfrogs en utilisant IthRandomNumber(n) avec n = i1 + S1 x i2. En pratique, on va utilise un majorant de
	// n1: la taille du fichier en octets, et on va prendre nombre premier P1 au dessus de ce majorant, pour
	// minimiser les regularites si on depasse la capacite des entiers longs

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
	boolean SlaveProcessExploitDatabase() override;
	boolean SlaveProcessExploitDatabaseObject(const KWObject* kwoObject) override;
	boolean SlaveProcessStopDatabase(boolean bProcessEndedCorrectly) override;
	boolean SlaveFinalize(boolean bProcessEndedCorrectly) override;

	// Flag pour avoir la trace des echanges principaux
	boolean bTrace;

	////////////////////////////////////////////////////
	// Variables du maitre

	// Flag indiquant que tous les processus esclaves sont termines, y compris pour le dernier tour
	// de traitement des objets de tables externes
	boolean bLastSlaveProcessDone;

	// Echantillonneur des donnees pour les operandes de selection, aglomerant les echantillons
	// collectes par les esclaves
	// Cette variable est une copie de la variable en entree de la tache
	KDSelectionOperandDataSampler* masterSelectionOperandDataSampler;

	////////////////////////////////////////////////////
	// Variables de l'esclave

	// Echantillonneur des donnees pour les operandes de selection local a l'esclave
	// Cet objet est utilise tout au long de la vie de l'esclave entre son initialisation et sa terminaison
	// afin de gerer les contexte des objet references des tables externes une fois pour toutes
	// Son contenu est echange avec output_selectionOperandDataSamplerSpec pour transmettre les donnees collectees
	// tout en gardant le contexte des objets referneces pour les SlaveProcess suivants
	KDSelectionOperandDataSampler* slaveSelectionOperandDataSampler;

	///////////////////////////////////////////////////////////
	// Parametres partages par le maitre et les esclaves

	// Specification de l'echantillonneur des donnees pour les operandes de selection
	// La variable partagee permet de partager les specifications entre le maitre et les esclaves
	PLShared_SelectionOperandDataSampler shared_selectionOperandDataSamplerSpec;

	///////////////////////////////////////////////////////////
	// Parametres en entree et sortie des esclaves

	// Seuil de selection des objets par classe de l'echantilonneur de donnees
	PLShared_DoubleVector input_dvObjectSelectionProbThresholds;

	// Flag indiquant que c'est le dernier tour, a utiliser pour traiter les objets des tables externes
	// apres le traitement de tous les objets principaux
	PLShared_Boolean input_bLastRound;
	PLShared_Boolean output_bLastRound;

	// Echantillonneur des donnees pour les operandes de selection en sortie d'un esclave
	PLShared_SelectionOperandDataSampler output_selectionOperandDataSampler;
};
