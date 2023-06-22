// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDatabaseCheckTask.h"

KWDatabaseCheckTask::KWDatabaseCheckTask() {}

KWDatabaseCheckTask::~KWDatabaseCheckTask() {}

boolean KWDatabaseCheckTask::CheckDatabase(const KWDatabase* sourceDatabase)
{
	boolean bOk = true;
	bOk = RunDatabaseTask(sourceDatabase);

	// Messages de fin de tache
	DisplayTaskMessage();
	return bOk;
}

void KWDatabaseCheckTask::DisplaySpecificTaskMessage()
{
	KWDatabaseTask::DisplaySpecificTaskMessage();
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
