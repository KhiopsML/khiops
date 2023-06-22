// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "PLDatabaseTextFile.h"
#include "PLParallelTask.h"
#include "KWMTDatabaseIndexer.h"
#include "KWFileIndexerTask.h"
#include "MemoryStatsManager.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KWDatabaseTask
// Tache parallele prenant en entree une base de donnees source
// La base en entree est mono ou multi-table, textuelle
// La tache est pilotee par la lecture de la base, quelle que soit sa nature
// Cette classe est une classe virtuelle a reimplementer dans toutes les taches
// gerant une base en entree: transfer, verfication, evaluation...
class KWDatabaseTask : public PLParallelTask
{
public:
	// Constructeur
	KWDatabaseTask();
	~KWDatabaseTask();

	// Parametrage de l'affichage des messages specifiques a la tache (nombre d'enregistrements lus...) a l'issue de
	// la tache (defaut: true)
	void SetDisplaySpecificTaskMessage(boolean bValue);
	boolean GetDisplaySpecificTaskMessage() const;

	// Parametrage de l'affichage des messages de fin de tache (erreur ou interruption) a l'issue de la tache
	// (defaut: true)
	void SetDisplayEndTaskMessage(boolean bValue);
	boolean GetDisplayEndTaskMessage() const;

	// Parametrage de l'affichage du temps de calcul de la tache a l'issue de la tache (defaut: true)
	void SetDisplayTaskTime(boolean bValue);
	boolean GetDisplayTaskTime() const;

	// Parametrage de tous types de messages (appel groupe aux methode precedentes)
	void SetDisplayAllTaskMessages(boolean bValue);

	///////////////////////////////////////////////////////////////////////////
	// Statistiques de base disponible apres execution de la tache

	// Nombre total d'enregistrements lus dans la base
	longint GetReadRecords() const;

	// Nombre total d'enregistrements lus, valides et selectionnes dans la base
	longint GetReadObjects() const;

	// Duree totale du job, y compris la phase d'indexation preliminaire du transfert
	// (il ne faut pas utiliser GetJobElapsedTime() car cette methode ne prend pas en compte le transfert monotable)
	double GetFullJobElapsedTime() const;

	// Libelles utilisateurs
	const ALString GetObjectLabel() const override;

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Methodes specifiques au lancement d'une tache parallele
	// A reimplementer potentiellement dans uen sous-classe

	// Methode principale a appeler pour lancer la tache pilotee par la base d'entree
	// Dans une sous-classe, il faut avoir initialisee les eventuelles variables partagees
	// specifiques, puis apres la tache collecter les resultats de la taches
	// On peut appeler ensuiite la methode d'affichage des messages utilisateurs
	// La base source doit etre mono ou multi-tables en fichiers textes
	// Methode interruptible, retourne false si erreur ou interruption (avec message), true sinon
	virtual boolean RunDatabaseTask(const KWDatabase* sourceDatabase);

	// Methode a appeler apres l'execution de la tache
	// Affichhage des messages si la tache s'est correctement deroulee et
	// si l'option d'affichage des message utilisateurs est activee
	// Affiche:
	//  . un message specifique a la tache
	//  . un message d'erreur ou d'interruption si necessaire
	//  . un temps d'execution de la tache
	virtual void DisplayTaskMessage();

	// Methode pour les message specifique a la tache
	// Par defaut, affichage de stats sur les enregistrements lus dans la base en entree
	virtual void DisplaySpecificTaskMessage();

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Calcul du plan d'indexation des tables du schema, a partir de la variable partagee de la base en entree
	// On calcule d'abord les index de champ de chaque table en entree, ce qui permet de savoir quelle table doit
	// etre lue. On collecte un echantillon de cle de la table principale, ainsi que leur position pour chaque table
	// On determine alors un decoupage des tables pour les traitement par esclave
	// En sortie, si OK, on a donc initialise les tableaux oaAllChunksBeginPos, oaAllChunksBeginRecordIndex, et
	// oaAllChunksLastRootKeys

	// Calcul du plan d'indexation global des bases pour le pilotage de la parallelisation
	// Erreur possible si pas assez de ressource par exemple
	virtual boolean ComputeAllDataTableIndexation();

	// Nombre de taches elementaires issue de l'indexation
	virtual int GetIndexationSlaveProcessNumber();

	// Nettoyage des informations d'indexation
	virtual void CleanAllDataTableIndexation();

	// Calcul d'informations lie a l'ouverture des bases
	virtual boolean ComputeDatabaseOpenInformation();

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Reimplementation des methodes virtuelles de tache
	// Lors des reimplementation dans les sous-classes, on peut appeler explicitement la methode
	// de KWDatabaseTask, en debut de reimplementation pour toute les methodes, sauf pour les
	// methodes Finalize devant etre appelee en fin de reimplementation

	// Dimensionnement des ressources
	boolean ComputeResourceRequirements() override;

	// Initialisation du maitre
	boolean MasterInitialize() override;

	// Preparation de la tache d'un esclave
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;

	// Agregation des resultats d'un esclave
	boolean MasterAggregateResults() override;

	// Finalisation du maitre
	// A appeler en fin de reimplementation eventuelle
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;

	// Initialisation de l'esclave, qui appele en sequence les trois methodes suivantes:
	//  SlaveInitializePrepareDictionary
	//  SlaveInitializePrepareDatabase
	//  SlaveInitializeOpenDatabase
	boolean SlaveInitialize() override;

	// Debut de l'initialisation de l'esclave: preparation du dictionnaire
	// Gestion du dictionnaire, mise a jour des bases et dimensionnement des ressources
	virtual boolean SlaveInitializePrepareDictionary();

	// Milieu de l'initialisation de l'esclave: preparation et dimensionnement de la base en utilisant les
	// ressources
	virtual boolean SlaveInitializePrepareDatabase();

	// Fin de l'initialisation de l'esclave: ouverture de la base
	virtual boolean SlaveInitializeOpenDatabase();

	// Methode principale de l'esclave, qui appele en sequence les trois methodes suivantes:
	//  SlaveProcessStartDatabase
	//  SlaveProcessExploitDatabase
	//    SlaveProcessExploitDatabaseObject (loop)
	//  SlaveProcessStopDatabase
	boolean SlaveProcess() override;

	// Methode principale de l'esclave, avec demarage de l'utilisation de la base
	//   Recuperation des variables d'entree
	//   Initialisation des variables de sortie
	//   Demarage des bases
	virtual boolean SlaveProcessStartDatabase();

	// Methode principale de l'esclave
	//  Boucle de traitement des objet lus
	//  Collecte des variables en sortie pour les stats sur les objet traites
	virtual boolean SlaveProcessExploitDatabase();

	// Traitement d'un objet, pilote par la methode precedente
	virtual boolean SlaveProcessExploitDatabaseObject(const KWObject* kwoObject);

	// Methode principale de l'esclave, appelee systematiquement
	//   Collecte des variable en sortie par base
	//   Arret des bases
	virtual boolean SlaveProcessStopDatabase(boolean bProcessEndedCorrectly);

	// Finalisation du maitre, avec fermeture des bases
	// A appeler en fin de reimplementation eventuelle
	boolean SlaveFinalize(boolean bProcessEndedCorrectly) override;

	// Calcul de la memoire allouee pour un esclave, en fonction de ce qui etait demande et de ce est alloue
	// Une partie de la memoire alouee est consacree a la gestion de la base source (bForSourceDatabase=true),
	// et la quantite restante est allouee a la gestion du reste de la tache
	// Methode appelable depuis le maitre ou l'esclave
	// Il n'y a pas de methode analogue cote maitre: rien n'est consacre a la gestion de la base source,
	// et donc tout peut etre alloue au reste de la tache
	virtual longint ComputeSlaveGrantedMemory(const RMResourceRequirement* slaveRequirement,
						  longint lSlaveGrantedMemory, boolean bForSourceDatabase);

	/////////////////////////////////////////////////////////////////////////////////
	// Parametres avances pour les tests

	friend class KWTestDatabaseTransfer;

	// Taille des buffers pour effectuer des tests
	// Cette valeur n'est prise en compte que si elle est non nulle
	int nForcedBufferSize;

	// Taille traitee par esclave pour effectuer des tests
	// Cette valeur n'est prise en compte que si elle est non nulle
	longint lForcedMaxFileSizePerProcess;

	///////////////////////////////////////////////////////////
	// Parametres partages par le maitre et les esclaves

	// Fichier dictionnaire echange entre le maitre et les esclaves
	PLShared_String shared_sClassTmpFile;

	// Database source
	PLShared_DatabaseTextFile shared_sourceDatabase;

	//////////////////////////////////////////////////////
	// Parametre en entree des esclaves
	// Un troncon est defini par
	//   . un vecteur de position de debut (comprises),
	//   . un vecteur de position de fin (non comprises),
	//   . un vecteur d'index de record de debut (en fait, le dernier index du troncon precedent)
	//   . la derniere cle racine du troncon precedent
	// La taille des vecteurs et egale au nombre de tables en entree
	// (les index des table non utilises sont ignores lors de traitements).

	// Vecteur des positions de depart des fichiers de la base multi-table en entree
	// Cf. variable oaAllChunksBeginPos du maitre
	PLShared_LongintVector input_lvChunkBeginPos;

	// Vecteur des positions de fin (non compris) des fichiers de la base multi-table en entree
	PLShared_LongintVector input_lvChunkEndPos;

	// Vecteur des index de record de depart des fichiers de la base multi-table en entree
	// Permet une gestion correcte des message d'erreurs
	// Cf. variable oaAllChunksBeginRecordIndex du maitre
	PLShared_LongintVector input_lvChunkBeginRecordIndex;

	// Cles racine de depart pour le traitement de l'esclave
	PLShared_Key input_ChunkLastRootKey;

	// Position pour le pilotage de la lecture du fichier dans le cas mono-table
	PLShared_Longint input_lFileBeginPosition;

	// Nombre de lignes deja lues dans le cas mono-table
	PLShared_Longint input_lFileBeginRecordIndex;

	// Taille du buffer de lecture
	PLShared_Int input_nBufferSize;

	//////////////////////////////////////////////////////
	// Resultats de la tache executee par un esclave

	// Nombre de records lus (lectures physiques)
	PLShared_Longint output_lReadRecords;

	// Nombre d'objets lus (records correctement lus et selectionnes)
	PLShared_Longint output_lReadObjects;

	// Nombre d'enregistrement lus pour chaque table secondaire
	// La valeur -1 est utilisee pour les tables non ouvertes
	PLShared_LongintVector output_lvMappingReadRecords;

	//////////////////////////////////////////////////////
	// Variables du Master

	// Resultats : nombre de records et d'objets lus
	longint lReadRecords;
	longint lReadObjects;

	// Nombre d'objet lus pour toutes les tables
	// La valeur -1 est utilisee pour les tables non ouvertes
	LongintVector lvMappingReadRecords;

	// Indexer de base de donnees
	KWMTDatabaseIndexer mtDatabaseIndexer;

	// Duree d'indexation
	double dDatabaseIndexerTime;

	// Index courant pour acceder aux vecteurs de position et d'index de record
	int nChunkCurrentIndex;

	// Parametrage d'affichage des messages utilisateurs de la taches
	boolean bDisplaySpecificTaskMessage;
	boolean bDisplayEndTaskMessage;
	boolean bDisplayTaskTime;

	//////////////////////////////////////////////////////
	// Variables du Master dans le cas mono-table
	// Pilotage de la lecture par des troncons de fichier
	// definis par une position de depart, une position d'arrivee et
	// nombre de lignes lues precedemment, pour calculer le numero
	// de ligne global par rapport au  numero de ligne local a un esclave

	// Vecteur des positions de lectures du fichier, de 0  a la taille du fichier
	LongintVector lvFileBeginPositions;

	// Vecteur des nombre de lignes lues, de 0 au nombre total de lignes du fichier
	LongintVector lvFileBeginRecordIndexes;

	//////////////////////////////////////////////////////
	// Variables des esclaves

	// Buffer a utiliser dans le cas mono-tabke
	InputBufferedFile inputFileMonoTable;
};
