// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWKeySizeEvaluatorTask.h"

KWKeySizeEvaluatorTask::KWKeySizeEvaluatorTask()
{
	lInputFileSize = 0;
	lTotalLineNumber = 0;
	lTotalKeySize = 0;
	lCummulatedBufferSize = 0;
	DeclareSharedParameter(&shared_sInputFileName);
	DeclareSharedParameter(&shared_cInputFieldSeparator);
	DeclareSharedParameter(&shared_bInputHeaderLineUsed);
	DeclareSharedParameter(&shared_ivKeyFieldIndexes);
	DeclareSharedParameter(&shared_nBufferSize);
	DeclareTaskInput(&input_lBufferStartPosition);
	DeclareTaskOutput(&output_nTotalKeySize);
	DeclareTaskOutput(&output_nLineNumber);
	DeclareTaskOutput(&output_nBufferSize);
}

KWKeySizeEvaluatorTask::~KWKeySizeEvaluatorTask() {}

boolean KWKeySizeEvaluatorTask::EvaluateKeySize(const IntVector* ivKeyIndexes, const ALString& sFileName,
						boolean bIsHeaderLineUsed, char cFieldSeparator, longint& lMeanKeySize,
						longint& lFileLineNumber)
{
	boolean bOk;

	require(ivKeyIndexes != NULL);
	require(ivKeyIndexes->GetSize() > 0);

	// Parametrage du fichier en entree
	shared_sInputFileName.SetValue(sFileName);
	shared_bInputHeaderLineUsed = bIsHeaderLineUsed;
	shared_cInputFieldSeparator.SetValue(cFieldSeparator);

	// Parametrage des index des cles
	shared_ivKeyFieldIndexes.GetIntVector()->CopyFrom(ivKeyIndexes);

	// Lancement de la tache
	lMeanKeySize = 0;
	lFileLineNumber = 0;
	bOk = Run();

	// Calcul des resultats
	if (bOk)
	{
		// Calcul de la taille moyenne des clefs
		lMeanKeySize = 1 + lTotalKeySize / (lTotalLineNumber + 1);

		// Calcul du nombre de lignes
		lFileLineNumber =
		    1 + (longint)ceil(lTotalLineNumber * 1.0 * PLRemoteFileService::GetFileSize(sFileName) /
				      (lCummulatedBufferSize + 1));
	}
	return bOk;
}

void KWKeySizeEvaluatorTask::Test()
{
	KWArtificialDataset artificialDataset;
	longint lMeanKeySize;
	longint lLineNumber;
	KWKeySizeEvaluatorTask keySizeEvaluator;

	// Gestion des taches
	TaskProgression::SetTitle("Test " + keySizeEvaluator.GetTaskLabel());
	if (PLParallelTask::GetDriver()->IsParallelModeAvailable())
		TaskProgression::SetDisplayedLevelNumber(1);
	else
		TaskProgression::SetDisplayedLevelNumber(2);
	TaskProgression::Start();

	// Creation d'un fichier avec des champs cle
	artificialDataset.SpecifySortDataset();
	artificialDataset.CreateDataset();
	artificialDataset.DisplayFirstLines(15);

	// Evaluation de la taille des cles
	KWKeySizeEvaluatorTask::TestWithArtificialDataset(&artificialDataset, lMeanKeySize, lLineNumber);

	// Destruction du fichier
	artificialDataset.DeleteDataset();

	// Gestion des taches
	TaskProgression::Stop();
}

boolean KWKeySizeEvaluatorTask::TestWithArtificialDataset(const KWArtificialDataset* artificialDataset,
							  longint& lMeanKeySize, longint& lLineNumber)
{
	boolean bOk = true;
	KWKeySizeEvaluatorTask keySizeEvaluator;

	require(artificialDataset != NULL);

	bOk = bOk and
	      keySizeEvaluator.EvaluateKeySize(artificialDataset->GetConstKeyFieldIndexes(),
					       artificialDataset->GetFileName(), artificialDataset->GetHeaderLineUsed(),
					       artificialDataset->GetFieldSeparator(), lMeanKeySize, lLineNumber);
	cout << "Mean key size: " << lMeanKeySize << endl;
	cout << "Mean line number: " << lLineNumber << endl;
	return bOk;
}

const ALString KWKeySizeEvaluatorTask::GetTaskName() const
{
	return "Key size evaluator";
}

PLParallelTask* KWKeySizeEvaluatorTask::Create() const
{
	return new KWKeySizeEvaluatorTask;
}

boolean KWKeySizeEvaluatorTask::ComputeResourceRequirements()
{
	boolean bOk = true;
	longint lReadMemoryMax;
	longint lSlaveMemoryMin;
	const int nPreferredSize = PLRemoteFileService::GetPreferredBufferSize(shared_sInputFileName.GetValue());

	// Un buffer de lecture est necessaire
	lSlaveMemoryMin = max((int)InputBufferedFile::nDefaultBufferSize, nPreferredSize);
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMin(lSlaveMemoryMin);

	// Pour la memoire max, on ne veut souhaite pas lire plus de 64Mo et on veut un arrondi par un multiple de
	// GetPreferredBufferSize
	lReadMemoryMax = (SystemFile::nMaxPreferredBufferSize / nPreferredSize) * nPreferredSize;
	lReadMemoryMax = max(lReadMemoryMax, lSlaveMemoryMin);
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMax(lReadMemoryMax);

	// Il n'y aura pas plus de slaveProcess que de buffer a lire
	lInputFileSize = PLRemoteFileService::GetFileSize(shared_sInputFileName.GetValue());
	GetResourceRequirements()->SetMaxSlaveProcessNumber(
	    (int)ceil(lInputFileSize * 1.0 / BufferedFile::nDefaultBufferSize));

	return bOk;
}

boolean KWKeySizeEvaluatorTask::MasterInitialize()
{
	const int nMinBufferNumber = 3;
	int nBufferNumber;
	int nBufferSize;
	longint lOffsetRange;
	LongintVector lvOffsets;
	int i;
	longint lStartPos;
	const int nPreferredBufferSize = PLRemoteFileService::GetPreferredBufferSize(shared_sInputFileName.GetValue());
	const int nMinBufferSize = max((int)BufferedFile::nDefaultBufferSize, nPreferredBufferSize);

	// Initialisation des resultats de la tache
	lTotalKeySize = 0;
	lTotalLineNumber = 0;
	lCummulatedBufferSize = 0;

	// Erreur si fichier inexistant ou vide
	if (not PLRemoteFileService::FileExists(shared_sInputFileName.GetValue()))
	{
		AddError("File " + shared_sInputFileName.GetValue() + " is missing");
		return false;
	}
	assert(lInputFileSize == PLRemoteFileService::GetFileSize(shared_sInputFileName.GetValue()));
	if (lInputFileSize == 0)
	{
		AddError("File " + shared_sInputFileName.GetValue() + " is empty");
		return false;
	}

	/////////////////////////////////////////////////////////////////
	// Calcul de la taille du buffer de lecture

	// Calcul du nombre de buffers, en fonction des contraintes et des ressources
	nBufferNumber = GetProcessNumber();
	if (nBufferNumber < nMinBufferNumber)
		nBufferNumber = nMinBufferNumber;

	// Calcul de la taille des buffers : on prend le plus grand multiple de GetPreferredBufferSize
	nBufferSize = InputBufferedFile::FitBufferSize(
	    (GetTaskResourceGrant()->GetSlaveMemory() / nPreferredBufferSize) * nPreferredBufferSize);
	assert(nBufferSize != 0);

	// Reduction de la taille et du nombre de buffers si le fichier est trop petit
	if (lInputFileSize < nBufferNumber * nBufferSize)
	{
		nBufferSize = (int)(lInputFileSize / nMinBufferNumber);
		if (nBufferSize > nPreferredBufferSize)
			nBufferSize = (nBufferSize / nPreferredBufferSize) * nPreferredBufferSize;

		// Rajustement si buffer trop petits
		if (nBufferSize < nMinBufferSize)
		{
			nBufferSize = nMinBufferSize;
			if (nBufferNumber * nBufferSize > lInputFileSize)
				nBufferNumber = (int)ceil(lInputFileSize * 1.0 / nBufferSize);
		}
	}
	shared_nBufferSize = nBufferSize;
	assert(nBufferNumber >= 1);

	/////////////////////////////////////////////////////////////////
	// Calcul des offset des chaque buffer analyse par les esclaves

	// Calcul de l'espace dans lequel on tire les positions de depart des buffers
	// (en tenant compte de la taille des buffers)
	lOffsetRange = lInputFileSize - (longint)nBufferNumber * nBufferSize;
	if (lOffsetRange < 0)
		lOffsetRange = 0;

	// Tirage aleatoire de positions de depart
	SetRandomSeed(314);
	for (i = 0; i < nBufferNumber; i++)
		lvOffsets.Add((longint)(RandomDouble() * lOffsetRange));
	lvOffsets.Sort();

	// Calcul des positions de depart des buffers
	for (i = 0; i < nBufferNumber; i++)
	{
		lStartPos = lvOffsets.GetAt(i) + (longint)i * nBufferSize;
		if (lStartPos < lInputFileSize)
			lvBufferStartPositions.Add(lStartPos);
	}
	return true;
}

boolean KWKeySizeEvaluatorTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	// Arret quand on a epuise les buffers a analyser
	if (GetTaskIndex() == lvBufferStartPositions.GetSize())
		bIsTaskFinished = true;
	else
	{
		// Avancement represente par cette tache
		dTaskPercent = 1.0 / lvBufferStartPositions.GetSize();

		// Debut du prochain buffer a analyser
		input_lBufferStartPosition = lvBufferStartPositions.GetAt(GetTaskIndex());
	}
	return true;
}

boolean KWKeySizeEvaluatorTask::MasterAggregateResults()
{
	lTotalKeySize += output_nTotalKeySize;
	lTotalLineNumber += output_nLineNumber;
	lCummulatedBufferSize += output_nBufferSize;
	return true;
}

boolean KWKeySizeEvaluatorTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	// Nettoyage
	lvBufferStartPositions.SetSize(0);
	lInputFileSize = 0;
	return bProcessEndedCorrectly;
}

boolean KWKeySizeEvaluatorTask::SlaveInitialize()
{
	boolean bOk = true;
	require(shared_ivKeyFieldIndexes.GetSize() > 0);

	// Initialisation de l'extracteur de cle
	keyExtractor.SetKeyFieldIndexes(shared_ivKeyFieldIndexes.GetConstIntVector());

	// Parametrage du fichier d'entree a analyser pour la tache
	inputFile.SetFileName(shared_sInputFileName.GetValue());
	inputFile.SetFieldSeparator(shared_cInputFieldSeparator.GetValue());
	inputFile.SetHeaderLineUsed(shared_bInputHeaderLineUsed);
	inputFile.SetBufferSize(shared_nBufferSize);

	// Ouverture du fichier en entree
	bOk = inputFile.Open();
	return bOk;
}

boolean KWKeySizeEvaluatorTask::SlaveProcess()
{
	boolean bOk = true;
	KWKey recordKey;
	ALString sTmp;
	longint lBeginPos;
	longint lBeginLinePos;
	longint lMaxEndPos;
	boolean bIsLineOk;

	require(inputFile.IsOpened());

	// Initialisation des resultats
	output_nTotalKeySize = 0;
	output_nLineNumber = 0;
	output_nBufferSize = 0;

	// Recherche du debut de la prochaine ligne
	lMaxEndPos = min(input_lBufferStartPosition + shared_nBufferSize + 1, inputFile.GetFileSize());
	lBeginPos = input_lBufferStartPosition;
	bOk = inputFile.SearchNextLineUntil(lBeginPos, lMaxEndPos, lBeginLinePos);

	// Remplissage du buffer avec des lignes entiere, le buffer peut etre vide si la (seule) ligne est trop longue
	if (bOk)
	{
		lBeginPos = input_lBufferStartPosition;
		bOk = inputFile.FillInnerLines(lBeginPos);
	}

	if (bOk and inputFile.GetCurrentBufferSize() > 0)
	{
		// Prise en compte des caracteristique du buffer, en tenant compte de la premiere ligne sautee
		output_nBufferSize = inputFile.GetCurrentBufferSize();
		output_nLineNumber = inputFile.GetBufferLineNumber();

		// Extraction des clefs
		keyExtractor.SetBufferedFile(&inputFile);

		// Extraction des clefs du fichier, que les lignes soit valides ou non
		while (not inputFile.IsBufferEnd())
		{
			bIsLineOk = keyExtractor.ParseNextKey(&recordKey, NULL);
			output_nTotalKeySize += (int)(sizeof(KWKey*) + recordKey.GetUsedMemory());
		}
	}
	return bOk;
}

boolean KWKeySizeEvaluatorTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;

	// Nettoyage
	keyExtractor.Clean();

	// Fermeture du fichier
	if (inputFile.IsOpened())
		bOk = inputFile.Close();
	return bOk;
}