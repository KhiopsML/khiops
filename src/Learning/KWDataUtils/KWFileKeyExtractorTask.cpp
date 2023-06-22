// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWFileKeyExtractorTask.h"

KWFileKeyExtractorTask::KWFileKeyExtractorTask()
{
	bInputHeaderLineUsed = true;
	cInputFieldSeparator = '\t';
	bOutputHeaderLineUsed = true;
	cOutputFieldSeparator = '\t';
	lInputFileSize = 0;
	lExtractedKeyNumber = 0;
	lReadLineNumber = 0;
	lEstimatedLineNumber = 0;
	lMeanKeySize = 0;
	lFilePos = 0;

	// Parametres du programme
	DeclareSharedParameter(&shared_ivKeyFieldIndexes);
	DeclareSharedParameter(&shared_outputFile);
	DeclareSharedParameter(&shared_sInputFileName);
	DeclareSharedParameter(&shared_bHeaderLineUsed);
	DeclareSharedParameter(&shared_cFieldSeparator);

	// Input des esclaves : lecture du fichier d'entree
	DeclareTaskInput(&input_nBufferSize);
	DeclareTaskInput(&input_lFilePos);

	// Resultats envoyes par l'esclave
	DeclareTaskOutput(&output_lExtractedKeyNumber);
	DeclareTaskOutput(&output_sChunkFileName);
	DeclareTaskOutput(&output_nReadLineCount);
}

KWFileKeyExtractorTask::~KWFileKeyExtractorTask() {}

StringVector* KWFileKeyExtractorTask::GetKeyAttributeNames()
{
	return keyFieldsIndexer.GetKeyAttributeNames();
}

StringVector* KWFileKeyExtractorTask::GetNativeFieldNames()
{
	return keyFieldsIndexer.GetNativeFieldNames();
}

void KWFileKeyExtractorTask::SetInputFileName(const ALString& sValue)
{
	sInputFileName = sValue;
}

const ALString& KWFileKeyExtractorTask::GetInputFileName() const
{
	return sInputFileName;
}

void KWFileKeyExtractorTask::SetInputHeaderLineUsed(boolean bValue)
{
	bInputHeaderLineUsed = bValue;
}

boolean KWFileKeyExtractorTask::GetInputHeaderLineUsed() const
{
	return bInputHeaderLineUsed;
}

void KWFileKeyExtractorTask::SetInputFieldSeparator(char cValue)
{
	cInputFieldSeparator = cValue;
}

char KWFileKeyExtractorTask::GetInputFieldSeparator() const
{
	return cInputFieldSeparator;
}

void KWFileKeyExtractorTask::SetOutputFileName(const ALString& sValue)
{
	sOutputFileName = sValue;
}

const ALString& KWFileKeyExtractorTask::GetOutputFileName() const
{
	return sOutputFileName;
}

void KWFileKeyExtractorTask::SetOutputHeaderLineUsed(boolean bValue)
{
	bOutputHeaderLineUsed = bValue;
}

boolean KWFileKeyExtractorTask::GetOutputHeaderLineUsed() const
{
	return bOutputHeaderLineUsed;
}

void KWFileKeyExtractorTask::SetOutputFieldSeparator(char cValue)
{
	cOutputFieldSeparator = cValue;
}

char KWFileKeyExtractorTask::GetOutputFieldSeparator() const
{
	return cOutputFieldSeparator;
}

boolean KWFileKeyExtractorTask::ExtractKeys(boolean bDisplayUserMessage)
{
	boolean bOk;
	ALString sTmp;
	KWKeySizeEvaluatorTask keySizeEvaluator;
	StringVector svFirstLine;
	longint lRemainingDisk;

	require(GetKeyAttributeNames()->GetSize() > 0);

	// Extraction des champs de  la premiere ligne du fichier d'entree
	bOk = InputBufferedFile::GetFirstLineFields(sInputFileName, cInputFieldSeparator, &svFirstLine, this);

	// Calcul des index des champs de la cle
	if (bOk)
	{
		bOk = keyFieldsIndexer.ComputeKeyFieldIndexes(bInputHeaderLineUsed, &svFirstLine);
		if (not bOk)
			AddError("Error while indexing fields");
	}
	if (bOk)
	{
		// Evaluation de la taille de clefs pour estimer les ressources necessaires
		keySizeEvaluator.EvaluateKeySize(keyFieldsIndexer.GetConstKeyFieldIndexes(), sInputFileName,
						 bInputHeaderLineUsed, cInputFieldSeparator, lMeanKeySize,
						 lEstimatedLineNumber);

		// On enleve l'espace necessaire a la structure objet (on abesoin de la tailel sur le fichier)
		lMeanKeySize = lMeanKeySize - sizeof(KWKey) - sizeof(KWKey*) - sizeof(StringVector);

		// Y a-t-il assez de place pour ecrire le fichier de sortie
		lRemainingDisk = PLRemoteFileService::GetDiskFreeSpace(FileService::GetPathName(sOutputFileName)) -
				 (lMeanKeySize + 1) * lEstimatedLineNumber;
		if (lRemainingDisk < 0)
		{
			bOk = false;
			AddError(sTmp + "There is not enough space available on the disk to write output file (" +
				 sOutputFileName + "): needs at least " +
				 LongintToHumanReadableString(-lRemainingDisk) + ")");
		}
	}

	// Lancement de la tache
	if (bOk)
		bOk = Run();

	// Messages utilisateur
	if (bDisplayUserMessage)
	{
		// Message d'erreur si necessaire
		if (IsTaskInterruptedByUser())
			AddWarning("Interrupted by user");
		else if (not bOk)
			AddError("Interrupted because of errors");

		// Message de fin
		if (bOk)
			AddSimpleMessage(sTmp + "Extracted keys: " + LongintToReadableString(GetExtractedKeyNumber()) +
					 " keys from " + LongintToReadableString(GetLineNumber()) +
					 " records (Extraction time: " + SecondsToString(GetJobElapsedTime()) + ")");
	}
	return bOk;
}

longint KWFileKeyExtractorTask::GetExtractedKeyNumber() const
{
	require(IsJobDone());
	return lExtractedKeyNumber;
}

longint KWFileKeyExtractorTask::GetLineNumber() const
{
	require(IsJobDone());
	return lReadLineNumber;
}

const ALString KWFileKeyExtractorTask::GetObjectLabel() const
{
	return sInputFileName;
}

void KWFileKeyExtractorTask::Test()
{
	TestArtificialData(10000, 10, true, true, true);
	TestArtificialData(10000, 10, false, true, true);
	TestArtificialData(10000, 10, true, false, false);
	TestArtificialData(100000, 10, true, false, false);
}

boolean KWFileKeyExtractorTask::TestArtificialData(int nInputLineNumber, int nInputFieldNumber,
						   boolean bInputKeyFieldsAscending, boolean bInputHeaderLine,
						   boolean bOutputHeaderLine)
{
	boolean bOk = true;
	KWArtificialDataset artificialDataset;
	KWFileKeyExtractorTask fileKeyExtractor;

	require(nInputLineNumber >= 0);
	require(nInputFieldNumber >= 3);

	// Gestion des taches
	TaskProgression::SetTitle("Test " + fileKeyExtractor.GetTaskLabel());
	TaskProgression::SetDisplayedLevelNumber(2);
	TaskProgression::Start();

	// Creation d'un fichier avec des champs cle
	artificialDataset.SpecifySortDataset();
	artificialDataset.SetLineNumber(nInputLineNumber);
	artificialDataset.SetFieldNumber(nInputFieldNumber);
	artificialDataset.SetAscendingSort(bInputKeyFieldsAscending);
	artificialDataset.SetHeaderLineUsed(bInputHeaderLine);
	artificialDataset.SetFileName(artificialDataset.BuildFileName());
	artificialDataset.CreateDataset();
	artificialDataset.DisplayFirstLines(15);

	// Extraction des cles
	bOk = bOk and KWFileKeyExtractorTask::TestWithArtificialDataset(&artificialDataset, bOutputHeaderLine,
									artificialDataset.GetFieldSeparator());

	// Destruction du fichier
	artificialDataset.DeleteDataset();

	// Gestion des taches
	TaskProgression::Stop();
	return bOk;
}

boolean KWFileKeyExtractorTask::TestWithArtificialDataset(const KWArtificialDataset* artificialDataset,
							  boolean bOutputHeaderLine, char cOutputFiedSeparator)
{
	boolean bOk = true;
	KWFileKeyExtractorTask dataTableKeyExtractor;
	ALString sKeyFileName;

	require(artificialDataset != NULL);

	// Calcul d'un nom de fichier de sortie
	sKeyFileName =
	    FileService::CreateUniqueTmpFile("Key_" + FileService::GetFileName(artificialDataset->GetFileName()), NULL);

	// Appel de la tache de comptage des lignes
	artificialDataset->ExportKeyAttributeNames(dataTableKeyExtractor.GetKeyAttributeNames());
	artificialDataset->ExportNativeFieldNames(dataTableKeyExtractor.GetNativeFieldNames());
	dataTableKeyExtractor.SetInputFileName(artificialDataset->GetFileName());
	dataTableKeyExtractor.SetInputHeaderLineUsed(artificialDataset->GetHeaderLineUsed());
	dataTableKeyExtractor.SetInputFieldSeparator(artificialDataset->GetFieldSeparator());
	dataTableKeyExtractor.SetOutputFileName(sKeyFileName);
	dataTableKeyExtractor.SetOutputHeaderLineUsed(bOutputHeaderLine);
	dataTableKeyExtractor.SetOutputFieldSeparator(cOutputFiedSeparator);
	bOk = bOk and dataTableKeyExtractor.ExtractKeys(false);

	// Second run pour tester la re-entrance
	bOk = bOk and dataTableKeyExtractor.ExtractKeys(false);

	// Affichage des resultats
	cout << "Articial test:\n " << artificialDataset->GetLineNumber() << " lines, "
	     << artificialDataset->GetFieldNumber() << " fields, "
	     << "key ascending=" << artificialDataset->GetAscendingSort()
	     << ", input header=" << artificialDataset->GetHeaderLineUsed() << ", output header=" << bOutputHeaderLine
	     << endl;
	cout << "Extraction done: " << bOk << endl;
	cout << "\tInput line number: " << dataTableKeyExtractor.GetLineNumber() << endl;
	if (bOk)
	{
		cout << "\tExtracted key number: " << dataTableKeyExtractor.GetExtractedKeyNumber() << endl;
		cout << "\tKey file size: " << PLRemoteFileService::GetFileSize(sKeyFileName) << endl;

		// Lecture des premieres lignes du fichier de cle
		KWArtificialDataset::DisplayFileFirstLines(sKeyFileName, 3);
	}

	// Destruction du fichier de cles
	FileService::RemoveFile(sKeyFileName);
	return bOk;
}

const ALString KWFileKeyExtractorTask::GetTaskName() const
{
	return "Key extractor";
}

PLParallelTask* KWFileKeyExtractorTask::Create() const
{
	return new KWFileKeyExtractorTask;
}

boolean KWFileKeyExtractorTask::ComputeResourceRequirements()
{
	boolean bOk = true;

	// Exigences sur les ressources de la tache
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->Set(
	    2 * BufferedFile::nDefaultBufferSize); // lecture + ecriture

	// Espace utilise par les clefs sur l'ensemble des esclaves
	GetResourceRequirements()->GetGlobalSlaveRequirement()->GetDisk()->Set(lMeanKeySize * lEstimatedLineNumber);

	// Nombre max de slaveProcess
	lInputFileSize = PLRemoteFileService::GetFileSize(sInputFileName);
	GetResourceRequirements()->SetMaxSlaveProcessNumber(
	    (int)ceil(lInputFileSize * 1.0 / BufferedFile::nDefaultBufferSize));
	return bOk;
}

boolean KWFileKeyExtractorTask::MasterInitialize()
{
	boolean bOk = true;
	StringVector svFirstLine;

	// Initialisation des variables aggregees
	lExtractedKeyNumber = 0;
	lReadLineNumber = 0;

	// Initialisation du fichier en entree
	shared_sInputFileName.SetValue(sInputFileName);
	shared_bHeaderLineUsed = bInputHeaderLineUsed;
	shared_cFieldSeparator.SetValue(cInputFieldSeparator);

	// Parametrage du fichier en sortie
	shared_outputFile.GetBufferedFile()->SetFileName(sOutputFileName);
	shared_outputFile.GetBufferedFile()->SetHeaderLineUsed(bOutputHeaderLineUsed);
	shared_outputFile.GetBufferedFile()->SetFieldSeparator(cOutputFieldSeparator);

	// Parametrage du fichier en entree (pour les messages d'erreur)
	shared_sInputFileName.SetValue(sInputFileName);

	// Initialisation du tableau partage des index
	if (bOk)
	{
		shared_ivKeyFieldIndexes.GetIntVector()->CopyFrom(keyFieldsIndexer.GetConstKeyFieldIndexes());

		// La taille du fichier d'entree est necessaire pour estimer la progression
		// Elle doit avoir ete initialisee dans le ComputeRequirements
		assert(lInputFileSize == PLRemoteFileService::GetFileSize(sInputFileName));
	}
	lFilePos = 0;
	return bOk;
}

boolean KWFileKeyExtractorTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	// Est-ce qu'il y a encore du travail ?
	if (lFilePos >= lInputFileSize)
		bIsTaskFinished = true;
	else
	{
		// Initialisation de l'input buffer
		input_nBufferSize =
		    ComputeStairBufferSize(lMB, BufferedFile::nDefaultBufferSize, lFilePos, lInputFileSize);
		input_lFilePos = lFilePos;
		lFilePos += input_nBufferSize;
		// Calcul de la progression
		dTaskPercent = (input_nBufferSize * 1.0) / (lInputFileSize + 1);
		if (dTaskPercent > 1)
			dTaskPercent = 1;

		// Augmentation de la taille du tableau des chunks
		svChunkFileNames.Add("");
	}
	return true;
}

boolean KWFileKeyExtractorTask::MasterAggregateResults()
{
	// Aggregation du nombre de clefs extraites
	lExtractedKeyNumber += output_lExtractedKeyNumber;

	// Mise a jour de la liste des chunks
	assert(svChunkFileNames.GetAt(GetTaskIndex()) == "");
	svChunkFileNames.SetAt(GetTaskIndex(), output_sChunkFileName.GetValue());

	lReadLineNumber += output_nReadLineCount;
	return true;
}

boolean KWFileKeyExtractorTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;
	ALString sKeyName;
	PLFileConcatenater concatenater;
	int nBufferToConcatenate;

	// Concatenation des fichiers
	bOk = bProcessEndedCorrectly;
	if (bProcessEndedCorrectly and not TaskProgression::IsInterruptionRequested())
	{
		// Suppression du fichier de sortie
		if (PLRemoteFileService::Exist(sOutputFileName))
			PLRemoteFileService::RemoveFile(sOutputFileName);

		// Concatenation des fichiers de sortie
		TaskProgression::DisplayLabel("Chunk concatenation");

		// Memoire disponible pour la concatenation
		nBufferToConcatenate =
		    InputBufferedFile::FitBufferSize(RMResourceManager::GetRemainingAvailableMemory());

		// On prend au moins 8 Mo
		if (nBufferToConcatenate < 8 * lMB)
			nBufferToConcatenate = 8 * lMB;

		// On ne prend pas plus de 1Go
		if (nBufferToConcatenate > lGB)
			nBufferToConcatenate = lGB;

		// On ne prend pas plus que la taille du fichier
		if (nBufferToConcatenate > lInputFileSize)
			nBufferToConcatenate = (int)lInputFileSize;

		bOk = ConcatenateFilesWithoutDuplicateKeys(&svChunkFileNames, nBufferToConcatenate,
							   GetKeyAttributeNames());
	}

	// Suppression des fichiers intermediaires
	TaskProgression::DisplayLabel("Cleaning");
	TaskProgression::DisplayProgression(0);
	concatenater.SetDisplayProgression(true);
	concatenater.RemoveChunks(&svChunkFileNames);

	// Suppression du fichier resultat si erreur
	if (not bOk)
		PLRemoteFileService::RemoveFile(sOutputFileName);

	// Nombre total de lignes lues par les esclaves
	if (shared_bHeaderLineUsed)
		lReadLineNumber--;
	return bOk;
}

boolean KWFileKeyExtractorTask::SlaveInitialize()
{
	require(shared_ivKeyFieldIndexes.GetSize() > 0);

	// Recopie du nom de la base (pour les messages d'erreur)
	sInputFileName = shared_sInputFileName.GetValue();

	// Parametrage de l'esclave par les index des cle a extraire
	keyExtractor.SetKeyFieldIndexes(shared_ivKeyFieldIndexes.GetConstIntVector());
	return true;
}

boolean KWFileKeyExtractorTask::SlaveProcess()
{
	KWKey currentKey;
	KWKey oldKey;
	InputBufferedFile inputBufferedFile;
	OutputBufferedFile* outputBufferedFile;
	ALString sChunkFileName;
	int i;
	double dProgression;
	boolean bOk;
	boolean bCloseOk;
	ALString sTmp;
	boolean bIsOpen;

	outputBufferedFile = NULL;

	// Initialisation du resultat
	output_lExtractedKeyNumber = 0;

	// Initialisation du buffer de lecture
	inputBufferedFile.SetFileName(shared_sInputFileName.GetValue());
	inputBufferedFile.SetFieldSeparator(shared_cFieldSeparator.GetValue());
	inputBufferedFile.SetHeaderLineUsed(shared_bHeaderLineUsed);
	inputBufferedFile.SetBufferSize(input_nBufferSize);

	// Lecture du fichier
	bIsOpen = inputBufferedFile.Open();
	if (bIsOpen)
		bOk = inputBufferedFile.Fill(input_lFilePos);
	else
		bOk = false;
	if (bOk)
	{
		// Creation et initialisation du buffer de sortie
		outputBufferedFile = new OutputBufferedFile;
		outputBufferedFile->CopyFrom(shared_outputFile.GetBufferedFile());

		// Construction d'un nom de chunk specifique a l'esclave
		sChunkFileName = FileService::CreateUniqueTmpFile(
		    FileService::GetFilePrefix(shared_outputFile.GetBufferedFile()->GetFileName()) + "_" +
			IntToString(GetTaskIndex()),
		    this);
		bOk = not sChunkFileName.IsEmpty();
	}
	if (bOk)
	{
		output_sChunkFileName.SetValue(FileService::BuildLocalURI(sChunkFileName));
		outputBufferedFile->SetFileName(sChunkFileName);
		bOk = outputBufferedFile->Open();
	}
	if (bOk)
	{
		// Saut de ligne si c'est le header
		if (inputBufferedFile.GetHeaderLineUsed() and input_lFilePos == (longint)0)
			inputBufferedFile.SkipLine();

		// Extraction des cles
		keyExtractor.SetBufferedFile(&inputBufferedFile);

		while (not inputBufferedFile.IsBufferEnd())
		{
			// Extraction de la clef
			keyExtractor.ParseNextKey(this);
			keyExtractor.ExtractKey(&currentKey);
			if (oldKey.GetSize() == 0 or currentKey.Compare(&oldKey) != 0)
			{
				if (oldKey.GetSize() != 0 and currentKey.Compare(&oldKey) < 0)
				{
					AddLocalError(sTmp + "key " + currentKey.GetObjectLabel() +
							  " inferior to previous key " + oldKey.GetObjectLabel(),
						      inputBufferedFile.GetCurrentLineNumber());
					bOk = false;
					break;
				}
				output_lExtractedKeyNumber++;

				// Ecriture de la clef dans le fichier
				for (i = 0; i < currentKey.GetSize(); i++)
				{
					if (i > 0)
						outputBufferedFile->Write(outputBufferedFile->GetFieldSeparator());
					outputBufferedFile->Write(currentKey.GetAt(i));
				}

				outputBufferedFile->WriteEOL();
			}
			oldKey.CopyFrom(&currentKey);

			// Gestion de la progesssion
			if (inputBufferedFile.GetCurrentLineNumber() % 100 == 0)
			{
				dProgression = 1.0 * inputBufferedFile.GetCurrentLineNumber() /
					       inputBufferedFile.GetBufferLineNumber();
				TaskProgression::DisplayProgression((int)floor(dProgression * 100));
				if (TaskProgression::IsInterruptionRequested())
				{
					bOk = false;
					break;
				}
			}
		}

		// Fermeture et destruction du buffer de sortie
		bCloseOk = outputBufferedFile->Close();
		if (not bCloseOk)
		{
			bOk = false;
			AddError("Cannot close file " + outputBufferedFile->GetFileName());
		}
	}
	if (sChunkFileName != "" and not bOk)
		FileService::RemoveFile(sChunkFileName);
	delete outputBufferedFile;
	output_nReadLineCount = inputBufferedFile.GetBufferLineNumber();
	SetLocalLineNumber(inputBufferedFile.GetBufferLineNumber());
	if (bIsOpen)
		inputBufferedFile.Close();
	return bOk;
}

boolean KWFileKeyExtractorTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	// Nettoyage
	keyExtractor.Clean();

	return true;
}

boolean CharVectorCompare(const CharVector* cv1, const CharVector* cv2)
{
	int i;
	if (cv1->GetSize() != cv2->GetSize())
		return false;

	for (i = 0; i < cv1->GetSize(); i++)
	{
		if (cv1->GetAt(i) != cv2->GetAt(i))
			return false;
	}

	return true;
}

boolean KWFileKeyExtractorTask::ConcatenateFilesWithoutDuplicateKeys(StringVector* svFileURIs, int nBufferSize,
								     StringVector* svHeader)
{
	boolean bOk;
	boolean bDisplay = false;
	int nChunk;
	InputBufferedFile inputFile;
	OutputBufferedFile outputFile;
	ALString sChunkFileURI;
	longint lYetToRead;
	int nReadSize;
	int i;
	longint lPos;
	CharVector cvFirstLine;
	CharVector cvLastLine;
	boolean bFirstChunk;
	boolean bFirstBuffer;

	require(svFileURIs != NULL);
	require(svFileURIs->GetSize() > 0);
	require(nBufferSize > 0);

	outputFile.SetFileName(sOutputFileName);
	bOk = outputFile.Open();
	if (bOk)
	{
		// Ecriture du header
		if (svHeader->GetSize() > 0)
		{
			for (i = 0; i < svHeader->GetSize(); i++)
			{
				if (i > 0)
					outputFile.Write(cOutputFieldSeparator);
				outputFile.Write(GetKeyAttributeNames()->GetAt(i));
			}
			outputFile.WriteEOL();
		}

		// Parcours de tous les chunks
		bFirstChunk = true;
		for (nChunk = 0; nChunk < svFileURIs->GetSize(); nChunk++)
		{
			sChunkFileURI = svFileURIs->GetAt(nChunk);
			if (TaskProgression::IsInterruptionRequested() or not bOk)
				break;

			// Concatenation d'un nouveau chunk
			if (PLRemoteFileService::Exist(sChunkFileURI))
			{
				if (bDisplay)
					cout << "Read " << sChunkFileURI << " size "
					     << LongintToHumanReadableString(
						    PLRemoteFileService::GetFileSize(sChunkFileURI))
					     << endl;
				// Ouverture du chunk en lecture
				inputFile.SetFileName(sChunkFileURI);
				bOk = inputFile.Open();
				if (bOk)
				{
					// Initialisation de la taille restant a lire avec la taille du chunk
					lYetToRead = inputFile.GetFileSize();

					// Lecture du chunk par blocs et ecriture dans le fichier de sortie
					lPos = 0;
					bFirstBuffer = true;
					while (lPos < inputFile.GetFileSize())
					{
						// Calcul de la taille a lire, de facon a ce que le premier et le
						// dernier block soit assez grand pour contenir des lignes
						if (lYetToRead <= inputFile.GetBufferSize() or
						    lYetToRead > 2 * inputFile.GetBufferSize())
							nReadSize = nBufferSize;
						else
							nReadSize = nBufferSize / 2;

						// On n'a pas besoin d'un buffer trop grand
						if (nReadSize > inputFile.GetFileSize())
							nReadSize = (int)inputFile.GetFileSize();

						inputFile.SetBufferSize(inputFile.FitBufferSize(nReadSize));
						if (bDisplay)
						{
							cout << "\t"
							     << "Buffer size " << LongintToReadableString(nReadSize)
							     << " => " << LongintToHumanReadableString(nReadSize)
							     << endl;
							cout << "\t"
							     << "Position " << LongintToReadableString(lPos) << endl;
						}

						// Lecture dans le fichier
						inputFile.Fill(lPos);
						lPos += inputFile.GetBufferSize();
						if (inputFile.IsError())
						{
							bOk = false;
							break;
						}

						lYetToRead -= inputFile.GetCurrentBufferSize();
						assert(lYetToRead >= 0);

						// Extraction de la premiere ligne
						inputFile.GetNextLine(&cvFirstLine);

						// On l'ecrit si elle est differente de la derniere ligne du chunk
						// precedent
						// TODO BG: CharVectorCompare doit renvoyer un entier
						// TODO BG: si <: recopie; si =; skip; si >: erreur et arret
						if (bFirstChunk or not bFirstBuffer or
						    CharVectorCompare(&cvFirstLine, &cvLastLine) == false)
							outputFile.Write(&cvFirstLine);
						else
							lExtractedKeyNumber--;

						// On recopie la premiere ligne dans la derniere ligne, au cas ou le
						// buffer ne contient qu'une seule ligne On doit la copier pour chaque
						// chunk, car un chunk peut ne contenir que sa premiere ligne
						cvLastLine.CopyFrom(&cvFirstLine);

						// Ecriture de la fin du chunk
						while (not inputFile.IsBufferEnd())
						{
							inputFile.GetNextLine(&cvLastLine);
							outputFile.Write(&cvLastLine);
							if (outputFile.IsError())
							{
								bOk = false;
								break;
							}
						}
						bFirstBuffer = false;
					}

					// Fermeture du chunk
					bOk = inputFile.Close() and bOk;
					TaskProgression::DisplayProgression((nChunk + 1) * 100 / svFileURIs->GetSize());
				}
			}
			else
			{
				AddError("Missing chunk file : " + PLRemoteFileService::URItoUserString(sChunkFileURI));
				bOk = false;
				break;
			}
			bFirstChunk = false;
		}
		bOk = outputFile.Close();
	}
	return bOk;
}
