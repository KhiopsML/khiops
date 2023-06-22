// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLMTDatabaseTextFile.h"

PLMTDatabaseTextFile::PLMTDatabaseTextFile()
{
	lTotalFileSize = 0;
	lTotalUsedFileSize = 0;
	lOutputNecessaryDiskSpace = 0;
	lEmptyOpenNecessaryMemory = 0;
	lMinOpenNecessaryMemory = 0;
	lMaxOpenNecessaryMemory = 0;

	// Changement de driver : on prend le driver de table parallele
	KWDataTableDriver* parallelDataTableDriver = new PLDataTableDriverTextFile;
	parallelDataTableDriver->CopyFrom(dataTableDriverCreator);
	delete dataTableDriverCreator;
	dataTableDriverCreator = parallelDataTableDriver;
}

PLMTDatabaseTextFile::~PLMTDatabaseTextFile()
{
	oaIndexedMappingsDataItemLoadIndexes.DeleteAll();
	oaIndexedMappingsRootKeyIndexes.DeleteAll();
}

boolean PLMTDatabaseTextFile::ComputeOpenInformation(boolean bRead, boolean bIncludingClassMemory,
						     PLMTDatabaseTextFile* outputDatabaseTextFile)
{
	boolean bOk = true;
	KWMTDatabaseMapping* mapping;
	KWMTDatabaseMapping* outputMapping;
	PLDataTableDriverTextFile* driver;
	KWLoadIndexVector* livDataItemLoadIndexes;
	IntVector* ivRootKeyIndexes;
	KWClass* kwcDriverLogicalClass;
	longint lDatabaseClassNecessaryMemory;
	int nReferenceBufferSize;
	int i;

	require(outputDatabaseTextFile == NULL or bRead);
	require(not IsOpenedForRead() and not IsOpenedForWrite());

	// Initialisation des resultats
	CleanOpenInformation();

	// Acces a la classe
	kwcClass = KWClassDomain::GetCurrentDomain()->LookupClass(GetClassName());
	require(kwcClass != NULL);
	require(kwcClass->Check());

	// En lecture, construction de la classe physique
	if (bRead)
		BuildPhysicalClass();

	// Estimation de la memoire minimale necessaire pour ouvrir la base avec des buffers de taille nulle
	// Attention, cette methode cree et detruit les mapping pour faire son estimation
	nReferenceBufferSize = GetBufferSize();
	SetBufferSize(0);
	lEmptyOpenNecessaryMemory = ComputeOpenNecessaryMemory(bRead, bIncludingClassMemory);
	SetBufferSize(nReferenceBufferSize);

	// La memoire precedente est estimee principalement en fonction de la taille occupee par le dictionnaire
	// qui peut etre largement sous-estimee dans certains cas (difference d'occupation memoire selon que le
	// dictionnaire vient d'etre charge en memoire, est optimise, compile...)
	// On corrige cette sous-estimation en prenant en compte en plus deux fois les dictionnaires
	lDatabaseClassNecessaryMemory = 2 * KWDatabase::ComputeOpenNecessaryMemory(bRead, bIncludingClassMemory);
	lEmptyOpenNecessaryMemory += lDatabaseClassNecessaryMemory;
	lMinOpenNecessaryMemory = lEmptyOpenNecessaryMemory;

	// Pour le max, on ne se limite pas (il peut y avoir des sous-estimation importantes pour les tables externes)
	lMaxOpenNecessaryMemory = lEmptyOpenNecessaryMemory * 5;

	// Initialisation recursive du mapping a partir de la racine pour avoir des driver initialises
	DMTMPhysicalTerminateMapping(rootMultiTableMapping);
	if (bRead)
	{
		// En lecture, on utilise la classe physique
		DMTMPhysicalInitializeMapping(rootMultiTableMapping, kwcPhysicalClass, true);
	}
	else
	{
		// En ecriture, on utile la classe logique
		DMTMPhysicalInitializeMapping(rootMultiTableMapping, kwcClass, false);
	}

	// Calcul des index pour tous les mappings, et recopie des caracteristiques des drivers
	// dans leur version serialisee
	oaIndexedMappingsDataItemLoadIndexes.SetSize(GetMultiTableMappings()->GetSize());
	oaIndexedMappingsRootKeyIndexes.SetSize(GetMultiTableMappings()->GetSize());
	oaUsedMappings.SetSize(GetMultiTableMappings()->GetSize());
	lvFileSizes.SetSize(GetMultiTableMappings()->GetSize());
	for (i = 0; i < GetMultiTableMappings()->GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(i));
		driver = cast(PLDataTableDriverTextFile*, mapping->GetDataTableDriver());

		// Memorisation du mapping si driver utilise
		if (driver != NULL)
		{
			assert(mapping->GetDataPath() != "");
			oaUsedMappings.SetAt(i, mapping);
		}

		// Memorisation de la taille du fichier d'entree
		lvFileSizes.SetAt(i, PLRemoteFileService::GetFileSize(mapping->GetDataTableName()));
		if (bRead)
			lTotalFileSize += lvFileSizes.GetAt(i);
		if (bRead and driver != NULL)
			lTotalUsedFileSize += lvFileSizes.GetAt(i);

		// Si driver present, on calcule et memorise le vecteur d'index des champs de la table en entree
		if (bRead and driver != NULL)
		{
			// Recherche de la classe logique correspondant a la classe du driver
			kwcDriverLogicalClass =
			    KWClassDomain::GetCurrentDomain()->LookupClass(driver->GetClass()->GetName());
			check(kwcDriverLogicalClass);

			// Indexation du driver
			bOk = bOk and driver->ComputeDataItemLoadIndexes(kwcDriverLogicalClass);
			if (not bOk)
				break;

			// Memorisation du vecteur des indexes des champs
			livDataItemLoadIndexes = driver->GetDataItemLoadIndexes()->Clone();
			oaIndexedMappingsDataItemLoadIndexes.SetAt(i, livDataItemLoadIndexes);

			// Memorisation du vecteur des indexes des champs de la cle
			ivRootKeyIndexes = driver->GetRootKeyIndexes()->Clone();
			oaIndexedMappingsRootKeyIndexes.SetAt(i, ivRootKeyIndexes);

			// Estimation de la place disque necessaire en sortie
			if (outputDatabaseTextFile != NULL)
			{
				outputMapping =
				    cast(KWMTDatabaseMapping*,
					 outputDatabaseTextFile->LookupMultiTableMapping(mapping->GetDataPath()));

				// Mise a jour de la taille estimee si necessaire
				if (outputMapping != NULL and outputMapping->GetDataTableName() != "")
				{
					// On recherche si l'attribut terminal du DataPath est utilise
					if (outputMapping->IsTerminalAttributeUsed())
					{
						assert(outputMapping->GetClassName() ==
						       kwcDriverLogicalClass->GetName());
						lOutputNecessaryDiskSpace +=
						    driver->ComputeNecessaryDiskSpaceForFullWrite(
							kwcDriverLogicalClass);
					}
				}
			}
		}
	}

	// Correction de la place disque necessaire en sortie en fonction du pourcentage d'exemples a lire dans la base
	// d'entree
	if (GetModeExcludeSample())
		lOutputNecessaryDiskSpace = (lOutputNecessaryDiskSpace * (100 - GetSampleNumberPercentage())) / 100;
	else
		lOutputNecessaryDiskSpace = (lOutputNecessaryDiskSpace * GetSampleNumberPercentage()) / 100;

	// S'il y a un critere de selection, on ne peut prevoir la place necessaire
	// On se place alors de facon heuristique sur 5% (pour ne pas sur-estimer cette place et interdire la tache)
	// De toutes facon, il faudra que la tache suive regulierement la place restante en ecriture
	if (GetSelectionAttribute() != "")
		lOutputNecessaryDiskSpace /= 20;

	// Nettoyage
	DMTMPhysicalTerminateMapping(rootMultiTableMapping);
	if (bRead)
		DeletePhysicalClass();
	kwcClass = NULL;

	// Calcul des min et max de la memoire necessaire en fonction de taille de buffer min et max
	if (bOk)
	{
		// Mise a jour des min et max de memoire necessaire, en tenant compte des tailles de fichier
		lMinOpenNecessaryMemory += ComputeBufferNecessaryMemory(bRead, nMinOpenBufferSize);
		lMaxOpenNecessaryMemory += ComputeBufferNecessaryMemory(bRead, nMaxOpenBufferSize);
	}

	// Nettoyage des resultat si KO
	if (not bOk)
		CleanOpenInformation();

	ensure(not bOk or GetTableNumber() == oaUsedMappings.GetSize());
	return bOk;
}

boolean PLMTDatabaseTextFile::IsOpenInformationComputed() const
{
	return lMinOpenNecessaryMemory > 0;
}

void PLMTDatabaseTextFile::CleanOpenInformation()
{
	oaUsedMappings.SetSize(0);
	lvFileSizes.SetSize(0);
	lTotalFileSize = 0;
	lTotalUsedFileSize = 0;
	lOutputNecessaryDiskSpace = 0;
	lEmptyOpenNecessaryMemory = 0;
	lMinOpenNecessaryMemory = 0;
	lMaxOpenNecessaryMemory = 0;
	oaIndexedMappingsDataItemLoadIndexes.DeleteAll();
	oaIndexedMappingsRootKeyIndexes.DeleteAll();
}

const ObjectArray* PLMTDatabaseTextFile::GetUsedMappings() const
{
	int i;

	require(IsOpenInformationComputed());
	ensure(GetTableNumber() == oaUsedMappings.GetSize());

	// On doit resynchroniser potentiellement le tableau avec celui des mappings, qui ont potentiellement
	// ete mis a jour au moyen d'un UpdateTableMappings
	for (i = 0; i < oaUsedMappings.GetSize(); i++)
	{
		if (oaUsedMappings.GetAt(i) != NULL)
			oaUsedMappings.SetAt(i, oaMultiTableMappings.GetAt(i));
	}
	return &oaUsedMappings;
}

int PLMTDatabaseTextFile::GetUsedMappingNumber() const
{
	int nUsedMappingNumber;
	int i;

	require(IsOpenInformationComputed());
	ensure(GetTableNumber() == oaUsedMappings.GetSize());

	// Il suffit de compter le nombre d'elements non nuls, sans avoir besoin de resynchroniser le tableau
	nUsedMappingNumber = 0;
	for (i = 0; i < oaUsedMappings.GetSize(); i++)
	{
		if (oaUsedMappings.GetAt(i) != NULL)
			nUsedMappingNumber++;
	}
	return nUsedMappingNumber;
}

const LongintVector* PLMTDatabaseTextFile::GetFileSizes() const
{
	require(IsOpenInformationComputed());
	return &lvFileSizes;
}

longint PLMTDatabaseTextFile::GetTotalFileSize() const
{
	require(IsOpenInformationComputed());
	return lTotalFileSize;
}

longint PLMTDatabaseTextFile::GetTotalUsedFileSize() const
{
	require(IsOpenInformationComputed());
	return lTotalUsedFileSize;
}

longint PLMTDatabaseTextFile::GetEmptyOpenNecessaryMemory() const
{
	require(IsOpenInformationComputed());
	return lEmptyOpenNecessaryMemory;
}

longint PLMTDatabaseTextFile::GetMinOpenNecessaryMemory() const
{
	require(IsOpenInformationComputed());
	return lMinOpenNecessaryMemory;
}

longint PLMTDatabaseTextFile::GetMaxOpenNecessaryMemory() const
{
	require(IsOpenInformationComputed());
	return lMaxOpenNecessaryMemory;
}

longint PLMTDatabaseTextFile::GetOutputNecessaryDiskSpace() const
{
	require(IsOpenInformationComputed());
	return lOutputNecessaryDiskSpace;
}

int PLMTDatabaseTextFile::GetMaxSlaveProcessNumber() const
{
	require(IsOpenInformationComputed());
	return (int)ceil((lTotalUsedFileSize + 1.0) / nMinOpenBufferSize);
}

int PLMTDatabaseTextFile::ComputeOpenBufferSize(boolean bRead, longint lOpenGrantedMemory) const
{
	longint lOpenNecessaryMemory;
	int nLowerBufferSize;
	int nUpperBufferSize;
	int nBufferSize;

	require(IsOpenInformationComputed());
	require(lMinOpenNecessaryMemory <= lOpenGrantedMemory);
	require(lOpenGrantedMemory <= lMaxOpenNecessaryMemory);

	// Cas ou on a alloue le min
	nBufferSize = 0;
	if (lOpenGrantedMemory == lMinOpenNecessaryMemory)
		nBufferSize = nMinOpenBufferSize;
	// Cas ou on a alloue le min
	else if (lOpenGrantedMemory == lMaxOpenNecessaryMemory)
		nBufferSize = nMaxOpenBufferSize;
	// Cas intermediaire
	else
	{
		// On recherche par dichotomie la taille de buffer permettant d'utiliser au mieux la memoire alouee
		nLowerBufferSize = nMinOpenBufferSize;
		nUpperBufferSize = nMaxOpenBufferSize;
		while (nUpperBufferSize - nLowerBufferSize > MemSegmentByteSize)
		{
			nBufferSize = (nUpperBufferSize + nLowerBufferSize) / 2;

			// Calcul de la memoire necessaire avec la taille de buffer courante
			lOpenNecessaryMemory =
			    lEmptyOpenNecessaryMemory + ComputeBufferNecessaryMemory(bRead, nBufferSize);

			// Dichotomie
			if (lOpenNecessaryMemory < lOpenGrantedMemory)
				nLowerBufferSize = nBufferSize;
			else
				nUpperBufferSize = nBufferSize;
		}

		// Ajustement final pour que les buffer soient un nombre entier de segments
		nBufferSize = MemSegmentByteSize * (nBufferSize / MemSegmentByteSize);
		if (nBufferSize < nMinOpenBufferSize)
			nBufferSize = nMinOpenBufferSize;
		if (nBufferSize > nMaxOpenBufferSize)
			nBufferSize = nMaxOpenBufferSize;
		lOpenNecessaryMemory = lEmptyOpenNecessaryMemory + ComputeBufferNecessaryMemory(bRead, nBufferSize);
		if (lOpenNecessaryMemory > lOpenGrantedMemory)
			nBufferSize -= MemSegmentByteSize;
		ensure(lEmptyOpenNecessaryMemory + ComputeBufferNecessaryMemory(bRead, nBufferSize) <=
		       lOpenGrantedMemory);
	}
	ensure(nMinOpenBufferSize <= nBufferSize and nBufferSize <= nMaxOpenBufferSize);
	return nBufferSize;
}

void PLMTDatabaseTextFile::SetBufferSize(int nBufferSize)
{
	cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->SetBufferSize(nBufferSize);
}

int PLMTDatabaseTextFile::GetBufferSize() const
{
	return cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->GetBufferSize();
}

void PLMTDatabaseTextFile::SetInputBufferedFileAt(KWMTDatabaseMapping* mapping, InputBufferedFile* buffer)
{
	require(IsOpenedForRead());
	require(mapping != NULL);
	require(LookupMultiTableMapping(mapping->GetDataPath()) == mapping);
	require(mapping->GetDataTableDriver() != NULL);
	cast(PLDataTableDriverTextFile*, mapping->GetDataTableDriver())->SetInputBuffer(buffer);
}

InputBufferedFile* PLMTDatabaseTextFile::GetInputBufferedFileAt(KWMTDatabaseMapping* mapping)
{
	require(IsOpenedForRead());
	require(mapping != NULL);
	require(LookupMultiTableMapping(mapping->GetDataPath()) == mapping);
	if (mapping->GetDataTableDriver() == NULL)
		return NULL;
	else
		return cast(PLDataTableDriverTextFile*, mapping->GetDataTableDriver())->GetInputBuffer();
}

void PLMTDatabaseTextFile::SetOutputBufferedFileAt(KWMTDatabaseMapping* mapping, OutputBufferedFile* buffer)
{
	require(IsOpenedForWrite());
	require(mapping != NULL);
	require(LookupMultiTableMapping(mapping->GetDataPath()) == mapping);
	require(mapping->GetDataTableDriver() != NULL);
	cast(PLDataTableDriverTextFile*, mapping->GetDataTableDriver())->SetOutputBuffer(buffer);
}

OutputBufferedFile* PLMTDatabaseTextFile::GetOutputBufferedFileAt(KWMTDatabaseMapping* mapping)
{
	require(IsOpenedForWrite());
	require(mapping != NULL);
	require(LookupMultiTableMapping(mapping->GetDataPath()) == mapping);
	if (mapping->GetDataTableDriver() == NULL)
		return NULL;
	else
		return cast(PLDataTableDriverTextFile*, mapping->GetDataTableDriver())->GetOutputBuffer();
}

boolean PLMTDatabaseTextFile::CreateInputBuffers()
{
	InputBufferedFile* inputBuffer;
	int i;
	KWMTDatabaseMapping* mapping;

	require(IsOpenedForRead());

	// Parcours des mapping pour creer et ouvrir les buffers en lecture
	for (i = 0; i < GetTableNumber(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(i));

		// On ne considere que les mapping des tables internes
		if (not IsReferencedClassMapping(mapping) and IsMappingInitialized(mapping))
		{
			// Creation et initialisation du buffer pour la base d'entree
			inputBuffer = new InputBufferedFile;
			inputBuffer->SetHeaderLineUsed(GetHeaderLineUsed());
			inputBuffer->SetFieldSeparator(GetFieldSeparator());
			SetInputBufferedFileAt(mapping, inputBuffer);
		}
	}
	return true;
}

boolean PLMTDatabaseTextFile::OpenInputBuffers()
{
	boolean bOk = true;
	InputBufferedFile* inputBuffer;
	int i;
	KWMTDatabaseMapping* mapping;

	require(IsOpenedForRead());

	// Parcours des mapping pour creer et ouvrir les buffers en lecture
	for (i = 0; i < GetTableNumber(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(i));

		// On ne considere que les mapping des tables internes
		if (not IsReferencedClassMapping(mapping) and IsMappingInitialized(mapping))
		{
			inputBuffer = GetInputBufferedFileAt(mapping);
			check(inputBuffer);

			// Ouverture du buffer en entree
			inputBuffer->SetFileName(mapping->GetDataTableName());
			bOk = inputBuffer->Open();
			if (not bOk)
			{
				AddError("Unable to open imput database " + GetDatabaseName());
				break;
			}
		}
	}

	// Nettoyage si necessaire
	if (not bOk)
		CloseInputBuffers();
	return bOk;
}

boolean PLMTDatabaseTextFile::CloseInputBuffers()
{
	boolean bOk = true;
	InputBufferedFile* inputBuffer;
	KWMTDatabaseMapping* mapping;
	int i;

	// Parcours des mapping pour fermer les buffers en lecture
	if (IsOpenedForRead())
	{
		for (i = 0; i < GetTableNumber(); i++)
		{
			mapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(i));
			inputBuffer = GetInputBufferedFileAt(mapping);
			if (inputBuffer != NULL)
			{
				if (inputBuffer->IsOpened())
					bOk = inputBuffer->Close() and bOk;
			}
		}
	}
	return bOk;
}

boolean PLMTDatabaseTextFile::DeleteInputBuffers()
{
	InputBufferedFile* inputBuffer;
	KWMTDatabaseMapping* mapping;
	int i;

	// Parcours des mapping pour fermer les buffers en lecture
	if (IsOpenedForRead())
	{
		for (i = 0; i < GetTableNumber(); i++)
		{
			mapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(i));
			inputBuffer = GetInputBufferedFileAt(mapping);
			if (inputBuffer != NULL)
			{
				assert(not inputBuffer->IsOpened());
				SetInputBufferedFileAt(mapping, NULL);
				delete inputBuffer;
			}
		}
	}
	return true;
}

boolean PLMTDatabaseTextFile::CreateOutputBuffers()
{
	OutputBufferedFile* outputBuffer;
	int i;
	KWMTDatabaseMapping* mapping;

	require(IsOpenedForWrite());

	// Parcours des mapping pour creer et ouvrir les buffers en ecriture
	for (i = 0; i < GetTableNumber(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(i));

		// On ne considere que les mapping des tables internes
		if (not IsReferencedClassMapping(mapping) and IsMappingInitialized(mapping))
		{
			// Creation et initialisation du buffer pour la base de sortie
			outputBuffer = new OutputBufferedFile;
			outputBuffer->SetHeaderLineUsed(GetHeaderLineUsed());
			outputBuffer->SetFieldSeparator(GetFieldSeparator());
			SetOutputBufferedFileAt(mapping, outputBuffer);
		}
	}
	return true;
}

boolean PLMTDatabaseTextFile::OpenOutputBuffers(const PLParallelTask* task, int nTaskIndex,
						StringVector* svOutputBufferFileNames)
{
	boolean bOk = true;
	OutputBufferedFile* outputBuffer;
	int i;
	KWMTDatabaseMapping* mapping;
	ALString sChunkBaseName;
	ALString sChunkFileName;
	ALString sTmp;

	require(IsOpenedForWrite());
	require(task != NULL);
	require(nTaskIndex >= 0);
	require(svOutputBufferFileNames != NULL);

	// Parcours des mapping pour creer et ouvrir les buffers en ecriture
	svOutputBufferFileNames->SetSize(GetTableNumber());
	svOutputBufferFileNames->Initialize();
	for (i = 0; i < GetTableNumber(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(i));

		// On ne considere que les mapping des tables internes
		if (not IsReferencedClassMapping(mapping) and IsMappingInitialized(mapping))
		{
			outputBuffer = GetOutputBufferedFileAt(mapping);
			check(outputBuffer);

			// Creation d'un nom de chunk temporaire propre a l'esclave
			// On incorpore l'index de la table secondaires, car les fichiers sources correspondants
			//  pourraient avoir le meme nom s'ils sont dans des repertoire differents
			sChunkBaseName = sTmp + IntToString(nTaskIndex) + "_M" + IntToString(i) + "_" +
					 FileService::GetFileName(mapping->GetDataTableName());
			sChunkFileName = FileService::CreateUniqueTmpFile(sChunkBaseName, task);
			bOk = sChunkFileName != "";
			if (bOk)
			{
				// Memorisation du fichier
				svOutputBufferFileNames->SetAt(i, FileService::BuildLocalURI(sChunkFileName));

				// Ouverture du fichier
				outputBuffer->SetFileName(sChunkFileName);
				bOk = outputBuffer->Open();
			}
			if (not bOk)
			{
				AddError("Unable to open target database " + GetDatabaseName());
				break;
			}
		}
	}

	// Nettoyage si necessaire
	if (not bOk)
	{
		CloseOutputBuffers();
		DeleteOutputBuffers();

		// Destruction des fichiers
		for (i = 0; i < svOutputBufferFileNames->GetSize(); i++)
		{
			sChunkFileName = svOutputBufferFileNames->GetAt(i);
			if (sChunkFileName != "")
				FileService::RemoveFile(FileService::GetURIFilePathName(sChunkFileName));
		}
		svOutputBufferFileNames->Initialize();
	}
	return bOk;
}

boolean PLMTDatabaseTextFile::CloseOutputBuffers()
{
	boolean bOk = true;
	boolean bCloseOk;
	OutputBufferedFile* outputBuffer;
	KWMTDatabaseMapping* mapping;
	int i;

	// Parcours des mapping pour fermer les buffers en ecriture
	if (IsOpenedForWrite())
	{
		for (i = 0; i < GetTableNumber(); i++)
		{
			mapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(i));
			outputBuffer = GetOutputBufferedFileAt(mapping);
			if (outputBuffer != NULL)
			{
				if (outputBuffer->IsOpened())
				{
					bCloseOk = outputBuffer->Close();
					bOk = bOk and bCloseOk;
					if (not bCloseOk)
					{
						AddError("Cannot close output file " + outputBuffer->GetFileName());
						break;
					}
				}
			}
		}
	}
	return bOk;
}

boolean PLMTDatabaseTextFile::DeleteOutputBuffers()
{
	OutputBufferedFile* outputBuffer;
	KWMTDatabaseMapping* mapping;
	int i;

	// Parcours des mapping pour fermer les buffers en ecriture
	if (IsOpenedForWrite())
	{
		for (i = 0; i < GetTableNumber(); i++)
		{
			mapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(i));
			outputBuffer = GetOutputBufferedFileAt(mapping);
			if (outputBuffer != NULL)
			{
				assert(not outputBuffer->IsOpened());
				SetOutputBufferedFileAt(mapping, NULL);
				delete outputBuffer;
			}
		}
	}
	return true;
}

boolean PLMTDatabaseTextFile::IsMappingInitialized(KWMTDatabaseMapping* mapping)
{
	require(mapping != NULL);
	require(LookupMultiTableMapping(mapping->GetDataPath()) == mapping);
	return (mapping->GetDataTableDriver() != NULL);
}

void PLMTDatabaseTextFile::SetLastReadRootKey(const KWObjectKey* objectKey)
{
	KWMTDatabaseMapping* rootMapping;

	require(IsOpenedForRead());

	rootMapping = cast(KWMTDatabaseMapping*, GetMultiTableMappings()->GetAt(0));
	check(rootMapping);
	assert(rootMapping->GetLastReadKey()->GetSize() == 0);
	assert(rootMapping->GetLastReadObject() == NULL);
	rootMapping->SetLastReadKey(objectKey);
}

void PLMTDatabaseTextFile::CleanMapping(KWMTDatabaseMapping* mapping)
{
	require(IsOpenedForRead());
	require(mapping != NULL);
	require(LookupMultiTableMapping(mapping->GetDataPath()) == mapping);

	// Nettoyage de la cle
	mapping->CleanLastReadKey();

	// Destruction du dernier objet lu en cours si necessaire
	if (mapping->GetLastReadObject() != NULL)
	{
		delete mapping->GetLastReadObject();
		mapping->SetLastReadObject(NULL);
	}
}

PLDataTableDriverTextFile* PLMTDatabaseTextFile::GetDriverAt(KWMTDatabaseMapping* mapping)
{
	require(mapping != NULL);
	require(LookupMultiTableMapping(mapping->GetDataPath()) == mapping);
	return cast(PLDataTableDriverTextFile*, mapping->GetDataTableDriver());
}

void PLMTDatabaseTextFile::PhysicalDeleteDatabase()
{
	KWMTDatabaseTextFile::PhysicalDeleteDatabase();
}

longint PLMTDatabaseTextFile::ComputeBufferNecessaryMemory(boolean bRead, int nBufferSize) const
{
	longint lNecessaryMemory;
	int i;
	KWMTDatabaseMapping* mapping;

	require(nBufferSize >= 0);

	// On appelle la methode suivante pour forcer un rafraichissements des mapping utilises
	GetUsedMappings();

	// Parcours des mapping utilises pour prendre en compte les tailles des buffers
	lNecessaryMemory = 0;
	for (i = 0; i < oaUsedMappings.GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, oaUsedMappings.GetAt(i));
		assert(mapping == NULL or mapping == oaMultiTableMappings.GetAt(i));

		// Test si mapping utilise
		if (mapping != NULL)
		{
			// Mise a jour de la de memoire necessaire, en tenant compte des tailles de fichier
			lNecessaryMemory += PLDataTableDriverTextFile::ComputeBufferNecessaryMemory(
			    bRead, nBufferSize, lvFileSizes.GetAt(i));
		}
	}
	return lNecessaryMemory;
}

KWDataTableDriver* PLMTDatabaseTextFile::CreateDataTableDriver(KWMTDatabaseMapping* mapping) const
{
	KWDataTableDriverTextFile dataTableDriverTextFileCreator;
	PLDataTableDriverTextFile* dataTableDriver;
	KWLoadIndexVector* livDataItemLoadIndexes;
	IntVector* ivRootKeyIndexes;
	int i;
	KWMTDatabaseMapping* usedMapping;
	int nLastReadKeySize;

	require(mapping != NULL);

	// Cas d'une table reference, geree par des fichiers
	if (IsReferencedClassMapping(mapping))
	{
		dataTableDriverTextFileCreator.SetHeaderLineUsed(GetHeaderLineUsed());
		dataTableDriverTextFileCreator.SetFieldSeparator(GetFieldSeparator());
		return dataTableDriverTextFileCreator.Clone();
	}
	// Cas d'une table interne, geree en memoire
	else
	{
		// Recherche des index des attributs s'ils sont specifies
		livDataItemLoadIndexes = NULL;
		ivRootKeyIndexes = NULL;
		assert(oaIndexedMappingsDataItemLoadIndexes.GetSize() == 0 or
		       oaIndexedMappingsDataItemLoadIndexes.GetSize() == oaMultiTableMappings.GetSize());
		for (i = 0; i < oaIndexedMappingsDataItemLoadIndexes.GetSize(); i++)
		{
			usedMapping = cast(KWMTDatabaseMapping*, oaMultiTableMappings.GetAt(i));
			if (usedMapping == mapping)
			{
				livDataItemLoadIndexes =
				    cast(KWLoadIndexVector*, oaIndexedMappingsDataItemLoadIndexes.GetAt(i));
				ivRootKeyIndexes = cast(IntVector*, oaIndexedMappingsRootKeyIndexes.GetAt(i));
				break;
			}
		}

		// Creation du driver parallele
		dataTableDriver = cast(PLDataTableDriverTextFile*, dataTableDriverCreator->Clone());
		if (livDataItemLoadIndexes != NULL)
		{
			// Initialisation des index des attributs, et des attribut de cle
			dataTableDriver->GetDataItemLoadIndexes()->CopyFrom(livDataItemLoadIndexes);
			dataTableDriver->GetRootKeyIndexes()->CopyFrom(ivRootKeyIndexes);

			// Calcul du nombre de champs de la cle, puis initilisation du driver pour cette taille de cle
			nLastReadKeySize = 0;
			for (i = 0; i < ivRootKeyIndexes->GetSize(); i++)
			{
				if (ivRootKeyIndexes->GetAt(i) >= 0)
					nLastReadKeySize++;
			}
			dataTableDriver->InitializeLastReadKeySize(nLastReadKeySize);
		}
		return dataTableDriver;
	}
}

///////////////////////////////////////////////////////////////////////
// Implementation de la classe PLShared_MTDatabaseTextFile

PLShared_MTDatabaseTextFile::PLShared_MTDatabaseTextFile() {}

PLShared_MTDatabaseTextFile::~PLShared_MTDatabaseTextFile() {}

void PLShared_MTDatabaseTextFile::SetDatabase(PLMTDatabaseTextFile* database)
{
	require(database != NULL);
	SetObject(database);
}

PLMTDatabaseTextFile* PLShared_MTDatabaseTextFile::GetDatabase()
{
	return cast(PLMTDatabaseTextFile*, GetObject());
}

void PLShared_MTDatabaseTextFile::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	PLMTDatabaseTextFile* database;
	int i;
	KWMTDatabaseMapping* mapping;
	IntVector ivUsedMappingFlags;
	PLShared_ObjectArray shared_oaIndexedMappingsDataItemLoadIndexes(new PLShared_LoadIndexVector);
	PLShared_ObjectArray shared_oaIndexedMappingsRootKeyIndexes(new PLShared_IntVector);

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());

	// Acces a la base
	database = cast(PLMTDatabaseTextFile*, o);
	assert(database->CheckPartially(true));
	assert(database->oaMultiTableMappings.GetSize() == database->oaUsedMappings.GetSize());

	// Ecriture des parametres de specification de la base
	serializer->PutString(database->GetDatabaseName());
	serializer->PutString(database->GetClassName());
	serializer->PutInt(database->GetSampleNumberPercentage());
	serializer->PutBoolean(database->GetModeExcludeSample());
	serializer->PutString(database->GetSelectionAttribute());
	serializer->PutString(database->GetSelectionValue());
	serializer->PutIntVector(database->GetMarkedInstances());
	serializer->PutBoolean(database->GetVerboseMode());
	serializer->PutBoolean(database->GetSilentMode());
	serializer->PutBoolean(database->GetHeaderLineUsed());
	serializer->PutChar(database->GetFieldSeparator());

	// Ecriture des parametres specifiques au driver
	serializer->PutInt(database->GetBufferSize());

	// Ecriture des mappings (pour permettre leur reconstruction)
	serializer->PutInt(database->GetMultiTableMappings()->GetSize());
	for (i = 0; i < database->GetMultiTableMappings()->GetSize(); i++)
	{
		mapping = cast(KWMTDatabaseMapping*, database->GetMultiTableMappings()->GetAt(i));

		// Serialisation des informations de mapping
		serializer->PutString(mapping->GetDataPathClassName());
		serializer->PutString(mapping->GetDataPathAttributeNames());
		serializer->PutString(mapping->GetDataTableName());
	}

	// Memorisation dans un vecteur d'indicateurs des mappings utilises
	ivUsedMappingFlags.SetSize(database->oaUsedMappings.GetSize());
	for (i = 0; i < database->oaUsedMappings.GetSize(); i++)
	{
		if (database->oaUsedMappings.GetAt(i) != NULL)
			ivUsedMappingFlags.SetAt(i, 1);
	}

	// Ecriture du vecteur d'indicateur des mappings utilises
	serializer->PutIntVector(&ivUsedMappingFlags);

	// Ecriture des informations d'ouverture de la base
	serializer->PutLongintVector(&database->lvFileSizes);
	serializer->PutLongint(database->lTotalFileSize);
	serializer->PutLongint(database->lTotalUsedFileSize);
	serializer->PutLongint(database->lOutputNecessaryDiskSpace);
	serializer->PutLongint(database->lEmptyOpenNecessaryMemory);
	serializer->PutLongint(database->lMinOpenNecessaryMemory);
	serializer->PutLongint(database->lMaxOpenNecessaryMemory);

	// Ecriture des index des attributs par mapping
	assert(database->oaIndexedMappingsDataItemLoadIndexes.GetSize() == 0 or
	       database->oaIndexedMappingsDataItemLoadIndexes.GetSize() ==
		   database->GetMultiTableMappings()->GetSize());
	shared_oaIndexedMappingsDataItemLoadIndexes.SerializeObject(serializer,
								    &database->oaIndexedMappingsDataItemLoadIndexes);

	// Ecriture des index des champs de la cle par mapping
	assert(database->oaIndexedMappingsRootKeyIndexes.GetSize() == 0 or
	       database->oaIndexedMappingsRootKeyIndexes.GetSize() == database->GetMultiTableMappings()->GetSize());
	shared_oaIndexedMappingsRootKeyIndexes.SerializeObject(serializer, &database->oaIndexedMappingsRootKeyIndexes);
}

void PLShared_MTDatabaseTextFile::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	PLMTDatabaseTextFile* database;
	int nMappingNumber;
	int i;
	IntVector ivUsedMappingFlags;
	KWMTDatabaseMapping* mapping;
	PLShared_ObjectArray shared_oaIndexedMappingsDataItemLoadIndexes(new PLShared_LoadIndexVector);
	PLShared_ObjectArray shared_oaIndexedMappingsRootKeyIndexes(new PLShared_IntVector);

	require(serializer != NULL);
	require(serializer->IsOpenForRead());

	database = cast(PLMTDatabaseTextFile*, o);

	// Lecture des parametres de specification de la base
	database->SetDatabaseName(serializer->GetString());
	database->SetClassName(serializer->GetString());
	database->SetSampleNumberPercentage(serializer->GetInt());
	database->SetModeExcludeSample(serializer->GetBoolean());
	database->SetSelectionAttribute(serializer->GetString());
	database->SetSelectionValue(serializer->GetString());
	serializer->GetIntVector(database->GetMarkedInstances());
	database->SetVerboseMode(serializer->GetBoolean());
	database->SetSilentMode(serializer->GetBoolean());
	database->SetHeaderLineUsed(serializer->GetBoolean());
	database->SetFieldSeparator(serializer->GetChar());

	// Lecture des parametres specifiques au driver
	database->SetBufferSize(serializer->GetInt());

	// Lecture des mappings serialises
	nMappingNumber = serializer->GetInt();
	assert(database->oaMultiTableMappings.GetSize() == 1);
	for (i = 0; i < nMappingNumber; i++)
	{
		// Le premier mapping est pre-existant (table racine)
		if (i == 0)
			mapping = database->rootMultiTableMapping;
		// Les autre sont a creer
		else
		{
			mapping = new KWMTDatabaseMapping;
			database->oaMultiTableMappings.Add(mapping);
		}

		// Deserialisation du mapping
		mapping->SetDataPathClassName(serializer->GetString());
		mapping->SetDataPathAttributeNames(serializer->GetString());
		mapping->SetDataTableName(serializer->GetString());
	}

	// Lecture du vecteur d'indicateur des mappings utilises
	serializer->GetIntVector(&ivUsedMappingFlags);
	assert(database->oaMultiTableMappings.GetSize() == ivUsedMappingFlags.GetSize());

	// Reconstitution du tableau des mappings utuilises
	database->oaUsedMappings.SetSize(ivUsedMappingFlags.GetSize());
	for (i = 0; i < database->oaUsedMappings.GetSize(); i++)
	{
		if (ivUsedMappingFlags.GetAt(i) == 1)
			database->oaUsedMappings.SetAt(i, database->oaMultiTableMappings.GetAt(i));
	}

	// Lecture des informations d'ouverture de la base
	serializer->GetLongintVector(&database->lvFileSizes);
	database->lTotalFileSize = serializer->GetLongint();
	database->lTotalUsedFileSize = serializer->GetLongint();
	database->lOutputNecessaryDiskSpace = serializer->GetLongint();
	database->lEmptyOpenNecessaryMemory = serializer->GetLongint();
	database->lMinOpenNecessaryMemory = serializer->GetLongint();
	database->lMaxOpenNecessaryMemory = serializer->GetLongint();

	// Lecture des index des attributs par mapping
	shared_oaIndexedMappingsDataItemLoadIndexes.DeserializeObject(serializer,
								      &database->oaIndexedMappingsDataItemLoadIndexes);

	// Lecture des index des attributs de cle par mapping
	shared_oaIndexedMappingsRootKeyIndexes.DeserializeObject(serializer,
								 &database->oaIndexedMappingsRootKeyIndexes);
}

inline Object* PLShared_MTDatabaseTextFile::Create() const
{
	return new PLMTDatabaseTextFile;
}