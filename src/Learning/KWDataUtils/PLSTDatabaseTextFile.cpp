// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "PLSTDatabaseTextFile.h"

PLSTDatabaseTextFile::PLSTDatabaseTextFile()
{
	lTotalFileSize = 0;
	nDatabasePreferredBufferSize = 0;
	lInMemoryEstimatedFileObjectNumber = 0;
	lOutputNecessaryDiskSpace = 0;
	lEmptyOpenNecessaryMemory = 0;
	lMinOpenNecessaryMemory = 0;
	lMaxOpenNecessaryMemory = 0;
	nReadSizeMin = 0;
	nReadSizeMax = 0;

	// Changement de driver : on prend le driver de table parallele
	KWDataTableDriver* parallelDataTableDriver = new PLDataTableDriverTextFile;
	parallelDataTableDriver->CopyFrom(dataTableDriverCreator);
	delete dataTableDriverCreator;
	dataTableDriverCreator = parallelDataTableDriver;
}

PLSTDatabaseTextFile::~PLSTDatabaseTextFile() {}

void PLSTDatabaseTextFile::Reset()
{
	kwcHeaderLineClass.SetName("");
	kwcHeaderLineClass.DeleteAllAttributes();
	lTotalFileSize = 0;
	CleanOpenInformation();
}

boolean PLSTDatabaseTextFile::ComputeOpenInformation(boolean bRead, boolean bIncludingClassMemory,
						     PLSTDatabaseTextFile* outputDatabaseTextFile)
{
	boolean bOk = true;
	const boolean bTrace = GetPreparationTraceMode();
	PLDataTableDriverTextFile* driver;
	boolean bCurrentVerboseMode;
	KWClass* kwcUsedHeaderLineClass;
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
		driver = cast(PLDataTableDriverTextFile*, dataTableDriverCreator);
		assert(driver->GetDataTableName() == "");
		assert(driver->GetClass() == NULL);
		driver->SetDataTableName(sDatabaseName);
		driver->SetClass(kwcPhysicalClass);

		// Creation d'une classe fictive basee sur l'analyse de la premiere ligne du fichier, dans le cas d'une
		// ligne d'entete
		kwcUsedHeaderLineClass = NULL;
		if (GetHeaderLineUsed())
		{
			// Calcul bufferise pour minimiser les acces au fichier
			if (kwcHeaderLineClass.GetName() == "")
			{
				// On passe temporairement par un mode verbeux, car ce calcul bufferise ne sera effectue
				// qu'une seule fois
				bCurrentVerboseMode = dataTableDriverCreator->GetVerboseMode();
				dataTableDriverCreator->SetVerboseMode(true);

				// Calcul d'une classe representant le header a partir du fichier
				kwcHeaderLineClass.SetName(
				    kwcClass->GetDomain()->BuildClassName(GetClassName() + "HeaderLine"));
				bOk = driver->BuildDataTableClass(&kwcHeaderLineClass);
				assert(bOk or kwcHeaderLineClass.GetName() == "");

				// Restitution du mode initial
				dataTableDriverCreator->SetVerboseMode(bCurrentVerboseMode);
			}
			kwcUsedHeaderLineClass = &kwcHeaderLineClass;
		}

		// Calcul des index
		bOk = bOk and driver->ComputeDataItemLoadIndexes(kwcClass, kwcUsedHeaderLineClass);

		// Calcul de la taille du fichier si non deja calcule ou si erreur
		if (bRead and lTotalFileSize == 0)
			lTotalFileSize = PLRemoteFileService::GetFileSize(sDatabaseName);

		// Memorisation de l'estimation du nombre d'objet du fichier
		lInMemoryEstimatedFileObjectNumber = driver->GetInMemoryEstimatedObjectNumber(lTotalFileSize);

		// Calcul de la taille memoire en sortie
		if (outputDatabaseTextFile != NULL)
		{
			lOutputNecessaryDiskSpace =
			    driver->ComputeNecessaryDiskSpaceForFullWrite(kwcClass, lTotalFileSize);
		}

		// Calcul des informations necessaires pour la DatabaseMemoryGuard
		ComputeMemoryGuardOpenInformation();

		// Nettoyage
		driver->SetClass(NULL);
		driver->SetDataTableName("");
	}

	// Correction de la place disque necessaire en sortie en fonction du pourcentage d'exemples a lire dans la base
	// d'entree
	if (GetModeExcludeSample())
		lOutputNecessaryDiskSpace =
		    (longint)(lOutputNecessaryDiskSpace * (100 - GetSampleNumberPercentage()) / 100);
	else
		lOutputNecessaryDiskSpace = (longint)(lOutputNecessaryDiskSpace * GetSampleNumberPercentage() / 100);

	// S'il y a un critere de selection, on ne peut prevoir la place necessaire
	// On se place alors de facon heuristique sur 5% (pour ne pas sur-estimer cette place et interdire la tache)
	// De toutes facon, il faudra que la tache suive regulierement la place restante en ecriture
	if (GetSelectionAttribute() != "")
		lOutputNecessaryDiskSpace /= 20;

	// Exigences sur la taille du buffer de lecture
	nDatabasePreferredBufferSize = PLRemoteFileService::GetPreferredBufferSize(sDatabaseName);
	nReadSizeMin = SystemFile::nMinPreferredBufferSize;
	nReadSizeMax =
	    (SystemFile::nMaxPreferredBufferSize / nDatabasePreferredBufferSize) * nDatabasePreferredBufferSize;
	nReadSizeMax = max(nReadSizeMax, nReadSizeMin);

	// En mono-table, on prevoie la presence systematique d'un cache
	// en demandant en plus une taille preferee pour le min et le max
	if (bRead)
	{
		nReadSizeMin += nDatabasePreferredBufferSize;
		nReadSizeMax += nDatabasePreferredBufferSize;
	}

	// En mode lecture, on limite la taille des buffer par la taille du fichier en entree
	// (lTotalFileSize est la taille du fichier en mono-table)
	if (bRead)
	{
		if (nReadSizeMin > lTotalFileSize)
			nReadSizeMin = (int)lTotalFileSize;
		if (nReadSizeMax > lTotalFileSize)
			nReadSizeMax = (int)lTotalFileSize;
	}

	// Estimation de la memoire minimale et maximale necessaire pour ouvrir la base avec un buffer vide
	if (bOk)
	{
		nReferenceBufferSize = GetBufferSize();
		SetBufferSize(0);
		lEmptyOpenNecessaryMemory = ComputeOpenNecessaryMemory(bRead, bIncludingClassMemory);
		SetBufferSize(nReadSizeMin);
		lMinOpenNecessaryMemory = ComputeOpenNecessaryMemory(bRead, bIncludingClassMemory);
		SetBufferSize(nReadSizeMax);
		lMaxOpenNecessaryMemory = ComputeOpenNecessaryMemory(bRead, bIncludingClassMemory);
		SetBufferSize(nReferenceBufferSize);
	}

	// La memoire precedente est estimee principalement en fonction de la taille occupee par le dictionnaire
	// qui peut etre largement sous-estimee dans certains cas (difference d'occupation memoire selon que le
	// dictionnaire vient d'etre charge en memoire, est optimise, compile...)
	// On corrige cette sous-estimation en prenant en compte en plus deux fois les dictionnaires
	lDatabaseClassNecessaryMemory = 0;
	if (bOk)
	{
		lDatabaseClassNecessaryMemory =
		    2 * KWDatabase::ComputeOpenNecessaryMemory(bRead, bIncludingClassMemory);
		lEmptyOpenNecessaryMemory += lDatabaseClassNecessaryMemory;
		lMinOpenNecessaryMemory += lDatabaseClassNecessaryMemory;
		lMaxOpenNecessaryMemory += lEmptyOpenNecessaryMemory * 2;
	}

	// Mise a jour des min et max de memoire necessaire, en tenant compte du DatabaseMemoryGuard
	if (bOk)
	{
		lMinOpenNecessaryMemory += GetMemoryGuard()->GetEstimatedMinMemoryLimit();
		lMaxOpenNecessaryMemory += GetMemoryGuard()->GetEstimatedMaxMemoryLimit();
	}

	// Affichage
	if (bTrace)
	{
		cout << "PLSTDatabaseTextFile::ComputeOpenInformation " << bRead << "\n";
		cout << "\tDatabaseClassNecessaryMemory\t"
		     << LongintToHumanReadableString(lDatabaseClassNecessaryMemory) << "\n";
		cout << "\tEmptyOpenNecessaryMemory\t" << LongintToHumanReadableString(lEmptyOpenNecessaryMemory)
		     << "\n";
		cout << "\tMinOpenBufferSize\t" << LongintToHumanReadableString(nReadSizeMin) << "\n";
		cout << "\tMinOpenNecessaryMemory\t" << LongintToHumanReadableString(lMinOpenNecessaryMemory) << "\n";
		cout << "\tMaxOpenBufferSize\t" << LongintToHumanReadableString(nReadSizeMax) << "\n";
		cout << "\tMaxOpenNecessaryMemory\t" << LongintToHumanReadableString(lMaxOpenNecessaryMemory) << "\n";
		GetMemoryGuard()->WriteParameters(cout);
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
	nDatabasePreferredBufferSize = 0;
	lInMemoryEstimatedFileObjectNumber = 0;
	lOutputNecessaryDiskSpace = 0;
	lEmptyOpenNecessaryMemory = 0;
	lMinOpenNecessaryMemory = 0;
	lMaxOpenNecessaryMemory = 0;
	nReadSizeMin = 0;
	nReadSizeMax = 0;
	GetMemoryGuard()->Reset();
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

longint PLSTDatabaseTextFile::GetTotalFileSize() const
{
	require(IsOpenInformationComputed());
	return lTotalFileSize;
}

longint PLSTDatabaseTextFile::GetTotalUsedFileSize() const
{
	require(IsOpenInformationComputed());
	return lTotalFileSize;
}

int PLSTDatabaseTextFile::GetDatabasePreferredBuferSize() const
{
	require(IsOpenInformationComputed());
	return nDatabasePreferredBufferSize;
}

longint PLSTDatabaseTextFile::GetInMemoryEstimatedFileObjectNumber() const
{
	require(IsOpenInformationComputed());
	return lInMemoryEstimatedFileObjectNumber;
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
	return (int)ceil((lTotalFileSize + 1.0) / (nReadSizeMin + 1));
}

int PLSTDatabaseTextFile::ComputeOpenBufferSize(boolean bRead, longint lOpenGrantedMemory, int nProcessNumber) const
{
	boolean bDisplay = false;
	longint lOpenGrantedMemoryForBuffers;
	int nBufferSize;

	require(IsOpenInformationComputed());
	require(lMinOpenNecessaryMemory <= lOpenGrantedMemory);
	require(lOpenGrantedMemory <= lMaxOpenNecessaryMemory);
	require(lOpenGrantedMemory - lEmptyOpenNecessaryMemory + SystemFile::nMaxPreferredBufferSize);
	require(nProcessNumber >= 1);

	// On calcul la taille du buffer en fonction de la memoire alouee
	if (lOpenGrantedMemory == lMinOpenNecessaryMemory)
		nBufferSize = nReadSizeMin;
	else if (lOpenGrantedMemory == lMaxOpenNecessaryMemory)
		nBufferSize = nReadSizeMax;
	// Cas d'un dimensionnement intermediaire
	else
	{
		// Calcul de la partie de la memoire pour les buffers
		lOpenGrantedMemoryForBuffers =
		    lOpenGrantedMemory - ComputeEstimatedSingleInstanceMemoryLimit(lOpenGrantedMemory);
		assert(nReadSizeMin <= lOpenGrantedMemoryForBuffers and lOpenGrantedMemoryForBuffers <= nReadSizeMax);

		// Calcul de la taille au prorata de la memoire disponible
		nBufferSize = nReadSizeMin + (int)floor(((nReadSizeMax - nReadSizeMin) * 1.0 *
							 (lOpenGrantedMemoryForBuffers - lMinOpenNecessaryMemory)) /
							(lMaxOpenNecessaryMemory - lMinOpenNecessaryMemory));
	}

	// En lecture, chaque esclave doit lire au moins 5 buffer (pour que le travail soit bien reparti entre les
	// esclaves)
	if (bRead)
	{
		if (lTotalFileSize / (nProcessNumber * (longint)5) < nBufferSize)
			nBufferSize = int(lTotalFileSize / (nProcessNumber * (longint)5));
		nBufferSize = InputBufferedFile::FitBufferSize(nBufferSize);
	}

	// Arrondi a un multiple de preferredBufferSize du buffer de lecture
	assert(nDatabasePreferredBufferSize == PLRemoteFileService::GetPreferredBufferSize(sDatabaseName));
	if (nBufferSize > nDatabasePreferredBufferSize)
		nBufferSize = (nBufferSize / nDatabasePreferredBufferSize) * nDatabasePreferredBufferSize;

	// Projection sur les exigences min et max
	nBufferSize = max(nBufferSize, nReadSizeMin);
	nBufferSize = min(nBufferSize, nReadSizeMax);

	// Affichage
	if (bDisplay)
	{
		cout << "PLSTDatabaseTextFile::ComputeOpenBufferSize " << bRead << endl;
		cout << "\tMinOpenNecessaryMemory\t" << lMinOpenNecessaryMemory << endl;
		cout << "\tMaxOpenNecessaryMemory\t" << lMaxOpenNecessaryMemory << endl;
		cout << "\tOpenGrantedMemory\t" << lOpenGrantedMemory << endl;
		cout << "\tBufferSize\t" << nBufferSize << endl;
		cout << "\tTotalFileSize\t" << lTotalFileSize << endl;
	}

	assert(SystemFile::nMinPreferredBufferSize <= nBufferSize or
	       lTotalFileSize < SystemFile::nMinPreferredBufferSize);
	ensure(nBufferSize > 0 or lTotalFileSize == 0);
	return nBufferSize;
}

longint PLSTDatabaseTextFile::ComputeEstimatedSingleInstanceMemoryLimit(longint lOpenGrantedMemory) const
{
	longint lEstimatedSingleInstanceMemoryLimit;
	double dPercentage;

	require(IsOpenInformationComputed());
	require(lMinOpenNecessaryMemory <= lOpenGrantedMemory);
	require(lOpenGrantedMemory <= lMaxOpenNecessaryMemory);

	// Cas ou on a alloue le min
	if (lOpenGrantedMemory == lMinOpenNecessaryMemory)
		lEstimatedSingleInstanceMemoryLimit = GetConstMemoryGuard()->GetEstimatedMinMemoryLimit();
	// Cas ou on a alloue le min
	else if (lOpenGrantedMemory == lMaxOpenNecessaryMemory)
		lEstimatedSingleInstanceMemoryLimit = GetConstMemoryGuard()->GetEstimatedMaxMemoryLimit();
	// Cas intermediaire
	else
	{
		// Calcul selon une regle de 3
		dPercentage = (lOpenGrantedMemory - lMinOpenNecessaryMemory) * 1.0 /
			      (lMaxOpenNecessaryMemory - lMinOpenNecessaryMemory);
		lEstimatedSingleInstanceMemoryLimit =
		    GetConstMemoryGuard()->GetEstimatedMinMemoryLimit() +
		    longint(dPercentage * (GetConstMemoryGuard()->GetEstimatedMaxMemoryLimit() -
					   GetConstMemoryGuard()->GetEstimatedMinMemoryLimit()));
	}
	ensure(GetConstMemoryGuard()->GetEstimatedMinMemoryLimit() <= lEstimatedSingleInstanceMemoryLimit and
	       lEstimatedSingleInstanceMemoryLimit <= GetConstMemoryGuard()->GetEstimatedMaxMemoryLimit());
	ensure(lEstimatedSingleInstanceMemoryLimit <= lOpenGrantedMemory);
	return lEstimatedSingleInstanceMemoryLimit;
}

void PLSTDatabaseTextFile::SetBufferSize(int nSize)
{
	cast(PLDataTableDriverTextFile*, dataTableDriverCreator)->SetBufferSize(nSize);
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

void PLSTDatabaseTextFile::ComputeMemoryGuardOpenInformation()
{
	const boolean bTrace = false;
	PLDataTableDriverTextFile* driver;
	longint lUsedMemoryPerObject;
	longint lEstimatedMemoryForSingleObjectCreation;
	longint lEstimatedMemoryForMultipleObjectCreation;
	longint lEstimatedTotalCreatedSingleObjectNumber;
	longint lEstimatedTotalCreatedMultipleObjectNumber;
	longint lUsedMemoryForTableSelection;
	longint lTotalMaxCreatedRecordNumber;
	longint lEstimatedMinSingleInstanceMemoryLimit;
	longint lEstimatedMaxSingleInstanceMemoryLimit;
	const KWDataPath* dataPath;

	// Initialisation des resultats
	GetMemoryGuard()->Reset();
	lTotalMaxCreatedRecordNumber = 0;
	lEstimatedMinSingleInstanceMemoryLimit = 0;
	lEstimatedMaxSingleInstanceMemoryLimit = 0;

	// Initialisation de tous les data paths a destination des objets lus ou cree
	// pour le dimensionnement des objets cree
	assert(objectDataPathManager.GetDataPathNumber() == 0);
	objectDataPathManager.ComputeAllDataPaths(kwcPhysicalClass);

	// Acces au driver
	driver = cast(PLDataTableDriverTextFile*, GetDataTableDriver());
	assert(driver != NULL);

	// Stats sur les records
	lUsedMemoryPerObject = driver->GetClass()->GetEstimatedUsedMemoryPerObject();

	// Cas de la table principale
	lEstimatedMinSingleInstanceMemoryLimit = lUsedMemoryPerObject;
	lEstimatedMaxSingleInstanceMemoryLimit = lUsedMemoryPerObject;

	// Estimation des stats sur les objets crees dans la hierarchie
	dataPath = objectDataPathManager.GetMainDataPath();
	dataPath->ComputeEstimatedMemoryForObjectCreation(
	    driver->GetClass()->GetDomain(), lEstimatedMemoryForSingleObjectCreation,
	    lEstimatedMemoryForMultipleObjectCreation, lEstimatedTotalCreatedSingleObjectNumber,
	    lEstimatedTotalCreatedMultipleObjectNumber);

	// Prise en compte de la memoire pour les objets crees
	lTotalMaxCreatedRecordNumber += lEstimatedTotalCreatedSingleObjectNumber +
					lEstimatedTotalCreatedMultipleObjectNumber *
					    KWDatabaseMemoryGuard::GetDefautMaxSecondaryRecordNumberFactor();
	lEstimatedMinSingleInstanceMemoryLimit += lEstimatedMemoryForSingleObjectCreation +
						  lEstimatedMemoryForMultipleObjectCreation *
						      KWDatabaseMemoryGuard::GetDefautMinSecondaryRecordNumberFactor();
	lEstimatedMaxSingleInstanceMemoryLimit += lEstimatedMemoryForSingleObjectCreation +
						  lEstimatedMemoryForMultipleObjectCreation *
						      KWDatabaseMemoryGuard::GetDefautMaxSecondaryRecordNumberFactor();

	// Prise en compte pour le dimensionnement, en prenant un peu de marge pour les variables
	// secondaires memorisant des selections
	lUsedMemoryForTableSelection =
	    sizeof(KWObject*) + sizeof(KWValueIndexPair) + sizeof(ObjectArray*) + sizeof(ObjectArray);
	lEstimatedMinSingleInstanceMemoryLimit += (lUsedMemoryPerObject + lUsedMemoryForTableSelection);
	lEstimatedMaxSingleInstanceMemoryLimit += (lUsedMemoryPerObject + lUsedMemoryForTableSelection);

	// On utilise une limite minimale au nombre max de records secondaires ou crees,
	// pour de pas declencher de warning utilisateurs excessifs
	lTotalMaxCreatedRecordNumber =
	    max(lTotalMaxCreatedRecordNumber, KWDatabaseMemoryGuard::GetDefautMinSecondaryRecordNumberLowerBound());

	// On utilise une limite minimale a la memoire RAM, pour de pas declencher de warning utilisateurs excessifs
	lEstimatedMinSingleInstanceMemoryLimit += BufferedFile::nDefaultBufferSize / 8;
	lEstimatedMaxSingleInstanceMemoryLimit += 2 * BufferedFile::nDefaultBufferSize;

	// Destruction des data paths de gestion des objets
	objectDataPathManager.Reset();

	// Memorisation dans le memory guard
	GetMemoryGuard()->SetMaxSecondaryRecordNumber(0);
	GetMemoryGuard()->SetMaxCreatedRecordNumber(lTotalMaxCreatedRecordNumber);
	GetMemoryGuard()->SetEstimatedMinMemoryLimit(lEstimatedMinSingleInstanceMemoryLimit);
	GetMemoryGuard()->SetEstimatedMaxMemoryLimit(lEstimatedMaxSingleInstanceMemoryLimit);

	// Affichage
	if (bTrace)
	{
		cout << "ComputeMemoryGuardOpenInformation\t" << GetDatabaseName() << "\n";
		cout << "\tUsedMemoryPerObject\t" << lUsedMemoryPerObject << "\n";
		cout << "\tMemoryForSingleObjectCreation\t" << lEstimatedMemoryForSingleObjectCreation << "\n";
		cout << "\tMemoryForMultipleObjectCreation\t" << lEstimatedMemoryForMultipleObjectCreation << "\n";
		cout << "\tTotalCreatedSingleObjectNumber\t" << lEstimatedTotalCreatedSingleObjectNumber << "\n";
		cout << "\tTotalCreatedMultipleObjectNumber\t" << lEstimatedTotalCreatedMultipleObjectNumber << "\n";
		GetMemoryGuard()->WriteParameters(cout);
	}
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
	serializer->PutDouble(database->GetSampleNumberPercentage());
	serializer->PutBoolean(database->GetModeExcludeSample());
	serializer->PutString(database->GetSelectionAttribute());
	serializer->PutString(database->GetSelectionValue());
	serializer->PutIntVector(database->GetMarkedInstances());
	serializer->PutBoolean(database->GetDenseOutputFormat());
	serializer->PutBoolean(database->GetVerboseMode());
	serializer->PutBoolean(database->GetSilentMode());
	serializer->PutBoolean(database->GetHeaderLineUsed());
	serializer->PutChar(database->GetFieldSeparator());

	// Ecriture des informations d'ouverture de la base
	serializer->PutLongint(database->lTotalFileSize);
	serializer->PutInt(database->nDatabasePreferredBufferSize);
	serializer->PutLongint(database->lInMemoryEstimatedFileObjectNumber);
	serializer->PutLongint(database->lOutputNecessaryDiskSpace);
	serializer->PutLongint(database->lEmptyOpenNecessaryMemory);
	serializer->PutLongint(database->lMinOpenNecessaryMemory);
	serializer->PutLongint(database->lMaxOpenNecessaryMemory);
	serializer->PutInt(database->nReadSizeMin);
	serializer->PutInt(database->nReadSizeMax);

	// Ecriture des parametres specifiques au driver
	shared_livDataItemLoadIndexes.SerializeObject(serializer, database->GetDataItemLoadIndexes());
	serializer->PutInt(database->GetBufferSize());

	// Ecriture des parametres du memory guard
	serializer->PutLongint(database->GetConstMemoryGuard()->GetMaxSecondaryRecordNumber());
	serializer->PutLongint(database->GetConstMemoryGuard()->GetMaxCreatedRecordNumber());
	serializer->PutLongint(database->GetConstMemoryGuard()->GetEstimatedMinMemoryLimit());
	serializer->PutLongint(database->GetConstMemoryGuard()->GetEstimatedMaxMemoryLimit());
	serializer->PutLongint(database->GetConstMemoryGuard()->GetMemoryLimit());
	serializer->PutLongint(KWDatabaseMemoryGuard::GetCrashTestMaxSecondaryRecordNumber());
	serializer->PutLongint(KWDatabaseMemoryGuard::GetCrashTestMaxCreatedRecordNumber());
	serializer->PutLongint(KWDatabaseMemoryGuard::GetCrashTestMemoryLimit());
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
	database->SetSampleNumberPercentage(serializer->GetDouble());
	database->SetModeExcludeSample(serializer->GetBoolean());
	database->SetSelectionAttribute(serializer->GetString());
	database->SetSelectionValue(serializer->GetString());
	serializer->GetIntVector(database->GetMarkedInstances());
	database->SetDenseOutputFormat(serializer->GetBoolean());
	database->SetVerboseMode(serializer->GetBoolean());
	database->SetSilentMode(serializer->GetBoolean());
	database->SetHeaderLineUsed(serializer->GetBoolean());
	database->SetFieldSeparator(serializer->GetChar());

	// Lecture des informations d'ouverture de la base
	database->lTotalFileSize = serializer->GetLongint();
	database->nDatabasePreferredBufferSize = serializer->GetInt();
	database->lInMemoryEstimatedFileObjectNumber = serializer->GetLongint();
	database->lOutputNecessaryDiskSpace = serializer->GetLongint();
	database->lEmptyOpenNecessaryMemory = serializer->GetLongint();
	database->lMinOpenNecessaryMemory = serializer->GetLongint();
	database->lMaxOpenNecessaryMemory = serializer->GetLongint();
	database->nReadSizeMin = serializer->GetInt();
	database->nReadSizeMax = serializer->GetInt();

	// Lecture des parametres specifiques au driver
	shared_livDataItemLoadIndexes.DeserializeObject(serializer, database->GetDataItemLoadIndexes());
	database->SetBufferSize(serializer->GetInt());

	// Lecture des parametres du memory guard
	database->GetMemoryGuard()->SetMaxSecondaryRecordNumber(serializer->GetLongint());
	database->GetMemoryGuard()->SetMaxCreatedRecordNumber(serializer->GetLongint());
	database->GetMemoryGuard()->SetEstimatedMinMemoryLimit(serializer->GetLongint());
	database->GetMemoryGuard()->SetEstimatedMaxMemoryLimit(serializer->GetLongint());
	database->GetMemoryGuard()->SetMemoryLimit(serializer->GetLongint());
	KWDatabaseMemoryGuard::SetCrashTestMaxSecondaryRecordNumber(serializer->GetLongint());
	KWDatabaseMemoryGuard::SetCrashTestMaxCreatedRecordNumber(serializer->GetLongint());
	KWDatabaseMemoryGuard::SetCrashTestMemoryLimit(serializer->GetLongint());
}

Object* PLShared_STDatabaseTextFile::Create() const
{
	return new PLSTDatabaseTextFile;
}
