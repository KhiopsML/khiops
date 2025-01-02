// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWFileIndexerTask.h"

KWFileIndexerTask::KWFileIndexerTask()
{
	lvTaskFileBeginPositions = NULL;
	lvTaskFileBeginRecordIndexes = NULL;
	lFileSize = 0;
	lFilePos = 0;

	// Declaration des variables partagees
	DeclareSharedParameter(&shared_sFileName);
	DeclareSharedParameter(&shared_nBufferSize);
	DeclareSharedParameter(&shared_nPositionNumberPerBuffer);
	DeclareTaskInput(&input_lFilePos);
	DeclareTaskOutput(&output_lvFileStartLinePositions);
	DeclareTaskOutput(&output_ivBufferLineCounts);
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

boolean KWFileIndexerTask::ComputeIndexation(int nBufferSize, int nPositionNumberPerBuffer,
					     LongintVector* lvFileBeginPositions,
					     LongintVector* lvFileBeginRecordIndexes)
{
	boolean bOk;
	boolean bDisplay = false;
	int i;

	require(nBufferSize > SystemFile::nMinPreferredBufferSize);
	require(nPositionNumberPerBuffer >= 1);
	require(lvFileBeginPositions != NULL);
	require(lvFileBeginRecordIndexes != NULL);
	require(lvFileBeginPositions->GetSize() == 0);
	require(lvFileBeginRecordIndexes->GetSize() == 0);

	// Acces aux parametres de la methode, pour toutes les methodes
	// du maitre et de l'esclave
	shared_nBufferSize = nBufferSize;
	shared_nPositionNumberPerBuffer = nPositionNumberPerBuffer;
	lvTaskFileBeginPositions = lvFileBeginPositions;
	lvTaskFileBeginRecordIndexes = lvFileBeginRecordIndexes;

	// Lancement de la tache
	bOk = Run();

	// Nettoyage
	shared_nBufferSize = 0;
	shared_nPositionNumberPerBuffer = 0;
	lvTaskFileBeginPositions = NULL;
	lvTaskFileBeginRecordIndexes = NULL;

	// Affichage des resultats
	if (bDisplay)
	{
		cout << " KWFileIndexerTask::ComputeIndexation\t" << nBufferSize << "\n";
		cout << "Position\tRecord index\n";
		for (i = 0; i < lvFileBeginPositions->GetSize(); i++)
			cout << "\t" << lvFileBeginPositions->GetAt(i) << "\t" << lvFileBeginRecordIndexes->GetAt(i)
			     << "\n";
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

	// Indexation pour faire des chunk de 1 MB en moyenne
	bOk = task.ComputeIndexation(4 * lMB, 4, &lvFileBegin, &lvFileBeginRecords);
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
	longint lSlaveResultMemory;

	// Taille du fichier
	lFileSize = PLRemoteFileService::GetFileSize(sFileName);

	// Pour les esclave, en memoire, taille du buffer + taille maximum d'une ligne (la taille du buffer peut etre
	// etendue en cas de ligne trop longue) plus stockage des resultats
	lSlaveResultMemory = shared_nPositionNumberPerBuffer * (sizeof(int) + sizeof(longint)) + sizeof(IntVector) +
			     sizeof(LongintVector);
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->Set(
	    shared_nBufferSize + InputBufferedFile::GetMaxLineLength() + lSlaveResultMemory);

	// Pour le maitre en memoire: stockage des resultats des esclaves plus des resultats globaux
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->Set(
	    (1 + (lFileSize / shared_nBufferSize)) *
	    (lSlaveResultMemory + shared_nPositionNumberPerBuffer * 2 * sizeof(longint)));

	// Nombre d'esclaves
	nMaxSlaveProcessNumber = (int)min(ceil((lFileSize + 1.0) / shared_nBufferSize), INT_MAX * 1.0);
	ensure(nMaxSlaveProcessNumber > 0);
	GetResourceRequirements()->SetMaxSlaveProcessNumber(nMaxSlaveProcessNumber);
	return true;
}

boolean KWFileIndexerTask::MasterInitialize()
{
	require(lvTaskFileBeginPositions != NULL and lvTaskFileBeginRecordIndexes != NULL);
	require(oaMasterFileStartLinePositionVectors.GetSize() == 0);
	require(oaMasterBufferLineCountVectors.GetSize() == 0);

	// Initialisations des variables de travail
	shared_sFileName.SetValue(sFileName);
	lFilePos = 0;
	lvTaskFileBeginPositions->SetSize(0);
	lvTaskFileBeginRecordIndexes->SetSize(0);
	return true;
}

boolean KWFileIndexerTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	int nInputBufferSize;

	if (lFilePos >= lFileSize)
		bIsTaskFinished = true;
	else
	{
		assert(lFilePos < lFileSize);

		// On n'utilise pas le service de StairBufferSize de ParallelTask pour une tache aussi elementaire
		nInputBufferSize = shared_nBufferSize;
		if ((longint)nInputBufferSize > lFileSize - lFilePos)
			nInputBufferSize = (int)(lFileSize - lFilePos);
		assert(nInputBufferSize > 0);
		input_lFilePos = lFilePos;
		assert(input_lFilePos % shared_nBufferSize == 0);

		// Nouvelle position a traiter pour le prochain esclave
		lFilePos += nInputBufferSize;
		assert(lFilePos <= lFileSize);

		// On reserve une place dans le tableau des vecteur resultats
		oaMasterFileStartLinePositionVectors.Add(NULL);
		oaMasterBufferLineCountVectors.Add(NULL);
		assert(oaMasterFileStartLinePositionVectors.GetSize() == GetTaskIndex() + 1);
		assert(oaMasterFileStartLinePositionVectors.GetSize() == oaMasterBufferLineCountVectors.GetSize());

		// Calcul de la progression
		dTaskPercent = nInputBufferSize * 1.0 / lFileSize;
		if (dTaskPercent > 1)
			dTaskPercent = 1;
	}
	return true;
}

boolean KWFileIndexerTask::MasterAggregateResults()
{
	require(oaMasterFileStartLinePositionVectors.GetAt(GetTaskIndex()) == NULL);
	require(oaMasterBufferLineCountVectors.GetAt(GetTaskIndex()) == NULL);
	require(output_lvFileStartLinePositions.GetSize() == output_ivBufferLineCounts.GetSize());
	require(output_ivBufferLineCounts.GetSize() >= 1);

	// Memorisation en position TaskIndex des resultats de l'esclave
	oaMasterFileStartLinePositionVectors.SetAt(
	    GetTaskIndex(), cast(Object*, output_lvFileStartLinePositions.GetConstLongintVector()));
	oaMasterBufferLineCountVectors.SetAt(GetTaskIndex(),
					     cast(Object*, output_ivBufferLineCounts.GetConstIntVector()));

	// Derefencement des vecteur renvoyes par l'esclave, qui sont maintenant gere par le maitre
	output_lvFileStartLinePositions.RemoveObject();
	output_ivBufferLineCounts.RemoveObject();
	return true;
}

boolean KWFileIndexerTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	LongintVector* lvSlaveFileStartLinePositions;
	IntVector* ivSlaveBufferLineCounts;
	longint lLineCount;
	int nIndex;
	int i;

	require(lvTaskFileBeginPositions->GetSize() == 0);
	require(lvTaskFileBeginRecordIndexes->GetSize() == 0);
	require(oaMasterFileStartLinePositionVectors.GetSize() == oaMasterBufferLineCountVectors.GetSize());

	// Finalisation des resultats si ok
	if (bProcessEndedCorrectly)
	{
		// Le premier record commence a la ligne 0
		lvTaskFileBeginPositions->Add(0);
		lvTaskFileBeginRecordIndexes->Add(0);

		// Prise en compte des resulatts des esclave dans l'ordre des taches
		lLineCount = 0;
		for (nIndex = 0; nIndex < oaMasterFileStartLinePositionVectors.GetSize(); nIndex++)
		{
			// Acces aux resultats
			lvSlaveFileStartLinePositions =
			    cast(LongintVector*, oaMasterFileStartLinePositionVectors.GetAt(nIndex));
			ivSlaveBufferLineCounts = cast(IntVector*, oaMasterBufferLineCountVectors.GetAt(nIndex));
			check(lvSlaveFileStartLinePositions);
			check(ivSlaveBufferLineCounts);
			assert(lvSlaveFileStartLinePositions->GetSize() == ivSlaveBufferLineCounts->GetSize());

			// Prise en compte des resultats
			for (i = 0; i < lvSlaveFileStartLinePositions->GetSize(); i++)
			{
				// Cumul du nombre de lignes, en tenant compte que l'on traite des comptes de ligne dans
				// les buffers des esclaves
				assert(ivSlaveBufferLineCounts->GetAt(i) > 0);
				assert(i == 0 or
				       ivSlaveBufferLineCounts->GetAt(i) > ivSlaveBufferLineCounts->GetAt(i - 1));
				if (i == 0)
					lLineCount += ivSlaveBufferLineCounts->GetAt(0);
				else
					lLineCount +=
					    ivSlaveBufferLineCounts->GetAt(i) - ivSlaveBufferLineCounts->GetAt(i - 1);

				// Mise a jour des resultats du maitre
				lvTaskFileBeginPositions->Add(lvSlaveFileStartLinePositions->GetAt(i));
				lvTaskFileBeginRecordIndexes->Add(lLineCount);
				assert(lvTaskFileBeginPositions->GetAt(lvTaskFileBeginPositions->GetSize() - 1) >
				       lvTaskFileBeginPositions->GetAt(lvTaskFileBeginPositions->GetSize() - 2));
				assert(
				    lvTaskFileBeginRecordIndexes->GetAt(lvTaskFileBeginRecordIndexes->GetSize() - 1) >
				    lvTaskFileBeginRecordIndexes->GetAt(lvTaskFileBeginRecordIndexes->GetSize() - 2));
			}
		}
	}

	// Nettoyage des donnees de travail
	oaMasterFileStartLinePositionVectors.DeleteAll();
	oaMasterBufferLineCountVectors.DeleteAll();

	ensure(lvTaskFileBeginPositions->GetSize() == lvTaskFileBeginRecordIndexes->GetSize());
	assert(lvTaskFileBeginPositions->GetAt(lvTaskFileBeginPositions->GetSize() - 1) == lFileSize);
	return true;
}

boolean KWFileIndexerTask::SlaveInitialize()
{
	boolean bOk;

	// Parametrage du fichier en entree
	inputFile.SetFileName(shared_sFileName.GetValue());

	// Ouverture du fichier en entree
	inputFile.SetBufferSize(shared_nBufferSize);
	bOk = inputFile.Open();
	return bOk;
}

boolean KWFileIndexerTask::SlaveProcess()
{
	boolean bOk = true;
	int nPositionNumber;
	longint lBeginPos;
	longint lMaxEndPos;
	longint lNextLinePos;
	boolean bLineTooLong;
	int nCumulatedLineNumber;

	require(inputFile.IsOpened());
	require(inputFile.GetBufferSize() == shared_nBufferSize);
	require(input_lFilePos % shared_nBufferSize == 0);

	assert(output_lvFileStartLinePositions.GetSize() == 0);
	assert(output_ivBufferLineCounts.GetSize() == 0);

	// Specification de la portion du fichier a traiter
	lBeginPos = input_lFilePos;
	lMaxEndPos = min(input_lFilePos + shared_nBufferSize, inputFile.GetFileSize());

	// On commence par se caller sur un debut de ligne, sauf si on est en debut de fichier
	nCumulatedLineNumber = 0;
	if (lBeginPos > 0)
	{
		bOk = inputFile.SearchNextLineUntil(lBeginPos, lMaxEndPos, lNextLinePos);
		if (bOk)
		{
			// On se positionne sur le debut de la ligne suivante si elle est trouvee
			if (lNextLinePos != -1)
			{
				lBeginPos = lNextLinePos;
				nCumulatedLineNumber = 1;
			}
			// Si non trouvee, on se place en fin de chunk
			else
				lBeginPos = lMaxEndPos;
		}
	}
	assert(inputFile.GetCurrentBufferSize() == 0);

	// Remplissage du buffer avec des lignes entieres dans la limite de la taille du buffer
	// On ignorera ainsi le debut de la derniere ligne commencee avant lMaxEndPos
	// Ce n'est pas grave d'ignorer au plus une ligne par buffer, puisque l'on est ici dans une optique
	// d'échantillonnage
	if (bOk and lBeginPos < lMaxEndPos)
		bOk = inputFile.FillInnerLinesUntil(lBeginPos, lMaxEndPos);

	// Analyse des lignes du buffer
	if (bOk and inputFile.GetCurrentBufferSize() > 0)
	{
		assert(inputFile.GetCurrentBufferSize() <= shared_nBufferSize);

		// Calcul du nombre de position a calculer
		nPositionNumber =
		    int(shared_nPositionNumberPerBuffer * inputFile.GetCurrentBufferSize() * 1.0 / shared_nBufferSize);
		nPositionNumber = max(nPositionNumber, 1);
		nPositionNumber = min(nPositionNumber, inputFile.GetBufferLineNumber());

		// Ajout de position intermediaires si necessaire
		while (output_lvFileStartLinePositions.GetSize() < nPositionNumber - 1)
		{
			assert(not inputFile.IsBufferEnd());

			// Lecture d'une ligne
			// Il n'est pas utile ici d'avoir un warning en cas de ligne trop longue
			inputFile.SkipLine(bLineTooLong);

			// Memorisation si necessaire de l'index de la ligne precedente et de sa position de fin
			if (inputFile.GetCurrentLineIndex() - 1 ==
			    ((output_ivBufferLineCounts.GetSize() + 1) * (longint)inputFile.GetBufferLineNumber()) /
				nPositionNumber)
			{
				output_lvFileStartLinePositions.Add(inputFile.GetPositionInFile());
				output_ivBufferLineCounts.Add(nCumulatedLineNumber + inputFile.GetCurrentLineIndex() -
							      1);
			}
		}

		// Prise en compte des lignes du buffer, avec rajout sauf en fin de fichier de la derniere ligne traitee
		// dans le chunk suivant
		nCumulatedLineNumber += inputFile.GetBufferLineNumber();

		// Ajout des informations d'indexation sur la derniere ligne du buffer
		output_lvFileStartLinePositions.Add(inputFile.GetBufferStartInFile() +
						    inputFile.GetCurrentBufferSize());
		output_ivBufferLineCounts.Add(nCumulatedLineNumber);
	}
	return bOk;
}

boolean KWFileIndexerTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;

	// Fermeture du fichier
	if (inputFile.IsOpened())
		bOk = inputFile.Close();
	return bOk;
}
