// Copyright (c) 2024 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDatabaseIndexer.h"

KWDatabaseIndexer::KWDatabaseIndexer()
{
	nMainTableNumber = 0;
	bHasMainTableKeys = false;
	nResourceSlaveNumber = 1;
	nMaxSlaveNumber = 0;
	lMaxIndexationMemory = 64 * lMB;
	lMaxTotalFileSizePerProcess = 0;
	bIsIndexationInterruptedByUser = false;
}

KWDatabaseIndexer::~KWDatabaseIndexer()
{
	CleanIndexation();
}

void KWDatabaseIndexer::InitializeFromDatabase(const KWDatabase* database)
{
	KWClass* kwcMainClass;
	RMTaskResourceGrant defaultGrantedResources;
	RMTaskResourceRequirement defaultRequirements;

	require(database == NULL or database->Check());

	// Nettoyage prealable si necessaire
	if (database == NULL or (sourcePLDatabase.IsInitialized() and not HasDatabaseSameStructure(database)))
	{
		nMainTableNumber = 0;
		bHasMainTableKeys = false;
		sourcePLDatabase.Reset();
		CleanIndexation();
	}

	// Initialisation si base non nulle
	if (database != NULL)
	{
		// Recalcul des informations d'initialisation si necessaire
		if (not sourcePLDatabase.IsInitialized())
		{
			sourcePLDatabase.InitializeFrom(database);

			// Recherche de la classe racine
			kwcMainClass = KWClassDomain::GetCurrentDomain()->LookupClass(database->GetClassName());
			check(kwcMainClass);

			// Memorisation de la presence de cle dans la table principale
			bHasMainTableKeys = (kwcMainClass->GetKeyAttributeNumber() > 0);

			// Memorisation du nombre de tables principales pour eviter de le recalculer sans arret
			if (IsMultiTableTechnology())
				nMainTableNumber = sourcePLDatabase.GetMTDatabase()->GetMainTableNumber();
			else
				nMainTableNumber = 1;
		}

		// Recopie des specifications generiques de la base en entree
		sourcePLDatabase.GetDatabase()->KWDatabase::CopyFrom(database);

		// On recopie les modes silencieux et verbeux de la base d'origine, qui sont a leur valeur par defaut
		// sinon
		sourcePLDatabase.GetDatabase()->SetSilentMode(database->GetSilentMode());
		sourcePLDatabase.GetDatabase()->SetVerboseMode(database->GetVerboseMode());

		// On nettoie les informations d'ouverture, pour reforcer leur calcul
		sourcePLDatabase.CleanOpenInformation();
	}

	// Memorisation du nombre d'esclaves utilisables au moment de l'indexation
	// Pour cela, les ressources obtenues pour des exigences par defaut fournissent le nombre total d'esclaves disponibles
	// (sur un cluster le nombre d'esclave est different du nombre de processus -1)

	RMParallelResourceManager::ComputeGrantedResources(&defaultRequirements, &defaultGrantedResources);
	nResourceSlaveNumber = defaultGrantedResources.GetSlaveNumber();
}

boolean KWDatabaseIndexer::IsInitialized() const
{
	return sourcePLDatabase.IsInitialized();
}

boolean KWDatabaseIndexer::IsMultiTableTechnology() const
{
	require(sourcePLDatabase.IsInitialized());
	return sourcePLDatabase.IsMultiTableTechnology();
}

const PLDatabaseTextFile* KWDatabaseIndexer::GetPLDatabase() const
{
	require(sourcePLDatabase.IsInitialized());
	return &sourcePLDatabase;
}

const PLSTDatabaseTextFile* KWDatabaseIndexer::GetSTDatabase() const
{
	require(sourcePLDatabase.IsInitialized());
	return sourcePLDatabase.GetSTDatabase();
}

const PLMTDatabaseTextFile* KWDatabaseIndexer::GetMTDatabase() const
{
	require(sourcePLDatabase.IsInitialized());
	return sourcePLDatabase.GetMTDatabase();
}

int KWDatabaseIndexer::GetTableNumber() const
{
	require(sourcePLDatabase.IsInitialized());
	if (not IsMultiTableTechnology())
		return 1;
	else
		return sourcePLDatabase.GetMTDatabase()->GetTableNumber();
}

void KWDatabaseIndexer::SetMaxSlaveNumber(int nValue)
{
	require(nValue >= 0);
	nMaxSlaveNumber = nValue;
	CleanIndexation();
}

int KWDatabaseIndexer::GetMaxSlaveNumber() const
{
	return nMaxSlaveNumber;
}

int KWDatabaseIndexer::GetResourceSlaveNumber() const
{
	return nResourceSlaveNumber;
}

int KWDatabaseIndexer::GetUsedSlaveNumber() const
{
	if (nMaxSlaveNumber > 0)
		return nMaxSlaveNumber;
	else
		return nResourceSlaveNumber;
}

void KWDatabaseIndexer::SetMaxIndexationMemory(longint lValue)
{
	require(lValue >= 0);
	lMaxIndexationMemory = lValue;
	CleanIndexation();
}

longint KWDatabaseIndexer::GetMaxIndexationMemory() const
{
	return lMaxIndexationMemory;
}

void KWDatabaseIndexer::SetMaxTotalFileSizePerProcess(longint lValue)
{
	require(lValue >= 0);
	lMaxTotalFileSizePerProcess = lValue;
	CleanIndexation();
}

longint KWDatabaseIndexer::GetMaxTotalFileSizePerProcess() const
{
	return lMaxTotalFileSizePerProcess;
}

boolean KWDatabaseIndexer::ComputeIndexation()
{
	boolean bOk = true;
	ALString sTmp;

	require(sourcePLDatabase.IsInitialized());
	require(sourcePLDatabase.Check());
	require(sourcePLDatabase.IsOpenInformationComputed());

	// Calcul du plan d'indexation des tables du schema
	bOk = ComputeAllDataTableIndexation();
	return bOk;
}

boolean KWDatabaseIndexer::IsInterruptedByUser() const
{
	return bIsIndexationInterruptedByUser;
}

boolean KWDatabaseIndexer::IsIndexationComputed() const
{
	// A minima, on doit avoir les index de debut et fin de fichier pour la table principale
	return oaTableRecordIndexVectors.GetSize() > 0;
}

void KWDatabaseIndexer::CleanIndexation()
{
	oaExtractedKeys.DeleteAll();
	oaTableRecordIndexVectors.DeleteAll();
	oaTableNextRecordPositionVectors.DeleteAll();
}

int KWDatabaseIndexer::GetChunkNumber() const
{
	require(IsIndexationComputed());
	require(oaTableRecordIndexVectors.GetAt(0) != NULL);

	return cast(LongintVector*, oaTableRecordIndexVectors.GetAt(0))->GetSize();
}

const KWKey* KWDatabaseIndexer::GetChunkPreviousRootKeyAt(int nChunkIndex) const
{
	require(IsIndexationComputed());
	require(0 <= nChunkIndex and nChunkIndex < GetChunkNumber());
	if (nChunkIndex == 0 or not IsMultiTableTechnology() or not HasMainTableKeys())
		return &emptyKey;
	else
		return cast(const KWKey*, oaExtractedKeys.GetAt(nChunkIndex - 1));
}

longint KWDatabaseIndexer::GetChunkPreviousRecordIndexesAt(int nChunkIndex, int nTableIndex) const
{
	LongintVector* lvChunkRecordIndexVectors;

	require(IsIndexationComputed());
	require(0 <= nChunkIndex and nChunkIndex < GetChunkNumber());
	require(0 <= nTableIndex and nTableIndex < GetTableNumber());

	lvChunkRecordIndexVectors = cast(LongintVector*, oaTableRecordIndexVectors.GetAt(nTableIndex));
	if (nTableIndex >= GetMainTableNumber())
		return 0;
	else if (lvChunkRecordIndexVectors == NULL)
		return -1;
	else
		return lvChunkRecordIndexVectors->GetAt(nChunkIndex);
}

longint KWDatabaseIndexer::GetChunkBeginPositionsAt(int nChunkIndex, int nTableIndex) const
{
	LongintVector* lvChunkBeginPositions;

	require(IsIndexationComputed());
	require(0 <= nChunkIndex and nChunkIndex < GetChunkNumber());
	require(0 <= nTableIndex and nTableIndex < GetTableNumber());

	lvChunkBeginPositions = cast(LongintVector*, oaTableNextRecordPositionVectors.GetAt(nTableIndex));
	if (nTableIndex >= GetMainTableNumber())
		return 0;
	else if (lvChunkBeginPositions == NULL)
		return -1;
	else
		return lvChunkBeginPositions->GetAt(nChunkIndex);
}

longint KWDatabaseIndexer::GetChunkEndPositionsAt(int nChunkIndex, int nTableIndex) const
{
	LongintVector* lvChunkBeginPositions;

	require(IsIndexationComputed());
	require(0 <= nChunkIndex and nChunkIndex < GetChunkNumber());
	require(0 <= nTableIndex and nTableIndex < GetTableNumber());

	lvChunkBeginPositions = cast(LongintVector*, oaTableNextRecordPositionVectors.GetAt(nTableIndex));
	if (nTableIndex >= GetMainTableNumber())
		return 0;
	else if (lvChunkBeginPositions == NULL)
		return -1;
	else
		return lvChunkBeginPositions->GetAt(nChunkIndex + 1);
}

boolean KWDatabaseIndexer::Check() const
{
	boolean bOk = true;
	int nTable;
	LongintVector* lvRecordIndexVector;
	LongintVector* lvNextRecordPositionVector;
	int i;

	if (IsIndexationComputed())
	{
		bOk = bOk and sourcePLDatabase.IsInitialized();
		bOk = bOk and sourcePLDatabase.IsOpenInformationComputed();

		// Taille des tableau de resultats
		bOk = bOk and oaTableRecordIndexVectors.GetSize() == GetTableNumber();
		bOk = bOk and oaTableNextRecordPositionVectors.GetSize() == GetTableNumber();
		assert(bOk);

		// Presence des resultats pour la premiere table
		bOk = bOk and (oaExtractedKeys.GetSize() == 0 or oaExtractedKeys.GetAt(0) != NULL);
		bOk = bOk and oaTableRecordIndexVectors.GetAt(0) != NULL;
		bOk = bOk and oaTableNextRecordPositionVectors.GetAt(0) != NULL;
		assert(bOk);

		// Resultats pour le tableau des cles
		if (bOk and IsMultiTableTechnology())
		{
			for (i = 1; i < oaExtractedKeys.GetSize(); i++)
			{
				bOk = bOk and cast(const KWKey*, oaExtractedKeys.GetAt(i - 1))
						      ->Compare(cast(const KWKey*, oaExtractedKeys.GetAt(i))) < 0;
				assert(bOk);
			}
		}

		// Resultats pour chaque table dans les cas mono-table et multi-tables
		if (bOk)
		{
			for (nTable = 0; nTable < GetTableNumber(); nTable++)
			{
				lvRecordIndexVector = cast(LongintVector*, oaTableRecordIndexVectors.GetAt(nTable));
				lvNextRecordPositionVector =
				    cast(LongintVector*, oaTableNextRecordPositionVectors.GetAt(nTable));

				// Coherence de la presence des vecteurs
				bOk = bOk and ((lvRecordIndexVector == NULL and lvNextRecordPositionVector == NULL) or
					       (lvRecordIndexVector != NULL and lvNextRecordPositionVector != NULL));

				// Taille des vecteurs
				bOk = bOk and (lvRecordIndexVector == NULL or
					       lvRecordIndexVector->GetSize() == GetChunkNumber());
				bOk = bOk and (lvNextRecordPositionVector == NULL or
					       lvNextRecordPositionVector->GetSize() == GetChunkNumber() + 1);
				assert(bOk);

				// Contenu du vecteur d'index
				if (lvRecordIndexVector != NULL)
				{
					bOk = bOk and lvRecordIndexVector->GetAt(0) == 0;
					for (i = 1; i < lvRecordIndexVector->GetSize(); i++)
					{
						// Les inegalites sont strictes uniquement pour la table racine
						if (nTable == 0)
							bOk = bOk and lvRecordIndexVector->GetAt(i - 1) <
									  lvRecordIndexVector->GetAt(i);
						else
							bOk = bOk and lvRecordIndexVector->GetAt(i - 1) <=
									  lvRecordIndexVector->GetAt(i);
						assert(bOk);
					}
				}

				// Contenu du vecteur de positions
				if (lvRecordIndexVector != NULL)
				{
					bOk = bOk and lvNextRecordPositionVector->GetAt(0) == 0;
					bOk = bOk and lvNextRecordPositionVector->GetAt(GetChunkNumber()) ==
							  GetPLDatabase()->GetFileSizeAt(nTable);
					assert(bOk);

					// Cas particulier d'un fichier vide
					if (GetPLDatabase()->GetFileSizeAt(nTable) == 0)
						bOk = bOk and lvNextRecordPositionVector->GetSize() == 2;
					// Cas general
					else
					{
						for (i = 1; i < lvNextRecordPositionVector->GetSize(); i++)
						{
							// Les inegalites sont strictes uniquement pour la table racine
							if (nTable == 0)
								bOk = bOk and lvNextRecordPositionVector->GetAt(i - 1) <
										  lvNextRecordPositionVector->GetAt(i);
							else
								bOk =
								    bOk and lvNextRecordPositionVector->GetAt(i - 1) <=
										lvNextRecordPositionVector->GetAt(i);
							assert(bOk);
						}
					}
				}
			}
		}
	}
	return bOk;
}

void KWDatabaseIndexer::Write(ostream& ost) const
{
	int nChunk;
	int nTable;
	KWMTDatabase* mtDatabase;
	KWMTDatabaseMapping* mapping;

	cout << GetClassLabel() << "\t" << GetObjectLabel() << "\n";
	if (IsIndexationComputed())
	{
		// Parametres
		cout << "  Resource slave number\t" << GetResourceSlaveNumber() << endl;
		cout << "  Max slave number\t" << GetMaxSlaveNumber() << endl;
		cout << "  Max memory for indexation results of source database\t" << GetMaxIndexationMemory() << endl;
		cout << "  Max total file size per process\t" << GetMaxTotalFileSizePerProcess() << endl;

		// Donnees sur la base
		cout << "\tTables\t" << GetTableNumber() << "\n";
		cout << "\tIs multi-table\t" << BooleanToString(IsMultiTableTechnology()) << "\n";
		cout << "\tAll table file size\t" << sourcePLDatabase.GetTotalFileSize() << "\n";

		// Descriptif des tables
		cout << "Tables\t" << GetTableNumber() << endl;
		if (IsMultiTableTechnology())
		{
			cout << "Index\tData path\tData table name\tSize\n";
			mtDatabase = cast(KWMTDatabase*, GetMTDatabase());
			for (nTable = 0; nTable < GetTableNumber(); nTable++)
			{
				mapping =
				    cast(KWMTDatabaseMapping*, mtDatabase->GetMultiTableMappings()->GetAt(nTable));
				cout << nTable + 1 << "\t";
				if (mapping != NULL)
					cout << mapping->GetDataPath() << "\t" << mapping->GetDataTableName();
				else
					cout << "\t";
				cout << "\t" << GetMTDatabase()->GetFileSizes()->GetAt(nTable);
				cout << "\n";
			}
		}

		// Detail des chunks
		cout << "Chunks\t" << GetChunkNumber() << "\n";
		if (GetChunkNumber() > 0)
		{
			// En tete pour les chunks
			ost << "Chunk\t";
			for (nTable = 0; nTable < GetTableNumber(); nTable++)
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
			for (nTable = 0; nTable < GetTableNumber(); nTable++)
			{
				ost << GetChunkPreviousRecordIndexesAt(nChunk, nTable) << "\t";
				ost << GetChunkBeginPositionsAt(nChunk, nTable) << "\t";
				ost << GetChunkEndPositionsAt(nChunk, nTable) << "\t";
			}
			ost << GetChunkPreviousRootKeyAt(nChunk)->GetObjectLabel() << "\n";
		}
	}
}

const ALString KWDatabaseIndexer::GetClassLabel() const
{
	return "Database indexer";
}

const ALString KWDatabaseIndexer::GetObjectLabel() const
{
	if (sourcePLDatabase.IsInitialized())
		return sourcePLDatabase.GetDatabase()->GetDatabaseName();
	else
		return "";
}

int KWDatabaseIndexer::GetMainTableNumber() const
{
	require(sourcePLDatabase.IsInitialized());
	require(IsMultiTableTechnology() or nMainTableNumber == 1);
	require(not IsMultiTableTechnology() or
		nMainTableNumber == sourcePLDatabase.GetMTDatabase()->GetMainTableNumber());
	return nMainTableNumber;
}

boolean KWDatabaseIndexer::HasMainTableKeys() const
{
	require(sourcePLDatabase.IsInitialized());
	require(IsMultiTableTechnology() or not bHasMainTableKeys);
	require(bHasMainTableKeys == (KWClassDomain::GetCurrentDomain()
					  ->LookupClass(sourcePLDatabase.GetDatabase()->GetClassName())
					  ->GetKeyAttributeNumber() > 0));
	return bHasMainTableKeys;
}

boolean KWDatabaseIndexer::HasDatabaseSameStructure(const KWDatabase* database)
{
	boolean bOk = true;
	boolean bIsSingleTableDatabase;
	KWMTDatabase* mtDatabase;
	KWMTDatabase* mtDatabaseRef;
	KWMTDatabaseMapping* mtDatabaseMapping;
	KWMTDatabaseMapping* mtDatabaseMappingRef;
	KWClass* kwcClass;
	KWClass* kwcClassRef;
	int i;

	require(database != NULL);
	require(sourcePLDatabase.IsInitialized());

	// Test si on est dans un cas monoi-table
	// (une seule table et pas de cle; en presence de cle, il faut en effet detecter les doublons potentiels)
	// En effet, dans ce cas, il n'y a pas besoin de pre-indexer la table d'entree pour la parallelisation
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(database->GetClassName());
	bIsSingleTableDatabase = (database->GetTableNumber() == 1 and kwcClass->GetKeyAttributeNumber() == 0);

	// Comparaison sur la technologie multi-table
	if (bIsSingleTableDatabase)
		bOk = bOk and not sourcePLDatabase.IsMultiTableTechnology();
	else
		bOk = bOk and sourcePLDatabase.IsMultiTableTechnology();

	// Comparaison generale
	bOk = bOk and database->GetDatabaseName() == sourcePLDatabase.GetDatabase()->GetDatabaseName();
	bOk = bOk and database->GetTableNumber() == sourcePLDatabase.GetDatabase()->GetTableNumber();

	// Comparaison sur la structure des classes dans le cas mono-table
	kwcClassRef = KWClassDomain::GetCurrentDomain()->LookupClass(sourcePLDatabase.GetDatabase()->GetClassName());
	bOk = bOk and CompareClassStructure(kwcClass, kwcClassRef);

	// Comparaison du schema dans le cas multi-table
	if (bOk and database->GetTableNumber() > 1)
	{
		mtDatabase = cast(KWMTDatabase*, database);
		mtDatabaseRef = sourcePLDatabase.GetMTDatabase();
		for (i = 1; i < mtDatabase->GetMultiTableMappings()->GetSize(); i++)
		{
			mtDatabaseMapping = cast(KWMTDatabaseMapping*, mtDatabase->GetMultiTableMappings()->GetAt(i));
			mtDatabaseMappingRef =
			    cast(KWMTDatabaseMapping*, mtDatabaseRef->GetMultiTableMappings()->GetAt(i));

			// Comparaison des chemins et des fichiers
			bOk = bOk and mtDatabaseMapping->GetDataPathAttributeNames() ==
					  mtDatabaseMappingRef->GetDataPathAttributeNames();
			bOk = bOk and mtDatabaseMapping->GetDataTableName() == mtDatabaseMappingRef->GetDataTableName();

			// Comparaison de la structure des classes
			kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(mtDatabaseMapping->GetClassName());
			kwcClassRef =
			    KWClassDomain::GetCurrentDomain()->LookupClass(mtDatabaseMappingRef->GetClassName());
			bOk = bOk and CompareClassStructure(kwcClass, kwcClassRef);
		}
	}
	return bOk;
}

boolean KWDatabaseIndexer::CompareClassStructure(const KWClass* kwcClass1, const KWClass* kwcClass2)
{
	boolean bOk = true;
	KWAttribute* attribute1;
	KWAttribute* attribute2;
	KWAttributeBlock* attributeBlock1;
	KWAttributeBlock* attributeBlock2;
	int i;

	require(kwcClass1 != NULL);
	require(kwcClass2 != NULL);

	// Comparaison generale
	bOk = bOk and kwcClass1->GetNativeDataItemNumber() == kwcClass2->GetNativeDataItemNumber();
	bOk = bOk and kwcClass1->GetKeyAttributeNumber() == kwcClass2->GetKeyAttributeNumber();

	// Comparaison sur les attributs ou blocs d'attributs natifs
	if (bOk)
	{
		attribute1 = kwcClass1->GetHeadAttribute();
		while (attribute1 != NULL)
		{
			// Comparaison si attribut de donne non calcule
			if (not attribute1->IsInBlock() and attribute1->GetDerivationRule() == NULL and
			    KWType::IsData(attribute1->GetType()))
			{
				attribute2 = kwcClass2->LookupAttribute(attribute1->GetName());
				bOk = bOk and attribute2 != NULL;
				bOk = bOk and attribute2->GetDerivationRule() == NULL;
				bOk = bOk and attribute1->GetType() == attribute2->GetType();
			}
			// Comparaison si bloc d'attribut natif, pour le premier attribut du bloc uniquement
			else if (attribute1->IsInBlock() and attribute1->IsFirstInBlock() and
				 attribute1->GetAttributeBlock()->GetDerivationRule() == NULL)
			{
				attributeBlock1 = attribute1->GetAttributeBlock();
				attributeBlock2 = kwcClass2->LookupAttributeBlock(attributeBlock1->GetName());
				bOk = bOk and attributeBlock2 != NULL;
				bOk = bOk and attributeBlock2->GetDerivationRule() == NULL;
				bOk = bOk and attributeBlock1->GetType() == attributeBlock2->GetType();
			}
			if (not bOk)
				break;

			// Attribut suivant
			kwcClass1->GetNextAttribute(attribute1);
		}
	}

	// Comparaison sur les attributs de la cle
	if (bOk)
	{
		for (i = 0; i < kwcClass1->GetKeyAttributeNumber(); i++)
			bOk = bOk and kwcClass1->GetKeyAttributeNameAt(i) == kwcClass2->GetKeyAttributeNameAt(i);
	}
	return bOk;
}

boolean KWDatabaseIndexer::ComputeAllDataTableIndexation()
{
	boolean bOk = true;

	require(sourcePLDatabase.IsInitialized());
	require(sourcePLDatabase.Check());
	require(sourcePLDatabase.IsOpenInformationComputed());
	require(Check());

	// Initialisation du resultat
	bIsIndexationInterruptedByUser = false;

	// Indexation de la table principale si rien n'a ete calcule
	if (bOk and not IsIndexationComputed())
	{
		// Indexation basique en un seul troncon dans le cas d'un seul esclave
		if (GetUsedSlaveNumber() == 1)
			bOk = ComputeRootTableBasicIndexation();
		// Cas ou la table principale ne contient pas de cle, et que l'on est donc reduit au cas d'une seule
		// table principale Remarque: on peut quand meme etre dans le cas multi-table, s'il y a des tables
		// externes
		else if (not HasMainTableKeys())
			bOk = ComputeSingleTableIndexation();
		// Cas ou la table principale contient une cle, ce qui est possible meme avec une seule table
		else
			bOk = ComputeRootTableIndexation();
	}

	// Indexation des tables secondaires
	if (bOk and HasMainTableKeys() and GetMainTableNumber() > 1)
	{
		assert(IsIndexationComputed());

		// Cas d'un seul troncon
		if (GetChunkNumber() == 1)
			bOk = ComputeSecondaryTablesBasicIndexation();
		// Cas de plusieurs troncons
		else
			bOk = ComputeSecondaryTablesIndexation();
	}
	ensure(Check());
	return bOk;
}

boolean KWDatabaseIndexer::ComputeRootTableBasicIndexation()
{
	boolean bOk = true;
	LongintVector* lvFileBeginPositions;
	LongintVector* lvFileBeginRecordIndexes;

	require(oaTableRecordIndexVectors.GetSize() == 0);

	// Initialisation des tableau de vecteur d'index et de positions
	assert(oaExtractedKeys.GetSize() == 0);
	oaTableRecordIndexVectors.SetSize(GetTableNumber());
	oaTableNextRecordPositionVectors.SetSize(GetTableNumber());

	//////////////////////////////////////////////////////////
	// Creation d'un troncon unique pour la table principale

	// Creation, initialisation et memorisation des vecteur d'index
	lvFileBeginRecordIndexes = new LongintVector;
	lvFileBeginRecordIndexes->SetSize(1);
	lvFileBeginRecordIndexes->SetAt(0, 0);
	oaTableRecordIndexVectors.SetAt(0, lvFileBeginRecordIndexes);

	// Creation, initialisation et memorisation des vecteurs de position
	lvFileBeginPositions = new LongintVector;
	lvFileBeginPositions->SetSize(2);
	lvFileBeginPositions->SetAt(0, 0);
	lvFileBeginPositions->SetAt(1, GetPLDatabase()->GetFileSizeAt(0));
	oaTableNextRecordPositionVectors.SetAt(0, lvFileBeginPositions);
	ensure(Check());
	return bOk;
}

boolean KWDatabaseIndexer::ComputeSingleTableIndexation()
{
	boolean bOk = true;
	boolean bDisplay = false;
	longint lTotalFileSizePerProcess;
	longint lPositionMemory;
	longint lMaxPositionNumber;
	int nPositionNumberPerBuffer;
	int nFileIndexerBufferSize;
	int nFileIndexerBufferNumber;
	KWFileIndexerTask fileIndexerTask;
	LongintVector* lvFileBeginPositions;
	LongintVector* lvFileBeginRecordIndexes;

	require(not IsMultiTableTechnology() or GetMTDatabase()->GetMainTableNumber() == 1);
	require(KWClassDomain::GetCurrentDomain()
		    ->LookupClass(GetPLDatabase()->GetDatabase()->GetClassName())
		    ->GetKeyAttributeNumber() == 0);
	require(oaTableRecordIndexVectors.GetSize() == 0);

	// Evaluation de la taille max totale des fichiers a traiter par process
	lTotalFileSizePerProcess = ComputeTotalFileSizePerProcess();

	// Affichage des entrees
	if (bDisplay)
	{
		cout << "ComputeSingleTableIndexation" << endl;
		cout << "\tDatabase\t" << GetObjectLabel() << endl;
	}

	// Indexation basique si le fichier est trop petit
	if (lTotalFileSizePerProcess >= GetPLDatabase()->GetTotalFileSize())
		bOk = ComputeRootTableBasicIndexation();
	// Indexation effective sinon
	else
	{
		assert(lTotalFileSizePerProcess > 0);

		// Taille necessaire pour stocker une paire (position, index)
		lPositionMemory = sizeof(longint) * 2;

		// Calcul du nombre max de cles et de leur position par table utilise, en fonction de la memoire
		// disponible
		lMaxPositionNumber = GetMaxIndexationMemory() / lPositionMemory;
		if (lPositionMemory < 0)
			lPositionMemory = 0;

		// Taille du buffer pour l'indexation
		nFileIndexerBufferSize =
		    max((int)InputBufferedFile::nDefaultBufferSize,
			PLRemoteFileService::GetPreferredBufferSize(GetPLDatabase()->GetDatabase()->GetDatabaseName()));

		// On l'agrandit si la taille a traiter est importante
		if (lTotalFileSizePerProcess >= SystemFile::nMaxPreferredBufferSize)
			nFileIndexerBufferSize = SystemFile::nMaxPreferredBufferSize;

		// Calcul du nombre moyen de positions d'indexation par buffer: chaque buffer indexe doit produit
		// assez de points de coupure pour copier le fichier en morceaux de taille lTotalFileSizePerProcess
		nPositionNumberPerBuffer = (int)ceil(nFileIndexerBufferSize * 1.0 / lTotalFileSizePerProcess);
		assert(nPositionNumberPerBuffer >= 1);

		// Limitation du nombre de positions en cas de depassement memoire
		nFileIndexerBufferNumber = 1 + int(GetPLDatabase()->GetTotalFileSize() / nFileIndexerBufferSize);
		if (nFileIndexerBufferNumber * nPositionNumberPerBuffer > lMaxPositionNumber)
		{
			nPositionNumberPerBuffer = max(1, int(lMaxPositionNumber / nFileIndexerBufferNumber));

			// Cas extreme ou cela ne suffit pas
			if (nPositionNumberPerBuffer == 1 and nFileIndexerBufferNumber > lMaxPositionNumber)
			{
				// On change la taille du buffer, ce qui permet de gerer des fichiers de taille
				// gigantesque si la memoire de travail par defaut (64 MB) est assez grande
				nFileIndexerBufferSize = SystemFile::nMaxPreferredBufferSize;
				nFileIndexerBufferNumber =
				    1 + int(GetPLDatabase()->GetTotalFileSize() / nFileIndexerBufferSize);
			}
		}
		assert(SystemFile::nMaxPreferredBufferSize * (64 * lMB / lPositionMemory) == 256 * lTB);
		assert(nFileIndexerBufferNumber * nPositionNumberPerBuffer <= lMaxPositionNumber or
		       GetPLDatabase()->GetTotalFileSize() >= 256 * lTB or GetMaxIndexationMemory() < 64 * lMB);

		// Affichage des parametres
		if (bDisplay)
		{
			cout << "\tTotalFileSize\t" << GetPLDatabase()->GetTotalFileSize() << endl;
			cout << "\tFileIndexerBufferSize\t" << nFileIndexerBufferSize << endl;
			cout << "\tMaxIndexationMemory\t" << GetMaxIndexationMemory() << endl;
			cout << "\tMaxPositionNumber\t" << lMaxPositionNumber << endl;
			cout << "\tPositionNumberPerBuffer\t" << nPositionNumberPerBuffer << endl;
			cout << "\tFileIndexerBufferNumber\t" << nFileIndexerBufferNumber << endl;
		}

		// Calcul de l'indexation avec la taille de buffer par defaut
		fileIndexerTask.SetFileName(GetPLDatabase()->GetDatabase()->GetDatabaseName());
		lvFileBeginPositions = new LongintVector;
		lvFileBeginRecordIndexes = new LongintVector;
		bOk = fileIndexerTask.ComputeIndexation(nFileIndexerBufferSize, nPositionNumberPerBuffer,
							lvFileBeginPositions, lvFileBeginRecordIndexes);
		bIsIndexationInterruptedByUser =
		    bIsIndexationInterruptedByUser or fileIndexerTask.IsTaskInterruptedByUser();

		// Memorisation des resultats si ok
		if (bOk)
		{
			assert(oaExtractedKeys.GetSize() == 0);

			// Initialisation des tableaux
			oaTableRecordIndexVectors.SetSize(GetTableNumber());
			oaTableNextRecordPositionVectors.SetSize(GetTableNumber());

			// Retaillage du vecteur d'index, pour supprmier la derniere valeur contenant le nombre de
			// lignes du fichier
			assert(lvFileBeginPositions->GetSize() >= 2);
			assert(lvFileBeginRecordIndexes->GetSize() == lvFileBeginPositions->GetSize());
			lvFileBeginRecordIndexes->SetSize(lvFileBeginRecordIndexes->GetSize() - 1);

			// Memorisation des vecteur d'index et de position
			oaTableRecordIndexVectors.SetAt(0, lvFileBeginRecordIndexes);
			oaTableNextRecordPositionVectors.SetAt(0, lvFileBeginPositions);
		}
		// Nettoyage sinon
		else
		{
			delete lvFileBeginPositions;
			delete lvFileBeginRecordIndexes;
		}
	}

	// Affichage des resultats
	if (bDisplay)
	{
		cout << "\tRaw chunks\t" << GetChunkNumber() << endl;
	}

	ensure(Check());
	return bOk;
}

boolean KWDatabaseIndexer::ComputeRootTableIndexation()
{
	boolean bOk = true;
	boolean bDisplay = false;
	longint lTotalFileSizePerProcess;
	longint lRootMeanKeySize;
	longint lRootLineNumber;
	longint lAllKeyPositionMemory;
	longint lMaxKeyNumber;
	longint lMaxSlaveProcessNumber;
	KWKeyFieldsIndexer rootKeyFieldsIndexer;
	double dSamplingRate;
	ObjectArray oaFoundKeyPositions;
	KWKeyPosition* recordKeyPosition;
	LongintVector* lvFileBeginPositions;
	LongintVector* lvFileBeginRecordIndexes;
	KWKeyPosition* keyPosition;
	int i;

	require(IsMultiTableTechnology());
	require(oaTableRecordIndexVectors.GetSize() == 0);

	// Evaluation de la taille max totale des fichiers a traiter par process
	lTotalFileSizePerProcess = ComputeTotalFileSizePerProcess();

	// Determination du taux d'echantillonnage des cles si necessaire
	dSamplingRate = 0;
	lRootMeanKeySize = 0;
	lRootLineNumber = 0;
	if (bOk and lTotalFileSizePerProcess < GetPLDatabase()->GetTotalFileSize())
	{
		// Initialisation d'un indexeur de cle pour la table principale
		bOk = bOk and InitializeKeyFieldIndexer(0, &rootKeyFieldsIndexer);

		// Evaluation de la taille des cles et du nombre de lignes de la table racine
		bOk = bOk and EvaluateKeySize(&rootKeyFieldsIndexer, lRootMeanKeySize, lRootLineNumber);

		// Calcul du taux d'echantillonnage
		if (bOk)
		{
			// Calcul de la taille totale necessaire pour stocker une cle pour toutes les tables
			// La cle n'est stockee que pour la table racine, plus au plus un indexe t une position
			// par table non externe
			lAllKeyPositionMemory = lRootMeanKeySize + sizeof(KWKey*) +
						GetMTDatabase()->GetMainTableNumber() * sizeof(longint) * 2;

			// Calcul du nombre max de cles et de leur position par table utilise, en fonction de la memoire
			// disponible
			lMaxKeyNumber = GetMaxIndexationMemory() / lAllKeyPositionMemory;
			if (lMaxKeyNumber < 0)
				lMaxKeyNumber = 0;

			// On limite ce nombre de cle on fonction de la taille des fichiers a analyser
			// On utilise (lMaxFileSizePerProcess/8) pour la gestion de la fin des taches
			lMaxSlaveProcessNumber =
			    1 + GetMTDatabase()->GetTotalFileSize() / (lTotalFileSizePerProcess / 8);
			if (lMaxKeyNumber > lMaxSlaveProcessNumber)
				lMaxKeyNumber = lMaxSlaveProcessNumber;

			// On en deduit le taux d'echantillonnage max
			dSamplingRate = lMaxKeyNumber * 1.0 / lRootLineNumber;
			if (dSamplingRate > 1)
				dSamplingRate = 1;
			if (dSamplingRate * lRootLineNumber < 10)
				dSamplingRate = 0;
			if (lTotalFileSizePerProcess >= GetMTDatabase()->GetTotalFileSize())
				dSamplingRate = 0;
			if (bDisplay)
			{
				cout << "ComputeRootTableIndexation\n";
				cout << "\tAll tables size\t" << GetMTDatabase()->GetTotalFileSize() << "\n";
				cout << "\tTotal file size per process\t" << lTotalFileSizePerProcess << "\n";
				cout << "\tMax slave process number\t" << lMaxSlaveProcessNumber << "\n";
				cout << "\tMax key number\t" << lMaxKeyNumber << "\n";
				cout << "\tSampling rate\t" << dSamplingRate << "\n";
				cout << "\tRoot mean key size\t" << lRootMeanKeySize << "\n";
				cout << "\tRoot line number\t" << lRootLineNumber << "\n";
			}
		}
	}

	// Indexation basique si aucune cle ne doit etre extraite
	if (bOk and dSamplingRate == 0)
		bOk = ComputeRootTableBasicIndexation();

	// Extraction des cles de la table principale si necessaire
	if (bOk and dSamplingRate > 0)
	{
		// Extraction des cles et de leur position de la table racine
		bOk = ExtractRootSampleKeyPositions(&rootKeyFieldsIndexer, lRootMeanKeySize, lRootLineNumber,
						    dSamplingRate, &oaFoundKeyPositions);

		// Cas particulier ou la derniere cle arrive en fin de fichier: on la supprime, car elle delimite un
		// chunk suivant vide inutile
		if (bOk)
		{
			if (oaFoundKeyPositions.GetSize() > 0)
			{
				recordKeyPosition =
				    cast(KWKeyPosition*, oaFoundKeyPositions.GetAt(oaFoundKeyPositions.GetSize() - 1));

				// Destruction de cette cle si elle delimite la fin du fichier
				if (recordKeyPosition->GetLinePosition() == GetMTDatabase()->GetFileSizes()->GetAt(0))
				{
					delete recordKeyPosition;
					oaFoundKeyPositions.SetSize(oaFoundKeyPositions.GetSize() - 1);
				}
			}
		}

		// Memorisation des resultats si ok
		if (bOk)
		{
			// Collecte des cles a partir des positions extraites de la table racine
			KWKeyPosition::CollectClonedKeys(&oaFoundKeyPositions, &oaExtractedKeys);

			// Initialisation des tableaux
			oaTableRecordIndexVectors.SetSize(GetTableNumber());
			oaTableNextRecordPositionVectors.SetSize(GetTableNumber());

			// Creation et memorisation des vecteur d'index
			lvFileBeginRecordIndexes = new LongintVector;
			lvFileBeginRecordIndexes->SetSize(oaExtractedKeys.GetSize() + 1);
			oaTableRecordIndexVectors.SetAt(0, lvFileBeginRecordIndexes);

			// Creation et memorisation des vecteur de position
			lvFileBeginPositions = new LongintVector;
			lvFileBeginPositions->SetSize(oaExtractedKeys.GetSize() + 2);
			oaTableNextRecordPositionVectors.SetAt(0, lvFileBeginPositions);

			// Initialisation des vecteur d'index et de posiiton
			lvFileBeginRecordIndexes->SetAt(0, 0);
			lvFileBeginPositions->SetAt(0, 0);
			lvFileBeginPositions->SetAt(lvFileBeginPositions->GetSize() - 1,
						    GetMTDatabase()->GetFileSizes()->GetAt(0));
			for (i = 0; i < oaFoundKeyPositions.GetSize(); i++)
			{
				keyPosition = cast(KWKeyPosition*, oaFoundKeyPositions.GetAt(i));
				lvFileBeginRecordIndexes->SetAt(i + 1, keyPosition->GetLineIndex());
				lvFileBeginPositions->SetAt(i + 1, keyPosition->GetLinePosition());
			}
		}

		// Nettoyage
		oaFoundKeyPositions.DeleteAll();
	}
	ensure(Check());
	return bOk;
}

boolean KWDatabaseIndexer::ComputeSecondaryTablesBasicIndexation()
{
	boolean bOk = true;
	int nMapping;
	KWMTDatabaseMapping* mapping;
	LongintVector* lvFileBeginPositions;
	LongintVector* lvFileBeginRecordIndexes;

	require(Check());
	require(GetChunkNumber() == 1);
	require(IsMultiTableTechnology());
	require(GetMainTableNumber() > 1);
	require(HasMainTableKeys());
	require(not bIsIndexationInterruptedByUser);

	// Creation d'un troncon unique par table secondaire
	for (nMapping = 1; nMapping < GetMTDatabase()->GetTableNumber(); nMapping++)
	{
		mapping = GetMTDatabase()->GetUsedMappingAt(nMapping);

		// Indexation si necessaire et si non deja calcule
		if (mapping != NULL and oaTableRecordIndexVectors.GetAt(nMapping) == NULL)
		{
			// Creation, initialisation et memorisation des vecteur d'index
			lvFileBeginRecordIndexes = new LongintVector;
			lvFileBeginRecordIndexes->SetSize(1);
			lvFileBeginRecordIndexes->SetAt(0, 0);
			oaTableRecordIndexVectors.SetAt(nMapping, lvFileBeginRecordIndexes);

			// Creation, initialisation et memorisation des vecteur de position
			lvFileBeginPositions = new LongintVector;
			lvFileBeginPositions->SetSize(2);
			lvFileBeginPositions->SetAt(0, 0);
			lvFileBeginPositions->SetAt(1, GetMTDatabase()->GetFileSizes()->GetAt(nMapping));
			oaTableNextRecordPositionVectors.SetAt(nMapping, lvFileBeginPositions);
		}
	}
	ensure(Check());
	return bOk;
}

boolean KWDatabaseIndexer::ComputeSecondaryTablesIndexation()
{
	boolean bOk = true;
	int nMapping;
	KWMTDatabaseMapping* mapping;
	KWKeyPositionFinderTask keyPositionFinder;
	KWKeyFieldsIndexer keyFieldsIndexer;
	ObjectArray oaFoundKeyPositions;
	KWKeyPosition* keyPosition;
	LongintVector* lvFileBeginPositions;
	LongintVector* lvFileBeginRecordIndexes;
	int i;

	require(Check());
	require(GetChunkNumber() > 1);
	require(IsMultiTableTechnology());
	require(GetMainTableNumber() > 1);
	require(HasMainTableKeys());
	require(oaExtractedKeys.GetSize() > 0);
	require(not bIsIndexationInterruptedByUser);

	// Indexation des tables secondaire
	for (nMapping = 1; nMapping < GetMTDatabase()->GetTableNumber(); nMapping++)
	{
		mapping = GetMTDatabase()->GetUsedMappingAt(nMapping);

		// Indexation si necessaire et si non deja calcule
		if (mapping != NULL and oaTableRecordIndexVectors.GetAt(nMapping) == NULL)
		{
			// Initialisation d'un indexeur de cle pour la table secondaire
			bOk = bOk and InitializeKeyFieldIndexer(nMapping, &keyFieldsIndexer);

			// Extraction des cles
			if (bOk)
			{
				keyPositionFinder.SetFileName(mapping->GetDataTableName());
				keyPositionFinder.SetHeaderLineUsed(GetMTDatabase()->GetHeaderLineUsed());
				keyPositionFinder.SetFieldSeparator(GetMTDatabase()->GetFieldSeparator());
				keyPositionFinder.GetKeyFieldIndexes()->CopyFrom(
				    keyFieldsIndexer.GetConstKeyFieldIndexes());
				bOk =
				    bOk and keyPositionFinder.FindKeyPositions(&oaExtractedKeys, &oaFoundKeyPositions);
				bIsIndexationInterruptedByUser =
				    bIsIndexationInterruptedByUser or keyPositionFinder.IsTaskInterruptedByUser();
				assert(bOk or oaFoundKeyPositions.GetSize() == 0);
			}

			// Arret si erreur
			if (not bOk)
				break;
			// Memorisation des resultats sinon
			else
			{
				assert(oaFoundKeyPositions.GetSize() == oaExtractedKeys.GetSize());

				// Nettoyage des cles du tableau resultat pour gagner en memoire
				KWKeyPosition::CleanKeys(&oaFoundKeyPositions);

				// Creation et memorisation des vecteur d'index
				lvFileBeginRecordIndexes = new LongintVector;
				lvFileBeginRecordIndexes->SetSize(oaExtractedKeys.GetSize() + 1);
				oaTableRecordIndexVectors.SetAt(nMapping, lvFileBeginRecordIndexes);

				// Creation et memorisation des vecteur de position
				lvFileBeginPositions = new LongintVector;
				lvFileBeginPositions->SetSize(oaExtractedKeys.GetSize() + 2);
				oaTableNextRecordPositionVectors.SetAt(nMapping, lvFileBeginPositions);

				// Initialisation des vecteur d'index et de posiiton
				lvFileBeginRecordIndexes->SetAt(0, 0);
				lvFileBeginPositions->SetAt(0, 0);
				lvFileBeginPositions->SetAt(lvFileBeginPositions->GetSize() - 1,
							    GetMTDatabase()->GetFileSizes()->GetAt(nMapping));
				for (i = 0; i < oaFoundKeyPositions.GetSize(); i++)
				{
					keyPosition = cast(KWKeyPosition*, oaFoundKeyPositions.GetAt(i));
					lvFileBeginRecordIndexes->SetAt(i + 1, keyPosition->GetLineIndex());
					lvFileBeginPositions->SetAt(i + 1, keyPosition->GetLinePosition());
				}

				// Nettoyage
				oaFoundKeyPositions.DeleteAll();
			}
		}
	}
	ensure(Check());
	return bOk;
}

longint KWDatabaseIndexer::ComputeTotalFileSizePerProcess() const
{
	const int nMaxProcessBySlave = 5;
	longint lTotalFileSizePerProcess;

	// Valeur par defaut selon la taille preferee des buffers de fichier pour la base
	lTotalFileSizePerProcess = (longint)GetPLDatabase()->GetDatabasePreferredBuferSize();

	// On limite le nombre de process par esclave en cas de base trop petite
	if (lTotalFileSizePerProcess * GetUsedSlaveNumber() * nMaxProcessBySlave > GetPLDatabase()->GetTotalFileSize())
		lTotalFileSizePerProcess =
		    max((longint)SystemFile::nMinPreferredBufferSize,
			GetPLDatabase()->GetTotalFileSize() / (GetUsedSlaveNumber() * nMaxProcessBySlave));
	lTotalFileSizePerProcess = min(lTotalFileSizePerProcess, GetPLDatabase()->GetTotalFileSize());

	// On peut imposer la taille du buffer pour raison de tests
	if (GetMaxTotalFileSizePerProcess() > 0)
		lTotalFileSizePerProcess = GetMaxTotalFileSizePerProcess();
	return lTotalFileSizePerProcess;
}

boolean KWDatabaseIndexer::InitializeKeyFieldIndexer(int nTableIndex, KWKeyFieldsIndexer* keyFieldsIndexer)
{
	boolean bOk = true;
	boolean bDisplay = false;
	const KWMTDatabaseMapping* readMapping;
	KWClass* kwcRootClass;
	KWClass* kwcMappingClass;
	KWClass* kwcHeaderLineClass;
	StringVector svFirstLine;
	StringVector* svHeaderLineFieldNames;

	require(sourcePLDatabase.IsInitialized());
	require(IsMultiTableTechnology());
	require(0 <= nTableIndex and nTableIndex < GetTableNumber());
	require(keyFieldsIndexer != NULL);

	// Recherche du mapping
	readMapping = GetMTDatabase()->GetUsedMappingAt(nTableIndex);
	check(readMapping);

	// Recherche de la classe racine
	kwcRootClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetMTDatabase()->GetClassName());
	check(kwcRootClass);
	assert(kwcRootClass->GetKeyAttributeNumber() > 0);

	// Recherche de la table associee au mapping
	kwcMappingClass = KWClassDomain::GetCurrentDomain()->LookupClass(readMapping->GetClassName());
	check(kwcMappingClass);

	// Export des noms des attributs cle et natif
	kwcMappingClass->ExportNativeFieldNames(keyFieldsIndexer->GetNativeFieldNames());
	kwcMappingClass->ExportKeyAttributeNames(keyFieldsIndexer->GetKeyAttributeNames());

	// On restreint les champs cle a ceux correspondant a la classe racine
	keyFieldsIndexer->GetKeyAttributeNames()->SetSize(kwcRootClass->GetKeyAttributeNumber());

	// Extraction des champs de la premiere ligne du fichier d'entree a partir de la classe representant la ligne
	// d'entete
	svHeaderLineFieldNames = NULL;
	if (GetMTDatabase()->GetHeaderLineUsed())
	{
		// Recherche de la ligne d'entete du fichier, memorise par la sourceDatabase, ce qui evite de relire la
		// premier ligne du fichier
		kwcHeaderLineClass = cast(KWClass*, GetMTDatabase()->GetUsedMappingHeaderLineClassAt(nTableIndex));
		check(kwcHeaderLineClass);

		// Extraction des champs
		kwcHeaderLineClass->ExportNativeFieldNames(&svFirstLine);
		assert(svFirstLine.GetSize() == kwcHeaderLineClass->GetAttributeNumber());

		// On utilise ces champs
		svHeaderLineFieldNames = &svFirstLine;
	}
	// Cas sans ligne d'entete: on reutilise les champs tels quels
	// Il n'y aura pas d'erreur detectee a ce moment la, mais cela evite un acces au fichier
	// Le probleme sera detecte plus tard lors des ouvertures des fichiers
	else
		svHeaderLineFieldNames = keyFieldsIndexer->GetNativeFieldNames();
	check(svHeaderLineFieldNames);

	// Calcul des index des champs de la cle (on est ici dans le cas multi-tables)
	bOk = keyFieldsIndexer->ComputeKeyFieldIndexes(GetMTDatabase()->GetHeaderLineUsed(), svHeaderLineFieldNames);
	if (not bOk)
		AddError("Error while indexing fields of table " + readMapping->GetDataTableName());

	// Affichage
	if (bDisplay)
	{
		cout << "InitializeKeyFieldIndexer" << endl;
		cout << "\tClass\t" << kwcMappingClass->GetName() << endl;
		cout << "\tHeader line used\t" << GetMTDatabase()->GetHeaderLineUsed() << endl;
		cout << "\tHeader line names\t" << *svHeaderLineFieldNames;
		cout << "\tKey field names\t" << *keyFieldsIndexer->GetKeyAttributeNames();
		if (bOk)
			cout << "\tKey fields indexes\t" << *keyFieldsIndexer->GetConstKeyFieldIndexes();
		else
			cout << "\tFailure";
		cout << endl;
	}
	return bOk;
}

boolean KWDatabaseIndexer::EvaluateKeySize(const KWKeyFieldsIndexer* rootKeyFieldsIndexer, longint& lRootMeanKeySize,
					   longint& lRootLineNumber)
{
	boolean bOk = true;
	KWKeySizeEvaluatorTask keySizeEvaluator;

	require(rootKeyFieldsIndexer != NULL);
	require(not bIsIndexationInterruptedByUser);
	require(IsMultiTableTechnology());

	// Evaluation de la taille des cles a partir de la table racine
	lRootMeanKeySize = 0;
	lRootLineNumber = 0;
	if (bOk)
	{
		bOk = keySizeEvaluator.EvaluateKeySize(
		    rootKeyFieldsIndexer->GetConstKeyFieldIndexes(), GetMTDatabase()->GetDatabaseName(),
		    GetMTDatabase()->GetHeaderLineUsed(), GetMTDatabase()->GetFieldSeparator(), lRootMeanKeySize,
		    lRootLineNumber);
		bIsIndexationInterruptedByUser =
		    bIsIndexationInterruptedByUser or keySizeEvaluator.IsTaskInterruptedByUser();
	}
	return bOk;
}

boolean KWDatabaseIndexer::ExtractRootSampleKeyPositions(const KWKeyFieldsIndexer* rootKeyFieldsIndexer,
							 longint lRootMeanKeySize, longint lRootLineNumber,
							 double dSamplingRate, ObjectArray* oaRootKeyPositions)
{
	boolean bOk = true;
	KWKeyPositionSampleExtractorTask keyPositionSampleExtractor;

	require(not bIsIndexationInterruptedByUser);
	require(IsMultiTableTechnology());
	require(rootKeyFieldsIndexer != NULL);
	require(oaRootKeyPositions != NULL);
	require(oaRootKeyPositions->GetSize() == 0);
	require(lRootMeanKeySize > 0);
	require(lRootLineNumber > 0);
	require(dSamplingRate > 0);

	// Extraction des cles et de leur position
	keyPositionSampleExtractor.SetFileName(GetMTDatabase()->GetDatabaseName());
	keyPositionSampleExtractor.SetHeaderLineUsed(GetMTDatabase()->GetHeaderLineUsed());
	keyPositionSampleExtractor.SetFieldSeparator(GetMTDatabase()->GetFieldSeparator());
	keyPositionSampleExtractor.SetSamplingRate(dSamplingRate);
	keyPositionSampleExtractor.SetKeyUsedMemory(lRootMeanKeySize);
	keyPositionSampleExtractor.SetFileLineNumber(lRootLineNumber);
	keyPositionSampleExtractor.GetKeyFieldIndexes()->CopyFrom(rootKeyFieldsIndexer->GetConstKeyFieldIndexes());
	bOk = keyPositionSampleExtractor.ExtractSample(oaRootKeyPositions);
	bIsIndexationInterruptedByUser =
	    bIsIndexationInterruptedByUser or keyPositionSampleExtractor.IsTaskInterruptedByUser();
	assert(bOk or oaRootKeyPositions->GetSize() == 0);
	return bOk;
}
