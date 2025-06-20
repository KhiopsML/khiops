// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDatabaseCheckTask.h"

KWDatabaseCheckTask::KWDatabaseCheckTask() {}

KWDatabaseCheckTask::~KWDatabaseCheckTask() {}

boolean KWDatabaseCheckTask::CheckDatabase(const KWDatabase* sourceDatabase)
{
	boolean bOk = true;

	// Lancement de la tache
	// On utilise un DatabaseIndexer local a la tache, car il n'y pas ici d'indexation de base a partager
	bOk = RunDatabaseTask(sourceDatabase);

	// Message sur les eventuelles erreurs d'encodage
	if (bOk)
		sourceDatabase->AddEncodingErrorMessage();
	return bOk;
}

void KWDatabaseCheckTask::DisplaySpecificTaskMessage()
{
	// Affichage par defaut pour avoir le nombre de records selectionnes et les records secobdaire en multi-tables
	KWDatabaseTask::DisplaySpecificTaskMessage();

	// Message specifique
	shared_sourceDatabase.GetPLDatabase()->DisplayAllTableMessages("Checked database", "Checked records",
								       GetReadObjects(), -1, NULL);
}

const ALString KWDatabaseCheckTask::GetTaskName() const
{
	return "Database check";
}

PLParallelTask* KWDatabaseCheckTask::Create() const
{
	return new KWDatabaseCheckTask;
}
