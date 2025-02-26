// Copyright (c) 2023-2025 Orange. All rights reserved.
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

	// Indexeur de base de donnee reutiulisable par plusieurs taches
	reusableDatabaseIndexer = NULL;

	// Gestion des chunks dans le cas multi-tables
	nChunkCurrentIndex = 0;
	nForcedBufferSize = 0;
	lForcedMaxFileSizePerProcess = 0;

	// Declaration des variables partagees
	DeclareSharedParameter(&shared_sClassTmpFile);
	DeclareSharedParameter(&shared_sourceDatabase);

	// Declaration des variables en entree de l'esclave dans le cas multi-tables
	DeclareTaskInput(&input_lvChunkBeginPositions);
	DeclareTaskInput(&input_lvChunkEndPositions);
	DeclareTaskInput(&input_lvChunkPreviousRecordIndexes);
	DeclareTaskInput(&input_ChunkLastMainKey);
	DeclareTaskInput(&input_lFileBeginPosition);
	DeclareTaskInput(&input_lFileEndPosition);
	DeclareTaskInput(&input_lFilePreviousRecordIndex);

	// Resultats envoyes par l'esclave dans le cas general
	DeclareTaskOutput(&output_lReadRecords);
	DeclareTaskOutput(&output_lReadObjects);

	// Resultats envoyes par l'esclave dans le cas multi-tables
	DeclareTaskOutput(&output_lvMappingReadRecords);
}

KWDatabaseTask::~KWDatabaseTask() {}

void KWDatabaseTask::SetReusableDatabaseIndexer(KWDatabaseIndexer* databaseIndexer)
{
	reusableDatabaseIndexer = databaseIndexer;
}

KWDatabaseIndexer* KWDatabaseTask::GetReusableDatabaseIndexer() const
{
	return reusableDatabaseIndexer;
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
	boolean bDisplay = false;
	KWMTDatabaseTextFile refMTDatabaseTextFile;
	KWSTDatabaseTextFile refSTDatabaseTextFile;

	require(sourceDatabase != NULL);
	require(sourceDatabase->Check());
	require(sourceDatabase->GetTechnologyName() == refMTDatabaseTextFile.GetTechnologyName() or
		sourceDatabase->GetTechnologyName() == refSTDatabaseTextFile.GetTechnologyName());

	// Parametrage du ChunkBuilder par le DatabaseIndexer reutilisable s'il est disponible
	if (reusableDatabaseIndexer != NULL)
		databaseChunkBuilder.SetDatabaseIndexer(reusableDatabaseIndexer);
	// Parametre par le DatabaseIndexer specifique a la tache sinon
	else
		databaseChunkBuilder.SetDatabaseIndexer(&taskDatabaseIndexer);

	// On force l'indexation dans le cas parallele simule
	if (PLParallelTask::GetParallelSimulated())
	{
		// On ne la force que la premiere fois, pour reutiliser l'indexation
		if (databaseChunkBuilder.GetDatabaseIndexer()->GetMaxSlaveNumber() == 0)
		{
			databaseChunkBuilder.GetDatabaseIndexer()->SetMaxSlaveNumber(
			    PLParallelTask::GetSimulatedSlaveNumber());

			// On force egalement une petite taille de chunks
			databaseChunkBuilder.GetDatabaseIndexer()->SetMaxTotalFileSizePerProcess(lMB);
		}
	}

	// Parametrage de la base
	databaseChunkBuilder.InitializeFromDatabase(sourceDatabase);

	// Reinitialisation de la variable partagee, pour ne pas la detruire
	shared_sourceDatabase.RemoveObject();

	// Initialisation des bases de travail
	shared_sourceDatabase.SetPLDatabase(
	    cast(PLDatabaseTextFile*, databaseChunkBuilder.GetDatabaseIndexer()->GetPLDatabase()));

	// Affichage
	if (bDisplay)
	{
		cout << "Database task\t" << GetTaskName() << endl;
		cout << "\tDatabase indexer\t" << databaseChunkBuilder.GetDatabaseIndexer() << endl;
		if (reusableDatabaseIndexer != NULL)
			cout << "\tReusable database indexer" << endl;
		cout << "\tIs indexation available\t"
		     << databaseChunkBuilder.GetDatabaseIndexer()->IsIndexationComputed() << endl;
		cout << "\tClass\t"
		     << databaseChunkBuilder.GetDatabaseIndexer()->GetPLDatabase()->GetDatabase()->GetClassName()
		     << endl;
		cout << "\tDatabase\t" << databaseChunkBuilder.GetDatabaseIndexer()->GetPLDatabase()->GetDatabase()
		     << endl;
		cout << "\tDatabase name\t"
		     << databaseChunkBuilder.GetDatabaseIndexer()->GetPLDatabase()->GetDatabase()->GetDatabaseName()
		     << endl;
		cout << "\tSample rate\t"
		     << databaseChunkBuilder.GetDatabaseIndexer()
			    ->GetPLDatabase()
			    ->GetDatabase()
			    ->GetSampleNumberPercentage()
		     << endl;
		cout << "\tSampling mode\t"
		     << databaseChunkBuilder.GetDatabaseIndexer()->GetPLDatabase()->GetDatabase()->GetSamplingMode()
		     << endl;
	}

	// Calcul du plan d'indexation des tables
	bOk = bOk and ComputeAllDataTableIndexation();

	// Installation du handler specifique pour ignorer le flow des erreur dans le cas du memory guard
	KWDatabaseMemoryGuard::InstallMemoryGuardErrorFlowIgnoreFunction();

	// Lancement de la tache
	CleanJobResults();
	if (bOk)
		bOk = Run();

	// Desinstallation du handler specifique pour ignorer le flow des erreur dans le cas du memory guard
	KWDatabaseMemoryGuard::UninstallMemoryGuardErrorFlowIgnoreFunction();

	// Affichage des messages de la tache
	DisplayTaskMessage();

	// Nettoyage des informations d'indexation
	CleanAllDataTableIndexation();

	// On dereference la variable partagee en fin de tache, car elle appartient au DatabaseIndexer
	shared_sourceDatabase.RemoveObject();
	return bOk;
}

void KWDatabaseTask::DisplayTaskMessage()
{
	require(shared_sourceDatabase.GetPLDatabase() != NULL);

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
		if (databaseChunkBuilder.GetDatabaseIndexer()->IsInterruptedByUser() or
		    (IsJobDone() and IsTaskInterruptedByUser()))
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
	require(shared_sourceDatabase.GetPLDatabase() != NULL);
	shared_sourceDatabase.GetPLDatabase()->DisplayAllTableMessages("Input database", "Read records", lReadRecords,
								       lReadObjects, &lvMappingReadRecords);
}

boolean KWDatabaseTask::ComputeAllDataTableIndexation()
{
	boolean bOk = true;
	boolean bDisplay = false;
	PLDatabaseTextFile* sourceDatabase;
	RMTaskResourceGrant grantedResources;
	KWFileIndexerTask fileIndexerTask;
	int nSlaveNumber;
	longint lMinSlaveGrantedMemoryForSourceDatabase;
	ObjectArray oaAllTableFoundKeyPositions;
	Timer timer;

	require(shared_sourceDatabase.GetDatabase()->Check());

	/////////////////////////////////////////////////////////////////////////////////////////
	// Collectes d'informations prealables sur les tables a traiter en lecture ou en ecriture

	// Acces a la base source
	sourceDatabase = shared_sourceDatabase.GetPLDatabase();
	timer.Start();

	// Calcul d'informations lie a l'ouverture des bases
	bOk = bOk and ComputeDatabaseOpenInformation();

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
			lMinSlaveGrantedMemoryForSourceDatabase = ComputeSlaveGrantedMemory(
			    GetResourceRequirements()->GetSlaveRequirement(), grantedResources.GetSlaveMemory(), true);
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
		// Indexation de la base
		bOk = databaseChunkBuilder.BuildChunks(nSlaveNumber, lMinSlaveGrantedMemoryForSourceDatabase,
						       lForcedMaxFileSizePerProcess);

		// Affichage des resultats d'indexation et des chunks
		if (bDisplay)
		{
			cout << "=== " << GetTaskName() << " ===\n";
			cout << *databaseChunkBuilder.GetDatabaseIndexer() << endl;
			cout << databaseChunkBuilder << endl;
		}
	}

	// Memorisation du temps d'indexation
	timer.Stop();
	dDatabaseIndexerTime = timer.GetElapsedTime();
	return bOk;
}

int KWDatabaseTask::GetIndexationSlaveProcessNumber()
{
	return databaseChunkBuilder.GetChunkNumber();
}

void KWDatabaseTask::CleanAllDataTableIndexation()
{
	databaseChunkBuilder.CleanChunks();
}

boolean KWDatabaseTask::ComputeDatabaseOpenInformation()
{
	boolean bOk;

	// Calcul de la memoire necessaire et collecte des informations permettant d'ouvrir les bases dans les esclaves
	// Attention: on ne prend en compte qu'une seule fois le dictionaire de la base, qui est partage entre
	// les bases en lecture et en ecriture
	bOk = shared_sourceDatabase.GetPLDatabase()->ComputeOpenInformation(true, true, NULL);
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

	// Acces au nombre de taches elementaires resultant de l'indexation (0 si non calcule)
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
	ALString sAlphaNumClassName;
	int i;
	char c;

	require(databaseChunkBuilder.IsComputed());

	// Ecriture du fichier dictionnaire dans un repertoire temporaire pour passage au slave
	if (IsParallel())
	{
		// Le nom du dictionnaire temporaire ne doit contenir que des alphanumeriques. Sinon, on remplace les char par '_'
		for (i = 0; i < shared_sourceDatabase.GetDatabase()->GetClassName().GetLength(); i++)
		{
			c = shared_sourceDatabase.GetDatabase()->GetClassName().GetAt(i);
			if (isalnum(c))
				sAlphaNumClassName += c;
			else
				sAlphaNumClassName += '_';
		}

		// Construction du nom du dictionnaire temporaire
		sClassTmpFile = FileService::CreateUniqueTmpFile(sAlphaNumClassName + ".kdic", this);
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

	// Initialisation de la base via sa variable partagee, notamment pour le dimensionnement de ses ressources
	// propres
	if (bOk)
		bOk = MasterInitializeDatabase();

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

boolean KWDatabaseTask::MasterInitializeDatabase()
{
	boolean bOk = true;
	ALString sClassName;
	PLDatabaseTextFile* sourceDatabase;
	longint lSourceDatabaseGrantedMemory;
	int nSourceBufferSize;

	///////////////////////////////////////////////////////////////////////////////////////
	// Gestion des ressources pour determiner la taille des buffers
	// En multi-table, chaque esclave gere ses buffers en fonctionnement du dimensionnement
	// effectue au niveau des esclaves.

	// Acces a la base
	sourceDatabase = shared_sourceDatabase.GetPLDatabase();

	// Parametrage de l'ouverture a la demande dans le cas multi-tables
	if (shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		// On passe en mode ouverture a la demande s'il y a trop de tables locales (avec un petite marge)
		if (shared_sourceDatabase.GetMTDatabase()->GetMainLocalTableNumber() + 10 > GetMaxOpenedFileNumber())
			shared_sourceDatabase.GetMTDatabase()->SetOpenOnDemandMode(true);
	}

	// Calcul des tailles de buffer et gestion du MemoryGuard
	nSourceBufferSize = 0;
	if (bOk)
	{
		// Calcul de la memoire allouee pour la base source
		lSourceDatabaseGrantedMemory = ComputeSlaveGrantedMemory(GetSlaveResourceRequirement(),
									 GetSlaveResourceGrant()->GetMemory(), true);

		// Calcul des tailles de buffer permises par ces memoire allouees
		nSourceBufferSize =
		    sourceDatabase->ComputeOpenBufferSize(true, lSourceDatabaseGrantedMemory, GetProcessNumber());

		// On peut imposer la taille du buffer pour raison de tests
		if (nForcedBufferSize > 0)
			nSourceBufferSize = nForcedBufferSize;

		// Parametrage du MemoryGuard dans le cas multi-tables
		if (shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
		{
			shared_sourceDatabase.GetMTDatabase()->SetMemoryGuardMaxSecondaryRecordNumber(
			    shared_sourceDatabase.GetMTDatabase()->GetEstimatedMaxSecondaryRecordNumber());
			shared_sourceDatabase.GetMTDatabase()->SetMemoryGuardSingleInstanceMemoryLimit(
			    shared_sourceDatabase.GetMTDatabase()->ComputeEstimatedSingleInstanceMemoryLimit(
				lSourceDatabaseGrantedMemory));
		}
	}

	// Parametrage des tailles de buffer
	if (bOk)
		sourceDatabase->SetBufferSize(nSourceBufferSize);
	return bOk;
}

boolean KWDatabaseTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	require(databaseChunkBuilder.IsComputed());

	// Est-ce qu'il y a encore du travail ?
	if (nChunkCurrentIndex >= databaseChunkBuilder.GetChunkNumber() or
	    shared_sourceDatabase.GetPLDatabase()->GetFileSizeAt(0) == 0)
		bIsTaskFinished = true;
	else
	{
		/////////////////////////////////////////////////////////////////////////////////////////////////////
		// Gestion de la partie de la base a traiter

		// Cas multi-tables
		if (shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
		{
			// Recopie de la cle principale
			databaseChunkBuilder.GetChunkLastMainKeyAt(nChunkCurrentIndex, input_ChunkLastMainKey.GetKey());

			// Recopie des vecteurs de position de debut et fin pour l'esclave
			databaseChunkBuilder.GetChunkPreviousRecordIndexesAt(
			    nChunkCurrentIndex, input_lvChunkPreviousRecordIndexes.GetLongintVector());
			databaseChunkBuilder.GetChunkBeginPositionsAt(nChunkCurrentIndex,
								      input_lvChunkBeginPositions.GetLongintVector());
			databaseChunkBuilder.GetChunkEndPositionsAt(nChunkCurrentIndex,
								    input_lvChunkEndPositions.GetLongintVector());
			nChunkCurrentIndex++;
		}
		// Cas mono-table
		else
		{
			// Preparation des infos de lecture du buffer
			input_lFileBeginPosition = databaseChunkBuilder.GetChunkBeginPositionAt(nChunkCurrentIndex);
			input_lFileEndPosition = databaseChunkBuilder.GetChunkEndPositionAt(nChunkCurrentIndex);
			input_lFilePreviousRecordIndex =
			    databaseChunkBuilder.GetChunkPreviousRecordIndexAt(nChunkCurrentIndex);
			nChunkCurrentIndex++;
		}

		// Calcul de la progression
		dTaskPercent = 1.0 / databaseChunkBuilder.GetChunkNumber();
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

	// Cas mono-table
	if (bOk and not shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		// Parametrage du buffer en entree de la table
		inputFileMonoTable.SetFileName(shared_sourceDatabase.GetDatabase()->GetDatabaseName());
		inputFileMonoTable.SetFieldSeparator(shared_sourceDatabase.GetSTDatabase()->GetFieldSeparator());
		inputFileMonoTable.SetHeaderLineUsed(shared_sourceDatabase.GetSTDatabase()->GetHeaderLineUsed());
		shared_sourceDatabase.GetSTDatabase()->SetInputBuffer(&inputFileMonoTable);

		// Ouverture du buffer en entree
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
	KWObjectKey lastMainObjectKey;
	InputBufferedFile* inputBuffer;
	boolean bLineTooLong;
	ALString sChunkFileName;
	int nBufferSize;
	longint lChunkSize;
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

		// Parametrage de la derniere cle principale lue, apres avoir transforme la cle de fichier (KWKey) en cle
		// d'objet (KWObjectKey)
		if (bOk)
		{
			lastMainObjectKey.SetSize(input_ChunkLastMainKey.GetKey()->GetSize());
			for (i = 0; i < lastMainObjectKey.GetSize(); i++)
				lastMainObjectKey.SetAt(i, (Symbol)input_ChunkLastMainKey.GetKey()->GetAt(i));
			sourceMTDatabase->SetLastReadMainKey(&lastMainObjectKey);
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
					driver->SetBeginPosition(input_lvChunkBeginPositions.GetAt(i));
					driver->SetEndPosition(input_lvChunkEndPositions.GetAt(i));
					driver->SetRecordIndex(input_lvChunkPreviousRecordIndexes.GetAt(i));

					// Parametrage de la taille du buffer
					nBufferSize = sourceMTDatabase->GetBufferSize();
					lChunkSize =
					    input_lvChunkEndPositions.GetAt(i) - input_lvChunkBeginPositions.GetAt(i);
					if (nBufferSize > lChunkSize)
						nBufferSize = (int)lChunkSize;
					inputBuffer->SetBufferSize(nBufferSize);

					// On remet a 0 le nombre de records traites
					driver->SetUsedRecordNumber(0);

					// Lecture du premier buffer
					bOk = driver->FillFirstInputBuffer();
					if (not bOk)
						break;

					// Cas particulier : traitement du header
					// Il n'est pas utile ici d'avoir un warning en cas de ligne trop longue
					if (GetTaskIndex() == 0 and inputBuffer->GetHeaderLineUsed())
					{
						// On ne lit pas la premiere ligne du fichier
						if (not driver->IsEnd())
							driver->SetRecordIndex(driver->GetRecordIndex() + 1);
						inputBuffer->SkipLine(bLineTooLong);

						// Ne pas oublier de relire le buffer si necessaire
						driver->UpdateInputBuffer();
						bOk = not driver->IsError();
						if (not bOk)
							break;
					}

					// Trace
					if (bTrace)
					{
						if (GetTaskIndex() == 0 and i == 0)
							cout << "Task\tSlave\tTask\tinput\tTable\tData path\tPrev "
								"index\tChunk begin\tChunk end\tLast key\tBufferSize\n";
						cout << GetTaskLabel() << "\tSlave\t" << GetTaskIndex() << "\tinput"
						     << "\t" << i << "\t"
						     << cast(KWMTDatabaseMapping*,
							     sourceMTDatabase->GetMultiTableMappings()->GetAt(i))
							    ->GetDataPath()
						     << "\t" << input_lvChunkPreviousRecordIndexes.GetAt(i) << "\t"
						     << input_lvChunkBeginPositions.GetAt(i) << "\t"
						     << input_lvChunkEndPositions.GetAt(i) << "\t"
						     << *input_ChunkLastMainKey.GetKey() << "\t"
						     << sourceMTDatabase->GetBufferSize() << endl;
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
			assert(sourceSTDatabase->GetInputBuffer() == &inputFileMonoTable);

			// Parametrage de la taille du buffer
			inputBuffer = sourceSTDatabase->GetInputBuffer();
			nBufferSize = sourceSTDatabase->GetBufferSize();
			lChunkSize = input_lFileEndPosition - input_lFileBeginPosition;
			if (nBufferSize > lChunkSize and lChunkSize > 0)
				nBufferSize = (int)lChunkSize;
			inputBuffer->SetBufferSize(nBufferSize);

			// Initialisation des buffers des databases
			driver = sourceSTDatabase->GetDriver();
			driver->SetBeginPosition(input_lFileBeginPosition);
			driver->SetEndPosition(input_lFileEndPosition);

			// Initialisation du nombre de lignes du fichier d'entree
			driver->SetRecordIndex(input_lFilePreviousRecordIndex);

			// Lecture du premier buffer
			bOk = driver->FillFirstInputBuffer();

			// Cas particulier : traitement du header
			// Il n'est pas utile ici d'avoir un warning en cas de ligne trop longue
			if (bOk and GetTaskIndex() == 0 and inputBuffer->GetHeaderLineUsed())
			{
				// On ne lit pas la premiere ligne du fichier
				if (not driver->IsEnd())
					driver->SetRecordIndex(driver->GetRecordIndex() + 1);
				inputBuffer->SkipLine(bLineTooLong);

				// Ne pas oublier de relire le buffer si necessaire
				driver->UpdateInputBuffer();
				bOk = not driver->IsError();
			}

			// Trace
			if (bTrace)
			{
				if (GetTaskIndex() == 0)
					cout << "Task\tSlave\tTask\tinput\tTable\tData path\tPrev index\tChunk "
						"begin\tChunk end\tLast key\tBufferSize\n";
				cout << GetTaskLabel() << "\tSlave\t" << GetTaskIndex() << "\t"
				     << sourceSTDatabase->GetDatabaseName() << "\t" << input_lFilePreviousRecordIndex
				     << "\t" << input_lFileBeginPosition << "\t" << input_lFileEndPosition << "\t"
				     << sourceSTDatabase->GetBufferSize() << endl;
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
	KWObject* kwoObject;
	ALString sChunkFileName;
	PLDataTableDriverTextFile* mainDriver;
	double dProgression;
	ALString sTmp;

	// Acces a la base source
	sourceDatabase = shared_sourceDatabase.GetDatabase();

	// Parcours des objets de la base
	lObjectNumber = 0;
	lRecordNumber = 0;
	if (bOk)
	{
		// Dans le cas multi-tables, acces au driver de la table principale, pour la gestion de la progression
		mainDriver = NULL;
		if (sourceDatabase->IsMultiTableTechnology())
		{
			mapping = cast(KWMTDatabaseMapping*,
				       shared_sourceDatabase.GetMTDatabase()->GetMultiTableMappings()->GetAt(0));
			mainDriver = shared_sourceDatabase.GetMTDatabase()->GetDriverAt(mapping);
		}
		// Sinon, on prend le driver de la base mono-table
		else
			mainDriver = shared_sourceDatabase.GetSTDatabase()->GetDriver();

		// Parcours des objets sources
		Global::ActivateErrorFlowControl();
		while (not sourceDatabase->IsEnd())
		{
			// Suivi de la tache
			if (TaskProgression::IsRefreshNecessary())
			{
				// Avancement
				dProgression = mainDriver->GetReadPercentage();
				TaskProgression::DisplayProgression((int)floor(dProgression * 100));

				// Message d'avancement, uniquement dans la premiere tache (la seule ou les comptes sont
				// corrects)
				if (GetTaskIndex() == 0)
					sourceDatabase->DisplayReadTaskProgressionLabel(lRecordNumber, lObjectNumber);

				// Arret si interruption utilisateur
				if (TaskProgression::IsInterruptionRequested())
				{
					bOk = false;
					break;
				}
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

			// Arret si erreur de lecture
			// On emet pas de message d'erreur, vcar le message ddetaille a deja ete emis precedemment,
			// et le message synthetique sera emis par l'appelant
			if (sourceDatabase->IsError())
			{
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
	PLSTDatabaseTextFile* sourceSTDatabase;
	int i;
	KWMTDatabaseMapping* mapping;
	PLDataTableDriverTextFile* driver;

	// Dans le cas multi-table, on renvoie le nombre de records traites par mapping en entree
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
	// Dans le cas mono-table, on nettoie le contexte de travail
	else
	{
		sourceSTDatabase = shared_sourceDatabase.GetSTDatabase();

		// Nettoyage du contexte de travail
		driver = sourceSTDatabase->GetDriver();
		check(driver);
		driver->SetBeginPosition(0);
		driver->SetEndPosition(0);
		driver->SetRecordIndex(0);
	}
	return bOk;
}

boolean KWDatabaseTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;
	PLMTDatabaseTextFile* sourceMTDatabase;
	PLSTDatabaseTextFile* sourceSTDatabase;
	boolean bCloseOk;

	// Initialisation
	bOk = bProcessEndedCorrectly and not TaskProgression::IsInterruptionRequested();

	// Cas multi-tables
	if (shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		// Acces a la base source
		sourceMTDatabase = shared_sourceDatabase.GetMTDatabase();

		// Fermeture et de destruction des buffers d'entree et de sortie
		bOk = sourceMTDatabase->CloseInputBuffers() and bOk;
		bOk = sourceMTDatabase->DeleteInputBuffers() and bOk;
	}
	// Cas mono-table
	else
	{
		// Fermeture de l'input buffer mono-table
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

	// Nettoyage du driver dans le cas mono-table
	if (not shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		sourceSTDatabase = shared_sourceDatabase.GetSTDatabase();
		sourceSTDatabase->SetInputBuffer(NULL);
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
