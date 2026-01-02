// Copyright (c) 2023-2026 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PEIOParallelTestTask.h"

PEIOParallelTestTask::PEIOParallelTestTask()
{
	// Declaration des variables partagees
	DeclareTaskInput(&input_sFileToProcess);
	DeclareTaskOutput(&output_lFieldNumber);
	DeclareTaskOutput(&output_lLineNumber);

	// Resultats de la tache
	lTotalFieldNumber = 0;
	lTotalLineNumber = 0;
	lTotalFileSizes = 0;
	nFileIndex = 0;
}

PEIOParallelTestTask::~PEIOParallelTestTask() {}

void PEIOParallelTestTask::AddFileName(const ALString& sName)
{
	require(FileService::FileExists(sName));

	// Ajout au tableau des variables partagees
	svFileName.Add(sName);
}

void PEIOParallelTestTask::ComputeFilesStats()
{
	Run();
}

longint PEIOParallelTestTask::GetTotalFieldNumber() const
{
	require(IsJobDone());
	return lTotalFieldNumber;
}

longint PEIOParallelTestTask::GetTotalLineNumber() const
{
	require(IsJobDone());
	return lTotalLineNumber;
}

void PEIOParallelTestTask::TestCountTotalLines(int nSlaveNumber, const StringVector* svFilesName)
{
	PEIOParallelTestTask task;
	int i;
	ALString sTmp;

	require(nSlaveNumber >= 0);
	require(svFilesName != NULL);
	require(svFilesName->GetSize() > 0);

	// Parametrage de la tache
	for (i = 0; i < svFilesName->GetSize(); i++)
		task.AddFileName(svFilesName->GetAt(i));

	// Gestion des tache, avec deux barres d'avancement dans le cas sequentiel
	// (maitre, et esclave)
	if (not PLParallelTask::GetDriver()->IsParallelModeAvailable())
		TaskProgression::SetDisplayedLevelNumber(2);
	else
		TaskProgression::SetDisplayedLevelNumber(1);

	// Utilisation du nombre maximum de coeurs : coeurs  du systeme -1
	RMResourceConstraints::SetMaxCoreNumberOnCluster(nSlaveNumber + 1);

	// Lancement de la tache avec gestion de l'avancement
	TaskProgression::SetTitle("Parallel IO test");
	TaskProgression::Start();
	task.ComputeFilesStats();
	TaskProgression::Stop();

	// Affichage du resultat
	Global::AddSimpleMessage(sTmp + "Found " + LongintToReadableString(task.GetTotalFieldNumber()) + " fields, " +
				 +LongintToReadableString(task.GetTotalLineNumber()) + " lines in files");
}

void PEIOParallelTestTask::Test()
{
	const int nFileNumber = 10;
	const int nLineNumber = 10000;
	const int nFieldNumber = 10;

	const ALString sFilePrefix = "TestFile";
	boolean bCreateFiles = true;
	int nSlaveNumber;
	ALString sFileName;
	fstream fst;
	StringVector svFileNames;
	int nFile;
	int nLine;
	int nField;

	// Creation de fichiers
	for (nFile = 0; nFile < nFileNumber; nFile++)
	{
		sFileName =
		    FileService::BuildFilePathName(FileService::GetTmpDir(), sFilePrefix + IntToString(nFile) + ".txt");

		svFileNames.Add(sFileName);
		if (bCreateFiles)
		{
			FileService::OpenOutputFile(sFileName, fst);
			for (nLine = 0; nLine < nLineNumber; nLine++)
			{
				fst << "Line " << nLine + 1 << ", ";
				for (nField = 0; nField < nFieldNumber - 1; nField++)
					fst << "\t" << nField + 1;
				fst << "\n";
			}
			fst.close();
		}
	}

	// Appel de la tache de comptage des lignes
	nSlaveNumber = RMResourceManager::GetLogicalProcessNumber() - 1;
	TestCountTotalLines(nSlaveNumber, &svFileNames);

	// Destruction des fichiers
	for (nFile = 0; nFile < nFileNumber; nFile++)
	{
		sFileName = svFileNames.GetAt(nFile);
		if (bCreateFiles)
			FileService::RemoveFile(sFileName);
	}

	char c;
	cout << "Break..." << endl;
	cin >> c;
}

const ALString PEIOParallelTestTask::GetTaskName() const
{
	return "IO parallel test task";
}

PLParallelTask* PEIOParallelTestTask::Create() const
{
	return new PEIOParallelTestTask;
}

boolean PEIOParallelTestTask::ComputeResourceRequirements()
{
	// Exigences de la tache
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->Set(BufferedFile::nDefaultBufferSize);
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->SetMax(0);
	GetResourceRequirements()->SetDiskAllocationPolicy(RMTaskResourceRequirement::masterPreferred);
	return true;
}

boolean PEIOParallelTestTask::MasterInitialize()
{
	int i;

	// On commnce par le premier fichier de la liste
	nFileIndex = 0;

	// Resultats de la tache
	lTotalFieldNumber = 0;
	lTotalLineNumber = 0;

	// Calcul de la taille de l'ensemble des  fichiers (pour la progression)
	lTotalFileSizes = 0;
	for (i = 0; i < svFileName.GetSize(); i++)
		lTotalFileSizes += FileService::GetFileSize(svFileName.GetAt(i));
	return true;
}

boolean PEIOParallelTestTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	ALString sFileName;

	// Est-ce qu'il y a encore du travail ?
	if (nFileIndex >= svFileName.GetSize())
		bIsTaskFinished = true;
	else
	{
		// On specifie le prochain fichier a traiter
		sFileName = svFileName.GetAt(nFileIndex);
		input_sFileToProcess.SetValue(sFileName);
		nFileIndex++;

		// Calcul de la progression representee par le traitement de ce fichier
		dTaskPercent = FileService::GetFileSize(sFileName) * 1.0 / lTotalFileSizes;
		assert(dTaskPercent <= 1);
	}
	return true;
}

boolean PEIOParallelTestTask::MasterAggregateResults()
{
	// Ajout des statistiques traitees par l'esclave
	lTotalFieldNumber += output_lFieldNumber;
	lTotalLineNumber += output_lLineNumber;

	return true;
}

boolean PEIOParallelTestTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	return true;
}

boolean PEIOParallelTestTask::SlaveInitialize()
{
	return true;
}

boolean PEIOParallelTestTask::SlaveProcess()
{
	InputBufferedFile* inputFile;
	char* sField;
	int nFieldLength;
	int nFieldError;
	boolean bEol;
	ALString sTmp;
	boolean bOk;
	longint lBeginChunk;
	longint lEndChunk;
	longint lFilePos;
	longint lMaxEndPos;
	boolean bLineTooLong;
	longint lNextLinePos;
	longint lLineNumberUsingCurrentBuffer;

	// Initialisation des resultats
	output_lFieldNumber = 0;
	output_lLineNumber = 0;

	// Message de debut
	AddMessage("Processing file " + input_sFileToProcess.GetValue() + " in task " + IntToString(GetTaskIndex()));

	// Traitement du fichier a analyser, au moyen d'un fichier bufferise
	inputFile = new InputBufferedFile;
	check(inputFile);
	inputFile->SetFileName(input_sFileToProcess.GetValue());

	// Ouverture du fichier a traiter
	bOk = inputFile->Open();
	if (not bOk)
	{
		AddError("Unable to open " + input_sFileToProcess.GetValue());
		return false;
	}
	else
	{
		lBeginChunk = 0;
		lLineNumberUsingCurrentBuffer = 0;
		lEndChunk = inputFile->GetBufferSize();

		// Parcours du fichier par chunk
		while (bOk and lBeginChunk < inputFile->GetFileSize())
		{
			// On commence l'analyse au debut du chunk
			lFilePos = lBeginChunk;

			// On la termine sur la derniere ligne commencant dans le chunk, donc dans le '\n' se trouve
			// potentiellement sur le debut de chunk suivant
			lMaxEndPos = min(lEndChunk + 1, inputFile->GetFileSize());

			// On commence par rechercher un debut de ligne, sauf si on est en debut de fichier
			if (lFilePos > 0)
			{
				bOk = inputFile->SearchNextLineUntil(lFilePos, lMaxEndPos, lNextLinePos);
				if (not bOk)
					break;

				// On se positionne sur le debut de la ligne suivante si elle est trouvee
				if (lNextLinePos != -1)
					lFilePos = lNextLinePos;
				// Si non trouvee, on se place en fin de chunk
				else
					lFilePos = lMaxEndPos;
			}

			// Analyse du chunk si on a y trouve un debut de ligne
			if (bOk and lFilePos < lMaxEndPos)
			{
				// Parcours du chunk par paquets de lignes
				while (bOk and lFilePos < lMaxEndPos)
				{
					// Remplissage du buffer avec des ligne
					bOk = inputFile->FillOuterLinesUntil(lFilePos, lMaxEndPos, bLineTooLong);
					if (not bOk)
						break;

					// Comptage des lignes du buffer si pas de ligne trop longue
					if (not bLineTooLong)
					{
						while (not inputFile->IsBufferEnd())
						{
							// Lecture du prochain champs
							bEol = inputFile->GetNextField(sField, nFieldLength,
										       nFieldError, bLineTooLong);

							// Memorisation des erreurs, en considerant qu'un tabulation
							// remplacee par un blanc n'est pas une erreur
							if (bLineTooLong)
							{
								// On saute les champ jusqu'a la fin de ligne
								if (not bEol)
								{
									inputFile->SkipLastFields(bLineTooLong);
									bEol = true;
								}
								break;
							}
							assert(nFieldError == InputBufferedFile::FieldNoError);
							output_lFieldNumber += 1;

							// Incrementation du nombre de lignes
							if (bEol)
								output_lLineNumber += 1;
						}
						lFilePos += inputFile->GetCurrentBufferSize();
						lLineNumberUsingCurrentBuffer += inputFile->GetBufferLineNumber();

						assert(inputFile->GetCurrentLineIndex() ==
							   inputFile->GetBufferLineNumber() - 1 or
						       inputFile->GetCurrentBufferSize() == 0);
						assert(output_lLineNumber == lLineNumberUsingCurrentBuffer);
					}
					// Sinon, on se positionne sur la fin de la ligne suivante ou la a derniere
					// position
					else
					{
						assert(bLineTooLong);
						assert(inputFile->GetCurrentBufferSize() == 0);
						assert(inputFile->GetPositionInFile() - lFilePos >
							   inputFile->GetMaxLineLength() or
						       inputFile->GetPositionInFile() == lMaxEndPos);
						output_lLineNumber++;
						lLineNumberUsingCurrentBuffer++;
						lFilePos = inputFile->GetPositionInFile();
					}
				}
			}

			// Passage au chunk suivant
			lBeginChunk += inputFile->GetBufferSize();
			lEndChunk += inputFile->GetBufferSize();
		}

		// Nettoyage
		inputFile->Close();
		delete inputFile;

		// Message de fin
		AddMessage("Processed file " + input_sFileToProcess.GetValue() + " in task " +
			   IntToString(GetTaskIndex()) + ": " + LongintToReadableString(output_lFieldNumber) +
			   " fields, " + LongintToReadableString(output_lLineNumber) + " lines");

		return bOk;
	}
}

boolean PEIOParallelTestTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	return true;
}
