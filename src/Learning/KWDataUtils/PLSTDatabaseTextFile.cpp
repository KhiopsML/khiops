// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLSTDatabaseTextFile.h"

PLSTDatabaseTextFile::PLSTDatabaseTextFile()
{
	lTotalInputFileSize = 0;
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

PLSTDatabaseTextFile::~PLSTDatabaseTextFile() {}

boolean PLSTDatabaseTextFile::ComputeOpenInformation(boolean bRead, boolean bIncludingClassMemory,
						     PLSTDatabaseTextFile* outputDatabaseTextFile)
{
	boolean bDisplay = false;
	boolean bOk = true;
	longint lDatabaseClassNecessaryMemory;
	int nReferenceBufferSize;

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

	// Calcul des index des champs en lecture et de la taille disque en sortie si necessaire
	if (bRead)
	{
		// Parametrage du driver
		assert(cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->GetDataTableName() == "");
		assert(cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->GetClass() == NULL);
		cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->SetDataTableName(sDatabaseName);
		cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->SetClass(kwcPhysicalClass);

		// Calcul des index
		bOk = cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->ComputeDataItemLoadIndexes(kwcClass);

		// Calcul de la taille du fichier
		lTotalInputFileSize = PLRemoteFileService::GetFileSize(sDatabaseName);

		// Calcul de la taille memoire en sortie
		if (outputDatabaseTextFile != NULL)
		{
			lOutputNecessaryDiskSpace = cast(PLDataTableDriverTextFile*, dataTableDriverCreator)
							->ComputeNecessaryDiskSpaceForFullWrite(kwcClass);
		}

		// Nettoyage
		cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->SetClass(NULL);
		cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->SetDataTableName("");
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

	// Estimation de la memoire minimale et maximale necessaire pour ouvrir la base ave un buffer vide
	nReferenceBufferSize = GetBufferSize();
	SetBufferSize(0);
	lEmptyOpenNecessaryMemory = ComputeOpenNecessaryMemory(bRead, bIncludingClassMemory);
	SetBufferSize(nMinOpenBufferSize);
	lMinOpenNecessaryMemory = ComputeOpenNecessaryMemory(bRead, bIncludingClassMemory);
	SetBufferSize(nMaxOpenBufferSize);
	lMaxOpenNecessaryMemory = ComputeOpenNecessaryMemory(bRead, bIncludingClassMemory);
	SetBufferSize(nReferenceBufferSize);

	// La memoire precedente est estimee principalement en fonction de la taille occupee par le dictionnaire
	// qui peut etre largement sous-estimee dans certains cas (difference d'occupation memoire selon que le
	// dictionnaire vient d'etre charge en memoire, est optimise, compile...)
	// On corrige cette sous-estimation en prenant en compte en plus deux fois les dictionnaires
	lDatabaseClassNecessaryMemory = 2 * KWDatabase::ComputeOpenNecessaryMemory(bRead, bIncludingClassMemory);
	lEmptyOpenNecessaryMemory += lDatabaseClassNecessaryMemory;
	lMinOpenNecessaryMemory += lDatabaseClassNecessaryMemory;
	lMaxOpenNecessaryMemory += lEmptyOpenNecessaryMemory * 2;

	// Affichage
	if (bDisplay)
	{
		cout << "PLSTDatabaseTextFile::ComputeOpenInformation " << bRead << endl;
		cout << "\tDatabaseClassNecessaryMemory\t"
		     << LongintToHumanReadableString(lDatabaseClassNecessaryMemory) << endl;
		cout << "\tEmptyOpenNecessaryMemory\t" << LongintToHumanReadableString(lEmptyOpenNecessaryMemory)
		     << endl;
		cout << "\tMinOpenBufferSize\t" << LongintToHumanReadableString(nMinOpenBufferSize) << endl;
		cout << "\tMinOpenNecessaryMemory\t" << LongintToHumanReadableString(lMinOpenNecessaryMemory) << endl;
		cout << "\tMaxOpenBufferSize\t" << LongintToHumanReadableString(nMaxOpenBufferSize) << endl;
		cout << "\tMaxOpenNecessaryMemory\t" << LongintToHumanReadableString(lMaxOpenNecessaryMemory) << endl;
	}

	// Nettoyage
	if (bRead)
		DeletePhysicalClass();
	kwcClass = NULL;

	// Nettoyage des resultat si KO
	if (not bOk)
		CleanOpenInformation();
	return bOk;
}

void PLSTDatabaseTextFile::CleanOpenInformation()
{
	lTotalInputFileSize = 0;
	lOutputNecessaryDiskSpace = 0;
	lEmptyOpenNecessaryMemory = 0;
	lMinOpenNecessaryMemory = 0;
	lMaxOpenNecessaryMemory = 0;
}

boolean PLSTDatabaseTextFile::IsOpenInformationComputed() const
{
	return lMinOpenNecessaryMemory > 0;
}

const KWLoadIndexVector* PLSTDatabaseTextFile::GetConstDataItemLoadIndexes() const
{
	require(IsOpenInformationComputed());
	return cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->GetConstDataItemLoadIndexes();
}

KWLoadIndexVector* PLSTDatabaseTextFile::GetDataItemLoadIndexes()
{
	require(IsOpenInformationComputed());
	return cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->GetDataItemLoadIndexes();
}

longint PLSTDatabaseTextFile::GetTotalInputFileSize() const
{
	require(IsOpenInformationComputed());
	return lTotalInputFileSize;
}

longint PLSTDatabaseTextFile::GetEmptyOpenNecessaryMemory() const
{
	require(IsOpenInformationComputed());
	return lEmptyOpenNecessaryMemory;
}

longint PLSTDatabaseTextFile::GetMinOpenNecessaryMemory() const
{
	require(IsOpenInformationComputed());
	return lMinOpenNecessaryMemory;
}

longint PLSTDatabaseTextFile::GetMaxOpenNecessaryMemory() const
{
	require(IsOpenInformationComputed());
	return lMaxOpenNecessaryMemory;
}

longint PLSTDatabaseTextFile::GetOutputNecessaryDiskSpace() const
{
	require(IsOpenInformationComputed());
	return lOutputNecessaryDiskSpace;
}

int PLSTDatabaseTextFile::GetMaxSlaveProcessNumber() const
{
	require(IsOpenInformationComputed());
	return (int)ceil((lTotalInputFileSize + 1.0) / nMinOpenBufferSize);
}

int PLSTDatabaseTextFile::ComputeOpenBufferSize(boolean bRead, longint lOpenGrantedMemory) const
{
	boolean bDisplay = false;
	longint lBufferSize;
	int nBufferSize;
	longint lChunkNumber;

	require(IsOpenInformationComputed());
	require(lMinOpenNecessaryMemory <= lOpenGrantedMemory);
	require(lOpenGrantedMemory <= lMaxOpenNecessaryMemory);
	require(lOpenGrantedMemory - lEmptyOpenNecessaryMemory + nMaxOpenBufferSize);

	// On calcul la taille du buffer en fonction de la memoire alouee
	if (lOpenGrantedMemory == lMinOpenNecessaryMemory)
		lBufferSize = nMinOpenBufferSize;
	else if (lOpenGrantedMemory == lMaxOpenNecessaryMemory)
		lBufferSize = nMaxOpenBufferSize;
	else
		lBufferSize = nMinOpenBufferSize + (longint)floor((nMaxOpenBufferSize - nMinOpenBufferSize) * 1.0 *
								  (lOpenGrantedMemory - lMinOpenNecessaryMemory) /
								  (lMaxOpenNecessaryMemory - lMinOpenNecessaryMemory));

	// Affichage
	if (bDisplay)
	{
		cout << "PLSTDatabaseTextFile::ComputeOpenBufferSize " << bRead << endl;
		cout << "\tMinOpenNecessaryMemory\t" << lMinOpenNecessaryMemory << endl;
		cout << "\tMaxOpenNecessaryMemory\t" << lMaxOpenNecessaryMemory << endl;
		cout << "\tOpenGrantedMemory\t" << lOpenGrantedMemory << endl;
		cout << "\tBufferSize\t" << lBufferSize << endl;
		cout << "\tTotalInputFileSize\t" << lTotalInputFileSize << endl;
	}

	// On ajuste si necessaire la taille du buffer pour eviter le cas d'un dernier buffer de lecture tres peu rempli
	if (bRead and lBufferSize <= lTotalInputFileSize)
	{
		// Retaillage si le dernier chunk est trop petit
		lChunkNumber = lTotalInputFileSize / lBufferSize;
		if (lTotalInputFileSize - lChunkNumber * lBufferSize < lBufferSize / 2)
		{
			// On calcul la taille du buffer avec un chunk supplemenatire
			lChunkNumber++;
			lBufferSize = 1 + longint(ceil(lTotalInputFileSize * 1.0) / lChunkNumber);

			// On ajuste a un nombre entier de segments superieur
			lBufferSize = MemSegmentByteSize * (longint)ceil(lBufferSize * 1.0 / MemSegmentByteSize);
		}
	}

	// Ajustement final pour que les buffer soient un nombre entier de segments
	lBufferSize = MemSegmentByteSize * (lBufferSize / MemSegmentByteSize);
	if (lBufferSize < nMinOpenBufferSize)
		lBufferSize = nMinOpenBufferSize;
	if (lBufferSize > nMaxOpenBufferSize)
		lBufferSize = nMaxOpenBufferSize;

	// Retaillage en lecture si on depasse la taille du fichier
	if (bRead and lBufferSize > lTotalInputFileSize)
		lBufferSize = lTotalInputFileSize + 1;

	// Passage en entier court
	assert((nMinOpenBufferSize <= lBufferSize and lBufferSize <= lMaxOpenNecessaryMemory) or
	       lTotalInputFileSize < nMinOpenBufferSize);
	assert(lBufferSize <= INT_MAX);
	nBufferSize = (int)lBufferSize;
	ensure(nBufferSize > 0);
	return nBufferSize;
}

void PLSTDatabaseTextFile::SetBufferSize(int nBufferSize)
{
	cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->SetBufferSize(nBufferSize);
}

int PLSTDatabaseTextFile::GetBufferSize() const
{
	return cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->GetBufferSize();
}

void PLSTDatabaseTextFile::SetOutputBuffer(OutputBufferedFile* buffer)
{
	cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->SetOutputBuffer(buffer);
}

void PLSTDatabaseTextFile::SetInputBuffer(InputBufferedFile* buffer)
{
	cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->SetInputBuffer(buffer);
}

OutputBufferedFile* PLSTDatabaseTextFile::GetOutputBuffer() const
{
	return cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->GetOutputBuffer();
}

InputBufferedFile* PLSTDatabaseTextFile::GetInputBuffer() const
{
	return cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->GetInputBuffer();
}

PLDataTableDriverTextFile* PLSTDatabaseTextFile::GetDriver()
{
	return cast(PLDataTableDriverTextFile*, dataTableDriverCreator);
}

void PLSTDatabaseTextFile::PhysicalDeleteDatabase()
{
	KWSTDatabaseTextFile::PhysicalDeleteDatabase();
}

///////////////////////////////////////////////////////////////////////
// Implementation de la classe PLShared_STDatabaseTextFile

PLShared_STDatabaseTextFile::PLShared_STDatabaseTextFile() {}

PLShared_STDatabaseTextFile::~PLShared_STDatabaseTextFile() {}

void PLShared_STDatabaseTextFile::SetDatabase(PLSTDatabaseTextFile* database)
{
	require(database != NULL);
	SetObject(database);
}

PLSTDatabaseTextFile* PLShared_STDatabaseTextFile::GetDatabase()
{
	return cast(PLSTDatabaseTextFile*, GetObject());
}

void PLShared_STDatabaseTextFile::SerializeObject(PLSerializer* serializer, const Object* o) const
{
	PLSTDatabaseTextFile* database;
	PLShared_LoadIndexVector shared_livDataItemLoadIndexes;

	require(serializer != NULL);
	require(serializer->IsOpenForWrite());

	database = cast(PLSTDatabaseTextFile*, o);

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

	// Ecriture des informations d'ouverture de la base
	serializer->PutLongint(database->lTotalInputFileSize);
	serializer->PutLongint(database->lOutputNecessaryDiskSpace);
	serializer->PutLongint(database->lEmptyOpenNecessaryMemory);
	serializer->PutLongint(database->lMinOpenNecessaryMemory);
	serializer->PutLongint(database->lMaxOpenNecessaryMemory);

	// Ecriture des parametres specifiques au driver
	shared_livDataItemLoadIndexes.SerializeObject(serializer, database->GetDataItemLoadIndexes());
	serializer->PutInt(database->GetBufferSize());
}

void PLShared_STDatabaseTextFile::DeserializeObject(PLSerializer* serializer, Object* o) const
{
	PLSTDatabaseTextFile* database;
	PLShared_LoadIndexVector shared_livDataItemLoadIndexes;

	require(serializer != NULL);
	require(serializer->IsOpenForRead());

	database = cast(PLSTDatabaseTextFile*, o);

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

	// Lecture des informations d'ouverture de la base
	database->lTotalInputFileSize = serializer->GetLongint();
	database->lOutputNecessaryDiskSpace = serializer->GetLongint();
	database->lEmptyOpenNecessaryMemory = serializer->GetLongint();
	database->lMinOpenNecessaryMemory = serializer->GetLongint();
	database->lMaxOpenNecessaryMemory = serializer->GetLongint();

	// Lecture des parametres specifiques au driver
	shared_livDataItemLoadIndexes.DeserializeObject(serializer, database->GetDataItemLoadIndexes());
	database->SetBufferSize(serializer->GetInt());
}

Object* PLShared_STDatabaseTextFile::Create() const
{
	return new PLSTDatabaseTextFile;
}