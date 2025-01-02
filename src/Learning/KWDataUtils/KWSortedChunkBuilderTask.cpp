// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWSortedChunkBuilderTask.h"

KWSortedChunkBuilderTask::KWSortedChunkBuilderTask()
{
	bHeaderLineUsed = true;
	bLastSlaveProcessDone = false;
	lInputFileSize = 0;
	lFilePos = 0;
	lBucketsUsedMemory = 0;
	lBucketsSizeMin = 0;
	lBucketsSizeMax = 0;
	nReadSizeMin = 0;
	nReadSizeMax = 0;
	nReadBufferSize = 0;

	// Variables partagees
	DeclareSharedParameter(&shared_ivKeyFieldIndexes);

	shared_oaBuckets = new PLShared_ObjectArray(new PLShared_SortBucket);
	DeclareSharedParameter(shared_oaBuckets);
	DeclareSharedParameter(&shared_sFileName);
	DeclareSharedParameter(&shared_bHeaderLineUsed);
	DeclareSharedParameter(&shared_cInputFieldSeparator);
	DeclareSharedParameter(&shared_lFileSize);
	DeclareSharedParameter(&shared_lMaxSlaveBucketMemory);

	DeclareTaskInput(&input_bLastRound);
	DeclareTaskInput(&input_nBufferSize);
	DeclareTaskInput(&input_lFilePos);

	DeclareTaskOutput(&output_svBucketIds);
	DeclareTaskOutput(&output_svBucketFilePath);
	DeclareTaskOutput(&output_svBucketIds_dictionary);
	DeclareTaskOutput(&output_ivBucketSize_dictionary);

	shared_cInputFieldSeparator.SetValue('\t');
}

KWSortedChunkBuilderTask::~KWSortedChunkBuilderTask()
{
	delete shared_oaBuckets;
}

void KWSortedChunkBuilderTask::SetFileURI(const ALString& sValue)
{
	sFileURI = sValue;
}

const ALString& KWSortedChunkBuilderTask::GetFileURI() const
{
	return sFileURI;
}

void KWSortedChunkBuilderTask::SetHeaderLineUsed(boolean bValue)
{
	bHeaderLineUsed = bValue;
}

boolean KWSortedChunkBuilderTask::GetHeaderLineUsed() const
{
	return bHeaderLineUsed;
}

void KWSortedChunkBuilderTask::SetInputFieldSeparator(char cValue)
{
	shared_cInputFieldSeparator.SetValue(cValue);
}

char KWSortedChunkBuilderTask::GetInputFieldSeparator() const
{
	return shared_cInputFieldSeparator.GetValue();
}

IntVector* KWSortedChunkBuilderTask::GetKeyFieldIndexes()
{
	return shared_ivKeyFieldIndexes.GetIntVector();
}

const IntVector* KWSortedChunkBuilderTask::GetConstKeyFieldIndexes() const
{
	return shared_ivKeyFieldIndexes.GetConstIntVector();
}

boolean KWSortedChunkBuilderTask::BuildSortedChunks(const KWSortBuckets* buckets)
{
	boolean bOk;
	KWSortBucket* bucket;
	ALString sChunkFileName;
	int i;
	ALString sTmp;

	require(buckets != NULL);
	require(buckets->Check());
	require(buckets->GetBucketNumber() > 0);

	// Initialisation des buckets servant de specification aux chunks
	// Les buckets sont dupliques pour etre transferes a la variable partagee
	for (i = 0; i < buckets->GetBucketNumber(); i++)
	{
		bucket = buckets->GetBucketAt(i)->Clone();
		require(bucket->GetChunkFileNames()->GetSize() == 0);
		require(bucket->GetOutputFileName() == "");

		// Identification du bucket
		bucket->SetId(IntToString(i));
		shared_oaBuckets->GetObjectArray()->Add(bucket);
	}
	if (GetVerbose())
		AddMessage(sTmp + "Bucket Number : " + IntToString(i));

	// Execution de la tache
	bOk = Run();
	if (bOk)
	{
		// Mise a jour des buckets en entree avec les buckets resultats (avec chunks ou tries)
		for (i = 0; i < shared_oaBuckets->GetObjectArray()->GetSize(); i++)
		{
			buckets->GetBucketAt(i)->CopyFrom(
			    (cast(KWSortBucket*, shared_oaBuckets->GetObjectArray()->GetAt(i))));
		}
	}

	// Nettoyage
	shared_oaBuckets->Clean();
	return bOk;
}

void KWSortedChunkBuilderTask::Test()
{
	boolean bOk = true;
	KWArtificialDataset artificialDataset;
	longint lMeanKeySize;
	longint lLineNumber;
	ObjectArray oaKeys;
	KWSortBuckets sortBuckets;
	KWSortedChunkBuilderTask sortedChunkBuilder;

	// Gestion des taches
	TaskProgression::SetTitle("Test " + sortedChunkBuilder.GetTaskLabel());
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

	// Nettoyage
	oaKeys.DeleteAll();
	sortBuckets.DeleteBucketFiles();

	// Destruction du fichier
	artificialDataset.DeleteDataset();

	// Gestion des taches
	TaskProgression::Stop();
}

boolean KWSortedChunkBuilderTask::TestWithArtificialDataset(const KWArtificialDataset* artificialDataset,
							    const ObjectArray* oaKeySample, KWSortBuckets* sortBuckets)
{
	boolean bOk = true;
	KWSortedChunkBuilderTask sortedChunkBuilder;
	KWSortBucket mainBucket;
	KWSortBucket* bucket;
	int i;
	ALString sChunkFileName;

	require(artificialDataset != NULL);
	require(oaKeySample != NULL);
	require(sortBuckets != NULL);
	require(sortBuckets->GetBucketNumber() == 0);

	// Creation des buckets a partir des cles
	sortBuckets->Build(&mainBucket, oaKeySample);
	sortBuckets->IndexBuckets();

	// Creation d'un nom de fichier par bucket
	for (i = 0; i < sortBuckets->GetBucketNumber(); i++)
	{
		bucket = sortBuckets->GetBucketAt(i);
		sChunkFileName = FileService::GetFileName(FileService::GetFileName(artificialDataset->GetFileName()) +
							  "_chunk" + IntToString(i) + ".txt");
		sChunkFileName = FileService::CreateUniqueTmpFile(sChunkFileName, &sortedChunkBuilder);
		if (sChunkFileName.IsEmpty())
		{
			bOk = false;
			break;
		}
		bucket->SetOutputFileName(sChunkFileName);
	}

	// Extraction des chunks
	if (bOk)
	{
		sortedChunkBuilder.SetFileURI(FileService::BuildLocalURI(artificialDataset->GetFileName()));
		sortedChunkBuilder.SetHeaderLineUsed(artificialDataset->GetHeaderLineUsed());
		sortedChunkBuilder.SetInputFieldSeparator(artificialDataset->GetFieldSeparator());
		sortedChunkBuilder.GetKeyFieldIndexes()->CopyFrom(artificialDataset->GetConstKeyFieldIndexes());
		bOk = sortedChunkBuilder.BuildSortedChunks(sortBuckets);
	}

	// Affichage des resultats
	if (bOk)
	{
		for (i = 0; i < sortBuckets->GetBucketNumber(); i++)
		{
			KWArtificialDataset::DisplayFileFirstLines(
			    sortBuckets->GetBucketAt(i)->GetChunkFileNames()->GetAt(0), 10);
		}
	}
	return bOk;
}

const ALString KWSortedChunkBuilderTask::GetTaskName() const
{
	return "Chunks builder";
}

PLParallelTask* KWSortedChunkBuilderTask::Create() const
{
	return new KWSortedChunkBuilderTask;
}

boolean KWSortedChunkBuilderTask::ComputeResourceRequirements()
{
	boolean bOk = true;
	int nSlaveProcessNumber;
	const int nPreferredSize = PLRemoteFileService::GetPreferredBufferSize(sFileURI);

	// Taille du fichier
	lInputFileSize = PLRemoteFileService::GetFileSize(sFileURI);

	// Exigences de l'esclave min : Buffer de lecture (nPreferredSize) + buffer d'ecriture (nDefaultBufferSize) +
	// stockage des bucket (nPreferredSize)
	nReadSizeMin = nPreferredSize;
	lBucketsSizeMin = nPreferredSize;
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMin(nReadSizeMin + lBucketsSizeMin +
									      BufferedFile::nDefaultBufferSize);

	// Exigences de l'esclave max : on ne veut pas plus de 64 Mo de buffer de lecture, pas plus de 2Go pour le
	// stockage et pas plus de nDefaultBufferSize pour ecrire (en local)
	nReadSizeMax = (SystemFile::nMaxPreferredBufferSize / nPreferredSize) * nPreferredSize;
	nReadSizeMax = max(nReadSizeMax, nReadSizeMin);
	lBucketsSizeMax = 2 * lGB;

	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMax(nReadSizeMax + lBucketsSizeMax +
									      BufferedFile::nDefaultBufferSize);
	ensure(GetResourceRequirements()->GetSlaveRequirement()->Check());

	// Stockage des chunks sur les esclaves
	GetResourceRequirements()->GetGlobalSlaveRequirement()->GetDisk()->Set(lInputFileSize);

	// Nombre de slaveProcess
	nSlaveProcessNumber = (int)ceil(lInputFileSize * 1.0 / BufferedFile::nDefaultBufferSize);
	GetResourceRequirements()->SetMaxSlaveProcessNumber(nSlaveProcessNumber);
	return bOk;
}

boolean KWSortedChunkBuilderTask::MasterInitialize()
{
	boolean bOk = true;
	const int nPreferredBufferSize = PLRemoteFileService::GetPreferredBufferSize(sFileURI);
	ALString sTmp;
	longint lReadBufferSize;

	// Parametrage du fichier en entree
	shared_sFileName.SetValue(sFileURI);
	shared_bHeaderLineUsed = bHeaderLineUsed;

	shared_lFileSize = lInputFileSize;

	// Initialisation des attributs
	bLastSlaveProcessDone = false;

	// Test si fichier d'entree present
	if (not PLRemoteFileService::FileExists(shared_sFileName.GetValue()))
	{
		AddError("Input file " + FileService::GetURIUserLabel(shared_sFileName.GetValue()) + " does not exist");
		bOk = false;
	}

	// Test si taille du fichier non nulle
	if (lInputFileSize == 0)
	{
		AddError("Input file " + FileService::GetURIUserLabel(shared_sFileName.GetValue()) + " is empty");
		bOk = false;
	}
	lFilePos = 0;

	// Memoire residuelle a partager entre le buffer de lecture et le stockage
	longint lRemainingMemory = GetSlaveResourceGrant()->GetMemory() - InputBufferedFile::nDefaultBufferSize -
				   lBucketsSizeMin - nReadSizeMin;
	assert(lRemainingMemory >= 0);

	// Partage de la memoire restante
	if (lRemainingMemory > 0)
	{
		// On donne une taille proportionnelle aux tailles max demandees
		lReadBufferSize = nReadSizeMin + lRemainingMemory * nReadSizeMax / (lBucketsSizeMax + nReadSizeMax);
		if (GetVerbose())
			AddMessage(sTmp + "Read buffer size adjusted with remaining memory " +
				   LongintToHumanReadableString(lReadBufferSize));

		// Chaque esclave doit lire au moins 5 buffer (pour que le travail soit bien reparti entre les esclaves)
		if (lInputFileSize / (GetProcessNumber() * 5) < lReadBufferSize)
		{
			lReadBufferSize = lInputFileSize / (GetProcessNumber() * 5);
			if (GetVerbose())
				AddMessage(sTmp + "Read buffer size reduced to " +
					   LongintToHumanReadableString(lReadBufferSize));
		}

		nReadBufferSize = InputBufferedFile::FitBufferSize(lReadBufferSize);

		// Arrondi a un multiple de preferredBufferSize du buffer de lecture
		if (nReadBufferSize > nPreferredBufferSize)
		{
			nReadBufferSize = (nReadBufferSize / nPreferredBufferSize) * nPreferredBufferSize;
			if (GetVerbose())
				AddMessage(sTmp + "Read buffer size shrunk to preferred size multiple " +
					   LongintToHumanReadableString(nReadBufferSize));
		}
		nReadBufferSize = max(nReadBufferSize, nReadSizeMin);
		if (GetVerbose())
			AddMessage(sTmp + "Read buffer size bounded by min " +
				   LongintToHumanReadableString(nReadBufferSize));

		// Affectation de la memoire restante aux buckets
		shared_lMaxSlaveBucketMemory = max(lRemainingMemory - nReadBufferSize, (longint)lBucketsSizeMax);
		if (GetVerbose())
			AddMessage(sTmp + "Buckets size adjusted with remaining memory " +
				   LongintToHumanReadableString(shared_lMaxSlaveBucketMemory));
		assert(shared_lMaxSlaveBucketMemory > (longint)lBucketsSizeMin);

		// On borne de telle sorte que chaque esclave vide 5 fois son buffer
		if (lInputFileSize / (GetProcessNumber() * 5) < shared_lMaxSlaveBucketMemory)
		{
			shared_lMaxSlaveBucketMemory = lInputFileSize / (GetProcessNumber() * 5);
			if (GetVerbose())
				AddMessage(sTmp + "Buckets size shrunk for 5 dumps " +
					   LongintToHumanReadableString(shared_lMaxSlaveBucketMemory));
		}
		if (shared_lMaxSlaveBucketMemory < (longint)lBucketsSizeMin)
			shared_lMaxSlaveBucketMemory = lBucketsSizeMin;
	}
	else
	{
		// Initialisation avec le min
		shared_lMaxSlaveBucketMemory = lBucketsSizeMin;
		nReadBufferSize = nReadSizeMin;

		if (GetVerbose())
			AddMessage(sTmp + "Read buffer and bucket are set with minimum values");
	}
	ensure(nReadSizeMin <= nReadBufferSize and nReadBufferSize <= nReadSizeMax);
	ensure((longint)lBucketsSizeMin <= shared_lMaxSlaveBucketMemory and
	       shared_lMaxSlaveBucketMemory <= (longint)lBucketsSizeMax);

	return bOk;
}

boolean KWSortedChunkBuilderTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	ALString sTmp;

	// Est-ce qu'il y a encore du travail ?
	if (bLastSlaveProcessDone)
	{
		// Tous les esclaves ont effectue la derniere passe de SlaveProcess
		bIsTaskFinished = true;
		return true;
	}

	// Si fin de fichier les esclaves effectuent la derniere passe de SlaveProcess : ecriture des fichiers
	if (lFilePos >= lInputFileSize)
	{
		input_bLastRound = true;
		SetSlaveAtRestAfterProcess();
	}
	else
	{
		input_bLastRound = false;
		input_nBufferSize = ComputeStairBufferSize(nReadSizeMin, nReadBufferSize,
							   PLRemoteFileService::GetPreferredBufferSize(sFileURI),
							   lFilePos, lInputFileSize);
		input_lFilePos = lFilePos;

		// Calcul de la progression
		dTaskPercent = input_nBufferSize * 1.0 / lInputFileSize;
		if (dTaskPercent > 1)
			dTaskPercent = 1;

		lFilePos += input_nBufferSize;
	}
	return true;
}

boolean KWSortedChunkBuilderTask::MasterAggregateResults()
{
	int i;
	StringVector* svPath;
	LongintObject* lBucketSize;
	ALString sBucketId;

	// Si tous les esclaves sont en sommeil, c'est qu'ils ont tous execute le dernier SlaveProcess
	// Il faut les reveiller pour recevoir l'ordre d'arret
	if (GetRestingSlaveNumber() == GetProcessNumber())
	{
		SetAllSlavesAtWork();
		bLastSlaveProcessDone = true;
	}

	// Pour chaque bucket
	for (i = 0; i < output_svBucketIds.GetSize(); i++)
	{
		svPath = cast(StringVector*, odBucketsFiles.Lookup(output_svBucketIds.GetAt(i)));

		// Creation d'une nouvelle clef si cet Id n'est pas deja present
		if (svPath == NULL)
		{
			svPath = new StringVector;
			odBucketsFiles.SetAt(output_svBucketIds.GetAt(i), svPath);
		}

		// Ajout du chemin pour cet Id
		svPath->Add(output_svBucketFilePath.GetAt(i));
	}

	// Reception du dictionaire id bucket / size bucket
	for (i = 0; i < output_svBucketIds_dictionary.GetSize(); i++)
	{
		sBucketId = output_svBucketIds_dictionary.GetAt(i);

		lBucketSize = cast(LongintObject*, odIdBucketsSize_master.Lookup(sBucketId));
		if (lBucketSize == NULL)
		{
			lBucketSize = new LongintObject;
			odIdBucketsSize_master.SetAt(sBucketId, lBucketSize);
		}
		lBucketSize->SetLongint(lBucketSize->GetLongint() + output_ivBucketSize_dictionary.GetAt(i));
	}

	return true;
}

boolean KWSortedChunkBuilderTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	int i, j;
	KWSortBucket* bucket;
	StringVector svChunks;
	ALString sSortedFileName;
	StringVector* svChunkFileNames;
	StringVector svFilesToRemove;
	PLFileConcatenater concatenater;
	LongintObject* loBucketSize;

	svChunkFileNames = NULL;

	// Verification que la taille des buckets enregistree au long des slaveProcess est bien la bonne (par rapport a
	// GetFileSize())
	debug(; ALString sBucketID; Object * oElement; longint lComputeFileSize; StringVector * svBucketFiles;
	      POSITION position = odIdBucketsSize_master.GetStartPosition(); while (position != NULL) {
		      odIdBucketsSize_master.GetNextAssoc(position, sBucketID, oElement);
		      loBucketSize = cast(LongintObject*, oElement);
		      svBucketFiles = cast(StringVector*, odBucketsFiles.Lookup(sBucketID));
		      assert(svBucketFiles != NULL);
		      lComputeFileSize = 0;
		      for (i = 0; i < svBucketFiles->GetSize(); i++)
		      {
			      lComputeFileSize += PLRemoteFileService::GetFileSize(svBucketFiles->GetAt(i));
		      }
		      assert(lComputeFileSize == loBucketSize->GetLongint());
	      });

	// Assignation de la liste des chunks a chaque bucket
	if (bProcessEndedCorrectly and not TaskProgression::IsInterruptionRequested())
	{
		for (i = 0; i < shared_oaBuckets->GetObjectArray()->GetSize(); i++)
		{
			// Acces au bucket
			bucket = cast(KWSortBucket*, shared_oaBuckets->GetObjectArray()->GetAt(i));

			// Acces aux chunks de ce bucket
			svChunkFileNames = cast(StringVector*, odBucketsFiles.Lookup(bucket->GetId()));
			assert(svChunkFileNames != NULL);

			// Si le bucket contient des fichiers on les ajoute
			// (ce n'est pas forcement le cas on peur avoir un singleton [a,a] suivi de ]a,b[ qui peut etre
			// vide)
			if (svChunkFileNames != NULL)
			{
				bucket->SetChunkFileNames(svChunkFileNames);
				loBucketSize = cast(LongintObject*, odIdBucketsSize_master.Lookup(bucket->GetId()));
				if (loBucketSize != NULL)
					bucket->SetChunkFileSize(loBucketSize->GetLongint());
			}
		}
	}

	// Nettoyage des resultats si erreur
	if (not bProcessEndedCorrectly or TaskProgression::IsInterruptionRequested())
	{
		for (i = 0; i < shared_oaBuckets->GetObjectArray()->GetSize(); i++)
		{
			// Acces au bucket
			bucket = cast(KWSortBucket*, shared_oaBuckets->GetObjectArray()->GetAt(i));

			// Bucket avec un seul fichier deja trie
			if (bucket->GetOutputFileName() != "")
			{
				svFilesToRemove.Add(bucket->GetOutputFileName());
			}
			// Bucket avec une liste de chunks
			else
			{
				// Acces aux chunks de ce bucket
				svChunkFileNames = cast(StringVector*, odBucketsFiles.Lookup(bucket->GetId()));
				if (svChunkFileNames != NULL)
				{
					for (j = 0; j < svChunkFileNames->GetSize(); j++)
					{
						svFilesToRemove.Add(svChunkFileNames->GetAt(j));
					}
				}
			}
		}

		// Suppression de tous les fichiers
		concatenater.RemoveChunks(&svFilesToRemove);
	}

	// Nettoyage
	odBucketsFiles.DeleteAll();
	odIdBucketsSize_master.DeleteAll();
	return not TaskProgression::IsInterruptionRequested();
}

boolean KWSortedChunkBuilderTask::SlaveInitialize()
{
	int i;
	boolean bOk;

	require(shared_ivKeyFieldIndexes.GetSize() > 0);
	require(shared_oaBuckets->GetObjectArray()->GetSize() > 0);

	// Initialisation de l'extracteur de clef a partir du tableau d'index envoye par le master
	keyExtractor.SetKeyFieldIndexes(shared_ivKeyFieldIndexes.GetConstIntVector());

	// Initialisation des buckets a partir d'un tableau de buckets (transfert des buckets)
	slaveBuckets.Initialize(shared_oaBuckets->GetObjectArray());
	slaveBuckets.IndexBuckets();
	assert(slaveBuckets.Check());

	// Initialisation de la memoire utilisee
	// Parcours des buckets a ecrire
	lBucketsUsedMemory = 0;
	for (i = 0; i < shared_oaBuckets->GetObjectArray()->GetSize(); i++)
	{
		lBucketsUsedMemory +=
		    cast(KWSortBucket*, shared_oaBuckets->GetObjectArray()->GetAt(i))->GetUsedMemory();
	}

	inputFile.SetFieldSeparator(shared_cInputFieldSeparator.GetValue());
	inputFile.SetFileName(shared_sFileName.GetValue());
	inputFile.SetHeaderLineUsed(shared_bHeaderLineUsed);
	bOk = inputFile.Open();

	return bOk;
}

boolean KWSortedChunkBuilderTask::SlaveProcess()
{
	boolean bOk = true;
	boolean bTrace = false;
	KWKey key;
	int i;
	double dProgression;
	KWSortBucket* bucket;
	int nLineBeginPos;
	int nLineEndPos;
	CharVector cvLine;
	ObjectArray oaBucketsToWrite;
	PLFileConcatenater concatenater;
	int nBucketToWriteIndex;
	ObjectArray oaSortBucketsOnUsedMemory;
	longint lBucketSizeMean;
	POSITION position;
	ALString sBucketID;
	LongintObject* nBucketSize;
	Object* oElement;
	longint lBeginPos;
	longint lMaxEndPos;
	longint lNextLinePos;
	boolean bLineTooLong;
	int nCumulatedLineNumber;
	PeriodicTest periodicTestInterruption;
	boolean bIsLineOk;

	if (not input_bLastRound)
	{
		// Specification de la portion du fichier a traiter
		// On la termine sur la derniere ligne commencant dans le chunk, donc dans le '\n' se trouve
		// potentiellement sur le debut de chunk suivant, un octet au dela de la fin
		lBeginPos = input_lFilePos;
		lMaxEndPos = min(input_lFilePos + input_nBufferSize + 1, inputFile.GetFileSize());

		// Affichage
		if (bTrace)
			cout << GetClassLabel() << "\t" << GetTaskIndex() << "\tBegin\t"
			     << FileService::GetFileName(inputFile.GetFileName()) << "\t" << lBeginPos << "\t"
			     << lMaxEndPos << endl;

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

		// Initialisation des variables de travail
		keyExtractor.SetBufferedFile(&inputFile);

		// Remplissage du buffer avec des lignes entieres dans la limite de la taille du buffer
		// On reitere tant que l'on a pas atteint la derniere position pour lire toutes les ligne, y compris la
		// derniere
		while (bOk and lBeginPos < lMaxEndPos)
		{
			bOk = inputFile.FillOuterLinesUntil(lBeginPos, lMaxEndPos, bLineTooLong);
			if (not bOk)
				break;

			// Cas d'un ligne trop longue: on se deplace a la ligne suivante ou a la fin de la portion a
			// traiter
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
				if (shared_bHeaderLineUsed and inputFile.IsFirstPositionInFile())
					inputFile.SkipLine(bLineTooLong);

				// Parcours du buffer d'entree
				while (bOk and not inputFile.IsBufferEnd())
				{
					// Gestion de la progresssion
					if (periodicTestInterruption.IsTestAllowed(nCumulatedLineNumber +
										   inputFile.GetCurrentLineIndex()))
					{
						// Calcul de la progression par rapport a la proportion de la portion du
						// fichier traitee parce que l'on ne sait pas le nombre total de ligne
						// que l'on va traiter
						dProgression = (inputFile.GetPositionInFile() - input_lFilePos) * 1.0 /
							       (lMaxEndPos - input_lFilePos);

						TaskProgression::DisplayProgression((int)floor(dProgression * 100));
						if (TaskProgression::IsInterruptionRequested())
						{
							bOk = false;
							break;
						}
					}

					// Extraction de la clef
					bIsLineOk = keyExtractor.ParseNextKey(&key, this);

					// Ajout de la ligne dans le bucket approprie
					if (bIsLineOk)
					{
						keyExtractor.ExtractLine(nLineBeginPos, nLineEndPos);
						inputFile.ExtractSubBuffer(nLineBeginPos, nLineEndPos, &cvLine);

						// Si la derniere ligne du fichier n'a pas le caractere fin de ligne, on
						// le rajoute
						if (inputFile.IsFileEnd() and
						    cvLine.GetAt(cvLine.GetSize() - 1) != '\n')
							cvLine.Add('\n');

						// Ajout de la ligne dans le bon bucket
						slaveBuckets.AddLineAtKey(&key, &cvLine);

						// Mise a jour de la memoire utilisee
						lBucketsUsedMemory += (nLineEndPos - nLineBeginPos) * sizeof(char);

						// Ecriture des plus gros bucket si depassement de la memoire de
						// l'esclave
						if (lBucketsUsedMemory > shared_lMaxSlaveBucketMemory)
						{
							// Tri des bucket suivant la memoire utilise
							oaSortBucketsOnUsedMemory.CopyFrom(slaveBuckets.GetBuckets());
							oaSortBucketsOnUsedMemory.SetCompareFunction(
							    KWSortBucketCompareChunkSize);
							oaSortBucketsOnUsedMemory.Sort();

							// Ecriture des plus gros bucket pour diminuer la taille
							// utilisee par 2
							nBucketToWriteIndex = 0;

							// Taille moyenne des buckets stockes
							lBucketSizeMean =
							    lBucketsUsedMemory / slaveBuckets.GetBucketNumber();

							// Tant que la memoire utilise depasse la moitie de la memoire
							// autorisee
							while (bOk and
							       lBucketsUsedMemory > shared_lMaxSlaveBucketMemory / 2 and
							       nBucketToWriteIndex <
								   oaSortBucketsOnUsedMemory.GetSize())
							{
								bucket =
								    cast(KWSortBucket*, oaSortBucketsOnUsedMemory.GetAt(
											    nBucketToWriteIndex));
								assert(bucket != NULL);

								// Ecriture du bucket seulement il est plus gros que la
								// moyenne des tailles des buckets stockes
								if ((longint)(bucket->GetChunk()->GetSize() *
									      sizeof(char)) > lBucketSizeMean)
								{
									// Mise a jour de la memoire utilisee
									lBucketsUsedMemory -=
									    bucket->GetChunk()->GetSize() *
									    sizeof(char);

									// Ecriture du bucket
									bOk = WriteBucket(bucket);
								}

								// Bucket suivant
								nBucketToWriteIndex++;
							}
						}
					}
				}

				// On se deplace de la taille du buffer analyse
				lBeginPos += inputFile.GetCurrentBufferSize();
				nCumulatedLineNumber += inputFile.GetBufferLineNumber();
			}
		}

		// Envoi du nombre de lignes au master, en fin de SlaveProcess
		if (bOk)
			SetLocalLineNumber(nCumulatedLineNumber);
	}
	else
	{
		// On determine les buckets a ecrire
		for (i = 0; i < slaveBuckets.GetBucketNumber(); i++)
		{
			bucket = slaveBuckets.GetBucketAt(i);
			if (bucket->GetChunk()->GetSize() > 0)
				oaBucketsToWrite.Add(bucket);
		}

		// Parcours des buckets a ecrire
		for (i = 0; i < oaBucketsToWrite.GetSize(); i++)
		{
			bucket = cast(KWSortBucket*, oaBucketsToWrite.GetAt(i));

			if (TaskProgression::IsInterruptionRequested())
				break;
			bOk = WriteBucket(bucket);
			if (not bOk)
				break;
		}

		// Transfert du dictionnaire bucket Id / bucket size
		position = odIdBucketsSize_master.GetStartPosition();
		while (position != NULL)
		{
			odIdBucketsSize_master.GetNextAssoc(position, sBucketID, oElement);
			nBucketSize = cast(LongintObject*, oElement);
			if (nBucketSize != NULL)
			{
				output_ivBucketSize_dictionary.Add(nBucketSize->GetLongint());
				output_svBucketIds_dictionary.Add(sBucketID);
			}
		}
	}

	// Nettoyage en cas d'interruption (les noms des fichiers ne sont pas envoyes au maitre)
	if (not bOk)
	{
		concatenater.RemoveChunks(output_svBucketFilePath.GetConstStringVector());
	}
	return bOk;
}

boolean KWSortedChunkBuilderTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;

	odIdBucketsSize_master.DeleteAll();

	// Nettoyage
	if (inputFile.IsOpened())
		bOk = inputFile.Close();
	keyExtractor.Clean();
	slaveBuckets.DeleteAll();
	return bOk;
}

boolean KWSortedChunkBuilderTask::WriteBucket(KWSortBucket* bucketToWrite)
{
	boolean bOk;
	OutputBufferedFile bufferedFile;
	ALString sBucketFilePath;
	LongintObject* lBucketSize;

	require(bucketToWrite != NULL);
	bOk = true;
	if (bucketToWrite->GetOutputFileName() == "")
	{
		// Creation d'un nouveau chunk (on n'utilise pas CreateUnique car il peut y avoir des appels recursifs
		// du split)
		sBucketFilePath = FileService::CreateTmpFile(
		    "bucket_" + bucketToWrite->GetId() + "_task" + IntToString(GetTaskIndex()) + ".txt", this);
		bOk = not sBucketFilePath.IsEmpty();
		if (bOk)
		{
			bucketToWrite->SetOutputFileName(sBucketFilePath);

			// Envoi au master du nom du chunk associe au bucket
			output_svBucketIds.Add(bucketToWrite->GetId());
			output_svBucketFilePath.Add(FileService::BuildLocalURI(sBucketFilePath));
			bufferedFile.SetFileName(sBucketFilePath);
			bOk = bufferedFile.Open();
		}
	}
	else
	{
		// On ouvre le chunk deja ecrit
		bufferedFile.SetFileName(bucketToWrite->GetOutputFileName());
		bOk = bufferedFile.OpenForAppend();
	}

	// Ecriture du contenu du bucket
	if (bOk)
	{
		bOk = bufferedFile.Write(bucketToWrite->GetChunk());
		bOk = bufferedFile.Close() and bOk;

		// Mise a jour de la taille du bucket
		lBucketSize = cast(LongintObject*, odIdBucketsSize_master.Lookup(bucketToWrite->GetId()));
		if (lBucketSize == NULL)
		{
			lBucketSize = new LongintObject;
			odIdBucketsSize_master.SetAt(bucketToWrite->GetId(), lBucketSize);
		}
		assert(lBucketSize->GetLongint() + bucketToWrite->GetChunk()->GetSize() ==
		       PLRemoteFileService::GetFileSize(bufferedFile.GetFileName()));

		lBucketSize->SetLongint(lBucketSize->GetLongint() + bucketToWrite->GetChunk()->GetSize());

		// On vide le bucket
		bucketToWrite->GetChunk()->SetSize(0);
	}
	else
		AddError("Cannot open file  " + bufferedFile.GetFileName());
	return bOk;
}
