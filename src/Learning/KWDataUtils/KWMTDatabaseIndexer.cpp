// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWMTDatabaseIndexer.h"
#include "KWClassDomain.h"

KWMTDatabaseIndexer::KWMTDatabaseIndexer()
{
	sourceDatabase = NULL;
	nChunkCurrentIndex = 0;
	bIsIndexationInterruptedByUser = false;
}

KWMTDatabaseIndexer::~KWMTDatabaseIndexer()
{
	assert(oaAllChunksBeginPos.GetSize() == 0);
	assert(oaAllChunksBeginRecordIndex.GetSize() == 0);
	assert(oaAllChunksLastRootKeys.GetSize() == 0);
}

boolean KWMTDatabaseIndexer::ComputeIndexation(const PLMTDatabaseTextFile* sourceMTDatabase, int nSlaveNumber,
					       longint lSlaveGrantedMemoryForSourceDatabase,
					       longint lForcedMaxTotalFileSizePerProcess)
{
	boolean bOk = true;
	ALString sTmp;

	require(sourceMTDatabase != NULL);
	require(sourceMTDatabase->Check());
	require(sourceMTDatabase->IsOpenInformationComputed());
	require(nSlaveNumber >= 1);
	require(lForcedMaxTotalFileSizePerProcess >= 0);
	require(sourceDatabase == NULL);

	// Calcul du plan d'indexation des tables du schema
	sourceDatabase = sourceMTDatabase;
	bOk = bOk and ComputeAllDataTableIndexation(nSlaveNumber, lSlaveGrantedMemoryForSourceDatabase,
						    lForcedMaxTotalFileSizePerProcess);
	sourceDatabase = NULL;
	return bOk;
}

boolean KWMTDatabaseIndexer::IsInterruptedByUser() const
{
	return bIsIndexationInterruptedByUser;
}

boolean KWMTDatabaseIndexer::IsComputed() const
{
	return oaAllChunksBeginPos.GetSize() > 0;
}

void KWMTDatabaseIndexer::CleanIndexation()
{
	nChunkCurrentIndex = 0;
	oaAllChunksBeginPos.DeleteAll();
	oaAllChunksBeginRecordIndex.DeleteAll();
	oaAllChunksLastRootKeys.DeleteAll();
}

int KWMTDatabaseIndexer::GetChunkNumber() const
{
	return oaAllChunksBeginRecordIndex.GetSize();
}

const LongintVector* KWMTDatabaseIndexer::GetChunkBeginPositionsAt(int nChunkIndex) const
{
	require(IsComputed());
	require(0 <= nChunkIndex and nChunkIndex < GetChunkNumber());
	return cast(LongintVector*, oaAllChunksBeginPos.GetAt(nChunkIndex));
}

const LongintVector* KWMTDatabaseIndexer::GetChunkEndPositionsAt(int nChunkIndex) const
{
	require(IsComputed());
	require(0 <= nChunkIndex and nChunkIndex < GetChunkNumber());
	return cast(LongintVector*, oaAllChunksBeginPos.GetAt(nChunkIndex + 1));
}

const LongintVector* KWMTDatabaseIndexer::GetChunkBeginRecordIndexesAt(int nChunkIndex) const
{
	require(IsComputed());
	require(0 <= nChunkIndex and nChunkIndex < GetChunkNumber());
	return cast(LongintVector*, oaAllChunksBeginRecordIndex.GetAt(nChunkIndex));
}

const KWKey* KWMTDatabaseIndexer::GetChunkLastRootKeyAt(int nChunkIndex) const
{
	require(IsComputed());
	require(0 <= nChunkIndex and nChunkIndex < GetChunkNumber());
	return cast(const KWKey*, oaAllChunksLastRootKeys.GetAt(nChunkIndex));
}

void KWMTDatabaseIndexer::Write(ostream& ost) const
{
	int nChunk;
	int nTable;
	int nTableNumber;

	ost << GetClassLabel() << "\n";
	if (IsComputed())
	{
		cout << "Chunks\t" << GetChunkNumber() << "\n";
		nTableNumber = 0;
		if (GetChunkNumber() > 0)
		{
			nTableNumber = GetChunkBeginPositionsAt(0)->GetSize();
			cout << "Tables\t" << nTableNumber << endl;

			// En tete
			ost << "Chunk\t";
			for (nTable = 0; nTable < nTableNumber; nTable++)
			{
				ost << "Record" << nTable + 1 << "\t";
				ost << "Begin" << nTable + 1 << "\t";
				ost << "End" << nTable + 1 << "\t";
			}
			ost << "LastKey\n";
		}

		// Detail par chunk
		for (nChunk = 0; nChunk < GetChunkNumber(); nChunk++)
		{
			ost << nChunk << "\t";
			for (nTable = 0; nTable < nTableNumber; nTable++)
			{
				ost << GetChunkBeginRecordIndexesAt(nChunk)->GetAt(nTable) << "\t";
				ost << GetChunkBeginPositionsAt(nChunk)->GetAt(nTable) << "\t";
				ost << GetChunkEndPositionsAt(nChunk)->GetAt(nTable) << "\t";
			}
			ost << GetChunkLastRootKeyAt(nChunk)->GetObjectLabel() << "\n";
		}
	}
}

const ALString KWMTDatabaseIndexer::GetClassLabel() const
{
	return "Database indexer";
}

const ALString KWMTDatabaseIndexer::GetObjectLabel() const
{
	if (sourceDatabase != NULL)
		return sourceDatabase->GetDatabaseName();
	else
		return "";
}

boolean KWMTDatabaseIndexer::ComputeAllDataTableIndexation(int nSlaveNumber,
							   longint lSlaveGrantedMemoryForSourceDatabase,
							   longint lForcedMaxTotalFileSizePerProcess)
{
	boolean bOk = true;
	boolean bDisplay = false;
	boolean bShowTime = false;
	const int nMaxProcessBySlave = 5;
	int nMapping;
	KWMTDatabaseMapping* mapping;
	KWClass* kwcRootClass;
	boolean bIsRootClassWithKey;
	longint lMaxFileSizePerProcess;
	ObjectArray oaRootKeys;
	ObjectArray oaAllTableFoundKeyPositions;
	ObjectArray* oaFoundKeyPositions;
	Timer timer;
	Timer timerGlobal;

	require(sourceDatabase->Check());
	require(sourceDatabase->IsOpenInformationComputed());
	require(nSlaveNumber >= 1);
	require(lSlaveGrantedMemoryForSourceDatabase > 0);
	require(lForcedMaxTotalFileSizePerProcess >= 0);
	require(
	    KWClassDomain::GetCurrentDomain()->LookupClass(sourceDatabase->GetClassName())->GetKeyAttributeNumber() >
		0 or
	    sourceDatabase->GetTableNumber() > 1);
	require(oaAllChunksBeginPos.GetSize() == 0);
	require(oaAllChunksBeginRecordIndex.GetSize() == 0);
	require(oaAllChunksLastRootKeys.GetSize() == 0);

	// Initialisation du resultat
	CleanIndexation();
	bIsIndexationInterruptedByUser = false;

	// Demarrage du timer global
	timerGlobal.Start();

	// Affichage des infos sur les tables utilisees
	if (bDisplay)
	{
		cout << GetClassLabel() << " " << GetObjectLabel() << ": "
		     << "ComputeAllDataTableIndexation" << endl;
		cout << "  Slave number: " << nSlaveNumber << endl;
		cout << "  Slave granted memory for source database: " << lSlaveGrantedMemoryForSourceDatabase << endl;
		cout << "  Forced max total file size per process: " << lForcedMaxTotalFileSizePerProcess << endl;
		cout << "  Read data tables (Min open size: " << sourceDatabase->GetMinOpenNecessaryMemory() << ")"
		     << endl;
		for (nMapping = 0; nMapping < sourceDatabase->GetUsedMappings()->GetSize(); nMapping++)
		{
			mapping = cast(KWMTDatabaseMapping*, sourceDatabase->GetUsedMappings()->GetAt(nMapping));
			if (mapping != NULL)
				cout << "    " << mapping->GetDataPath() << " "
				     << FileService::GetFileName(mapping->GetDataTableName()) << " ("
				     << sourceDatabase->GetFileSizes()->GetAt(nMapping) << ")" << endl;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// Collecte d'un echantillon de cle et de position pour la table principale et les tables secondaires

	// Evaluation de la taille max totale des fichiers a traiter par process
	lMaxFileSizePerProcess = 0;
	if (bOk)
	{
		// Initialisation a partir des caracteristiques de la base source
		// On soustrait par rapport au cas vide pour ne pas tenir compte des dictionnaires et tables externes,
		// et se focaliser ainsi sur les buffers pour l'ensenble de la base
		lMaxFileSizePerProcess =
		    max((longint)BufferedFile::nDefaultBufferSize,
			(lSlaveGrantedMemoryForSourceDatabase - sourceDatabase->GetEmptyOpenNecessaryMemory()) / 2);

		// On limite le nombre de process par esclave en cas de base trop petite
		if (lMaxFileSizePerProcess * nSlaveNumber * nMaxProcessBySlave > sourceDatabase->GetTotalUsedFileSize())
			lMaxFileSizePerProcess =
			    max((longint)BufferedFile::nDefaultBufferSize,
				sourceDatabase->GetTotalUsedFileSize() / (nSlaveNumber * nMaxProcessBySlave));

		// On peut imposer la taille du buffer pour raison de tests
		if (lForcedMaxTotalFileSizePerProcess > 0)
			lMaxFileSizePerProcess = lForcedMaxTotalFileSizePerProcess;
	}

	// On determine si on est dans du "vrai" multi-table, avec une table principale comportant une cle
	// On peut en effet avoir une table principale sans cle, utilisant une table externe pour enrichir sa
	// representation
	kwcRootClass = KWClassDomain::GetCurrentDomain()->LookupClass(sourceDatabase->GetClassName());
	check(kwcRootClass);
	bIsRootClassWithKey = kwcRootClass->GetKeyAttributeNumber() > 0;

	// Indexation des tables uniquement si au moins deux esclaves, dans le cas ou la table principale ne contient
	// pas de cle
	if (bOk and not bIsRootClassWithKey and nSlaveNumber >= 2)
	{
		// Extraction des positions, avec des cle vides
		oaFoundKeyPositions = new ObjectArray;
		bOk = ExtractMonoTableRootSampleKeyPositions(oaFoundKeyPositions);

		// Memorisation
		oaAllTableFoundKeyPositions.SetSize(sourceDatabase->GetTableNumber());
		if (bOk)
			oaAllTableFoundKeyPositions.SetAt(0, oaFoundKeyPositions);

		// Collecte des cles a partir des positions extraites de la table racine
		if (bOk)
			KWKeyPosition::CollectKeys(oaFoundKeyPositions, &oaRootKeys);
	}

	// Indexation des tables uniquement si au moins deux esclaves, dans le cas ou la table principale contient une
	// cle
	if (bOk and bIsRootClassWithKey and nSlaveNumber >= 2)
	{
		longint lRootMeanKeySize;
		longint lRootLineNumber;
		longint lAllKeyPositionMemory;
		longint lMaxKeyNumber;
		longint lMaxSlaveProcessNumber;
		KWKeyFieldsIndexer rootKeyFieldsIndexer;
		double dSamplingRate;

		// Initialisation d'un indexeur de cle pour la table principale
		timer.Reset();
		timer.Start();
		bOk = bOk and
		      InitializeKeyFieldIndexer(cast(KWMTDatabaseMapping*, sourceDatabase->GetUsedMappings()->GetAt(0)),
						&rootKeyFieldsIndexer);

		// Evaluation de la taille des cles et du nombre de lignes de la table racine
		lRootMeanKeySize = 0;
		lRootLineNumber = 0;
		bOk = bOk and EvaluateKeySize(&rootKeyFieldsIndexer, lRootMeanKeySize, lRootLineNumber);
		if (bShowTime)
			cout << GetClassLabel() << "\tEvaluate key size\t" << timer.GetElapsedTime() << endl;

		// Determination du taux d'echantillonnage des cles
		dSamplingRate = 0;
		if (bOk)
		{
			// Calcul de la taille totale necessaire pour stocker une cle pour toutes les tables
			// La cle n'est stockee que pour la table racine, et au plus une fois par table secondaire
			// effectivement utilisee avant d'etre nettoyee
			lAllKeyPositionMemory = 2 * lRootMeanKeySize + sizeof(KWKey) +
						sourceDatabase->GetUsedMappings()->GetSize() * sizeof(Object*) +
						sourceDatabase->GetUsedMappingNumber() * sizeof(KWKeyPosition);

			// Calcul du nombre max de cles et de leur position par table utilise, en fonction de la memoire
			// disponible
			lMaxKeyNumber =
			    (RMResourceManager::GetRemainingAvailableMemory() - 2 * BufferedFile::nDefaultBufferSize) /
			    lAllKeyPositionMemory;
			if (lMaxKeyNumber < 0)
				lMaxKeyNumber = 0;

			// On limite ce nombre de cle on fonction de la taille des fichiers a analyser
			// On utilise (lMaxFileSizePerProcess/8) pour la gestion de la fin des taches (cf.
			// ComputeAllChunksInformations)
			lMaxSlaveProcessNumber =
			    1 + sourceDatabase->GetTotalUsedFileSize() / (lMaxFileSizePerProcess / 8);
			if (lMaxKeyNumber > 10 * lMaxSlaveProcessNumber)
				lMaxKeyNumber = 10 * lMaxSlaveProcessNumber;

			// On en deduit le taux d'echantillonnage max
			dSamplingRate = lMaxKeyNumber * 1.0 / lRootLineNumber;
			if (dSamplingRate > 1)
				dSamplingRate = 1;
			if (dSamplingRate * lRootLineNumber < 10)
				dSamplingRate = 0;
			if (lMaxFileSizePerProcess >= sourceDatabase->GetTotalUsedFileSize())
				dSamplingRate = 0;
			if (bDisplay)
			{
				cout << "  All tables size: " << sourceDatabase->GetTotalUsedFileSize() << endl;
				cout << "  Max file size per process: " << lMaxFileSizePerProcess << endl;
				cout << "  Max slave process number: " << lMaxSlaveProcessNumber << endl;
				cout << "  Max key number: " << lMaxKeyNumber << endl;
				cout << "  Sampling rate: " << dSamplingRate << endl;
			}
		}

		// Extraction des cles de la table principale
		oaAllTableFoundKeyPositions.SetSize(sourceDatabase->GetTableNumber());
		if (bOk and dSamplingRate > 0)
		{
			timer.Reset();
			timer.Start();

			// Initialisation du tableau resultat
			oaFoundKeyPositions = new ObjectArray;
			oaAllTableFoundKeyPositions.SetAt(0, oaFoundKeyPositions);

			// Extraction des cles et de leur position de la table racine
			bOk = ExtractRootSampleKeyPositions(&rootKeyFieldsIndexer, lRootMeanKeySize, lRootLineNumber,
							    dSamplingRate, oaFoundKeyPositions);

			// Collecte des cles a partir des positions extraites de la table racine
			if (bOk)
				KWKeyPosition::CollectKeys(oaFoundKeyPositions, &oaRootKeys);
			if (bDisplay)
				cout << "  Extracted keys: " << oaRootKeys.GetSize() << endl;
			if (bShowTime)
				cout << GetClassLabel() << "\tExtract root keys\t" << timer.GetElapsedTime() << endl;
		}

		// Extraction des positions des cles pour les tables secondaires
		timer.Reset();
		timer.Start();
		if (bOk and sourceDatabase->GetTableNumber() > 1 and oaRootKeys.GetSize() > 0)
		{
			assert(dSamplingRate > 0);
			bOk = ExtractSecondaryKeyPositions(&oaRootKeys, sourceDatabase->GetUsedMappings(),
							   &oaAllTableFoundKeyPositions);
		}

		// Affichage des nombre de position par mapping
		if (bDisplay)
		{
			// Parcours de mapping
			for (nMapping = 0; nMapping < sourceDatabase->GetUsedMappings()->GetSize(); nMapping++)
			{
				mapping =
				    cast(KWMTDatabaseMapping*, sourceDatabase->GetUsedMappings()->GetAt(nMapping));
				if (mapping != NULL)
				{
					oaFoundKeyPositions =
					    cast(ObjectArray*, oaAllTableFoundKeyPositions.GetAt(nMapping));

					// Affichage du nombre de position
					cout << "\t" << mapping->GetDataPath() << "\t";
					if (oaFoundKeyPositions == NULL)
						cout << "NULL" << endl;
					else
						cout << oaFoundKeyPositions->GetSize() << endl;
				}
			}
		}
		if (bShowTime)
			cout << GetClassLabel() << "\tExtract secondary key positions\t" << timer.GetElapsedTime()
			     << endl;
	}

	// Calcul prealable des vecteurs de positions de depart des fichiers multi-table pour decouper
	// la tache de transfert en sous-taches confiees aux esclaves
	if (bOk)
	{
		timer.Reset();
		timer.Start();
		if (bOk)
		{
			assert(oaRootKeys.GetSize() == 0 or
			       oaAllTableFoundKeyPositions.GetSize() == sourceDatabase->GetUsedMappings()->GetSize());
			bOk = ComputeAllChunksInformations(
			    &oaRootKeys, &oaAllTableFoundKeyPositions, sourceDatabase->GetUsedMappings(),
			    sourceDatabase->GetFileSizes(), nSlaveNumber, lMaxFileSizePerProcess);
		}
		if (bShowTime)
			cout << GetClassLabel() << "\tCompute chunk information\t" << timer.GetElapsedTime() << endl;
	}

	// Nettoyage
	nChunkCurrentIndex = 0;
	for (nMapping = 0; nMapping < oaAllTableFoundKeyPositions.GetSize(); nMapping++)
	{
		oaFoundKeyPositions = cast(ObjectArray*, oaAllTableFoundKeyPositions.GetAt(nMapping));

		// Attention, les positions n'ont ete collectees que pour les tables a lire effectivement
		if (oaFoundKeyPositions != NULL)
			oaFoundKeyPositions->DeleteAll();
	}
	oaAllTableFoundKeyPositions.DeleteAll();
	if (not bOk)
		CleanIndexation();
	if (bShowTime)
		cout << GetClassLabel() << "\tCalcul du plan d'indexation global\t" << timerGlobal.GetElapsedTime()
		     << endl;
	ensure(not bOk or oaAllChunksBeginPos.GetSize() > 0);
	ensure(not bOk or oaAllChunksBeginPos.GetSize() == oaAllChunksBeginRecordIndex.GetSize() + 1);
	ensure(not bOk or oaAllChunksLastRootKeys.GetSize() == oaAllChunksBeginRecordIndex.GetSize());
	return bOk;
}

boolean KWMTDatabaseIndexer::ExtractMonoTableRootSampleKeyPositions(ObjectArray* oaRootKeyPositions)
{
	boolean bOk = true;
	KWFileIndexerTask fileIndexerTask;
	LongintVector lvFileBeginPositions;
	LongintVector lvFileBeginRecordIndexes;
	KWKeyPosition* recordKeyPosition;
	int i;

	require(oaRootKeyPositions != NULL);
	require(oaRootKeyPositions->GetSize() == 0);

	// Calcul de l'indexation avec la taille de buffer par defaut
	fileIndexerTask.SetFileName(sourceDatabase->GetDatabaseName());
	bOk = fileIndexerTask.ComputeIndexation(InputBufferedFile::nDefaultBufferSize, &lvFileBeginPositions,
						&lvFileBeginRecordIndexes);
	bIsIndexationInterruptedByUser = fileIndexerTask.IsTaskInterruptedByUser();

	// Parcours des positions pour creer des objet cle (dans champ) et des positions (sans cle)
	if (bOk)
	{
		for (i = 0; i < lvFileBeginPositions.GetSize(); i++)
		{
			recordKeyPosition = new KWKeyPosition;
			recordKeyPosition->SetLinePosition(lvFileBeginPositions.GetAt(i));
			recordKeyPosition->SetLineIndex(lvFileBeginRecordIndexes.GetAt(i));
			oaRootKeyPositions->Add(recordKeyPosition);
		}
	}
	return bOk;
}

boolean KWMTDatabaseIndexer::InitializeKeyFieldIndexer(const KWMTDatabaseMapping* readMapping,
						       KWKeyFieldsIndexer* keyFieldsIndexer)
{
	boolean bOk = true;
	KWClass* kwcRootClass;
	KWClass* kwcMappingClass;
	StringVector svFirstLine;

	require(readMapping != NULL);
	require(sourceDatabase->LookupMultiTableMapping(readMapping->GetDataPath()) == readMapping);
	require(keyFieldsIndexer != NULL);

	// Recherche de la classe racine
	kwcRootClass = KWClassDomain::GetCurrentDomain()->LookupClass(sourceDatabase->GetClassName());
	check(kwcRootClass);

	// Recherche de la table associee au mapping
	kwcMappingClass = KWClassDomain::GetCurrentDomain()->LookupClass(readMapping->GetClassName());
	check(kwcMappingClass);

	// Export des noms des attributs cle et natif
	if (bOk)
	{
		kwcMappingClass->ExportNativeFieldNames(keyFieldsIndexer->GetNativeFieldNames());
		kwcMappingClass->ExportKeyAttributeNames(keyFieldsIndexer->GetKeyAttributeNames());

		// On restreint les champs cle a ceux correspondant a la classe racine
		keyFieldsIndexer->GetKeyAttributeNames()->SetSize(kwcRootClass->GetKeyAttributeNumber());
	}

	// Extraction des champs de la premiere ligne du fichier d'entree
	if (bOk)
		bOk = InputBufferedFile::GetFirstLineFields(readMapping->GetDataTableName(),
							    sourceDatabase->GetFieldSeparator(), &svFirstLine, this);

	// Calcul des index des champs de la cle (on est ici dans le cas multi-tables)
	if (bOk)
	{
		bOk = keyFieldsIndexer->ComputeKeyFieldIndexes(sourceDatabase->GetHeaderLineUsed(), &svFirstLine);
		if (not bOk)
			AddError("Error while indexing fields of table " + readMapping->GetDataTableName());
	}
	return bOk;
}

boolean KWMTDatabaseIndexer::EvaluateKeySize(const KWKeyFieldsIndexer* rootKeyFieldsIndexer, longint& lRootMeanKeySize,
					     longint& lRootLineNumber)
{
	boolean bOk = true;
	boolean bTrace = false;
	KWKeySizeEvaluatorTask keySizeEvaluator;

	require(rootKeyFieldsIndexer != NULL);
	require(not bIsIndexationInterruptedByUser);

	// Evaluation de la taille des cles a partir de la table racine
	lRootMeanKeySize = 0;
	lRootLineNumber = 0;
	if (bOk)
	{
		bOk = keySizeEvaluator.EvaluateKeySize(
		    rootKeyFieldsIndexer->GetConstKeyFieldIndexes(), sourceDatabase->GetDatabaseName(),
		    sourceDatabase->GetHeaderLineUsed(), sourceDatabase->GetFieldSeparator(), lRootMeanKeySize,
		    lRootLineNumber);
		bIsIndexationInterruptedByUser = keySizeEvaluator.IsTaskInterruptedByUser();
	}
	if (bTrace)
	{
		cout << "  Mean key size: " << lRootMeanKeySize << endl;
		cout << "  Mean line number: " << lRootLineNumber << endl;
	}
	return bOk;
}

boolean KWMTDatabaseIndexer::ExtractRootSampleKeyPositions(const KWKeyFieldsIndexer* rootKeyFieldsIndexer,
							   longint lRootMeanKeySize, longint lRootLineNumber,
							   double dSamplingRate, ObjectArray* oaRootKeyPositions)
{
	boolean bOk = true;
	boolean bTrace = false;
	KWKeyPositionSampleExtractorTask keyPositionSampleExtractor;

	require(not bIsIndexationInterruptedByUser);
	require(rootKeyFieldsIndexer != NULL);
	require(oaRootKeyPositions != NULL);
	require(oaRootKeyPositions->GetSize() == 0);
	require(lRootMeanKeySize > 0);
	require(lRootLineNumber > 0);
	require(dSamplingRate > 0);

	// Extraction des cles
	keyPositionSampleExtractor.SetFileName(sourceDatabase->GetDatabaseName());
	keyPositionSampleExtractor.SetHeaderLineUsed(sourceDatabase->GetHeaderLineUsed());
	keyPositionSampleExtractor.SetFieldSeparator(sourceDatabase->GetFieldSeparator());
	keyPositionSampleExtractor.SetSamplingRate(dSamplingRate);
	keyPositionSampleExtractor.SetKeyUsedMemory(lRootMeanKeySize);
	keyPositionSampleExtractor.SetFileLineNumber(lRootLineNumber);
	keyPositionSampleExtractor.GetKeyFieldIndexes()->CopyFrom(rootKeyFieldsIndexer->GetConstKeyFieldIndexes());
	bOk = keyPositionSampleExtractor.ExtractSample(oaRootKeyPositions);
	bIsIndexationInterruptedByUser = keyPositionSampleExtractor.IsTaskInterruptedByUser();

	// Affichage des resultats
	if (bTrace)
		cout << "Extracted root key positions: " << oaRootKeyPositions->GetSize() << " in "
		     << FileService::GetFileName(sourceDatabase->GetDatabaseName()) << endl;
	return bOk;
}

boolean KWMTDatabaseIndexer::ExtractSecondaryKeyPositions(const ObjectArray* oaRootKeys,
							  const ObjectArray* oaUsedReadMappings,
							  ObjectArray* oaAllTableFoundKeyPositions)
{
	boolean bOk = true;
	boolean bTrace = false;
	int nMapping;
	KWMTDatabaseMapping* mapping;
	KWKeyPositionFinderTask keyPositionFinder;
	KWClass* kwcRootClass;
	KWClass* kwcClass;
	StringVector svFirstLine;
	KWKeyFieldsIndexer keyFieldsIndexer;
	ObjectArray* oaFoundKeyPositions;

	require(not bIsIndexationInterruptedByUser);
	require(oaRootKeys != NULL);
	require(oaUsedReadMappings != NULL);
	require(oaAllTableFoundKeyPositions != NULL);
	require(oaAllTableFoundKeyPositions->GetSize() == oaUsedReadMappings->GetSize());
	require(oaAllTableFoundKeyPositions->GetAt(0) != NULL);

	// Recherche de la classe associee au mapping
	kwcRootClass = KWClassDomain::GetCurrentDomain()->LookupClass(sourceDatabase->GetClassName());
	check(kwcRootClass);

	// Parcours des mapping (sauf celui de la table racine)
	for (nMapping = 1; nMapping < oaUsedReadMappings->GetSize(); nMapping++)
	{
		mapping = cast(KWMTDatabaseMapping*, oaUsedReadMappings->GetAt(nMapping));

		// On ne traite que les mapping en lecture effectivement utilises
		assert(oaAllTableFoundKeyPositions->GetAt(nMapping) == NULL);
		if (mapping != NULL)
		{
			// Initialisation du tableau resultat
			oaFoundKeyPositions = new ObjectArray;
			oaAllTableFoundKeyPositions->SetAt(nMapping, oaFoundKeyPositions);

			// Recherche de la classe associee au mapping
			kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(mapping->GetClassName());
			check(kwcClass);

			// Export des noms des attributs cle et natif
			if (bOk)
			{
				kwcClass->ExportKeyAttributeNames(keyFieldsIndexer.GetKeyAttributeNames());
				kwcClass->ExportNativeFieldNames(keyFieldsIndexer.GetNativeFieldNames());

				// On ne s'interesse qu'aux champs cle commun avec ceux la la table racine
				keyFieldsIndexer.GetKeyAttributeNames()->SetSize(kwcRootClass->GetKeyAttributeNumber());
			}

			// Extraction des champs de la premiere ligne du fichier d'entree
			if (bOk)
				bOk = InputBufferedFile::GetFirstLineFields(mapping->GetDataTableName(),
									    sourceDatabase->GetFieldSeparator(),
									    &svFirstLine, this);

			// Calcul des index des champs de la cle (on est ici dans le cas multi-tables)
			if (bOk)
			{
				bOk = keyFieldsIndexer.ComputeKeyFieldIndexes(sourceDatabase->GetHeaderLineUsed(),
									      &svFirstLine);
				if (not bOk)
					AddError("Error while indexing fields of table " + mapping->GetDataTableName());
			}

			// Extraction des cles
			if (bOk)
			{
				keyPositionFinder.SetFileName(mapping->GetDataTableName());
				keyPositionFinder.SetHeaderLineUsed(sourceDatabase->GetHeaderLineUsed());
				keyPositionFinder.SetFieldSeparator(sourceDatabase->GetFieldSeparator());
				keyPositionFinder.GetKeyFieldIndexes()->CopyFrom(
				    keyFieldsIndexer.GetConstKeyFieldIndexes());
				bOk = bOk and keyPositionFinder.FindKeyPositions(oaRootKeys, oaFoundKeyPositions);
				bIsIndexationInterruptedByUser = keyPositionFinder.IsTaskInterruptedByUser();
				if (bTrace)
					cout << "Found key positions: " << oaFoundKeyPositions->GetSize() << " in "
					     << mapping->GetDataPath() << endl;
			}

			// Nettoyage des cles pour gagner en memoire
			KWKeyPosition::CleanKeys(oaFoundKeyPositions);

			// Arret si erreur
			if (not bOk)
				break;
		}
	}
	return bOk;
}

boolean KWMTDatabaseIndexer::ComputeAllChunksInformations(const ObjectArray* oaRootKeys,
							  const ObjectArray* oaAllTableFoundKeyPositions,
							  const ObjectArray* oaUsedReadMappings,
							  const LongintVector* lvDataTableSizes, int nSlaveNumber,
							  longint lMaxFileSizePerProcess)
{
	boolean bOk = true;
	boolean bDisplayChunkInformation = false;
	int nMapping;
	KWMTDatabaseMapping* mapping;
	ObjectArray* oaFoundKeyPositions;
	int nKey;
	KWKeyPosition* keyPosition;
	KWKeyPosition* keyPreviousPosition;
	LongintVector* lvDataTableBeginPos;
	LongintVector* lvDataTableEndPos;
	LongintVector* lvDataTableBeginRecordIndex;
	KWKey* lastRootKey;
	longint lFileSizePerProcess;
	longint lCurrentMaxFileSizePerProcess;
	longint lTotalFileSize;
	longint lCurrentTotalFileSize;

	require(oaRootKeys != NULL);
	require(oaAllTableFoundKeyPositions != NULL);
	require(oaUsedReadMappings != NULL);
	require(sourceDatabase->GetTableNumber() == oaUsedReadMappings->GetSize());
	require(oaRootKeys->GetSize() == 0 or oaUsedReadMappings->GetSize() == oaAllTableFoundKeyPositions->GetSize());
	require(lvDataTableSizes != NULL);
	require(oaRootKeys->GetSize() == 0 or lvDataTableSizes->GetSize() == oaAllTableFoundKeyPositions->GetSize());
	require(nSlaveNumber >= 1);
	require(lMaxFileSizePerProcess > 0);

	// Creation du premier vecteur de positions, avec des 0 partout
	lvDataTableBeginPos = new LongintVector;
	lvDataTableBeginPos->SetSize(sourceDatabase->GetTableNumber());
	oaAllChunksBeginPos.Add(lvDataTableBeginPos);

	// Creation du premier vecteur d'index de record, avec des 0 partout
	lvDataTableBeginRecordIndex = new LongintVector;
	lvDataTableBeginRecordIndex->SetSize(sourceDatabase->GetTableNumber());
	oaAllChunksBeginRecordIndex.Add(lvDataTableBeginRecordIndex);

	// Creation d'une nouvelle cle racine
	lastRootKey = new KWKey;
	oaAllChunksLastRootKeys.Add(lastRootKey);

	// Cas ou il y a des donnees d'indexation: on cree des coupures intermediaires
	if (oaRootKeys->GetSize() > 0)
	{
		// Calcul de la taille totale des fichiers a traiter
		lTotalFileSize = 0;
		for (nMapping = 0; nMapping < oaUsedReadMappings->GetSize(); nMapping++)
		{
			mapping = cast(KWMTDatabaseMapping*, oaUsedReadMappings->GetAt(nMapping));
			if (mapping != NULL)
				lTotalFileSize += lvDataTableSizes->GetAt(nMapping);
		}

		// On introduit autant de points de coupure que necessaire
		lFileSizePerProcess = 0;
		lCurrentTotalFileSize = 0;
		for (nKey = 0; nKey < oaRootKeys->GetSize(); nKey++)
		{
			// Ajout de la taille a traiter
			for (nMapping = 0; nMapping < oaAllTableFoundKeyPositions->GetSize(); nMapping++)
			{
				oaFoundKeyPositions = cast(ObjectArray*, oaAllTableFoundKeyPositions->GetAt(nMapping));

				// Seules les tables effectivement utilisees ont des positions de cle
				if (oaFoundKeyPositions != NULL)
				{
					// Ajout de la taille a traiter: difference entre la position courante et
					// precedente
					keyPosition = cast(KWKeyPosition*, oaFoundKeyPositions->GetAt(nKey));
					lFileSizePerProcess += keyPosition->GetLinePosition();
					if (nKey > 0)
					{
						keyPreviousPosition =
						    cast(KWKeyPosition*, oaFoundKeyPositions->GetAt(nKey - 1));
						lFileSizePerProcess -= keyPreviousPosition->GetLinePosition();
					}
				}
			}

			// On passe a des chunks de plus en plus petit a la fin, pour favoriser la fin simultannee des
			// taches
			if (lCurrentTotalFileSize > lTotalFileSize - 3 * nSlaveNumber * lMaxFileSizePerProcess / 2)
			{
				if (lCurrentTotalFileSize > lTotalFileSize - nSlaveNumber * lMaxFileSizePerProcess / 4)
					lCurrentMaxFileSizePerProcess = lMaxFileSizePerProcess / 8;
				else if (lCurrentTotalFileSize > lTotalFileSize - nSlaveNumber * lMaxFileSizePerProcess)
					lCurrentMaxFileSizePerProcess = lMaxFileSizePerProcess / 4;
				else
					lCurrentMaxFileSizePerProcess = lMaxFileSizePerProcess / 2;
			}
			// On echelonne la taille des chunks au debut, pour eviter les acces disques simultanes
			else if (oaAllChunksBeginPos.GetSize() < nSlaveNumber)
				lCurrentMaxFileSizePerProcess = lMaxFileSizePerProcess *
								(oaAllChunksBeginPos.GetSize() - 1 + nSlaveNumber) /
								(2 * nSlaveNumber);
			// Au milieu, on prend la taille standard
			else
				lCurrentMaxFileSizePerProcess = lMaxFileSizePerProcess;

			// On cree un nouveau point de coupure si necessaire
			if (lFileSizePerProcess > lCurrentMaxFileSizePerProcess)
			{
				lCurrentTotalFileSize += lFileSizePerProcess;

				// Affichage des specifications de decoupage
				if (bDisplayChunkInformation)
				{
					cout << "Chunk\t" << oaAllChunksBeginPos.GetSize() << "\t"
					     << lCurrentTotalFileSize << "\t" << lFileSizePerProcess << "\t"
					     << lCurrentMaxFileSizePerProcess << endl;
				}

				// Creation du nouveau vecteur de positions
				lvDataTableBeginPos = new LongintVector;
				lvDataTableBeginPos->SetSize(sourceDatabase->GetTableNumber());
				oaAllChunksBeginPos.Add(lvDataTableBeginPos);

				// Creation du nouveau vecteur d'index de record
				lvDataTableBeginRecordIndex = new LongintVector;
				lvDataTableBeginRecordIndex->SetSize(sourceDatabase->GetTableNumber());
				oaAllChunksBeginRecordIndex.Add(lvDataTableBeginRecordIndex);

				// Creation d'une nouvelle cle racine
				lastRootKey = new KWKey;
				oaAllChunksLastRootKeys.Add(lastRootKey);

				// Memorisation de la cle racine
				lastRootKey->CopyFrom(cast(KWKey*, oaRootKeys->GetAt(nKey)));

				// Memorisation des index de coupure, en parcourant toutes les tables, utilisee ou non
				for (nMapping = 0; nMapping < oaUsedReadMappings->GetSize(); nMapping++)
				{
					mapping = cast(KWMTDatabaseMapping*, oaUsedReadMappings->GetAt(nMapping));

					// On ne traite que les mapping effectivement utilisee en lecture
					if (mapping != NULL)
					{
						// Recherche de la position courante pour le mapping
						oaFoundKeyPositions =
						    cast(ObjectArray*, oaAllTableFoundKeyPositions->GetAt(nMapping));
						check(oaFoundKeyPositions);
						keyPosition = cast(KWKeyPosition*, oaFoundKeyPositions->GetAt(nKey));

						// Memorisation de la position et de l'index de la ligne
						lvDataTableBeginPos->SetAt(nMapping, keyPosition->GetLinePosition());
						lvDataTableBeginRecordIndex->SetAt(nMapping,
										   keyPosition->GetLineIndex());
					}
				}

				// Reinitialisation de la taille courante
				lFileSizePerProcess = 0;
			}
		}
	}

	// Creation du dernier vecteur de position, avec les tailles de fichier
	lvDataTableBeginPos = new LongintVector;
	oaAllChunksBeginPos.Add(lvDataTableBeginPos);
	lvDataTableBeginPos->SetSize(sourceDatabase->GetTableNumber());
	for (nMapping = 0; nMapping < oaUsedReadMappings->GetSize(); nMapping++)
	{
		mapping = cast(KWMTDatabaseMapping*, oaUsedReadMappings->GetAt(nMapping));
		if (mapping != NULL)
			lvDataTableBeginPos->SetAt(nMapping, lvDataTableSizes->GetAt(nMapping));
	}

	// Affichage des position calculees
	if (bDisplayChunkInformation)
	{
		cout << "Positions:" << endl;
		for (nKey = 0; nKey < oaAllChunksBeginRecordIndex.GetSize(); nKey++)
		{
			lvDataTableBeginPos = cast(LongintVector*, oaAllChunksBeginPos.GetAt(nKey));
			lvDataTableEndPos = cast(LongintVector*, oaAllChunksBeginPos.GetAt(nKey + 1));
			lvDataTableBeginRecordIndex = cast(LongintVector*, oaAllChunksBeginRecordIndex.GetAt(nKey));

			// Affichage de la cle
			cout << nKey << "\t" << *oaAllChunksLastRootKeys.GetAt(nKey) << endl;

			// Affichage des positions par table
			lFileSizePerProcess = 0;
			for (nMapping = 0; nMapping < oaUsedReadMappings->GetSize(); nMapping++)
			{
				mapping = cast(KWMTDatabaseMapping*, oaUsedReadMappings->GetAt(nMapping));
				if (mapping != NULL)
				{
					cout << "\t" << lvDataTableBeginPos->GetAt(nMapping) << "\t"
					     << lvDataTableEndPos->GetAt(nMapping) << "\t"
					     << lvDataTableBeginRecordIndex->GetAt(nMapping) << "\t"
					     << mapping->GetDataPath() << endl;
				}

				// Calcul de la taille traitee
				if (nKey > 0)
					lFileSizePerProcess +=
					    lvDataTableBeginPos->GetAt(nMapping) -
					    cast(LongintVector*, oaAllChunksBeginPos.GetAt(nKey - 1))->GetAt(nMapping);
			}

			// Affichage de la taille traitee
			cout << "\tSize\t" << lFileSizePerProcess << endl;
		}
	}
	return bOk;
}