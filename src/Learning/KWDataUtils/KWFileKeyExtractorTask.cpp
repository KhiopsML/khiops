// Copyright (c) 2023-2025 Orange. All rights reserved.
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
	nReadSizeMin = 0;
	nReadSizeMax = 0;
	nWriteSizeMin = 0;
	nWriteSizeMax = 0;
	lEncodingErrorNumber = 0;

	// Parametres du programme
	DeclareSharedParameter(&shared_ivKeyFieldIndexes);
	DeclareSharedParameter(&shared_sInputFileName);
	DeclareSharedParameter(&shared_bInputHeaderLineUsed);
	DeclareSharedParameter(&shared_cInputFieldSeparator);
	DeclareSharedParameter(&shared_sOutputFileName);
	DeclareSharedParameter(&shared_cOutputFieldSeparator);

	// Input des esclaves : lecture du fichier d'entree
	DeclareTaskInput(&input_nBufferSize);
	DeclareTaskInput(&input_lFilePos);

	// Resultats envoyes par l'esclave
	DeclareTaskOutput(&output_lExtractedKeyNumber);
	DeclareTaskOutput(&output_sChunkFileName);
	DeclareTaskOutput(&output_nReadLineCount);
	DeclareTaskOutput(&output_lEncodingErrorNumber);
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
	bOk = InputBufferedFile::GetFirstLineFields(sInputFileName, cInputFieldSeparator, false, false, &svFirstLine);

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
		bOk = keySizeEvaluator.EvaluateKeySize(keyFieldsIndexer.GetConstKeyFieldIndexes(), sInputFileName,
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
		if (IsTaskInterruptedByUser() or keySizeEvaluator.IsTaskInterruptedByUser())
			AddWarning("Interrupted by user");
		else if (not bOk)
			AddError("Interrupted because of errors");

		// Message de fin
		if (bOk)
			AddSimpleMessage(sTmp + "Extracted keys: " + LongintToReadableString(GetExtractedKeyNumber()) +
					 " keys from " + LongintToReadableString(GetLineNumber()) +
					 " records (Extraction time: " + SecondsToString(GetJobElapsedTime()) + ")");

		// Message sur les eventuelles erreurs d'encodage
		if (bOk)
			InputBufferedFile::AddEncodingErrorMessage(lEncodingErrorNumber, this);
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

longint KWFileKeyExtractorTask::GetEncodingErrorNumber() const
{

	require(IsJobDone());
	return lEncodingErrorNumber;
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
	longint lInputPreferredSize;
	longint lOutputPreferredSize;

	// Exigences sur les ressources de la tache
	nReadSizeMin = PLRemoteFileService::GetPreferredBufferSize(sInputFileName);
	nWriteSizeMin = PLRemoteFileService::GetPreferredBufferSize(FileService::GetTmpDir());

	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMin(nReadSizeMin + (longint)nWriteSizeMin);
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMax(SystemFile::nMaxPreferredBufferSize +
									      (longint)nWriteSizeMin);
	assert(GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->GetMax() < INT_MAX);

	// Espace utilise par les clefs sur l'ensemble des esclaves
	GetResourceRequirements()->GetGlobalSlaveRequirement()->GetDisk()->Set(lMeanKeySize * lEstimatedLineNumber);

	// Nombre max de slaveProcess
	lInputFileSize = PLRemoteFileService::GetFileSize(sInputFileName);
	GetResourceRequirements()->SetMaxSlaveProcessNumber((int)ceil(lInputFileSize * 1.0 / nReadSizeMin));

	// Concatenation du fichier dans le master, 1 buffer d'ecriture et 1 buffer de lecture
	// Au minimum 1Mo pour chaque buffer
	// Au maximum 8 * preferred Size pour la lectur et preferred Size pour l'ecriture
	lInputPreferredSize = PLRemoteFileService::GetPreferredBufferSize(FileService::GetTmpDir());
	lOutputPreferredSize = PLRemoteFileService::GetPreferredBufferSize(sOutputFileName);
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->SetMin(2 * lMB);
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->SetMax(8 * lInputPreferredSize +
									       lOutputPreferredSize);
	return bOk;
}

boolean KWFileKeyExtractorTask::MasterInitialize()
{
	boolean bOk = true;
	StringVector svFirstLine;
	ALString sTmp;

	// Initialisation des variables aggregees
	lExtractedKeyNumber = 0;
	lReadLineNumber = 0;
	lEncodingErrorNumber = 0;

	// Initialisation du fichier en entree
	shared_sInputFileName.SetValue(sInputFileName);
	shared_bInputHeaderLineUsed = bInputHeaderLineUsed;
	shared_cInputFieldSeparator.SetValue(cInputFieldSeparator);

	// Parametrage du fichier en sortie
	shared_sOutputFileName.SetValue(sOutputFileName);
	shared_cOutputFieldSeparator.SetValue(cOutputFieldSeparator);

	// Parametrage du fichier en entree (pour les messages d'erreur)
	shared_sInputFileName.SetValue(sInputFileName);

	// Initialisation du tableau partage des index
	if (bOk)
	{
		shared_ivKeyFieldIndexes.GetIntVector()->CopyFrom(keyFieldsIndexer.GetConstKeyFieldIndexes());

		// La taille du fichier d'entree est necessaire pour estimer la progression
		// Elle doit avoir ete initialisee dans le ComputeResourceRequirements
		assert(lInputFileSize == PLRemoteFileService::GetFileSize(sInputFileName));
	}
	lFilePos = 0;

	//	Calcule de la taille maximale du buffer de lecture
	if (GetTaskResourceGrant()->GetSlaveMemory() > nReadSizeMin + (longint)nWriteSizeMin)
	{
		assert(GetTaskResourceGrant()->GetSlaveMemory() <= INT_MAX);
		nReadSizeMax = int(GetTaskResourceGrant()->GetSlaveMemory() - nWriteSizeMin);

		// Chaque esclave doit lire au moins 5 buffers (pour que le travail soit bien reparti entre les
		// esclaves)
		if (lInputFileSize / (GetProcessNumber() * (longint)5) < nReadSizeMax)
		{
			nReadSizeMax =
			    InputBufferedFile::FitBufferSize(lInputFileSize / (GetProcessNumber() * (longint)5));
			nReadSizeMax = max(nReadSizeMax, nReadSizeMin);
		}
	}
	else
	{
		nReadSizeMax = nReadSizeMin;
	}
	if (GetVerbose())
	{
		AddMessage(sTmp + "read buffer size min " + LongintToHumanReadableString(nReadSizeMin));
		AddMessage(sTmp + "read buffer size max " + LongintToHumanReadableString(nReadSizeMax));
	}

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
		input_nBufferSize = ComputeStairBufferSize(nReadSizeMin, nReadSizeMax,
							   PLRemoteFileService::GetPreferredBufferSize(sInputFileName),
							   lFilePos, lInputFileSize);
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

	// Mise a jout du nombre de lignes
	lReadLineNumber += output_nReadLineCount;

	// Mise a jour du nombre d'erreurs d'encodage
	lEncodingErrorNumber += output_lEncodingErrorNumber;
	return true;
}

boolean KWFileKeyExtractorTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;
	ALString sKeyName;
	PLFileConcatenater concatenater;
	int nBufferToConcatenate;
	StringVector svHeader;

	// Concatenation des fichiers
	bOk = bProcessEndedCorrectly;
	if (bProcessEndedCorrectly and not TaskProgression::IsInterruptionRequested())
	{
		// Suppression du fichier de sortie
		if (PLRemoteFileService::FileExists(sOutputFileName))
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

		if (bOutputHeaderLineUsed)
			svHeader.CopyFrom(GetKeyAttributeNames());
		bOk = ConcatenateFilesWithoutDuplicateKeys(&svChunkFileNames, &svHeader);
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
	if (shared_bInputHeaderLineUsed)
		lReadLineNumber--;
	return bOk;
}

boolean KWFileKeyExtractorTask::SlaveInitialize()
{
	boolean bOk = true;

	require(shared_ivKeyFieldIndexes.GetSize() > 0);

	// Recopie du nom de la base (pour les messages d'erreur)
	sInputFileName = shared_sInputFileName.GetValue();

	// Parametrage de l'esclave par les index des cle a extraire
	keyExtractor.SetKeyFieldIndexes(shared_ivKeyFieldIndexes.GetConstIntVector());

	// Parametrage du fichier d'entree a analyser pour la tache
	inputFile.SetFileName(shared_sInputFileName.GetValue());
	inputFile.SetFieldSeparator(shared_cInputFieldSeparator.GetValue());
	inputFile.SetHeaderLineUsed(shared_bInputHeaderLineUsed);

	// Ouverture du fichier en entree
	bOk = inputFile.Open();
	return bOk;
}

boolean KWFileKeyExtractorTask::SlaveProcess()
{
	boolean bOk = true;
	longint lSlaveEncodingErrorNumber;
	KWKey* previousKey;
	KWKey* key;
	int nCompareKey;
	KWKey keyStore1;
	KWKey keyStore2;
	OutputBufferedFile outputFile;
	ALString sChunkFileName;
	int i;
	longint lDisplayFreshness;
	double dProgression;
	boolean bCloseOk;
	longint lBeginPos;
	longint lMaxEndPos;
	longint lNextLinePos;
	boolean bLineTooLong;
	int nCumulatedLineNumber;
	boolean bIsLineOK;
	ALString sTmp;
	ALString sObjectLabel;
	ALString sOtherObjectLabel;

	// Fraicheur d'affichage pour la gestion de la barre de progression
	lDisplayFreshness = 0;

	// Memorisation du nombre d'erreurs d'encodage initiales
	lSlaveEncodingErrorNumber = inputFile.GetEncodingErrorNumber();

	// Initialisation du resultat
	output_lExtractedKeyNumber = 0;

	// Specification de la portion du fichier a traiter
	// On la termine sur la derniere ligne commencant dans le chunk, donc dans le '\n' se trouve potentiellement
	// sur le debut de chunk suivant, un octet au dela de la fin
	lBeginPos = input_lFilePos;
	lMaxEndPos = min(input_lFilePos + input_nBufferSize + 1, inputFile.GetFileSize());

	// Parametrage de la taille du buffer
	inputFile.SetBufferSize(input_nBufferSize);

	// On commence par se caller sur un debut de ligne, sauf si on est en debut de fichier
	// On ne compte pas la ligne sautee en debut de chunk, car elle est traitee en fin du chunk precedent
	nCumulatedLineNumber = 0;
	if (lBeginPos > 0)
	{
		bOk = inputFile.SearchNextLineUntil(lBeginPos, lMaxEndPos, lNextLinePos);
		if (bOk)
		{
			// On se positionne sur le debut de la ligne suivante si elle est trouvee
			if (lNextLinePos != -1)
				lBeginPos = lNextLinePos;
			// Si non trouvee, on se place en fin de chunk
			else
				lBeginPos = lMaxEndPos;
		}
	}

	if (bOk)
	{
		// Initialisation du buffer de sortie
		outputFile.SetFieldSeparator(shared_cOutputFieldSeparator.GetValue());

		// Construction d'un nom de chunk specifique a l'esclave
		sChunkFileName = FileService::CreateUniqueTmpFile(
		    FileService::GetFilePrefix(shared_sOutputFileName.GetValue()) + "_" + IntToString(GetTaskIndex()),
		    this);
		bOk = not sChunkFileName.IsEmpty();
	}
	if (bOk)
	{
		output_sChunkFileName.SetValue(FileService::BuildLocalURI(sChunkFileName));
		outputFile.SetFileName(sChunkFileName);
		bOk = outputFile.Open();
	}

	// Initialisation des variables de travail
	keyExtractor.SetBufferedFile(&inputFile);
	nCompareKey = -1;
	previousKey = NULL;
	key = &keyStore1;

	// Remplissage du buffer avec des lignes entieres dans la limite de la taille du buffer
	// On reitere tant que l'on a pas atteint la derniere position pour lire toutes les ligne, y compris la derniere
	while (bOk and lBeginPos < lMaxEndPos)
	{
		bOk = inputFile.FillOuterLinesUntil(lBeginPos, lMaxEndPos, bLineTooLong);
		if (not bOk)
			break;

		// Cas d'un ligne trop longue: on se deplace a la ligne suivante ou a la fin de la portion a traite
		if (bLineTooLong)
		{
			lBeginPos = inputFile.GetPositionInFile();

			// Ajout d'un ligne si elle termine dans la portion traitee
			if (lBeginPos < lMaxEndPos)
				nCumulatedLineNumber++;
		}
		// Cas general
		else
		{
			// Saut du Header
			if (inputFile.GetHeaderLineUsed() and inputFile.IsFirstPositionInFile())
			{
				inputFile.SkipLine(bLineTooLong);
				if (bLineTooLong)
				{
					AddLocalError("Header line, " + InputBufferedFile::GetLineTooLongErrorLabel(),
						      nCumulatedLineNumber + inputFile.GetCurrentLineIndex());
					bOk = false;
				}
			}

			// Parcours du buffer d'entree
			while (bOk and not inputFile.IsBufferEnd())
			{
				// Gestion de la progresssion
				lDisplayFreshness++;
				if (TaskProgression::IsRefreshNecessary(lDisplayFreshness))
				{
					// Calcul de la progression par rapport a la proportion de la portion du fichier
					// traitee parce que l'on ne sait pas le nombre total de ligne que l'on va
					// traiter
					dProgression = (inputFile.GetPositionInFile() - input_lFilePos) * 1.0 /
						       (lMaxEndPos - input_lFilePos);
					TaskProgression::DisplayProgression((int)floor(dProgression * 100));
					if (TaskProgression::IsInterruptionRequested())
					{
						bOk = false;
						break;
					}
				}

				// Extraction et comparaison de la cle
				bIsLineOK = keyExtractor.ParseNextKey(key, this);
				if (bIsLineOK)
				{
					nCompareKey = 0;
					if (previousKey != NULL)
						nCompareKey = previousKey->Compare(key);

					// Si la clef extraite est plus petite que la precedente => erreur
					if (nCompareKey > 0)
					{
						// Creation de libelles utilisateurs distincts pour les deux cles
						key->BuildDistinctObjectLabels(previousKey, sObjectLabel,
									       sOtherObjectLabel);
						AddLocalError(sTmp + "Unsorted record with key " + sObjectLabel +
								  " inferior to previous key " + sOtherObjectLabel,
							      nCumulatedLineNumber + inputFile.GetCurrentLineIndex());
						bOk = false;
						break;
					}

					// Ecriture de la clef dans le fichier si elle est superieure a la precedente
					if (previousKey == NULL or nCompareKey < 0)
					{
						output_lExtractedKeyNumber++;
						for (i = 0; i < key->GetSize(); i++)
						{
							if (i > 0)
								outputFile.Write(outputFile.GetFieldSeparator());
							outputFile.Write(key->GetAt(i));
						}
						outputFile.WriteEOL();
					}

					// Memorisation de la cle precedente
					previousKey = key;

					// Utilisation de l'autre "cle de stockage" pour la cle courante
					// (optimisation de la memoire pour eviter les recopies entre cle et cle
					// precedente)
					if (key == &keyStore1)
						key = &keyStore2;
					else
						key = &keyStore1;
				}
			}

			// On se deplace de la taille du buffer analyse
			lBeginPos += inputFile.GetCurrentBufferSize();
			nCumulatedLineNumber += inputFile.GetBufferLineNumber();
		}
	}

	// Fermeture et destruction du buffer de sortie
	if (outputFile.IsOpened())
	{
		bCloseOk = outputFile.Close();
		if (not bCloseOk)
		{
			bOk = false;
			AddError("Cannot close file " + outputFile.GetFileName());
		}
	}

	// On ajoute un test sur l'interruption car pour les petits fichiers IsRefreshNecessary n'est jamais declenchee dans
	// la boucle precedente et l'interruption n'est pas detectee, il faut neanmoins detruire les fichiers
	// temporaires
	if (TaskProgression::IsInterruptionRequested())
		bOk = false;

	if (sChunkFileName != "" and not bOk)
		FileService::RemoveFile(sChunkFileName);

	// Nombre d'erreurs d'encodage detectees dans la methode, par difference avec le nombre d'erreurs initiales
	lSlaveEncodingErrorNumber = inputFile.GetEncodingErrorNumber() - lSlaveEncodingErrorNumber;
	output_lEncodingErrorNumber = lSlaveEncodingErrorNumber;

	if (bOk)
	{
		output_nReadLineCount = nCumulatedLineNumber;
		SetLocalLineNumber(nCumulatedLineNumber);
	}
	return bOk;
}

boolean KWFileKeyExtractorTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;

	// Nettoyage
	keyExtractor.Clean();

	// Fermeture du fichier
	if (inputFile.IsOpened())
		bOk = inputFile.Close();
	return bOk;
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

boolean KWFileKeyExtractorTask::ConcatenateFilesWithoutDuplicateKeys(StringVector* svFileURIs, StringVector* svHeader)
{
	boolean bOk;
	boolean bDisplay = false;
	int nChunk;
	InputBufferedFile chunkFile;
	OutputBufferedFile outputFile;
	ALString sChunkFileURI;
	longint lYetToRead;
	int i;
	longint lPos;
	CharVector cvFirstLine;
	CharVector cvLastLine;
	boolean bLineTooLong;
	boolean bFirstChunk;
	boolean bFirstBuffer;
	longint lMaxEndPos;
	longint lInputBufferSize;
	longint lOutputBufferSize;
	longint lRemainingMemory;
	longint lInputPreferredSize;
	longint lOutputPreferredSize;

	require(svFileURIs != NULL);
	require(svFileURIs->GetSize() > 0);

	// Memoire disponible sur la machine
	lRemainingMemory = RMResourceManager::GetRemainingAvailableMemory();
	lInputPreferredSize = PLRemoteFileService::GetPreferredBufferSize(svFileURIs->GetAt(0));
	lOutputPreferredSize = PLRemoteFileService::GetPreferredBufferSize(sOutputFileName);

	if (lRemainingMemory >= lInputPreferredSize + lOutputPreferredSize)
	{
		lOutputBufferSize = lOutputPreferredSize;
		lInputBufferSize = lRemainingMemory - lOutputBufferSize;

		// Ajustement de la  taille a un multiple de preferred size
		if (lInputBufferSize > lInputPreferredSize)
			lInputBufferSize = (lInputBufferSize / lInputPreferredSize) * lInputPreferredSize;

		// On n'utilise pas plus de 8 preferred size
		if (lInputBufferSize > 8 * lOutputPreferredSize)
			lInputBufferSize = 8 * lOutputPreferredSize;
	}
	else
	{
		// On fait au mieux avec les ressources disponibles
		lInputBufferSize = lRemainingMemory / 2;
		lOutputBufferSize = lRemainingMemory - lInputBufferSize;
	}

	if (bDisplay)
	{
		cout << "Available memory on the host " << LongintToHumanReadableString(lRemainingMemory) << endl;
		cout << "Dispatched for input " << LongintToHumanReadableString(lInputBufferSize) << " and output "
		     << LongintToHumanReadableString(lOutputBufferSize) << endl;
	}

	// Ajustement a des tailles raisonables
	lOutputBufferSize = InputBufferedFile::FitBufferSize(lOutputBufferSize);
	lInputBufferSize = InputBufferedFile::FitBufferSize(lInputBufferSize);

	outputFile.SetFileName(sOutputFileName);
	outputFile.SetBufferSize((int)lOutputBufferSize);
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
			if (PLRemoteFileService::FileExists(sChunkFileURI))
			{
				if (bDisplay)
					cout << "Read " << sChunkFileURI << " size "
					     << LongintToHumanReadableString(
						    PLRemoteFileService::GetFileSize(sChunkFileURI))
					     << endl;

				// Ouverture du chunk en lecture
				chunkFile.SetFileName(sChunkFileURI);
				bOk = chunkFile.Open();
				if (bOk)
				{
					chunkFile.SetBufferSize((int)min(lInputBufferSize, chunkFile.GetFileSize()));

					// Initialisation de la taille restant a lire avec la taille du chunk
					lYetToRead = chunkFile.GetFileSize();

					// Lecture du chunk par blocs et ecriture dans le fichier de sortie
					lPos = 0;
					bFirstBuffer = true;
					while (lYetToRead != 0)
					{
						lMaxEndPos = min(lPos + lInputBufferSize + 1, chunkFile.GetFileSize());

						// Lecture dans le fichier
						chunkFile.FillOuterLinesUntil(lPos, lMaxEndPos, bLineTooLong);
						assert(not bLineTooLong);
						lPos += chunkFile.GetCurrentBufferSize();
						if (chunkFile.IsError())
						{
							bOk = false;
							break;
						}

						lYetToRead -= chunkFile.GetCurrentBufferSize();
						assert(lYetToRead >= 0);

						// Extraction de la premiere ligne
						// A ce stade, les lignes trop longues doivent etre filtrees
						chunkFile.GetNextLine(&cvFirstLine, bLineTooLong);
						assert(not bLineTooLong);

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
						while (not chunkFile.IsBufferEnd())
						{
							chunkFile.GetNextLine(&cvLastLine, bLineTooLong);
							assert(not bLineTooLong);
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
					bOk = chunkFile.Close() and bOk;
					TaskProgression::DisplayProgression((nChunk + 1) * 100 / svFileURIs->GetSize());
				}
			}
			else
			{
				// TODO verifier si l'on peut factoriser un service dans FileService?
				if (RMResourceManager::GetResourceSystem()->GetHostNumber() > 1)
					FileService::SetURISmartLabels(false);
				AddError("Missing chunk file : " + FileService::GetURIUserLabel(sChunkFileURI));
				FileService::SetURISmartLabels(true);
				bOk = false;
				break;
			}
			bFirstChunk = false;
		}
		bOk = outputFile.Close() and bOk;
	}
	return bOk;
}
