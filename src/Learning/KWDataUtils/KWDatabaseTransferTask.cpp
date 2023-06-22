// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDatabaseTransferTask.h"

KWDatabaseTransferTask::KWDatabaseTransferTask()
{
	lWrittenObjects = 0;

	// Declaration des variables partagees
	DeclareSharedParameter(&shared_targetDatabase);

	// Nom des fichiers par table renvoyes par l'esclave
	DeclareTaskOutput(&output_svOutputChunkFileNames);

	// Resultats envoyes par l'esclave dans le cas general
	DeclareTaskOutput(&output_lWrittenObjects);

	// Resultats envoyes par l'esclave dans le cas multi-tables
	DeclareTaskOutput(&output_lvMappingWrittenRecords);
}

KWDatabaseTransferTask::~KWDatabaseTransferTask()
{
	assert(oaTableChunkFileNames.GetSize() == 0);
}

boolean KWDatabaseTransferTask::Transfer(const KWDatabase* sourceDatabase, const KWDatabase* targetDatabase,
					 longint& lWrittenObjectNumber)
{
	boolean bOk = true;
	KWMTDatabaseTextFile refMTDatabaseTextFile;
	KWSTDatabaseTextFile refSTDatabaseTextFile;
	ALString sTmp;

	require(sourceDatabase != NULL);
	require(sourceDatabase->Check());
	require(targetDatabase != NULL);
	require(targetDatabase->CheckPartially(true));
	require(sourceDatabase->GetTechnologyName() == targetDatabase->GetTechnologyName());
	require(sourceDatabase->GetTechnologyName() == refMTDatabaseTextFile.GetTechnologyName() or
		sourceDatabase->GetTechnologyName() == refSTDatabaseTextFile.GetTechnologyName());

	// Initialisation de la base en sortie
	shared_targetDatabase.GetPLDatabase()->InitializeFrom(targetDatabase);

	// Lancement de la tache
	// On utilise un DatabaseIndexer local a la tache, car il n'y pas ici d'indexation de base a partager
	lWrittenObjectNumber = 0;
	bOk = RunDatabaseTask(sourceDatabase);
	if (bOk)
		lWrittenObjectNumber = lWrittenObjects;
	return bOk;
}

void KWDatabaseTransferTask::DisplaySpecificTaskMessage()
{
	shared_sourceDatabase.GetPLDatabase()->DisplayAllTableMessages("Input database", "Read records", lReadRecords,
								       -1, &lvMappingReadRecords);
	shared_targetDatabase.GetPLDatabase()->DisplayAllTableMessages("Output database", "Written records",
								       lWrittenObjects, -1, &lvMappingWrittenRecords);
}

void KWDatabaseTransferTask::CleanAllDataTableIndexation()
{
	// Appel de la methode ancetre
	KWDatabaseTask::CleanAllDataTableIndexation();

	// Nettoyage des informations d'ouverture des bases
	shared_targetDatabase.GetPLDatabase()->CleanOpenInformation();
}

boolean KWDatabaseTransferTask::ComputeDatabaseOpenInformation()
{
	boolean bOk = true;
	const boolean bIncludingClassMemory = true;

	// Calcul de la memoire necessaire et collecte des informations permettant d'ouvrir les bases dans les esclaves
	// Attention: on ne prend en compte qu'une seule fois le dictionaire de la base, qui est partage entre
	// les bases en lecture et en ecriture
	bOk = bOk and shared_sourceDatabase.GetPLDatabase()->ComputeOpenInformation(
			  true, bIncludingClassMemory, shared_targetDatabase.GetPLDatabase());
	bOk = bOk and
	      shared_targetDatabase.GetPLDatabase()->ComputeOpenInformation(false, not bIncludingClassMemory, NULL);
	return bOk;
}

const ALString KWDatabaseTransferTask::GetTaskName() const
{
	return "Model deployment";
}

PLParallelTask* KWDatabaseTransferTask::Create() const
{
	return new KWDatabaseTransferTask;
}

boolean KWDatabaseTransferTask::ComputeResourceRequirements()
{
	boolean bOk = true;
	boolean bDisplayRequirements = false;
	PLDatabaseTextFile* sourceDatabase;
	PLDatabaseTextFile* targetDatabase;
	longint lOutputNecessaryDiskSpace;
	ALString sOutputPath;
	longint lOutputAvailableDiskSpace;
	longint lInputPreferredSize;
	longint lOutputPreferredSize;
	ALString sTmp;

	// Acces aux bases
	sourceDatabase = shared_sourceDatabase.GetPLDatabase();
	targetDatabase = shared_targetDatabase.GetPLDatabase();

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::ComputeResourceRequirements();
	assert(targetDatabase->IsOpenInformationComputed());

	// Specialisation de la methode
	lOutputNecessaryDiskSpace = 0;
	lOutputAvailableDiskSpace = 0;
	if (bOk)
	{
		// Calcul des ressources pour les esclave: prise en compte additionnelle de la base en ecriture
		GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMin(
		    targetDatabase->GetMinOpenNecessaryMemory());
		GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMax(
		    targetDatabase->GetMaxOpenNecessaryMemory());

		// Calcul des resources pour le maitre: concatenation des fichiers resultats
		lInputPreferredSize = PLRemoteFileService::GetPreferredBufferSize(FileService::GetTmpDir());
		lOutputPreferredSize =
		    PLRemoteFileService::GetPreferredBufferSize(targetDatabase->GetDatabase()->GetDatabaseName());

		// On prendre le max de l'existant et de ce qu'il faut pour la concatenation, car la concatenation se
		// fait en toute fin, quand on a plus besoin des ressources de depart
		GetResourceRequirements()->GetMasterRequirement()->GetMemory()->SetMin(
		    max(GetResourceRequirements()->GetMasterRequirement()->GetMemory()->GetMin(),
			lInputPreferredSize + lOutputPreferredSize));
		GetResourceRequirements()->GetMasterRequirement()->GetMemory()->SetMax(
		    max(GetResourceRequirements()->GetMasterRequirement()->GetMemory()->GetMax(),
			8 * lInputPreferredSize + lOutputPreferredSize));

		// Les besoins en disque pour les esclaves permettent de stocker les fichiers temporaires de transfer
		// sur l'ensemble de tous les esclaves (et non par esclave)
		lOutputNecessaryDiskSpace = sourceDatabase->GetOutputNecessaryDiskSpace();
		GetResourceRequirements()->GetGlobalSlaveRequirement()->GetDisk()->Set(lOutputNecessaryDiskSpace);

		// On regarde si on a assez de place pour stocker les fichiers en sortie par le maitre
		sOutputPath = FileService::GetPathName(targetDatabase->GetDatabase()->GetDatabaseName());
		lOutputAvailableDiskSpace = PLRemoteFileService::GetDiskFreeSpace(sOutputPath);
		if (lOutputAvailableDiskSpace < lOutputNecessaryDiskSpace)
		{
			bOk = false;
			AddError(sTmp + "There is not enough space available on the disk to write output database (" +
				 targetDatabase->GetDatabase()->GetDatabaseName() + "): needs at least " +
				 LongintToHumanReadableString(lOutputNecessaryDiskSpace - lOutputAvailableDiskSpace));
		}
	}

	// Affichage detaille des demande de ressource
	if (bDisplayRequirements)
	{
		cout << "KWDatabaseTransferTask::ComputeResourceRequirements, source memory\t"
		     << LongintToHumanReadableString(sourceDatabase->GetMinOpenNecessaryMemory()) << "\t"
		     << LongintToHumanReadableString(sourceDatabase->GetMaxOpenNecessaryMemory()) << endl;
		cout << "KWDatabaseTransferTask::ComputeResourceRequirements, target memory\t"
		     << LongintToHumanReadableString(targetDatabase->GetMinOpenNecessaryMemory()) << "\t"
		     << LongintToHumanReadableString(targetDatabase->GetMaxOpenNecessaryMemory()) << endl;
		cout << "KWDatabaseTransferTask::ComputeResourceRequirements, target disk\t"
		     << LongintToHumanReadableString(lOutputNecessaryDiskSpace) << endl;
		cout << "KWDatabaseTransferTask::ComputeResourceRequirements, available disk\t"
		     << LongintToHumanReadableString(lOutputAvailableDiskSpace) << endl;
	}
	return bOk;
}

boolean KWDatabaseTransferTask::MasterInitialize()
{
	boolean bOk = true;
	PLMTDatabaseTextFile* targetDatabase;
	KWMTDatabaseMapping* mapping;
	int i;

	require(oaTableChunkFileNames.GetSize() == 0);

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterInitialize();

	// Initialisation des vecteur de fichier de chunk par table
	// On ne cree un vecteur de fichiers que si necessaire
	oaTableChunkFileNames.SetSize(shared_targetDatabase.GetDatabase()->GetTableNumber());
	if (shared_targetDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		targetDatabase = shared_targetDatabase.GetMTDatabase();
		for (i = 0; i < targetDatabase->GetTableNumber(); i++)
		{
			mapping = cast(KWMTDatabaseMapping*, targetDatabase->GetMultiTableMappings()->GetAt(i));

			// Test si mapping correspond a une table cible a ecrire
			if (not targetDatabase->IsReferencedClassMapping(mapping) and
			    mapping->GetDataTableName() != "" and mapping->IsTerminalAttributeUsed())
				oaTableChunkFileNames.SetAt(i, new StringVector);
		}
	}
	else
		oaTableChunkFileNames.SetAt(0, new StringVector);
	assert(oaTableChunkFileNames.GetAt(0) != NULL);

	// Initialisations des nombres de records ecrits
	lWrittenObjects = 0;
	lvMappingWrittenRecords.SetSize(shared_sourceDatabase.GetDatabase()->GetTableNumber());

	// Par defaut, on stocke -1 pour indique que les tables ne sont pas traitees
	for (i = 0; i < lvMappingWrittenRecords.GetSize(); i++)
		lvMappingWrittenRecords.SetAt(i, -1);
	return bOk;
}

boolean KWDatabaseTransferTask::MasterInitializeDatabase()
{
	boolean bOk = true;
	ALString sClassName;
	PLDatabaseTextFile* targetDatabase;
	longint lTargetDatabaseGrantedMemory;
	int nTargetBufferSize;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterInitializeDatabase();

	///////////////////////////////////////////////////////////////////////////////////////
	// Gestion des ressources pour determiner la taille des buffers

	// Acces a la base cible
	targetDatabase = shared_targetDatabase.GetPLDatabase();

	// Parametrage de l'ouverture a la demande dans le cas multi-tables
	if (bOk and shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		assert(shared_sourceDatabase.GetMTDatabase()->GetMainLocalTableNumber() + 10 <=
			   GetMaxOpenedFileNumber() or
		       shared_sourceDatabase.GetMTDatabase()->GetOpenOnDemandMode());

		// On passe en mode ouverture a la demande s'il y a trop de tables locales (avec un petite marge)
		if (shared_sourceDatabase.GetMTDatabase()->GetMainLocalTableNumber() +
			shared_targetDatabase.GetMTDatabase()->GetMainLocalTableNumber() + 10 >
		    GetMaxOpenedFileNumber())
			shared_targetDatabase.GetMTDatabase()->SetOpenOnDemandMode(true);
	}

	// Calcul des tailles de buffer
	nTargetBufferSize = 0;
	if (bOk)
	{
		// Calcul de la memoire allouee pour la base cible
		lTargetDatabaseGrantedMemory = ComputeSlaveGrantedMemory(GetSlaveResourceRequirement(),
									 GetSlaveResourceGrant()->GetMemory(), false);

		// Calcul des tailles de buffer permises par ces memoire allouees
		nTargetBufferSize =
		    targetDatabase->ComputeOpenBufferSize(false, lTargetDatabaseGrantedMemory, GetProcessNumber());

		// On peut imposer la taille du buffer pour raison de tests
		if (nForcedBufferSize > 0)
			nTargetBufferSize = nForcedBufferSize;
	}

	// Parametrage des tailles de buffer
	if (bOk)
		targetDatabase->SetBufferSize(nTargetBufferSize);
	return bOk;
}

boolean KWDatabaseTransferTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	boolean bOk;
	int i;
	StringVector* svChunkFileNames;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterPrepareTaskInput(dTaskPercent, bIsTaskFinished);

	// Creation si necessaire d'un nouveau poste dans les vecteurs de nom de fichiers resultats
	if (bOk and not bIsTaskFinished)
	{
		for (i = 0; i < oaTableChunkFileNames.GetSize(); i++)
		{
			svChunkFileNames = cast(StringVector*, oaTableChunkFileNames.GetAt(i));

			// Seul les vecteurs non NULL correspondent a des tables a ecrire en sortie (cf.
			// MasterInitialize)
			if (svChunkFileNames != NULL)
			{
				assert(svChunkFileNames->GetSize() == GetTaskIndex());
				svChunkFileNames->Add("");
			}
		}
	}

	return bOk;
}

boolean KWDatabaseTransferTask::MasterAggregateResults()
{
	boolean bOk = true;
	int i;
	PLMTDatabaseTextFile* targetDatabase;
	StringVector* svChunkFileNames; // URI

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterAggregateResults();

	// Collecte du nombre d'enregistrement transferes
	lWrittenObjects += output_lWrittenObjects;

	// Cas specifique au multi-tables
	if (shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		// Parcours des tables cibles pour recolter le nom de fichier de chunk et
		// le nombre d'enregistrements ecrits par mapping
		targetDatabase = shared_targetDatabase.GetMTDatabase();
		for (i = 0; i < targetDatabase->GetTableNumber(); i++)
		{
			// Prise en compte du nom de fichier ecrit pour le chunk courant
			svChunkFileNames = cast(StringVector*, oaTableChunkFileNames.GetAt(i));

			// Seul les vecteurs non NULL correspondent a des tables a ecrire en sortie (cf.
			// MasterInitialize)
			assert(svChunkFileNames != NULL or output_svOutputChunkFileNames.GetAt(i) == "" or not bOk);
			if (svChunkFileNames != NULL)
			{
				svChunkFileNames->SetAt(GetTaskIndex(), output_svOutputChunkFileNames.GetAt(i));

				// La premiere fois, on passe a 0 pour indique que la base est ouverte
				if (lvMappingWrittenRecords.GetAt(i) == -1)
					lvMappingWrittenRecords.SetAt(i, 0);

				// Mise a jour du nombre d'enregistrements ecrits
				lvMappingWrittenRecords.UpgradeAt(i, output_lvMappingWrittenRecords.GetAt(i));
			}
		}
	}
	// Cas specifique au mono-table
	else
	{
		// Prise en compte du nom de fichier ecrit pour le chunk courant
		assert(output_svOutputChunkFileNames.GetAt(0) != "" or not bOk);
		svChunkFileNames = cast(StringVector*, oaTableChunkFileNames.GetAt(0));
		svChunkFileNames->SetAt(GetTaskIndex(), output_svOutputChunkFileNames.GetAt(0));
	}
	return bOk;
}

boolean KWDatabaseTransferTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;
	int i;
	PLMTDatabaseTextFile* targetDatabase;
	KWMTDatabaseMapping* mapping;
	KWClass* dictionary;
	ALString sKeyName;
	ALString sOutputFileName;
	StringVector* svChunkFileNames; // URI
	PLFileConcatenater concatenater;

	// Initialisation du code retour
	bOk = bProcessEndedCorrectly;

	// Debut de la progression
	TaskProgression::DisplayLabel("Tables concatenation");
	concatenater.SetDisplayProgression(true);

	// Cas specifique au multi-tables
	// Meme en cas d'erreur, on passe dans cette methode, qui procedera si necessaire au nettoyage des fichiers
	// intermediaires
	if (shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		// Acces aux bases
		targetDatabase = shared_targetDatabase.GetMTDatabase();

		// Concatenation des chunks pour chaque table transferee, et netoyage si necessaire
		for (i = 0; i < targetDatabase->GetTableNumber(); i++)
		{
			mapping = cast(KWMTDatabaseMapping*, targetDatabase->GetMultiTableMappings()->GetAt(i));

			// Seul les vecteurs non NULL correspondent a des tables a ecrire en sortie (cf.
			// MasterInitialize)
			svChunkFileNames = cast(StringVector*, oaTableChunkFileNames.GetAt(i));
			if (svChunkFileNames != NULL)
			{
				// Concatenation si ok
				if (bOk)
				{
					// Nom du fichier en sortie
					sOutputFileName = mapping->GetDataTableName();

					// Calcul des bornes pour la progression
					concatenater.SetProgressionBegin(i * 1.0 / targetDatabase->GetTableNumber());
					concatenater.SetProgressionEnd((i + 1) * 1.0 /
								       targetDatabase->GetTableNumber());

					// Initialisation de la concatenation
					dictionary =
					    KWClassDomain::GetCurrentDomain()->LookupClass(mapping->GetClassName());
					if (targetDatabase->GetHeaderLineUsed())
						dictionary->ExportStoredFieldNames(concatenater.GetHeaderLine());
					concatenater.SetFieldSeparator(targetDatabase->GetFieldSeparator());

					// Concatenation des chunks
					concatenater.SetFileName(sOutputFileName);
					bOk = bOk and not TaskProgression::IsInterruptionRequested();
					if (bOk)
					{
						TaskProgression::DisplayLabel("concatenation");
						bOk = concatenater.Concatenate(svChunkFileNames, this, true) and bOk;
					}
				}

				// Suppression des fichiers intermediaires
				if (not bOk)
				{
					TaskProgression::DisplayLabel("cleaning");
					concatenater.RemoveChunks(svChunkFileNames);
				}
				svChunkFileNames->SetSize(0);
			}
			TaskProgression::DisplayProgression(i * 100 / targetDatabase->GetTableNumber());
		}
		TaskProgression::DisplayProgression(100);
	}
	// Cas mono-tables
	else
	{
		// Construction de la liste des fichiers a partir des chunks
		sOutputFileName = shared_targetDatabase.GetDatabase()->GetDatabaseName();
		svChunkFileNames = cast(StringVector*, oaTableChunkFileNames.GetAt(0));

		// Concatenation des chunks
		bOk = bOk and not TaskProgression::IsInterruptionRequested();
		if (bOk)
		{
			// Parametrage de la concatenation
			dictionary = KWClassDomain::GetCurrentDomain()->LookupClass(
			    shared_targetDatabase.GetDatabase()->GetClassName());
			if (shared_targetDatabase.GetSTDatabase()->GetHeaderLineUsed())
				dictionary->ExportStoredFieldNames(concatenater.GetHeaderLine());
			concatenater.SetFileName(sOutputFileName);
			concatenater.SetFieldSeparator(shared_targetDatabase.GetSTDatabase()->GetFieldSeparator());

			// Concatenation des chunks
			TaskProgression::DisplayLabel("concatenation");
			bOk = concatenater.Concatenate(svChunkFileNames, this, true);
		}

		// Suppression des fichiers intermediaires
		if (not bOk)
		{
			TaskProgression::DisplayLabel("cleaning");
			concatenater.RemoveChunks(svChunkFileNames);
		}
	}

	// On met a 0 le nombre de records ecrits si necessaire
	if (not bOk)
	{
		lWrittenObjects = 0;
		lvMappingWrittenRecords.Initialize();
	}

	// Nettoyage
	oaTableChunkFileNames.DeleteAll();

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterFinalize(bOk) and bOk;
	ensure(oaTableChunkFileNames.GetSize() == 0);
	return bOk;
}

boolean KWDatabaseTransferTask::SlaveInitializePrepareDatabase()
{
	boolean bOk = true;
	ALString sClassName;
	PLDatabaseTextFile* targetDatabase;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveInitializePrepareDatabase();

	// Acces aux bases
	targetDatabase = shared_targetDatabase.GetPLDatabase();

	// Cas specifique au multi-tables
	if (bOk and shared_targetDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		// Les dictionnaire etant disponibles, on peut remettre a jour les mappings
		if (bOk)
		{
			shared_targetDatabase.GetMTDatabase()->UpdateMultiTableMappings();
			assert(shared_targetDatabase.GetDatabase()->CheckPartially(true));
		}
	}
	return bOk;
}

boolean KWDatabaseTransferTask::SlaveInitializeOpenDatabase()
{
	boolean bOk = true;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveInitializeOpenDatabase();

	// Ouverture de la base destination
	bOk = bOk and shared_targetDatabase.GetDatabase()->OpenForWrite();

	// Creation des buffers dans le cas multi-tables
	if (bOk and shared_sourceDatabase.GetDatabase()->IsMultiTableTechnology())
		bOk = shared_targetDatabase.GetMTDatabase()->CreateOutputBuffers();
	return bOk;
}

boolean KWDatabaseTransferTask::SlaveProcessStartDatabase()
{
	boolean bOk = true;
	KWDatabase* targetDatabase;
	PLSTDatabaseTextFile* targetSTDatabase;
	PLMTDatabaseTextFile* targetMTDatabase;
	OutputBufferedFile* outputBuffer;
	int i;
	KWMTDatabaseMapping* mapping;
	ALString sChunkBaseName;
	ALString sChunkFileName;
	ALString sTmp;

	// Initialisation des variables de travail
	targetDatabase = shared_targetDatabase.GetDatabase();
	targetSTDatabase = NULL;
	targetMTDatabase = NULL;
	outputBuffer = NULL;

	// Initialisation des variables en sortie
	output_lWrittenObjects = 0;
	output_lvMappingWrittenRecords.GetLongintVector()->SetSize(targetDatabase->GetTableNumber());
	output_lvMappingWrittenRecords.GetLongintVector()->Initialize();

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveProcessStartDatabase();

	// Cas specifique au multi-tables
	if (targetDatabase->IsMultiTableTechnology())
	{
		targetMTDatabase = shared_targetDatabase.GetMTDatabase();

		// Ouverture des buffers en ecriture (ceux-ci ont un nom unique par SlaveProcess), et memorisation dans
		// la variable en sortie de l'esclave
		bOk = bOk and targetMTDatabase->OpenOutputBuffers(this, GetTaskIndex(),
								  output_svOutputChunkFileNames.GetStringVector());

		// Parcours des mappings pour reinitialiser le nombre de records traite par chaque mapping
		if (bOk)
		{
			for (i = 0; i < targetMTDatabase->GetTableNumber(); i++)
			{
				mapping =
				    cast(KWMTDatabaseMapping*, targetMTDatabase->GetMultiTableMappings()->GetAt(i));

				// Parametrage des mapping initialises uniquement
				if (targetMTDatabase->IsMappingInitialized(mapping))
				{
					// Parametrage de l'index de depart (pour une gestion correcte des messages
					// d'erreur)
					targetMTDatabase->GetDriverAt(mapping)->SetUsedRecordNumber(0);
				}
			}
		}
	}
	// Cas mono-table
	else
	{
		targetSTDatabase = shared_targetDatabase.GetSTDatabase();

		// Creation et initialisation du buffer de sortie a partir de la base de sortie
		outputBuffer = new OutputBufferedFile;
		outputBuffer->SetHeaderLineUsed(targetSTDatabase->GetHeaderLineUsed());
		outputBuffer->SetFieldSeparator(targetSTDatabase->GetFieldSeparator());

		// Construction d'un nom de chunk
		// Creation d'un nom de chunk propre a l'esclave
		sChunkBaseName = sTmp + IntToString(GetTaskIndex()) + "_" +
				 FileService::GetFileName(targetSTDatabase->GetDatabaseName());
		sChunkFileName = FileService::CreateUniqueTmpFile(sChunkBaseName, this);
		bOk = sChunkFileName != "";

		// Ouverture du buffer en sortie
		if (bOk)
		{
			output_svOutputChunkFileNames.GetStringVector()->SetSize(1);
			output_svOutputChunkFileNames.GetStringVector()->SetAt(
			    0, FileService::BuildLocalURI(sChunkFileName));

			outputBuffer->SetFileName(sChunkFileName);
			bOk = outputBuffer->Open();
			if (not bOk)
			{
				AddError("Unable to open target database " + targetSTDatabase->GetDatabaseName());

				// Destruction du fichier
				FileService::RemoveFile(sChunkFileName);
			}
		}

		// Parametrage du bufffer de la base en sortie
		if (bOk)
			targetSTDatabase->SetOutputBuffer(outputBuffer);
	}

	// Memorisation des fichiers temporaires cree par l'esclave
	if (bOk)
		SlaveRegisterUniqueTmpFiles(output_svOutputChunkFileNames.GetConstStringVector());
	return bOk;
}

boolean KWDatabaseTransferTask::SlaveProcessExploitDatabaseObject(const KWObject* kwoObject)
{
	boolean bOk = true;
	KWDatabase* targetDatabase;

	require(kwoObject != NULL);

	// Acces a la base
	targetDatabase = shared_targetDatabase.GetDatabase();

	// Ecriture de l'objet
	targetDatabase->Write(kwoObject);

	// Arret si erreur d'ecriture
	if (targetDatabase->IsError())
	{
		targetDatabase->AddError(GetTaskName() + " interrupted because of write errors");
		bOk = false;
	}
	return bOk;
}

boolean KWDatabaseTransferTask::SlaveProcessStopDatabase(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;
	boolean bTrace = false;
	PLSTDatabaseTextFile* targetSTDatabase;
	PLMTDatabaseTextFile* targetMTDatabase;
	OutputBufferedFile* outputBuffer;
	KWMTDatabaseMapping* mapping;
	int i;
	ALString sChunkFileName;
	boolean bCloseOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveProcessStopDatabase(bProcessEndedCorrectly);

	// On renvoi le nombre d'object ecrit
	if (bOk)
		output_lWrittenObjects = output_lReadObjects;

	// On renvoie le nombre de records traites par mapping en sortie
	if (bOk and shared_targetDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		targetMTDatabase = shared_targetDatabase.GetMTDatabase();
		for (i = 0; i < targetMTDatabase->GetTableNumber(); i++)
		{
			mapping = cast(KWMTDatabaseMapping*, targetMTDatabase->GetMultiTableMappings()->GetAt(i));
			if (targetMTDatabase->IsMappingInitialized(mapping))
			{
				// Trace
				if (bTrace)
				{
					cout << "Slave\t" << GetTaskIndex() << "\toutput written records"
					     << "\t" << i << "\t"
					     << LongintToReadableString(
						    targetMTDatabase->GetDriverAt(mapping)->GetUsedRecordNumber())
					     << endl;
				}

				output_lvMappingWrittenRecords.SetAt(
				    i, targetMTDatabase->GetDriverAt(mapping)->GetUsedRecordNumber());
			}
		}
	}

	// Fermeture des buffers de sortie
	if (shared_targetDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		targetMTDatabase = shared_targetDatabase.GetMTDatabase();
		bCloseOk = targetMTDatabase->CloseOutputBuffers();
		bOk = bOk and bCloseOk;
	}
	else
	{
		targetSTDatabase = shared_targetDatabase.GetSTDatabase();

		// Acces au bufffer de la base en sortie
		outputBuffer = targetSTDatabase->GetOutputBuffer();

		// Fermeture et destruction du buffer de sortie
		assert(outputBuffer != NULL);
		bCloseOk = outputBuffer->Close();
		if (not bCloseOk)
			AddError("Cannot close file " + outputBuffer->GetFileName());
		bOk = bOk and bCloseOk;
		delete outputBuffer;
	}

	// Si erreur, supression des fichiers en sortie
	if (not bOk)
	{
		for (i = 0; i < output_svOutputChunkFileNames.GetStringVector()->GetSize(); i++)
		{
			sChunkFileName = output_svOutputChunkFileNames.GetStringVector()->GetAt(i);
			if (sChunkFileName != "")
				FileService::RemoveFile(FileService::GetURIFilePathName(sChunkFileName));
		}
		output_svOutputChunkFileNames.GetStringVector()->SetSize(0);
	}
	return bOk;
}

boolean KWDatabaseTransferTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;
	PLMTDatabaseTextFile* targetDatabase;
	boolean bCloseOk;

	// Initialisation du code retour
	bOk = bProcessEndedCorrectly;

	// Cas specifique au multi-tables
	if (shared_targetDatabase.GetDatabase()->IsMultiTableTechnology())
	{
		// Acces a la base cible
		targetDatabase = shared_targetDatabase.GetMTDatabase();

		// Destruction des buffers de sortie
		bOk = targetDatabase->DeleteOutputBuffers() and bOk;
	}

	// Fermeture des bases
	if (shared_targetDatabase.GetDatabase()->IsOpenedForWrite())
	{
		bCloseOk = shared_targetDatabase.GetDatabase()->Close();
		bOk = bOk and bCloseOk;
		if (not bCloseOk)
			AddError("Error while closing output database " +
				 shared_targetDatabase.GetDatabase()->GetDatabaseName());
	}

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveFinalize(bOk);
	return bOk;
}