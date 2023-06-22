// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDatabaseTask.h"

KWDatabaseTask::KWDatabaseTask()
{
	lReadRecords = 0;
	lReadObjects = 0;
	dDatabaseIndexerTime = -1;
	bDisplaySpecificTaskMessage = true;
	bDisplayEndTaskMessage = true;
	bDisplayTaskTime = true;

	// Gestion des chunks dans le cas multi-tables
	nChunkCurrentIndex = 0;
	nForcedBufferSize = 0;
	lForcedMaxFileSizePerProcess = 0;

	// Declaration des variables partagees
	DeclareSharedParameter(&shared_sClassTmpFile);
	DeclareSharedParameter(&shared_sourceDatabase);

	// Declaration des variables en entree de l'esclave dans le cas multi-tables
	DeclareTaskInput(&input_lvChunkBeginPos);
	DeclareTaskInput(&input_lvChunkEndPos);
	DeclareTaskInput(&input_lvChunkBeginRecordIndex);
	DeclareTaskInput(&input_ChunkLastRootKey);
	DeclareTaskInput(&input_nBufferSize);
	DeclareTaskInput(&input_lFileBeginPosition);
	DeclareTaskInput(&input_lFileBeginRecordIndex);

	// Resultats envoyes par l'esclave dans le cas general
	DeclareTaskOutput(&output_lReadRecords);
	DeclareTaskOutput(&output_lReadObjects);

	// Resultats envoyes par l'esclave dans le cas multi-tables
	DeclareTaskOutput(&output_lvMappingReadRecords);
}

KWDatabaseTask::~KWDatabaseTask()
{
	assert(not mtDatabaseIndexer.IsComputed());
}

void KWDatabaseTask::SetDisplaySpecificTaskMessage(boolean bValue)
{
	bDisplaySpecificTaskMessage = bValue;
}

boolean KWDatabaseTask::GetDisplaySpecificTaskMessage() const
{
	return bDisplaySpecificTaskMessage;
}

void KWDatabaseTask::SetDisplayEndTaskMessage(boolean bValue)
{
	bDisplayEndTaskMessage = bValue;
}

boolean KWDatabaseTask::GetDisplayEndTaskMessage() const
{
	return bDisplayEndTaskMessage;
}

void KWDatabaseTask::SetDisplayTaskTime(boolean bValue)
{
	bDisplayTaskTime = bValue;
}

boolean KWDatabaseTask::GetDisplayTaskTime() const
{
	return bDisplayTaskTime;
}

void KWDatabaseTask::SetDisplayAllTaskMessages(boolean bValue)
{
	bDisplaySpecificTaskMessage = bValue;
	bDisplayEndTaskMessage = bValue;
	bDisplayTaskTime = bValue;
}

longint KWDatabaseTask::GetReadRecords() const
{
	assert(dDatabaseIndexerTime >= 0);
	require(IsJobDone());
	return lReadRecords;
}

longint KWDatabaseTask::GetReadObjects() const
{
	assert(dDatabaseIndexerTime >= 0);
	require(IsJobDone());
	return lReadObjects;
}

double KWDatabaseTask::GetFullJobElapsedTime() const
{
	assert(dDatabaseIndexerTime >= 0);
	require(IsJobDone());
	return dDatabaseIndexerTime + GetJobElapsedTime();
}

const ALString KWDatabaseTask::GetObjectLabel() const
{
	return shared_sourceDatabase.GetObjectLabel();
}

boolean KWDatabaseTask::RunDatabaseTask(const KWDatabase* sourceDatabase)
{
	boolean bOk = true;
	KWMTDatabaseTextFile refMTDatabaseTextFile;
	KWSTDatabaseTextFile refSTDatabaseTextFile;
	ALString sTmp;

	require(sourceDatabase != NULL);
	require(sourceDatabase->Check());
	require(sourceDatabase->GetTechnologyName() == refMTDatabaseTextFile.GetTechnologyName() or
		sourceDatabase->GetTechnologyName() == refSTDatabaseTextFile.GetTechnologyName());

	// Initialisation des bases de travail pour un transfer mono-table si c'est possible
	// (une seule table et pas de cle; en presence de cle, il faut en effet detecter les doublons potentiels)
	// En effet, dans ce cas, il n'y a pas besoin de pre-indexer la table d'entree pour la parallelisation
	shared_sourceDatabase.GetPLDatabase()->InitializeFrom(sourceDatabase);

	// Calcul du plan d'indexation des tables
	bOk = bOk and ComputeAllDataTableIndexation();

	// Lancement de la tache
	CleanJobResults();
	if (bOk)
		bOk = Run();

	// Nettoyage des informations d'indexation
	CleanAllDataTableIndexation();
	return bOk;
}

void KWDatabaseTask::DisplayTaskMessage()
{
	// Messages de fin de tache
	if (GetDisplaySpecificTaskMessage())
	{
		// Messages de resultat sur les bases
		if (IsJobDone() and IsJobSuccessful())
			DisplaySpecificTaskMessage();
	}

	// Messages de fin de tache
	if (GetDisplayEndTaskMessage())
	{
		// Message d'erreur si necessaire
		if (IsJobDone() and IsTaskInterruptedByUser())
			AddWarning("Interrupted by user");
		else if (not IsJobDone() or not IsJobSuccessful())
			AddError("Interrupted because of errors");
	}

	// Temps d'execution de la tache
	if (GetDisplayTaskTime())
	{
		if (IsJobDone() and IsJobSuccessful() and not IsTaskInterruptedByUser())
			AddSimpleMessage(GetTaskLabel() + " time: " + SecondsToString(GetFullJobElapsedTime()));
	}
}

void KWDatabaseTask::DisplaySpecificTaskMessage()
{
	shared_sourceDatabase.GetPLDatabase()->DisplayAllTableMessages("Input database", "Read records", lReadRecords,
								       lReadObjects, &lvMappingReadRecords);
}

boolean KWDatabaseTask::ComputeAllDataTableIndexation()
{
	boolean bOk = true;
	boolean bShowTime = false;
	PLDatabaseTextFile* sourceDatabase;
	RMTaskResourceGrant grantedResources;
	KWFileIndexerTask fileIndexerTask;
	int nSlaveNumber;
	longint lMinSlaveGrantedMemoryForSourceDatabase;
	ObjectArray oaRootKeys;
	ObjectArray oaAllTableFoundKeyPositions;
	int nSourceBufferSize;
	Timer timer;

	require(shared_sourceDatabase.GetDatabase()->Check());
	require(not mtDatabaseIndexer.IsComputed());

	/////////////////////////////////////////////////////////////////////////////////////////
	// Collectes d'informations prealables sur les tables a traiter en lecture ou en ecriture

	// Acces a la base source
	sourceDatabase = shared_sourceDatabase.GetPLDatabase();
	timer.Start();

	// Calcul d'informations lie a l'ouverture des bases
	bOk = bOk and ComputeDatabaseOpenInformation();
	if (bShowTime)
		cout << GetClassLabel() << "\tCompute databases open information\t" << timer.GetElapsedTime() << endl;

	// Appel prealable pour evaluer les ressources allouees, de facon a piloter les taches d'indexation
	if (bOk)
		bOk = ComputeResourceRequirements();

	// Calcul des ressources allouees, pour pouvoir estimer le nombre d'esclaves
	nSlaveNumber = 0;
	lMinSlaveGrantedMemoryForSourceDatabase = 0;
	if (bOk)
	{
		RMParallelResourceManager::ComputeGrantedResources(GetResourceRequirements(), &grantedResources);
		if (not grantedResources.IsEmpty())
		{
			nSlaveNumber = grantedResources.GetSlaveNumber();

			// Calcul de la memoire allouee pour la base source
			lMinSlaveGrantedMemoryForSourceDatabase =
			    ComputeSlaveGrantedMemory(GetResourceRequirements()->GetSlaveRequirement(),
						      grantedResources.GetMinSlaveMemory(), true);
		}
		else
		{
			// Les ressources sont vide alors qu'on a demande strictement plus de un esclave
			assert(GetResourceRequirements()->GetMaxSlaveProcessNumber() > 0);

			// Si il n'y a pas de ressources disponibles, affichage de l'erreur
			AddError(grantedResources.GetMissingResourceMessage());
			bOk = false;
		}
	}

	// Plan d'indexation des tables
	if (bOk)
	{
		// Indexation multi-tables
		if (sourceDatabase->IsMultiTableTechnology())
		{
			bOk = mtDatabaseIndexer.ComputeIndexation(sourceDatabase->GetMTDatabase(), nSlaveNumber,
								  lMinSlaveGrantedMemoryForSourceDatabase,
								  lForcedMaxFileSizePerProcess);

			// Message d'erreur si necessaire
			if (mtDatabaseIndexer.IsInterruptedByUser())
				AddWarning("Preliminary indexation of database interrupted by user");
			else if (not bOk)
				AddError("A problem occurred during preliminary indexation of database");
		}
		// Indexation mono-table
		else
		{
			// Calcul des tailles de buffer permises par ces memoire allouees
			nSourceBufferSize = shared_sourceDatabase.GetPLDatabase()->ComputeOpenBufferSize(
			    true, lMinSlaveGrantedMemoryForSourceDatabase);

			// Calcul de l'indexation
			fileIndexerTask.SetFileName(sourceDatabase->GetDatabase()->GetDatabaseName());
			bOk = fileIndexerTask.ComputeIndexation(nSourceBufferSize, &lvFileBeginPositions,
								&lvFileBeginRecordIndexes);

			// Message d'erreur si necessaire
			if (fileIndexerTask.IsTaskInterruptedByUser())
				AddWarning("Preliminary indexation of database interrupted by user");
			else if (not bOk)
				AddError("A problem occurred during preliminary indexation of database");
		}
	}

	// Message synthetique en cas d'erreur
	if (not bOk)
		sourceDatabase->GetDatabase()->AddError("Unable to open input database due to previous errors");

	// Memorisation du temps d'indexation
	timer.Stop();
	dDatabaseIndexerTime = timer.GetElapsedTime();
	return bOk;
}

int KWDatabaseTask::GetIndexationSlaveProcessNumber()
{
	int nSlaveProcessNumber;
	PLDatabaseTextFile* sourceDatabase;

	// Acces a la base source
	sourceDatabase = shared_sourceDatabase.GetPLDatabase();

	// Cas multi-tables
	if (sourceDatabase->IsMultiTableTechnology())
		nSlaveProcessNumber = mtDatabaseIndexer.GetChunkNumber();
	// Cas mono-tables
	else
		nSlaveProcessNumber = max(0, lvFileBeginPositions.GetSize() - 1);
	return nSlaveProcessNumber;
}

void KWDatabaseTask::CleanAllDataTableIndexation()
{
	// Nettoyage des informations d'indexation des bases
	mtDatabaseIndexer.CleanIndexation();

	// Nettoyage des informations d'ouverture des bases
	shared_sourceDatabase.GetPLDatabase()->CleanOpenInformation();

	// Nettoyage dans le cas mono-table
	lvFileBeginPositions.SetSize(0);
	lvFileBeginRecordIndexes.SetSize(0);
}

boolean KWDatabaseTask::ComputeDatabaseOpenInformation()
{
	boolean bOk = true;
	const boolean bIncludingClassMemory = true;

	// Calcul de la memoire necessaire et collecte des informations permettant d'ouvir les bases dans les esclaves
	// Attention: on ne prend en compte qu'une seule fois le dictionaire de la base, qui est partage entre
	// les bases en lecture et en ecriture
	bOk = bOk and shared_sourceDatabase.GetPLDatabase()->ComputeOpenInformation(true, bIncludingClassMemory, NULL);
	return bOk;
}

boolean KWDatabaseTask::ComputeResourceRequirements()
{
	boolean bOk = true;
	PLDatabaseTextFile* sourceDatabase;
	int nSlaveProcessNumber;

	// Acces aux bases
	sourceDatabase = shared_sourceDatabase.GetPLDatabase();

	// Les informations de ressources sur les bases doivent deja etre calculees
	// Cela doit etre fait avant le run, et permet d'appeler cette methode de facon prealable
	// pour le dimensionnement des taches prealable d'indexation des bases et pour
	// la tache principale de tranfert
	assert(sourceDatabase->IsOpenInformationComputed());

	// Calcul des ressources pour les esclave: lecture plus ecriture, avec une reserve minimum par base
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMin(
	    sourceDatabase->GetMinOpenNecessaryMemory());
	GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->SetMax(
	    sourceDatabase->GetMaxOpenNecessaryMemory());

	// Calcul des resources pour le maitre: rien de special
	GetResourceRequirements()->GetMasterRequirement()->GetMemory()->Set(lMB);

	// Acces au nombre de taches elementaires resultant de lindexation (0 si non calcule)
	nSlaveProcessNumber = GetIndexationSlaveProcessNumber();
	if (nSlaveProcessNumber == 0)
		nSlaveProcessNumber = sourceDatabase->GetMaxSlaveProcessNumber();

	// Limitation du nombre de slave process
	GetResourceRequirements()->SetMaxSlaveProcessNumber(nSlaveProcessNumber);
	return bOk;
}

boolean KWDatabaseTask::MasterInitialize()
{
	boolean bOk = true;
	KWClass* kwcClass;
	ALString sClassTmpFile;
	int i;

	require(not shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology() or mtDatabaseIndexer.IsComputed());

	// Ecriture du fichier dictionnaire dans un repertoire temporaire pour passage au slave
	if (IsParallel())
	{
		// Construction du nom du dictionnaire temporaire
		sClassTmpFile = FileService::CreateUniqueTmpFile(
		    shared_sourceDatabase.GetDatabase()->GetClassName() + ".kdic", this);
		bOk = sClassTmpFile != "";

		// Recherche de la classe du dictionnaire
		kwcClass =
		    KWClassDomain::GetCurrentDomain()->LookupClass(shared_sourceDatabase.GetDatabase()->GetClassName());
		check(kwcClass);

		// Ecriture uniquement de la classe et de ses dependances
		if (bOk)
			bOk = KWClassDomain::GetCurrentDomain()->WriteFileFromClass(kwcClass, sClassTmpFile);

		// On passe par une URI pour pouvoir le partager avec les esclaves
		shared_sClassTmpFile.SetValue(FileService::BuildLocalURI(sClassTmpFile));
	}

	// Nettoyage
	if (not bOk)
	{
		if (IsParallel())
			FileService::RemoveFile(sClassTmpFile);
	}

	// Initialisations
	lReadRecords = 0;
	lReadObjects = 0;
	nChunkCurrentIndex = 0;

	// Initialisations dans le cas multi-tables
	lvMappingReadRecords.SetSize(shared_sourceDatabase.GetDatabase()->GetTableNumber());

	// Par defaut, on stocke -1 pour indique que les tables ne sont pas traitees
	for (i = 0; i < lvMappingReadRecords.GetSize(); i++)
		lvMappingReadRecords.SetAt(i, -1);
	return bOk;
}

boolean KWDatabaseTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	require(not shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology() or mtDatabaseIndexer.IsComputed());

	// Cas multi-tables
	if (shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		// Est-ce qu'il y a encore du travail ?
		if (nChunkCurrentIndex >= mtDatabaseIndexer.GetChunkNumber())
			bIsTaskFinished = true;
		else
		{
			/////////////////////////////////////////////////////////////////////////////////////////////////////
			// Gestion de la partie de la base a traiter

			// Recopie de la cle racine
			input_ChunkLastRootKey.GetKey()->CopyFrom(
			    mtDatabaseIndexer.GetChunkLastRootKeyAt(nChunkCurrentIndex));

			// Recopie des vecteurs de position de debut et fin pour l'esclave
			input_lvChunkBeginPos.GetLongintVector()->CopyFrom(
			    mtDatabaseIndexer.GetChunkBeginPositionsAt(nChunkCurrentIndex));
			input_lvChunkEndPos.GetLongintVector()->CopyFrom(
			    mtDatabaseIndexer.GetChunkEndPositionsAt(nChunkCurrentIndex));
			input_lvChunkBeginRecordIndex.GetLongintVector()->CopyFrom(
			    mtDatabaseIndexer.GetChunkBeginRecordIndexesAt(nChunkCurrentIndex));
			nChunkCurrentIndex++;

			// Calcul de la progression
			dTaskPercent = 1.0 / mtDatabaseIndexer.GetChunkNumber();
		}
	}
	// et dans le cas mono-table
	else
	{
		// Est-ce qu'il y a encore du travail ?
		if (nChunkCurrentIndex >= lvFileBeginPositions.GetSize() - 1)
			bIsTaskFinished = true;
		else
		{
			// Preparation des infos de lecture du buffer
			input_lFileBeginPosition = lvFileBeginPositions.GetAt(nChunkCurrentIndex);
			input_lFileBeginRecordIndex = lvFileBeginRecordIndexes.GetAt(nChunkCurrentIndex);
			input_nBufferSize = int(lvFileBeginPositions.GetAt(nChunkCurrentIndex + 1) -
						lvFileBeginPositions.GetAt(nChunkCurrentIndex));
			nChunkCurrentIndex++;
			assert(input_nBufferSize > 0);

			// Calcul de la progression (la derniere valeur de lvFileBeginPositions contient la taille du
			// fichier)
			dTaskPercent =
			    input_nBufferSize * 1.0 / lvFileBeginPositions.GetAt(lvFileBeginPositions.GetSize() - 1);
		}
	}
	return true;
}

boolean KWDatabaseTask::MasterAggregateResults()
{
	int i;
	PLMTDatabaseTextFile* sourceDatabase;

	// Collecte du nombre d'enregistrement lus
	lReadRecords += output_lReadRecords;
	lReadObjects += output_lReadObjects;

	// Cas specifique au multi-tables
	if (shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		// Parcours des tables sources pour recolter le nombre d'enregistrement lus par mapping
		sourceDatabase = shared_sourceDatabase.GetMTDatabase();
		for (i = 0; i < sourceDatabase->GetTableNumber(); i++)
		{
			// Prise en compte si base ouverte (-1 indique une base non ouverte)
			if (output_lvMappingReadRecords.GetAt(i) >= 0)
			{
				// La premiere fois, on passe a 0 pour indique que la base est ouverte
				if (lvMappingReadRecords.GetAt(i) == -1)
					lvMappingReadRecords.SetAt(i, 0);

				// Mise a jour du nombre d'enregistrements lus
				lvMappingReadRecords.UpgradeAt(i, output_lvMappingReadRecords.GetAt(i));
			}
		}
	}

	// Message d'avancement
	shared_sourceDatabase.GetDatabase()->DisplayReadTaskProgressionLabel(lReadRecords, lReadObjects);
	return true;
}

boolean KWDatabaseTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;

	// Initialisation du code retour
	bOk = bProcessEndedCorrectly;

	// On met a 0 le nombre de records lus si necessaire
	if (not bOk)
	{
		lReadRecords = 0;
		lReadObjects = 0;
		lvMappingReadRecords.Initialize();
	}

	// Suppression du fichier dictionnaire
	if (IsParallel())
		FileService::RemoveFile(FileService::GetURIFilePathName(shared_sClassTmpFile.GetValue()));
	return bOk;
}

boolean KWDatabaseTask::SlaveInitialize()
{
	boolean bOk = true;
	bOk = bOk and SlaveInitializePrepareDictionary();
	bOk = bOk and SlaveInitializePrepareDatabase();
	bOk = bOk and SlaveInitializeOpenDatabase();
	return bOk;
}

boolean KWDatabaseTask::SlaveInitializePrepareDictionary()
{
	boolean bOk = true;
	boolean bDisplayMemoryStats = false;
	ALString sClassName;
	ALString sClassTmpFile;

	// Memoire initiale
	if (bDisplayMemoryStats)
	{
		cout << "KWDatabaseTask::SlaveInitializePrepareDictionary, process id\t" << GetProcessId() << endl;
		cout << "KWDatabaseTask::SlaveInitializePrepareDictionary, available memory begin\t"
		     << LongintToHumanReadableString(RMResourceManager::GetRemainingAvailableMemory()) << endl;
	}

	// Chargement du dictionnaire (sauf dans le cas sequentiel ou il est global)
	if (IsParallel())
	{
		assert(KWClassDomain::GetCurrentDomain()->GetClassNumber() == 0);
		sClassName = shared_sourceDatabase.GetDatabase()->GetClassName();

		// Copie du fichier dictionnaire chez l'esclave
		// sauf si on est sur la meme machine
		if (FileService::GetURIHostName(shared_sClassTmpFile.GetValue()) != GetLocalHostName())
		{
			sClassTmpFile =
			    FileService::BuildFilePathName(FileService::GetApplicationTmpDir(), sClassName + ".kdic");
			bOk = PLRemoteFileService::CopyFile(shared_sClassTmpFile.GetValue(), sClassTmpFile);
		}
		else
		{
			sClassTmpFile = FileService::GetURIFilePathName(shared_sClassTmpFile.GetValue());
		}
		if (bOk)
		{
			bOk = KWClassDomain::GetCurrentDomain()->ReadFile(sClassTmpFile);
			if (not bOk)
				AddError("Error while loading dictionary file " +
					 FileService::GetURIUserLabel(sClassTmpFile));
			if (bOk and KWClassDomain::GetCurrentDomain()->LookupClass(sClassName) == NULL)
			{
				bOk = false;
				AddError("Dictionary " + sClassName + " not found");
			}
			if (bOk and not KWClassDomain::GetCurrentDomain()->LookupClass(sClassName)->Check())
			{
				bOk = false;
				AddError("Dictionary " + sClassName + " not valid");
			}
			if (bOk)
			{
				KWClassDomain::GetCurrentDomain()->Compile();
				bOk = KWClassDomain::GetCurrentDomain()->LookupClass(sClassName)->IsCompiled();
				if (not bOk)
					AddError("Dictionary " + sClassName + " not compiled");
			}
		}

		// Destruction du fichier dictionnaire chez l'esclave,
		// desormais inutile si on a du le recopier
		if (FileService::GetURIHostName(shared_sClassTmpFile.GetValue()) != GetLocalHostName())
			FileService::RemoveFile(sClassTmpFile);
	}

	// Memoire finale
	if (bDisplayMemoryStats)
	{
		cout << "KWDatabaseTask::SlaveInitializePrepareDictionary, available memory end\t"
		     << LongintToHumanReadableString(RMResourceManager::GetRemainingAvailableMemory()) << endl;
	}
	return bOk;
}

boolean KWDatabaseTask::SlaveInitializePrepareDatabase()
{
	boolean bOk = true;
	ALString sClassName;
	PLDatabaseTextFile* sourceDatabase;
	longint lSourceDatabaseGrantedMemory;
	int nSourceBufferSize;

	// Acces aux bases
	sourceDatabase = shared_sourceDatabase.GetPLDatabase();

	// Cas specifique au multi-tables
	if (shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		// Les dictionnaire etant disponibles, on peut remettre a jour les mappings
		if (bOk)
		{
			shared_sourceDatabase.GetMTDatabase()->UpdateMultiTableMappings();
			assert(shared_sourceDatabase.GetDatabase()->Check());
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////
	// Gestion des ressources pour determiner la taille des buffers
	// En multi-table, chaque esclave gere ses buffers en fonctionnement du dimensionnement
	// effectue au niveau des esclaves.

	// Calcul des tailles de buffer
	nSourceBufferSize = 0;
	if (bOk)
	{
		// Calcul de la memoire allouee pour la base source
		lSourceDatabaseGrantedMemory = ComputeSlaveGrantedMemory(GetSlaveResourceRequirement(),
									 GetSlaveResourceGrant()->GetMemory(), true);

		// Calcul des tailles de buffer permises par ces memoire allouees
		nSourceBufferSize = sourceDatabase->ComputeOpenBufferSize(true, lSourceDatabaseGrantedMemory);

		// On peut imposer la taille du buffer pour raison de tests
		if (nForcedBufferSize > 0)
			nSourceBufferSize = nForcedBufferSize;
	}

	// Parametrage des tailles de buffer
	if (bOk)
		sourceDatabase->SetBufferSize(nSourceBufferSize);
	return bOk;
}

boolean KWDatabaseTask::SlaveInitializeOpenDatabase()
{
	boolean bOk = true;
	PLMTDatabaseTextFile* sourceDatabase;

	// Ouverture de la base source en lecture
	bOk = bOk and shared_sourceDatabase.GetDatabase()->OpenForRead();

	// Cas multi-tables
	if (bOk and shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		sourceDatabase = shared_sourceDatabase.GetMTDatabase();

		// Creation des buffers, et ouverture en lecture
		bOk = bOk and sourceDatabase->CreateInputBuffers();
		bOk = bOk and sourceDatabase->OpenInputBuffers();
	}

	// Cas mono-tables
	if (bOk and not shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		inputFileMonoTable.SetFileName(shared_sourceDatabase.GetDatabase()->GetDatabaseName());
		inputFileMonoTable.SetFieldSeparator(shared_sourceDatabase.GetSTDatabase()->GetFieldSeparator());
		inputFileMonoTable.SetHeaderLineUsed(shared_sourceDatabase.GetSTDatabase()->GetHeaderLineUsed());
		bOk = inputFileMonoTable.Open();
	}
	return bOk;
}

boolean KWDatabaseTask::SlaveProcess()
{
	boolean bOk = true;

	// Demarage des base
	bOk = bOk and SlaveProcessStartDatabase();

	// Exploitation des base
	bOk = bOk and SlaveProcessExploitDatabase();

	// La methode finale est toujours appelee, pour les nettoyage potentiels
	bOk = SlaveProcessStopDatabase(bOk) and bOk;
	return bOk;
}

boolean KWDatabaseTask::SlaveProcessStartDatabase()
{
	boolean bOk = true;
	boolean bTrace = false;
	KWDatabase* sourceDatabase;
	PLSTDatabaseTextFile* sourceSTDatabase;
	PLMTDatabaseTextFile* sourceMTDatabase;
	int i;
	KWMTDatabaseMapping* mapping;
	PLDataTableDriverTextFile* driver;
	KWObjectKey lastRootObjectKey;
	InputBufferedFile* inputBuffer;
	ALString sChunkFileName;
	int nBufferSize;
	PeriodicTest periodicTestInterruption;
	ALString sTmp;

	// Initialisation des variables de travail
	sourceDatabase = shared_sourceDatabase.GetDatabase();
	sourceSTDatabase = NULL;
	sourceMTDatabase = NULL;

	// Initialisation des variables en sortie
	output_lReadRecords = 0;
	output_lReadObjects = 0;
	output_lvMappingReadRecords.GetLongintVector()->SetSize(sourceDatabase->GetTableNumber());
	output_lvMappingReadRecords.GetLongintVector()->Initialize();

	// Cas specifique au multi-tables
	if (sourceDatabase->IsMultiTableTechnology())
	{
		sourceMTDatabase = shared_sourceDatabase.GetMTDatabase();

		// Parametrage de la derniere cle racine lue, apres avoir transforme la cle de fichier (KWKey) en cle
		// d'objet (KWObjectKey)
		if (bOk)
		{
			lastRootObjectKey.SetSize(input_ChunkLastRootKey.GetKey()->GetSize());
			for (i = 0; i < lastRootObjectKey.GetSize(); i++)
				lastRootObjectKey.SetAt(i, (Symbol)input_ChunkLastRootKey.GetKey()->GetAt(i));
			sourceMTDatabase->SetLastReadRootKey(&lastRootObjectKey);
		}

		// Parcours des mapping pour lire les buffers en lecture
		if (bOk)
		{
			for (i = 0; i < sourceMTDatabase->GetTableNumber(); i++)
			{
				mapping =
				    cast(KWMTDatabaseMapping*, sourceMTDatabase->GetMultiTableMappings()->GetAt(i));

				// Parametrage du buffer en lecture
				inputBuffer = sourceMTDatabase->GetInputBufferedFileAt(mapping);
				if (inputBuffer != NULL)
				{
					assert(inputBuffer->IsOpened());

					// Parametrage de la portion du fichier a lire par le driver
					driver = sourceMTDatabase->GetDriverAt(mapping);
					driver->SetBeginPosition(input_lvChunkBeginPos.GetAt(i));
					driver->SetEndPosition(input_lvChunkEndPos.GetAt(i));
					driver->SetRecordIndex(input_lvChunkBeginRecordIndex.GetAt(i));

					// Parametrage de la taille du buffer
					nBufferSize = sourceMTDatabase->GetBufferSize();
					if (nBufferSize > input_lvChunkEndPos.GetAt(i) - input_lvChunkBeginPos.GetAt(i))
						nBufferSize = (int)(input_lvChunkEndPos.GetAt(i) -
								    input_lvChunkBeginPos.GetAt(i));
					inputBuffer->SetBufferSize(nBufferSize);

					// On remet a 0 le nombre de records traites
					driver->SetUsedRecordNumber(0);

					// Lecture du pemier buffer
					bOk = driver->FillFirstInputBuffer();
					if (not bOk)
						break;

					// Cas particulier : traitement du header
					if (GetTaskIndex() == 0 and inputBuffer->GetHeaderLineUsed())
					{
						// On ne lit pas la premiere ligne du fichier
						inputBuffer->SkipLine();

						// Ne pas oublier de relire le buffer si necessaire
						driver->SetRecordIndex(driver->GetRecordIndex() + 1);
						driver->UpdateInputBuffer();
						bOk = not driver->IsError();
						if (not bOk)
							break;
					}

					// Trace
					if (bTrace)
					{
						cout << "Slave\t" << GetTaskIndex() << "\tinput"
						     << "\t" << i << "\t"
						     << cast(KWMTDatabaseMapping*,
							     sourceMTDatabase->GetMultiTableMappings()->GetAt(i))
							    ->GetDataPath()
						     << "\t" << input_lvChunkBeginRecordIndex.GetAt(i) << "\t"
						     << input_lvChunkBeginPos.GetAt(i) << "\t"
						     << input_lvChunkEndPos.GetAt(i) << "\t"
						     << *input_ChunkLastRootKey.GetKey() << endl;
					}
				}
			}
		}
	}
	// Cas mono-table
	else
	{
		sourceSTDatabase = shared_sourceDatabase.GetSTDatabase();

		// Parametrage du buffer de lecture
		if (bOk)
		{
			// Lecture et remplissage du buffer
			inputFileMonoTable.SetBufferSize(input_nBufferSize);
			bOk = inputFileMonoTable.Fill(input_lFileBeginPosition);
			if (bOk)
			{
				// Initialisation des buffers des databases
				sourceSTDatabase->SetInputBuffer(&inputFileMonoTable);
				sourceSTDatabase->GetDriver()->SetBeginPosition(input_lFileBeginPosition);
				sourceSTDatabase->GetDriver()->SetEndPosition(input_lFileBeginPosition +
									      input_nBufferSize);
				assert(inputFileMonoTable.GetCurrentBufferSize() == inputFileMonoTable.GetBufferSize());

				// Initialisation du nombre de lignes du fichier d'entree
				sourceSTDatabase->GetDriver()->SetRecordIndex(input_lFileBeginRecordIndex);

				// Cas particulier : traitement du header
				if (GetTaskIndex() == 0 and inputFileMonoTable.GetHeaderLineUsed())
				{
					// On ne lit pas la premiere ligne du fichier
					inputFileMonoTable.SkipLine();

					// On indique au driver qu'une ligne a ete sautee
					sourceSTDatabase->GetDriver()->SetRecordIndex(
					    sourceSTDatabase->GetDriver()->GetRecordIndex() + 1);
				}
			}
		}
	}
	return bOk;
}

boolean KWDatabaseTask::SlaveProcessExploitDatabase()
{
	boolean bOk = true;
	KWDatabase* sourceDatabase;
	longint lObjectNumber;
	longint lRecordNumber;
	KWMTDatabaseMapping* mapping;
	KWObjectKey lastRootObjectKey;
	KWObject* kwoObject;
	ALString sChunkFileName;
	PeriodicTest periodicTestInterruption;
	PLDataTableDriverTextFile* rootDriver;
	double dProgression;
	ALString sTmp;

	// Acces a la base source
	sourceDatabase = shared_sourceDatabase.GetDatabase();

	// Parcours des objets de la base
	lObjectNumber = 0;
	lRecordNumber = 0;
	if (bOk)
	{
		// Dans le cas multi-tables, acces au buffer de la table racine, pour la gestion de la progression
		rootDriver = NULL;
		if (sourceDatabase->IsMultiTableTechnology())
		{
			mapping = cast(KWMTDatabaseMapping*,
				       shared_sourceDatabase.GetMTDatabase()->GetMultiTableMappings()->GetAt(0));
			rootDriver = shared_sourceDatabase.GetMTDatabase()->GetDriverAt(mapping);
		}

		// Parcours des objets sources
		Global::ActivateErrorFlowControl();
		while (not sourceDatabase->IsEnd())
		{
			// Suivi de la tache
			if (periodicTestInterruption.IsTestAllowed(lRecordNumber))
			{
				// Avancement selon le type de base
				if (sourceDatabase->IsMultiTableTechnology())
					dProgression = rootDriver->GetReadPercentage();
				else
					dProgression = inputFileMonoTable.GetCurrentLineNumber() * 1.0 /
						       max(1, inputFileMonoTable.GetBufferLineNumber());
				TaskProgression::DisplayProgression((int)floor(dProgression * 100));

				// Message d'avancement, uniquement dans la premiere tache (la seule ou les comptes sont
				// corrects)
				if (GetTaskIndex() == 0)
					sourceDatabase->DisplayReadTaskProgressionLabel(lRecordNumber, lObjectNumber);
			}

			// Lecture (la gestion de l'avancement se fait dans la methode Read)
			kwoObject = sourceDatabase->Read();
			lRecordNumber++;
			if (kwoObject != NULL)
			{
				lObjectNumber++;

				// Appel de la methode exploitant l'objet
				bOk = SlaveProcessExploitDatabaseObject(kwoObject);

				// Destruction de l'objet
				delete kwoObject;
				if (not bOk)
					break;
			}
			// Arret si interruption utilisateur (deja detectee avant et ayant donc rendu un objet NULL)
			else if (TaskProgression::IsInterruptionRequested())
			{
				assert(kwoObject == NULL);
				bOk = false;
				break;
			}

			// Arret si erreur de lecture
			if (sourceDatabase->IsError())
			{
				sourceDatabase->AddError(GetTaskLabel() + " interrupted because of read errors");
				bOk = false;
				break;
			}
		}
		Global::DesactivateErrorFlowControl();
	}

	// On renvoi le nombre d'object lus
	if (bOk)
	{
		output_lReadRecords = lRecordNumber;
		output_lReadObjects = lObjectNumber;
	}
	return bOk;
}

boolean KWDatabaseTask::SlaveProcessExploitDatabaseObject(const KWObject* kwoObject)
{
	boolean bOk = true;
	require(kwoObject != NULL);
	return bOk;
}

boolean KWDatabaseTask::SlaveProcessStopDatabase(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;
	boolean bTrace = false;
	PLMTDatabaseTextFile* sourceMTDatabase;
	int i;
	KWMTDatabaseMapping* mapping;

	// On renvoie le nombre de records traites par mapping en en entree
	// et nettoie le contexte de travail des mapping en entree (last key et last record)
	bOk = bProcessEndedCorrectly;
	if (shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		sourceMTDatabase = shared_sourceDatabase.GetMTDatabase();
		for (i = 0; i < sourceMTDatabase->GetTableNumber(); i++)
		{
			mapping = cast(KWMTDatabaseMapping*, sourceMTDatabase->GetMultiTableMappings()->GetAt(i));

			// Memorisation du nombre d'enregistrements par base ouverte
			if (sourceMTDatabase->IsMappingInitialized(mapping))
			{
				// Trace
				if (bTrace)
				{
					cout << "Slave\t" << GetTaskIndex() << "\toutput read records"
					     << "\t" << i << "\t"
					     << LongintToReadableString(
						    sourceMTDatabase->GetDriverAt(mapping)->GetUsedRecordNumber())
					     << endl;
				}
				// Nettoyage du contexte de travail
				sourceMTDatabase->CleanMapping(mapping);

				// Memorisation de la variable en sortie
				if (bOk)
					output_lvMappingReadRecords.SetAt(
					    i, sourceMTDatabase->GetDriverAt(mapping)->GetUsedRecordNumber());
			}
			// On memorise -1 pour les base non ouvertes
			else
				output_lvMappingReadRecords.SetAt(i, -1);
		}
	}
	return bOk;
}

boolean KWDatabaseTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;
	PLMTDatabaseTextFile* sourceDatabase;
	boolean bCloseOk;

	// Cas multi-tables
	if (shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		// Acces a la base source
		sourceDatabase = shared_sourceDatabase.GetMTDatabase();

		// Fermeture et de destruction des buffers d'entree et de sortie
		bOk = sourceDatabase->CloseInputBuffers() and bOk;
		bOk = sourceDatabase->DeleteInputBuffers() and bOk;
	}
	// Cas mono-table
	else
	{
		if (inputFileMonoTable.IsOpened())
		{
			bOk = inputFileMonoTable.Close() and bOk;
			inputFileMonoTable.CleanBuffer();
		}
	}

	// Fermeture des bases
	if (shared_sourceDatabase.GetDatabase()->IsOpenedForRead())
	{
		bCloseOk = shared_sourceDatabase.GetDatabase()->Close();
		bOk = bOk and bCloseOk;
		if (not bCloseOk)
			AddError("Error while closing input database " +
				 FileService::GetURIUserLabel(shared_sourceDatabase.GetDatabase()->GetDatabaseName()));
	}

	// Destruction du dictionnaire
	if (IsParallel())
		KWClassDomain::GetCurrentDomain()->DeleteAllClasses();
	return bOk;
}

longint KWDatabaseTask::ComputeSlaveGrantedMemory(const RMResourceRequirement* slaveRequirement,
						  longint lSlaveGrantedMemory, boolean bForSourceDatabase)
{
	boolean bDisplay = false;
	PLDatabaseTextFile* sourceDatabase;
	longint lMinNecessaryMemory;
	longint lMaxNecessaryMemory;
	double dMemoryRatio;
	longint lSourceDatabaseGrantedMemory;

	require(slaveRequirement != NULL);
	require(lSlaveGrantedMemory >= 0);

	// Acces aux bases
	sourceDatabase = shared_sourceDatabase.GetPLDatabase();

	// Acces a la memoire totale allouee pour le prochain esclave
	lMinNecessaryMemory = slaveRequirement->GetMemory()->GetMin();
	lMaxNecessaryMemory = slaveRequirement->GetMemory()->GetMax();
	assert(lMinNecessaryMemory <= lSlaveGrantedMemory and lSlaveGrantedMemory <= lMaxNecessaryMemory);

	// On dispatche la memoire allouee au prorata de la demande la base source
	if (lSlaveGrantedMemory <= lMinNecessaryMemory)
		lSourceDatabaseGrantedMemory = sourceDatabase->GetMinOpenNecessaryMemory();
	else if (lSlaveGrantedMemory >= lMaxNecessaryMemory)
		lSourceDatabaseGrantedMemory = sourceDatabase->GetMaxOpenNecessaryMemory();
	else
	{
		dMemoryRatio =
		    (lSlaveGrantedMemory - lMinNecessaryMemory) * 1.0 / (lMaxNecessaryMemory - lMinNecessaryMemory);
		lSourceDatabaseGrantedMemory = (longint)(sourceDatabase->GetMinOpenNecessaryMemory() +
							 dMemoryRatio * (sourceDatabase->GetMaxOpenNecessaryMemory() -
									 sourceDatabase->GetMinOpenNecessaryMemory()));
	}

	// Affichage detaille
	if (bDisplay)
	{
		cout << "KWDatabaseTask::ComputeSlaveGrantedMemory " << GetTaskName() << endl;
		cout << "\tSlave min necessary memory\t" << lMinNecessaryMemory << endl;
		cout << "\tSlave max necessary memory\t" << lMaxNecessaryMemory << endl;
		cout << "\tSource database min necessary memory\t" << sourceDatabase->GetMinOpenNecessaryMemory()
		     << endl;
		cout << "\tSource database max necessary memory\t" << sourceDatabase->GetMaxOpenNecessaryMemory()
		     << endl;
		cout << "\tSlave granted memory\t" << lSlaveGrantedMemory << endl;
		cout << "\tSource database granted memory\t" << lSourceDatabaseGrantedMemory << endl;
	}

	// On retourne la part alloue revenant a la base source ou au reste de la tache
	if (bForSourceDatabase)
		return lSourceDatabaseGrantedMemory;
	else
		return lSlaveGrantedMemory - lSourceDatabaseGrantedMemory;
}