// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "KWDatabaseSlicerTask.h"

/////////////////////////////////////////////////////////////////////////////////
// Classe KWDatabaseSlicerTask

KWDatabaseSlicerTask::KWDatabaseSlicerTask()
{
	allSliceOutputBuffer = NULL;

	// Declaration des variables partagees
	DeclareSharedParameter(&shared_DataTableSliceSet);
	DeclareSharedParameter(&shared_nTotalBlockNumber);
	DeclareSharedParameter(&shared_nTotalDenseSymbolAttributeNumber);
	DeclareSharedParameter(&shared_nAllSliceOutputBufferSize);

	// Variable en entree et sortie des esclaves
	DeclareTaskOutput(&output_lWrittenObjects);
	DeclareTaskOutput(&output_svSliceFileNames);
	DeclareTaskOutput(&output_lvSliceFileSizes);
	DeclareTaskOutput(&output_lvAllAttributeBlockValueNumbers);
	DeclareTaskOutput(&output_lvAllDenseSymbolAttributeDiskSizes);
}

KWDatabaseSlicerTask::~KWDatabaseSlicerTask()
{
	assert(allSliceOutputBuffer == NULL);
}

boolean KWDatabaseSlicerTask::SliceDatabase(const KWDatabase* sourceDatabase, const ALString& sTargetAttributName,
					    int nInstanceNumber, int nMaxAttributeNumberPerSlice,
					    KWDataTableSliceSet* outputDataTableSliceSet)
{
	boolean bOk = true;
	boolean bDisplay = GetPreparationTraceMode();
	boolean bDisplaySliceDictionaries = false;
	KWClass* kwcClass;
	KWAttribute* attribute;
	ALString sTmp;

	require(sourceDatabase != NULL);
	require(sourceDatabase->Check());
	require(nInstanceNumber >= 0);
	require(nMaxAttributeNumberPerSlice >= 0);
	require(outputDataTableSliceSet != NULL);

	// Recherche de la classe associee a la base de donnees
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(sourceDatabase->GetClassName());
	check(kwcClass);

	// Parametrage de la classe pour la preparation, en ne chargeant que les attributs a preparer
	InitClassForPreparation(kwcClass, sTargetAttributName);

	// Specification des tranches de la base en partitionnant les attributs en troncons
	outputDataTableSliceSet->ComputeSpecification(kwcClass, sTargetAttributName, nInstanceNumber,
						      nMaxAttributeNumberPerSlice);

	// Calcul des informations de chargement des attributs de chaque slice pour la classe a partitionner du sliceset
	outputDataTableSliceSet->ComputeSlicesLoadIndexes(kwcClass);

	// Parametrage de cette specification, partagee avec les esclaves
	shared_DataTableSliceSet.SetDataTableSliceSet(outputDataTableSliceSet);

	// Calcul du nombre total de bloc du sliceset
	shared_nTotalBlockNumber = outputDataTableSliceSet->GetTotalAttributeBlockNumber();

	// Calcul des nombres d'attributs denses Symbol du sliceSet
	shared_nTotalDenseSymbolAttributeNumber = 0;
	attribute = kwcClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		// On compte les attribut denses utilises Symbol
		if (attribute->GetUsed() and not attribute->IsInBlock() and attribute->GetType() == KWType::Symbol and
		    attribute->GetName() != sTargetAttributName)
			shared_nTotalDenseSymbolAttributeNumber++;

		// Attribut suivant
		kwcClass->GetNextAttribute(attribute);
	}

	// Lancement de la tache
	bOk = RunDatabaseTask(sourceDatabase);

	// Nettoyage des informations de chargement des attributs de chaque slice pour la classe a partitionner du
	// sliceSet
	outputDataTableSliceSet->CleanSlicesLoadIndexes();

	// On dereference la specification de la variable partagee, pour ne pas qu'elle soit detruite
	shared_DataTableSliceSet.RemoveObject();

	// Reinitialisation de la classe, avec tous les attributs a charger
	ResetClass(kwcClass);

	// Affichage
	if (bDisplay)
	{
		cout << "KWDatabaseSlicerTask::SliceDatabase\t" << GetJobElapsedTime() << endl;
		outputDataTableSliceSet->Write(cout);
	}
	if (bDisplaySliceDictionaries)
		outputDataTableSliceSet->SliceDictionariesWrite(cout);

	// Information additionnel en mode verbeux des taches
	if (GetVerbose())
	{
		AddMessage(sTmp + "Max att per slice: " + IntToString(nMaxAttributeNumberPerSlice));
		AddMessage(sTmp + "Slices: " + IntToString(outputDataTableSliceSet->GetSliceNumber()));
		AddMessage(sTmp + "Chunks: " + IntToString(outputDataTableSliceSet->GetChunkNumber()));
	}
	return bOk;
}

void KWDatabaseSlicerTask::InitClassForPreparation(KWClass* kwcClass, const ALString& sTargetAttributName)
{
	KWAttribute* attribute;

	require(kwcClass != NULL);

	// Passage en Unload de tous les attributs
	kwcClass->SetAllAttributesLoaded(false);

	// Parcours des attributs pour mettre charger en memoire ceux a preparer
	attribute = kwcClass->GetHeadAttribute();
	while (attribute != NULL)
	{
		// Passage en Load des attributs utilises de type simple
		if (attribute->GetUsed() and KWType::IsSimple(attribute->GetType()))
			attribute->SetLoaded(true);
		kwcClass->GetNextAttribute(attribute);
	}

	// Passage en Unload de l'eventuel attribut cible
	if (sTargetAttributName != "")
	{
		attribute = kwcClass->LookupAttribute(sTargetAttributName);
		check(attribute);
		attribute->SetLoaded(true);
	}

	// Compilation de la classe
	kwcClass->GetDomain()->Compile();
}

void KWDatabaseSlicerTask::ResetClass(KWClass* kwcClass)
{
	require(kwcClass != NULL);

	// Passage en Load de tous les attributs
	kwcClass->SetAllAttributesLoaded(true);

	// Compilation de la classe
	kwcClass->GetDomain()->Compile();
}

void KWDatabaseSlicerTask::DisplaySpecificTaskMessage()
{
	shared_sourceDatabase.GetPLDatabase()->DisplayAllTableMessages("Database", "Read records", lReadRecords,
								       lReadObjects, &lvMappingReadRecords);
}

boolean KWDatabaseSlicerTask::ComputeDatabaseOpenInformation()
{
	boolean bOk;

	// Calcul de la memoire necessaire et collecte des informations permettant d'ouvir les bases dans les esclaves
	// Attention: on ne prend en compte qu'une seule fois le dictionaire de la base, qui est partage entre
	// les bases en lecture et en ecriture
	// On passe ici la base en entree comme parametre de base en sortie, pour evaluer la place disque necessaire
	bOk = shared_sourceDatabase.GetPLDatabase()->ComputeOpenInformation(true, true,
									    shared_sourceDatabase.GetPLDatabase());
	return bOk;
}

longint KWDatabaseSlicerTask::GetEmptyOutputNecessaryMemory()
{
	longint lNecessaryMemorySpace;

	// Au minimum, il faut stocker les specification et le buffer en sortie
	lNecessaryMemorySpace = shared_DataTableSliceSet.GetDataTableSliceSet()->GetUsedMemory();
	lNecessaryMemorySpace += KWDatabaseSlicerOutputBufferedFile::GetMinimumNecessaryMemory();
	return lNecessaryMemorySpace;
}

longint KWDatabaseSlicerTask::GetMinOutputNecessaryMemory()
{
	longint lNecessaryMemorySpace;
	lNecessaryMemorySpace = GetEmptyOutputNecessaryMemory();

	// On rajoute au minimum 64 KB par tranche plus deux tailles de buffer pour la tranche courante
	lNecessaryMemorySpace += 2 * BufferedFile::nDefaultBufferSize +
				 shared_DataTableSliceSet.GetDataTableSliceSet()->GetSliceNumber() * 64 * lKB;
	return lNecessaryMemorySpace;
}

longint KWDatabaseSlicerTask::GetMaxOutputNecessaryMemory()
{
	longint lNecessaryMemorySpace;
	lNecessaryMemorySpace = GetEmptyOutputNecessaryMemory();

	// On rajoute au maximum une taille de buffer standard par tranche plus deux tailles de buffer pour la tranche
	// courante
	lNecessaryMemorySpace +=
	    2 * BufferedFile::nDefaultBufferSize + shared_DataTableSliceSet.GetDataTableSliceSet()->GetSliceNumber() *
						       longint(BufferedFile::nDefaultBufferSize);
	return lNecessaryMemorySpace;
}

const ALString KWDatabaseSlicerTask::GetTaskName() const
{
	return "Database slicer";
}

PLParallelTask* KWDatabaseSlicerTask::Create() const
{
	return new KWDatabaseSlicerTask;
}

boolean KWDatabaseSlicerTask::ComputeResourceRequirements()
{
	boolean bOk = true;
	boolean bDisplayRequirements = false;
	longint lDataTableSliceSetUsedMemory;
	longint lOutputNecessaryDiskSpace;
	ALString sTmp;

	require(shared_DataTableSliceSet.GetDataTableSliceSet()->Check());
	require(shared_DataTableSliceSet.GetDataTableSliceSet()->GetClassName() ==
		shared_sourceDatabase.GetDatabase()->GetClassName());
	require(shared_DataTableSliceSet.GetDataTableSliceSet()->GetSliceNumber() == 0 or
		shared_DataTableSliceSet.GetDataTableSliceSet()->GetSliceAt(0)->GetDataFileNames()->GetSize() == 0);

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::ComputeResourceRequirements();

	// Specialisation de la methode
	lOutputNecessaryDiskSpace = 0;
	if (bOk)
	{
		// Calcul des ressources pour les esclave: prise en compte additionnelle de la base en ecriture
		GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMin(
		    GetMinOutputNecessaryMemory());
		GetResourceRequirements()->GetSlaveRequirement()->GetMemory()->UpgradeMax(
		    GetMaxOutputNecessaryMemory());

		// Prise en compte de la definition des tranches en memoire partagee
		lDataTableSliceSetUsedMemory = shared_DataTableSliceSet.GetDataTableSliceSet()->GetUsedMemory();
		GetResourceRequirements()->GetSharedRequirement()->GetMemory()->UpgradeMin(
		    lDataTableSliceSetUsedMemory);
		GetResourceRequirements()->GetSharedRequirement()->GetMemory()->UpgradeMax(
		    lDataTableSliceSetUsedMemory);

		// Calcul des resources pour le maitre: rien
		GetResourceRequirements()->GetMasterRequirement()->GetMemory()->Set(lMB);

		// Les besoins en disque pour les esclaves permettent de stocker les fichiers temporaires de transfer
		// sur l'ensemble de tous les esclaves (et non par esclave)
		lOutputNecessaryDiskSpace = shared_sourceDatabase.GetPLDatabase()->GetOutputNecessaryDiskSpace();
		GetResourceRequirements()->GetGlobalSlaveRequirement()->GetDisk()->Set(lOutputNecessaryDiskSpace);
	}

	// Affichage detaille des demandes de ressource
	if (bDisplayRequirements)
	{
		cout << "KWDatabaseSlicerTask::ComputeResourceRequirements, slice number\t"
		     << shared_DataTableSliceSet.GetDataTableSliceSet()->GetSliceNumber() << endl;
		cout << "KWDatabaseSlicerTask::ComputeResourceRequirements, data table splicer used memory\t"
		     << LongintToHumanReadableString(shared_DataTableSliceSet.GetDataTableSliceSet()->GetUsedMemory())
		     << endl;
		cout << "KWDatabaseSlicerTask::ComputeResourceRequirements, output min memory\t"
		     << LongintToHumanReadableString(GetMinOutputNecessaryMemory()) << endl;
		cout << "KWDatabaseSlicerTask::ComputeResourceRequirements, output max memory\t"
		     << LongintToHumanReadableString(GetMaxOutputNecessaryMemory()) << endl;
		cout << "KWDatabaseSlicerTask::ComputeResourceRequirements, output disk\t"
		     << LongintToHumanReadableString(lOutputNecessaryDiskSpace) << endl;
	}
	return bOk;
}

boolean KWDatabaseSlicerTask::MasterInitialize()
{
	boolean bOk = true;
	KWDataTableSliceSet* dataTableSliceSet;
	KWDataTableSlice* dataTableSlice;
	int nSlice;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterInitialize();

	// Initialisation des vecteurs de statistiques sur les valeurs
	if (bOk)
	{
		dataTableSliceSet = shared_DataTableSliceSet.GetDataTableSliceSet();
		for (nSlice = 0; nSlice < dataTableSliceSet->GetSliceNumber(); nSlice++)
		{
			dataTableSlice = dataTableSliceSet->GetSliceAt(nSlice);
			assert(dataTableSlice->GetClass()->GetUsedAttributeNumber() ==
			       dataTableSlice->GetClass()->GetLoadedAttributeNumber());

			// Initialisation des nombres de valeurs par bloc d'attributs
			assert(dataTableSlice->GetAttributeBlockValueNumbers()->GetSize() == 0);
			dataTableSlice->GetAttributeBlockValueNumbers()->SetSize(
			    dataTableSlice->GetClass()->GetLoadedAttributeBlockNumber());

			// Initialisation de tailles fichier par attrbut dense
			assert(dataTableSlice->GetDenseSymbolAttributeDiskSizes()->GetSize() == 0);
			dataTableSlice->GetDenseSymbolAttributeDiskSizes()->SetSize(
			    dataTableSlice->GetClass()->GetLoadedDenseAttributeNumber());
		}
	}
	return bOk;
}

boolean KWDatabaseSlicerTask::MasterInitializeDatabase()
{
	boolean bOk = true;
	ALString sClassName;
	longint lDataTableSplicerGrantedMemory;
	int nSliceNumber;
	longint lAllSliceOutputBufferSize;
	KWDataTableSliceSet* dataTableSliceSet;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterInitializeDatabase();

	///////////////////////////////////////////////////////////////////////////////////////
	// Gestion des ressources pour determiner la taille des buffers de sortie

	// Calcul des tailles de buffer
	shared_nAllSliceOutputBufferSize = 0;
	if (bOk)
	{
		// Calcul de la memoire allouee pour le slicer (hors base source)
		lDataTableSplicerGrantedMemory = ComputeSlaveGrantedMemory(GetSlaveResourceRequirement(),
									   GetSlaveResourceGrant()->GetMemory(), false);

		// Acces au caracteristiques des tranches
		dataTableSliceSet = shared_DataTableSliceSet.GetDataTableSliceSet();
		nSliceNumber = shared_DataTableSliceSet.GetDataTableSliceSet()->GetSliceNumber();

		// Calcul des tailles de buffer permises par ces memoire allouees
		if (nSliceNumber > 0)
		{
			lAllSliceOutputBufferSize = lDataTableSplicerGrantedMemory - GetEmptyOutputNecessaryMemory();
			assert(lAllSliceOutputBufferSize > 0);
			lAllSliceOutputBufferSize =
			    min(longint(dataTableSliceSet->GetTotalAttributeNumber() * sizeof(KWValue) *
					dataTableSliceSet->GetTotalInstanceNumber()),
				lAllSliceOutputBufferSize);
			lAllSliceOutputBufferSize = min(lGB, lAllSliceOutputBufferSize);
			shared_nAllSliceOutputBufferSize = (int)lAllSliceOutputBufferSize;
			shared_nAllSliceOutputBufferSize =
			    MemSegmentByteSize *
			    ((shared_nAllSliceOutputBufferSize + MemSegmentByteSize - 1) / MemSegmentByteSize);
			if (shared_nAllSliceOutputBufferSize < SystemFile::nMinPreferredBufferSize)
				shared_nAllSliceOutputBufferSize = SystemFile::nMinPreferredBufferSize;
		}
	}
	return bOk;
}

boolean KWDatabaseSlicerTask::MasterPrepareTaskInput(double& dTaskPercent, boolean& bIsTaskFinished)
{
	boolean bOk;
	KWDataTableSliceSet* dataTableSliceSet;
	KWDataTableSlice* dataTableSlice;
	int nSlice;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterPrepareTaskInput(dTaskPercent, bIsTaskFinished);

	// Preparation de la collecte des noms des fichiers des tranches dans leur ordre de creation
	if (bOk and not bIsTaskFinished)
	{
		dataTableSliceSet = shared_DataTableSliceSet.GetDataTableSliceSet();

		// On agrandi la taille du vecteur de nombre d'instances par chunk
		dataTableSliceSet->GetChunkInstanceNumbers()->Add(0);

		// Il faut agrandir la taille des vecteurs de noms et taille de fichier par tranche
		for (nSlice = 0; nSlice < dataTableSliceSet->GetSliceNumber(); nSlice++)
		{
			dataTableSlice = dataTableSliceSet->GetSliceAt(nSlice);
			dataTableSlice->GetDataFileNames()->Add("");
			dataTableSlice->GetDataFileSizes()->Add(0);
		}
	}

	return bOk;
}

boolean KWDatabaseSlicerTask::MasterAggregateResults()
{
	boolean bOk = true;
	KWDataTableSliceSet* dataTableSliceSet;
	KWDataTableSlice* dataTableSlice;
	int nSlice;
	int nSliceBlockIndex;
	int nGlobalBlockIndex;
	int nDenseAttributeIndex;
	int nGlobalDenseSymbolAttributeIndex;
	KWAttribute* attribute;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterAggregateResults();

	// Specialisation de la methode
	if (bOk)
	{
		dataTableSliceSet = shared_DataTableSliceSet.GetDataTableSliceSet();

		// Collecte du nombre d'enregistrement transferes
		assert((longint)output_lWrittenObjects <= INT_MAX);
		dataTableSliceSet->GetChunkInstanceNumbers()->SetAt(GetTaskIndex(), (int)output_lWrittenObjects);

		// Memorisation des noms et tailles de fichier, selon le rang de l'esclave
		for (nSlice = 0; nSlice < dataTableSliceSet->GetSliceNumber(); nSlice++)
		{
			dataTableSlice = dataTableSliceSet->GetSliceAt(nSlice);
			dataTableSlice->GetDataFileNames()->SetAt(GetTaskIndex(),
								  output_svSliceFileNames.GetAt(nSlice));
			dataTableSlice->GetDataFileSizes()->SetAt(GetTaskIndex(),
								  output_lvSliceFileSizes.GetAt(nSlice));
		}

		// Memorisation des nombres totaux de valeurs par tranche
		assert(output_lvAllAttributeBlockValueNumbers.GetSize() == shared_nTotalBlockNumber);
		nGlobalBlockIndex = 0;
		for (nSlice = 0; nSlice < dataTableSliceSet->GetSliceNumber(); nSlice++)
		{
			dataTableSlice = dataTableSliceSet->GetSliceAt(nSlice);

			// Parcours des blocs de chaque tranche
			for (nSliceBlockIndex = 0;
			     nSliceBlockIndex < dataTableSlice->GetClass()->GetLoadedAttributeBlockNumber();
			     nSliceBlockIndex++)
			{
				// Ajout du nombre de valeurs collecte pour le bloc courant
				dataTableSlice->GetAttributeBlockValueNumbers()->UpgradeAt(
				    nSliceBlockIndex, output_lvAllAttributeBlockValueNumbers.GetAt(nGlobalBlockIndex));
				nGlobalBlockIndex++;
			}
		}
		assert(nGlobalBlockIndex == shared_nTotalBlockNumber);

		// Memorisation des tailles occupees dans les fichiers par les attributs denses Symbol
		assert(output_lvAllDenseSymbolAttributeDiskSizes.GetSize() == shared_nTotalDenseSymbolAttributeNumber);
		nGlobalDenseSymbolAttributeIndex = 0;
		for (nSlice = 0; nSlice < dataTableSliceSet->GetSliceNumber(); nSlice++)
		{
			dataTableSlice = dataTableSliceSet->GetSliceAt(nSlice);

			// Parcours des attributs denses de chaque tranche
			for (nDenseAttributeIndex = 0;
			     nDenseAttributeIndex < dataTableSlice->GetClass()->GetLoadedDenseAttributeNumber();
			     nDenseAttributeIndex++)
			{
				attribute = dataTableSlice->GetClass()->GetLoadedDenseAttributeAt(nDenseAttributeIndex);

				// Ajout du nombre de la taille collectees collecte dans le cas des attributs denses
				// sparses
				assert(attribute->GetType() == KWType::Symbol or
				       dataTableSlice->GetDenseSymbolAttributeDiskSizes()->GetAt(
					   nDenseAttributeIndex) == 0);
				if (attribute->GetType() == KWType::Symbol)
				{
					dataTableSlice->GetDenseSymbolAttributeDiskSizes()->UpgradeAt(
					    nDenseAttributeIndex, output_lvAllDenseSymbolAttributeDiskSizes.GetAt(
								      nGlobalDenseSymbolAttributeIndex));
					nGlobalDenseSymbolAttributeIndex++;
				}
			}

			// Verification de l'intergrite des tranches lors de cette derniere passe sur les tranches
			assert(dataTableSlice->Check());
		}
		assert(nGlobalDenseSymbolAttributeIndex == shared_nTotalDenseSymbolAttributeNumber);
	}
	return bOk;
}

boolean KWDatabaseSlicerTask::MasterFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;
	KWDataTableSliceSet* dataTableSliceSet;
	KWDataTableSlice* dataTableSlice;
	int nSlice;
	ALString sSliceBaseName;
	ALString sSliceFileName;
	ALString sTmp;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::MasterFinalize(bProcessEndedCorrectly);

	// Acces au slice set
	dataTableSliceSet = shared_DataTableSliceSet.GetDataTableSliceSet();

	// Verification du nombre total d'instances traitees
	if (bOk and dataTableSliceSet->GetTotalInstanceNumber() != lReadObjects)
	{
		AddError(sTmp +
			 LongintToString(shared_DataTableSliceSet.GetDataTableSliceSet()->GetTotalInstanceNumber()) +
			 " recoded instances instead of " + LongintToString(lReadObjects) + " expected");
		bOk = false;
	}

	// Gestion de l'effet de bord d'une base vide, avec aucun fichier cree par tranche
	// On cree ici des fichiers vides pour eviter d'avoir a gerer cet effet de bord partout ailleurs
	if (bOk and dataTableSliceSet->GetSliceNumber() > 0 and dataTableSliceSet->GetChunkNumber() == 0)
	{
		assert(dataTableSliceSet->GetTotalInstanceNumber() == 0);

		// Creation d'un fichier vide par slice
		for (nSlice = 0; nSlice < dataTableSliceSet->GetSliceNumber(); nSlice++)
		{
			dataTableSlice = dataTableSliceSet->GetSliceAt(nSlice);

			// Creation d'un nom de chunk temporaire propre a l'esclave
			sSliceBaseName = KWDataTableSlice::BuildSliceFileName(
			    nSlice, 0, shared_sourceDatabase.GetDatabase()->GetDatabaseName());
			sSliceFileName = FileService::CreateUniqueTmpFile(sSliceBaseName, this);
			bOk = sSliceFileName != "";

			// Memorisation du fichier et de sa taille
			if (bOk)
			{
				dataTableSlice->GetDataFileNames()->Add(sSliceFileName);
				dataTableSlice->GetDataFileSizes()->Add(0);
			}
			else
				break;

			// Arret si d'interruption utilisateur
			if (TaskProgression::IsInterruptionRequested())
			{
				bOk = false;
				break;
			}
		}
	}

	// Netoyage si necessaire des fichiers des tranches
	// pour supprimer les eventuels fichiers vides
	if (not bOk)
	{
		dataTableSliceSet->DeleteAllSliceFiles();
	}
	ensure(not bOk or dataTableSliceSet->Check());
	return bOk;
}

boolean KWDatabaseSlicerTask::SlaveInitialize()
{
	boolean bOk;

	require(allSliceOutputBuffer == NULL);

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveInitialize();

	// Creation et initialisation du buffer en sortie pour l'ensemble des tranches
	allSliceOutputBuffer = new KWDatabaseSlicerOutputBufferedFile;
	allSliceOutputBuffer->SetDataTableSliceSet(shared_DataTableSliceSet.GetDataTableSliceSet());
	allSliceOutputBuffer->SetBufferSize(shared_nAllSliceOutputBufferSize);

	return bOk;
}

boolean KWDatabaseSlicerTask::SlaveProcess()
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveProcess();
	return bOk;
}

boolean KWDatabaseSlicerTask::SlaveProcessStartDatabase()
{
	boolean bOk = true;
	int nSlice;
	ALString sSliceBaseName;
	ALString sSliceFileName;
	ALString sTmp;

	// Initialisation des variables en sortie
	output_lWrittenObjects = 0;
	output_svSliceFileNames.GetStringVector()->SetSize(0);
	output_lvSliceFileSizes.GetLongintVector()->SetSize(0);
	output_lvAllAttributeBlockValueNumbers.GetLongintVector()->SetSize(shared_nTotalBlockNumber);
	output_lvAllAttributeBlockValueNumbers.GetLongintVector()->Initialize();
	output_lvAllDenseSymbolAttributeDiskSizes.GetLongintVector()->SetSize(shared_nTotalDenseSymbolAttributeNumber);
	output_lvAllDenseSymbolAttributeDiskSizes.GetLongintVector()->Initialize();

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveProcessStartDatabase();

	// Creation des fichiers en sortie par tranche
	if (bOk)
	{
		// Creation des fichiers en sortie par tranche
		allSliceOutputBuffer->GetSliceFileNames()->SetSize(
		    allSliceOutputBuffer->GetDataTableSliceSet()->GetSliceNumber());
		for (nSlice = 0; nSlice < allSliceOutputBuffer->GetSliceFileNames()->GetSize(); nSlice++)
		{
			// Creation d'un nom de chunk temporaire propre a l'esclave
			sSliceBaseName = KWDataTableSlice::BuildSliceFileName(
			    nSlice, GetTaskIndex(), shared_sourceDatabase.GetDatabase()->GetDatabaseName());
			sSliceFileName = FileService::CreateUniqueTmpFile(sSliceBaseName, this);
			bOk = sSliceFileName != "";

			// Memorisation du fichier
			if (bOk)
			{
				allSliceOutputBuffer->GetSliceFileNames()->SetAt(nSlice, sSliceFileName);
				SlaveRegisterUniqueTmpFile(sSliceFileName);
			}
			else
				break;

			// Arret si d'interruption utilisateur
			if (TaskProgression::IsInterruptionRequested())
			{
				bOk = false;
				break;
			}
		}

		// Ouverture du buffer en sortie pour l'ensemble des tranches
		if (bOk)
			bOk = allSliceOutputBuffer->Open();

		// Test d'interruption utilisateur
		if (TaskProgression::IsInterruptionRequested())
			bOk = false;

		// Destruction des fichiers cree en cas d'erreur
		if (not bOk)
		{
			assert(not allSliceOutputBuffer->IsOpened() or TaskProgression::IsInterruptionRequested());
			DeleteSliceFiles();
		}
	}
	return bOk;
}

boolean KWDatabaseSlicerTask::SlaveProcessExploitDatabaseObject(const KWObject* kwoObject)
{
	boolean bOk = true;

	require(shared_DataTableSliceSet.GetDataTableSliceSet() == allSliceOutputBuffer->GetDataTableSliceSet());

	// Ecriture de l'objet pour le dispatcher dans les fichiers de tranche
	bOk = allSliceOutputBuffer->WriteObject(kwoObject, output_lvAllAttributeBlockValueNumbers.GetLongintVector(),
						output_lvAllDenseSymbolAttributeDiskSizes.GetLongintVector());

	// Test si erreur
	if (allSliceOutputBuffer->IsError())
	{
		AddError("Error in writing database slice files");
		bOk = false;
	}
	return bOk;
}

boolean KWDatabaseSlicerTask::SlaveProcessStopDatabase(boolean bProcessEndedCorrectly)
{
	boolean bOk = true;
	int nSlice;
	ALString sSliceFileName;
	boolean bCloseOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveProcessStopDatabase(bProcessEndedCorrectly);

	// On renvoi le nombre d'object ecrit
	if (bOk)
		output_lWrittenObjects = output_lReadObjects;

	// Fermeture des fichiers en sortie
	if (allSliceOutputBuffer->IsOpened())
	{
		bCloseOk = allSliceOutputBuffer->Close();
		bOk = bOk and bCloseOk;
		if (not bCloseOk)
			AddError("Error in closing database slice files");
	}

	// Collecte des nom complet de fichier (avec URI) et de leur taille, par tranche
	if (bOk)
	{
		// Dimensionnement des vecteurs en sortie
		assert(allSliceOutputBuffer->GetSliceFileNames()->GetSize() ==
		       allSliceOutputBuffer->GetDataTableSliceSet()->GetSliceNumber());
		output_svSliceFileNames.GetStringVector()->SetSize(
		    allSliceOutputBuffer->GetSliceFileNames()->GetSize());
		output_lvSliceFileSizes.GetLongintVector()->SetSize(
		    allSliceOutputBuffer->GetSliceFileNames()->GetSize());

		// Collecte des resultats
		for (nSlice = 0; nSlice < output_svSliceFileNames.GetSize(); nSlice++)
		{
			sSliceFileName = allSliceOutputBuffer->GetSliceFileNames()->GetAt(nSlice);
			assert(FileService::GetURIScheme(sSliceFileName) == "");

			// Memorisation de la taille du fichier, cree localement
			output_lvSliceFileSizes.SetAt(nSlice, FileService::GetFileSize(sSliceFileName));

			// Memorisation du nom complet avec URI, car le fichier pourra ensuite etre accede a distance
			output_svSliceFileNames.SetAt(nSlice, FileService::BuildLocalURI(sSliceFileName));
		}
	}

	// Destruction des fichiers si probleme
	if (not bOk)
		DeleteSliceFiles();
	return bOk;
}

boolean KWDatabaseSlicerTask::SlaveFinalize(boolean bProcessEndedCorrectly)
{
	boolean bOk;

	// Appel de la methode ancetre
	bOk = KWDatabaseTask::SlaveFinalize(bProcessEndedCorrectly);

	// Nettoyage
	if (allSliceOutputBuffer != NULL)
		delete allSliceOutputBuffer;
	allSliceOutputBuffer = NULL;
	return bOk;
}

void KWDatabaseSlicerTask::DeleteSliceFiles()
{
	int nSlice;

	require(IsInSlaveMethod());

	// Destruction des fichiers en sortie par tranche
	for (nSlice = 0; nSlice < allSliceOutputBuffer->GetSliceFileNames()->GetSize(); nSlice++)
	{
		// Destruction du fichier (pas d'URI)
		if (allSliceOutputBuffer->GetSliceFileNames()->GetAt(nSlice) != "")
		{
			assert(FileService::GetURIScheme(allSliceOutputBuffer->GetSliceFileNames()->GetAt(nSlice)) ==
			       "");
			FileService::RemoveFile(allSliceOutputBuffer->GetSliceFileNames()->GetAt(nSlice));
		}
	}

	// On dereference les noms de fichier et on reinitialise leur taille
	output_svSliceFileNames.GetStringVector()->Initialize();
	output_lvSliceFileSizes.GetLongintVector()->Initialize();
}

/////////////////////////////////////////////////////////////////////////////////
// Classe KWDatabaseSlicerOutputBufferedFile

KWDatabaseSlicerOutputBufferedFile::KWDatabaseSlicerOutputBufferedFile()
{
	nNextSliceIndex = 0;
	dataTableSliceSet = NULL;
	outputSliceFile = NULL;
}

KWDatabaseSlicerOutputBufferedFile::~KWDatabaseSlicerOutputBufferedFile()
{
	assert(outputSliceFile == NULL);
}

void KWDatabaseSlicerOutputBufferedFile::SetDataTableSliceSet(KWDataTableSliceSet* sliceSet)
{
	require(not IsOpened());
	dataTableSliceSet = sliceSet;
}

KWDataTableSliceSet* KWDatabaseSlicerOutputBufferedFile::GetDataTableSliceSet()
{
	return dataTableSliceSet;
}

StringVector* KWDatabaseSlicerOutputBufferedFile::GetSliceFileNames()
{
	return &svSliceFileNames;
}

longint KWDatabaseSlicerOutputBufferedFile::GetMinimumNecessaryMemory()
{
	longint lNecessaryMemory;
	lNecessaryMemory = sizeof(KWDatabaseSlicerOutputBufferedFile);
	lNecessaryMemory += sizeof(OutputBufferedFile);
	lNecessaryMemory += BufferedFile::nDefaultBufferSize;
	return lNecessaryMemory;
}

boolean KWDatabaseSlicerOutputBufferedFile::WriteObject(const KWObject* kwoObject,
							LongintVector* lvAllAttributeBlockValueNumbers,
							LongintVector* lvAllDenseSymbolAttributeBlockFileSizes)
{
	boolean bOk = true;
	KWDataTableSlice* dataTableSlice;
	int nSlice;
	const KWClass* kwcSliceSetClass;
	const KWClass* kwcSliceClass;
	int i;
	KWDataItem* dataItem;
	KWAttribute* attribute;
	KWAttributeBlock* attributeBlock;
	KWLoadIndex liLoadIndex;
	int nFieldIndex;
	const char* sValue;
	KWContinuousValueBlock* continuousValueBlock;
	KWSymbolValueBlock* symbolValueBlock;
	ALString sValueBlockField;
	int nGlobalBlockIndex;
	int nGlobalDenseSymbolAttributeIndex;
	int nWrittenValueNumber;

	require(dataTableSliceSet != NULL);
	require(dataTableSliceSet->GetSliceNumber() == svSliceFileNames.GetSize());
	require(lvAllAttributeBlockValueNumbers != NULL);
	require(lvAllDenseSymbolAttributeBlockFileSizes != NULL);

	// Acces a la specification des tranches
	kwcSliceSetClass = kwoObject->GetClass();
	assert(kwcSliceSetClass->GetName() == dataTableSliceSet->GetClassName());

	// Ecriture de l'objet en repartissant ses attributs dans les tranches
	nGlobalBlockIndex = 0;
	nGlobalDenseSymbolAttributeIndex = 0;
	for (nSlice = 0; nSlice < svSliceFileNames.GetSize(); nSlice++)
	{
		dataTableSlice = dataTableSliceSet->GetSliceAt(nSlice);

		// Parcours des data item de la classe associee a la tranche
		nFieldIndex = 0;
		kwcSliceClass = dataTableSlice->GetClass();
		assert(kwcSliceClass->GetLoadedAttributeNumber() == kwcSliceClass->GetUsedAttributeNumber());
		for (i = 0; i < kwcSliceClass->GetLoadedDataItemNumber(); i++)
		{
			dataItem = kwcSliceClass->GetLoadedDataItemAt(i);

			// Acces l'index de chargement de l'attribut correspondant de la classe a decouper
			liLoadIndex = dataTableSlice->GetDataItemLoadIndexes()->GetAt(i);

			// Cas d'un attribut dense
			if (dataItem->IsAttribute())
			{
				// Acces a l'attribut de la classe a decouper
				attribute = kwcSliceSetClass->GetAttributeAtLoadIndex(liLoadIndex);

				// Ecriture de la valeur du champs
				if (KWType::IsStored(attribute->GetType()))
				{
					if (nFieldIndex > 0)
						Write(GetFieldSeparator());
					sValue = kwoObject->ValueToString(attribute);
					WriteField(sValue);
					nFieldIndex++;

					// Memorisation de la taille occupee dans le fichier pouyr les attributs Symbol
					if (attribute->GetType() == KWType::Symbol)
					{
						lvAllDenseSymbolAttributeBlockFileSizes->UpgradeAt(
						    nGlobalDenseSymbolAttributeIndex, (longint)strlen(sValue));
						nGlobalDenseSymbolAttributeIndex++;
					}
				}
			}
			// Cas d'un bloc d'attributs
			else
			{
				// Acces au bloc d'attributs de la classe a decouper
				attributeBlock = kwcSliceSetClass->GetAttributeBlockAtLoadIndex(liLoadIndex);

				// Acces au bloc de valeur sparse, selon son type
				if (KWType::IsStored(attributeBlock->GetType()))
				{
					if (nFieldIndex > 0)
						Write(GetFieldSeparator());
					if (attributeBlock->GetType() == KWType::Continuous)
					{
						continuousValueBlock =
						    kwoObject->GetContinuousValueBlockAt(liLoadIndex);

						// Ecriture du bloc
						nWrittenValueNumber = continuousValueBlock->WriteFieldPart(
						    attributeBlock->GetLoadedAttributesIndexedKeyBlock(),
						    dataTableSlice->GetValueBlockFirstSparseIndexes()->GetAt(i),
						    dataTableSlice->GetValueBlockLastSparseIndexes()->GetAt(i),
						    sValueBlockField);
						WriteField(sValueBlockField);

						// Memorisation du nombre de valeurs presentes du bloc
						lvAllAttributeBlockValueNumbers->UpgradeAt(nGlobalBlockIndex,
											   nWrittenValueNumber);
						nGlobalBlockIndex++;
					}
					else if (attributeBlock->GetType() == KWType::Symbol)
					{
						symbolValueBlock = kwoObject->GetSymbolValueBlockAt(liLoadIndex);

						// Ecriture du bloc
						nWrittenValueNumber = symbolValueBlock->WriteFieldPart(
						    attributeBlock->GetLoadedAttributesIndexedKeyBlock(),
						    dataTableSlice->GetValueBlockFirstSparseIndexes()->GetAt(i),
						    dataTableSlice->GetValueBlockLastSparseIndexes()->GetAt(i),
						    sValueBlockField);
						WriteField(sValueBlockField);

						// Memorisation du nombre de valeur presentes du bloc
						lvAllAttributeBlockValueNumbers->UpgradeAt(nGlobalBlockIndex,
											   nWrittenValueNumber);
						nGlobalBlockIndex++;
					}
					nFieldIndex++;
				}
			}
		}
		WriteEOL();
	}
	assert(lvAllDenseSymbolAttributeBlockFileSizes->GetSize() == nGlobalDenseSymbolAttributeIndex);
	assert(lvAllAttributeBlockValueNumbers->GetSize() == nGlobalBlockIndex);
	return bOk;
}

boolean KWDatabaseSlicerOutputBufferedFile::Open()
{
	boolean bOk;

	require(not IsOpened());
	require(GetFileName() == "");
	require(dataTableSliceSet != NULL);
	require(dataTableSliceSet->GetSliceNumber() == svSliceFileNames.GetSize());
	require(nNextSliceIndex == 0);
	require(ivLineOffsets.GetSize() == 0);

	// Initialisation de la taille du buffer
	bOk = AllocateBuffer();

	// Creation du fichier en sortie
	if (bOk)
	{
		outputSliceFile = new OutputBufferedFile;
		outputSliceFile->SetHeaderLineUsed(false);
	}

	// Ouverture uniquement du buffer, sans ouvrir de fichier
	bIsOpened = bOk;
	bIsError = not bOk;
	nCurrentBufferSize = 0;
	return IsOpened();
}

boolean KWDatabaseSlicerOutputBufferedFile::Close()
{
	boolean bOk;

	require(IsOpened());

	// Ecriture du contenu du buffer
	FlushCache();
	bOk = not bIsError;

	// Nettoyage
	nNextSliceIndex = 0;
	ivLineOffsets.SetSize(0);

	// Destruction du fichier en sortie
	assert(not outputSliceFile->IsOpened());
	delete outputSliceFile;
	outputSliceFile = NULL;

	// Fermeture du buffer
	assert(fileHandle == NULL);
	bIsOpened = false;
	bIsError = false;
	nCurrentBufferSize = 0;
	return bOk;
}

boolean KWDatabaseSlicerOutputBufferedFile::FlushCache()
{
	boolean bOk;
	int nOutputBufferSize;
	int nLargestSliceBufferSize;
	int nSliceBufferSize;
	int nBeginOffset;
	int nEndOffset;
	int nSliceNumber;
	int nWrittenSliceNumber;
	int nSlice;
	int i;
	int nLine;

	require(IsOpened());
	require(nBufferSize >= 0);
	require(outputSliceFile != NULL);
	require(not outputSliceFile->IsOpened());

	// Arret si traitement inutile
	if (nCurrentBufferSize == 0)
		return not bIsError;
	if (bIsError)
		return false;

	// Calcul du nombre de tranche a ecrire
	nSliceNumber = dataTableSliceSet->GetSliceNumber();
	nWrittenSliceNumber = min(nSliceNumber, ivLineOffsets.GetSize() + 1);

	// Cas particulier d'une seule tranche a ecrire
	if (nWrittenSliceNumber == 1)
	{
		// Index de la tranche
		nSlice = nNextSliceIndex % nSliceNumber;

		// Parametrage de la taille du buffer de sortie
		nOutputBufferSize = nCurrentBufferSize;
		nOutputBufferSize =
		    MemSegmentByteSize * ((nOutputBufferSize + MemSegmentByteSize - 1) / MemSegmentByteSize);
		if (BufferedFile::nDefaultBufferSize < nOutputBufferSize)
			nOutputBufferSize = BufferedFile::nDefaultBufferSize;
		outputSliceFile->SetBufferSize(nOutputBufferSize);

		// Ouverture du fichier a ecrire (pas d'URI ici)
		assert(FileService::GetURIScheme(svSliceFileNames.GetAt(nSlice)) == "");
		outputSliceFile->SetFileName(svSliceFileNames.GetAt(nSlice));
		bOk = outputSliceFile->OpenForAppend();
		bIsError = not bOk;

		// Ecriture de tout le buffer dans le fichier
		if (not bIsError)
		{
			outputSliceFile->WriteSubPart(GetCache(), 0, nCurrentBufferSize);

			// Fermeture du fichier
			bOk = outputSliceFile->Close();
			bIsError = not bOk;
		}
	}
	// Cas general
	else
	{
		// Calcul de la taille de la plus grosse tranche a ecrire
		// Parcours des tranches a ecrire
		nLargestSliceBufferSize = 0;
		for (nSlice = 0; nSlice < nWrittenSliceNumber; nSlice++)
		{
			nSliceBufferSize = 0;

			// Parcours des lignes de la tranche
			nLine = nSlice;
			while (nLine <= ivLineOffsets.GetSize())
			{
				// Ajout de la longueur de ligne
				if (nLine == 0)
					nSliceBufferSize += ivLineOffsets.GetAt(0);
				else if (nLine == ivLineOffsets.GetSize())
					nSliceBufferSize += nCurrentBufferSize - ivLineOffsets.GetAt(nLine - 1);
				else
					nSliceBufferSize += ivLineOffsets.GetAt(nLine) - ivLineOffsets.GetAt(nLine - 1);

				// Ligne suivante
				nLine += nSliceNumber;
			}

			// Mise a jour de la plus grosse tranche
			nLargestSliceBufferSize = max(nLargestSliceBufferSize, nSliceBufferSize);
		}

		// Parametrage de la taille du buffer
		nOutputBufferSize = nLargestSliceBufferSize;
		nOutputBufferSize =
		    MemSegmentByteSize * ((nOutputBufferSize + MemSegmentByteSize - 1) / MemSegmentByteSize);
		if (BufferedFile::nDefaultBufferSize < nOutputBufferSize)
			nOutputBufferSize = BufferedFile::nDefaultBufferSize;
		outputSliceFile->SetBufferSize(nOutputBufferSize);

		// Parcours des tranches a ecrire
		for (i = 0; i < nWrittenSliceNumber; i++)
		{
			// Index de la tranche
			nSlice = (nNextSliceIndex + i) % nSliceNumber;

			// Ouverture du fichier a ecrire  (pas d'URI ici)
			assert(FileService::GetURIScheme(svSliceFileNames.GetAt(nSlice)) == "");
			outputSliceFile->SetFileName(svSliceFileNames.GetAt(nSlice));
			bOk = outputSliceFile->OpenForAppend();
			bIsError = not bOk;

			// Ecriture des lignes concernees dans le fichier
			if (not bIsError)
			{
				// Index de la premiere ligne a ecrire
				if (i == 0)
					nBeginOffset = 0;
				else
					nBeginOffset = ivLineOffsets.GetAt(i - 1);
				assert(0 <= nBeginOffset and nBeginOffset <= nCurrentBufferSize);

				// Parcours des lignes concernees
				nLine = i;
				while (nBeginOffset < nCurrentBufferSize)
				{
					// Ecriture de la partie concernee
					if (nLine < ivLineOffsets.GetSize())
						nEndOffset = ivLineOffsets.GetAt(nLine);
					else
						nEndOffset = nCurrentBufferSize;
					outputSliceFile->WriteSubPart(GetCache(), nBeginOffset,
								      nEndOffset - nBeginOffset);

					// Passage a la ligne suivante pour cette tranche
					nLine += nSliceNumber;
					if (nLine <= ivLineOffsets.GetSize())
						nBeginOffset = ivLineOffsets.GetAt(nLine - 1);
					else
						nBeginOffset = nCurrentBufferSize;
				}

				// Fermeture du fichier
				bOk = outputSliceFile->Close();
				bIsError = not bOk;
			}

			// Arret si erreur
			if (bIsError)
				break;
		}
	}

	// Calcul de l'index de la prochaine tranche a ecrire
	nNextSliceIndex = (nNextSliceIndex + ivLineOffsets.GetSize()) + dataTableSliceSet->GetSliceNumber();

	// Reinitialisation des positions des lignes dans le buffer
	ivLineOffsets.SetSize(0);

	// On vide le buffer
	nCurrentBufferSize = 0;

	return not bIsError;
}