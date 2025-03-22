// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#pragma once

#include "KWDatabaseTask.h"
#include "PLFileConcatenater.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KWDatabaseTransferTask
// Transfert de base en parallele
class KWDatabaseTransferTask : public KWDatabaseTask
{
public:
	// Constructeur
	KWDatabaseTransferTask();
	~KWDatabaseTransferTask();

	// Transfer d'une base source vers une base cible
	// Les bases sources et cible doivent etre de meme type, mono ou multi-tables en fichiers textes
	// Methode interruptible, retourne false si erreur ou interruption (avec message), true sinon
	// Les messages utilisateurs sont affiche par defaut
	virtual boolean Transfer(const KWDatabase* sourceDatabase, const KWDatabase* targetDatabase,
				 longint& lWrittenObjectNumber);

	///////////////////////////////////////////////////////////////////////////////
	///// Implementation
protected:
	// Collecte des attributs de type relation non necessaires, que l'on peut passer en not loaded si les tables correspondantes
	// ne sont pas specifiee dans la base en sortie
	void CollectRelationAttributesToUnload(const KWDatabase* targetDatabase, const KWClass* databaseClass,
					       ObjectArray* oaAttributesToUnload) const;

	// Reimplemenattion de l'affichage des messages
	void DisplaySpecificTaskMessage() override;

	// Reimplementation de methodes de gestion du plan d'indexation des bases
	void CleanAllDataTableIndexation() override;
	boolean ComputeDatabaseOpenInformation() override;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Reimplementation des methodes virtuelles de tache

	// Reimplementation des methodes
	const ALString GetTaskName() const override;
	PLParallelTask* Create() const override;
	boolean ComputeResourceRequirements() override;
	boolean MasterInitialize() override;
	boolean MasterInitializeDatabase() override;
	boolean MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished) override;
	boolean MasterAggregateResults() override;
	boolean MasterFinalize(boolean bProcessEndedCorrectly) override;
	boolean SlaveInitializePrepareDatabase() override;
	boolean SlaveInitializeOpenDatabase() override;
	boolean SlaveProcessStartDatabase() override;
	boolean SlaveProcessExploitDatabaseObject(const KWObject* kwoObject) override;
	boolean SlaveProcessStopDatabase(boolean bProcessEndedCorrectly) override;
	boolean SlaveFinalize(boolean bProcessEndedCorrectly) override;

	///////////////////////////////////////////////////////////
	// Parametres partages par le maitre et les esclaves

	// Database destination
	PLShared_DatabaseTextFile shared_targetDatabase;

	//////////////////////////////////////////////////////
	// Parametre en entree des esclaves

	//////////////////////////////////////////////////////
	// Resultats de la tache executee par un esclave

	// Liste des fichiers creee par table lors du traitement d'un chunk
	// Ne contient qu'un seul fichier dans le cas mono-table
	// URI: fichiers partages entre maitre et esclave
	PLShared_StringVector output_svOutputChunkFileNames;

	// Nombre d'enregistrement ecrits
	PLShared_Longint output_lWrittenObjects;

	// Nombre d'enregistrement ecrits pour chaque table secondaire dans le cas multi-tables
	PLShared_LongintVector output_lvMappingWrittenRecords;

	//////////////////////////////////////////////////////
	// Variables du Master

	// Tableau par table des liste de fichiers cree par chunk
	// Chaque poste du tableau (un seul en mono-table) contient la liste des fichiers a concatener
	ObjectArray oaTableChunkFileNames;

	// Resultats : nombre d'objets ecrits dans le transfert
	longint lWrittenObjects;

	// Nombre d'objet ecrits pour toutes les tables
	// La valeur -1 est utilisee pour les tables non ouvertes
	LongintVector lvMappingWrittenRecords;
};
