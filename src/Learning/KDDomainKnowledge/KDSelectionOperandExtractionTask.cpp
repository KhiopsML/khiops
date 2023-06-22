// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KDSelectionOperandExtractionTask.h"

KDSelectionOperandExtractionTask::KDSelectionOperandExtractionTask() {}

KDSelectionOperandExtractionTask::~KDSelectionOperandExtractionTask() {}

boolean KDSelectionOperandExtractionTask::CollectSelectionOperandSamples(
    KDSelectionOperandAnalyser* selectionOperandAnalyser, const KWDatabase* sourceDatabase, int& nReadObjectNumber)
{
	boolean bOk;

	require(selectionOperandAnalyser != NULL);
	require(selectionOperandAnalyser->Check());
	require(sourceDatabase != NULL);
	require(sourceDatabase->IsMultiTableTechnology());

	// Lancement de la tache
	bOk = RunDatabaseTask(sourceDatabase);
	return true;
}

const ALString KDSelectionOperandExtractionTask::GetTaskName() const
{
	return "Selection operand extraction";
}

PLParallelTask* KDSelectionOperandExtractionTask::Create() const
{
	return new KDSelectionOperandExtractionTask;
}

boolean KDSelectionOperandExtractionTask::ComputeResourceRequirements()
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::ComputeResourceRequirements();
	return bOk;
}

boolean KDSelectionOperandExtractionTask::MasterInitialize()
{
	boolean bOk;
	// longint lMainRecordNumber;

	require(mtDatabaseIndexer.IsComputed());

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterInitialize();

	// Recherche du nombre de record de la table principales
	// lMainRecordNumber = mtDatabaseIndexer.GetChunkBeginRecordIndexesAt(0);

	return bOk;
}

boolean KDSelectionOperandExtractionTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterPrepareTaskInput(dTaskPercent, bIsTaskFinished);
	return bOk;
}

boolean KDSelectionOperandExtractionTask::MasterAggregateResults()
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterAggregateResults();
	return bOk;
}

boolean KDSelectionOperandExtractionTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterFinalize(bProcessEndedCorrectly);
	return bOk;
}

boolean KDSelectionOperandExtractionTask::SlaveInitializePrepareDatabase()
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveInitializePrepareDatabase();
	return bOk;
}

boolean KDSelectionOperandExtractionTask::SlaveInitializeOpenDatabase()
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveInitializeOpenDatabase();
	return bOk;
}

boolean KDSelectionOperandExtractionTask::SlaveProcessStartDatabase()
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveProcessStartDatabase();
	return bOk;
}

boolean KDSelectionOperandExtractionTask::SlaveProcessExploitDatabaseObject(const KWObject* kwoObject)
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveProcessExploitDatabaseObject(kwoObject);
	return bOk;
}

boolean KDSelectionOperandExtractionTask::SlaveProcessStopDatabase(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveProcessStopDatabase(bProcessEndedCorrectly);
	return bOk;
}

boolean KDSelectionOperandExtractionTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveFinalize(bProcessEndedCorrectly);
	return bOk;
}

// Tableau de nombres premiers superieurs aux puissances de 2 pour des longint
// pour servir de base a la generation de nombre aleatoires associes
// a des enregistrements en multi-tables
static const longint lKDSelectionOperandExtractionTaskPrimeSizes[] = {65537,
								      131101,
								      262147,
								      524309,
								      1048583,
								      2097169,
								      4194319,
								      8388617,
								      16777259,
								      33554467,
								      67108879,
								      134217757,
								      268435459,
								      536870923,
								      1073741827,
								      2147483659,
								      4294967311,
								      8589934609,
								      17179869209,
								      34359738421,
								      68719476767,
								      137438953481,
								      274877906951,
								      549755813911,
								      1099511627791,
								      2199023255579,
								      4398046511119,
								      8796093022237,
								      17592186044423,
								      35184372088891,
								      70368744177679,
								      140737488355333,
								      281474976710677,
								      562949953421381,
								      1125899906842679,
								      2251799813685269,
								      4503599627370517,
								      9007199254740997,
								      18014398509482143,
								      36028797018963971,
								      72057594037928017,
								      144115188075855881,
								      288230376151711813,
								      576460752303423619,
								      1152921504606847009,
								      2305843009213693967,
								      4611686018427388039};

longint KDSelectionOperandExtractionTask::GetNextPrimeNumber(longint lMinValue)
{
	const int nDictionaryPrimeSizeNumber = sizeof(lKDSelectionOperandExtractionTaskPrimeSizes) / sizeof(longint);
	longint lPrimeNumber = 0;
	int i;

	require(0 <= lMinValue and
		lMinValue <= lKDSelectionOperandExtractionTaskPrimeSizes[nDictionaryPrimeSizeNumber - 1]);

	// Rend la taille de table superieure ou egale a une taille donnee
	for (i = 0; i < nDictionaryPrimeSizeNumber; i++)
	{
		lPrimeNumber = lKDSelectionOperandExtractionTaskPrimeSizes[i];
		if (lPrimeNumber >= lMinValue)
			break;
	}
	assert(0 < lPrimeNumber);
	return lPrimeNumber;
}
