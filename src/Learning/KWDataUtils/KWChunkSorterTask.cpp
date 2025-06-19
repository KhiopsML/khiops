// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWChunkSorterTask.h"
#include "TaskProgression.h"
#include "RMStandardResourceDriver.h"

KWChunkSorterTask::KWChunkSorterTask()
{
	bIsInputHeaderLineUsed = false;
	buckets = NULL;
	nCurrentBucketIndexToSort = 0;
	lSortedLinesNumber = 0;
	lLineNumber = 0;
	lKeySize = 0;
	lBucketSize = 0;

	// Variables en entree des taches
	DeclareTaskInput(&input_nBucketIndex);
	DeclareTaskInput(&input_svFileNames);
	DeclareTaskInput(&input_bSingleton);

	// Variables en sortie des taches
	DeclareTaskOutput(&output_nBucketIndex);
	DeclareTaskOutput(&output_sOutputFileName);
	DeclareTaskOutput(&output_nLinesSortedNumber);

	// Variables partagees
	DeclareSharedParameter(&shared_bHeaderLineUsed);
	DeclareSharedParameter(&shared_bOnlyOneBucket);
	DeclareSharedParameter(&shared_ivKeyFieldIndexes);
	DeclareSharedParameter(&shared_lBucketSize);
	DeclareSharedParameter(&shared_cInputSeparator);
	DeclareSharedParameter(&shared_cOutputSeparator);
	DeclareSharedParameter(&shared_bSameSeparator);

	shared_cInputSeparator.SetValue('\t');
	shared_cOutputSeparator.SetValue('\t');
}

KWChunkSorterTask::~KWChunkSorterTask() {}

void KWChunkSorterTask::SetInputHeaderLineUsed(boolean bValue)
{
	bIsInputHeaderLineUsed = bValue;
}

boolean KWChunkSorterTask::GetInputHeaderLineUsed() const
{
	return bIsInputHeaderLineUsed;
}

void KWChunkSorterTask::SetInputFieldSeparator(char cValue)
{
	shared_cInputSeparator.SetValue(cValue);
}

char KWChunkSorterTask::GetInputFieldSeparator() const
{
	return shared_cInputSeparator.GetValue();
}

IntVector* KWChunkSorterTask::GetKeyFieldIndexes()
{
	return shared_ivKeyFieldIndexes.GetIntVector();
}

const IntVector* KWChunkSorterTask::GetConstKeyFieldIndexes() const
{
	return shared_ivKeyFieldIndexes.GetConstIntVector();
}

void KWChunkSorterTask::SetOutputFieldSeparator(char cValue)
{
	shared_cOutputSeparator.SetValue(cValue);
}

char KWChunkSorterTask::GetOutputFieldSeparator() const
{
	return shared_cOutputSeparator.GetValue();
}

const StringVector* KWChunkSorterTask::GetSortedFiles() const
{
	return &svResultFileNames;
}

void KWChunkSorterTask::SetBuckets(KWSortBuckets* bucketsToSort)
{
	buckets = bucketsToSort;
}

KWSortBuckets* KWChunkSorterTask::GetBuckets()
{
	return buckets;
}

void KWChunkSorterTask::SetLineNumber(longint lValue)
{
	lLineNumber = lValue;
}

longint KWChunkSorterTask::GetLineNumber() const
{
	return lLineNumber;
}
void KWChunkSorterTask::SetKeySize(longint lValue)
{
	lKeySize = lValue;
}

longint KWChunkSorterTask::GetKeySize() const
{
	return lKeySize;
}

void KWChunkSorterTask::SetBucketSize(longint lValue)
{
	require(lValue > 0);
	lBucketSize = lValue;
}

longint KWChunkSorterTask::GetBucketSize() const
{
	return lBucketSize;
}

boolean KWChunkSorterTask::Sort()
{
	boolean bOk;
	KWSortBucket* sortedBucket;
	int i;

	require(shared_ivKeyFieldIndexes.GetSize() > 0);
	require(lLineNumber > 0 and lKeySize > 0);
	require(lBucketSize > 0);
	bOk = Run();

	// Nettoyage des fichiers de sortie en cas d'erreur
	if (not bOk)
	{
		for (i = 0; i < GetBuckets()->GetBucketNumber(); i++)
		{
			sortedBucket = GetBuckets()->GetBucketAt(i);
			if (sortedBucket->GetSorted() or sortedBucket->IsSingleton())
				PLRemoteFileService::RemoveFile(sortedBucket->GetOutputFileName());
		}
	}
	return bOk;
}

longint KWChunkSorterTask::GetSortedLinesNumber()
{
	require(IsJobDone());
	return lSortedLinesNumber;
}

void KWChunkSorterTask::Test()
{
	boolean bOk = true;
	KWArtificialDataset artificialDataset;
	longint lMeanKeySize;
	longint lLineNumber;
	ObjectArray oaKeys;
	KWSortBuckets sortBuckets;
	KWChunkSorterTask chunkSorter;

	// Gestion des taches
	TaskProgression::SetTitle("Test " + chunkSorter.GetTaskLabel());
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
	lMeanKeySize = 0;
	lLineNumber = 0;
	bOk = bOk and KWKeySizeEvaluatorTask::TestWithArtificialDataset(&artificialDataset, lMeanKeySize, lLineNumber);

	// Extraction des cles
	bOk = bOk and KWKeySampleExtractorTask::TestWithArtificialDataset(&artificialDataset, lMeanKeySize, 10, 20,
									  artificialDataset.GetLineNumber() / 10,
									  lLineNumber, &oaKeys);

	// Creation des buckets a partir des cle
	bOk = bOk and KWSortedChunkBuilderTask::TestWithArtificialDataset(&artificialDataset, &oaKeys, &sortBuckets);

	// Tri des chunks et concatenation
	bOk = bOk and KWChunkSorterTask::TestWithArtificialDataset(&artificialDataset, &sortBuckets);

	// Nettoyage
	oaKeys.DeleteAll();
	sortBuckets.DeleteBucketFiles();

	// Destruction du fichier
	artificialDataset.DeleteDataset();

	// Gestion des taches
	TaskProgression::Stop();
}

boolean KWChunkSorterTask::TestWithArtificialDataset(const KWArtificialDataset* artificialDataset,
						     KWSortBuckets* bucketsToSort)
{
	boolean bOk = true;
	KWChunkSorterTask chunkSorter;
	KWSortBucket mainBucket;
	ALString sOutputFileName;

	require(artificialDataset != NULL);
	require(bucketsToSort != NULL);
	require(bucketsToSort->GetBucketNumber() > 0);

	// Nom du fichier de sortie base sur le fichier d'entree
	sOutputFileName =
	    FileService::CreateTmpFile(FileService::GetFileName(artificialDataset->GetFileName()) + "_sort.txt", NULL);

	// Tri des chunks
	chunkSorter.SetInputHeaderLineUsed(artificialDataset->GetHeaderLineUsed());
	chunkSorter.SetInputFieldSeparator(artificialDataset->GetFieldSeparator());
	chunkSorter.GetKeyFieldIndexes()->CopyFrom(artificialDataset->GetConstKeyFieldIndexes());

	chunkSorter.SetOutputFieldSeparator(artificialDataset->GetFieldSeparator());
	chunkSorter.SetBuckets(bucketsToSort);
	bOk = chunkSorter.Sort();

	// Affichage des resultats
	KWArtificialDataset::DisplayFileFirstLines(chunkSorter.GetSortedFiles()->GetAt(0), 10);

	// Nettoyage
	PLFileConcatenater concatenater;
	concatenater.RemoveChunks(chunkSorter.GetSortedFiles());

	return bOk;
}

longint KWChunkSorterTask::ComputeSlaveMemoryRequirements(longint lBucketSize, longint lBucketLineNumber,
							  longint lKeySize)
{
	// ATTENTION si cette methode est modifiee, il faut modifier, il faut modifier
	// KWChunkSorterTask::ComputeMaxChunkSize (et vice versa)
	longint lKeyPairSize;

	// Taille d'une KeyPair
	lKeyPairSize = sizeof(KWKeyLinePair) + lKeySize;

	// Buffer d'entree, buffer de sortie, tableau de clefs
	// ( En theorie, en entree on a pas besoin de lBucketSize, mais seulement de la taille du plus gros chunk du
	// bucket. Or cette methode est utilisee dans la calcul de la taille des chunks dans
	// KWFileSorter::ComputeChunkSize et a ce moment on n'a pas la taille des fichiers qui constituent les buckets,
	// ils n'existent pas.)
	return lBucketSize + BufferedFile::nDefaultBufferSize + lBucketLineNumber * lKeyPairSize;
}

int KWChunkSorterTask::ComputeMaxChunkSize(longint lKeySize, longint lSlaveMemory, longint lFileLineNumber,
					   longint lFileSize)
{
	longint lKeyPairSize;
	longint lMaxChunkSize;

	// Taille d'une KeyPair
	lKeyPairSize = sizeof(KWKeyLinePair) + lKeySize;

	// Ce calcul est directement deduit de la methode ComputeSlaveMemoryRequirements ci-dessus. Elle donne la
	// memoire minimum necessaire pour construire des chunk d'une taille donnee. Ici c'est le contraire on donne la
	// taille maximale des chunks qu'on peut construire si les esclaves ont lSlaveMemory de memoire disponible.
	// ATTENTION si la methode KWChunkSorterTask::ComputeSlaveMemoryRequirements est modifiee,
	// il faut modifier celle-ci (et vice versa)
	lMaxChunkSize = (longint)((lSlaveMemory - BufferedFile::nDefaultBufferSize) /
				  (1 + (1.0 * lFileLineNumber * lKeyPairSize) / (lFileSize * 1.0)));
	if (lMaxChunkSize > INT_MAX)
		lMaxChunkSize = INT_MAX;
	return (int)lMaxChunkSize;
}

const ALString KWChunkSorterTask::GetTaskName() const
{
	return "Chunk sorter";
}

PLParallelTask* KWChunkSorterTask::Create() const
{
	return new KWChunkSorterTask;
}

boolean KWChunkSorterTask::ComputeResourceRequirements()
{
	boolean bOk = true;
	ALString sTmp;

	// Parametrage des besoins
	// Les esclaves n'utilisent pas de disque supplementaire car les chunks sont
	// detruit des qu'ils sont charges en memoire pour etre tries, puis ecrit
	// sur le disque
	if (GetVerbose())
	{
		AddMessage(sTmp + "Chunk line number " + LongintToReadableString(lLineNumber));
		AddMessage(sTmp + "Key Size " + LongintToHumanReadableString(lKeySize));
		AddMessage(sTmp + "Bucket Size " + LongintToHumanReadableString(lBucketSize));
	}
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->Set(
	    ComputeSlaveMemoryRequirements(lBucketSize, lLineNumber, lKeySize));

	// Nombre de SlaveProcess
	GetResourceRequirements()->SetMaxSlaveProcessNumber(buckets->GetBucketNumber());
	return bOk;
}

boolean KWChunkSorterTask::MasterInitialize()
{
	require(shared_ivKeyFieldIndexes.GetSize() != 0);

	// Initialisation
	svResultFileNames.SetSize(0);
	nCurrentBucketIndexToSort = 0;
	lSortedLinesNumber = 0;
	shared_bHeaderLineUsed = bIsInputHeaderLineUsed;
	shared_bOnlyOneBucket = buckets->GetBucketNumber() == 1;
	shared_lBucketSize = lBucketSize;
	shared_bSameSeparator = shared_cInputSeparator.GetValue() == shared_cOutputSeparator.GetValue();
	return true;
}

boolean KWChunkSorterTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	KWSortBucket* bucketToSort;
	int nTreatedBucketNumber;
	InputBufferedFile ib;

	// Recherche du prochain bucket a trier
	bucketToSort = NULL;
	nTreatedBucketNumber = 0;
	while (nCurrentBucketIndexToSort < buckets->GetBucketNumber())
	{
		bucketToSort = buckets->GetBucketAt(nCurrentBucketIndexToSort);
		nTreatedBucketNumber++;

		// On doit trier un fichier si il n'est pas un singleton.
		// De plus, les esclaves doivent egalement traiter les singletons quand les separateur sont differents.
		// Dans ce dernier cas, les esclaves ne vont pas trier mais seulement reecrire le fichier pour changer le separateur.
		if (not bucketToSort->IsSingleton() or not shared_bSameSeparator)
		{
			assert(bucketToSort->GetOutputFileName() == "");
			break;
		}
		else
		{
			// Comptage des lignes
			lSortedLinesNumber += bucketToSort->GetLineNumber();
		}
		nCurrentBucketIndexToSort++;
	}

	// Derniere tache ou non ?
	if (nCurrentBucketIndexToSort >= buckets->GetBucketNumber())
		bIsTaskFinished = true;
	else
	{
		// Memorisation de l'index du bucket traite par la tache et incrementation pour la suite
		input_nBucketIndex = nCurrentBucketIndexToSort;
		nCurrentBucketIndexToSort++;

		// Parametrage du prochain bucket a trier
		assert(bucketToSort != NULL);
		assert(not bucketToSort->GetSorted());
		input_svFileNames.GetStringVector()->CopyFrom(bucketToSort->GetChunkFileNames());
		input_bSingleton = bucketToSort->IsSingleton();

		// Calcul de l'avancement
		dTaskPercent = nTreatedBucketNumber * 1.0 / buckets->GetBucketNumber();
	}
	return true;
}

boolean KWChunkSorterTask::MasterAggregateResults()
{
	KWSortBucket* bucket;

	// Recherche du bucket venant d'etre trie
	bucket = buckets->GetBucketAt(output_nBucketIndex);

	// Mise a jour du bucket
	bucket->SetOutputFileName(output_sOutputFileName.GetValue());
	bucket->SetSorted();

	// Mise a jour du nombre de lignes triees
	lSortedLinesNumber += output_nLinesSortedNumber;
	return true;
}

boolean KWChunkSorterTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;
	int i;
	int j;
	KWSortBucket* bucket;
	PLFileConcatenater concatenater;

	// Construction de la liste des fichiers a concatener (et detruire ensuite)
	bOk = bProcessEndedCorrectly;
	for (i = 0; i < buckets->GetBucketNumber(); i++)
	{
		bucket = buckets->GetBucketAt(i);

		// Collecte du nom de fichier trie a concatener (puis detruire)
		if (bucket->GetSorted() or bucket->IsSingleton())
		{
			if (bucket->IsSingleton() and shared_bSameSeparator)
			{
				for (j = 0; j < bucket->GetChunkFileNames()->GetSize(); j++)
				{
					svResultFileNames.Add(bucket->GetChunkFileNames()->GetAt(j));
				}
			}
			else
				svResultFileNames.Add(bucket->GetOutputFileName());
		}
		// Sinon: c'est qu'il y a eu un arret utilisateur
		else
		{
			// Suppression des fichiers non tries (normalement, ils sont supprimes apres le tri dans le SlaveProcess, mais on suppose qu'ils n'ont pas ete supprimes puisqu'il y a une erreur)
			// (sauf dans le cas du tri InMemory, ou le chunk est le fichier d'entree)
			if (not shared_bOnlyOneBucket)
			{
				for (j = 0; j < bucket->GetChunkFileNames()->GetSize(); j++)
				{
					PLRemoteFileService::RemoveFile(bucket->GetChunkFileNames()->GetAt(j));
				}
			}
			bOk = false;
		}
	}

	if (not bOk)
	{
		// Suppression des fichiers tries
		concatenater.RemoveChunks(&svResultFileNames);
		svResultFileNames.SetSize(0);
	}

	return bOk;
}

boolean KWChunkSorterTask::SlaveInitialize()
{
	return true;
}

boolean KWChunkSorterTask::SlaveProcess()
{
	boolean bOk;
	ALString sOutputFileName;
	int nObjectNumer;
	KWKeyExtractor keysExtractor;
	ObjectArray oaKeyLines;
	KWKeyLinePair* keyLine;
	KWKey* key;
	longint lBeginPos;
	int nLineBeginPos;
	int nLineEndPos;
	boolean bLineTooLong;
	int i;
	CharVector cvLineToWrite;
	InputBufferedFile* inputFile;
	OutputBufferedFile* outputFile;
	MemoryInputBufferedFile memoryFile;
	ObjectArray oaBufferedFiles;
	double dTotalProgression;
	double dLocalProgression;
	ALString sInputFileName;
	boolean bSkipFirstLine;
	ALString sFileURI;
	PLParallelTask* errorSender;
	longint lOneBucketFileSize;
	longint lCumulatedFileSize;
	boolean bIsLineOK;
	ALString sTmp;
	boolean bEndOfLine;
	char* sField;
	int nFieldLength;
	int nFieldError;
	boolean bLastLine = false;
	int nMaxLineLength;
	ALString sLocalHugeBuffer;

	// Initialisations
	outputFile = NULL;
	nObjectNumer = 0;
	lOneBucketFileSize = 0;
	lCumulatedFileSize = 0;
	memoryFile.SetFieldSeparator(shared_cInputSeparator.GetValue());

	// Si il n'y a qu'un fichier le header est dans ce fichier, il faudra le supprimer
	// En effet, en cas d'un fichier trop petit, on le tri directement (et il garde son eventuelle ligne d'entete)
	// Sinon, les buckets fabriques lors des etapes precedentes n'ont plus de ligne d'entete
	bSkipFirstLine = shared_bHeaderLineUsed and shared_bOnlyOneBucket;

	// Si il n'y a qu'un bucket on emet les messages lors de l'extraction des clefs
	// Sinon, on a deja vu le fichier, on n'emet pas de message
	if (shared_bOnlyOneBucket)
		errorSender = this;
	else
		errorSender = NULL;

	// Construction du nom du chunk trie
	sOutputFileName =
	    FileService::CreateUniqueTmpFile(sTmp + "bucket_sorted_" + IntToString(GetTaskIndex()) + ".txt", this);
	bOk = not sOutputFileName.IsEmpty();
	if (bOk)
	{
		// Fichier a supprimer en cas d'echec du programme
		SlaveRegisterUniqueTmpFile(sOutputFileName);

		// Suivi de la tache
		TaskProgression::DisplayProgression(0);

		nObjectNumer = 0;

		// Lecture de chaque chunk : remplissage d'un inputBuffer par chunk et stockage de ces buffers dans un
		// tableau
		for (i = 0; i < input_svFileNames.GetConstStringVector()->GetSize(); i++)
		{
			// Construction et initialisation du buffer
			sFileURI = input_svFileNames.GetConstStringVector()->GetAt(i);
			inputFile = new InputBufferedFile;
			inputFile->SetFileName(sFileURI);
			inputFile->SetFieldSeparator(shared_cInputSeparator.GetValue());

			// Pas de gestion du BOM du fichier (interne), sauf dans le cas shared_bOnlyOneBucket (fichier
			// utilisateur)
			if (not shared_bOnlyOneBucket)
				inputFile->SetUTF8BomManagement(false);

			// Stockage du buffer
			oaBufferedFiles.Add(inputFile);

			// Ouverture du fichier
			bOk = inputFile->Open();
			if (not bOk)
				break;

			// Parametrage de la taille du buffer
			inputFile->SetBufferSize(InputBufferedFile::FitBufferSize(inputFile->GetFileSize()));
			assert(inputFile->GetFileSize() <= shared_lBucketSize);

			// Remplissage du buffer avec la totalite du fichier
			lBeginPos = 0;
			bOk = inputFile->FillBytes(lBeginPos);
			if (not bOk)
			{
				inputFile->Close();
				break;
			}

			// Prise en compte du nombre de lignes
			nObjectNumer += inputFile->GetBufferLineNumber();

			// Prise en compte de la taille du buffer courante, qui peut etre plus petite que la taille du
			// fichier si un BOM est saute
			lCumulatedFileSize += inputFile->GetCurrentBufferSize();

			// Tout le fichier doit tenir en memoire
			assert(inputFile->IsLastBuffer());

			// Suppression du fichier
			if (not shared_bOnlyOneBucket)
				PLRemoteFileService::RemoveFile(inputFile->GetFileName());
		}

		if (shared_bOnlyOneBucket and input_svFileNames.GetConstStringVector()->GetSize() > 0)
			lOneBucketFileSize =
			    PLRemoteFileService::GetFileSize(input_svFileNames.GetConstStringVector()->GetAt(0));
	}

	if (bOk)
	{
		// Initialisation de l'extracteur de clef a partir des index
		keysExtractor.SetKeyFieldIndexes(shared_ivKeyFieldIndexes.GetConstIntVector());

		// Extraction des clefs de chaque buffer
		dTotalProgression = 0;
		nMaxLineLength = 0;
		for (i = 0; i < oaBufferedFiles.GetSize(); i++)
		{
			inputFile = cast(InputBufferedFile*, oaBufferedFiles.GetAt(i));

			// Saut du Header (dans ce cas, il n'y a qu'un seul bucket, donc une seule tache)
			// Attention, on ne peut pas se baser sur la presence d'une ligne de header dans
			// le input file (cf. MasterInitialize)
			if (bSkipFirstLine and i == 0)
			{
				assert(GetTaskIndex() == 0);
				inputFile->SkipLine(bLineTooLong);
				nObjectNumer--;

				// Ne pas oublier de retirer la longueur de la ligne d'entete
				lCumulatedFileSize -= inputFile->GetPositionInBuffer();
			}

			// Extraction des cles et lignes en vue de leur tri
			// On garde les fichiers ouverts en lecture, car on a besoin de leur buffer
			keysExtractor.SetBufferedFile(inputFile);
			while (not inputFile->IsBufferEnd())
			{
				// Ajout d'une nouvel enregistrement dans la liste des lignes
				key = new KWKey;
				bIsLineOK = keysExtractor.ParseNextKey(key, errorSender);
				keysExtractor.ExtractLine(nLineBeginPos, nLineEndPos);
				if (bIsLineOK)
				{
					keyLine = new KWKeyLinePair;
					keyLine->SetKey(key);
					keyLine->SetInputBuffer(inputFile);
					oaKeyLines.Add(keyLine);
					keyLine->SetLinePosition(nLineBeginPos, nLineEndPos);
					nMaxLineLength = max(nMaxLineLength, nLineEndPos - nLineBeginPos + 1);
				}
				else
				{
					delete key;
					nObjectNumer--;

					// Ne pas oublier de retirer la longueur de la ligne trop longue
					lCumulatedFileSize -= (longint)nLineEndPos - nLineBeginPos;
				}

				// Gestion de l'avancement (entre 0 et 25 pour cette partie)
				if (inputFile->GetCurrentLineIndex() % 100 == 0)
				{
					dLocalProgression = inputFile->GetPositionInBuffer() /
							    (double)inputFile->GetCurrentBufferSize();

					TaskProgression::DisplayProgression(
					    int(25.0 *
						(dTotalProgression + dLocalProgression / oaBufferedFiles.GetSize())));
					if (TaskProgression::IsInterruptionRequested())
						break;
				}
			}

			// On donne le nombre de ligne lues pour affichage des warnings (dans le cas in memory)
			if (errorSender != NULL)
			{
				SetLocalLineNumber(inputFile->GetBufferLineNumber());
				errorSender = NULL;
			}
			dTotalProgression = i * 1.0 / oaBufferedFiles.GetSize();
		}

		// Tri des lignes extraites
		if (bOk and not TaskProgression::IsInterruptionRequested())
		{
			// Si c'est un singleton, on ne trie pas, on veut juste changer le separateur du fichier
			if (not input_bSingleton)
			{
				oaKeyLines.SetCompareFunction(KWKeyLinePairCompare);
				oaKeyLines.Sort();
			}
		}

		// Ecriture du resultat du tri
		if (bOk and not TaskProgression::IsInterruptionRequested())
		{
			outputFile = new OutputBufferedFile;
			outputFile->SetFileName(sOutputFileName);
			outputFile->SetFieldSeparator(shared_cOutputSeparator.GetValue());
			ensure(outputFile->GetBufferSize() > 0);
			assert(!shared_bOnlyOneBucket or lOneBucketFileSize > 0);

			// Ouverture
			bOk = outputFile->Open();
			if (bOk)
			{
				if (shared_bSameSeparator)
				{
					// On reserve la taille du fichier pour eviter la fragmentation du disque,
					// seulement dans le cas ou les separateurs sont identiques. Si ils sont
					// differents, le chanp ecrit peut etre plus petit que le chanp lu car on
					// supprime les caracteres '"'. Dans ce cas, on aura reserve trop de memoire et
					// le fichier sera corrompu
					outputFile->ReserveExtraSize(lCumulatedFileSize);
				}
				else
				{
					// Utilisation d'un MemoryInputBufferedFile pour transformer le champ si les
					// separateurs sont differents
					memoryFile.SetBufferSize(nMaxLineLength);
				}

				for (i = 0; i < oaKeyLines.GetSize(); i++)
				{
					bLastLine = false;

					// Extraction de la ligne a partir de ses offsets
					keyLine = cast(KWKeyLinePair*, oaKeyLines.GetAt(i));
					keyLine->GetLinePosition(nLineBeginPos, nLineEndPos);

					// Remplacement du separateur
					if (not shared_bSameSeparator)
					{
						// extraction du buffer
						assert(not bLastLine);
						keyLine->GetInputBuffer()->ExtractSubBuffer(nLineBeginPos, nLineEndPos,
											    &cvLineToWrite);
						memoryFile.ResetBuffer();
						memoryFile.FillBuffer(&cvLineToWrite);
						bEndOfLine = false;

						// Lecture et ecriture de chaque champ
						while (not bEndOfLine)
						{
							bEndOfLine = memoryFile.GetNextField(sField, nFieldLength,
											     nFieldError, bLineTooLong);
							ensure(bLineTooLong == false);

							// Recopie de sField dans un buffer intermediaire car sField est
							// le buffer de grande taille qui est unique (GetHugeBuffer) Ce
							// buffer est modifie dans WriteField ce qui produirait un bug
							sLocalHugeBuffer = sField;
							outputFile->WriteField(sLocalHugeBuffer);
							if (not bEndOfLine)
								outputFile->Write(shared_cOutputSeparator.GetValue());
						}
						outputFile->WriteEOL();
					}
					else
					{
						// Si il n'y a rien a changer, on ecrit directement, sans recopie
						bOk = outputFile->WriteSubPart(
						    keyLine->GetInputBuffer()->GetCache(),
						    keyLine->GetInputBuffer()->GetBufferStartInCache() + nLineBeginPos,
						    nLineEndPos - nLineBeginPos);

						// Cas particulier du InMemory : pour la derniere ligne, il faut peut
						// etre ajouter un EOL (on l'a fait de toute facon si les separateurs
						// sont differents)
						if (shared_bOnlyOneBucket and nLineEndPos == lOneBucketFileSize)
						{
							// Si la derniere ligne n'a pas le caractere fin de ligne, on le
							// rajoute
							if (keyLine->GetInputBuffer()->GetCache()->GetAt(
								keyLine->GetInputBuffer()->GetBufferStartInCache() +
								nLineEndPos - 1) != '\n')
								outputFile->WriteEOL();
						}
					}

					// Gestion de l'avancement (entre 75 et 100 pour cette partie)
					if (i % 100 == 0)
					{
						TaskProgression::DisplayProgression(
						    75 + int(i * 25.0 / oaKeyLines.GetSize()));
						if (TaskProgression::IsInterruptionRequested())
							break;
					}
				}
				bOk = outputFile->Close() and bOk;
			}
			delete outputFile;
		}
	}

	// Fermeture des fichiers ouverts en lecture
	for (i = 0; i < oaBufferedFiles.GetSize(); i++)
	{
		inputFile = cast(InputBufferedFile*, oaBufferedFiles.GetAt(i));
		if (inputFile->IsOpened())
			inputFile->Close();
	}

	// Nettoyage
	oaKeyLines.DeleteAll();
	oaBufferedFiles.DeleteAll();

	// Envoi des resultats
	output_nLinesSortedNumber = nObjectNumer;
	output_sOutputFileName.SetValue(FileService::BuildLocalURI(sOutputFileName));

	// Recopie en sortie de l'index du bucket, ce qui permet au maitre de retrouver son bucket
	output_nBucketIndex = input_nBucketIndex;

	// Dans le cas d'une interruption utilisateur, les resultats ne sont pas envoye au maitre
	// Il faut detruire le fichier
	if (TaskProgression::IsInterruptionRequested() or not bOk)
	{
		FileService::RemoveFile(sOutputFileName);
	}
	return bOk;
}

boolean KWChunkSorterTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	return true;
}

void KWChunkSorterTask::SkipField(CharVector* cvLineToWrite, char cOriginalSeparator, int& nPos)
{
	char c;

	require(nPos < cvLineToWrite->GetSize());

	c = cvLineToWrite->GetAt(nPos);

	// Cas normal : on cherche le prochain separateur
	if (c != '"')
	{
		while (c != cOriginalSeparator)
		{
			nPos++;
			if (nPos == cvLineToWrite->GetSize())
				return;
			c = cvLineToWrite->GetAt(nPos);
		}
		return;
	}
	// Le champ commence par un double quote
	else
	{
		// On cherche le prochain double quote = soit fin de champ soit double quote qui est double
		nPos++;
		while (nPos < cvLineToWrite->GetSize())
		{
			c = cvLineToWrite->GetAt(nPos);

			// Si nouveau double quote
			if (c == '"')
			{
				nPos++;
				if (nPos < cvLineToWrite->GetSize())
				{
					c = cvLineToWrite->GetAt(nPos);

					// Double quote interne double: on continue de chercher un double quote
					if (c == '"')
					{
						nPos++;
						continue;
					}
					// Sinon, ok si caractere separateur
					// Sinon, KO si on trouve un double quote isole au milieu du champ:
					//  on continue quand-meme a parser pour recuperer l'erreur
					else
					{
						while (c != cOriginalSeparator)
						{
							nPos++;
							if (nPos < cvLineToWrite->GetSize())
								c = cvLineToWrite->GetAt(nPos);
						}
						return;
					}
				}
			}
			nPos++;
		}
	}
}

//////////////////////////////////////////////////
// Implementatoon de KWKeyLinePair

KWKeyLinePair::KWKeyLinePair()
{
	theKey = NULL;
	inputBuffer = NULL;
	nLineBeginPos = 0;
	nLineEndPos = 0;
}

KWKeyLinePair::~KWKeyLinePair()
{
	if (theKey != NULL)
		delete theKey;
}

int KWKeyLinePairCompare(const void* elem1, const void* elem2)
{
	int nCompareResult;
	require(elem1 != NULL);
	require(elem2 != NULL);

	KWKeyLinePair* k1;
	KWKeyLinePair* k2;

	// Acces aux objets
	k1 = cast(KWKeyLinePair*, *(Object**)elem1);
	k2 = cast(KWKeyLinePair*, *(Object**)elem2);

	require(k1->GetKey() != NULL);
	require(k2->GetKey() != NULL);

	nCompareResult = k1->GetKey()->Compare(k2->GetKey());

	// Si les clefs sont egales : tri suivant la position dans le fichier (le tri sera reproductible)
	if (nCompareResult == 0)
		nCompareResult = k1->nLineBeginPos - k2->nLineBeginPos;
	return nCompareResult;
}
