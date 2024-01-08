// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDatabaseChunkBuilder.h"

KWDatabaseChunkBuilder::KWDatabaseChunkBuilder()
{
	databaseIndexer = NULL;
}

KWDatabaseChunkBuilder::~KWDatabaseChunkBuilder() {}

void KWDatabaseChunkBuilder::SetDatabaseIndexer(KWDatabaseIndexer* indexer)
{
	CleanChunks();
	databaseIndexer = indexer;
}

KWDatabaseIndexer* KWDatabaseChunkBuilder::GetDatabaseIndexer() const
{
	return databaseIndexer;
}

void KWDatabaseChunkBuilder::InitializeFromDatabase(const KWDatabase* database)
{
	require(databaseIndexer != NULL);
	CleanChunks();
	databaseIndexer->InitializeFromDatabase(database);
}

boolean KWDatabaseChunkBuilder::IsInitialized() const
{
	require(databaseIndexer != NULL);
	return databaseIndexer != NULL and databaseIndexer->IsInitialized();
}

boolean KWDatabaseChunkBuilder::BuildChunks(int nSlaveNumber, longint lSlaveGrantedMemoryForSourceDatabase,
					    longint lForcedMaxTotalFileSizePerProcess)
{
	boolean bOk = true;
	const int nMaxProcessBySlave = 3; // On se limite a 3, car le debut et la fin sont de toute facon echelonnes
	const PLDatabaseTextFile* sourcePLDatabase;
	longint lMaxFileSizePerProcess;

	require(IsInitialized());
	require(nSlaveNumber >= 1);
	require(lSlaveGrantedMemoryForSourceDatabase > 0);
	require(lForcedMaxTotalFileSizePerProcess >= 0);

	// Calcul prealable de l'indexation en micro-chunks
	bOk = databaseIndexer->ComputeIndexation();

	// Message d'erreur si necessaire
	if (databaseIndexer->IsInterruptedByUser())
		databaseIndexer->AddWarning("Preliminary indexation of database interrupted by user");
	else if (not bOk)
		databaseIndexer->AddError("A problem occurred during preliminary indexation of database");

	// Recalcul des chunks si possible
	CleanChunks();
	if (bOk)
	{
		assert(databaseIndexer->GetChunkNumber() > 0);

		// Acces a la base dans sa version parallele
		sourcePLDatabase = databaseIndexer->GetPLDatabase();
		assert(sourcePLDatabase->IsOpenInformationComputed());

		// On limite le nombre de process par esclave en cas de base trop petite
		lMaxFileSizePerProcess =
		    max((longint)sourcePLDatabase->GetDatabasePreferredBuferSize(),
			sourcePLDatabase->GetTotalUsedFileSize() / (nSlaveNumber * nMaxProcessBySlave));

		// On peut imposer la taille du buffer pour raison de tests
		if (lForcedMaxTotalFileSizePerProcess > 0)
			lMaxFileSizePerProcess = lForcedMaxTotalFileSizePerProcess;

		// Calcul des chunks
		ComputeAllChunksInformations(nSlaveNumber, lMaxFileSizePerProcess);
	}
	return bOk;
}

boolean KWDatabaseChunkBuilder::IsComputed() const
{
	require(databaseIndexer != NULL);
	return ivChunkBeginIndexes.GetSize() > 0;
}

void KWDatabaseChunkBuilder::CleanChunks()
{
	ivUsedTableFlags.SetSize(0);
	ivChunkBeginIndexes.SetSize(0);
}

int KWDatabaseChunkBuilder::GetChunkNumber() const
{
	require(databaseIndexer != NULL);
	return ivChunkBeginIndexes.GetSize();
}

void KWDatabaseChunkBuilder::GetChunkLastRootKeyAt(int nChunkIndex, KWKey* lastRootKey) const
{
	int nMicroChunkIndex;

	require(databaseIndexer != NULL);
	require(0 <= nChunkIndex and nChunkIndex < GetChunkNumber());
	require(lastRootKey != NULL);

	// On recherche l'information pour le micro-chunk correspondant au chunk
	nMicroChunkIndex = ivChunkBeginIndexes.GetAt(nChunkIndex);
	lastRootKey->CopyFrom(databaseIndexer->GetChunkPreviousRootKeyAt(nMicroChunkIndex));
}

void KWDatabaseChunkBuilder::GetChunkPreviousRecordIndexesAt(int nChunkIndex,
							     LongintVector* lvChunkPreviousRecordIndexes) const
{
	int nTable;
	int nMicroChunkIndex;

	require(databaseIndexer != NULL);
	require(0 <= nChunkIndex and nChunkIndex < GetChunkNumber());
	require(lvChunkPreviousRecordIndexes != NULL);

	// On recherche l'information pour le micro-chunk correspondant au chunk
	nMicroChunkIndex = ivChunkBeginIndexes.GetAt(nChunkIndex);
	lvChunkPreviousRecordIndexes->SetSize(databaseIndexer->GetTableNumber());
	for (nTable = 0; nTable < lvChunkPreviousRecordIndexes->GetSize(); nTable++)
	{
		lvChunkPreviousRecordIndexes->SetAt(
		    nTable, ivUsedTableFlags.GetAt(nTable) *
				databaseIndexer->GetChunkPreviousRecordIndexesAt(nMicroChunkIndex, nTable));
	}
}

void KWDatabaseChunkBuilder::GetChunkBeginPositionsAt(int nChunkIndex, LongintVector* lvChunkBeginPositions) const
{
	int nTable;
	int nMicroChunkIndex;

	require(databaseIndexer != NULL);
	require(0 <= nChunkIndex and nChunkIndex < GetChunkNumber());
	require(lvChunkBeginPositions != NULL);

	// On recherche l'information pour le micro-chunk correspondant au chunk
	nMicroChunkIndex = ivChunkBeginIndexes.GetAt(nChunkIndex);
	lvChunkBeginPositions->SetSize(databaseIndexer->GetTableNumber());
	for (nTable = 0; nTable < lvChunkBeginPositions->GetSize(); nTable++)
		lvChunkBeginPositions->SetAt(nTable,
					     ivUsedTableFlags.GetAt(nTable) *
						 databaseIndexer->GetChunkBeginPositionsAt(nMicroChunkIndex, nTable));
}

void KWDatabaseChunkBuilder::GetChunkEndPositionsAt(int nChunkIndex, LongintVector* lvChunkEndPositions) const
{
	int nTable;
	int nMicroChunkIndex;

	require(databaseIndexer != NULL);
	require(0 <= nChunkIndex and nChunkIndex < GetChunkNumber());
	require(lvChunkEndPositions != NULL);

	// On recherche l'information pour le micro-chunk precedent le chunk suivant
	// ou pour le dernier micro-chunk sinon
	if (nChunkIndex < GetChunkNumber() - 1)
		nMicroChunkIndex = ivChunkBeginIndexes.GetAt(nChunkIndex + 1) - 1;
	else
		nMicroChunkIndex = databaseIndexer->GetChunkNumber() - 1;
	lvChunkEndPositions->SetSize(databaseIndexer->GetTableNumber());
	for (nTable = 0; nTable < lvChunkEndPositions->GetSize(); nTable++)
		lvChunkEndPositions->SetAt(nTable,
					   ivUsedTableFlags.GetAt(nTable) *
					       databaseIndexer->GetChunkEndPositionsAt(nMicroChunkIndex, nTable));
}

longint KWDatabaseChunkBuilder::GetChunkPreviousRecordIndexAt(int nChunkIndex) const
{
	int nMicroChunkIndex;

	require(databaseIndexer != NULL);
	require(0 <= nChunkIndex and nChunkIndex < GetChunkNumber());

	// On recherche l'information pour le micro-chunk correspondant au chunk
	nMicroChunkIndex = ivChunkBeginIndexes.GetAt(nChunkIndex);
	return databaseIndexer->GetChunkPreviousRecordIndexesAt(nMicroChunkIndex, 0);
}

longint KWDatabaseChunkBuilder::GetChunkBeginPositionAt(int nChunkIndex) const
{
	int nMicroChunkIndex;

	require(databaseIndexer != NULL);
	require(0 <= nChunkIndex and nChunkIndex < GetChunkNumber());

	// On recherche l'information pour le micro-chunk correspondant au chunk
	nMicroChunkIndex = ivChunkBeginIndexes.GetAt(nChunkIndex);
	return databaseIndexer->GetChunkBeginPositionsAt(nMicroChunkIndex, 0);
}

longint KWDatabaseChunkBuilder::GetChunkEndPositionAt(int nChunkIndex) const
{
	int nMicroChunkIndex;

	require(databaseIndexer != NULL);
	require(0 <= nChunkIndex and nChunkIndex < GetChunkNumber());

	// On recherche l'information pour le micro-chunk precedent le chunk suivant
	// ou pour le dernier micro-chunk sinon
	if (nChunkIndex < GetChunkNumber() - 1)
		nMicroChunkIndex = ivChunkBeginIndexes.GetAt(nChunkIndex + 1) - 1;
	else
		nMicroChunkIndex = databaseIndexer->GetChunkNumber() - 1;
	return databaseIndexer->GetChunkEndPositionsAt(nMicroChunkIndex, 0);
}

void KWDatabaseChunkBuilder::Write(ostream& ost) const
{
	int nChunk;
	int nTable;
	LongintVector lvChunkBeginPositions;
	LongintVector lvChunkEndPositions;
	LongintVector lvChunkPreviousRecordIndexes;
	KWKey lastRootKey;
	longint lChunkSize;

	ost << GetClassLabel() << " " << GetObjectLabel() << "\n";
	if (databaseIndexer != NULL)
	{
		// Detail des chunks
		cout << "Chunks\t" << GetChunkNumber() << "\n";
		if (GetChunkNumber() > 0)
		{
			// En tete pour les chunks
			ost << "Chunk\tMicro-chunk\t";
			for (nTable = 0; nTable < databaseIndexer->GetTableNumber(); nTable++)
			{
				ost << "Record" << nTable + 1 << "\t";
				ost << "Begin" << nTable + 1 << "\t";
				ost << "End" << nTable + 1 << "\t";
			}
			ost << "Chunk size\t";
			ost << "LastKey\n";
		}

		// Detail par chunk
		for (nChunk = 0; nChunk < GetChunkNumber(); nChunk++)
		{
			// Acces aux caracteristique du chunk
			GetChunkLastRootKeyAt(nChunk, &lastRootKey);
			GetChunkPreviousRecordIndexesAt(nChunk, &lvChunkPreviousRecordIndexes);
			GetChunkBeginPositionsAt(nChunk, &lvChunkBeginPositions);
			GetChunkEndPositionsAt(nChunk, &lvChunkEndPositions);

			// Calcul de la taille cumule du chunk
			lChunkSize = 0;
			for (nTable = 0; nTable < databaseIndexer->GetTableNumber(); nTable++)
				lChunkSize += lvChunkEndPositions.GetAt(nTable) - lvChunkBeginPositions.GetAt(nTable);

			// Affichage des caracteristiques
			ost << nChunk << "\t";
			ost << ivChunkBeginIndexes.GetAt(nChunk) << "\t";
			for (nTable = 0; nTable < databaseIndexer->GetTableNumber(); nTable++)
			{
				ost << lvChunkPreviousRecordIndexes.GetAt(nTable) << "\t";
				ost << lvChunkBeginPositions.GetAt(nTable) << "\t";
				ost << lvChunkEndPositions.GetAt(nTable) << "\t";
			}
			ost << lChunkSize << "\t";
			ost << lastRootKey.GetObjectLabel() << "\n";
		}
	}
}

const ALString KWDatabaseChunkBuilder::GetClassLabel() const
{
	return "Database chunk builder";
}

const ALString KWDatabaseChunkBuilder::GetObjectLabel() const
{
	if (IsInitialized())
		return databaseIndexer->GetPLDatabase()->GetDatabase()->GetDatabaseName();
	else
		return "";
}

void KWDatabaseChunkBuilder::ComputeAllChunksInformations(int nSlaveNumber, longint lMaxFileSizePerProcess)
{
	boolean bDisplayChunkInformation = false;
	longint lTotalUsedFileSize;
	int nChunk;
	int nMapping;
	ObjectArray oaCollectedUsedMappings;
	longint lFileSizePerProcess;
	longint lMinFileSizePerProcess;
	longint lCurrentMaxFileSizePerProcess;
	longint lCurrentTotalFileSize;
	longint lEndFileSize;
	double dRemainingEndFilePercentage;

	require(databaseIndexer->GetChunkNumber() >= 1);
	require(ivUsedTableFlags.GetSize() == 0);
	require(ivChunkBeginIndexes.GetSize() == 0);
	require(nSlaveNumber >= 1);
	require(lMaxFileSizePerProcess > 0);

	// Affichage
	if (bDisplayChunkInformation)
	{
		cout << "ComputeAllChunksInformations\t" << nSlaveNumber << "\t" << lMaxFileSizePerProcess << endl;
		cout << "\tDatabase\t" << GetDatabaseIndexer()->GetObjectLabel() << endl;
		cout << "\tRaw chunks\t" << GetDatabaseIndexer()->GetChunkNumber() << endl;
		cout << "\tSlaveNumber\t" << nSlaveNumber << endl;
		cout << "\tMaxFileSizePerProcess\t" << lMaxFileSizePerProcess << endl;
		cout << "\tGetTotalUsedFileSize\t" << databaseIndexer->GetPLDatabase()->GetTotalUsedFileSize() << endl;
		cout << "\tChunk\tCurrentFileSize\tFileSizePerProcess\tCurrentMaxFileSizePerProcess" << endl;
	}

	// Calcul des index des tables utilisees
	ivUsedTableFlags.SetSize(databaseIndexer->GetTableNumber());
	if (databaseIndexer->IsMultiTableTechnology())
	{
		for (nMapping = 0; nMapping < databaseIndexer->GetMTDatabase()->GetTableNumber(); nMapping++)
		{
			if (databaseIndexer->GetMTDatabase()->GetUsedMappingAt(nMapping) != NULL)
				ivUsedTableFlags.SetAt(nMapping, 1);
		}
	}
	else
		ivUsedTableFlags.SetAt(0, 1);

	// Creation du premier chunk, avec l'index du premier micro-chunk
	ivChunkBeginIndexes.Add(0);

	// Creation potentielle d'autre chunks si possible
	if (nSlaveNumber > 1 and databaseIndexer->GetChunkNumber() > 1)
	{
		// Taille totale des fichiers a traiter
		lTotalUsedFileSize = databaseIndexer->GetPLDatabase()->GetTotalUsedFileSize();

		// Taille minimum a traiter par process
		lMinFileSizePerProcess = max(lMaxFileSizePerProcess / 8, (longint)SystemFile::nMinPreferredBufferSize);
		lMinFileSizePerProcess = min(lMinFileSizePerProcess, lMaxFileSizePerProcess);

		// Taille de fin de fichier a partir de laquelle on diminue la taille des chunks
		lEndFileSize = (nSlaveNumber + 1) * lMaxFileSizePerProcess / 2;

		// On introduit autant de points de coupure que necessaire, en calculer des chunks a traiter exploitant
		// la liste des micro-chunks disponibles dans le databaseIndexer
		// En debut et fin, ou diminue la taille des parties de fichier a traiter pour mieux repartir la charge,
		// de facon similaire a la methode ComputeStairBufferSize de PLParallelTask
		lFileSizePerProcess = 0;
		lCurrentTotalFileSize = 0;
		lCurrentMaxFileSizePerProcess = 0;
		for (nChunk = 0; nChunk < databaseIndexer->GetChunkNumber(); nChunk++)
		{
			// On determine si necessaire la taille de fichier a traiter pour le chunk en cours
			if (lCurrentMaxFileSizePerProcess == 0)
			{
				// On passe a des chunks de plus en plus petits a la fin, pour favoriser la fin
				// simultannee des taches
				if (lCurrentTotalFileSize > lTotalUsedFileSize - lEndFileSize)
				{
					// Calcul du pourcentage restant a traiter de la fin du fichier
					dRemainingEndFilePercentage =
					    (lTotalUsedFileSize - lCurrentTotalFileSize) * 1.0 / lEndFileSize;

					// On diminue la portion du fichier a traiter a mesure que l'on s'approche de la
					// fin
					lCurrentMaxFileSizePerProcess =
					    lMinFileSizePerProcess +
					    longint(dRemainingEndFilePercentage *
						    (lMaxFileSizePerProcess - lMinFileSizePerProcess));
				}
				// On echelonne la taille des chunks au debut, pour eviter les acces disques simultanens
				else if (GetChunkNumber() < nSlaveNumber)
					lCurrentMaxFileSizePerProcess =
					    (lMaxFileSizePerProcess * GetChunkNumber()) / nSlaveNumber;
				// Au milieu, on prend la taille standard
				else
					lCurrentMaxFileSizePerProcess = lMaxFileSizePerProcess;
			}

			// Ajout de la taille cumulee des micro-chunks a traiter
			for (nMapping = 0; nMapping < databaseIndexer->GetTableNumber(); nMapping++)
			{
				// Seules les tables effectivement utilisees sont a prendre en compte
				if (ivUsedTableFlags.GetAt(nMapping) == 1)
				{
					// Ajout de la taille a traiter: difference entre la position courante et
					// precedente
					assert(databaseIndexer->GetChunkEndPositionsAt(nChunk, nMapping) != -1);
					lFileSizePerProcess +=
					    databaseIndexer->GetChunkEndPositionsAt(nChunk, nMapping);
					lFileSizePerProcess -=
					    databaseIndexer->GetChunkBeginPositionsAt(nChunk, nMapping);
				}
			}

			// On cree un nouveau point de coupure si on a atteint l'objet de taille du chunk
			if (lFileSizePerProcess > lCurrentMaxFileSizePerProcess)
			{
				lCurrentTotalFileSize += lFileSizePerProcess;

				// Affichage des specifications de decoupage
				if (bDisplayChunkInformation)
				{
					cout << "\t" << ivChunkBeginIndexes.GetSize() << "\t" << lCurrentTotalFileSize
					     << "\t" << lFileSizePerProcess << "\t" << lCurrentMaxFileSizePerProcess
					     << endl;
				}

				// Memorisation d'un nouvel index de micro-chunk, sauf si on est sur le dernier
				// micro-chunk
				if (nChunk < databaseIndexer->GetChunkNumber() - 1)
					ivChunkBeginIndexes.Add(nChunk + 1);

				// Reinitialisation de la taille courante
				lFileSizePerProcess = 0;

				// REinitialisation de l'objectif de taille a atteindre
				lCurrentMaxFileSizePerProcess = 0;
			}
		}
	}
}
