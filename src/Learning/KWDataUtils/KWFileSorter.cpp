// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWFileSorter.h"

#include "KWKeyExtractor.h"
#include "KWChunkSorterTask.h"
#include "KWKeySampleExtractorTask.h"
#include "KWSortedChunkBuilderTask.h"
#include "KWKeySizeEvaluatorTask.h"

//////////////////////////////////////////////////////////////////////
// Classe KWFileSorter
//
//	L'algorithme de tri a 3 parametres qui interviennent dans differentes classes.
//	La taille maximale des chunks est determinee automatiquement. Les bornes de chaque chunks sont determinees en
// realisant un sample de la base (methode  KWFileSorter::SplitDatabase). 	Le calcul de la taille du sample est
// basee sur le papier de DeWitt 1991 "Parallel sorting on a shared-nothing architecture using probabilistic splitting".
// Ce calcul fait intervenir le parametre dSkew (deviation). Il represente la taille maximale / la moyenne des tailles,
// plus il est
// proche de 1 plus les chunks 	auront la taille attendue et plus l'echantillon necessaire sera grand. On a choisi
// dSkew=1.1, ceci permet d'avoir une bonne estimation de la distribution, 	le temps pour trier le sample n'est
// quant a lui pas problematique.
//
//	Si des chunks construits sont plus gros que la taille maximale il faut les recouper. Pour eviter de recouper
// trop souvent on donne pour cible a l'algo de DeWitt 70% 	de la taille maximale (appel de SplitDatabase dans
// KWFileSorter::Sort). Les chunks seront recoupes si ils sont plus grands que la taille maximale.
//
//	Si il y a beaucoup d'esclaves et beaucoup de chunks, chaque esclave ecrit une toute petite partie de chaque
// chunk ce qui conduit a fragmenter le disque. 	On impose une taille minimale de segment contigu a 2 Mo
// (lMinFragmentSize dans la methode KWFileSorter::ComputeChunkSize). Cette valeur a une influence sur 	le nombre max de
// chunks.

const longint KWFileSorter::lChunkSizeLimit = (longint)INT_MAX;

KWFileSorter::KWFileSorter()
{
	bInputHeaderLineUsed = true;
	cInputFieldSeparator = '\t';
	bOutputHeaderLineUsed = true;
	cOutputFieldSeparator = '\t';
}

KWFileSorter::~KWFileSorter() {}

StringVector* KWFileSorter::GetKeyAttributeNames()
{
	return keyFieldsIndexer.GetKeyAttributeNames();
}

StringVector* KWFileSorter::GetNativeFieldNames()
{
	return keyFieldsIndexer.GetNativeFieldNames();
}

void KWFileSorter::SetInputFileName(const ALString& sValue)
{
	sInputFileName = sValue;
}

const ALString& KWFileSorter::GetInputFileName() const
{
	return sInputFileName;
}

void KWFileSorter::SetInputHeaderLineUsed(boolean bValue)
{
	bInputHeaderLineUsed = bValue;
}

boolean KWFileSorter::GetInputHeaderLineUsed() const
{
	return bInputHeaderLineUsed;
}

void KWFileSorter::SetInputFieldSeparator(char cValue)
{
	cInputFieldSeparator = cValue;
}

char KWFileSorter::GetInputFieldSeparator() const
{
	return cInputFieldSeparator;
}

void KWFileSorter::SetOutputFileName(const ALString& sValue)
{
	sOutputFileName = sValue;
}

const ALString& KWFileSorter::GetOutputFileName() const
{
	return sOutputFileName;
}

void KWFileSorter::SetOutputHeaderLineUsed(boolean bValue)
{
	bOutputHeaderLineUsed = bValue;
}

boolean KWFileSorter::GetOutputHeaderLineUsed() const
{
	return bOutputHeaderLineUsed;
}

void KWFileSorter::SetOutputFieldSeparator(char cValue)
{
	cOutputFieldSeparator = cValue;
}

char KWFileSorter::GetOutputFieldSeparator() const
{
	return cOutputFieldSeparator;
}

boolean KWFileSorter::Sort(boolean bDisplayUserMessage)
{
	boolean bOk = true;
	boolean bTrace = false;
	longint lEncodingErrorNumber;
	longint lSplitEncodingErrorNumber;
	boolean bSplitSilentMode;
	int nChunkSize;    // Taille des fichier chunks : un chunk doit tenir en memoire pour etre trie
	int nChunkSizeMin; // Taille minimum des chunks pour ne pas (trop) fragmenter le disque
	longint lObjectNumber;
	RMTaskResourceRequirement taskRequirement;
	int nSplitNumber;
	Timer timerSort;
	Timer timerSplit;
	Timer timerConcat;
	KWSortBucket* bucket;
	KWSortBuckets sortedBuckets;    // Ensemble des buckets
	KWSortBucket* overweightBucket; // Bucket trop gros
	KWSortBucket*
	    smallBucket; // Bucket qui tient en memoire, pour les petits fichiers qui seront traite en sequentiel
	KWSortBuckets* subBuckets; // Resultat du split du bucket trop gros
	boolean bIsHeaderLineUsed;
	StringVector svChunkFileNames;
	ALString sNewFileName;
	StringVector svFirstLine;
	ALString sTmp;
	KWChunkSorterTask parallelSorter;
	boolean bIsInterruptedByUser;
	longint lMeanKeySize;
	longint lLineNumber;
	KWKeySizeEvaluatorTask keySizeParallelEvaluator;
	longint lFileSize;
	ALString sOutputPah;
	longint lRemainingDisk;
	PLFileConcatenater concatenater;
	int i, j;

	require(sInputFileName != "");

	// Initialisation du nombre d'erreur d'encodage
	lEncodingErrorNumber = 0;

	// Emission de trace si les tache parallele sont en mode verbeux
	bTrace = bTrace or PLParallelTask::GetVerbose();

	// Test d'existence du fichier d'entree
	if (not PLRemoteFileService::FileExists(sInputFileName))
	{
		AddError("Input file is missing");
		return false;
	}

	// Creation du fichier de sortie pour reserver son nom
	bOk = PLRemoteFileService::CreateEmptyFile(sOutputFileName);
	if (not bOk)
	{
		AddError("Unable to create file " + sOutputFileName + " " + FileService::GetLastSystemIOErrorMessage());
		return false;
	}

	// Debut du timer
	timerSort.Start();

	// Initialisations
	lObjectNumber = 0;
	bIsInterruptedByUser = false;
	lMeanKeySize = 0;
	lLineNumber = 0;
	nChunkSize = 0;
	lFileSize = PLRemoteFileService::GetFileSize(sInputFileName);

	// Cas d'un fichier vide ou inexistant
	if (lFileSize == 0)
	{
		bOk = false;
		if (PLRemoteFileService::FileExists(sInputFileName))
			AddWarning("Empty input file");
		else
			AddWarning("Missing input file");
	}

	// Extraction du chemin du fichier de sortie
	sOutputPah = FileService::GetPathName(sOutputFileName);
	if (sOutputPah == "")
		sOutputPah = ".";

	// Y a-t-il assez de place pour ecrire le fichier de sortie
	if (bOk)
	{
		lRemainingDisk = PLRemoteFileService::GetDiskFreeSpace(sOutputPah) - lFileSize;
		if (lRemainingDisk < 0)
		{
			bOk = false;
			AddError(sTmp + "There is not enough space available on the disk to write output file (" +
				 sOutputPah + "): needs at least " + LongintToHumanReadableString(lFileSize) + ")");
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// Recherche des index des clefs
	if (bOk)
		// Extraction des champs de  la premiere ligne du fichier d'entree
		bOk = InputBufferedFile::GetFirstLineFields(sInputFileName, cInputFieldSeparator, false, false,
							    &svFirstLine);

	// Calcul des index des champs de la cle
	if (bOk)
		bOk = keyFieldsIndexer.ComputeKeyFieldIndexes(bInputHeaderLineUsed, &svFirstLine);

	if (bOk)
	{
		// Evaluation de la taille des clefs et du nombre de lignes
		bOk = keySizeParallelEvaluator.EvaluateKeySize(keyFieldsIndexer.GetConstKeyFieldIndexes(),
							       sInputFileName, bInputHeaderLineUsed,
							       cInputFieldSeparator, lMeanKeySize, lLineNumber);
		bIsInterruptedByUser = keySizeParallelEvaluator.IsTaskInterruptedByUser();
	}

	// Si il n'y a pas de header dans le fichier d'entree, on le construit a partir du dictionanire
	if (not bInputHeaderLineUsed)
		svFirstLine.CopyFrom(GetNativeFieldNames());

	if (bOk and not bIsInterruptedByUser)
	{
		// Cas des petits fichiers ou le tri parallele ne vaut pas le coup
		if (IsInMemorySort(lMeanKeySize, lLineNumber, lFileSize))
		{
			// Lancement de la tache KWChunkSorterTask sur un seul bucket
			smallBucket = new KWSortBucket;
			smallBucket->AddChunkFileName(sInputFileName);
			smallBucket->SetChunkSize(lFileSize);
			sortedBuckets.AddBucket(smallBucket);
			parallelSorter.SetTaskUserLabel("In memory sort");
			parallelSorter.SetBuckets(&sortedBuckets);
			parallelSorter.SetKeySize(lMeanKeySize);
			parallelSorter.SetBucketSize(lFileSize);
			parallelSorter.SetLineNumber(lLineNumber);
			parallelSorter.GetKeyFieldIndexes()->CopyFrom(keyFieldsIndexer.GetConstKeyFieldIndexes());
			parallelSorter.SetInputFieldSeparator(cInputFieldSeparator);
			parallelSorter.SetInputHeaderLineUsed(bInputHeaderLineUsed);
			parallelSorter.SetOutputFieldSeparator(cOutputFieldSeparator);
			bOk = parallelSorter.Sort();

			// Copie du fichier trie vers le fichier de sortie (utilisation de l'API de concatenation)
			if (bOk)
			{
				concatenater.SetFileName(sOutputFileName);
				concatenater.SetFieldSeparator(cOutputFieldSeparator);
				concatenater.SetVerbose(PLParallelTask::GetVerbose());
				if (bOutputHeaderLineUsed)
					concatenater.GetHeaderLine()->CopyFrom(&svFirstLine);
				concatenater.SetHeaderLineUsed(bOutputHeaderLineUsed);
				concatenater.SetDisplayProgression(false);

				// Concatenation
				bOk = concatenater.Concatenate(parallelSorter.GetSortedFiles(), this);
			}

			bIsInterruptedByUser = parallelSorter.IsTaskInterruptedByUser();
			if (bTrace)
				AddMessage(sTmp + "Time for parallel sorting : " +
					   SecondsToString(parallelSorter.GetJobElapsedTime()) + " (initialize : " +
					   SecondsToString(parallelSorter.GetMasterInitializeElapsedTime()) +
					   " finalize : " +
					   SecondsToString(parallelSorter.GetMasterFinalizeElapsedTime()) + ")");
			lObjectNumber = parallelSorter.GetSortedLinesNumber();

			// Mise a jour du nombre d'erreur d'encodage
			lEncodingErrorNumber = max(lEncodingErrorNumber, parallelSorter.GetEncodingErrorNumber());
		}
		else
		{
			// Calcul de la taille des chunks
			ComputeChunkSize(lFileSize, lLineNumber, lMeanKeySize, bTrace, nChunkSizeMin, nChunkSize);

			///////////////////////////////////////////////////////////////////////////
			// Creation des chunks
			nSplitNumber = 0;
			if (nChunkSize == -1 or nChunkSizeMin == -1)
				bOk = false;
			timerSplit.Start();

			// Initialisation des buckets avec un unique bucket initial : le fichier complet
			KWSortBucket* initBucket = new KWSortBucket;

			// Si le nom de fichier est standard, on construit une URI
			// Car en entree de ChunkBuilder on a des fichiers distants
			// (Le split peu etre appele plisuers fois, tant qu'on a des chunks trop gros)
			if (FileService::GetURIScheme(sInputFileName) == "")
				sNewFileName = FileService::BuildLocalURI(sInputFileName);
			else
				sNewFileName = sInputFileName;

			initBucket->AddChunkFileName(sNewFileName);
			initBucket->SetChunkSize(lFileSize);
			sortedBuckets.AddBucket(initBucket);
			overweightBucket = sortedBuckets.GetOverweightBucket(nChunkSize);

			// Tant qu'il y a des chunks plus gros que la taille souhaitee, on divise chaque chunk trop gros
			bSplitSilentMode = false;
			while (bOk and overweightBucket != NULL)
			{
				if (TaskProgression::IsInterruptionRequested())
					break;
				nSplitNumber++;

				if (nSplitNumber == 1)
					bIsHeaderLineUsed = bInputHeaderLineUsed;
				else
					bIsHeaderLineUsed = false;

				// Split du chunk trop gros
				subBuckets =
				    SplitDatabase(nChunkSizeMin, nChunkSize, overweightBucket, bIsHeaderLineUsed,
						  cInputFieldSeparator, lMeanKeySize, lLineNumber, bSplitSilentMode,
						  lSplitEncodingErrorNumber, bIsInterruptedByUser);

				// Mise a jour du nombre d'erreurs d'encodage
				lEncodingErrorNumber = max(lEncodingErrorNumber, lSplitEncodingErrorNumber);

				// Les prochaines passes potentielles de split se feront en mode silencieux, pour eviter
				// de gerer plusieurs fois les messages d'erreur
				bSplitSilentMode = true;

				// subBuckets est NULL en cas d'erreur ou d'arret utilisateur
				if (subBuckets == NULL)
				{
					bOk = false;
					break;
				}

				// Suppression du fichier correspondant au chunk (sauf le fichier initial)
				if (nSplitNumber != 1)
				{
					concatenater.RemoveChunks(overweightBucket->GetChunkFileNames());
				}

				// Remplacement du chunk par ses splits
				sortedBuckets.SplitLargeBucket(overweightBucket, subBuckets);
				delete subBuckets;

				// Est-ce qu'il y a encore un chunk trop gros
				overweightBucket = sortedBuckets.GetOverweightBucket(nChunkSize);

				// Evaluation de la taille des clefs et du nombre de lignes du chunk
				if (overweightBucket != NULL)
				{
					if (bTrace)
						AddMessage(
						    sTmp + "Remaining overweight chunk with size " +
						    LongintToHumanReadableString(overweightBucket->GetChunkSize()));

					// Concatenation des fichiers du bucket pour utiliser l'api de
					// KWKeySizeEvaluatorTask
					sNewFileName = FileService::CreateUniqueTmpFile(
					    FileService::GetFilePrefix(sInputFileName) + "_chunk" +
						overweightBucket->GetId() + ".txt",
					    this);
					bOk = sNewFileName != "";
					if (bOk)
					{
						// Concatenation interne, sans ligne d'entete
						concatenater.SetFileName(sNewFileName);
						concatenater.SetHeaderLineUsed(false);
						bOk = concatenater.Concatenate(overweightBucket->GetChunkFileNames(),
									       this);
					}
					else
					{
						concatenater.RemoveChunks(overweightBucket->GetChunkFileNames());
					}

					if (bOk)
					{
						sNewFileName = FileService::BuildLocalURI(sNewFileName);
						overweightBucket->RemoveChunkFileNames();
						overweightBucket->AddChunkFileName(sNewFileName);
						bOk = keySizeParallelEvaluator.EvaluateKeySize(
						    keyFieldsIndexer.GetConstKeyFieldIndexes(), sNewFileName,
						    bInputHeaderLineUsed, cInputFieldSeparator, lMeanKeySize,
						    lLineNumber);
					}

					bIsInterruptedByUser = keySizeParallelEvaluator.IsTaskInterruptedByUser();
				}
			}
			timerSplit.Stop();
			if (bTrace)
				AddMessage(sTmp + "Splitting done with " + IntToString(nSplitNumber) + " rounds");

			///////////////////////////////////////////////////////////////////////////
			// Tri
			if (bOk and not bIsInterruptedByUser)
			{
				if (nSplitNumber > 0)
					parallelSorter.SetTaskUserLabel(parallelSorter.GetTaskLabel() +
									" (sort phase 3/3)");

				parallelSorter.SetBuckets(&sortedBuckets);
				parallelSorter.SetBucketSize(nChunkSize);
				parallelSorter.SetKeySize(lMeanKeySize);
				parallelSorter.SetLineNumber(lLineNumber * nChunkSize / lFileSize);
				parallelSorter.GetKeyFieldIndexes()->CopyFrom(
				    keyFieldsIndexer.GetConstKeyFieldIndexes());
				parallelSorter.SetInputFieldSeparator(cInputFieldSeparator);
				parallelSorter.SetInputHeaderLineUsed(bInputHeaderLineUsed);
				parallelSorter.SetOutputFieldSeparator(cOutputFieldSeparator);

				// Tri des fichiers.
				// En cas d'erreur, les fichiers d'entree et de sortie sont nettoyes.
				bOk = parallelSorter.Sort();
				lObjectNumber = parallelSorter.GetSortedLinesNumber();
				bIsInterruptedByUser = parallelSorter.IsTaskInterruptedByUser();
				if (bTrace)
					AddSimpleMessage(
					    sTmp + "Time for parallel sorting : " +
					    SecondsToString(parallelSorter.GetJobElapsedTime()) + " (initialize : " +
					    SecondsToString(parallelSorter.GetMasterInitializeElapsedTime()) +
					    " finalize : " +
					    SecondsToString(parallelSorter.GetMasterFinalizeElapsedTime()) + ")");

				// Mise a jour du nombre d'erreurs d'encodage
				lEncodingErrorNumber =
				    max(lEncodingErrorNumber, parallelSorter.GetEncodingErrorNumber());

				// Concatenation
				if (bOk)
				{
					concatenater.SetDisplayProgression(true);
					concatenater.SetFieldSeparator(cOutputFieldSeparator);
					if (bOutputHeaderLineUsed)
						concatenater.GetHeaderLine()->CopyFrom(&svFirstLine);
					concatenater.SetHeaderLineUsed(bOutputHeaderLineUsed);
					concatenater.SetFileName(sOutputFileName);
					TaskProgression::BeginTask();
					TaskProgression::DisplayMainLabel("files concatenation");
					timerConcat.Start();
					bOk = concatenater.Concatenate(parallelSorter.GetSortedFiles(), this);
					timerConcat.Stop();
					TaskProgression::EndTask();
					if (bTrace)
						AddSimpleMessage(sTmp + "Time to concatenate files : " +
								 DoubleToString(timerConcat.GetElapsedTime()));
				}
			}
		} // Fin du if (IsFileTooSmall(sInputFileName) or lFileSize < nChunkSize)
	}         // Fin du if (bOk and not bIsInterruptedByUser)

	// Fin du timer
	timerSort.Stop();

	// Nettoyage en cas d'erreur
	if (not bOk or bIsInterruptedByUser)
	{
		// Suppression du fichier resultat
		PLRemoteFileService::RemoveFile(sOutputFileName);

		// Nettoyage des chunks construits
		// (Si il n'y a qu'un chunk, c'est le fichier initial, on ne le supprime pas)
		if (sortedBuckets.GetBucketNumber() > 1)
		{
			for (i = 0; i < sortedBuckets.GetBucketNumber(); i++)
			{
				bucket = sortedBuckets.GetBucketAt(i);
				for (j = 0; j < bucket->GetChunkFileNames()->GetSize(); j++)
				{
					PLRemoteFileService::RemoveFile(bucket->GetChunkFileNames()->GetAt(j));
				}
			}
		}
	}

	// Messages utilisateur
	if (bDisplayUserMessage)
	{
		// Message d'erreur si necessaire
		if (bOk)
			AddSimpleMessage(sTmp + "Sorted records: " + LongintToReadableString(lObjectNumber) +
					 " (Sort time: " + SecondsToString(timerSort.GetElapsedTime()) + ")");
		else if (bIsInterruptedByUser)
			AddWarning("Interrupted by user");
		else
			AddError("Interrupted because of errors");

		// Message sur les eventuelles erreurs d'encodage
		if (bOk)
			InputBufferedFile::AddEncodingErrorMessage(lEncodingErrorNumber, this);
	}
	return bOk;
}

void KWFileSorter::ComputeChunkSize(longint lFileSize, longint lLineNumber, longint lKeySize, boolean bTrace,
				    int& nChunkSizeMin, int& nChunkSize)
{
	RMTaskResourceRequirement taskRequirement;
	longint lSlaveMemory;
	longint lMaxChunkSize;
	longint lMinChunkSize;
	RMTaskResourceGrant grantedResources;
	ALString sTmp;
	double dMaxBucketNumber;
	longint lChunkSize;

	// Initialisation des output a -1
	nChunkSizeMin = -1;
	nChunkSize = -1;

	// Nombre de chunks que chaque esclave doit traite, plus il y en a, plus le travail sera bien
	// reparti et plus les esclaves finiront leur traitement en meme temps.
	// A contrario plus il y a en a plus la taille des chunks va etre petite, ce qui conduira a une
	// sous-utilisation de la memoire disponible
	const int nChunkNumberPerSlave = 4;

	// Taille minimum des fragments contigus ecrits sur le disque: 2MB
	// Si on compte environ 10 ms pour un temps d'acces disque, et un debit de 100 MB/s, cela fait 20 ms pour la
	// lecture, soit deux fois plus seulement que pour l'acces. Avec des chunks plus petits, il y aura trop de
	// morcellement, et les IO seront domines par les acces
	const longint lMinFragmentSize = 2 * lMB;

	// Evaluation de la taille minimale des bucket pour eviter de trop fragmenter le disque
	// on approxime nbBuffer par nbBucket, en supposant que dans l'etape de Chunk builder, chaque esclave
	// peut utiliser au maximum la meme quantite de memoire que dans l'etape de Chunk sorter.
	// Chacun des nbBuffer va donc devoir ecrire nbBuckets fichiers, soit en tout (nbBuffer * nbBucker) petits
	// fichiers, devant chacun etre plus gros que lMinFragmentSize La taille moyenne des tailles de fragments
	// correspond a la taille du fichier / (nbBuffer * nbBucker) On obtient ainsi un nombre maximal de buckets et
	// une taille minimale de chunks
	dMaxBucketNumber = sqrt(lFileSize * 1.0 / lMinFragmentSize);
	lMinChunkSize = (longint)(lFileSize / dMaxBucketNumber);

	// On apporte maintenant une correction en raison de l'heuristique utilisee dans l'etape de Chunk builder,
	// qui consite a ne vider que la moitie de son buffer pour ecrire uniquement les fichier les plus gros.
	// En supposant (regle sur le pouce) que l'on peut esperer gagner un facteur 2 sur la tailles des chunks,
	// on va de facon heuristique diminuer d'un facteur 2 la contrainte sur la plus petite taille de chunk.
	lMinChunkSize /= 2;
	if (bTrace)
		AddMessage(sTmp + "Min chunk size " + LongintToHumanReadableString(lMinChunkSize));

	// Calcul des ressources necessaires avec la taille de buffer calculee
	taskRequirement.GetGlobalSlaveRequirement()->GetDisk()->Set(lFileSize);
	taskRequirement.GetSlaveRequirement()->GetMemory()->SetMin(KWChunkSorterTask::ComputeSlaveMemoryRequirements(
	    lMinChunkSize, (longint)(lLineNumber / dMaxBucketNumber), lKeySize));
	taskRequirement.GetSlaveRequirement()->GetMemory()->SetMax(LLONG_MAX);

	taskRequirement.SetMemoryAllocationPolicy(RMTaskResourceRequirement::slavePreferred);
	RMParallelResourceManager::ComputeGrantedResources(&taskRequirement, &grantedResources);

	// Si il n'y pas asssez de ressources
	if (grantedResources.IsEmpty())
	{
		AddError(grantedResources.GetMissingResourceMessage());
		return;
	}

	// Minimum des memoires et espaces disques disponibles sur l'ensemble des esclaves
	// On enleve 10 Mo pour tenir compte de la variation de memoire a reserver actuellement et dans la tache de tri
	lSlaveMemory = grantedResources.GetSlaveMemory() - 10 * lMB;

	// Calcul de la taille maximum des chunk que la tache KWChunkSorterTask peut traiter si chaque esclave a
	// au moins lSlaveMemory de memoire disponible
	lMaxChunkSize = KWChunkSorterTask::ComputeMaxChunkSize(lKeySize, lSlaveMemory, lLineNumber, lFileSize);

	if (bTrace)
		AddMessage(sTmp + "Max Chunk size allowed by KWChunkSorterTask: " +
			   LongintToHumanReadableString(lMaxChunkSize));

	// Un chunk ne peut depasser 2 Go - 1 (taille des CharVector)
	lMaxChunkSize = min(lMaxChunkSize, lChunkSizeLimit);

	// Un chunk ne peut depasser la taille du fichier
	lMaxChunkSize = min(lMaxChunkSize, lFileSize);

	if (bTrace)
		AddMessage(sTmp + "Chunk size: " + LongintToHumanReadableString(lMaxChunkSize));

	// Si les chunks sont trop gros par rapport au fichier.
	// En sequentiel plus ils sont gros mieux c'est.
	// En parallele, on souhaite que chaque esclave traite 4 chunks, pour avoir une bonne repartition des
	// traitements
	if (not grantedResources.IsSequentialTask() and
	    grantedResources.GetSlaveNumber() * (longint)nChunkNumberPerSlave * lMaxChunkSize > lFileSize)
	{
		if (bTrace)
			AddMessage(sTmp + "Chunks are too big: we want " + IntToString(nChunkNumberPerSlave) +
				   " chunks per slave");

		// Calcul de la taille optimale : 4 chunks par esclave
		lChunkSize = lFileSize / ((longint)nChunkNumberPerSlave * grantedResources.GetSlaveNumber());

		// TODO BG: eviter la boucle si possible
		// Par contre on veut eviter que lChunkSize==nChunkSizeMin pour donner de la latitude a l'algo de DeWitt
		// (si on demande precisement lChunkSize on risque de se tromper, il faut mieux demander 0.8*lChunkSize
		// et on peut le faire uniquement si 0.8*lChunkSize > nChunkSizeMin )
		int nbChunks = nChunkNumberPerSlave - 1;
		while (dDeWittRatio * lChunkSize < lMinChunkSize and nbChunks > 1)
		{
			lChunkSize = lFileSize / ((longint)nbChunks * grantedResources.GetSlaveNumber());
			nbChunks--;
		}
		lChunkSize = min(lMaxChunkSize, lChunkSize);
		if (bTrace)
			AddMessage(sTmp + "New estimated chunk size: " + LongintToHumanReadableString(lChunkSize));
	}
	else
	{
		lChunkSize = lMaxChunkSize;
	}

	// La taille des chunks ne doit pas etre trop petite
	if (lChunkSize < lMinChunkSize)
	{
		if (bTrace)
			AddMessage(sTmp + "Chunks are too small, set to " +
				   LongintToHumanReadableString(lMinChunkSize));
		lChunkSize = lMinChunkSize;
	}

	// Un chunk ne peut pas faire plus de 2 Go car il est charge en memoire entierement dans un BufferedFile = un
	// charVector indexe par un int Il faut donc veiller a renvoyer moins de 2 Go dans cette methode
	ensure(lChunkSize <= INT_MAX);

	nChunkSizeMin = (int)lMinChunkSize;
	nChunkSize = (int)lChunkSize;
}

KWSortBuckets* KWFileSorter::SplitDatabase(int nChunkSizeMin, int nChunkSize, KWSortBucket* bucket,
					   boolean bIsHeaderLineUsed, char cFieldSeparator, longint lMeanKeySize,
					   longint lLineNumber, boolean bSilentMode, longint& lEncodingErrorNumber,
					   boolean& bIsInterruptedByUser)
{
	boolean bTrace = false;
	ALString sTmp;
	KWSortBuckets* sortedBuckets;
	InputBufferedFile bufferdFile;
	ALString sChunkFileName;
	int nEstimatedChunkNumberMin;
	int nEstimatedChunkNumberMax;
	ObjectArray oaSplitkeys;
	int nSampleSize;
	int nConservativeChunkSize;
	ALString sLine;
	KWKeySampleExtractorTask sampleExtractor;
	KWSortedChunkBuilderTask chunksBuilder;
	ALString sFileURI;
	boolean bOk;
	longint lSourceFileSize;
	const double dSkew = 1.1;

	require(bucket != NULL);
	require(bucket->GetOutputFileName() == "");
	require(bucket->GetChunkFileNames()->GetSize() == 1);
	require(nChunkSizeMin > 0);
	require(nChunkSize >= nChunkSizeMin);

	// Initialisations
	lEncodingErrorNumber = 0;
	bIsInterruptedByUser = false;
	bTrace = bTrace or PLParallelTask::GetVerbose();
	sortedBuckets = NULL;
	sFileURI = bucket->GetChunkFileNames()->GetAt(0);

	// Pour minimiser le nombre de redecoupage (appel recursif de Splitdatabase)
	// On vise une taille inferieure a la taille max
	nConservativeChunkSize = (int)(nChunkSize * dDeWittRatio);
	if (nConservativeChunkSize < nChunkSizeMin)
		nConservativeChunkSize = nChunkSizeMin;

	// Estimation du nombre de chunks (un chunk doit tenir en memoire)
	lSourceFileSize = PLRemoteFileService::GetFileSize(sFileURI);
	nEstimatedChunkNumberMax = 1 + (int)(lSourceFileSize / nChunkSizeMin);
	nEstimatedChunkNumberMin = 1 + (int)(lSourceFileSize / nConservativeChunkSize);

	// Estimation de la taille de l'echantillon en s'inspirant de DeWitt 1991
	// "Parallel sorting on a shared-nothing architecture using probabilistic splitting."
	// Parallel and Distributed Information Systems, p280 - 291
	nSampleSize = (int)ceil(nEstimatedChunkNumberMin * 2 * log(nEstimatedChunkNumberMin / 1e-6) /
				    ((1 - 1 / dSkew) * (1 - 1 / dSkew) * dSkew) +
				1000);
	ensure((nSampleSize * 1.0) / lLineNumber > 0);
	if (nSampleSize > lLineNumber)
		nSampleSize = (int)lLineNumber;

	if (bTrace)
	{
		AddMessage(sTmp + "Min sample size: " + LongintToReadableString(nSampleSize) + " / " +
			   LongintToReadableString(lLineNumber));
		AddMessage(sTmp + "Chunk number min: " + IntToString(nEstimatedChunkNumberMin) +
			   " max: " + IntToString(nEstimatedChunkNumberMax));
	}
	// Extraction du sample
	sampleExtractor.SetTaskUserLabel(sampleExtractor.GetTaskLabel() + " (sort phase 1/3)");
	sampleExtractor.SetFileURI(sFileURI);
	sampleExtractor.SetHeaderLineUsed(bIsHeaderLineUsed);
	sampleExtractor.SetFieldSeparator(cFieldSeparator);
	sampleExtractor.SetFileLineNumber(lLineNumber);
	sampleExtractor.SetSplitKeyNumber(nEstimatedChunkNumberMin - 1, nEstimatedChunkNumberMax - 1);
	sampleExtractor.SetSampleSize(nSampleSize);
	sampleExtractor.GetKeyFieldIndexes()->CopyFrom(keyFieldsIndexer.GetConstKeyFieldIndexes());
	sampleExtractor.SetKeyUsedMemory(lMeanKeySize);
	bOk = sampleExtractor.ExtractSample(&oaSplitkeys);
	bIsInterruptedByUser = sampleExtractor.IsTaskInterruptedByUser();

	if (bTrace and bOk)
	{
		AddSimpleMessage(
		    sTmp + "Time to extract sample : " + SecondsToString(sampleExtractor.GetJobElapsedTime()) +
		    " (Master finalize : " + SecondsToString(sampleExtractor.GetMasterFinalizeElapsedTime()) + ")");
	}

	// Creation des chunks
	if (bOk and not bIsInterruptedByUser)
	{
		chunksBuilder.SetTaskUserLabel(chunksBuilder.GetTaskLabel() + " (sort phase 2/3)");
		chunksBuilder.GetKeyFieldIndexes()->CopyFrom(keyFieldsIndexer.GetConstKeyFieldIndexes());
		chunksBuilder.SetFileURI(sFileURI);
		chunksBuilder.SetInputFieldSeparator(cFieldSeparator);
		chunksBuilder.SetHeaderLineUsed(bIsHeaderLineUsed);
		chunksBuilder.SetSilentMode(bSilentMode);
		sortedBuckets = new KWSortBuckets;

		// Initialization des buckets a partir des splits
		sortedBuckets->Build(bucket, &oaSplitkeys);

		// lancement de la tache de construction des chunks
		bOk = chunksBuilder.BuildSortedChunks(sortedBuckets);
		lEncodingErrorNumber = chunksBuilder.GetEncodingErrorNumber();
		bIsInterruptedByUser = chunksBuilder.IsTaskInterruptedByUser();
	}

	if (bTrace and bOk)
	{
		AddSimpleMessage(
		    sTmp + "Time to build sorted chunks : " + SecondsToString(chunksBuilder.GetJobElapsedTime()) +
		    " (Master finalize : " + SecondsToString(chunksBuilder.GetMasterFinalizeElapsedTime()) + ")");
	}

	// Nettoyage si pbm ou sortie utilisateur
	if (not bOk)
	{
		delete sortedBuckets;
		sortedBuckets = NULL;
	}

	// Nettoyage
	oaSplitkeys.DeleteAll();
	return sortedBuckets;
}

boolean KWFileSorter::IsInMemorySort(longint lKeySize, longint lLineNumber, longint lFileSize)
{
	RMTaskResourceRequirement requirement;
	RMTaskResourceGrant grantedResource;
	boolean bIsInMemory;

	if (lFileSize > 100 * lMB)
		return false;

	// Exigences des esclaves et du maitre
	requirement.GetMasterRequirement()->GetMemory()->Set(InputBufferedFile::nDefaultBufferSize);
	requirement.GetSlaveRequirement()->GetMemory()->Set(
	    KWChunkSorterTask::ComputeSlaveMemoryRequirements(lFileSize, lLineNumber, lKeySize));

	// On force en sequentiel
	requirement.SetMaxSlaveProcessNumber(1);

	// Y a-t-il des ressources disponibles pour ces exigences
	RMParallelResourceManager::ComputeGrantedResources(&requirement, &grantedResource);
	bIsInMemory = not grantedResource.IsEmpty();
	return bIsInMemory;
}

const ALString KWFileSorter::GetClassLabel() const
{
	return "File sorter";
}

const ALString KWFileSorter::GetObjectLabel() const
{
	return sInputFileName;
}

void KWFileSorter::Test()
{
	KWArtificialDataset artificialDataset1;
	KWArtificialDataset artificialDataset2;
	KWFileSorter fileSorter;

	// Gestion des taches
	TaskProgression::SetTitle("Test " + fileSorter.GetClassLabel());
	if (PLParallelTask::GetDriver()->IsParallelModeAvailable())
		TaskProgression::SetDisplayedLevelNumber(1);
	else
		TaskProgression::SetDisplayedLevelNumber(2);

	TaskProgression::Start();

	// Specification de deux types de fichier
	artificialDataset1.SpecifySortDataset();
	artificialDataset1.SetHeaderLineUsed(true);
	artificialDataset1.SetFieldSeparator(';');
	artificialDataset1.SetLineNumber(200000);
	artificialDataset1.SetFileName(artificialDataset1.BuildFileName());

	artificialDataset2.SpecifySortDataset();
	artificialDataset2.SetHeaderLineUsed(false);
	artificialDataset2.SetFieldSeparator(',');
	artificialDataset2.SetFileName(artificialDataset2.BuildFileName());

	// Creation des fichiers
	assert(artificialDataset1.GetFileName() != artificialDataset2.GetFileName());
	artificialDataset1.CreateDataset();
	artificialDataset1.DisplayFirstLines(15);
	artificialDataset2.CreateDataset();
	artificialDataset2.DisplayFirstLines(15);

	// Tri selon toutes les croisements possibles de specs de fichiers
	KWFileSorter::TestWithArtificialData(&artificialDataset1, &artificialDataset1);
	KWFileSorter::TestWithArtificialData(&artificialDataset1, &artificialDataset2);
	KWFileSorter::TestWithArtificialData(&artificialDataset2, &artificialDataset2);
	KWFileSorter::TestWithArtificialData(&artificialDataset2, &artificialDataset1);

	// Destruction du fichier
	artificialDataset1.DeleteDataset();
	artificialDataset2.DeleteDataset();

	// Gestion des taches
	TaskProgression::Stop();
}

boolean KWFileSorter::TestWithArtificialData(const KWArtificialDataset* inputArtificialDataset,
					     const KWArtificialDataset* outputArtificialDataset)
{
	boolean bOk = true;
	KWFileSorter fileSorter;
	ObjectArray oaSortAttributeNames;
	Timer timer;

	require(inputArtificialDataset != NULL);
	require(outputArtificialDataset != NULL);

	// Parametrage du tri
	fileSorter.SetInputFileName(inputArtificialDataset->GetFileName());
	fileSorter.SetInputHeaderLineUsed(inputArtificialDataset->GetHeaderLineUsed());
	fileSorter.SetInputFieldSeparator(inputArtificialDataset->GetFieldSeparator());
	fileSorter.SetOutputFileName(outputArtificialDataset->GetFileName() + ".sort");
	fileSorter.SetOutputHeaderLineUsed(outputArtificialDataset->GetHeaderLineUsed());
	fileSorter.SetOutputFieldSeparator(outputArtificialDataset->GetFieldSeparator());
	inputArtificialDataset->ExportKeyAttributeNames(fileSorter.GetKeyAttributeNames());
	inputArtificialDataset->ExportNativeFieldNames(fileSorter.GetNativeFieldNames());

	// Tri
	cout << "Sort " << FileService::GetFileName(inputArtificialDataset->GetFileName()) << flush;
	timer.Start();
	bOk = fileSorter.Sort(false);
	timer.Stop();
	cout << ": " << timer.GetElapsedTime() << endl;

	// Affichage des resultats
	cout << "Sorted file" << endl;
	KWArtificialDataset::DisplayFileFirstLines(fileSorter.GetOutputFileName(), 15);

	// Nettoyage
	FileService::RemoveFile(fileSorter.GetOutputFileName());

	return bOk;
}
