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

	// Un buffer de lecture est necessaire
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->Set(BufferedFile::nDefaultBufferSize);

	// Il n'y aura pas plus de slaveProcess que de buffer a lire
	lInputFileSize = PLRemoteFileService::GetFileSize(shared_sInputFileName.GetValue());
	GetResourceRequirements()->SetMaxSlaveProcessNumber(
	    (int)ceil(lInputFileSize * 1.0 / BufferedFile::nDefaultBufferSize));

	return bOk;
}

boolean KWKeySizeEvaluatorTask::MasterInitialize()
{
	const int nMinBufferNumber = 3;
	const int nMinBufferSize = 4 * lMB;
	int nBufferNumber;
	int nBufferSize;
	longint lOffsetRange;
	LongintVector lvOffsets;
	int i;

	// Initialisation des resultats de la tache
	lTotalKeySize = 0;
	lTotalLineNumber = 0;
	lCummulatedBufferSize = 0;

	// Erreur si fichier inexistant ou vide
	if (not PLRemoteFileService::Exist(shared_sInputFileName.GetValue()))
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
	// Calcul des offset des chaque buffer analyse par les esclaves

	// Calcul du nombre de buffers, en fonction des contraintes et des ressources
	nBufferNumber = GetProcessNumber();
	if (nBufferNumber < nMinBufferNumber)
		nBufferNumber = nMinBufferNumber;

	// Calcul de la taille des buffers, en fonction des contraintes et des ressources
	nBufferSize = BufferedFile::nDefaultBufferSize;
	if (lInputFileSize < nBufferNumber * nBufferSize)
	{
		nBufferSize = (int)(lInputFileSize / nMinBufferNumber);

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

	// Calcul de l'espace dans lequel on tire les positions de depart des buffers
	// (en tenant compte de la taille des buffers)
	lOffsetRange = lInputFileSize - nBufferNumber * nBufferSize;
	if (lOffsetRange < 0)
		lOffsetRange = 0;

	// Tirage aleatoire de positions de depart
	for (i = 0; i < nBufferNumber; i++)
		lvOffsets.Add((longint)(RandomDouble() * lOffsetRange));
	lvOffsets.Sort();

	// Calcul des positions de depart des buffers
	for (i = 0; i < nBufferNumber; i++)
	{
		if (lvOffsets.GetAt(i) + i * nBufferSize < lInputFileSize)
			lvBufferStartPositions.Add(lvOffsets.GetAt(i) + i * nBufferSize);
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
	require(shared_ivKeyFieldIndexes.GetSize() > 0);

	// Initialisation de l'extracteur de cle
	keyExtractor.SetKeyFieldIndexes(shared_ivKeyFieldIndexes.GetConstIntVector());
	return true;
}

boolean KWKeySizeEvaluatorTask::SlaveProcess()
{
	boolean bOk = true;
	InputBufferedFile* inputFile;
	KWKey recordKey;
	ALString sTmp;

	// Initialisation des resultats
	output_nTotalKeySize = 0;
	output_nLineNumber = 0;
	output_nBufferSize = 0;

	// Creation du fichier d'entree a analyser pour la tache
	inputFile = new InputBufferedFile;
	inputFile->SetFieldSeparator(shared_cInputFieldSeparator.GetValue());
	inputFile->SetFileName(shared_sInputFileName.GetValue());
	inputFile->SetBufferSize(shared_nBufferSize);
	inputFile->SetHeaderLineUsed(shared_bInputHeaderLineUsed);
	// TODO le fichier ne change pas, il faudrait peut etre mieux l'ouvrir dans le SlaveInitialize et le fermer dans
	// le SlaveFinalize

	// Ouverture
	bOk = inputFile->Open();
	if (bOk)
	{
		// Remplissage du buffer, avec gestion des lignes trop longues
		inputFile->Fill(input_lBufferStartPosition);
		if (inputFile->IsError())
			bOk = false;
	}

	if (bOk and not inputFile->GetBufferSkippedLine())
	{
		// Prise en compte des caracteristique du buffer, en tenant compte de la premiere ligne sautee
		output_nBufferSize = inputFile->GetCurrentBufferSize();
		output_nLineNumber = inputFile->GetBufferLineNumber();

		// Extraction des clefs
		keyExtractor.SetBufferedFile(inputFile);

		// Extraction des clefs du fichier
		while (not inputFile->IsBufferEnd())
		{
			keyExtractor.ParseNextKey(NULL);
			keyExtractor.ExtractKey(&recordKey);
			output_nTotalKeySize += (int)(sizeof(KWKey*) + recordKey.GetUsedMemory());
		}
	}

	// Fermeture du fichier
	if (inputFile->IsOpened())
		bOk = inputFile->Close() and bOk;

	// Nettoyage
	delete inputFile;
	return bOk;
}

boolean KWKeySizeEvaluatorTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	// Nettoyage
	keyExtractor.Clean();

	return true;
}