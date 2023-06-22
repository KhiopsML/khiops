// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWFileIndexerTask.h"

KWFileIndexerTask::KWFileIndexerTask()
{
	lvTaskFileBeginPositions = NULL;
	lvTaskFileBeginRecordIndexes = NULL;
	nTaskMaxBufferSize = 0;
	lFileSize = 0;
	lFilePos = 0;

	// Declaration des variables partagees
	DeclareSharedParameter(&shared_sFileName);
	DeclareTaskInput(&input_lFilePos);
	DeclareTaskInput(&input_nBufferSize);
	DeclareTaskOutput(&output_lStartLinePos);
	DeclareTaskOutput(&output_nLineCount);
}

KWFileIndexerTask::~KWFileIndexerTask() {}

void KWFileIndexerTask::SetFileName(const ALString& sValue)
{
	sFileName = sValue;
}

const ALString& KWFileIndexerTask::GetFileName() const
{
	return sFileName;
}

boolean KWFileIndexerTask::ComputeIndexation(int nMaxBufferSize, LongintVector* lvFileBeginPositions,
					     LongintVector* lvFileBeginRecordIndexes)
{
	boolean bOk;
	boolean bDisplay = false;
	int i;

	require(0 < nMaxBufferSize);
	require(lvFileBeginPositions != NULL);
	require(lvFileBeginRecordIndexes != NULL);
	require(lvFileBeginPositions->GetSize() == 0);
	require(lvFileBeginRecordIndexes->GetSize() == 0);

	// Acces aux parametres de la methode, pour toutes les methodes
	// du maitre et de l'esclave
	lvTaskFileBeginPositions = lvFileBeginPositions;
	lvTaskFileBeginRecordIndexes = lvFileBeginRecordIndexes;
	nTaskMaxBufferSize = nMaxBufferSize;

	// Lancement de la tache
	bOk = Run();

	// Nettoyage
	lvTaskFileBeginPositions = NULL;
	lvTaskFileBeginRecordIndexes = NULL;
	nTaskMaxBufferSize = 0;

	// Affichage des resultats
	if (bDisplay)
	{
		cout << " KWFileIndexerTask::ComputeIndexation\t" << nMaxBufferSize << "\n";
		cout << "Position\tRecord index\n";
		for (i = 0; i < lvFileBeginPositions->GetSize(); i++)
			cout << lvFileBeginPositions->GetAt(i) << "\t" << lvFileBeginRecordIndexes->GetAt(i) << "\n";
	}
	return bOk;
}

const ALString KWFileIndexerTask::GetObjectLabel() const
{
	return sFileName;
}

void KWFileIndexerTask::Test()
{
	KWArtificialDataset dataset;
	KWFileIndexerTask task;
	LongintVector lvFileBegin;
	LongintVector lvFileBeginRecords;
	int i;
	boolean bOk;

	// Creation d'un fichier artificiel de 8Mo
	dataset.SetLineNumber(10000);
	dataset.SetFieldNumber(100);
	dataset.CreateDataset();
	cout << "File size : " << LongintToReadableString(FileService::GetFileSize(dataset.GetFileName())) << endl;

	task.SetFileName(dataset.GetFileName());

	// Indexation pour faire des chunk de 1Mo
	bOk = task.ComputeIndexation(1 * lMB, &lvFileBegin, &lvFileBeginRecords);
	if (bOk)
	{
		// resultats de l'indexation
		for (i = 0; i < lvFileBegin.GetSize(); i++)
		{
			cout << "Line " << LongintToReadableString(lvFileBeginRecords.GetAt(i)) << " at position "
			     << LongintToReadableString(lvFileBegin.GetAt(i)) << endl;
		}
	}
	else
	{
		cout << "Error" << endl;
	}

	FileService::RemoveFile(dataset.GetFileName());
}

const ALString KWFileIndexerTask::GetTaskName() const
{
	return "File indexer";
}

PLParallelTask* KWFileIndexerTask::Create() const
{
	return new KWFileIndexerTask;
}

boolean KWFileIndexerTask::ComputeResourceRequirements()
{
	int nMaxSlaveProcessNumber;

	// Taille du fichier
	lFileSize = PLRemoteFileService::GetFileSize(sFileName);

	// En memoire, taille du buffer + taille maximum d'une ligne (la taille du buffer peut etre etendue en cas de
	// ligne trop longue)
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->Set(nTaskMaxBufferSize +
									   InputBufferedFile::GetMaxLineLength());
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->Set((lFileSize / nTaskMaxBufferSize) * 2 *
									    sizeof(longint));

	// Nombre d'esclaves
	nMaxSlaveProcessNumber = (int)min(ceil((lFileSize + 1.0) / nMinBufferSize), INT_MAX * 1.0);
	ensure(nMaxSlaveProcessNumber > 0);
	GetResourceRequirements()->SetMaxSlaveProcessNumber(nMaxSlaveProcessNumber);
	return true;
}

boolean KWFileIndexerTask::MasterInitialize()
{
	require(lvTaskFileBeginPositions != NULL and lvTaskFileBeginRecordIndexes != NULL);

	// Initialisations des variables de travail
	shared_sFileName.SetValue(sFileName);
	lFilePos = 0;
	lvTaskFileBeginPositions->SetSize(0);
	lvTaskFileBeginRecordIndexes->SetSize(0);

	// Le premier record commence a la ligne 0
	lvTaskFileBeginPositions->Add(0);
	lvTaskFileBeginRecordIndexes->Add(0);
	return true;
}

boolean KWFileIndexerTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	if (lFilePos >= lFileSize)
		bIsTaskFinished = true;
	else
	{
		if (nMinBufferSize < nTaskMaxBufferSize)
			input_nBufferSize =
			    ComputeStairBufferSize(nMinBufferSize, nTaskMaxBufferSize, lFilePos, lFileSize);
		else
			input_nBufferSize = nTaskMaxBufferSize;
		if ((longint)input_nBufferSize > lFileSize - lFilePos)
			input_nBufferSize = (int)(lFileSize - lFilePos);
		input_lFilePos = lFilePos;
		lFilePos += input_nBufferSize;
		assert(input_nBufferSize > 0);
		assert(lFilePos <= lFileSize);

		// On reserve une place dans le vecteurs
		lvTaskFileBeginPositions->Add(0);
		lvTaskFileBeginRecordIndexes->Add(0);
		assert(lvTaskFileBeginPositions->GetSize() == GetTaskIndex() + 2);
		assert(lvTaskFileBeginPositions->GetSize() == lvTaskFileBeginRecordIndexes->GetSize());

		// Calcul de la progression
		dTaskPercent = input_nBufferSize * 1.0 / lFileSize;
		if (dTaskPercent > 1)
			dTaskPercent = 1;
	}
	return true;
}

boolean KWFileIndexerTask::MasterAggregateResults()
{
	require(lvTaskFileBeginPositions->GetAt(GetTaskIndex() + 1) == 0);
	require(lvTaskFileBeginRecordIndexes->GetAt(GetTaskIndex() + 1) == 0);

	// Attention, c'est en position TaskIndex+1, car la premiere position a ete
	// initialisee par le maitre pur le tout debut de fichier en position 0
	lvTaskFileBeginPositions->SetAt(GetTaskIndex() + 1, output_lStartLinePos);
	lvTaskFileBeginRecordIndexes->SetAt(GetTaskIndex() + 1, output_nLineCount);
	return true;
}

boolean KWFileIndexerTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	longint lLineCount;
	int i;
	int nNewSize;

	// Cumul du nombre de lignes
	lLineCount = 0;
	for (i = 0; i < lvTaskFileBeginRecordIndexes->GetSize(); i++)
	{
		lLineCount += lvTaskFileBeginRecordIndexes->GetAt(i);
		lvTaskFileBeginRecordIndexes->SetAt(i, lLineCount);
	}
	assert(lvTaskFileBeginRecordIndexes->GetAt(0) == 0);

	// Nettoyage des doublons, pour le cas ou deux esclaves successifs produisent un meme resultats
	nNewSize = 1;
	for (i = 1; i < lvTaskFileBeginPositions->GetSize(); i++)
	{
		if (lvTaskFileBeginPositions->GetAt(i) > lvTaskFileBeginPositions->GetAt(i - 1))
		{
			lvTaskFileBeginPositions->SetAt(nNewSize, lvTaskFileBeginPositions->GetAt(i));
			lvTaskFileBeginRecordIndexes->SetAt(nNewSize, lvTaskFileBeginRecordIndexes->GetAt(i));
			nNewSize++;
		}
	}
	lvTaskFileBeginPositions->SetSize(nNewSize);
	lvTaskFileBeginRecordIndexes->SetSize(nNewSize);

	ensure(lvTaskFileBeginPositions->GetSize() == lvTaskFileBeginRecordIndexes->GetSize());
	assert(lvTaskFileBeginPositions->GetAt(lvTaskFileBeginPositions->GetSize() - 1) == lFileSize);
	return true;
}

boolean KWFileIndexerTask::SlaveInitialize()
{
	bufferedFile.SetFileName(shared_sFileName.GetValue());
	return true;
}

boolean KWFileIndexerTask::SlaveProcess()
{
	boolean bOk;

	output_lStartLinePos = 0;
	output_nLineCount = 0;

	bufferedFile.SetBufferSize(input_nBufferSize);
	bOk = bufferedFile.Open();
	if (bOk)
	{
		bOk = bufferedFile.Fill(input_lFilePos);
		if (bOk)
		{
			output_lStartLinePos =
			    bufferedFile.GetBufferStartFilePos() + bufferedFile.GetCurrentBufferSize();
			output_nLineCount = bufferedFile.GetBufferLineNumber();
		}
		bufferedFile.Close();
	}

	return bOk;
}

boolean KWFileIndexerTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	return true;
}
